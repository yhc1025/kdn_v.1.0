#ifndef __TB_BASH_H__
#define __TB_BASH_H__

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "KDN_LIB.h"

////////////////////////////////////////////////////////////////////////////////

typedef enum tagDEV_TYPE
{
	DEV_ROLE_GGW	= 1,
	DEV_ROLE_TERM,
	DEV_ROLE_RELAY,
	DEV_ROLE_REPEATER,
} TB_DEV_TYPE;

#define MANUF_DATE_YEAR		2021
#define MANUF_DATE_MONTH	11
#define MANUF_DATE_DAY		21

////////////////////////////////////////////////////////////////////////////////

#define VERSION_MAJOR		1
#define VERSION_MINOR		0

#define MODEL_NAME			"KDN_WISUN"

////////////////////////////////////////////////////////////////////////////////

#define _1SEC_MSEC	(1000)
#define _10SEC_MSEC	(_1SEC_MSEC*10)
#define _1MIN_MSEC	(_1SEC_MSEC*60)
#define _2MIN_MSEC	(_1SEC_MSEC*120)
#define _3MIN_MSEC	(_1SEC_MSEC*180)
#define _4MIN_MSEC	(_1SEC_MSEC*240)
#define _5MIN_MSEC	(_1SEC_MSEC*300)

#define _1MIN_SEC	(60)
#define _2MIN_SEC	(_1MIN_SEC*2)
#define _5MIN_SEC	(_1MIN_SEC*5)
#define _10MIN_SEC	(_1MIN_SEC*10)

#define YEAR_MAX	2100
#define YEAR_MIN	2000

////////////////////////////////////////////////////////////////////////////////

#define ENABLE_WDT
#define ENABLE_AUTO_CONNECT

////////////////////////////////////////////////////////////////////////////////

typedef struct tagCONDMUTEX
{
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
	int				flag;	
} TB_CONDMUTEX;

typedef struct tagTHREAD_HANDLER
{
	volatile int	run;		//	check a running of thread while loop
	volatile int	wakeup;		//	check a condition wait
	pthread_mutex_t	mutex;		//	mutex for sync 
	pthread_cond_t	cond;		//	condition for sync
	pthread_t		thread;		//	thread
} TB_THREAD_HANDLER;

#define MUTEX_TRYLOCK(m)		pthread_mutex_trylock(m)
#define MUTEX_LOCK(m)			pthread_mutex_lock(m)
#define MUTEX_UNLOCK(m)			pthread_mutex_unlock(m)
#define MUTEX_INIT(m,a)			pthread_mutex_init(m,a)

#define	COND_WAIT(c,m)			pthread_cond_wait(c, m)
#define	COND_TIMEWAIT(c,m,t)	pthread_cond_timedwait(c,m,t)
#define COND_BROADCAST(c)		pthread_cond_broadcast(c)
#define COND_SIGNAL(c)			pthread_cond_signal(c)
#define COND_INIT(c,a)			pthread_cond_init(c,a)

/*
*	init a pointer of condition & mutext variable
*/
#define INIT_COND_MUTEX_P(p)	do {									\
									p->flag = 0;						\
									MUTEX_INIT( &p->mutex, NULL );		\
									COND_INIT ( &p->cond, NULL );		\
								} while(0)

#define INIT_COND_MUTEX(p)		do {									\
									INIT_COND_MUTEX_P(p);				\
								} while(0)

/*
*	init a variable of condition & mutext
*/
#define INIT_COND_MUTEX_V(v)	do {									\
									v.flag = 0;							\
									MUTEX_INIT( &v.mutex, NULL );		\
									COND_INIT ( &v.cond, NULL );		\
								} while(0)

#define INIT_THREAD_HANDLER(p)	do {									\
									*(volatile int *)&p->run = 1;		\
									*(volatile int *)&p->wakeup = 0;	\
									MUTEX_INIT( &p->mutex, NULL );		\
									COND_INIT ( &p->cond, NULL );		\
									p->thread = -1;						\
								} while(0)

#endif//__TB_BASH_H__

