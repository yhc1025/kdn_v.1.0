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
#include "TB_aes_evp.h"
#include "TB_msgbox.h"
#include "TB_log.h"
#include "TB_login.h"

////////////////////////////////////////////////////////////////////////////////
//				Personal Information Without Encryption
////////////////////////////////////////////////////////////////////////////////
static TBUC s_file_path_accnt_aes[] 	= { 0x80, 0x35, 0xEA, 0xAD, 0x1D, 0x37, 0xC5, 0x9F, 0x2F, 0x8A, 0xA9, 0x70, 0x4E, 0xAB, 0x08, 0x41,
											0xF9, 0x30, 0x02, 0x90, 0x21, 0x49, 0x1B, 0x11, 0xBC, 0x49, 0x2F, 0x3E, 0x1C, 0xA6, 0xA1, 0x40 };

static TBUC s_file_path_lgn_fail_aes[]  = { 0xF0, 0x60, 0x0B, 0x55, 0x62, 0xC1, 0x79, 0xCA, 0xF1, 0x2D, 0x05, 0x0C, 0x92, 0xCB, 0x5B, 0xD9,
											0xB8, 0x09, 0x11, 0x2E, 0x0F, 0xF0, 0x68, 0x0D, 0x3A, 0x27, 0x4C, 0x5E, 0x32, 0x74, 0x06, 0x10 };

static TBUC s_file_path_accnt   [128];		//	/root/account.dat
static TBUC s_file_path_lgn_fail[128];		//	/root/login_fail.dat

////////////////////////////////////////////////////////////////////////////////
//				Personal Information Without Encryption
////////////////////////////////////////////////////////////////////////////////
static TBUC s_def_pw_admin_aes[] = { 0xAB, 0xB7, 0x4D, 0x5D, 0x9D, 0xDC, 0xD2, 0x9B, 0x37, 0x9B, 0x71, 0xDB, 0x9C, 0xFE, 0x1D, 0xB4 };
static TBUC s_def_pw_user_aes[]  = { 0x59, 0x7E, 0x0B, 0x89, 0x04, 0x88, 0xB7, 0x61, 0xAF, 0xDF, 0x1C, 0x72, 0x2F, 0xF3, 0x07, 0x58 };

static TBUC s_def_pw_admin[MAX_PW_LEN];
static TBUC s_def_pw_user [MAX_PW_LEN];

enum
{
	FOCUS_CHG_PW_CURRENT = 0,
	FOCUS_CHG_PW_INPUT,
	FOCUS_CHG_PW_CONFIRM,
	FOCUS_CHG_PW_MAX,
};

////////////////////////////////////////////////////////////////////////////////

static TB_LOGIN s_login;
static TB_LOGIN s_login_default;

////////////////////////////////////////////////////////////////////////////////

#define CHK_AVALABLE_PW_CHAR(c)		(c >= 0x21 && c <= 0x7E)

#define ATTR_NONE		0x00
#define ATTR_LOWER		0x01
#define ATTR_UPPER		0x02
#define ATTR_NUM		0x04
#define ATTR_SYMBOL		0x08
#define ATTR_ALL		(ATTR_LOWER |	\
						 ATTR_UPPER |	\
						 ATTR_NUM   |	\
						 ATTR_SYMBOL)
TB_LOGIN_ERROR TB_login_check_pw_attribute( TBUC *p_pw )
{
	TB_LOGIN_ERROR ret = LOGIN_ERROR_NONE;
	TBUC check = ATTR_NONE;

	if( p_pw )
	{
		int len = wstrlen( p_pw );
		if( len >= MIN_PW_LEN && len <= MAX_PW_LEN )
		{
			int i;
			for( i=0; i<len; i++ )
			{
				/***************************************************************
				*		     Character is Alpha, Numeric, Simbol
				***************************************************************/
				if( CHK_AVALABLE_PW_CHAR(p_pw[i]) )
				{
					if( isalnum(p_pw[i]) )
					{
						if( islower(p_pw[i]) )	check |= ATTR_LOWER;
						if( isupper(p_pw[i]) )	check |= ATTR_UPPER;
						if( isdigit(p_pw[i]) )	check |= ATTR_NUM;
					}
					else if( iscntrl(p_pw[i]) == 0 )
					{
						check |= ATTR_SYMBOL;
					}
				}
				else
				{
					ret = LOGIN_ERROR_ATTRIBUTE;
					break;
				}
			}			

			if( check != ATTR_ALL )
			{
				ret = LOGIN_ERROR_ATTRIBUTE;
			}
		}
		else if( len < MIN_PW_LEN )
		{
			ret = LOGIN_ERROR_PW_LENGTH_MIN;
		}
		else if( len > MAX_PW_LEN )
		{
			ret = LOGIN_ERROR_PW_LENGTH_MAX;
		}
	}
	else
	{
		ret = LOGIN_ERROR_UNKNOWN;
	}

	return ret;
}

TB_LOGIN_ERROR TB_login_check_pw_default( TBUC *p_pw, TB_ACCOUNT_TYPE type )
{
	TB_LOGIN_ERROR ret = LOGIN_ERROR_UNKNOWN;

	if( p_pw )
	{
		TBUC enc_buf[AES_BUF_SIZE+AES_BLOCK_SIZE] = {0, };
		TBUI enc_size = 0;
		size_t pw_size = wstrlen( p_pw );

		if( pw_size > 0 )
		{		
			if( type == ACCOUNT_TYPE_ADMIN )
			{
				bzero( enc_buf, sizeof(enc_buf) );
				TB_aes_evp_encrypt( p_pw, pw_size, enc_buf, sizeof(enc_buf) );
				enc_size = (pw_size + AES_BLOCK_SIZE) / AES_BLOCK_SIZE * AES_BLOCK_SIZE;

				if( enc_size == sizeof(s_def_pw_admin_aes) )
				{
					if( memcmp(enc_buf, s_def_pw_admin_aes, enc_size) == 0 )
					{
						ret = LOGIN_ERROR_PW_DEFAULT;
					}
					else
					{
						ret = LOGIN_ERROR_NONE;
					}
				}
			}
			else
			{
				bzero( enc_buf, sizeof(enc_buf) );
				TB_aes_evp_encrypt( p_pw, pw_size, enc_buf, sizeof(enc_buf) );
				enc_size = (pw_size + AES_BLOCK_SIZE) / AES_BLOCK_SIZE * AES_BLOCK_SIZE;
				
				if( enc_size == sizeof(s_def_pw_user_aes) )
				{
					if( memcmp(enc_buf, s_def_pw_user_aes, enc_size) == 0 )
					{
						ret = LOGIN_ERROR_PW_DEFAULT;
					}
					else
					{
						ret = LOGIN_ERROR_NONE;
					}
				}
			}
		}
	}

	return ret;
}

TB_LOGIN_ERROR TB_login_check_account( TB_ACCOUNT_INFO account, TB_ACCOUNT_TYPE type )
{
	TB_LOGIN_ERROR ret = LOGIN_ERROR_UNKNOWN;

	/***************************************************************************
	*	STEP.01 : PW attribute check.
	***************************************************************************/
	ret = TB_login_check_pw_attribute( account.pw );
	if( ret != LOGIN_ERROR_NONE )
	{
		TB_dbg_login("ERROR. password string : attribute\r\n");
	}

	/***************************************************************************
	*	STEP.02 : PW Default.
	***************************************************************************/
	if( ret == LOGIN_ERROR_NONE )
	{
		if( type == ACCOUNT_TYPE_ADMIN )
		{
			ret = TB_login_check_pw_default( account.pw, type );
			if( ret != LOGIN_ERROR_NONE )
			{
				TB_dbg_login("ERROR. The Admin password is the same as the default.\r\n" );
			}
		}
	}

	return ret;
}

int TB_login_create_default_account_file( void )
{
	int ret = -1;
	FILE *fp = fopen( s_file_path_accnt, "w" );
	if( fp )
	{
		TBUC   		enc_buf[AES_BUF_SIZE+AES_BLOCK_SIZE] = {0, };
		TBUC 		write_buf[4 + sizeof(TB_LOGIN)] = {0, };//	4 is 'CAFE'(4byte)
		size_t		write_size;
		int			idx = 0;
		TB_LOGIN 	login;
		size_t		size = 0;
		
		bzero( &login, sizeof(login) );
		
		login.admin.login_time = 0;
		size = wstrlen( s_def_pw_admin );
		if( size > 0 )
			wmemcpy( login.admin.pw, sizeof(login.admin.pw), s_def_pw_admin, size );
		
		login.user.login_time = 0;
		size = wstrlen( s_def_pw_user );
		if( size > 0 )
			wmemcpy( login.user.pw, sizeof(login.user.pw), s_def_pw_user, size );

		write_buf[idx] = 'C';	idx++;
		write_buf[idx] = 'A';	idx++;
		write_buf[idx] = 'F';	idx++;
		write_buf[idx] = 'E';	idx++;
		
		wmemcpy( &write_buf[idx], sizeof(write_buf)-idx, &login, sizeof(login) );
		idx += sizeof(login);

		TB_aes_evp_encrypt( write_buf, idx, enc_buf, sizeof(enc_buf) );
		TBUI enc_size = (idx + AES_BLOCK_SIZE) / AES_BLOCK_SIZE * AES_BLOCK_SIZE;
		if( enc_size < sizeof(enc_buf) )
		{
			if( fwrite( (void *)&enc_buf[0], 1, enc_size, fp ) != enc_size )
			{
				TB_dbg_login( "ERROR. Write File\r\n" );
			}
			else
			{
				wmemcpy( &s_login, sizeof(s_login), &login, sizeof(login) );
				ret = LOGIN_ERROR_NONE;
			}
		}

		fflush( fp );
		fclose( fp );
	}

	return ret;
}

int TB_login_init( void )
{
	int ret = LOGIN_ERROR;

#if 0
	/***************************************************************************
	        평문				                        암호문
	 ***************************************************************************
	    "Kdnderms1!"  ======>	0xA4, 0xE5, 0xCD, 0xBC, 0x2D, 0x27, 0xFF, 0x5B,
						        0xAE, 0xD1, 0x2E, 0xBD, 0xEA, 0xDF, 0x46, 0x40
	 ***************************************************************************
	    "Kdnderms2@"  ======>	0x5E, 0x3E, 0xA9, 0x6C, 0x1D, 0x01, 0x6D, 0xC3,
						        0x47, 0x19, 0xF5, 0xD5, 0xA8, 0x49, 0x6E, 0x15
	***************************************************************************/
	TBUC des_buf[AES_BUF_SIZE+AES_BLOCK_SIZE] = {0, };
	TBUC enc_buf[AES_BUF_SIZE+AES_BLOCK_SIZE] = {0, };
	TBUI enc_size = 0;
	TBUI dec_size = 0;

	printf( "\r\n" );
	bzero( enc_buf, sizeof(enc_buf) );
	enc_size = TB_aes_evp_encrypt( "/root/account.dat", wstrlen("/root/account.dat"), enc_buf, sizeof(enc_buf) );
	printf( "/root/account.dat ======> (enc_size=%d)\r\n", enc_size );
	for( TBUI i=0; i<enc_size; i++ )
		printf( "0x%02X, ", enc_buf[i] );
	printf( "\r\n" );

	bzero( des_buf, sizeof(des_buf) );
	dec_size = TB_aes_evp_decrypt( enc_buf, enc_size, des_buf, sizeof(des_buf) );
	printf( "Des(dec_size=%d) ==> %s\r\n", dec_size, des_buf );
	printf( "\r\n" );

	printf( "///////////////////////////////////////////////////////////////////////////\r\n" );

	printf( "\r\n" );	
	bzero( enc_buf, sizeof(enc_buf) );
	enc_size = TB_aes_evp_encrypt( "/root/login_fail.dat", wstrlen("/root/login_fail.dat"), enc_buf, sizeof(enc_buf) );
	printf( "/root/login_fail.dat ======> (enc_size=%d)\r\n", enc_size );
	for( TBUI i=0; i<enc_size; i++ )
		printf( "0x%02X, ", enc_buf[i] );
	printf( "\r\n" );

	bzero( des_buf, sizeof(des_buf) );
	dec_size = TB_aes_evp_decrypt( enc_buf, enc_size, des_buf, sizeof(des_buf) );
	printf( "Des(dec_size=%d) ==> %s\r\n", dec_size, des_buf );
	printf( "\r\n" );

	printf( "///////////////////////////////////////////////////////////////////////////\r\n" );

	bzero( enc_buf, sizeof(enc_buf) );
	enc_size = TB_aes_evp_encrypt( "Kdnderms1!", wstrlen("Kdnderms1!"), enc_buf, sizeof(enc_buf) );	
	printf( "Kdnderms1! ======> (enc_size=%d)\r\n", enc_size );
	for( TBUI i=0; i<enc_size; i++ )
		printf( "0x%02X, ", enc_buf[i] );
	printf( "\r\n" );

	bzero( des_buf, sizeof(des_buf) );
	TB_aes_evp_decrypt( enc_buf, enc_size, des_buf, sizeof(des_buf) );
	printf( "Des ==> %s\r\n", des_buf );

	printf( "///////////////////////////////////////////////////////////////////////////\r\n" );
	
	bzero( enc_buf, sizeof(enc_buf) );
	enc_size = TB_aes_evp_encrypt( "Kdnderms2@", wstrlen("Kdnderms2@"), enc_buf, sizeof(enc_buf) );	
	printf( "Kdnderms2@ ======> (enc_size=%d)\r\n", enc_size );
	for( TBUI i=0; i<enc_size; i++ )
		printf( "0x%02X, ", enc_buf[i] );
	printf( "\r\n" );

	bzero( des_buf, sizeof(des_buf) );
	TB_aes_evp_decrypt( enc_buf, enc_size, des_buf, sizeof(des_buf) );
	printf( "Des ==> %s\r\n", des_buf );

	TB_util_delay( 3000 );
	exit(0);
#endif

	bzero( s_file_path_accnt, sizeof(s_file_path_accnt) );
	bzero( s_file_path_lgn_fail, sizeof(s_file_path_lgn_fail) );
	TB_aes_evp_decrypt( s_file_path_accnt_aes, sizeof(s_file_path_accnt_aes), s_file_path_accnt, sizeof(s_file_path_accnt) );
	TB_aes_evp_decrypt( s_file_path_lgn_fail_aes, sizeof(s_file_path_lgn_fail_aes), s_file_path_lgn_fail, sizeof(s_file_path_lgn_fail) );
	TB_dbg_login( "***************** ACCOUNT PATH INFO **************\r\n" );
	TB_dbg_login( "     ACCOUNT     : %s\r\n", s_file_path_accnt );
	TB_dbg_login( "     LOGIN FAIL  : %s\r\n", s_file_path_lgn_fail);
	TB_dbg_login( "**************************************************\r\n" );

	bzero( s_def_pw_admin, sizeof(s_def_pw_admin) );
	bzero( s_def_pw_user,  sizeof(s_def_pw_user) );
	TB_aes_evp_decrypt( s_def_pw_admin_aes, sizeof(s_def_pw_admin_aes), s_def_pw_admin, sizeof(s_def_pw_admin) );
	TB_aes_evp_decrypt( s_def_pw_user_aes, sizeof(s_def_pw_user_aes), s_def_pw_user, sizeof(s_def_pw_user) );

	TB_dbg_login( "************** DEFAULT ACCOUNT INFO **************\r\n" );
	TB_dbg_login( "     ADMIN : pw=%s\r\n", s_def_pw_admin );
	TB_dbg_login( "     USER  : pw=%s\r\n", s_def_pw_user);
	TB_dbg_login( "**************************************************\r\n" );

	if( access(s_file_path_accnt, F_OK) == 0 )
	{
		FILE *fp = fopen( s_file_path_accnt, "r" );
		if( fp )
		{
			TBUC   read_buf[4+AES_BUF_SIZE+AES_BLOCK_SIZE+1] = {0, };
			TBUC   dec_buf [4+AES_BUF_SIZE+AES_BLOCK_SIZE+1] = {0, };

			size_t read_size = 0;

			TB_LOGIN *p_login = NULL;
			
			bzero( read_buf, sizeof(read_buf) );
			read_buf[0] = '\0';
			read_size = fread( read_buf, 1, sizeof(read_buf)-1, fp );
			if( read_size > 0 )
			{
				read_buf[read_size] = '\0';
				TB_aes_evp_decrypt( read_buf, read_size, dec_buf, sizeof(dec_buf) );

				if( dec_buf[0] == 'C' && dec_buf[1] == 'A' && dec_buf[2] == 'F' && dec_buf[3] == 'E' )
				{
					p_login = (TB_LOGIN *)&dec_buf[4];
					TB_dbg_login( "************** ACCOUNT INFO **************\r\n" );
					TB_dbg_login( "     ADMIN : pw=%s, login time=%s\r\n", p_login->admin.pw, TB_util_get_datetime_string( p_login->admin.login_time ) );
					TB_dbg_login( "     USER  : pw=%s, login time=%s\r\n", p_login->user.pw, TB_util_get_datetime_string( p_login->user.login_time ) );
					TB_dbg_login( "*******************************************\r\n" );

					wmemcpy( &s_login, sizeof(s_login), p_login, sizeof(TB_LOGIN) );
					ret = LOGIN_ERROR_NONE;
				}
				else
				{
					TB_dbg_login( "ERROR. Invalid Account file header : 0x%02X 0x%02X 0x%02X 0x%02X\r\n",	\
									read_buf[0], read_buf[1], read_buf[2], read_buf[3] );
				}
			}
			else
			{
				TB_dbg_login( "ERROR. Invalid PW\r\n" );
			}
			
			fclose(fp);
		}
	}
	else
	{		
		ret = TB_login_create_default_account_file();
	}

	////////////////////////////////////////////////////////////////////////////

	if( ret == LOGIN_ERROR_NONE )
	{
		ret = TB_login_check_account( s_login.admin, ACCOUNT_TYPE_ADMIN );
		if( ret != LOGIN_ERROR_NONE )
		{
			if( ret != LOGIN_ERROR_PW_DEFAULT )
			{
				TB_login_create_default_account_file();
				ret = LOGIN_ERROR_PW_DEFAULT;
			}
			
			if( ret == LOGIN_ERROR_PW_DEFAULT )
			{
				TB_ui_init();
				TB_login_show_change_pw( ACCOUNT_TYPE_ADMIN, TRUE );
				TB_ui_deinit();
			}
		}
		
		ret = TB_login_check_account( s_login.user, ACCOUNT_TYPE_USER );
		if( ret != LOGIN_ERROR_NONE )
		{
			if( ret != LOGIN_ERROR_PW_DEFAULT )
			{
				TB_login_create_default_account_file();
				ret = LOGIN_ERROR_PW_DEFAULT;
			}
			
			if( ret == LOGIN_ERROR_PW_DEFAULT )
			{
				TB_ui_init();
				TB_login_show_change_pw( ACCOUNT_TYPE_USER, TRUE );
				TB_ui_deinit();
			}
		}
	}
	else
	{
		TB_login_create_default_account_file();
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////

struct change_pw
{
	TBUC current[MAX_PW_LEN+1];
	TBUC input  [MAX_PW_LEN+1];
	TBUC confirm[MAX_PW_LEN+1];

	TBUC focus;
} s_change_pw;

static int s_pw_star_startx = 20;

int menu_chg_pw_linenum[FOCUS_CHG_PW_MAX] = { 2, 3, 4 };
static void tb_login_print_change_pw( TB_ACCOUNT_TYPE type )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s", type==ACCOUNT_TYPE_ADMIN ? TB_resid_get_string(RESID_TLT_CHG_ADMIN_PW) : TB_resid_get_string(RESID_TLT_CHG_USER_PW) );

	TB_ui_move( 2, 0 );
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_PW_CURRENT)); s_change_pw.focus==0 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx );	printw("*" );

	TB_ui_move( 3, 0 );
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_PW_NEW)); s_change_pw.focus==1 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );

	TB_ui_move( 4, 0 );
	TB_ui_attrset( FOCUS_OFF );	printw("%s : ", TB_resid_get_string(RESID_LBL_PW_CONFIRM)); s_change_pw.focus==2 ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s", TB_resid_get_string(RESID_TIP_ESC) );
	
	TB_ui_refresh();
}

static void tb_login_ui_clrtoeol( TBUC *p_pw, int x, int y )
{
	if( p_pw )
	{
		bzero( p_pw, MAX_PW_LEN+1 );
		TB_ui_move( y, x );
		TB_ui_clear( UI_CLEAR_LINE_END );		
		TB_ui_attrset( FOCUS_ON );
		printw( "*" );
	}
}

static int tb_login_ui_check_pw_current( TBUC *p_pw_current, TBUC *p_pw_default )
{
	int ret = -1;
	
	if( (p_pw_current && wstrlen(p_pw_current) > 0) &&	\
		(p_pw_default && wstrlen(p_pw_default) > 0) )
	{
		if( TB_login_check_pw_attribute( p_pw_current ) != LOGIN_ERROR_NONE )
		{
			tb_login_ui_clrtoeol( p_pw_current, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
			return ret;
		}
		else
		{
			int in_pw_len = wstrlen( p_pw_current );
			if( memcmp( p_pw_default, p_pw_current, in_pw_len ) != 0 )
			{
				tb_login_ui_clrtoeol( p_pw_current, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
				return ret;
			}
			else
			{
				/*
				*	redraw to current focus line
				*/			
				if( in_pw_len <= MAX_PW_LEN )
				{
					TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx );
					TB_ui_clear( UI_CLEAR_LINE_END );
					TB_ui_attrset( FOCUS_OFF );

					int i;
					for( i=0; i<in_pw_len; i++ )
					{
						TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx + i );
						printw( "*" );
					}
				}

				/*
				*	TB_ui_move focus to next line
				*/
				s_change_pw.focus ++;
				if( s_change_pw.focus < FOCUS_CHG_PW_MAX )
				{
					TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx );
					TB_ui_attrset( FOCUS_ON );
					printw( "*" );
				}
				
				ret = 0;
			}
		}
	}			

	return ret;
}

static int tb_login_ui_check_pw_input( TBUC *p_pw_input, TBUC *p_pw_current )
{
	int ret = -1;
	
	if( (p_pw_input   && wstrlen(p_pw_input)   > 0) &&	\
		(p_pw_current && wstrlen(p_pw_current) > 0) )
	{
		if( TB_login_check_pw_attribute( p_pw_input ) != LOGIN_ERROR_NONE )
		{
			tb_login_ui_clrtoeol( p_pw_input, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
		}
		else
		{
			int in_pw_len = wstrlen( p_pw_input );
			if( memcmp(p_pw_current, p_pw_input, wstrlen(p_pw_current)) != 0 )
			{
				if( memcmp(s_def_pw_admin, p_pw_input, wstrlen(p_pw_input)) == 0 )	//	If same as default admin pw. 
				{
					tb_login_ui_clrtoeol( p_pw_input, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
				}
				else if( memcmp(s_def_pw_user, p_pw_input, wstrlen(p_pw_input)) == 0 )	//	If same as default user pw. 
				{
					tb_login_ui_clrtoeol( p_pw_input, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
				}
				else
				{			
					/*
					*	redraw to current focus line
					*/			
					if( in_pw_len <= MAX_PW_LEN )
					{
						TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx );
						TB_ui_clear( UI_CLEAR_LINE_END );
						TB_ui_attrset( FOCUS_OFF );

						int i;
						for( i=0; i<in_pw_len; i++ )
						{
							TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx + i );
							printw( "*" );
						}
					}

					/*
					*	TB_ui_move focus to next line
					*/
					s_change_pw.focus ++;
					if( s_change_pw.focus < FOCUS_CHG_PW_MAX )
					{
						TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx );
						TB_ui_attrset( FOCUS_ON );
						printw( "*" );
					}

					ret = 0;
				}
			}
			else
			{
				tb_login_ui_clrtoeol( p_pw_input, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
				//TB_show_msgbox( TB_msgid_get_string(MSGID_PW_EQUAL), 0, TRUE );
			}
		}
	}

	return ret;
}

static int tb_login_ui_check_pw_confirm( TBUC *p_pw_confirm, TBUC *p_pw_input )
{
	int ret = -1;
	
	if( (p_pw_confirm && wstrlen(p_pw_confirm) > 0) &&	\
		(p_pw_input   && wstrlen(p_pw_input)   > 0) )
	{
		if( TB_login_check_pw_attribute( p_pw_confirm ) != LOGIN_ERROR_NONE )
		{
			/*
			*	clear a cofirm data.
			*/
			bzero( p_pw_confirm, MAX_PW_LEN+1 );
			TB_ui_move( menu_chg_pw_linenum[FOCUS_CHG_PW_CONFIRM], s_pw_star_startx );
			TB_ui_clear( UI_CLEAR_LINE_END );
			
			/*
			*	clear a input data and focus TB_ui_move.
			*/
			s_change_pw.focus = FOCUS_CHG_PW_INPUT;
			tb_login_ui_clrtoeol( p_pw_input, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
		}
		else
		{
			int in_pw_len = wstrlen( p_pw_confirm );
			if( memcmp(p_pw_input, p_pw_confirm, wstrlen(p_pw_confirm)) == 0 )
			{
				/*
				*	redraw to current focus line
				*/			
				if( in_pw_len <= MAX_PW_LEN )
				{
					TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx );
					TB_ui_clear( UI_CLEAR_LINE_END );
					TB_ui_attrset( FOCUS_OFF );

					int i;
					for( i=0; i<in_pw_len; i++ )
					{
						TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx + i );
						printw( "*" );
					}
				}
				
				TB_show_msgbox( TB_msgid_get_string(MSGID_PW_CHANGED), 0, TRUE );
				ret = 0;
			}
			else
			{
				tb_login_ui_clrtoeol( p_pw_confirm, s_pw_star_startx, menu_chg_pw_linenum[s_change_pw.focus] );
			}
		}
	}

	return ret;
}

#define AVAILABLE_PW_CHAR	0x80
int TB_login_show_change_pw( TB_ACCOUNT_TYPE type, TBBL force )
{
	int quit = 0;
	int input;
	int input_char;

	bzero( &s_change_pw, sizeof(s_change_pw) );
	tb_login_print_change_pw( type );

	TB_ui_key_init( TRUE );
	while( !quit )
	{
		input_char = input = TB_ui_getch( TRUE );
		if( CHK_AVALABLE_PW_CHAR(input) )
		{		
			input = AVAILABLE_PW_CHAR;
		}
		
	    switch( input )
	    {
			case AVAILABLE_PW_CHAR:
				{
					TBUC *p_pw = NULL;
					switch( s_change_pw.focus )
					{
						case FOCUS_CHG_PW_CURRENT	:	p_pw = s_change_pw.current;		break;
						case FOCUS_CHG_PW_INPUT		:	p_pw = s_change_pw.input;		break;
						case FOCUS_CHG_PW_CONFIRM	:	p_pw = s_change_pw.confirm;		break;
						default						:	break;
					}

					if( p_pw )
					{
						int len = wstrlen( p_pw );
						if( len < MAX_PW_LEN )
						{
							p_pw[len] = input_char;
							len ++;

							int i;
							for( i=0; i<=len; i++ )
							{
								TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx + i );
								( i == len ) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
								printw( "*" );
							}
						}
					}
				}
				break;
				
			case KEY_BACKSPACE :
				{
					TBUC *p_pw = NULL;
					switch( s_change_pw.focus )
					{
						case FOCUS_CHG_PW_CURRENT	:	p_pw = s_change_pw.current;		break;
						case FOCUS_CHG_PW_INPUT		:	p_pw = s_change_pw.input;		break;
						case FOCUS_CHG_PW_CONFIRM	:	p_pw = s_change_pw.confirm;		break;
						default						:	break;
					}

					if( p_pw )
					{
						int len = wstrlen( p_pw );
						if( len > 0 )
						{
							len --;
							p_pw[len] = 0;

							TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx );
							TB_ui_clear( UI_CLEAR_LINE_END );

							int i;
							for( i=0; i<=len; i++ )
							{
								TB_ui_move( menu_chg_pw_linenum[s_change_pw.focus], s_pw_star_startx + i );
								( i == len ) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
								printw( "*" );
							}
						}
					}
				}
				break;
				
			case '\n' 		:
			case KEY_ENTER 	:
				switch( s_change_pw.focus )
				{
					case FOCUS_CHG_PW_CURRENT 	:
						tb_login_ui_check_pw_current( s_change_pw.current, (type == ACCOUNT_TYPE_ADMIN) ? s_login.admin.pw : s_login.user.pw );
						break;
						
					case FOCUS_CHG_PW_INPUT		:
						tb_login_ui_check_pw_input( s_change_pw.input, s_change_pw.current );
						break;
						
					case FOCUS_CHG_PW_CONFIRM	:
						if( tb_login_ui_check_pw_confirm( s_change_pw.confirm, s_change_pw.input ) == 0 )
						{
							if( type == ACCOUNT_TYPE_ADMIN )
							{
								s_login.admin.login_time = time( NULL );
								wmemcpy( s_login.admin.pw, sizeof(s_login.admin.pw), s_change_pw.confirm, sizeof(s_login.admin.pw) );

								TB_log_append( RESID_LOG_SECU_PW_CHANGE_ADMIN, NULL, -1 );
							}
							else
							{
								s_login.user.login_time = time( NULL );
								wmemcpy( s_login.user.pw, sizeof(s_login.user.pw), s_change_pw.confirm, sizeof(s_login.user.pw) );

								TB_log_append( RESID_LOG_SECU_PW_CHANGE_USER, NULL, -1 );
							}

							////////////////////////////////////////////////////

							int idx = 0;
							TBUC aes_buf[AES_BUF_SIZE] = {0, };

							aes_buf[idx] = 'C';		idx++;
							aes_buf[idx] = 'A';		idx++;
							aes_buf[idx] = 'F';		idx++;
							aes_buf[idx] = 'E';		idx++;
							wmemcpy( &aes_buf[idx], sizeof(aes_buf)-4, (TBUC *)&s_login, sizeof(s_login) );
							idx += sizeof(s_login);
							TB_aes_evp_encrypt_buf2file( aes_buf, idx, s_file_path_accnt );
							
							quit = 1;
						}
						break;
				}
				break;

			case KEY_ESC		:
				if( force == FALSE )
				{
					quit = 1;
				}
				else
				{
					TB_show_msgbox( TB_msgid_get_string(MSGID_PW_MUST_CHANGE), 0, TRUE );
					bzero( &s_change_pw, sizeof(s_change_pw) );
					tb_login_print_change_pw( type );
				}
				break;

			case KEY_AUTO_QUIT 	:
				if( force == FALSE )
				{
					quit = 2;
				}
				else
				{
					TB_show_msgbox( TB_msgid_get_string(MSGID_PW_MUST_CHANGE), 0, TRUE );
					bzero( &s_change_pw, sizeof(s_change_pw) );
					tb_login_print_change_pw( type );
				}
				break;
	    }
	}	

	return quit;
}

////////////////////////////////////////////////////////////////////////////////

#define MAX_LOGIN_TRY_CNT		5
#define MAX_LOGIN_BLOCK_TIME	(5*60)	//5min

int menu_input_pw_linenum = 2;
static void tb_login_print_login_pw( void )
{
	TB_ui_clear( UI_CLEAR_ALL );
	TB_ui_attrset( FOCUS_OFF );	printw("%s", TB_resid_get_string(RESID_TLT_LOGIN));

	TB_ui_move( menu_input_pw_linenum, 0 );					TB_ui_attrset( FOCUS_OFF );	printw("%s", TB_resid_get_string(RESID_LBL_INPUT_PW) );
	TB_ui_move( menu_input_pw_linenum, s_pw_star_startx );	TB_ui_attrset( FOCUS_ON ); 	printw("*" );
	printw("\n\n");
	TB_ui_attrset( FOCUS_OFF );	printw("%s", TB_resid_get_string(RESID_TIP_ESC) );
	TB_ui_refresh();
}

static time_t 	s_login_fail_time = 0;
static int 		s_login_fail_cnt  = 0;
static void tb_login_clear_login_fail_info( void )
{
	s_login_fail_time = 0;
	s_login_fail_cnt  = 0;

	unlink( s_file_path_lgn_fail );
}

static int tb_login_save_login_fail_time( time_t login_fail_time )
{
	int ret = -1;

	FILE *fp = fopen( s_file_path_lgn_fail, "w" );
	if( fp )
	{
		size_t size = sizeof( login_fail_time );
		if( fwrite( &login_fail_time, 1, size, fp ) != size )
		{			
			TB_dbg_login( "ERROR. Login Fail : Write File\r\n" );
		}

		fflush( fp );
		fclose( fp );
		ret = 0;
	}

	TBUC *time_str = TB_util_get_datetime_string( login_fail_time );
	TB_dbg_login( "login_fail_time = %d : %s\r\n", login_fail_time, time_str );

	return ret;
}

static time_t tb_login_load_login_fail_time( void )
{
	time_t login_fail_time = 0;
	if( access(s_file_path_lgn_fail, F_OK) == 0 )
	{
		FILE *fp = fopen( s_file_path_lgn_fail, "r" );
		if( fp )
		{
			time_t temp_time = 0;
			TBUC read_buf[sizeof(time_t) + 1];
			size_t size = sizeof(read_buf) - 1;
			if( fread( read_buf, 1, size, fp ) != size )
			{
				read_buf[size] = '\0';
				login_fail_time = 0;
				TB_dbg_login( "ERROR. Login Fail time : Read File\r\n" );
			}
			else
			{
				wmemcpy( &temp_time, sizeof(time_t), read_buf, size );
				if( TB_util_time_check_validation( temp_time ) == FALSE )
				{
					login_fail_time = 0;
					TB_dbg_login( "ERROR. Login Fail time : Invalid time data\r\n" );
				}
				else
				{
					login_fail_time = temp_time;
				}
			}

			/*******************************************************************
			*	login fail time 값이 비정상일 때 last working time 값으로 대체한다.
			*******************************************************************/
			if( login_fail_time == 0 )
			{
				TB_log_last_working_time_read( &login_fail_time );
			}

			fclose(fp);
		}

		TBUC *time_str = TB_util_get_datetime_string( login_fail_time );
		TB_dbg_login( "login_fail_time = %d : %s\r\n", login_fail_time, time_str );
	}

	return login_fail_time;

}

static int tb_login_fail_count_increment( TBUC *input_pw )
{
	int ret = 0;

	if( input_pw )
	{
		tb_login_ui_clrtoeol( input_pw, s_pw_star_startx, menu_input_pw_linenum );

		s_login_fail_cnt ++;
		if( s_login_fail_cnt >= MAX_LOGIN_TRY_CNT )
		{			
			s_login_fail_time = time( NULL );
			tb_login_save_login_fail_time( s_login_fail_time );

			TB_show_msgbox( TB_msgid_get_string(MSGID_LOGIN_FAIL), 0, TRUE );

			TB_log_append( LOG_SECU_LOGIN_FAIL, NULL, -1 );
			
			ret = 1;
		}
	}

	return ret;
}

int TB_login_check_login_fail_info( void )
{
	s_login_fail_time = tb_login_load_login_fail_time();
	if( s_login_fail_time != 0 )
	{
		time_t curr_time = time( NULL );

		/***********************************************************************
		*	부팅 시에 아직 시스템 시간이 변경이 안되었을 때	현재 시간은 로그인
		*	실패시간 보다 과거 시간이기 때문에 경과 시간을 계산할 수 없다.
		*	따라서 로그인 실패 시간으로 시스템 시간을 임시로 설정한다.
		***********************************************************************/
		if( s_login_fail_time > curr_time )
		{
			TB_util_set_systemtime2( s_login_fail_time );
			curr_time = s_login_fail_time;
		}

		if( curr_time >= s_login_fail_time )
		{
			time_t elapse_time = curr_time - s_login_fail_time;

			if( elapse_time < MAX_LOGIN_BLOCK_TIME )
			{
				char msg[MAX_MSG_LEN] = {0, };
				char *str1 = TB_msgid_get_string( MSGID_LOGIN_RETRY_5MIN );
				char *str2 = TB_msgid_get_string( MSGID_LOGIN_RETRY_REMAIN );

				if( str1 && str2 )
				{
					time_t remain_time = MAX_LOGIN_BLOCK_TIME - elapse_time;

					snprintf( msg, sizeof(msg), "%s %d %s",	str1, remain_time, str2 );
					TB_show_msgbox( msg, 0, TRUE );
					return  LOGIN_ERROR;
				}
			}
		}
	}

	return LOGIN_ERROR_NONE;
}

int TB_login_show_input_pw( TB_ACCOUNT_TYPE type )
{
	int ret = LOGIN_ERROR;
	int input, input_char;
	int idx = 0;
	int quit = 0;

	TBUC input_pw[MAX_PW_LEN+1];

	TB_dm_dbg_mode_all_off();

	bzero( input_pw, sizeof(input_pw) );

	s_login_fail_time = tb_login_load_login_fail_time();
	if( s_login_fail_time != 0 )
	{
		time_t curr_time = time( NULL );

		/***********************************************************************
		*	부팅 시에 아직 시스템 시간이 변경이 안되었을 때	현재 시간은 로그인
		*	실패시간 보다 과거 시간이기 때문에 경과 시간을 계산할 수 없다.
		*	따라서 로그인 실패 시간으로 시스템 시간을 임시로 설정한다.
		***********************************************************************/
		if( s_login_fail_time > curr_time )
		{
			TB_util_set_systemtime2( s_login_fail_time );
			curr_time = s_login_fail_time;
		}

		if( curr_time >= s_login_fail_time )
		{
			time_t diff_time = curr_time - s_login_fail_time;

			if( diff_time < MAX_LOGIN_BLOCK_TIME )
			{
				char msg[MAX_MSG_LEN] = {0, };
				char *str1 = TB_msgid_get_string( MSGID_LOGIN_RETRY_5MIN );
				char *str2 = TB_msgid_get_string( MSGID_LOGIN_RETRY_REMAIN );

				if( str1 && str2  )
				{
					diff_time = MAX_LOGIN_BLOCK_TIME - diff_time;
					snprintf( msg, sizeof(msg), "%s %d %s", str1,	diff_time, str2 );
					TB_show_msgbox( msg, 0, TRUE );
					return  ret;
				}
			}
		}
		else
		{
			TB_dbg_login( "login_fail_time = %d, curr_time = %d\r\n", s_login_fail_time, curr_time );
		}
	}

	TB_ui_clear( UI_CLEAR_ALL );
	
	TB_login_init();
	tb_login_clear_login_fail_info();
	tb_login_print_login_pw();	

	TB_ui_key_init( TRUE );
	while( !quit )
	{
		input_char = input = TB_ui_getch( TRUE );
		if( CHK_AVALABLE_PW_CHAR(input) )
		{		
			input = AVAILABLE_PW_CHAR;
		}
		
	    switch( input )
	    {
			case AVAILABLE_PW_CHAR:
				{
					int len = wstrlen( input_pw );
					if( len < MAX_PW_LEN )
					{
						input_pw[len] = (TBUC)input_char;
					}
					
					int i;
					for( i=0; i<=len; i++ )
					{
						TB_ui_move( menu_input_pw_linenum, s_pw_star_startx + i );
						( i == len ) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
						printw( "*" );
					}
				}
				break;
				
			case KEY_BACKSPACE :
				{
					int len = wstrlen( input_pw );
					if( len > 0 )
					{
						len --;
						input_pw[len] = 0;

						TB_ui_move( menu_input_pw_linenum, s_pw_star_startx );
						TB_ui_clear( UI_CLEAR_LINE_END );

						int i;
						for( i=0; i<=len; i++ )
						{
							TB_ui_move( menu_input_pw_linenum, s_pw_star_startx + i );
							( i == len ) ? TB_ui_attrset( FOCUS_ON ) : TB_ui_attrset( FOCUS_OFF );
							printw( "*" );
						}
					}
				}
				break;

			case '\n' 		:
			case KEY_ENTER 	:
				if( TB_login_check_pw_attribute( input_pw ) == LOGIN_ERROR_NONE )
				{
					int len_user  = wstrlen( s_login.user.pw );
					int len_admin = wstrlen( s_login.admin.pw );
					int len_input = wstrlen( input_pw );
					
					if( type == ACCOUNT_TYPE_ADMIN )
					{
						if( len_admin == len_input )
						{
							if( memcmp(s_login.admin.pw, input_pw, len_input) != 0 )
							{
								quit = tb_login_fail_count_increment( input_pw );
							}
							else
							{
								TB_log_append( LOG_SECU_LOGIN, (TB_LOGCODE_DATA *)NULL, ACCOUNT_TYPE_ADMIN );

								quit = 1;
								tb_login_clear_login_fail_info();
								ret = ACCOUNT_TYPE_ADMIN;
							}
						}
						else
						{
							quit = tb_login_fail_count_increment( input_pw );
						}
					}
					else 	//	User, Admin 계정 가능
					{
						if( len_user == len_input )
						{
							if( memcmp(s_login.user.pw, input_pw, len_input) == 0 )
							{
								TB_log_append( LOG_SECU_LOGIN, NULL, ACCOUNT_TYPE_USER );

								quit = 1;
								tb_login_clear_login_fail_info();
								ret = ACCOUNT_TYPE_USER;
							}
						}

						if( ret != ACCOUNT_TYPE_USER )
						{
							if( len_admin == len_input )
							{
								if( memcmp(s_login.admin.pw, input_pw, len_input) == 0 )
								{
									TB_log_append( LOG_SECU_LOGIN, NULL, ACCOUNT_TYPE_ADMIN );

									quit = 1;
									tb_login_clear_login_fail_info();
									ret = ACCOUNT_TYPE_ADMIN;
								}
								else
								{
									quit = tb_login_fail_count_increment( input_pw );
								}
							}
							else
							{
								quit = tb_login_fail_count_increment( input_pw );
							}
						}
					}
				}
				else
				{
					quit = tb_login_fail_count_increment( input_pw );
				}
				break;

			case KEY_ESC		:
				quit = 1;
				break;

			case KEY_AUTO_QUIT 	:
				quit = 2;
				break;
	    }
	}

	TB_ui_clear( UI_CLEAR_ALL );

	TB_dm_dbg_mode_restore();

	return ret;
}

int TB_login_account_clear( void )
{
	int ret = -1;
	if( access(s_file_path_accnt, F_OK) == 0 )
	{
		ret = unlink( s_file_path_accnt );
	}

	return ret;
}

