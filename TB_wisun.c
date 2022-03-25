#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "TB_sys_wdt.h"

#include "TB_lte.h"
#include "TB_led.h"
#include "TB_uart.h"
#include "TB_util.h"
#include "TB_j11_util.h"
#include "TB_modbus.h"
#include "TB_msg_queue.h"
#include "TB_packet.h"
#include "TB_psu.h"
#include "TB_log.h"
#include "TB_debug.h"
#include "TB_net_queue.h"
#include "TB_setup.h"
#include "TB_kcmvp.h"
#include "TB_ping.h"
#include "TB_rssi.h"
#include "TB_ems.h"
#include "TB_wisun_util.h"
#include "TB_wisun.h"
#include "TB_led.h"
#include "TB_modem.h"

////////////////////////////////////////////////////////////////////////////////

TB_WISUN_MODE 	g_wisun_mode[2] = {WISUN_MODE_PANC, WISUN_MODE_ENDD};
TB_ROLE 		g_role	= ROLE_GRPGW;

static pthread_t 	s_thid_wisun[2];
static TBUC 		s_proc_flag_wisun = 1;

////////////////////////////////////////////////////////////////////////////////

int TB_wisun_uart_init( int idx )
{
	if( idx == 0 )
		TB_uart_wisun_1st_init();

	if( idx == 1 )
		TB_lte_init();
}

void TB_wisun_set_mode( TBUC idx, TB_WISUN_MODE mode )
{
	if( idx == 0 || idx == 1 )
		g_wisun_mode[idx] = mode;
}

TB_WISUN_MODE TB_wisun_get_mode( TBUC idx )
{
	TB_WISUN_MODE mode = WISUN_MODE_ERR_MIN;
	if( idx == 0 || idx == 1 )
		mode = g_wisun_mode[idx];
	return mode;
}

void TB_set_role( TB_ROLE role )
{
	g_role = role;
}

TB_ROLE TB_get_role( void )
{
	return g_role;
}

TBBL TB_get_role_is_grpgw( void )
{
	return ( g_role == ROLE_GRPGW ) ? TRUE : FALSE;
}

TBBL TB_get_role_is_terminal( void )
{
	return ( g_role >= ROLE_TERM_MIN && g_role <= ROLE_TERM_MAX ) ? TRUE : FALSE;
}

TBBL TB_get_role_is_relay( void )
{
	return ( g_role >= ROLE_RELAY_MIN && g_role <= ROLE_RELAY_MAX ) ? TRUE : FALSE;
}

int TB_wisun_recv_data_process_terminal( int idx, TB_ROLE role, TBUC *payload, TBUS pl_size )
{
	TB_MESSAGE 	msg;
	
	if( TB_wisun_util_ipaddr_req_check_packet(payload, pl_size) == TRUE )
	{
		TB_wisun_util_ipaddr_res_send( idx, role, ROLE_GRPGW );
	}
	else if( TB_wisun_util_ems_check_packet(payload, pl_size) == TRUE )
	{
		TB_dbg_ems( "SEND EMS CHECK message : pl_size=%d\r\n", pl_size);
		
		msg.type 	= MSG_TYPE_EMS;
		msg.id		= MSG_CMD_EMS_CHECK;
		msg.data	= &payload[4];	//	header offset
		msg.size	= pl_size - 8;	//	header + tail size

		TB_msgq_send( &msg );
	}
	else
	{
		if( TB_dm_is_on(DMID_WISUN) )
			TB_util_data_dump( "From GROUP-GW", payload, pl_size );
		
		msg.type 	= MSG_TYPE_PSU_DNP;
		msg.id		= MSG_CMD_PSU_DNP_WRITE;
		msg.data	= payload;
		msg.size	= pl_size;

		TB_msgq_send( &msg );
	}

	return 0;
}

int TB_wisun_recv_data_process_relay( TB_ROLE role, TBUC *payload, TBUS pl_size )
{
	TB_MESSAGE 	msg;
	
	if( TB_wisun_util_ipaddr_req_check_packet(payload, pl_size) == TRUE )
	{
		TB_wisun_util_ipaddr_res_send( 0, role, ROLE_TERM );
	}
	else if( TB_wisun_util_timeinfo_res_check_packet(payload, pl_size) == TRUE )
	{
		TB_wisun_util_timeinfo_set_systemtime( &payload[11] );	//	11 is head(4) + 'RESTIME'(7)
	}
	else if( TB_wisun_util_ems_check_packet(payload, pl_size) == TRUE )
	{
		TB_dbg_ems( "SEND EMS CHECK message\r\n");
		
		msg.type 	= MSG_TYPE_EMS;
		msg.id		= MSG_CMD_EMS_CHECK;
		msg.data	= &payload[4];	//	header offset
		msg.size	= pl_size - 8;	//	header + tail size

		TB_msgq_send( &msg );
	}
	else
	{
		static TBUC s_cmd_buf[128] = {0,};
		static TBUC	s_cmd_idx = 0;
		static TBUC	s_cmd_len = 0;

		TBUC relay_slvid = 0xFF;

			 if( role == ROLE_RELAY1 )	relay_slvid = SLAVEID_RELAY1;
		else if( role == ROLE_RELAY2 )	relay_slvid = SLAVEID_RELAY2;
		else if( role == ROLE_RELAY3 )	relay_slvid = SLAVEID_RELAY3;
		else							TB_dbg_wisun( "Error. Invalid ROLE : %d\r\n", role );
		
		TB_dbg_wisun( "payload[0] = 0x%02X, relay_slvid = 0x%02X\r\n", payload[0], relay_slvid );
		if( payload[0] == relay_slvid || payload[0] == 0x00 )
		{
			if( TB_dm_is_on(DMID_WISUN) )
				TB_util_data_dump1( payload, pl_size );

			TB_dbg_wisun( "payload[0] = 0x%02X, payload[1] = 0x%02X    0x%02X\r\n", payload[0], payload[1], FUNC_DOUBLE_READ_REC_REG );
			
			switch( payload[1] )
			{
				case FUNC_DOUBLE_READ_REC_REG :
					if( TB_crc16_modbus_check( payload, pl_size ) )
					{
						TBUS ivt_addr = (payload[3] << 8) | payload[2];
						TBUS reg_addr = (payload[4] << 8) | payload[5];
						TBUS reg_cont = (payload[6] << 8) | payload[7];

						if( reg_addr >= 1000 && reg_addr < 2000 )
						{									
							msg.type 	= MSG_TYPE_INVERTER;
							msg.id		= FUNC_DOUBLE_READ_REC_REG;
							msg.data	= payload;
							msg.size	= pl_size;
							
							TB_msgq_send( &msg );
						}
					}
					else
					{
						TB_dbg_wisun("crc error\n" );
					}
					break;

				case FUNC_DOUBLE_READ_REC_REG_IMMEDI :
					{
						TBUS ivt_addr = (payload[3] << 8) | payload[2];
						TBUS reg_addr = (payload[4] << 8) | payload[5];
						TBUS reg_cont = (payload[6] << 8) | payload[7];

						if( reg_addr >= 1000 && reg_addr < 2000 )
						{									
							msg.type 	= MSG_TYPE_INVERTER;
							msg.id		= FUNC_DOUBLE_READ_REC_REG_IMMEDI;
							msg.data	= payload;
							msg.size	= pl_size;

							TB_dbg_wisun("Send Message --> MSG_TYPE_INVERTER : FUNC_DOUBLE_READ_REC_REG_IMMEDI\n" );
							
							TB_msgq_send( &msg );
						}
					}
					break;

				case FUNC_READ_HOLDING_REG :
					{
						TBUS reg_addr = (payload[2] << 8) | payload[3];
						if( reg_addr >= 6000 && reg_addr < 7000 )	//	0x1770 -> 6000
						{
							printf( "[%s:%d]--------> SETTING REQ \r\n", __FILE__, __LINE__ );
							printf( "[%s:%d]--------> SETTING REQ \r\n", __FILE__, __LINE__ );
							printf( "[%s:%d]--------> SETTING REQ \r\n", __FILE__, __LINE__ );

							/***************************************************
							*					설정값 요청
							***************************************************/
							TB_SETUP_INVT *p_invt = TB_setup_get_invt_info();
							if( p_invt )
							{
								TBUC util_buf[128] = {0, };
								int idx = 0;

								bzero( util_buf, sizeof(util_buf) );
								util_buf[idx++] = relay_slvid;//Slave ID of Relay
								util_buf[idx++] = FUNC_READ_HOLDING_REG;
								util_buf[idx++] = 0x08;
								util_buf[idx++] = (p_invt->cnt     >> 8) & 0xFF;
								util_buf[idx++] = (p_invt->cnt     >> 0) & 0xFF;
								util_buf[idx++] = (p_invt->retry   >> 8) & 0xFF;
								util_buf[idx++] = (p_invt->retry   >> 0) & 0xFF;
								util_buf[idx++] = (p_invt->timeout >> 8) & 0xFF;
								util_buf[idx++] = (p_invt->timeout >> 0) & 0xFF;
								util_buf[idx++] = 0;	//	Reserved
								util_buf[idx++] = 0;	//	Reserved
								TB_crc16_modbus_fill( util_buf, idx );
								idx += 2;

								msg.type 	= MSG_TYPE_WISUN_TERM2RELAY;
								msg.id		= MSG_CMD_WISUN_SEND_RELAY2TERM;
								msg.data	= util_buf;
								msg.size	= idx;

								printf( "[%s:%d]--------> SEND ========= SETTING REQ \r\n", __FILE__, __LINE__ );

							    TB_msgq_send( &msg );
							}
						}
					}
					break;
				case FUNC_WRITE_SINGLE_REG :		//	Write Single Register( 0x06 )
					if( TB_crc16_modbus_check( payload, pl_size ) )
					{
						TBUS reg_addr  = (payload[2] << 8) | payload[3];
						TBUS reg_value = (payload[4] << 8) | payload[5];
						
						if( reg_addr >= 2100 && reg_addr < 3000 )
						{
							msg.type 	= MSG_TYPE_INVERTER;
							msg.id		= FUNC_WRITE_SINGLE_REG;
							msg.data	= payload;
							msg.size	= pl_size;

							TB_msgq_send( &msg );
						}
					}
					break;

				case FUNC_WRITE_MULTIPLE_REG :	//	Write Multiple Register( 0x10 )
					if( TB_crc16_modbus_check( payload, pl_size ) )
					{
						TBUS reg_addr  = (payload[2] << 8) | payload[3];
						TBUS reg_cont  = (payload[4] << 8) | payload[5];
						TBUS byte_cont = payload[6];

						if( reg_addr >= 2000 && reg_addr < 3000 )
						{
							msg.type 	= MSG_TYPE_INVERTER;
							msg.id		= FUNC_WRITE_MULTIPLE_REG;
							msg.data	= payload;
							msg.size	= pl_size;
							TB_msgq_send( &msg );
						}
						else if( reg_addr >= 6000 && reg_addr < 7000 )	//	0x1770 --> 6000
						{
							printf( "[%s:%d]--------> SETTING MOD \r\n", __FILE__, __LINE__ );
							printf( "[%s:%d]--------> SETTING MOD \r\n", __FILE__, __LINE__ );
							printf( "[%s:%d]--------> SETTING MOD \r\n", __FILE__, __LINE__ );

							/***************************************************
							*					설정값 변경
							***************************************************/
							TB_SETUP_INVT invt;

							invt.cnt 	 = (payload[8]  << 8) | payload[7];
							invt.retry 	 = (payload[10] << 8) | payload[9];
							invt.timeout = (payload[12] << 8) | payload[11];

							if( (invt.cnt >= MIN_INVT_CNT && invt.cnt <= MAX_INVT_CNT)    &&	\
								(invt.retry >= MIN_INVT_RETRY && invt.retry <= MAX_INVT_RETRY)  &&	\
								(invt.timeout >= MIN_INVT_TIMEOUT && invt.timeout <= MAX_INVT_TIMEOUT) )
							{
								TB_setup_set_invt_info( &invt );
							}

							printf( "[%s:%d]--------> SETTING RES \r\n", __FILE__, __LINE__ );
							printf( "[%s:%d]--------> SETTING RES \r\n", __FILE__, __LINE__ );
							printf( "[%s:%d]--------> SETTING RES \r\n", __FILE__, __LINE__ );

							/***************************************************
							*				설정값 변경 후 응답
							***************************************************/
							TBUC util_buf[128];
							int idx = 0;

							bzero( util_buf, sizeof(util_buf) );
							util_buf[idx++] = relay_slvid;//Slave ID of Relay
							util_buf[idx++] = FUNC_WRITE_MULTIPLE_REG;
							util_buf[idx++] = 0x17;	//	(reg_addr >> 8) & 0xFF;
							util_buf[idx++] = 0x70;	//	(reg_addr >> 0) & 0xFF;
							util_buf[idx++] = 0x00;
							util_buf[idx++] = 0x04;
							TB_crc16_modbus_fill( util_buf, idx );
							idx += 2;
								
							msg.type 	= MSG_TYPE_WISUN_TERM2RELAY;
							msg.id		= MSG_CMD_WISUN_SEND_RELAY2TERM;
							msg.data	= util_buf;
							msg.size	= idx;
							TB_msgq_send( &msg );
						}
					}
					break;

				default :
					break;
			}
		}
	}

	return 0;
}	

int TB_wisun_recv_data_parsing( int idx, TBUC *recv_buf, TBUS recv_length )
{
	int ret = -1;

	if( recv_buf && recv_length > 0 )
	{
		static TB_BUF1024 util_buf;

		TB_WISUN_MODE 	mode = TB_wisun_get_mode( idx );
		TB_ROLE 		role = TB_get_role();

		TB_dbg_wisun("idx=%d, mode=%d, role=%d\r\n", idx, mode, role );
		
		TB_MESSAGE 	msg;
		TB_PACKET  *p_packet = (TB_PACKET *)recv_buf;

//		if( TB_dm_is_on(DMID_WISUN) )
//			TB_packet_dump( p_packet );

		if( TB_packet_check( p_packet ) == TRUE )
		{
			TB_PACKET_HEAD 	*p_head  = (TB_PACKET_HEAD *)p_packet->data;
			TBUC 			*payload = &p_packet->data[sizeof(TB_PACKET_HEAD)];
			TBUS 			pl_size  = p_head->TLENGTH;

			switch( p_head->CMD )
			{
				case MSG_CMD_WISUN_SEND_TERM2RELAY1 :
				case MSG_CMD_WISUN_SEND_TERM2RELAY2 :
				case MSG_CMD_WISUN_SEND_TERM2RELAY3 :
					TB_wisun_recv_data_process_relay( role, payload, pl_size );
					break;

				////////////////////////////////////////////////////////////////

				case MSG_CMD_WISUN_SEND_GGW2TERM1 	:
				case MSG_CMD_WISUN_SEND_GGW2TERM2 	:
				case MSG_CMD_WISUN_SEND_GGW2TERM3 	:
					TB_wisun_recv_data_process_terminal( idx, role, payload, pl_size );
					break;

				////////////////////////////////////////////////////////////////
				
				case MSG_CMD_WISUN_SEND_GGW2REPEATER:
					if( TB_wisun_util_ipaddr_req_check_packet(payload, pl_size) == TRUE )
						TB_wisun_util_ipaddr_res_send( idx, role, ROLE_GRPGW );
					break;

				case MSG_CMD_WISUN_SEND_TERM2GGW	:
					if( TB_wisun_util_ipaddr_res_check_packet( payload, pl_size ) == TRUE )
					{
						TB_wisun_util_ipaddr_connt_device_save( &payload[9] );	//	9 is head(4) + 'RESIP'(5)
					}
					else if( TB_wisun_util_timeinfo_res_check_packet(payload, pl_size) == TRUE )
					{
						TB_wisun_util_timeinfo_set_systemtime( &payload[11] );	//	11 is head(4) + 'RESTIME'(7)
					}
					else if( TB_wisun_util_ems_check_packet( payload, pl_size ) == TRUE )
					{
						bzero( &util_buf, sizeof(util_buf) );
						wmemcpy( &util_buf.buffer[0], sizeof(util_buf.buffer), payload, pl_size );
						util_buf.length = pl_size;

						if( TB_dm_is_on(DMID_WISUN) )
							TB_util_data_dump("RECV. send from Term to GGW", util_buf.buffer, util_buf.length );

						//	4 : skip ems check header
						//	8 : ems check header(4) + ems check tail(4)
						TB_ems_write( &util_buf.buffer[4], util_buf.length - 8 );
					}
					else
					{
#if 1
						TB_modem_write( payload, pl_size );
#else
						bzero( &util_buf, sizeof(util_buf) );
						wmemcpy( &util_buf.buffer[0], sizeof(util_buf.buffer), payload, pl_size );
						util_buf.length = pl_size;
						
						msg.type 	= MSG_TYPE_MODEM;
						msg.id		= MSG_CMD_MODEM_WRITE;
						msg.data	= &util_buf.buffer[0];
						msg.size	= util_buf.length;
						TB_msgq_send( &msg );

						TB_util_delay( 500 );
#endif
					}
					break;

				case MSG_CMD_WISUN_SEND_RELAY2TERM 	:
					if( TB_wisun_util_ipaddr_res_check_packet( payload, pl_size ) == TRUE )
					{
						TB_wisun_util_ipaddr_connt_device_save( &payload[9] );	//	9 is head(4) + 'RESIP'(5)
					}
					else if( TB_wisun_util_ems_check_packet( payload, pl_size ) == TRUE )
					{
						bzero( &util_buf, sizeof(util_buf) );
						wmemcpy( &util_buf.buffer[0], sizeof(util_buf.buffer), payload, pl_size );
						util_buf.length = pl_size;

						if( TB_dm_is_on(DMID_WISUN) )
							TB_util_data_dump("RECV. send from Relay to Term", util_buf.buffer, util_buf.length );

						TBSI comm_type = TB_setup_get_comm_type_master();
						if( comm_type == COMM_MODE_MASTER_WISUN )
						{
							TB_packet_set_send_direction( WISUN_SECOND, WISUN_DIR_TERM2GGW );
							TB_packet_send( WISUN_SECOND, MSG_CMD_WISUN_SEND_TERM2GGW, &util_buf.buffer, util_buf.length );
						}
					}
					else
					{
						bzero( &util_buf, sizeof(util_buf) );
						wmemcpy( &util_buf.buffer[0], sizeof(util_buf.buffer), payload, pl_size );
						util_buf.length = pl_size;

						//	인버터에서 보내는 데이터.
						if( TB_dm_is_on(DMID_WISUN) )
							TB_util_data_dump( "INVERTER DATA", util_buf.buffer, util_buf.length );
						
						msg.type 	= MSG_TYPE_PSU_MOD;
						msg.id		= MSG_CMD_PSU_MOD_WRITE;
						msg.data	= &util_buf.buffer[0];
						msg.size	= util_buf.length;

						TB_msgq_send( &msg );
					}
					break;

				////////////////////////////////////////////////////////////////

				case MSG_CMD_WISUN_SEND_REQ_IP_FROM_GGW	:
					if( TB_wisun_util_ipaddr_req_check_packet( payload, pl_size ) == TRUE )
					{
						TB_wisun_util_ipaddr_res_send( idx, role, ROLE_GRPGW );
					}
					break;

				case MSG_CMD_WISUN_SEND_REQ_IP_FROM_TERM :
					if( TB_wisun_util_ipaddr_req_check_packet( payload, pl_size ) == TRUE )
					{
						TB_wisun_util_ipaddr_res_send( 0, role, ROLE_TERM );
					}
					break;

				case MSG_CMD_WISUN_SEND_REQ_TIMEINFO_FROM_GGW	:
					if( TB_wisun_util_timeinfo_req_check_packet( payload, pl_size ) == TRUE )
					{
						TB_dbg_wisun("Recv. Req TimeInfo from GGW\r\n" );
						TB_wisun_util_timeinfo_res_send( idx, role, ROLE_GRPGW );
					}
					break;
					
				case MSG_CMD_WISUN_SEND_REQ_TIMEINFO_FROM_RELAY	:
					if( TB_wisun_util_timeinfo_req_check_packet( payload, pl_size ) == TRUE )
					{
						TB_dbg_wisun("Recv. Req TimeInfo from Relay\r\n" );
						TB_wisun_util_timeinfo_res_send( idx, role, payload[4] );
					}
					break;

				case MSG_CMD_WISUN_SEND_REQ_AVAILABLE_TIMEINFO	:
					if( TB_get_role_is_relay() )
					{
						TB_dbg_wisun("Recv[Relay]. REQ_AVAILABLE_TIMEINFO\r\n" );
						TB_wisun_util_timeinfo_req_send( TRUE, __FILE__, __LINE__ );
					}
					else if( TB_get_role_is_grpgw() )
					{
						TB_dbg_wisun("Recv[GGW]. REQ_AVAILABLE_TIMEINFO\r\n" );
						TB_wisun_util_timeinfo_req_send( FALSE, __FILE__, __LINE__ );
					}
					break;

				default :
					TB_dbg_wisun("Unknown packet command : 0x%X\r\n", p_head->CMD );
					break;
			}

			ret = 0;
		}
		else
		{
			TB_dbg_wisun("Invalid Packet\r\n" );
		}
	}

	return ret;
}

void TB_wisun_proc_netq_data( int idx, TBUS cmd_flag )
{
	static TBUC reenq_buf[512] = {0,};
	static TBUS reenq_length = 0;

	static TBUC recv_buf[512] = {0,};
	static TBUS recv_length = 0;

	TB_WISUN_QUEUE *p_wisun_netq = (idx==0) ? &g_wisun_netq_1st : &g_wisun_netq_2nd;
	while( 1 )
	{
		if( p_wisun_netq->deq( &p_wisun_netq->netq, recv_buf, &recv_length ) != 0 )
		{
//			TB_dbg_j11( "%d. Dequeue None\n", idx );
			return;
		}

		TBUS recv_cmd = (recv_buf[4] << 8) | recv_buf[5];
		TB_dbg_wisun( "idx=%d, recv_cmd = 0x%04X\r\n", idx, recv_cmd );
		switch( recv_cmd )
		{
			case (NORT_RCV_DAT):
				{
					int idx_ip 			= 12;
					int idx_mac 		= 20;
					int idx_rssi 		= 36;
					int idx_data_len 	= 37;
					int idx_data 		= 39;
					
					TBUC *util_buf = &recv_buf[idx_data];
					TBUS  len = (recv_buf[idx_data_len] << 8) | recv_buf[idx_data_len+1];

					if( recv_length != len + 39 )	//	39 is wisun header size
					{
						printf("[%s:%d] ------ wisun network dequeue [%d:%d]\r\n", __FILE__, __LINE__, recv_length, len );
						printf("[%s:%d] ------ wisun network dequeue [%d:%d]\r\n", __FILE__, __LINE__, recv_length, len );
						printf("[%s:%d] ------ wisun network dequeue [%d:%d]\r\n", __FILE__, __LINE__, recv_length, len );
						printf("[%s:%d] ------ wisun network dequeue [%d:%d]\r\n", __FILE__, __LINE__, recv_length, len );
						printf("[%s:%d] ------ wisun network dequeue [%d:%d]\r\n", __FILE__, __LINE__, recv_length, len );
						printf("[%s:%d] ------ wisun network dequeue [%d:%d]\r\n", __FILE__, __LINE__, recv_length, len );
						return;
					}

					TB_rssi_set_comm_info( idx, (TBSC)recv_buf[idx_rssi], &recv_buf[idx_mac] );

					TB_KCMVP_DATA in, out;
					TB_KCMVP_KEY tag;
					int offset = 0;

					bzero( &in,  sizeof(in) );
					bzero( &out, sizeof(out) );
					bzero( &tag, sizeof(tag) );

					wmemcpy( tag.data, sizeof(tag.data), &util_buf[offset], LEN_BYTE_TAG );
					tag.size = LEN_BYTE_TAG;
					TB_kcmvp_set_keyinfo_tag( &tag );
					offset += tag.size;

					TB_dbg_wisun("len=%d, 0ffset=%d\r\n", len, offset );
					if( (unsigned int)(len-offset) > sizeof(in.data) )
					{
						TB_dbg_wisun("ERROR. len=%d, 0ffset=%d\r\n", len, offset );
						break;
					}
					
					wmemcpy( in.data, sizeof(in.data), &util_buf[offset], len-offset );
					in.size = len - offset;
					
					TB_kcmvp_decryption( &in, &out );

					if( TB_dm_is_on(DMID_WISUN) )
						TB_util_data_dump( "WISUN RECV", out.data, out.size );

#ifdef	COMM_RATE_TEST
					TBBL is_recv_ack = 0;
					if( TB_wisun_init_complete(idx) == TRUE )
					{
						TB_PACKET *p_packet = (TB_PACKET *)&out.data[0];
						if( TB_packet_check( p_packet ) == TRUE )
						{
							TB_PACKET_HEAD 	*p_head  = (TB_PACKET_HEAD *)&p_packet->data[0];
							TBUC 			*payload = &p_packet->data[sizeof(TB_PACKET_HEAD)];
							TBUS 			 pl_size = p_head->TLENGTH;

							if( TB_wisun_util_ack_check_packet( payload, pl_size ) == TRUE )
							{
								TB_rssi_increment_recv_count( idx, &recv_buf[idx_mac] );
								is_recv_ack = TRUE;
							}
							else
							{
								static TBUC s_ack_data[32] = {0} ;
								static TBUS s_ack_data_len = 0;
								static TBUI s_count = 0;

								s_ack_data_len = 0;
								wmemcpy( &s_ack_data[0], sizeof(s_ack_data), &recv_buf[idx_ip], LEN_IP_ADDR );
								s_ack_data_len += LEN_IP_ADDR;
								s_ack_data_len += TB_wisun_util_ack_make_packet( &s_ack_data[LEN_IP_ADDR] );

								TB_MESSAGE msg;
								if( TB_get_role_is_grpgw() )
								{
									msg.type = MSG_TYPE_WISUN_GGW2TERM;
								}
								else
								{
									msg.type = MSG_TYPE_WISUN_GGW2TERM;
									if( idx == WISUN_FIRST )
									{
										if( TB_setup_get_product_info_voltage() == VOLTAGE_LOW )
											msg.type = MSG_TYPE_WISUN_TERM2RELAY;
									}
								}
								
								msg.id   = MSG_CMD_RECV_DATA_ACK;
								msg.data = s_ack_data;
								msg.size = s_ack_data_len;
								TB_msgq_send( &msg );

								//printf( "[msg send] MSG_CMD_RECV_DATA_ACK. send count = %d\r\n", ++s_count );
							}
						}
					}

					if( is_recv_ack == FALSE )
					{
						TB_wisun_recv_data_parsing( idx, out.data, out.size );
					}
#else
					TB_wisun_recv_data_parsing( idx, out.data, out.size );
#endif
				}
				break;
				
			case (NORT_RCV_DAT_ERR):
				TB_j11_print_recv_data_info( idx, NORT_RCV_DAT_ERR, recv_buf );
				break;
				
			case (RES_UDPSEND):
				if( recv_buf[12] == 0x01 )
				{
					//TB_j11_print_recv_data_info( idx, RES_UDPSEND, recv_buf );
					TB_dbg_wisun("******************>>>>> UDP send Success\r\n" );
				}
				else 
				{
					TB_dbg_wisun("******************>>>>> UDP send Error : 0x%02X\r\n", recv_buf[12] );
				}
				break;

			case RES_PING :
				break;
				
			case NORT_PING :
				TB_j11_print_recv_data_info( idx, NORT_PING, recv_buf );
				if( recv_buf[12] == 0x01 )
					TB_ping_set_notify_success( idx );
				break;
				
			default :
				if( TB_wisun_get_mode( idx ) == WISUN_MODE_PANC )
				{
#ifdef	ENABLE_REENQ
					if( recv_cmd == NORT_CON_STAT_CHANG )
					{
						TB_dbg_netq("re-enq5 : 0x%04X\n", recv_cmd);

						wmemcpy( &reenq_buf[0], sizeof(reenq_buf), &recv_buf[0], recv_length );
						reenq_length = recv_length;
						p_wisun_netq->enq( &p_wisun_netq->netq, reenq_buf, reenq_length );
					}
					else if( recv_cmd == RES_HAN || recv_cmd == RES_PANA || recv_cmd == RES_PANA_SET )
					{
						if( recv_buf[12] == 0x01 )
						{
							TB_dbg_netq("re-enq6 : 0x%04X\n", recv_cmd);

							wmemcpy( &reenq_buf[0], sizeof(reenq_buf), &recv_buf[0], recv_length );
							reenq_length = recv_length;
							p_wisun_netq->enq( &p_wisun_netq->netq, reenq_buf, reenq_length );
						}
					}
#endif
					TB_j11_connect( idx );
				}
				break;
		}

		if( cmd_flag != 0 && cmd_flag == recv_cmd )
			break;

		if( cmd_flag == 0 )
			break;
	}
}

int TB_wisun_proc_mesgq_select_encryption_type2( TB_EMS_BUF *p_ems_send_buf )
{
	int ret = -1;

	if( p_ems_send_buf )
	{
		TBUC *p_data  = p_ems_send_buf->buffer;
		TBUI data_len = p_ems_send_buf->length;

		if( TB_wisun_util_ems_check_packet( p_data, data_len ) )	
		{
			TB_EMS_PACKET packet;
			/*******************************************************************
			*	 		HEAD_EMS_INFO ----- data ---- TAIL_EMS_INFO
			*       		4byte                        4byte
			*    		data_len = head + data + tail
			*******************************************************************/
			if( TB_ems_convert_emsdata2packet( &p_data[4], data_len-8, &packet ) == 0 )
			{
				if( packet.org == FUNC_ORG_AUTH )
				{
					if( packet.ctrl == FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV )
						printf("[%s:%d] send ems-auth : FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV\r\n", __FILE__, __LINE__ );
					TB_kcmvp_set_keyinfo_key_type( KCMVP_KEY_TYPE_MASTER );
				}

				ret = 0;
			}
		}
	}

	return ret;
}

int TB_wisun_proc_mesgq_select_encryption_type( TB_MESSAGE *p_msg )
{
	int ret = -1;

	if( p_msg )
	{
		TBUC *p_data  = (TBUC *)p_msg->data;
		TBUI data_len = p_msg->size;

		if( TB_wisun_util_ems_check_packet( p_data, data_len ) )	
		{
			TB_EMS_PACKET packet;
			/*******************************************************************
			*	 		HEAD_EMS_INFO ----- data ---- TAIL_EMS_INFO
			*       		4byte                        4byte
			*    		data_len = head + data + tail
			*******************************************************************/
			if( TB_ems_convert_emsdata2packet( &p_data[4], data_len-8, &packet ) == 0 )
			{
				if( packet.org == FUNC_ORG_AUTH )
				{
					if( packet.ctrl == FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV )
						printf("[%s:%d] send ems-auth : FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV\r\n", __FILE__, __LINE__ );
					TB_kcmvp_set_keyinfo_key_type( KCMVP_KEY_TYPE_MASTER );
				}

				ret = 0;
			}
		}
	}

	return ret;
}

int TB_wisun_proc_mesgq_groupgw( int idx, TB_MESSAGE *p_msg )
{
	int ret = -1;
	
	if( TB_get_role_is_grpgw() )
	{
		if( p_msg )
		{
			if( TB_wisun_util_ipaddr_res_check_packet( p_msg->data, p_msg->size ) )
			{
				if( TB_dm_is_on(DMID_WISUN) )
					TB_util_data_dump("GROUP G/W IPINFO", p_msg->data, p_msg->size );
			}
			else						
			{
				if( TB_wisun_util_ems_check_packet( p_msg->data, p_msg->size ) )
					TB_wisun_proc_mesgq_select_encryption_type( p_msg );

				TBUC dir = 0xFF;
				switch( p_msg->id )
				{
					case MSG_CMD_WISUN_SEND_GGW2TERM1 : dir = WISUN_DIR_GGW2TERM1;	break;
					case MSG_CMD_WISUN_SEND_GGW2TERM2 : dir = WISUN_DIR_GGW2TERM2;	break;
					case MSG_CMD_WISUN_SEND_GGW2TERM3 : dir = WISUN_DIR_GGW2TERM3;	break;
				}

				if( dir != 0xFF )
				{
					TB_dbg_wisun("Recv Message : MSG_CMD_WISUN_SEND_GGW2TERM1\r\n" );
					TB_packet_set_send_direction( idx, dir );
					TB_packet_send( idx, p_msg->id, p_msg->data, p_msg->size );

					ret = 0;
				}
			}
		}
	}

	return ret;
}

void TB_wisun_proc_mesgq_data( int idx )
{
	TB_MESSAGE 	recv_msg;
	TB_MSG_TYPE msg_type;

	if( TB_get_role_is_grpgw() )
	{
		msg_type = MSG_TYPE_WISUN_GGW2TERM;
	}
	else
	{
		msg_type = MSG_TYPE_WISUN_GGW2TERM;
		if( idx == WISUN_FIRST )
		{
			if( TB_setup_get_product_info_voltage() == VOLTAGE_LOW )			
				msg_type = MSG_TYPE_WISUN_TERM2RELAY;
		}
	}
	
	if( TB_msgq_recv( &recv_msg, msg_type, NOWAIT) > 0 )
	{
		TB_dbg_wisun("Recv Message. 0x%X:0x%X\r\n", msg_type, recv_msg.id );
		
		switch( recv_msg.id )
		{
#ifdef	COMM_RATE_TEST
			case MSG_CMD_RECV_DATA_ACK :
				if( TB_wisun_init_complete(idx) == TRUE )
				{
					TB_PACKET packet;
					TBUC *payload = (TBUC *)recv_msg.data;
					//	|----------|-----------|-----------|
					//	|  dest ip |ack-header |ack-tail   |
					//	|----------|-----------|-----------|
					//	|0| ~~~ |15|16|17|18|19|20|21|22|23|
					//	|----------|-----------|-----------|

					if( TB_packet_make( &packet, 0, &payload[LEN_IP_ADDR], 8, 8, 1, 0 ) == 0 )
					{
						TB_packet_make_kcmvp( &packet );
						TB_j11_send_recv_data_ack( idx, &payload[0], packet.data, packet.size );
					}
				}
				break;
#endif

			case MSG_CMD_WISUN_SEND_TERM2GGW	:
				if( TB_get_role_is_terminal() )
				{
					TB_dbg_wisun("Recv Message : MSG_CMD_WISUN_SEND_TERM2GGW\r\n" );
					TB_wisun_proc_mesgq_select_encryption_type( &recv_msg );
					TB_packet_set_send_direction( idx, WISUN_DIR_TERM2GGW );
					TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2GGW, recv_msg.data, recv_msg.size );
				}
				break;

			case MSG_CMD_WISUN_SEND_TERM2RELAY1	:
				if( TB_get_role_is_terminal() )
				{
					TB_dbg_wisun("Recv Message : MSG_CMD_WISUN_SEND_TERM2RELAY1\r\n");
					if( TB_j11_check_connect_relay(idx, ROLE_RELAY1) )
					{
						TB_wisun_proc_mesgq_select_encryption_type( &recv_msg );
						TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY1 );
						TB_packet_send( idx, recv_msg.id, recv_msg.data, recv_msg.size );
					}
					else
					{
						TB_dbg_wisun("Ignore Message : MSG_CMD_WISUN_SEND_TERM2RELAY1\r\n" );
						TB_dbg_wisun("RELAY1 - end device is not Connected\r\n" );
					}
				}
				break;

			case MSG_CMD_WISUN_SEND_TERM2RELAY2	:
				if( TB_get_role_is_terminal() )
				{
					TB_dbg_wisun("Recv Message : MSG_CMD_WISUN_SEND_TERM2RELAY2\r\n" );
					if( TB_j11_check_connect_relay(idx, ROLE_RELAY2) )
					{
						TB_wisun_proc_mesgq_select_encryption_type( &recv_msg );
						TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY2 );
						TB_packet_send( idx, recv_msg.id, recv_msg.data, recv_msg.size );
					}
					else
					{
						TB_dbg_wisun("Ignore Message : MSG_CMD_WISUN_SEND_TERM2RELAY2\r\n" );
						TB_dbg_wisun("RELAY2 - end device is not Connected\r\n" );
					}
				}
				break;
				
			case MSG_CMD_WISUN_SEND_TERM2RELAY3	:
				if( TB_get_role_is_terminal() )
				{
					TB_dbg_wisun("Recv Message : MSG_CMD_WISUN_SEND_TERM2RELAY3\r\n");
					if( TB_j11_check_connect_relay(idx, ROLE_RELAY3) )
					{
						TB_wisun_proc_mesgq_select_encryption_type( &recv_msg );
						TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY3 );
						TB_packet_send( idx, recv_msg.id, recv_msg.data, recv_msg.size );
					}
					else
					{
						TB_dbg_wisun("Ignore Message : MSG_CMD_WISUN_SEND_TERM2RELAY3\r\n" );
						TB_dbg_wisun("RELAY3 - end device is not Connected\r\n" );
					}					
				}
				break;
				
			case MSG_CMD_WISUN_SEND_RELAY2TERM	:
				if( TB_get_role_is_relay() )
				{
					if( recv_msg.size == 25 )
					{
						if( TB_dm_is_on(DMID_WISUN) )
							TB_util_data_dump( "RELAY IPINFO", recv_msg.data, recv_msg.size );
					}
					else
					{
						TB_dbg_wisun("Recv Message : MSG_CMD_WISUN_SEND_RELAY2TERM\r\n" );
						
						TB_wisun_proc_mesgq_select_encryption_type( &recv_msg );						
						TB_packet_set_send_direction( idx, WISUN_DIR_RELAY2TERM );
						TB_packet_send( idx, MSG_CMD_WISUN_SEND_RELAY2TERM, recv_msg.data, recv_msg.size );
					}
				}
				break;

			case MSG_CMD_WISUN_SEND_GGW2TERM1	:
			case MSG_CMD_WISUN_SEND_GGW2TERM2	:
			case MSG_CMD_WISUN_SEND_GGW2TERM3 	:
				TB_wisun_proc_mesgq_groupgw( idx, &recv_msg );
				break;

			////////////////////////////////////////////////////////////////////
			
			case MSG_CMD_WISUN_INVERTER_DATA_RES :
				TB_dbg_wisun("Recv Message : MSG_CMD_WISUN_INVERTER_DATA_RES\r\n" );
				if( TB_dm_is_on(DMID_WISUN) )
					TB_util_data_dump1( recv_msg.data, recv_msg.size );
				
				if( TB_get_role_is_relay() )
				{					
					TB_packet_set_send_direction( idx, WISUN_DIR_RELAY2TERM );
					TB_packet_send( idx, MSG_CMD_WISUN_SEND_RELAY2TERM, recv_msg.data, recv_msg.size );
				}
				else if( TB_get_role_is_terminal() )
				{
					TB_packet_set_send_direction( idx, WISUN_DIR_TERM2GGW  );
					TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2GGW, recv_msg.data, recv_msg.size );
				}
				
				/***************************************************************
				* 	CAUTION3. Wisun에서 Message를 수신 후 상위 Wisun으로 전송 후
				*			  결과를 하위 Wisun으로 송신한다.
				***************************************************************/
				{
					TB_MESSAGE 	send_msg;
					send_msg.type 	= MSG_TYPE_INVERTER;
					send_msg.id	 	= MSG_CMD_INVERTER_WISUN_READY;
					send_msg.data	= NULL;
					send_msg.size	= 0;
					TB_msgq_send( &send_msg );

					TB_dbg_wisun("Send Message : MSG_TYPE_INVERTER : MSG_CMD_INVERTER_WISUN_READY\r\n" );
				}
				break;

			default :
				TB_dbg_wisun("ERROR. Unknown Message id : 0x%02X\r\n", recv_msg.id );
				break;
		}
	}
}

static TBBL s_wisun_init_complete[2] = {FALSE, FALSE};
TBBL TB_wisun_init_complete( int idx )
{
	return s_wisun_init_complete[idx];
}

static void *tb_wisun_proc_first( void *arg )
{
	int idx = 0;
	if( arg )
	{
		idx = *(int*)arg;
	}
	
	TB_WISUN_MODE 	mode = TB_wisun_get_mode( idx );
	TB_ROLE 		role = TB_get_role();

	TB_dbg_wisun( "====================================\r\n" );
	TB_dbg_wisun( "      START wisun Proc # %d : %s    \r\n", idx, __FUNCTION__ );
	TB_dbg_wisun( "====================================\r\n" );

	s_wisun_init_complete[idx] = TRUE;

	if( TB_get_role_is_relay() )
	{
		TB_wisun_util_timeinfo_req_send( TRUE, __FILE__, __LINE__ );
	}
	else if( TB_get_role_is_grpgw() )
	{
		TB_wisun_util_timeinfo_req_send( FALSE, __FILE__, __LINE__ );
	}
	else if( TB_get_role_is_terminal() )
	{
		TB_wisun_util_timeinfo_req_send_avaliable_timeinfo( idx, role, mode );
	}

	TB_ping_init( idx );
	TB_led_wisun_work( idx );	

#define LOOP_DELAY	(500)
	while( s_proc_flag_wisun )
	{
		TB_ping_send( idx, FALSE );

		/***********************************************************************
		*						Internal Message
		***********************************************************************/
   		TB_wisun_proc_mesgq_data( idx );

		/***********************************************************************
		*						External Message
		***********************************************************************/
		TB_wisun_proc_netq_data( idx, 0 );

		if( TB_ping_check_notify_success(idx) != 0 )
		{
			TB_wdt_reboot( WDT_COND_PING_FAIL );
		}
		
		TB_util_delay( LOOP_DELAY );

		/***********************************************************************
		*	WiSUN 통신 연결 완료 시 end device의 갯수와 RSSI 처리루틴에서 
		*	계산한 연결된 하위 장치의 갯수가 다를 경우 리부팅한다.
		*	--> PANC에서 end device 갯수를 비교한다.
		*	--> 1분 여유의 이유는 초기 연결완료 시 c2의 값이 0이기 때문이다.
		***********************************************************************/
		if( TB_get_role_is_grpgw() )
		{
			TBUS c1 = TB_rssi_get_connection_count( idx );
			TBUS c2 = TB_j11_get_connection_count( idx );
			if( c2 > 0 )
			{				
				if( c1 != c2 )
				{
					static int s_diff_cnt = 0;
					s_diff_cnt++;
					if( s_diff_cnt > (int)(_2MIN_MSEC/LOOP_DELAY) )
					{
						TB_dbg_wisun("Connection does't finished. c1=%d, c2=%d, cnt=%d\r\n", c1, c2, s_diff_cnt );
						
						TB_log_append( RESID_LOG_COMM_STATE_CHANGED, NULL, -1 );
						TB_wdt_reboot( WDT_COND_COMM_STATE_CHANGED );
					}
					else
					{
						TB_dbg_wisun("Connection state changed. Ready rebooting.... : cnt=%d, max_cnt=%d\r\n",\
									s_diff_cnt, (int)(_2MIN_MSEC/LOOP_DELAY) );
					}
				}
			}
		}
	}
}

static void *tb_wisun_proc_second( void *arg )
{
	int idx = 0;
	if( arg )
	{
		idx = *(int*)arg;
	}
	
	TB_WISUN_MODE 	mode = TB_wisun_get_mode( idx );
	TB_ROLE 		role = TB_get_role();	

	TB_dbg_wisun( "====================================\r\n" );
	TB_dbg_wisun( "      START wisun Proc # %d : %s\r\n", idx, __FUNCTION__ );
	TB_dbg_wisun( "====================================\r\n" );

	s_wisun_init_complete[idx] = TRUE;

	TB_ping_init( idx );
	TB_led_wisun_work( idx );	

	while( s_proc_flag_wisun )
	{
		TB_ping_send( idx, FALSE );
		
		/***********************************************************************
		*						Internal Message
		***********************************************************************/
		TB_wisun_proc_mesgq_data( idx );

		/***********************************************************************
		*						External Message
		***********************************************************************/
		TB_wisun_proc_netq_data( idx, 0 );

		if( TB_ping_check_notify_success(idx) != 0 )
		{
			TB_wdt_reboot( WDT_COND_PING_FAIL );
		}
		
		TB_util_delay( 500 );
	}
}

/*******************************************************************************
*						Group G/W(PANC) <----> Repeater(COORD)
*******************************************************************************/
static void *tb_wisun_proc_ggw2repeater( void *arg )
{
	int idx = 0;
	if( arg )
	{
		idx = *(int*)arg;
	}
	
	TB_WISUN_MODE mode = TB_wisun_get_mode( idx );
	TB_ROLE role = TB_get_role();

	TB_dbg_wisun( "====================================\r\n" );
	TB_dbg_wisun( "      START wisun Proc # %d : %s\r\n", idx, __FUNCTION__ );
	TB_dbg_wisun( "====================================\r\n" );

	TB_ping_init( idx );
	TB_led_wisun_work( idx );

	while( s_proc_flag_wisun )
	{
		TB_ping_send( idx, FALSE );
		
		/***********************************************************************
		*						External Message
		***********************************************************************/
		TB_wisun_proc_netq_data( idx, 0 );

		if( TB_ping_check_notify_success(idx) != 0 )
		{
			TB_wdt_reboot( WDT_COND_PING_FAIL );
		}
		
		TB_util_delay( 500 );
	}
}

void TB_wisun_proc_start( int idx )
{
	printf( "[%s:%d] CONNECTED IP\r\n", __FILE__, __LINE__ );
#if 0
	for( int i=0; i<4; i++)
	{
		TBUC *data = (TBUC *)&g_ip_adr[idx][i][0];
		TB_util_data_dump2( data, LEN_IP_ADDR );
	}
#endif
	////////////////////////////////////////////////////////////////////////////

	static int wisun_idx = 0;
	if( idx == 0 )
	{
		wisun_idx = idx;
		if( pthread_create( &s_thid_wisun[0], NULL, tb_wisun_proc_first, (void*)&wisun_idx ) < 0 )
		{
			TB_dbg_wisun("ERROR. thread create.......%d, %s\r\n", errno, strerror(errno) );
		}
	}
	else if( idx == 1 )
	{
		wisun_idx = idx;
		if( pthread_create( &s_thid_wisun[1], NULL, tb_wisun_proc_second, (void*)&wisun_idx ) < 0 )
		{
			TB_dbg_wisun("ERROR. thread create.......%d, %s\r\n", errno, strerror(errno) );
		}
	}
	else
	{
		wisun_idx = 0;
		if( pthread_create( &s_thid_wisun[0], NULL, tb_wisun_proc_ggw2repeater, (void*)&wisun_idx ) < 0 )
		{
			TB_dbg_wisun("ERROR. thread create.......%d, %s\r\n", errno, strerror(errno) );
		}
	}
}

void TB_wisun_proc_stop( void )
{
	s_proc_flag_wisun = 0;
	pthread_join( s_thid_wisun[0], NULL );
	pthread_join( s_thid_wisun[1], NULL );
}

