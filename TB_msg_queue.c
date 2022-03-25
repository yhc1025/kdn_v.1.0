#include <stdio.h>

#include "TB_util.h"
#include "TB_debug.h"
#include "TB_msg_queue.h"

////////////////////////////////////////////////////////////////////////////////

static key_t s_msg_queue_key;

////////////////////////////////////////////////////////////////////////////////

int TB_msgq_remove( void )
{
	/* remove queue */
    msgctl( s_msg_queue_key, IPC_RMID, 0 );
}

int TB_msgq_send( TB_MESSAGE *p_msg )
{
	int ret = -1;
	
    struct msqid_ds m_stat;	
    if( msgctl(s_msg_queue_key, IPC_STAT, &m_stat) == 0 )
    {
		if( p_msg )
		{
			ret = msgsnd( s_msg_queue_key, (void *)p_msg, sizeof(TB_MESSAGE) - sizeof(long), IPC_NOWAIT );
			if( ret < 0 )
				TB_dbg_msgq( "Error. Send Message : %d, 0x%04, 0x%04X\n", ret,  p_msg->type, p_msg->id );
		}

		TB_msgq_info( &m_stat, 1 );
    }

	return ret;
}

int TB_msgq_recv( TB_MESSAGE *p_msg, TB_MSG_TYPE type, int flag )
{
	int ret = -1;
	
    struct msqid_ds m_stat;	
    if( msgctl(s_msg_queue_key, IPC_STAT, &m_stat) == 0 )
    {
		if( p_msg )
			ret = msgrcv( s_msg_queue_key, (void *)p_msg, sizeof(TB_MESSAGE) - sizeof(long), type, flag ? IPC_NOWAIT : 0 );

		TB_msgq_info( &m_stat, 2 );
    }
	
	return ret;
}

int TB_msgq_reset( TB_MSG_TYPE type )
{
	struct msqid_ds m_stat;	
	while( 1 )
	{
	    if( msgctl(s_msg_queue_key, IPC_STAT, &m_stat) == 0 )
	    {
			if( m_stat.msg_qnum > 0 )
			{
				TB_MESSAGE msg;
				if( msgrcv( s_msg_queue_key, (void *)&msg, sizeof(TB_MESSAGE) - sizeof(long), type, IPC_NOWAIT ) == -1 )
					break;
			}
			else
				break;
	    }
	}

	return TRUE;
}

void TB_msgq_info( struct msqid_ds *p_q_stat, int flag )
{
#if 0
	if( p_q_stat )
	{
		//if( p_q_stat->msg_qnum > 2 )
		{		
		    TB_dbg_msgq("========== messege queue info =============\n");
		    TB_dbg_msgq(" message queue info : %s\n", flag==1 ? "SEND" : "RECV" );
		    TB_dbg_msgq(" 	msg_lspid : %d\n",p_q_stat->msg_lspid );
		    TB_dbg_msgq(" 	msg_qnum  : %d\n",p_q_stat->msg_qnum );
		    TB_dbg_msgq(" 	msg_stime : %d\n",p_q_stat->msg_stime );
		    TB_dbg_msgq("========== messege queue info end =============\n");
		}
	}
#endif
}

int TB_msgq_init( void )
{
	int ret = -1;
	
	s_msg_queue_key = msgget( (key_t)4292, IPC_CREAT | 0666 );
	if( s_msg_queue_key != -1 )
	{
		TB_msgq_reset( 0 );
		ret = 0;
	}
	else
	{
		TB_dbg_msgq( "ERROR. Message QUEUE init\r\n" );
	}
	
 	return ret;
}

int TB_msgq_deinit( void )
{
	TB_msgq_remove();
 	return TRUE;
}

