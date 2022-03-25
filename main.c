/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "TB_sys_gpio.h"
#include "TB_sys_wdt.h"
#include "TB_type.h"
#include "TB_wisun.h"
#include "TB_inverter.h"
#include "TB_modem.h"
#include "TB_ems.h"
#include "TB_frtu.h"
#include "TB_lte.h"
#include "TB_psu.h"
#include "TB_util.h"
#include "TB_j11.h"
#include "TB_setup.h"
#include "TB_msg_queue.h"
#include "TB_log.h"
#include "TB_debug.h"
#include "TB_elapse.h"
#include "TB_led.h"
#include "TB_kcmvp.h"
#include "TB_ping.h"
#include "TB_login.h"
#include "TB_ui.h"
#include "TB_resource.h"
#include "TB_msgbox.h"
#include "TB_optical.h"
#include "TB_test.h"
#include "TB_aes_evp.h"

////////////////////////////////////////////////////////////////////////////////

TBUC g_device_role = DEV_ROLE_GGW;

////////////////////////////////////////////////////////////////////////////////

int main_pre_process( void )
{
}

int main_post_process( void )
{
	//TB_wdt_deinit();

	TB_log_save_check( LOG_TYPE_SECU_START );
	TB_log_save_check( LOG_TYPE_COMM_START );

	TB_ui_deinit();
}

void __attribute__((constructor)) constructor_init_app( void )
{
	printf("---------------- App constructor START ----------------\r\n" );
	main_pre_process();
	printf("---------------- App constructor END ----------------\r\n" );
}

void __attribute__((destructor)) destructor_release_app( void )
{
	printf("---------------- App destructor START ----------------\r\n" );
	main_post_process();
	printf("---------------- App destructor END ----------------\r\n" );
}

void interrupt_handler( int sig )
{
    printf( "Signal Handler : %d\r\n", sig );
	if( sig == SIGINT )
	{
		main_post_process();
		exit( 0 );
	}
}

int interrupt_register( void )
{
	signal( SIGINT, interrupt_handler );
}

////////////////////////////////////////////////////////////////////////////////

int main( int argc, char **argv )
{
	/*
	*	Makefile에 -DDEVICE_ROLE로 정의됨.
	*/
	g_device_role = DEVICE_ROLE;	
	printf( "Device ROLE = %d\r\n", g_device_role );
	
	TB_testmode_check();

	////////////////////////////////////////////////////////////////////////////
		
	interrupt_register();

	TB_wdt_init( WDT_TIMEOUT_DEFAULT );

	////////////////////////////////////////////////////////////////////////////

	TB_dm_set_mode( DMID_ALL, DM_OFF );	
	
	TB_dm_set_mode( DMID_LOGIN, DM_ON );
//	TB_dm_set_mode( DMID_SETUP, DM_ON );
//	TB_dm_set_mode( DMID_EMS, DM_ON );
//	TB_dm_set_mode( DMID_FRTU, DM_ON );
//	TB_dm_set_mode( DMID_LOG, DM_ON );
	TB_dm_set_mode( DMID_KCMVP, DM_ON );
	TB_dm_set_mode( DMID_MODEM, DM_ON );
//	TB_dm_set_mode( DMID_PSU, DM_ON );
//	TB_dm_set_mode( DMID_INVERTER, DM_ON );
//	TB_dm_set_mode( DMID_J11, DM_ON );
	TB_dm_set_mode( DMID_WISUN, DM_ON );
//	TB_dm_set_mode( DMID_NETQ, DM_ON );
//	TB_dm_set_mode( DMID_RSSI, DM_ON );
//	TB_dm_set_mode( DMID_PING, DM_ON );
//	TB_dm_set_mode( DMID_ELAPSE, DM_ON );
	
	////////////////////////////////////////////////////////////////////////////

	TB_aes_evp_init();

	////////////////////////////////////////////////////////////////////////////

	TB_log_init( TRUE );
	TB_setup_init();
	TB_msgq_init();
	TB_login_init();
	TB_kcmvp_pre_init();

	////////////////////////////////////////////////////////////////////////////

	if( TB_show_msgbox( TB_msgid_get_string(MSGID_ENTER_SETUP), 5, TRUE ) == 0 )
	{
		TB_setup_enter( ACCOUNT_TYPE_USER, TRUE );
	}

	////////////////////////////////////////////////////////////////////////////

	TB_SETUP_WORK config_org;

	bzero( &config_org, sizeof(config_org) );
	if( TB_setup_read_file( &config_org ) == 0 )
	{
		TB_setup_init();

		while( 1 )
		{
			if( TB_kcmvp_key_check() != 0 )
			{
				TB_show_msgbox( TB_msgid_get_string(MSGID_MASTERKEY), 0, TRUE );

				TB_setup_enter( ACCOUNT_TYPE_ADMIN, TRUE );
				TB_setup_read_file( NULL );
			}
			else
			{
				break;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////

	TB_gpio_init();	
	TB_led_init();

	TB_util_print_fw_version_info();

	////////////////////////////////////////////////////////////////////////////

	TB_kcmvp_init();

	////////////////////////////////////////////////////////////////////////////

	TB_ROLE role = TB_setup_get_role();

	if( role == ROLE_GRPGW )
	{
		TB_modem_init();
		TB_util_delay( 100 );
	}
	else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
	{
		TBSI comm_type = TB_setup_get_comm_type_master();
		if( comm_type == COMM_MODE_MASTER_RS232 )
		{
			TB_modem_init();
			TB_util_delay( 100 );
		}
	}
	
	if( role == ROLE_GRPGW )
	{
		TB_frtu_init();
		TB_util_delay( 100 );
	}

#if 1
	TB_lte_init();
#else
	if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
	{
		TBSI comm_type = TB_setup_get_comm_type_master();
		if( comm_type == COMM_MODE_MASTER_LTE )
		{
			TB_lte_init();
			TB_util_delay( 100 );
		}
	}
#endif

	if( role >= ROLE_RELAY1 && role <= ROLE_RELAY3 )
	{
		TB_rs485_init();
		TB_util_delay( 100 );
	}
	else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
	{
		TBSI comm_type = TB_setup_get_comm_type_slave();
		if( comm_type == COMM_MODE_SLAVE_RS485 )
		{
			TB_rs485_init();
			TB_util_delay( 100 );
		}
		
		TB_psu_mod_init();
		TB_util_delay( 100 );
		
		TB_psu_dnp_init();
		TB_util_delay( 100 );

		if( TB_setup_get_optical_sensor() == TRUE )
		{
			TB_optical_init();
			TB_util_delay( 100 );
		}
	}

	////////////////////////////////////////////////////////////////////////////

	TB_util_delay( 1000 );

	////////////////////////////////////////////////////////////////////////////

	TB_wisun_uart_init( 0 );
	TB_wisun_uart_init( 1 );
		
	TB_j11_init();

	////////////////////////////////////////////////////////////////////////////

	TB_log_last_working_time_init();
	TB_log_last_working_time_save();

	////////////////////////////////////////////////////////////////////////////
	//			시간 동기화가 완료된 후에 로그를 저장하기 위함.
	////////////////////////////////////////////////////////////////////////////
	TB_SETUP_WORK *p_config = TB_setup_get_config();
	if( p_config )
	{
		if( memcmp( &config_org, p_config, sizeof(TB_SETUP_WORK) ) != 0 )
		{
			TB_log_append( LOG_SECU_SETUP, NULL, -1 );
		}
	}

	////////////////////////////////////////////////////////////////////////////	

	if( TB_setup_get_product_info_ems_use() == TRUE )
	{
		if( TB_setup_get_role() != ROLE_REPEATER )
		{
			TB_ems_init();
		}
	}

	////////////////////////////////////////////////////////////////////////////
	
	TB_log_append( LOG_SECU_BOOT, NULL, -1 );

	////////////////////////////////////////////////////////////////////////////

	TB_wdt_enable();

	////////////////////////////////////////////////////////////////////////////

	TB_ui_init();
	TB_ui_clear( UI_CLEAR_ALL );

	////////////////////////////////////////////////////////////////////////////

	while( 1 )
	{
		if( TB_setup_is_run() == FALSE )
		{
			int input = TB_ui_getch( FALSE );
			if( input == '\n' || input == KEY_ENTER )
			{
				TB_setup_enter( ACCOUNT_TYPE_USER, FALSE );
				TB_setup_init();

				TB_ui_nonblocking( TRUE );
			}
		}

		TB_util_delay( 100 );
	}

	return 0;
}

