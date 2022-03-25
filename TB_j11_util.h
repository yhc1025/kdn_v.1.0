#ifndef	__TB_J11_UTIL_H__
#define __TB_J11_UTIL_H__

#include "TB_type.h"
#include "TB_j11.h"

extern int 	TB_j11_print_recv_data_info( int idx, TBUS rcv_cmd, TBUC *rcvdata );

extern void TB_j11_util_print_wisun_direction( TB_WISUN_DIR dir );
extern void TB_j11_util_debug_state( int idx, TB_STATE state );

#endif//__TB_J11_UTIL_H__

