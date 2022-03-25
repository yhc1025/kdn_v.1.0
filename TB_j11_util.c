#include <stdio.h>
#include "TB_debug.h"
#include "TB_elapse.h"
#include "TB_led.h"
#include "TB_rssi.h"
#include "TB_util.h"
#include "TB_j11.h"
#include "TB_j11_util.h"

int TB_j11_print_recv_data_info( int idx, TBUS recv_cmd, TBUC *recv_data )
{
	int ret = 0;
	
	if( recv_data )
	{
		if( recv_cmd == NORT_RCV_DAT )
		{
			int i = 0;
			int index = 12;
			int index_mac = 12+8;
			TBUS rcvheader_size = 39;
			TBUS size;
			TBUC percent;

//			TB_dbg_j11( "Notification Receved Data info (NORT_RCV_DAT)-------------------------------\r\n" );
//			TB_dbg_j11( "    Source IPv6 Address\n        " );
//			for( i=0; i<16; i++ )	TB_dbg_j11("%02X ", recv_data[index++] );
//			TB_dbg_j11("\r\n" );

//			TB_dbg_j11( "    Port number Source       : %02X %02X\r\n",	recv_data[index+0], recv_data[index+1] );
																	index += 2;
//			TB_dbg_j11( "    Port number Destination  : %02X %02X\r\n",	recv_data[index+0], recv_data[index+1] );
																	index += 2;
//			TB_dbg_j11( "    Source PAN ID            : %02X %02X\r\n",	recv_data[index+0], recv_data[index+1] );
																	index += 2;
//			TB_dbg_j11( "    Destination Address type : %d(%s)\r\n", 	recv_data[index],	\
																	recv_data[index] == 0x00 ? "Unicast"   		:	\
																	recv_data[index] == 0x01 ? "Multicast" 		:	\
																						   	   "Unknown" );
																	index += 1;
//			TB_dbg_j11( "    Encription               : %d(%s)\r\n",	recv_data[index],	\
																	recv_data[index] == 0x01 ? "Not encrpted" 	:	\
																	recv_data[index] == 0x02 ? "Encrypted"		:	\
																						   	   "Unknown" );
																	index += 1;
//			TB_rssi_set_comm_info( idx, recv_data[index], &recv_data[index_mac] );
			TB_dbg_j11( "RSSI = %ddbm\r\n", recv_data[index] );
																	index += 1;
//			size = (recv_data[index] << 8) | recv_data[index+1];
																	index += 2;
//			TB_dbg_j11( "    Reception data size      : %d\r\n", size );
//			TB_dbg_j11( "    Reception data( All data )\n        " );
//			for( i=0; i<size; i++ )	TB_dbg_j11("%02X ", recv_data[index++] );
//			TB_prt_j11("\r\n" );
//			TB_dbg_j11( "--------------------------------------------------------------\r\n" );
		}
		else if( recv_cmd == NORT_RCV_DAT_ERR )
		{
			static TBUI s_count = 0;
			int i;
			int index = 12;

			printf("[%s:%d] NORT_RCV_DAT_ERR = %d\r\n", __FILE__, __LINE__, ++s_count );
			
			TB_dbg_j11( "Notify Packet Reception Failure (NORT_RCV_DAT_ERR)-------------------------------\r\n" );
			TB_dbg_j11( "    1. Reason for reception failure\r\n");
			switch( recv_data[12] )
			{
				case 0x01 : TB_dbg_j11("          Decoding failure\r\n");	break;
				case 0x02 : TB_dbg_j11("          MAC failure : Except decoding failure\r\n");	break;
				case 0x20 : TB_dbg_j11("          6LowPAN failure\r\n");	break;
				case 0x30 : TB_dbg_j11("          IP failure\r\n");			break;
				case 0x40 : TB_dbg_j11("          UDP failure\r\n");		break;
			}

			TB_dbg_j11( "    2. Source IPv6 address\r\n        ");
			for( i=0; i<16; i++, index++ )
				TB_prt_j11( "%02X ", recv_data[12+i] );
			TB_prt_j11( "\r\n" );
			
			TB_dbg_j11( "    3. Reception data sequence number = %d\r\n",   recv_data[index] );	index++;
			TB_dbg_j11( "    4. Fragment                       = %02X\r\n", recv_data[index] );	index++;
			TB_dbg_j11( "    5. Fragment tag                   = %02X\r\n", recv_data[index] );	index++;
			TB_dbg_j11( "    6. Overview of reception data\n");
			for( i=0; i<5; i++ )
			{
				TB_prt_j11( "%02X ", recv_data[index] );
				index++;
			}
			TB_prt_j11( "\r\n" );
		}
		else if( recv_cmd == RES_UDPSEND )
		{
			if( recv_data[12] == 0x01 )
			{
				int Y = (recv_data[13] >> 4) & 0x0F;
				int Z = (recv_data[13] >> 0) & 0x0F;
				TB_dbg_j11("1. Result of data transmission(0xYZ) = 0x%02X\r\n", recv_data[13] );
				TB_dbg_j11("	1.1 Y represents result of indirect queuing = 0x%X\r\n", Y );
				switch( Y )
				{
					case 0x00 : TB_dbg_j11("		No data queued in indirect queue\r\n" );			break;
					case 0x01 : TB_dbg_j11("		Data queued in indirect queue\r\n" );				break;
					case 0x02 : TB_dbg_j11("		Failed to queue data in indirect queue\r\n" );		break;
				}
				TB_dbg_j11("	1.2 Z represents result of UDP data transmission = 0x%X\r\n", Z );
				switch( Z )
				{
					case 0x00 : TB_dbg_j11("		Succeeded\r\n" );									break;
					case 0x02 : TB_dbg_j11("		Transmission failed due to limitation on the sum of transmission data amount\r\n" );	break;
					case 0x03 : TB_dbg_j11("		Transmission failed due to failure in CCA\r\n" );	break;
					case 0x05 : TB_dbg_j11("		Transmission failed due to unreceived ACK\r\n" );	break;
					case 0x08 : TB_dbg_j11("		Transmission failed due to other cause of failure\r\n" );	break;
					case 0x0F : TB_dbg_j11("		No transmission (only queuing)\r\n" );				break;
				}
				TB_dbg_j11("2. Overview of transmission data = 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n", recv_data[13], recv_data[14], recv_data[15], recv_data[16], recv_data[17] );
			}
		}
		else if( recv_cmd == RES_HAN )
		{
			if( recv_data[12] == 0x01 )
			{
//				TB_dbg_j11("HAN Act result ------------------------------\r\n");
//				TB_dbg_j11("    response result = %d\r\n", recv_data[12]);
//				TB_dbg_j11("    channel         = %d\r\n", recv_data[13]);
//				TB_dbg_j11("    pan id          = 0x%02X 0x%02X\r\n", recv_data[14], recv_data[15]);
//				TB_dbg_j11("    MAC of pancoord = %02X %02X %02X %02X %02X %02X %02X %02X\r\n",	\
												recv_data[16], recv_data[17], recv_data[18], recv_data[19],	\
												recv_data[20], recv_data[21], recv_data[22], recv_data[23] );
//				TB_rssi_set_comm_info( idx, recv_data[24], &recv_data[16]  );
//				TB_dbg_j11("    RSSI            = %ddBm(0x%02X), %d%%\r\n", (TBSC)recv_data[24], recv_data[24], percent );
//				TB_dbg_j11("---------------------------------------------\r\n");
			}
		}
		else if( recv_cmd == NORT_CON_STAT_CHANG )
		{			
			int i = 0;
			int index = 13;
			TBUC percent;
			
			TB_dbg_j11("MAC address of devices connected\r\n    " );
			for( i=0; i<8; i++, index++ )
				TB_prt_j11("%02X ", recv_data[13 + i] );
			TB_prt_j11("\r\n" );

//			TB_rssi_set_comm_info( idx, recv_data[index], &recv_data[13] );
		}
		else if( recv_cmd == RES_PANA )
		{
//			TB_dbg_j11("PANA Act result ------------------------------\r\n");
//			TB_dbg_j11("    response result = %d\r\n", recv_data[12]);
//			TB_dbg_j11("    MAC of pancoord = %02X %02X %02X %02X %02X %02X %02X %02X\r\n",	\
											recv_data[13], recv_data[14], recv_data[15], recv_data[16],	\
											recv_data[17], recv_data[18], recv_data[19], recv_data[20] );
//			TB_dbg_j11("----------------------------------------------\r\n");
		}
		else if( recv_cmd == NORT_PING )
		{
			int i = 0;
			int index = 12;
			TBUS size;
			TBUC percent;

			TB_dbg_j11("NORT_PING : ----------------------------------------------------\r\n");
			TB_dbg_j11( "    1. PING Result\r\n");
			switch( recv_data[index] )
			{
				case 0x00 : TB_dbg_j11("          Not responded (Echo Reply not received)\r\n" );	break;
				case 0x01 : TB_dbg_j11("          Responded (Echo Reply received)\r\n");			break;
			}

			if( recv_data[index] == 0x01 )
			{
				index += 1;
				
				TB_dbg_j11( "    2. Source IPv6 address\r\n        ");
				int idx_mac = index + 8;
				for( i=0; i<16; i++ )
					TB_prt_j11( "%02X ", recv_data[index++] );
				TB_prt_j11( "\r\n" );
				
				TB_dbg_j11( "    3. Encription : 0x%02X(%s)\r\n",	recv_data[index],	\
																	recv_data[index] == 0x01 ? "Not encrpted" 	:	\
																	recv_data[index] == 0x02 ? "Encrypted"		:	\
																						   	   "Unknown" );
				index += 1;
				
				TB_rssi_set_comm_info( idx, recv_data[index], &recv_data[idx_mac] );
				index += 1;
				
				size = (recv_data[index] << 8) | recv_data[index+1];
				index += 2;
				
				TB_dbg_j11( "    5. Reception data size : %d\r\n", size );
				TB_dbg_j11( "    6. Reception data\n        " );
				for( i=0; i<size; i++ )
					TB_prt_j11("%02X ", recv_data[index++] );
				TB_prt_j11("\r\n" );
			}
			TB_dbg_j11( "--------------------------------------------------------------\r\n" );
		}
	}
	else
	{
		TB_dbg_j11( "[%s:%d] print data is NULL\r\n", __FILE__, __LINE__ );
		ret = -1;
	}
	
	return ret;
}

void TB_j11_util_debug_state( int idx, TB_STATE state )
{
	char buf[256] = {0, };

	char *status_string[] = 
	{
		"WISUN_STEP_RESET\r\n",
		"WISUN_STEP_GETIP\r\n",
		"WISUN_STEP_GETMAC\r\n",
		"WISUN_STEP_INIT_STATE\r\n",
		"WISUN_STEP_HAN_PANA_SET\r\n",
		"WISUN_STEP_HAN_ACT\r\n",
		"WISUN_STEP_HAN_PANA_ACT\r\n",
		"WISUN_STEP_CONNECT_SET\r\n",
		"WISUN_STEP_PORT_OPEN\r\n",
		"WISUN_STEP_UDP_SEND\r\n",
		"WISUN_STEP_RECV_WAIT\r\n",
		"WISUN_STEP_CONNECTION\r\n",
		"UNKNOWN STATUS\r\n"
	};
	
	snprintf( buf, sizeof(buf), "[%d] WiSUN State = (%2d) ", idx, state );
	if( state >= WISUN_STEP_RESET && state <= WISUN_STEP_CONNECTION )
	{
		wstrcat( buf, sizeof(buf), status_string[state] );
	}
	else if( state == WISUN_STEP_INIT_DONE )
	{
		wstrcat( buf, sizeof(buf), status_string[WISUN_STEP_CONNECTION+1] );
	}
	else
	{
		wstrcat( buf, sizeof(buf), status_string[WISUN_STEP_CONNECTION+2] );
	}

	TB_prt_j11( "%s", buf );
}

void TB_j11_util_print_wisun_direction( TB_WISUN_DIR dir )
{
	char *string = NULL;
	char *dir_strng[WISUN_DIR_MAX] = 
	{
		"WISUN_DIR_UNKNOWN",
		"WISUN_DIR_RELAY2TERM",
		"WISUN_DIR_RELAY2GGW",
		"WISUN_DIR_TERM2GGW",
		"WISUN_DIR_TERM2RELAY1",
		"WISUN_DIR_TERM2RELAY2",
		"WISUN_DIR_TERM2RELAY3",
		"WISUN_DIR_GGW2TERM1",
		"WISUN_DIR_GGW2TERM2",
		"WISUN_DIR_GGW2TERM3",
	};

	if( dir > WISUN_DIR_MIN && dir < WISUN_DIR_MAX )
	{
		string = dir_strng[dir];
	}
	else
	{
		string = dir_strng[0];
	}

	TB_prt_j11( "%s\r\n", string );
}

