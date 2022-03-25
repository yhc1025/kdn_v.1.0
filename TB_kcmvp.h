#ifndef __TB_KCMVP_H__
#define __TB_KCMVP_H__

#include "TB_type.h"
#include "TB_debug.h"
#include "KDN_LIB.h"

////////////////////////////////////////////////////////////////////////////////

#define LEN_BYTE_KEY	16
#define LEN_BYTE_IV		16
#define LEN_BYTE_AUTH	14
#define LEN_BYTE_TAG	16

////////////////////////////////////////////////////////////////////////////////

typedef struct tagKCMVP_DATA
{
	TBUC 	data[1024];
	TBUI	size;
} TB_KCMVP_DATA;

typedef struct tagKCMVP_KEY
{
	TBUC 	data[LEN_BYTE_KEY];
	TBUI	size;
} TB_KCMVP_KEY;

typedef struct tagKCMVP_INFO
{
	TB_KCMVP_KEY master_key;	
	TB_KCMVP_KEY session_key_now;
	TB_KCMVP_KEY session_key_new;
	
	TB_KCMVP_KEY iv;
	TB_KCMVP_KEY auth;
	TB_KCMVP_KEY tag;
} TB_KCMVP_INFO;

typedef enum tagKEY_TYPE
{
	KCMVP_KEY_TYPE_NONE		= 0,
	KCMVP_KEY_TYPE_SESSION_NOW,
	KCMVP_KEY_TYPE_SESSION_NEW,
	KCMVP_KEY_TYPE_MASTER
} TB_KEY_TYPE;

////////////////////////////////////////////////////////////////////////////////

extern int TB_kcmvp_pre_init( void );
extern int TB_kcmvp_init( void );
extern int TB_kcmvp_key_check( void );
extern int TB_kcmvp_key_gen( TB_KCMVP_KEY *p_key );
extern TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_iv( void );
extern TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_auth( void );
extern TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_tag( void );
extern TB_KCMVP_KEY *TB_kcmvp_get_keyinfo_master_key( void );
extern TB_KCMVP_INFO *TB_kcmvp_get_keyinfo( void );
extern int TB_kcmvp_set_keyinfo_key_type( TB_KEY_TYPE key_type );
extern int TB_kcmvp_set_keyinfo_tag( TB_KCMVP_KEY *p_tag );
extern int TB_kcmvp_set_keyinfo_master_key( TB_KCMVP_KEY *p_key );
extern int TB_kcmvp_set_keyinfo_session_key( TB_KCMVP_KEY *p_key );
extern int TB_kcmvp_encryption( TB_KCMVP_DATA *p_plaintext, TB_KCMVP_DATA *p_ciphertext );
extern int TB_kcmvp_decryption( TB_KCMVP_DATA *p_ciphertext, TB_KCMVP_DATA *p_plaintext );
extern void TB_kcmvp_decryption_error_count_clear( void );
extern void TB_kcmvp_decryption_error_count_inc( void );

#endif//__TB_KCMVP_H__

