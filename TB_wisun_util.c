#include "TB_msg_queue.h"
#include "TB_wisun.h"
#include "TB_packet.h"
#include "TB_util.h"
#include "TB_debug.h"
#include "TB_log.h"
#include "TB_wisun_util.h"

////////////////////////////////////////////////////////////////////////////////

extern TBBL g_psu_get_time;

////////////////////////////////////////////////////////////////////////////////

volatile int g_check_ip = 0;
volatile int g_check_time = 0;

////////////////////////////////////////////////////////////////////////////////

#define HEAD_TIME_INFO		0x11111111
#define TAIL_TIME_INFO		0x22222222
#define HEAD_IPADDR_INFO	0x33333333
#define TAIL_IPADDR_INFO	0x44444444
#define HEAD_PING_INFO		0x55555555
#define TAIL_PING_INFO		0x66666666
#define HEAD_PONG_INFO		0x77777777
#define TAIL_PONG_INFO		0x88888888
#define HEAD_EMS_INFO		0x99999999
#define TAIL_EMS_INFO		0xAAAAAAAA
#define HEAD_ACK_INFO		0xBBBBBBBB
#define TAIL_ACK_INFO		0xCCCCCCCC

////////////////////////////////////////////////////////////////////////////////

static TBBL tb_wisun_util_ack_set_head( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (HEAD_ACK_INFO >> 24) & 0xFF;
		buf[1] = (HEAD_ACK_INFO >> 16) & 0xFF;
		buf[2] = (HEAD_ACK_INFO >>  8) & 0xFF;
		buf[3] = (HEAD_ACK_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ack_set_tail( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (TAIL_ACK_INFO >> 24) & 0xFF;
		buf[1] = (TAIL_ACK_INFO >> 16) & 0xFF;
		buf[2] = (TAIL_ACK_INFO >>  8) & 0xFF;
		buf[3] = (TAIL_ACK_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ack_check_head( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((HEAD_ACK_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((HEAD_ACK_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((HEAD_ACK_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((HEAD_ACK_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

static TBBL tb_wisun_util_ack_check_tail( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((TAIL_ACK_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((TAIL_ACK_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((TAIL_ACK_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((TAIL_ACK_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

TBUS TB_wisun_util_ack_make_packet( TBUC *p_dest )
{
	TBUS len = 0;
	if( p_dest )
	{	
		tb_wisun_util_ack_set_head( &p_dest[len] );		len += 4;
		tb_wisun_util_ack_set_tail( &p_dest[len] );		len += 4;		
	}
	
	return len;
}

TBBL TB_wisun_util_ack_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;
		if( tb_wisun_util_ack_check_head( &buf[head] ) &&	\
			tb_wisun_util_ack_check_tail( &buf[tail] ) )
		{
			ret = TRUE;
		}
	}

	return  ret;
}

////////////////////////////////////////////////////////////////////////////////

static TBBL tb_wisun_util_ems_set_head( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (HEAD_EMS_INFO >> 24) & 0xFF;
		buf[1] = (HEAD_EMS_INFO >> 16) & 0xFF;
		buf[2] = (HEAD_EMS_INFO >>  8) & 0xFF;
		buf[3] = (HEAD_EMS_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ems_set_tail( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (TAIL_EMS_INFO >> 24) & 0xFF;
		buf[1] = (TAIL_EMS_INFO >> 16) & 0xFF;
		buf[2] = (TAIL_EMS_INFO >>  8) & 0xFF;
		buf[3] = (TAIL_EMS_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ems_check_head( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((HEAD_EMS_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((HEAD_EMS_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((HEAD_EMS_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((HEAD_EMS_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

static TBBL tb_wisun_util_ems_check_tail( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((TAIL_EMS_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((TAIL_EMS_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((TAIL_EMS_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((TAIL_EMS_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

TBUS TB_wisun_util_ems_make_packet( TBUC *p_dest, size_t dest_size, TBUC *p_sour, TBUS sour_len )
{
	TBUS len = 0;
	if( p_dest && p_sour && sour_len > 0 )
	{	
		tb_wisun_util_ems_set_head( &p_dest[len] );
		len += 4;
		
		wmemcpy( &p_dest[len], dest_size-len, &p_sour[0], sour_len );
		len += sour_len;
		
		tb_wisun_util_ems_set_tail( &p_dest[len] );
		len += 4;
	}
	
	return len;
}

TBBL TB_wisun_util_ems_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;
		if( tb_wisun_util_ems_check_head( &buf[head] ) &&	\
			tb_wisun_util_ems_check_tail( &buf[tail] ) )
		{
			ret = TRUE;
		}
	}

	return  ret;
}

////////////////////////////////////////////////////////////////////////////////

static TBBL tb_wisun_util_ping_set_head( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (HEAD_PING_INFO >> 24) & 0xFF;
		buf[1] = (HEAD_PING_INFO >> 16) & 0xFF;
		buf[2] = (HEAD_PING_INFO >>  8) & 0xFF;
		buf[3] = (HEAD_PING_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ping_set_tail( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (TAIL_PING_INFO >> 24) & 0xFF;
		buf[1] = (TAIL_PING_INFO >> 16) & 0xFF;
		buf[2] = (TAIL_PING_INFO >>  8) & 0xFF;
		buf[3] = (TAIL_PING_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ping_check_head( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((HEAD_PING_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((HEAD_PING_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((HEAD_PING_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((HEAD_PING_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

static TBBL tb_wisun_util_ping_check_tail( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((TAIL_PING_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((TAIL_PING_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((TAIL_PING_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((TAIL_PING_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

TBBL TB_wisun_util_ping_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;
		if( tb_wisun_util_ping_check_head( &buf[head] ) &&	\
			tb_wisun_util_ping_check_tail( &buf[tail] ) )
		{
			if( buf[4] == 'P' &&
				buf[5] == 'I' &&
				buf[6] == 'N' &&
				buf[7] == 'G' )
			{
				ret = TRUE;
			}
		}
	}

	return  ret;
}

////////////////////////////////////////////////////////////////////////////////

static TBBL tb_wisun_util_pong_set_head( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (HEAD_PONG_INFO >> 24) & 0xFF;
		buf[1] = (HEAD_PONG_INFO >> 16) & 0xFF;
		buf[2] = (HEAD_PONG_INFO >>  8) & 0xFF;
		buf[3] = (HEAD_PONG_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_pong_set_tail( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (TAIL_PONG_INFO >> 24) & 0xFF;
		buf[1] = (TAIL_PONG_INFO >> 16) & 0xFF;
		buf[2] = (TAIL_PONG_INFO >>  8) & 0xFF;
		buf[3] = (TAIL_PONG_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_pong_check_head( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((HEAD_PONG_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((HEAD_PONG_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((HEAD_PONG_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((HEAD_PONG_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

static TBBL tb_wisun_util_pong_check_tail( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((TAIL_PONG_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((TAIL_PONG_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((TAIL_PONG_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((TAIL_PONG_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

TBBL TB_wisun_util_pong_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;
		if( tb_wisun_util_pong_check_head( &buf[head] ) &&	\
			tb_wisun_util_pong_check_tail( &buf[tail] ) )
		{
			if( buf[4] == 'P' &&
				buf[5] == 'O' &&
				buf[6] == 'N' &&
				buf[7] == 'G' )
			{
				ret = TRUE;
			}
		}
	}

	return  ret;
}

////////////////////////////////////////////////////////////////////////////////

static TBBL tb_wisun_util_timeinfo_set_head( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (HEAD_TIME_INFO >> 24) & 0xFF;
		buf[1] = (HEAD_TIME_INFO >> 16) & 0xFF;
		buf[2] = (HEAD_TIME_INFO >>  8) & 0xFF;
		buf[3] = (HEAD_TIME_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_timeinfo_set_tail( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (TAIL_TIME_INFO >> 24) & 0xFF;
		buf[1] = (TAIL_TIME_INFO >> 16) & 0xFF;
		buf[2] = (TAIL_TIME_INFO >>  8) & 0xFF;
		buf[3] = (TAIL_TIME_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBUS tb_wisun_util_timeinfo_res_packet_make( TBUC *buf )
{
	TBUS len = 0;
	if( buf )
	{	
		time_t t = time( NULL );
	
		tb_wisun_util_timeinfo_set_head( &buf[len] );	len += 4;
		buf[len] = 'R';									len ++;
		buf[len] = 'E';									len ++;
		buf[len] = 'S';									len ++;
		buf[len] = 'T';									len ++;
		buf[len] = 'I';									len ++;
		buf[len] = 'M';									len ++;
		buf[len] = 'E';									len ++;
		buf[len] = (t >>  0) & 0xFF;					len ++;
		buf[len] = (t >>  8) & 0xFF;					len ++;
		buf[len] = (t >> 16) & 0xFF;					len ++;
		buf[len] = (t >> 24) & 0xFF;					len ++;
		tb_wisun_util_timeinfo_set_tail( &buf[15] );	len += 4;
	}
	
	return len;
}

static TBUS tb_wisun_util_timeinfo_req_packet_make( TBUC *buf )
{
	TBUS len = 0;
	if( buf )
	{
		tb_wisun_util_timeinfo_set_head( &buf[len] );	len += 4;
		buf[len] = TB_get_role();						len ++;
		buf[len] = 'R';									len ++;
		buf[len] = 'E';									len ++;
		buf[len] = 'Q';									len ++;
		buf[len] = 'T';									len ++;
		buf[len] = 'I';									len ++;
		buf[len] = 'M';									len ++;
		buf[len] = 'E';									len ++;
		tb_wisun_util_timeinfo_set_tail( &buf[12] );	len += 4;
	}
	
	return len;
}

static TBBL tb_wisun_util_timeinfo_check_head( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((HEAD_TIME_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((HEAD_TIME_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((HEAD_TIME_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((HEAD_TIME_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

static TBBL tb_wisun_util_timeinfo_check_tail( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((TAIL_TIME_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((TAIL_TIME_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((TAIL_TIME_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((TAIL_TIME_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

TBBL TB_wisun_util_timeinfo_res_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;

		if( tb_wisun_util_timeinfo_check_head( &buf[head] ) &&	\
			tb_wisun_util_timeinfo_check_tail( &buf[tail] ) )
		{
			if( buf[ 4] == 'R' &&	\
				buf[ 5] == 'E' &&	\
				buf[ 6] == 'S' &&	\
				buf[ 7] == 'T' &&	\
				buf[ 8] == 'I' &&	\
				buf[ 9] == 'M' &&	\
				buf[10] == 'E' )
			{
				ret = TRUE;
			}
			else
			{
				TB_dbg_wisun( "ERROR. timeinfo res packet : %c%c%c%c%c%c%c\r\n", buf[4],
																				 buf[5],
																				 buf[6],
																				 buf[7],
																				 buf[8],
																				 buf[9],
																				 buf[10]  );
			}
		}
	}

	return  ret;
}

int TB_wisun_util_timeinfo_res_send( int idx, TB_ROLE role_src, TB_ROLE role_dst )
{
	int ret = 0;

	if( g_psu_get_time == TRUE )
	{
		TB_dbg_wisun("[TimeInfo Respose] idx=%d, src=%d, dest=%d\r\n", idx, role_src, role_dst );
		
		if( role_src >= ROLE_TERM1 && role_src <= ROLE_TERM3 )
		{
			TBUC info[128];
			TBUS len = tb_wisun_util_timeinfo_res_packet_make( info );
			switch( role_dst )
			{
				case ROLE_GRPGW	:
					TB_packet_set_send_direction( idx, WISUN_DIR_TERM2GGW );
					TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2GGW, info, len );	//	19 is head(4) + 'RESTIME'(7) + time(4) + tail(4)
					break;

				case ROLE_RELAY1 :
					TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY1 );
					TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2RELAY1, info, len );
					break;
				case ROLE_RELAY2 :
					TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY2 );
					TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2RELAY2, info, len );
					break;
				case ROLE_RELAY3 :
					TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY3 );
					TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2RELAY3, info, len );
					break;
				default			:
					TB_dbg_wisun("Error. Invalid role destination : %d\r\n", role_dst );
					break;
			}
		}
		else
		{
			TB_dbg_wisun("Unknown restime src : %d\r\n", role_src );
			ret = -1;
		}
	}

	return ret;
}

/*
*	Terminal에서 다른 장치들에게 시간 정보 요청이 가능하다고 알려줌.
*/
int TB_wisun_util_timeinfo_req_send_avaliable_timeinfo( int idx, TB_ROLE role_src, TB_WISUN_MODE mode_src )
{
	int ret = 0;

	if( g_psu_get_time == TRUE )
	{	
		if( role_src >= ROLE_TERM1 && role_src <= ROLE_TERM3 )
		{
			if( mode_src == WISUN_MODE_ENDD )
			{
				TB_packet_set_send_direction( idx, WISUN_DIR_TERM2GGW );
				TB_packet_send( idx, MSG_CMD_WISUN_SEND_REQ_AVAILABLE_TIMEINFO, NULL, 0 );
			}

			if( mode_src == WISUN_MODE_PANC )
			{
				TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY1 );
				TB_packet_send( idx, MSG_CMD_WISUN_SEND_REQ_AVAILABLE_TIMEINFO, NULL, 0 );

				TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY2 );
				TB_packet_send( idx, MSG_CMD_WISUN_SEND_REQ_AVAILABLE_TIMEINFO, NULL, 0 );

				TB_packet_set_send_direction( idx, WISUN_DIR_TERM2RELAY3 );
				TB_packet_send( idx, MSG_CMD_WISUN_SEND_REQ_AVAILABLE_TIMEINFO, NULL, 0 );
			}
		}
		else
		{
			TB_dbg_wisun("Unknown restime src : %d\r\n", role_src );
			ret = -1;
		}
	}

	return ret;
}

TBBL TB_wisun_util_timeinfo_req_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;

		if( tb_wisun_util_timeinfo_check_head( &buf[head] ) &&	\
			tb_wisun_util_timeinfo_check_tail( &buf[tail] ) )
		{
			/*
			*	buf[4] is ROLE
			*/
			if( buf[ 5] == 'R' &&	\
				buf[ 6] == 'E' &&	\
				buf[ 7] == 'Q' &&	\
				buf[ 8] == 'T' &&	\
				buf[ 9] == 'I' &&	\
				buf[10] == 'M' &&	\
				buf[11] == 'E' )
			{
				ret = TRUE;
			}
			else
			{
				TB_dbg_wisun( "ERROR. timeinfo req packet : %c%c%c%c%c%c%c\r\n",	\
								buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11] );
			}
		}
	}

	return  ret;
}

int TB_wisun_util_timeinfo_req_send( TBBL is_relay, char *file, int line )
{			
	int ret = 0;
	TBUC buf[128];
	
	g_check_time = 0;

	TB_dbg_wisun("--> [%s:%d]SEND. REQ TIMEINFO : %s\r\n", file, line, is_relay ? "RELAY TO TERM" : "GGW TO TERM" );
	TB_dbg_wisun("--> [%s:%d]SEND. REQ TIMEINFO : %s\r\n", file, line, is_relay ? "RELAY TO TERM" : "GGW TO TERM" );
	TB_dbg_wisun("--> [%s:%d]SEND. REQ TIMEINFO : %s\r\n", file, line, is_relay ? "RELAY TO TERM" : "GGW TO TERM" );
	TB_dbg_wisun("--> [%s:%d]SEND. REQ TIMEINFO : %s\r\n", file, line, is_relay ? "RELAY TO TERM" : "GGW TO TERM" );
	TB_dbg_wisun("--> [%s:%d]SEND. REQ TIMEINFO : %s\r\n", file, line, is_relay ? "RELAY TO TERM" : "GGW TO TERM" );
	TB_dbg_wisun("--> [%s:%d]SEND. REQ TIMEINFO : %s\r\n", file, line, is_relay ? "RELAY TO TERM" : "GGW TO TERM" );
	TB_dbg_wisun("--> [%s:%d]SEND. REQ TIMEINFO : %s\r\n", file, line, is_relay ? "RELAY TO TERM" : "GGW TO TERM" );

	TBUS len = tb_wisun_util_timeinfo_req_packet_make( buf );

	if( is_relay )
	{
		TB_packet_set_send_direction( 0, WISUN_DIR_RELAY2TERM );
		TB_packet_send( 0, MSG_CMD_WISUN_SEND_REQ_TIMEINFO_FROM_RELAY, buf, len );
	}
	else	//	group gw
	{
		TB_packet_set_send_direction( 0, WISUN_DIR_GGW2TERM1 );
		TB_packet_send( 0, MSG_CMD_WISUN_SEND_REQ_TIMEINFO_FROM_GGW, buf, len );
	}
	
	return ret;
}

int TB_wisun_util_timeinfo_set_systemtime( TBUC *p_datetime )
{
	int ret = -1;

	if( p_datetime )
	{
		if( g_check_time == 0 )
		{
			time_t t = 	((TBUI)p_datetime[0] <<  0) & 0x000000FF |
						((TBUI)p_datetime[1] <<  8) & 0x0000FF00 |
						((TBUI)p_datetime[2] << 16) & 0x00FF0000 |
						((TBUI)p_datetime[3] << 24) & 0xFF000000;

			struct tm *tm = localtime( &t );
			if( tm )
			{
				TB_util_set_systemtime1( tm->tm_year+1900,	\
										 tm->tm_mon+1,		\
										 tm->tm_mday,		\
										 tm->tm_hour,		\
										 tm->tm_min,		\
										 tm->tm_sec );

				g_check_time = 1;

				TB_log_append( LOG_SECU_TIMESYNC, NULL, -1 );

				TB_dbg_wisun("TIME SYNC\r\n");
				TB_dbg_wisun("TIME SYNC\r\n");
				TB_dbg_wisun("TIME SYNC\r\n");
				TB_dbg_wisun("TIME SYNC\r\n");
				TB_dbg_wisun("TIME SYNC\r\n");
				
				ret = 0;
			}
		}
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

static TBBL tb_wisun_util_ipaddr_set_head( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (HEAD_IPADDR_INFO >> 24) & 0xFF;
		buf[1] = (HEAD_IPADDR_INFO >> 16) & 0xFF;
		buf[2] = (HEAD_IPADDR_INFO >>  8) & 0xFF;
		buf[3] = (HEAD_IPADDR_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ipaddr_set_tail( TBUC *buf )
{
	TBBL ret = FALSE;
	
	if( buf )
	{
		buf[0] = (TAIL_IPADDR_INFO >> 24) & 0xFF;
		buf[1] = (TAIL_IPADDR_INFO >> 16) & 0xFF;
		buf[2] = (TAIL_IPADDR_INFO >>  8) & 0xFF;
		buf[3] = (TAIL_IPADDR_INFO >>  0) & 0xFF;

		ret = TRUE;
	}
	
	return ret;
}

static TBBL tb_wisun_util_ipaddr_check_head( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((HEAD_IPADDR_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((HEAD_IPADDR_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((HEAD_IPADDR_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((HEAD_IPADDR_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

static TBBL tb_wisun_util_ipaddr_check_tail( TBUC *buf )
{
	TBBL ret = ( buf[0] == ((TAIL_IPADDR_INFO >> 24) & 0xFF) &&	\
				 buf[1] == ((TAIL_IPADDR_INFO >> 16) & 0xFF) &&	\
			 	 buf[2] == ((TAIL_IPADDR_INFO >>  8) & 0xFF) &&	\
			 	 buf[3] == ((TAIL_IPADDR_INFO >>  0) & 0xFF) ) ? TRUE : FALSE;

	return ret;
}

static TBUS tb_wisun_util_ipaddr_req_make_packet( TBUC *buf )
{
	TBUS size = 0;
	if( buf )
	{
		TBUS serial = TB_setup_get_product_info_ems_addr();
		TBUS temp = 0;
		
		tb_wisun_util_ipaddr_set_head( &buf[temp] );	temp += 4;
		buf[temp] = 'R';								temp += 1;
		buf[temp] = 'E';								temp += 1;
		buf[temp] = 'Q';								temp += 1;
		buf[temp] = 'I';								temp += 1;
		buf[temp] = 'P';								temp += 1;
		buf[temp] = TB_get_role();						temp += 1;
		buf[temp] = (serial >> 0) & 0xFF;				temp += 1;
		buf[temp] = (serial >> 8) & 0xFF;				temp += 1;
		tb_wisun_util_ipaddr_set_tail( &buf[temp] );	temp += 4;

		size = temp;
	}
	
	return size;
}

TBBL TB_wisun_util_ipaddr_req_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;
		if( tb_wisun_util_ipaddr_check_head( &buf[head] ) &&	\
			tb_wisun_util_ipaddr_check_tail( &buf[tail] ) )
		{
			if( buf[4] == 'R' &&
				buf[5] == 'E' &&
				buf[6] == 'Q' &&
				buf[7] == 'I' &&
				buf[8] == 'P' )
			{
				g_serial_upper = 0;
				g_serial_upper |= (buf[10] << 0);
				g_serial_upper |= (buf[11] << 8);

				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );
				TB_dbg_wisun( "g_serial_upper = %d\r\n", g_serial_upper );

				ret = TRUE;
			}
		}
	}

	return  ret;
}

int TB_wisun_util_ipaddr_req_send( int idx, TB_ROLE role_src, TB_ROLE role_dst )
{
	int ret = 0;
	TBUC buf[128];

	g_check_ip = 0;

	TB_dbg_wisun("SEND. REQ IPADDR : src=%d, dst=%d\r\n", role_src, role_dst );
	TB_dbg_wisun("SEND. REQ IPADDR : src=%d, dst=%d\r\n", role_src, role_dst );

	TBUS size = tb_wisun_util_ipaddr_req_make_packet( buf );

	if( role_src >= ROLE_TERM1 && role_src <= ROLE_TERM3 )
	{
		TB_packet_send( idx, MSG_CMD_WISUN_SEND_REQ_IP_FROM_TERM, buf, size );
	}
	else if( role_src == ROLE_GRPGW )
	{
		TB_packet_send( idx, MSG_CMD_WISUN_SEND_REQ_IP_FROM_GGW, buf, size );
	}

	TB_util_delay( 500 );

	////////////////////////////////////////////////////////////////////////////

	while( g_check_ip == 0 )
	{
		TB_wisun_proc_netq_data( idx, 0 );
		TB_util_delay( 100 );
	}
}

static TBUS tb_wisun_util_ipaddr_res_make_packet( int idx, TBUC *buf )
{
	TBUS size = 0;
	if( buf )
	{
		int temp = 0;
		int i = 0;
		TBUS serial = TB_setup_get_product_info_ems_addr();
		
		tb_wisun_util_ipaddr_set_head( &buf[temp] );	temp += 4;		//	4		
		buf[temp] = 'R';								temp += 1;		//	1
		buf[temp] = 'E';								temp += 1;		//	1
		buf[temp] = 'S';								temp += 1;		//	1
		buf[temp] = 'I';								temp += 1;		//	1
		buf[temp] = 'P';								temp += 1;		//	1
		buf[temp] = TB_get_role();						temp += 1;		//	1
		buf[temp] = (serial >> 0) & 0xFF;				temp += 1;		//	1
		buf[temp] = (serial >> 8) & 0xFF;				temp += 1;		//	1
		for( int i=0; i<16; i++ )
			buf[temp++] = g_ip_adr_my[idx][i];							//	16
		tb_wisun_util_ipaddr_set_tail( &buf[temp] );	temp += 4;		//	4

		size = temp;

		if( TB_dm_is_on(DMID_WISUN) )
		{
			printf("IP Addr ACK Packget\r\n" );
			TB_util_data_dump2( buf, temp );
		}
	}
	
	return size;
}

TBBL TB_wisun_util_ipaddr_res_check_packet( TBUC *buf, TBUS length )
{
	TBBL ret = FALSE;

	if( buf && length > 0 )
	{
		int head = 0;
		int tail = length - 4;
		if( tb_wisun_util_ipaddr_check_head( &buf[head] ) &&	\
			tb_wisun_util_ipaddr_check_tail( &buf[tail] ) )
		{
			if( buf[4] == 'R' &&
				buf[5] == 'E' &&
				buf[6] == 'S' &&
				buf[7] == 'I' &&
				buf[8] == 'P' )
			{
				ret = TRUE;
			}
		}
	}

	return  ret;
}

int TB_wisun_util_ipaddr_res_send( int idx, TB_ROLE role_src, TB_ROLE role_dst )
{
	int ret = 0;
	TBUS size = 0;
	TBUC info[128];

	if( role_src >= ROLE_TERM1 && role_src <= ROLE_TERM3 )
	{
		size = tb_wisun_util_ipaddr_res_make_packet( idx, info );
		if( role_dst == ROLE_GRPGW )
		{
			TB_packet_set_send_direction( idx, WISUN_DIR_TERM2GGW );
			TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2GGW, info, size );
		}
	}
	else if( role_src >= ROLE_RELAY1 && role_src <= ROLE_RELAY3 )
	{
		size = tb_wisun_util_ipaddr_res_make_packet( idx, info );
		if( role_dst == ROLE_TERM )
		{
			TB_packet_set_send_direction( idx, WISUN_DIR_RELAY2TERM );
			TB_packet_send( idx, MSG_CMD_WISUN_SEND_RELAY2TERM, info, size );
		}
	}
	else if( role_src == ROLE_REPEATER )
	{
		size = tb_wisun_util_ipaddr_res_make_packet( idx, info );
		if( role_dst == ROLE_GRPGW )
		{
			TB_packet_set_send_direction( idx, WISUN_DIR_TERM2GGW );
			TB_packet_send( idx, MSG_CMD_WISUN_SEND_TERM2GGW, info, size );
		}
	}
	else
	{
		TB_dbg_wisun("Unknown resipaddr : %d\r\n", role_src );
		ret = -1;
	}

	return ret;
}

int TB_wisun_util_ipaddr_connt_device_save( TBUC *ip_info )
{
	int ret = -1;

	if( ip_info )
	{
		int offset = 0;
		int dev_idx = 0;
		TBUC name[16];
		TBUC role = 0;
		TBUS serial = 0;

		role    = ip_info[offset++];
		serial |= (ip_info[offset++] << 0);
		serial |= (ip_info[offset++] << 8);
		switch( role )
		{
			case ROLE_RELAY1   :	dev_idx = 0;	wstrncpy( name, sizeof(name), "RELAY1", wstrlen("RELAY1") );		break;
			case ROLE_RELAY2   :	dev_idx = 1;	wstrncpy( name, sizeof(name), "RELAY2", wstrlen("RELAY2") );		break;
			case ROLE_RELAY3   :	dev_idx = 2;	wstrncpy( name, sizeof(name), "RELAY3", wstrlen("RELAY3") );		break;
			
			case ROLE_TERM1	   :	dev_idx = 0;	wstrncpy( name, sizeof(name), "TERMINAL1", wstrlen("TERMINAL1") );	break;
			case ROLE_TERM2	   :	dev_idx = 1;	wstrncpy( name, sizeof(name), "TERMINAL2", wstrlen("TERMINAL2") );	break;
			case ROLE_TERM3	   :	dev_idx = 2;	wstrncpy( name, sizeof(name), "TERMINAL3", wstrlen("TERMINAL3") );	break;

			case ROLE_REPEATER :	g_check_ip = 1;
									TB_dbg_wisun( "Received a Repeater IP info\r\n" );
									return -1;
									
			default			   :	TB_dbg_wisun( "Invalid device index : %d\r\n", ip_info[0] );
									return -1;
		}

		TB_dbg_wisun( "*************************************************************\r\n" );
		TB_dbg_wisun( "%s's IP[%d] : ", name, dev_idx );
		for( int i=0; i<16; i++ )
		{
			g_ip_adr_temp[0][dev_idx][i] = ip_info[offset++];
			TB_prt_wisun( "%02X ", g_ip_adr_temp[0][dev_idx][i] );
		}
		g_serial_lower[dev_idx] = serial;
		
		TB_prt_wisun( "\r\n" );
		TB_dbg_wisun( "*************************************************************\r\n" );
		ret = 0;

		g_check_ip = 1;
	}

	return ret;
}

int TB_wisun_util_ipaddr_connt_device_set( TBBL is_term )
{
	int i, j;
	TB_ROLE role = TB_get_role();

	extern TBUS g_connection_cnt[2];
	extern TBUC g_connection_max[2];
	extern TBUC	s_packet_dest_ip[2][16];

	if( is_term )
	{
		TB_dbg_wisun("SEND[TREM]. REQ IPADDR : g_connection_max = %d, g_connection_cnt = %d\r\n", g_connection_max[0], g_connection_cnt[0] );
		TB_dbg_wisun("SEND[TREM]. REQ IPADDR : g_connection_max = %d, g_connection_cnt = %d\r\n", g_connection_max[0], g_connection_cnt[0] );
		TB_dbg_wisun("SEND[TREM]. REQ IPADDR : g_connection_max = %d, g_connection_cnt = %d\r\n", g_connection_max[0], g_connection_cnt[0] );

		if( g_connection_cnt[0] > 0 )
		{
			for( i=0; i<16; i++ )	s_packet_dest_ip[0][i] = g_ip_adr[0][0][i];
			TB_wisun_util_ipaddr_req_send( 0, role, ROLE_RELAY1 );
		}
		if( g_connection_cnt[0] > 1 )
		{
			for( i=0; i<16; i++ )	s_packet_dest_ip[0][i] = g_ip_adr[0][1][i];
			TB_wisun_util_ipaddr_req_send( 0, role, ROLE_RELAY2 );
		}
		if( g_connection_cnt[0] > 2 )
		{
			for( i=0; i<16; i++ )	s_packet_dest_ip[0][i] = g_ip_adr[0][2][i];
			TB_wisun_util_ipaddr_req_send( 0, role, ROLE_RELAY3 );
		}
	}
	else	//	is_ggw
	{
		TB_dbg_wisun("SEND[GGW]. REQ IPADDR : g_connection_max = %d, g_connection_cnt = %d\r\n", g_connection_max[0], g_connection_cnt[0] );
		TB_dbg_wisun("SEND[GGW]. REQ IPADDR : g_connection_max = %d, g_connection_cnt = %d\r\n", g_connection_max[0], g_connection_cnt[0] );
		TB_dbg_wisun("SEND[GGW]. REQ IPADDR : g_connection_max = %d, g_connection_cnt = %d\r\n", g_connection_max[0], g_connection_cnt[0] );

		if( g_connection_cnt[0] > 0 )
		{
			for( i=0; i<16; i++ )	s_packet_dest_ip[0][i] = g_ip_adr[0][0][i];
			TB_wisun_util_ipaddr_req_send( 0, role, ROLE_TERM1 );
		}
		if( g_connection_cnt[0] > 1 )
		{
			for( i=0; i<16; i++ )	s_packet_dest_ip[0][i] = g_ip_adr[0][1][i];
			TB_wisun_util_ipaddr_req_send( 0, role, ROLE_TERM2 );
		}
		if( g_connection_cnt[0] > 2 )
		{
			for( i=0; i<16; i++ )	s_packet_dest_ip[0][i] = g_ip_adr[0][2][i];
			TB_wisun_util_ipaddr_req_send( 0, role, ROLE_TERM3 );
		}
		if( g_connection_cnt[0] > 3 )
		{
			for( i=0; i<16; i++ )	s_packet_dest_ip[0][i] = g_ip_adr[0][3][i];
			TB_wisun_util_ipaddr_req_send( 0, role, ROLE_REPEATER );
		}
	}

	for( j=0; j<MAX_CONNECT_CNT; j++ )
	{
		for( i=0; i<16; i++ )
			g_ip_adr[0][j][i] = g_ip_adr_temp[0][j][i];
	}

	TB_dbg_wisun( "\r\n\r\n" );
	TB_dbg_wisun( "************** Connected Device IP info **************\r\n" );
	
	for( j=0; j<MAX_CONNECT_CNT; j++ )
	{
		TB_dbg_wisun( "IP Addr = " );
		for( i=0; i<16; i++ )
			TB_prt_wisun( "%02X ", g_ip_adr[0][j][i] );
		TB_prt_wisun("\r\n");

		TB_dbg_wisun( "Serial  = %d\r\n", g_serial_lower[j] );
	}
	TB_dbg_wisun( "\r\n\r\n" );

	return 0;
}

