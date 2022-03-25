#ifndef	__TB_AES_EVP_H__
#define __TB_AES_EVP_H__

#include "TB_type.h"

#define AES_BUF_SIZE		1024
#define AES_BLOCK_SIZE 		16
#define AES_IV_SIZE 		16
#define AES_KEY_SIZE		32
#define AES_KEY_BIT 		256

////////////////////////////////////////////////////////////////////////////////

extern int TB_aes_evp_init( void );
extern size_t TB_aes_evp_encrypt( TBUC *p_in, size_t in_size, TBUC *p_out, size_t out_buf_size );
extern size_t TB_aes_evp_decrypt( TBUC *p_in, size_t in_size, TBUC *p_out, size_t out_buf_size );
extern size_t TB_aes_evp_encrypt_buf2file( const char *plain_text, size_t plain_size, const char *enc_path );
//extern size_t TB_aes_evp_decrypt_file2buf( const char *cipher_path, TBUC *p_out, TBUI out_size );
extern size_t __TB_aes_evp_decrypt_file2buf( const char *cipher_path, TBUC *p_out, TBUI out_buf_size, char *file, int line );
#define TB_aes_evp_decrypt_file2buf(c,p,s)	__TB_aes_evp_decrypt_file2buf(c,p,s,__FILE__, __LINE__)
extern TBBL TB_aes_evp_encrypt_file2file( const char *plain_path, const char *enc_path );
extern TBBL TB_aes_evp_decrypt_file2file( const char *cipher_path, const char *dec_path );

#endif//__TB_AES_EVP_H__

