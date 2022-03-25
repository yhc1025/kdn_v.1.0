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

#include "TB_ui.h"
#include "TB_util.h"
#include "TB_login.h"
#include "TB_msgbox.h"

int TB_show_msgbox( char *p_msg, int timeout, TBBL use_keyinput )
{
	int ret = -1;	
	
	if( p_msg == NULL )
		return ret;
	
	size_t msg_len = wstrlen( p_msg );
	if( msg_len > 0 )
	{
		char msg[MAX_MSG_LEN+1] = {0, };
		TB_UI_RECT win_rect = { 0, 10, 60, 3 };

		TB_ui_init();
		
		bzero( msg, sizeof(msg) );		
		if( msg_len > MAX_MSG_LEN )
		{
			msg_len = MAX_MSG_LEN-1;
			wmemcpy( msg, sizeof(msg), p_msg, msg_len );
			if( msg[msg_len] != '\n' )
				msg[msg_len+1] = '\n';
		}
		else
		{
			wmemcpy( msg, sizeof(msg), p_msg, msg_len );
		}

		unsigned int i;
		unsigned int line_cnt = 0;
		for( i=0; i<wstrlen(msg); i++ )
		{
			if( msg[i] == '\n' )
				line_cnt ++;
		}

		if( line_cnt == 0 )	//	'\n'가 없는 경우
		{
			wstrcat( msg, sizeof(msg), "\n" );
			line_cnt = 1;
		}

		if( line_cnt > 1 )
			win_rect.h += (line_cnt-1);

		////////////////////////////////////////////////////////////////////////
		
		WINDOW *win = newwin( win_rect.h, win_rect.w, win_rect.y, win_rect.x );
	    wbkgd( win, COLOR_PAIR(1) );
	    wattron( win, COLOR_PAIR(1));
		wborder( win, '|','|','-','-','+','+','+','+' );		

		wattrset( win, FOCUS_ON );
		wtimeout( win, 100 );

		char *p_start = msg;
		char *p_enter = NULL;
		char buf[128] = {0, };
		for( i=0; i<line_cnt; i++ )
		{
			p_enter = strchr( p_start, '\n' ); 
			if( p_enter )
			{
				bzero( buf, sizeof(buf) );
				wmemcpy( buf, sizeof(buf), p_start, (p_enter-p_start) );
				p_start = p_enter + 1;

				int draw_x = ( win_rect.w - wstrlen(buf) ) / 2;
	    		mvwprintw( win, (i+1), draw_x, "%s", buf );
			}
		}

		wattrset( win, FOCUS_OFF );
	    wrefresh( win );

		////////////////////////////////////////////////////////////////////////

		time_t t_old = time( NULL );
		time_t t_new = t_old;

		int input = -1;

		TB_ui_key_init( TRUE );
		while( 1 )
		{
			if( use_keyinput == TRUE )
			{
				input = TB_ui_getch( TRUE );
			}
			
			if( input != -1 )
			{
				if( input != KEY_MOUSE )		//	Any Key input
				{
					ret = 0;
					break;
				}

				if( input == KEY_AUTO_QUIT )
				{
					break;
				}
			}
			else if( timeout > 0 )
			{
				t_new = time( NULL );
				if( t_old + timeout < t_new )
				{
					ret = -1;
					break;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////
		
	    delwin( win );

		TB_ui_move( win_rect.y, win_rect.x );
		TB_ui_clear( UI_CLEAR_LINE_UNDER );

		TB_ui_deinit();
	}	

	return ret;
}

int TB_show_dialog( char *p_msg, int timeout )
{
	int ret = -1;
	
	if( p_msg == NULL )
		return ret;
	
	size_t msg_len = wstrlen( p_msg );
	if( msg_len > 0 )
	{
		char msg[MAX_MSG_LEN+1] = {0, };
		TB_UI_RECT win_rect = { 0, 10, 60, 3 };

		TB_ui_init();
		
		bzero( msg, sizeof(msg) );		
		if( msg_len > MAX_MSG_LEN )
		{
			msg_len = MAX_MSG_LEN-1;
			wmemcpy( msg, sizeof(msg), p_msg, msg_len );
			if( msg[msg_len] != '\n' )
				msg[msg_len+1] = '\n';
		}
		else
		{
			wmemcpy( msg, sizeof(msg), p_msg, msg_len );
		}

		unsigned int i;
		unsigned int line_cnt = 0;
		for( i=0; i<wstrlen(msg); i++ )
		{
			if( msg[i] == '\n' )
				line_cnt ++;
		}

		if( line_cnt == 0 )	//	'\n'가 없는 경우
		{
			wstrcat( msg, sizeof(msg), "\n" );
			line_cnt = 1;
		}

		if( line_cnt > 1 )
			win_rect.h += (line_cnt-1);

		////////////////////////////////////////////////////////////////////////
		
		WINDOW *win = newwin( win_rect.h, win_rect.w, win_rect.y, win_rect.x );
	    wbkgd( win, COLOR_PAIR(1) );
	    wattron( win, COLOR_PAIR(1));
		wborder( win, '|','|','-','-','+','+','+','+' );		

		wattrset( win, FOCUS_ON );
		wtimeout( win, 100 );

		char *p_start = msg;
		char *p_enter = NULL;
		char buf[128] = {0, };
		for( i=0; i<line_cnt; i++ )
		{
			p_enter = strchr( p_start, '\n' ); 
			if( p_enter )
			{
				bzero( buf, sizeof(buf) );
				wmemcpy( buf, sizeof(buf), p_start, (p_enter-p_start) );
				p_start = p_enter + 1;

				int draw_x = ( win_rect.w - wstrlen(buf) ) / 2;
	    		mvwprintw( win, (i+1), draw_x, "%s", buf );
			}
		}

		wattrset( win, FOCUS_OFF );
	    wrefresh( win );

		////////////////////////////////////////////////////////////////////////

		time_t t_old = time( NULL );
		time_t t_new = t_old;

		int input = -1;

		TB_ui_key_init( TRUE );
		while( 1 )
		{
			input = TB_ui_getch( TRUE );				
			if( input != -1 )
			{
				if( isalpha(input) )
				{
					input = toupper( input );
					if( input == 'N' || input == 'Y' )
					{
						ret = (input == 'N') ? 0 : 1;
						break;
					}
				}

				if( input == KEY_AUTO_QUIT )
				{
					ret = 0;
					break;
				}
			}
			else if( timeout > 0 )
			{
				t_new = time( NULL );
				if( t_old + timeout < t_new )
				{
					ret = -1;
					break;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////
		
	    delwin( win );

		TB_ui_move( win_rect.y, win_rect.x );
		TB_ui_clear( UI_CLEAR_LINE_UNDER );

		TB_ui_deinit();
	}	

	return ret;
}

