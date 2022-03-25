#ifndef	__TB_TEST_ITEM_H__
#define __TB_TEST_ITEM_H__

#include "TB_type.h"

#define SUCCESS_WRITE	0x01
#define SUCCESS_READ	0x02

////////////////////////////////////////////////////////////////////////////////

#define TEST_ITEM_NONE		0x0000
#define TEST_ITEM_WISUN		0x0001
#define TEST_ITEM_RS485A	0x0002
#define TEST_ITEM_RS485B	0x0004
#define TEST_ITEM_MODEM		0x0008
#define TEST_ITEM_PSU_MOD	0x0010
#define TEST_ITEM_PSU_DNP	0x0020
#define TEST_ITEM_EMS		0x0040
#define TEST_ITEM_FRTU		0x0080
#define TEST_ITEM_ALL		(TEST_ITEM_WISUN 	|	\
							 TEST_ITEM_RS485A 	|	\
							 TEST_ITEM_RS485B 	|	\
							 TEST_ITEM_MODEM 	|	\
							 TEST_ITEM_PSU_MOD 	|	\
							 TEST_ITEM_PSU_DNP 	|	\
							 TEST_ITEM_EMS 		|	\
							 TEST_ITEM_FRTU)

////////////////////////////////////////////////////////////////////////////////

extern TBUI g_test_item;

extern TBBL TB_testmode_is_on( void );
extern void TB_testmode_set_on( void );
extern void TB_testmode_set_off( void );
extern void TB_testmode_set_test_result( TBUI test_item, TBBL normal );
extern int  TB_testmode_check( void );
extern int  TB_testmode_start( void );

#endif//__TB_TEST_ITEM_H__

