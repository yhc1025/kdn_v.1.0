#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <term.h>
#include <ncurses.h>

#include "TB_sys_wdt.h"
#include "TB_util.h"
#include "TB_j11.h"
#include "TB_debug.h"
#include "TB_kcmvp.h"
#include "TB_log.h"
#include "TB_ui.h"
#include "TB_login.h"
#include "TB_base.h"
#include "TB_msgbox.h"
#include "TB_test.h"
#include "TB_rssi.h"
#include "TB_inverter.h"
#include "TB_aes_evp.h"
#include "TB_setup.h"

#define MINUTE_1	(60)
#define MINUTE_30	(30*MINUTE_1)
#define HOUR_1		(60*MINUTE_1)

////////////////////////////////////////////////////////////////////////////////

extern TBUC g_device_role;

////////////////////////////////////////////////////////////////////////////////

#define SERIAL_COMM_RECV		"lrz"
#define SERIAL_COMM_SEND		"lsz"

////////////////////////////////////////////////////////////////////////////////

#define SETUP_FILE_PATH			"/mnt/setup.cfg"
#define MASTERKEY_FILE_PATH		"/root/master_key.bin"

#define KDN_FILE_PATH			"/root/kdn"
#define KDN_FILE_NAME			"kdn"

#define KDN_ORG_FILE_PATH		"/root/kdn_org"
#define KDN_ORG_FILE_NAME		"kdn_org"

#define KDN_TAR_FILE_PATH		"/root/kdn.tar"
#define KDN_TAR_FILE_NAME		"kdn.tar"

#define KDN_GZIP_FILE_PATH		"/root/kdn.tar.gz"
#define KDN_GZIP_FILE_NAME		"kdn.tar.gz"

#define KDN_HASH_FILE_PATH		"/root/kdn_hash"
#define KDN_HASH_FILE_NAME		"kdn_hash"

#define KDN_HASH_ORG_FILE_PATH	"/root/kdn_hash_org"
#define KDN_HASH_ORG_FILE_NAME	"kdn_hash_org"

#define KDN_HASH_FILE_NAME_CHK	"kdn_hash_chk"
#define HASH_COMPARE_FILE_NAME	"compare"

#define MAX_CONNECT				3

#define AUTO_LOCK_TIME_MAX		10
#define AUTO_LOCK_TIME_DEF		5

////////////////////////////////////////////////////////////////////////////////

int baudrate_table[] = { 2400, 4800, 9600, 19200, 38400, 57600, 115200 };

static TB_ACCOUNT_TYPE s_setup_login_account = ACCOUNT_TYPE_USER;

static int s_idx_main = 0;
static int s_idx_setting = 0;
static int s_idx_wisun1 = 0;
static int s_idx_wisun2 = 0;
static int s_idx_time = 0;
static int s_idx_kcmvp = 0;
static int s_idx_history = 0;
static int s_idx_pw_change = 0;
static int s_idx_info = 0;
static int s_idx_kcmvp_key_edit = 0;
static int s_idx_admin = 0;

static char *sp_str_wisun_freq[WISUN_FREQ_MAX+1] = 
{
	"922.5MHz",
	"922.9MHz",
	"923.3MHz",
	"923.7MHz",
	"924.1MHz",
	"924.5MHz",
	"924.9MHz",
	"925.3MHz",
	"925.7MHz",
	"926.1MHz",
	"926.5MHz",
	"926.9MHz",
	"927.3MHz",
	"927.7MHz",
};

static char *sp_str_comm_mode_master[COMM_MODE_MASTER_CNT] = 
{
	"Wi-SUN",
	"LTE   ",
	"RS-232"
};

static char *sp_str_comm_mode_slave[COMM_MODE_SLAVE_CNT] = 
{
	"Wi-SUN",
	"RS-485"
};

////////////////////////////////////////////////////////////////////////////////

static TB_SETUP_FILE  	s_setup_file;
static TB_SETUP_FILE  	s_setup_file_org;
static TB_ACCOUNT_TYPE	s_login_type;
static TBUI				s_setup_chg_flag = 0;

////////////////////////////////////////////////////////////////////////////////

typedef enum tagMENUID
{
	MID_SETTING,
	MID_COMM_STATUS,
	
	MID_ROLE,
	MID_WISUN_1ST,
	MID_WISUN_2ND,
	
	MID_HISTORY,	
	MID_TIME,
	MID_FACTORY,
	MID_DOWNLOAD,
	
	MID_MASTERKEY,
	MID_INFO,
	
	MID_REBOOT,
	
	MID_ADMIN,

	MID_MAX
} TB_MENUID;

////////////////////////////////////////////////////////////////////////////////

int util_is_leap_year( int year );
int util_get_lastday_of_month( int mon, int year );

////////////////////////////////////////////////////////////////////////////////

void print_menu_sub_setting( void );
void update_menu_sub_setting_value( TBSI focus, TBSI value );
void update_menu_sub_setting_focus( TBSI old, TBSI now );
void op_menu_sub_setting( void );

int op_menu_sub_log_comm( void );
int op_menu_sub_log_security( void );

void update_time_focus( TBSI old, TBSI now );
void print_time_info( void );
void update_time_value( TBSI focus, TBSI value );
void print_menu_sub_time( void );
void op_menu_sub_time( void );

void print_menu_sub_factory( void );
void op_menu_sub_factory( void );

void print_menu_sub_fwupdate( void );
void op_menu_sub_fwupdate( void );

void print_menu_main( void );
void update_menu_main( TBSI old, TBSI now );
void op_menu_sub_menu( void );

void op_menu_sub_masterkey( void );
void op_menu_sub_masterkey_edit( void );
void op_menu_sub_masterkey_info( void );

void print_menu_sub_wisun( TBUC idx );
void print_menu_sub_admin_setting( void );

////////////////////////////////////////////////////////////////////////////////

TBSC TB_setup_get_invt_timeout( void )
{
	return s_setup_file.setup.invt.timeout;
}

TBSC TB_setup_get_invt_retry( void )
{
	return s_setup_file.setup.invt.retry;
}

TBSC TB_setup_get_invt_count( void )
{
	return s_setup_file.setup.invt.cnt;
}

TBUS TB_setup_get_invt_delay_read( void )
{
	return s_setup_file.setup.invt.delay_read;
}

TBUS TB_setup_get_invt_delay_write( void )
{
	return s_setup_file.setup.invt.delay_write;
}

TB_SETUP_INVT *TB_setup_get_invt_info( void )
{
	return &s_setup_file.setup.invt;
}

int TB_setup_set_invt_info( TB_SETUP_INVT *p_invt )
{
	int ret = -1;
	if( p_invt )
	{
		wmemcpy( &s_setup_file.setup.invt, sizeof(s_setup_file.setup.invt), p_invt, sizeof(TB_SETUP_INVT) );
		ret = 0;
	}

	return ret;
}

TBSC TB_setup_get_baud_frtu( void )
{
	return s_setup_file.setup.baud.frtu;
}

TBSC TB_setup_get_baud_inverter( void )
{
	return s_setup_file.setup.baud.invt;
}

TB_ROLE TB_setup_get_role( void )
{
	return s_setup_file.setup.role;
}

TBBL TB_setup_get_wisun_repeater( void )
{
	return s_setup_file.setup.repeater;
}

TB_SETUP_WISUN *TB_setup_get_wisun_info( TBUC idx )
{
	return &s_setup_file.setup.wisun[idx];
}

TB_WISUN_MODE TB_setup_get_wisun_info_mode( TBUC idx )
{
	return s_setup_file.setup.wisun[idx].mode;
}

TBBL TB_setup_get_wisun_info_enable( TBUC idx )
{
	return s_setup_file.setup.wisun[idx].enable;
}

void TB_setup_set_wisun_info_enable( TBUC idx, TBBL enable )
{
	s_setup_file.setup.wisun[idx].enable = enable;
}

TB_WISUN_FREQ TB_setup_get_wisun_info_freq( TBUC idx )
{
	return s_setup_file.setup.wisun[idx].frequency;
}

TBUC TB_setup_get_wisun_info_max_connect( TBUC idx )
{
	return s_setup_file.setup.wisun[idx].max_connect;
}

TB_COMM_TYPE TB_setup_get_comm_type( void )
{
	return s_setup_file.setup.comm_type;
}

TBSI TB_setup_get_comm_type_master( void )
{
	return s_setup_file.setup.comm_type.master;
}

TBSI TB_setup_get_comm_type_slave( void )
{
	return s_setup_file.setup.comm_type.slave;
}

TB_KCMVP_KEY *TB_setup_get_kcmvp_master_key( void )
{
	return &s_setup_file.setup.master_key;
}

int TB_setup_set_kcmvp_master_key( TB_KCMVP_KEY *p_key )
{
	int ret = -1;

	if( p_key )
	{
		wmemcpy( &s_setup_file.setup.master_key, sizeof(s_setup_file.setup.master_key), p_key, sizeof(TB_KCMVP_KEY) );
		ret = 0;
	}

	return ret;
}

TB_SETUP_WORK *TB_setup_get_config( void )
{
	return &s_setup_file.setup;
}

TBSI TB_setup_get_auto_lock( void )
{
	return s_setup_file.setup.auto_lock;
}

TBBL TB_setup_get_optical_sensor( void )
{
	return s_setup_file.setup.optical_sensor;
}

time_t TB_setup_get_session_update_time( void )
{
	return s_setup_file.setup.session_update;
}

int TB_setup_set_product_info( TB_PRODUCT_INFO *p_info )
{
	int ret = -1;

	if( p_info )
	{
		wmemcpy( &s_setup_file.setup.info, sizeof(s_setup_file.setup.info), p_info, sizeof(TB_PRODUCT_INFO) );
		ret = 0;
	}

	return ret;
}

TB_PRODUCT_INFO *TB_setup_get_product_info( void )
{
	return &s_setup_file.setup.info;
}

TB_VERSION *TB_setup_get_product_info_version( void )
{
	return &s_setup_file.setup.info.version;
}

TB_VOLTAGE TB_setup_get_product_info_voltage( void )
{
	return s_setup_file.setup.info.voltage;
}

TBUI TB_setup_get_product_info_ems_dest( void )
{
	return s_setup_file.setup.info.ems_dest;
}

TBUS TB_setup_get_product_info_ems_addr( void )
{
	return s_setup_file.setup.info.ems_addr;
}

TB_DATE TB_setup_get_product_info_manuf_date( void )
{
	return s_setup_file.setup.info.manuf_date;
}

TBUC *TB_setup_get_product_info_cot_ip( void )
{
	return s_setup_file.setup.info.cot_ip;
}

TBUS TB_setup_get_product_info_rt_port( void )
{
	return s_setup_file.setup.info.rt_port;
}

TBUS TB_setup_get_product_info_frtp_dnp( void )
{
	return s_setup_file.setup.info.frtp_dnp;
}

TBBL TB_setup_get_product_info_ems_use( void )
{
	return s_setup_file.setup.info.ems_use;
}

////////////////////////////////////////////////////////////////////////////////

enum 
{
	FOCUS_WISUN_ENABLE,
	FOCUS_WISUN_MODE,
	FOCUS_WISUN_MAXCON,
	FOCUS_WISUN_FREQ,
};

#define FOCUS_WISUN_MAX		FOCUS_WISUN_FREQ

int wisun_val_startx = 19;
char *get_wisun_enable_string( TBBL enable )
{
	return enable ? TB_resid_get_string(RESID_VAL_ENABLE) :	\
					TB_resid_get_string(RESID_VAL_DISABLE);
}

char *get_role_string( TB_ROLE role )
{
	char *p_string = NULL;

	TB_RESID rid = 0;
	if( role >= ROLE_MIN && role <= ROLE_MAX )
	{
		switch( role )
		{
			case ROLE_GRPGW		: rid = RESID_LBL_ROLE_GGW;		break;
			case ROLE_TERM1		: rid = RESID_LBL_ROLE_FRTU1;	break;
			case ROLE_TERM2		: rid = RESID_LBL_ROLE_FRTU2;	break;
			case ROLE_TERM3		: rid = RESID_LBL_ROLE_FRTU3;	break;
			case ROLE_RELAY1	: rid = RESID_LBL_ROLE_MID1;	break;
			case ROLE_RELAY2	: rid = RESID_LBL_ROLE_MID2;	break;
			case ROLE_RELAY3	: rid = RESID_LBL_ROLE_MID3;	break;
			case ROLE_REPEATER	: rid = RESID_LBL_ROLE_REP;		break;
		}

		if( rid != 0 )
			p_string = TB_resid_get_string( rid );
		else
			p_string = TB_resid_get_string( RESID_LBL_ROLE_GGW );			
	}
	
	return p_string;
}

char *get_wisun_mode_string( TB_WISUN_MODE mode )
{
	char *mode_string = NULL;

	if( mode >= WISUN_MODE_MIN && mode <= WISUN_MODE_MAX )
	{
		TB_RESID rid = 0;
		switch( mode )
		{
			case WISUN_MODE_PANC  : rid = RESID_LBL_MASTER_MDL;		break;
			case WISUN_MODE_COORD : rid = RESID_LBL_REPEATER_MDL;	break;
			case WISUN_MODE_ENDD  : rid = RESID_LBL_SLAVE_MDL;		break;
		}

		if( rid != 0 )
			mode_string = TB_resid_get_string( rid );
	}

	return mode_string;
}

char *get_wisun_max_connect_string( TBUC idx, int max_connect )
{
	static char buf[4] = {0, };

	bzero( buf, sizeof(buf) );
	if( s_setup_file.setup.wisun[idx].mode == WISUN_MODE_PANC )
	{
		snprintf( buf, sizeof(buf), "%d", max_connect );
	}
	else
	{
		snprintf( buf, sizeof(buf), "1" );
	}

	return buf;
}

char *get_wisun_freq_string( TB_WISUN_FREQ freq )
{
	char *freq_string = NULL;

	if( freq >= WISUN_FREQ_MIN && freq <= WISUN_FREQ_MAX )
		freq_string = sp_str_wisun_freq[freq];

	return freq_string;
}

char *get_comm_mode_string_master( TBSI mode )
{
	char *mode_string = NULL;

	if( mode >= 0 && mode < COMM_MODE_MASTER_CNT )
		mode_string = sp_str_comm_mode_master[mode];
	else
		mode_string = TB_resid_get_string( RESID_UNKNOWN );
	
	return mode_string;
}

char *get_comm_mode_string_slave( TBSI mode )
{
	char *mode_string = NULL;

	if( mode >= 0 && mode < COMM_MODE_SLAVE_CNT )
		mode_string = sp_str_comm_mode_slave[mode];
	else
		mode_string = TB_resid_get_string( RESID_UNKNOWN );
	
	return mode_string;
}

void print_menu_sub_wisun_val_enable( TBUC idx )
{
	TB_ui_move(2, wisun_val_startx);
	printw("%s\n", get_wisun_enable_string( s_setup_file.setup.wisun[idx].enable) );
}

void print_menu_sub_wisun_val_opmode( TBUC idx )
{
	TB_ui_move(3, wisun_val_startx);
	printw("%s\n", get_wisun_mode_string(s_setup_file.setup.wisun[idx].mode) );
}

void print_menu_sub_wisun_val_max_conn( TBUC idx )
{
	TB_ui_move(4, wisun_val_startx);
	printw("%s\n", get_wisun_max_connect_string(idx, s_setup_file.setup.wisun[idx].max_connect) );
}

void print_menu_sub_wisun_val_freq( TBUC idx )
{
	TB_ui_move(5, wisun_val_startx);
	printw("%s\n", get_wisun_freq_string(s_setup_file.setup.wisun[idx].frequency) );
}

void print_menu_sub_wisun( TBUC idx )
{
	int focus = idx==0 ? s_idx_wisun1 : s_idx_wisun2;
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", idx==0 ? TB_resid_get_string(RESID_TLT_WISUN_1ST) : TB_resid_get_string(RESID_TLT_WISUN_2ND) );
	printw("\n");

	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_ENABLE));
	focus==0 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	print_menu_sub_wisun_val_enable( idx );
	
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_OPMODE));
	focus==1 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	
	print_menu_sub_wisun_val_opmode( idx );
	
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_MAX_CONN));
	focus==2 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	
	print_menu_sub_wisun_val_max_conn( idx );
	
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_FREQ));
	focus==3 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	
	print_menu_sub_wisun_val_freq( idx );

	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();
}

void update_menu_sub_wisun( TBUC idx, TBSI old, TBSI now )
{
	if( old != now )
	{
		int focus = idx==0 ? s_idx_wisun1 : s_idx_wisun2;
		
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+old, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_ENABLE+old) );
		TB_ui_attrset( FOCUS_OFF );
		switch( old )
		{
			case FOCUS_WISUN_ENABLE	: print_menu_sub_wisun_val_enable( idx );	break;
			case FOCUS_WISUN_MODE	: print_menu_sub_wisun_val_opmode( idx );	break;
			case FOCUS_WISUN_MAXCON	: print_menu_sub_wisun_val_max_conn( idx );	break;
			case FOCUS_WISUN_FREQ	: print_menu_sub_wisun_val_freq( idx );		break;
		}
		
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+now, 0);	printw("%s : \n", TB_resid_get_string(RESID_LBL_ENABLE+now));
		TB_ui_attrset( FOCUS_ON );
		switch( now )
		{
			case FOCUS_WISUN_ENABLE	: print_menu_sub_wisun_val_enable( idx );	break;
			case FOCUS_WISUN_MODE	: print_menu_sub_wisun_val_opmode( idx );	break;
			case FOCUS_WISUN_MAXCON	: print_menu_sub_wisun_val_max_conn( idx );	break;
			case FOCUS_WISUN_FREQ	: print_menu_sub_wisun_val_freq( idx );		break;
		}
		
		TB_ui_refresh();
	}
}

void update_menu_sub_wisun_value( TBSI focus, char *value )
{
	TB_ui_attrset( FOCUS_ON );
	TB_ui_move( 2+focus, wisun_val_startx );
	printw( "%s", value );
	TB_ui_attrset( FOCUS_OFF );
}

void op_menu_sub_wisun( TBUC idx )
{
	TBSI quit = 0;
	TBSI input;

	int focus = (idx==0) ? s_idx_wisun1 : s_idx_wisun2;

	print_menu_sub_wisun( idx );

	TB_ui_key_init( TRUE );
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}
		
		switch( input )
		{
			case KEY_LEFT :
				switch( focus )
				{
					case FOCUS_WISUN_ENABLE :
						s_setup_file.setup.wisun[idx].enable = s_setup_file.setup.wisun[idx].enable ? FALSE : TRUE;
						update_menu_sub_wisun_value( focus, get_wisun_enable_string(s_setup_file.setup.wisun[idx].enable) );
						break;
					case FOCUS_WISUN_MODE :
						//if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
						{
							s_setup_file.setup.wisun[idx].mode  = --s_setup_file.setup.wisun[idx].mode < 0 ? WISUN_MODE_MAX : s_setup_file.setup.wisun[idx].mode;
							update_menu_sub_wisun_value( focus, get_wisun_mode_string(s_setup_file.setup.wisun[idx].mode) );
							print_menu_sub_wisun( idx );
						}
						break;
					case FOCUS_WISUN_MAXCON	:
						if( s_setup_file.setup.wisun[idx].mode == WISUN_MODE_PANC )
							s_setup_file.setup.wisun[idx].max_connect = --s_setup_file.setup.wisun[idx].max_connect < 1 ? MAX_CONNECT : s_setup_file.setup.wisun[idx].max_connect;
						else
							s_setup_file.setup.wisun[idx].max_connect = 1;
						update_menu_sub_wisun_value( focus, get_wisun_max_connect_string(idx, s_setup_file.setup.wisun[idx].max_connect) );
						break;
					case FOCUS_WISUN_FREQ	:
						s_setup_file.setup.wisun[idx].frequency = --s_setup_file.setup.wisun[idx].frequency < 0 ? WISUN_FREQ_MAX : s_setup_file.setup.wisun[idx].frequency;								
						update_menu_sub_wisun_value( focus, get_wisun_freq_string( s_setup_file.setup.wisun[idx].frequency ) );
						break;
				}
				break;
			case KEY_RIGHT :
				switch( focus )
				{
					case FOCUS_WISUN_ENABLE :
						s_setup_file.setup.wisun[idx].enable = s_setup_file.setup.wisun[idx].enable ? FALSE : TRUE;
						update_menu_sub_wisun_value( focus, get_wisun_enable_string(s_setup_file.setup.wisun[idx].enable) );
						break;
					case FOCUS_WISUN_MODE 	:
						//if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
						{
							s_setup_file.setup.wisun[idx].mode  = ++s_setup_file.setup.wisun[idx].mode > WISUN_MODE_MAX ? 0 : s_setup_file.setup.wisun[idx].mode;
							update_menu_sub_wisun_value( focus, get_wisun_mode_string(s_setup_file.setup.wisun[idx].mode) );
							print_menu_sub_wisun( idx );
						}
						break;
					case FOCUS_WISUN_MAXCON	:
						if( s_setup_file.setup.wisun[idx].mode == WISUN_MODE_PANC )
							s_setup_file.setup.wisun[idx].max_connect = ++s_setup_file.setup.wisun[idx].max_connect > MAX_CONNECT ? 1 : s_setup_file.setup.wisun[idx].max_connect;
						else
							s_setup_file.setup.wisun[idx].max_connect = 1;
						update_menu_sub_wisun_value( focus, get_wisun_max_connect_string(idx, s_setup_file.setup.wisun[idx].max_connect) );
						break;
					case FOCUS_WISUN_FREQ	:
						s_setup_file.setup.wisun[idx].frequency = ++s_setup_file.setup.wisun[idx].frequency > WISUN_FREQ_MAX ? 0 : s_setup_file.setup.wisun[idx].frequency;
						update_menu_sub_wisun_value( focus, get_wisun_freq_string( s_setup_file.setup.wisun[idx].frequency ) );
						break;
				}
				break;
			case KEY_UP :
				if( s_setup_file.setup.wisun[idx].enable == TRUE )
				{
					TBSI focus_old = focus;
					
					focus = --focus < 0 ? FOCUS_WISUN_MAX : focus;
					
					if( idx == 0 )
						s_idx_wisun1 = focus;
					else 
						s_idx_wisun2 = focus;
					update_menu_sub_wisun( idx, focus_old, focus );
				}
				break;
				
			case KEY_DOWN :
				if( s_setup_file.setup.wisun[idx].enable == TRUE )
				{
					TBSI focus_old = focus;
					
					focus = ++focus > FOCUS_WISUN_MAX ? 0 : focus;
					
					if( idx == 0 )
						s_idx_wisun1 = focus;
					else 
						s_idx_wisun2 = focus;
					update_menu_sub_wisun( idx, focus_old, focus );
				}				
				break;

			case '\n' 	:
			case KEY_ENTER :
				break;

			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}		
}

////////////////////////////////////////////////////////////////////////////////

enum 
{
	FOCUS_SETTING_FRTU_BAUD,
	FOCUS_SETTING_INVT_BAUD,
	FOCUS_SETTING_INVT_NUM,
	FOCUS_SETTING_INVT_TIMEOUT,
	FOCUS_SETTING_INVT_RETRY,
	FOCUS_SETTING_INVT_DELAY_READ,
	FOCUS_SETTING_INVT_DELAY_WRITE,
	FOCUS_SETTING_AUTO_LOCKING,
	FOCUS_SETTING_OPTICAL_SENSOR,
	FOCUS_SETTING_COMM_TYPE_MASTER,
	FOCUS_SETTING_COMM_TYPE_SLAVE,

	FOCUS_SETTING_MAX
};

char *get_auto_locking_string( TBSI value )
{
	static char s_str[32] = {0, };

	if( value >= 0 )
	{
		TBUC *str = NULL;
		if( value == 0 )
		{
			str = TB_resid_get_string( RESID_VAL_DISABLE );
			if( str )
				wstrncpy( s_str, sizeof(s_str), str, wstrlen(str) );
			
		}
		else if( value > 0 && value <= AUTO_LOCK_TIME_MAX )
		{
			str = TB_resid_get_string( RESID_VAL_MINUTE );
			if( str )
				snprintf( s_str, sizeof(s_str)-1, "%2d %s    ", value, str );
		}
		else
		{
			str = TB_resid_get_string( RESID_VAL_DISABLE );
			if( str )
				wstrncpy( s_str, sizeof(s_str),  str, wstrlen(str) );
			
			s_setup_file.setup.auto_lock = 0;
		}
	}

	return s_str;
}

char *get_optical_sensor_string( TBBL enable )
{
	static char s_str[32] = {0, };

	TBUC *str = NULL;
	if( enable )
	{
		str = TB_resid_get_string( RESID_VAL_ENABLE );
		if( str )
			wstrncpy( s_str, sizeof(s_str),  str, wstrlen(str) );
		
	}
	else
	{
		str = TB_resid_get_string( RESID_VAL_DISABLE );
		if( str )
			wstrncpy( s_str, sizeof(s_str),  str, wstrlen(str) );
	}

	return s_str;
}

int setting_startx = 33;
int menu_sub_setting_linenum[FOCUS_SETTING_MAX] = { 2, 3,    5, 6, 7,     9, 10,     12, 13,    15, 16 };
void print_menu_sub_setting( void )
{
	s_idx_setting = 0;
		
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_SETTING) );
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_FRTU_COM_SPEED));	TB_ui_attrset( FOCUS_ON ); 	printw("%6d bps\n", baudrate_table[s_setup_file.setup.baud.frtu] );
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INVT_COM_SPEED)); printw("%6d bps\n", baudrate_table[s_setup_file.setup.baud.invt] );
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INVT_NUM)); 		printw("%2d\n", s_setup_file.setup.invt.cnt );
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INVT_TIMEOUT)); 	printw("%4d msec\n", s_setup_file.setup.invt.timeout * 100); 
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INVT_RETRY));		printw("%d\n", s_setup_file.setup.invt.retry);
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INVT_READ_DELAY));	printw("%4d msec\n", s_setup_file.setup.invt.delay_read);
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INVT_WRITE_DELAY));	printw("%4d msec\n", s_setup_file.setup.invt.delay_write);
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_AUTO_LOCK));		printw("%s\n", get_auto_locking_string(s_setup_file.setup.auto_lock) );
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_OPTICAL_SENSOR));	printw("%s\n", get_optical_sensor_string(s_setup_file.setup.optical_sensor) );
	printw("\n");
	TB_ROLE role = TB_setup_get_role();
	if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
	{
		TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTER_COMM_MODE));	printw("%s\n", get_comm_mode_string_master(s_setup_file.setup.comm_type.master) );
		TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_SLAVE_COMM_MODE));	printw("%s\n", get_comm_mode_string_slave(s_setup_file.setup.comm_type.slave) );
	}
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();
}

void update_menu_sub_setting_value( TBSI focus, TBSI value )
{
	TB_ui_move( menu_sub_setting_linenum[focus], setting_startx );
	s_idx_setting==focus ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF ); 	
	
	switch( focus )
	{
		case FOCUS_SETTING_FRTU_BAUD		:	printw("%6d bps\n", value );				break;
		case FOCUS_SETTING_INVT_BAUD		:	printw("%6d bps\n", value );				break;
		case FOCUS_SETTING_INVT_NUM			:	printw("%2d\n", 	value );				break;
		case FOCUS_SETTING_INVT_TIMEOUT		:	printw("%4d msec\n",value );				break;
		case FOCUS_SETTING_INVT_RETRY		:	printw("%d\n",		value );				break;
		case FOCUS_SETTING_INVT_DELAY_READ	:	printw("%4d msec\n",value );				break;
		case FOCUS_SETTING_INVT_DELAY_WRITE	:	printw("%4d msec\n",value );				break;
		case FOCUS_SETTING_AUTO_LOCKING		:	printw("%s", get_auto_locking_string(value));		break;
		case FOCUS_SETTING_OPTICAL_SENSOR	:	printw("%s", get_optical_sensor_string(value));		break;
		case FOCUS_SETTING_COMM_TYPE_MASTER	:	printw("%s", get_comm_mode_string_master(value));	break;
		case FOCUS_SETTING_COMM_TYPE_SLAVE	:	printw("%s", get_comm_mode_string_slave(value));	break;
	}

	if( focus == FOCUS_SETTING_OPTICAL_SENSOR )
	{
		if( value == TRUE )
		{
			if( s_setup_file.setup.invt.cnt > 20 )
			{
				s_setup_file.setup.invt.cnt = 20;
				update_menu_sub_setting_value( FOCUS_SETTING_INVT_NUM, s_setup_file.setup.invt.cnt );
			}
		}
	}
}

void update_menu_sub_setting_focus( TBSI old, TBSI now )
{
	switch( old )
	{
		case FOCUS_SETTING_FRTU_BAUD		:	update_menu_sub_setting_value(old, (TBSI)baudrate_table[s_setup_file.setup.baud.frtu] );	break;
		case FOCUS_SETTING_INVT_BAUD		:	update_menu_sub_setting_value(old, (TBSI)baudrate_table[s_setup_file.setup.baud.invt] );	break;
		case FOCUS_SETTING_INVT_NUM		    :	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.invt.cnt );					break;
		case FOCUS_SETTING_INVT_TIMEOUT		:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.invt.timeout * 100 );		break;
		case FOCUS_SETTING_INVT_RETRY		:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.invt.retry );				break;
		case FOCUS_SETTING_INVT_DELAY_READ	:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.invt.delay_read);			break;
		case FOCUS_SETTING_INVT_DELAY_WRITE	:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.invt.delay_write);			break;
		case FOCUS_SETTING_AUTO_LOCKING		:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.auto_lock);					break;
		case FOCUS_SETTING_OPTICAL_SENSOR	:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.optical_sensor);			break;
		case FOCUS_SETTING_COMM_TYPE_MASTER	:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.comm_type.master);			break;
		case FOCUS_SETTING_COMM_TYPE_SLAVE	:	update_menu_sub_setting_value(old, (TBSI)s_setup_file.setup.comm_type.slave );			break;
	}

	switch( now )
	{
		case FOCUS_SETTING_FRTU_BAUD		:	update_menu_sub_setting_value(now, (TBSI)baudrate_table[s_setup_file.setup.baud.frtu] );	break;
		case FOCUS_SETTING_INVT_BAUD		:	update_menu_sub_setting_value(now, (TBSI)baudrate_table[s_setup_file.setup.baud.invt] );	break;
		case FOCUS_SETTING_INVT_NUM		    :	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.invt.cnt );					break;
		case FOCUS_SETTING_INVT_TIMEOUT		:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.invt.timeout * 100 );		break;
		case FOCUS_SETTING_INVT_RETRY		:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.invt.retry );				break;
		case FOCUS_SETTING_INVT_DELAY_READ	:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.invt.delay_read);			break;
		case FOCUS_SETTING_INVT_DELAY_WRITE	:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.invt.delay_write);			break;
		case FOCUS_SETTING_AUTO_LOCKING		:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.auto_lock);					break;
		case FOCUS_SETTING_OPTICAL_SENSOR	:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.optical_sensor);			break;
		case FOCUS_SETTING_COMM_TYPE_MASTER	:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.comm_type.master);			break;
		case FOCUS_SETTING_COMM_TYPE_SLAVE	:	update_menu_sub_setting_value(now, (TBSI)s_setup_file.setup.comm_type.slave );			break;
	}
}

void op_menu_sub_setting( void )
{
	int i;
	int quit = 0;
	int input;

	int menu_max = 0;
	TB_ROLE role = TB_setup_get_role();
	if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
	{
		menu_max = FOCUS_SETTING_MAX - 1;
	}
	else
	{
		menu_max = FOCUS_SETTING_MAX - 4;	//	except communication type and optical sensor
		s_setup_file.setup.optical_sensor = FALSE;
	}

	print_menu_sub_setting();

	TB_ui_key_init( TRUE );
	while( quit == 0 )
	{
	    input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

	    switch( input )
	    {
			case KEY_LEFT	:
				switch( s_idx_setting )
				{
					case FOCUS_SETTING_FRTU_BAUD	    :	s_setup_file.setup.baud.frtu 		= --s_setup_file.setup.baud.frtu < SET_DEF_FRTU_BAUD_MIN ? SET_DEF_FRTU_BAUD_MAX : s_setup_file.setup.baud.frtu;		update_menu_sub_setting_value(s_idx_setting, baudrate_table[s_setup_file.setup.baud.frtu] );	break;
					case FOCUS_SETTING_INVT_BAUD	    :	s_setup_file.setup.baud.invt 		= --s_setup_file.setup.baud.invt < SET_DEF_INVT_BAUD_MIN ? SET_DEF_INVT_BAUD_MAX : s_setup_file.setup.baud.invt;		update_menu_sub_setting_value(s_idx_setting, baudrate_table[s_setup_file.setup.baud.invt] );	break;
					case FOCUS_SETTING_INVT_NUM		    :	if( s_setup_file.setup.optical_sensor == TRUE )
																s_setup_file.setup.invt.cnt = --s_setup_file.setup.invt.cnt  < SET_DEF_INVT_CNT_MIN ? (SET_DEF_INVT_CNT_MAX/2) : s_setup_file.setup.invt.cnt;
															else
																s_setup_file.setup.invt.cnt = --s_setup_file.setup.invt.cnt  < SET_DEF_INVT_CNT_MIN ? SET_DEF_INVT_CNT_MAX : s_setup_file.setup.invt.cnt;
															update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.cnt );
															break;
					case FOCUS_SETTING_INVT_TIMEOUT		:	s_setup_file.setup.invt.timeout		= --s_setup_file.setup.invt.timeout < SET_DEF_INVT_TOUT_MIN ? SET_DEF_INVT_TOUT_MAX : s_setup_file.setup.invt.timeout;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.timeout * 100 );		break;
					case FOCUS_SETTING_INVT_RETRY		:	s_setup_file.setup.invt.retry 		= --s_setup_file.setup.invt.retry < SET_DEF_INVT_RETRY_MIN ? SET_DEF_INVT_RETRY_MAX : s_setup_file.setup.invt.retry;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.retry );				break;
					case FOCUS_SETTING_INVT_DELAY_READ	:	s_setup_file.setup.invt.delay_read 	= s_setup_file.setup.invt.delay_read-100 < SET_DEF_INVT_RDELAY_MIN ? SET_DEF_INVT_RDELAY_MAX : s_setup_file.setup.invt.delay_read-100;		update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.delay_read);		break;
					case FOCUS_SETTING_INVT_DELAY_WRITE	:	s_setup_file.setup.invt.delay_write	= s_setup_file.setup.invt.delay_write-100 < SET_DEF_INVT_WDELAY_MIN ? SET_DEF_INVT_WDELAY_MAX : s_setup_file.setup.invt.delay_write-100;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.delay_write);		break;
					case FOCUS_SETTING_AUTO_LOCKING		:	s_setup_file.setup.auto_lock		= --s_setup_file.setup.auto_lock  < 0 ? AUTO_LOCK_TIME_MAX  : s_setup_file.setup.auto_lock;		update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.auto_lock);	break;
					case FOCUS_SETTING_OPTICAL_SENSOR	:	s_setup_file.setup.optical_sensor	= !s_setup_file.setup.optical_sensor;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.optical_sensor);	break;
					case FOCUS_SETTING_COMM_TYPE_MASTER	:	s_setup_file.setup.comm_type.master	= --s_setup_file.setup.comm_type.master < 0 ? COMM_MODE_MASTER_CNT-1 : s_setup_file.setup.comm_type.master;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.comm_type.master);	break;
					case FOCUS_SETTING_COMM_TYPE_SLAVE	:	s_setup_file.setup.comm_type.slave	= --s_setup_file.setup.comm_type.slave  < 0 ? COMM_MODE_SLAVE_CNT-1  : s_setup_file.setup.comm_type.slave;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.comm_type.slave);	break;
				}
				break;
				
			case KEY_RIGHT	:
				switch( s_idx_setting )
				{
					case FOCUS_SETTING_FRTU_BAUD		:	s_setup_file.setup.baud.frtu 		= ++s_setup_file.setup.baud.frtu > SET_DEF_FRTU_BAUD_MAX ? SET_DEF_FRTU_BAUD_MIN : s_setup_file.setup.baud.frtu;		update_menu_sub_setting_value(s_idx_setting, baudrate_table[s_setup_file.setup.baud.frtu] );	break;
					case FOCUS_SETTING_INVT_BAUD		:	s_setup_file.setup.baud.invt 		= ++s_setup_file.setup.baud.invt > SET_DEF_INVT_BAUD_MAX ? SET_DEF_INVT_BAUD_MIN : s_setup_file.setup.baud.invt;		update_menu_sub_setting_value(s_idx_setting, baudrate_table[s_setup_file.setup.baud.invt] );	break;
					case FOCUS_SETTING_INVT_NUM		    :	if( s_setup_file.setup.optical_sensor == TRUE )
																s_setup_file.setup.invt.cnt = ++s_setup_file.setup.invt.cnt > (SET_DEF_INVT_CNT_MAX/2) ? SET_DEF_INVT_CNT_MIN : s_setup_file.setup.invt.cnt;
															else
																s_setup_file.setup.invt.cnt = ++s_setup_file.setup.invt.cnt > SET_DEF_INVT_CNT_MAX ? SET_DEF_INVT_CNT_MIN : s_setup_file.setup.invt.cnt;
															update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.cnt );
															break;
					case FOCUS_SETTING_INVT_TIMEOUT		:	s_setup_file.setup.invt.timeout		= ++s_setup_file.setup.invt.timeout > SET_DEF_INVT_TOUT_MAX ? SET_DEF_INVT_TOUT_MIN : s_setup_file.setup.invt.timeout;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.timeout * 100 );		break;
					case FOCUS_SETTING_INVT_RETRY		:	s_setup_file.setup.invt.retry 		= ++s_setup_file.setup.invt.retry > SET_DEF_INVT_RETRY_MAX ? SET_DEF_INVT_RETRY_MIN : s_setup_file.setup.invt.retry;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.retry );				break;
					case FOCUS_SETTING_INVT_DELAY_READ	:	s_setup_file.setup.invt.delay_read 	= s_setup_file.setup.invt.delay_read+100 > SET_DEF_INVT_RDELAY_MAX ? SET_DEF_INVT_RDELAY_MIN : s_setup_file.setup.invt.delay_read+100;		update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.delay_read);		break;
					case FOCUS_SETTING_INVT_DELAY_WRITE	:	s_setup_file.setup.invt.delay_write	= s_setup_file.setup.invt.delay_write+100 > SET_DEF_INVT_WDELAY_MAX ? SET_DEF_INVT_WDELAY_MIN : s_setup_file.setup.invt.delay_write+100;		update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.invt.delay_write);	break;
					case FOCUS_SETTING_AUTO_LOCKING		:	s_setup_file.setup.auto_lock		= ++s_setup_file.setup.auto_lock  > AUTO_LOCK_TIME_MAX ? 0  : s_setup_file.setup.auto_lock;		update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.auto_lock);	break;
					case FOCUS_SETTING_OPTICAL_SENSOR	:	s_setup_file.setup.optical_sensor	= !s_setup_file.setup.optical_sensor;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.optical_sensor);	break;
					case FOCUS_SETTING_COMM_TYPE_MASTER	:	s_setup_file.setup.comm_type.master	= ++s_setup_file.setup.comm_type.master >= COMM_MODE_MASTER_CNT ? 0 : s_setup_file.setup.comm_type.master;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.comm_type.master);	break;
					case FOCUS_SETTING_COMM_TYPE_SLAVE	:	s_setup_file.setup.comm_type.slave	= ++s_setup_file.setup.comm_type.slave  >= COMM_MODE_SLAVE_CNT  ? 0 : s_setup_file.setup.comm_type.slave;	update_menu_sub_setting_value(s_idx_setting, s_setup_file.setup.comm_type.slave);	break;
				}
				break;
				
		    case KEY_UP 	:
				{
					int old = s_idx_setting;
					s_idx_setting --;
					if( s_idx_setting < 0 )
						s_idx_setting = menu_max;
					update_menu_sub_setting_focus( old, s_idx_setting );
		    	}
				break;
				
		    case KEY_DOWN 	:
				{
					int old = s_idx_setting;						
					s_idx_setting ++;
					if( s_idx_setting > menu_max )
						s_idx_setting = 0;
					update_menu_sub_setting_focus( old, s_idx_setting );
		    	}
	       		break;

		    case KEY_ESC :
				quit = 1;
			    break;
				
			case KEY_AUTO_QUIT :
				quit = 2;
			    break;
				
		    default :				
				break;
	    }
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}

	return;
}

////////////////////////////////////////////////////////////////////////////////

void print_menu_sub_comm_status( void );

void draw_table_comm_status( TB_UI_POINT pnt, TBUC column, TBUC row, TBUC pp_table_value[][12], TBUC max_len )
{
	int max_len_table_value = 0;
	int margin = 5;
	int i, j, k;
	int idx = 0;

	if( max_len == 0 )
	{
		for( i=0; i<column*row; i++ )
		{
			int len = wstrlen( pp_table_value[i] );
			if( len > max_len_table_value )
			{
				max_len_table_value = len;
			}
		}

		max_len_table_value++;
		margin = 0;
	}
	else
	{
		max_len_table_value = max_len;
	}

	TB_ui_move( pnt.y, pnt.x );
	for( j=0; j<column*2+1; j++ )
	{
		if( (j % 4) == 0 )
		{
			TB_ui_move( pnt.y+j, pnt.x );
			for( i=0; i<row; i++ )
			{
				for( k=0; k<max_len_table_value+margin; k++ )
					printw( "%c", k==0 ? '|' : '=' );
			}
		}
		else if( (j % 2) == 0 )
		{
			TB_ui_move( pnt.y+j, pnt.x );
			for( i=0; i<row; i++ )
			{
				for( k=0; k<max_len_table_value+margin; k++ )
				{
					if( column == 1 )
						printw( "%c", k==0 ? '|' : '=' );
					else
						printw( "%c", k==0 ? '|' : '-' );
				}
			}
		}
		else
		{
			for( i=0; i<row; i++ )
			{
				TB_ui_move( pnt.y+j, pnt.x + ((max_len_table_value+margin) * i) );
				if( margin == 5 )
					printw( "|  %s", pp_table_value[idx++] );
				else if( margin == 0 )
					printw( "|%s", pp_table_value[idx++] );
			}			
		}
		TB_ui_move( pnt.y+j, pnt.x + (max_len_table_value+margin) * row );
		printw("|\n");
	}
}

int common_key_loop( void )
{
	int quit = 0;
	int input;
	
	TB_ui_key_init( TRUE );
	while( quit == 0 )
	{
	    input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

	    switch( input )
	    {		
			case KEY_ESC :
				quit = 1;
				break;
				
			case KEY_AUTO_QUIT :
				quit = 2;
				break;
	    }
	}

	return input;
}

////////////////////////////////////////////////////////////////////////////////

static int s_idx_comm_status = 0;

void op_menu_sub_comm_status_ggw( void )
{
	struct comm_info *p_comm_info = NULL;
	TB_UI_POINT pnt;

	char *dev_name[] = 
	{
		"DER-FRTU-1",
		"DER-FRTU-2",
		"DER-FRTU-3",
	};
	
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );
	printw("%s", TB_resid_get_string(RESID_TLT_COMM_STATUS) );
	
	TB_ui_move( 2, 0 );	printw("%s", TB_resid_get_string(RESID_LBL_COMM_STATUS));

	pnt.x = 0;
	pnt.y = 3;
	int idx = 0;
	TBUC value_comm_status[2*3][12];
	wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[0], wstrlen(dev_name[0]) );	idx++;
	wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[1], wstrlen(dev_name[1]) );	idx++;
	wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[2], wstrlen(dev_name[2]) );	idx++;
	for( int i=0; i<MAX_CONNECT_CNT; i++ )
	{
		p_comm_info = TB_rssi_get_comm_info( 0, i );
		TBUC *str = (p_comm_info->percent == 0) ? "FAIL" : "SUCCESS";
		if( str )
			wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), str, wstrlen(str) );
		idx++;
	}
	draw_table_comm_status( pnt, 2, 3, value_comm_status, 10 );

	////////////////////////////////////////////////////////////////////////////

	TB_ui_move( 9, 0 );	printw("%s", TB_resid_get_string(RESID_LBL_COMM_RATE_DBM));

	pnt.x = 0;
	pnt.y = 10;
	idx = 0;
	TBUC value_rate_dbm[3*3][12];
	wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[0], wstrlen(dev_name[0]) );	idx++;
	wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[1], wstrlen(dev_name[1]) );	idx++;
	wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[2], wstrlen(dev_name[2]) );	idx++;
	for( int i=0; i<MAX_CONNECT_CNT; i++ )
	{
		p_comm_info = TB_rssi_get_comm_info( 0, i );
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%4.1f%%", p_comm_info->rate );		idx++;
	}
	
	for( int i=0; i<MAX_CONNECT_CNT; i++ )
	{
		p_comm_info = TB_rssi_get_comm_info( 0, i );
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%3ddbm", p_comm_info->dbm );	idx ++;
	}
	draw_table_comm_status( pnt, 3, 3, value_rate_dbm, 10 );
	
	printw("\n");
	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();

	////////////////////////////////////////////////////////////////////////////

	common_key_loop();
	print_menu_main();
}

void op_menu_sub_comm_status_terml( void )
{
	struct comm_info *p_comm_info = NULL;
	TB_UI_POINT pnt;

	char *dev_name[] = 
	{
		"GROUP G/W",
		"DER-MID-1",
		"DER-MID-2",
		"DER-MID-3",
	};
	
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );
	printw("%s", TB_resid_get_string(RESID_TLT_COMM_STATUS) );
	
	TB_ui_move( 2, 0 );	printw("%s", TB_resid_get_string(RESID_LBL_COMM_STATUS));
	
	pnt.x = 0;
	pnt.y = 3;
	int idx = 0;
	TB_VOLTAGE vol = TB_setup_get_product_info_voltage();
	if( vol == VOLTAGE_LOW )
	{
		TBUC value_comm_status[2*4][12];

		wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[0], wstrlen(dev_name[0]) );	idx++;
		wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[1], wstrlen(dev_name[1]) );	idx++;
		wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[2], wstrlen(dev_name[2]) );	idx++;
		wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[3], wstrlen(dev_name[3]) );	idx++;
		p_comm_info = TB_rssi_get_comm_info( 1, 0 );
		TBUC *str = (p_comm_info->percent == 0) ? "FAIL" : "SUCCESS";
		if( str )
			wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), str, wstrlen(str) );
		idx++;
		for( int i=0; i<MAX_CONNECT_CNT; i++ )
		{
			p_comm_info = TB_rssi_get_comm_info( 0, i );
			TBUC *str = (p_comm_info->percent == 0) ? "FAIL" : "SUCCESS";
			if( str )
				wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), str, wstrlen(str) );
			idx ++;
		}
		draw_table_comm_status( pnt, 2, 4, value_comm_status, 10 );
	}
	else
	{
		TBUC value_comm_status[2*1][12];
		
		wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), dev_name[0], wstrlen(dev_name[0]) );	idx++;
		p_comm_info = TB_rssi_get_comm_info( 0, 0 );
		TBUC *str = (p_comm_info->percent == 0) ? "FAIL" : "SUCCESS";
		if( str )
			wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), str, wstrlen(str) );
		
		draw_table_comm_status( pnt, 2, 1, value_comm_status, 10 );
	}

	////////////////////////////////////////////////////////////////////////////

	TB_ui_move( 9, 0 );	printw("%s", TB_resid_get_string(RESID_LBL_COMM_RATE_DBM));	

	pnt.x = 0;
	pnt.y = 10;
	if( vol == VOLTAGE_LOW )
	{
		TBUC value_rate_dbm[3*4][12];

		int idx = 0;
		wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[0], wstrlen(dev_name[0]) );	idx++;
		wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[1], wstrlen(dev_name[1]) );	idx++;
		wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[2], wstrlen(dev_name[2]) );	idx++;
		wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[3], wstrlen(dev_name[3]) );	idx++;

		p_comm_info = TB_rssi_get_comm_info( 1, 0 );	//	GGW
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%4.1f%%", p_comm_info->rate );	idx ++;
		for( int i=0; i<MAX_CONNECT_CNT; i++ )			//	MID
		{
			p_comm_info = TB_rssi_get_comm_info( 0, i );
			snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%4.1f%%", p_comm_info->rate );
			idx ++;
		}

		p_comm_info = TB_rssi_get_comm_info( 1, 0 );	//	GGW
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%ddbm", p_comm_info->dbm );	idx ++;
		for( int i=0; i<MAX_CONNECT_CNT; i++ )			//	MID
		{
			p_comm_info = TB_rssi_get_comm_info( 0, i );
			snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%3ddbm", p_comm_info->dbm );
			idx ++;
		}
		draw_table_comm_status( pnt, 3, 4, value_rate_dbm, 10 );
	}
	else
	{
		TBUC value_rate_dbm[3*1][12];

		int idx = 0;
		wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), dev_name[0], wstrlen(dev_name[0]) );	idx++;
		p_comm_info = TB_rssi_get_comm_info( 0, 0 );
		if( p_comm_info )
		{
			snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%4.1f%%", p_comm_info->rate );
			idx ++;
			snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%3ddbm", p_comm_info->dbm );
		}
		draw_table_comm_status( pnt, 3, 1, value_rate_dbm, 10 );
	}
	
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();

	////////////////////////////////////////////////////////////////////////////

	common_key_loop();
	print_menu_main();
}

////////////////////////////////////////////////////////////////////////////////

void update_menu_sub_comm_status_termh( void )
{
	TB_ui_move( 2, 0 );	s_idx_comm_status==0 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_STATUS));
	TB_ui_move( 3, 0 );	s_idx_comm_status==1 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_RATE_DBM));

	TB_ui_refresh();
}

void print_menu_sub_comm_status_termh( void )
{	
	update_menu_sub_comm_status_termh();
	
	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();
}

void op_menu_sub_comm_status_termh_comm_status( void )
{
	int idx = 0;
	struct comm_info *p_comm_info = NULL;
	
	TB_UI_POINT pnt;
	
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_COMM_STATUS) );
	TB_ui_move( 2, 0 );
	TB_ui_attrset( FOCUS_ON ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_STATUS));	
	TB_ui_attrset( FOCUS_OFF );

	pnt.x = 0;
	pnt.y = 3;

	TBUC value_comm_status[1*2][12];
	wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), "GROUP G/W", wstrlen("GROUP G/W") );	idx++;
	p_comm_info = TB_rssi_get_comm_info( 0, 0 );
	TBUC *str = (p_comm_info->percent == 0) ? "FAIL" : "SUCCESS";
	if( str )
		wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), str, wstrlen(str) );
	idx++;
	draw_table_comm_status( pnt, 1, 2, value_comm_status, 10 );

	idx = 0;
	TB_ui_move( 7, 0 );	printw("INVERTER( O:Success, X:Fail )");
	pnt.x = 0;
	pnt.y = 8;
	TBUC value_invt_status[4*20][12];

#if 1
	for( int i=0; i<20; i++ )
	{
		//sprintf( &value_invt_status[idx][0], "%02d", i+1 );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%02d", i+1 );
		idx ++;
	}
	for( int i=0; i<20; i++ )
	{
		TBSC size = TB_rs485_each_info_datasize( i );
		//sprintf( &value_invt_status[idx][0], "%c", size > 0 ? 'O' : 'X' );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%c", size > 0 ? 'O' : 'X' );
		idx ++;
	}
	for( int i=20; i<40; i++ )
	{
		//sprintf( &value_invt_status[idx][0], "%02d", i+1 );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]),  "%02d", i+1 );
		idx ++;
	}
	for( int i=20; i<40; i++ )
	{
		TBSC size = TB_rs485_each_info_datasize( i );
		//sprintf( &value_invt_status[idx][0], "%c", size > 0 ? 'O' : 'X' );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%c", size > 0 ? 'O' : 'X' );
		idx ++;
	}
#else
	for( int i=0; i<20; i++ )
	{
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%02d", i+1 );
		i++;
	}
	for( int i=0; i<20; i++ )
	{
		TBSC size = TB_rs485_each_info_datasize( i );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%c", size > 0 ? 'O' : 'X' );
		i++;
	}
	for( int i=20; i<40; i++ )
	{
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]),  "%02d", i+1 );
		i++;
	}
	for( int i=20; i<40; i++ )
	{
		TBSC size = TB_rs485_each_info_datasize( i );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%c", size > 0 ? 'O' : 'X' );
		i++;
	}
#endif
	draw_table_comm_status( pnt, 4, 20, value_invt_status, 0 );

	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();

	////////////////////////////////////////////////////////////////////////////

	common_key_loop();
	print_menu_sub_comm_status_termh();
	TB_ui_clear( UI_CLEAR_LINE_UNDER );
}

void op_menu_sub_comm_status_termh_rate_dbm( void )
{	
	TB_UI_POINT pnt;

	int idx = 0;
	struct comm_info *p_comm_info = NULL;
	
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_COMM_STATUS) );
	TB_ui_move( 2, 0 );
	TB_ui_attrset( FOCUS_ON ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_RATE_DBM));
	TB_ui_attrset( FOCUS_OFF );

	pnt.x = 0;
	pnt.y = 3;
	TBUC value_rate_dbm[1*3][12];

	wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "GROUP G/W", wstrlen("GROUP G/W") );	idx++;
	p_comm_info = TB_rssi_get_comm_info( 0, 0 );
	if( p_comm_info )
	{
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%4.1f%%", p_comm_info->rate );
		idx ++;
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%3ddbm", p_comm_info->dbm );
	}
	draw_table_comm_status( pnt, 1, 3, value_rate_dbm, 10 );

	TB_ui_move( 7, 0 );	printw("INVERTER ( Unit %% )");
	idx = 0;
	pnt.x = 0;
	pnt.y = 8;
	TBUC value_invt_rate[4*20][12];
	for( int i=0; i<20; i++ )
	{
		snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%02d", i+1 );
		idx ++;
	}
	for( int i=0; i<20; i++ )
	{
		TB_INVT_INFO *info = TB_rs485_each_info( i );
		if( info )
		{
			TBUI rate = (TBUI)(((TBFLT)info->count_success / (TBFLT)info->count_try) * 100);
			if( rate > 0 )
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%d", rate );
			else
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), " - " );
			idx ++;
		}
	}
	for( int i=20; i<40; i++ )
	{
		snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%02d", i+1 );
		idx ++;
	}
	for( int i=20; i<40; i++ )
	{
		TB_INVT_INFO *info = TB_rs485_each_info( i );
		if( info )
		{
			TBUI rate = (TBUI)(((TBFLT)info->count_success / (TBFLT)info->count_try) * 100);
			if( rate > 0 )
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%d", rate );
			else
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), " - " );
			idx ++;
		}
	}		
	
	draw_table_comm_status( pnt, 4, 20, value_invt_rate, 0 );
	
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();

	////////////////////////////////////////////////////////////////////////////

	common_key_loop();
	print_menu_sub_comm_status_termh();
	TB_ui_clear( UI_CLEAR_LINE_UNDER );
}

void op_menu_sub_comm_status_termh( void )
{
	print_menu_sub_comm_status_termh();

	int quit = 0;
	int input;

	s_idx_comm_status = 0;

	TB_ui_key_init( TRUE );
	while( quit == 0 )
	{
	    input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

	    switch( input )
	    {		
			case KEY_UP		:
			case KEY_DOWN	:	s_idx_comm_status = !s_idx_comm_status;
								update_menu_sub_comm_status_termh();			
								break;

		    case '\n' 		:
			case KEY_ENTER 	:	if( s_idx_comm_status == 0 )
									op_menu_sub_comm_status_termh_comm_status();
								else
									op_menu_sub_comm_status_termh_rate_dbm();
								break;
				
			case KEY_ESC 	:	quit = 1;
								break;
				
			case KEY_AUTO_QUIT:	quit = 2;
								break;
	    }
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

void update_menu_sub_comm_status_relay( void )
{
	TB_ui_move( 2, 0 );	s_idx_comm_status==0 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_STATUS));
	TB_ui_move( 3, 0 );	s_idx_comm_status==1 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_RATE_DBM));

	TB_ui_refresh();
}

void print_menu_sub_comm_status_relay( void )
{	
	update_menu_sub_comm_status_relay();
	
	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();
}

void op_menu_sub_comm_status_relay_comm_status( void )
{
	int idx = 0;
	struct comm_info *p_comm_info = NULL;
	
	TB_UI_POINT pnt;
	
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_COMM_STATUS) );
	TB_ui_move( 2, 0 );
	TB_ui_attrset( FOCUS_ON ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_STATUS));	
	TB_ui_attrset( FOCUS_OFF );

	pnt.x = 0;
	pnt.y = 3;

	TBUC value_comm_status[1*2][12];
	wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), "DER-FRTU", wstrlen("DER-FRTU") );	idx++;
	p_comm_info = TB_rssi_get_comm_info( 0, 0 );
	TBUC *str = (p_comm_info->percent == 0) ? "FAIL" : "SUCCESS";
	if( str )
		wstrncpy( value_comm_status[idx], sizeof(value_comm_status[idx]), str, wstrlen(str) );
	idx++;
	draw_table_comm_status( pnt, 1, 2, value_comm_status, 10 );

	idx = 0;
	TB_ui_move( 7, 0 );	printw("INVERTER( O:Success, X:Fail )");
	pnt.x = 0;
	pnt.y = 8;
	TBUC value_invt_status[4*20][12];
	for( int i=0; i<20; i++ )
	{
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%02d", i+1 );
		idx ++;
	}
	for( int i=0; i<20; i++ )
	{
		TBSC size = TB_rs485_each_info_datasize( i );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%c", size > 0 ? 'O' : 'X' );
		idx ++;
	}
	for( int i=20; i<40; i++ )
	{
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%02d", i+1 );
		idx ++;
	}
	for( int i=20; i<40; i++ )
	{
		TBSC size = TB_rs485_each_info_datasize( i );
		snprintf( value_invt_status[idx], sizeof(value_invt_status[idx]), "%c", size > 0 ? 'O' : 'X' );
		idx ++;
	}

	draw_table_comm_status( pnt, 4, 20, value_invt_status, 0 );

	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();

	////////////////////////////////////////////////////////////////////////////

	common_key_loop();
	print_menu_sub_comm_status_relay();
	TB_ui_clear( UI_CLEAR_LINE_UNDER );
}

void op_menu_sub_comm_status_relay_rate_dbm( void )
{	
	TB_UI_POINT pnt;

	int idx = 0;
	struct comm_info *p_comm_info = NULL;
	
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_COMM_STATUS) );
	TB_ui_move( 2, 0 );
	TB_ui_attrset( FOCUS_ON ); 	printw("%s", TB_resid_get_string(RESID_LBL_COMM_RATE_DBM));
	TB_ui_attrset( FOCUS_OFF );

	pnt.x = 0;
	pnt.y = 3;
	TBUC value_rate_dbm[1*3][12];

	wstrncpy( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "DER-FRTU", wstrlen("DER-FRTU") );	idx ++;
	p_comm_info = TB_rssi_get_comm_info( 0, 0 );
	if( p_comm_info )
	{
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%4.1f%%", p_comm_info->rate );
		idx ++;
		snprintf( value_rate_dbm[idx], sizeof(value_rate_dbm[idx]), "%3ddbm", p_comm_info->dbm );
	}
	draw_table_comm_status( pnt, 1, 3, value_rate_dbm, 10 );

	TB_ui_move( 7, 0 );	printw("INVERTER ( Unit %% )");
	idx = 0;
	pnt.x = 0;
	pnt.y = 8;
	TBUC value_invt_rate[4*20][12];
	for( int i=0; i<20; i++ )
	{
		snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%02d", i+1 );
		idx ++;
	}
	for( int i=0; i<20; i++ )
	{
		TB_INVT_INFO *info = TB_rs485_each_info( i );
		if( info )
		{
			TBUI rate = (TBUI)(((TBFLT)info->count_success / (TBFLT)info->count_try) * 100);
			if( rate > 0 )
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%d", rate );
			else
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), " - " );
			idx ++;
		}
	}
	for( int i=20; i<40; i++ )
	{
		snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%02d", i+1 );
		idx ++;
	}
	for( int i=20; i<40; i++ )
	{
		TB_INVT_INFO *info = TB_rs485_each_info( i );
		if( info )
		{
			TBUI rate = (TBUI)(((TBFLT)info->count_success / (TBFLT)info->count_try) * 100);
			if( rate > 0 )
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), "%d", rate );
			else
				snprintf( value_invt_rate[idx], sizeof(value_invt_rate[idx]), " - " );
			idx ++;
		}
	}		
	
	draw_table_comm_status( pnt, 4, 20, value_invt_rate, 0 );
	
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();

	////////////////////////////////////////////////////////////////////////////

	common_key_loop();
	print_menu_sub_comm_status_relay();
	TB_ui_clear( UI_CLEAR_LINE_UNDER );
}

void op_menu_sub_comm_status_relay( void )
{
	print_menu_sub_comm_status_relay();

	int quit = 0;
	int input;

	s_idx_comm_status = 0;

	TB_ui_key_init( TRUE );
	while( quit == 0 )
	{
	    input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

	    switch( input )
	    {		
			case KEY_UP		:
			case KEY_DOWN	:	s_idx_comm_status = !s_idx_comm_status;
								update_menu_sub_comm_status_relay();
								break;

		    case '\n' 		:
			case KEY_ENTER 	:	if( s_idx_comm_status == 0 )
									op_menu_sub_comm_status_relay_comm_status();
								else
									op_menu_sub_comm_status_relay_rate_dbm();
								break;
				
			case KEY_ESC 	:	quit = 1;
								break;
				
			case KEY_AUTO_QUIT:	quit = 2;
								break;
	    }
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

void print_menu_sub_comm_status( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_COMM_STATUS) );
	printw("\n");

	TB_ROLE role = TB_setup_get_role();
	switch( role )
	{
		case ROLE_GRPGW :	op_menu_sub_comm_status_ggw();
							break;
		case ROLE_TERM1 :
		case ROLE_TERM2 :
		case ROLE_TERM3	:	if( TB_setup_get_product_info_voltage() == VOLTAGE_LOW )
								op_menu_sub_comm_status_terml();
							else
								op_menu_sub_comm_status_termh();
							break;
		case ROLE_RELAY1:
		case ROLE_RELAY2:
		case ROLE_RELAY3:	op_menu_sub_comm_status_relay();
							break;
	}
}

void op_menu_sub_comm_status( void )
{
	if( TB_setup_get_role() != ROLE_REPEATER )
	{
		print_menu_sub_comm_status();
	}
}

////////////////////////////////////////////////////////////////////////////////

#define MAX_LOG_LIST_LINE_CNT	10
#define MOVE_PAGE_CNT			1
#define LIST_START_Y			5

static TB_LOG s_setup_log_sys;
static TB_LOG s_setup_log_comm;

typedef struct tagLOG_LIST_INFO
{
	int line;
	int page;
	int total_page;
} TB_LOG_LIST_INFO;

static TB_LOG_LIST_INFO s_log_list_info_sys;
static TB_LOG_LIST_INFO s_log_list_info_comm;

/*
*	오름차순
*/
int sort_log_compare_callback_ascending( const void *p_data1, const void *p_data2 )
{
	if( ((TB_LOGITEM *)p_data1)->t < ((TB_LOGITEM *)p_data2)->t )
		return -1;

	if( ((TB_LOGITEM *)p_data1)->t > ((TB_LOGITEM *)p_data2)->t )
		return 1;

    return 0;
}

/*
*	내림차순
*/
int sort_log_compare_callback_descending( const void *p_data1, const void *p_data2 )
{
	if( ((TB_LOGITEM *)p_data1)->t < ((TB_LOGITEM *)p_data2)->t )
		return 1;

	if( ((TB_LOGITEM *)p_data1)->t > ((TB_LOGITEM *)p_data2)->t )
		return -1;

    return 0;
}

int sort_log( TB_LOG *p_log )
{
	int ret = -1;
	if( p_log )
	{
		qsort( p_log->log, p_log->rb.count, sizeof(TB_LOGITEM), sort_log_compare_callback_descending );
		ret = 0;
	}

	return ret;
}

int reverse_log( TB_LOG *p_log )
{
	int ret = -1;

	if( p_log )
	{
		TB_LOGITEM temp;
		TB_LOGITEM *p_temp1 = NULL;
		TB_LOGITEM *p_temp2 = NULL;

		int count = p_log->rb.count;
		int right = count - 1;

		int idx1, idx2;
		int i;
		for( i=0; i<count/2; i++ )
		{
			idx1 = sizeof(TB_LOGITEM) * i;
			idx2 = sizeof(TB_LOGITEM) * right;

			p_temp1 = (TB_LOGITEM *)&p_log->log[idx1];
			p_temp2 = (TB_LOGITEM *)&p_log->log[idx2];

			wmemcpy( &temp,   sizeof(TB_LOGITEM), p_temp1, sizeof(TB_LOGITEM) );
			wmemcpy( p_temp1, sizeof(TB_LOGITEM), p_temp2, sizeof(TB_LOGITEM) );
			wmemcpy( p_temp2, sizeof(TB_LOGITEM), &temp,   sizeof(TB_LOGITEM) );

			right --;
		}

		ret = 0;
	}

	return ret;
}

void print_log_list_common_info( int page_curr, int page_total )
{
	TB_ui_attrset( FOCUS_OFF );
	TB_ui_move( 16, 30 );

	char page_buf[128] = {0, };
	snprintf( page_buf, sizeof(page_buf)-1, "< %03d / %03d >", page_curr, page_total );
	printw( "%s\n", page_buf );
	
	printw("\n");
	printw("%s          %s\n", TB_resid_get_string(RESID_TIP_LIST_HOME), TB_resid_get_string(RESID_TIP_LIST_END) );
	printw("%s          %s\n", TB_resid_get_string(RESID_TIP_LIST_LEFT), TB_resid_get_string(RESID_TIP_LIST_RIGHT) );
	printw("%s          %s\n", TB_resid_get_string(RESID_TIP_LIST_UP),   TB_resid_get_string(RESID_TIP_LIST_DOWN) );
	printw("%s          %s\n", TB_resid_get_string(RESID_TIP_LIST_ENTER),TB_resid_get_string(RESID_TIP_ESC) );
}

void print_menu_sub_log_list( TB_LOG *p_log, int page_curr, int page_total, int focus )
{
	if( p_log )
	{		
		if( page_curr < page_total )
		{
			int i;
			int idx = 0;
			char buf[128] = {0, };
			TB_LOGITEM *p_item = NULL;

			/*
			*	Clear a List Area
			*/
			for( i=0; i<MAX_LOG_LIST_LINE_CNT; i++ )
			{
				TB_ui_move( LIST_START_Y+i, 0 );
				TB_ui_clear( UI_CLEAR_LINE_END );;
			}
			
			for( i=0; i<MAX_LOG_LIST_LINE_CNT; i++ )
			{
				idx = (page_curr * MAX_LOG_LIST_LINE_CNT) + i;
				if( p_log->rb.count > idx )
				{
					TB_ui_move( LIST_START_Y+i, 0 );
					
					(focus==i) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );

					p_item = (TB_LOGITEM *)((unsigned char *)p_log->rb.data + (idx * sizeof(TB_LOGITEM)) );
					if( p_item )
					{
						TB_log_item_string( p_item, buf, sizeof(buf) );
						printw( "[%04d]%s", idx+1, buf );
					}
				}
			}

			print_log_list_common_info( page_curr+1, page_total );
		}
		else if( page_curr == 0 && page_total == 0 )
		{
			TB_ui_move( 9, 6 );
			TB_ui_attrset( FOCUS_ON );
			printw("[ %s ]", TB_resid_get_string(RESID_LOG_NO_DETAIL_DATA) );

			print_log_list_common_info( page_curr, page_total );
		}

		TB_ui_refresh();
	}
}

void print_menu_sub_log_security_list( void )
{
	print_menu_sub_log_list( &s_setup_log_sys,				\
							 s_log_list_info_sys.page,		\
							 s_log_list_info_sys.total_page,\
							 s_log_list_info_sys.line );
}

void draw_menu_sub_log_security_list_line( int line )
{
	TB_LOG *p_log = &s_setup_log_sys;
	if( p_log )
	{
		TB_LOGITEM *p_item = NULL;
		
		char buf[128] = {0, };
		int idx = (s_log_list_info_sys.page * MAX_LOG_LIST_LINE_CNT) + line;
		if( idx < p_log->rb.count )
		{		
			TB_ui_move( LIST_START_Y+line, 0 );
			
			(s_log_list_info_sys.line==line) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
			idx = (s_log_list_info_sys.page * MAX_LOG_LIST_LINE_CNT) + line;		

			p_item = (TB_LOGITEM *)((unsigned char *)p_log->rb.data + (idx * sizeof(TB_LOGITEM)) );
			if( p_item )
			{
				bzero( buf, sizeof(buf) );
				TB_log_item_string( p_item, buf, sizeof(buf) );
				printw( "[%04d]%s", idx+1, buf );
			}
		}
	}	
}

void update_menu_sub_log_security_list_line( int old, int now )
{
	draw_menu_sub_log_security_list_line( old );
	draw_menu_sub_log_security_list_line( now );

	TB_ui_refresh();
}

void print_menu_sub_log_security( void )
{
	char *p_temp = NULL;
	TB_ui_clear( UI_CLEAR_ALL );
	
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_LOG_SECURITY) );
	printw("\n");
	printw("===========================================================================\n" );
	printw("INDEX      DATA/TIME        ACCOUNT              DATA                DETAIL\n" );
	printw("===========================================================================\n" );

	print_menu_sub_log_security_list();
	
	TB_ui_refresh();
}

void op_menu_sub_log_security_detail( TB_LOGITEM *p_item )
{
	if( p_item )
	{
		TBUC buf[128] = {0, };

		bzero( buf, sizeof(buf) );
		TB_ui_clear( UI_CLEAR_ALL );
		
		TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_LOG_SECU_DETAIL) );
		printw("\n");

		switch( p_item->code )
		{
			case LOG_SECU_TIMESYNC 	:
				{
					TBUC *p_time_str = TB_util_get_datetime_string( p_item->t );
					if( p_time_str )
					{
						TB_ui_move( 2, 0 );
						printw("[%s] %s", TB_resid_get_string(RESID_LOG_DETAIL_TIME_SYNC), p_time_str );
					}
				}
				break;

			case LOG_SECU_WATCHDOG 	:
				{
					TB_WDT_COND cond = *p_item->code_data.data;
					switch( cond  )
					{
						case WDT_COND_CHANGE_CONFIG			:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_1), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_1)) );	break;
						case WDT_COND_WISUN_CONNECTION_FAIL	:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_2), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_2)) );	break;
						case WDT_COND_PING_FAIL				:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_3), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_3)) );	break;
						case WDT_COND_SETUP_REBOOT			:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_4), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_4)) );	break;
						case WDT_COND_EMS_REBOOT			:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_5), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_5)) );	break;
						case WDT_COND_SET_TEST_MODE			:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_6), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_6)) );	break;
						case WDT_COND_COMM_STATE_CHANGED	:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_7), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_WATCHDOG_7)) );	break;
						case WDT_COND_UNKNOWN				:
						default								:	wstrncpy( buf, sizeof(buf), TB_resid_get_string(RESID_LOG_DETAIL_UNKNOWN), wstrlen(TB_resid_get_string(RESID_LOG_DETAIL_UNKNOWN)) );		break;
					}

					TB_ui_move( 2, 0 );
					printw("[WATCHDOG] %s", buf );
				}
				break;

			case LOG_SECU_LAST_WORKING_TIME :
				{
					time_t t;

					wmemcpy( &t, sizeof(t), p_item->code_data.data, sizeof(t) );
					TBUC *time_str = TB_util_get_datetime_string( t );

					TB_ui_move( 2, 0 );
					printw("[%s] %s", TB_resid_get_string(RESID_LOG_DETAIL_WORK_TIME), time_str );
				}
				break;

			case LOG_SECU_SETUP :
				{
					int idx = 0;
					TBUI chg_flag = SETUP_CHG_FLAG_NONE;
					
					wmemcpy( &chg_flag, sizeof(chg_flag), p_item->code_data.data, p_item->code_data.size );

					if( chg_flag & SETUP_CHG_FLAG_SETTING )
					{
						TB_ui_move( 2+idx++, 0 );	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_SETTING) );
					}
					if( chg_flag & SETUP_CHG_FLAG_ROLE )
					{
						TB_ui_move( 2+idx++, 0 );	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_ROLE) );
					}
					if( chg_flag & SETUP_CHG_FLAG_WISUN1 )
					{
						TB_ui_move( 2+idx++, 0 );	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_WISUN_1ST) );
					}
					if( chg_flag & SETUP_CHG_FLAG_WISUN2 )
					{
						TB_ui_move( 2+idx++, 0 );	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_WISUN_2ND) );
					}
					if( chg_flag & SETUP_CHG_FLAG_TIME )
					{
						TB_ui_move( 2+idx++, 0 );	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_TIME) );
					}
					if( chg_flag & SETUP_CHG_FLAG_MASTERKEY )
					{
						TB_ui_move( 2+idx++, 0 );	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_MASTERKEY) );
					}
					if( chg_flag & SETUP_CHG_FLAG_INFO )
					{
						TB_ui_move( 2+idx++, 0 );	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_INFO) );
					}
				}
				break;

			default					:
				TB_ui_move( 3, 8 );
				TB_ui_attrset( FOCUS_ON );
				printw("[ %s ]", TB_resid_get_string(RESID_LOG_NO_DETAIL_DATA) );
				break;
		}

		TB_ui_attrset( FOCUS_OFF );

		printw("\n\n");
		printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );

		TB_ui_refresh();

		////////////////////////////////////////////////////////////////////////

		if( common_key_loop() != KEY_AUTO_QUIT )
		{
			print_menu_sub_log_security();
		}
	}
}

int op_menu_sub_log_security( void )
{
	int i;
	int quit = 0;
	int input;

	bzero( &s_log_list_info_sys, sizeof(s_log_list_info_sys) );	
	
	TB_LOG *p_log = &s_setup_log_sys;

	TB_rb_init( &p_log->rb, (void*)&p_log->log[0], sizeof(p_log->log), sizeof(TB_LOGITEM) );
	if( TB_log_load( DEF_LOG_FILE_SYS, p_log ) == 0 )
	{
		reverse_log( p_log );
		
		s_log_list_info_sys.total_page = p_log->rb.count / MAX_LOG_LIST_LINE_CNT;
		if( (p_log->rb.count % MAX_LOG_LIST_LINE_CNT) != 0 )
			s_log_list_info_sys.total_page ++;
	}
	else
		return -1;

	print_menu_sub_log_security();	

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
	    input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

	    switch( input )
	    {
			case KEY_LEFT 	:
				s_log_list_info_sys.page -= MOVE_PAGE_CNT;
				if( s_log_list_info_sys.page < 0 )
					s_log_list_info_sys.page = 0;

				s_log_list_info_sys.line = 0;
				print_menu_sub_log_security_list();
				break;				
				
			case KEY_RIGHT 	:
				s_log_list_info_sys.page += MOVE_PAGE_CNT;
				if( s_log_list_info_sys.total_page <= s_log_list_info_sys.page )
					s_log_list_info_sys.page = s_log_list_info_sys.total_page-1;

				s_log_list_info_sys.line = 0;
				print_menu_sub_log_security_list();
				break;
				
		    case KEY_UP 	:
				{
					int old = s_log_list_info_sys.line;
					int now = s_log_list_info_sys.line;
					
					if( now > 0 )
					{
						now --;

						int idx = (s_log_list_info_sys.page * MAX_LOG_LIST_LINE_CNT) + now;						
						if( p_log )
						{
							if( idx < p_log->rb.count )
							{
								s_log_list_info_sys.line = now;
								update_menu_sub_log_security_list_line( old, now );
							}
						}
					}
					else
					{
						if( s_log_list_info_sys.page > 0 )
						{
							s_log_list_info_sys.page --;
							s_log_list_info_sys.line = MAX_LOG_LIST_LINE_CNT-1;
							print_menu_sub_log_security_list();
						}
					}
		    	}
				break;
				
		    case KEY_DOWN 	:
				{
					int old = s_log_list_info_sys.line;
					int now = s_log_list_info_sys.line;
					
					if( now < MAX_LOG_LIST_LINE_CNT-1 )
					{
						now ++;

						int idx = (s_log_list_info_sys.page * MAX_LOG_LIST_LINE_CNT) + now;
						if( p_log )
						{							
							if( idx < p_log->rb.count )
							{
								s_log_list_info_sys.line = now;
								update_menu_sub_log_security_list_line( old, now );
							}
						}
					}
					else
					{
						if( s_log_list_info_sys.page < s_log_list_info_sys.total_page )
						{
							s_log_list_info_sys.page ++;
							s_log_list_info_sys.line = 0;
							print_menu_sub_log_security_list();
						}
					}					
		    	}
				break;

		    case KEY_USER_HOME :
				s_log_list_info_sys.page = 0;
				s_log_list_info_sys.line = 0;
				print_menu_sub_log_security_list();
				break;

		    case KEY_USER_END :
				s_log_list_info_sys.page = s_log_list_info_sys.total_page-1;
				s_log_list_info_sys.line = 0;
				print_menu_sub_log_security_list();
				break;

		    case '\n' 		:
			case KEY_ENTER 	:
				{					
					if( p_log )
					{
						int idx = (s_log_list_info_sys.page * MAX_LOG_LIST_LINE_CNT) + s_log_list_info_sys.line;
						if( idx < p_log->rb.count )
						{
							TB_LOGITEM *p_item = (TB_LOGITEM *)((unsigned char *)p_log->rb.data + (idx * sizeof(TB_LOGITEM)) );
							if( TB_log_check_detail_data( p_item ) == TRUE )
							{
								op_menu_sub_log_security_detail( p_item );
							}
						}
					}
				}
				break;

		    case KEY_ESC 		:
				quit = 1;
			    break;

			case KEY_AUTO_QUIT 	:
				quit = 2;
			    break;
	    }
	}

	bzero( &s_setup_log_sys, sizeof(s_setup_log_sys) );

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}

	return quit;
}

////////////////////////////////////////////////////////////////////////////////

enum 
{
	FOCUS_LOG_COMM,
	FOCUS_LOG_SECURITY,

	FOCUS_LOG_MAX
};

void print_menu_sub_history( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_HISTORY) );
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_LOG_COMM));
	(s_idx_history == FOCUS_LOG_COMM) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );			printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_LOG_SECURITY));
	(s_idx_history == FOCUS_LOG_SECURITY)  ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	TB_ui_refresh();
}

void update_menu_sub_history( TBSI old, TBSI now )
{
	if( old != now )
	{
		if( old >= 0 )
		{
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+old, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_LOG_COMM+old));
			TB_ui_attrset( FOCUS_OFF );	printw("%s                                             \n", TB_resid_get_string(RESID_VAL_ENTER));
		}

		if( now >= 0 )
		{		
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+now, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_LOG_COMM+now));
			TB_ui_attrset( FOCUS_ON );	printw("%s                                             \n", TB_resid_get_string(RESID_VAL_ENTER));
		}
		
		TB_ui_refresh();
	}
}

void op_menu_sub_history( void )
{
	int quit = 0;
	int input;
	int focus = s_idx_history;

	print_menu_sub_history();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case KEY_UP :
				{
					TBSI focus_old = focus;
					
					focus = --focus < 0 ? FOCUS_LOG_MAX-1 : focus;
					s_idx_history = focus;
					update_menu_sub_history( focus_old, focus );
				}
				break;
				
			case KEY_DOWN :
				{
					TBSI focus_old = focus;
					
					focus = ++focus > FOCUS_LOG_MAX-1 ? 0 : focus;
					s_idx_history = focus;
					update_menu_sub_history( focus_old, focus );
				}				
				break;

			case '\n' 	:
			case KEY_ENTER :
				{
					int ret;
					if( focus == FOCUS_LOG_COMM )
					{
						ret = op_menu_sub_log_comm();
						if( ret == 2 )
						{
							input = KEY_AUTO_QUIT;
							break;
						}
						else
						{
							print_menu_sub_history();
						}
					}
					else if( focus == FOCUS_LOG_SECURITY )
					{
						ret = op_menu_sub_log_security();
						if( ret == 2 )
						{
							input = KEY_AUTO_QUIT;
							break;
						}
						else
						{
							print_menu_sub_history();
						}
					}
				}
				break;

			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

void print_menu_sub_log_comm_list( void )
{
	print_menu_sub_log_list( &s_setup_log_comm,		\
							 s_log_list_info_comm.page,	\
							 s_log_list_info_comm.total_page,		\
							 s_log_list_info_comm.line );
}

void draw_menu_sub_log_comm_list_line( int line )
{
	TB_LOG *p_log = &s_setup_log_comm;
	if( p_log )
	{
		TB_LOGITEM *p_item = NULL;
		
		char buf[128] = {0, };
		int idx = (s_log_list_info_comm.page * MAX_LOG_LIST_LINE_CNT) + line;
		if( idx < p_log->rb.count )
		{		
			TB_ui_move( LIST_START_Y+line, 0 );
			
			(s_log_list_info_comm.line==line) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
			idx = (s_log_list_info_comm.page * MAX_LOG_LIST_LINE_CNT) + line;			

			p_item = (TB_LOGITEM *)((unsigned char *)p_log->rb.data + (idx * sizeof(TB_LOGITEM)) );
			if( p_item )
			{
				TB_log_item_string( p_item, buf, sizeof(buf) );
				printw( "[%04d]%s\n", idx+1, buf );
			}
		}
	}	
}

void update_menu_sub_log_comm_list_line( int old, int now )
{
	draw_menu_sub_log_comm_list_line( old );
	draw_menu_sub_log_comm_list_line( now );

	TB_ui_refresh();
}

void print_menu_sub_log_comm( void )
{
	char *p_temp = NULL;
	TB_ui_clear( UI_CLEAR_ALL );
	
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_LOG_COMM) );
	printw("\n");
	printw("===========================================================================\n" );
	printw("INDEX      DATA/TIME        ACCOUNT              DATA                DETAIL\n" );
	printw("===========================================================================\n" );

	print_menu_sub_log_comm_list();
	
	TB_ui_refresh();
}

void op_menu_sub_log_comm_detail( TB_LOGITEM *p_item )
{
	if( p_item )
	{
		char *p_temp = NULL;
		TB_ui_clear( UI_CLEAR_ALL );
		
		TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_LOG_COMM_DETAIL) );
		printw("\n");

		switch( p_item->code )
		{
			case LOG_SECU_TIMESYNC 	:
				{
					TBUC *p_time_str = TB_util_get_datetime_string( p_item->t );
					if( p_time_str )
					{
						TB_ui_move( 2, 0 );
						printw("[%s] %s", TB_resid_get_string(RESID_LOG_DETAIL_TIME_SYNC), p_time_str );
					}
				}
				break;

			default					:
				TB_ui_move( 3, 6 );
				TB_ui_attrset( FOCUS_ON );
				printw("[ %s ]", TB_resid_get_string(RESID_LOG_NO_DETAIL_DATA) );
				break;
		}

		TB_ui_attrset( FOCUS_OFF );

		printw("\n\n");
		printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );

		TB_ui_refresh();

		////////////////////////////////////////////////////////////////////////

		if( common_key_loop() != KEY_AUTO_QUIT )
		{
			print_menu_sub_log_comm();
		}

	}
}

int op_menu_sub_log_comm( void )
{
	int i;
	int quit = 0;
	int input;

	bzero( &s_log_list_info_comm, sizeof(s_log_list_info_comm) );

	TB_LOG *p_log = &s_setup_log_comm;

	TB_rb_init( &p_log->rb, (void*)&p_log->log[0], sizeof(p_log->log), sizeof(TB_LOGITEM) );
	if( TB_log_load( DEF_LOG_FILE_COMM, p_log ) == 0 )
	{
		reverse_log( p_log );
		
		s_log_list_info_comm.total_page = p_log->rb.count / MAX_LOG_LIST_LINE_CNT;
		if( (p_log->rb.count % MAX_LOG_LIST_LINE_CNT) != 0 )
			s_log_list_info_comm.total_page ++;
	}
	else
		return -1;

	print_menu_sub_log_comm();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
	    input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

	    switch( input )
	    {
			case KEY_LEFT 	:
				s_log_list_info_comm.page -= MOVE_PAGE_CNT;
				if( s_log_list_info_comm.page < 0 )
					s_log_list_info_comm.page = 0;

				s_log_list_info_comm.line = 0;
				print_menu_sub_log_comm_list();
				break;				
				
			case KEY_RIGHT 	:
				s_log_list_info_comm.page += MOVE_PAGE_CNT;
				if( s_log_list_info_comm.total_page <= s_log_list_info_comm.page )
					s_log_list_info_comm.page = s_log_list_info_comm.total_page-1;

				s_log_list_info_comm.line = 0;
				print_menu_sub_log_comm_list();
				break;
				
		    case KEY_UP 	:
				{
					int old = s_log_list_info_comm.line;
					int now = s_log_list_info_comm.line;
					
					if( now > 0 )
					{
						now --;

						int idx = (s_log_list_info_comm.page * MAX_LOG_LIST_LINE_CNT) + now;
						if( p_log )
						{
							if( idx < p_log->rb.count )
							{
								s_log_list_info_comm.line = now;
								update_menu_sub_log_comm_list_line( old, now );
							}
						}
					}
					else
					{
						if( s_log_list_info_comm.page > 0 )
						{
							s_log_list_info_comm.page --;
							s_log_list_info_comm.line = MAX_LOG_LIST_LINE_CNT-1;
							print_menu_sub_log_comm_list();
						}
					}
		    	}
				break;
				
		    case KEY_DOWN 	:
				{
					int old = s_log_list_info_comm.line;
					int now = s_log_list_info_comm.line;
					
					if( now < MAX_LOG_LIST_LINE_CNT-1 )
					{
						now ++;

						int idx = (s_log_list_info_comm.page * MAX_LOG_LIST_LINE_CNT) + now;
						if( p_log )
						{							
							if( idx < p_log->rb.count )
							{
								s_log_list_info_comm.line = now;
								update_menu_sub_log_comm_list_line( old, now );
							}
						}
					}
					else
					{
						if( s_log_list_info_comm.page < s_log_list_info_comm.total_page )
						{
							s_log_list_info_comm.page ++;
							s_log_list_info_comm.line = 0;
							print_menu_sub_log_comm_list();
						}
					}					
		    	}
				break;

		    case '\n' 		:
			case KEY_ENTER 	:
				{					
					if( p_log )
					{
						int idx = (s_log_list_info_comm.page * MAX_LOG_LIST_LINE_CNT) + s_log_list_info_comm.line;
						if( idx < p_log->rb.count )
						{
							TB_LOGITEM *p_item = (TB_LOGITEM *)((unsigned char *)p_log->rb.data + (idx * sizeof(TB_LOGITEM)) );
							if( p_item && p_item->code_data.type > 0 && p_item->code_data.size > 0 )
							{
								op_menu_sub_log_comm_detail( p_item );
							}
						}
					}
				}
				break;

		    case KEY_ESC 	:
				quit = 1;
			    break;

			case KEY_AUTO_QUIT :
				quit = 2;
			    break;
	    }
	}

	bzero( &s_setup_log_comm, sizeof(s_setup_log_comm) );

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}

	return quit;
}

////////////////////////////////////////////////////////////////////////////////

char time_year_org[5];
char time_month_org[3];
char time_day_org[3];
char time_hour_org[3];
char time_min_org[3];
char time_sec_org[3];

struct datetime
{
	char year[5];
	char month[3];
	char day[3];
	char hour[3];
	char min[3];
	char sec[3];
};

struct datetime datatime_curr;
struct datetime datatime_prev;

static TBSI s_time_value_start_x = 10;
void update_time_focus( TBSI old, TBSI now )
{
	if( old != now )
	{
		TBSI x1 = 		old==0 ? s_time_value_start_x+ 0 :
				 		old==1 ? s_time_value_start_x+ 5 :
				 		old==2 ? s_time_value_start_x+ 8 :
				 		old==3 ? s_time_value_start_x+11 :
				 		old==4 ? s_time_value_start_x+14 :
				 		old==5 ? s_time_value_start_x+17 : s_time_value_start_x;

		TBSI x2 = 		now==0 ? s_time_value_start_x+ 0 :
				 		now==1 ? s_time_value_start_x+ 5 :
				 		now==2 ? s_time_value_start_x+ 8 :
				 		now==3 ? s_time_value_start_x+11 :
				 		now==4 ? s_time_value_start_x+14 :
				 		now==5 ? s_time_value_start_x+17 : s_time_value_start_x;

		char *str1 = 	old==0 ? datatime_curr.year :
					 	old==1 ? datatime_curr.month:
					 	old==2 ? datatime_curr.day  :
					 	old==3 ? datatime_curr.hour :
					 	old==4 ? datatime_curr.min  :
					 	old==5 ? datatime_curr.sec  : datatime_curr.year;
					 
		char *str2 = 	now==0 ? datatime_curr.year :
					 	now==1 ? datatime_curr.month:
					 	now==2 ? datatime_curr.day  :
					 	now==3 ? datatime_curr.hour :
					 	now==4 ? datatime_curr.min  :
					 	now==5 ? datatime_curr.sec  : datatime_curr.year;
					 
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2, x1);	printw("%s", str1);
		TB_ui_attrset( FOCUS_ON );	TB_ui_move(2, x2);	printw("%s", str2);
		TB_ui_refresh();
	}
}

void print_time_info( void )
{
	TB_ui_move(2, s_time_value_start_x+ 0);	s_idx_time == 0 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw( "%s", datatime_curr.year );		TB_ui_attrset( FOCUS_OFF );	printw( "/" );
	TB_ui_move(2, s_time_value_start_x+ 5);	s_idx_time == 1 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw( "%s", datatime_curr.month );	TB_ui_attrset( FOCUS_OFF );	printw( "/" );
	TB_ui_move(2, s_time_value_start_x+ 8);	s_idx_time == 2 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw( "%s", datatime_curr.day );		TB_ui_attrset( FOCUS_OFF );	printw( " " );
	TB_ui_move(2, s_time_value_start_x+11);	s_idx_time == 3 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw( "%s", datatime_curr.hour );		TB_ui_attrset( FOCUS_OFF );	printw( ":" );
	TB_ui_move(2, s_time_value_start_x+14);	s_idx_time == 4 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw( "%s", datatime_curr.min );		TB_ui_attrset( FOCUS_OFF );	printw( ":" );
	TB_ui_move(2, s_time_value_start_x+17);	s_idx_time == 5 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw( "%s", datatime_curr.sec );		TB_ui_attrset( FOCUS_OFF );
}

void update_time_value( int idx_time, int value )
{
	int start = 10;
	
	int x1 =	 	idx_time==0 ? start+ 0 :
			 		idx_time==1 ? start+ 5 :
			 		idx_time==2 ? start+ 8 :
			 		idx_time==3 ? start+11 :
			 		idx_time==4 ? start+14 :
			 		idx_time==5 ? start+17 : start;
	
	char *str1 = 	idx_time==0 ? datatime_curr.year :
				 	idx_time==1 ? datatime_curr.month:
				 	idx_time==2 ? datatime_curr.day  :
				 	idx_time==3 ? datatime_curr.hour :
				 	idx_time==4 ? datatime_curr.min  :
				 	idx_time==5 ? datatime_curr.sec  : datatime_curr.year;

	TB_ui_attrset( FOCUS_ON );
	TB_ui_move( 2, x1 );
	printw( "%s", str1 );
	TB_ui_attrset( FOCUS_OFF );
}

void auto_update_time( void )
{
	static time_t s_time_prev = 0;
	time_t	time_curr = time( NULL );

	if( s_time_prev == 0 )
		s_time_prev = time_curr;
	
	if( s_time_prev != time_curr )
	{
		struct tm tm;			
		gmtime_r( &time_curr, &tm );

		snprintf( datatime_curr.year, sizeof(datatime_curr.year),   "%04d", tm.tm_year+1900 );
		snprintf( datatime_curr.month, sizeof(datatime_curr.month), "%02d", tm.tm_mon+1 );
		snprintf( datatime_curr.day, sizeof(datatime_curr.day),   "%02d", tm.tm_mday );
		snprintf( datatime_curr.hour, sizeof(datatime_curr.hour),  "%02d", tm.tm_hour );
		snprintf( datatime_curr.min, sizeof(datatime_curr.min),   "%02d", tm.tm_min );
		snprintf( datatime_curr.sec, sizeof(datatime_curr.sec),   "%02d", tm.tm_sec );

		print_time_info();
		
		s_time_prev = time_curr;
	}
}

void print_menu_sub_time( void )
{
	time_t t = time( NULL );
	struct tm tm;	
	gmtime_r( &t, &tm );

	snprintf( datatime_curr.year, sizeof(datatime_curr.year),   "%04d", tm.tm_year+1900 );
	snprintf( datatime_curr.month, sizeof(datatime_curr.month), "%02d", tm.tm_mon+1 );
	snprintf( datatime_curr.day, sizeof(datatime_curr.day),   "%02d", tm.tm_mday );
	snprintf( datatime_curr.hour, sizeof(datatime_curr.hour),  "%02d", tm.tm_hour );
	snprintf( datatime_curr.min, sizeof(datatime_curr.min),   "%02d", tm.tm_min );
	snprintf( datatime_curr.sec, sizeof(datatime_curr.sec),   "%02d", tm.tm_sec );

	wmemcpy( &datatime_prev, sizeof(datatime_prev), &datatime_curr, sizeof(datatime_curr) );
	
	TB_ui_attrset( FOCUS_OFF  );
	TB_ui_clear( UI_CLEAR_ALL );
	printw("%s\n", TB_resid_get_string(RESID_TLT_TIME) );
	printw("\n");
	printw("1. Time : ");
	print_time_info();
	printw("\n");
	printw("\n");
	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	printw("\n");
	printw("%s\n", TB_resid_get_string(RESID_TIP_LEFT) );
	printw("%s\n", TB_resid_get_string(RESID_TIP_RIGH) );
	printw("%s\n", TB_resid_get_string(RESID_TIP_UP) );
	printw("%s\n", TB_resid_get_string(RESID_TIP_DOWN) );
}

void op_menu_sub_time( void )
{
	TBSI quit = 0;
	TBSI input;

	print_menu_sub_time();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		//auto_update_time();
		
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case KEY_LEFT :
				{
					TBSI focus_old = s_idx_time;
					s_idx_time = --s_idx_time < 0 ? 5 : s_idx_time;
					update_time_focus( focus_old, s_idx_time );
				}
				break;
			case KEY_RIGHT :
				{
					TBSI focus_old = s_idx_time;
					s_idx_time = ++s_idx_time > 5 ? 0 : s_idx_time;
					update_time_focus( focus_old, s_idx_time );
				}
				break;
			case KEY_UP :
				{					
					TBSI val = -1;
					TBSI max_day;
					switch( s_idx_time )
					{
						case 0 :	val = watoi( datatime_curr.year );
									val = ++val > YEAR_MAX ? YEAR_MIN : val;
									snprintf( datatime_curr.year, sizeof(datatime_curr.year), "%04d", val );
									break;
						case 1 :	val = watoi( datatime_curr.month );
									val = ++val > 12 ? 1 : val;
									snprintf( datatime_curr.month, sizeof(datatime_curr.month), "%02d", val );
									break;
						case 2 :	val = watoi( datatime_curr.day );
									max_day = TB_util_get_lastday_of_month( watoi(datatime_curr.month), watoi(datatime_curr.year) );
									val = ++val > max_day ? 1 : val;
									snprintf( datatime_curr.day, sizeof(datatime_curr.day), "%02d", val );
									break;
						case 3 :	val = watoi( datatime_curr.hour );
									val = ++val > 23 ? 0 : val;
									snprintf( datatime_curr.hour, sizeof(datatime_curr.hour), "%02d", val );
									break;
						case 4 :	val = watoi( datatime_curr.min );
									val = ++val > 59 ? 0 : val;
									snprintf( datatime_curr.min, sizeof(datatime_curr.min), "%02d", val );
									break;
						case 5 :	val = watoi( datatime_curr.sec );
									val = ++val > 59 ? 0 : val;
									snprintf( datatime_curr.sec, sizeof(datatime_curr.sec), "%02d", val );
									break;
					}

					if( val != -1 )
						update_time_value( s_idx_time, val );
				}
				
				break;
			case KEY_DOWN :
				{
					TBSI val = -1;
					TBSI max_day;
					switch( s_idx_time )
					{
						case 0 :	val = watoi( datatime_curr.year );
									val = --val < YEAR_MIN ? YEAR_MAX : val;
									snprintf( datatime_curr.year, sizeof(datatime_curr.year), "%04d", val );
									break;
						case 1 :	val = watoi( datatime_curr.month );
									val = --val < 1 ? 12 : val;
									snprintf( datatime_curr.month, sizeof(datatime_curr.month), "%02d", val );
									break;
						case 2 :	val = watoi( datatime_curr.day );
									max_day = TB_util_get_lastday_of_month( watoi(datatime_curr.month), watoi(datatime_curr.year) );
									val = --val < 1 ? max_day : val;
									snprintf( datatime_curr.day, sizeof(datatime_curr.day), "%02d", val );
									break;
						case 3 :	val = watoi( datatime_curr.hour );
									val = --val < 0 ? 23 : val;
									snprintf( datatime_curr.hour, sizeof(datatime_curr.hour), "%02d", val );
									break;
						case 4 :	val = watoi( datatime_curr.min );
									val = --val < 0 ? 59 : val;
									snprintf( datatime_curr.min, sizeof(datatime_curr.min), "%02d", val );
									break;
						case 5 :	val = watoi( datatime_curr.sec );
									val = --val < 0 ? 59 : val;
									snprintf( datatime_curr.sec, sizeof(datatime_curr.sec), "%02d", val );
									break;
					}

					if( val != -1 )
						update_time_value( s_idx_time, val );
				}
				
				break;

		    case '\n' 	:
			case KEY_ENTER :
				if( memcmp(&datatime_prev, &datatime_curr, sizeof(datatime_curr) ) != 0 )
				{					
					TB_ui_clear( UI_CLEAR_ALL );
					TB_ui_refresh();

					TB_util_set_systemtime1( watoi(datatime_curr.year), watoi(datatime_curr.month), watoi(datatime_curr.day),	\
											 watoi(datatime_curr.hour), watoi(datatime_curr.min),   watoi(datatime_curr.sec) );

					s_setup_chg_flag |= SETUP_CHG_FLAG_TIME;

					print_menu_sub_time();
					quit = 1;
				}
				break;

			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

void print_menu_sub_factory( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );
	printw("%s\n", TB_resid_get_string(RESID_TLT_FACTORY) );
	printw("\n");
	TB_ui_attrset( FOCUS_ON );
	printw("%s\n", TB_resid_get_string(RESID_LBL_FACTORY) );
	TB_ui_attrset( FOCUS_OFF );
	printw("\n");
	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	TB_ui_refresh();
}

void op_menu_sub_factory( void )
{
	TBSI quit = 0;
	TBSI input;

	print_menu_sub_factory();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
		    case '\n' 	:
			case KEY_ENTER :
				TB_ui_clear( UI_CLEAR_ALL );
				TB_ui_refresh();
				
				if( TB_setup_set_default() != 0 )
					TB_log_append( LOG_SECU_FACTORY_FAIL, NULL, s_login_type );
				else
					TB_log_append( LOG_SECU_FACTORY, NULL, s_login_type );

				sleep( 1 );

				print_menu_sub_factory();
				break;

			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

void print_menu_sub_fwupdate( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );
	printw("%s\n", TB_resid_get_string(RESID_TLT_DOWNLOAD) );
	printw("\n");
	TB_ui_attrset( FOCUS_ON );
	printw("%s\n", TB_resid_get_string(RESID_LBL_DOWNLOAD) );
	TB_ui_attrset( FOCUS_OFF );
	printw("\n");
	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	TB_ui_refresh();
}

int run_fwupdate( void )
{
	int is_error = 0;
	
	char curr_path[128];
	char move_path[128];
	if( getcwd(curr_path, sizeof(curr_path)) != NULL )
	{
		char command[128];
		
		TB_ui_clear( UI_CLEAR_ALL );
		TB_ui_refresh();
		
		/*******************************************************
		*	0. Change Working Directory : to /root
		*******************************************************/
		printf( "[%s:%d] Current Working Dir : %s\r\n", __FILE__, __LINE__, curr_path );
		chdir( "/root" );
		getcwd( move_path, sizeof(move_path) );
		printf( "[%s:%d] Current Working Dir : %s\r\n", __FILE__, __LINE__, move_path );

		/*******************************************************
		*	1. 이전의 압축 파일 제거
		*******************************************************/
		if( access(KDN_TAR_FILE_PATH, F_OK) == 0 )
		{
			snprintf( command, sizeof(command), "rm -rf %s", KDN_TAR_FILE_PATH );
			printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
			system( command ); printf("\r\n");
		}
		
		/*******************************************************
		*	1. KDN_FILE_NAME 파일 유무 검사
		*	2. 임시파일로 이름 변경
		*	   kdn 		--> kdn_org
		*	   kdn_hash --> kdn_hash_org
		*******************************************************/
		if( access(KDN_FILE_PATH, F_OK) == 0 )
		{
			if( access(KDN_ORG_FILE_NAME, F_OK) == 0 )
			{
				snprintf( command, sizeof(command), "rm -rf %s", KDN_ORG_FILE_NAME );
				printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
				system( command ); printf("\r\n");
			}
			
			snprintf( command, sizeof(command), "mv %s %s", KDN_FILE_NAME, KDN_ORG_FILE_NAME );
			printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
			system( command ); printf("\r\n");
		}
		
		if( access(KDN_HASH_FILE_PATH, F_OK) == 0 )
		{
			if( access(KDN_HASH_ORG_FILE_NAME, F_OK) == 0 )
			{
				snprintf( command, sizeof(command), "rm -rf %s", KDN_HASH_ORG_FILE_NAME );
				printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
				system( command ); printf("\r\n");
			}
			
			snprintf( command, sizeof(command), "mv %s %s", KDN_HASH_FILE_NAME, KDN_HASH_ORG_FILE_NAME );
			printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
			system( command ); printf("\r\n");
		}
		
		/*******************************************************
		*	1. Download update file( kdn.tar )
		*******************************************************/
		snprintf( command, sizeof(command), "%s -v -b -Z", SERIAL_COMM_RECV );
		printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
		system( command ); printf("\r\n");

		if( access(KDN_GZIP_FILE_NAME, F_OK) == 0 )
		{
			/*******************************************************
			*	1. 압축 해제
			*******************************************************/
			snprintf( command, sizeof(command), "gzip -d %s", KDN_GZIP_FILE_NAME );
			printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
			system( command ); printf("\r\n");

			snprintf( command, sizeof(command), "tar xvf %s", KDN_TAR_FILE_NAME );
			printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
			system( command ); printf("\r\n");

			/*******************************************************
			*	1. 압축 해제 파일 검사
			*******************************************************/
			if( access(KDN_FILE_NAME, F_OK) == 0 && access(KDN_HASH_FILE_NAME, F_OK) == 0 )
			{
				/*******************************************************
				*	1. 신규 kdn 파일의 md5 hash 생성
				*******************************************************/
				snprintf( command, sizeof(command), "md5sum %s > %s", KDN_FILE_NAME, KDN_HASH_FILE_NAME_CHK );
				printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
				system( command ); printf("\r\n");

				/*******************************************************************
				*	1. 신규 kdn 파일의 md5 hash file과
				*		Download된 md5 hash file과 비교 -> 결과 파일 생성 : kdn_hash_chk
				*		md5 hash 값이 서로 동일하다면 kdn_hash_chk 파일의 크기가 0이다.
				*******************************************************************/
				snprintf( command, sizeof(command), "cmp %s %s > %s", KDN_HASH_FILE_NAME, KDN_HASH_FILE_NAME_CHK, HASH_COMPARE_FILE_NAME );
				printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
				system( command ); printf("\r\n");

				if( TB_util_get_file_size( HASH_COMPARE_FILE_NAME ) == 0 )
				{
					printf( "[%s:%d] Compare file is equal.\r\n", __FILE__, __LINE__ );
					
					/***********************************************************
					*	HASH_COMPARE_FILE_NAME 파일 제거
					***********************************************************/
					snprintf( command, sizeof(command), "rm -rf %s", HASH_COMPARE_FILE_NAME );
					printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
					system( command ); printf("\r\n");
					
					/***********************************************************
					*	KDN_FILE_PATH 권한 변경
					***********************************************************/
					snprintf( command, sizeof(command), "chmod 777 %s", KDN_FILE_PATH );
					printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
					system( command ); printf("\r\n");

					/***********************************************************
					*	KDN_ORG_FILE이 없으면 신규 생성
					***********************************************************/
					if( access(KDN_ORG_FILE_PATH, F_OK) != 0 )
					{
						snprintf( command, sizeof(command), "cp %s %s", KDN_FILE_NAME, KDN_ORG_FILE_NAME );
						printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
						system( command ); printf("\r\n");
					}
					
					/***********************************************************
					*	KDN_ORG_HASH_FILE이 없으면 신규 생성
					***********************************************************/
					if( access(KDN_HASH_ORG_FILE_PATH, F_OK) != 0 )
					{
						snprintf( command, sizeof(command), "cp %s %s", KDN_HASH_FILE_NAME, KDN_HASH_ORG_FILE_NAME );
						printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
						system( command ); printf("\r\n");
					}
				}
				else
				{
					is_error = 1;
				}
			}
			else
			{
				is_error = 1;
			}
		}
		else
		{
			is_error = 1;
		}

		/***************************************************************
		*	1. tar 파일 제거
		***************************************************************/
		snprintf( command, sizeof(command), "rm -rf %s", KDN_TAR_FILE_PATH );
		printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
		system( command ); printf("\r\n");
		
		snprintf( command, sizeof(command), "rm -rf %s", KDN_HASH_FILE_NAME_CHK );
		printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
		system( command ); printf("\r\n");

		////////////////////////////////////////////////////////////////////////

		if( is_error )
		{
			/***************************************************************
			*	1. 이전 backup file을 원래의 파일 이름으로 변경
			*		kdn_org 	 --> kdn
			*		kdn_hash_org --> kdn_hash
			***************************************************************/
			printf( "[%s:%d] FW Update ERROR\r\n", __FILE__, __LINE__ );

			if( access(KDN_FILE_NAME, F_OK) == 0 )
			{
				snprintf( command, sizeof(command), "rm -rf %s", KDN_FILE_NAME );
				printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
				system( command ); printf("\r\n");
			}

			snprintf( command, sizeof(command), "mv %s %s", KDN_ORG_FILE_NAME, KDN_FILE_NAME );
			printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
			system( command ); printf("\r\n");

			////////////////////////////////////////////////////////////////////

			if( access(KDN_HASH_FILE_NAME, F_OK) == 0 )
			{
				snprintf( command, sizeof(command), "rm -rf %s", KDN_HASH_FILE_NAME );
				printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
				system( command ); printf("\r\n");
			}
			
			snprintf( command, sizeof(command), "mv %s %s", KDN_HASH_ORG_FILE_NAME, KDN_HASH_FILE_NAME );
			printf( "[%s:%d] command : %s\r\n", __FILE__, __LINE__, command );
			system( command ); printf("\r\n");
		}
		else
		{
			printf( "[%s:%d] F/W Update Complete\r\n", __FILE__, __LINE__ );
		}
		
		chdir( curr_path );
		getcwd( curr_path, sizeof(curr_path) );
		printf( "[%s:%d] Current Working Dir : %s\r\n", __FILE__, __LINE__, curr_path );

		sleep( 3 );
		
		print_menu_sub_fwupdate();
	}

	return is_error ? -1 : 0;
}

void op_menu_sub_fwupdate( void )
{
	TBSI quit = 0;
	TBSI input;

	print_menu_sub_fwupdate();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
		    case '\n' 	:
			case KEY_ENTER :
				if( run_fwupdate() != 0 )
				{
					TB_log_append( LOG_SECU_FWUPDATE_FAIL, NULL, s_login_type );
				}
				else
				{
					TB_log_append( LOG_SECU_FWUPDATE, NULL, s_login_type );
					TB_setup_save();

					TB_show_msgbox( TB_msgid_get_string(MSGID_REBOOT), 5, FALSE );
					TB_wdt_reboot( WDT_COND_CHANGE_CONFIG );
				}
				break;
				
			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

void op_menu_sub_reboot( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_refresh();
	TB_setup_save();

	if( TB_show_msgbox( TB_msgid_get_string(MSGID_REBOOT_CANCELABLE), 5, TRUE ) != 0 )
	{
		TB_wdt_reboot( WDT_COND_SETUP_REBOOT );
	}
	else
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

enum 
{
	FOCUS_MASTERKEY_EDIT,
	FOCUS_MASTERKEY_INFO,

	FOCUS_MASTERKEY_MAX
};

void print_menu_sub_masterkey( void )
{
	int focus = s_idx_kcmvp;
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_MASTERKEY) );
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_EDIT)); 	focus==FOCUS_MASTERKEY_EDIT ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_LIBINFO)); 	focus==FOCUS_MASTERKEY_INFO ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));

	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	TB_ui_refresh();
}

void update_menu_sub_masterkey( TBSI old, TBSI now )
{
	if( old != now )
	{
		if( old >= 0 )
		{
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+old, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_EDIT+old));
			TB_ui_attrset( FOCUS_OFF );
			switch( old )
			{
				case FOCUS_MASTERKEY_EDIT	: 
				case FOCUS_MASTERKEY_INFO	: printw("%s                                          \n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
			}
		}

		if( now >= 0 )
		{		
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+now, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_EDIT+now));
			TB_ui_attrset( FOCUS_ON );
			switch( now )
			{
				case FOCUS_MASTERKEY_EDIT	: 
				case FOCUS_MASTERKEY_INFO	: printw("%s                                          \n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
			}
		}
		
		TB_ui_refresh();
	}
}

void op_menu_sub_masterkey( void )
{
	TBSI quit = 0;
	TBSI input;
	int focus = s_idx_kcmvp;	

	print_menu_sub_masterkey();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case KEY_UP :
				{
					TBSI focus_old = focus;
					
					focus = --focus < 0 ? FOCUS_MASTERKEY_MAX-1 : focus;
					s_idx_kcmvp = focus;
					update_menu_sub_masterkey( focus_old, focus );
				}
				break;
				
			case KEY_DOWN :
				{
					TBSI focus_old = focus;
					
					focus = ++focus > FOCUS_MASTERKEY_MAX-1 ? 0 : focus;
					s_idx_kcmvp = focus;
					update_menu_sub_masterkey( focus_old, focus );
				}				
				break;

			case '\n' 	:
			case KEY_ENTER :
				if( focus == FOCUS_MASTERKEY_EDIT )
				{
					op_menu_sub_masterkey_edit();
				}
				else if( focus == FOCUS_MASTERKEY_INFO )
				{
					op_menu_sub_masterkey_info();
				}
				break;

			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

enum 
{
	FOCUS_PASSWORD_ADMIN,
	FOCUS_PASSWORD_USER,

	FOCUS_PASSWORD_MAX
};

void update_menu_sub_admin_setting_pw_change( TBSI old, TBSI now )
{
	if( old != now )
	{
		if( old >= 0 )
		{
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+old, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_ACCOUNT_ADMIN+old));
			TB_ui_attrset( FOCUS_OFF );	printw("%s                                             \n", TB_resid_get_string(RESID_VAL_ENTER));
		}

		if( now >= 0 )
		{		
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+now, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_ACCOUNT_ADMIN+now));
			TB_ui_attrset( FOCUS_ON );	printw("%s                                             \n", TB_resid_get_string(RESID_VAL_ENTER));
		}
		
		TB_ui_refresh();
	}
}

void print_menu_sub_admin_setting_pw_change( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_PW_CHANGE) );
	printw("\n");
	TB_ui_attrset( FOCUS_OFF ); TB_ui_move(2, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_ACCOUNT_ADMIN));
	(s_idx_pw_change==FOCUS_PASSWORD_ADMIN) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s", TB_resid_get_string(RESID_VAL_ENTER));	
	TB_ui_attrset( FOCUS_OFF ); TB_ui_move(3, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_ACCOUNT_USER));
	(s_idx_pw_change==FOCUS_PASSWORD_USER) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s", TB_resid_get_string(RESID_VAL_ENTER));
	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	TB_ui_refresh();
}

int op_menu_sub_admin_setting_pw_change( void )
{
	int quit = 0;
	int input;
	int focus = s_idx_pw_change = 0;

	print_menu_sub_admin_setting_pw_change();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case KEY_UP :
				{
					TBSI focus_old = focus;
					
					focus = --focus < 0 ? FOCUS_PASSWORD_MAX-1 : focus;
					s_idx_pw_change = focus;
					update_menu_sub_admin_setting_pw_change( focus_old, focus );
				}
				break;
				
			case KEY_DOWN :
				{
					TBSI focus_old = focus;
					
					focus = ++focus > FOCUS_PASSWORD_MAX-1 ? 0 : focus;
					s_idx_pw_change = focus;
					update_menu_sub_admin_setting_pw_change( focus_old, focus );
				}
				break;

			case '\n' 	:
			case KEY_ENTER :
				{
					int ret;
					if( focus == FOCUS_PASSWORD_ADMIN )
					{
						ret = TB_login_show_change_pw( ACCOUNT_TYPE_ADMIN, FALSE );
						if( ret == 2 )
						{
							input = KEY_AUTO_QUIT;
						}
						else
						{
							print_menu_sub_admin_setting_pw_change();
						}
					}
					else if( focus == FOCUS_PASSWORD_USER )
					{
						ret = TB_login_show_change_pw( ACCOUNT_TYPE_USER, FALSE );
						if( ret == 2 )
						{
							input = KEY_AUTO_QUIT;
						}
						else
						{
							print_menu_sub_admin_setting_pw_change();
						}
					}
				}
				break;

			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_sub_admin_setting();
	}

	return input;
}

#define USE_TEST_DEBUG_MODE

static int 	s_admin_ems_val_startx = 0;
enum 
{
	FOCUS_ADMIN_SET_PW,
	FOCUS_ADMIN_CLR_PW,
#ifdef	USE_TEST_DEBUG_MODE
	FOCUS_ADMIN_DEBUG,
#endif

	FOCUS_ADMIN_MAX
};

TBUC *debug_menu[] = 
{
	"[ ] J11     ",	"[ ] WISUN   ",
	"[ ] MODEM   ",	"[ ] PSU	 ",
	"[ ] PACKET  ",	"[ ] INVERTER",
	"[ ] LOG     ", "[ ] OPTICAL ",
	"[ ] KCMVP   ",	"[ ] PING    ",	
	"[ ] ACCOUNT ",	"[ ] RSSI    ",
	"[ ] EMS     ",	"[ ] FRTU    ",
	"[ ] NETQ    ", "[ ] MSGQ    ",
	"[ ] --------- ALL ---------",
};

enum
{
	IDX_DBG_J11,
	IDX_DBG_WISUN,
	IDX_DBG_MODEM,
	IDX_DBG_PSU,
	IDX_DBG_PACKET,
	IDX_DBG_INVERTER,
	IDX_DBG_LOG,
	IDX_DBG_OPTICAL,
	IDX_DBG_KCMVP,
	IDX_DBG_PING,
	IDX_DBG_ACCOUNT,
	IDX_DBG_RSSI,
	IDX_DBG_EMS,
	IDX_DBG_FRTU,
	IDX_DBG_NETQ,
	IDX_DBG_MSGQ,
	IDX_DBG_ALL,

	IDX_DBG_MAX
};

TBUC *sp_dbg_flag = NULL;
TBUC dbg_flag_all = FALSE;
int s_idx_admin_debug = 0;

void print_debug_menu_mark( int idx )
{
	switch( idx )
	{
		case IDX_DBG_J11 		: printw("%c", sp_dbg_flag[DMID_J11] 		? 'O' : 'X' );		break;
		case IDX_DBG_WISUN 		: printw("%c", sp_dbg_flag[DMID_WISUN] 		? 'O' : 'X' );		break;
		case IDX_DBG_MODEM 		: printw("%c", sp_dbg_flag[DMID_MODEM] 		? 'O' : 'X' );		break;
		case IDX_DBG_PSU 		: printw("%c", sp_dbg_flag[DMID_PSU] 		? 'O' : 'X' );		break;
		case IDX_DBG_PACKET 	: printw("%c", sp_dbg_flag[DMID_PACKET] 	? 'O' : 'X' );		break;
		case IDX_DBG_INVERTER 	: printw("%c", sp_dbg_flag[DMID_INVERTER] 	? 'O' : 'X' );		break;
		case IDX_DBG_LOG 		: printw("%c", sp_dbg_flag[DMID_LOG] 		? 'O' : 'X' );		break;
		case IDX_DBG_OPTICAL 	: printw("%c", sp_dbg_flag[DMID_OPTICAL] 	? 'O' : 'X' );		break;
		case IDX_DBG_KCMVP 		: printw("%c", sp_dbg_flag[DMID_KCMVP] 		? 'O' : 'X' );		break;
		case IDX_DBG_PING 		: printw("%c", sp_dbg_flag[DMID_PING] 		? 'O' : 'X' );		break;
		case IDX_DBG_ACCOUNT 	: printw("%c", sp_dbg_flag[DMID_LOGIN] 		? 'O' : 'X' );		break;
		case IDX_DBG_RSSI 		: printw("%c", sp_dbg_flag[DMID_RSSI] 		? 'O' : 'X' );		break;
		case IDX_DBG_EMS 		: printw("%c", sp_dbg_flag[DMID_EMS] 		? 'O' : 'X' );		break;
		case IDX_DBG_FRTU 		: printw("%c", sp_dbg_flag[DMID_FRTU] 		? 'O' : 'X' );		break;
		case IDX_DBG_NETQ 		: printw("%c", sp_dbg_flag[DMID_NETQ] 		? 'O' : 'X' );		break;
		case IDX_DBG_MSGQ 		: printw("%c", sp_dbg_flag[DMID_MSGQ] 		? 'O' : 'X' );		break;
		case IDX_DBG_ALL 		: printw("%c", dbg_flag_all 				? 'O' : 'X' );		break;
	}
}

void print_menu_sub_admin_debug( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_DEBUG_MODE) );
	printw("\n");

	int y, x;
	for( int i=0; i<IDX_DBG_MAX; i++ )
	{
		(s_idx_admin_debug == i) ? TB_ui_attrset(FOCUS_ON) : TB_ui_attrset(FOCUS_OFF);

		y = 2 + (i/2);
		x = ( i % 2 == 0 ) ? 2 : 20;

		TB_ui_move( y, x );		printw("%s", debug_menu[i] );
		TB_ui_move( y, x+1 );	print_debug_menu_mark( i );
		
		if( i == IDX_DBG_ALL )
			 TB_ui_move( y, 40 );
	}

	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	TB_ui_refresh();
}

void update_menu_sub_admin_debug( TBSI old, TBSI now )
{
	if( old != now )
	{
		int y, x;
		if( old >= 0 )
		{
			TB_ui_attrset( FOCUS_OFF );
			
			y = 2 + (old / 2);
			x = ( (old % 2) == 0 ) ? 2 : 20;

			TB_ui_move( y, x );		printw("%s", debug_menu[old] );
			TB_ui_move( y, x+1 );	print_debug_menu_mark( old );
		}

		if( now >= 0 )
		{
			TB_ui_attrset( FOCUS_ON );

			y = 2 + (now / 2);
			x = ( (now % 2) == 0 ) ? 2 : 20;

			TB_ui_move( y, x );		printw("%s", debug_menu[now] );
			TB_ui_move( y, x+1 );	print_debug_menu_mark( now );
		}
		
		TB_ui_refresh();
	}
}

void op_menu_sub_admin_debug( void )
{
	int quit = 0;
	int input;

	/*
	*	환경설정에 진입하기 전에 모드 디버그 모드가 FALSE가 되고,
	*	기존 디버그 모드값을 백업한다. 이 백업된 값을 변경하여 환경설정을
	*	빠져나갈 때 변경된 디버그 모드가 동작하도혹 한다.
	*/
	sp_dbg_flag = TB_dm_dbg_get_mode_backup();

	print_menu_sub_admin_debug();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case '\n' 	:
			case KEY_ENTER :
				{
					switch( s_idx_admin_debug )
					{
						case IDX_DBG_J11 		: sp_dbg_flag[DMID_J11] 	= !sp_dbg_flag[DMID_J11];		break;
						case IDX_DBG_WISUN 		: sp_dbg_flag[DMID_WISUN] 	= !sp_dbg_flag[DMID_WISUN];		break;
						case IDX_DBG_MODEM 		: sp_dbg_flag[DMID_MODEM] 	= !sp_dbg_flag[DMID_MODEM];		break;
						case IDX_DBG_PSU 		: sp_dbg_flag[DMID_PSU] 	= !sp_dbg_flag[DMID_PSU];		break;
						case IDX_DBG_PACKET 	: sp_dbg_flag[DMID_PACKET] 	= !sp_dbg_flag[DMID_PACKET];	break;
						case IDX_DBG_INVERTER 	: sp_dbg_flag[DMID_INVERTER]= !sp_dbg_flag[DMID_INVERTER];	break;
						case IDX_DBG_LOG 		: sp_dbg_flag[DMID_LOG] 	= !sp_dbg_flag[DMID_LOG];		break;
						case IDX_DBG_OPTICAL 	: sp_dbg_flag[DMID_OPTICAL] = !sp_dbg_flag[DMID_OPTICAL];	break;
						case IDX_DBG_KCMVP 		: sp_dbg_flag[DMID_KCMVP] 	= !sp_dbg_flag[DMID_KCMVP];		break;
						case IDX_DBG_PING 		: sp_dbg_flag[DMID_PING] 	= !sp_dbg_flag[DMID_PING];		break;
						case IDX_DBG_ACCOUNT 	: sp_dbg_flag[DMID_LOGIN] 	= !sp_dbg_flag[DMID_LOGIN];		break;
						case IDX_DBG_RSSI 		: sp_dbg_flag[DMID_RSSI] 	= !sp_dbg_flag[DMID_RSSI];		break;
						case IDX_DBG_EMS 		: sp_dbg_flag[DMID_EMS] 	= !sp_dbg_flag[DMID_EMS];		break;
						case IDX_DBG_FRTU 		: sp_dbg_flag[DMID_FRTU] 	= !sp_dbg_flag[DMID_FRTU];		break;
						case IDX_DBG_NETQ 		: sp_dbg_flag[DMID_NETQ] 	= !sp_dbg_flag[DMID_NETQ];		break;
						case IDX_DBG_MSGQ 		: sp_dbg_flag[DMID_MSGQ] 	= !sp_dbg_flag[DMID_MSGQ];		break;
						case IDX_DBG_ALL		: dbg_flag_all				= !dbg_flag_all;
												  sp_dbg_flag[DMID_J11]     = dbg_flag_all;
												  sp_dbg_flag[DMID_WISUN] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_MODEM] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_PSU] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_PACKET] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_INVERTER]= dbg_flag_all;
												  sp_dbg_flag[DMID_LOG] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_OPTICAL]	= dbg_flag_all;
												  sp_dbg_flag[DMID_KCMVP] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_PING] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_LOGIN] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_RSSI] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_EMS] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_FRTU] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_NETQ] 	= dbg_flag_all;
												  sp_dbg_flag[DMID_MSGQ] 	= dbg_flag_all;

												  print_menu_sub_admin_debug();								break;
					}

					update_menu_sub_admin_debug( -1, s_idx_admin_debug );
				}
				break;

			case KEY_LEFT :
				{
					TBSI focus_old = s_idx_admin_debug;
					s_idx_admin_debug = --s_idx_admin_debug < 0 ? IDX_DBG_MAX-1 : s_idx_admin_debug;
					update_menu_sub_admin_debug( focus_old, s_idx_admin_debug );
				}
				break;
				
			case KEY_RIGHT :
				{
					TBSI focus_old = s_idx_admin_debug;
					s_idx_admin_debug = ++s_idx_admin_debug > IDX_DBG_MAX-1 ? 0 : s_idx_admin_debug;
					update_menu_sub_admin_debug( focus_old, s_idx_admin_debug );
				}
				break;
				
			case KEY_UP   :
				{
					TBSI focus_old = s_idx_admin_debug;
					s_idx_admin_debug = (s_idx_admin_debug-2) < 0 ? IDX_DBG_MAX-1 : (s_idx_admin_debug-2);
					update_menu_sub_admin_debug( focus_old, s_idx_admin_debug );
				}			
				break;
				
			case KEY_DOWN  :
				{
					TBSI focus_old = s_idx_admin_debug;
					s_idx_admin_debug = (s_idx_admin_debug+2) > IDX_DBG_MAX-1 ? 0 : (s_idx_admin_debug+2);
					update_menu_sub_admin_debug( focus_old, s_idx_admin_debug );
				}
				break;

			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_sub_admin_setting();
	}
}

void print_menu_sub_admin_setting( void )
{
	s_admin_ems_val_startx = wstrlen( TB_resid_get_string(RESID_LBL_ACCOUNT_SETTING) ) + 3;
	
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_ADMIN_SETTING) );
	printw("\n");

	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_ACCOUNT_SETTING));
	TB_ui_move( 2, s_admin_ems_val_startx );
	( s_idx_admin == FOCUS_ADMIN_SET_PW ) ? TB_ui_attrset(FOCUS_ON) : TB_ui_attrset(FOCUS_OFF);
	printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));

	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_ACCOUNT_CLEAR));
	TB_ui_move( 3, s_admin_ems_val_startx );
	( s_idx_admin == FOCUS_ADMIN_CLR_PW ) ? TB_ui_attrset(FOCUS_ON) : TB_ui_attrset(FOCUS_OFF);
	printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));

#ifdef	USE_TEST_DEBUG_MODE
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_DEBUG_MODE));
	TB_ui_move( 4, s_admin_ems_val_startx );
	( s_idx_admin == FOCUS_ADMIN_DEBUG ) ? TB_ui_attrset(FOCUS_ON) : TB_ui_attrset(FOCUS_OFF);
	printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));
#endif

	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	TB_ui_refresh();
}

void update_menu_sub_admin( TBSI old, TBSI now )
{
	if( old != now )
	{
		if( old >= 0 )
		{
			TB_ui_attrset( FOCUS_OFF );
			TB_ui_move(2+old, s_admin_ems_val_startx);
			switch( old )
			{
				case FOCUS_ADMIN_SET_PW		: printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
				case FOCUS_ADMIN_CLR_PW		: printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
#ifdef	USE_TEST_DEBUG_MODE
				case FOCUS_ADMIN_DEBUG		: printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
#endif
			}
		}

		if( now >= 0 )
		{		
			TB_ui_attrset( FOCUS_ON );
			TB_ui_move(2+now, s_admin_ems_val_startx);
			switch( now )
			{
				case FOCUS_ADMIN_SET_PW		: printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
				case FOCUS_ADMIN_CLR_PW		: printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
#ifdef	USE_TEST_DEBUG_MODE
				case FOCUS_ADMIN_DEBUG		: printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER)); 	break;
#endif
			}
		}
		
		TB_ui_refresh();
	}
}

void op_menu_sub_admin_setting( void )
{
	int quit = 0;
	int input;

	if( s_setup_login_account == ACCOUNT_TYPE_USER )
		return;

	s_idx_admin = 0;
	print_menu_sub_admin_setting();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case '\n' 	:
			case KEY_ENTER :
				{
					if( s_idx_admin == FOCUS_ADMIN_SET_PW )
					{
						int ret = op_menu_sub_admin_setting_pw_change();
						if( ret == 2 )
						{
							input = KEY_AUTO_QUIT;
						}
						else
						{
							print_menu_sub_admin_setting();
						}
					}
					else if( s_idx_admin == FOCUS_ADMIN_CLR_PW )
					{
						if( TB_show_dialog( TB_msgid_get_string(MSGID_ACCOUNT_CLEAR), 5 ) == 1 )
						{
							TB_login_account_clear();
						}
						print_menu_sub_admin_setting();
					}
#ifdef	USE_TEST_DEBUG_MODE
					else if( s_idx_admin == FOCUS_ADMIN_DEBUG )
					{
						op_menu_sub_admin_debug();
					}
#endif
				}
				break;

			case KEY_UP :
				{
					TBSI focus_old = s_idx_admin;					
					s_idx_admin = --s_idx_admin < 0 ? FOCUS_ADMIN_MAX-1 : s_idx_admin;
					update_menu_sub_admin( focus_old, s_idx_admin );
				}
				break;
				
			case KEY_DOWN :
				{
					TBSI focus_old = s_idx_admin;					
					s_idx_admin = ++s_idx_admin > FOCUS_ADMIN_MAX-1 ? 0 : s_idx_admin;
					update_menu_sub_admin( focus_old, s_idx_admin );
				}				
				break;


			case KEY_ESC :
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();
	}
}

////////////////////////////////////////////////////////////////////////////////

static TB_PRODUCT_INFO s_product_info;

TBUC *get_product_info_string_voltage( void )
{
	static TBUC s_info_str[128] = {0, };
	char temp[8] = {0,};

	bzero( s_info_str, sizeof(s_info_str) );

	TBUC *str = NULL;
	if( s_product_info.voltage == VOLTAGE_HIGH )
	{
		str = TB_resid_get_string( RESID_VAL_HIGH_VOLTAGE );
		if( str )
			wstrncpy( s_info_str, sizeof(s_info_str), str, wstrlen(str) );
	}
	else
	{
		str = TB_resid_get_string( RESID_VAL_LOW_VOLTAGE );
		if( str )
			wstrncpy( s_info_str, sizeof(s_info_str), str, wstrlen(str) );
	}

	return &s_info_str[0];
}

TBUC *get_product_info_string_fw_version( void )
{
	static TBUC s_info_str[128] = {0, };

	if( wstrlen(s_info_str) == 0 )
	{
		char month_name[12][4] =
		{
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};

		int  year  = 0;
		int  month = 0;
		int  day   = 0;

		char date_str[32] = {0, };

		/***********************************************************************
		*		date_str ====> 	"Jan  1 2022",
		*						"Feb  9 2022",
		*						"Dec 25 2021"
		***********************************************************************/
		wstrncpy( date_str, sizeof(date_str), __DATE__, wstrlen(__DATE__) );

		char buf[32] = {0, };
		bzero( buf, sizeof(buf) );
		if( wstrlen(date_str) > 0 )
		{
			wmemcpy( buf, sizeof(buf), date_str, wstrlen(date_str) );
		}

		char *p_begin = &buf[0];
		char element_buf[32] = {0, };
		while( 1 )
		{
			int offset = wstrtok( p_begin, " " );
			if( offset > 0 )
			{
				bzero( element_buf, sizeof(element_buf) );
				wmemcpy( element_buf, sizeof(element_buf), p_begin, offset );

				if( isalpha(element_buf[0]) )
				{
					for( int i=0; i<12; i++ )
					{
						if( strcmp( month_name[i], element_buf) == 0 )
						{
							month = i+1;
							break;
						}
					}
				}
				else
				{
					int number = watoi( element_buf );
					if( number > 31 )
						year = number;
					else
						day = number;
				}
			}
			else if( offset == 0 )
			{
				//	skip
			}
			else if( offset < 0 )
			{
				break;
			}

			p_begin += (offset + 1);
			if( p_begin >= (buf + wstrlen(buf)) )
				break;
		}

		TB_VERSION *p_version = TB_setup_get_product_info_version();
		snprintf( s_info_str, sizeof(s_info_str), "V.%d.%d (Build : %04d/%02d/%02d %s)",	\
													p_version->major, p_version->minor,	\
													year, month, day, __TIME__ );
	}

	return &s_info_str[0];
}

TBUC *get_product_info_string_cot_ip( void )
{
	static TBUC s_cot_ip_string[32] = {0, };
	bzero( s_cot_ip_string, sizeof(s_cot_ip_string) );
	
	TBUC *p_ip = &s_product_info.cot_ip[0];
	if( p_ip )
	{
		snprintf( s_cot_ip_string, sizeof(s_cot_ip_string), "%3d.%3d.%3d.%3d", p_ip[0], p_ip[1], p_ip[2], p_ip[3] );
	}

	return s_cot_ip_string;
}

TBUC *get_product_info_string_ems_addr( void )
{
	static TBUC s_ems_addr_string[16] = {0, };
	bzero( s_ems_addr_string, sizeof(s_ems_addr_string) );
	
	snprintf( s_ems_addr_string, sizeof(s_ems_addr_string), "%-5d", s_product_info.ems_addr );

	return s_ems_addr_string;
}

TBUC *get_product_info_string_manuf_date( void )
{
	static TBUC s_manuf_date_string[32] = {0, };
	bzero( s_manuf_date_string, sizeof(s_manuf_date_string) );
	
	snprintf( s_manuf_date_string, sizeof(s_manuf_date_string), "%4d/%02d/%02d",					\
																s_product_info.manuf_date.year,	\
																s_product_info.manuf_date.month,	\
																s_product_info.manuf_date.day );

	return s_manuf_date_string;
}

TBUC *get_product_info_string_ems_dest( void )
{
	static TBUC s_ems_addr_string[16] = {0, };
	bzero( s_ems_addr_string, sizeof(s_ems_addr_string) );
	
	snprintf( s_ems_addr_string, sizeof(s_ems_addr_string), "%-5d", s_product_info.ems_dest );

	return s_ems_addr_string;
}

TBUC *get_product_info_string_rt_port( void )
{
	static TBUC s_rtport_string[16] = {0, };
	bzero( s_rtport_string, sizeof(s_rtport_string) );
	
	snprintf( s_rtport_string, sizeof(s_rtport_string), "%-4d", s_product_info.rt_port );

	return s_rtport_string;
}

TBUC *get_product_info_string_frtu_dnp( void )
{
	static TBUC s_frtu_dnp_string[16] = {0, };
	bzero( s_frtu_dnp_string, sizeof(s_frtu_dnp_string) );
	
	snprintf( s_frtu_dnp_string, sizeof(s_frtu_dnp_string), "%-5d", s_product_info.frtp_dnp );

	return s_frtu_dnp_string;
}

TBUC *get_product_info_stringe_ems_use( void )
{
	static TBUC s_ems_use_string[16] = {0, };
	bzero( s_ems_use_string, sizeof(s_ems_use_string) );

	TBUC *str = NULL;
	if( s_product_info.ems_use )
	{
		str = TB_resid_get_string( RESID_VAL_ENABLE );
		if( str )
			snprintf( s_ems_use_string, sizeof(s_ems_use_string), "%s", str );
	}
	else
	{
		str = TB_resid_get_string( RESID_VAL_DISABLE );
		if( str )
			snprintf( s_ems_use_string, sizeof(s_ems_use_string), "%s", str );
	}

	return s_ems_use_string;
}

TBUC *get_product_info_stringe_session_update( void )
{
	static TBUC s_string[16] = {0, };
	char *p_str_h = TB_resid_get_string( RESID_VAL_HOUR );
	char *p_str_m = TB_resid_get_string( RESID_VAL_MINUTE );

	bzero( s_string, sizeof(s_string) );

	int h = s_setup_file.setup.session_update / HOUR_1;
	int m = (s_setup_file.setup.session_update % HOUR_1) / MINUTE_1;

	if( p_str_h && p_str_m )
	{
		snprintf( s_string, sizeof(s_string), "%02d%s%02d%s", h, p_str_h, m, p_str_m );
	}
	else
	{
		snprintf( s_string, sizeof(s_string), "%02d:%02d",    h, m );
	}

	return s_string;
}

enum 
{
	FOCUS_INFO_VERSION,
	FOCUS_INFO_VOTAGE,
	FOCUS_INFO_EMS_DEST,
	FOCUS_INFO_SERIAL,
	FOCUS_INFO_MANUFACTURE,
	FOCUS_INFO_COT_IP,
	FOCUS_INFO_RT_PORT,
	FOCUS_INFO_FRTP_DNP,
	FOCUS_INFO_EMS_USE,
	FOCUS_INFO_SESSION_KEY,

	FOCUS_INFO_MAX
};

int info_val_starty_ver 		= 2;
int info_val_starty_voltage		= 3;
int info_val_starty_ems_dest	= 4;
int info_val_starty_ems_addr	= 5;
int info_val_starty_manuf_date  = 6;
int info_val_starty_cotip 		= 7;
int info_val_starty_rtport 		= 8;
int info_val_starty_frtu_dnp	= 9;
int info_val_starty_ems			= 10;
int info_val_starty_session_update	= 11;
int info_val_startx = 18;

int focus_cot_ip = 0;
int focus_manuf_date = 0;
void print_menu_sub_product_info( void )
{
	TB_ui_clear( UI_CLEAR_ALL );

	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_INFO) );

	TB_ui_move(info_val_starty_ver, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_FW_VER) );
	(s_idx_info == FOCUS_INFO_VERSION) 	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_ver, info_val_startx);	printw("%s", get_product_info_string_fw_version() );
	
	TB_ui_move(info_val_starty_voltage, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_VOLTAGE) );
	(s_idx_info == FOCUS_INFO_VOTAGE)	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_voltage, info_val_startx);	printw("%s", get_product_info_string_voltage() );
	
	TB_ui_move(info_val_starty_ems_dest, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_EMS_DEST) );
	(s_idx_info == FOCUS_INFO_EMS_DEST)	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_ems_dest, info_val_startx);	printw("%s", get_product_info_string_ems_dest() );
	
	TB_ui_move(info_val_starty_ems_addr, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_SERIAL) );
	(s_idx_info == FOCUS_INFO_SERIAL)  ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_ems_addr, info_val_startx);	printw("%s", get_product_info_string_ems_addr() );
	
	TB_ui_move(info_val_starty_manuf_date, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_MANUFACTURE) );
	(s_idx_info == FOCUS_INFO_MANUFACTURE)  ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_manuf_date, info_val_startx);	printw("%s", get_product_info_string_manuf_date() );
	
	TB_ui_move(info_val_starty_cotip, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_COT_IP) );
	(s_idx_info == FOCUS_INFO_COT_IP)  ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_cotip, info_val_startx);	printw("%s", get_product_info_string_cot_ip() );	
	
	TB_ui_move(info_val_starty_rtport, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_RT_PORT) );
	(s_idx_info == FOCUS_INFO_RT_PORT) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_rtport, info_val_startx);	printw("%s", get_product_info_string_rt_port() );
	
	TB_ui_move(info_val_starty_frtu_dnp, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_FRTU_DNP) );
	(s_idx_info == FOCUS_INFO_FRTP_DNP) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_frtu_dnp, info_val_startx);	printw("%s", get_product_info_string_frtu_dnp() );

	TB_ui_move(info_val_starty_ems, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_EMS_MODE) );
	(s_idx_info == FOCUS_INFO_EMS_USE) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_ems, info_val_startx);	printw("%s", get_product_info_stringe_ems_use() );
	
	TB_ui_move(info_val_starty_session_update, 0);	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_SESSION_UPDATE) );
	(s_idx_info == FOCUS_INFO_SESSION_KEY) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move(info_val_starty_session_update, info_val_startx);	printw("%s", get_product_info_stringe_session_update() );
	
	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );

	TB_ui_refresh();
}

void print_product_info_cot_ip( void )
{
	char *p_ip = &s_product_info.cot_ip[0];
	
	if( s_idx_info == FOCUS_INFO_COT_IP )
	{		
		move( info_val_starty_cotip, info_val_startx+0 );	focus_cot_ip==0 ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF ); 	printw("%3d", p_ip[0] );
		move( info_val_starty_cotip, info_val_startx+4 );	focus_cot_ip==1 ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF ); 	printw("%3d", p_ip[1] );
		move( info_val_starty_cotip, info_val_startx+8 );	focus_cot_ip==2 ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF ); 	printw("%3d", p_ip[2] );
		move( info_val_starty_cotip, info_val_startx+12 );	focus_cot_ip==3 ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF ); 	printw("%3d", p_ip[3] );
	}
	else
	{
		move( info_val_starty_cotip, info_val_startx+0 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[0] );
		move( info_val_starty_cotip, info_val_startx+4 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[1] );
		move( info_val_starty_cotip, info_val_startx+8 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[2] );
		move( info_val_starty_cotip, info_val_startx+12 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[3] );
	}
}

void update_product_info_cot_ip( TBSI focus_old, TBSI focus_ip )
{
	char *p_ip = &s_product_info.cot_ip[0];

	switch( focus_old )
	{
		case 0 : 	move( info_val_starty_cotip, info_val_startx+0 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[0] );	break;
		case 1 : 	move( info_val_starty_cotip, info_val_startx+4 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[1] );	break;
		case 2 : 	move( info_val_starty_cotip, info_val_startx+8 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[2] );	break;
		case 3 : 	move( info_val_starty_cotip, info_val_startx+12 );	attrset( FOCUS_OFF ); 	printw("%3d", p_ip[3] );	break;
	}

	switch( focus_ip )
	{
		case 0 : 	move( info_val_starty_cotip, info_val_startx+0 );	attrset( FOCUS_ON ); 	printw("%3d", p_ip[0] );	break;
		case 1 : 	move( info_val_starty_cotip, info_val_startx+4 );	attrset( FOCUS_ON ); 	printw("%3d", p_ip[1] );	break;
		case 2 : 	move( info_val_starty_cotip, info_val_startx+8 );	attrset( FOCUS_ON ); 	printw("%3d", p_ip[2] );	break;
		case 3 : 	move( info_val_starty_cotip, info_val_startx+12 );	attrset( FOCUS_ON ); 	printw("%3d", p_ip[3] );	break;
	}
}

void update_product_info_cot_ip_each( TBSI input_value, int focus )
{	
	update_product_info_cot_ip( -1, focus );
}

void print_product_info_manuf_date( void )
{
	if( s_idx_info == FOCUS_INFO_MANUFACTURE )
	{		
		move( info_val_starty_manuf_date, info_val_startx+0 );	focus_manuf_date==0 ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF ); 	printw("%4d", s_product_info.manuf_date.year );
		move( info_val_starty_manuf_date, info_val_startx+5 );	focus_manuf_date==1 ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF ); 	printw("%02d", s_product_info.manuf_date.month );
		move( info_val_starty_manuf_date, info_val_startx+8 );	focus_manuf_date==2 ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF ); 	printw("%02d", s_product_info.manuf_date.day );
	}
	else
	{
		move( info_val_starty_manuf_date, info_val_startx+0 );	attrset( FOCUS_OFF ); 	printw("%4d", s_product_info.manuf_date.year );
		move( info_val_starty_manuf_date, info_val_startx+5 );	attrset( FOCUS_OFF ); 	printw("%02d", s_product_info.manuf_date.month );
		move( info_val_starty_manuf_date, info_val_startx+8 );	attrset( FOCUS_OFF ); 	printw("%02d", s_product_info.manuf_date.day );
	}
}

void update_product_info_manuf_date( TBSI focus_old, TBSI focus_ip )
{
	switch( focus_old )
	{
		case 0 : 	move( info_val_starty_manuf_date, info_val_startx+0 );	attrset( FOCUS_OFF ); 	printw("%4d", s_product_info.manuf_date.year );	break;
		case 1 : 	move( info_val_starty_manuf_date, info_val_startx+5 );	attrset( FOCUS_OFF ); 	printw("%02d", s_product_info.manuf_date.month);break;
		case 2 : 	move( info_val_starty_manuf_date, info_val_startx+8 );	attrset( FOCUS_OFF ); 	printw("%02d", s_product_info.manuf_date.day );	break;
	}

	switch( focus_ip )
	{
		case 0 : 	move( info_val_starty_manuf_date, info_val_startx+0 );	attrset( FOCUS_ON ); 	printw("%4d", s_product_info.manuf_date.year );	break;
		case 1 : 	move( info_val_starty_manuf_date, info_val_startx+5 );	attrset( FOCUS_ON ); 	printw("%02d", s_product_info.manuf_date.month);break;
		case 2 : 	move( info_val_starty_manuf_date, info_val_startx+8 );	attrset( FOCUS_ON ); 	printw("%02d", s_product_info.manuf_date.day );	break;
	}
}

void print_product_info_session_update_time( void )
{
	TBUC *p = get_product_info_stringe_session_update();
	if( p )
	{
		move( info_val_starty_session_update, info_val_startx );
		(s_idx_info == FOCUS_INFO_SESSION_KEY) ? attrset( FOCUS_ON ) : attrset( FOCUS_OFF );
		printw("%s", p );
	}
}

void update_menu_sub_product_info( TBSI old, TBSI now )
{
	if( old != now )
	{
		if( old >= 0 )
		{
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+old, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_FW_VER+old) );
			TB_ui_attrset( FOCUS_OFF );
			TB_ui_move(2+old, info_val_startx);
			switch( old )
			{
				case FOCUS_INFO_VERSION 	: 	printw("%s", get_product_info_string_fw_version() );	break;
				case FOCUS_INFO_VOTAGE		: 	printw("%s", get_product_info_string_voltage() );		break;
				case FOCUS_INFO_EMS_DEST	: 	printw("%s", get_product_info_string_ems_dest() );		break;
				case FOCUS_INFO_SERIAL		:	printw("%s", get_product_info_string_ems_addr() );		break;
				case FOCUS_INFO_MANUFACTURE	:	print_product_info_manuf_date();						break;
				case FOCUS_INFO_COT_IP		:	print_product_info_cot_ip();							break;
				case FOCUS_INFO_RT_PORT		: 	printw("%s", get_product_info_string_rt_port() );		break;
				case FOCUS_INFO_FRTP_DNP	: 	printw("%s", get_product_info_string_frtu_dnp() );		break;
				case FOCUS_INFO_EMS_USE 	: 	printw("%s", get_product_info_stringe_ems_use() );		break;
				case FOCUS_INFO_SESSION_KEY	: 	printw("%s", get_product_info_stringe_session_update() );	break;
			}
		}

		if( now >= 0 )
		{		
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+now, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_INFO_FW_VER+now) );
			TB_ui_attrset( FOCUS_ON );
			TB_ui_move(2+now, info_val_startx);
			switch( now )
			{
				case FOCUS_INFO_VERSION 	: 	printw("%s", get_product_info_string_fw_version() );	break;
				case FOCUS_INFO_VOTAGE		: 	printw("%s", get_product_info_string_voltage() );		break;
				case FOCUS_INFO_EMS_DEST	: 	printw("%s", get_product_info_string_ems_dest() );		break;
				case FOCUS_INFO_SERIAL		:	printw("%s", get_product_info_string_ems_addr() );		break;
				case FOCUS_INFO_MANUFACTURE	:	print_product_info_manuf_date();						break;
				case FOCUS_INFO_COT_IP		:	print_product_info_cot_ip();							break;
				case FOCUS_INFO_RT_PORT		: 	printw("%s", get_product_info_string_rt_port() );		break;
				case FOCUS_INFO_FRTP_DNP	: 	printw("%s", get_product_info_string_frtu_dnp() );		break;
				case FOCUS_INFO_EMS_USE 	: 	printw("%s", get_product_info_stringe_ems_use() );		break;
				case FOCUS_INFO_SESSION_KEY	: 	printw("%s", get_product_info_stringe_session_update() );	break;
			}
		}
		
		TB_ui_refresh();
	}
}

void op_menu_sub_product_info( void )
{
	int quit = 0;
	int input;	
	
	char input_buf[128] = {0, };
	int  input_value = 0;
	int  input_buf_idx_ems_addr = 0;
	int  input_buf_idx_manuf_date = 0;
	int  input_buf_idx_cotip  = 0;
	int  input_buf_idx_rtport = 0;
	int  input_buf_idx_frtu_dnp = 0;

	s_idx_info = FOCUS_INFO_VERSION;

	snprintf( input_buf, sizeof(input_buf), "%s : ", TB_resid_get_string(RESID_LBL_INFO_FW_VER) );
	info_val_startx = wstrlen( input_buf );

	bzero( input_buf, sizeof(input_buf) );
	wmemcpy( &s_product_info, sizeof(s_product_info), &s_setup_file.setup.info, sizeof(TB_PRODUCT_INFO) );
	
	print_menu_sub_product_info();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case '0'...'9'	:
				{
					//	0 ~ 65535
					if( s_idx_info == FOCUS_INFO_SERIAL || s_idx_info == FOCUS_INFO_EMS_DEST || s_idx_info == FOCUS_INFO_FRTP_DNP )
					{
						input_buf[input_buf_idx_ems_addr+0] = (char)(((char)input) & 0xFF);
						input_buf[input_buf_idx_ems_addr+1] = 0;
						input_buf_idx_ems_addr ++;
						input_value = watoi( input_buf );
						
						if( input_buf_idx_cotip > 5 )
						{
							bzero( input_buf, sizeof(input_buf) );
							input_value = 0;
							input_buf_idx_ems_addr = 0;
						}

						if( input_value > 65535 )
						{
							bzero( input_buf, sizeof(input_buf) );
							input_value = 0;
							input_buf_idx_ems_addr = 0;
						}

						if( s_idx_info == FOCUS_INFO_SERIAL )
						{
							s_product_info.ems_addr = input_value;
							TB_ui_move(info_val_starty_ems_addr, info_val_startx);
							printw("%s", get_product_info_string_ems_addr() );
						}
						else if( s_idx_info == FOCUS_INFO_EMS_DEST )
						{
							s_product_info.ems_dest = input_value;
							TB_ui_move(info_val_starty_ems_dest, info_val_startx);
							printw("%s", get_product_info_string_ems_dest() );
						}
						else if( s_idx_info == FOCUS_INFO_FRTP_DNP )
						{
							s_product_info.frtp_dnp = input_value;
							TB_ui_move(info_val_starty_frtu_dnp, info_val_startx);
							printw("%s", get_product_info_string_frtu_dnp() );
						}
					}
					else if( s_idx_info == FOCUS_INFO_MANUFACTURE )
					{
						switch( focus_manuf_date )
						{
							case 0 :
								input_buf[input_buf_idx_manuf_date+0] = (char)(((char)input) & 0xFF);
								input_buf[input_buf_idx_manuf_date+1] = 0;
								input_buf_idx_manuf_date ++;
								input_value = watoi( input_buf );

								if( input_buf_idx_manuf_date > 4 )
								{
									bzero( input_buf, sizeof(input_buf) );
									input_value = 0;
									input_buf_idx_manuf_date = 0;
								}

								if( input_value > YEAR_MAX )
								{
									bzero( input_buf, sizeof(input_buf) );
									input_value = 0;
									input_buf_idx_manuf_date = 0;
								}
								s_product_info.manuf_date.year = input_value;
								break;
							case 1 :
								input_buf[input_buf_idx_manuf_date+0] = (char)(((char)input) & 0xFF);
								input_buf[input_buf_idx_manuf_date+1] = 0;
								input_buf_idx_manuf_date ++;
								input_value = watoi( input_buf );

								if( input_buf_idx_manuf_date > 2 )
								{
									bzero( input_buf, sizeof(input_buf) );
									input_value = 0;
									input_buf_idx_manuf_date = 0;
								}

								if( input_value > 12 )
								{
									bzero( input_buf, sizeof(input_buf) );
									input_value = 0;
									input_buf_idx_manuf_date = 0;
								}
								s_product_info.manuf_date.month = input_value;
								break;
							case 2 :
								input_buf[input_buf_idx_manuf_date+0] = (char)(((char)input) & 0xFF);
								input_buf[input_buf_idx_manuf_date+1] = 0;
								input_buf_idx_manuf_date ++;
								input_value = watoi( input_buf );

								if( input_buf_idx_manuf_date > 2 )
								{
									bzero( input_buf, sizeof(input_buf) );
									input_value = 0;
									input_buf_idx_manuf_date = 0;
								}

								int max_day = TB_util_get_lastday_of_month( s_product_info.manuf_date.month, s_product_info.manuf_date.year );
								if( input_value > max_day )
								{
									bzero( input_buf, sizeof(input_buf) );
									input_value = 0;
									input_buf_idx_manuf_date = 0;
								}
								s_product_info.manuf_date.day = input_value;
								break;
						}
						update_product_info_manuf_date( -1, focus_manuf_date );
					}
					else if( s_idx_info == FOCUS_INFO_COT_IP )	//	0 ~ 255
					{
						input_buf[input_buf_idx_cotip+0] = (char)(((char)input) & 0xFF);
						input_buf[input_buf_idx_cotip+1] = 0;
						input_buf_idx_cotip ++;
						input_value = watoi( input_buf );

						if( input_buf_idx_cotip > 3 )
						{
							bzero( input_buf, sizeof(input_buf) );
							input_value = 0;
							input_buf_idx_cotip = 0;
						}

						if( input_value > 255 )
						{
							bzero( input_buf, sizeof(input_buf) );
							input_value = 0;
							input_buf_idx_cotip = 0;
						}

						s_product_info.cot_ip[focus_cot_ip] = input_value;
						update_product_info_cot_ip_each( input_value, focus_cot_ip );
					}
					else if( s_idx_info == FOCUS_INFO_RT_PORT )		//	0 ~ 9999
					{
						input_buf[input_buf_idx_rtport+0] = (char)(((char)input) & 0xFF);
						input_buf[input_buf_idx_rtport+1] = 0;
						input_buf_idx_rtport ++;
						input_value = watoi( input_buf );
						
						if( input_buf_idx_cotip > 4 )
						{
							bzero( input_buf, sizeof(input_buf) );
							input_value = 0;
							input_buf_idx_rtport = 0;
						}

						if( input_value > 9999 )
						{
							bzero( input_buf, sizeof(input_buf) );
							input_value = 0;
							input_buf_idx_rtport = 0;
						}

						s_product_info.rt_port = input_value;
						TB_ui_move(info_val_starty_rtport, info_val_startx);
						printw("%s", get_product_info_string_rt_port() );
					}
				}
				break;

			case KEY_LEFT :
				if( s_idx_info == FOCUS_INFO_EMS_USE )
				{
					s_product_info.ems_use = !s_product_info.ems_use;
					update_menu_sub_product_info( -1, FOCUS_INFO_EMS_USE );
				}
				else if( s_idx_info == FOCUS_INFO_VOTAGE )
				{
					s_product_info.voltage = !s_product_info.voltage;
					update_menu_sub_product_info( -1, FOCUS_INFO_VOTAGE );

					if( s_product_info.voltage == VOLTAGE_HIGH )
						TB_setup_set_wisun_info_enable( WISUN_SECOND, FALSE );
					else
						TB_setup_set_wisun_info_enable( WISUN_SECOND, TRUE );
				}
				else if( s_idx_info == FOCUS_INFO_MANUFACTURE )
				{
					bzero( input_buf, sizeof(input_buf) );
					input_value = 0;
					input_buf_idx_manuf_date = 0;

					int focus_old = focus_manuf_date;
					focus_manuf_date = --focus_manuf_date < 0 ? 2 : focus_manuf_date;
					update_product_info_manuf_date( focus_old, focus_manuf_date );
				}
				else if( s_idx_info == FOCUS_INFO_COT_IP )
				{
					bzero( input_buf, sizeof(input_buf) );
					input_value = 0;
					input_buf_idx_cotip = 0;

					int focus_old = focus_cot_ip;
					focus_cot_ip = --focus_cot_ip < 0 ? 3 : focus_cot_ip;
					update_product_info_cot_ip( focus_old, focus_cot_ip );
				}
				else if( s_idx_info == FOCUS_INFO_SESSION_KEY )
				{
					if( s_setup_file.setup.session_update >= MINUTE_30 )
						s_setup_file.setup.session_update -= MINUTE_30;
					else
						s_setup_file.setup.session_update = (23*HOUR_1)+(MINUTE_30);	//	23:30
					print_product_info_session_update_time();
				}
				break;
				
			case KEY_RIGHT :
				if( s_idx_info == FOCUS_INFO_EMS_USE )
				{
					s_product_info.ems_use = !s_product_info.ems_use;
					update_menu_sub_product_info( -1, FOCUS_INFO_EMS_USE );
				}
				else if( s_idx_info == FOCUS_INFO_VOTAGE )
				{
					s_product_info.voltage = !s_product_info.voltage;
					update_menu_sub_product_info( -1, FOCUS_INFO_VOTAGE );

					if( s_product_info.voltage == VOLTAGE_HIGH )
						TB_setup_set_wisun_info_enable( WISUN_SECOND, FALSE );
					else
						TB_setup_set_wisun_info_enable( WISUN_SECOND, TRUE );
				}
				else if( s_idx_info == FOCUS_INFO_MANUFACTURE )
				{
					bzero( input_buf, sizeof(input_buf) );
					input_value = 0;
					input_buf_idx_manuf_date = 0;

					int focus_old = focus_manuf_date;
					focus_manuf_date = ++focus_manuf_date > 2 ? 0 : focus_manuf_date;
					update_product_info_manuf_date( focus_old, focus_manuf_date );
				}
				else if( s_idx_info == FOCUS_INFO_COT_IP )
				{
					bzero( input_buf, sizeof(input_buf) );
					input_value = 0;
					input_buf_idx_cotip = 0;

					int focus_old = focus_cot_ip;
					focus_cot_ip = ++focus_cot_ip > 3 ? 0 : focus_cot_ip;
					update_product_info_cot_ip( focus_old, focus_cot_ip );
				}
				else if( s_idx_info == FOCUS_INFO_SESSION_KEY )
				{
					if( s_setup_file.setup.session_update < (23*(HOUR_1))+MINUTE_30 )	//23:30
						s_setup_file.setup.session_update += MINUTE_30;
					else
						s_setup_file.setup.session_update = 0;
					print_product_info_session_update_time();
				}
				break;
				
			case KEY_UP :
				{
					focus_cot_ip 		= 0;
					focus_manuf_date 	= 0;
					bzero( input_buf, sizeof(input_buf) );
					input_value = 0;
					input_buf_idx_ems_addr = 0;
					input_buf_idx_manuf_date = 0;
					input_buf_idx_cotip  = 0;
					input_buf_idx_rtport = 0;
					input_buf_idx_frtu_dnp = 0;

					TBSI focus_old = s_idx_info;
					s_idx_info = --s_idx_info < 0 ? FOCUS_INFO_MAX-1 : s_idx_info;
					update_menu_sub_product_info( focus_old, s_idx_info );
				}
				break;
				
			case KEY_DOWN :
				{
					focus_cot_ip 		= 0;
					focus_manuf_date 	= 0;
					bzero( input_buf, sizeof(input_buf) );
					input_value = 0;
					input_buf_idx_ems_addr = 0;
					input_buf_idx_manuf_date = 0;
					input_buf_idx_cotip  = 0;
					input_buf_idx_rtport = 0;
					input_buf_idx_frtu_dnp = 0;

					TBSI focus_old = s_idx_info;
					s_idx_info = ++s_idx_info > FOCUS_INFO_MAX-1 ? 0 : s_idx_info;
					update_menu_sub_product_info( focus_old, s_idx_info );
				}				
				break;

			case KEY_ESC :
				wmemcpy( &s_setup_file.setup.info, sizeof(s_setup_file.setup.info), &s_product_info, sizeof(TB_PRODUCT_INFO) );
				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_main();	
	}
}

////////////////////////////////////////////////////////////////////////////////

enum 
{
	FOCUS_KEY_INPUT,
	FOCUS_KEY_UPLOAD,
	FOCUS_KEY_DOWNLOAD,

	FOCUS_KEY_MAX
};

#define KEY_INPUT_LINE	2

TBSI focus_masterkey = 0;
TBSI focus_masterkey_edit = 0;
TBUC s_masterkey_edit[LEN_BYTE_KEY] = {0, };

int kcmvp_key_edit_val_startx = 21;

void print_key_edit_string( void )
{
	for( int i=0; i<LEN_BYTE_KEY; i++ )
	{
		TB_ui_move( KEY_INPUT_LINE, kcmvp_key_edit_val_startx+(i*3) );
		if( focus_masterkey == FOCUS_KEY_INPUT )
			(i == focus_masterkey_edit) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
		else
			TB_ui_attrset( FOCUS_OFF );
		printw("%02X", s_masterkey_edit[i] );
	}
	printw("\n");
}

void print_menu_sub_masterkey_edit( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TLT_MASTERKEY_EDIT) );
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_INPUT)); 	focus_masterkey==FOCUS_KEY_INPUT ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		print_key_edit_string();
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_UPLOAD)); 	focus_masterkey==FOCUS_KEY_UPLOAD ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_DOWNLOAD)); focus_masterkey==FOCUS_KEY_DOWNLOAD ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );		printw("%s\n", TB_resid_get_string(RESID_VAL_ENTER));
	printw("\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_TIP_ENTER) );
	
	TB_ui_refresh();
}

void update_menu_sub_masterkey_edit( TBSI old, TBSI now )
{
	if( old != now )
	{
		if( old >= 0 )
		{
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+old, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_INPUT+old) );
			TB_ui_attrset( FOCUS_OFF );
			switch( old )
			{
				case FOCUS_KEY_INPUT 	: print_key_edit_string();									break;
				case FOCUS_KEY_UPLOAD 	: printw("%s", TB_resid_get_string(RESID_VAL_ENTER));		break;
				case FOCUS_KEY_DOWNLOAD	: printw("%s", TB_resid_get_string(RESID_VAL_ENTER));		break;
			}
		}

		if( now >= 0 )
		{		
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(2+now, 0);	printw("%s : ", TB_resid_get_string(RESID_LBL_MASTERKEY_INPUT+now) );
			TB_ui_attrset( FOCUS_ON );
			switch( now )
			{
				case FOCUS_KEY_INPUT 	: print_key_edit_string();									break;
				case FOCUS_KEY_UPLOAD 	: printw("%s", TB_resid_get_string(RESID_VAL_ENTER));		break;
				case FOCUS_KEY_DOWNLOAD	: printw("%s", TB_resid_get_string(RESID_VAL_ENTER));		break;
			}
		}
		
		TB_ui_refresh();
	}
}

#include "KDN_LIB.h"
extern TBUC g_seed_table_low[MAX_SEED_TABLE_CNT];

int make_encrypt_key( TBUC *p_key )
{
	int ret = -1;
	if( p_key )
	{
		TBUC key[16] = {0, };
		
		srand( time(NULL) );
		/*
		*	하위 8비트만 구한다.( 0 ~ 255 )
		*/
		TBUC random = (TBUC)TB_util_get_rand( 0, 255 );
		if( random + 16 < MAX_SEED_TABLE_CNT )
		{
			wmemcpy( &key[0], sizeof(key), &g_seed_table_low[random], 16 );
		}
		else
		{
			int s = MAX_SEED_TABLE_CNT - random;
			wmemcpy( &key[0], sizeof(key), &g_seed_table_low[random], s );
			wmemcpy( &key[s], sizeof(key)-s, &g_seed_table_low[0], 16 - s );
		}

		wmemcpy( p_key, 16, key, 16 );
		ret = 0;
	}
	else
	{
		printf( "[%s] key is NULL\r\n", __FUNCTION__ );
	}
	
	return ret;
}

int find_master_key( TBUC *p_keydata, TBUI keydata_size, TB_KCMVP_KEY *p_masterkey )
{
	int ret = -1;
	
	if( p_keydata && keydata_size == 32 && p_masterkey )
	{
		TBUC *p_data = &p_keydata[16];
		TBUC *p_tag  = &p_keydata[0];

		TBUC dec_data[16] = {0, };
		TBUI dec_size;
		
		TBUC test_enc_key[16] = {0, };

		TB_KCMVP_KEY *p_iv   = TB_kcmvp_get_keyinfo_iv();
		TB_KCMVP_KEY *p_auth = TB_kcmvp_get_keyinfo_auth();

printf("\r\n");
for( TBUI i=0; i<keydata_size; i++ )
{
	printf("%02X ", p_keydata[i] );
	if( (i % 16) == 15 )
		printf("\r\n");
}
printf("\r\n");
printf("\r\n");
printf("\r\n");
printf("\r\n");


		int idx = 0;
		while( 1 )
		{
			if( idx + 16 < MAX_SEED_TABLE_CNT )
			{
				wmemcpy( test_enc_key, sizeof(test_enc_key), &g_seed_table_low[idx], 16 );
			}
			else
			{
				int s = MAX_SEED_TABLE_CNT - idx;
				wmemcpy( &test_enc_key[0], sizeof(test_enc_key), &g_seed_table_low[idx], s );
				wmemcpy( &test_enc_key[s], sizeof(test_enc_key)-s, &g_seed_table_low[0], 16 - s );
			}

printf("test key = ");
for( TBUI i=0; i<16; i++ )
	printf("%02X ", test_enc_key[i] );
printf("\r\n");


			TBUI kcmvp_ret = KDN_BC_Decrypt( &dec_data[0], &dec_size,	\
											 &p_data[0], 16,			\
											 &test_enc_key[0], 16,		\
											 KDN_BC_Algo_ARIA_GCM, 		\
											 p_iv->data, p_iv->size,	\
											 
											 p_auth->data, p_auth->size,\
											 p_tag, 16 );
			if( kcmvp_ret == KDN_OK )
			{
				wmemcpy( &p_masterkey->data, sizeof(p_masterkey->data), &dec_data[0], dec_size );
				p_masterkey->size = dec_size;

				ret = 0;
				break;
			}
			else
			{
printf("decryption fail : %d\r\n", idx);
			}

			idx ++;
			idx = idx % MAX_SEED_TABLE_CNT;

			if( idx == 0 )
			{
				printf( "********** FAIL. KCMVP KEY FIND....\r\n" );
				break;
			}
		}
	}
	else
	{
		if( p_keydata == 0 )		printf( "[%s] keydata is NULL\r\n", 			__FUNCTION__ );
		if( keydata_size != 32 )	printf( "[%s] keydata size is not 32byte\r\n", 	__FUNCTION__ );
		if( p_masterkey == 0 )		printf( "[%s] masterkey is NULL\r\n", 			__FUNCTION__ );
	}

	return ret;
}

int encryption( TBUC *p_text, TBUI text_size, TB_BUF128 *p_enc_buf )
{
	int ret = -1;
	if( p_text && p_enc_buf )
	{
		TBUC enc_data[128] = {0, };
		TBUI enc_size;

		TBUC tag_data[128] = {0, };
		TBUI tag_size = 16;
		
		TBUC enc_key[16] = {0, };

		TB_KCMVP_KEY *p_iv	 = TB_kcmvp_get_keyinfo_iv();
		TB_KCMVP_KEY *p_auth = TB_kcmvp_get_keyinfo_auth();

		if( make_encrypt_key( enc_key ) == 0 )
		{
			TB_util_data_dump( "MAKE KEY", enc_key, 16 );
			TBUI kcmvp_ret = KDN_BC_Encrypt( &enc_data[0], &enc_size,	\
											 p_text, 16,				\
											 &enc_key[0], 16,			\
											 KDN_BC_Algo_ARIA_GCM,	 	\
											 p_iv->data, p_iv->size, 	\
											 p_auth->data, p_auth->size,\
											 &tag_data[0], tag_size );
			if( kcmvp_ret == KDN_OK )
			{
				int idx = 0;
				wmemcpy( &p_enc_buf->buffer[idx], sizeof(p_enc_buf->buffer)-idx, tag_data, tag_size );		idx += tag_size;
				wmemcpy( &p_enc_buf->buffer[idx], sizeof(p_enc_buf->buffer)-idx, enc_data, enc_size );		idx += enc_size;
				p_enc_buf->length = idx;
				
				ret = 0;
			}
		}
		else
		{
			printf( "error. make key fail\r\n");
		}
	}
	else
	{
		if( p_text == NULL )		printf( "[%s] text is NULL\r\n", 	 __FUNCTION__ );
		if( p_enc_buf == NULL )		printf( "[%s] enc_buf is NULL\r\n", __FUNCTION__ );
	}

	return ret;
}

int make_upload_masterkey_data( TB_KCMVP_KEY *p_masterkey, TB_BUF128 *p_enc_buf )
{
	int ret = -1;

	if( p_enc_buf && p_masterkey )
	{
		ret = encryption( p_masterkey->data,	\
						  p_masterkey->size,	\
						  p_enc_buf );
	}
	else
	{
		if( p_masterkey == NULL )	printf( "[%s] masterkey buffer is NULL\r\n", 	 __FUNCTION__ );
		if( p_enc_buf == NULL )		printf( "[%s] p_enc_buf is NULL\r\n", __FUNCTION__ );
	}

	return ret;
}

int get_masterkey_data( TBUC *buf, TBUI buf_size, TB_KCMVP_KEY *p_masterkey )
{
	int ret = -1;

	if( buf && p_masterkey )
	{
		ret = find_master_key( 	buf, buf_size, p_masterkey );

		if( ret != 0 )
		{
			printf( "FAIL. find_master_key\r\n" );
		}
	}

	return ret;
}

void op_menu_sub_masterkey_edit( void )
{
	int quit = 0;
	int input;

	s_idx_kcmvp_key_edit = 0;
	focus_masterkey_edit = 0;	

	char key[8];
	TBSI key_value = 0;
	static TBSI key_idx = 0;
	
	TB_KCMVP_KEY *p_key = TB_setup_get_kcmvp_master_key();
	bzero( s_masterkey_edit, sizeof(s_masterkey_edit) );
	wmemcpy( s_masterkey_edit, sizeof(s_masterkey_edit), p_key->data, sizeof(s_masterkey_edit) );

	kcmvp_key_edit_val_startx = wstrlen( TB_resid_get_string(RESID_LBL_MASTERKEY_INPUT) ) + wstrlen(" : ");

	print_menu_sub_masterkey_edit();

	TB_ui_key_init( TRUE );	
	while( quit == 0 )
	{
		input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}

		switch( input )
		{
			case 'a'...'f'	:
			case 'A'...'F'	:
			case '0'...'9'	:
				if( key_idx > 1 )
				{
					bzero( key, sizeof(key) );
					key_value = 0;
					key_idx = 0;
				}

				if( input == '0' && key_idx == 0 )
				{
					bzero( key, sizeof(key) );					
					key_value = 0;
					key_idx = 0;
				}
				else
				{
					key[key_idx++] = (char)input;
					key[key_idx] = 0;					
					key_value = wstrtol_16( key );
				}

				if( key_value > 255 )
				{
					key_idx = 0;
					key[key_idx++] = (char)input;
					key[key_idx] = 0;
					key_value = wstrtol_16( key );
				}

				////////////////////////////////////////////////////////////

				s_masterkey_edit[focus_masterkey_edit] = (TBUC)key_value;

				////////////////////////////////////////////////////////////

				if( focus_masterkey_edit < 16 )
				{
					TB_ui_move( KEY_INPUT_LINE, kcmvp_key_edit_val_startx+(focus_masterkey_edit*3) );
					TB_ui_attrset( FOCUS_ON );
					printw("%02X", s_masterkey_edit[focus_masterkey_edit] );
				}
				break;

			case KEY_LEFT :
				{
					int focus_old = focus_masterkey_edit;
					focus_masterkey_edit = --focus_masterkey_edit < 0 ? 15 : focus_masterkey_edit;

					if( focus_old < 16 )
					{
						TB_ui_move( KEY_INPUT_LINE, kcmvp_key_edit_val_startx+(focus_old*3) );
						TB_ui_attrset( FOCUS_OFF );
						printw("%02X", s_masterkey_edit[focus_old] );
					}

					if( focus_masterkey_edit < 16 )
					{
						TB_ui_move( KEY_INPUT_LINE, kcmvp_key_edit_val_startx+(focus_masterkey_edit*3) );
						TB_ui_attrset( FOCUS_ON );
						printw("%02X", s_masterkey_edit[focus_masterkey_edit] );
					}
				}
				break;
				
			case KEY_RIGHT :
				{
					int focus_old = focus_masterkey_edit;
					focus_masterkey_edit = ++focus_masterkey_edit > 15 ? 0 : focus_masterkey_edit;

					if( focus_old < 16 )
					{
						TB_ui_move( KEY_INPUT_LINE, kcmvp_key_edit_val_startx+(focus_old*3) );
						TB_ui_attrset( FOCUS_OFF );
						printw("%02X", s_masterkey_edit[focus_old] );
					}

					if( focus_masterkey_edit < 16 )
					{
						TB_ui_move( KEY_INPUT_LINE, kcmvp_key_edit_val_startx+(focus_masterkey_edit*3) );
						TB_ui_attrset( FOCUS_ON );
						printw("%02X", s_masterkey_edit[focus_masterkey_edit] );
					}
				}
				break;

			case KEY_UP 	:
				{
					TBSI focus_old = focus_masterkey;					
					focus_masterkey = --focus_masterkey < 0 ? FOCUS_KEY_MAX-1 : focus_masterkey;
					update_menu_sub_masterkey_edit( focus_old, focus_masterkey );
				}
				break;
				
			case KEY_DOWN 	:
				{
					TBSI focus_old = focus_masterkey;					
					focus_masterkey = ++focus_masterkey > FOCUS_KEY_MAX-1 ? 0 : focus_masterkey;
					update_menu_sub_masterkey_edit( focus_old, focus_masterkey );
				}				
				break;
				
			case '\n' 		:
			case KEY_ENTER 	:
				switch( focus_masterkey )
				{
					case FOCUS_KEY_INPUT 	:
						break;
						
					case FOCUS_KEY_UPLOAD	:
						if( TB_kcmvp_key_check() == 0 )
						{
							TB_BUF128	enc_buf;
							
							if( make_upload_masterkey_data( &s_setup_file.setup.master_key, &enc_buf) == 0 )
							{	
								TB_ui_clear( UI_CLEAR_ALL );
								TB_ui_refresh();
								
								FILE* fp = fopen( MASTERKEY_FILE_PATH, "w" );
								if( fp )
								{
									if( fwrite( &enc_buf.buffer[0], 1, enc_buf.length, fp ) == enc_buf.length )
									{
										printf("SUCCESS. SAVE MASTERKEY KEY FILE : %s\r\n", MASTERKEY_FILE_PATH );
										fflush( fp );
										fclose( fp );

										////////////////////////////////////////////
										
										TBUC command[128] = {0, };
										
										snprintf( command, sizeof(command), "%s -t 10 %s", SERIAL_COMM_SEND, MASTERKEY_FILE_PATH );
										printf( "command : %s\r\n", command );
										system( command );
										
										snprintf( command, sizeof(command), "rm -rf %s", MASTERKEY_FILE_PATH );
										system( command );

										printf("SUCCESS. UPLOAD MASTER KEY\r\n" );
									}
									else
									{
										printf("key upload : WRITE ERROR\r\n");
									}
								}
								else
								{
									printf("FAIL. SAVE MASTERKEY KEY FILE : %s\r\n", MASTERKEY_FILE_PATH );
								}

								sleep( 3 );
								print_menu_sub_masterkey_edit();
							}
						}
						break;
						
					case FOCUS_KEY_DOWNLOAD	:
						{
							TB_ui_clear( UI_CLEAR_ALL );
							TB_ui_refresh();

							char curr_path[128] = {0, };
							char move_path[128] = {0, };
							if( getcwd(curr_path, sizeof(curr_path)) != NULL )
							{								
								printf( "[%s:%d] Current Working Dir : %s\r\n", __FILE__, __LINE__, curr_path );
								chdir( "/root" );
								getcwd( move_path, sizeof(move_path) );
								printf( "[%s:%d] Current Working Dir : %s\r\n", __FILE__, __LINE__, move_path );

								////////////////////////////////////////////////
							
								TBUC command[128] = {0, };
								snprintf( command, sizeof(command), "%s -v -b -Z", SERIAL_COMM_RECV );
								printf( "command : %s\r\n", command );
								system( command );

								if( access(MASTERKEY_FILE_PATH, F_OK) == 0 )
								{
									FILE* fp = fopen( MASTERKEY_FILE_PATH, "r" );
									if( fp )
									{
										TBUC buf[128] = {0, };
										TBUI read_size = 32;	//	16(key) + 16(tag)
										
										bzero( &buf, sizeof(buf) );
										if( fread( buf, 1, sizeof(buf)-1, fp ) == read_size )
										{
											buf[read_size] = '\0';
											TB_KCMVP_KEY masterkey;
											if( get_masterkey_data( buf, read_size, &masterkey ) == 0 )
											{
												wmemcpy( &s_setup_file.setup.master_key.data[0],		\
														  sizeof(s_setup_file.setup.master_key.data),	\
														  &masterkey.data[0], masterkey.size );
												s_setup_file.setup.master_key.size = masterkey.size;
												bzero( s_masterkey_edit, sizeof(s_masterkey_edit) );
												wmemcpy( s_masterkey_edit, sizeof(s_masterkey_edit),	\
														  &s_setup_file.setup.master_key.data[0], masterkey.size );

												printf("SUCCESS. DOWNLOAD MASTER KEY\r\n" );
											}
										}
										fclose(fp);
									}
									else
									{
										printf("FAIL. FILE OPEN : %s\r\n", MASTERKEY_FILE_PATH );
									}

									snprintf( command, sizeof(command), "rm -rf %s", MASTERKEY_FILE_PATH );
									system( command );

									chdir( curr_path );
									getcwd( curr_path, sizeof(curr_path) );
									printf( "Current Working Dir : %s\r\n", curr_path );
								}							
								else
								{
									printf("FAIL. FILE ACCESS : %s\r\n", MASTERKEY_FILE_PATH );
								}
							}

							sleep( 3 );
							print_menu_sub_masterkey_edit();
						}
						break;
				}
				break;

			case KEY_ESC :
				wmemcpy( &s_setup_file.setup.master_key.data[0],	\
								 sizeof(s_setup_file.setup.master_key.data),	\
								 s_masterkey_edit, sizeof(s_masterkey_edit) );
				s_setup_file.setup.master_key.size = sizeof(s_masterkey_edit);

				quit = 1;
				break;

			case KEY_AUTO_QUIT :
				quit = 2;
				break;
		}
	}

	if( input != KEY_AUTO_QUIT )
	{
		TB_ui_key_init( TRUE );
		print_menu_sub_masterkey();
	}
}

void op_menu_sub_masterkey_info( void )
{
	int input;

	KDN_INFO kcmvp_info;
    if( KDN_GetInfo( &kcmvp_info ) == KDN_OK )
    {
		TB_ui_clear( UI_CLEAR_ALL );

		TB_ui_attrset( FOCUS_OFF );
		
		printw("%s\n", TB_resid_get_string(RESID_TLT_MASTERKEY_LIB_INFO) );
		printw("\n");
		printw("%s : %s\n", TB_resid_get_string(RESID_LBL_MASTERKEY_LIB_NAME), kcmvp_info.libraryName );
		printw("%s : %d.%d\n", TB_resid_get_string(RESID_LBL_MASTERKEY_LIB_INTERFACE_VERSION), kcmvp_info.KdnapiVersion.major, kcmvp_info.KdnapiVersion.minor );
		printw("%s : %d.%d\n", TB_resid_get_string(RESID_LBL_MASTERKEY_LIB_VERSION), kcmvp_info.libraryVersion.major, kcmvp_info.libraryVersion.minor );
		printw("\n");
		printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
		
		TB_ui_refresh();
    }

	if( common_key_loop() != KEY_AUTO_QUIT )
	{
		print_menu_sub_masterkey();	
	}
}

////////////////////////////////////////////////////////////////////////////////

TBUC *get_main_role_name_string( int idx )
{
	static TBUC s_wisun_name[128] = {0, };
	char *str = NULL;
	if( idx == 0 )
	{
		if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
		{
			if( TB_setup_get_product_info_voltage() == VOLTAGE_HIGH )
			{
				str = TB_resid_get_string( RESID_LBL_WISUN_INFO_1ST_GGW );
				if( str )
					snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
			}
			else
			{
				str = TB_resid_get_string( RESID_LBL_WISUN_INFO_1ST_MID );
				if( str )
					snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
			}
		}
		else if( s_setup_file.setup.role == ROLE_GRPGW )
		{
			str = TB_resid_get_string( RESID_LBL_WISUN_INFO_1ST_FRTU );
			if( str )
				snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
		}
		else if( s_setup_file.setup.role >= ROLE_RELAY1 && s_setup_file.setup.role <= ROLE_RELAY3 )
		{
			str = TB_resid_get_string( RESID_LBL_WISUN_INFO_1ST_FRTU );
			if( str )
				snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
		}
		else if( s_setup_file.setup.role >= ROLE_REPEATER )
		{
			str = TB_resid_get_string( RESID_LBL_WISUN_INFO_1ST_GGW );
			if( str )
				snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
		}
	}
	else
	{
		if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
		{
			if( TB_setup_get_product_info_voltage() == VOLTAGE_HIGH )
			{
				str = TB_resid_get_string( RESID_LBL_WISUN_INFO_2ND_UNDERBAR );
				if( str )
					snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
			}
			else
			{
				str = TB_resid_get_string( RESID_LBL_WISUN_INFO_2ND_GGW );
				if( str )
					snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
			}
		}
		else if( s_setup_file.setup.role == ROLE_GRPGW )
		{
			str = TB_resid_get_string( RESID_LBL_WISUN_INFO_2ND_REPEATER );
			if( str )
				snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
		}
		else if( s_setup_file.setup.role >= ROLE_RELAY1 && s_setup_file.setup.role <= ROLE_RELAY3 )
		{
			str = TB_resid_get_string( RESID_LBL_WISUN_INFO_2ND_UNDERBAR );
			if( str )
				snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
		}
		else if( s_setup_file.setup.role >= ROLE_REPEATER )
		{
			str = TB_resid_get_string( RESID_LBL_WISUN_INFO_2ND_UNDERBAR );
			if( str )
				snprintf( s_wisun_name, sizeof(s_wisun_name), "%s", str );
		}
	}

	return s_wisun_name;
}

char *get_main_repeater_enable_string( TBBL enable )
{
	char *p_str = NULL;
	if( s_setup_file.setup.role == ROLE_GRPGW )
		p_str = get_wisun_enable_string( enable );
	
	return p_str;
}

char *get_main_comm_status_string( void )
{
	char *p_str = NULL;
	if( s_setup_file.setup.role == ROLE_REPEATER )
		p_str = TB_resid_get_string( RESID_MAIN_MENU_COMM_STATUS_UNDERBAR );
	else
		p_str = TB_resid_get_string( RESID_MAIN_MENU_COMM_STATUS );

	return p_str;
}

int menu_main_linenum[MID_MAX] = { 2, 3,   5, 6, 7,   9, 10, 11, 12,    14, 15,   17,  19 };
void print_menu_main( void )
{
	char *p_temp = NULL;
	TB_ui_clear( UI_CLEAR_ALL );
	
	TB_ui_attrset( FOCUS_OFF );																printw("%s\n", TB_resid_get_string(RESID_TLT_MAIN) );
	printw("\n");
	s_idx_main==MID_SETTING 	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_SETTING) );
	s_idx_main==MID_COMM_STATUS ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", get_main_comm_status_string() );
	printw("\n");
	s_idx_main==MID_ROLE    	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s%s\n", TB_resid_get_string(RESID_MAIN_MENU_ROLE), get_role_string(s_setup_file.setup.role) );
	s_idx_main==MID_WISUN_1ST	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", get_main_role_name_string(0) );
	s_idx_main==MID_WISUN_2ND	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s", get_main_role_name_string(1) );
	p_temp = get_main_repeater_enable_string(s_setup_file.setup.repeater);
	if( p_temp )
		printw("%s\n", get_main_repeater_enable_string(s_setup_file.setup.repeater) );
	else
		printw("\n" );
	printw("\n");
	s_idx_main==MID_HISTORY		? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_HISTORY) );
	s_idx_main==MID_TIME		? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_TIME) );
	s_idx_main==MID_FACTORY		? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_FACTORY) );
	s_idx_main==MID_DOWNLOAD	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_FWUPDATE) );
	printw("\n");
	s_idx_main==MID_MASTERKEY	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_MASTERKEY) );
	s_idx_main==MID_INFO		? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_INFO) );
	printw("\n");
	s_idx_main==MID_REBOOT		? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_REBOOT) );
	printw("\n");
	if( s_login_type == ACCOUNT_TYPE_ADMIN )
	{
		s_idx_main==MID_ADMIN	? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_ADMIN_SETTING) );
	}
	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );																printw("%s\n", TB_resid_get_string(RESID_TIP_ESC) );
	
	TB_ui_refresh();
}

void update_main_role_name_string( void )
{
	TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[MID_WISUN_1ST], 0);	printw("%s", get_main_role_name_string(0) );
	TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[MID_WISUN_2ND], 0);	printw("%s", get_main_role_name_string(1) );

	if( s_setup_file.setup.role == ROLE_GRPGW )
	{
		char *p_temp = get_main_repeater_enable_string(s_setup_file.setup.repeater);
		if( p_temp )
			printw("%s\n", get_main_repeater_enable_string(s_setup_file.setup.repeater) );
	}
	else
	{
		printw("%s\n", TB_resid_get_string( RESID_LBL_WISUN_INFO_BLANK ) );
	}
}

void update_menu_main( TBSI old, TBSI now )
{
	if( old == MID_COMM_STATUS )
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s", get_main_comm_status_string() );
	}
	else if( old == MID_ROLE )
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s%s", TB_resid_get_string(RESID_MAIN_MENU_ROLE), get_role_string(s_setup_file.setup.role) );
	}
	else if( old == MID_WISUN_1ST )
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s", get_main_role_name_string(0) );
	}
	else if( old == MID_WISUN_2ND )
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s", get_main_role_name_string(1) );
		if( s_setup_file.setup.role == ROLE_GRPGW )
		{
			char *p_temp = get_main_repeater_enable_string(s_setup_file.setup.repeater);
			if( p_temp )
			{
				TB_ui_attrset( FOCUS_OFF );
				printw("%s\n", p_temp );
			}
		}
		else
		{
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s\n", get_main_role_name_string(1) );
		}
	}
	else if( old == MID_HISTORY )
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_HISTORY) );
	}
	else if( old == MID_INFO )
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_INFO) );
	}
	else
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[old], 0);	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_SETTING+old) );
	}

	////////////////////////////////////////////////////////////////////////////
				
	if( now == MID_COMM_STATUS )
	{
		TB_ui_attrset( FOCUS_ON );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s", get_main_comm_status_string() );
	}
	else if( now == MID_ROLE )
	{
		TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_ROLE) );
		TB_ui_attrset( FOCUS_ON );	printw("%s\n", get_role_string(s_setup_file.setup.role) );
	}
	else if( now == MID_WISUN_1ST )
	{
		TB_ui_attrset( FOCUS_ON );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s\n", get_main_role_name_string(0) );
	}
	else if( now == MID_WISUN_2ND )
	{					
		if( s_setup_file.setup.role == ROLE_GRPGW )
		{
			TB_ui_attrset( FOCUS_OFF );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s", get_main_role_name_string(1) );
			char *p_temp = get_main_repeater_enable_string(s_setup_file.setup.repeater);
			if( p_temp )
			{
				TB_ui_attrset( FOCUS_ON );
				printw("%s\n", p_temp );
			}
		}
		else
		{
			TB_ui_attrset( FOCUS_ON );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s", get_main_role_name_string(1) );
		}
	}
	else if( now == MID_HISTORY )
	{
		TB_ui_attrset( FOCUS_ON );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_HISTORY) );
	}
	else if( now == MID_INFO )
	{
		TB_ui_attrset( FOCUS_ON );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s", TB_resid_get_string(RESID_MAIN_MENU_INFO) );
	}
	else
	{
		TB_ui_attrset( FOCUS_ON );	TB_ui_move(menu_main_linenum[now], 0);	printw("%s\n", TB_resid_get_string(RESID_MAIN_MENU_SETTING+now) );
	}
		
	TB_ui_refresh();
}

////////////////////////////////////////////////////////////////////////////////

void op_menu_sub_menu( void )
{
	switch( s_idx_main )
	{
	    case MID_SETTING		: op_menu_sub_setting();	       		break;
		case MID_COMM_STATUS	: op_menu_sub_comm_status();   	 		break;
	    case MID_WISUN_1ST		: op_menu_sub_wisun( 0 );				break;
	    case MID_WISUN_2ND		: op_menu_sub_wisun( 1 );				break;
	    case MID_HISTORY		: op_menu_sub_history();				break;
	    case MID_TIME			: op_menu_sub_time();			    	break;
	    case MID_FACTORY		: op_menu_sub_factory();	    		break;
	    case MID_DOWNLOAD		: op_menu_sub_fwupdate();	       		break;
	    case MID_MASTERKEY		: op_menu_sub_masterkey();	       		break;
	    case MID_INFO			: op_menu_sub_product_info();  			break;
	    case MID_REBOOT			: op_menu_sub_reboot();		   			break;
	    case MID_ADMIN		    : op_menu_sub_admin_setting(); 			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

int TB_setup_check( TB_SETUP_WORK *p_setup )
{
	int ret = -1;

	if( p_setup )
	{		
		if( strcmp(p_setup->model, MODEL_NAME) != 0 )
		{
			TB_dbg_setup("CONFIG : ERROR. Invalid Model Name( %s  vs %s )\n", p_setup->model, MODEL_NAME );
			return ret;
		}
		
		if( p_setup->info.version.major != VERSION_MAJOR || p_setup->info.version.minor != VERSION_MINOR )
		{
			TB_dbg_setup("CONFIG : ERROR. Invalid Version info( %d.%d vs %d.%d)\n",	\
								p_setup->info.version.major,	\
								p_setup->info.version.minor,	\
								VERSION_MAJOR,					\
								VERSION_MINOR );
			return ret;
		}

		if( p_setup->wisun[0].mode <= WISUN_MODE_ERR_MIN || p_setup->wisun[0].mode >= WISUN_MODE_ERR_MAX )
		{
			TB_dbg_setup("CONFIG : ERROR. Invalid WiSUN Mode( %d )\n", p_setup->wisun[0].mode );
			return ret;
		}
		
		if( p_setup->role > ROLE_MAX )
		{
			TB_dbg_setup("CONFIG : ERROR. Invalid WiSUN Position( %d )\n", p_setup->role );
			return ret;
		}
	
		if( p_setup->wisun[0].max_connect > 4 )
		{
			TB_dbg_setup("CONFIG : ERROR. Invalid WiSUN MAX connection ( %d )\n", p_setup->wisun[0].max_connect );
			return ret;
		}
	
		ret = 0;
	}

	return ret;
}

//int TB_setup_save( void )
int __TB_setup_save( char *file, int line )
{
	int  ret = -1;
	int  write_size  = 0;
	TBUC write_buf[4 + sizeof(s_setup_file)] = {0, };	//	4 is 'CAFE'

	if( file )
		TB_dbg_setup( "[%s] Call from [%s:%d]------------\r\n", __FUNCTION__, file, line );
	else
		TB_dbg_setup( "[%s] ------------\r\n", __FUNCTION__ );

	TB_setup_dump( &s_setup_file.setup, (char *)__FUNCTION__ );

	s_setup_file.crc = TB_crc16_modbus_get( (TBUC *)&s_setup_file.setup, sizeof(s_setup_file.setup) );

	bzero( &write_buf[0], sizeof(write_buf) );
	write_buf[write_size] = 'C';	write_size++;
	write_buf[write_size] = 'A';	write_size++;
	write_buf[write_size] = 'F';	write_size++;
	write_buf[write_size] = 'E';	write_size++;

	wmemcpy( &write_buf[write_size], sizeof(write_buf)-write_size, &s_setup_file, sizeof(s_setup_file) );
	write_size += sizeof(s_setup_file);
	//TB_util_data_dump("SETUP LOW DATA for Save", write_buf, write_size );
	if( TB_aes_evp_encrypt_buf2file( write_buf, write_size, SETUP_FILE_PATH ) > 0 )
	{
		TB_dbg_setup("SUCCESS. SAVE TB_SETUP_WORK : %s\r\n", SETUP_FILE_PATH );
		ret = 0;
	}
	else
	{
		TB_dbg_setup("ERROR. SAVE TB_SETUP_WORK : %s\r\n", SETUP_FILE_PATH );
		
		TB_LOGCODE_DATA data;
		bzero( &data, sizeof(data) );
		data.type	 = LOGCODE_DATA_FILE_WRITE_ERROR;
		TB_log_append( LOG_SECU_ERROR_FILE_SETUP, &data, -1 );
	}

	return ret;
}

int TB_setup_read_file( TB_SETUP_WORK *p_config )
{
	int ret = -1;
	int idx = 0;

	TB_SETUP_FILE *p_setup_file = NULL;
	TBUC read_buf[4 + sizeof(s_setup_file)] = {0, };	//	4 is 'CAFE'
	int read_size = TB_aes_evp_decrypt_file2buf( SETUP_FILE_PATH, read_buf, sizeof(read_buf) );
	if( read_size > 0 ) 
	{
		//TB_util_data_dump("SETUP LOW DATA for Read", read_buf, read_size );

		if( read_buf[0] == 'C' && read_buf[1] == 'A' && read_buf[2] == 'F' && read_buf[3] == 'E' )
		{
			idx += 4;

			p_setup_file = (TB_SETUP_FILE *)&read_buf[idx];

			if( p_setup_file->head[0] == 'C' && p_setup_file->head[1] == 'A' && p_setup_file->head[2] == 'F' && p_setup_file->head[3] == 'E' && 
				p_setup_file->tail[0] == 'B' && p_setup_file->tail[1] == 'E' && p_setup_file->tail[2] == 'B' && p_setup_file->tail[3] == 'E' )
			{
				TBUS crc = TB_crc16_modbus_get( (TBUC *)&p_setup_file->setup, sizeof(TB_SETUP_WORK) );
				if( crc == p_setup_file->crc )
				{
					if( TB_setup_check( &p_setup_file->setup ) == 0 )
					{
						if( p_config )
						{
							wmemcpy( p_config, sizeof(TB_SETUP_WORK), &p_setup_file->setup, sizeof(TB_SETUP_WORK) );
						}

						wmemcpy( &s_setup_file, sizeof(s_setup_file),	\
										  p_setup_file, sizeof(TB_SETUP_FILE) );


						TB_dbg_setup("SUCCESS. LOAD SETUP : %s\r\n", SETUP_FILE_PATH );								
						ret = 0;
					}
					else
					{
						TB_dbg_setup("FAIL. LOAD SETUP : %s]r\n", SETUP_FILE_PATH );
					}
				}
				else
				{
					TB_dbg_setup("FAIL. LOAD SETUP : CRC error : %s\r\n", SETUP_FILE_PATH );
				}
			}
			else
			{
				TB_dbg_setup("FAIL. LOAD SETUP : %s\r\n", SETUP_FILE_PATH );
			}

			if( ret == -1 )
			{
				TB_LOGCODE_DATA data;
				bzero( &data, sizeof(data) );
				data.type	 = LOGCODE_DATA_FILE_DATA_ERROR;
				TB_log_append( LOG_SECU_ERROR_FILE_SETUP, &data, -1 );
			}
		}
		else
		{
			TB_dbg_setup("ERROR. SETUP file header : 0x%02X 0x%02X 0x%02X 0x%02X \r\n",	\
							read_buf[0], read_buf[1], read_buf[2], read_buf[3] );
		}
	}
	else
	{
		TB_dbg_setup("FAIL. LOAD SETUP : %s\r\n", SETUP_FILE_PATH );
	}

	if( ret == -1 )
	{
		ret = TB_setup_set_default();
	}

	wmemcpy( &s_setup_file_org, sizeof(s_setup_file_org),	\
					  &s_setup_file, sizeof(TB_SETUP_FILE) );
	
	return ret;
}

int TB_setup_dump( TB_SETUP_WORK *p_setup, char *call_function )
{
	int ret = -1;
	
	if( p_setup )
	{
		TB_dbg_setup( "=======================================\r\n" );
		TB_dbg_setup( "                  SETUP                \r\n" );
		TB_dbg_setup( "=======================================\r\n" );
		TB_dbg_setup( "Call Function : %s\r\n", call_function ? call_function : "Unknown" );
		TB_dbg_setup( "\r\n" );
		TB_dbg_setup( "VERSION           = %d.%d\r\n", 	p_setup->info.version.major, p_setup->info.version.minor );
		TB_dbg_setup( "\r\n" );
		
		TB_dbg_setup( "WISUN ROLE        = %s\r\n", 	get_role_string( p_setup->role ) );

		TB_dbg_setup( "WISUN#1           = %s\r\n",		p_setup->wisun[0].enable ? "ENABLE" : "DISABLE");
		TB_dbg_setup( "    MODE          = %s\r\n", 	get_wisun_mode_string( p_setup->wisun[0].mode ) );
		TB_dbg_setup( "    FREQ          = %s\r\n", 	get_wisun_freq_string( p_setup->wisun[0].frequency ) );
		TB_dbg_setup( "    CONNECTION    = %d\r\n", 	p_setup->wisun[0].max_connect );
		TB_dbg_setup( "\r\n" );
		
		TB_dbg_setup( "WISUN#2           = %s\r\n",		p_setup->wisun[1].enable ? "ENABLE" : "DISABLE");
		TB_dbg_setup( "    MODE          = %s\r\n", 	get_wisun_mode_string( p_setup->wisun[1].mode ) );
		TB_dbg_setup( "    FREQ          = %s\r\n", 	get_wisun_freq_string( p_setup->wisun[1].frequency ) );
		TB_dbg_setup( "    CONNECTION    = %d\r\n", 	p_setup->wisun[1].max_connect );
		TB_dbg_setup( "\r\n" );
		
		TB_dbg_setup( "FRTU Baud Rate    = %d bps\r\n", 	baudrate_table[p_setup->baud.frtu] );
		TB_dbg_setup( "INVT Baud Rate    = %d bps\r\n", 	baudrate_table[p_setup->baud.invt] );
		TB_dbg_setup( "INVT COUNT        = %d \r\n", 		p_setup->invt.cnt );
		TB_dbg_setup( "INVT TIMEOUT      = %d msec\r\n", 	p_setup->invt.timeout * 100 );
		TB_dbg_setup( "INVT RETRY        = %d \r\n", 		p_setup->invt.retry );
		TB_dbg_setup( "INVT Read Delay   = %d msec\r\n", 	p_setup->invt.delay_read );
		TB_dbg_setup( "INVT Write Delay  = %d msec\r\n",	p_setup->invt.delay_write );
		TB_dbg_setup( "\r\n" );

		TB_dbg_setup( "INFO : VERSION    = %s\r\n",	get_product_info_string_fw_version() );
		TB_dbg_setup( "INFO : SERIAL     = %s\r\n",	get_product_info_string_ems_addr() );
		TB_dbg_setup( "INFO : COT IP     = %s\r\n",	get_product_info_string_cot_ip() );
		TB_dbg_setup( "INFO : RT PORT    = %s\r\n",	get_product_info_string_rt_port() );
		TB_dbg_setup( "INFO : FRTU DNP   = %s\r\n",	get_product_info_string_frtu_dnp() );

		TB_util_data_dump( "MASTERKEY KEY", p_setup->master_key.data, p_setup->master_key.size );
		TB_dbg_setup( "=======================================\r\n" );

		ret = 0;
	}
	
	return ret;
}

//int TB_setup_set_default( void )
int __TB_setup_set_default( char *file, int line )
{
	int ret = -1;

	TB_SETUP_FILE *p = &s_setup_file;

	TB_KCMVP_KEY	master_key;
	TB_PRODUCT_INFO info;

	if( file )
		TB_dbg_setup( "[%s] Call from [%s:%d]------------\r\n", __FUNCTION__, file, line );
	else
		TB_dbg_setup( "[%s] ------------\r\n", __FUNCTION__ );

	wmemcpy( &info, sizeof(info), &p->setup.info, sizeof(TB_PRODUCT_INFO) );
	wmemcpy( &master_key, sizeof(master_key), &p->setup.master_key, sizeof(TB_KCMVP_KEY) );

	bzero( p, sizeof(TB_SETUP_FILE) );
	
	p->head[0] = 'C';
	p->head[1] = 'A';
	p->head[2] = 'F';
	p->head[3] = 'E';
	
	wstrncpy( p->setup.model, sizeof(p->setup.model), MODEL_NAME, wstrlen(MODEL_NAME) );

	p->setup.repeater				= FALSE;
	switch( g_device_role )
	{
		case DEV_ROLE_GGW 		:	p->setup.role = ROLE_GRPGW;		break;
		case DEV_ROLE_TERM		:	p->setup.role = ROLE_TERM1;		break;
		case DEV_ROLE_RELAY		:	p->setup.role = ROLE_RELAY1;	break;
		case DEV_ROLE_REPEATER	:	p->setup.role = ROLE_REPEATER;	break;
	}
	
	p->setup.wisun[0].enable		= TRUE;
	switch( g_device_role )
	{
		case DEV_ROLE_GGW 		: 	p->setup.wisun[0].mode = WISUN_MODE_PANC;	break;
		case DEV_ROLE_TERM		:	
		case DEV_ROLE_RELAY		:	p->setup.wisun[0].mode = WISUN_MODE_ENDD;	break;
		case DEV_ROLE_REPEATER	:	p->setup.wisun[0].mode = WISUN_MODE_COORD;	break;
	}
	p->setup.wisun[0].max_connect	= 1;
	p->setup.wisun[0].frequency		= WISUN_FREQ_DEFAULT;
	
	p->setup.wisun[1].enable		= FALSE;
	p->setup.wisun[1].mode			= WISUN_MODE_ENDD;
	p->setup.wisun[1].frequency		= WISUN_FREQ_DEFAULT+1;
	p->setup.wisun[1].max_connect	= 1;
	
	p->setup.comm_type.master		= COMM_MODE_MASTER_WISUN;
	if( g_device_role == DEV_ROLE_TERM )
		p->setup.comm_type.slave	= COMM_MODE_SLAVE_RS485;
	else
		p->setup.comm_type.slave	= COMM_MODE_MASTER_WISUN;
	
	p->setup.baud.frtu 				= SET_DEF_FRTU_BAUD_DEFAULT;	//	9600bps
	p->setup.baud.invt 				= SET_DEF_INVT_BAUD_DEFAULT;	//	9600bps
	p->setup.invt.cnt 				= 1;
	p->setup.invt.timeout			= SET_DEF_INVT_TOUT_DEFAULT;	//	2000msec
	p->setup.invt.retry 			= SET_DEF_INVT_RETRY_DEFAULT;
	p->setup.invt.delay_read		= SET_DEF_INVT_RDELAY_DEFAULT;	//	1000msec
	p->setup.invt.delay_write		= SET_DEF_INVT_WDELAY_DEFAULT;	//	1000msec

	wmemcpy( &p->setup.info, sizeof(p->setup.info), &info, sizeof(TB_PRODUCT_INFO) );
	p->setup.info.version.major 	= VERSION_MAJOR;
	p->setup.info.version.minor 	= VERSION_MINOR;
	p->setup.info.voltage			= VOLTAGE_HIGH;
	p->setup.info.ems_use			= TRUE;
	p->setup.info.manuf_date.year	= MANUF_DATE_YEAR;
	p->setup.info.manuf_date.month	= MANUF_DATE_MONTH;
	p->setup.info.manuf_date.day	= MANUF_DATE_DAY;

	p->setup.auto_lock				= AUTO_LOCK_TIME_DEF;
	p->setup.optical_sensor			= FALSE;
	p->setup.session_update			= 6 * (HOUR_1);	//	06:00:00

	wmemcpy( &p->setup.master_key, sizeof(p->setup.master_key), &master_key, sizeof(TB_KCMVP_KEY) );

	////////////////////////////////////////////////////////////////////////////
	//	Reserved 영역에 의미없는 값으로 채운다.
	////////////////////////////////////////////////////////////////////////////
	srand( 3 );
	for( TBUI i=0; i<sizeof(p->setup.reserved); i++ )
		p->setup.reserved[i] = (TBUC)TB_util_get_rand( 0, 255 );

	p->tail[0] = 'B';
	p->tail[1] = 'E';
	p->tail[2] = 'B';
	p->tail[3] = 'E';

	ret = TB_setup_save();

	if( ret == 0 )
		TB_dbg_setup("SUCCESS. MAKE DEFAULT SETUP_WORK\r\n" );
	else
		TB_dbg_setup("FAIL. MAKE DEFAULT SETUP_WORK\r\n" );

	return ret;
}

int TB_setup_init( void )
{
	int ret = -1;
	
	if( access( SETUP_FILE_PATH, F_OK) != 0 )
		TB_setup_set_default();

	ret = TB_setup_read_file( NULL );
	TB_setup_dump( &s_setup_file.setup, (char *)__FUNCTION__ );

	s_setup_chg_flag = 0;

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

static TBUC s_proc_flag_main = 0;

int menu_main_get_menuid( int curr_menuid, TBBL is_prev )
{
	int loop = 1;
	do
	{
		if( is_prev )
		{
			curr_menuid = --curr_menuid < 0 ? MID_MAX-1: curr_menuid;	//	previoud menuid
		}
		else
		{
			curr_menuid  = ++curr_menuid > MID_MAX-1 ? 0 : curr_menuid;	//	next menuid
		}

		////////////////////////////////////////////////////////////////////////

		if( curr_menuid == MID_WISUN_2ND )
		{
			if( s_setup_file.setup.role >= ROLE_TERM1 &&
				s_setup_file.setup.role <= ROLE_TERM3 )
			{
				TB_VOLTAGE vol = TB_setup_get_product_info_voltage();
				if( vol == VOLTAGE_LOW )
					loop = 0;
			}
			else if( s_setup_file.setup.role == ROLE_GRPGW )
			{
				loop = 0;
			}
		}
		else if( curr_menuid == MID_ADMIN )
		{
			if( s_login_type == ACCOUNT_TYPE_ADMIN )
			{
				loop = 0;
			}
		}
		else
		{
			loop = 0;
		}
	} while( loop );

	return curr_menuid;
}

int menu_main_get_prev_menuid( int curr_menuid )
{
	return menu_main_get_menuid( curr_menuid, TRUE );
}

int menu_main_get_next_menuid( int curr_menuid )
{
	return menu_main_get_menuid( curr_menuid, FALSE );
}

int TB_setup_main_proc( TB_ACCOUNT_TYPE account )
{
	int ret = -1;
	int input;

	int check = TB_dm_is_on( DMID_SETUP );

	TB_dm_dbg_mode_all_off();

	if( check )
		TB_dm_set_mode( DMID_SETUP, DM_ON );

	s_proc_flag_main = 1;

	s_setup_login_account = account;
	s_idx_main = 0;
	print_menu_main();

	attroff( COLOR_PAIR(1) );
	TB_ui_refresh();

	TB_ui_key_init( TRUE );
	while( s_proc_flag_main )
	{
	    input = TB_ui_getch( TRUE );
		if( input == -1 )
		{
			TB_util_delay( 50 );
			continue;
		}
		
	    switch( input )
	    {
			case KEY_LEFT :
				if( s_idx_main == MID_ROLE )
				{
					TB_VOLTAGE vol = TB_setup_get_product_info_voltage();
					if( vol == VOLTAGE_HIGH )
					{
						while( 1 )
						{
							s_setup_file.setup.role = --s_setup_file.setup.role < 0 ? ROLE_MAX : s_setup_file.setup.role;
							if( !(s_setup_file.setup.role >= ROLE_RELAY1 && s_setup_file.setup.role <= ROLE_RELAY3) )
							{
								break;
							}
						}
					}
					else
					{
						s_setup_file.setup.role = --s_setup_file.setup.role < 0 ? ROLE_MAX : s_setup_file.setup.role;
					}
					
					TB_ui_attrset( FOCUS_OFF );
					TB_ui_move(menu_main_linenum[MID_ROLE], 0);
					printw("%s", TB_resid_get_string(RESID_MAIN_MENU_ROLE) );
					
					TB_ui_attrset( FOCUS_ON );
					printw("%s", get_role_string(s_setup_file.setup.role) );

					if( vol == VOLTAGE_HIGH )
					{
						if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
						{
							s_setup_file.setup.wisun[0].enable = TRUE;
							s_setup_file.setup.wisun[1].enable = FALSE;

							s_setup_file.setup.wisun[0].mode = WISUN_MODE_ENDD;
						}
					}
					else
					{
						if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
						{
							s_setup_file.setup.wisun[0].enable = TRUE;
							s_setup_file.setup.wisun[1].enable = TRUE;
						}
						else
						{
							s_setup_file.setup.wisun[0].enable = TRUE;
							s_setup_file.setup.wisun[1].enable = FALSE;
						}

						if( s_setup_file.setup.role >= ROLE_RELAY1 && s_setup_file.setup.role <= ROLE_RELAY3 )
						{
							s_setup_file.setup.wisun[0].mode = WISUN_MODE_ENDD;
						}
						if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
						{
							s_setup_file.setup.wisun[0].mode = WISUN_MODE_PANC;
							s_setup_file.setup.wisun[1].mode = WISUN_MODE_ENDD;
						}
						else if( s_setup_file.setup.role == ROLE_GRPGW )
						{
							s_setup_file.setup.wisun[0].mode = WISUN_MODE_PANC;
						}
					}

					update_main_role_name_string();
				}
				else if( s_idx_main == MID_WISUN_2ND )
				{
					if( s_setup_file.setup.role == ROLE_GRPGW )
					{
						s_setup_file.setup.repeater = !s_setup_file.setup.repeater;

						TB_ui_attrset( FOCUS_OFF );
						TB_ui_move(menu_main_linenum[MID_WISUN_2ND], 0);
						printw("%s", get_main_role_name_string(1) );
						
						TB_ui_attrset( FOCUS_ON );
						printw("%s", get_main_repeater_enable_string(s_setup_file.setup.repeater) );
					}
				}
				break;
				
			case KEY_RIGHT :
				if( s_idx_main == MID_ROLE )
				{
					TB_VOLTAGE vol = TB_setup_get_product_info_voltage();
					if( vol == VOLTAGE_HIGH )
					{
						while( 1 )
						{
							s_setup_file.setup.role = ++s_setup_file.setup.role > ROLE_MAX ? 0 : s_setup_file.setup.role;
							if( !(s_setup_file.setup.role >= ROLE_RELAY1 && s_setup_file.setup.role <= ROLE_RELAY3) )
							{
								break;
							}
						}
					}
					else
					{
						s_setup_file.setup.role = ++s_setup_file.setup.role > ROLE_MAX ? 0 : s_setup_file.setup.role;
					}					
					
					TB_ui_attrset( FOCUS_OFF );
					TB_ui_move(menu_main_linenum[MID_ROLE], 0);
					printw("%s", TB_resid_get_string(RESID_MAIN_MENU_ROLE) );
					
					TB_ui_attrset( FOCUS_ON );
					printw("%s", get_role_string(s_setup_file.setup.role) );

					if( vol == VOLTAGE_HIGH )
					{
						if( s_setup_file.setup.role >= ROLE_TERM1 && s_setup_file.setup.role <= ROLE_TERM3 )
						{
							s_setup_file.setup.wisun[0].enable = TRUE;
							s_setup_file.setup.wisun[1].enable = FALSE;

							s_setup_file.setup.wisun[0].mode = WISUN_MODE_ENDD;
						}
					}
					else
					{
						if( s_setup_file.setup.role == ROLE_TERM1 && s_setup_file.setup.role == ROLE_TERM3 )
						{
							s_setup_file.setup.wisun[0].enable = TRUE;
							s_setup_file.setup.wisun[1].enable = TRUE;
						}
						else
						{
							s_setup_file.setup.wisun[0].enable = TRUE;
							s_setup_file.setup.wisun[1].enable = FALSE;
						}

						if( s_setup_file.setup.role >= ROLE_RELAY1 && s_setup_file.setup.role <= ROLE_RELAY3 )
						{
							s_setup_file.setup.wisun[0].mode = WISUN_MODE_ENDD;
						}
						else if( s_setup_file.setup.role == ROLE_GRPGW )
						{
							s_setup_file.setup.wisun[0].mode = WISUN_MODE_PANC;
						}
					}

					update_main_role_name_string();
				}
				else if( s_idx_main == MID_WISUN_2ND )
				{
					if( s_setup_file.setup.role == ROLE_GRPGW )
					{
						s_setup_file.setup.repeater = !s_setup_file.setup.repeater;

						TB_ui_attrset( FOCUS_OFF );
						TB_ui_move(menu_main_linenum[MID_WISUN_2ND], 0);
						printw("%s", get_main_role_name_string(1) );
						
						TB_ui_attrset( FOCUS_ON );
						printw("%s", get_main_repeater_enable_string(s_setup_file.setup.repeater) );
					}
				}
				break;
				
		    case KEY_UP 	:
				{
					TBSI idx_main_old = s_idx_main;
					s_idx_main = menu_main_get_prev_menuid( s_idx_main );
					update_menu_main( idx_main_old, s_idx_main );
		    	}
				break;

		    case KEY_DOWN 	:
				{
					TBSI idx_main_old = s_idx_main;
					s_idx_main = menu_main_get_next_menuid( s_idx_main );
					update_menu_main( idx_main_old, s_idx_main );
		    	}
	       		break;

			case KEY_USER_HOME :
				{
					TBSI idx_main_old = s_idx_main;
					s_idx_main = MID_SETTING;	//	first menu
					update_menu_main( idx_main_old, s_idx_main );
				}
				break;
				
			case KEY_USER_END :
				{
					TBSI idx_main_old = s_idx_main;
					s_idx_main = MID_MAX-1;		//	last menu
					if( s_idx_main == MID_ADMIN )
					{
						if( s_login_type != ACCOUNT_TYPE_ADMIN )
							s_idx_main --;
					}
					update_menu_main( idx_main_old, s_idx_main );
				}
				break;
				
		    case '\n' 	:
		    case KEY_ENTER 	:
				op_menu_sub_menu();
				break;
				
			case KEY_ESC  :
				if( TB_kcmvp_key_check() != 0 )
				{
					TB_show_msgbox( TB_msgid_get_string(MSGID_MASTERKEY), 0, TRUE );
					print_menu_main();
				}
				else
				{
					TB_setup_save();
					s_proc_flag_main = 0;
				}
			    break;

			case KEY_AUTO_QUIT :
				s_proc_flag_main = 0;
				break;
	    }
	}

	////////////////////////////////////////////////////////////////////////////

	if( memcmp(&s_setup_file, &s_setup_file_org, sizeof(s_setup_file_org)) != 0 )
	{
		TB_SETUP_WORK *p_setup1 = &s_setup_file.setup;
		TB_SETUP_WORK *p_setup2 = &s_setup_file_org.setup;
		
		if( memcmp(&p_setup1->role, &p_setup2->role, sizeof(TB_ROLE)) != 0 )
			s_setup_chg_flag |= SETUP_CHG_FLAG_ROLE;
		
		if( memcmp(&p_setup1->baud, &p_setup2->baud, sizeof(TB_SETUP_BAUD)) != 0 )
			s_setup_chg_flag |= SETUP_CHG_FLAG_SETTING;
		
		if( memcmp(&p_setup1->invt, &p_setup2->invt, sizeof(TB_SETUP_INVT)) != 0 )
			s_setup_chg_flag |= SETUP_CHG_FLAG_SETTING;
		
		if( memcmp(&p_setup1->wisun[0], &p_setup2->wisun[0], sizeof(TB_SETUP_WISUN)) != 0 )
			s_setup_chg_flag |= SETUP_CHG_FLAG_WISUN1;
		
		if( memcmp(&p_setup1->wisun[1], &p_setup2->wisun[1], sizeof(TB_SETUP_WISUN)) != 0 )
			s_setup_chg_flag |= SETUP_CHG_FLAG_WISUN2;

		if( memcmp(&p_setup1->master_key, &p_setup2->master_key, sizeof(TB_KCMVP_KEY)) != 0 )
			s_setup_chg_flag |= SETUP_CHG_FLAG_MASTERKEY;

		if( memcmp(&p_setup1->info, &p_setup2->info, sizeof(TB_PRODUCT_INFO)) != 0 )
			s_setup_chg_flag |= SETUP_CHG_FLAG_INFO;

		TB_LOGCODE_DATA data;
		data.type = LOGCODE_DATA_SETUP_CHANGE;
		data.size = sizeof( s_setup_chg_flag );
		wmemcpy( data.data, sizeof(data.data), &s_setup_chg_flag, data.size );
		TB_log_append( LOG_SECU_SETUP, &data, s_login_type );

		ret = 1;
	}
	else
	{
		ret = 0;
	}

	TB_dm_dbg_mode_restore();

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

TBBL g_setup_mode = FALSE;
int TB_setup_run( TB_ACCOUNT_TYPE account )
{
	g_setup_mode = TRUE;
	TB_ui_init();
	
	int login = TB_login_show_input_pw( account );
	if( login != LOGIN_ERROR )
	{
		s_login_type = (TB_ACCOUNT_TYPE)login;
		
		TB_setup_init();
		if( TB_setup_main_proc( s_login_type ) == 1 )
		{
			TB_show_msgbox( TB_msgid_get_string(MSGID_REBOOT), 5, FALSE );
			TB_wdt_reboot( WDT_COND_CHANGE_CONFIG );
		}
	}

	TB_ui_deinit();
	g_setup_mode = FALSE;

	return login;
}

static pthread_t s_thid_setup;
void *TB_setup_thread( void *arg )
{
	TB_ACCOUNT_TYPE account = *(TB_ACCOUNT_TYPE *)arg;
	TB_setup_run( account );
}

void TB_setup_thread_start( TB_ACCOUNT_TYPE account )
{
	pthread_create( &s_thid_setup, NULL, TB_setup_thread, (void *)&account );
}

void TB_setup_thread_stop( void )
{
	pthread_join( s_thid_setup, NULL );
#if 1
	printf( "*************************************************\r\n");
	printf( "                 Exited the setup.               \r\n");
	printf( "*************************************************\r\n");
#endif
}

////////////////////////////////////////////////////////////////////////////////

int TB_setup_enter( TB_ACCOUNT_TYPE account, TBBL block_mode )
{
	int login = -1;
	
	if( block_mode )
	{
		login = TB_setup_run( account );
	}
	else
	{
		login = 0;
		TB_setup_thread_start( account );
		TB_setup_thread_stop();
	}

	return login;
}

TBBL TB_setup_is_run( void )
{
	return g_setup_mode;
}

int TB_setup_auto_exit( void )
{
	TB_ui_deinit();
}

