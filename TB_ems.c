#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "TB_util.h"
#include "TB_uart.h"
#include "TB_sys_gpio.h"
#include "TB_msg_queue.h"
#include "TB_modbus.h"
#include "TB_psu.h"
#include "TB_elapse.h"
#include "TB_debug.h"
#include "TB_setup.h"
#include "TB_wisun_util.h"
#include "TB_wisun.h"
#include "TB_rssi.h"
#include "TB_sys_wdt.h"
#include "TB_packet.h"
#include "TB_led.h"

#include "TB_ems.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_EMS_AUTH_TRY_COUNT	5
#define AUTH_REQ_TIMEOUT		(60*1000)	//	60sec

static TBUC 	 s_ems_auth_state 	= EMS_AUTH_STATE_READY;
static TBUC 	 s_ems_auth_req_cnt = 0;
static TB_ELAPSE s_elapse_auth;

////////////////////////////////////////////////////////////////////////////////

static TB_EMS_BUF	s_ems_buf_read;
static TB_EMS_BUF	s_ems_buf_write;
static TB_EMS_BUF	s_ems_buf_ack;
static TB_EMS_BUF	s_ems_buf_udb;

typedef struct __attribute__((__packed__)) tagEMS_REQ_DATA_AUTH
{
	TBUC 		cot_ip[SIZE_COT_IP];
	TBUS		rt_port;
	TBUS		serial;
} TB_EMS_REQ_AUTH;

typedef struct  __attribute__((__packed__)) tagEMS_ACK_DATA_STATE_INFO
{
	TB_VERSION 	version;		//	2byte
	TBUS		serial;			//	2byte, unsigned int
	TB_DATE		manuf_date;		//	4byte
} TB_EMS_ACK_DATA_STATE_INFO;

typedef struct  __attribute__((__packed__)) tagEMS_DBM
{
	TB_ROLE 	role;			//	4byte, typedef enum ...
	TBUS		serial;			//	2byte, unsigned short
	TBSI		dbm;			//	4byte, signed int
	TBUC		percent;		//	1byte, signed char
} TB_EMS_DBM;

typedef struct  __attribute__((__packed__)) tagEMS_ACK_DATA_STATE_DBM
{
	TB_EMS_DBM	dbm[MAX_CONNECT_CNT+1];
	TBUC		count;
} TB_EMS_ACK_DATA_STATE_DBM;

typedef struct  __attribute__((__packed__)) tagEMS_ACK_DATA_STATE_TIME
{
	time_t t;					//	4byte
} TB_EMS_ACK_DATA_STATE_TIME;

////////////////////////////////////////////////////////////////////////////////

TBUS TB_ems_convert_emsbuf2packet( TB_EMS_BUF *p_ems_buf, TBUS offset, TB_EMS_PACKET *p_packet );
TBUS TB_ems_convert_emsbuf2packet2( TB_EMS_BUF *p_ems_buf, TBUS offset, TB_EMS_PACKET *p_packet );
TBUC TB_ems_get_equ( void );
int TB_ems_make_outer_packet( TB_EMS_BUF *p_dest, TBUC *p_data, TBUS size );
int TB_ems_make_ems_data( TB_EMS_BUF *p_ems_buf, TB_EMS_BUF *p_udb_buf );
int TB_ems_make_udb( TBUC org, TBUC ctrl, TBUC ack_org, TBUC ack_ctrl, TB_EMS_BUF *p_udb );
int TB_ems_make_ack_data( TB_EMS_PACKET *p_packet, TB_EMS_BUF *p_ems_ack_buf );

////////////////////////////////////////////////////////////////////////////////

#if 1
#define MAXLEN	64
static void tb_ems_packet_get_string_org_ctrl( TBUC org, TBUC ctrl, char ret_str_org[MAXLEN], char ret_str_ctrl[MAXLEN] )
{
	bzero( ret_str_org, MAXLEN );
	bzero( ret_str_ctrl, MAXLEN );

	char *str_org_unknown  = "Unknown ORG";
	char *str_ctrl_unknown = "Unknown CTRL";

	char *str_org[] = 
	{
		"FUNC_ORG_AUTH",
		"FUNC_ORG_STATE",
		"FUNC_ORG_SET",
		"FUNC_ORG_CTRL"
	};

	char *str_org_auth_ctrl[] = 
	{
		"FUNC_ORG_AUTH_CTRL_AUTH_REQ",
		"FUNC_ORG_AUTH_CTRL_AUTH_RESULT",
		"FUNC_ORG_AUTH_CTRL_SESSION_KEY_SEND",
		"FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV",
		"FUNC_ORG_AUTH_CTRL_REAUTH_REQ_SEND"
	};

	char *str_org_state_ctrl[] = 
	{
		"FUNC_ORG_STATE_CTRL_REQ_COMM",
		"FUNC_ORG_STATE_CTRL_ACK_COMM",
		"FUNC_ORG_STATE_CTRL_REQ_INFO",
		"FUNC_ORG_STATE_CTRL_ACK_INFO",
		"FUNC_ORG_STATE_CTRL_REQ_COMM_DBM",
		"FUNC_ORG_STATE_CTRL_ACK_COMM_DBM",
		"FUNC_ORG_STATE_CTRL_REQ_TIME",
		"FUNC_ORG_STATE_CTRL_ACK_TIME",
		"FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO",
		"FUNC_ORG_STATE_CTRL_ACK_DEVICE_INFO"
	};

	char *str_org_set_ctrl[] = 
	{
		"FUNC_ORG_SET_CTRL_REQ_TIME_SYNC",
		"FUNC_ORG_SET_CTRL_ACK_TIME_SYNC"
	};

	char *str_org_ctrl_ctrl[] = 
	{
		"FUNC_ORG_CTRL_CTRL_REQ_RESET",
		"FUNC_ORG_CTRL_CTRL_ACK_RESET"
	};

	char *p_org  = NULL;
	char *p_ctrl = NULL;
	
	switch( org )
	{
		case FUNC_ORG_AUTH 	:
			p_org = str_org[org-1];
			switch( ctrl )
			{
				case FUNC_ORG_AUTH_CTRL_AUTH_REQ		:
				case FUNC_ORG_AUTH_CTRL_AUTH_RESULT		:
				case FUNC_ORG_AUTH_CTRL_SESSION_KEY_SEND:
				case FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV:
				case FUNC_ORG_AUTH_CTRL_REAUTH_REQ_SEND	: p_ctrl = str_org_auth_ctrl[ctrl-1];		break;
				default									: p_ctrl = str_ctrl_unknown;				break;
			}
			break;

		case FUNC_ORG_STATE	:
			p_org = str_org[org-1];
			switch( ctrl )
			{
				case FUNC_ORG_STATE_CTRL_REQ_COMM		: 
				case FUNC_ORG_STATE_CTRL_ACK_COMM		: 
				case FUNC_ORG_STATE_CTRL_REQ_INFO		: 
				case FUNC_ORG_STATE_CTRL_ACK_INFO		: 
				case FUNC_ORG_STATE_CTRL_REQ_COMM_DBM	: 
				case FUNC_ORG_STATE_CTRL_ACK_COMM_DBM	: 
				case FUNC_ORG_STATE_CTRL_REQ_TIME		: 
				case FUNC_ORG_STATE_CTRL_ACK_TIME		: 
				case FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO:
				case FUNC_ORG_STATE_CTRL_ACK_DEVICE_INFO: p_ctrl = str_org_state_ctrl[ctrl-1];		break;
				default									: p_ctrl = str_ctrl_unknown;				break;
			}
			break;
			
		case FUNC_ORG_SET	:
			p_org = str_org[org-1];
			switch( ctrl )
			{
				case FUNC_ORG_SET_CTRL_REQ_TIME_SYNC	: 
				case FUNC_ORG_SET_CTRL_ACK_TIME_SYNC	: p_ctrl = str_org_set_ctrl[ctrl-1];		break;
				default									: p_ctrl = str_ctrl_unknown;				break;
			}
			break;
			
		case FUNC_ORG_CTRL	:
			p_org = str_org[org-1];
			switch( ctrl )
			{
				case FUNC_ORG_CTRL_CTRL_REQ_RESET		: 
				case FUNC_ORG_CTRL_CTRL_ACK_RESET		: p_ctrl = str_org_ctrl_ctrl[ctrl-1];		break;
				default									: p_ctrl = str_ctrl_unknown;				break;
			}
			break;
			
		default 			:
			p_org = str_org_unknown;																break;
	}

	if( p_org )
		wstrncpy( ret_str_org, MAXLEN, p_org, wstrlen(p_org) );

	if( p_ctrl )
		wstrncpy( ret_str_ctrl, MAXLEN, p_ctrl, wstrlen(p_ctrl) );
}
#endif

static void tb_ems_packet_dump( TB_EMS_PACKET *p_packet, char *file, int line )
{
	if( TB_dm_is_on(DMID_EMS) )
	{
		if( p_packet )
		{
			char str_org [MAXLEN] = {0, };
			char str_ctrl[MAXLEN] = {0, };

			tb_ems_packet_get_string_org_ctrl( p_packet->org, p_packet->ctrl, str_org, str_ctrl );
			
			printf("============ EMS PACKET DUMP ============\r\n" );
			printf("caller [%s:%d]\r\n", file, line );
			printf("     stx     = 0x%02X\r\n", p_packet->stx );
			printf("     equ     = 0x%02X\r\n", p_packet->equ );
			printf("     udb_len = %d\r\n", p_packet->udb_len );
			printf("     dest    = %d\r\n", p_packet->dest );
			printf("     sour    = %d\r\n", p_packet->sour );
			printf("     org     = 0x%02X : %s\r\n", p_packet->org, str_org );
			printf("     ctrl    = 0x%02X : %s\r\n", p_packet->ctrl,str_ctrl );
			printf("     crc     = 0x%02X\r\n", p_packet->crc );
			printf("     etx     = 0x%02X\r\n", p_packet->etx );		
			if( p_packet->udb_len > 0 )
			{
				printf("     data    = " );
				if( p_packet->udb_len > 16 )	printf("\r\n");
				TB_util_data_dump2( p_packet->udb, p_packet->udb_len );
			}
			printf("=========================================\r\n" );
		}
		else
		{
			printf("ERROR. emp packet dump\r\n" );
		}
	}
}

static void tb_ems_buf_dump( TB_EMS_BUF *p_ems_buf, char *file, int line )
{
	if( TB_dm_is_on(DMID_EMS) )
	{
		if( p_ems_buf )
		{
			printf("============ EMS DATA DUMP ============\r\n" );
			TB_EMS_PACKET packet;
			int idx = 0;

			packet.stx		= *(TBUS*)&p_ems_buf->buffer[idx];			idx ++;
			packet.equ		= *(TBUS*)&p_ems_buf->buffer[idx];			idx ++;
			packet.udb_len	= *(TBUS*)&p_ems_buf->buffer[idx];			idx += 2;
			packet.dest		= *(TBUS*)&p_ems_buf->buffer[idx];			idx += 2;
			packet.sour		= *(TBUS*)&p_ems_buf->buffer[idx];			idx += 2;
			packet.org		= *(TBUS*)&p_ems_buf->buffer[idx];			idx ++;
			packet.ctrl		= *(TBUS*)&p_ems_buf->buffer[idx];			idx ++;
			if( packet.udb_len > 0 )
				wmemcpy( packet.udb, MAX_DNP_BUF, &p_ems_buf->buffer[idx], packet.udb_len );
			idx += packet.udb_len;
			packet.crc		= *(TBUS*)&p_ems_buf->buffer[idx];			idx += 2;
			packet.etx		= *(TBUS*)&p_ems_buf->buffer[idx];			idx += 1;

			tb_ems_packet_dump( &packet, file, line );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

static pthread_mutex_t		s_ems_write_mutex;
static pthread_mutexattr_t 	s_ems_mutex_attr;
int TB_ems_init( void )
{
	int ret = -1;

	bzero( &s_ems_buf_read, sizeof(s_ems_buf_read) );
	
	if( TB_uart_ems_init() == 0 )
	{
		pthread_mutexattr_init( &s_ems_mutex_attr );
		pthread_mutexattr_settype( &s_ems_mutex_attr, PTHREAD_MUTEX_ERRORCHECK );
		pthread_mutex_init( &s_ems_write_mutex, &s_ems_mutex_attr );
		
		TB_ems_proc_start();
		ret = 0;
	}
	
	return 0;
}

TBUS TB_ems_read( void )
{
	TBUS offset = 0;
	TBUS size = 0;

	bzero( &s_ems_buf_read, sizeof(s_ems_buf_read) );
	while( 1 )
	{
		size = TB_uart_ems_read( &s_ems_buf_read.buffer[offset], sizeof(s_ems_buf_read.buffer) );
		if( size > 0 )
		{
			TBUS udb_len	 = *(TBUS*)&s_ems_buf_read.buffer[EMS_PACKET_OFFSET_UDBLEN];
			TBUS packet_size = SIZE_STX + SIZE_EQU + SIZE_UDB_LEN + SIZE_DEST + SIZE_SOUR + LEN_BYTE_TAG + udb_len + SIZE_CRC + SIZE_ETX;

			TB_dbg_ems("offset = %d, ems read size = %d, packet size = %d\r\n", offset, size, packet_size );

			if( offset < packet_size )
			{
				offset += size;
				TB_util_delay( 100 );
				continue;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	s_ems_buf_read.length = offset;
	return s_ems_buf_read.length;
}

TBUS TB_ems_write( TBUC *buf, TBUS len )
{
	TBUS ret = 0;
	static volatile int s_count = 0;

	if( buf && len > 0 )
	{
		while( s_count > 1 )
		{
			TB_util_delay( 10 );
		}

		s_count ++;		
		pthread_mutex_lock( &s_ems_write_mutex );

		ret = TB_uart_ems_write( buf, len );
		if( ret > 0 )
		{
			if( TB_dm_is_on(DMID_EMS) )
				TB_util_data_dump( "EMS Write", buf, ret );
		}

		pthread_mutex_unlock( &s_ems_write_mutex );
		s_count --;
	}

	return ret;
}

void TB_ems_deinit( void )
{
	TB_uart_ems_deinit();
}

int TB_ems_send_to_lower( TBUC *p_data, TBUS size )
{
	int ret = -1;
	
	if( p_data && size > 0 )
	{
		TB_MESSAGE 		msg;		
		TB_SETUP_WISUN *p_wisun_info = TB_setup_get_wisun_info( 0 );
		TB_ROLE 		role 		 = TB_setup_get_role();
		
		if( role == ROLE_GRPGW )
		{			
			for( int i=0; i<p_wisun_info->max_connect; i++ )
			{	
				msg.type = MSG_TYPE_WISUN_GGW2TERM;
				msg.id   = MSG_CMD_WISUN_SEND_GGW2TERM1 + i;

				TB_dbg_ems("MSG_CMD_WISUN_SEND_GGW2TERM1 ---> 0x%X\r\n", msg.id );

				TB_ems_make_outer_packet( &s_ems_buf_write, p_data, size );
				msg.data = s_ems_buf_write.buffer;
				msg.size = s_ems_buf_write.length;

				TB_dbg_ems("Send Message [EMS] From GGW to Terminal. msgid=0x%X\r\n", msg.id );
				TB_msgq_send( &msg );

				TB_util_delay( 1000 );
			}

			ret = 0;
		}
		else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
		{
			if( TB_setup_get_product_info_voltage() == VOLTAGE_LOW )
			{
				for( int i=0; i<p_wisun_info->max_connect; i++ )
				{	
					msg.type = MSG_TYPE_WISUN_TERM2RELAY;
					msg.id	 = MSG_CMD_WISUN_SEND_TERM2RELAY1 + i;

					TB_ems_make_outer_packet( &s_ems_buf_write, p_data, size );
					msg.data = s_ems_buf_write.buffer;
					msg.size = s_ems_buf_write.length;

					TB_dbg_ems("Send Message [EMS] From Terminal to Relay. msgid=0x%X\r\n", msg.id );
					TB_msgq_send( &msg );

					TB_util_delay( 1000 );
				}

				ret = 0;
			}
		}		
	}

	return ret;
}

int TB_ems_send_to_upper( TB_EMS_BUF *p_ems_buf )
{
	int ret = -1;
	
	if( p_ems_buf )
	{
		TB_MESSAGE 		msg;		
		TB_SETUP_WISUN *p_wisun_info = TB_setup_get_wisun_info( 0 );
		TB_ROLE 		role 		 = TB_setup_get_role();
		
		if( role == ROLE_GRPGW )
		{
			TB_ems_write( p_ems_buf->buffer, p_ems_buf->length );
		}
		else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
		{
			msg.type = MSG_TYPE_WISUN_GGW2TERM;
			msg.id	 = MSG_CMD_WISUN_SEND_TERM2GGW;

			TB_ems_make_outer_packet( &s_ems_buf_write, p_ems_buf->buffer, p_ems_buf->length );
			msg.data = s_ems_buf_write.buffer;
			msg.size = s_ems_buf_write.length;

			TB_dbg_ems("Send Message [EMS] From Terminal to GGW. msgid=0x%X\r\n", msg.id );
			TB_msgq_send( &msg );

			TB_util_delay( 1000 );
		}
		else if( role >= ROLE_RELAY1 && role <= ROLE_RELAY3 )
		{
			msg.type = MSG_TYPE_WISUN_TERM2RELAY;
			msg.id	 = MSG_CMD_WISUN_SEND_RELAY2TERM;

			TB_ems_make_outer_packet( &s_ems_buf_write, p_ems_buf->buffer, p_ems_buf->length );
			msg.data = s_ems_buf_write.buffer;
			msg.size = s_ems_buf_write.length;

			TB_dbg_ems("Send Message [EMS] From Relay to Terminal. msgid=0x%X\r\n", msg.id );
			TB_msgq_send( &msg );

			TB_util_delay( 1000 );
		}

		ret = 0;
	}

	return ret;
}

/*******************************************************************************
*					Group G/W에서만 동작하는 함수.
*******************************************************************************/
int TB_ems_read_data_check( TB_EMS_BUF *p_ems_buf )
{
	int ret = -1;
	
	if( p_ems_buf && p_ems_buf->length > 0 )
	{
		TBUS offset = 0;
		TB_EMS_PACKET  packet;

		TBUS this_addr = TB_setup_get_product_info_ems_addr();
		TBUS ems_dest  = TB_setup_get_product_info_ems_dest();	//	EMS 서버 주소
		while( 1 )
		{
			if( offset < p_ems_buf->length )
			{
				TBUS size = TB_ems_convert_emsbuf2packet( p_ems_buf, offset, &packet );
				if( size > 0 )
				{
					TB_dbg_ems("this_addr=%d, dest_addr=%d\r\n", this_addr, packet.dest );
					if( this_addr == packet.dest )
					{
						if( TB_ems_make_ack_data( &packet, &s_ems_buf_write ) == 0 )
						{
							TB_ems_write( s_ems_buf_write.buffer, s_ems_buf_write.length );
						}
					}
					else
					{
						TBUC *p_data 	= &p_ems_buf->buffer[offset];
						TBUS  data_size = size;
						if( p_ems_buf->buffer[offset] == 0x4B && p_ems_buf->buffer[offset+1] == 0x4B )
						{
							p_data 	  = &p_ems_buf->buffer[offset+4];
							data_size = size - 4;
						}

						if( TB_ems_send_to_lower(p_data, data_size) != 0 )
						{
							TB_dbg_ems("Unknown Dest Addr = %d\r\n", packet.dest );
						}
					}
				}
				else
				{
					break;
				}

				offset += size;
			}
			else
			{
				break;
			}
		}
		
		ret = 0;
	}

	return ret;
}

int TB_ems_recv_data_check( TB_EMS_BUF *p_ems_buf )
{
	int ret = -1;
	
	if( p_ems_buf && p_ems_buf->length > 0 )
	{
		TB_EMS_PACKET packet;

		TBUS offset = 0;
		TBUS size = TB_ems_convert_emsbuf2packet2( p_ems_buf, offset, &packet );
		if( size > 0 )
		{
			TBUS this_addr = TB_setup_get_product_info_ems_addr();
			TBUS ems_addr  = TB_setup_get_product_info_ems_dest();
			
			TB_dbg_ems("ems_addr=%d, this_addr=%d, packet_dest=%d, packet_sour=%d\r\n",	\
						ems_addr, this_addr, packet.dest, packet.sour );

			if( this_addr == packet.dest )
			{
				TB_EMS_BUF ems_buf;
				if( TB_ems_make_ack_data( &packet, &ems_buf ) == 0 )
				{
					TB_ems_send_to_upper( &ems_buf );

					if( packet.org == FUNC_ORG_AUTH && packet.ctrl == FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV )
						TB_ems_send_to_upper( &ems_buf );
				}
			}
			else if( ems_addr == packet.dest )
			{
				TB_ems_send_to_upper( p_ems_buf );
			}
			else
			{
				if( TB_ems_send_to_lower( p_ems_buf->buffer, p_ems_buf->length) != 0 )
				{
					//TB_dbg_ems("Unknown Dest Addr = %d\r\n", packet.dest );
				}
			}
		}
		
		ret = 0;
	}

	return ret;
}

//void TB_ems_set_auth_status( TBUC status )
void __TB_ems_set_auth_status( TB_EMS_AUTH_STATE status, char *file, int line )
{
	if( file )
	{
		TB_dbg_ems("[%s] call from[%s:%d] status = %d\r\n",	\
					__FUNCTION__, file, line, status );
	}
	
	if( status < EMS_AUTH_STATE_MAX )
	{
		if( status != s_ems_auth_state )
		{
			if( s_ems_auth_state == EMS_AUTH_STATE_REQ )
			{
				if( status == EMS_AUTH_STATE_READY || status == EMS_AUTH_STATE_RETRY )
				{
					/***********************************************************
					*	인증요청 중이기 때문에 신규 인증요청은 무시된다.
					***********************************************************/
					if( file )
					{
						TB_dbg_ems("[%s] call from[%s:%d] Ignore Ignore Ignore  = %d\r\n",	\
									__FUNCTION__, file, line, status );
					}					
				}
				else
				{
					s_ems_auth_state = status;
					TB_dbg_ems("[%s] call from[%s:%d] SET Auth STATUS = %d\r\n",	\
									__FUNCTION__, file, line, status );
				}
			}
			else
			{
				s_ems_auth_state = status;
				TB_dbg_ems("[%s] call from[%s:%d] SET Auth STATUS = %d\r\n",	\
								__FUNCTION__, file, line, status );
			}
		}
	}
}

TBUC s_proc_flag_ems = 0;
static void *tb_ems_proc( void *arg )
{	
	TB_MESSAGE msg;

	printf("===========================================\r\n" );
	printf("                Start EMS Proc             \r\n" );
	printf("===========================================\r\n" );

	while( TB_wisun_init_complete(WISUN_FIRST) == FALSE )
	{
		TB_util_delay( 100 );
	}

	TB_ROLE role = TB_setup_get_role();

	TB_ems_set_auth_status( EMS_AUTH_STATE_READY );
	s_proc_flag_ems = 1;
	while( s_proc_flag_ems )
	{
		if( role == ROLE_GRPGW	)
		{
			if( TB_ems_read() > 0 )
			{
				if( TB_dm_is_on(DMID_EMS) )
					TB_util_data_dump( "EMS Read data", s_ems_buf_read.buffer, s_ems_buf_read.length );

				TB_ems_read_data_check( &s_ems_buf_read );
			}
		}

		////////////////////////////////////////////////////////////////////////
		
		switch( s_ems_auth_state )
		{
			case EMS_AUTH_STATE_READY		:
			case EMS_AUTH_STATE_RETRY		:
			case EMS_AUTH_STATE_RETRY_FORCE	:
				TB_elapse_set_init( &s_elapse_auth );	
				TB_ems_auth_req();
				TB_ems_set_auth_status( EMS_AUTH_STATE_REQ );
				TB_elapse_set_start( &s_elapse_auth );
				break;
				
			case EMS_AUTH_STATE_REQ		:
				TB_elapse_print_elapse_time( &s_elapse_auth );
				if( TB_elapse_check_expire( &s_elapse_auth, AUTH_REQ_TIMEOUT, 0 ) == TRUE )
				{
					TB_ems_set_auth_status( EMS_AUTH_STATE_RETRY_FORCE );
				}
				break;
		}

		////////////////////////////////////////////////////////////////////////

		if( TB_msgq_recv( &msg, MSG_TYPE_EMS, NOWAIT) > 0 )
		{
			switch( msg.id )
			{
				case MSG_CMD_EMS_CHECK 	:
					TB_dbg_ems("RECV EMS CHECK message\r\n");
					wmemcpy( &s_ems_buf_read.buffer[0], sizeof(s_ems_buf_read.buffer), msg.data, msg.size );
					s_ems_buf_read.length = msg.size;

					TB_ems_recv_data_check( &s_ems_buf_read );
					break;
					
				case MSG_CMD_EMS_WRITE 	:
					if( msg.data && msg.size > 0 )
					{
						TB_ems_write( msg.data, msg.size );
					}
					break;

				default	:
					TB_dbg_ems("Unknown Message ID : %d\r\n", msg.id );
					break;
			}
		}

		TB_util_delay( 500 );
	}
}

TBBL TB_ems_proc_is_run( void )
{
	return s_proc_flag_ems ? TRUE : FALSE;
}

pthread_t s_thid_ems;
void TB_ems_proc_start( void )
{
	pthread_create( &s_thid_ems, NULL, tb_ems_proc, NULL );
}

void TB_ems_proc_stop( void )
{
	s_proc_flag_ems = 0;
	pthread_join( s_thid_ems, NULL );
}

////////////////////////////////////////////////////////////////////////////////

int TB_ems_encryption_data( TBUC *p_sour, TBUI sour_len, TBUC *p_dest, TBUI *p_dest_len, TB_KEY_TYPE key_type )
{
	int ret = -1;
	if( p_sour && sour_len > 0 )
	{
		if( p_dest )
		{
			TB_KCMVP_KEY *p_key  = NULL;
			TB_KCMVP_INFO *p_keyinfo = TB_kcmvp_get_keyinfo();
			if( p_keyinfo )
			{
				if( key_type == KCMVP_KEY_TYPE_NONE )
				{
					if( p_keyinfo->session_key_new.size > 0 )
					{
						p_key = &p_keyinfo->session_key_new;
						TB_dbg_ems("--->EMS KEY - SESSION - NeW\r\n" );
					}
					else if( p_keyinfo->session_key_now.size > 0 )
					{
						p_key = &p_keyinfo->session_key_now;
						TB_dbg_ems("--->EMS KEY - SESSION - NoW\r\n" );
					}
					else
					{
						p_key = &p_keyinfo->master_key;
						TB_dbg_ems("--->EMS KEY - MASTER\r\n" );
					}
				}
				else
				{
					if( key_type == KCMVP_KEY_TYPE_SESSION_NEW )
					{
						p_key = &p_keyinfo->session_key_new;
						TB_dbg_ems("--->EMS KEY - SESSION - NeW\r\n" );
					}
					else if( key_type == KCMVP_KEY_TYPE_SESSION_NOW )
					{
						p_key = &p_keyinfo->session_key_now;
						TB_dbg_ems("--->EMS KEY - SESSION - NoW\r\n" );
					}
					else
					{
						p_key = &p_keyinfo->master_key;
						TB_dbg_ems("--->EMS KEY - MASTER\r\n" );
					}
				}

				TBUC *p_enc_tag  = &p_dest[0];
				TBUC *p_enc_data = &p_dest[LEN_BYTE_TAG];
				TBUI enc_data_len = 0;
				TBUI kcmvp_ret = KDN_BC_Encrypt( &p_enc_data[0], &enc_data_len,	\
												 p_sour, sour_len,				\
												 p_key->data, LEN_BYTE_KEY,		\
												 KDN_BC_Algo_ARIA_GCM,	 		\
												 p_keyinfo->iv.data, p_keyinfo->iv.size, 		\
												 p_keyinfo->auth.data, p_keyinfo->auth.size,	\
												 &p_enc_tag[0], LEN_BYTE_TAG );
				if( kcmvp_ret == KDN_OK )
				{
					*p_dest_len = LEN_BYTE_TAG + enc_data_len;
					TB_dbg_ems("enc data len = %d, p_dest_len=%d\r\n", enc_data_len, *p_dest_len );
					if( TB_dm_is_on(DMID_EMS) )
						TB_util_data_dump( "ENC-UDB", p_dest, *p_dest_len );
					ret = 0;
				}
				else
				{
					TB_dbg_ems("ERROR. KCMVP encryption : %d\r\n", kcmvp_ret );
				}
			}
		}
		else
		{
			TB_dbg_ems("ERROR. encryption\r\n" );
		}
	}
	else
	{
		TB_dbg_ems("ERROR. encryption\r\n" );
	}

	return ret;
}

int TB_ems_decryption_data( TBUC *p_data, TBUS data_len, TBUC *p_org, TBUC *p_ctrl, TBUC *p_udb, TBUS *p_udb_len )
{
	int ret = -1;
	
	if( (p_data && data_len > 0) && p_org && p_ctrl && p_udb && p_udb_len )
	{
		TBUC dec_data[256] = {0, };
		TBUI dec_size;
		
		TBUC *p_tag  	= &p_data[0];
		TBUC *p_cipher 	= &p_data[LEN_BYTE_TAG];

		TB_KCMVP_INFO *p_keyinfo = TB_kcmvp_get_keyinfo();
		if( p_keyinfo )
		{
			TBUI kcmvp_ret = UNKNOWN_ERROR;
			TB_KCMVP_KEY *p_key = NULL;

			/*
			*	1st try.
			*/
			if( p_keyinfo->session_key_new.size > 0 )
			{
				p_key = &p_keyinfo->session_key_new;
				kcmvp_ret = KDN_BC_Decrypt( &dec_data[0], &dec_size,			\
											 p_cipher, data_len - LEN_BYTE_TAG,	\
											 p_key->data, p_key->size,			\
											 KDN_BC_Algo_ARIA_GCM, 				\
											 p_keyinfo->iv.data, p_keyinfo->iv.size, 		\
											 p_keyinfo->auth.data, p_keyinfo->auth.size,	\
											 p_tag, LEN_BYTE_TAG );

				if( kcmvp_ret == KDN_OK )
				{
					TB_dbg_ems("kcmvp decryption SESSION KEY - NEW\r\n" );
				}
			}

			/*
			*	2nd try.
			*/
			if( kcmvp_ret != KDN_OK )
			{
				if( p_keyinfo->session_key_now.size > 0 )
				{
					p_key = &p_keyinfo->session_key_now;
					kcmvp_ret = KDN_BC_Decrypt( &dec_data[0], &dec_size,			\
												 p_cipher, data_len - LEN_BYTE_TAG,	\
												 p_key->data, p_key->size,			\
												 KDN_BC_Algo_ARIA_GCM, 				\
												 p_keyinfo->iv.data, p_keyinfo->iv.size, 		\
												 p_keyinfo->auth.data, p_keyinfo->auth.size,	\
												 p_tag, LEN_BYTE_TAG );

					if( kcmvp_ret == KDN_OK )
					{
						TB_dbg_ems("kcmvp decryption SESSION KEY - NoW\r\n" );
					}
				}
			}

			/*
			*	3th try.
			*/
			if( kcmvp_ret != KDN_OK )
			{
				p_key = &p_keyinfo->master_key;
				kcmvp_ret = KDN_BC_Decrypt( &dec_data[0], &dec_size,			\
											 p_cipher, data_len - LEN_BYTE_TAG,	\
											 p_key->data, p_key->size,			\
											 KDN_BC_Algo_ARIA_GCM, 				\
											 p_keyinfo->iv.data, p_keyinfo->iv.size, 		\
											 p_keyinfo->auth.data, p_keyinfo->auth.size,	\
											 p_tag, LEN_BYTE_TAG );

				if( kcmvp_ret == KDN_OK )
				{
					TB_dbg_ems("kcmvp decryption MASTER KEY\r\n" );
				}				
			}

			////////////////////////////////////////////////////////////////////

			if( kcmvp_ret == KDN_OK )
			{
				*p_org  = dec_data[0];
				*p_ctrl = dec_data[1];

				if( dec_size > SIZE_ORG + SIZE_CTRL )
				{
					*p_udb_len = dec_size - 2;
					p_udb[0] = 0;//NULL;
					wmemcpy( p_udb, MAX_DNP_BUF, &dec_data[2], *p_udb_len  );
				}
				else
				{
					*p_udb_len = 0;
				}

				TB_led_dig_del_critical();
				TB_kcmvp_decryption_error_count_clear();
				ret = 0;
			}
			else
			{
				TB_dbg_ems("ERROR. KCMVP Decryption : %d\r\n", kcmvp_ret );
				TB_led_dig_add_critical();
				TB_kcmvp_decryption_error_count_inc();
			}
		}
	}
	else
	{
		TB_dbg_ems("error. parameter.\r\n" );
	}

	return ret;
}

TBUS TB_ems_convert_emsbuf2packet( TB_EMS_BUF *p_ems_buf, TBUS offset, TB_EMS_PACKET *p_packet )
{
	int ret = -1;
	TBUS idx = 0;
	TBUS offset_org = offset;
	
	TB_EMS_PACKET packet;
	if( p_packet )
	{
		bzero( p_packet, sizeof(TB_EMS_PACKET) );
	}
	else
	{
		return (TBUS)0;
	}
	
	if( p_ems_buf && p_ems_buf->length > 0 )
	{
		bzero( &packet, sizeof(TB_EMS_PACKET) );

		TB_dbg_ems( "p_ems_buf->length = %d\r\n", p_ems_buf->length );

		/***********************************************************************
		*  4B 4B 15 00 --> 'K'+'K'+len_lowb+len_highb -->   len 0015H --> 21D
		***********************************************************************/
		if( p_ems_buf->buffer[offset] == 0x4B && p_ems_buf->buffer[offset+1] == 0x4B )
			offset += 4;

		////////////////////////////////////////////////////////////////////////

		idx = offset;

		packet.stx = *(TBUC*)&p_ems_buf->buffer[idx];					idx += SIZE_STX;
		if( packet.stx == STX )
		{
			packet.equ = *(TBUC*)&p_ems_buf->buffer[idx];				idx += SIZE_EQU;
			if( packet.equ < EQU_TYPE_MAX )
			{
				packet.udb_len	= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_UDB_LEN;		
				packet.dest		= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_DEST;
				packet.sour		= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_SOUR;
																		idx += LEN_BYTE_TAG;
																		idx += packet.udb_len;
				packet.crc		= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_CRC;
				packet.etx		= *(TBUC*)&p_ems_buf->buffer[idx];		idx += SIZE_ETX;

				if( packet.etx == ETX )
				{			
					TBUS crc_offset = SIZE_EQU + SIZE_UDB_LEN + SIZE_DEST + SIZE_SOUR + LEN_BYTE_TAG + packet.udb_len;
					TBUS crc = TB_crc16_modbus_get( &p_ems_buf->buffer[offset+EMS_PACKET_OFFSET_EQU], crc_offset );
					if( crc == packet.crc )
					{
						/***************************************************************
						*	암호화하기 전 실제 ORG, CTRL, UDB data와 Length를 구한다.
						***************************************************************/
						TBUS tag_offset = SIZE_STX + SIZE_EQU + SIZE_UDB_LEN + SIZE_DEST + SIZE_SOUR;
						if( TB_ems_decryption_data( &p_ems_buf->buffer[offset+tag_offset],
													LEN_BYTE_TAG + packet.udb_len,
										  	    	&packet.org,
										  	    	&packet.ctrl,
										  	    	&packet.udb[0],
										  	    	&packet.udb_len ) == 0 )
						{
							if( TB_dm_is_on(DMID_EMS) )
							{
								TBUS this_addr = TB_setup_get_product_info_ems_addr();
								TBUS ems_addr  = TB_setup_get_product_info_ems_dest();
								
								//if( this_addr == packet.dest )
								{
									tb_ems_packet_dump( &packet, __FILE__, __LINE__ );
								}
							}

							TB_dbg_ems( "OK. ems packet\r\n" );
							ret = 0;
						}
						else
							TB_dbg_ems( "ERROR. descryption data\r\n" );
					}
					else
						TB_dbg_ems( "ERROR. ems packet crc. 0x%02X : 0x%02X\r\n", crc, packet.crc );
				}
				else
					TB_dbg_ems( "ERROR. packet etx=0x%02X\r\n", packet.etx );
			}
			else
				TB_dbg_ems( "ERROR. packet equ=0x%02X\r\n", packet.equ );
		}
		else
			TB_dbg_ems( "ERROR. packet stx=0x%02X\r\n", packet.stx );
	}
	else
	{
		return (TBUS)0;
	}

	////////////////////////////////////////////////////////////////////////////

	TBUS size = 0;
	if( ret == 0 )
	{
		wmemcpy( p_packet, sizeof(TB_EMS_PACKET), &packet, sizeof(TB_EMS_PACKET) );
		size = (TBUS)(idx - offset_org);
	}
	else
	{
		size = (TBUS)(p_ems_buf->length);
	}

	return size;
}

TBUS TB_ems_convert_emsbuf2packet2( TB_EMS_BUF *p_ems_buf, TBUS offset, TB_EMS_PACKET *p_packet )
{
	int ret = -1;
	int idx = 0;
	
	TB_EMS_PACKET packet;

	if( p_packet )
	{
		bzero( p_packet, sizeof(TB_EMS_PACKET) );
	}
	else
	{
		return (TBUS)0;
	}
	
	if( p_ems_buf && p_ems_buf->length > 0 )
	{
		bzero( &packet, sizeof(TB_EMS_PACKET) );

		TB_dbg_ems( "p_ems_buf->length = %d\r\n", p_ems_buf->length );

		idx = offset;

		packet.stx = *(TBUC*)&p_ems_buf->buffer[idx];					idx += SIZE_STX;
		if( packet.stx == STX )
		{
			packet.equ = *(TBUC*)&p_ems_buf->buffer[idx];				idx += SIZE_EQU;
			if( packet.equ < EQU_TYPE_MAX )
			{
				packet.udb_len	= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_UDB_LEN;		
				packet.dest		= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_DEST;
				packet.sour		= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_SOUR;
																		idx += LEN_BYTE_TAG;
																		idx += packet.udb_len;
				packet.crc		= *(TBUS*)&p_ems_buf->buffer[idx];		idx += SIZE_CRC;
				packet.etx		= *(TBUC*)&p_ems_buf->buffer[idx];		idx += SIZE_ETX;

				if( packet.etx == ETX )
				{			
					TBUS crc_offset = SIZE_EQU + SIZE_UDB_LEN + SIZE_DEST + SIZE_SOUR + LEN_BYTE_TAG + packet.udb_len;
					TBUS crc = TB_crc16_modbus_get( &p_ems_buf->buffer[offset+EMS_PACKET_OFFSET_EQU], crc_offset );
					if( crc == packet.crc )
					{
						/***************************************************************
						*	암호화하기 전 실제 ORG, CTRL, UDB data와 Length를 구한다.
						***************************************************************/
						TBUS tag_offset = SIZE_STX + SIZE_EQU + SIZE_UDB_LEN + SIZE_DEST + SIZE_SOUR;
						if( TB_ems_decryption_data( &p_ems_buf->buffer[offset+tag_offset],
													LEN_BYTE_TAG + packet.udb_len,
										  	    	&packet.org,
										  	    	&packet.ctrl,
										  	    	&packet.udb[0],
										  	    	&packet.udb_len ) == 0 )
						{
							if( TB_dm_is_on(DMID_EMS) )
							{
								TBUS this_addr = TB_setup_get_product_info_ems_addr();
								TBUS ems_addr  = TB_setup_get_product_info_ems_dest();
								
								if( this_addr == packet.dest )
									tb_ems_packet_dump( &packet, __FILE__, __LINE__ );

								if( packet.org == FUNC_ORG_AUTH && packet.ctrl == FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV )
									tb_ems_packet_dump( &packet, __FILE__, __LINE__ );
							}

							TB_dbg_ems( "OK. ems packet\r\n" );
							ret = 0;
						}
						else
						{
							TB_dbg_ems( "ERROR. descryption data\r\n" );
						}
					}
					else
					{
						TB_dbg_ems( "ERROR. ems packet crc. 0x%02X : 0x%02X\r\n", crc, packet.crc );				
					}
				}
				else
				{			
					TB_dbg_ems( "ERROR. packet stx=0x%02X, etx=0x%02X\r\n", packet.stx, packet.etx );			
				}
			}
			else
				TB_dbg_ems( "ERROR. packet equ=0x%02X\r\n", packet.equ );
		}
		else
			TB_dbg_ems( "ERROR. packet stx=0x%02X\r\n", packet.stx );
	}
	else
		return (TBUS)0;

	////////////////////////////////////////////////////////////////////////////

	TBUS size = 0;
	if( ret == 0 )
	{
		wmemcpy( p_packet, sizeof(TB_EMS_PACKET), &packet, sizeof(TB_EMS_PACKET) );
		size = (TBUS)(idx - offset);
	}
	else
	{
		size = (TBUS)(p_ems_buf->length);
	}

	return size;
}

/*******************************************************************************
* WiSUN에서 KCMVP 암호화를 할 때 암호키를 어떤것을 사용할 것인가 판단할 때 사용
********************************************************************************/
int TB_ems_convert_emsdata2packet( TBUC *p_ems_data, TBUI ems_data_len, TB_EMS_PACKET *p_packet )
{
	int ret = -1;
	
	TB_EMS_PACKET packet;
	if( p_ems_data && ems_data_len > 0 && p_packet )
	{
		int idx = 0;
		
		bzero( &packet, sizeof(TB_EMS_PACKET) );

		packet.stx		= *(TBUC*)&p_ems_data[idx];		idx += SIZE_STX;
		packet.equ		= *(TBUC*)&p_ems_data[idx];		idx += SIZE_EQU;
		packet.udb_len	= *(TBUS*)&p_ems_data[idx];		idx += SIZE_UDB_LEN;
		packet.dest		= *(TBUS*)&p_ems_data[idx];		idx += SIZE_DEST;
		packet.sour		= *(TBUS*)&p_ems_data[idx];		idx += SIZE_SOUR;
														idx += LEN_BYTE_TAG;
														idx += packet.udb_len;
		packet.crc		= *(TBUS*)&p_ems_data[idx];		idx += SIZE_CRC;
		packet.etx		= *(TBUC*)&p_ems_data[idx];		idx += SIZE_ETX;

		if( packet.stx == STX && packet.etx == ETX )
		{			
			TBUS crc_offset = SIZE_EQU + SIZE_UDB_LEN + SIZE_DEST + SIZE_SOUR + LEN_BYTE_TAG + packet.udb_len;
			TBUS crc = TB_crc16_modbus_get( &p_ems_data[EMS_PACKET_OFFSET_EQU], crc_offset );
			if( crc == packet.crc )
			{
				/***************************************************************
				*	암호화하기 전 실제 ORG, CTRL, UDB data와 Length를 구한다.
				***************************************************************/
				TBUS tag_offset = SIZE_STX + SIZE_EQU + SIZE_UDB_LEN + SIZE_DEST + SIZE_SOUR;
				TB_ems_decryption_data( &p_ems_data[tag_offset],
										LEN_BYTE_TAG + packet.udb_len,
								  	    &packet.org,
								  	    &packet.ctrl,
								  	    &packet.udb[0],
								  	    &packet.udb_len );

				if( TB_dm_is_on(DMID_EMS) )
					tb_ems_packet_dump( &packet, __FILE__, __LINE__ );

				TB_dbg_ems( "OK. ems packet\r\n" );
				ret = 0;
			}
			else
			{
				TB_dbg_ems( "ERROR. ems packet crc. 0x%02X : 0x%02X\r\n", crc, packet.crc );				
			}
		}
		else
		{			
			TB_dbg_ems( "ERROR. packet stx=0x%02X, etx=0x%02X\r\n", packet.stx, packet.etx );			
		}
	}

	if( ret == 0 )
	{
		wmemcpy( p_packet, sizeof(TB_EMS_PACKET), &packet, sizeof(TB_EMS_PACKET) );
	}

	return ret;
}

TBUC TB_ems_get_equ( void )
{
	TBUC 	equ = -1;
	TB_ROLE	role = TB_setup_get_role();
	switch( role )
	{
		case ROLE_GRPGW		:	equ = EQU_TYPE_DIST_GGW;
								break;
		case ROLE_TERM1		:
		case ROLE_TERM2		:
		case ROLE_TERM3 	:	equ = ( TB_setup_get_product_info_voltage() == VOLTAGE_HIGH ) ?	\
										EQU_TYPE_DIST_HIGH_TERM :	\
										EQU_TYPE_DIST_LOW_TERM;
								break;

		case ROLE_RELAY1	:
		case ROLE_RELAY2	:
		case ROLE_RELAY3	:	equ = EQU_TYPE_DIST_LOW_RLY;
								break;
	}

	return equ;
}

/*
*	외부 장치로 전송
*/
int TB_ems_make_outer_packet( TB_EMS_BUF *p_dest, TBUC *p_data, TBUS size )
{
	int ret = -1;

	if( p_dest && p_data && size > 0 )
	{
		bzero( p_dest, sizeof(TB_EMS_BUF) );
		p_dest->length = TB_wisun_util_ems_make_packet( p_dest->buffer,			\
														sizeof(p_dest->buffer), \
														p_data,					\
														size );

		ret = 0;
	}

	return ret;
}

/*
*	내부 EMS 포트로 전송
*/
int TB_ems_make_ems_data( TB_EMS_BUF *p_ems_buf, TB_EMS_BUF *p_udb_buf )
{
	int ret = -1;
	if( p_ems_buf && p_udb_buf )
	{
		TBUS crc;
		TBUS dest = TB_setup_get_product_info_ems_dest();
		TBUS sour = TB_setup_get_product_info_ems_addr();

		bzero( p_ems_buf, sizeof(TB_EMS_BUF) );

		int idx = 0;

		p_ems_buf->buffer[idx] = STX;									idx ++;
		p_ems_buf->buffer[idx] = TB_ems_get_equ();						idx ++;
		p_ems_buf->buffer[idx] = ((p_udb_buf->length-LEN_BYTE_TAG)>> 0) & 0xFF;		idx ++;
		p_ems_buf->buffer[idx] = ((p_udb_buf->length-LEN_BYTE_TAG)>> 8) & 0xFF;		idx ++;
		p_ems_buf->buffer[idx] = (dest >> 0) & 0xFF;					idx ++;
		p_ems_buf->buffer[idx] = (dest >> 8) & 0xFF;					idx ++;
		p_ems_buf->buffer[idx] = (sour >> 0) & 0xFF;					idx ++;
		p_ems_buf->buffer[idx] = (sour >> 8) & 0xFF;					idx ++;
		/*
		*	p_udb_buf : TAG + ORG + CTRL + UDB
		*/
		wmemcpy( &p_ems_buf->buffer[idx], MAX_DNP_BUF-idx, &p_udb_buf->buffer[0], p_udb_buf->length );
		idx += p_udb_buf->length;

		crc = TB_crc16_modbus_get( &p_ems_buf->buffer[EMS_PACKET_OFFSET_EQU], idx-1 );	//	EQU ~ UDB
		p_ems_buf->buffer[idx] = (crc >> 0) & 0xFF;						idx ++;
		p_ems_buf->buffer[idx] = (crc >> 8) & 0xFF;						idx ++;
		p_ems_buf->buffer[idx] = ETX;									idx ++;

		p_ems_buf->length = idx;

		ret = 0;
	}
	else
	{
		TB_dbg_ems( "ERROR. make ems data.\r\n" );
	}

	return ret;
}

static int tb_ems_make_udb_ack_data_info( TB_EMS_ACK_DATA_STATE_INFO *p_info )
{
	int ret = -1;

	if( p_info )
	{
		bzero( p_info, sizeof(TB_EMS_ACK_DATA_STATE_INFO) );

		TB_VERSION *p_ver = TB_setup_get_product_info_version();
		wmemcpy( &p_info->version, sizeof(p_info->version), p_ver, sizeof(TB_VERSION) );

		TBUS serial = TB_setup_get_product_info_ems_addr();
		wmemcpy( &p_info->serial, sizeof(p_info->serial), &serial, sizeof(serial) );

		TB_DATE date = TB_setup_get_product_info_manuf_date();
		wmemcpy( &p_info->manuf_date, sizeof(p_info->manuf_date), &date, sizeof(date) );

		ret = 0;
	}

	return ret;
}

static int tb_ems_make_udb_ack_data_dbm( TB_EMS_ACK_DATA_STATE_DBM *p_comm_dbm )
{
	int ret = -1;

	if( p_comm_dbm )
	{
		int i;
		int temp = 0;

		bzero( p_comm_dbm, sizeof(TB_EMS_ACK_DATA_STATE_DBM) );

		TB_ROLE role = TB_setup_get_role();
		if( role == ROLE_GRPGW )
		{
			for( i=0; i<MAX_CONNECT_CNT; i++ )
			{
				p_comm_dbm->dbm[i].role 	= ROLE_TERM;
				p_comm_dbm->dbm[i].serial 	= g_serial_lower[i];
				p_comm_dbm->dbm[i].dbm 		= TB_rssi_get_comm_info_dbm( 0, i );
				p_comm_dbm->dbm[i].percent	= TB_rssi_get_comm_info_percent( 0, i );
			}
			p_comm_dbm->count = MAX_CONNECT_CNT;
		}
		else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
		{
			if( TB_setup_get_product_info_voltage() == VOLTAGE_LOW )
			{
				TB_WISUN_MODE mode = TB_wisun_get_mode( 0 );
				if( mode == WISUN_MODE_PANC )
				{
					for( i=0; i<MAX_CONNECT_CNT; i++ )
					{
						p_comm_dbm->dbm[i].role 	= ROLE_RELAY;
						p_comm_dbm->dbm[i].serial 	= g_serial_lower[i];
						p_comm_dbm->dbm[i].dbm 		= TB_rssi_get_comm_info_dbm( 0, i );
						p_comm_dbm->dbm[i].percent	= TB_rssi_get_comm_info_percent( 0, i );
					}
					p_comm_dbm->count = MAX_CONNECT_CNT;
				}
				else
				{
					p_comm_dbm->dbm[0].role 	= ROLE_GRPGW;
					p_comm_dbm->dbm[0].serial 	= g_serial_upper;
					p_comm_dbm->dbm[0].dbm 		= TB_rssi_get_comm_info_dbm( 1, 0 );
					p_comm_dbm->dbm[0].percent	= TB_rssi_get_comm_info_percent( 1, 0 );
					p_comm_dbm->count ++;
				}
			}
			else
			{
				p_comm_dbm->dbm[0].role 	= ROLE_GRPGW;
				p_comm_dbm->dbm[0].serial 	= g_serial_upper;
				p_comm_dbm->dbm[0].dbm 		= TB_rssi_get_comm_info_dbm( 0, 0 );
				p_comm_dbm->dbm[0].percent	= TB_rssi_get_comm_info_percent( 0, 0 );
				p_comm_dbm->count 			= 1;
			}
		}
		else if( role >= ROLE_RELAY1 && role <= ROLE_RELAY3 )
		{
			p_comm_dbm->dbm[0].role 	= ROLE_TERM;
			p_comm_dbm->dbm[0].serial 	= g_serial_upper;
			p_comm_dbm->dbm[0].dbm 		= TB_rssi_get_comm_info_dbm( 0, 0 );
			p_comm_dbm->dbm[0].percent	= TB_rssi_get_comm_info_percent( 0, 0 );
			p_comm_dbm->count = 1;
		}

		ret = 0;
	}
	else
	{
		TB_dbg_ems("---> FUNC_ORG_STATE_CTRL_REQ_COMM_DBM \r\n");
	}

	return ret;
}

static int tb_ems_make_udb_ack_data_time( TB_EMS_ACK_DATA_STATE_TIME *p_time_info )
{
	int ret = -1;

	if( p_time_info )
	{
		p_time_info->t = time( NULL );
		ret = 0;
	}

	return ret;
}


static TBUC udb_temp[1024];
static TBUI udb_temp_len = 0;
int TB_ems_make_udb( TBUC org, TBUC ctrl, TBUC ack_org, TBUC ack_ctrl, TB_EMS_BUF *p_udb )
{
	int ret = -1;

	if( p_udb )
	{
		bzero( udb_temp, sizeof(udb_temp) );
		udb_temp_len = 0;

		/*
		*	이 명령만 통신장치에서 EMS로 요청한다.
		*/
		if( org == FUNC_ORG_AUTH &&	ctrl == FUNC_ORG_AUTH_CTRL_AUTH_REQ )
		{
			udb_temp[udb_temp_len] = org;	udb_temp_len++;
			udb_temp[udb_temp_len] = ctrl;	udb_temp_len++;
		}
		/*
		*	EMS에서 요청(REQ)에 의해서 응답(ACK)을 한다.
		*/
		else
		{		
			udb_temp[udb_temp_len] = ack_org;	udb_temp_len++;
			udb_temp[udb_temp_len] = ack_ctrl;	udb_temp_len++;
		}

		bzero( p_udb, sizeof(TB_EMS_BUF) );
		switch( org )
		{
			case FUNC_ORG_AUTH :
				switch( ctrl )
				{
					case FUNC_ORG_AUTH_CTRL_SESSION_KEY_SEND :
						ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_MASTER );
						break;
					case FUNC_ORG_AUTH_CTRL_AUTH_REQ :
						{
							TB_EMS_REQ_AUTH auth_info;
							
							wmemcpy( &auth_info.cot_ip[0], sizeof(auth_info.cot_ip), TB_setup_get_product_info_cot_ip(), sizeof(auth_info.cot_ip) );
							auth_info.rt_port = TB_setup_get_product_info_rt_port();
							auth_info.serial  = TB_setup_get_product_info_ems_addr();

							wmemcpy( &udb_temp[udb_temp_len], sizeof(udb_temp)-udb_temp_len, &auth_info, sizeof(TB_EMS_REQ_AUTH) );
							udb_temp_len += sizeof(TB_EMS_REQ_AUTH);
							ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_MASTER );
						}
						break;
				}
				break;
			case FUNC_ORG_STATE :
				switch( ctrl )
				{
					case FUNC_ORG_STATE_CTRL_REQ_COMM :
						ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_NONE );
						break;

					case FUNC_ORG_STATE_CTRL_REQ_INFO :						
						{
							TB_EMS_ACK_DATA_STATE_INFO info;

							tb_ems_make_udb_ack_data_info( &info );

							wmemcpy( &udb_temp[udb_temp_len], sizeof(udb_temp)-udb_temp_len, &info, sizeof(TB_EMS_ACK_DATA_STATE_INFO) );
							udb_temp_len += sizeof(TB_EMS_ACK_DATA_STATE_INFO);
							ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_NONE );
						}
						break;

					case FUNC_ORG_STATE_CTRL_REQ_COMM_DBM :
						{
							TB_EMS_ACK_DATA_STATE_DBM comm_dbm;

							TB_dbg_ems("---> FUNC_ORG_STATE_CTRL_REQ_COMM_DBM \r\n");
							tb_ems_make_udb_ack_data_dbm( &comm_dbm );

							wmemcpy( &udb_temp[udb_temp_len], sizeof(udb_temp)-udb_temp_len, &comm_dbm, sizeof(TB_EMS_ACK_DATA_STATE_DBM) );
							udb_temp_len += sizeof(TB_EMS_ACK_DATA_STATE_DBM);
							ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_NONE );
						}
						break;

					case FUNC_ORG_STATE_CTRL_REQ_TIME :
						{
							TB_EMS_ACK_DATA_STATE_TIME time_info;

							tb_ems_make_udb_ack_data_time( &time_info );

							wmemcpy( &udb_temp[udb_temp_len], sizeof(udb_temp)-udb_temp_len, &time_info, sizeof(TB_EMS_ACK_DATA_STATE_TIME) );
							udb_temp_len += sizeof(TB_EMS_ACK_DATA_STATE_TIME);
							ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_NONE );
						}
						break;

					case FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO :
						{
							int temp = 0;
							TB_EMS_ACK_DATA_STATE_INFO 	info;
							TB_EMS_ACK_DATA_STATE_DBM 	comm_dbm;
							TB_EMS_ACK_DATA_STATE_TIME 	time_info;

							TB_dbg_ems("FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO \r\n");

							tb_ems_make_udb_ack_data_info( &info );
							wmemcpy( &udb_temp[udb_temp_len], sizeof(udb_temp)-udb_temp_len, &info, sizeof(TB_EMS_ACK_DATA_STATE_INFO) );
							udb_temp_len += sizeof(TB_EMS_ACK_DATA_STATE_INFO);

							tb_ems_make_udb_ack_data_dbm( &comm_dbm );
							wmemcpy( &udb_temp[udb_temp_len], sizeof(udb_temp)-udb_temp_len, &comm_dbm, sizeof(TB_EMS_ACK_DATA_STATE_DBM) );
							udb_temp_len += sizeof(TB_EMS_ACK_DATA_STATE_DBM);
							
							tb_ems_make_udb_ack_data_time( &time_info );
							wmemcpy( &udb_temp[udb_temp_len], sizeof(udb_temp)-udb_temp_len, &time_info, sizeof(TB_EMS_ACK_DATA_STATE_TIME) );
							udb_temp_len += sizeof(TB_EMS_ACK_DATA_STATE_TIME);

							ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_NONE );
						}
						break;

					default :
						ret = -1;
						break;
				}
				break;

			case FUNC_ORG_SET :
				switch( ctrl )
				{
					case FUNC_ORG_SET_CTRL_REQ_TIME_SYNC :
						ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_NONE );
						break;
				}
				break;

			case FUNC_ORG_CTRL :
				switch( ctrl )
				{
					case FUNC_ORG_CTRL_CTRL_REQ_RESET :
						ret = TB_ems_encryption_data( udb_temp, udb_temp_len, &p_udb->buffer[0], &p_udb->length, KCMVP_KEY_TYPE_NONE );
						break;
				}
				break;

		}
	}

	return ret;
}

int TB_ems_make_ack_data( TB_EMS_PACKET *p_packet, TB_EMS_BUF *p_ems_ack_buf )
{
	int ret = -1;
	
	if( p_packet && p_ems_ack_buf )
	{
		bzero( p_ems_ack_buf, sizeof(TB_EMS_BUF) );
		switch( p_packet->org )
		{
			case FUNC_ORG_AUTH 	:
				switch( p_packet->ctrl )
				{
					//	EMS로부터 재인증요청을 받으면, 무조건 인증요청을 진행시키기
					//	위하여 EMS_AUTH_STATE_RETRY_FORCE 명령을 사용한다.
					case FUNC_ORG_AUTH_CTRL_REAUTH_REQ_SEND :
						TB_ems_set_auth_status( EMS_AUTH_STATE_RETRY_FORCE );
						break;
						
					case FUNC_ORG_AUTH_CTRL_AUTH_RESULT :
						TB_dbg_ems("RECV. FUNC_ORG_AUTH_CTRL_AUTH_RESULT\r\n");
						if( p_packet->udb_len == 1 )
						{
							if( p_packet->udb[0] == 1 )
							{
								TB_dbg_ems("     SUCCESS. FUNC_ORG_AUTH_CTRL_AUTH_RESULT\r\n");
								TB_ems_set_auth_status( EMS_AUTH_STATE_SUCCESS );
							}
							else
							{
								TB_dbg_ems("     FAIL. FUNC_ORG_AUTH_CTRL_AUTH_RESULT\r\n");
								TB_ems_set_auth_status( EMS_AUTH_STATE_RETRY );
							}
						}						
						break;
						
					case FUNC_ORG_AUTH_CTRL_SESSION_KEY_SEND :
						if( p_packet->udb_len == LEN_BYTE_KEY )
						{
							TB_KCMVP_KEY session_key;
														
							wmemcpy( &session_key.data[0], sizeof(session_key.data), &p_packet->udb[0], p_packet->udb_len );
							session_key.size = p_packet->udb_len;
							TB_kcmvp_set_keyinfo_session_key( &session_key );
							TB_ems_set_auth_status( EMS_AUTH_STATE_RECV_KEY );

							if( TB_dm_is_on(DMID_EMS) )
								TB_util_data_dump("SESSION_KEY", &session_key.data[0], session_key.size );

							TB_dbg_ems("SESSION KEY RECV. -----> Send ACK\r\n");

							TB_ems_make_udb( FUNC_ORG_AUTH, FUNC_ORG_AUTH_CTRL_SESSION_KEY_SEND, FUNC_ORG_AUTH, FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV, &s_ems_buf_udb );
							ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
							if( ret == 0 )
							{
								TB_ems_set_auth_status( EMS_AUTH_STATE_RECV_KEY_ACK );
								TB_elapse_set_stop( &s_elapse_auth );
								TB_ems_set_auth_status( EMS_AUTH_STATE_FINISH );

								s_ems_auth_req_cnt = 0;
							}
						}
						break;
				}
				break;
				
			case FUNC_ORG_STATE :
				switch( p_packet->ctrl )
				{
					case FUNC_ORG_STATE_CTRL_REQ_COMM		:
						TB_dbg_ems("RECV. FUNC_ORG_STATE_CTRL_REQ_COMM\r\n");
						TB_ems_make_udb( FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_REQ_COMM, FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_ACK_COMM,&s_ems_buf_udb );
						ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
						break;
					
					case FUNC_ORG_STATE_CTRL_REQ_INFO		:
						TB_dbg_ems("RECV. FUNC_ORG_STATE_CTRL_REQ_INFO\r\n");
						TB_ems_make_udb( FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_REQ_INFO, FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_ACK_INFO, &s_ems_buf_udb );
						ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
						break;
					
					case FUNC_ORG_STATE_CTRL_REQ_COMM_DBM	:
						TB_dbg_ems("RECV. FUNC_ORG_STATE_CTRL_REQ_COMM_DBM\r\n");
						TB_ems_make_udb( FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_REQ_COMM_DBM, FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_ACK_COMM_DBM,&s_ems_buf_udb );
						ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
						break;
					
					case FUNC_ORG_STATE_CTRL_REQ_TIME		:
						TB_dbg_ems("RECV. FUNC_ORG_STATE_CTRL_REQ_TIME\r\n");
						TB_ems_make_udb( FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_REQ_TIME, FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_ACK_TIME,&s_ems_buf_udb );
						ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
						break;
					
					case FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO	:
						TB_dbg_ems("RECV. FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO\r\n");
						TB_ems_make_udb( FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO, FUNC_ORG_STATE, FUNC_ORG_STATE_CTRL_ACK_DEVICE_INFO, &s_ems_buf_udb );
						ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
						break;
				}
				break;
				
			case FUNC_ORG_SET 	:
				switch( p_packet->ctrl )
				{
					case FUNC_ORG_SET_CTRL_REQ_TIME_SYNC :
						TB_dbg_ems("RECV. FUNC_ORG_SET_CTRL_REQ_TIME_SYNC\r\n");
						{
							tb_ems_packet_dump( p_packet, __FILE__, __LINE__ );
							
							time_t t = *(time_t *)&p_packet->udb[0];
							t += (9*60*60);	//	UTC+0 --> UTC+9(Korea)
							TB_util_set_systemtime2( t );

							TB_ems_make_udb( FUNC_ORG_SET, FUNC_ORG_SET_CTRL_REQ_TIME_SYNC, FUNC_ORG_SET, FUNC_ORG_SET_CTRL_ACK_TIME_SYNC,&s_ems_buf_udb );
							ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
						}
						break;
				}
				break;
				
			case FUNC_ORG_CTRL	:
				switch( p_packet->ctrl )
				{
					case FUNC_ORG_CTRL_CTRL_REQ_RESET	:
						TB_dbg_ems("RECV. FUNC_ORG_CTRL_CTRL_REQ_RESET\r\n");
						TB_ems_make_udb( FUNC_ORG_CTRL, FUNC_ORG_CTRL_CTRL_REQ_RESET, FUNC_ORG_CTRL, FUNC_ORG_CTRL_CTRL_ACK_RESET,&s_ems_buf_udb );
						ret = TB_ems_make_ems_data( p_ems_ack_buf, &s_ems_buf_udb );
						if( ret == 0 )
						{
							TB_wdt_reboot( WDT_COND_EMS_REBOOT );
						}
						break;
				}

				break;
		}
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

TBUC TB_ems_auth_req( void )
{
	TB_MESSAGE msg;
	TB_EMS_BUF ems_buf;
	TB_EMS_BUF ems_send_buf;
	TB_EMS_BUF udb;

	TB_dbg_ems( "Auth REQ\r\n" );

	TB_ems_make_udb( FUNC_ORG_AUTH, FUNC_ORG_AUTH_CTRL_AUTH_REQ, 0xFF, 0xFF, &udb );
	TB_ems_make_ems_data( &ems_buf, &udb );

	TB_ROLE role = TB_setup_get_role();
	if( role == ROLE_GRPGW )
	{
		TB_ems_write( ems_buf.buffer, ems_buf.length );
	}
	else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
	{
		TBSI comm_type = TB_setup_get_comm_type_master();
		switch( comm_type )
		{
			case COMM_MODE_MASTER_WISUN :
				{
					int wisun_idx = WISUN_FIRST;
					if( TB_setup_get_product_info_voltage() == VOLTAGE_LOW )
						wisun_idx = WISUN_SECOND;
					
					TB_ems_make_outer_packet( &ems_send_buf, &ems_buf.buffer[0], ems_buf.length );
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_TERM2GGW );
					//TB_wisun_proc_mesgq_select_encryption_type2( &ems_send_buf );
					TB_kcmvp_set_keyinfo_key_type( KCMVP_KEY_TYPE_MASTER );
					TB_packet_send( wisun_idx, MSG_CMD_WISUN_SEND_TERM2GGW, &ems_send_buf.buffer[0], ems_send_buf.length );
				}
				break;
		}
	}
	else if( role >= ROLE_RELAY1 && role <= ROLE_RELAY3 )
	{
		TB_ems_make_outer_packet( &ems_send_buf, &ems_buf.buffer[0], ems_buf.length );
		TB_packet_set_send_direction( WISUN_FIRST, WISUN_DIR_RELAY2TERM );
		//TB_wisun_proc_mesgq_select_encryption_type2( &ems_send_buf );
		TB_kcmvp_set_keyinfo_key_type( KCMVP_KEY_TYPE_MASTER );
		TB_packet_send( WISUN_FIRST, MSG_CMD_WISUN_SEND_RELAY2TERM, &ems_send_buf.buffer[0], ems_send_buf.length );
	}

	////////////////////////////////////////////////////////////////////////////

	s_ems_auth_req_cnt ++;
	TB_dbg_ems("AUTH REQ - TRY COUNT = %d\r\n", s_ems_auth_req_cnt );
	TB_dbg_ems("AUTH REQ - TRY COUNT = %d\r\n", s_ems_auth_req_cnt );
	TB_dbg_ems("AUTH REQ - TRY COUNT = %d\r\n", s_ems_auth_req_cnt );
	TB_dbg_ems("AUTH REQ - TRY COUNT = %d\r\n", s_ems_auth_req_cnt );
	
	return s_ems_auth_state;
}

////////////////////////////////////////////////////////////////////////////////

#include "TB_test.h"
pthread_t 	g_thid_ems_test;
int 		g_proc_flag_ems_test = 1;
static void *tb_ems_test_proc( void *arg )
{
	printf("===========================================\r\n" );
	printf("             Start EMS TEST Proc           \r\n" );
	printf("===========================================\r\n" );

	time_t cur_t, old_t;
    cur_t = old_t = time(NULL);
	
	int check = 0;
	TBUC buf1[32] = "1234567890";
	TBUC buf2[32] = "12345";
	TBUC buf_read[128];
	int normal = 0;
	TBUS len = 0;
	while( g_proc_flag_ems_test )
	{		
		cur_t = time(NULL);
        if( cur_t != old_t )
		{
			TBUC buffer[128] = {0, };
			TBUS s = 0;
			TBUC *p	= (check == 0) ? (TBUC *)&buf1[0] :	\
									 (TBUC *)&buf2[0];
			check = !check;
			
			snprintf( buffer, sizeof(buffer), "[%s] %s\r\n", "EMS", p );
			s = wstrlen( buffer );
			if( s > 0 )
			{
				len = TB_uart_ems_write( buffer, s );
				if( len == s )
				{
					TB_dbg_ems( "EMS WRITE = %s\r\n", buffer );
					normal |= SUCCESS_WRITE;
				}
			}
			
			old_t = cur_t;
        }

		bzero( buf_read, sizeof(buf_read) );
		len = TB_uart_ems_read( buf_read, sizeof(buf_read) ) ;
        if( len > 1 )
        {
			TB_dbg_ems( "EMS READ = %s\r\n", buf_read );
			normal |= SUCCESS_READ;
        }

		if( (normal & (SUCCESS_WRITE|SUCCESS_READ)) == (SUCCESS_WRITE|SUCCESS_READ) )
		{
			TB_testmode_set_test_result( TEST_ITEM_EMS, TRUE );
		}

		TB_util_delay( 2000 );
	}
}

void TB_ems_test_proc_start( void )
{
	pthread_create( &g_thid_ems_test, NULL, tb_ems_test_proc, NULL );
}

void TB_ems_test_proc_stop( void )
{
	g_proc_flag_ems_test = 0; 
	pthread_join( g_thid_ems_test, NULL );
}

