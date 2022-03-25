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
#include "TB_debug.h"
#include "TB_at.h"

int TB_at_init( void )
{
	int ret = -1;
	
	return ret;
}

TBUS TB_at_read( void )
{
}

TBUS TB_at_write( TBUC *buf, int len )
{
}

void TB_at_deinit( void )
{
}

static void *tb_at_proc( void *arg )
{
}

pthread_t s_thid_at;
void TB_at_proc_start( void )
{
	pthread_create( &s_thid_at, NULL, tb_at_proc, NULL );
}

void TB_at_proc_stop( void )
{
	pthread_join( s_thid_at, NULL );
}


