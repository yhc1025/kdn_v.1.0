#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

#include "TB_util.h"
#include "TB_log.h"
#include "TB_sys_wdt.h"

////////////////////////////////////////////////////////////////////////////////

static int s_fd_wdt 		= -1;
static int s_wdt_enable		= 0;
static int s_wdt_timeout 	= WDT_TIMEOUT_DEFAULT;
static int s_wdt_reboot 	= 0;
static struct watchdog_info s_wdt_info;

////////////////////////////////////////////////////////////////////////////////

static pthread_t s_thid_wdt;

void *tb_wdt_proc( void *arg )
{
	while( 1 )
	{
		if( s_wdt_enable == 1 )
		{
			TB_wdt_keepalive();
		}

		TB_util_delay( 100 );
	}
}

void TB_wdt_thread( void )
{
	pthread_create( &s_thid_wdt, NULL, tb_wdt_proc, NULL );
}


int TB_wdt_init( int timeout )
{
	if( s_wdt_timeout < WDT_TIMEOUT_MIN || s_wdt_timeout > WDT_TIMEOUT_MAX )
	{
		s_wdt_timeout = WDT_TIMEOUT_DEFAULT;
	}

	s_wdt_timeout = s_wdt_timeout;

	printf( "[%s:%d] Watchdog Timeout : %d sec\r\n", __FILE__, __LINE__, s_wdt_timeout );
	if( s_fd_wdt < 0 )
	{
		s_fd_wdt = open( "/dev/watchdog", O_WRONLY );
		if( s_fd_wdt < 0 )
		{
			printf("[%s:%d] %s : errno:%d, %s\r\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
			return -1;
		}

		int ret = ioctl(s_fd_wdt, WDIOC_GETSUPPORT, &s_wdt_info);
		if( ret != 0 )
		{
			printf("[%s:%d] %s : WDIOC_GETSUPPORT errno:%d, %s\r\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
			TB_wdt_deinit();
			return -1;
		}
	}

	TB_wdt_enable();
	TB_wdt_thread();

	return 0;
}

int TB_wdt_enable( void )
{
	int ret = -1;

#ifdef	ENABLE_WDT
	if( s_fd_wdt < 0 )
	{
		TB_wdt_init( s_wdt_timeout );
	}
	
	if( s_fd_wdt >= 0 )
	{		
		int flags = WDIOS_ENABLECARD;
		ret = ioctl( s_fd_wdt, WDIOC_SETOPTIONS, &flags );
		if( ret )
		{
			printf("[%s:%d] %s : WDIOS_ENABLECARD error '%s'\r\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno) );
			TB_wdt_deinit();
			return ret;
		}

		flags = s_wdt_timeout;
		ret = ioctl( s_fd_wdt, WDIOC_SETTIMEOUT, &flags );
		if( ret )
		{
			printf("[%s:%d] %s : WDIOC_SETTIMEOUT error '%s'\r\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno) );
			TB_wdt_deinit();
			return ret;
		}
		else
		{
			s_wdt_enable = 1;
			printf("[%s:%d] %s \r\n", __FILE__, __LINE__, __FUNCTION__ );
		}		

		ret = 0;
	}
#endif

	return ret;
}

int TB_wdt_disable( void )
{
	int ret = -1;
	if( s_fd_wdt >= 0 )
	{
		int flags = WDIOS_DISABLECARD;
		ret = ioctl( s_fd_wdt, WDIOC_SETOPTIONS, &flags );
		if( ret )
		{
			printf("[%s:%d] %s : WDIOS_ENABLECARD error '%s'\r\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno) );
		}
		else
		{
			s_wdt_enable = 0;
			printf("[%s:%d] %s \r\n", __FILE__, __LINE__, __FUNCTION__ );
		}
	}

	return ret;
}

int TB_wdt_deinit( void )
{
	printf("[%s:%d] %s \r\n", __FILE__, __LINE__, __FUNCTION__ );

	TB_wdt_disable();
	
	if( s_fd_wdt >= 0 )
	{
		close( s_fd_wdt );
		s_fd_wdt = -1;
	}
	
	return 0;
}

int TB_wdt_keepalive( void )
{
	if( s_wdt_reboot == 1 )
		return -1;
	
	if( s_fd_wdt >= 0 )
	{
		int dummy;
		int ret;

		ret = ioctl( s_fd_wdt, WDIOC_KEEPALIVE, &dummy );
		if( ret )
		{
			printf("[%s:%d] %s : WDIOC_KEEPALIVE error '%s'\r\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno) );
		}
	}
	
	return 0;
}

void __TB_wdt_reboot( TB_WDT_COND cond, char *file, int line )
{
	static int s_check_wdt = 0;

	if( s_check_wdt == 0 )
	{
		if( cond != WDT_COND_NO_LOG )
		{
			TB_LOGCODE_DATA code_data;
			bzero( &code_data, sizeof(code_data) );
			code_data.type	 = LOGCODE_DATA_WDT_COND;
			code_data.size	 = sizeof(TB_WDT_COND);
			wmemcpy( &code_data.data, sizeof(code_data.data), &cond, code_data.size );
			TB_log_append( LOG_SECU_WATCHDOG, &code_data, -1 );
		}
		
		TB_log_save_all();

		if( file && line > 0 )
		{
			printf( "---------------------------------------------------------------\r\n" );
			printf( "     Call a WDT reboot from [%s:%d] wdt_cond = %d\r\n", file, line, cond );
			printf( "---------------------------------------------------------------\r\n" );
		}
		
#ifdef ENABLE_WDT
		while( 1 )
		{
			//s_wdt_reboot = 1;
			s_wdt_enable = 0;
			sleep( 1 );
		}
#else
		system( "reboot" );
#endif
		s_check_wdt = 1;
	}
}

