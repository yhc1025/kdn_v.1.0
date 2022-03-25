#ifndef	__TB_AT_H__
#define __TB_AT_H__

#include "TB_type.h"

extern int 	TB_at_init( void );
extern TBUS TB_at_read( void );
extern TBUS TB_at_write( TBUC *buf, int len );
extern void TB_at_deinit( void );
extern void TB_at_proc_start( void );
extern void TB_at_proc_stop( void );

#endif//__TB_AT_H__

