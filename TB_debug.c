#include <stdio.h>
#include <stdarg.h>
#include "TB_util.h"
#include "TB_debug.h"

static TBUC s_debug_mode[DMID_COUNT] = {0, };
static TBUC s_debug_mode_backup[DMID_COUNT] = {0, };

int TB_dm_print( TB_DMID mode, const char *fmt, ... )
{
	int	ret = 0;

	if( mode > 0 && mode < DMID_COUNT )
	{
		if( s_debug_mode[mode] == DM_ON )
		{
			va_list	ap;

			va_start( ap, fmt );
			ret = vfprintf( stdout, fmt, ap );
			va_end( ap );
		}
	}

	return ret;
}

int TB_dm_debug( const char *file, int line, TB_DMID mode, const char *fmt, ... )
{
	int	ret = 0;

	if( mode > 0 && mode < DMID_COUNT )
	{
		if( s_debug_mode[mode] == DM_ON )
		{
			va_list	ap;

			if( file )
				fprintf( stdout, "[%s:%d] ", file, line );

			va_start( ap, fmt );
			ret = vfprintf( stdout, fmt, ap );
			va_end( ap );
		}
	}

	return ret;
}

int TB_dm_set_mode( TB_DMID mode, char onoff )
{
	if( mode == DMID_ALL )
	{
		int i;
		for( i=0; i<DMID_COUNT; i++ )
			s_debug_mode[i] = onoff;
	}	
	else if( mode > 0 && mode < DMID_COUNT )
	{
		s_debug_mode[mode] = onoff;
	}

	return 0;
}

void TB_dm_dbg_mode_all_off( void )
{
	wmemcpy( &s_debug_mode_backup[0], sizeof(s_debug_mode_backup),	\
					  &s_debug_mode[0],		   sizeof(s_debug_mode) );

	for( int i=0; i<DMID_COUNT; i++ )
		s_debug_mode[i] = DM_OFF;
}

void TB_dm_dbg_mode_restore( void )
{
	wmemcpy( &s_debug_mode[0], 	   sizeof(s_debug_mode),	\
					  &s_debug_mode_backup[0], sizeof(s_debug_mode_backup) );

	for( int i=0; i<DMID_COUNT; i++ )
		s_debug_mode[i] = s_debug_mode_backup[i];
}

TBUC *TB_dm_dbg_get_mode( void )
{
	return &s_debug_mode[0];
}

TBUC *TB_dm_dbg_get_mode_backup( void )
{
	return &s_debug_mode_backup[0];
}

int TB_dm_is_on( TB_DMID mode )
{
	int onoff = DM_OFF;
	if( mode > 0 && mode < DMID_COUNT )
	{
		onoff = s_debug_mode[mode];
	}

	return onoff;
}

int TB_dm_init( void )
{
	TB_dm_set_mode( DMID_ALL, DM_ON );	
	return 0;
}

