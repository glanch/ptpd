{ config, lib, pkgs, ... }:
let
  cfg = config.services.ptpd;
in
{
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

    openFirewall = lib.mkOption {
      type = lib.types.bool;
      default = true;
      example = true;
    };
  };


  config = lib.mkIf cfg.enable
    {
      environment.systemPackages = [ pkgs.ptpd ];
      services.timesyncd.enable = false;
      networking.firewall.allowedUDPPorts = if cfg.openFirewall then [ 319 320 ] else [];

      systemd.services = {
        ptpd = {
          enable = true;
          description = "Precision Time Protocol Daemon";
          after = [ "syslog.target" "ntpdate.service" "sntp.service" "ntp.service" "chronyd.service" "network.target" ];
          wantedBy = [ "multi-user.target" ];

          serviceConfig = {
            User = "root";
            Group = "root";
            WorkingDirectory = "/root";
            ExecStart = "${pkgs.ptpd}/bin/ptpd2 --interface ${cfg.interface} ${if cfg.slaveOnly then "-s" else ""} -V -C";
            Restart = "always";
            RestartSec = "5";
          };
        };



      };
    };
}
