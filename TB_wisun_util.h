#ifndef	__TB_WISUN_UTIL_H__
#define __TB_WISUN_UTIL_H__

#include "TB_j11.h"
#include "TB_type.h"

extern TBBL TB_wisun_util_ipaddr_req_check_packet( TBUC *buf, TBUS length );
extern int  TB_wisun_util_ipaddr_req_send( int idx, TB_ROLE role_src, TB_ROLE role_dst );
extern TBBL TB_wisun_util_ipaddr_res_check_packet( TBUC *buf, TBUS length );
extern int  TB_wisun_util_ipaddr_res_send( int idx, TB_ROLE role_src, TB_ROLE role_dst );
extern int 	TB_wisun_util_ipaddr_connt_device_save( TBUC *ip );
extern int 	TB_wisun_util_ipaddr_connt_device_set( TBBL is_term );

extern int  TB_wisun_util_timeinfo_res_send( int idx, TB_ROLE role_src, TB_ROLE role_dst );
extern TBBL TB_wisun_util_timeinfo_res_check_packet( TBUC *buf, TBUS length );
extern int  TB_wisun_util_timeinfo_set_systemtime( TBUC *p_datetime );
extern TBBL TB_wisun_util_timeinfo_req_check_packet( TBUC *buf, TBUS length );
extern int  TB_wisun_util_timeinfo_req_send( TBBL is_relay, char *file, int line );
extern int  TB_wisun_util_timeinfo_req_send_avaliable_timeinfo( int idx, TB_ROLE role_src, TB_WISUN_MODE mode_src );

extern TBUS TB_wisun_util_ems_make_packet( TBUC *p_dest, size_t dest_size, TBUC *p_sour, TBUS sour_len );
extern TBBL TB_wisun_util_ems_check_packet( TBUC *buf, TBUS length );

extern TBUS TB_wisun_util_ack_make_packet( TBUC *p_dest );
extern TBBL TB_wisun_util_ack_check_packet( TBUC *buf, TBUS length );

#endif//__TB_WISUN_UTIL_H__

