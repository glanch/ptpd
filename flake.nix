{
  description = "An over-engineered Hello World in C";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-21.05";

  outputs = { self, nixpkgs }:
    let

      # Generate a user-friendly version number.
      version = builtins.substring 0 8 self.lastModifiedDate;

      # System types to support.
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlay ]; });

    in
    {

      # A Nixpkgs overlay.
      overlay = final: prev: {

        hello = with final; stdenv.mkDerivation rec {
          name = "ptpd-${version}";

          src = ./.;
          buildInputs = [ libpcap libbpf ];
          nativeBuildInputs = [ autoreconfHook libpcap libbpf ];
        };

      };

      # Provide some binary packages for selected system types.
      packages = forAllSystems (system:
        {
          inherit (nixpkgsFor.${system}) hello;
        });

      # The default package for 'nix build'. This makes sense if the
      # flake provides only one package or there is a clear "main"
      # package.
      defaultPackage = forAllSystems (system: self.packages.${system}.ptpd);

      # A NixOS module, if applicable (e.g. if the package provides a system service).
      nixosModules.ptpd =

        { config, lib, pkgs, ... }:
        let
          cfg = config.services.ptpd;
        in
        {
          nixpkgs.overlays = [ self.overlay ];
          options.services.ptpd.enable = lib.mkEnableOption "ptpd";

          options.services.ptpd = {
            interface = lib.mkOption {
              type = lib.types.str;
              default = "eth0";
              example = "ens19s";
            };

            slaveOnly = lib.mkOption {
              type = lib.types.bool;
              default = true;
              example = true;
            };

          };

          environment.systemPackages = [ pkgs.ptpd ];
          config = lib.mkIf cfg.enable
            {
              services.timesyncd.enable = false;

              systemd.services = {
                ptpd = {
                  enable = true;
                  description = "Precision Time Protocol Daemon";
                  after = [ "syslog.target" "ntpdate.service" "sntp.service" "ntp.service" "chronyd.service" "network.target" ];
                  serviceConfig = {
                    Type = "forking";
                    User = "root";
                  };
                };


                serviceConfig = {
                  User = "root";
                  Group = "root";
                  WorkingDirectory = "/root";
                  ExecStart = "${pkgs.ptpd}/bin/ptpd --interface ${cfg.interface} ${if cfg.slaveOnly then "-s" else ""} -V -C";
                  Restart = "always";
                  RestartSec = "5";
                };
              };
            };
          #systemd.services = { ... };
        };

      # Tests run by 'nix flake check' and by Hydra.
      # checks = forAllSystems
      #   (system:
      #     with nixpkgsFor.${system};

      #     {
      #       inherit (self.packages.${system}) hello;

      #       # Additional tests, if applicable.
      #       test = stdenv.mkDerivation {
      #         name = "hello-test-${version}";

      #         buildInputs = [ hello ];

      #         unpackPhase = "true";

      #         buildPhase = ''
      #           echo 'running some integration tests'
      #           [[ $(hello) = 'Hello Nixers!' ]]
      #         '';

      #         installPhase = "mkdir -p $out";
      #       };
      #     }

      #     // lib.optionalAttrs stdenv.isLinux {
      #       # A VM test of the NixOS module.
      #       vmTest =
      #         with import (nixpkgs + "/nixos/lib/testing-python.nix") {
      #           inherit system;
      #         };

      #         makeTest {
      #           nodes = {
      #             client = { ... }: {
      #               imports = [ self.nixosModules.ptpd ];
      #             };
      #           };

      #           testScript =
      #             ''
      #               start_all()
      #               client.wait_for_unit("multi-user.target")
      #               client.succeed("hello")
      #             '';
      #         };
      #     }
      #   );

    };
}
