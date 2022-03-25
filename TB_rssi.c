#include <stdio.h>

#include "TB_debug.h"
#include "TB_elapse.h"
#include "TB_led.h"
#include "TB_j11.h"
#include "TB_j11_util.h"
#include "TB_rssi.h"

static struct comm_info	s_comm_info[2][MAX_CONNECT_CNT];

int TB_rssi_init_comm_info( void )
{
	bzero( s_comm_info, sizeof(s_comm_info) );
	return 0;
}

void TB_rssi_increment_send_count( int idx_wisun, TBUC *p_dest_mac )
{
	int i, j;
	int device_idx = -1;

	if( idx_wisun < 2 && p_dest_mac != NULL )
	{
		/***********************************************************************
		*	입력된 IP주소를 저장된 IP주소에서 검색을 한다.
		***********************************************************************/
		for( i=0; i<MAX_CONNECT_CNT; i++ )
		{
			if( memcmp( &g_ip_adr[idx_wisun][i][8], &p_dest_mac[0], LEN_MAC_ADDR ) == 0 )
			{
				device_idx = i;

				s_comm_info[idx_wisun][device_idx].cnt_send ++;
				TB_dbg_rssi( "send info.[%d][%d] send=%d, recv=%d\r\n", 	\
							idx_wisun, i,									\
							s_comm_info[idx_wisun][i].cnt_send,				\
							s_comm_info[idx_wisun][i].cnt_recv );
				break;
			}
		}
	}
}

void TB_rssi_increment_recv_count( int idx_wisun, TBUC *p_sour_mac )
{
	int i, j;
	int device_idx = -1;

	if( idx_wisun < 2 && p_sour_mac != NULL )
	{
		/***********************************************************************
		*	입력된 IP주소를 저장된 IP주소에서 검색을 한다.
		***********************************************************************/
		for( i=0; i<MAX_CONNECT_CNT; i++ )
		{
			if( memcmp( &g_ip_adr[idx_wisun][i][8], &p_sour_mac[0], LEN_MAC_ADDR ) == 0 )
			{
				device_idx = i;

				s_comm_info[idx_wisun][i].cnt_recv ++;
				TB_dbg_rssi( "recv info.[%d][%d] send=%d, recv=%d\r\n", 	\
							idx_wisun, i,									\
							s_comm_info[idx_wisun][i].cnt_send,				\
							s_comm_info[idx_wisun][i].cnt_recv );
				break;
			}
		}
	}
}

TBUC TB_rssi_set_comm_info( int idx_wisun, TBSC rssi, TBUC *p_sour_mac )
{
	int i, j;
	int device_idx = 0;

	TBUC percent = rssi + 104;	//	(-104 to -34 dBM) ====> (0 to 70 dBM)
	percent = (TBUC)(((TBFLT)percent / (TBFLT)70) * 100);

	if( idx_wisun < 2 && p_sour_mac != NULL && percent <= 100 )
	{
		/***********************************************************************
		*	입력된 IP주소를 저장된 IP주소에서 검색을 한다.
		***********************************************************************/
		for( i=0; i<MAX_CONNECT_CNT; i++ )
		{
			if( memcmp( &g_ip_adr[idx_wisun][i][LEN_MAC_ADDR], &p_sour_mac[0], LEN_MAC_ADDR ) == 0 )
			{
				device_idx = i;			
				break;
			}
		}

		/***********************************************************************
		*	Elapse Time이 초기화되지 않았으면 초기화한다.
		***********************************************************************/		
		if( s_comm_info[idx_wisun][device_idx].elapse.t.tv_sec == 0 )
		{
			TB_elapse_set_init( &s_comm_info[idx_wisun][device_idx].elapse );
			TB_elapse_set_start( &s_comm_info[idx_wisun][device_idx].elapse );
		}

		/***********************************************************************
		*	전파 감도를 저장하고, 저장 시간을 기록한다.
		***********************************************************************/
		TBUI val1, val2;
		if( s_comm_info[idx_wisun][i].cnt_recv >= (TBFLT)s_comm_info[idx_wisun][i].cnt_send )
		{
			val1 = s_comm_info[idx_wisun][i].cnt_send;
			val2 = s_comm_info[idx_wisun][i].cnt_recv;
		}
		else
		{
			val1 = s_comm_info[idx_wisun][i].cnt_recv;
			val2 = s_comm_info[idx_wisun][i].cnt_send;
		}

		if( val1 != 0 &&  val2 != 0 )
		{
			s_comm_info[idx_wisun][device_idx].rate		= (TBFLT)(((TBFLT)val1 / (TBFLT)val2) * 100.0);
			s_comm_info[idx_wisun][device_idx].dbm		= rssi;
			s_comm_info[idx_wisun][device_idx].percent 	= percent;
		}
		else
		{
			s_comm_info[idx_wisun][device_idx].rate		= 0;
			s_comm_info[idx_wisun][device_idx].dbm		= 0;
			s_comm_info[idx_wisun][device_idx].percent 	= 0;
		}
		TB_elapse_set_reload( &s_comm_info[idx_wisun][device_idx].elapse );

		/***********************************************************************
		*	전파 감도를 저장한 시간과 현재의 시간을 비교하여 차이가
		*	DISCONNECTION_TIME보다 크면 연결이 끊어졌다고 판단한다.
		***********************************************************************/
		const int DISCONNECTION_TIME = (3*60);	//	3분
		for( j=0; j<2; j++ )
		{
			for( i=0; i<MAX_CONNECT_CNT; i++ )
			{
				if( s_comm_info[idx_wisun][i].elapse.t.tv_sec != 0 )
				{
					TBUL diff = TB_elapse_get_elapse_time( &s_comm_info[idx_wisun][i].elapse );
					diff /= 1000;	//	msec to sec
					if( diff > DISCONNECTION_TIME )
					{
						bzero( &s_comm_info[idx_wisun][i], sizeof(struct comm_info) );
						TB_dbg_rssi( "WiSUN Disconnect --> [%d][%d]\r\n", j, i );
					}
					//TB_dbg_rssi( "[%d][%d] diff time --> [%d]\r\n", j, i, diff );
				}
				else
				{
					s_comm_info[idx_wisun][i].rate 		= 0;
					s_comm_info[idx_wisun][i].dbm 		= 0;
					s_comm_info[idx_wisun][i].percent 	= 0;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////

		for( j=0; j<2; j++ )
		{
			for( i=0; i<MAX_CONNECT_CNT; i++ )
			{
				if( s_comm_info[j][i].percent != 0 )
				{
					TB_dbg_rssi( "WISUN=%d, DEVICE=%d, PERCENT = %d%%, RSSI = %ddbm, RATE=%3.1f \r\n",	\
								j, i, s_comm_info[j][i].percent, s_comm_info[j][i].dbm, s_comm_info[j][i].rate );
					TB_led_dig_set_wisun_state( s_comm_info[j][i].percent );
				}
			}
		}
	}
	else
	{
		TB_dbg_rssi( "ERROR. RSSI Info wisun idx=%d, p_sour_mac=0x%X, percent=%d, rssi=%d\r\n", idx_wisun, p_sour_mac, percent, rssi );
	}
	
	return percent;
}

TBSI TB_rssi_get_comm_info_dbm( int idx_wisun, int idx_endd )
{
	return s_comm_info[idx_wisun][idx_endd].dbm;
}

TBUC TB_rssi_get_comm_info_percent( int idx_wisun, int idx_endd )
{
	return s_comm_info[idx_wisun][idx_endd].percent;
}

TBFLT TB_rssi_get_comm_info_rate( int idx_wisun, int idx_endd )
{
	return s_comm_info[idx_wisun][idx_endd].rate;
}

struct comm_info *TB_rssi_get_comm_info( int idx_wisun, int idx_endd )
{
	return &s_comm_info[idx_wisun][idx_endd];
}

TBUS TB_rssi_get_connection_count( int wisun_idx )
{
	TBUS count = 0;
	for( int i=0; i<MAX_CONNECT_CNT; i++ )
	{
		struct comm_info *p_comm_info = TB_rssi_get_comm_info( wisun_idx, i );
		if( p_comm_info )
		{
			if( p_comm_info->percent != 0 )
			{
				count++;
			}
		}
	}

	return count;
}

