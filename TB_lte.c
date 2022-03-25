#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "TB_util.h"
#include "TB_uart.h"
#include "TB_sys_gpio.h"
#include "TB_msg_queue.h"
#include "TB_modbus.h"
#include "TB_debug.h"
#include "TB_wisun.h"
#include "TB_lte.h"

#define MAX_LTE_BUF		(1024)

////////////////////////////////////////////////////////////////////////////////

typedef struct tagLTE_BUF
{
	TBUC buffer[MAX_LTE_BUF];
	TBUS length;
} TB_LTE_BUF;

TB_LTE_BUF	s_buffer_lte;

////////////////////////////////////////////////////////////////////////////////

int TB_lte_init( void )
{
	int ret = -1;
	
	if( TB_uart_lte_init() == 0 )
	{
		TB_ROLE role = TB_setup_get_role();
		if( role >= ROLE_TERM_MIN && role <= ROLE_TERM_MAX )
		{
			TBSI master = TB_setup_get_comm_type_master();
			if( master == COMM_MODE_MASTER_LTE )
			{
				TB_lte_proc_start();
				ret = 0;
			}
		}
		else
		{
			ret = -1;
		}
	}
	
	return ret;
}

TBUS TB_lte_read( void )
{
	s_buffer_lte.length = TB_uart_lte_read( s_buffer_lte.buffer, MAX_LTE_BUF );
	return s_buffer_lte.length;
}

TBUS TB_lte_write( TBUC *buf, int len )
{
	TBUS ret = 0;

	if( buf && len > 0 )
		ret = TB_uart_lte_write( buf, len );

	return ret;
}

void TB_lte_deinit( void )
{
	TB_uart_lte_deinit();
}

TBUC s_proc_flag_lte = 1;
static void *tb_lte_proc( void *arg )
{
	TB_MESSAGE msg;
	TB_ROLE role = TB_setup_get_role();

	printf("===========================================\r\n" );
	printf("               Start LTE Proc              \r\n" );
	printf("===========================================\r\n" );

	while( s_proc_flag_lte )
	{
		if( TB_lte_read() > 0 )
		{
			if( TB_dm_is_on(DMID_LTE) )
				TB_util_data_dump( "LTE Read data", s_buffer_lte.buffer, s_buffer_lte.length );
			
			if( role >= ROLE_TERM_MIN && role <= ROLE_TERM_MAX )
			{
				msg.type 	= MSG_TYPE_PSU_DNP;
				msg.id		= MSG_CMD_PSU_DNP_WRITE;
				msg.data	= s_buffer_lte.buffer;
				msg.size	= s_buffer_lte.length;

				TB_dbg_modem("Send Message : From LTE to Terminal. msgid=0x%X\r\n", msg.id );
				TB_msgq_send( &msg );
			}
		}

		////////////////////////////////////////////////////////////////////////
		
		if( TB_msgq_recv( &msg, MSG_TYPE_LTE, NOWAIT) > 0 )
		{
			switch( msg.id )
			{
				case MSG_CMD_LTE_WRITE 	:
					if( msg.data && msg.size > 0 )
					{
						if( TB_dm_is_on(DMID_LTE) )
							TB_util_data_dump( "LTE Write data", msg.data, msg.size );
						TB_lte_write( msg.data, msg.size );
					}
					break;
			}
		}

		TB_util_delay( 500 );
	}
}

pthread_t s_thid_lte;
void TB_lte_proc_start( void )
{
	pthread_create( &s_thid_lte, NULL, tb_lte_proc, NULL );
}

void TB_lte_proc_stop( void )
{
	s_proc_flag_lte = 0;
	pthread_join( s_thid_lte, NULL );
}

