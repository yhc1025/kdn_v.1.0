#ifndef __TB_NET_QUEUE_H__
#define __TB_NET_QUEUE_H__

#include "TB_type.h"

typedef struct tagQUEUE
{
	int 	idx;
	TBUC *	buf;
	int 	idx_f;
	int 	idx_r;
	int 	free;
	int 	size;

	pthread_mutexattr_t attr;
	pthread_mutex_t 	mutex;
	pthread_cond_t  	cond;
} TB_QUEUE;

typedef struct tagWISUNQ
{
	const char 	*name;
	void 		*ctx;

	TB_QUEUE	netq;
	
	/* common function */
	int		(*init)		(int, TB_QUEUE *, int);
	int		(*deinit)	(TB_QUEUE *);
	int		(*enq)		(TB_QUEUE *, TBUC*, TBUS);
	int		(*deq)		(TB_QUEUE *, TBUC*, TBUS*);
	int	 	(*reset)	(TB_QUEUE *);			//	Queue init - reset
	int	 	(*clear)	(TB_QUEUE *);			//	Queue init - reset and buffer clear
	int 	(*size)		(TB_QUEUE *);			//	Get size
	int 	(*remain)	(TB_QUEUE *);			//	Get remain size
} TB_WISUN_QUEUE;

extern TB_WISUN_QUEUE g_wisun_netq_1st;
extern TB_WISUN_QUEUE g_wisun_netq_2nd;

#endif	//__TB_NET_QUEUE_H__

