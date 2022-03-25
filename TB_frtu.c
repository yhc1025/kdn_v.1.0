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
#include "TB_psu.h"
#include "TB_elapse.h"
#include "TB_debug.h"
#include "TB_setup.h"
#include "TB_wisun.h"
#include "TB_modem.h"

#include "TB_frtu.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct tagFRTU_DNP
{
	TBUC buffer[MAX_DNP_BUF];
	TBUS length;
} TB_FRTU_DNP;

////////////////////////////////////////////////////////////////////////////////

static TB_FRTU_DNP	s_frtu_dnp;

////////////////////////////////////////////////////////////////////////////////

int TB_frtu_init( void )
{
	int ret = -1;

	TB_ROLE role = TB_setup_get_role();
	if( (role >= ROLE_TERM1 && role <= ROLE_TERM3) || (role == ROLE_GRPGW ) )
	{
		bzero( &s_frtu_dnp, sizeof(s_frtu_dnp) );
		
		if( TB_uart_frtu_init() == 0 )
		{
			TB_frtu_proc_start();
			ret = 0;
		}
	}
	
	return 0;
}

TBUS TB_frtu_read( void )
{
	s_frtu_dnp.length = TB_uart_frtu_read( s_frtu_dnp.buffer, sizeof(s_frtu_dnp.buffer) );
	return s_frtu_dnp.length;
}

TBUS TB_frtu_write( TBUC *buf, int len )
{
	TBUS ret = 0;

	if( buf && len > 0 )
		ret = TB_uart_frtu_write( buf, len );

	return ret;
}

void TB_frtu_deinit( void )
{
	TB_uart_frtu_deinit();
}

TBUC s_proc_flag_frtu = 1;
static void *tb_frtu_proc( void *arg )
{
	TB_MESSAGE recv_msg;

	printf("===========================================\r\n" );
	printf("               Start FRTU Proc             \r\n" );
	printf("===========================================\r\n" );

	TB_FRTU_DNP	frtu_dnp;
	int check = 0;
	while( s_proc_flag_frtu )
	{
		check = 0;

		while( TB_frtu_read() > 0 )
		{
			if( check == 0 )
				bzero( &frtu_dnp, sizeof(frtu_dnp) );
			
			check = 1;
			wmemcpy( &frtu_dnp.buffer[frtu_dnp.length], sizeof(frtu_dnp.buffer)-frtu_dnp.length, s_frtu_dnp.buffer, s_frtu_dnp.length );
			frtu_dnp.length += s_frtu_dnp.length;
			TB_util_delay( 100 );
		}

		if( check == 1 )
		{
			if( TB_dm_is_on(DMID_FRTU) )
				TB_util_data_dump( "FRTU Read data", frtu_dnp.buffer, frtu_dnp.length );

#if 1
			TB_dbg_frtu("From FRTU : Modem Write\r\n" );
			TB_modem_write( frtu_dnp.buffer, frtu_dnp.length );
#else
			TB_MESSAGE send_msg;
			send_msg.type = MSG_TYPE_MODEM;
			send_msg.id	  = MSG_CMD_MODEM_WRITE;
			send_msg.data = frtu_dnp.buffer;
			send_msg.size = frtu_dnp.length;

			TB_dbg_frtu("Send Message : From FRTU to MODEM. msgid=0x%X\r\n", send_msg.id );
			TB_msgq_send( &send_msg );
#endif
		}

		////////////////////////////////////////////////////////////////////////

		if( TB_msgq_recv( &recv_msg, MSG_TYPE_FRTU, NOWAIT) > 0 )
		{
			switch( recv_msg.id )
			{
				case MSG_CMD_FRTU_WRITE 	:
					TB_dbg_frtu("Recv Message : From MODEM to FRTU. msgid=0x%X\r\n", recv_msg.id );
					if( recv_msg.data && recv_msg.size > 0 )
					{
						if( TB_dm_is_on(DMID_FRTU) )
							TB_util_data_dump( "FRTU Write data", recv_msg.data, recv_msg.size );
						TB_frtu_write( recv_msg.data, recv_msg.size );
					}
					break;

				default	:
					TB_dbg_frtu("Unknown Message ID : %d\r\n", recv_msg.id );
					break;
			}
		}

		TB_util_delay( 500 );
	}
}

pthread_t s_thid_frtu;
void TB_frtu_proc_start( void )
{
	pthread_create( &s_thid_frtu, NULL, tb_frtu_proc, NULL );
}

void TB_frtu_proc_stop( void )
{
	s_proc_flag_frtu = 0;
	pthread_join( s_thid_frtu, NULL );
}

////////////////////////////////////////////////////////////////////////////////
#include "TB_test.h"
pthread_t 	g_thid_frtu_test;
int 		g_proc_flag_frtu_test = 1;
static void *tb_frtu_test_proc( void *arg )
{
	printf("===========================================\r\n" );
	printf("             Start FRTU TEST Proc          \r\n" );
	printf("===========================================\r\n" );

	time_t cur_t, old_t;
    cur_t = old_t = time(NULL);
	
	int check = 0;
	TBUC buf1[32] = "1234567890";
	TBUC buf2[32] = "12345";
	TBUC buf_read[128];
	int normal = 0;
	TBUS len = 0;
	while( g_proc_flag_frtu_test )
	{		
		cur_t = time(NULL);
        if( cur_t != old_t )
		{			
			TBUC buffer[128] = {0, };
			TBUS s = 0;
			TBUC *p	= check == 0 ? buf1 : buf2;
			check = !check;
			
			snprintf( buffer, sizeof(buffer), "[%s] %s\r\n", "FRTU", p );
			s = wstrlen( buffer );
			if( s > 0 )
			{
				len = TB_uart_frtu_write( buffer, s );
				if( len == s )
				{
					TB_dbg_frtu( "FRTU WRITE = %s\r\n", buffer );
					normal |= SUCCESS_WRITE;
				}
			}
			
			old_t = cur_t;
        }

		bzero( buf_read, sizeof(buf_read) );
		len = TB_uart_frtu_read( buf_read, sizeof(buf_read) ) ;
        if( len > 1 )
        {
			TB_dbg_frtu( "FRTU  READ = %s\r\n", buf_read );
			normal |= SUCCESS_READ;			
        }

		if( (normal & (SUCCESS_WRITE|SUCCESS_READ)) == (SUCCESS_WRITE|SUCCESS_READ) )
		{
			TB_testmode_set_test_result( TEST_ITEM_FRTU, TRUE );
		}
		
		TB_util_delay( 2000 );
	}
}

void TB_frtu_test_proc_start( void )
{
	pthread_create( &g_thid_frtu_test, NULL, tb_frtu_test_proc, NULL );
}

void TB_frtu_test_proc_stop( void )
{
	g_proc_flag_frtu_test = 0;
	pthread_join( g_thid_frtu_test, NULL );
}


