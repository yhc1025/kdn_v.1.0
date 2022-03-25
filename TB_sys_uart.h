#ifndef __UARTLIB_H__
#define __UARTLIB_H__

#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

typedef struct tagUART_T
{
	int type;
	int	fd;		// ttyS1, ttyUSB0 ~ ttyUSB3
	int baud;
	int	us_per_bit;
	int rs485;
	int gpio;
	struct termios	tio;

	int size;
} UART_T;

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	UART_TYPE_PSU_DNP	= 0,	// RS232, 9600, ttyS1
	UART_TYPE_PSU_MOD	= 1,	// RS232, 9600, ttyUSB1	
	UART_TYPE_WISUN		= 2,	// RS232, 115200, ttyS2	
	UART_TYPE_RS485A	= 3,	// RS485, 9600, ttyS3
	UART_TYPE_RS485B	= 4,	// RS485, 9600, ttyS4
	UART_TYPE_MODEM		= 5,	// RS232, 9600, ttyUSB1	
	UART_TYPE_LTEM		= 6,	// RS232, 9600, ttyUSB1	
	UART_TYPE_EMS		= 7,	// RS232, 9600, ttyUSB1	
	UART_TYPE_FRTU		= 8,	// RS232, 9600, ttyUSB1	
	
	UART_TYPE_MAX
} UART_TYPE_E;

typedef enum
{
	UART_BAUD_2400		= 0,
	UART_BAUD_4800		= 1,
	UART_BAUD_9600		= 2,
	UART_BAUD_19200		= 3,
	UART_BAUD_38400		= 4,
	UART_BAUD_59600		= 5,
	UART_BAUD_115200	= 6,
} UART_BAUD_E;

////////////////////////////////////////////////////////////////////////////////

int uart_lock(void);
int uart_unlock(void);
UART_T *uart_new(int uart_type);
void uart_delete(UART_T *uart);
int uart_set(UART_T *uart, int baud, int type);
int uart_write(UART_T *uart, void *p_data, int len);
int uart_read(UART_T *uart, void *p_data, int max_len);

#endif	//__UARTLIB_H__

