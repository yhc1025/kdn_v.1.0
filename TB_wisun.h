#ifndef	__TB_WISUN_H__
#define __TB_WISUN_H__

#include <stdio.h>
#include <string.h>
#include "TB_j11.h"

////////////////////////////////////////////////////////////////////////////////

#define SLAVEID_RELAY1			(0x64)
#define SLAVEID_RELAY2			(0x65)
#define SLAVEID_RELAY3			(0x66)
#define SLAVEID_RELAY_ALL		(0x00)

#define SLAVEID_FLAG_RELAY1		(0x01)
#define SLAVEID_FLAG_RELAY2		(0x02)
#define SLAVEID_FLAG_RELAY3		(0x04)
#define SLAVEID_FLAG_RELAY_ALL	(SLAVEID_FLAG_RELAY1 | SLAVEID_FLAG_RELAY2 | SLAVEID_FLAG_RELAY3 )

////////////////////////////////////////////////////////////////////////////////

extern int TB_wisun_uart_init( int idx );
extern void TB_wisun_set_mode( TBUC idx, TB_WISUN_MODE mode );
extern TB_WISUN_MODE TB_wisun_get_mode( TBUC idx );
extern void TB_set_role( TB_ROLE role );
extern TB_ROLE TB_get_role( void );
extern TBBL TB_get_role_is_grpgw( void );
extern TBBL TB_get_role_is_terminal( void );
extern TBBL TB_get_role_is_relay( void );

extern void TB_wisun_proc_mesgq_data( int idx );
extern void TB_wisun_proc_netq_data( int idx, TBUS flag );

extern void TB_wisun_proc_start( int idx );
extern void TB_wisun_proc_stop( void );
extern TBBL TB_wisun_init_complete( int idx );

#endif//__TB_WISUN_H__

