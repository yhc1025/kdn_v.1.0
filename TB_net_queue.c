#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "TB_debug.h"
#include "TB_j11.h"
#include "TB_log.h"
#include "TB_util.h"
#include "TB_net_queue.h"

////////////////////////////////////////////////////////////////////////////////

int TB_netq_init( int idx, TB_QUEUE *q, int size );
int TB_netq_deinit( TB_QUEUE *q );
int TB_netq_enq( TB_QUEUE *q, TBUC* input, TBUS input_size );
int TB_netq_deq( TB_QUEUE *q, TBUC* output, TBUS* output_size );
int TB_netq_reset( TB_QUEUE *q );
int TB_netq_clear( TB_QUEUE *q );
int TB_netq_get_size( TB_QUEUE *q );
int TB_netq_get_remain( TB_QUEUE *q );

////////////////////////////////////////////////////////////////////////////////

TB_WISUN_QUEUE g_wisun_netq_1st =
{
	.name			= "wisun#1 data queue",
	
	.init			= TB_netq_init,
	.deinit			= TB_netq_deinit,
	.enq			= TB_netq_enq,
	.deq			= TB_netq_deq,
	.reset			= TB_netq_reset,
	.clear			= TB_netq_clear,
	.size			= TB_netq_get_size,
	.remain			= TB_netq_get_remain,
};

TB_WISUN_QUEUE g_wisun_netq_2nd =
{
	.name			= "wisun#2 data queue",
	
	.init			= TB_netq_init,
	.deinit			= TB_netq_deinit,
	.enq			= TB_netq_enq,
	.deq			= TB_netq_deq,
	.reset			= TB_netq_reset,
	.clear			= TB_netq_clear,
	.size			= TB_netq_get_size,
	.remain			= TB_netq_get_remain,
};

////////////////////////////////////////////////////////////////////////////////

int TB_netq_init( int idx, TB_QUEUE *q, int size )
{
	int ret = -1;

	if( q )
	{
		TB_netq_deinit( q );

		if( size > 0 )
		{
			q->idx		= idx;
			q->buf		= NULL;
			q->idx_f	= 0;
			q->idx_r	= 0;
			q->free		= size;
			q->size		= size;

			if( q->size > 0 )
			{
				size_t alloc_size = sizeof(TBUC) * q->size;
				q->buf = malloc( alloc_size );
				if( q->buf )
				{
					bzero( q->buf, alloc_size );
				}
			}

			pthread_mutexattr_init( &q->attr );
			pthread_mutexattr_settype( &q->attr, PTHREAD_MUTEX_ERRORCHECK );
			pthread_mutex_init( &q->mutex, &q->attr );
			pthread_cond_init ( &q->cond, NULL );

			ret = 0;
		}
	}
	
	return ret;
}

int TB_netq_deinit( TB_QUEUE *q )
{
	int ret = -1;

	if( q )
	{
		if( q->buf )
		{
			free( q->buf );
			q->buf = NULL;
		}

		bzero( q, sizeof(TB_QUEUE) );

		ret = 0;
	}

	return ret;
}

#define __LOCK(m)	\
{\
	TB_dbg_netq( "Try. Lock\r\n" );		\
	pthread_mutex_lock( m );			\
	TB_dbg_netq( "Ok. Lock\r\n" );		\
}

#define __UNLOCK(m)	\
{\
	TB_dbg_netq( "Try. Unlock\r\n" );	\
	pthread_mutex_unlock( m );			\
	TB_dbg_netq( "Ok. Unlock\r\n" );	\
}

#define __SIGNAL(c)	\
{\
	TB_dbg_netq( "Try. Signal\r\n" );	\
	pthread_cond_signal( c );			\
	TB_dbg_netq( "Ok. Signal\r\n" );	\
}

#define __WAIT(c, m)	\
{\
	TB_dbg_netq( "Try. Wait\r\n" );		\
	pthread_cond_wait( c, m );			\
	TB_dbg_netq( "Ok. Wait\r\n" );		\
}

volatile int s_deq_check = 0;
volatile int s_enq_check = 0;
int TB_netq_enq( TB_QUEUE *q, TBUC* input, TBUS input_size )
{
	int ret = -1;
	
	if( q && input && input_size > 1 )
	{
__LOCK( &q->mutex );
		TB_dbg_netq( "ENQ START [%d] idx_r=%d, idx_f=%d, free=%d\r\n", q->idx, q->idx_r, q->idx_f, q->free );	
		
		/***********************************************************************
		*	남아 있는 공간이 없거나 입력 사이즈보다 작으면
		*		--> deq에서 COND_SIGNAL이 보낼 때까지 기다린다.
		***********************************************************************/
		if( q->free < input_size )
		{
			TB_dbg_netq( "1. Can't ENQ [%d] : %d, %d\r\n", q->idx, input_size, q->free );
__WAIT( &q->cond, &q->mutex );
		}

		if( q->free > input_size && input_size > 0 )
		{
			for( int i=0; i<input_size; i++ )
			{
				*(q->buf + q->idx_r) = *(input + i);
				q->idx_r ++;
				q->idx_r %= q->size;
			}
			q->free -= input_size;

			TB_dbg_netq( "enq : input_size=%d, idx_r=%d, idx_f=%d, free=%d\r\n", input_size, q->idx_r, q->idx_f, q->free );

			ret = 0;
		}
		else
		{
			TB_dbg_netq( "2. Can't ENQ [%d] : %d, %d\r\n", q->idx, input_size, q->free );	
		}

		if( TB_dm_is_on( DMID_NETQ ) )
			TB_util_data_dump( "enq", input, input_size );

		TB_dbg_netq( "ENQ END [%d] idx_r=%d, idx_f=%d, free=%d\r\n", q->idx, q->idx_r, q->idx_f, q->free );

//__SIGNAL( &q->cond );
__UNLOCK( &q->mutex );
	}

	TB_dbg_netq("ENQ END - ret = %d\r\n", ret );
	
	return ret;
}

extern TBBL TB_j11_check_receive_header( TBUC *p_header );
int TB_netq_deq( TB_QUEUE *q, TBUC* output, TBUS* output_size )
{
	int ret = -1;
	TBUC uni_buf[4]	= {0, 0, 0, 0};
	
	if( q == NULL || output == NULL || output_size == NULL )
	{
		TB_dbg_netq( "ERROR. QUEUE is NULL\r\n" );
		return ret;
	}

	/*
	*	20211129
	*/
	if( q->free == q->size )
	{
		TB_dbg_netq("DEQ : Queue Data is empty\r\n" );
		return ret;
	}
	/*
	*	20211129
	*/
	if( q->free + 10 >= q->size )
	{
		TB_dbg_netq("DEQ : Queue Data is too small : %d\r\n", q->size - q->free );
		return ret;
	}

__LOCK( &q->mutex );

	if( q->free >= q->size || q->idx_f == q->idx_r )
	{
		TB_dbg_netq("DEQ : Queue Reset\r\n" );
		TB_netq_reset( q );
		*output_size = 0;

__SIGNAL( &q->cond );
__UNLOCK( &q->mutex );

		return ret;
	}

__GOTO_LABEL__ :
	
	TB_dbg_netq( "DEQ START [%d] idx_r=%d, idx_f=%d, free=%d \r\n", q->idx, q->idx_r, q->idx_f, q->free );

	TBUS packet_size;
	TBUS skip_size = 0;
	TBBL is_q_empty = FALSE;

	int idx_f_backup1 = q->idx_f;
	int idx_r_backup1 = q->idx_r;
	
	while( 1 )
	{
		/*
		*	Read Unique Header
		*/
		int idx_f = q->idx_f;
		for( int i=0; i<4; i++ )
		{
			uni_buf[i] = *(TBUC *)(q->buf + idx_f);
			idx_f++;
			idx_f %= q->size;

			if( idx_f == q->idx_r )
			{
				TB_dbg_netq("DEQ : Queue Reset\r\n" );
				TB_netq_reset( q );
				*output_size = 0;

				is_q_empty = TRUE;
				break;
			}
		}

		////////////////////////////////////////////////////////////////////////

		if( is_q_empty == FALSE )
		{
			TB_dbg_netq("deq : idx_r=%4d, idx_f=%4d(%4d)   ------>  %02X %02X %02X %02X\r\n",	\
								q->idx_r, q->idx_f, idx_f,									\
								uni_buf[0], uni_buf[1], uni_buf[2], uni_buf[3] ) ;

			extern TBUC g_uniq_res[LEN_UNIQ_CODE];
			if( memcmp( &uni_buf[0], &g_uniq_res[0], LEN_UNIQ_CODE ) == 0 )
			{
				TBUC *q_buf_f = (TBUC*)(q->buf + q->idx_f);
				
				TBUC size_high;
				TBUC size_low;

				/***************************************************************
				*					Read Packet size info
				***************************************************************/
				int idx_f1 = q->idx_f;
				if( idx_f1 + 6 > q->size-1 )
				{
					TBUS s = 6 - (q->size - q->idx_f);
					size_high = *(TBUC *)(q->buf + s);
					TB_dbg_netq( "deq : %d, %02X \r\n", s, size_high );
				}
				else
				{
					size_high = q_buf_f[6];
				}

				if( idx_f1 + 7 > q->size-1 )
				{
					TBUS s = 7 - (q->size - q->idx_f);
					size_low = *(TBUC *)(q->buf + s);
					TB_dbg_netq( "deq : %d, %02X \r\n", s, size_low );
				}
				else
				{
					size_low = q_buf_f[7];
				}				

				packet_size = (size_high << 8) | (size_low << 0);
				packet_size += 8;	//	4byte(uniqe code) + 2byte(func code) + 2byte(packet size);
				TB_dbg_netq( "deq : q->idx_f=%d, q->idx_r=%d, packet_size = %d \r\n", q->idx_f, q->idx_r, packet_size );

				////////////////////////////////////////////////////////////////

				int idx_f_backup2 = q->idx_f;
				int idx_r_backup2 = q->idx_r;

				int temp = 0;
				int check_error = 0;

				uni_buf[0] = 0x00;
				uni_buf[1] = 0x00;
				uni_buf[2] = 0x00;
				uni_buf[3] = 0x00;

				for( int i=0; i<packet_size; i++ )
				{
					if( q->idx_f == q->idx_r )
					{
						/*******************************************************
						*	WISUN 패킷데이터가 중간에 끊어진 상태.
						*	queue buffer 마지막까지 읽었지만 온전한 데이터가 아니다.
						*   에러처리하고 queue index를 복원한다.
						*	--> 미수신 데이터가 마저 수신될 때까지 다시 돌아간다.
						//------------------------------------------------------
						//    | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
						//------------------------------------------------------
						//0000| D0 F9 EE 5D 60 18 00 65 03 F1 25 E4 FE 80 00 00
						//0010| 00 00 00 00 02 1D 12 91 00
						*******************************************************/

						TB_dbg_netq( "1. deq : WISUN data packet is abnormal. q->idx_f=%d, q->idx_r=%d, %d\r\n", q->idx_f, q->idx_r,  i );
						
						q->idx_f = idx_f_backup1;
						q->idx_r = idx_r_backup1;

						TB_dbg_netq( "2. deq : WISUN data packet is abnormal. q->idx_f=%d, q->idx_r=%d, %d\r\n", q->idx_f, q->idx_r,  i );

						check_error = 1;
						break;
					}

					////////////////////////////////////////////////////////////
					//	0x0000 offset에 D0 F9 EE 5D, size 00 65이다.
					//	0x0019 offset에 D0 F9 EE 5D, size 00 65이다.
					//	0x0019 offset부터 데이터를 다시 읽도록 한다.
					//	 --> 이전 데이터가 수신이 덜 된 상태에서 신규 데이터가
					//	     수신된 상태. 이전 데이터는 버리고 신규데이터부터
					//		 다시 읽도록 한다.
					//----------------------------------------------------------
					//    | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
					//----------------------------------------------------------
					//0000|*D0 F9 EE 5D*60 18 00 65 03 F1 25 E4 FE 80 00 00
					//0010| 00 00 00 00 02 1D 12 91 00*D0 F9 EE 5D*60 18 00
					//0020| 65 03 F1 28 03 FE 80 00 00 00 00 00 00 02 1D 12
					//0030| 91 00 03 5A 85 0E 1A 0E 1A 56 78 00 02 DE 00 46
					//0040| 61 E6 5C 15 37 C6 5E C2 DB 0D DF F2 09 A9 AE 2A
					//0050| 16 84 93 78 9F 3F C1 5E CF 4F 8C 8B 89 C1 66 C6
					//0060| C9 57 03 95 FB 66 EA 1F BA B6 FA 9C 13
					//----------------------------------------------------------
					if( i >= 4 )
					{
						if( (uni_buf[0] == g_uniq_res[0]) &&	\
							(uni_buf[1] == g_uniq_res[1]) &&	\
							(uni_buf[2] == g_uniq_res[2]) &&	\
							(uni_buf[3] == g_uniq_res[3]) )
						{
							TB_dbg_netq( "3. deq : WISUN data packet is abnormal. q->idx_r=%d, q->idx_f=%d, %d\r\n", q->idx_r, q->idx_f, i );
							
							q->idx_f = idx_f_backup2;
							q->idx_r = idx_r_backup2;
							q->free += (i - sizeof(uni_buf));

							TB_log_append( LOG_TYPE_WISUN_DATA_ERROR, NULL, -1 );

							TB_dbg_netq( "4. deq : WISUN data packet is abnormal. q->idx_r=%d, q->idx_f=%d, %d\r\n", q->idx_r, q->idx_f, i );

							goto __GOTO_LABEL__;
						}
						else if( (uni_buf[0] == g_uniq_res[0]) &&	\
								 (uni_buf[1] == g_uniq_res[1]) &&	\
								 (uni_buf[2] == g_uniq_res[2]) )
						{
							if( *(q->buf + q->idx_f) == g_uniq_res[3] )
							{
								uni_buf[3] = *(q->buf + q->idx_f);	
							}
							else
							{
								uni_buf[0] = 0x00;
								uni_buf[1] = 0x00;
								uni_buf[2] = 0x00;
							}
						}
						else if( (uni_buf[0] == g_uniq_res[0]) &&	\
								 (uni_buf[1] == g_uniq_res[1]) )
						{
							if( *(q->buf + q->idx_f) == g_uniq_res[2] )
							{
								uni_buf[2] = *(q->buf + q->idx_f);	
							}
							else
							{
								uni_buf[0] = 0x00;
								uni_buf[1] = 0x00;
							}
						}
						else if( (uni_buf[0] == g_uniq_res[0]) )
						{
							if( *(q->buf + q->idx_f) == g_uniq_res[1] )
							{
								uni_buf[1] = *(q->buf + q->idx_f);	
							}
							else
							{
								uni_buf[0] = 0x00;
							}
						}
						else if( *(q->buf + q->idx_f) == g_uniq_res[0] )
						{
							uni_buf[0] = *(q->buf + q->idx_f);
							uni_buf[1] = 0x00;
							uni_buf[2] = 0x00;
							uni_buf[3] = 0x00;

							idx_f_backup2 = q->idx_f;
							idx_r_backup2 = q->idx_r;
						}
					}

					////////////////////////////////////////////////////////////
						
					*(output + i) = *(q->buf + q->idx_f);

					q->idx_f ++;
					q->idx_f %= q->size;
				}

				if( check_error == 0 )
				{
					if( skip_size != 0 )
					{
						TB_dbg_netq( "deq : skip_size = %d\r\n", skip_size );
					}
					
					q->free += (packet_size + skip_size);

					////////////////////////////////////////////////////////////

					*output_size = packet_size;

					if( TB_dm_is_on( DMID_NETQ ) )
						TB_util_data_dump( "deq", output, *output_size );

					TB_dbg_netq( "deq : idx_r=%d, idx_f=%d, free=%d\r\n", q->idx_r, q->idx_f, q->free );

					ret = 0;
				}
				break;
			}
			else
			{
				q->idx_f ++;
				q->idx_f %= q->size;

				skip_size ++;
			}

			////////////////////////////////////////////////////////////////////

			if( idx_f == q->idx_r )
			{
				TB_dbg_netq("DEQ : Queue Reset\r\n" );
				TB_netq_reset( q );
				*output_size = 0;
				break;
			}
		}
		else
		{
			TB_dbg_netq("DEQ : Queue Reset\r\n" );
			TB_netq_reset( q );
			*output_size = 0;
			break;
		}
	}

	TB_dbg_netq( "DEQ END [%d] idx_r=%d, idx_f=%d, free=%d\r\n", q->idx, q->idx_r, q->idx_f, q->free );

__SIGNAL( &q->cond );
__UNLOCK( &q->mutex );	

	return ret;
}

int TB_netq_reset( TB_QUEUE *q )										// 큐의 내용을 전부 초기화
{
	if( q == NULL )
		return -1;
	
	q->idx_f	= 0;
	q->idx_r	= 0;
	q->free		= q->size;

	return 0;
}

int TB_netq_clear( TB_QUEUE *q )										// 큐의 내용을 전부 초기화
{
	if( q == NULL )
		return -1;
	
	q->idx_f	= 0;
	q->idx_r	= 0;
	q->free		= q->size;
	bzero( q->buf, sizeof(TBUC) * q->size );

	return 0;
}

int TB_netq_get_size( TB_QUEUE *q )									// 큐의 크기 반환
{
	if( q == NULL )
		return -1;
	
	return q->size;
}

int TB_netq_get_remain( TB_QUEUE *q )								// 큐에 남아 있는 공간 반환
{
	if( q == NULL )
		return -1;
	
	return (q->size - q->idx_r);
}

