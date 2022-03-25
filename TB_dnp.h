#ifndef	__TB_DNPP_H__
#define __TB_DNPP_H__

#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_DNPP_BUF	(2048)
#define CRC_SIZE		(2)

////////////////////////////////////////////////////////////////////////////////

typedef struct tagDNPP_RESPONSE
{
	TBUC 	mBuf[MAX_DNPP_BUF];
	TBUC 	mTmpBuf[MAX_DNPP_BUF];
	TBSI 	mWrite;
	TBSI 	mLength;
	TBBL 	mValidLength;
	TBBL 	mValidCrc;
	TBBL 	mSync;
	TBUS	mFilter;	//	0x0564

	/**********************************
	*		Function Pointers
	***********************************/
	void	(*reset)	(void);
	TBUC *	(*buffers)	(void);
	TBSI 	(*length)	(void);
	void 	(*filter)	(TBUC, TBUC);
	TBBL 	(*push)		(TBUC *, int);
} TB_DNPP_RESPONSE;

////////////////////////////////////////////////////////////////////////////////

extern TB_DNPP_RESPONSE g_dnpp_handle_t;

////////////////////////////////////////////////////////////////////////////////

extern TB_DNPP_RESPONSE *TB_dnpp_init( void );
extern void 	TB_dnpp_reset( void );
extern TBUC *	TB_dnpp_get_buffer( void );
extern TBSI 	TB_dnpp_get_length( void );
extern void 	TB_dnpp_set_filter( TBUC slave_id, TBUC function_code );
extern TBBL 	TB_dnpp_push( TBUC *buf, int len );

TBBL dnpp_response_crc( TBUC *buf );
TBUC dnpp_response_function_code( TBUC *buf );
TBUC dnpp_response_byte_count( TBUC *buf );

#endif	//__TB_DNPP_H__


