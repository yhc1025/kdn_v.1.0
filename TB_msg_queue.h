#ifndef	__TB_MSG_QUEUE_H__
#define	__TB_MSG_QUEUE_H__

#include <sys/types.h>  
#include <sys/ipc.h>  
#include <sys/msg.h>  
#include <sys/stat.h>

#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

#define	WAIT				0
#define	NOWAIT				1

#ifndef TRUE
#define TRUE				(1)
#endif

#ifndef FALSE
#define FALSE				(0)
#endif

////////////////////////////////////////////////////////////////////////////////

typedef enum tagMSG_TYPE
{
	MSG_TYPE_INVERTER_EACH		= 0x0010,
	MSG_TYPE_INVERTER			= 0x0100,
	MSG_TYPE_MODEM				= 0x0200,
	MSG_TYPE_WISUN_TERM2RELAY	= 0x0300,
	MSG_TYPE_WISUN_GGW2TERM		= 0x0400,
	MSG_TYPE_PSU_MOD			= 0x0500,
	MSG_TYPE_PSU_MOD_READY		= 0x0520,
	MSG_TYPE_PSU_DNP			= 0x0600,
	MSG_TYPE_LOG				= 0x0700,
	MSG_TYPE_LTE				= 0x0800,
	MSG_TYPE_LTE_READY			= 0x0820,
	MSG_TYPE_EMS				= 0x1000,
	MSG_TYPE_FRTU				= 0x2000,
	MSG_TYPE_OPTICAL			= 0x3000,
} TB_MSG_TYPE;

typedef enum tagMSG_CMD
{
	MSG_CMD_INVERTER_READ_IMMEDI_REQ		= MSG_TYPE_INVERTER_EACH,
	MSG_CMD_INVERTER_READ_IMMEDI_RES,
		
	MSG_CMD_INVERTER_READ 					= MSG_TYPE_INVERTER,
	MSG_CMD_INVERTER_WRITE,
	MSG_CMD_INVERTER_WISUN_READY,

	MSG_CMD_MODEM_READ 						= MSG_TYPE_MODEM,
	MSG_CMD_MODEM_WRITE,

	MSG_CMD_WISUN_SEND_RELAY2TERM			= MSG_TYPE_WISUN_TERM2RELAY,
	MSG_CMD_WISUN_SEND_TERM2RELAY1,
	MSG_CMD_WISUN_SEND_TERM2RELAY2,
	MSG_CMD_WISUN_SEND_TERM2RELAY3,

	/*
	*	GGW에 접속된 장치들의 IP주소와 Role 정보 요청(Terminal, Repeater)
	*/
	MSG_CMD_WISUN_SEND_REQ_IP_FROM_GGW,
	MSG_CMD_WISUN_SEND_REQ_IP_FROM_TERM,

	MSG_CMD_WISUN_SEND_REQ_TIMEINFO_FROM_GGW,
	MSG_CMD_WISUN_SEND_REQ_TIMEINFO_FROM_RELAY,

	MSG_CMD_WISUN_SEND_REQ_AVAILABLE_TIMEINFO,

	MSG_CMD_WISUN_INVERTER_DATA_REQ,
	MSG_CMD_WISUN_INVERTER_DATA_RES,
	
	MSG_CMD_WISUN_SEND_TERM2GGW				= MSG_TYPE_WISUN_GGW2TERM,
	MSG_CMD_WISUN_SEND_GGW2TERM1,
	MSG_CMD_WISUN_SEND_GGW2TERM2,
	MSG_CMD_WISUN_SEND_GGW2TERM3,
	MSG_CMD_WISUN_SEND_GGW2REPEATER,

	MSG_CMD_PSU_MOD_READ					= MSG_TYPE_PSU_MOD,
	MSG_CMD_PSU_MOD_WRITE,
	MSG_CMD_PSU_MOD_REQ_TIME,
	MSG_CMD_PSU_MOD_RES_TIME,

	MSG_CMD_PSU_MOD_WRITE_READY 			= MSG_TYPE_PSU_MOD_READY,

	MSG_CMD_PSU_DNP_READ					= MSG_TYPE_PSU_DNP,
	MSG_CMD_PSU_DNP_WRITE,

	MSG_CMD_LOG_APPEND						= MSG_TYPE_LOG,
	MSG_CMD_LOG_SAVE,

	MSG_CMD_LTE_READ 						= MSG_TYPE_LTE,
	MSG_CMD_LTE_WRITE,

	MSG_CMD_LTE_WRITE_READY 				= MSG_TYPE_LTE_READY,
	
	MSG_CMD_EMS_READ						= MSG_TYPE_EMS,
	MSG_CMD_EMS_WRITE,
	MSG_CMD_EMS_CHECK,
	
	MSG_CMD_FRTU_READ						= MSG_TYPE_FRTU,
	MSG_CMD_FRTU_WRITE,
	MSG_CMD_FRTU_BYPASS,

	MSG_CMD_OPTICAL_READ					= MSG_TYPE_OPTICAL,
	MSG_CMD_OPTICAL_WRITE,

	MSG_CMD_RECV_DATA_ACK,

} TB_MSG_CMD;

typedef struct tagTB_MESSAGE
{ 
    long 	type;		// message type
	TBUI	id;			// message id
    TBVD *	data;		// user data
	TBUS	size;		// data size
} TB_MESSAGE;

////////////////////////////////////////////////////////////////////////////////

extern int 	TB_msgq_init( void );
extern int 	TB_msgq_deinit( void );
extern int 	TB_msgq_send( TB_MESSAGE* msg );
extern int 	TB_msgq_recv( TB_MESSAGE* msg, TB_MSG_TYPE type, int flag );
extern int 	TB_msgq_reset( TB_MSG_TYPE type );
extern void TB_msgq_info( struct msqid_ds *m_stat, int flag );

#endif //__TB_MSG_QUEUE_H__

