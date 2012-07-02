/**
 * @file   ptpmanager.c
 * @date   Wed Many 29 11:40:38 2012
 * 
 * @brief  The code implements ptpmanager responsible for sending and 
 *         receiving management messages
 */

#include "constants.h"
#include "ptpmanager.h"

#define PTP_GENERAL_PORT 320
#define PTP_EVENT_PORT 319

NetPath * netPath;

/* Function to pack the header of outgoing message */
void 
packCommonHeader(Octet *buf)
{
	printf("Taking default values..\n");
	
	Nibble transport = 0x80;
	Nibble versionNumber = 0x02;
	UInteger8 domainNumber = 0x00;
	
	*(UInteger8 *) (buf + 0) = transport;
	/*Message Type = 13 */
	*(UInteger8 *) (buf + 0) = 	*(UInteger8 *) (buf + 0) | 0x0D;
	
	*(unsigned char *)(buf + 1) = *(unsigned char *)(buf + 1) & 0x00;
	*(UInteger4 *) (buf + 1) = *(unsigned char *)(buf + 1) | versionNumber;

	*(UInteger8 *) (buf + 4) = domainNumber;
	*(unsigned char *)(buf + 5) = 0x00;
	*(unsigned char *)(buf + 6) = 0x04; /*Table 20*/
	*(unsigned char *)(buf + 7) = 0x00; /*Table 20*/
	
	/*correction field to be zero for management messages*/
	memset((buf + 8), 0, 8);
	
	/*reserved2 to be 0*/
	memset((buf + 16), 0, 4);
	
	/* To be corrected: temporarily setting sourcePortIdentity to be zero*/
	memset((buf + 20), 0, 10);
	
	/* sequence id */
	*(UInteger16 *) (buf + 30) = flip16(out_sequence + 1);
	
	/*Table 23*/
	*(UInteger8 *) (buf + 32) = 0x04;	
	
	/*Table 24*/
	*(UInteger8 *) (buf + 33) = 0x7F;

}

/*Function to pack Management Header*/
void 
packManagementHeader(Octet *buf)
{
	/* targetPortIdentity to be zero for now*/
	memset((buf + 34), 0, 10);
	
	/* assuming that management message doesn't need to
	 * be retransmitted by boundary clocks
	 */  
	*(UInteger8 *) (buf + 44) = 0x00;
	*(UInteger8 *) (buf + 45) = 0x00;
	
	/* reserved fields to be zero
	 * and actionField to be set during packing tlv data
	 */
	*(UInteger8 *) (buf + 46) = 0x00;
	*(UInteger8 *) (buf + 47) = 0x00;

	out_length += MANAGEMENT_LENGTH;
}


Boolean 
packCommandOnly(Octet *buf, MsgManagement* manage)
{
	int actionField;
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	manage->tlv->lengthField = flip16(0x0002);
	
	printf("\n>actionField (3 for Command)?");
	scanf("%d",&actionField);
	switch(actionField){
	case COMMAND:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | COMMAND;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	out_length += TLV_LENGTH;
	return TRUE;
}

Boolean
packGETOnly(Octet *buf, MsgManagement* manage)
{
	int actionField;
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	manage->tlv->lengthField = flip16(0x0002);
	
	printf("\n>actionField (0 for GET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	out_length += TLV_LENGTH;
	return TRUE;
}

/*
 * Functions to pack payload for each type of managementId. 
 * These will ask questions from user and set the data fields accordingly.
 */
Boolean
packMMNullManagement(Octet *buf)
{
	int actionField;
	MsgManagement *manage = (MsgManagement*)(buf);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	*(UInteger16 *) (buf + 48) = flip16(TLV_MANAGEMENT);
	/* lengthField = 2+N*/
	*(UInteger16 *) (buf + 50) = flip16(0x0002);
	/*managementId Table 40*/
	*(UInteger16 *) (buf + 52) = flip16(MM_NULL_MANAGEMENT);
	
	printf("\n>actionField (0 for GET, 1 for SET, 3 for Command) ?");
	scanf("%d",&actionField);
	if (actionField == 0)
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
	else if (actionField == 1)
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
	else if (actionField == 3)
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | COMMAND;
	else {
		printf("This action is not allowed for this managementId\n");
		return FALSE;
	}
		
	out_length += TLV_LENGTH;
	return TRUE;
} 

Boolean 
packMMClockDescription(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CLOCK_DESCRIPTION);
	return packGETOnly(buf,manage);
}

Boolean
packMMUserDescription(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	MMUserDescription* data = NULL;
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	manage->tlv->lengthField = flip16(0x0002);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_USER_DESCRIPTION);

	//*(UInteger16 *) (buf + 52) = flip16(0x0002);		
	printf("\n>actionField (0 for GET, 1 for SET) ?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		manage->tlv->dataField = NULL;
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
		out_length += TLV_LENGTH;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		data = (MMUserDescription*)malloc(sizeof(MMUserDescription));
		char text[128];
		int dataFieldLength = 0;
		printf("\n>UserDescription (DeviceName;PhysicalLocation)?");
		scanf("%s",text);
		data->userDescription.lengthField = strlen(text);
		
		if (data->userDescription.lengthField > USER_DESCRIPTION_MAX){
			printf("user description exceeds specification length");
			return FALSE;
		}
		
		data->userDescription.textField = 
					(Octet*)malloc(data->userDescription.lengthField);
		memcpy(data->userDescription.textField,
                    text, data->userDescription.lengthField);

		memcpy((Octet*)manage->tlv + TLV_LENGTH, data, 1);	
		memcpy((Octet*)manage->tlv +  TLV_LENGTH + 1,
			data->userDescription.textField,
			data->userDescription.lengthField);
	
		               
        /* is the TLV length odd? TLV must be even according to Spec 5.3.8 */
        if (data->userDescription.lengthField % 2 == 0){
        	memset(buf + MANAGEMENT_LENGTH + 
	        		TLV_LENGTH + data->userDescription.lengthField + 1, 0, 1);
			dataFieldLength = data->userDescription.lengthField + 2;
		} 
		else {	
			dataFieldLength = data->userDescription.lengthField + 1;
		}

		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH + TLV_LENGTH + 
										dataFieldLength);		
		out_length += TLV_LENGTH + dataFieldLength;

		free(data->userDescription.textField);
		free(data);
		break;		       
	default:
		printf("This action is not allowed for this managementId\n");
		return FALSE;
	}
		
	return TRUE;
}


Boolean
packMMSaveInNonVolatileStorage(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_SAVE_IN_NON_VOLATILE_STORAGE);
	return packCommandOnly(buf,manage);
}

Boolean
packMMResetNonVolatileStorage(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_RESET_NON_VOLATILE_STORAGE);
	return packCommandOnly(buf,manage);
}

Boolean
packMMInitialize(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_INITIALIZE);
	out_length += TLV_LENGTH;	

	printf("\n>actionField (3 for Command) ?");
	scanf("%d",&actionField);
	switch(actionField){
	case COMMAND:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | COMMAND;
		printf("Initialize key (0 for Initialize event)?");
		scanf("%hu",(UInteger16*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH));
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n");
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMFaultLog(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_FAULT_LOG);
	out_length += TLV_LENGTH;

	printf("\n>actionField (0 for GET) ?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		printf("Number of fault logs to be returned (0-128)?");
		scanf("%hu",(UInteger16*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH));
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}

	return TRUE;
}

Boolean
packMMFaultLogReset(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_FAULT_LOG_RESET);
	return packCommandOnly(buf,manage);
}

Boolean
packMMTime(Octet *buf)
{
	int actionField;

	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TIME);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;

		UInteger32 hours;
		UInteger32 mins;
		UInteger32 secs;
		UInteger32 nanosecs;
		UInteger16 epoch;
		printf(">Timestamp - Epoch Number?");
		scanf("%hu",&epoch);
		*(UInteger16 *)((Octet*)(manage->tlv) + TLV_LENGTH) = flip16(epoch);
		printf(">Timestamp 'hours mins secs nanosecs' Eg - '1 20 30 400' ?");
		scanf("%u %u %u %u",&hours,&mins,&secs,&nanosecs);
		*(UInteger32*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 2) = 
								flip32(hours*60*60 + mins*60 + secs);
		*(UInteger32*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 6) = 
								flip32(nanosecs);								
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 10);
		manage->tlv->lengthField = flip16(0x000C);
		out_length += 10;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMClockAccuracy(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CLOCK_ACCURACY);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;

	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf(">clock accuracy number (20-31) see table 6?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;

		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMEnablePort(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_ENABLE_PORT);
	return packCommandOnly(buf,manage);
}


Boolean
packMMDisablePort(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	manage->tlv->managementId = flip16(MM_DISABLE_PORT);
	return packCommandOnly(buf,manage);
}

Boolean 
packMMDefaultDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_DEFAULT_DATA_SET);
	return 	packGETOnly(buf,manage);
}


Boolean
packMMPriority1(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PRIORITY1);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
		
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of Priority1?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMPriority2(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PRIORITY2);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;

	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of Priority2?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}


Boolean
packMMDomain(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_DOMAIN);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of domainNumber?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMSlaveOnly(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_SLAVE_ONLY);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;

	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		unsigned int slaveOnly;
		printf("Slave Only (1 for yes, 0 for no)?");
		scanf("%u",&slaveOnly);
		if (slaveOnly == 1)
			*((UInteger8 *)(manage->tlv) + TLV_LENGTH) = 0x01;
		 else if (slaveOnly == 0)
			*((UInteger8 *)(manage->tlv) + TLV_LENGTH) = 0x00;
		 else {
		 	printf("Invalid value\n");
			return FALSE;
		}
		
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}


Boolean 
packMMCurrentDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_CURRENT_DATA_SET);
	return 	packGETOnly(buf,manage);
}

Boolean 
packMMParentDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PARENT_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMTimePropertiesDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TIME_PROPERTIES_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMPortDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PORT_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMTransparentClockDefaultDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET);
	return packGETOnly(buf,manage);
}

Boolean 
packMMTransparentClockPortDataSet(Octet *buf)
{
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRANSPARENT_CLOCK_PORT_DATA_SET);
	return packGETOnly(buf,manage);
}


Boolean
packMMPrimaryDomain(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_PRIMARY_DOMAIN);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of primaryDomain?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}


Boolean
packMMLogAnnounceInterval(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_LOG_ANNOUNCE_INTERVAL);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of log announce interval?");
		scanf("%hhd",(Integer8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}


Boolean
packMMAnnounceReceiptTimeout(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_ANNOUNCE_RECEIPT_TIMEOUT);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of Announce Receipt Timeout?");
		scanf("%hhu",(UInteger8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMLogSyncInterval(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_LOG_SYNC_INTERVAL);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of log sync interval?");
		scanf("%hhd",(Integer8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMVersionNumber(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_VERSION_NUMBER);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		UInteger8 versionNumber;
		printf("version number?");
		scanf("%hhd",&versionNumber);
		*((UInteger8*)(manage->tlv) + TLV_LENGTH) = versionNumber & 0x0F;
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}


Boolean
packMMLogMinPdelayReqInterval(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_LOG_MIN_PDELAY_REQ_INTERVAL);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("value of log_min_pdelay_req_interval?");
		scanf("%hhd",(Integer8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean
packMMDelayMechanism(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_DELAY_MECHANISM);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		printf("delay mechanism?");
		scanf("%hhu",(Enumeration8 *)(manage->tlv) + TLV_LENGTH);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean 
packMMUtcProperties(Octet *buf)
{
	int actionField;
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_UTC_PROPERTIES);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		Integer16 currentUtcOffset;
		UInteger8 l161;
		UInteger8 l159;
		UInteger8 utcv;
		printf("value of currentUtcOffset?");
		scanf("%hd",&currentUtcOffset);
		printf("value of UTCV,L1-59,L1-61 (Boolean) Eg - 1,0,0 ?");
		scanf("%hhu,%hhu,%hhu",&utcv,&l159,&l161);
		if (utcv > 1 || l159 > 1 || l161 > 1){
			printf("Enter only boolean values (1 or 0) separated by comma\n");
			return FALSE;
		}
		
		*(Integer16*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH) =
							flip16(currentUtcOffset);
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 2) = 
							(utcv<<2 & 0x04) | (l159<<1 & 0x02) | (l161 & 0x01);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 4);
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH + 3) = 0x00;
		out_length += 4;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	
	return TRUE;
}

Boolean 
packMMTraceabilityProperties(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TRACEABILITY_PROPERTIES);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		UInteger8 ftra;
		UInteger8 ttra;
		printf("value of FTRA,TTRA (Boolean) Eg - 1,0 ?");
		scanf("%hhu,%hhu",&ftra,&ttra);
		if (ftra > 1 || ttra > 1){
			printf("Enter only boolean values (1 or 0) separated by comma\n");
			return FALSE;
		}
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH) = 
							(ftra<<5 & 0x20) | (ttra<<4 & 0x10);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	return TRUE;
}

Boolean 
packMMTimescaleProperties(Octet *buf)
{
	int actionField;
	
	MsgManagement *manage = (MsgManagement*)(buf);
	manage->tlv = (ManagementTLV*)(buf+48);
	*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH);
	manage->tlv->tlvType = flip16(TLV_MANAGEMENT);
	/*managementId Table 40*/
	manage->tlv->managementId = flip16(MM_TIMESCALE_PROPERTIES);
	manage->tlv->lengthField = flip16(0x0002);
	out_length += TLV_LENGTH;
	
	printf("\n>actionField (0 for GET, 1 for SET)?");
	scanf("%d",&actionField);
	switch(actionField){
	case GET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | GET;
		break;
	case SET:
		*(UInteger8 *) (buf + 46) = *(UInteger8 *) (buf + 46) | SET;
		UInteger8 ptp;
		printf("value of timeSource?");
		scanf("%hhu",(Enumeration8*)(manage->tlv) + TLV_LENGTH + 1);
		printf("value of ptp (Boolean)?");
		scanf("%hhu",&ptp);
		if (ptp > 1){
			printf("Enter only boolean values (1 or 0)\n");
			return FALSE;
		}
		*(UInteger8*)(buf + MANAGEMENT_LENGTH + TLV_LENGTH) = (ptp<<3 & 0x08);
		*(UInteger16 *) (buf + 2) = flip16(MANAGEMENT_LENGTH+TLV_LENGTH + 2);
		manage->tlv->lengthField = flip16(0x0004);
		out_length += 2;
		break;
	default:
		printf("This action is not allowed for this managementId\n"); 
		return FALSE;
	}
	return TRUE;
}

/*function to initialize the UDP networking stuff*/
Boolean 
netInit(char *ifaceName)
{
	int temp;
	struct sockaddr_in addr;
	
	/* open sockets */
	if ((netPath->eventSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 || 
			(netPath->generalSock = socket(PF_INET, SOCK_DGRAM, 
					      IPPROTO_UDP)) < 0) {
		printf("failed to initalize sockets");
		return (FALSE);
	}
	
	temp = 1;			/* allow address reuse */
	if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_REUSEADDR, 
		       &temp, sizeof(int)) < 0 ||
	     setsockopt(netPath->generalSock, SOL_SOCKET, SO_REUSEADDR, 
			  &temp, sizeof(int)) < 0) {
		printf("failed to set socket reuse\n");
	}


	/* bind sockets */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PTP_GENERAL_PORT);


/* To be used later for receiving management response */
/*	if (bind(netPath->generalSock, (struct sockaddr *)&addr, 
		 sizeof(struct sockaddr_in)) < 0) {
		printf("failed to bind general socket");
		return (FALSE);
	}

	addr.sin_port = htons(PTP_EVENT_PORT);
	if (bind(netPath->eventSock, (struct sockaddr *)&addr, 
		 sizeof(struct sockaddr_in)) < 0) {
		printf("failed to bind event socket");
		return (FALSE);
	}
*/

#ifdef linux
	/*
	 * The following code makes sure that the data is only received on the specified interface.
	 * Without this option, it's possible to receive PTP from another interface, and confuse the protocol.
	 * Calling bind() with the IP address of the device instead of INADDR_ANY does not work.
	 *
	 * More info:
	 *   http://developerweb.net/viewtopic.php?id=6471
	 *   http://stackoverflow.com/questions/1207746/problems-with-so-bindtodevice-linux-socket-option
	 */
	if (setsockopt(netPath->eventSock, SOL_SOCKET, SO_BINDTODEVICE,
			ifaceName, strlen(ifaceName)) < 0 ||
		 setsockopt(netPath->generalSock, SOL_SOCKET, SO_BINDTODEVICE,
			ifaceName, strlen(ifaceName)) < 0){
			
		printf("failed to call SO_BINDTODEVICE on the interface");
		return FALSE;
		
	}

#endif
	return TRUE;
}

/*Function to send management messages*/
ssize_t 
netSendGeneral(Octet * buf, UInteger16 length, char *ip)
{
	ssize_t ret;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PTP_GENERAL_PORT);
	if ((inet_pton(PF_INET, ip, &addr.sin_addr) ) == 0)	{
		printf("\nAddress is not parsable\n");
		return (0);
	} else if ( (inet_pton(AF_INET, ip, &addr.sin_addr) ) < 0){
		printf("\nAddress family is not supproted by this function\n");
		return (0);
	}
	
	ret = sendto(netPath->generalSock, buf, out_length, 0, 
			(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	
	if (ret <= 0)
	printf("error sending uni-cast general message\n");

	return (ret);
}

/* Function to receive the management response/ack/error message */
ssize_t
netRecv(Octet *message)
{
}

/*Function to unpack the header of received message*/
void 
unpackHeader(Octet *message, MsgHeader *h)
{
}

/*Function to unpack management header*/
void 
unpackManagementHeader(Octet *inmessage, MsgManagement *manage)
{
}

/*Function to handle management response and display the required fields*/
void 
handleManagementResponse(Octet *inmessage, MsgManagement *manage)
{
}

/*Function to handle management ack*/
void 
handleManagementAck(Octet *inmessage, MsgManagement *manage)
{
}

/*Function to handle management error message*/
void 
handleManagementError(Octet *inmessage, MsgManagement *manage)
{
}

/*Function to display last received message*/
void 
displayMessage(Octet *inmessage)
{
}

/*Function to display all management messages or any one based on user input*/
void 
displayManagementFields()
{
}

/*shutdown the network layer*/
void 
netShutdown()
{
}

/*Function to parse the command and return a command id*/
int 
getCommandId(char *command)
{

	if (strcmp(command, "send") == 0)
		return (1);
	else if (strcmp(command, "show_prev") == 0)
		return (2);
	else if (strcmp(command, "show_fields")==0)
		return (3);
	
	return (0); /* for wrong command */
}


/* 
 * The code implements ptpmanager responsible for sending and 
 * receiving management messages. Main function receives user commands and 
 * executes it. It also receives management response from server and handles it
 */
int 
main(int argc, char *argv[ ])
{

	Octet *outmessage = (Octet*)(malloc(PACKET_SIZE));
	Octet *inmessage = (Octet*)(malloc(PACKET_SIZE));
	
	
	int tlvtype;
	int managementId;
	Boolean toSend = TRUE;
	Boolean toRec = TRUE;	
	out_sequence = 0;
	in_sequence = 0;
	out_length = 0;
	netPath = (NetPath*)malloc(sizeof(NetPath));
	
	if (outmessage == NULL || inmessage == NULL || netPath == NULL){
		err(1, "Malloc error");
		exit(1);
	}
	
	char command[10];
	char new_command[10];
	/* 
	 * Command line arguments would include the IP address of PTP daemon and
	 * interface on which it runs	
	 */
	if (argc != 3){
		printf("\nThe input is not in the required format. \
		Please give the IP of PTP daemon and and its interface name\n");
		exit(1);
	}

	if (!netInit(argv[2])) {
		printf("failed to initialize network\n");
		return (1);
	}
	
	printf("Enter your command: 'send' for sending management message, \n \
	'show_previous' to display received response, \n \
	'show_fields' to see management message fields\ncommand>");

	scanf("%s", command);

	int j = 0;
	while (strcmp(command,"quit")){

		memset(outmessage, 0, PACKET_SIZE);
		memset(inmessage, 0, PACKET_SIZE);
		toSend = TRUE;
		
		switch (getCommandId(command)){

		case 1:
				packCommonHeader(outmessage);	
				packManagementHeader(outmessage);
			
				printf(">tlvType (1 for TLV_MANAGEMENT) ?");
				scanf("%d", &tlvtype);
			
				if (tlvtype==TLV_MANAGEMENT){
					printf(">managementId (see Table 40) (Eg: '2010' for ClockAccuracy)?");
					scanf("%x", &managementId);
			
					switch(managementId){
					case MM_NULL_MANAGEMENT:
						toSend = packMMNullManagement(outmessage);
						break;
					case MM_CLOCK_DESCRIPTION:
						toSend = packMMClockDescription(outmessage); 	
						break;
					case MM_USER_DESCRIPTION:
						toSend = packMMUserDescription(outmessage);
						break;
					case MM_SAVE_IN_NON_VOLATILE_STORAGE:
						toSend = packMMSaveInNonVolatileStorage(outmessage);
						break;
					case MM_RESET_NON_VOLATILE_STORAGE:
						toSend = packMMResetNonVolatileStorage(outmessage);	
						break;
					case MM_INITIALIZE:
						toSend = packMMInitialize(outmessage);
						break;
					case MM_FAULT_LOG:
						toSend = packMMFaultLog(outmessage);
						break;
					case MM_FAULT_LOG_RESET:
						toSend = packMMFaultLogReset(outmessage);
						break;
					case MM_DEFAULT_DATA_SET:
						toSend = packMMDefaultDataSet(outmessage);
						break;
					case MM_CURRENT_DATA_SET:
						toSend = packMMCurrentDataSet(outmessage);
						break;
					case MM_PARENT_DATA_SET:
						toSend = packMMParentDataSet(outmessage);
						break;
					case MM_TIME_PROPERTIES_DATA_SET:
						toSend = packMMTimePropertiesDataSet(outmessage);
						break;
					case MM_PORT_DATA_SET:
						toSend = packMMPortDataSet(outmessage);
						break;
					case MM_PRIORITY1:
						toSend = packMMPriority1(outmessage);
						break;
					case MM_PRIORITY2:
						toSend = packMMPriority2(outmessage);
						break;
					case MM_DOMAIN:
						toSend = packMMDomain(outmessage);
						break;
					case MM_SLAVE_ONLY:
						toSend = packMMSlaveOnly(outmessage);
						break;
					case MM_LOG_ANNOUNCE_INTERVAL:
						toSend = packMMLogAnnounceInterval(outmessage);
						break;
					case MM_ANNOUNCE_RECEIPT_TIMEOUT:
						toSend = packMMAnnounceReceiptTimeout(outmessage);
						break;
					case MM_LOG_SYNC_INTERVAL:
						toSend = packMMLogSyncInterval(outmessage);
						break;
					case MM_VERSION_NUMBER:
						toSend = packMMVersionNumber(outmessage);
						break;
					case MM_ENABLE_PORT:
						toSend = packMMEnablePort(outmessage);
						break;
					case MM_DISABLE_PORT:
						toSend = packMMDisablePort(outmessage);
						break;
					case MM_TIME:
						toSend = packMMTime(outmessage);
						break;
					case MM_CLOCK_ACCURACY:
						toSend = packMMClockAccuracy(outmessage);
						break;
					case MM_UTC_PROPERTIES:
						toSend = packMMUtcProperties(outmessage);
						break;
					case MM_TRACEABILITY_PROPERTIES:
						toSend = packMMTraceabilityProperties(outmessage);
						break;
					case MM_DELAY_MECHANISM:
						toSend = packMMDelayMechanism(outmessage);
						break;
					case MM_LOG_MIN_PDELAY_REQ_INTERVAL:
						toSend = packMMLogMinPdelayReqInterval(outmessage);
						break;
					case MM_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
						toSend = packMMTransparentClockDefaultDataSet(outmessage);
						break;
					case MM_TRANSPARENT_CLOCK_PORT_DATA_SET:
						toSend = packMMTransparentClockPortDataSet(outmessage);
						break;
					case MM_PRIMARY_DOMAIN:
						toSend = packMMPrimaryDomain(outmessage);
						break;
					case MM_TIMESCALE_PROPERTIES:
						toSend = packMMTimescaleProperties(outmessage);
						break;
	
					case MM_UNICAST_NEGOTIATION_ENABLE:
					case MM_PATH_TRACE_LIST:
					case MM_PATH_TRACE_ENABLE:
					case MM_GRANDMASTER_CLUSTER_TABLE:
					case MM_UNICAST_MASTER_TABLE:
					case MM_UNICAST_MASTER_MAX_TABLE_SIZE:
					case MM_ACCEPTABLE_MASTER_TABLE:
					case MM_ACCEPTABLE_MASTER_TABLE_ENABLED:
					case MM_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
					case MM_ALTERNATE_MASTER:
					case MM_ALTERNATE_TIME_OFFSET_ENABLE:
					case MM_ALTERNATE_TIME_OFFSET_NAME:
					case MM_ALTERNATE_TIME_OFFSET_MAX_KEY:
					case MM_ALTERNATE_TIME_OFFSET_PROPERTIES:

					default:
						printf("This managementId is not supported.\n");
						toSend = FALSE;
					}
					
				} else {
					printf("Only TLV_MANAGEMENT (enter 1 for this) is allowed yet\n");
					toSend = FALSE;
					break;
				}

				if (toSend){
					if (!netSendGeneral(outmessage, sizeof(outmessage),
						 argv[1]))
						printf("Error sending message\n");
					else {
						printf("Message sent, waiting for response...\n");

						receivedFlag = FALSE;
//						for (;;){
									
						/* 
						 * TODO wait for some time to receive a response 
						 * or ack, if not received till timeout send the 
						 * management message again, Use receivedFlag for this.
						 */

							if (netRecv(inmessage)) {
								MsgHeader *h; MsgManagement *manage;
								unpackHeader(inmessage, h);
								if (h->messageType == MANAGEMENT){
									receivedFlag = TRUE;
									unpackManagementHeader(inmessage, manage);
									if (manage->actionField == RESPONSE)
										handleManagementResponse(inmessage,
									   	 manage);
									else if (manage->actionField == RESPONSE)
										handleManagementAck(inmessage, manage);
									else if (manage->tlv->tlvType == 
										TLV_MANAGEMENT_ERROR_STATUS)
										handleManagementError(inmessage, manage);
								}
							}
							printf("received messages not handled yet\n");
						//}
					}
				}

			printf("\n");
			break;
		
		case 2:
			displayMessage(inmessage);
			break;
		
		case 3:
			displayManagementFields();
			break;
		
		default:
			printf("Invalid command\n\n");
		}

		printf("command>");
		scanf("%s", command);
	}
	
	free(outmessage);
	free(inmessage);	
	netShutdown();
	return (0);
}