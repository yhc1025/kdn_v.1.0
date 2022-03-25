#ifndef	__TB_LTE_H__
#define __TB_LTE_H__

#include "TB_type.h"

extern int 	TB_lte_init( void );
extern TBUS TB_lte_read( void );
extern TBUS TB_lte_write( TBUC *buf, int len );
extern void TB_lte_deinit( void );
extern void TB_lte_proc_start( void );
extern void TB_lte_proc_stop( void );

#endif//__TB_LTE_H__

