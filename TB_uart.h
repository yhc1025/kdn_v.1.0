#ifndef	__TB_UART_H__
#define __TB_UART_H__

#include "TB_sys_uart.h"
#include "TB_type.h"

#if 1
typedef struct tagUART_OPERATION
{
	const char 	*name;
	void 		*ctx;
	
	/* common function */
	int		(*init)		(void);
	void	(*deinit)	(void);
	TBUS	(*read)		(TBUC *, TBUI );
	TBUS	(*write)	(void *, int );
} TB_OP_UART;

extern TB_OP_UART g_op_uart_wisun;
extern TB_OP_UART g_op_uart_invta;
extern TB_OP_UART g_op_uart_invtb;
extern TB_OP_UART g_op_uart_mod;
extern TB_OP_UART g_op_uart_dnp;
extern TB_OP_UART g_op_uart_mdm;
extern TB_OP_UART g_op_uart_lte;
#endif

extern int 	TB_uart_wisun_1st_init( void );
extern void	TB_uart_wisun_1st_deinit( void );
extern TBUS TB_uart_wisun_1st_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_wisun_1st_write( void *p_data, int size );
extern TBUC TB_uart_wisun_1st_get_status( void );

extern int  TB_uart_rs485a_init( void );
extern void TB_uart_rs485a_deinit( void );
extern TBUS TB_uart_rs485a_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_rs485a_write( void *p_data, int size );

extern int  TB_uart_rs485b_init( void );
extern void TB_uart_rs485b_deinit( void );
extern TBUS TB_uart_rs485b_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_rs485b_write( void *p_data, int size );

extern int  TB_uart_mdm_init( void );
extern void TB_uart_mdm_deinit( void );
extern TBUS TB_uart_mdm_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_mdm_write( void *p_data, int size );

extern int  TB_uart_lte_init( void );
extern void TB_uart_lte_deinit( void );
extern TBUS TB_uart_lte_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_lte_write( void *p_data, int size );

extern int  TB_uart_psu_mod_init( void );
extern void TB_uart_psu_mod_deinit( void );
extern TBUS TB_uart_psu_mod_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_psu_mod_write( void *p_data, int size );

extern int  TB_uart_psu_dnp_init( void );
extern void TB_uart_psu_dnp_deinit( void );
extern TBUS TB_uart_psu_dnp_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_psu_dnp_write( void *p_data, int size );

extern int  TB_uart_ems_init( void );
extern void TB_uart_ems_deinit( void );
extern TBUS TB_uart_ems_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_ems_write( void *p_data, int size );

extern int  TB_uart_frtu_init( void );
extern void TB_uart_frtu_deinit( void );
extern TBUS TB_uart_frtu_read( TBUC *p_buffer, TBUI buf_size );
extern TBUS TB_uart_frtu_write( void *p_data, int size );

#endif//__TB_UART_H__

