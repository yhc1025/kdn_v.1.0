#include "TB_sys_gpio.h"
#include "TB_sys_uart.h"
#include "TB_debug.h"
#include "TB_util.h"
#include "TB_setup.h"

////////////////////////////////////////////////////////////////////////////////

#define SIZE_UART_STR	128

////////////////////////////////////////////////////////////////////////////////

//#define DEBUG_UART

////////////////////////////////////////////////////////////////////////////////

static pthread_mutex_t	uart_mutex  = PTHREAD_MUTEX_INITIALIZER;
int uart_lock(void)
{
	return pthread_mutex_lock(&uart_mutex);
}

int uart_unlock(void)
{
	return pthread_mutex_unlock(&uart_mutex);
}

#define	DIR_USB_SERIAL		"/sys/bus/usb-serial/devices"

////////////////////////////////////////////////////////////////////////////////

UART_T *uart_new( int uart_type )
{
	UART_T *uart = NULL;
	TB_dbg_uart("uart_type = %d\r\n", uart_type);
	if( uart_type < 0 || uart_type >= UART_TYPE_MAX )
	{
		TB_dbg_uart("ERROR. Unknown Uart Type = %d\r\n", uart_type );
		return NULL;
	}

	size_t alloc_size = sizeof( UART_T );
	uart = (UART_T *)malloc( alloc_size );
	if( uart == NULL )
	{
		TB_dbg_uart("uart_new malloc failed!! type:%d\r\n", uart_type);
		return NULL;
	}
	
	bzero( uart, alloc_size );

	uart->type 	= uart_type;
	uart->fd 	= -1;
	uart->baud 	= -1;
	uart->rs485 = 0;
	uart->gpio 	= -1;
	uart->size 	= sizeof(UART_T);

	char szdev[SIZE_UART_STR] = {0, };
	int baud = -1;

	switch( uart->type )
	{
		case UART_TYPE_PSU_DNP:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyS1", wstrlen("/dev/ttyS1") );
			baud = TB_setup_get_baud_frtu();
			break;
		case UART_TYPE_WISUN:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyS2", wstrlen("/dev/ttyS2") );
			baud = UART_BAUD_115200;
			break;
		case UART_TYPE_RS485A:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyS3", wstrlen("/dev/ttyS3") );
			baud = TB_setup_get_baud_inverter();
			uart->rs485 = 1;
			uart->gpio = GPIO_TYPE_485A_RE;
			TB_gpio_set( uart->gpio, 1 );
			break;
		case UART_TYPE_RS485B:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyS4", wstrlen("/dev/ttyS4") );
			baud = TB_setup_get_baud_inverter();
			uart->rs485 = 1;
			uart->gpio = GPIO_TYPE_485B_RE;
			TB_gpio_set( uart->gpio, 1 );
			break;
		case UART_TYPE_LTEM:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyS5", wstrlen("/dev/ttyS5") );
			baud = UART_BAUD_115200;
			break;
		////////////////////////////////////////////////////////////////////////
		case UART_TYPE_MODEM:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyUSB0", wstrlen("/dev/ttyUSB0") );
			baud = UART_BAUD_9600;
			break;
		case UART_TYPE_PSU_MOD:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyUSB1", wstrlen("/dev/ttyUSB1") );
			baud = TB_setup_get_baud_frtu();
			break;
		case UART_TYPE_FRTU:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyUSB2", wstrlen("/dev/ttyUSB2") );
			baud = UART_BAUD_9600;
			break;
		case UART_TYPE_EMS:
			wstrncpy(szdev, sizeof(szdev),  "/dev/ttyUSB3", wstrlen("/dev/ttyUSB3") );
			baud = UART_BAUD_9600;
			break;
		default:
			free( uart );
			return NULL;
	}
	TB_dbg_uart("[%s:%d] UART path = %s\r\n", __FILE__, __LINE__, szdev );

	uart->fd = open( szdev, O_RDWR | O_NOCTTY | O_NONBLOCK );
	TB_dbg_uart("uart:%p szdev:%s fd:%d rs485:%d gpio:%d\r\n", uart, szdev, uart->fd, uart->rs485, uart->gpio);
	if( uart->fd >= 0 )
	{  
		fcntl( uart->fd, F_SETFL, 0 );
		tcgetattr( uart->fd, &uart->tio );
	}
	
	if( baud >= 0 )
	{
		uart_set( uart, baud, uart->type );
	}
	
	return uart;
}

void uart_delete( UART_T *uart )
{
	TB_dbg_uart("uart_delete uart:%p\r\n", uart);
	if( uart )
	{
		if( uart->fd >= 0 )
		{
			tcsetattr( uart->fd, TCSANOW, &uart->tio );
			close( uart->fd );
			uart->fd = -1;
		}
		free( uart );
	}
}

int uart_set(UART_T *uart, int baud, int type)
{
	if( uart == NULL )
	{
		TB_dbg_uart("uart is NULL\r\n");
		return -1;
	}
	
	if( uart->size != sizeof(UART_T) )
	{
		TB_dbg_uart("size mismatch!!\r\n");
		return -1;
	}

	TB_dbg_uart("type:%d baud:%d\r\n", uart->type, baud);

	if( uart->fd < 0 )
		return -1;

	if( uart->baud == baud )
	{
		TB_dbg_uart("baud is same!!\r\n", baud);
		return 0;
	}
	uart->baud = baud;

	struct termios newtio;
	bzero( &newtio, sizeof(newtio) );

	switch( uart->baud )
	{
		case UART_BAUD_115200	:	newtio.c_cflag = B115200;
									uart->us_per_bit = 1000000/115200;
									break;
		case UART_BAUD_38400	:	newtio.c_cflag = B38400;
									uart->us_per_bit = 1000000/38400;
									break;
		case UART_BAUD_19200	:	newtio.c_cflag = B19200;
									uart->us_per_bit = 1000000/19200;
									break;
		case UART_BAUD_9600		:	newtio.c_cflag = B9600;
									uart->us_per_bit = 1000000/9600;
									break;
		case UART_BAUD_4800		:	newtio.c_cflag = B4800;
									uart->us_per_bit = 1000000/4800;
									break;
		default					:	newtio.c_cflag = B9600;
									uart->baud = UART_BAUD_9600;
									uart->us_per_bit = 1000000/9600;
									break;
	}

	newtio.c_cflag |= CLOCAL|CREAD|CS8;
#if 0
	if( type == UART_TYPE_MODEM || type == UART_TYPE_EMS || type == UART_TYPE_FRTU )
		newtio.c_cflag |= CRTSCTS;
#endif

	newtio.c_iflag  = 0;//IGNPAR; //ISTRIP|IGNPAR|IGNBRK; //|ICRNL;
	newtio.c_oflag  = 0;//|= OPOST;
	newtio.c_lflag  = 0;//ICANON;

	tcflush( uart->fd, TCIOFLUSH );
	tcsetattr( uart->fd, TCSANOW, &newtio );
	return 0;
}

int uart_write( UART_T *uart, void *p_data, int len )
{
	if( uart == NULL || uart->size != sizeof(UART_T) )
	{
		TB_dbg_uart("uart_write uart is invalid!!\r\n");
		return -1;
	}

	if( uart->fd < 0 )
	{
		TB_dbg_uart( "uart fd is invalid!! : %d\r\n", uart->fd );
		return -1;
	}

	TB_dbg_uart("uart_write fd:%d type:%d len:%d rs485:%d\r\n", uart->fd, uart->type, len, uart->rs485);

	if( uart->rs485 && uart->gpio >= 0 )
		TB_gpio_set( uart->gpio, 0 );

	int r = write( uart->fd, p_data, len );
	if( r < 0 )
	{
		TB_dbg_uart( "ERROR = %s\r\n", strerror(errno) );
	}
	tcdrain( uart->fd );

	if( uart->rs485 && uart->gpio >= 0 )
		TB_gpio_set( uart->gpio, 1 );

	return r;
}

int uart_read( UART_T *uart, void *p_data, int max_len )
{
	if( uart == NULL || uart->size != sizeof(UART_T) )
	{
		TB_dbg_uart( "uart is invalid!!\r\n" );
		return -1;
	}

	if( uart->fd < 0 )
	{
		TB_dbg_uart( "uart fd is invalid!! : %d\r\n", uart->fd );
		return -1;
	}

	int len = read( uart->fd, p_data, max_len );
	if( len > 0 )
	{
		TB_dbg_uart("uart->fd:%d type:%d read len:%d\r\n", uart->fd, uart->type, len);
	}

	tcflush( uart->fd, TCIOFLUSH );

	return len;
}

