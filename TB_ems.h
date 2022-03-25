#ifndef	__TB_EMS_H__
#define __TB_EMS_H__

#include "TB_psu.h"
#include "TB_type.h"

#define STX				0x02
#define ETX				0x03

////////////////////////////////////////////////////////////////////////////////

#define SIZE_STX 		1
#define SIZE_EQU		1
#define SIZE_UDB_LEN 	2
#define SIZE_DEST 		2
#define SIZE_SOUR 		2
#define SIZE_ORG 		1
#define SIZE_CTRL		1
#define SIZE_CRC		2
#define SIZE_ETX		1

#define EMS_PACKET_OFFSET_STX		0
#define EMS_PACKET_OFFSET_EQU		(EMS_PACKET_OFFSET_STX	+ SIZE_STX)
#define EMS_PACKET_OFFSET_UDBLEN	(EMS_PACKET_OFFSET_EQU	+ SIZE_EQU)
#define EMS_PACKET_OFFSET_DEST		(EMS_PACKET_OFFSET_UDBLEN+SIZE_UDB_LEN)
#define EMS_PACKET_OFFSET_SOUR		(EMS_PACKET_OFFSET_DEST	+ SIZE_DEST)
#define EMS_PACKET_OFFSET_TAG		(EMS_PACKET_OFFSET_SOUR	+ SIZE_SOUR)
#define EMS_PACKET_OFFSET_ORG		(EMS_PACKET_OFFSET_TAG	+ LEN_BYTE_TAG)
#define EMS_PACKET_OFFSET_CTRL		(EMS_PACKET_OFFSET_ORG	+ SIZE_ORG)
#define EMS_PACKET_OFFSET_UDB		(EMS_PACKET_OFFSET_CTRL	+ SIZE_CTRL)

////////////////////////////////////////////////////////////////////////////////

#define EQU_TYPE_EMS				0x01
#define EQU_TYPE_DIST_LOW_RLY		0x02
#define EQU_TYPE_DIST_LOW_TERM		0x03
#define EQU_TYPE_DIST_HIGH_TERM		0x04
#define EQU_TYPE_DIST_GGW			0x05
#define EQU_TYPE_MAX				(EQU_TYPE_DIST_GGW+1)

////////////////////////////////////////////////////////////////////////////////

#define FUNC_ORG_AUTH						0x01
#define FUNC_ORG_AUTH_CTRL_AUTH_REQ			0x01	//	인증요청			기기-->EMS
#define FUNC_ORG_AUTH_CTRL_AUTH_RESULT		0x02	//	인증요청결과		EMS-->기기
#define FUNC_ORG_AUTH_CTRL_SESSION_KEY_SEND	0x03	//	세션키 전송			EMS-->기기
#define FUNC_ORG_AUTH_CTRL_SESSION_KEY_RECV	0x04	//	세션키 수신 응답	기기-->EMS
#define FUNC_ORG_AUTH_CTRL_REAUTH_REQ_SEND	0x05	//	인증 요청			EMS-->기기

#define FUNC_ORG_STATE						0x02
#define FUNC_ORG_STATE_CTRL_REQ_COMM		0x01
#define FUNC_ORG_STATE_CTRL_ACK_COMM		0x02
#define FUNC_ORG_STATE_CTRL_REQ_INFO		0x03
#define FUNC_ORG_STATE_CTRL_ACK_INFO		0x04
#define FUNC_ORG_STATE_CTRL_REQ_COMM_DBM	0x05
#define FUNC_ORG_STATE_CTRL_ACK_COMM_DBM	0x06
#define FUNC_ORG_STATE_CTRL_REQ_TIME		0x07
#define FUNC_ORG_STATE_CTRL_ACK_TIME		0x08
#define FUNC_ORG_STATE_CTRL_REQ_DEVICE_INFO	0x09
#define FUNC_ORG_STATE_CTRL_ACK_DEVICE_INFO	0x0A

#define FUNC_ORG_SET						0x03
#define FUNC_ORG_SET_CTRL_REQ_TIME_SYNC		0x01
#define FUNC_ORG_SET_CTRL_ACK_TIME_SYNC		0x02

#define FUNC_ORG_CTRL						0x04
#define FUNC_ORG_CTRL_CTRL_REQ_RESET		0x01
#define FUNC_ORG_CTRL_CTRL_ACK_RESET		0x02

////////////////////////////////////////////////////////////////////////////////

typedef struct tagEMS_PACKET
{
	TBUC stx;
	TBUC equ;
	TBUS udb_len;
	TBUS dest;
	TBUS sour;
	TBUC org;
	TBUC ctrl;
	TBUC udb[MAX_DNP_BUF];
	TBUS crc;
	TBUC etx;
} TB_EMS_PACKET;

typedef enum tagEMS_AUTH_STATE
{
	EMS_AUTH_STATE_READY,
	EMS_AUTH_STATE_REQ,
	EMS_AUTH_STATE_SUCCESS,
	EMS_AUTH_STATE_RECV_KEY,
	EMS_AUTH_STATE_RECV_KEY_ACK,
	EMS_AUTH_STATE_FINISH,
	EMS_AUTH_STATE_RETRY,
	EMS_AUTH_STATE_RETRY_FORCE,

	EMS_AUTH_STATE_MAX
} TB_EMS_AUTH_STATE;

typedef struct tagEMS_DNP
{
	TBUC buffer[MAX_DNP_BUF];
	TBUI length;
} TB_EMS_BUF;

////////////////////////////////////////////////////////////////////////////////

extern int 	TB_ems_init( void );
extern TBUS TB_ems_read( void );
extern TBUS TB_ems_write( TBUC *buf, TBUS len );
extern void TB_ems_deinit( void );
extern void TB_ems_proc_start( void );
extern void TB_ems_proc_stop( void );

//extern void TB_ems_set_auth_status( TB_EMS_AUTH_STATE status );
extern void __TB_ems_set_auth_status( TB_EMS_AUTH_STATE status, char *file, int line );
#define TB_ems_set_auth_status(s)	__TB_ems_set_auth_status(s,(char *)__FILE__,(int)__LINE__)
extern TBUC TB_ems_auth_req( void );

extern TBUS TB_ems_convert_emsbuf2packet( TB_EMS_BUF *p_ems_buf, TBUS offset, TB_EMS_PACKET *p_packet );
extern int  TB_ems_convert_emsdata2packet( TBUC *p_ems_data, TBUI ems_data_len, TB_EMS_PACKET *p_packet );

extern void TB_ems_test_proc_start( void );
extern void TB_ems_test_proc_stop( void );

#endif//__TB_EMS_H__

