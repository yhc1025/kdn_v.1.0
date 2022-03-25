#ifndef __TB_LOG_H__
#define __TB_LOG_H__

#include "TB_rb.h"
#include "TB_login.h"
#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

//#define USE_LOG_SAVE_DELAY
#define USE_LOG_SAVE_IMMEDIATE_MSGQ

#define DEF_LOG_WORK_TIME		"/mnt/last_work.dat"
#define DEF_LOG_FILE_SYS		"/mnt/log_secu.dat"
#define DEF_LOG_FILE_COMM		"/mnt/log_comm.dat"
#define DEF_SAVE_PERIOD			(1 * 60)	//	1 min

////////////////////////////////////////////////////////////////////////////////

#define MAX_LOG_COUNT	1000

#define LOG_TYPE_SECU_START		1
#define LOG_TYPE_COMM_START		100
#define LOG_TYPE_SECU			LOG_TYPE_SECU_START
#define LOG_TYPE_COMM			LOG_TYPE_COMM_START

////////////////////////////////////////////////////////////////////////////////

typedef enum tagLOGCODE
{
	LOG_SECU_BOOT						= LOG_TYPE_SECU_START,
	LOG_SECU_WATCHDOG,
	LOG_SECU_SETUP,
	LOG_SECU_TIMESYNC,
	LOG_SECU_LAST_WORKING_TIME,		/* max 1min time diff	*/
	LOG_SECU_WORKING_NORMAL,		/* for test				*/
	LOG_SECU_FWUPDATE_FAIL,
	LOG_SECU_FWUPDATE,
	LOG_SECU_FACTORY_FAIL,
	LOG_SECU_FACTORY,
	LOG_SECU_LOGIN_FAIL,
	LOG_SECU_LOGIN,
	LOG_SECU_PW_CHANGE_ADMIN,
	LOG_SECU_PW_CHANGE_USER,
	LOG_SECU_ERROR_FILE_SETUP,
	LOG_SECU_ERROR_FILE_LOG_COMM,
	LOG_SECU_ERROR_FILE_LOG_SECU,
	LOG_TYPE_SECU_END,
	
	LOG_COMM_GGW2TERM_SUCCESS			= LOG_TYPE_COMM_START,
	LOG_COMM_GGW2REPEATER_SUCCESS,
	LOG_COMM_TERM2RELAY_SUCCESS,
	LOG_COMM_GGW2TERM_FAILED,
	LOG_COMM_GGW2REPEATER_FAILED,
	LOG_COMM_TERM2RELAY_FAILED,
	LOG_TYPE_MODEM_DATA_ERROR,
	LOG_TYPE_WISUN_DATA_ERROR,
	LOG_TYPE_COMM_END,
	
	LOG_CODE_MAX
} TB_LOGCODE;

typedef enum tagLOGCODE_DATA_TYPE
{
	LOGCODE_DATETIME			 = 1,
	LOGCODE_DATA_SETUP_CHANGE,
	LOGCODE_DATA_WDT_COND,
	LOGCODE_DATA_FILE_OPEN_ERROR,
	LOGCODE_DATA_FILE_DATA_ERROR,
	LOGCODE_DATA_FILE_READ_ERROR,
	LOGCODE_DATA_FILE_WRITE_ERROR,
} TB_LOGCODE_DATA_TYPE;

typedef struct tagLOGCODE_DATA
{
	TB_LOGCODE_DATA_TYPE 	type;
	TBUC					data[16+1];
	TBUC					size;
} TB_LOGCODE_DATA;

typedef struct tagLOGITEM
{
	time_t 				t;
	TB_LOGCODE			code;
	TB_LOGCODE_DATA		code_data;
	TB_ACCOUNT_TYPE 	account;
} TB_LOGITEM;

typedef struct tagLOG
{
	TB_RB_H 	rb;
	TBUC 		log[sizeof(TB_LOGITEM) * (MAX_LOG_COUNT+1)];
	int			flag;
} TB_LOG;

////////////////////////////////////////////////////////////////////////////////

extern int TB_log_append( TB_LOGCODE code, TB_LOGCODE_DATA *p_data, TB_ACCOUNT_TYPE account );
extern int TB_log_save_check( int type );
extern int TB_log_save_all( void );
extern int TB_log_save( TBUC *p_path, TB_LOG *p_log );
extern int TB_log_load( TBUC *p_path, TB_LOG *p_log );
extern int TB_log_init( TBBL run_proc );
extern int TB_log_deinit( TBBL run_proc );
extern void TB_log_dump( TB_LOG *p_log );

extern TB_LOG *TB_log_get_log_sys( void );
extern TB_LOG *TB_log_get_log_comm( void );
extern TB_LOG *TB_log_get_log( int type );
extern TBBL TB_log_check_detail_data( TB_LOGITEM *p_item );

extern int  TB_log_item_string( TB_LOGITEM *p_item, char *p_log_str, size_t log_str_size );
extern void TB_log_item_dump( TB_LOGITEM *p_item, char *p_msg );

extern int TB_log_last_working_time_init( void );
extern int TB_log_last_working_time_read( time_t *p_last );
extern int TB_log_last_working_time_save( void );

#endif//__TB_LOG_H__

