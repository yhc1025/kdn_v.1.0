#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "TB_modbus.h"
#include "TB_util.h"
#include "TB_debug.h"

////////////////////////////////////////////////////////////////////////////////
//	SF : Scale Factor
#define SF_0	(1.0)
#define SF_1	(0.1)
#define SF_2	(0.01)
#define SF_3	(0.001)
#define SF_4	(0.0001)
#define SF_5	(0.00001)

////////////////////////////////////////////////////////////////////////////////

#define SET_4BYTE(tbul, src, s)	(tbul =	(src[s+0] << 24) +	\
									    (src[s+1] << 16) + 	\
									    (src[s+2] <<  8) + 	\
									    (src[s+3] <<  0))

#define SET_2BYTE(tbul, src, s)	(tbul = (src[s+0] <<  8) + 	\
										(src[s+1] <<  0))

////////////////////////////////////////////////////////////////////////////////

static int print_value( char *p_name, TBUL value_org, TBFLT sf )
{
	TBUL value = (TBUL)( (TBFLT)value_org * sf );
	TB_dbg_inverter( "%s : %8d %d\r\n", p_name, value_org, value );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void TB_modbus_print_FUNC_READ_HOLDING_REG( TBUC *p_data )
{
	TBUL temp;
	TBUL idx = 0;

	TB_dbg_inverter("\n********************************************************\r\n" );
	if( p_data )
	{
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Currrent R-phase    ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Currrent S-phase    ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Currrent T-phase    ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Voltage  R-phase    ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Voltage  S-phase    ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Voltage  T-phase    ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "3phase valid watt   ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "3phase invalid watt ", temp, SF_0 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "3phase power rate   ", temp, SF_3 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Frequency           ", temp, SF_1 );	idx += 4;
		SET_4BYTE(temp, p_data, 3+idx);	print_value( "Status Flag         ", temp, SF_0 );
		TB_dbg_inverter("        Operation status             = %s\r\n", temp & 0x01 ? "STOP" : "RUN" );
		TB_dbg_inverter("        Inverter CB Operation status = %s\r\n", temp & 0x02 ? "FAIL" : "NORMAL" );
		TB_dbg_inverter("        Power rate Status            = %s\r\n", temp & 0x03 ? "SELF RUN" : "POWER-RATE RUN" );
		TB_dbg_inverter("        Valid Watt Control           = %s\r\n", temp & 0x04 ? "SUCCESS" : "FAIL" );
		TB_dbg_inverter("        Inverter RUN:STOP Control    = %s\r\n", temp & 0x05 ? "SUCCESS" : "FAIL" );
	}
	TB_dbg_inverter("********************************************************\n\r\n" );
}

TBSI TB_modbus_make_command_read_holding_reg( TBUC *buf, TBUC slave_id, TBUS addr, TBUS num )
{
	TBSI idx = 0;
	if( buf )
	{
		TB_dbg_inverter("SLAVE_ID:%d REGISTER:0x%04X(%d) NUM:%d\r\n", slave_id, addr, addr, num );

		buf[idx++] = slave_id;
		buf[idx++] = FUNC_READ_HOLDING_REG;
		buf[idx++] = (addr >> 8) & 0xFF;
		buf[idx++] = (addr >> 0) & 0xFF;
		buf[idx++] = (num  >> 8) & 0xFF;
		buf[idx++] = (num  >> 0) & 0xFF;
		
		TB_crc16_modbus_fill( buf, idx );
		idx += CRC_SIZE;

		if( TB_dm_is_on(DMID_INVERTER) )
			TB_util_data_dump( "COMMAND : READ HOLDING", buf, idx );
	}

	return idx;
}

TBSI TB_modbus_make_command_write_single_reg( TBUC *buf, TBUC slave_id, TBUS addr, TBUS num )
{
	TBSI idx = 0;
	if( buf )
	{
		TB_dbg_inverter("SLAVE_ID:%02x REGISTER:%04x(%d) NUM:%d\r\n", slave_id, addr, addr, num );

		buf[idx++] = slave_id;
		buf[idx++] = FUNC_WRITE_SINGLE_REG;
		buf[idx++] = (addr >> 8) & 0xFF;
		buf[idx++] = (addr >> 0) & 0xFF;
		buf[idx++] = (num  >> 8) & 0xFF;
		buf[idx++] = (num  >> 0) & 0xFF;
		
		TB_crc16_modbus_fill( buf, idx );
		idx += CRC_SIZE;

		if( TB_dm_is_on(DMID_INVERTER) )
			TB_util_data_dump( "COMMAND : WRITE SINGLE", buf, idx );
	}

	return idx;
}

TBSI TB_modbus_make_command_write_multiple_reg( TBUC *buf, TBUC slave_id, TBUS addr, TBUS num )
{
	TBSI idx = 0;
	if( buf )
	{
		TB_dbg_inverter("SLAVE_ID:%02x REGISTER:%04x(%d) NUM:%d\r\n", slave_id, addr, addr, num );

		buf[idx++] = slave_id;
		buf[idx++] = FUNC_WRITE_MULTIPLE_REG;
		buf[idx++] = (addr >> 8) & 0xFF;
		buf[idx++] = (addr >> 0) & 0xFF;
		buf[idx++] = (num  >> 8) & 0xFF;
		buf[idx++] = (num  >> 0) & 0xFF;
		
		TB_crc16_modbus_fill( buf, idx );
		idx += CRC_SIZE;

		if( TB_dm_is_on(DMID_INVERTER) )
			TB_util_data_dump( "COMMAND : WRITE MULTIPLE", buf, idx );
	}

	return idx;
}

////////////////////////////////////////////////////////////////////////////////

TB_MB_RESPONSE g_mb_handle_t;

TB_MB_RESPONSE *TB_modbus_init( void )
{
	TB_modbus_reset();

	g_mb_handle_t.mFilter.mSlaveId = -1;
	g_mb_handle_t.mFilter.mFunctionCode = -1;

	g_mb_handle_t.reset		= TB_modbus_reset;
	g_mb_handle_t.buffers	= TB_modbus_get_buffer;
	g_mb_handle_t.length 	= TB_modbus_get_length;
	g_mb_handle_t.filter 	= TB_modbus_set_filter;
	g_mb_handle_t.push 		= TB_modbus_push;

	return &g_mb_handle_t;	
}

void TB_modbus_reset( void )
{
	g_mb_handle_t.mWrite 		= 0;
	g_mb_handle_t.mValidLength 	= FALSE;
	g_mb_handle_t.mValidCrc 	= FALSE;
	g_mb_handle_t.mLength 		= 0;
	g_mb_handle_t.mSync 		= FALSE;
}

TBUC *TB_modbus_get_buffer( void )
{
	return g_mb_handle_t.mBuf;
};

TBSI TB_modbus_get_length( void )
{
	return g_mb_handle_t.mWrite;
}

void TB_modbus_set_filter( TBUC slave_id, TBUC function_code )
{
	g_mb_handle_t.mFilter.mSlaveId 		= slave_id;
	g_mb_handle_t.mFilter.mFunctionCode = function_code;
}

TBBL TB_modbus_push( TBUC *buf, int len )
{
	if( buf == NULL || len <= 0 )
	{
		TB_dbg_inverter( "buf is NULL : len=%d\r\n", len );
		return FALSE;
	}
	
	if( g_mb_handle_t.mWrite < 0 || (g_mb_handle_t.mWrite + len) > MAX_MODBUS_BUF )
	{
		TB_dbg_inverter("ERROR. mWrite:%d + len:%d > MAX_MODBUS_BUF(%d)\r\n", g_mb_handle_t.mWrite, len, MAX_MODBUS_BUF);
		TB_modbus_reset();		
		return FALSE;
	}
	if( g_mb_handle_t.mFilter.mSlaveId < 0 || g_mb_handle_t.mFilter.mFunctionCode < 0 )
	{
		TB_dbg_inverter("ERROR. mFilter(%d,%d)\r\n",	g_mb_handle_t.mFilter.mSlaveId,	\
														g_mb_handle_t.mFilter.mFunctionCode);
		TB_modbus_reset();
		return FALSE;
	}

	wmemcpy( g_mb_handle_t.mBuf + g_mb_handle_t.mWrite, sizeof(g_mb_handle_t.mBuf)-g_mb_handle_t.mWrite, buf, len );
	g_mb_handle_t.mWrite += len;

	if( g_mb_handle_t.mWrite < 4 )
		return FALSE;

	if( !g_mb_handle_t.mSync )
	{
		int i;
		for( i=0; i<(g_mb_handle_t.mWrite-1); i++ )
		{
			if( g_mb_handle_t.mBuf[i]   == g_mb_handle_t.mFilter.mSlaveId &&	\
				g_mb_handle_t.mBuf[i+1] == g_mb_handle_t.mFilter.mFunctionCode )
			{
//				TB_dbg_inverter("found sync i:%d mBuf:%02x,%02x \r\n", i, g_mb_handle_t.mBuf[i], g_mb_handle_t.mBuf[i+1]);
				wmemcpy( g_mb_handle_t.mTmpBuf, sizeof(g_mb_handle_t.mTmpBuf), g_mb_handle_t.mBuf+i, g_mb_handle_t.mWrite-i );
				g_mb_handle_t.mWrite -= i;
				wmemcpy( g_mb_handle_t.mBuf, sizeof(g_mb_handle_t.mBuf), g_mb_handle_t.mTmpBuf, g_mb_handle_t.mWrite );
				g_mb_handle_t.mSync = TRUE;
				break;
			}
		}
	}
//	TB_dbg_inverter("mSync:%d mWrite:%d mBuf[2]:%d\r\n", g_mb_handle_t.mSync,	\
														g_mb_handle_t.mWrite,	\
														g_mb_handle_t.mBuf[2]);

	if( !g_mb_handle_t.mSync )
	{
		return FALSE;
	}
	
	switch( g_mb_handle_t.mBuf[1] )
	{
		case FUNC_READ_HOLDING_REG:
			if( (g_mb_handle_t.mBuf[2] + 5) <= g_mb_handle_t.mWrite )
			{
				g_mb_handle_t.mValidLength 	= TRUE;
				g_mb_handle_t.mLength 		= g_mb_handle_t.mBuf[2] + 5;
				
				TBUS crc_calc = TB_crc16_modbus_get( g_mb_handle_t.mBuf, g_mb_handle_t.mLength - 2 );
				TBUS crc_ret  = ( (g_mb_handle_t.mBuf[g_mb_handle_t.mLength-1] << 8) +	\
					               g_mb_handle_t.mBuf[g_mb_handle_t.mLength-2] );
				g_mb_handle_t.mValidCrc = (crc_calc == crc_ret) ? TRUE : FALSE;
				
//				TB_dbg_inverter("mLength:%d mWrite:%d mValidCrc:%d(%04x)\r\n", g_mb_handle_t.mLength,	\
																				g_mb_handle_t.mWrite,	\
																				g_mb_handle_t.mValidCrc,\
																				crc_calc);
				if( g_mb_handle_t.mValidCrc )
				{
//					TB_modbus_print_FUNC_READ_HOLDING_REG( g_mb_handle_t.mBuf );
				}
				return TRUE;
			}
			break;
			
		default:
			TB_dbg_inverter("Not Support mWrite:%d mBuf[1]:%d\r\n",	g_mb_handle_t.mWrite,	\
																	g_mb_handle_t.mBuf[1]);
			TB_modbus_reset();
			break;
	}
	
	return FALSE;
}

