#include "TB_timer_util.h"

////////////////////////////////////////////////////////////////////////////////

#define msec2sec(m)		(m/1000)
#define msec2usec(m)	(m*1000)
#define msec2nsec(m)	(m*1000000)

////////////////////////////////////////////////////////////////////////////////

TB_TIMER *TB_timer_handle_open( TB_TIMER_PROC proc, TBUL period_msec )
{
	size_t 	  size  = sizeof( TB_TIMER );
	TB_TIMER *timer = malloc( size );
	if( timer )
	{
		bzero( timer, size );

		struct timespec period;
		period.tv_sec = msec2sec( period_msec );
		if( period.tv_sec > 0 )
		{
			TBUL mod = period_msec % 1000;
			period.tv_nsec = msec2nsec( mod );
		}
		else
		{
			period.tv_nsec = msec2nsec( period_msec );
		}

		if( TB_timer_create( timer, &period, proc, NULL ) != 0 )
		{
			free( timer );
			timer = NULL;
		}
	}

	if( timer )
	{
		TB_timer_start( timer );
	}

	return timer;
}

void TB_timer_handle_stop( TB_TIMER *timer )
{
	if( timer && TB_timer_running(timer) )
		TB_timer_stop( timer );
}

TB_TIMER *TB_timer_handle_close( TB_TIMER *timer )
{
	if( timer )
	{
		TB_timer_stop( timer );
		TB_timer_destroy( timer );

		free( timer );
		timer = NULL;
	}
	
	return (TB_TIMER *)NULL;
}

