#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "TB_util.h"
#include "TB_debug.h"
#include "TB_elapse.h"

TBBL TB_elapse_set_init( TB_ELAPSE *p_elapse )
{
	TBBL ret = FALSE;

	if( p_elapse )
	{
		bzero( p_elapse, sizeof(TB_ELAPSE) );
		p_elapse->status = ELPASE_STATUS_READY;

		ret = TRUE;
	}

	return ret;
}

TBBL TB_elapse_set_start( TB_ELAPSE *p_elapse )
{
	TBBL ret = FALSE;
	
	if( p_elapse )
	{
		if( p_elapse->status == ELPASE_STATUS_READY )
		{
			gettimeofday( &p_elapse->t, NULL );
			p_elapse->status = ELPASE_STATUS_RUN;

			ret = TRUE;
		}
	}

	return ret;
}

TBBL TB_elapse_set_stop( TB_ELAPSE *p_elapse )
{
	TBBL ret = FALSE;
	
	if( p_elapse )
	{
		bzero( p_elapse, sizeof(TB_ELAPSE) );
		p_elapse->status = ELPASE_STATUS_READY;

		ret = TRUE;
	}

	return ret;
}

TBBL TB_elapse_set_reload( TB_ELAPSE *p_elapse )
{
	TBBL ret = FALSE;
	
	if( p_elapse )
	{
		if( p_elapse->status == ELPASE_STATUS_RUN )
		{
			struct timeval curr;
			gettimeofday( &p_elapse->t, NULL );

			ret = TRUE;
		}
	}

	return ret;
}

int TB_elapse_get_status( TB_ELAPSE *p_elapse )
{
	int status = ELPASE_STATUS_READY;
	if( p_elapse )
	{
		status = p_elapse->status;
	}

	return status;
}

TBUL TB_elapse_get_elapse_time( TB_ELAPSE *p_elapse )
{
	TBUL elapse = 0;
	
	if( p_elapse )
	{
		if( p_elapse->status == ELPASE_STATUS_RUN )
		{
			struct timeval curr;
			gettimeofday( &curr, NULL );
			
			elapse = TB_elapse_get_diff_time( &p_elapse->t, &curr );
		}
	}

	return elapse;
}

TBUL TB_elapse_print_elapse_time( TB_ELAPSE *p_elapse )
{
	TBUL elapse = TB_elapse_get_elapse_time( p_elapse );
	TB_dbg_elapse("elapse time = %d msec\r\n", elapse );
	return elapse;
}

TBBL TB_elapse_check_expire( TB_ELAPSE *p_elapse, TBUL expire_msec, int cont )
{
	TBBL ret = FALSE;
	if( p_elapse )
	{
		if( p_elapse->status == ELPASE_STATUS_RUN )
		{
			struct timeval curr;

			gettimeofday( &curr, NULL );
			
			TBUL elapse = TB_elapse_get_diff_time( &p_elapse->t, &curr );
			if( expire_msec <= elapse )
			{
				if( cont )
				{
					gettimeofday( &p_elapse->t, NULL );
				}
				else
				{
					TB_elapse_set_init( p_elapse );
				}
				
				ret = TRUE;

				TB_dbg_elapse( "Expired\r\n" );
			}
		}
	}

	return ret;
}

TBUL TB_elapse_get_diff_time( struct timeval *p_ts, struct timeval *p_te )
{
	TBUL elapse = 0;

	if( p_ts && p_te )
	{
		TBUL e = (p_te->tv_sec * 1000) + (p_te->tv_usec / 1000);
		TBUL s = (p_ts->tv_sec * 1000) + (p_ts->tv_usec / 1000);

		if( e >= s )
		{
			elapse = (TBUL)(e - s);
		}
		else
		{
			TB_dbg_elapse( "[%s] ERROR. check elapse time. **** (%d >= %d) *****\r\n", __FUNCTION__, e, s );
			
			struct timeval curr;
			gettimeofday( &curr, NULL );
			wmemcpy( p_ts, sizeof(struct timeval), &curr, sizeof(struct timeval) );
			wmemcpy( p_te, sizeof(struct timeval), &curr, sizeof(struct timeval) );

			TB_dbg_elapse( "[%s] RESET. elapse time. **** (%d >= %d) *****\r\n", __FUNCTION__, e, s );
		}
	}

	return elapse;
}

