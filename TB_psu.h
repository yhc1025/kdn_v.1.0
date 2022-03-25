#ifndef	__TB_PSU_H__
#define __TB_PSU_H__

#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_DNP_BUF				(1024)

////////////////////////////////////////////////////////////////////////////////

#define PSU_MODBUS_TIME_SLAVEID	0xFF
#define PSU_MODBUS_TIME_FUNC 	0x66

////////////////////////////////////////////////////////////////////////////////

extern int 	TB_psu_mod_init( void );
extern TBUS TB_psu_mod_read( void );
extern TBUS TB_psu_mod_write( TBUC *buf, int len );
extern void TB_psu_mod_deinit( void );
extern int  TB_psu_mod_write_get_time_command( void );
extern int  TB_psu_mod_set_system_time( TBUC *p_data, TBUC size );
extern void TB_psu_mod_proc_start( void );
extern void TB_psu_mod_proc_stop( void );

extern int 	TB_psu_dnp_init( void );
extern TBUS TB_psu_dnp_read( void );
extern TBUS TB_psu_dnp_write( TBUC *buf, int len );
extern void TB_psu_dnp_deinit( void );
extern void TB_psu_dnp_proc_start( void );
extern void TB_psu_dnp_proc_stop( void );

extern void TB_psu_dnp_test_proc_start( void );
extern void TB_psu_dnp_test_proc_stop( void );
extern void TB_psu_mod_test_proc_start( void );
extern void TB_psu_mod_test_proc_stop( void );

#endif//__TB_PSU_H__

