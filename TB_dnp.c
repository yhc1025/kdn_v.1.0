#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "TB_dnp.h"
#include "TB_util.h"
#include "TB_debug.h"

////////////////////////////////////////////////////////////////////////////////
/*
*
*						Dead code. Not currently in use.
*
*/
////////////////////////////////////////////////////////////////////////////////

TB_DNPP_RESPONSE g_dnpp_handle_t;

TB_DNPP_RESPONSE *TB_dnpp_init( void )
{
	TB_dnpp_reset();

	g_dnpp_handle_t.mFilter.mSlaveId = -1;
	g_dnpp_handle_t.mFilter.mFunctionCode = -1;

	g_dnpp_handle_t.reset	= TB_dnpp_reset;
	g_dnpp_handle_t.buffers	= TB_dnpp_get_buffer;
	g_dnpp_handle_t.length 	= TB_dnpp_get_length;
	g_dnpp_handle_t.filter 	= TB_dnpp_set_filter;
	g_dnpp_handle_t.push 	= TB_dnpp_push;

	return &g_dnpp_handle_t;	
}

void TB_dnpp_reset( void )
{
	g_dnpp_handle_t.mWrite 			= 0;
	g_dnpp_handle_t.mValidLength 	= FALSE;
	g_dnpp_handle_t.mValidCrc 		= FALSE;
	g_dnpp_handle_t.mLength 		= 0;
	g_dnpp_handle_t.mSync 			= FALSE;
}

TBUC *TB_dnpp_get_buffer( void )
{
	return g_dnpp_handle_t.mBuf;
};

TBSI TB_dnpp_get_length( void )
{
	return g_dnpp_handle_t.mWrite;
}

void TB_dnpp_set_filter( TBUS filter )
{
	g_dnpp_handle_t.mFilter = filter;
}

TBBL TB_dnpp_push( TBUC *buf, int len )
{
	if( buf == NULL || len <= 0 )
	{
		TB_dbg_inverter( "buf is NULL : len=%d\r\n", len );
		return FALSE;
	}
	
	if( g_dnpp_handle_t.mWrite < 0 || (g_dnpp_handle_t.mWrite + len) > MAX_DNPP_BUF )
	{
		TB_dbg_inverter("ERROR. mWrite:%d + len:%d > MAX_DNPP_BUF(%d)\r\n", g_dnpp_handle_t.mWrite, len, MAX_DNPP_BUF);
		TB_dnpp_reset();		
		return FALSE;
	}

	wmemcpy( g_dnpp_handle_t.mBuf + g_dnpp_handle_t.mWrite, sizeof(g_dnpp_handle_t.mBuf) - g_dnpp_handle_t.mWrite, buf, len );
	g_dnpp_handle_t.mWrite += len;

	if( g_dnpp_handle_t.mWrite < 4 )
		return FALSE;

	if( !g_dnpp_handle_t.mSync )
	{
		int i;
		for( i=0; i<(g_dnpp_handle_t.mWrite-1); i++ )
		{
			if( g_dnpp_handle_t.mBuf[i]   == (g_dnpp_handle_t.mFilter >> 0) & 0xFF
				g_dnpp_handle_t.mBuf[i+1] == (g_dnpp_handle_t.mFilter >> 8) & 0xFF )
			{
				wmemcpy( g_dnpp_handle_t.mTmpBuf, sizeof(g_dnpp_handle_t.mTmpBuf)-g_dnpp_handle_t.mBuf+i, g_dnpp_handle_t.mBuf+i, g_dnpp_handle_t.mWrite-i );
				g_dnpp_handle_t.mWrite -= i;
				wmemcpy( g_dnpp_handle_t.mBuf, sizeof(g_dnpp_handle_t.mBuf), g_dnpp_handle_t.mTmpBuf, g_dnpp_handle_t.mWrite );
				g_dnpp_handle_t.mSync = TRUE;
				break;
			}
		}
	}

	if( !g_dnpp_handle_t.mSync )
	{
		return FALSE;
	}

	if( g_dnpp_handle_t.mBuf[0] == (g_dnpp_handle_t.mFilter >> 0) & 0xFF
		g_dnpp_handle_t.mBuf[1] == (g_dnpp_handle_t.mFilter >> 8) & 0xFF )
	{
		g_dnpp_handle_t.mValidLength = TRUE;
		g_dnpp_handle_t.mLength 	 = g_dnpp_handle_t.mBuf[2] + 2;
				
		TBUS crc_calc = TB_crc16_modbus_get( g_dnpp_handle_t.mBuf, g_dnpp_handle_t.mLength - 2 );
		TBUS crc_ret  = ( (g_dnpp_handle_t.mBuf[g_dnpp_handle_t.mLength-1] << 8) +	\
			               g_dnpp_handle_t.mBuf[g_dnpp_handle_t.mLength-2] );
		g_dnpp_handle_t.mValidCrc = (crc_calc == crc_ret) ? TRUE : FALSE;
				
//				TB_dbg_inverter("mLength:%d mWrite:%d mValidCrc:%d(%04x)\r\n", g_dnpp_handle_t.mLength,	\
																				g_dnpp_handle_t.mWrite,	\
																				g_dnpp_handle_t.mValidCrc,\
																				crc_calc);
				if( g_dnpp_handle_t.mValidCrc )
				{
//					TB_dnpp_print_FUNC_READ_HOLDING_REG( g_dnpp_handle_t.mBuf );
				}
				return TRUE;
			}
			break;
			
		default:
			TB_dbg_inverter("Not Support mWrite:%d mBuf[1]:%d\r\n",	g_dnpp_handle_t.mWrite,	\
																	g_dnpp_handle_t.mBuf[1]);
			TB_dnpp_reset();
			break;
	}
	
	return FALSE;
}

