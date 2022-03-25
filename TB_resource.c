#include "TB_res_kor.h"
#include "TB_res_eng.h"
#include "TB_resource.h"

TB_LANG g_lang = LANG_KOR;

////////////////////////////////////////////////////////////////////////////////

char *TB_msgid_get_string( TB_MSGID mid )
{
	char *p_string = NULL;
	if( mid >= MSGID_MAX )
		mid = MSGID_UNKNOWN;

	if( g_lang == LANG_KOR )
		p_string = sp_message_kor[mid];
	else
		p_string = sp_message_eng[mid];

	return p_string;
}

char *TB_resid_get_string( TB_RESID rid )
{
	char *p_string = NULL;
	if( rid >= RESID_MAX )
		rid = RESID_UNKNOWN;

	if( g_lang == LANG_KOR )
		p_string = sp_res_kor[rid];
	else
		p_string = sp_res_eng[rid];

	return p_string;
}

TB_LANG TB_resid_get_lang( void )
{
	return g_lang;
}

void TB_resid_set_lang( TB_LANG lang )
{
	if( lang >= LANG_MIN && lang <= LANG_MAX )
		g_lang = lang;
}

