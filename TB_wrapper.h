#ifndef	__TB_WRAPPER_H__
#define __TB_WRAPPER_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>

#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

extern int 		wstrrev		( char *begin, char *end );
extern void 	witoa			( int value, char *str, int base );
extern int 		watoi			( const char *nptr );
extern int 	 	wmemcmp	( const void *s1, size_t s1_size, const void *s2, size_t s2_size, size_t cmp_size );
extern void * __wmemcpy	( void *dest, size_t dest_size, const void *src, size_t count, char *file, int line );
extern void * __wmemset		( void *dest, size_t dest_size, int c, size_t set_size, char *file, int line );
extern size_t __wstrlen		( const char *p_str, char *file, int line );
extern char * __wstrcpy		( char *dest, size_t dest_size, const char *sour, char *file, int line );
extern char * __wstrncpy		( char *dest, size_t dest_size, const char *sour, size_t sour_size, char *file, int line );
extern char * __wstrcat		( char *dest, size_t dest_size, const char *sour, char *file, int line );
extern char * __wstrncat		( char *dest, size_t dest_size, const char *sour, size_t sour_size, char *file, int line );
extern int 		wstrtok		( char *sour, char *token );
extern int 		wstrtol			( const char *nptr, int base );
extern int 		wstrtol_8		( const char *nptr );
extern int 		wstrtol_10	( const char *nptr );
extern int 		wstrtol_16	( const char *nptr );

#define wmemcpy(d,ds,s,ss)	__wmemcpy((void *)d,(size_t)ds,(const void *)s,(size_t)ss,(char *)__FILE__,(int)__LINE__)
#define wmemset(d,ds,c,cs)	__wmemset((void *)d,(size_t)ds,(char)c,(size_t)cs,(char *)__FILE__,(int)__LINE__)
#define wstrlen(p)			__wstrlen((const char *)p,(char *)__FILE__,(int)__LINE__)
#define wstrcpy(d,ds,s)		__wstrcpy((char *)d,(size_t)ds,(const char *)s,(char *)__FILE__,(int)__LINE__)
#define wstrncpy(d,ds,s,ss)	__wstrncpy((char *)d,(size_t)ds,(const char *)s,(size_t)ss,(char *)__FILE__,(int)__LINE__)
#define wstrcat(d,ds,s)		__wstrcat((char *)d,(size_t)ds,(const char *)s,(char *)__FILE__,(int)__LINE__)
#define wstrncat(d,ds,s,ss)	__wstrncat((char *)d,(size_t)ds,(const char *)s,(size_t)ss,(char *)__FILE__,(int)__LINE__)

#endif//__TB_WRAPPER_H__

