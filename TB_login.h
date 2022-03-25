#ifndef	__TB_LOGIN_H__
#define __TB_LOGIN_H__

#include "TB_resource.h"
#include "TB_type.h"
#include "TB_kcmvp.h"

////////////////////////////////////////////////////////////////////////////////

#define MIN_PW_LEN		9
#define MAX_PW_LEN		16

////////////////////////////////////////////////////////////////////////////////

typedef enum tagLOGIN_ERROR
{
	LOGIN_ERROR_PW_FILE_ABNORMAL,
	LOGIN_ERROR_PW_DEFAULT,
	LOGIN_ERROR_ATTRIBUTE,
	LOGIN_ERROR_PW_LENGTH_MIN,
	LOGIN_ERROR_PW_LENGTH_MAX,
	LOGIN_ERROR_UNKNOWN,
	LOGIN_ERROR,

	LOGIN_ERROR_NONE = 0,
	LOGIN_ERROR_MAX = 0
} TB_LOGIN_ERROR;

typedef enum tagACCOUNT_TYPE
{
	ACCOUNT_TYPE_ERROR,
	ACCOUNT_TYPE_ADMIN = 0,
	ACCOUNT_TYPE_USER
} TB_ACCOUNT_TYPE;

typedef struct tagACCOUNT_INFO
{
	time_t 	login_time;
	TBUC 	pw[MAX_PW_LEN+1];
} TB_ACCOUNT_INFO;

typedef struct tagLOGIN
{
	TB_ACCOUNT_INFO admin;
	TB_ACCOUNT_INFO user;
} TB_LOGIN;

////////////////////////////////////////////////////////////////////////////////

extern int  TB_login_init( void );
extern int  TB_login_show_change_pw( TB_ACCOUNT_TYPE type, TBBL force );
extern int  TB_login_show_input_pw( TB_ACCOUNT_TYPE type );
extern int  TB_login_account_clear( void );

#endif//__TB_LOGIN_H__

