//#include <openssl/conf.h>
#include <openssl/evp.h>
//#include <openssl/err.h>

#include "TB_util.h"
#include "TB_crc.h"
#include "TB_aes_evp.h"

void tb_aes_evp_err_handle( char *file, int line )
{
    printf("from [%s:%d] critcal error\r\n", file, line);
}

////////////////////////////////////////////////////////////////////////////////

#define TYPE_AES_KEY	0
#define TYPE_AES_IV		1

////////////////////////////////////////////////////////////////////////////////

static TBUC 	s_lut_idx [32] = {0, };
static TBUC 	s_user_key[32] = {0, };
static TBUC 	s_aes_iv  [16] = {0, };

static const EVP_CIPHER *sp_cipher_type = NULL;

static TBUC *tb_aes_get_lut_idx( int type, unsigned int length )
{
	unsigned int i;
	unsigned int cnt = 0;
	unsigned int chk = 0;
	TBUC random;
	TBUI seed1 = 1;
	TBUI seed2 = 2;

	/***************************************************************************
	*			type 이나 length가 문제가 있으면 AES_KEY로 처리한다.
	***************************************************************************/
	if( type == TYPE_AES_KEY )
	{
		seed1 = 1;
		seed2 = 2;
	}
	else if( type == TYPE_AES_IV )
	{
		seed1 = 3;
		seed2 = 4;
	}
	else
	{
		printf("[%s:%d] ERROR. Invalid type : %d\r\n", __FILE__, __LINE__, type );
	}

	if( length % 2 != 0 )
	{
		printf("[%s:%d] ERROR. Invalid length : %d\r\n", __FILE__, __LINE__, length );
		length = 32;
	}

	////////////////////////////////////////////////////////////////////////////

	srand( seed1 );
	while( cnt < (length/2) )
	{
		random = (TBUC)TB_util_get_rand( 0, 255 );
		for( i=0; i<cnt; i++ )
		{
			if( s_lut_idx[i] == random )
			{
				chk = 1;
				break;
			}
		}

		if( chk == 0 )
		{
			s_lut_idx[cnt++] = random;
		}

		chk = 0;
	}

	srand( seed2 );
	while( cnt < length )
	{
		random = (TBUC)TB_util_get_rand( 0, 255 );
		for( i=0; i<cnt; i++ )
		{
			if( s_lut_idx[i] == random )
			{
				chk = 1;
				break;
			}
		}

		if( chk == 0 )
		{
			s_lut_idx[cnt++] = random;
		}

		chk = 0;
	}

	return s_lut_idx;
}

int TB_aes_evp_init( void )
{
	int ret = -1;
	
	TBUC *p_lut_idx = NULL;

	/***************************************************************************
	*						AES key를 생성한다.
	***************************************************************************/
	p_lut_idx = tb_aes_get_lut_idx( TYPE_AES_KEY, 32 );
	if( p_lut_idx )
	{
		for( int i=0; i<32; i++ )
		{
			TBUC idx = p_lut_idx[i];
			s_user_key[i] = ( i < (32/2) ) ? g_seed_table_high[idx] :	\
										 	 g_seed_table_low[idx];
		}
		
		ret = 0;
	}

	/***************************************************************************
	*						AES iv를 생성한다.
	***************************************************************************/
	p_lut_idx = tb_aes_get_lut_idx( TYPE_AES_IV, 16 );
	if( p_lut_idx )
	{
		for( int i=0; i<16; i++ )
		{
			TBUC idx = p_lut_idx[i];
			s_aes_iv[i] = ( i < (16/2) ) ? 	g_seed_table_high[idx] :	\
										 	g_seed_table_low[idx];
		}
		
		ret = 0;
	}

	sp_cipher_type = EVP_aes_256_cbc();

	return ret;
}

size_t TB_aes_evp_encrypt( TBUC *p_in, size_t in_size, TBUC *p_out, size_t out_buf_size )
{
	int result = -1;
	size_t sum = 0;
	if( p_in && p_out && in_size > 0 && out_buf_size >= in_size )
	{
	   	int rsz = 0, wsz = 0, tsz = 0;    
	    TBUC wbuf[AES_BUF_SIZE + EVP_MAX_BLOCK_LENGTH] = {0, };
	 
	    bzero( wbuf, sizeof(wbuf) );
	 
	    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
		if( ctx )
		{
			TBUC iv[16] = {0, };
			wmemcpy( iv, sizeof(iv), s_aes_iv, sizeof(s_aes_iv) );

			if( sp_cipher_type )
			{
			    if( EVP_EncryptInit_ex(ctx, sp_cipher_type, NULL, s_user_key, iv) == 1 )
			    {
					size_t offset = 0;
				    while( offset < in_size-1 )
				    {
						rsz = ( (in_size-offset) < AES_BUF_SIZE ) ? (in_size-offset) : AES_BUF_SIZE;
				        if( EVP_EncryptUpdate(ctx, wbuf, &wsz, &p_in[offset], rsz) == 1 )
				        {
							offset += rsz;
							if( wsz > 0 )
							{
							 	wmemcpy( &p_out[sum], out_buf_size-sum, wbuf, wsz );
								sum += wsz;
							}
							EVP_CIPHER_CTX_set_padding( ctx, 1 );
				        }
						else
						{
							sum = 0;
				            tb_aes_evp_err_handle(__FILE__,__LINE__);
				            break;
						}
				    }

				    if( EVP_EncryptFinal_ex(ctx, wbuf, &tsz) == 1 )
				    {		 	
						if( tsz > 0 )
						{
						 	wmemcpy( &p_out[sum], out_buf_size-sum, wbuf, tsz );
							sum += tsz;
						}					    

						result = 0;
				    }
					else
					{
				        tb_aes_evp_err_handle(__FILE__,__LINE__);
					}
			    }

				EVP_CIPHER_CTX_free( ctx );
			}
		}
	}

	if( result != 0 )
		sum = 0;
 
    return sum;
}

size_t TB_aes_evp_decrypt( TBUC *p_in, size_t in_size, TBUC *p_out, size_t out_buf_size )
{
	int result = -1;
	size_t sum = 0;

	if( p_in && p_out && in_size > 0 && out_buf_size >= in_size )
	{
	   	int rsz = 0, wsz = 0, tsz = 0;    
	    TBUC wbuf[AES_BUF_SIZE + EVP_MAX_BLOCK_LENGTH] = {0, };
	 
	    bzero( wbuf, sizeof(wbuf) );
	 
	    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
		if( ctx )
		{
			TBUC iv[16] = {0, };
			wmemcpy( iv, sizeof(iv), s_aes_iv, sizeof(s_aes_iv) );

			if( sp_cipher_type )
			{
			    if( EVP_DecryptInit_ex(ctx, sp_cipher_type, NULL, s_user_key, iv) == 1 )
			    {
					size_t offset = 0;				
				    while( offset < in_size-1 )
				    {
						rsz = ( (in_size-offset) < AES_BUF_SIZE ) ? (in_size-offset) : AES_BUF_SIZE;
				        if( EVP_DecryptUpdate(ctx, wbuf, &wsz, p_in, rsz) == 1 )
				        {
							offset += rsz;
							if( wsz > 0 )
							{
							 	wmemcpy( &p_out[sum], out_buf_size-sum, wbuf, wsz );
								sum += wsz;							
							}
							EVP_CIPHER_CTX_set_padding( ctx, 1 );
				        }
						else
						{
				            tb_aes_evp_err_handle(__FILE__,__LINE__);
				            break;
						}
				    }

				    if( EVP_DecryptFinal_ex(ctx, wbuf, &tsz) == 1 )
				    {
						if( tsz > 0 )
						{
						 	wmemcpy( &p_out[sum], out_buf_size-sum, wbuf, tsz );
							sum += tsz;
						}					    

						result = 0;
				    }
			    }
				else
				{
			        tb_aes_evp_err_handle(__FILE__,__LINE__);
				}
			}

			EVP_CIPHER_CTX_free( ctx );
		}
	}

	if( result != 0 )
		sum = 0;
 
    return sum;
}

size_t TB_aes_evp_encrypt_buf2file( const char *plain_text, size_t plain_size, const char *enc_path )
{
	size_t ret = 0;

	if( plain_text && plain_size > 0 && enc_path )
	{
	    int rsz = 0, wsz = 0, tsz = 0;
	    FILE *fp_out = NULL;
	    TBUC wbuf[AES_BUF_SIZE + EVP_MAX_BLOCK_LENGTH] = {0, };
	 
	    bzero( wbuf, sizeof(wbuf) );
	 
	    fp_out = fopen( enc_path, "wb" );
	    if( fp_out )
	    {
		    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
			if( ctx )
			{ 
				TBUC iv[16] = {0, };
				wmemcpy( iv, sizeof(iv), s_aes_iv, sizeof(s_aes_iv) );

				if( sp_cipher_type )
				{				
				    if( EVP_EncryptInit_ex(ctx, sp_cipher_type, NULL, s_user_key, iv) != 1 )
				    {
				        fclose( fp_out );
				        tb_aes_evp_err_handle(__FILE__,__LINE__);
				    }

				 	size_t sum = 0;
				    while( sum < plain_size )
				    {
						rsz = ( (plain_size-sum) < AES_BUF_SIZE ) ? (plain_size-sum) : AES_BUF_SIZE;
				        if( EVP_EncryptUpdate(ctx, wbuf, &wsz, &plain_text[sum], rsz) != 1 )
				        {
				            fclose( fp_out );
				            tb_aes_evp_err_handle(__FILE__,__LINE__);
				            break;
				        }
						EVP_CIPHER_CTX_set_padding( ctx, 1 );

				        fwrite( wbuf, 1, wsz, fp_out );
						sum += rsz;
				    }		
				 
				    if( EVP_EncryptFinal_ex(ctx, wbuf + wsz, &tsz) != 1 )
				    {
				        fclose( fp_out );
				        tb_aes_evp_err_handle(__FILE__,__LINE__);
				    }

				    fwrite( wbuf + wsz, 1, tsz, fp_out );
					sum += tsz;

				 	fflush( fp_out );
				    fclose( fp_out );			    

					ret = sum;					
				}

				EVP_CIPHER_CTX_free( ctx );
			}
		}
		else
		{
	        printf("[%s:%d]write file open fail\r\n", __FILE__, __LINE__);
		}
	}
 
    return ret;
}

//size_t TB_aes_evp_decrypt_file2buf( const char *cipher_path, TBUC *p_out, TBUI out_buf_size )
size_t __TB_aes_evp_decrypt_file2buf( const char *cipher_path, TBUC *p_out, TBUI out_buf_size, char *file, int line )
{
	size_t ret = 0;

	if( file )
		printf("[%s] call from [%s:%d]\r\n", __FUNCTION__, file, line );

	if( cipher_path && p_out && out_buf_size > 0 )
	{	
	    int rsz = 0, wsz = 0, tsz = 0;
	    FILE *fp_in = NULL;

		printf( "[%s:%d][%s] input file = %s\r\n", __FILE__, __LINE__, __FUNCTION__, cipher_path );
	 
	    fp_in = fopen(cipher_path, "rb");
	    if( fp_in )
	    {
		    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
			if( ctx )
			{
			    TBUC rbuf[AES_BUF_SIZE] = {0, };
			    TBUC wbuf[AES_BUF_SIZE + EVP_MAX_BLOCK_LENGTH] = {0, };
			 
				TBUC iv[16] = {0, };
				wmemcpy( iv, sizeof(iv), s_aes_iv, sizeof(s_aes_iv) );

				if( sp_cipher_type )
				{
				    if( EVP_DecryptInit_ex(ctx, sp_cipher_type, NULL, s_user_key, iv) != 1 )
				    {
				        fclose( fp_in );
				        tb_aes_evp_err_handle(__FILE__,__LINE__);
				    }

				    fseek( fp_in, 0, SEEK_END );
				    size_t total_size = ftell( fp_in );
				    fseek( fp_in, 0, SEEK_SET );

					printf( "[%s:%d][%s] input filesize = %d\r\n", __FILE__, __LINE__, __FUNCTION__, total_size );

				 	size_t sum = 0;
					bzero( rbuf, sizeof(rbuf) );
				    while( 1 )
				    {
						rsz = fread( rbuf, 1, sizeof(rbuf)-1, fp_in );
						if( rsz > 0 )
						{
							rbuf[rsz] = '\0';
					        if( EVP_DecryptUpdate(ctx, wbuf, &wsz, rbuf, rsz) != 1 )
					        {
					            tb_aes_evp_err_handle(__FILE__,__LINE__);
					            break;
					        }
							EVP_CIPHER_CTX_set_padding( ctx, 1 );

							if( wsz > 0 )
							{
						 		wmemcpy( &p_out[sum], out_buf_size-sum, wbuf, wsz );
								sum += wsz;
							}
						}
						else
						{
							break;
						}
					}
				 	
				    if( EVP_DecryptFinal_ex(ctx, wbuf + wsz, &tsz) != 1 )
				    {
				        tb_aes_evp_err_handle(__FILE__,__LINE__);
				    }

					if( tsz > 0 )
					{
						wmemcpy( &p_out[sum], out_buf_size-sum, wbuf + wsz, tsz );
						sum += tsz;
					}

					ret = sum;
				}

				EVP_CIPHER_CTX_free( ctx );
			} 

		    fclose( fp_in );
	    }
		else
		{
	        printf("[%s:%d]read file open fail\r\n", __FILE__, __LINE__);
		}
	}
 
    return ret;
}
 
TBBL TB_aes_evp_encrypt_file2file(const char *plain_path, const char *enc_path)
{
    int rsz = 0, wsz = 0, tsz = 0;
    FILE *fp_in = NULL;
	FILE *fp_out = NULL;
 
    fp_in = fopen(plain_path, "rb");
    if( fp_in == NULL )
    {
        printf("[%s:%d]read file open fail\r\n", __FILE__, __LINE__);
        return FALSE;
    }
	else
	{
	    fseek( fp_in, 0, SEEK_END );
	    size_t total_size = ftell( fp_in );
	    fseek( fp_in, 0, SEEK_SET );

		printf( "[%s:%d][%s] input filesize = %d\r\n", __FILE__, __LINE__, __FUNCTION__, total_size );
	}
 
    fp_out = fopen( enc_path, "wb" );
    if( fp_out == NULL )
    {
        printf("[%s:%d]write file open fail\r\n", __FILE__, __LINE__);
        return FALSE;
    }
 
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if( ctx )
	{
	    TBUC rbuf[AES_BUF_SIZE] = {0, };
	    TBUC wbuf[AES_BUF_SIZE + EVP_MAX_BLOCK_LENGTH] = {0, };	    
		
		TBUC iv[16] = {0, };
		wmemcpy( iv, sizeof(iv), s_aes_iv, sizeof(s_aes_iv) );
	    if( EVP_EncryptInit_ex(ctx, sp_cipher_type, NULL, s_user_key, iv) != 1 )
	    {
	        fclose( fp_in );
	        fclose( fp_out );
	        tb_aes_evp_err_handle( __FILE__,__LINE__ );
	    }

	 	bzero( rbuf, sizeof(rbuf) );
		bzero( wbuf, sizeof(wbuf) );
	    while( 1 )
	    {
			rsz = fread( rbuf, 1, sizeof(rbuf)-1, fp_in );
			if( rsz > 0 )
			{
				rbuf[rsz] = '\0';
				if( EVP_EncryptUpdate(ctx, wbuf, &wsz, rbuf, rsz) != 1 )
		        {
		            fclose( fp_in );
		            fclose( fp_out );
		            tb_aes_evp_err_handle(__FILE__,__LINE__);
		            break;
		        }
				EVP_CIPHER_CTX_set_padding( ctx, 1 );

		 		fwrite( wbuf, 1, wsz, fp_out );
			}
			else
			{
				break;
			}
	    }

	    if( EVP_EncryptFinal_ex(ctx, wbuf + wsz, &tsz) != 1 )
	    {
	        fclose( fp_in );
	        fclose( fp_out );
	        tb_aes_evp_err_handle(__FILE__,__LINE__);
	    }
	 
	    fwrite( wbuf + wsz, 1, tsz, fp_out );

	    fseek( fp_out, 0, SEEK_END );
	    size_t total_size = ftell( fp_out );

	    fclose( fp_in );
	    fflush( fp_out );
	    fclose( fp_out );
	    EVP_CIPHER_CTX_free( ctx );
	}
 
    return TRUE;   
}
 
TBBL TB_aes_evp_decrypt_file2file(const char *cipher_path, const char *plain_path)
{
    int rsz = 0, wsz = 0, tsz = 0;
    FILE *fp_in = NULL;
	FILE *fp_out = NULL;
 
    fp_in = fopen(cipher_path, "rb");
    if( fp_in == NULL )
    {
        printf("[%s:%d]read file open fail\r\n", __FILE__, __LINE__);
        return FALSE;
    }
 
    fp_out = fopen(plain_path, "wb");
    if( fp_out == NULL )
    {
        printf("[%s:%d]write file open fail\r\n", __FILE__, __LINE__);
        return FALSE;
    }
 
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if( ctx )
	{
	    TBUC rbuf[AES_BUF_SIZE] = {0, };
	    TBUC wbuf[AES_BUF_SIZE + EVP_MAX_BLOCK_LENGTH] = {0, };
				
		TBUC iv[16] = {0, };
		wmemcpy( iv, sizeof(iv), s_aes_iv, sizeof(s_aes_iv) );

		if( sp_cipher_type )
		{
		    if( EVP_DecryptInit_ex(ctx, sp_cipher_type, NULL, s_user_key, iv) != 1 )
		    {
		        fclose( fp_in );
		        fclose( fp_out );
		        tb_aes_evp_err_handle(__FILE__,__LINE__);
		    }
		}

	    bzero( rbuf, sizeof(rbuf) );
	    bzero( wbuf, sizeof(wbuf) );
	    while( 1 )
	    {
			rsz = fread( rbuf, 1, sizeof(rbuf)-1, fp_in );
			if( rsz > 0 )
			{
				rbuf[rsz] = '\0';
		        if( EVP_DecryptUpdate(ctx, wbuf, &wsz, rbuf, rsz) != 1 )
		        {
		            fclose( fp_in );
		            fclose( fp_out );
		            tb_aes_evp_err_handle(__FILE__,__LINE__);
		            break;
		        }

				EVP_CIPHER_CTX_set_padding( ctx, 1 );	 
		        fwrite(wbuf, 1, wsz, fp_out);
			}
			else
			{
				break;
			}
	    }
		
	    if( EVP_DecryptFinal_ex(ctx, wbuf + wsz, &tsz) != 1 )
	    {
	        fclose( fp_in );
	        fclose( fp_out );
	        tb_aes_evp_err_handle(__FILE__,__LINE__);
	    }

	 	printf("[%s:%d]tsz = %d\r\n", __FILE__, __LINE__, tsz);
	    fwrite(wbuf + wsz, 1, tsz, fp_out);
	 
	    fclose( fp_in );
	    fflush( fp_out );
	    fclose( fp_out );
	    EVP_CIPHER_CTX_free( ctx );
	}

	printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
 
    return TRUE;
}

/* 
int main()
{
    TBUC *key = (TBUC *)"31ACBFE93F0380049CFE9B236641E42E28plain_pathE05DDA5820A07D52CCB881C3308D";
    TBUC *iv = (TBUC *)"00F037907D1DA656647B474DADE9C52C";
 
    encrypt_aes_evp("test.txt", "encrypted.ipk", key, iv);
    decrypt_aes_evp("encrypted.ipk", "test1.txt", key, iv);
 
    return 0;
}
*/
