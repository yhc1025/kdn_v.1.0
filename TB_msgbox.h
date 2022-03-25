#ifndef __TB_MSGBOX_H__
#define __TB_MSGBOX_H__

#define MAX_MSG_LEN		256

extern int TB_show_msgbox( char *p_msg, int timeout, TBBL use_keyinput );
extern int TB_show_dialog( char *p_msg, int timeout );

#endif//__TB_MSGBOX_H__
