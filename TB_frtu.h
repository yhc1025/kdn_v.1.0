#ifndef	__TB_FRTU_H__
#define __TB_FRTU_H__

#include "TB_type.h"

extern int 	TB_frtu_init( void );
extern TBUS TB_frtu_read( void );
extern TBUS TB_frtu_write( TBUC *buf, int len );
extern void TB_frtu_deinit( void );
extern void TB_frtu_proc_start( void );
extern void TB_frtu_proc_stop( void );

extern void TB_frtu_test_proc_start( void );
extern void TB_frtu_test_proc_stop( void );

#endif//__TB_FRTU_H__

