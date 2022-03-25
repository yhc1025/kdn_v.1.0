#ifndef	__TB_OPTICAL_H__
#define __TB_OPTICAL_H__

#include "TB_type.h"

extern int 	TB_optical_init( void );
extern TBUS TB_optical_read( void );
extern TBUS TB_optical_write( TBUC *buf, int len );
extern void TB_optical_deinit( void );
extern void TB_optical_proc_start( void );
extern void TB_optical_proc_stop( void );

#endif//__TB_OPTICAL_H__

