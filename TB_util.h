#ifndef	__TB_UTIL_H__
#define __TB_UTIL_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>

#include "TB_crc.h"
#include "TB_wrapper.h"
#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

extern void TB_util_delay( unsigned long delay_msec );
extern void TB_util_data_dump( TBUC *msg, TBUC *data, TBUS length );
extern void TB_util_data_dump1( TBUC *data, TBUS length );
extern void TB_util_data_dump2( TBUC *data, TBUS length );
extern int 	TB_util_run_cmd( TBUC *command );
extern void TB_util_print_time1( int YY, int MM, int DD, int hh, int mm, int ss);
extern void TB_util_print_time2( time_t t );
extern int 	TB_util_set_systemtime1( int YY, int MM, int DD, int hh, int mm, int ss );
extern int  TB_util_set_systemtime2( time_t t );
extern TBBL TB_util_time_check_validation( time_t t );
extern int 	TB_util_is_leap_year( int year );
extern int 	TB_util_get_lastday_of_month( int mon, int year );

extern void TB_util_print_fw_version_info( void );

extern int TB_util_switch_bg_proc( void );
extern TBUC *TB_util_get_datetime_string( time_t t );
extern int TB_util_get_file_size( TBUC *p_path );
extern int TB_util_get_rand( int range_min, int range_max );

#endif//__TB_UTIL_H__

