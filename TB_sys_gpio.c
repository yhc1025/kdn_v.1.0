#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>

#include "TB_util.h"
#include "TB_debug.h"
#include "TB_sys_gpio.h"

////////////////////////////////////////////////////////////////////////////////

#define DEV_GPIO "/dev/gpiochip0"

////////////////////////////////////////////////////////////////////////////////

static int s_fd_gpio = -1;
static int s_req_fd[NUM_GPIO_TYPE];
/*
=
{
	-1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1,
	-1, -1, -1, -1
};
*/
static struct gpiochip_info s_gpio_cinfo;
static struct gpioline_info s_gpio_linfo;

////////////////////////////////////////////////////////////////////////////////

/* set gpio_pb2 output */
// 128 gpio in gpiochip0
//  0 ~  31 PA0 -> PA31
// 32 ~  63 PB0 -> PB31
// 63 ~  95 PC0 -> PC31
// 96 ~ 127 PD0 -> PD31
//

static int tb_gpio_get_line_offset( int gpio_type )
{
	switch( gpio_type )
	{
		case GPIO_TYPE_LTEM_LED		:	return 64+21;
		case GPIO_TYPE_WISUN_LED	:	return 64+22;
		case GPIO_TYPE_LTE_PWR_EN	:	return 64+23;
		case GPIO_TYPE_WISUN_RESET2	:	return 64+24;
		case GPIO_TYPE_MAIN_LED		:	return 64+26;
		case GPIO_TYPE_WISUN_RESET1	:	return 64+27;
		case GPIO_TYPE_POWER_LED	:	return 64+28;
		case GPIO_TYPE_485A_RE		:	return 64+29;
		case GPIO_TYPE_485B_RE		:	return 64+30;
		case GPIO_TYPE_DIG_LED		:	return 64+31;
		case GPIO_TYPE_RF_CH1		:	return 96+1;
		case GPIO_TYPE_RF_CH2		:	return 64+25;
		case GPIO_TYPE_RF_CH3		:	return 96+19;
		case GPIO_TYPE_RF_CH4		:	return 96+20;
		default						:	break;
	}
	
	TB_dbg_gpio("out of range.. gpio_type:%d\r\n", gpio_type);
	return 64+31;
}

static const char *tb_gpio_get_line_label( int gpio_type )
{
	switch( gpio_type )
	{
		case GPIO_TYPE_LTEM_LED		:	return "LTEM_LED";
		case GPIO_TYPE_WISUN_LED	:	return "WISUN_LED";
		case GPIO_TYPE_LTE_PWR_EN	:	return "LTE PWR EN";
		case GPIO_TYPE_WISUN_RESET2	:	return "WISUN_RESET2";
		case GPIO_TYPE_MAIN_LED		:	return "TEST MAIN";
		case GPIO_TYPE_WISUN_RESET1	:	return "WISUN_RESET1";
		case GPIO_TYPE_POWER_LED	:	return "POWER_LED";
		case GPIO_TYPE_485A_RE		:	return "485A_RE";
		case GPIO_TYPE_485B_RE		:	return "485B_RE";
		case GPIO_TYPE_DIG_LED		:	return "DIG_LED";
		case GPIO_TYPE_RF_CH1		:	return "RF_CH1";
		case GPIO_TYPE_RF_CH2		:	return "RF_CH2";
		case GPIO_TYPE_RF_CH3		:	return "RF_CH3";
		case GPIO_TYPE_RF_CH4		:	return "RF_CH4";
		default						:	break;
	}
	
	TB_dbg_gpio("out of range.. gpio_type:%d\r\n", gpio_type );
	return "NONE";

}

static int tb_gpio_get_line_high(int gpio_type, TBBL led_on)
{
	switch( gpio_type )
	{
		case GPIO_TYPE_MAIN_LED		:	return !led_on;
		default						:	return led_on;
	}
}

static int tb_gpio_check_port( int gpio_type ) 
{
	if( s_fd_gpio < 0 )
	{
		if( TB_gpio_init() < 0 )
		{
			TB_dbg_gpio("type_type:%d init fail!!\r\n", gpio_type );
			return -1;
		}
	}

	if( s_fd_gpio < 0 )
	{
		TB_dbg_gpio("s_fd_gpio:%d error\r\n", s_fd_gpio);
		return -1;
	}
	
	if( gpio_type < 0 || gpio_type >= NUM_GPIO_TYPE )
	{
		TB_dbg_gpio("type_type:%d out of range!!\r\n", gpio_type );
		return -1;
	}
	
	if( s_req_fd[gpio_type] < 0 )
	{
		struct gpiohandle_request req;
		req.lineoffsets[0] = tb_gpio_get_line_offset(gpio_type);
		bzero( req.consumer_label, sizeof(req.consumer_label) );			
		wstrncpy( req.consumer_label, sizeof(req.consumer_label), tb_gpio_get_line_label(gpio_type), wstrlen(tb_gpio_get_line_label(gpio_type)) );
		req.lines = 1;
		req.flags = GPIOHANDLE_REQUEST_OUTPUT;
		int lhfd = ioctl(s_fd_gpio, GPIO_GET_LINEHANDLE_IOCTL, &req);

		TB_dbg_gpio("gpio_type:%d req.s_fd_gpio:%d offsets:%d label:%s\r\n",gpio_type,			\
																			req.fd,				\
																			req.lineoffsets[0],	\
																			req.consumer_label );

		if( lhfd < 0 )
		{
			TB_dbg_gpio("ERROR get line handle lhdf=%d, %d\r\n", lhfd, gpio_type);
			return -1;
		}
		s_req_fd[gpio_type] = req.fd;
	}
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

static int tb_gpio_set_dir( int gpio_type, int input )
{
	int ret = 0;
	if( tb_gpio_check_port(gpio_type) >= 0 )
	{
		close( s_req_fd[gpio_type] );
		s_req_fd[gpio_type] = -1;
		
		struct gpiohandle_request req;
		req.lineoffsets[0] = tb_gpio_get_line_offset(gpio_type);
		bzero( req.consumer_label, sizeof(req.consumer_label) );
		wstrncpy(req.consumer_label, sizeof(req.consumer_label), tb_gpio_get_line_label(gpio_type), wstrlen(tb_gpio_get_line_label(gpio_type)) );
		req.lines = 1;
		if( input )
			req.flags = GPIOHANDLE_REQUEST_INPUT;
		else
			req.flags = GPIOHANDLE_REQUEST_OUTPUT;		
		
		int lhfd = ioctl(s_fd_gpio, GPIO_GET_LINEHANDLE_IOCTL, &req);
		if( lhfd < 0 )
		{
			printf("[%s:%d] ERROR get line handle lhdf=%d, %d\r\n", __FILE__, __LINE__, lhfd, gpio_type);
			ret = -1;
		}

		s_req_fd[gpio_type] = req.fd;
	}

	return ret;
}

int TB_gpio_get( int gpio_type )
{
	int ret = -1;
	if( tb_gpio_check_port(gpio_type) >= 0 )
	{
		tb_gpio_set_dir( gpio_type, 1 );

		////////////////////////////////////////////////////////////////////////

		struct gpiohandle_data data;
		bzero( &data, sizeof(data) );
		int temp = ioctl( s_req_fd[gpio_type], GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data );

		printf( "*****************************************************\r\n" );
		printf( "     TESTMODE : GPIO Status : value:%d ret:%d\r\n", data.values[0], temp );
		printf( "*****************************************************\r\n" );
		if( temp >= 0 )
		{
			ret = data.values[0];
		}
		else
		{
			printf( "[%s:%d] ERROR get line value ret=%d\r\n", __FILE__, __LINE__, temp );
		}

		////////////////////////////////////////////////////////////////////////

		tb_gpio_set_dir( gpio_type, 0 );
	}

	return ret;
}

int TB_gpio_set( int gpio_type, TBBL high )
{
	int ret = -1;
	if( tb_gpio_check_port(gpio_type) >= 0 )
	{
		struct gpiohandle_data data;
		data.values[0] = high;
		int temp = ioctl(s_req_fd[gpio_type], GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
		TB_dbg_gpio( "gpio_type:%d high:%d ret:%d\r\n", gpio_type, high, temp );
		if( temp >= 0 )
		{
			ret = 0;
		}
		else
		{
			TB_dbg_gpio( "ERROR set line value ret=%d\r\n", temp );
		}
	}

	return ret;
}

int TB_gpio_led( int gpio_type, TBBL led_on )
{
	int ret = -1;
	if( tb_gpio_check_port(gpio_type) >= 0 )
	{
		struct gpiohandle_data data;
		data.values[0] = tb_gpio_get_line_high(gpio_type, led_on);
		int temp = ioctl(s_req_fd[gpio_type], GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
		if( temp >= 0 )
		{
			ret = 0;
		}
		else
		{
			TB_dbg_gpio( "ERROR set line value ret=%d\r\n", ret );
		}
	}
	
	return ret;
}

int TB_gpio_init( void )
{
	int ret = -1;

	wmemset( s_req_fd, sizeof(s_req_fd), 0xFF, sizeof(s_req_fd) );

	if( s_fd_gpio < 0 )
	{	
		/* open gpio */
		s_fd_gpio = open( DEV_GPIO, 0 );
		if( s_fd_gpio >= 0 )
		{
			/* get gpio chip info */
			int temp = ioctl( s_fd_gpio, GPIO_GET_CHIPINFO_IOCTL, &s_gpio_cinfo );
			if( temp >= 0 )
			{
				TB_dbg_gpio("GPIO chip: %s, \"%s\", %u GPIO lines\r\n", s_gpio_cinfo.name, s_gpio_cinfo.label, s_gpio_cinfo.lines );
				temp = ioctl( s_fd_gpio, GPIO_GET_LINEINFO_IOCTL, &s_gpio_linfo );
				if( temp >= 0 )
				{
					ret = 0;
				}
				else
				{
					TB_dbg_gpio( "ERROR get line info ret=%d\r\n", temp );
				}
				TB_dbg_gpio( "line %2d: %s\r\n", s_gpio_linfo.line_offset, s_gpio_linfo.name );
			}
			else
			{
				TB_dbg_gpio("ERROR get chip info ret=%d\r\n", temp );
			}
		}
		else
		{
			TB_dbg_gpio("ERROR: open %s ret=%d\r\n", DEV_GPIO, s_fd_gpio );
		}

	}

	return ret;

}

void TB_gpio_deinit(void)
{
	if( s_fd_gpio >= 0 )
	{
		close( s_fd_gpio );
		s_fd_gpio = -1;
	}
}

