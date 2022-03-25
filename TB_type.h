#ifndef __KDN_TYPE_H__
#define __KDN_TYPE_H__

#include "TB_base.h"

#ifndef	TRUE
#define TRUE 			1
#endif

#ifndef	FALSE
#define FALSE 			0
#endif

////////////////////////////////////////////////////////////////////////////////

typedef signed char			TBSC;
typedef unsigned char		TBUC;
typedef signed short int	TBSS;
typedef unsigned short int	TBUS;
typedef signed int			TBSI;
typedef unsigned int		TBUI;
typedef signed long			TBSL;
typedef unsigned long		TBUL;
typedef signed long long	TBSLL;
typedef unsigned long long	TBULL;
typedef double				TBDBL;
typedef float				TBFLT;
typedef void				TBVD;
typedef unsigned char		TBBL;

////////////////////////////////////////////////////////////////////////////////

typedef struct tagUI_POINT
{
	TBUC x;
	TBUC y;
} TB_UI_POINT;

typedef struct tagUI_RECT
{
	TBUC x;
	TBUC y;
	TBUC w;
	TBUC h;
} TB_UI_RECT;

typedef struct tagDATE
{
	TBUS	year;
	TBUC	month;
	TBUC	day;
} TB_DATE;

typedef struct tagTIME
{
	TBUC	hour;
	TBUC	min;
	TBUC	sec;
} TB_TIME;

typedef struct tagDATE_TIME
{
	TBUS	year;
	TBUC	month;
	TBUC	day;
	TBUC	hour;
	TBUC	min;
	TBUC	sec;
} TB_DATE_TIME;

typedef struct tagBUF1024
{
	TBUC buffer[1024];
	TBUI length;
} TB_BUF1024;

typedef struct tagBUF512
{
	TBUC buffer[512];
	TBUI length;
} TB_BUF512;

typedef struct tagBUF128
{
	TBUC buffer[128];
	TBUI length;
} TB_BUF128;

#endif//__KDN_TYPE_H__

