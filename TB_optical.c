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
#include "TB_optical.h"

#define MAX_OPTICAL_BUF		(128)

////////////////////////////////////////////////////////////////////////////////

typedef struct tagOPTICAL_BUF
{
	TBUC buffer[MAX_OPTICAL_BUF];
	TBUS length;
} TB_OPTICAL_BUF;

TB_OPTICAL_BUF	s_buf_optical;

extern TB_OP_UART g_op_uart_invtb;

////////////////////////////////////////////////////////////////////////////////

int TB_optical_init( void )
{
	int ret = -1;
	
	if( g_op_uart_invtb.init() == 0 )
	{
		TB_optical_proc_start();
		ret = 0;
	}
	else
	{
		ret = -1;
	}
	
	return ret;
}

TBUS TB_optical_read( void )
{
	s_buf_optical.length = g_op_uart_invtb.read( s_buf_optical.buffer, MAX_OPTICAL_BUF );
	return s_buf_optical.length;
}

TBUS TB_optical_write( TBUC *buf, int len )
{
	TBUS ret = 0;

	if( buf && len > 0 )
		ret = g_op_uart_invtb.write( buf, len );

	return ret;
}

void TB_optical_deinit( void )
{
	g_op_uart_invtb.deinit();
}

TBUC s_proc_flag_optical = 1;
static void *tb_optical_proc( void *arg )
{
	TB_MESSAGE msg;
	TB_ROLE role = TB_setup_get_role();

	printf("===========================================\r\n" );
	printf("               Start OPTICAL Proc          \r\n" );
	printf("===========================================\r\n" );

	while( s_proc_flag_optical )
	{
		if( TB_optical_read() > 0 )
		{
			if( TB_dm_is_on(DMID_OPTICAL) )
				TB_util_data_dump( "OPTICAL Read data", s_buf_optical.buffer, s_buf_optical.length );

			if( TB_crc16_modbus_check( s_buf_optical.buffer, s_buf_optical.length ) == TRUE )
			{
				msg.type 	= MSG_TYPE_PSU_MOD;
				msg.id		= MSG_CMD_PSU_MOD_WRITE;
				msg.data	= s_buf_optical.buffer;
				msg.size	= s_buf_optical.length;

				TB_dbg_optical("Send Message : From OPTICAL to PSU_MOD. msgid=0x%X\r\n", msg.id );
				TB_msgq_send( &msg );
			}
			else
			{
				TB_dbg_optical("ERROR. Read data CRC\r\n" );
			}
		}

		////////////////////////////////////////////////////////////////////////
		
		if( TB_msgq_recv( &msg, MSG_TYPE_OPTICAL, NOWAIT) > 0 )
		{
			switch( msg.id )
			{
				case MSG_CMD_OPTICAL_WRITE 	:
					if( msg.data && msg.size > 0 )
					{
						if( TB_dm_is_on(DMID_OPTICAL) )
							TB_util_data_dump( "OPTICAL Write data", msg.data, msg.size );

						if( TB_crc16_modbus_check( msg.data, msg.size ) == TRUE )
							TB_optical_write( msg.data, msg.size );
						else
							TB_dbg_optical("ERROR. Recv data CRC\r\n" );
					}
					break;
			}
		}

		TB_util_delay( 500 );
	}
}

pthread_t s_thid_optical;
void TB_optical_proc_start( void )
{
	pthread_create( &s_thid_optical, NULL, tb_optical_proc, NULL );
}

void TB_optical_proc_stop( void )
{
	s_proc_flag_optical = 0;
	pthread_join( s_thid_optical, NULL );
}

