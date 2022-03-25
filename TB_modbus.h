#ifndef	__TB_MODBUS_H__
#define __TB_MODBUS_H__

#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_MODBUS_BUF	(1024)

#define CRC_SIZE		(2)

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	FUNC_READ_HOLDING_REG			= 0x03,
	FUNC_WRITE_SINGLE_REG			= 0x06,
	FUNC_WRITE_MULTIPLE_REG			= 0x10,
	FUNC_DOUBLE_READ_REC_REG		= 0x65,
	FUNC_DOUBLE_READ_REC_REG_IMMEDI	= 0x67,
} TB_MB_FUNC;

typedef struct tagMB_RESPONSE
{
	TBUC 	mBuf[MAX_MODBUS_BUF];
	TBUC 	mTmpBuf[MAX_MODBUS_BUF];
	TBSI 	mWrite;
	TBSI 	mLength;
	TBBL 	mValidLength;
	TBBL 	mValidCrc;
	TBBL 	mSync;
	struct
	{
		TBSI mSlaveId;
		TBSI mFunctionCode;
	} mFilter;

	/**********************************
	*		Function Pointers
	***********************************/
	void	(*reset)	(void);
	TBUC *	(*buffers)	(void);
	TBSI 	(*length)	(void);
	void 	(*filter)	(TBUC, TBUC);
	TBBL 	(*push)		(TBUC *, int);
} TB_MB_RESPONSE;

////////////////////////////////////////////////////////////////////////////////

extern TB_MB_RESPONSE g_mb_handle_t;

////////////////////////////////////////////////////////////////////////////////

extern TB_MB_RESPONSE *TB_modbus_init( void );
extern void 	TB_modbus_reset( void );
extern TBUC *	TB_modbus_get_buffer( void );
extern TBSI 	TB_modbus_get_length( void );
extern void 	TB_modbus_set_filter( TBUC slave_id, TBUC function_code );
extern TBBL 	TB_modbus_push( TBUC *buf, int len );

TBSI TB_modbus_make_command_read_holding_reg( TBUC *buf, TBUC slave_id, TBUS addr, TBUS num );
TBSI TB_modbus_make_command_write_single_reg( TBUC *buf, TBUC slave_id, TBUS addr, TBUS num );
TBSI TB_modbus_make_command_write_multiple_reg( TBUC *buf, TBUC slave_id, TBUS addr, TBUS num );
TBBL modbus_response_crc( TBUC *buf );
TBUC modbus_response_function_code( TBUC *buf );
TBUC modbus_response_byte_count( TBUC *buf );

void TB_modbus_print_FUNC_READ_HOLDING_REG( TBUC *p_data );

#endif	//__TB_MODBUS_H__

