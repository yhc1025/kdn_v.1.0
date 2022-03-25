#include <pthread.h>

#include "TB_sys_wdt.h"
#include "TB_sys_gpio.h"
#include "TB_util.h"
#include "TB_elapse.h"
#include "TB_uart.h"
#include "TB_wisun.h"
#include "TB_packet.h"
#include "TB_setup.h"
#include "TB_debug.h"
#include "TB_log.h"
#include "TB_net_queue.h"
#include "TB_j11_util.h"
#include "TB_wisun_util.h"
#include "TB_ui.h"
#include "TB_led.h"
#include "TB_rssi.h"
#include "TB_msgbox.h"
#include "TB_test.h"
#include "TB_j11.h"

////////////////////////////////////////////////////////////////////////////////

#define ENABLE_REENQ

////////////////////////////////////////////////////////////////////////////////

int g_nort_con_stat_chang = 0;

////////////////////////////////////////////////////////////////////////////////

#define PANC_IP_FILE_PATH		"/mnt/pancip.cfg"
TBUC g_panc_mac[LEN_MAC_ADDR];

////////////////////////////////////////////////////////////////////////////////
//	- MAX Wi-SUN Packet size 	: 	1232 byte
//	- Wi-SUN Header size 		:	  26 byte (header 22 + Uniq Code 4)
//	- Real MAX data size		:	1206 byte
////////////////////////////////////////////////////////////////////////////////

TBUC g_uniq_req[LEN_UNIQ_CODE]	= {0xD0, 0xEA, 0x83, 0xFC};
TBUC g_uniq_res[LEN_UNIQ_CODE]	= {0xD0, 0xF9, 0xEE, 0x5D};

//	PAN Coordinator 0x01 / HAN Sleep Disable / 922.9MHz / 20mW 출력
//	Coordinator     0x02 / HAN Sleep Disable / 922.9MHz / 20mW 출력
//	End Device      0x03 / HAN Sleep Disable / 922.9MHz / 20mW 출력
TBUC g_wisun_init_info[2][4]	= {0x01, 0x00, 0x05, 0x00};
TBUC g_pan_id[2]				= {0x56, 0x78};
TBUC g_pair_id[8]				= {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

TBUC g_mac_adr    [2][MAX_CONNECT_CNT+1][8];
TBUC g_ip_adr	  [2][MAX_CONNECT_CNT+1][16];
TBUC g_ip_adr_temp[2][MAX_CONNECT_CNT+1][16];

/*******************************************************************************
*	하위 통신장치의 제품번호를 저장한다.( PANC <--- ENDD )
*******************************************************************************/
TBUS g_serial_lower[MAX_CONNECT_CNT+1];
/*******************************************************************************
*	상위 통신장치의 제품번호를 저장한다.( PANC ---> ENDD )
*******************************************************************************/
TBUS g_serial_upper;

TBUC g_ip_adr_common[8] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
TBUC g_ip_adr_my[2][16];

TBUS g_connection_cnt[2]  = {0, 0}; 	//	connection count
TBUC g_connection_max[2]  = {0, 0}; 	//	max connection count
TBUC g_connection_mode[2] = {0, 0}; 	//	1: initial connection mode, 2: normal connection mode

TBUC g_udp_port_src[2]	= {0x0E, 0x1A};
TBUC g_udp_port_dst[2]	= {0x0E, 0x1A};
TBUC g_pana_pw[16]		= { '1', '1', '1', '1',
							'2', '2', '2', '2',
							'3', '3', '3', '3',
							'4', '4', '4', '4' };

////////////////////////////////////////////////////////////////////////////////

static TB_STATE s_wisun_state[2]	   = {WISUN_STEP_RESET,WISUN_STEP_RESET};
static TBBL		s_j11_init_complete[2] = {FALSE, FALSE};

////////////////////////////////////////////////////////////////////////////////

TBUS TB_j11_get_serial_number_device_upper( void )
{
	return g_serial_upper;
}

TBUS TB_j11_get_serial_number_device_lower( int idx )
{
	TBUS serial = 0xFFFF;
	int cnt = sizeof(g_serial_lower) / sizeof(g_serial_lower[0]);
	if( cnt >= idx )
		serial = g_serial_lower[idx];

	return serial;
}

static void tb_j11_print_wisun_connect_info( int idx )
{
	TB_WISUN_MODE mode = TB_wisun_get_mode( idx );
	TB_ROLE role = TB_get_role();

	printf( "\n\n\r\n" );
	printf( "**************************************************\r\n" );
	printf( "               START Wi-SUN Process               \r\n" );
	printf( "                 IDX  = %d \r\n", idx	);
	printf( "                 ROLE = %s \r\n", role == ROLE_GRPGW 		? "GROUP G/W"	:	\
											   role == ROLE_RELAY1		? "RELAY1" 		:	\
											   role == ROLE_RELAY2		? "RELAY2" 		:	\
											   role == ROLE_RELAY3		? "RELAY3" 		:	\
											   role == ROLE_TERM1		? "TERMNAL1" 	:	\
											   role == ROLE_TERM2		? "TERMNAL2" 	:	\
											   role == ROLE_TERM3		? "TERMNAL3" 	: 	\
											   role == ROLE_REPEATER	? "REPEATER" 	: "UNKNOWN" );
	printf( "                 MODE = %s \r\n", mode == WISUN_MODE_ENDD 	? "ENDD" 		:	\
											   mode == WISUN_MODE_PANC	? "PANC" 		:	\
											   mode == WISUN_MODE_COORD ? "COOR" 		: "UNKNOWN" );
	printf( "**************************************************\r\n" );
	printf( "\n\n\r\n" );
}

#define MAX_CONECTION_TRY_SEC		(30*1000)	//	30sec
//#define MAX_CONECTION_TRY_SEC		(60*1000)	//	60sec
//#define MAX_CONECTION_TRY_SEC		(180*1000)	//	180sec

static int s_check_connect_set = 0;
static TB_ELAPSE s_elapse_wisun_connect;
static int tb_j11_init_panc( int idx )
{
	TBBL rc = FALSE;

	/***************************************************************************
	*	Initial connection mode timer : 180 sec.
	*		3분 이내에 PANC에 ENDD가 모두 접속이 완료되지 않으면
	*		다시 Initial connection 모드로 전환한다.
	***************************************************************************/
	if( TB_elapse_check_expire(&s_elapse_wisun_connect, MAX_CONECTION_TRY_SEC, 1) )
	{
		s_wisun_state[idx] = WISUN_STEP_CONNECT_SET;
		s_check_connect_set = 0;
	}
	
	switch( s_wisun_state[idx] )
	{
		case(WISUN_STEP_RESET):
			rc = TB_j11_cmd_send( idx, CMD_RESET );
			rc = TB_j11_wait_msg( idx, NORT_WAKE );
			if( rc == TRUE )
				s_wisun_state[idx] = WISUN_STEP_GETIP;
			break;

		case WISUN_STEP_GETIP :
			rc = TB_j11_cmd_send( idx, CMD_GETIP );
			rc = TB_j11_wait_msg( idx, RES_GETIP );
			if( rc == TRUE )
				s_wisun_state[idx] = WISUN_STEP_GETMAC;
			break;

		case WISUN_STEP_GETMAC :
			rc = TB_j11_cmd_send( idx, CMD_GETMAC );
			rc = TB_j11_wait_msg( idx, RES_GETMAC );
			if( rc == TRUE )
				s_wisun_state[idx] = WISUN_STEP_INIT_STATE;
			break;

		case(WISUN_STEP_INIT_STATE):
			rc = TB_j11_cmd_send( idx, CMD_INI );
			rc = TB_j11_wait_msg( idx, RES_INI );
			if( rc == TRUE )
				s_wisun_state[idx] = WISUN_STEP_HAN_ACT;
			break;

		case(WISUN_STEP_HAN_ACT):
			TB_elapse_set_init( &s_elapse_wisun_connect );
			
			rc = TB_j11_cmd_send( idx, CMD_HAN );								//	000A
			rc = TB_j11_wait_msg( idx, RES_HAN );								//	200A
			if( rc == TRUE )
			{
				s_wisun_state[idx] = WISUN_STEP_PORT_OPEN;
				TB_elapse_set_start( &s_elapse_wisun_connect );
			}
			break;

		case(WISUN_STEP_PORT_OPEN):
			rc = TB_j11_cmd_send( idx, CMD_PORTOPEN );							//	0005
			rc = TB_j11_wait_msg( idx, RES_PORTOPEN );							//	2005
			if( rc == TRUE )
			{
				s_wisun_state[idx] = WISUN_STEP_CONNECT_SET;
			}
			break;

		case(WISUN_STEP_CONNECT_SET):
			if( s_check_connect_set == 0 )
			{
				rc = TB_j11_cmd_send( idx, CMD_CON_SET );						//	0025
				TB_dbg_j11( "TRY. RES_CON_SET\r\n" );
				rc = TB_j11_wait_msg( idx, RES_CON_SET );						//	2025
				if( rc == TRUE )
				{
					s_check_connect_set = 1;
					TB_dbg_j11( "OK. RES_CON_SET\r\n" );
					s_wisun_state[idx] = WISUN_STEP_CONNECTION;
				}
				else
					break;
			}
			break;

		case WISUN_STEP_CONNECTION :
			TB_j11_connect( idx );
			break;

		case (WISUN_STEP_INIT_DONE):
			s_j11_init_complete[idx] = TRUE;
			
			TB_util_delay( 5000 );
			tb_j11_print_wisun_connect_info( idx );
			return 1;

		default:  // error 
			TB_dbg_j11( "Unknown state = %d\r\n", s_wisun_state[idx] );
			break;
	}

	return 0;
}

static int tb_j11_init_endd( int idx )
{
	TBBL rc = FALSE;
	switch( s_wisun_state[idx] )
	{
		case(WISUN_STEP_RESET):
			rc = TB_j11_cmd_send( idx, CMD_RESET );
			rc = TB_j11_wait_msg( idx, NORT_WAKE );
			if( rc == TRUE )
				s_wisun_state[idx] = WISUN_STEP_GETIP;
			break;

		case WISUN_STEP_GETIP :
			rc = TB_j11_cmd_send( idx, CMD_GETIP );
			rc = TB_j11_wait_msg( idx, RES_GETIP );
			if( rc == TRUE )
				s_wisun_state[idx] = WISUN_STEP_GETMAC;
			break;

		case WISUN_STEP_GETMAC :
			rc = TB_j11_cmd_send( idx, CMD_GETMAC );
			rc = TB_j11_wait_msg( idx, RES_GETMAC );
			if( rc == TRUE )
				s_wisun_state[idx] = WISUN_STEP_INIT_STATE;
			break;
			
		case(WISUN_STEP_INIT_STATE):			
			rc = TB_j11_cmd_send( idx, CMD_INI );
			rc = TB_j11_wait_msg( idx, RES_INI );
			if( rc == TRUE )
			{
				TB_j11_pancip_load( idx );
				s_wisun_state[idx] = WISUN_STEP_HAN_PANA_SET;
			}
			break;

		case(WISUN_STEP_HAN_PANA_SET):
			rc = TB_j11_cmd_send( idx, CMD_PANA_SET );							//	002C
			rc = TB_j11_wait_msg( idx, RES_PANA_SET );							//	202C
			if( rc == TRUE )
			{
				s_wisun_state[idx] = WISUN_STEP_HAN_ACT;
			}
			break;

		////////////////////////////////////////////////////////////////////////
			
		case(WISUN_STEP_HAN_ACT):
			{
				static int s_retry_cnt = 0;

				rc = TB_j11_cmd_send( idx, CMD_HAN );							//	000A
				s_retry_cnt ++;

				TB_util_delay( 500 );
				rc = TB_j11_wait_msg( idx, RES_HAN );							//	200A
				if( rc == TRUE )
				{
					TB_util_delay( 2000 );
					s_wisun_state[idx] = WISUN_STEP_HAN_PANA_ACT;
					s_retry_cnt = 0;
				}
#ifdef ENABLE_AUTO_CONNECT
				else
				{
					if( ++s_retry_cnt >= 3 )
					{
						bzero( g_panc_mac, sizeof(g_panc_mac) );
						TB_j11_pancip_save( idx );
					}

					TB_util_delay( 2000 );
					TB_dbg_j11( "WISUN_STEP_HAN_ACT : retry count = %d\r\n", s_retry_cnt );
				}
#else
				else
				{
					TB_util_delay( 2000 );
				}
#endif
			}
			break;

		case(WISUN_STEP_HAN_PANA_ACT):
			{
				static int s_check = 0 ;
				if( s_check == 0 )
				{
					static int s_check1 = 0 ;
					if( s_check1 == 0 )
					{
						rc = TB_j11_cmd_send( idx, CMD_PANA );					//	003A
						s_check1 = 1;
					}
					
					TB_dbg_j11( "TRY. RES_PANA\r\n" );
					rc = TB_j11_wait_msg( idx, RES_PANA );		 				//	203A
					if( rc == TRUE )
					{
						s_check = 1;
						TB_dbg_j11( "OK. RES_PANA\r\n" );
					}
					else
						break;
				}

				////////////////////////////////////////////////////////////////

				if( s_check == 1 )
				{
					TB_dbg_j11( "TRY. NORT_PANA\r\n" );
					rc = TB_j11_wait_msg( idx, NORT_PANA );		 				//	6028
					if( rc == TRUE )
					{
						TB_dbg_j11( "OK. NORT_PANA PANA\r\n" );
						s_wisun_state[idx] = WISUN_STEP_PORT_OPEN;
					}
				}
			}
			break;		

		case(WISUN_STEP_PORT_OPEN):
			rc = TB_j11_cmd_send( idx, CMD_PORTOPEN );							//	0005
			rc = TB_j11_wait_msg( idx, RES_PORTOPEN );							//	2005
			if( rc == TRUE )
			{
				s_wisun_state[idx] = WISUN_STEP_INIT_DONE;
			}
			break;
			
		case (WISUN_STEP_INIT_DONE):
			s_j11_init_complete[idx] = TRUE;			
			tb_j11_print_wisun_connect_info( idx );

#ifdef ENABLE_AUTO_CONNECT
			int i;
			for( i=0; i<8; i++ )
				g_panc_mac[i] = g_ip_adr[idx][0][8+i];
			TB_j11_pancip_save( idx );
#endif
			return 1;

		default:  // error 
			TB_dbg_j11( "Unknown state = %d\r\n", s_wisun_state[idx] );
			break;
	}

	return 0;
}

#define	QUEUE_SIZE		4096
#define ENQ_BUF_SIZE	512

int s_read_loop_flag_1st = 1;
int s_read_loop_flag_2nd = 1;

static void tb_wisun_read_proc_stop_1st( void )
{
	s_read_loop_flag_1st = 0;
}

static void tb_wisun_read_proc_stop_2nd( void )
{
	s_read_loop_flag_2nd = 0;
}

static void *tb_wisun_read_proc_1st( void *arg )
{
	TBUC read_buf[ENQ_BUF_SIZE] = {0, };
	TBUS read_size = 0;
	int idx = 0;

	if( arg )
	{
		idx = *(int*)arg;
	}

	TBUI q_size = QUEUE_SIZE / 2;

	TB_WISUN_MODE wisun_mode = TB_setup_get_wisun_info_mode( idx );
	if( wisun_mode == WISUN_MODE_PANC )
		q_size = QUEUE_SIZE;

	g_wisun_netq_1st.init( idx, &g_wisun_netq_1st.netq, q_size );

	while( s_read_loop_flag_1st )
	{
		//TB_dbg_j11("TRY. wisun read\r\n" );
		read_size = g_op_uart_wisun.read( read_buf, sizeof(read_buf) );
		if( read_size > 0 )
		{
			//TB_dbg_j11("wisun read # %d :%d\r\n", idx, read_size );
			g_wisun_netq_1st.enq( &g_wisun_netq_1st.netq, read_buf, read_size );
		}

		TB_util_delay( 100 );
	}

	g_wisun_netq_1st.deinit( &g_wisun_netq_1st.netq );
}

static void *tb_wisun_read_proc_2nd( void *arg )
{
	TBUC read_buf[ENQ_BUF_SIZE] = {0, };
	TBUS read_size = 0;
	int idx = 1;

	if( arg )
	{
		idx = *(int*)arg;
	}

	TBUI q_size = QUEUE_SIZE / 2;

	TB_WISUN_MODE wisun_mode = TB_setup_get_wisun_info_mode( idx );
	if( wisun_mode == WISUN_MODE_PANC )
		q_size = QUEUE_SIZE;

	g_wisun_netq_2nd.init( idx, &g_wisun_netq_2nd.netq, q_size );
	while( s_read_loop_flag_2nd )
	{
		read_size = g_op_uart_lte.read( read_buf, sizeof(read_buf) );
		if( read_size > 0 )
		{
			TB_dbg_j11("wisun read # %d :%d\n", idx, read_size );
			g_wisun_netq_2nd.enq( &g_wisun_netq_2nd.netq, read_buf, read_size );
		}

		TB_util_delay( 100 );
	}

	g_wisun_netq_1st.deinit( &g_wisun_netq_2nd.netq );
}

static pthread_t s_thid_wisun_read_1st;
static pthread_t s_thid_wisun_read_2nd;

int tb_j11_init( TBUC idx )
{
	int wisun_init_done = 0;
	
	int 		gpio 	= (idx==0) ? GPIO_TYPE_WISUN_RESET1 : GPIO_TYPE_WISUN_RESET2;
	pthread_t	*p_thid = (idx==0) ? &s_thid_wisun_read_1st : &s_thid_wisun_read_2nd;
	void *	(*wisun_thread_loop)(void *) = (idx==0) ? tb_wisun_read_proc_1st : tb_wisun_read_proc_2nd;
	
	TBBL rc = FALSE;
	TBUL delay_msec;

	TB_WISUN_MODE 	wisun_mode;
	TB_SETUP_WISUN *p_wisun = TB_setup_get_wisun_info( idx );

	if( p_wisun->enable == FALSE )
		return -1;

	////////////////////////////////////////////////////////////////////////////
	//							Wi-SUN H/W Reset
	////////////////////////////////////////////////////////////////////////////
	TB_gpio_set( gpio, 0 );	TB_util_delay( 500 );
	TB_gpio_set( gpio, 1 );	TB_util_delay( 500 );

	////////////////////////////////////////////////////////////////////////////

	switch( p_wisun->mode )
	{
		case WISUN_MODE_PANC	:	g_wisun_init_info[idx][0] = 0x01;	break;
		case WISUN_MODE_COORD	:	g_wisun_init_info[idx][0] = 0x02;	break;
		case WISUN_MODE_ENDD	:	g_wisun_init_info[idx][0] = 0x03;	break;
		default					:	g_wisun_init_info[idx][0] = 0x03;	break;
	}
	g_wisun_init_info[idx][2] = p_wisun->frequency + WISUN_FREQ_ADJUST;
	TB_wisun_set_mode( idx, p_wisun->mode );
	TB_set_role( TB_setup_get_role() );
	
	////////////////////////////////////////////////////////////////////////////

	g_connection_max[idx] = p_wisun->max_connect;
	for( int i=0; i<g_connection_max[idx]+1; i++ )
	{
		bzero( &g_ip_adr[idx][i][0], 16 );
		wmemcpy( &g_ip_adr[idx][i][0], 16, g_ip_adr_common, sizeof(g_ip_adr_common) );
		
		bzero( &g_ip_adr_temp[idx][i][0], 16 );
		wmemcpy( &g_ip_adr_temp[idx][i][0], 16, g_ip_adr_common, sizeof(g_ip_adr_common) );
	}

	////////////////////////////////////////////////////////////////////////////

	int wisun_idx = idx;
	pthread_create( p_thid, NULL, wisun_thread_loop, (void*)&wisun_idx );
	
	////////////////////////////////////////////////////////////////////////////

	wisun_mode = TB_wisun_get_mode( idx );
	delay_msec = (wisun_mode == WISUN_MODE_PANC) ? 500 : 1000;

	time_t endd_init_t = time( NULL );	

	TB_ui_key_init( TRUE );
	while( wisun_init_done == 0 )
	{
		if( TB_dm_is_on( DMID_J11 ) )
			TB_j11_util_debug_state( idx, s_wisun_state[idx] );
		
		TB_util_delay( delay_msec );
		switch( wisun_mode )
		{
			case WISUN_MODE_PANC 	:	wisun_init_done = tb_j11_init_panc( idx );	break;
			case WISUN_MODE_COORD	:
			case WISUN_MODE_ENDD	:	
			default					:	wisun_init_done = tb_j11_init_endd( idx );	break;
		}

		/***********************************************************************
		*	ENDD/COORD가 5min 이내에 PANC에 접속이 완료되지 않으면 리부팅한다.
		*	PANC가 ENDD/COORD보다 먼저 부팅이 되지 않아서 접속이 되지 않는다고 판단.
		*	PANC가 먼저 부팅이 된 다음에 ENDD/COORD가 접속할 수 있도록 한다.
		************************************************************************/		
		if( wisun_mode != WISUN_MODE_PANC )
		{
			if( wisun_init_done == 0 )
			{
				time_t t = time( NULL );
				if( endd_init_t + _5MIN_SEC < t )
				{
					TB_wdt_reboot( WDT_COND_WISUN_CONNECTION_FAIL );
				}
			}
		}
		
		int input = TB_ui_getch( FALSE );
		if( input != -1 )
		{
			if( input != KEY_MOUSE )		//	Any Key input
			{
				TB_setup_enter( ACCOUNT_TYPE_USER, TRUE );
			}
		}
	}

	return 0;
}

TBBL TB_j11_init( void )
{
	TB_SETUP_WISUN *p_wisun 	= NULL;
	TB_ROLE 		role 		= TB_setup_get_role();
	TB_COMM_TYPE 	comm_type 	= TB_setup_get_comm_type();

	TB_VOLTAGE 	vol = TB_setup_get_product_info_voltage(); 
 	
	if( vol == VOLTAGE_LOW )
	{
		p_wisun = TB_setup_get_wisun_info( WISUN_SECOND );
		if( p_wisun )
		{
			if( p_wisun->enable )
			{
				if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
				{
					if( p_wisun->mode == WISUN_MODE_ENDD )
					{
						if( comm_type.master == COMM_MODE_MASTER_WISUN )
						{
							printf( "========================================\r\n" );
							printf( "  Start J11 Init [%d] : ROLE TERM : ENDD \r\n", WISUN_SECOND );
							printf( "========================================\r\n" );
							if( tb_j11_init( WISUN_SECOND ) == 0 )
							{
								TB_log_append( LOG_COMM_GGW2TERM_SUCCESS, NULL, -1 );
								TB_wisun_proc_start( WISUN_SECOND );
							}
							else
							{
								TB_log_append( LOG_COMM_GGW2TERM_FAILED, NULL, -1 );
							}
						}
					}
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////

	p_wisun = TB_setup_get_wisun_info( WISUN_FIRST );
	if( p_wisun )
	{
		if( p_wisun->enable )
		{
			if( role == ROLE_GRPGW )
			{
				if( p_wisun->mode == WISUN_MODE_PANC )
				{
					printf( "=============================================\r\n" );
					printf( "  Start J11 Init [0] : ROLE GROUP GW/ : PANC \r\n" );
					printf( "=============================================\r\n" );
					if( tb_j11_init( WISUN_FIRST ) == 0 )
					{
						//TB_util_delay( 10000 );
						TB_util_delay( 1000 );
						
						/*******************************************************
						*	PANC에 연결된 ENDD들의 IP address값을 가져온다.
						*******************************************************/
						TB_wisun_util_ipaddr_connt_device_set( FALSE );

						TB_log_append( LOG_COMM_GGW2TERM_SUCCESS, NULL, -1 );
						TB_wisun_proc_start( WISUN_FIRST );
					}
					else
					{
						TB_log_append( LOG_COMM_GGW2TERM_FAILED, NULL, -1 );
					}
				}
			}
			else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
			{
				TB_VOLTAGE vol = TB_setup_get_product_info_voltage();
				if( vol == VOLTAGE_HIGH )
				{
					if( p_wisun->mode == WISUN_MODE_ENDD )
					{
						if( comm_type.master == COMM_MODE_SLAVE_WISUN )
						{
							printf( "=============================================\r\n" );
							printf( "  Start J11 Init [0] : ROLE TERM : ENDD      \r\n" );
							printf( "=============================================\r\n" );
							if( tb_j11_init( WISUN_FIRST ) == 0 )
							{
								TB_log_append( LOG_COMM_GGW2TERM_SUCCESS, NULL, -1 );
								TB_wisun_proc_start( WISUN_FIRST );
							}
							else
							{
								TB_log_append( LOG_COMM_TERM2RELAY_FAILED, NULL, -1 );
							}
						}
					}
				}
				else
				{
					if( p_wisun->mode == WISUN_MODE_PANC )
					{
						if( comm_type.slave == COMM_MODE_SLAVE_WISUN )
						{
							printf( "=============================================\r\n" );
							printf( "  Start J11 Init [0] : ROLE TERM : PANC      \r\n" );
							printf( "=============================================\r\n" );
							if( tb_j11_init( WISUN_FIRST ) == 0 )
							{
								/***************************************************
								*	PANC에 연결된 ENDD들의 IP address값을 가져온다.
								****************************************************/
								TB_wisun_util_ipaddr_connt_device_set( TRUE );

								TB_log_append( LOG_COMM_TERM2RELAY_SUCCESS, NULL, -1 );
								TB_wisun_proc_start( WISUN_FIRST );
							}
							else
							{
								TB_log_append( LOG_COMM_TERM2RELAY_FAILED, NULL, -1 );
							}
						}
					}
				}
			}
			else if( role >= ROLE_RELAY1 && role <= ROLE_RELAY3 )
			{
				printf( "=============================================\r\n" );
				printf( "  Start J11 Init [0] : ROLE RELAY : ENDD     \r\n" );
				printf( "=============================================\r\n" );
				TB_util_delay( 20000 );
				if( tb_j11_init( WISUN_FIRST ) == 0 )
				{
					TB_log_append( LOG_COMM_TERM2RELAY_SUCCESS, NULL, -1 );
					TB_wisun_proc_start( WISUN_FIRST );
				}
			}
			else if( role == ROLE_REPEATER )
			{
				printf( "=============================================\r\n" );
				printf( "  Start J11 Init [0] : ROLE REPEATER : COORD \r\n" );
				printf( "=============================================\r\n" );
				if( tb_j11_init( WISUN_FIRST ) == 0 )
				{
					TB_log_append( LOG_COMM_GGW2REPEATER_SUCCESS, NULL, -1 );
					TB_wisun_proc_start( WISUN_REPEATER );
				}
				else
				{
					TB_log_append( LOG_COMM_GGW2REPEATER_FAILED, NULL, -1 );
				}
			}
		}	
	}

	return TRUE;
}

static void tb_dbg_j11_mesg( TBUC *p_msg )
{
	if( p_msg )
		TB_dbg_j11( "**************** %s ****************\r\n", p_msg );
}

int TB_j11_connect( int idx )
{
	TBBL rc = FALSE;
	
	static int s_check1 = 0;
	static int s_check2 = 0;
	static int s_check3 = 0;
	static int s_check4 = 0;

	TB_dbg_j11( "g_connection_cnt[%d] = %d\r\n", idx, g_connection_cnt[idx] );
	
	if( s_check1 == 0 )
	{
		TB_dbg_j11( "TRY. NORT_CON_STAT_CHANG\r\n" );
		g_nort_con_stat_chang = 1;
		rc = TB_j11_wait_msg( idx, NORT_CON_STAT_CHANG );						//	601A	initial connection mode
		if( rc == TRUE )
		{
			s_check1 = 1;
			TB_dbg_j11( "OK. NORT_CON_STAT_CHANG\r\n" );
		}
	}
	else if( s_check2 == 0 )
	{
		rc = TB_j11_cmd_send( idx, CMD_PANA_SET );								//	002C
		TB_dbg_j11( "TRY. RES_PANA_SET\r\n" );
		rc = TB_j11_wait_msg( idx, RES_PANA_SET );								//	202C
		if( rc == TRUE )
		{
			TB_dbg_j11( "OK. RES_PANA_SET\r\n" );
			s_check2 = 1;
		}
	}
	else if( s_check3 == 0 && g_connection_cnt[idx] == 0 )
	{
		rc = TB_j11_cmd_send( idx, CMD_PANA );									//	003A
		TB_dbg_j11( "TRY. RES_PANA\r\n" );
		rc = TB_j11_wait_msg( idx, RES_PANA );	 								//	203A
		if( rc == TRUE )
		{
			TB_dbg_j11( "OK. RES_PANA\r\n" );
			s_check3 = 1;
		}
	}
	else if( s_check4 == 0 )
	{
		TB_dbg_j11( "TRY. NORT_CON_STAT_CHANG\r\n" );
		g_nort_con_stat_chang = 2;
		rc = TB_j11_wait_msg( idx, NORT_CON_STAT_CHANG );						// 601A
		if( rc == TRUE )
		{
			TB_dbg_j11( "OK. NORT_CON_STAT_CHANG\r\n" );
			s_check4 = 1;

			////////////////////////////////////////////////////////////////////

			if( TB_setup_get_wisun_repeater() )
			{
				if( g_connection_cnt[idx] >= g_connection_max[idx] + 1 )
				{
					s_wisun_state[idx] = WISUN_STEP_INIT_DONE;
				}
			}
			else
			{
				if( g_connection_cnt[idx] >= g_connection_max[idx] )
				{
					s_wisun_state[idx] = WISUN_STEP_INIT_DONE;
				}
			}
			
			s_check1 = 0;
			s_check2 = 0;
			s_check3 = 0;
			s_check4 = 0;
		}
	}

	return 0;
}

TBBL TB_j11_wait_msg( int idx, TBUS cmd_filter )
{
	static TBUC reenq_buf[512] = {0,};
	static TBUS reenq_length = 0;

	static TBUC recv_buf[512] = {0,};
	static TBUS recv_length = 0;
	
	TBUS recv_idx = 0;

	TB_WISUN_QUEUE *p_wisun_netq  = (idx==0) ? &g_wisun_netq_1st : &g_wisun_netq_2nd;
	TB_WISUN_MODE wisun_mode = TB_wisun_get_mode( idx );

	if( p_wisun_netq->deq( &p_wisun_netq->netq, recv_buf, &recv_length ) != 0 )
	{
		//TB_dbg_j11( "Fail. Network Queue : Dequeue\n" );
		return FALSE;
	}

	if( TB_dm_is_on(DMID_J11) )
		TB_util_data_dump( "RECV", recv_buf, recv_length );

	////////////////////////////////////////////////////////////////////////////

	TBUS recv_cmd = (recv_buf[4] << 8) | (recv_buf[5] << 0);

	////////////////////////////////////////////////////////////////////////////
	
	if( cmd_filter != 0 )
	{
		if( cmd_filter != recv_cmd )
		{
			if( wisun_mode == WISUN_MODE_PANC )
			{
#ifdef	ENABLE_REENQ
				if( recv_cmd == NORT_CON_STAT_CHANG )
				{
					TB_dbg_j11("re-enq1 : 0x%04X\n", recv_cmd);
					wmemcpy( &reenq_buf[0], sizeof(reenq_buf), &recv_buf[0], recv_length );
					reenq_length = recv_length;
					p_wisun_netq->enq( &p_wisun_netq->netq, reenq_buf, reenq_length );

				}
				else if( recv_cmd == RES_HAN || recv_cmd == RES_PANA || recv_cmd == RES_PANA_SET )
				{
					if( recv_buf[12] == 0x01 )
					{
						TB_dbg_j11("re-enq2 : 0x%04X\n", recv_cmd);
						wmemcpy( &reenq_buf[0], sizeof(reenq_buf), &recv_buf[0], recv_length );
						reenq_length = recv_length;
						p_wisun_netq->enq( &p_wisun_netq->netq, reenq_buf, reenq_length );
					}
				}
#endif
			}

			return FALSE;
		}
	}

	////////////////////////////////////////////////////////////////////////////

	TB_dbg_j11("-----------> recv_cmd = 0x%04X\r\n", recv_cmd );
	
	switch( recv_cmd )
	{
		case (NORT_WAKE):
			tb_dbg_j11_mesg( "NORT_WAKE");
			return TRUE;

		case (RES_GETIP)	:
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( "GetLocalIP Success");
				if( TB_dm_is_on(DMID_J11) )
					TB_util_data_dump( "RES_GETIP", recv_buf, (TBUS)13+16 );

				for( int i=0; i<16; i++ )
					g_ip_adr_my[idx][i] = recv_buf[13 + i];
			}				
			break;

		case (RES_GETMAC)	:
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( "GetLocalMAC Success");
				if( TB_dm_is_on(DMID_J11) )
					TB_util_data_dump( "RES_GETMAC", recv_buf, (TBUS)13+16 );
			}				
			break;
			
		case (RES_INI)	:
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( "Init Success");
			}
			else
			{
				tb_dbg_j11_mesg( "Init Error");
				return FALSE;
			}
			break;
			
		case (RES_PANA_SET):
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( "PANA Password set Success");
			}
			else
			{
				tb_dbg_j11_mesg( "PANA Password set Error");
				return FALSE;
			}
			break;
			
		case (RES_SCAN):
			tb_dbg_j11_mesg( "RES_SCAN");
			break;
					
		case (NORT_SCAN):
			tb_dbg_j11_mesg( "NORT_SCAN");
			break;

		case (RES_HAN):
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( "HAN Act(RES_HAN) Success" );

				if( wisun_mode != WISUN_MODE_PANC )
				{
					TB_j11_print_recv_data_info( idx, RES_HAN, recv_buf );
					
					for( int i=0; i<LEN_MAC_ADDR;i++)
					{
						g_pair_id[i] 	 		= recv_buf[16 + i];
						g_ip_adr[idx][0][8+i] 	= recv_buf[16 + i];	//	PANC's ip addr
					}

					if( TB_dm_is_on(DMID_J11 ) )
					{
						TB_dbg_j11( "PAIR ID = ");	TB_util_data_dump1( &g_pair_id[0], (TBUS)LEN_MAC_ADDR );
						TB_dbg_j11( "PANC IP = ");	TB_util_data_dump1( &g_ip_adr[idx][0][8], (TBUS)LEN_MAC_ADDR );
					}

					g_ip_adr[idx][0][8] = g_ip_adr[idx][0][8] ^ 0x02;
				}
			}
			else
			{
				tb_dbg_j11_mesg( "HAN Act(RES_HAN) Error");
				return FALSE;
			}
			break;
			
		case (RES_PANA):
			if( recv_buf[12] == 0x01 )
			{
//				TB_j11_print_recv_data_info( idx, RES_PANA, recv_buf );
				tb_dbg_j11_mesg( "PANA Act Success");
			}
			else
			{
				tb_dbg_j11_mesg( "PANA Act Error");
				return FALSE;
			}
			break;
			
		case (NORT_PANA):
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( "PANA Connect Success");
				if( TB_dm_is_on(DMID_J11) )
					TB_util_data_dump2( &recv_buf[13], LEN_MAC_ADDR );
			}
			else
			{
				tb_dbg_j11_mesg( "PANA Connect Error");
				return FALSE;
			}
			break;
			
		case (RES_CON_SET):
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( "Normal connect mode");
			}
			else
			{
				tb_dbg_j11_mesg( "Connect mode change error");
				return FALSE;
			}
			break;

		case (NORT_CON_STAT_CHANG):
			tb_dbg_j11_mesg( "Notify Connection Status Change" );
			TB_dbg_j11( "g_nort_con_stat_chang = 0x%02X : 0x%02X\r\n", g_nort_con_stat_chang, recv_buf[12] );
			switch( recv_buf[12] )
			{
				case 0x01 :	
					if( g_nort_con_stat_chang == 1 )
					{
						TB_dbg_j11( "    NORT_CON_STAT_CHANG : MAC connect Success\r\n");
						TB_dbg_j11( "    NORT_CON_STAT_CHANG : MAC ADDRESS : ");

						int temp = g_connection_cnt[idx];
						TBUC *p_mac = &g_mac_adr[idx][temp][0];
						TBUC *p_ip  = &g_ip_adr [idx][temp][8];

						for( int i=0; i<LEN_MAC_ADDR; i++)
						{								
							p_mac[i] = recv_buf[13+i];
							p_ip [i] = recv_buf[13+i];
						}

						if( TB_dm_is_on(DMID_J11) )
						{
							TB_util_data_dump2( p_mac, LEN_MAC_ADDR );
							TB_util_data_dump2( &g_ip_adr[idx][temp][0], LEN_IP_ADDR );
						}
						p_ip[0] = p_ip[0] ^ 0x02;
					}
					else
					{
#ifdef	ENABLE_REENQ
						TB_dbg_j11("re-enq3 : 0x%04X\n", recv_cmd);
						wmemcpy( &reenq_buf[0], sizeof(reenq_buf), &recv_buf[0], recv_length );
						reenq_length = recv_length;
						p_wisun_netq->enq( &p_wisun_netq->netq, reenq_buf, reenq_length );
#endif
						return FALSE;
					}
					break;
				
				case 0x02 :
					if( g_nort_con_stat_chang == 2 )
					{
						TB_dbg_j11( "    NORT_CON_STAT_CHANG : PANA authentication Success\r\n");
						g_connection_cnt[idx]++;
					}
					else
					{
#ifdef	ENABLE_REENQ
						TB_dbg_j11("re-enq4 : 0x%04X\n", recv_cmd);
						wmemcpy( &reenq_buf[0], sizeof(reenq_buf), &recv_buf[0], recv_length );
						reenq_length = recv_length;
						p_wisun_netq->enq( &p_wisun_netq->netq, reenq_buf, reenq_length );
#endif
						return FALSE;
					}
					break;

				case 0x03 :	TB_dbg_j11( "    NORT_CON_STAT_CHANG : MAC disconnect\r\n");	return FALSE;
				case 0x04 :	TB_dbg_j11( "    NORT_CON_STAT_CHANG : PANA disconnect\r\n");	return FALSE;
				default	  :	TB_dbg_j11( "    NORT_CON_STAT_CHANG : Unknown state\r\n");		return FALSE;
			}

			TB_j11_print_recv_data_info( idx, NORT_CON_STAT_CHANG, recv_buf );
			break;
			
		case (RES_PORTOPEN):
			if( recv_buf[12] == 0x01 )
			{
				tb_dbg_j11_mesg( " UDP port open Success");
			}
			else
			{
				tb_dbg_j11_mesg( " UDP port open Error");
				return FALSE;
			}
			break;
			
		case (RES_UDPSEND):
			if( recv_buf[12] == 0x01 )
			{
				TB_j11_print_recv_data_info( idx, RES_UDPSEND, recv_buf );
				tb_dbg_j11_mesg( "UDP send Success" );
			}
			else 
			{
				tb_dbg_j11_mesg( "UDP send Error");
				printf("-------> RES_UDPSEND : 0x%X\r\n", recv_buf[12] );
				return FALSE;
			}
			break;

		case (NORT_RCV_DAT):
			{
				TB_dbg_j11( "UDP recv Success : total read size = %d\r\n", recv_length );

				if( TB_testmode_is_on() )
				{				
					TBSC rssi = (TBSC)recv_buf[36];
					TBUC percent = rssi + 104;
					percent = (TBUC)(((TBFLT)percent / (TBFLT)70) * 100);
					printf( "[%s:%d] RSSI=%ddbm, Percent=%d%%\r\n", __FILE__, __LINE__, rssi, percent );
				}
			}
			break;

		case (NORT_RCV_DAT_ERR):
//			TB_j11_print_recv_data_info( idx, NORT_RCV_DAT_ERR, recv_buf );
			break;

		case (NORT_HAN_ACCEPT):
			tb_dbg_j11_mesg( "Notify HAN Acceptance Connection Mode Change");
			switch( recv_buf[12] )
			{
				case 0x01 	: 	TB_dbg_j11( "     Connection Mode : Default connection mode\r\n");			break;
				case 0x02 	:	TB_dbg_j11( "     Connection Mode : Normal connection mode\r\n");			break;
				default		:	TB_dbg_j11( "     Connection Mode : Unknown mode : %d\r\n", recv_buf[12] );	break;
			}
			break;

		case (RES_PING) :
			break;

		case (NORT_PING) :
			tb_dbg_j11_mesg( "Notification command parameters (PING)" );
			break;
			
		case (0x2FFF):
			tb_dbg_j11_mesg( "RES : Invalid Header Checksum error");
			return FALSE;
			
		default:
			TB_dbg_j11( "uni code error : 0x%X\r\n", recv_buf[4] << 8 | recv_buf[5] );
			return FALSE;		
	}
		
	return TRUE;
}

TBBL TB_j11_cmd_send( int idx, TBUS cmd )
{
	TBUS hdr_chksum = g_uniq_req[0] + g_uniq_req[1] + g_uniq_req[2] + g_uniq_req[3] ;
	TBUS dat_chksum = 0 ;
	TBUS msg_length = 0 ;
	TBUS dat_length = 0 ;
	TBUC data[128] = {0};
	TBUC send_data[128] = {0} ;
	unsigned int i = 0;
	static unsigned int dev_cnt = 0;
	int temp;

	TB_OP_UART *p_uart = (idx==0) ? &g_op_uart_wisun : &g_op_uart_lte;

	TB_WISUN_MODE wisun_mode = TB_wisun_get_mode( idx );

	switch( cmd )
	{
		case (CMD_RESET):
			dat_length = 0;
			msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
			hdr_chksum += CMD_RESET + msg_length;
			dat_chksum = 0 ;
			
			TB_j11_cmd_create( CMD_RESET, msg_length, hdr_chksum, dat_chksum, data, send_data );
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_RESET", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_INI):
			dat_length = (TBUS)4;
			msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
			hdr_chksum += CMD_INI + msg_length;

			for( i=0; i<dat_length; i++ )
				data[i] = g_wisun_init_info[idx][i];
			
			for( i=0; i<dat_length; i++ )
				dat_chksum += data[i];
			
			TB_j11_cmd_create( CMD_INI , msg_length, hdr_chksum, dat_chksum, data, send_data );
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_INI", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_GETIP) :
			dat_length = 0;
			msg_length = (TBUS)(4 + dat_length);
			hdr_chksum += CMD_GETIP + msg_length;
			dat_chksum = 0;
			
			TB_j11_cmd_create( CMD_GETIP , msg_length, hdr_chksum, dat_chksum, data, send_data );			
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_GETIP", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_GETMAC) :
			dat_length = 0;
			msg_length = (TBUS)(4 + dat_length);
			hdr_chksum += CMD_GETMAC + msg_length;
			dat_chksum = 0;
			
			TB_j11_cmd_create( CMD_GETMAC , msg_length, hdr_chksum, dat_chksum, data, send_data );			
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_GETMAC", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_PANA_SET):
			if( wisun_mode == WISUN_MODE_PANC )
				dat_length = (TBUS)(8+16);
			else
				dat_length = (TBUS)(16) ;
			msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
			hdr_chksum += CMD_PANA_SET + msg_length;

			if( wisun_mode == WISUN_MODE_PANC )
			{
				for( i=0; i<(LEN_MAC_ADDR); i++ )
					data[i] = g_mac_adr[idx][g_connection_cnt[idx]][i];
			
				for(    ; i<dat_length; i++ )
					data[i] = g_pana_pw[ i - (LEN_MAC_ADDR) ];
			}
			else
			{
				for( i=0; i<dat_length; i++ )
					data[i] = g_pana_pw[ i ];
			}
			
			for( i=0; i<dat_length; i++ )
			{
				dat_chksum += data[i];
			}

			TB_j11_cmd_create( CMD_PANA_SET, msg_length, hdr_chksum, dat_chksum, data, send_data );
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_PANA_SET", send_data, msg_length + LEN_CMD_HDR );

			dev_cnt ++;
			break;

		case (CMD_SCAN):
			break;
			
		case (CMD_HAN):
			if( wisun_mode == WISUN_MODE_PANC )
				dat_length = (TBUS)sizeof( g_pan_id );	//	2
			else
				dat_length = (TBUS)sizeof( g_pair_id );	//	8

			msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
			hdr_chksum += CMD_HAN + msg_length;

			if( wisun_mode == WISUN_MODE_PANC )
			{
				for( i=0; i<dat_length; i++ )
					data[i] = g_pan_id[i] ;
			}
			else
			{
#ifdef ENABLE_AUTO_CONNECT
				int check = 1;
				for( i=0; i<8; i++ )
				{
					if( g_panc_mac[i] != 0x00 )
					{
						check = 0;
						TB_dbg_j11( "g_panc_mac ==> [%d] %02X %02X %02X %02X %02X %02X %02X %02X \r\n", idx,	\
									g_panc_mac[0], g_panc_mac[1], g_panc_mac[2], g_panc_mac[3], \
									g_panc_mac[4], g_panc_mac[5], g_panc_mac[6], g_panc_mac[7] );
					}
				}

				if( check == 0 )
				{
					TB_dbg_j11( "ENDD - CMD_HAN : check -> %d : Use PANC Ip address\r\n", check );
					TB_dbg_j11( "       PANC IP = " );
					for( i=0; i<dat_length; i++ )
					{
						data[i] = g_panc_mac[i] ;
						TB_prt_j11( "%02X ", data[i] );
					}
					TB_prt_j11( "\r\n" );
				}
				else
				{
					TB_dbg_j11( "ENDD - CMD_HAN : check -> %d : Use Pair ID\r\n", check );
					TB_dbg_j11( "       PAIR ID = " );
					for( i=0; i<dat_length; i++ )
					{
						data[i] = g_pair_id[i] ;
						TB_prt_j11( "%02X ", data[i] );
					}
					TB_prt_j11( "\r\n" );
				}
#else
				for( i=0; i<dat_length; i++ )
					data[i] = g_pair_id[i] ;
#endif
			}

			for( i=0; i<dat_length; i++ )
			{
				dat_chksum += data[i];
			}
			
			TB_j11_cmd_create( CMD_HAN , msg_length, hdr_chksum, dat_chksum, data, send_data );
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_HAN", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_PANA):
			dat_length = 0;
			msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
			hdr_chksum += CMD_PANA + msg_length;
			dat_chksum = 0;
			
			TB_j11_cmd_create( CMD_PANA , msg_length, hdr_chksum, dat_chksum, data, send_data );
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_PANA", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_CON_SET):
			dat_length = 1;
			msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
			hdr_chksum += CMD_CON_SET + msg_length;

			switch( wisun_mode )
			{
				case WISUN_MODE_PANC 	:	if( g_connection_mode[idx] == 0 )
												g_connection_mode[idx] = 0x01; 	// initial connection mode
											else
												g_connection_mode[idx] = 0x03 - g_connection_mode[idx]; // toggle mode
											data[0] = g_connection_mode[idx];
											break;
				case WISUN_MODE_COORD	:	data[0] = 0x01;
											break;
#if 1	//	When panc + coord + endd
				case WISUN_MODE_ENDD	:	data[0] = 0x02;
											break;
#else	//	When panc + endd + endd +...
				case WISUN_MODE_ENDD	:	data[0] = 0x01;
											break;
#endif
				default					:	data[0] = 0x02;		//	endd
											break;
			}

			TB_dbg_j11( "g_connection_mode = %d\n", g_connection_mode[idx] );
			
			dat_chksum = data[0];
			
			TB_j11_cmd_create( CMD_CON_SET , msg_length, hdr_chksum, dat_chksum, data, send_data );
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
			if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_CON_SET", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_PORTOPEN):
			dat_length = 2;
			msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
			hdr_chksum += CMD_PORTOPEN + msg_length;

			for( i=0; i<dat_length; i++ )
			{
				switch( wisun_mode )
				{
					case WISUN_MODE_PANC	:	data[i] = g_udp_port_src[i];
												break;
					case WISUN_MODE_COORD	:	
					case WISUN_MODE_ENDD	:
					default					:	data[i] = g_udp_port_dst[i];
												break;
				}
			}
			
			for( i=0; i<dat_length; i++ )
			{
				dat_chksum += data[i];
			}
			
			TB_j11_cmd_create( CMD_PORTOPEN , msg_length, hdr_chksum, dat_chksum, data, send_data );
			p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
            if( TB_dm_is_on(DMID_J11) )
				TB_util_data_dump( "CMD_PORTOPEN", send_data, msg_length + LEN_CMD_HDR );
			break;

		case (CMD_PING):	
			break;

		default:
			break;
	}

	return TRUE;
}

int TB_j11_cmd_udpsend( int idx, TBUC *p_data, TBUI size )
{
	int ret = -1;

	if( p_data && size > 0 )
	{
		int i=0;

		TB_OP_UART *p_uart = (idx==0) ? &g_op_uart_wisun : &g_op_uart_lte;
		
		TBUS	hdr_chksum = g_uniq_req[0] + g_uniq_req[1] + g_uniq_req[2] + g_uniq_req[3] ;
		TBUS 	dat_chksum = 0 ;
		TBUS 	msg_length = 0 ;
		TBUS 	dat_length = 0 ;
		TBUS 	send_data_size = 0 ;
		TBUS	temp;

		static TBUC data[512] 	   = {0};
		static TBUC send_data[512] = {0} ;

		send_data_size = size;
		dat_length = (16 + 6) + send_data_size ;
		msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);

		////////////////////////////////////////////////////////////////////////
		
		temp = msg_length / 256;
		if( temp < 1 )
			hdr_chksum += CMD_UDPSEND + msg_length;
		else
			hdr_chksum += CMD_UDPSEND + (msg_length % 256) + temp;

		////////////////////////////////////////////////////////////////////////
		
		TB_dbg_j11( "[CMD_UDPSEND] dat_length=%d, msg_length=%d, hdr_chksum=0x%04X\r\n", dat_length, msg_length, hdr_chksum );

		/*
		*	16byte
		*/
		wmemcpy( &data[0], sizeof(data), TB_packet_get_dest_ip(idx), LEN_IP_ADDR );

#ifdef COMM_RATE_TEST
		if( TB_wisun_init_complete(idx) == TRUE )
		{
			TB_rssi_increment_send_count( idx, &data[i+LEN_MAC_ADDR] );
		}
#endif

		/*
		*	6byte
		*/
		data[16] = g_udp_port_src[0] ;
		data[17] = g_udp_port_src[1] ;
		data[18] = g_udp_port_dst[0] ;
		data[19] = g_udp_port_dst[1] ;
		data[20] = (unsigned char)((send_data_size >> 8) & 0xFF);
		data[21] = (unsigned char)((send_data_size >> 0) & 0xFF);

		wmemcpy( &data[22], sizeof(data)-22, &p_data[0], send_data_size );;

		for( i=0; i<dat_length; i++ )
		{
			dat_chksum += data[i];
		}

		TB_j11_cmd_create( CMD_UDPSEND, msg_length, hdr_chksum, dat_chksum, data, send_data );
		p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );
		if( TB_dm_is_on(DMID_J11) )
			TB_util_data_dump( "CMD_UDPSEND", send_data, msg_length + LEN_CMD_HDR );

		ret = 0;
	}

	return ret;

}

int TB_j11_send_recv_data_ack( int wisun_idx, TBUC *p_dest_ip, TBUC *p_ack_data, TBUS ack_data_len )
{
	int ret = -1;

	if( wisun_idx < 2 && p_dest_ip && p_ack_data )
	{
		TB_OP_UART *p_uart = (wisun_idx==0) ? &g_op_uart_wisun : &g_op_uart_lte;
		
		TBUS	hdr_chksum = g_uniq_req[0] + g_uniq_req[1] + g_uniq_req[2] + g_uniq_req[3];
		TBUS 	dat_chksum = 0 ;
		TBUS 	msg_length = 0 ;
		TBUS 	dat_length = 0 ;
		TBUS 	send_data_size = 0 ;

		TBUC 	ack_data[512] 	   = {0};
		TBUC 	send_ack_data[512] = {0} ;

		send_data_size = ack_data_len;
		dat_length = (16 + 6) + send_data_size ;
		msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);

		////////////////////////////////////////////////////////////////////////
		
		hdr_chksum += CMD_UDPSEND + msg_length;

		////////////////////////////////////////////////////////////////////////
		
		/*
		*	16byte
		*/
		wmemcpy( &ack_data[0], sizeof(ack_data), p_dest_ip, LEN_IP_ADDR );

		/*
		*	6byte
		*/
		ack_data[16] = g_udp_port_src[0] ;
		ack_data[17] = g_udp_port_src[1] ;
		ack_data[18] = g_udp_port_dst[0] ;
		ack_data[19] = g_udp_port_dst[1] ;
		ack_data[20] = (unsigned char)((ack_data_len >> 8) & 0xFF);
		ack_data[21] = (unsigned char)((ack_data_len >> 0) & 0xFF);

		wmemcpy( &ack_data[22], sizeof(ack_data)-22, p_ack_data, ack_data_len );

		for( int i=0; i<dat_length; i++ )
		{
			dat_chksum += ack_data[i];
		}

		TB_j11_cmd_create( CMD_UDPSEND, msg_length, hdr_chksum, dat_chksum, ack_data, send_ack_data );
		p_uart->write( send_ack_data, (msg_length + LEN_CMD_HDR) );
		if( TB_dm_is_on(DMID_J11) )
			TB_util_data_dump( "CMD_UDPSEND", send_ack_data, msg_length + LEN_CMD_HDR );

		ret = 0;
	}

	return ret;
}

/*******************************************************************************
*   Name     : TB_j11_cmd_create
*   Function : create Request command format
*   input    : cmd 			- Request command
*              msg_length 	- message data recv_length
*              hdr_chksum 	- header checksum
               dat_chksum 	- data checksum
               *pdada     	- wireless data
               *p_send_data	- request command format data
*   return   : -
*******************************************************************************/
int TB_j11_cmd_create( TBUS cmd,	TBUS msg_length, TBUS hdr_chksum, TBUS dat_chksum, TBUC *p_data, TBUC *p_send_data )
{
	int ret = -1;
	
	if( p_data && p_send_data )
	{
		TBUS i = 0 ;

		for( i=0; i<LEN_UNIQ_CODE; i++ )
			p_send_data[i] = g_uniq_req[i];
		
		p_send_data[ 4] = (TBUC)((cmd & 0xFF00) >> 8);
		p_send_data[ 5] = (TBUC)((cmd & 0x00FF));
		
		p_send_data[ 6] = (TBUC)((msg_length & 0xFF00) >> 8);
		p_send_data[ 7] = (TBUC)((msg_length & 0x00FF));
		
		p_send_data[ 8] = (TBUC)((hdr_chksum & 0xFF00) >> 8);
		p_send_data[ 9] = (TBUC)((hdr_chksum & 0x00FF));
		
		p_send_data[10] = (TBUC)((dat_chksum & 0xFF00) >> 8);
		p_send_data[11] = (TBUC)((dat_chksum & 0x00FF));

		if( msg_length > 4 )
		{
			for( i=0; i<(msg_length-4); i++ )
				p_send_data[12 + i] = p_data[i];
		}

		ret = 0;
	}

	return ret;
}

TBBL TB_j11_check_receive_header( TBUC *p_header )
{
	TBBL ret = FALSE;

	if( p_header )
	{
		if( p_header[0] == g_uniq_res[0] &&	p_header[1] == g_uniq_res[1] &&	\
			p_header[2] == g_uniq_res[2] &&	p_header[3] == g_uniq_res[3])
		{
			ret = TRUE;
		}
		else
		{
			//TB_dbg_j11( "Invalid Receive Header\r\n" );
		}
	}

	return ret;
}

TBBL TB_j11_check_connect_relay( int idx, TB_ROLE role )
{
	TBBL ret = FALSE;
	
	if( role >= ROLE_RELAY1 && role <= ROLE_RELAY3 )
	{
		//	ROLE_RELAY1(4) : g_ip_adr[0][...]
		//	ROLE_RELAY2(5) : g_ip_adr[1][...]
		//	ROLE_RELAY3(6) : g_ip_adr[2][...]
		role -= 4;

		TB_dbg_j11( "IP. idx=%d, role=%d --> %02X %02X %02X %02X %02X %02X %02X %02X\r\n",	idx, role, \
					g_ip_adr[idx][role][8],  g_ip_adr[idx][role][9],  g_ip_adr[idx][role][10], g_ip_adr[idx][role][11],	\
					g_ip_adr[idx][role][12], g_ip_adr[idx][role][13], g_ip_adr[idx][role][14], g_ip_adr[idx][role][15] );
		
		if( g_ip_adr[idx][role][8]  == 0 && g_ip_adr[idx][role][9]  == 0 && g_ip_adr[idx][role][10] == 0 && g_ip_adr[idx][role][11] == 0 &&
			g_ip_adr[idx][role][12] == 0 && g_ip_adr[idx][role][13] == 0 && g_ip_adr[idx][role][14] == 0 && g_ip_adr[idx][role][15] == 0 )
		{
			ret = FALSE;
		}
		else
		{
			ret = TRUE;
		}
	}

	return ret;
}

void TB_j11_print_connect_endd( int idx )
{
	int i;
	for( i=0; i<MAX_CONNECT_CNT; i++ )
	{
		if( TB_dm_is_on(DMID_J11) )
			TB_util_data_dump1( &g_ip_adr[idx][i][8], (TBUS)LEN_MAC_ADDR );
	}
}

TBBL TB_j11_init_complete( int idx )
{
	return s_j11_init_complete[idx];
}

TBUS TB_j11_get_connection_count( int idx )
{
	return g_connection_cnt[idx];
}

TBUS TB_j11_get_connection_max( int idx )
{
	return g_connection_max[idx];
}

////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_AUTO_CONNECT
int TB_j11_pancip_save( int idx )
{
	int ret = -1;
	FILE* fp = fopen( PANC_IP_FILE_PATH, "w" );
	if( fp )
	{
		int i;

		g_panc_mac[0] = 0x00;
		for( i=0; i<8; i++ )
		{
			TB_dbg_j11( "FILE SAVE : g_panc_mac ==> %02X %02X %02X %02X %02X %02X %02X %02X\r\n",	\
						g_panc_mac[0], g_panc_mac[1], g_panc_mac[2], g_panc_mac[3], \
						g_panc_mac[4], g_panc_mac[5], g_panc_mac[6], g_panc_mac[7] );
		}		
		
		if( fwrite( &g_panc_mac, 1, sizeof(g_panc_mac), fp ) == sizeof(g_panc_mac) )
		{
			TB_prt_j11("[%s:%d] SUCCESS. SAVE PANCIP_FILE\r\n", __FILE__, __LINE__ );
			ret = 0;
		}
		fclose( fp );
	}
	
	return ret;
}

int TB_j11_pancip_load( int idx )
{
	int ret = -1;
	FILE* fp = NULL;

	bzero( &g_panc_mac, sizeof(g_panc_mac) );
	
	fp = fopen( PANC_IP_FILE_PATH, "r" );
	if( fp )
	{
		TBUC panc_mac_temp[8] = {0, };
		if( fread( panc_mac_temp, 1, sizeof(panc_mac_temp), fp ) == sizeof(panc_mac_temp) )
		{
			int i;
			for( i=0; i<8; i++ )
				g_panc_mac[i] = panc_mac_temp[i];
			g_panc_mac[0] = 0x00;

			for( i=0; i<8; i++ )
			{
				TB_dbg_j11( "FILE LOAD : g_panc_mac ==> %02X %02X %02X %02X %02X %02X %02X %02X\r\n",	\
							g_panc_mac[0], g_panc_mac[1], g_panc_mac[2], g_panc_mac[3], \
							g_panc_mac[4], g_panc_mac[5], g_panc_mac[6], g_panc_mac[7] );
			}
			
			ret = 0;
		}
			
		fclose(fp);
	}

	return ret;
}
#else
int TB_j11_pancip_save( int idx )
{
	return 0;
}

int TB_j11_pancip_load( int idx )
{
	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
#include "TB_test.h"
int 		g_proc_wisun_test_flag = 1;
pthread_t 	g_thid_wisun_test;

void *tb_wisun_test_proc( void *arg )
{
	int wisun_idx = 0;
	TBBL rc = FALSE;

	TB_WISUN_MODE wisun_mode;

#undef ENABLE_AUTO_CONNECT

	TB_wisun_set_mode( wisun_idx, WISUN_MODE_ENDD );

	////////////////////////////////////////////////////////////////////////////
	//							Wi-SUN H/W Reset
	////////////////////////////////////////////////////////////////////////////
	int gpio = (wisun_idx==0) ? GPIO_TYPE_WISUN_RESET1 : GPIO_TYPE_WISUN_RESET2;
	TB_gpio_set( gpio, 0 );	TB_util_delay( 500 );
	TB_gpio_set( gpio, 1 );	TB_util_delay( 500 );

	////////////////////////////////////////////////////////////////////////////

	g_wisun_init_info[wisun_idx][0] = 0x03;	//	End Device
	g_wisun_init_info[wisun_idx][2] = 6;	//	922.9Mhz

	////////////////////////////////////////////////////////////////////////////

	pthread_create( &s_thid_wisun_read_1st, NULL, tb_wisun_read_proc_1st, NULL );
	TB_util_delay( 200 );

	////////////////////////////////////////////////////////////////////////////

	while( g_proc_wisun_test_flag )
	{
		TBBL rc = FALSE;
		switch( s_wisun_state[wisun_idx] )
		{
			case(WISUN_STEP_RESET):
				rc = TB_j11_cmd_send( wisun_idx, CMD_RESET );				
				rc = TB_j11_wait_msg( wisun_idx, NORT_WAKE );
				if( rc == TRUE )					
					s_wisun_state[wisun_idx] = WISUN_STEP_INIT_STATE;
				break;

			case WISUN_STEP_GETIP :
				rc = TB_j11_cmd_send( wisun_idx, CMD_GETIP );
				rc = TB_j11_wait_msg( wisun_idx, RES_GETIP );
				if( rc == TRUE )
					s_wisun_state[wisun_idx] = WISUN_STEP_GETMAC;
				break;

			case WISUN_STEP_GETMAC :
				rc = TB_j11_cmd_send( wisun_idx, CMD_GETMAC );
				rc = TB_j11_wait_msg( wisun_idx, RES_GETMAC );
				if( rc == TRUE )
					s_wisun_state[wisun_idx] = WISUN_STEP_INIT_STATE;
				break;
				
			case(WISUN_STEP_INIT_STATE):			
				rc = TB_j11_cmd_send( wisun_idx, CMD_INI );
				rc = TB_j11_wait_msg( wisun_idx, RES_INI );
				if( rc == TRUE )
				{
					TB_j11_pancip_load( wisun_idx );
					s_wisun_state[wisun_idx] = WISUN_STEP_HAN_PANA_SET;
				}
				break;

			case(WISUN_STEP_HAN_PANA_SET):
				rc = TB_j11_cmd_send( wisun_idx, CMD_PANA_SET );				//	002C
				rc = TB_j11_wait_msg( wisun_idx, RES_PANA_SET );				//	202C
				if( rc == TRUE )
				{
					s_wisun_state[wisun_idx] = WISUN_STEP_HAN_ACT;
				}
				break;

			////////////////////////////////////////////////////////////////////
				
			case(WISUN_STEP_HAN_ACT):
				{
					rc = TB_j11_cmd_send( wisun_idx, CMD_HAN );					//	000A
					rc = TB_j11_wait_msg( wisun_idx, RES_HAN );					//	200A
					if( rc == TRUE )
					{
						TB_util_delay( 2000 );
						s_wisun_state[wisun_idx] = WISUN_STEP_HAN_PANA_ACT;
					}
				}
				break;

			case(WISUN_STEP_HAN_PANA_ACT):
				{
					static int s_check = 0 ;
					if( s_check == 0 )
					{
						static int s_check1 = 0 ;
						if( s_check1 == 0 )
						{
							rc = TB_j11_cmd_send( wisun_idx, CMD_PANA );		//	003A
							s_check1 = 1;
						}
						
						TB_dbg_j11( "TRY. RES_PANA\r\n" );
						rc = TB_j11_wait_msg( wisun_idx, RES_PANA );		 	//	203A
						if( rc == TRUE )
						{
							s_check = 1;
							TB_dbg_j11( "OK. RES_PANA\r\n" );
						}
						else
							break;
					}

					////////////////////////////////////////////////////////////

					if( s_check == 1 )
					{
						TB_dbg_j11( "TRY. NORT_PANA\r\n" );
						rc = TB_j11_wait_msg( wisun_idx, NORT_PANA );		 	//	6028
						if( rc == TRUE )
						{
							TB_dbg_j11( "OK. NORT_PANA PANA\r\n" );
							s_wisun_state[wisun_idx] = WISUN_STEP_PORT_OPEN;
						}
					}
				}
				break;		

			case(WISUN_STEP_PORT_OPEN):
				rc = TB_j11_cmd_send( wisun_idx, CMD_PORTOPEN );				//	0005
				rc = TB_j11_wait_msg( wisun_idx, RES_PORTOPEN );				//	2005
				if( rc == TRUE )
				{
					//s_wisun_state[wisun_idx] = WISUN_STEP_UDP_SEND;
					s_wisun_state[wisun_idx] = WISUN_STEP_RECV_WAIT;

					TB_led_wisun_work( wisun_idx );
				}
				break;
			case (WISUN_STEP_UDP_SEND):
				{
					TBUC *p_wisun_test_data = "1234567890";
					if( p_wisun_test_data )
					{
						TBUI s = wstrlen( p_wisun_test_data );
						if( s > 0 )
						{
							TB_j11_cmd_udpsend( wisun_idx, p_wisun_test_data, s );
							rc = TB_j11_wait_msg( wisun_idx, RES_UDPSEND );
							if( rc == TRUE )
							{
								static int s_count_send = 0;
								if( s_count_send++ > 10 )
								{
									TB_testmode_set_test_result( TEST_ITEM_WISUN, TRUE );
								}

								printf( "[%s:%d]WISUN DATA SEND : %d\r\n", __FILE__, __LINE__, s_count_send );
							}
						}
					}
				}
				break;

			case(WISUN_STEP_RECV_WAIT):
				rc = TB_j11_wait_msg( wisun_idx, 0 );
				if( rc == TRUE )
				{					
					static int s_count_recv = 0;
					if( s_count_recv++ > 10 )
					{
						TB_testmode_set_test_result( TEST_ITEM_WISUN, TRUE );
					}

					printf( "[%s:%d]WISUN DATA RECEIVED : %d\r\n", __FILE__, __LINE__, s_count_recv );
				}
				break;

			case (WISUN_STEP_INIT_DONE):
				s_j11_init_complete[wisun_idx] = TRUE;			
				tb_j11_print_wisun_connect_info( wisun_idx );
				break;

			default:  // error 
				TB_dbg_j11( "Unknown state = %d\r\n", s_wisun_state[wisun_idx] );
				break;
		}

		TB_util_delay( 500 );
	}
}

void TB_wisun_test_proc_start( void )
{
	pthread_create( &g_thid_wisun_test, NULL, tb_wisun_test_proc, NULL );
}

void TB_wisun_test_proc_stop( void )
{
	g_proc_wisun_test_flag = 0;
	pthread_join( g_thid_wisun_test, NULL );
}

