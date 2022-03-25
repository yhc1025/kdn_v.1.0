#ifndef	 __TB_PING_H__
#define __TB_PING_H__

#include "TB_type.h"

extern int TB_ping_init( int idx );
extern int TB_ping_send( int idx, TBBL flag_immedi );
extern int TB_ping_send_data( int idx );

extern int TB_ping_set_notify_success( int idx );
extern int TB_ping_check_notify_success( int idx );

#endif//__TB_PING_H__

