#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/types.h>

#include "TB_uart.h"
#include "TB_util.h"
#include "TB_led.h"
#include "TB_sys_uart.h"
#include "TB_sys_gpio.h"
#include "TB_debug.h"

////////////////////////////////////////////////////////////////////////////////

#define ENABLE_UART_PSU_MOD_ASYNC

////////////////////////////////////////////////////////////////////////////////

static UART_T *s_uart_wisun_t 	= NULL;
static UART_T *s_uart_invta_t 	= NULL;
static UART_T *s_uart_invtb_t 	= NULL;
static UART_T *s_uart_mdm_t 	= NULL;
static UART_T *s_uart_lte_t 	= NULL;
static UART_T *s_uart_mod_t 	= NULL;
static UART_T *s_uart_dnp_t 	= NULL;
static UART_T *s_uart_ems_t 	= NULL;
static UART_T *s_uart_frtu_t 	= NULL;

static TBUC s_uart_wisun_rx_status = 0;
static TBUC s_uart_wisun_tx_status = 0;

////////////////////////////////////////////////////////////////////////////////

#define WISUN_STATUS_READ	0x01
#define WISUN_STATUS_WRITE	0x02

TBUS TB_uart_wisun_1st_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_wisun_t )
	{
#if 1	//	20211129
		fd_set reads, temps;

		FD_ZERO( &reads );
		FD_SET( s_uart_wisun_t->fd, &reads );

		temps = reads;
		int result = select( FD_SETSIZE, &temps, NULL, NULL, NULL );
        if( result >= 0 )
        {
	        if( FD_ISSET(s_uart_wisun_t->fd, &temps) )
	        {
	            bzero( p_buffer, buf_size );
	            int r = uart_read( s_uart_wisun_t, p_buffer, buf_size );
				if( r > 0 )
				{
					ret = r;
					TB_dbg_uart("[%s] UART read size = %d\r\n", __FUNCTION__, ret);
				}	            
	        }
        }
		else
		{
			TB_dbg_uart("[%s] ERROR. UART select = %d\r\n", __FUNCTION__, result );
		}
#else
		int r = uart_read( s_uart_wisun_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s]UART read size = %d\r\n", __FUNCTION__, ret);
		}
#endif
	}

	s_uart_wisun_rx_status = (ret > 0) ? WISUN_STATUS_READ : 0;

	return ret;
}

TBUS TB_uart_wisun_1st_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_wisun_t )
	{
		int r = uart_write( s_uart_wisun_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s]UART write size = %d, fd=%d\r\n", __FUNCTION__, ret, s_uart_wisun_t->fd );
		}
	}

	s_uart_wisun_tx_status = (ret > 0) ? WISUN_STATUS_WRITE : 0;
	
	return ret;
}

int TB_uart_wisun_1st_init( void )
{
	int ret = -1;

	if( s_uart_wisun_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_WISUN );
		if( uart )
		{
			s_uart_wisun_t = uart;
			TB_dbg_uart("OK. init UART for WISUN : 0x%X\r\n", s_uart_wisun_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for WISUN\r\n" );
		}
	}

	return ret;
}

void TB_uart_wisun_1st_deinit( void )
{
	if( s_uart_wisun_t )
	{
		uart_delete( s_uart_wisun_t );
		s_uart_wisun_t = NULL;
	}
}

TBUC TB_uart_wisun_1st_get_status( void )
{
	return (s_uart_wisun_rx_status + s_uart_wisun_tx_status);
}

////////////////////////////////////////////////////////////////////////////////

TBUS TB_uart_rs485a_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_invta_t )
	{
		int r = uart_read( s_uart_invta_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s] UART read size = %d\r\n", __FUNCTION__, ret);
		}
	}

	return ret;
}

TBUS TB_uart_rs485a_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_invta_t )
	{
		int r = uart_write( s_uart_invta_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s] UART write size = %d\r\n", __FUNCTION__, ret);
		}
		else
		{
			printf("[%s] ERROR. Inverter Write : %d\n", __FUNCTION__, r );
		}
	}

	return ret;
}

int TB_uart_rs485a_init( void )
{
	int ret = -1;

	if( s_uart_invta_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_RS485A );
		if( uart )
		{
			s_uart_invta_t = uart;
			TB_dbg_uart("OK. init UART for RS485A : 0x%X\r\n", s_uart_invta_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for RS485A\r\n" );
		}
	}

	return ret;
}

void TB_uart_rs485a_deinit( void )
{
	if( s_uart_invta_t )
	{
		uart_delete( s_uart_invta_t );
		s_uart_invta_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUS TB_uart_rs485b_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_invtb_t )
	{
		int r = uart_read( s_uart_invtb_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s]UART read size = %d\r\n", __FUNCTION__, ret);
		}
	}

	return ret;
}

TBUS TB_uart_rs485b_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_invtb_t )
	{
		int r = uart_write( s_uart_invtb_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s]UART write size = %d\r\n", __FUNCTION__, ret);
		}
		else
		{
			printf("[%s] ERROR. Inverter Write : %d\n", __FUNCTION__, r );
		}
	}

	return ret;
}

int TB_uart_rs485b_init( void )
{
	int ret = -1;

	if( s_uart_invtb_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_RS485B );
		if( uart )
		{
			s_uart_invtb_t = uart;
			TB_dbg_uart("OK. init UART for RS485B : 0x%X\r\n", s_uart_invtb_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for RS485B\r\n" );
		}
	}

	return ret;
}

void TB_uart_rs485b_deinit( void )
{
	if( s_uart_invtb_t )
	{
		uart_delete( s_uart_invtb_t );
		s_uart_invtb_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUS TB_uart_mdm_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_mdm_t )
	{
		int r = uart_read( s_uart_mdm_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart( "[%s] UART read size = %d\r\n", __FUNCTION__, ret );
		}
	}

	return ret;
}

TBUS TB_uart_mdm_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_mdm_t )
	{
		int r = uart_write( s_uart_mdm_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s]UART write size = %d\r\n", __FUNCTION__, ret);
		}
	}

	return ret;
}

int TB_uart_mdm_init( void )
{
	int ret = -1;

	if( s_uart_mdm_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_MODEM );
		if( uart )
		{
			s_uart_mdm_t = uart;
			TB_dbg_uart("OK. init UART for Full Modem : 0x%X\r\n", s_uart_mdm_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for Full Modem\r\n" );
		}
	}

	return ret;
}

void TB_uart_mdm_deinit( void )
{
	if( s_uart_mdm_t )
	{
		uart_delete( s_uart_mdm_t );
		s_uart_mdm_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUS TB_uart_lte_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_lte_t )
	{
#if 1	//	20211129
		fd_set reads, temps;

		FD_ZERO( &reads );
		FD_SET( s_uart_lte_t->fd, &reads );

		temps = reads;
		int result = select( FD_SETSIZE, &temps, NULL, NULL, NULL );
        if( result >= 0 )
        {
	        if( FD_ISSET(s_uart_lte_t->fd, &temps) )
	        {
	            bzero( p_buffer, buf_size );
	            int r = uart_read( s_uart_lte_t, p_buffer, buf_size );
				if( r > 0 )
				{
					ret = r;
					TB_dbg_uart("[%s] UART read size = %d\r\n", __FUNCTION__, ret);
				}	            
	        }
        }
		else
		{
			TB_dbg_uart("[%s] ERROR. UART select = %d\r\n", __FUNCTION__, result );
		}
#else		
		int r = uart_read( s_uart_lte_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s]UART read size = %d\r\n", __FUNCTION__, ret);
		}
#endif
	}

	s_uart_wisun_rx_status = (ret > 0) ? WISUN_STATUS_READ : 0;

	return ret;
}

TBUS TB_uart_lte_write( void *p_data, int size )
{
	TBUS ret = 0;

	if( s_uart_lte_t )
	{
		int r = uart_write( s_uart_lte_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("UART write size = %d\r\n", ret);
		}
	}

	return ret;
}

int TB_uart_lte_init( void )
{
	int ret = -1;

	if( s_uart_lte_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_LTEM );
		if( uart )
		{
			s_uart_lte_t = uart;
			TB_dbg_uart("OK. init UART for LTE : 0x%X\r\n", s_uart_lte_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for LTE\r\n" );
		}
	}

	return ret;
}

void TB_uart_lte_deinit( void )
{
	if( s_uart_lte_t )
	{
		uart_delete( s_uart_lte_t );
		s_uart_lte_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

volatile int g_signal_in_psu_mod = FALSE;
volatile int g_signal_in_psu_dnp = FALSE;
void signal_handler_psu_modbus( int status )
{
//	printf("---> UART SIGNAL IN PSU_MODBUS -------------------------\n" );
	g_signal_in_psu_mod = TRUE;
}

void signal_handler_psu_dnp( int status )
{
//	printf("---> UART SIGNAL IN PSU_DNP-------------------------\n" );
	g_signal_in_psu_dnp = TRUE;
}

TBUS TB_uart_psu_mod_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_mod_t )
	{
		extern volatile int g_signal_in_psu_mod;
		while( g_signal_in_psu_mod )
		{
			/*******************************************************************
			*	SIGNAL 입력 후 잔여 데이터가 모두 들어올 때까지 기다린다.
			*******************************************************************/
			TB_util_delay( 300 );
			int r = uart_read( s_uart_mod_t, p_buffer, buf_size );
			if( r > 0 )
			{
				ret = r;
				TB_dbg_uart("[%s]UART read size = %d\r\n", __FUNCTION__, ret );
			}

			g_signal_in_psu_mod = FALSE;
		}
	}

	return ret;
}

TBUS TB_uart_psu_mod_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_mod_t )
	{
		int r = uart_write( s_uart_mod_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart("[%s] UART write size = %d\r\n", __FUNCTION__, ret);
		}
		else
		{
			printf( "[%s] ERROR. r = %d\n", __FUNCTION__, r );
		}
	}

	return ret;
}

int TB_uart_psu_mod_init( void )
{
	int ret = -1;

	if( s_uart_mod_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_PSU_MOD );
		if( uart )
		{
			s_uart_mod_t = uart;

			////////////////////////////////////////////////////////////////////

			struct sigaction saio;

			saio.sa_handler = signal_handler_psu_modbus;
			sigemptyset( &saio.sa_mask );
			sigaddset( &saio.sa_mask, 0 );
			saio.sa_flags = 0;
			saio.sa_restorer = NULL;
			sigaction( SIGIO, &saio, NULL );

			fcntl( s_uart_mod_t->fd, F_SETOWN, getpid() );
			fcntl( s_uart_mod_t->fd, F_SETFL, FASYNC );

			TB_dbg_uart("OK. init UART for PSU-MODBUS : 0x%X\r\n", s_uart_mod_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for PSU-MODBUS\r\n" );
		}
	}

	return ret;
}

void TB_uart_psu_mod_deinit( void )
{
	if( s_uart_mod_t )
	{
		uart_delete( s_uart_mod_t );
		s_uart_mod_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUS TB_uart_psu_dnp_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_dnp_t )
	{
		int r = uart_read( s_uart_dnp_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart( "[%s] UART read size = %d\r\n", __FUNCTION__, ret );
		}
	}

	return ret;
}

TBUS TB_uart_psu_dnp_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_dnp_t )
	{
		int r = uart_write( s_uart_dnp_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart( "[%s] PSU-DNP Write size = %d\r\n", __FUNCTION__, ret );
		}
	}

	return ret;
}

int TB_uart_psu_dnp_init( void )
{
	int ret = -1;

	if( s_uart_dnp_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_PSU_DNP );
		if( uart )
		{
			s_uart_dnp_t = uart;

			TB_dbg_uart("OK. init UART for PSU-DNP : 0x%X\r\n", s_uart_dnp_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for PSU-DNP\r\n" );
		}
	}

	return ret;
}

void TB_uart_psu_dnp_deinit( void )
{
	if( s_uart_dnp_t )
	{
		uart_delete( s_uart_dnp_t );
		s_uart_dnp_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUS TB_uart_ems_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;

#if 1
	struct timeval timeout;
	fd_set reads, temps;

	timeout.tv_sec = 0;
    timeout.tv_usec = 1000 * 100;	//	100msec

	FD_ZERO( &reads );
	FD_SET( s_uart_ems_t->fd, &reads );

	temps = reads;
	int result = select( FD_SETSIZE, &temps, NULL, NULL, &timeout );
    if( result >= 0 )
    {
        if( FD_ISSET(s_uart_ems_t->fd, &temps) )
        {
            bzero( p_buffer, buf_size );
            int r = uart_read( s_uart_ems_t, p_buffer, buf_size );
			if( r > 0 )
			{
				ret = r;
				TB_dbg_uart("[%s] UART read size = %d\r\n", __FUNCTION__, ret);
			}	            
        }
    }
	else
	{
		TB_dbg_uart("[%s] ERROR. UART select = %d\r\n", __FUNCTION__, result );
	}
#else
	if( s_uart_ems_t )
	{
		int r = uart_read( s_uart_ems_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart( "[%s] EMS UART read size = %d\r\n", __FUNCTION__, ret );
		}
	}
#endif

	return ret;
}

TBUS TB_uart_ems_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_ems_t )
	{
		int r = uart_write( s_uart_ems_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart( "[%s] EMS UART Write size = %d\r\n", __FUNCTION__, ret );
		}
	}

	return ret;
}

int TB_uart_ems_init( void )
{
	int ret = -1;

	if( s_uart_ems_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_EMS );
		if( uart )
		{
			s_uart_ems_t = uart;

			TB_dbg_uart("OK. init UART for EMS : 0x%X\r\n", s_uart_ems_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for EMS\r\n" );
		}
	}

	return ret;
}

void TB_uart_ems_deinit( void )
{
	if( s_uart_ems_t )
	{
		uart_delete( s_uart_ems_t );
		s_uart_ems_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUS TB_uart_frtu_read( TBUC *p_buffer, TBUI buf_size )
{
	TBUS ret = 0;
	
	if( s_uart_frtu_t )
	{
		int r = uart_read( s_uart_frtu_t, p_buffer, buf_size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart( "[%s] FRTU UART read size = %d\r\n", __FUNCTION__, ret );
		}
	}

	return ret;
}

TBUS TB_uart_frtu_write( void *p_data, int size )
{
	TBUS ret = 0;
	
	if( s_uart_frtu_t )
	{
		int r = uart_write( s_uart_frtu_t, p_data, size );
		if( r > 0 )
		{
			ret = r;
			TB_dbg_uart( "[%s] FRTU UART Write size = %d\r\n", __FUNCTION__, ret );
		}
	}

	return ret;
}

int TB_uart_frtu_init( void )
{
	int ret = -1;

	if( s_uart_frtu_t == NULL )
	{
		UART_T *uart = uart_new( UART_TYPE_FRTU );
		if( uart )
		{
			s_uart_frtu_t = uart;

			TB_dbg_uart("OK. init UART for FRTU : 0x%X\r\n", s_uart_frtu_t );
			ret = 0;
		}
		else
		{
			TB_dbg_uart("ERROR. init UART for FRTU\r\n" );
		}
	}

	return ret;
}

void TB_uart_frtu_deinit( void )
{
	if( s_uart_frtu_t )
	{
		uart_delete( s_uart_frtu_t );
		s_uart_frtu_t = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

#if 1
TB_OP_UART g_op_uart_wisun =
{
	.name			= "uart_wisun",
	.init			= TB_uart_wisun_1st_init,
	.deinit			= TB_uart_wisun_1st_deinit,
	.read			= TB_uart_wisun_1st_read,
	.write			= TB_uart_wisun_1st_write,
};

TB_OP_UART g_op_uart_invta =
{
	.name			= "uart_rs485a",
	.init			= TB_uart_rs485a_init,
	.deinit			= TB_uart_rs485a_deinit,
	.read			= TB_uart_rs485a_read,
	.write			= TB_uart_rs485a_write,
};

TB_OP_UART g_op_uart_invtb =
{
	.name			= "uart_rs485b",
	.init			= TB_uart_rs485b_init,
	.deinit			= TB_uart_rs485b_deinit,
	.read			= TB_uart_rs485b_read,
	.write			= TB_uart_rs485b_write,
};

TB_OP_UART g_op_uart_mod =
{
	.name			= "uart_psu_modbus",

	.init			= TB_uart_psu_mod_init,
	.deinit			= TB_uart_psu_mod_deinit,
	.read			= TB_uart_psu_mod_read,
	.write			= TB_uart_psu_mod_write,
};

TB_OP_UART g_op_uart_dnp =
{
	.name			= "uart_psu_dnp",

	.init			= TB_uart_psu_dnp_init,
	.deinit			= TB_uart_psu_dnp_deinit,
	.read			= TB_uart_psu_dnp_read,
	.write			= TB_uart_psu_dnp_write,
};

TB_OP_UART g_op_uart_mdm =
{
	.name			= "uart_full_modem",

	.init			= TB_uart_mdm_init,
	.deinit			= TB_uart_mdm_deinit,
	.read			= TB_uart_mdm_read,
	.write			= TB_uart_mdm_write,
};

TB_OP_UART g_op_uart_lte =
{
	.name			= "uart_lte",

	.init			= TB_uart_lte_init,
	.deinit			= TB_uart_lte_deinit,
	.read			= TB_uart_lte_read,
	.write			= TB_uart_lte_write,
};

TB_OP_UART g_op_uart_ems =
{
	.name			= "uart_ems",

	.init			= TB_uart_ems_init,
	.deinit			= TB_uart_ems_deinit,
	.read			= TB_uart_ems_read,
	.write			= TB_uart_ems_write,
};

TB_OP_UART g_op_uart_frtu =
{
	.name			= "uart_frtu",

	.init			= TB_uart_frtu_init,
	.deinit			= TB_uart_frtu_deinit,
	.read			= TB_uart_frtu_read,
	.write			= TB_uart_frtu_write,
};

#endif

