#ifndef	__TB_SETUP_H__
#define __TB_SETUP_H__

#include "TB_type.h"
#include "TB_login.h"
#include "TB_kcmvp.h"

////////////////////////////////////////////////////////////////////////////////

#define SIZE_COT_IP					4

#define SETUP_CHG_FLAG_NONE			0x0000
#define SETUP_CHG_FLAG_SETTING		0x0001
#define SETUP_CHG_FLAG_ROLE			0x0002
#define SETUP_CHG_FLAG_WISUN1		0x0004
#define SETUP_CHG_FLAG_WISUN2		0x0008
#define SETUP_CHG_FLAG_TIME			0x0010
#define SETUP_CHG_FLAG_MASTERKEY	0x0020
#define SETUP_CHG_FLAG_INFO			0x0040

////////////////////////////////////////////////////////////////////////////////

#define SET_DEF_FRTU_BAUD_DEFAULT	2
#define SET_DEF_FRTU_BAUD_MIN		0
#define SET_DEF_FRTU_BAUD_MAX		6
#define SET_DEF_INVT_BAUD_DEFAULT	2
#define SET_DEF_INVT_BAUD_MIN		0
#define SET_DEF_INVT_BAUD_MAX		6
#define SET_DEF_INVT_CNT_MIN		0
#define SET_DEF_INVT_CNT_MAX		40
#define SET_DEF_INVT_TOUT_DEFAULT	20
#define SET_DEF_INVT_TOUT_MIN		1
#define SET_DEF_INVT_TOUT_MAX		50
#define SET_DEF_INVT_RETRY_DEFAULT	1
#define SET_DEF_INVT_RETRY_MIN		1
#define SET_DEF_INVT_RETRY_MAX		3
#define SET_DEF_INVT_RDELAY_DEFAULT	1000
#define SET_DEF_INVT_RDELAY_MIN		100
#define SET_DEF_INVT_RDELAY_MAX		2000
#define SET_DEF_INVT_WDELAY_DEFAULT	1000
#define SET_DEF_INVT_WDELAY_MIN		100
#define SET_DEF_INVT_WDELAY_MAX		2000

////////////////////////////////////////////////////////////////////////////////

typedef enum tagROLE
{
	ROLE_ERR_MIN 	= -1,
		
	ROLE_GRPGW		= 0,
	
	ROLE_TERM 		= 1,
	ROLE_TERM1 		= 1,
	ROLE_TERM2,
	ROLE_TERM3,

	ROLE_RELAY 		= 4,
	ROLE_RELAY1		= 4,
	ROLE_RELAY2,
	ROLE_RELAY3,

	ROLE_REPEATER,
	
	ROLE_ERR_MAX,
} TB_ROLE;

#define ROLE_MIN			ROLE_GRPGW
#define ROLE_MAX			ROLE_REPEATER
#define ROLE_RELAY_MIN		ROLE_RELAY1
#define ROLE_RELAY_MAX		ROLE_RELAY3
#define ROLE_TERM_MIN		ROLE_TERM1
#define ROLE_TERM_MAX		ROLE_TERM3

typedef enum tagWISUN_MODE
{
	WISUN_MODE_ERR_MIN	= -1,
		
	WISUN_MODE_PANC		= 0,
	WISUN_MODE_COORD,
	WISUN_MODE_ENDD,
	
	WISUN_MODE_ERR_MAX,
} TB_WISUN_MODE;
#define WISUN_MODE_MIN		WISUN_MODE_PANC
#define WISUN_MODE_MAX		WISUN_MODE_ENDD

typedef enum tagFREQUENCY
{
	WISUN_FREQ_ERR_MIN	= -1,
	WISUN_FREQ_922_5	= 0,
	WISUN_FREQ_922_9,
	WISUN_FREQ_923_3,
	WISUN_FREQ_923_7,
	WISUN_FREQ_924_1,
	WISUN_FREQ_924_5,
	WISUN_FREQ_924_9,
	WISUN_FREQ_925_3,
	WISUN_FREQ_925_7,
	WISUN_FREQ_926_1,
	WISUN_FREQ_926_5,
	WISUN_FREQ_926_9,
	WISUN_FREQ_927_3,
	WISUN_FREQ_927_7,
	WISUN_FREQ_ERR_MAX
} TB_WISUN_FREQ;

#define WISUN_FREQ_MIN		WISUN_FREQ_922_5
#define WISUN_FREQ_MAX		WISUN_FREQ_927_7
#define WISUN_FREQ_DEFAULT	WISUN_FREQ_922_5
#define WISUN_FREQ_ADJUST	4

#define	COMM_MODE_MASTER_WISUN		0
#define	COMM_MODE_MASTER_LTE		1
#define COMM_MODE_MASTER_RS232		2
#define COMM_MODE_MASTER_CNT		3

#define	COMM_MODE_SLAVE_WISUN		0
#define COMM_MODE_SLAVE_RS485		1
#define COMM_MODE_SLAVE_CNT			2

#define WISUN_FIRST		0
#define WISUN_SECOND	1
#define WISUN_REPEATER	2

////////////////////////////////////////////////////////////////////////////////

#define SETUP_ENTER_WAIT_TIME	5	//	sec

////////////////////////////////////////////////////////////////////////////////

typedef struct tagWISUN
{
	TBBL			enable;
	TB_WISUN_MODE 	mode;
	TB_WISUN_FREQ	frequency;	//	0 ~ 13
	TBUC			max_connect;
} TB_SETUP_WISUN;

#define MIN_INVT_CNT		0
#define MAX_INVT_CNT		40
#define MIN_INVT_TIMEOUT	1
#define MAX_INVT_TIMEOUT	50
#define MIN_INVT_RETRY		1
#define MAX_INVT_RETRY		3

typedef struct tagINVT
{
	TBSC cnt;			//	 0 ~ 40
	TBSC timeout;		//	 1 ~ 50 --> 100ms ~ 5000ms
	TBSC retry;			//	 1 ~ 3
	TBUS delay_read;	//	 100 ~ 2000 msec	: 다음 인버터를 읽기 위한 지연 시간
	TBUS delay_write;	//	 100 ~ 2000 msec	: 인버터 데이터를 상위 WISUN 또는 DNP로 Write한 후
						//						  다음 인버터 데이터를 Write하기 위한 지연 시간
} TB_SETUP_INVT;

typedef struct tagBAUD
{
	TBSC frtu;
	TBSC invt;
} TB_SETUP_BAUD;

typedef struct tagCOMM_TYPE
{
	TBSI master;
	TBSI slave;
} TB_COMM_TYPE;

typedef struct tagVERSION
{
	TBUC major;
	TBUC minor;
} TB_VERSION;

typedef enum tagVOLTAGE
{
	VOLTAGE_LOW,
	VOLTAGE_HIGH
} TB_VOLTAGE;

typedef struct tagINFO
{
	TB_VERSION		version;
	TB_VOLTAGE		voltage;
	TBUS			ems_dest;			//	0 ~ 65535
	TBUS			ems_addr;			//	0 ~ 65535
	TB_DATE			manuf_date;
	TBUC			cot_ip[SIZE_COT_IP];//	IPv4
	TBUS			rt_port;			//	1 ~ 9999
	TBUS			frtp_dnp;			//	0 ~ 65535
	TBBL			ems_use;
} TB_PRODUCT_INFO;

////////////////////////////////////////////////////////////////////////////////

typedef struct tagSETUP_WORK
{
	TBUC				model[32];	
	TB_ROLE 			role;
	TBBL				repeater;
	TB_SETUP_WISUN		wisun[2];
	TB_SETUP_BAUD		baud;
	TB_SETUP_INVT		invt;
	TB_COMM_TYPE		comm_type;		//	FRTU에서만 활성화. 상위 / 하위와 통신 방법 (WISUN, LTE, RE-232)
	TB_KCMVP_KEY		master_key;
	TB_PRODUCT_INFO		info;
	TBSI 				auto_lock;
	TBBL 				optical_sensor;
	time_t				session_update;

	TBUC				reserved[32];	
} TB_SETUP_WORK;

typedef struct __attribute__((__packed__)) tagSETUP_FILE
{
	TBUC 				head[4];
	TB_SETUP_WORK		setup;
	TBUS				crc;
	TBUC 				tail[4];
} TB_SETUP_FILE;

////////////////////////////////////////////////////////////////////////////////

extern TBSC TB_setup_get_invt_timeout( void );
extern TBSC TB_setup_get_invt_retry( void );
extern TBSC TB_setup_get_invt_count( void );
extern TBUS TB_setup_get_invt_delay_read( void );
extern TBUS TB_setup_get_invt_delay_write( void );
extern TB_SETUP_INVT *TB_setup_get_invt_info( void );
extern int TB_setup_set_invt_info( TB_SETUP_INVT *p_invt );

extern TB_SETUP_WISUN *TB_setup_get_wisun_info( TBUC idx );
extern TB_WISUN_MODE TB_setup_get_wisun_info_mode( TBUC idx );
extern TBBL TB_setup_get_wisun_info_enable( TBUC idx );
extern void TB_setup_set_wisun_info_enable( TBUC idx, TBBL enable );
extern TB_WISUN_FREQ TB_setup_get_wisun_info_freq( TBUC idx );
extern TBUC TB_setup_get_wisun_info_max_connect( TBUC idx );
extern TBSC TB_setup_get_baud_frtu( void );
extern TBSC TB_setup_get_baud_inverter( void );
extern TB_ROLE TB_setup_get_role( void );
extern TBBL TB_setup_get_wisun_repeater( void );
extern TB_KCMVP_KEY *TB_setup_get_kcmvp_master_key( void );
extern int TB_setup_set_kcmvp_master_key( TB_KCMVP_KEY *p_key );
extern TB_COMM_TYPE TB_setup_get_comm_type( void );
extern TBSI TB_setup_get_comm_type_master( void );
extern TBSI TB_setup_get_comm_type_slave( void );
extern TBSI TB_setup_get_auto_lock( void );
extern TBBL TB_setup_get_optical_sensor( void );
extern time_t TB_setup_get_session_update_time( void );

extern TB_PRODUCT_INFO *TB_setup_get_product_info( void );
extern int TB_setup_set_product_info( TB_PRODUCT_INFO *p_info );
extern TB_VERSION *TB_setup_get_product_info_version( void );
extern TB_VOLTAGE TB_setup_get_product_info_voltage( void );
extern TBUI TB_setup_get_product_info_ems_dest( void );
extern TBUS TB_setup_get_product_info_ems_addr( void );
extern TB_DATE TB_setup_get_product_info_manuf_date( void );
extern TBUC *TB_setup_get_product_info_cot_ip( void );
extern TBUS TB_setup_get_product_info_rt_port( void );
extern TBUS TB_setup_get_product_info_frtp_dnp( void );
extern TBBL TB_setup_get_product_info_ems_use( void );

extern TB_SETUP_WORK *TB_setup_get_config( void );
extern int __TB_setup_set_default( char *file, int line );
#define TB_setup_set_default()	__TB_setup_set_default(__FILE__, __LINE__)
extern int __TB_setup_save( char *file, int line );
#define TB_setup_save()	__TB_setup_save(__FILE__, __LINE__)
extern int TB_setup_read_file( TB_SETUP_WORK *p_config );
extern int TB_setup_init( void );
extern int TB_setup_dump( TB_SETUP_WORK *p_setup, char *call_function );

extern int  TB_setup_enter( TB_ACCOUNT_TYPE account, TBBL block_mode );
extern TBBL TB_setup_is_run( void );

#endif//__TB_SETUP_H__

