#ifndef	__TB_MODEM_H__
#define __TB_MODEM_H__

#include "TB_type.h"

extern int 	TB_modem_init( void );
extern TBUS TB_modem_read( void );
extern TBUS TB_modem_write( TBUC *buf, TBUS len );
extern void TB_modem_deinit( void );
extern void TB_modem_proc_start( void );
extern void TB_modem_proc_stop( void );

extern void TB_modem_test_proc_start( void );
extern void TB_modem_test_proc_stop( void );

#endif//__TB_MODEM_H__

