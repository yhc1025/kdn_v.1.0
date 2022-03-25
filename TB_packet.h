#ifndef	__TB_PACKET_H__
#define __TB_PACKET_H__

#include "TB_type.h"

#define MAX_PACKET_SIZE		1232

#define PACKET_SOP			0xFECA	//	CAFE
#define PACKET_EOP			0xBEBE	//	BEBE
#define PACKET_VER			0x0001

#define TB_CMD_REQ_INVERTER		0x01
#define TB_CMD_RES_INVERTER		0x11
#define TB_CMD_REQ_STAT			0x02
#define TB_CMD_RES_STAT			0x12
#define TB_CMD_REQ_ALIVE		0x04
#define TB_CMD_RES_ALIVE		0x14

#define TB_CMD_SEND_INFO_COOR	0x21
#define TB_CMD_SEND_INFO_ENDD	0x22

////////////////////////////////////////////////////////////////////////////////

typedef struct tagCTL
{
	TBUC	PTYPE;
	TBUC	FSN;
}  TB_CTL;

typedef struct tagSERIAL
{
	TBUS	MSB;
	TBUS	NUM;
}  TB_SERIAL;

typedef struct tagPACKET_HEAD
{
	TBUS		SOP;
	TBUS		VER;
	TB_CTL		CTL;
	TBUS 		LENGTH;
	TBUS 		TLENGTH;
	TB_SERIAL	SN;
	TBUS		CMD;
} TB_PACKET_HEAD;

typedef struct tagPACKET_TAIL
{
	TBUS		CRC16;
	TBUS		EOP;
}  TB_PACKET_TAIL;

typedef struct tagPACKET
{
	TBUC	data[MAX_PACKET_SIZE];
	TBUI	size;
}  TB_PACKET;

////////////////////////////////////////////////////////////////////////////////

extern int 	TB_packet_make( TB_PACKET *p_packet, TBUS cmd, void *p_pl, TBUS pl_size, TBUS pl_tsize, int pk_cnt, int pk_idx );
extern int 	TB_packet_make_kcmvp( TB_PACKET *p_packet );
extern int 	TB_packet_send( int idx, TBUS cmd, void *p_pl, TBUS pl_size );
extern int 	TB_packet_dump( TB_PACKET *p_packet );
extern TBBL TB_packet_check( TB_PACKET *p_packet );
extern TBUC TB_packet_get_send_direction( int idx );
extern void TB_packet_set_send_direction( int idx, TBUC dir );
extern void TB_packet_set_dest_ip( int idx, TBUC dir );
extern TBUC *TB_packet_get_dest_ip( int idx );

#endif//__TB_PACKET_H__

