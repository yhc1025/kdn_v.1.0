#ifndef __TB_J11_H__
#define __TB_J11_H__

#include <stdio.h>
#include <string.h>
#include "TB_setup.h"
#include "TB_type.h"

#define LEN_CMD_HDR  	((TBUC)8)
#define LEN_UNIQ_CODE 	((TBUC)4)
#define LEN_MAC_ADDR 	((TBUC)8)
#define LEN_IP_ADDR 	((TBUC)16)

////////////////////////////////////////////////////////////////////////////////

#define CMD_PORTOPEN 		(0x0005)
#define CMD_UDPSEND  		(0x0008)
#define CMD_GETIP			(0x0009)
#define CMD_HAN      		(0x000A)
#define CMD_GETMAC			(0x000E)
#define CMD_RESET    		(0x00D9)
#define CMD_CON_SET  		(0x0025)
#define CMD_PANA_SET 		(0x002C)
#define CMD_PANA     		(0x003A)
#define CMD_SCAN     		(0x0051)
#define CMD_INI      		(0x005F)
#define CMD_PING      		(0x00D1)

////////////////////////////////////////////////////////////////////////////////

#define RES_PORTOPEN 		(0x2005)
#define RES_UDPSEND  		(0x2008)
#define RES_GETIP			(0x2009)
#define RES_GETMAC			(0x200E)
#define RES_HAN      		(0x200A)
#define RES_PANA_SET 		(0x202C)
#define RES_CON_SET  		(0x2025)
#define RES_PANA     		(0x203A)
#define RES_SCAN     		(0x2051)
#define RES_INI      		(0x205F)
#define RES_PING			(0x20D1)
#define NORT_SCAN    		(0x4051)

#define NORT_RCV_DAT 		(0x6018)
#define NORT_WAKE    		(0x6019)
#define NORT_CON_STAT_CHANG (0x601A)
#define NORT_HAN_ACCEPT 	(0x6023)
#define NORT_PANA    		(0x6028)
#define NORT_RCV_DAT_ERR	(0x6038)
#define NORT_PING			(0x60D1)

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	WISUN_DIR_MIN			= 0,
		
	WISUN_DIR_RELAY2TERM,
	WISUN_DIR_RELAY2GGW,
	WISUN_DIR_TERM2GGW,
	
	WISUN_DIR_TERM2RELAY1,
	WISUN_DIR_TERM2RELAY2,
	WISUN_DIR_TERM2RELAY3,
	
	WISUN_DIR_GGW2TERM1,
	WISUN_DIR_GGW2TERM2,
	WISUN_DIR_GGW2TERM3,
	
	WISUN_DIR_GGW2REPEATER,
	WISUN_DIR_REPEATER2GGW,
	
	WISUN_DIR_MAX,
} TB_WISUN_DIR;

typedef enum
{
	WISUN_STEP_INIT_DONE 	= -1,
	WISUN_STEP_RESET 		= 0,
	WISUN_STEP_GETIP,
	WISUN_STEP_GETMAC,
	WISUN_STEP_INIT_STATE,
	WISUN_STEP_HAN_PANA_SET,
	WISUN_STEP_HAN_ACT,
	WISUN_STEP_HAN_PANA_ACT,
	WISUN_STEP_CONNECT_SET,
	WISUN_STEP_PORT_OPEN,
	WISUN_STEP_UDP_SEND,
	WISUN_STEP_RECV_WAIT,
	WISUN_STEP_CONNECTION,

	WISUN_STEP_MAX,
} TB_STATE;

////////////////////////////////////////////////////////////////////////////////

#define MAX_CONNECT_CNT		3

extern TBUC g_uniq_req[4];
extern TBUC g_uniq_res[4];
extern TBUC g_wisun_init_info[2][4];
extern TBUC g_pan_id[2];
extern TBUC g_pair_id[8];
extern TBUC g_mac_adr    [2][MAX_CONNECT_CNT+1][LEN_MAC_ADDR];
extern TBUC g_ip_adr	 [2][MAX_CONNECT_CNT+1][LEN_IP_ADDR];
extern TBUC g_ip_adr_temp[2][MAX_CONNECT_CNT+1][LEN_IP_ADDR];
extern TBUS g_serial_lower[MAX_CONNECT_CNT+1];
extern TBUS g_serial_upper;
extern TBUC g_ip_adr_my  [2][LEN_IP_ADDR];
extern TBUC g_udp_port_src[2];
extern TBUC g_udp_port_dst[2];

////////////////////////////////////////////////////////////////////////////////

extern TBBL TB_j11_init( void );
extern int  TB_j11_connect( int idx );
extern TBBL TB_j11_wait_msg( int idx, TBUS flag );
extern TBBL TB_j11_cmd_send( int idx, TBUS cmd );

extern int  TB_j11_cmd_create( TBUS cmd , TBUS msg_length , TBUS hdr_chksum , TBUS dat_chksum, TBUC *p_data, TBUC * p_send_data );
extern TBBL TB_j11_check_receive_header( TBUC *p_header );

extern int  TB_j11_cmd_udpsend( int idx, TBUC *p_data, TBUI length );
extern int  TB_j11_send_recv_data_ack( int idx, TBUC *dest_ip, TBUC *ack_data, TBUS ack_data_len );
extern TBBL TB_j11_check_connect_relay( int idx, TB_ROLE role );
extern void TB_j11_print_connect_endd( int idx );
extern TBUS TB_j11_get_connection_count( int idx );
extern TBUS TB_j11_get_connection_max( int idx );

extern TBBL TB_j11_init_complete( int idx );

extern int TB_j11_pancip_save( int idx );
extern int TB_j11_pancip_load( int idx );

extern TBUS TB_j11_get_serial_number_device_upper( void );
extern TBUS TB_j11_get_serial_number_device_lower( int idx );

////////////////////////////////////////////////////////////////////////////////

extern void TB_wisun_test_proc_start( void );
extern void TB_wisun_test_proc_stop( void );

#endif    //__TB_J11_H__

