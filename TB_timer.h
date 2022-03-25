#ifndef __TB_TIMER_H__
#define __TB_TIMER_H__

#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////

#ifndef _PARAMS
#if HAVE_ANSI_PROTOTYPES
#define _PARAMS(ARGS) ARGS
#else /* Traditional C */
#define _PARAMS(ARGS) ()
#endif /* __STDC__ */
#endif /* _PARAMS */

////////////////////////////////////////////////////////////////////////////////

typedef void (*TB_TIMER_PROC)(void *arg);

////////////////////////////////////////////////////////////////////////////////

typedef struct tagTIMER
{
	TB_TIMER_PROC		proc;
	
	void *				arg;
	unsigned long 		flag;
	struct timespec		req;
	struct timespec		timeout;
	
	pthread_t			thid;
	pthread_mutexattr_t attr;
	pthread_mutex_t		mutex;
	pthread_cond_t		cond;
} TB_TIMER;

////////////////////////////////////////////////////////////////////////////////

extern int		TB_timer_create				_PARAMS((TB_TIMER *t, const struct timespec *interval, tb_proc proc, void *arg));
extern int  	TB_timer_start				_PARAMS((TB_TIMER hTimer));
extern int  	TB_timer_stop				_PARAMS((TB_TIMER hTimer));
extern int  	TB_timer_destroy			_PARAMS((TB_TIMER hTimer));
extern int  	TB_timer_running			_PARAMS((TB_TIMER hTimer));

////////////////////////////////////////////////////////////////////////////////

#endif //__TB_TIMER_H__

