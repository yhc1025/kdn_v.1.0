#include "TB_wrapper.h"

//void *wmemcpy( void *dest, size_t dest_size, const void *sour, size_t sour_size )
void *__wmemcpy( void *dest, size_t dest_size, const void *sour, size_t sour_size, char *file, int line )
{
	void *ret = NULL;
	
	if( dest && sour )
	{
		if( dest != sour )
		{
			if( sour_size > 0 && dest_size > 0 && dest_size >= sour_size )
			{
			    char *tmp 	  = (char *)dest;
			    const char *s = sour;

			    while( sour_size -- )
			    {
			    	*tmp++ = *s++;
			    }
				
			    ret = dest;
			}
			else
			{
#ifdef WAPPER_DEBUG
				if( file )
				{
					printf("[%s] wrapper : call from [%s:%d] : (%d : %d)\r\n", __FUNCTION__, file, line, dest_size, sour_size );
				}
				else
				{
					printf("[%s] wrapper : (%d : %d)\r\n", __FUNCTION__, dest_size, sour_size );
				}
#endif
			}
		}
	}

	return ret;
}

//void *wmemset( void *dest, size_t dest_size, int c, size_t set_size )
void *__wmemset( void *dest, size_t dest_size, int c, size_t set_size, char *file, int line )
{
	void *ret = NULL;
	
	if( dest )
	{
		if( set_size > 0 && dest_size > 0 && dest_size >= set_size )
		{
		    char *tmp = (char *)dest;

		    while( set_size -- )
		    {
		    	*tmp++ = (char)c;
		    }
			
		    ret = dest;
		}
		else
		{
#ifdef WAPPER_DEBUG
			if( file )
			{
				printf("[%s] wrapper : call from [%s:%d] : (%d : %d)\r\n", __FUNCTION__, file, line, dest_size, set_size );
			}
			else
			{
				printf("[%s] wrapper : (%d : %d)\r\n", __FUNCTION__, dest_size, set_size );
			}
#endif
		}
	}

	return ret;
}

//size_t wstrlen( const char *p_str )
size_t __wstrlen( const char *p_str, char *file, int line )
{
	size_t size = 0;
	if( p_str )
	{ 
	    while( p_str[size] != '\0' )
	        size++;
	}
	else
	{
#ifdef WAPPER_DEBUG
		if( file )
			printf("[%s] wrapper : call from [%s:%d] sour is NULL\r\n", __FUNCTION__, file, line );
		else
			printf("[%s] wrapper : sour is NULL\r\n", __FUNCTION__ );
#endif
	}
 
    return size;
}

//char *wstrcpy( char *dest, size_t dest_size, const char *sour )
char *__wstrcpy( char *dest, size_t dest_size, const char *sour, char *file, int line )
{
	char *tmp = NULL;
	if( dest && sour )
	{
		tmp = dest;

		size_t sour_size = wstrlen( sour );
		if( dest_size > 0 && sour_size > 0 )
		{		
			/* dest의 여유크기가 sour_size보다 크다면 */		
			if( dest_size > sour_size )
			{
				while( (*dest++ = *sour++) != '\0')
				{
				}
			}
			else
			{
#ifdef WAPPER_DEBUG
				if( file )
					printf("[%s] wrapper : call from [%s:%d] (%d : %d)\r\n", __FUNCTION__, file, line, dest_size, sour_size );
				else
					printf("[%s] wrapper : (%d : %d)\r\n", __FUNCTION__, dest_size, sour_size );
#endif
			}
		}
	}

	return tmp;
}

//char *wstrncpy( char *dest, size_t dest_size, const char *sour, size_t sour_size )
char *__wstrncpy( char *dest, size_t dest_size, const char *sour, size_t sour_size, char *file, int line )
{
	char *tmp = NULL;

	if( dest && sour )
	{
		tmp = dest;
		if( dest_size > 0 && sour_size > 0 )
		{
			if( dest_size > sour_size )
			{
				while( (*dest++ = *sour++) != 0 )
				{
					if( --sour_size == 0 )
					{
						*dest = '\0';
						break;
					}
				}
			}
			else
			{
#ifdef WAPPER_DEBUG
				if( file )
					printf("[%s] wrapper : call from [%s:%d] (%d : %d)\r\n", __FUNCTION__, file, line, dest_size, sour_size );
				else
					printf("[%s] wrapper : (%d : %d)\r\n", __FUNCTION__, dest_size, sour_size );
#endif
			}
		}
	}
	return tmp;
}

//char *wstrcat( char *dest, size_t dest_size, const char *sour )
char *__wstrcat( char *dest, size_t dest_size, const char *sour, char *file, int line )
{
	char *tmp = NULL;
	if( dest && sour )
	{
		tmp = dest;

		size_t sour_size = wstrlen( sour );
		if( dest_size > 0 && sour_size > 0 )
		{		
			/* dest 문자열의 null byte 위치(문자열 끝)를 찾는다. */
			while( *dest != '\0' )
				dest++;

			/* dest의 여유크기가 sour_size보다 크다면 */		
			if( (dest_size - (dest - tmp)) > sour_size )
			{
				/*
				* dest 문자열 끝부터 src 문자열을 계속 복사합니다.
				* src 문자열의 마지막 null byte까지 복사한 후,
				* 루프문을 종료합니다.
				*/
				while( (*dest++ = *sour++) != '\0')
				{
				}
			}
			else
			{
#ifdef WAPPER_DEBUG
				if( file )
					printf("[%s] wrapper : call from [%s:%d] (%d : %d : %d)\r\n", __FUNCTION__, file, line, (size_t)(dest - tmp), dest_size, sour_size );
				else
					printf("[%s] wrapper : (%d : %d : %d)\r\n", __FUNCTION__, (size_t)(dest - tmp), dest_size, sour_size );
#endif
			}
		}
	}

	return tmp;
}

//char *wstrncat( char *dest, size_t dest_size, const char *sour, size_t sour_size )
char *__wstrncat( char *dest, size_t dest_size, const char *sour, size_t sour_size, char *file, int line )
{
	char *tmp = NULL;

	if( dest && sour )
	{
		tmp = dest;
		if( dest_size > 0 && sour_size > 0 )
		{
			while( *dest != '\0' )
				dest++;

			if( (dest_size - (dest - tmp)) > sour_size )
			{
				while( (*dest++ = *sour++) != 0 )
				{
					if( --sour_size == 0 )
					{
						*dest = '\0';
						break;
					}
				}
			}
			else
			{
#ifdef WAPPER_DEBUG
				if( file )
					printf("[%s] wrapper : call from [%s:%d] (%d : %d : %d)\r\n", __FUNCTION__, file, line, (size_t)(dest - tmp), dest_size, sour_size );
				else
					printf("[%s] wrapper : (%d : %d : %d)\r\n", __FUNCTION__, (size_t)(dest - tmp), dest_size, sour_size );
#endif
			}
		}
	}
	return tmp;
}

int wstrtok( char *sour, char *token )
{
	int offset = -1;
	if( sour && token )
	{
		int token_len = wstrlen( token );
		int sour_len  = wstrlen( sour );
		if( sour_len > 0 && token_len > 0 )
		{
			/*******************************************************************
			*	문자열 마지막 '\0'을 검사하기 위하여 length까지 검사한다.
			*******************************************************************/
			for( int i=0; i<=sour_len; i++ )
			{
				for( int j=0; j<token_len; j++ )
				{
					if( sour[i] == token[j] || sour[i] == '\0' )
					{
						offset = i;
						break;
					}
				}

				if( offset != -1 )
					break;
			}
		}
	}

	return offset;
}

int wstrtol( const char *nptr, int base )
{
	int si = -1;
	
	if( nptr )
	{
		char *end_ptr = NULL;
		long sl = 0;

		errno = 0;
		sl = strtol( nptr, &end_ptr, base );
#ifdef WAPPER_DEBUG
		if( ERANGE == errno )
		{
			printf("[%s] wrapper : number out of range\r\n", __FUNCTION__ );
		}
		else if( sl > INT_MAX )
		{
			printf("[%s] wrapper : %ld too large\r\n", __FUNCTION__, sl );
		}
		else if( sl < INT_MIN )
		{
			printf("[%s] wrapper : %ld too small\r\n", __FUNCTION__, sl );
		}
		else if( end_ptr == nptr )
		{
			printf("[%s] wrapper : invalid numeric input\r\n", __FUNCTION__ );
		}
		else if( *end_ptr != '\0' )
		{
			printf("[%s] wrapper : extra characters on input line\r\n", __FUNCTION__ );
		}
		else
#endif
		{
			si = (int)sl;
		}
	}
#ifdef WAPPER_DEBUG
	else
	{
		printf("[%s] wrapper : input nptr is NULL\r\n", __FUNCTION__ );
	}
#endif

	return si;

}

int wstrtol_8( const char *nptr )		//	string to long by base 8
{
	return wstrtol( nptr, 8 );
}

int wstrtol_10( const char *nptr )	//	string to long by base 10
{
	return wstrtol( nptr, 10 );
}

int wstrtol_16( const char *nptr )	//	string to long by base 16
{
	return wstrtol( nptr, 16 );
}

int watoi( const char *nptr )
{
	return wstrtol( nptr, 10 );
}

int wstrrev( char* p_begin, char* p_end )
{
	int ret = -1;
    char aux;

	if( p_begin && p_end )
	{
	    while( p_end > p_begin )
	    {
	        aux        = *p_end;
	        *p_end--   = *p_begin;
	        *p_begin++ = aux;
	    }
		
		ret = 0;
	}

	return ret;
}

void witoa( int value, char* str, int base )
{
    static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* wstr = str;

    int sign;

    // Validate base
    if( base < 2 || base > 35 )
	{
		*wstr='\0';
		return;
	}

    // Take care of sign
    if( (sign=value) < 0 )
		value = -value;

    // Conversion. Number is reversed.
    do
    {
		*wstr++ = num[value % base];
    } while( value /= base );

    if( sign < 0 )
		*wstr++='-';

    *wstr='\0';

    // Reverse string
    wstrrev( str, wstr-1 );
}

