#include <unistd.h>

#include "TB_uart.h"
#include "TB_inverter.h"
#include "TB_modem.h"
#include "TB_psu.h"
#include "TB_ems.h"
#include "TB_frtu.h"
#include "TB_j11.h"
#include "TB_util.h"
#include "TB_sys_gpio.h"
#include "TB_sys_wdt.h"
#include "TB_led.h"
#include "TB_test.h"

TBUI g_test_item = TEST_ITEM_NONE;
TBBL g_testmode = FALSE;

#define DEF_TESTMODE_FILE	"/mnt/testmode.dat"
TBBL TB_testmode_is_on( void )
{
	return g_testmode;
}

void TB_testmode_set_on( void )
{
	g_testmode = TRUE;
}

void TB_testmode_set_off( void )
{
	g_testmode = FALSE;
}

int TB_testmode_check( void )
{
	int check = TB_gpio_get( GPIO_TYPE_LTE_PWR_EN );	
	if( check == 1 )
	{
		TB_testmode_set_on();
		
		TB_gpio_deinit();	
		TB_gpio_init();
		
		TB_testmode_start();
	}

	TB_testmode_set_off();

	return check;
}

void TB_testmode_set_test_result( TBUI test_item, TBBL normal )
{
	if( normal == FALSE )
	{
		g_test_item &= ~test_item;
	}
	else
	{
		g_test_item |= test_item;
	}
}

pthread_t 	g_thid_test_mode;
int 		g_proc_flag_test_mode = 1;

static void *tb_test_mode_proc( void *arg )
{
	time_t t1 = time( NULL );
	time_t t2;
	time_t diff;
	while( g_proc_flag_test_mode )
	{
		t2 = time( NULL );
		diff = t2 - t1;
		if( diff >= 600 )	//	10 minute
		{
			break;
		}
		
		if( (g_test_item & TEST_ITEM_ALL) == TEST_ITEM_ALL )
		{
			break;
		}

		printf( "[%s:%d] TEST MODE : Elapse Time = [%d sec]\r\n", __FILE__, __LINE__, diff );

		if( (g_test_item & TEST_ITEM_WISUN) 	!= TEST_ITEM_WISUN )	printf( "     FAIL. WISUN\r\n" );		else printf( "     OK. WISUN\r\n" );
		if( (g_test_item & TEST_ITEM_RS485A) 	!= TEST_ITEM_RS485A )	printf( "     FAIL. RS485A\r\n" );		else printf( "     OK. RS485A\r\n" );
		if( (g_test_item & TEST_ITEM_RS485B) 	!= TEST_ITEM_RS485B )	printf( "     FAIL. RS485B\r\n" );		else printf( "     OK. RS485B\r\n" );
		if( (g_test_item & TEST_ITEM_MODEM) 	!= TEST_ITEM_MODEM )	printf( "     FAIL. MODEM\r\n" );		else printf( "     OK. MODEM\r\n" );
		if( (g_test_item & TEST_ITEM_PSU_MOD) 	!= TEST_ITEM_PSU_MOD )	printf( "     FAIL. PSU_MOD\r\n" );		else printf( "     OK. PSU_MOD\r\n" );
		if( (g_test_item & TEST_ITEM_PSU_DNP) 	!= TEST_ITEM_PSU_DNP )	printf( "     FAIL. PSU_DNP\r\n" );		else printf( "     OK. PSU_DNP\r\n" );
		if( (g_test_item & TEST_ITEM_EMS) 		!= TEST_ITEM_EMS )		printf( "     FAIL. EMS\r\n" );			else printf( "     OK. EMS\r\n" );
		if( (g_test_item & TEST_ITEM_FRTU) 		!= TEST_ITEM_FRTU )		printf( "     FAIL. FRTU\r\n" );		else printf( "     OK. FRTU\r\n" );

		TB_util_delay( 1000 );
	}

	TB_wisun_test_proc_stop();
	TB_rs485_test_proc_stop();
	TB_modem_test_proc_stop();
	TB_psu_mod_test_proc_stop();
	TB_psu_dnp_test_proc_stop();
	TB_ems_test_proc_stop();
	TB_frtu_test_proc_stop();

	TB_util_delay( 1000 );

	if( (g_test_item & TEST_ITEM_ALL) == TEST_ITEM_ALL )
	{
		printf("\r\n\r\n\r\n\r\n");
		printf( "[%s:%d] SUCCESS. TEST!!!!\r\n", __FILE__, __LINE__ );
		printf("\r\n\r\n\r\n\r\n");
	}
	else
	{
		printf("\r\n\r\n\r\n\r\n");
		printf( "[%s:%d] ERROR. TEST!!!!\r\n", __FILE__, __LINE__ );		
		if( (g_test_item & TEST_ITEM_WISUN) 	!= TEST_ITEM_WISUN )	printf( "     ERROR. TEST --> WISUN\r\n" );
		if( (g_test_item & TEST_ITEM_RS485A) 	!= TEST_ITEM_RS485A )	printf( "     ERROR. TEST --> RS485A\r\n" );
		if( (g_test_item & TEST_ITEM_RS485B) 	!= TEST_ITEM_RS485B )	printf( "     ERROR. TEST --> RS485B\r\n" );
		if( (g_test_item & TEST_ITEM_MODEM) 	!= TEST_ITEM_MODEM )	printf( "     ERROR. TEST --> MODEM\r\n" );
		if( (g_test_item & TEST_ITEM_PSU_MOD) 	!= TEST_ITEM_PSU_MOD )	printf( "     ERROR. TEST --> PSU_MOD\r\n" );
		if( (g_test_item & TEST_ITEM_PSU_DNP) 	!= TEST_ITEM_PSU_DNP )	printf( "     ERROR. TEST --> PSU_DNP\r\n" );
		if( (g_test_item & TEST_ITEM_EMS) 		!= TEST_ITEM_EMS )		printf( "     ERROR. TEST --> EMS\r\n" );
		if( (g_test_item & TEST_ITEM_FRTU) 		!= TEST_ITEM_FRTU )		printf( "     ERROR. TEST --> FRTU\r\n" );
		printf("\r\n\r\n\r\n\r\n");
	}

	TB_testmode_set_off();
	sleep( 3 );
	//system( "reboot");
}

int TB_testmode_start( void )
{
	TB_dm_set_mode( DMID_WISUN, DM_ON );	
	TB_dm_set_mode( DMID_EMS, DM_ON );	
	TB_dm_set_mode( DMID_MODEM, DM_ON );
	TB_dm_set_mode( DMID_FRTU, DM_ON );
	TB_dm_set_mode( DMID_J11, DM_ON );
	TB_dm_set_mode( DMID_PSU, DM_ON );

	if( pthread_create( &g_thid_test_mode, NULL, tb_test_mode_proc, NULL ) != 0 )
	{
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
		printf("FAIL. TEST THREAD CREATE.\r\n");
	}

	TB_led_init();
	
	if( TB_uart_wisun_1st_init() == 0 )
	{
		TB_wisun_test_proc_start();	
	}

	if( TB_uart_rs485a_init() == 0 &&	\
		TB_uart_rs485b_init() == 0 )
	{
		TB_rs485_test_proc_start();
	}
	
	if( TB_uart_mdm_init() == 0 )
	{
		TB_modem_test_proc_start();
	}
	
	if( TB_uart_psu_mod_init() == 0 )
	{
		TB_psu_mod_test_proc_start();
	}
	
	if( TB_uart_psu_dnp_init() == 0 )
	{
		TB_psu_dnp_test_proc_start();
	}
	
	if( TB_uart_ems_init() == 0 )
	{
		TB_ems_test_proc_start();
	}
	
	if( TB_uart_frtu_init() == 0 )
	{
		TB_frtu_test_proc_start();
	}

	while( 1 )
	{
		TB_util_delay( 1000 );
	}
}

int TB_testmode_end( void )
{
	g_proc_flag_test_mode = 0;
	pthread_join( g_thid_test_mode, NULL );
	
	//TB_testmode_set_off();
}

