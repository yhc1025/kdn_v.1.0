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

#include "TB_setup.h"
#include "TB_util.h"
#include "TB_ui.h"

////////////////////////////////////////////////////////////////////////////////

static time_t s_key_last_time = 0;
void TB_ui_init( void )
{
	initscr();

	keypad( stdscr, TRUE );
	noecho();
	start_color();
	init_pair( 1, COLOR_RED, COLOR_WHITE );
	attron( COLOR_PAIR(1) );
	curs_set( 0 );
	clear();
	refresh();	

	TB_ui_key_init( TRUE );
}

void TB_ui_deinit( void )
{
	TB_ui_nonblocking( TRUE );
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_refresh();
	endwin();
}

int TB_ui_key_init( TBBL nonblocking )
{
	s_key_last_time = 0;
	timeout( 100 );
	TB_ui_nonblocking( nonblocking );
}

int TB_ui_check_auto_quit( void )
{
	int key = -1;
	time_t lock_sec = TB_setup_get_auto_lock() * 60;
	if( lock_sec != 0 )
	{
		time_t curr_time = time( NULL );
		if( s_key_last_time == 0 )
			s_key_last_time = curr_time;
		
		if( s_key_last_time + lock_sec < curr_time )
		{
			key = KEY_AUTO_QUIT;
		}
	}

	return key;
}

static volatile TBBL s_key_force_block = FALSE;
void TB_ui_key_set_force_block( TBBL block )
{
	s_key_force_block = block;
}

int TB_ui_getch( TBBL check_auto_quit )
{
	int key = -1;
	if( s_key_force_block == FALSE )
	{
		key = getch();

		////////////////////////////////////////////////////////////////////////
		
		static TBUC s_ext_key[4];	
		if( key == KEY_ESC )
		{
			s_ext_key[0] = (TBUC)key;

			for( int i=1; i<4; i++ )
			{
				s_ext_key[i] = getch();
			}

			if( s_ext_key[0] == 0x1B && s_ext_key[1] == 0x5B && s_ext_key[3] == 0x7E )
			{
				switch( s_ext_key[2] )
				{
					case KEY_CHK_HOME   : key = KEY_USER_HOME;		break;
					case KEY_CHK_END    : key = KEY_USER_END;		break;
					case KEY_CHK_PGUP   : key = KEY_USER_PGUP;		break;
					case KEY_CHK_PGDN   : key = KEY_USER_PGDN;		break;
					case KEY_CHK_INSERT : key = KEY_USER_INSERT;	break;
					default				: key = -1;					break;
				}
			}
			else
			{
				key = KEY_ESC;
			}

			bzero( s_ext_key, sizeof(s_ext_key) );
		}

		////////////////////////////////////////////////////////////////////////////

		if( check_auto_quit == TRUE )
		{
			if( key != -1 )
			{
				s_key_last_time = time( NULL );
			}
			else	//	when key mode is non-block, return value is -1
			{
				key = TB_ui_check_auto_quit();
			}
		}
	}

	return key;
}

static int 	s_key_nonblock = -1;
void TB_ui_nonblocking( TBBL nonblock )
{
#if 1
	nodelay( stdscr, TRUE );
#else	
	if( s_key_nonblock != nonblock )
	{
		nodelay( stdscr, nonblock );
		s_key_nonblock = nonblock;
	}
#endif
}

void TB_ui_clear( TB_UI_CLEAR flag )
{
	switch( flag )
	{
		case UI_CLEAR_ALL 			: clear();		break;
		case UI_CLEAR_LINE_END 		: clrtoeol();	break;
		case UI_CLEAR_LINE_UNDER 	: clrtobot();	break;
	}
}

void TB_ui_attrset( int attrs )
{
	attrset( attrs );
}

void TB_ui_move( int y, int x )
{
	move( y, x );
}

void TB_ui_refresh( void )
{
	refresh();
}

