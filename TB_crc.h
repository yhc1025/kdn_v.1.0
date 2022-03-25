#ifndef	__TB_CRC_H__
#define __TB_CRC_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>

#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_SEED_TABLE_CNT	(256)

extern TBUC g_seed_table_high[MAX_SEED_TABLE_CNT];
extern TBUC g_seed_table_low[MAX_SEED_TABLE_CNT];

////////////////////////////////////////////////////////////////////////////////

extern TBUS TB_crc16_modbus_get( TBUC *p_data, TBUS length );
extern void TB_crc16_modbus_fill( TBUC *p_buf, TBUS buf_len );
extern TBBL TB_crc16_modbus_check( TBUC *p_buf, TBUS buf_len );

extern TBUS TB_crc16_dnp_get( TBUC *p_data, TBUS length );
extern void TB_crc16_dnp_fill( TBUC *p_buf, TBUS buf_len );
extern TBBL TB_crc16_dnp_check( TBUC *p_buf, TBUS buf_len );

#endif//__TB_CRC_H__

