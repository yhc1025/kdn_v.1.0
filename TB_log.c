#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "TB_elapse.h"
#include "TB_msg_queue.h"
#include "TB_debug.h"
#include "TB_util.h"
#include "TB_rb.h"
#include "TB_aes_evp.h"
#include "TB_resource.h"
#include "TB_log.h"

////////////////////////////////////////////////////////////////////////////////

TB_LOG s_log_sys;
TB_LOG s_log_comm;

////////////////////////////////////////////////////////////////////////////////

#define LOG_SECU_START	RESID_LOG_SECU_BOOT
#define LOG_COMM_START	RESID_LOG_COMM_SUCCESS_GGW2FRTU
static TBUC *tb_log_get_log_string( TB_LOGCODE code )
{
	TBUC *string = NULL;
	if( code < LOG_CODE_MAX )
	{
		TB_RESID rid;

		if( code >= LOG_TYPE_COMM_START )
		{
			code -= LOG_TYPE_COMM_START;
			rid = LOG_COMM_START + code;
			string = TB_resid_get_string( rid );
		}
		else
		{
			code -= LOG_TYPE_SECU_START;
			rid = LOG_SECU_START + code;
			string = TB_resid_get_string( rid );
		}
	}

	return string;
}

static int tb_log_item_append( TB_LOGITEM *p_item, TB_LOG *p_log )
{
	int ret = -1;

	if( p_item && p_log )
	{
		TB_log_item_dump( p_item, "append" );

		TB_rb_item_push( &p_log->rb, p_item );
		p_log->flag = 1;
		ret = 0;
	}

	return ret;
}

static int tb_log_item_append_sys( TB_LOGITEM *p_item )
{
	int ret = -1;

	if( p_item )
	{
		ret = tb_log_item_append( p_item, &s_log_sys );
		if( ret == 0 )
			TB_dbg_log( "SYS Log count = %d\r\n", TB_rb_get_count( &s_log_comm.rb ) );
	}
	else
	{
		TB_dbg_log( "ERROR. item is NULL\r\n" );
	}

	return ret;
}

static int tb_log_item_append_comm( TB_LOGITEM *p_item )
{
	int ret = -1;

	if( p_item )
	{
		ret = tb_log_item_append( p_item, &s_log_comm );
		if( ret == 0 )
			TB_dbg_log( "COMM Log count = %d\r\n", TB_rb_get_count( &s_log_comm.rb ) );
	}
	else
	{
		TB_dbg_log( "ERROR. item is NULL\r\n" );
	}

	return ret;
}

int TB_log_append( TB_LOGCODE code, TB_LOGCODE_DATA *p_code_data, TB_ACCOUNT_TYPE account )
{
	int ret = 0;	
	
	if( code < LOG_CODE_MAX )
	{
		TB_LOGITEM item;
		bzero( &item, sizeof(item) );

		item.t	  	 = time( NULL );
		item.code    = code;
		item.account = account;
		
		if( p_code_data )
		{
			if( p_code_data->size > 0 && p_code_data->size < sizeof(item.code_data) )
			{				
				wmemcpy( &item.code_data, sizeof(item.code_data), p_code_data, sizeof(TB_LOGCODE_DATA) );
			}
			else if( p_code_data->size >= sizeof(item.code_data) )
			{
				ret = -1;
				TB_dbg_log("ERROR. item's p_code_data size is too large.(%d)\r\n", p_code_data->size );
			}
			else if( p_code_data->size <= 0 )
			{
				ret = -1;
				TB_dbg_log("ERROR. item's p_code_data size is too small.(%d)\r\n", p_code_data->size );
			}
		}

		if( ret == 0 )
		{
			if( code >= LOG_TYPE_COMM_START )
				ret = tb_log_item_append_comm( &item );
			else
				ret = tb_log_item_append_sys( &item );

#ifdef USE_LOG_SAVE_DELAY
#else
	#ifdef USE_LOG_SAVE_IMMEDIATE_MSGQ
			TB_MESSAGE msg;
			static TBUC s_type = 0;

			s_type = (code >= LOG_TYPE_COMM_START) ? LOG_TYPE_COMM : LOG_TYPE_SECU;			

			msg.type = MSG_TYPE_LOG;
			msg.id	 = MSG_CMD_LOG_SAVE;
			msg.data = &s_type;
			msg.size = 1;
			TB_msgq_send( &msg );

			TB_dbg_log( "SEND. message log : %d\r\n", msg.id );
	#else
			TB_log_save_check( LOG_TYPE_SECU_START );
			TB_log_save_check( LOG_TYPE_COMM_START );
	#endif
#endif
		}
	}
	else
	{
		ret = -1;
	}

	return ret;
}

TBBL TB_log_check_detail_data( TB_LOGITEM *p_item )
{
	TBBL check = FALSE;
	
	if( p_item )
	{
		check = ( p_item->code_data.size > 0 && p_item->code_data.size < sizeof(p_item->code_data) ) ? TRUE : FALSE;
	}

	return check;
}

int TB_log_item_string( TB_LOGITEM *p_item, char *p_log_str, size_t log_str_size )
{
	int ret = -1;
	
	if( p_item && p_log_str )
	{
		TBUC *p_account	 = p_item->account == ACCOUNT_TYPE_ADMIN ? "ADMIN " :	\
						   p_item->account == ACCOUNT_TYPE_USER  ? "USER  " :	\
																   "SYSTEM";
		TBUC *p_str 	 = tb_log_get_log_string( p_item->code );
		TBUC *p_time_str = TB_util_get_datetime_string( p_item->t );
		
		if( p_str && p_time_str )
		{
			snprintf( p_log_str, log_str_size, "[%s][%s] %s", p_time_str, p_account, p_str );

			if( TB_log_check_detail_data( p_item ) == TRUE )
			{
				char *str = " [O]";
				if( str )
				{
					size_t ds = log_str_size - wstrlen(p_log_str);
					size_t ss = wstrlen( str );
					if( ds > ss )
					{
						wstrncat( p_log_str, ds, str, ss );
					}
				}
			}

			ret = 0;
		}
		else if( p_str )
		{
			snprintf( p_log_str, log_str_size, "[----/--/-- --:--:--] %s", p_str );
		}
		else if( p_time_str )
		{
			snprintf( p_log_str, log_str_size, "[%s] ------------------", p_time_str );
		}
	}

	return ret;
}

void TB_log_item_dump( TB_LOGITEM *p_item, char *p_msg )
{
	if( p_item )
	{
		TBUC *p_str 	 = tb_log_get_log_string( p_item->code );
		TBUC *p_time_str = TB_util_get_datetime_string( p_item->t );
		if( p_str && p_time_str )
		{
			if( p_msg )
				TB_prt_log( "[%s] [%s] %s (code=%d)\r\n", p_msg, p_time_str, p_str, p_item->code );
			else
				TB_prt_log( "[%s] %s (code=%d)\r\n", p_time_str, p_str, p_item->code );
		}
		else if( p_str )
		{
			if( p_msg )
				TB_prt_log( "[%s] [----/--/-- --:--:--] %s (code=%d)\r\n", p_msg, p_str, p_item->code );
			else
				TB_prt_log( "[----/--/-- --:--:--] %s (code=%d)\r\n", p_str, p_item->code );
		}
		else if( p_time_str )
		{
			if( p_msg )
				TB_prt_log( "[%s] [%s] XXXXXXXXXXXXXXX (code=%d)\r\n", p_msg, p_time_str, p_item->code );
			else
				TB_prt_log( "[%s] XXXXXXXXXXXXXXX (code=%d)\r\n", p_time_str, p_item->code );
		}
	}
}

void TB_log_dump( TB_LOG *p_log )
{
	if( p_log )
	{
		if( p_log->rb.count > 0 )
		{
			int i=0;

			TB_dbg_log( "******************** LOG DUMP ********************\r\n" );
			TB_LOGITEM *p_item = NULL;
			int head = p_log->rb.head;
			int tail = p_log->rb.tail;
			while( tail != head )
			{
			    if( tail >= p_log->rb.size )
			    {
			        tail = 0;
			    }

				p_item = (TB_LOGITEM *)((unsigned char *)p_log->rb.data + tail);
				if( p_item )
				{
					TB_dbg_log( "[%04d] ", i+1 );
					TB_log_item_dump( p_item, "log dump" );

					i++;
				}

				tail += p_log->rb.item_size;
			}
			TB_dbg_log( "**************************************************\r\n" );
		}
		else
		{
			TB_dbg_log( "LOG DATA IS NONE.\r\n" );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUC s_proc_flag_log = 1;
static void *tb_log_proc( void *arg )
{
	static time_t 	s_sec_prev = 0;
	time_t			sec_curr;
	
	TB_dbg_log( "=================================\r\n" );
	TB_dbg_log( "        Log Process Running      \r\n" );
	TB_dbg_log( "=================================\r\n" );
	while( s_proc_flag_log )
	{
#ifdef USE_LOG_SAVE_DELAY
		sec_curr = time( NULL );
		
		if( s_sec_prev == 0 )
			s_sec_prev = sec_curr;

		if( s_sec_prev + DEF_SAVE_PERIOD <= sec_curr )
		{
			TB_log_save_check( LOG_TYPE_SECU );
			TB_log_save_check( LOG_TYPE_COMM );
			
			s_sec_prev = sec_curr;
		}
#else
	#ifdef USE_LOG_SAVE_IMMEDIATE_MSGQ
		TB_MESSAGE msg;
		if( TB_msgq_recv(&msg, MSG_TYPE_LOG, NOWAIT) > 0 )
		{
			TB_dbg_log( "RECV. message log : %d\r\n", msg.id );
			if( msg.id == MSG_CMD_LOG_SAVE )
			{
				TBUC type = *(TBUC *)msg.data;
				if( type == LOG_TYPE_SECU )	TB_log_save_check( LOG_TYPE_SECU );				
				if( type == LOG_TYPE_COMM )	TB_log_save_check( LOG_TYPE_COMM );
			}
		}
	#endif
#endif

		TB_util_delay( 500 );
	}

	TB_log_save_check( LOG_TYPE_SECU_START );
	TB_log_save_check( LOG_TYPE_COMM_START );
}

static pthread_t s_thid_log;
static void tb_log_proc_start( void )
{
	pthread_create( &s_thid_log, NULL, tb_log_proc, NULL );
}

static void tb_log_proc_stop( void )
{
	s_proc_flag_log = 0;
	pthread_join( s_thid_log, NULL );
}

////////////////////////////////////////////////////////////////////////////////

int TB_log_save_check( int type )
{
	int ret = -1;

	TBUC   *p_path  = (type == LOG_TYPE_SECU) ? DEF_LOG_FILE_SYS : DEF_LOG_FILE_COMM;
	TB_LOG *p_log   = (type == LOG_TYPE_SECU) ? &s_log_sys : &s_log_comm;

	if( p_path && p_log )
	{
		//TB_dbg_log("Log Save Check : %s, flag=%d\r\n", p_path, p_log->flag );
		
		if( p_log->flag )
		{
			TB_log_save( p_path, p_log );
			p_log->flag = 0;
			
			ret = 0;
		}
	}

	return ret;
}

int TB_log_save( TBUC *p_path, TB_LOG *p_log )
{
	int ret = -1;

	TB_dbg_log("[%s]\r\n", __FUNCTION__ );

	if( p_path && p_log )
	{
		if( p_log->rb.count > 0 )
		{
			TBUC 	   write_buf[4 + sizeof(TB_LOGITEM) * MAX_LOG_COUNT] = {0, };	//	4 is 'CAFE'
			TB_LOGITEM *p_item = NULL;
			
			int cnt  = 0;
			int idx  = 0;
			int head = p_log->rb.head;
			int tail = p_log->rb.tail;

			bzero( &write_buf[0], sizeof(write_buf) );
			write_buf[idx] = 'C';	idx++;
			write_buf[idx] = 'A';	idx++;
			write_buf[idx] = 'F';	idx++;
			write_buf[idx] = 'E';	idx++;

			while( tail != head )
			{
				if( cnt >= MAX_LOG_COUNT-1 )
					break;

				if( tail == head )
					break;
				
			    if( tail >= p_log->rb.size )
			    {
			        tail = 0;
			    }

				p_item = (TB_LOGITEM *)((unsigned char *)p_log->rb.data + tail);
				if( p_item )
				{
					idx = 4 + (cnt * sizeof(TB_LOGITEM));	//	4 is 'CAFE'
					wmemcpy( &write_buf[idx], sizeof(write_buf)-idx, p_item, sizeof(TB_LOGITEM) );
					cnt ++;
				}

				tail += p_log->rb.item_size;
			}

			////////////////////////////////////////////////////////////////////

			if( cnt > 0 )
			{
				int write_size = 4 + sizeof(TB_LOGITEM) * cnt;
				ret = TB_aes_evp_encrypt_buf2file( write_buf, write_size, p_path );
				if( ret >= write_size )
					ret = 0;
				else
					ret = -1;
			}
			else
			{
				TB_dbg_log( "OK. %s Log count is ZERO\r\n", p_path );
			}
		}
		else
		{
	 		TB_dbg_log( "OK. %s Log count is ZERO\r\n", p_path );
		}
	}
		
	return ret;
}

int TB_log_save_all( void )
{
	int ret1 = TB_log_save( DEF_LOG_FILE_SYS, &s_log_sys );
	int ret2 = TB_log_save( DEF_LOG_FILE_COMM, &s_log_comm );

	return (ret1 == 0 && ret2 == 0) ? 0 : -1;
}

int TB_log_create_default_log_file( TBUC *p_path )
{
	int ret = -1;
	TBUC *p_header = "CAFE";
	size_t size = 0;
	if( p_header )
	{
		if( access(p_path, F_OK) == 0 )
		{
			unlink( p_path );
		}
		
		size = wstrlen( p_header );
		ret = TB_aes_evp_encrypt_buf2file( p_header, size, p_path );
	}

	return ret;
}

int TB_log_load( TBUC *p_path, TB_LOG *p_log )
{
	int ret = -1;

	if( p_path && p_log )
	{
		if( access(p_path, F_OK) != 0 )
		{
			TB_log_create_default_log_file( p_path );
		}

		#define HEADER_SIZE		4

		TBUC read_buf[HEADER_SIZE + sizeof(TB_LOGITEM) * MAX_LOG_COUNT];
		int read_size = TB_aes_evp_decrypt_file2buf( p_path, read_buf, sizeof(read_buf) );
		if( read_size > 0 ) 
		{
			if( read_buf[0] == 'C' && read_buf[1] == 'A' && read_buf[2] == 'F' && read_buf[3] == 'E' )
			{
				TB_LOGITEM *p_item = NULL;
				int count 	= (read_size-HEADER_SIZE) / sizeof(TB_LOGITEM);	//	4 is 'CAFE'
				int size	= sizeof(TB_LOGITEM);
				int idx		= HEADER_SIZE;									//	4 is 'CAFE'
				int i;
				for( i=0; i<count; i++ )
				{
					idx = HEADER_SIZE + i * size;
					p_item = (TB_LOGITEM *)&read_buf[idx];
					if( p_item )
					{
						TB_rb_item_push( &p_log->rb, p_item );
					}
				}

				ret = 0;
			}
			else
			{
				TB_dbg_log( "ERROR. INvalid LOG HEADER : %s\r\n", p_path );
				ret = TB_log_create_default_log_file( p_path );
			}
		}
		else
		{
			TB_dbg_log( "ERROR. LOG File\r : %s\n", p_path );
			ret = TB_log_create_default_log_file( p_path );
		}
	}

	return ret;
}

int TB_log_init( TBBL run_proc )
{
	int ret = -1;

	TB_rb_init( &s_log_sys.rb, (void*)&s_log_sys.log[0], sizeof(s_log_sys.log), sizeof(TB_LOGITEM) );
	TB_log_load( DEF_LOG_FILE_SYS, &s_log_sys );
	
	TB_rb_init( &s_log_comm.rb, (void*)&s_log_comm.log[0], sizeof(s_log_comm.log), sizeof(TB_LOGITEM) );
	TB_log_load( DEF_LOG_FILE_COMM, &s_log_comm );

	if( run_proc )
	{
		tb_log_proc_start();
	}

	ret = 0;

	return ret;
}

int TB_log_deinit( TBBL run_proc )
{
	int ret = -1;

	if( run_proc )
	{
		TB_log_save_all();		
		tb_log_proc_stop();
		ret = 0;
	}
	else
	{
		bzero( &s_log_sys.rb, sizeof(s_log_sys.rb) );
		bzero( &s_log_sys.log[0], sizeof(s_log_sys.log ) );

		bzero( &s_log_comm.rb, sizeof(s_log_comm.rb) );
		bzero( &s_log_comm.log[0], sizeof(s_log_comm.log ) );

		ret = 0;
	}

	return ret;
}

int TB_log_last_working_time_read( time_t *p_last )
{
	int ret = -1;

	if( p_last )
	{
		FILE* fp = fopen( DEF_LOG_WORK_TIME, "r" );
		if( fp )
		{
			TBUC 	read_buf[sizeof(time_t)+1] = {0, };
			size_t 	read_size 	 = 0;
			size_t 	size 		 = sizeof(read_buf) - 1;
			time_t 	last_time 	 = 0 ;
			
			long offset = 0;

			fseek( fp, 0, SEEK_END );
			offset = ftell( fp );
			if( offset >= (long)size )
				offset -= size;
			else
				offset = 0;
			fseek( fp, offset, SEEK_SET );
			
			read_size = fread( read_buf, 1, size, fp );
			if( read_size == size )
			{
				wmemcpy( &last_time, sizeof(last_time), read_buf, read_size );
				if( TB_util_time_check_validation(last_time) == FALSE )
					last_time = time( NULL );

				wmemcpy( p_last, sizeof(time_t), &last_time, sizeof(time_t) );
				ret = 0;
			}

			fclose( fp );
		}
	}

	return ret;	
}

int TB_log_last_working_time_init( void )
{
	int ret = -1;
	time_t last_time;

	if( TB_log_last_working_time_read( &last_time ) == 0 )
	{
		TB_LOGCODE_DATA code_data;
		bzero( &code_data, sizeof(TB_LOGCODE_DATA) );
		
		code_data.type	= LOGCODE_DATETIME;
		code_data.size	= sizeof(last_time);
		wmemcpy( &code_data.data, sizeof(code_data.data), &last_time, code_data.size );
		TB_log_append( LOG_SECU_LAST_WORKING_TIME, &code_data, -1 );

		ret = 0;
	}

	return ret;	
}

int TB_log_last_working_time_save( void )
{
	int ret = -1;

	FILE* fp = fopen( DEF_LOG_WORK_TIME, "a" );
	if( fp )
	{
		/***********************************************************************
		*	Flash Memory의 동일 영역을 지속적으로 Wirte하게 되면,
		*	Flash Memory 수명이 단축되기 때문에 최대 1000회까지 Append하여
		*	Write되는 영역을 분산시킨다.  1000회가 되면 파일 크기를
		*	0으로 만든다. 이때 file write offset이 0이 된다.
		***********************************************************************/
		static int s_count = 0;
		if( s_count >= 1000 )
		{
			ftruncate( fileno(fp), 0 );
			s_count = 0;
		}

		time_t t = time( NULL );

		fseek( fp, 0, SEEK_END );
		if( fwrite( &t, 1, sizeof(t), fp ) == sizeof(t) )
		{
			TB_dbg_log( "Update working time\r\n" );
			
			s_count ++;
			ret = 0;
		}
		else
		{
			TB_dbg_log( "ERROR = %s\r\n", strerror(errno) );
		}
		
		fclose( fp );
	}
	else
	{
		TB_dbg_log( "ERROR = %s\r\n", strerror(errno) );
	}
	
	return ret;
}

TB_LOG	*TB_log_get_log_sys( void )
{
	return &s_log_sys;
}

TB_LOG	*TB_log_get_log_comm( void )
{
	return &s_log_comm;
}

TB_LOG	*TB_log_get_log( int type )
{
	return (type == LOG_TYPE_SECU_START) ? TB_log_get_log_sys() :	\
						  	 		TB_log_get_log_comm();
}


