#ifndef	__TB_ELAPSE_H__
#define __TB_ELAPSE_H__

#include <stdio.h>
#include <sys/time.h>

#include "TB_type.h"

typedef enum tagELAPSE_STATUS
{
	ELPASE_STATUS_READY = 0,
	ELPASE_STATUS_RUN,
	
	ELPASE_STATUS_MAX,
} TB_ELAPSE_STATUS;

typedef struct tagELAPSE
{
	struct timeval 	t;
	int				status;
} TB_ELAPSE;

extern TBBL TB_elapse_set_init( TB_ELAPSE *p_elapse );
extern TBBL TB_elapse_set_start( TB_ELAPSE *p_elapse );
extern TBBL TB_elapse_set_stop( TB_ELAPSE *p_elapse );
extern TBBL TB_elapse_set_reload( TB_ELAPSE *p_elapse );
extern int  TB_elapse_get_status( TB_ELAPSE *p_elapse );
extern TBBL TB_elapse_check_expire( TB_ELAPSE *p_elapse, TBUL expire_msec, int cont );
extern TBUL TB_elapse_get_diff_time( struct timeval *p_ts, struct timeval *p_te );

extern TBUL TB_elapse_get_elapse_time( TB_ELAPSE *p_elapse );
extern TBUL TB_elapse_print_elapse_time( TB_ELAPSE *p_elapse );

#endif//__TB_ELAPSE_H__

