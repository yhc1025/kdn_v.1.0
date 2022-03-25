#ifndef	__TB_RSSI_H__
#define __TB_RSSI_H__

#include "TB_type.h"
#include "TB_elapse.h"

////////////////////////////////////////////////////////////////////////////////

#define COMM_RATE_TEST

////////////////////////////////////////////////////////////////////////////////

struct comm_info
{
	TBUI		cnt_send;
	TBUI		cnt_recv;
	
	TBFLT 		rate;
	TBSI		dbm;
	TBUC		percent;
	TB_ELAPSE	elapse;
};

extern int  TB_rssi_init_comm_info( void );
extern TBUC TB_rssi_set_comm_info( int idx_wisun, TBSC rssi, TBUC *src_mac );
extern TBSI TB_rssi_get_comm_info_dbm( int idx_wisun, int idx_endd );
extern TBUC TB_rssi_get_comm_info_percent( int idx_wisun, int idx_endd );
extern TBFLT TB_rssi_get_comm_info_rate( int idx_wisun, int idx_endd );
extern struct comm_info *TB_rssi_get_comm_info( int idx_wisun, int idx_endd );
extern TBUS TB_rssi_get_connection_count( int wisun_idx );

extern void TB_rssi_increment_send_count( int idx_wisun, TBUC *dest_mac );
extern void TB_rssi_increment_recv_count( int idx_wisun, TBUC *sour_mac );

#endif//__TB_RSSI_H__

