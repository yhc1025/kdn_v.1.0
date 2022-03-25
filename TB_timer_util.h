#ifndef __TB_TIMER_UTIL_H__
#define __TB_TIMER_UTIL_H__

#include "TB_timer.h"
#include "TB_type.h"

extern TB_TIMER *TB_timer_handle_open( TB_TIMER_PROC proc, TBUL period_msec );
extern void 	TB_timer_handle_stop( TB_TIMER *timer );
extern TB_TIMER *TB_timer_handle_close( TB_TIMER *timer );

#endif//__TB_TIMER_UTIL_H__

