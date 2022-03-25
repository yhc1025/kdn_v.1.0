#ifndef __TB_UI_H__
#define __TB_UI_H__

#include <ncurses.h>
#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

#define KEY_ESC				0x1B
#define KEY_CHK_HOME   		0x31
#define KEY_CHK_END    		0x34
#define KEY_CHK_PGUP   		0x35
#define KEY_CHK_PGDN   		0x36
#define KEY_CHK_INSERT 		0x32

#define KEY_AUTO_QUIT		(0xFFFE)
#define KEY_USER_HOME		(0xFFFD)
#define KEY_USER_END		(0xFFFC)
#define KEY_USER_PGUP		(0xFFFB)
#define KEY_USER_PGDN		(0xFFFA)
#define KEY_USER_INSERT		(0xFFF9)

#define AUTO_QUIT_TIME		(1*60)	//5min

#define FOCUS_ON			(A_BOLD | A_BLINK)
#define	FOCUS_OFF			(A_NORMAL)

////////////////////////////////////////////////////////////////////////////////

typedef enum tagUI_CLEAR
{
	UI_CLEAR_ALL,			//	현재의 원도우 영역 화면 전체 지우기
	UI_CLEAR_LINE_END,		//	문자열 출력 후 현재 라인 끝까지 지우기
	UI_CLEAR_LINE_UNDER,	//	현재 커서 이후 마지막 라인까지 지우기
} TB_UI_CLEAR;

////////////////////////////////////////////////////////////////////////////////

extern void TB_ui_init( void );
extern void TB_ui_deinit( void );
extern int  TB_ui_getch( TBBL check_auto_quit );
extern void TB_ui_nonblocking( TBBL nonblock );
extern int 	TB_ui_key_init( TBBL nonblocking );
extern void TB_ui_key_set_force_block( TBBL block );
extern void TB_ui_clear( TB_UI_CLEAR flag );
extern void TB_ui_attrset( int attrs );
extern void TB_ui_move( int y, int x );
extern void TB_ui_refresh( void );

#endif//__TB_UI_H__

