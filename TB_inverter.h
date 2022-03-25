#ifndef	__TB_INVERTER_H__
#define __TB_INVERTER_H__

#define RS485_INVERTER_LOW		1
#define RS485_INVERTER_HIGH		2

////////////////////////////////////////////////////////////////////////////////

#define INVT_EACH_BUF_SIZE	(64)
typedef struct tagINVT_INFO
{
	TBUC buf[INVT_EACH_BUF_SIZE];
	TBSC size;

	TBUI count_try;
	TBUI count_success;
	TBUI count_fail;
} TB_INVT_INFO;

////////////////////////////////////////////////////////////////////////////////

extern int  TB_rs485_init( void );
extern void  TB_rs485_deinit( void );
extern void TB_rs485_proc_start( void );
extern TBUC TB_rs485_read( TBUC rs485 );
extern TBUS TB_rs485_write( TBUC rs485, TBUC *buf, int len );

extern void TB_rs485_test_proc_start( void );
extern void TB_rs485_test_proc_stop( void );

extern TBSC TB_rs485_each_info_datasize( int idx );
extern TB_INVT_INFO *TB_rs485_each_info( int idx );

#endif//__TB_INVERTER_H__

