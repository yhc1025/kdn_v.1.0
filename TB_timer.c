#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include "TB_timer.h"

#define	TIMER_STATE_START		0x01
#define	TIMER_STATE_RUN			0x02
#define	TIMER_STATE_ALIVE		0x10
#define	TIMER_STATE_WAIT		0x20

////////////////////////////////////////////////////////////////////////////////

static void *make_timer( void *arg )
{
	TB_TIMER *t = (TB_TIMER *)arg;
	int		ret, alive;
	struct timeval	now;

	gettimeofday( &now, NULL );
	t->timeout.tv_sec  = now.tv_sec + t->req.tv_sec;
	t->timeout.tv_nsec = now.tv_usec * 1000 + t->req.tv_nsec;
	for( ; ; )
	{
		pthread_mutex_lock( &t->mutex );

		t->flag |= TIMER_STATE_WAIT;
		ret = 0;		

		while( (t->flag & TIMER_STATE_WAIT) && (ret != ETIMEDOUT) )
		{
			ret = pthread_cond_timedwait( &t->cond, &t->mutex, &t->timeout );
			break;
		}
			
		gettimeofday( &now, NULL );
		t->timeout.tv_sec  = now.tv_sec + t->req.tv_sec;
		t->timeout.tv_nsec = now.tv_usec * 1000 + t->req.tv_nsec;

		t->timeout.tv_sec += (t->timeout.tv_nsec / 1000000000);
		t->timeout.tv_nsec %= 1000000000;

		if( t->flag & TIMER_STATE_RUN )
			t->proc( t->arg );

		alive = (t->flag & TIMER_STATE_ALIVE) ? 1 : 0;

		pthread_mutex_unlock( &t->mutex );

		if( !alive ) 
			break;
	}

	return NULL;
}

static void timer_wakeup(TB_TIMER *t)
{
	pthread_mutex_lock( &t->mutex );
	t->flag &= ~TIMER_STATE_WAIT;
	pthread_cond_broadcast( &t->cond );
	pthread_mutex_unlock( &t->mutex );
}

/****************************************************************************
const struct timespec *interval	:	Timer interval
TB_TIMER_PROC proc,				:	Timer event handler
void *arg						:	Timer event handler arguments
*****************************************************************************/
int TB_timer_create( TB_TIMER *t, const struct timespec *interval, TB_TIMER_PROC proc, void *arg )
{
	int ret = -1;
	if( t )
	{
		bzero( t, sizeof(TB_TIMER) );
		
		t->proc	= proc;
		t->arg	= arg;
		t->flag	= TIMER_STATE_ALIVE;
		t->req.tv_sec	= interval->tv_sec;
		t->req.tv_nsec	= interval->tv_nsec;
		pthread_mutexattr_init( &t->attr );
		pthread_mutexattr_settype( &t->attr, PTHREAD_MUTEX_ERRORCHECK );
		pthread_mutex_init( &t->mutex, &t->attr );
		pthread_cond_init( &t->cond, NULL );

		if( pthread_create(&t->thid, NULL, make_timer, t) == 0 )
		{
			ret = 0;
		}
	}

	return ret;
}

int TB_timer_destroy( TB_TIMER *t )
{
	int ret = -1;
	if( t )
	{
		t->flag = 0;	/* stop and destroy */
		timer_wakeup( t );
		ret = pthread_join( t->thid, NULL );
		pthread_cond_destroy( &t->cond );
		pthread_mutex_destroy( &t->mutex );
	}
	
	return ret;
}

int TB_timer_start( TB_TIMER *t )
{
	int	ret = -1;
	if( t )
	{
		if( t->flag & TIMER_STATE_START )
			t->flag |= TIMER_STATE_RUN;
		else
			t->flag |= (TIMER_STATE_START | TIMER_STATE_RUN);

		timer_wakeup( t );
		ret = 0;
	}
	return ret;
}

int TB_timer_stop( TB_TIMER *t )
{
	int	ret = -1;
	if( t )
	{
		if( t->flag & TIMER_STATE_RUN )
		{
			t->flag &= ~TIMER_STATE_RUN;
			ret = 0;
		}
	}
	return ret;
}

int TB_timer_running( TB_TIMER *t )
{
	int	ret = 0;
	if( t )
	{
		if( t->flag & TIMER_STATE_RUN )
			ret = 1;
	}

	return ret;
}

