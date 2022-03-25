#include "TB_j11.h"
#include "TB_wisun.h"
#include "TB_packet.h"
#include "TB_elapse.h"
#include "TB_uart.h"
#include "TB_util.h"
#include "TB_debug.h"
#include "TB_ping.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_PING_RETRY_CNT		3
#define MAX_PING_WAIT_DELAY		30	//sec

////////////////////////////////////////////////////////////////////////////////

static TB_ELAPSE 	s_elapse_ping[2];
static TBUS			s_dest_flag[2];
static time_t		s_ping_send_time[2];
static TBUS			s_nort_ping_fail_cnt[2];

////////////////////////////////////////////////////////////////////////////////

int TB_ping_init( int idx )
{
	TB_elapse_set_init( &s_elapse_ping[idx] );
	TB_elapse_set_start( &s_elapse_ping[idx] );

	bzero( s_dest_flag, sizeof(s_dest_flag) );

	s_ping_send_time[0] = 0;
	s_ping_send_time[1] = 0;

	s_nort_ping_fail_cnt[0] = 0;
	s_nort_ping_fail_cnt[1] = 0;

	return 0;
}

int TB_ping_send( int wisun_idx, TBBL flag_immedi )
{
	TB_WISUN_MODE mode = TB_wisun_get_mode( wisun_idx );

	if( flag_immedi == FALSE )
	{
		TBUL diff = TB_elapse_get_elapse_time( &s_elapse_ping[wisun_idx] );
		if( diff < 60000 )	//60sec
			return -1;
	}

	/***************************************************************************
	*	ping은 ENDD/COORD에서 PANC 로만 날린다.
	***************************************************************************/	
	if( mode == WISUN_MODE_PANC )
	{
		return -1;
	}
	
	/***************************************************************************
	*	s_ping_send_time 변수가 0 이 아님은 이전에 CMD_PING을 실행하였으나
	*	아직 NORT_PING을 수신하지 못했다는 의미임.
	***************************************************************************/
	if( s_ping_send_time[wisun_idx] != 0 )
	{
		return -1;
	}
	
	////////////////////////////////////////////////////////////////////////////

	int  i, j;
	TBUS dest_flag = 0;	
	int cnt = TB_j11_get_connection_max( wisun_idx );
	if( cnt > 0 )
	{
		/*
		*	실제 연결된 WiSUN Device를 찾는다.
		*/
		for( i=0; i<cnt; i++ )
		{
			for( j=0; j<8; j++ )
			{
				if( g_ip_adr[wisun_idx][i][8+j] != 0x00 )
				{
					dest_flag |= (0x01 << i);
					break;
				}
			}
		}

		TB_dbg_ping("[%d] PING : Dest Flag = 0x%02X\r\n", wisun_idx, dest_flag );

		if( dest_flag & 0x01 )
		{
			if( mode == WISUN_MODE_PANC )
			{
				if( TB_get_role_is_grpgw() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_GGW2TERM1\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_GGW2TERM1 );
					TB_ping_send_data( wisun_idx );
				}
				else //if( TB_get_role_is_terminal() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_TERM2RELAY1\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_TERM2RELAY1 );
					TB_ping_send_data( wisun_idx );
				}
			}
			else
			{
				if( TB_get_role_is_terminal() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_TERM2GGW\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_TERM2GGW );
					TB_ping_send_data( wisun_idx );
				}
				else //if( TB_get_role_is_relay() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_RELAY2TERM\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_RELAY2TERM );
					TB_ping_send_data( wisun_idx );
				}
			}
		}

		if( dest_flag & 0x02 )
		{
			if( mode == WISUN_MODE_PANC )
			{
				if( TB_get_role_is_grpgw() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_GGW2TERM2\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_GGW2TERM2 );
					TB_ping_send_data( wisun_idx );
				}
				else //if( TB_get_role_is_terminal() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_TERM2GGW\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_TERM2GGW );
					TB_ping_send_data( wisun_idx );
				}
			}
		}
		
		if( dest_flag & 0x04 )
		{
			if( mode == WISUN_MODE_PANC )
			{
				if( TB_get_role_is_grpgw() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_GGW2TERM3\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_GGW2TERM3 );
					TB_ping_send_data( wisun_idx );
				}
				else //if( TB_get_role_is_terminal() )
				{
					TB_dbg_ping("PING TO WISUN_DIR_TERM2GGW\r\n");
					TB_packet_set_send_direction( wisun_idx, WISUN_DIR_TERM2GGW );
					TB_ping_send_data( wisun_idx );
				}
			}
		}
	}

	TB_elapse_set_reload( &s_elapse_ping[wisun_idx] );

	return 0;
}

int TB_ping_send_data( int wisun_idx )
{
	TBUS hdr_chksum = g_uniq_req[0] + g_uniq_req[1] + g_uniq_req[2] + g_uniq_req[3] ;
	TBUS dat_chksum = 0 ;
	TBUS msg_length = 0 ;
	TBUS dat_length = 0 ;
	TBUC data[128] = {0, };
	TBUC send_data[128] = {0, } ;

	TB_OP_UART *p_uart = (wisun_idx==0) ? &g_op_uart_wisun : &g_op_uart_lte;
	
	TBUI offset = 0;
	TBUC *destip = TB_packet_get_dest_ip( wisun_idx );
	TBUS ping_data_size = 4;
	
	dat_length = 16 + 2 + 1;
	msg_length = (TBUS)(sizeof(g_uniq_req) + dat_length);
	hdr_chksum += CMD_PING + msg_length;

	/*
	*	16byte
	*/
	wmemcpy( &data[offset], sizeof(data)-offset, &destip[offset], LEN_IP_ADDR );		offset += LEN_IP_ADDR;

	/*
	*	2byte
	*/
	data[offset] = (TBUC)((ping_data_size & 0xFF00) >> 8);		offset++;
	data[offset] = (TBUC)((ping_data_size & 0x00FF) >> 0);		offset++;

	/*
	*	1byte
	*		0x00: Arbitrary data transmission
	*		0x01: Transmission in fixed data pattern 1
	*		0x02: Transmission in fixed data pattern 2
	*/
	data[offset] = 0x01;										offset++;

	for( offset=0; offset<dat_length; offset++ )
	{
		dat_chksum += data[offset];
	}

	TB_j11_cmd_create( CMD_PING , msg_length, hdr_chksum, dat_chksum, data, send_data );
	p_uart->write( send_data, (msg_length + LEN_CMD_HDR) );

	if( TB_dm_is_on(DMID_PING) )
		TB_util_data_dump( "CMD_PING", send_data, msg_length + LEN_CMD_HDR );

	s_ping_send_time[wisun_idx] = time( NULL );

	return 0;
}

/*******************************************************************************
*	NORT_PING이 수신되었다면 PANC가 정상인 상태로 판단하고, CMD_PING을 다시
*	보낼 수 있도록 변수를 초기화한다.
********************************************************************************/
int TB_ping_set_notify_success( int idx )
{
	s_ping_send_time[idx] = 0;	
}

/*******************************************************************************
*	CMD_PING을 보냈던 시간과 현재의 시간을 비교하여 차이가 MAX_PING_WAIT_DELAY sec
*	보다 크면 PANC가 리부팅되었다고 판단한다.
********************************************************************************/
int TB_ping_check_notify_success( int idx )
{
	int ret = 0;
	
	if( s_ping_send_time[idx] != 0 )
	{
		time_t t = time( NULL );
		if( t > s_ping_send_time[idx] )
		{
			if( (t - s_ping_send_time[idx]) > MAX_PING_WAIT_DELAY )
			{
				if( s_nort_ping_fail_cnt[idx] >= MAX_PING_RETRY_CNT )
				{
					TB_dbg_ping( "ping notify fail (%d : %d)........... reboot\r\n", s_nort_ping_fail_cnt[idx], MAX_PING_RETRY_CNT );

					ret = -1;
				}
				else
				{
					s_nort_ping_fail_cnt[idx] ++;
					TB_dbg_ping( "ping notify fail count = %d\r\n", s_nort_ping_fail_cnt[idx] );
					TB_dbg_ping( "ping retry\r\n" );

					s_ping_send_time[idx] = 0;
					TB_ping_send( idx, TRUE );
				}
			}
		}

		TB_dbg_ping( "[%d] check ping notify --> [%d:%d]  %d\r\n", idx, t, s_ping_send_time[idx], (t - s_ping_send_time[idx]) );
	}
	else
	{
		s_nort_ping_fail_cnt[idx] = 0;
	}

	return ret;
}

