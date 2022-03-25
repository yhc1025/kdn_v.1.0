#include "TB_util.h"
#include "TB_led.h"
#include "TB_setup.h"
#include "TB_timer_util.h"
#include "TB_ems.h"
#include "TB_kcmvp.h"

////////////////////////////////////////////////////////////////////////////////

static TB_KCMVP_INFO s_keyinfo;

////////////////////////////////////////////////////////////////////////////////

int TB_kcmvp_key_check( void )
{
	int ret = 0;
	
	TB_KCMVP_KEY *p_key = TB_setup_get_kcmvp_master_key();
	if( p_key )
	{
		if( p_key->size == LEN_BYTE_KEY )
		{
			int i;
			int cnt = 0;
			for( i=0; i<LEN_BYTE_KEY; i++ )
			{
				if( p_key->data[i] == 0x00 )
					cnt ++;
			}

			if( cnt == LEN_BYTE_KEY )
			{
				ret = -1;
			}
		}
		else
		{
			ret = -1;
		}
	}
	else
	{
		ret = -1;
	}

	return ret;
}

static TBUC s_iv[] = { 0xED, 0xAC, 0xCA, 0xDF, 0xBA, 0x9B, 0x6A, 0xD7, 0x9C, 0x2C, 0xCD, 0xE9, 0x1B, 0xA6, 0x74, 0xAD};
static TBUC s_au[] = { 0x1B, 0xF6, 0x68, 0x91, 0x84, 0xB5, 0xEB, 0x90, 0x62, 0x87, 0x52, 0x09, 0xCE, 0x5F };

int TB_kcmvp_pre_init( void )
{
	bzero( &s_keyinfo, sizeof(s_keyinfo) );
	
	wmemcpy( &s_keyinfo.iv.data[0], sizeof(s_keyinfo.iv.data), &s_iv[0], LEN_BYTE_IV );
	s_keyinfo.iv.size = LEN_BYTE_IV;
	
	wmemcpy( &s_keyinfo.auth.data[0], sizeof(s_keyinfo.auth.data), &s_au[0], LEN_BYTE_AUTH );
	s_keyinfo.auth.size = LEN_BYTE_AUTH;

	return 0;
}

int TB_kcmvp_init( void )
{
	KDN_INFO kcmvp_info;
    if( KDN_GetInfo( &kcmvp_info ) == KDN_OK )
    {
		TB_dbg_kcmvp( "=============== KCMVP Info ================\r\n" );
		TB_dbg_kcmvp( "    Name              : %s\r\n",		kcmvp_info.libraryName );
		TB_dbg_kcmvp( "    Interface Version : %d.%d\r\n", 	kcmvp_info.KdnapiVersion.major, kcmvp_info.KdnapiVersion.minor );
		TB_dbg_kcmvp( "    Library Version   : %d.%d\r\n", 	kcmvp_info.libraryVersion.major, kcmvp_info.libraryVersion.minor );
		TB_dbg_kcmvp( "===========================================\r\n" );
    }

	if( TB_kcmvp_key_check() != 0 )
	{
		TB_setup_enter( ACCOUNT_TYPE_ADMIN, TRUE );
	}
	
	////////////////////////////////////////////////////////////////////////////

	TB_KCMVP_KEY *p_key = TB_setup_get_kcmvp_master_key();	
	wmemcpy( &s_keyinfo.master_key, sizeof(s_keyinfo.master_key), p_key, sizeof(TB_KCMVP_KEY) );

	wmemcpy( &s_keyinfo.iv.data[0], sizeof(s_keyinfo.iv.data), &s_iv[0], LEN_BYTE_IV );
	s_keyinfo.iv.size = LEN_BYTE_IV;
	
	wmemcpy( &s_keyinfo.auth.data[0], sizeof(s_keyinfo.auth.data), &s_au[0], LEN_BYTE_AUTH );
	s_keyinfo.auth.size = LEN_BYTE_AUTH;

	bzero( &s_keyinfo.tag.data[0], LEN_BYTE_TAG );
	s_keyinfo.tag.size = LEN_BYTE_TAG;
	
	return 0;
}

int TB_kcmvp_key_gen( TB_KCMVP_KEY *p_key )
{
	int ret = -1;

	if( p_key )
	{
		bzero( p_key, sizeof(TB_KCMVP_KEY) );

		TBUI kcmvp_ret = KDN_Keygen( p_key->data, LEN_BYTE_KEY*8/*128bit*/, KDN_BC_Algo_ARIA_GCM );
		if( kcmvp_ret == KDN_OK )
		{
			p_key->size = LEN_BYTE_KEY;

			if( TB_dm_is_on(DMID_KCMVP) )
			{
				TB_dbg_kcmvp("OK. KeyGen\r\n" );	
				TB_util_data_dump("KCMVP KEY VALUE", p_key->data, LEN_BYTE_KEY );
			}

			ret = 0;
		}
		else
		{
			if( TB_dm_is_on(DMID_KCMVP) )
				TB_dbg_kcmvp("ERROR. KeyGen : %d\n", kcmvp_ret );
		}
	}

	return ret;
}

TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_iv( void )
{
	return &s_keyinfo.iv;
}

TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_auth( void )
{
	return &s_keyinfo.auth;
}

TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_tag( void )
{
	return &s_keyinfo.tag;
}

TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_master_key( void )
{
	return &s_keyinfo.master_key;
}

TB_KCMVP_INFO *TB_kcmvp_get_keyinfo( void )
{
	return &s_keyinfo;
}

int TB_kcmvp_set_keyinfo_tag( TB_KCMVP_KEY *p_tag )
{
	int ret = -1;

	if( p_tag )
	{
		wmemcpy( &s_keyinfo.tag, sizeof(s_keyinfo.tag), p_tag, sizeof(TB_KCMVP_KEY) );
		ret = 0;
	}

	return ret;
}

int TB_kcmvp_set_keyinfo_master_key( TB_KCMVP_KEY *p_key )
{
	int ret = -1;

	if( p_key )
	{
		wmemcpy( &s_keyinfo.master_key, sizeof(s_keyinfo.master_key), p_key, sizeof(TB_KCMVP_KEY) );
		ret = 0;
	}

	return ret;
}

#define TIME_PERIOD_SESSION_KEY		(10*1000)	//	10sec. msec
static TB_TIMER *s_timer_session_key = NULL;
void timer_proc_session_key( void *arg )
{	
	if( s_keyinfo.session_key_new.size > 0 )
	{
		time_t update_time 	= TB_setup_get_session_update_time();
		time_t curr_time 	= time( NULL );

		curr_time = curr_time % (24*60*60);	//	1day

		if( TB_dm_is_on(DMID_KCMVP) )
		{
			TB_util_print_time2( update_time );
			TB_util_print_time2( curr_time );
		}
		
		if( update_time <= curr_time )
		{
			/*******************************************************************
			*	ex. when update_time is 06:00,
			*		curr_time 06:00:00 ~ 06:01:00
			*******************************************************************/
			TBUI diff = curr_time - update_time;
			if( diff <= 60 )
			{
				TB_KCMVP_KEY session_key;

				TB_dbg_kcmvp("TRY. Set a ***NOW*** session key by ***NEW*** session key\r\n");

				bzero( &session_key, sizeof(TB_KCMVP_KEY) );
				wmemcpy( &session_key, sizeof(session_key), &s_keyinfo.session_key_new, sizeof(TB_KCMVP_KEY) );				
				bzero( &s_keyinfo.session_key_new, sizeof(TB_KCMVP_KEY) );				
				wmemcpy( &s_keyinfo.session_key_now, sizeof(s_keyinfo.session_key_now), &session_key, sizeof(TB_KCMVP_KEY) );

				TB_dbg_kcmvp("OK. Set a ***NOW*** session key by ***NEW*** session key\r\n");
			}
		}
	}	
}

int TB_kcmvp_set_keyinfo_session_key( TB_KCMVP_KEY *p_key )
{
	int ret = -1;

	if( p_key )
	{
		if( s_keyinfo.session_key_now.size == 0 )
		{
			wmemcpy( &s_keyinfo.session_key_now, sizeof(s_keyinfo.session_key_now), p_key, sizeof(TB_KCMVP_KEY) );
			TB_dbg_kcmvp("SET a Session Key\r\n");
		}
		else
		{
			if( memcmp( &s_keyinfo.session_key_now, p_key, sizeof(TB_KCMVP_KEY) ) != 0 )
			{
				/***************************************************************
				* 이미 세션키가 2개일 때, EMS로부터 새로운 세션키가 들어왔을 때
				***************************************************************/
				if( s_keyinfo.session_key_new.size > 0 )
				{
					TB_dbg_kcmvp("A new session key exists. Changes to the received new session key.\r\n");
					if( memcmp( &s_keyinfo.session_key_new, p_key, sizeof(TB_KCMVP_KEY) ) != 0 )
					{
						bzero( &s_keyinfo.session_key_new, sizeof(TB_KCMVP_KEY) );
					}
				}
				
				if( s_keyinfo.session_key_new.size == 0 )
				{
					wmemcpy( &s_keyinfo.session_key_new, sizeof(s_keyinfo.session_key_new), p_key, sizeof(TB_KCMVP_KEY) );

					TB_dbg_kcmvp("*********************************************\r\n");
					TB_dbg_kcmvp("*****        Set a New session key      *****\r\n");
					TB_dbg_kcmvp("*********************************************\r\n");

					TB_dbg_kcmvp("TRY. Start a session key swap timer\r\n");
					if( s_timer_session_key == NULL )
					{
						s_timer_session_key = TB_timer_handle_open( timer_proc_session_key,	TIME_PERIOD_SESSION_KEY );
						if( s_timer_session_key )
							TB_dbg_kcmvp("OK. Start Timer ( for session key swap )\r\n");
						else
							TB_dbg_kcmvp("FAIL. Start Timer ( for session key swap )\r\n");
					}
				}
				else
				{
					TB_dbg_kcmvp("Alreay SET a New Session Key\r\n");
				}
				
				ret = 0;
			}
			else
			{
				TB_dbg_ems("Same to Session key\r\n");
			}
		}
	}

	return ret;
}

static TB_KEY_TYPE s_key_type = 0;
int TB_kcmvp_set_keyinfo_key_type( TB_KEY_TYPE key_type )
{
	if( key_type == KCMVP_KEY_TYPE_SESSION_NEW )
	{
		TB_dbg_kcmvp("**** SET **** kcmvp encryption SESSION_NEW KEY\r\n" );
		s_key_type = key_type;
	}
	else if( key_type == KCMVP_KEY_TYPE_SESSION_NOW )
	{
		TB_dbg_kcmvp("**** SET **** kcmvp encryption SESSION_NOW KEY\r\n" );
		s_key_type = key_type;
	}
	else if( key_type == KCMVP_KEY_TYPE_MASTER )
	{
		TB_dbg_kcmvp("**** SET **** kcmvp encryption MASTER KEY\r\n" );
		s_key_type = key_type;
	}
	else
	{
		s_key_type = KCMVP_KEY_TYPE_NONE;
	}
}

int tb_kcmvp_encryption( TBUC *p_sour, TBUI sour_len, TBUC *p_dest, TBUI *p_dest_len )
{
	int ret = -1;

	if( p_sour && sour_len > 0 && p_dest && p_dest_len )
	{
		TB_KCMVP_INFO *p_keyinfo = TB_kcmvp_get_keyinfo();
		if( p_keyinfo )
		{
			TB_KCMVP_KEY *p_key = NULL;
			if( s_key_type == KCMVP_KEY_TYPE_NONE )
			{
				if( p_keyinfo->session_key_new.size > 0 )
				{
					TB_dbg_kcmvp("kcmvp encryption SESSION KEY - NeW\r\n" );
					p_key = &p_keyinfo->session_key_new;
				}
				else if( p_keyinfo->session_key_now.size > 0 )
				{
					TB_dbg_kcmvp("kcmvp encryption SESSION KEY - NoW\r\n" );
					p_key = &p_keyinfo->session_key_now;
				}
				else
				{
					TB_dbg_kcmvp("kcmvp encryption MASTER KEY\r\n" );
					p_key = &p_keyinfo->master_key;
				}
			}
			else
			{
				if( s_key_type == KCMVP_KEY_TYPE_SESSION_NEW )
				{
					if( p_keyinfo->session_key_new.size > 0 )
					{
						TB_dbg_kcmvp("kcmvp encryption SESSION KEY - NeW\r\n" );
						p_key = &p_keyinfo->session_key_new;
					}
					else
					{
						TB_dbg_kcmvp("ERROR. kcmvp encryption MASTER KEY\r\n" );
					}
				}
				else if( s_key_type == KCMVP_KEY_TYPE_SESSION_NOW )
				{
					if( p_keyinfo->session_key_now.size > 0 )
					{
						TB_dbg_kcmvp("kcmvp encryption SESSION KEY - NoW\r\n" );
						p_key = &p_keyinfo->session_key_now;
					}
					else
					{
						TB_dbg_kcmvp("ERROR. kcmvp encryption SESSION_NOW KEY\r\n" );
					}
				}
				else
				{
					if( p_keyinfo->master_key.size > 0 )
					{
						TB_dbg_kcmvp("kcmvp encryption MASTER KEY\r\n" );
						p_key = &p_keyinfo->master_key;
					}
					else
					{
						TB_dbg_kcmvp("ERROR. kcmvp encryption MASTER KEY\r\n" );
					}
				}
			}

			////////////////////////////////////////////////////////////////////

			s_key_type = KCMVP_KEY_TYPE_NONE;

			////////////////////////////////////////////////////////////////////

			if( p_key && p_key->size > 0 )
			{
				TBUI kcmvp_ret = KDN_BC_Encrypt( p_dest, p_dest_len,	\
												 p_sour, sour_len, 		\
												 p_key->data, p_key->size,					\
												 KDN_BC_Algo_ARIA_GCM, 						\
												 s_keyinfo.iv.data, s_keyinfo.iv.size, 		\
												 s_keyinfo.auth.data, s_keyinfo.auth.size, 	\
												 s_keyinfo.tag.data, s_keyinfo.tag.size );
				if( kcmvp_ret == KDN_OK )
				{
					TB_led_dig_del_critical();
					ret = 0;
				}
				else
				{
					TB_dbg_kcmvp("Error. Encryption : %d\n", kcmvp_ret );
					TB_led_dig_add_critical();
				}
			}
			else
			{
				TB_dbg_kcmvp("Error. Encryption : There is no encryption key.\n" );
				TB_led_dig_add_critical();
			}
		}

	}

	return ret;
}

int TB_kcmvp_encryption( TB_KCMVP_DATA *p_plaintext, TB_KCMVP_DATA *p_ciphertext )
{
	int ret = -1;

	if( p_plaintext && p_ciphertext )
	{
		ret = tb_kcmvp_encryption( p_plaintext->data, p_plaintext->size,	\
								   &p_ciphertext->data[0], &p_ciphertext->size );
	}
	
	return ret;
}

#define DECRYPTION_TRY_CNT	3

static TBUC s_decryption_err_cnt = 0;
void TB_kcmvp_decryption_error_count_clear( void )
{
	s_decryption_err_cnt = 0;
}

void TB_kcmvp_decryption_error_count_inc( void )
{
	s_decryption_err_cnt ++;

	TB_dbg_kcmvp("=============================\r\n" );
	TB_dbg_kcmvp(" Error. Decryption : %d\r\n", s_decryption_err_cnt );
	TB_dbg_kcmvp("=============================\r\n" );

	if( s_decryption_err_cnt >= DECRYPTION_TRY_CNT )
	{
		//	20211209
		if( TB_setup_get_product_info_ems_use() )
		{
			/********************************
			*	request a new session key
			*********************************/
			TB_ems_set_auth_status( EMS_AUTH_STATE_RETRY );
		}
		TB_kcmvp_decryption_error_count_clear();
	}
}

int TB_kcmvp_decryption( TB_KCMVP_DATA *p_ciphertext, TB_KCMVP_DATA *p_plaintext )
{
	int ret = -1;

	if( p_ciphertext && p_plaintext )
	{
		TB_KCMVP_INFO *p_keyinfo = TB_kcmvp_get_keyinfo();
		if( p_keyinfo )
		{
			TBUI kcmvp_ret = UNKNOWN_ERROR;
			TB_KCMVP_KEY *p_key = NULL;
			if( p_keyinfo->session_key_new.size > 0 )
			{
				p_key = &p_keyinfo->session_key_new;
				kcmvp_ret = KDN_BC_Decrypt( p_plaintext->data, &p_plaintext->size,		\
											 p_ciphertext->data, p_ciphertext->size,	\
											 p_key->data, p_key->size,					\
											 KDN_BC_Algo_ARIA_GCM, 						\
											 s_keyinfo.iv.data, s_keyinfo.iv.size, 		\
											 s_keyinfo.auth.data, s_keyinfo.auth.size, 	\
											 s_keyinfo.tag.data, s_keyinfo.tag.size );

				if( kcmvp_ret == KDN_OK )
				{
					TB_dbg_ems("kcmvp decryption SESSION KEY - NEW\r\n" );
				}
			}

			if( kcmvp_ret != KDN_OK )
			{			
				if( p_keyinfo->session_key_now.size > 0 )
				{
					p_key = &p_keyinfo->session_key_now;
					kcmvp_ret = KDN_BC_Decrypt( p_plaintext->data, &p_plaintext->size,		\
												 p_ciphertext->data, p_ciphertext->size,	\
												 p_key->data, p_key->size,					\
												 KDN_BC_Algo_ARIA_GCM, 						\
												 s_keyinfo.iv.data, s_keyinfo.iv.size, 		\
												 s_keyinfo.auth.data, s_keyinfo.auth.size, 	\
												 s_keyinfo.tag.data, s_keyinfo.tag.size );
					if( kcmvp_ret == KDN_OK )
					{
						TB_dbg_kcmvp("kcmvp decryption SESSION KEY - NoW\r\n" );
					}
				}
			}

			if( kcmvp_ret != KDN_OK )
			{
				if( p_keyinfo->master_key.size > 0 )
				{
					p_key = &p_keyinfo->master_key;
					kcmvp_ret = KDN_BC_Decrypt( p_plaintext->data, &p_plaintext->size,		\
												 p_ciphertext->data, p_ciphertext->size,	\
												 p_key->data, p_key->size,					\
												 KDN_BC_Algo_ARIA_GCM, 						\
												 s_keyinfo.iv.data, s_keyinfo.iv.size, 		\
												 s_keyinfo.auth.data, s_keyinfo.auth.size, 	\
												 s_keyinfo.tag.data, s_keyinfo.tag.size );
					if( kcmvp_ret == KDN_OK )
					{
						TB_dbg_kcmvp("kcmvp decryption MASTER KEY\r\n" );
					}
				}
			}

			////////////////////////////////////////////////////////////////////

			if( kcmvp_ret == KDN_OK )
			{
				TB_led_dig_del_critical();
				TB_kcmvp_decryption_error_count_clear();
				ret = 0;
			}
			else
			{
				TB_led_dig_add_critical();
				TB_kcmvp_decryption_error_count_inc();

#if 0
				printf("*** SESSION NEW : ");TB_util_data_dump2( p_keyinfo->session_key_new.data, p_keyinfo->session_key_new.size );
				printf("*** SESSION NOW : ");TB_util_data_dump2( p_keyinfo->session_key_now.data, p_keyinfo->session_key_now.size );
				printf("*** MASTER      : ");TB_util_data_dump2( p_keyinfo->master_key.data, p_keyinfo->master_key.size );
#endif
			}
		}
	}

	return ret;
}

