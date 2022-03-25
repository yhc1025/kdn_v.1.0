#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "TB_util.h"
#include "TB_uart.h"
#include "TB_sys_gpio.h"
#include "TB_sys_wdt.h"
#include "TB_msg_queue.h"
#include "TB_modbus.h"
#include "TB_wisun.h"
#include "TB_j11.h"
#include "TB_ui.h"
#include "TB_debug.h"
#include "TB_setup.h"
#include "TB_msgbox.h"
#include "TB_modem.h"

#include "TB_psu.h"

////////////////////////////////////////////////////////////////////////////////

TBBL g_psu_get_time	= FALSE;

////////////////////////////////////////////////////////////////////////////////

typedef struct tagPSU_BUFF
{
	TBUC buffer[MAX_DNP_BUF];
	TBUS length;
} TB_PSU_BUF;

////////////////////////////////////////////////////////////////////////////////

static TB_PSU_BUF s_buf_mod;
static TB_PSU_BUF s_buf_dnp;

static	int	s_idx_mod = 0;
static	int	s_idx_dnp = 1;

////////////////////////////////////////////////////////////////////////////////

int TB_psu_mod_init( void )
{
	int ret = -1;
	int init = 0;

	TB_ROLE role = TB_setup_get_role();
	if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
	{
		TB_COMM_TYPE comm_type = TB_setup_get_comm_type();
		if( comm_type.slave == COMM_MODE_SLAVE_WISUN )
		{
			init = 1;
		}
		else if( comm_type.slave == COMM_MODE_SLAVE_RS485 )
		{
			init = 1;
		}
	}

	////////////////////////////////////////////////////////////////////////////

	if( init )
	{
		if( TB_uart_psu_mod_init() == 0 )
		{
			int loop = 1;
			int check = 0;
			int retry = 0;
			int timeout = 5;
			
			time_t time_start;
			time_t time_curr;

			int i;
			for( i=0; i<5; i++ )
			{
				time_start = time( NULL );
				time_curr = time_start;

				TB_psu_mod_write_get_time_command();
				loop = 1;
				while( loop )
				{
					TB_util_delay( 500 );
					
					if( time_start + timeout > time_curr )
					{
						TBUS len = TB_psu_mod_read();
						if( len > 0 )
						{
							if( s_buf_mod.buffer[0] == PSU_MODBUS_TIME_SLAVEID &&
								s_buf_mod.buffer[1] == PSU_MODBUS_TIME_FUNC )
							{
								TB_psu_mod_set_system_time( s_buf_mod.buffer, s_buf_mod.length );
								g_psu_get_time = TRUE;

								check = 1;
								loop = 0;
							}
						}
					}
					else
					{
						retry ++;
						loop = 0;

						printf("[%s:%d] ERROR. retry = %d\r\n", __FILE__, __LINE__, retry );
					}

					time_curr = time( NULL );
				}				

				if( check == 1 )
				{
					break;
				}
			}

			if( check == 0 )
			{
				printf("[%s:%d] ERROR. Time Set\r\n", __FILE__, __LINE__ );
			}
			else
			{
				if( retry > 0 )
					printf("[%s:%d] Time Set : Retry count = %d\r\n", __FILE__, __LINE__, retry );
			}

			////////////////////////////////////////////////////////////////////

			TB_psu_mod_proc_start();
			ret = 0;
		}

	}

	return ret;
}

TBUS TB_psu_mod_read( void )
{
	s_buf_mod.length = 0;
	s_buf_mod.length = TB_uart_psu_mod_read( s_buf_mod.buffer, sizeof(s_buf_mod.buffer) );
	if( s_buf_mod.length > 0 )
	{
		if( TB_dm_is_on(DMID_PSU) )
			TB_util_data_dump( "PSU-MODBUS Read", s_buf_mod.buffer, s_buf_mod.length );
	}
	
	return s_buf_mod.length;
}

TBUS TB_psu_mod_write( TBUC *buf, int len )
{
	return TB_uart_psu_mod_write( buf, len );
}

void TB_psu_mod_deinit( void )
{
	TB_psu_mod_proc_stop();
	TB_uart_psu_mod_deinit();
}

int TB_psu_mod_set_system_time( TBUC *p_data, TBUC size )
{
	int ret = -1;
	
	if( p_data && size > 0 )
	{
		TBUC time_packet[32] = {0, };

		if( TB_dm_is_on(DMID_PSU) )
			TB_util_data_dump( "<<<<< RES-TIME TO PSU-MODBUS", p_data, size );

		bzero( time_packet,  sizeof(time_packet) );
		wmemcpy( time_packet, sizeof(time_packet), p_data, size );

		TBUS YY;
		TBUS MM;
		TBUS DD;
		TBUS hh;
		TBUS mm;
		TBUS ss;

		int idx  		= 2;
		int inc_value 	= 2;
		TBUC temp[3];

		YY = time_packet[idx]<<8 | time_packet[idx+1];		idx += inc_value;
		MM = time_packet[idx]<<8 | time_packet[idx+1];		idx += inc_value;
		DD = time_packet[idx]<<8 | time_packet[idx+1];		idx += inc_value;
		hh = time_packet[idx]<<8 | time_packet[idx+1];		idx += inc_value;
		mm = time_packet[idx]<<8 | time_packet[idx+1];		idx += inc_value;
		ss = time_packet[idx]<<8 | time_packet[idx+1];		idx += inc_value;

		////////////////////////////////////////////////////////////////////////

		TB_util_set_systemtime1( YY, MM, DD, hh, mm, ss );
		ret = 0;
	}

	return ret;
}

int TB_psu_mod_write_get_time_command( void )
{
	static int s_check = 0;
	if( s_check == 0 )
	{
		TBUC cmd[32];
		
		cmd[0] = PSU_MODBUS_TIME_SLAVEID;
		cmd[1] = PSU_MODBUS_TIME_FUNC;
		TB_crc16_modbus_fill( cmd, 2 );

		if( TB_dm_is_on(DMID_PSU) )
			TB_util_data_dump( ">>>>> REQ-TIME TO PSU-MODBUS", cmd, 4 );

		TB_psu_mod_write( cmd, 4 );

		s_check = 1;
	}

	return 0;
}

int TB_psu_mod_action_wisun( TB_PSU_BUF *p_buf_mod )
{
	int ret = -1;

	if( p_buf_mod && p_buf_mod->length > 0 )
	{
		TBUC slv_flag = 0;

		TB_MESSAGE send_msg;

		send_msg.type = MSG_TYPE_WISUN_TERM2RELAY;
		send_msg.id	  = 0;
		send_msg.data = p_buf_mod->buffer;
		send_msg.size = p_buf_mod->length;

		/*******************************************************
		* 	Slave ID가 0x00이면 그룹제어로 모든 Relay로 전송.
		*******************************************************/
		switch( p_buf_mod->buffer[0] )
		{
			case SLAVEID_RELAY1 	: 	slv_flag = SLAVEID_FLAG_RELAY1;		break;
			case SLAVEID_RELAY2 	: 	slv_flag = SLAVEID_FLAG_RELAY2;		break;
			case SLAVEID_RELAY3 	: 	slv_flag = SLAVEID_FLAG_RELAY3;		break;
			case SLAVEID_RELAY_ALL 	: 	slv_flag = SLAVEID_FLAG_RELAY_ALL;	break;
			default	  				: 	TB_dbg_psu( "Unknown PSU-MODBUS data : 0x%02X\r\n", p_buf_mod->buffer[0] );	break;
		}

		TB_dbg_psu( "PSU-MODBUS data : 0x%02X, slv_flag = 0x%02X\r\n", p_buf_mod->buffer[0], slv_flag );
		
		if( slv_flag & SLAVEID_FLAG_RELAY1 )
		{
			send_msg.id = MSG_CMD_WISUN_SEND_TERM2RELAY1;
			TB_dbg_psu( "PSU-MODBUS Send to WiSUN >>>>>>>>>>> MSG_CMD_WISUN_SEND_TERM2RELAY1\r\n" );
			TB_msgq_send( &send_msg );
		}
		
		if( slv_flag & SLAVEID_FLAG_RELAY2 )
		{
			send_msg.id = MSG_CMD_WISUN_SEND_TERM2RELAY2;
			TB_dbg_psu( "PSU-MODBUS Send to WiSUN >>>>>>>>>>> MSG_CMD_WISUN_SEND_TERM2RELAY2\r\n" );
			TB_msgq_send( &send_msg );
		}
		
		if( slv_flag & SLAVEID_FLAG_RELAY3 )
		{
			send_msg.id = MSG_CMD_WISUN_SEND_TERM2RELAY3;
			TB_dbg_psu( "PSU-MODBUS Send to WiSUN >>>>>>>>>>> MSG_CMD_WISUN_SEND_TERM2RELAY3\r\n" );
			TB_msgq_send( &send_msg );
		}

		ret = 0;
	}

	return ret;
}

int TB_psu_mod_action_rs485( TB_PSU_BUF *p_buf_mod )
{
	int ret = -1;

	if( p_buf_mod && p_buf_mod->length > 0 )
	{
		TB_MESSAGE send_msg;
		
		TBUC *payload  = p_buf_mod->buffer;
		TBUS pl_size   = p_buf_mod->length;

		/***********************************************************************
		*			광센서는 단말장치의 RS485B포트에만 연결이 된다.
		***********************************************************************/
		if( payload[0] == 0x01 && payload[1] == 0x04 )
		{
			send_msg.type    = MSG_TYPE_OPTICAL;
			send_msg.id      = MSG_CMD_OPTICAL_WRITE;
			send_msg.data	 = payload;
			send_msg.size    = pl_size;

			TB_msgq_send( &send_msg );
		}
		else if( payload[0] == 0x64 || payload[0] == 0x00 )
		{
			switch( payload[1] )
			{
				case FUNC_DOUBLE_READ_REC_REG_IMMEDI :
					{
						TBUS ivt_addr = (payload[3] << 8) | payload[2];
						TBUS reg_addr = (payload[4] << 8) | payload[5];
						TBUS reg_cont = (payload[6] << 8) | payload[7];

						if( reg_addr >= 1000 && reg_addr < 2000 )
						{
							send_msg.type    = MSG_TYPE_INVERTER;
							send_msg.id      = FUNC_DOUBLE_READ_REC_REG_IMMEDI;
							send_msg.data	 = payload;
							send_msg.size    = pl_size;

							TB_msgq_send( &send_msg );
						}
					}
					break;
					
				case FUNC_DOUBLE_READ_REC_REG :
					{
						TBUS ivt_addr = (payload[3] << 8) | payload[2];
						TBUS reg_addr = (payload[4] << 8) | payload[5];
						TBUS reg_cont = (payload[6] << 8) | payload[7];

						if( reg_addr >= 1000 && reg_addr < 2000 )
						{
							send_msg.type    = MSG_TYPE_INVERTER;
							send_msg.id      = FUNC_DOUBLE_READ_REC_REG;
							send_msg.data	 = payload;
							send_msg.size    = pl_size;

							TB_msgq_send( &send_msg );
						}
					}
					break;

				case FUNC_READ_HOLDING_REG :
					{
						TBUS reg_addr = (payload[2] << 8) | payload[3];
						if( reg_addr >= 1000 && reg_addr < 2000 )
						{
							send_msg.type    = MSG_TYPE_INVERTER;
							send_msg.id      = FUNC_READ_HOLDING_REG;
							send_msg.data	 = payload;
							send_msg.size    = pl_size;

							TB_msgq_send( &send_msg );
						}
						else if( reg_addr >= 6000 && reg_addr < 7000 )	//	0x1770 : 6000
						{
							TB_dbg_psu("--------> Request to configuration value.\r\n" );
							
							/***********************************
							*			설정값 요청
							***********************************/
							TB_SETUP_INVT *p_invt = TB_setup_get_invt_info();
							if( p_invt )
							{
								TBUC buf[128];
								int idx = 0;

								buf[idx++] = 0x64;	//	Slave ID of Relay
								buf[idx++] = FUNC_READ_HOLDING_REG;
								buf[idx++] = 0x08;
								buf[idx++] = (p_invt->cnt     >> 8) & 0xFF;
								buf[idx++] = (p_invt->cnt     >> 0) & 0xFF;
								buf[idx++] = (p_invt->retry   >> 8) & 0xFF;
								buf[idx++] = (p_invt->retry   >> 0) & 0xFF;
								buf[idx++] = (p_invt->timeout >> 8) & 0xFF;
								buf[idx++] = (p_invt->timeout >> 0) & 0xFF;
								buf[idx++] = 0;	//	Reserved
								buf[idx++] = 0;	//	Reserved
								TB_crc16_modbus_fill( buf, idx );
								idx += 2;

								TB_dbg_psu("<-------- Response to configuration value.\r\n" );
								TB_psu_mod_write( buf, idx );
							}
						}
					}
					break;
					
				case FUNC_WRITE_SINGLE_REG :
					{
						TBUS reg_addr  = (payload[2] << 8) | payload[3];
						TBUS reg_value = (payload[4] << 8) | payload[5];

						if( reg_addr >= 2100 && reg_addr < 3000 )
						{
							send_msg.type    = MSG_TYPE_INVERTER;
							send_msg.id      = FUNC_WRITE_SINGLE_REG;
							send_msg.data	 = payload;
							send_msg.size    = pl_size;

							TB_msgq_send( &send_msg );
						}
					}
					break;
					
				case FUNC_WRITE_MULTIPLE_REG :
					{
						TBUS reg_addr = (payload[2] << 8) | payload[3];
						TBUS reg_cont = (payload[4] << 8) | payload[5];
						TBUS byte_cont = payload[6];

						if( reg_addr >= 2000 && reg_addr < 3000 )
						{
							send_msg.type    = MSG_TYPE_INVERTER;
							send_msg.id      = FUNC_WRITE_MULTIPLE_REG;
							send_msg.data	 = payload;
							send_msg.size    = pl_size;

							TB_msgq_send( &send_msg );
						}
						else if( reg_addr >= 6000 && reg_addr < 7000 )
						{
							/***********************************
							*			설정값 변경
							***********************************/
							TB_SETUP_INVT invt;

							wmemcpy( &invt, sizeof(invt), TB_setup_get_invt_info(), sizeof(TB_SETUP_INVT) );

							TB_dbg_psu("--------> Request to change the configuration value.\r\n" );
							TB_dbg_psu("        Inverter count   = %d\r\n", invt.cnt );
							TB_dbg_psu("        Inverter retry   = %d\r\n", invt.retry );
							TB_dbg_psu("        Inverter timeout = %d\r\n", invt.timeout );

							TBUS cnt		= (payload[7]  << 8) | payload[8];
							TBUS retry		= (payload[9]  << 8) | payload[10];
							TBUS timeout	= (payload[11] << 8) | payload[12];
							if( timeout >= 100 && timeout <= 5000 )	//	 100 ~ 5000
								timeout /= 100;	//	   1 ~ 50
							else
								timeout = SET_DEF_INVT_TOUT_DEFAULT;

							invt.cnt 	 = cnt;
							invt.retry 	 = retry;
							invt.timeout = timeout;

							if( invt.cnt     <= SET_DEF_INVT_CNT_MAX 	&&	\
								invt.retry   <= SET_DEF_INVT_RETRY_MAX 	&&	\
								invt.timeout <= SET_DEF_INVT_TOUT_MAX )
							{
								TB_ui_key_set_force_block( TRUE );
								TB_show_msgbox( TB_msgid_get_string(MSGID_INVERTER_CHANGE), 3, FALSE );

								TB_setup_set_invt_info( &invt );
								TB_setup_save();

								TB_dbg_psu("<-------- Response to change the configuration value.\r\n" );
								TB_dbg_psu("        Inverter count   = %d\r\n", invt.cnt );
								TB_dbg_psu("        Inverter retry   = %d\r\n", invt.retry );
								TB_dbg_psu("        Inverter timeout = %d\r\n", invt.timeout );
								
								/***********************************
								*		설정값 변경 후 응답
								***********************************/
								TBUC buf[128];
								int idx = 0;
								
								buf[idx++] = 0x64;
								buf[idx++] = FUNC_WRITE_MULTIPLE_REG;
								buf[idx++] = 0x17;	//	(reg_addr >> 8) & 0xFF;
								buf[idx++] = 0x70;	//	(reg_addr >> 0) & 0xFF;
								buf[idx++] = 0x00;
								buf[idx++] = 0x04;
								TB_crc16_modbus_fill( buf, idx );
								idx += 2;

								TB_psu_mod_write( buf, idx );

								TB_util_delay( 3000 );
								TB_wdt_reboot( WDT_COND_CHANGE_CONFIG );
							}
						}
					}
					break;

				default	:
					TB_dbg_psu( "Error. Invalid Function code : 0x%02X\r\n", payload[1] );
					break;
			}

			ret = 0;
		}
	}

	return ret;
}

TBUC s_proc_flag_psu_modbus = 0;
static void *tb_psu_mod_proc( void *arg )
{
	int 		run = FALSE;
	TB_MESSAGE 	msg;

	TB_COMM_TYPE comm_type 	= TB_setup_get_comm_type();
	TB_VOLTAGE 	 vol 		= TB_setup_get_product_info_voltage();

	if( comm_type.slave == COMM_MODE_SLAVE_WISUN )
	{
		TB_WISUN_MODE	mode = TB_wisun_get_mode( s_idx_mod );
		TB_ROLE 		role = TB_setup_get_role();
		if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
		{
			while( 1 )
			{
				if( TB_j11_init_complete(s_idx_mod) == TRUE )
				{
					run = 1;
					break;
				}

				TB_util_delay( 500 );
			}
		}
	}
	else if( comm_type.slave == COMM_MODE_SLAVE_RS485 )
	{
		run = 1;
	}
	else if( vol == VOLTAGE_HIGH )
	{
		run = 1;
	}

	////////////////////////////////////////////////////////////////////////////

	if( run )
	{
		printf("===========================================\r\n" );
		printf("           Start PSU-MODBUS Proc           \r\n" );
		printf("===========================================\r\n" );

		s_proc_flag_psu_modbus = 1;
		while( s_proc_flag_psu_modbus )
		{
			TB_MESSAGE send_msg;
			TB_MESSAGE recv_msg;

			TBUS len = TB_psu_mod_read();
			if( len > 0 )
			{
				if( TB_crc16_modbus_check( s_buf_mod.buffer, s_buf_mod.length ) == FALSE )
				{
					TB_dbg_psu( "******** ERROR. PSU-MODBUS CRC *******\r\n" );
					if( TB_dm_is_on(DMID_PSU) )
						TB_util_data_dump( "ERROR. PSU-MODBUS CRC", s_buf_mod.buffer, s_buf_mod.length );

					continue;
				}

				////////////////////////////////////////////////////////////////

				if( TB_dm_is_on(DMID_PSU) )
					TB_util_data_dump( "Read from PSU-MOD", s_buf_mod.buffer, s_buf_mod.length );

				////////////////////////////////////////////////////////////////

				if( vol == VOLTAGE_HIGH )
				{
					TB_psu_mod_action_rs485( &s_buf_mod );
				}
				else
				{
					/***********************************************************
					*	광센서는 단말장치의 RS485B포트에만 연결이 된다.
					***********************************************************/
					if( s_buf_mod.buffer[0] == 0x01 && s_buf_mod.buffer[1] == 0x04 )
					{
						send_msg.type    = MSG_TYPE_OPTICAL;
						send_msg.id      = MSG_CMD_OPTICAL_WRITE;
						send_msg.data	 = s_buf_mod.buffer;
						send_msg.size    = s_buf_mod.length;

						TB_msgq_send( &send_msg );
					}
					else
					{
						if( comm_type.slave == COMM_MODE_SLAVE_WISUN )
						{
							TB_psu_mod_action_wisun( &s_buf_mod );
						}
						else if( comm_type.slave == COMM_MODE_SLAVE_RS485 )
						{
							TB_psu_mod_action_rs485( &s_buf_mod );
						}
					}
				}
			}

			////////////////////////////////////////////////////////////////////
		
			if( TB_msgq_recv( &recv_msg, MSG_TYPE_PSU_MOD, NOWAIT ) > 0 )
			{
				if( recv_msg.id == MSG_CMD_PSU_MOD_WRITE	||
					recv_msg.id == MSG_CMD_PSU_MOD_REQ_TIME )
				{
					TB_dbg_psu( "Write to PSU-MODBUS : 0x%04X : %d\r\n", recv_msg.id, recv_msg.size );
					if( TB_dm_is_on(DMID_PSU) )
						TB_util_data_dump( "PSU-MODBUS Write", recv_msg.data, recv_msg.size );
					TB_psu_mod_write( recv_msg.data, recv_msg.size );
				}
				
				////////////////////////////////////////////////////////////////

				if( comm_type.master == COMM_MODE_MASTER_RS232 || vol == VOLTAGE_HIGH )
				{
					/***********************************************************
					* 	CAUTION2. PSU-MODBUS에서 Message를 수신 후
					*			  Write를 마친 후 결과를 송신한다.
					***********************************************************/
					if( recv_msg.id == MSG_CMD_PSU_MOD_WRITE )
					{
						send_msg.type   = MSG_TYPE_PSU_MOD_READY;
						send_msg.id     = MSG_CMD_PSU_MOD_WRITE_READY;
						send_msg.data   = NULL;
						send_msg.size   = 0;
						TB_msgq_send( &send_msg );
					}
				}
			}
		}
	}
}

pthread_t s_thid_psu_modbus;
void TB_psu_mod_proc_start( void )
{
	pthread_create( &s_thid_psu_modbus, NULL, tb_psu_mod_proc, NULL );
}

void TB_psu_mod_proc_stop( void )
{
	s_proc_flag_psu_modbus = 0;
	pthread_join( s_thid_psu_modbus, NULL );
}

////////////////////////////////////////////////////////////////////////////////

int TB_psu_dnp_init( void )
{
	int ret = -1;
	if( TB_uart_psu_dnp_init() == 0 )
	{
		TB_psu_dnp_proc_start();
		ret = 0;
	}
	
	return ret;
}

TBUS TB_psu_dnp_read( void )
{
	s_buf_dnp.length = TB_uart_psu_dnp_read( s_buf_dnp.buffer, sizeof(s_buf_dnp.buffer) );
	return s_buf_dnp.length;
}

TBUS TB_psu_dnp_write( TBUC *buf, int len )
{
	return TB_uart_psu_dnp_write( buf, len );
}

void TB_psu_dnp_deinit( void )
{
	TB_psu_dnp_proc_stop();
	TB_uart_psu_dnp_deinit();
}

TBUC s_proc_flag_psu_dnp = 0;
static void *tb_psu_dnp_proc( void *arg )
{
	TB_MESSAGE 		msg;
	TB_WISUN_MODE 	mode = TB_wisun_get_mode( s_idx_dnp );
	TB_ROLE 		role = TB_setup_get_role();
	
	if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )	
	{
		printf("===========================================\r\n" );
		printf("            Start PSU-DNP Proc             \r\n" );
		printf("===========================================\r\n" );

		s_proc_flag_psu_dnp = 1;
		while( s_proc_flag_psu_dnp )
		{
			TB_MESSAGE msg_send;
			TB_MESSAGE msg_recv;

			if( TB_psu_dnp_read() > 0 )
			{
				if( TB_dm_is_on( DMID_PSU ) )
					TB_util_data_dump( "PSU-DNP Read", s_buf_dnp.buffer, s_buf_dnp.length );

				TB_COMM_TYPE comm_type = TB_setup_get_comm_type();
#if 1
				if( comm_type.master == COMM_MODE_MASTER_RS232 )
				{
					TB_modem_write( s_buf_dnp.buffer, s_buf_dnp.length );
				}
				else if( comm_type.master == COMM_MODE_MASTER_LTE )
				{
					msg_send.type = MSG_TYPE_LTE;
					msg_send.id	  = MSG_CMD_LTE_WRITE;
					msg_send.data = s_buf_dnp.buffer;
					msg_send.size = s_buf_dnp.length;
					TB_msgq_send( &msg_send );
					TB_util_delay( 500 );
				}
				else //if( comm_type.master == COMM_MODE_MASTER_WISUN )
				{
					msg_send.type = MSG_TYPE_WISUN_GGW2TERM;
					msg_send.id	  = MSG_CMD_WISUN_SEND_TERM2GGW;
					msg_send.data = s_buf_dnp.buffer;
					msg_send.size = s_buf_dnp.length;
					TB_msgq_send( &msg_send );
					TB_util_delay( 500 );
				}
#else
				switch( comm_type.master )
				{													
					case COMM_MODE_MASTER_RS232 :	msg_send.type	= MSG_TYPE_MODEM;
													msg_send.id		= MSG_CMD_MODEM_WRITE;
													break;
													
					case COMM_MODE_MASTER_LTE	:	msg_send.type	= MSG_TYPE_LTE;
													msg_send.id		= MSG_CMD_LTE_WRITE;
													break;
													
					case COMM_MODE_MASTER_WISUN :
					default						:	msg_send.type	= MSG_TYPE_WISUN_GGW2TERM;
													msg_send.id		= MSG_CMD_WISUN_SEND_TERM2GGW;
													break;
				}
				
				msg_send.data = s_buf_dnp.buffer;
				msg_send.size = s_buf_dnp.length;
				TB_msgq_send( &msg_send );

				TB_util_delay( 500 );
#endif
			}

			////////////////////////////////////////////////////////////////////
				
			if( TB_msgq_recv( &msg_recv, MSG_TYPE_PSU_DNP, NOWAIT ) > 0 )
			{
				if( msg_recv.id == MSG_CMD_PSU_DNP_WRITE )
				{
					if( TB_dm_is_on(DMID_PSU) )
						TB_util_data_dump( "Write to PSU-DNP", msg_recv.data, msg_recv.size );
					TB_psu_dnp_write( msg_recv.data, msg_recv.size );
				}
			}
			
			TB_util_delay( 500 );
		}
	}
}

pthread_t s_thid_psu_dnp;
void TB_psu_dnp_proc_start( void )
{
	pthread_create( &s_thid_psu_dnp, NULL, tb_psu_dnp_proc, NULL );
}

void TB_psu_dnp_proc_stop( void )
{
	s_proc_flag_psu_dnp = 0;
	pthread_join( s_thid_psu_dnp, NULL );
}

////////////////////////////////////////////////////////////////////////////////
#include "TB_test.h"
pthread_t   g_thid_psu_dnp_test;
int			g_proc_flag_psu_dnp_test = 1;
static void *tb_psu_dnp_test_proc( void *arg )
{
	time_t cur_t, old_t;
    cur_t = old_t = time(NULL);
	
	printf("===========================================\r\n" );
	printf("           Start PSU-DNP TEST Proc         \r\n" );
	printf("===========================================\r\n" );

	int check = 0;
	TBUC buf1[32] = "1234567890";
	TBUC buf2[32] = "12345";
	TBUC buf_read[128];
	int normal = 0;
	TBUS len = 0;
	while( g_proc_flag_psu_dnp_test )
	{		
		cur_t = time(NULL);
        if( cur_t != old_t )
		{			
			TBUC buffer[128] = {0, };
			TBUC *p	= check == 0 ? (TBUC *)&buf1[0] :	\
								   (TBUC *)&buf2[2];
			check = !check;
			
			snprintf( buffer, sizeof(buffer), "[%s] %s\r\n", "PSU-DNP", p );
			len = TB_uart_psu_dnp_write( buffer, wstrlen(buffer) );
			if( len == wstrlen(buffer) )
			{
				TB_dbg_psu( "PSU_DNP WRITE = %s\r\n", buffer );
				normal |= SUCCESS_WRITE;
			}
			
			old_t = cur_t;
        }

		bzero( buf_read, sizeof(buf_read) );
		len = TB_uart_psu_dnp_read( buf_read, sizeof(buf_read) ) ;
        if( len > 1 )
        {
			normal |= SUCCESS_READ;
			TB_dbg_psu( "PSU_DNP READ = %s\r\n", buf_read );
        }

		if( (normal & (SUCCESS_WRITE|SUCCESS_READ)) == (SUCCESS_WRITE|SUCCESS_READ) )
		{
			TB_testmode_set_test_result( TEST_ITEM_PSU_DNP, TRUE );
		}

		TB_util_delay( 2000 );
	}
}

void TB_psu_dnp_test_proc_start( void )
{
	pthread_create( &g_thid_psu_dnp_test, NULL, tb_psu_dnp_test_proc, NULL );
}

void TB_psu_dnp_test_proc_stop( void )
{
	g_proc_flag_psu_dnp_test = 0;
	pthread_join( g_thid_psu_dnp_test, NULL );
}

pthread_t   g_thid_psu_mod_test;
int			g_proc_flag_psu_mod_test = 1;
static void *tb_psu_mod_test_proc( void *arg )
{
	time_t cur_t, old_t;
    cur_t = old_t = time(NULL);
	
	printf("===========================================\r\n" );
	printf("           Start PSU-MOD TEST Proc         \r\n" );
	printf("===========================================\r\n" );

	int check = 0;
	TBUC buf1[32] = "1234567890";
	TBUC buf2[32] = "12345";
	TBUC buf_read[128];
	int normal = 0;
	while( g_proc_flag_psu_mod_test )
	{		
		cur_t = time(NULL);
        if( cur_t != old_t )
		{			
			TBUC buffer[128];
			TBUC *p	= check == 0 ? buf1 : buf2;
			check = !check;
			
			snprintf( buffer, sizeof(buffer), "[%s] %s\r\n", "PSU-MOD", p );
			if( TB_uart_psu_mod_write( buffer, wstrlen(buffer) ) > 1 )
			{
				TB_dbg_psu( "PSU_MOD WRITE = %s\r\n", buffer );
				normal |= SUCCESS_WRITE;
			}
			
			old_t = cur_t;
        }

		bzero( buf_read, sizeof(buf_read) );
		int len = TB_uart_psu_mod_read( buf_read, sizeof(buf_read) ) ;
        if( len > 1 )
        {
			normal |= SUCCESS_READ;
			TB_dbg_psu( "PSU_MOD  READ = %s\r\n", buf_read );
        }

		if( (normal & (SUCCESS_WRITE|SUCCESS_READ)) == (SUCCESS_WRITE|SUCCESS_READ) )
		{
			TB_testmode_set_test_result( TEST_ITEM_PSU_MOD, TRUE );
		}

		TB_util_delay( 2000 );
	}
}

void TB_psu_mod_test_proc_start( void )
{
	pthread_create( &g_thid_psu_mod_test, NULL, tb_psu_mod_test_proc, NULL );
}

void TB_psu_mod_test_proc_stop( void )
{
	g_proc_flag_psu_mod_test = 0;
	pthread_join( g_thid_psu_mod_test, NULL );
}

