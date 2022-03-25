#include "TB_uart.h"
#include "TB_timer_util.h"
#include "TB_rssi.h"
#include "TB_j11.h"
#include "TB_setup.h"
#include "TB_test.h"
#include "TB_led.h"

////////////////////////////////////////////////////////////////////////////////

#define LED_OFF		0
#define LED_ON		1

////////////////////////////////////////////////////////////////////////////////

static int s_led_wisun_work[2] = {0, 0};

////////////////////////////////////////////////////////////////////////////////

#define TIMER_PERIOD_LED		50
#define TIMER_PERIOD_LED_DIG	50

////////////////////////////////////////////////////////////////////////////////

//#define WARNING_PERCENT		20	//	-90dbm
#define WARNING_PERCENT		27	//	-85dbm
//#define WARNING_PERCENT		34	//	-80dbm
static TBBL tb_led_get_wisun_dbm_warning( int wisun_idx )
{
	TBBL warning = FALSE;
	struct comm_info *p_info = NULL;

	if( wisun_idx == 0 || wisun_idx == 1 )
	{
		TB_WISUN_MODE mode = TB_setup_get_wisun_info_mode( wisun_idx );
		if( mode == WISUN_MODE_PANC )	//	Master Mode
		{
			for( int i=0; i<MAX_CONNECT_CNT; i++ )
			{
				p_info = TB_rssi_get_comm_info( wisun_idx, i );
				if( p_info )
				{
					if( p_info->percent != 0 )
					{
						if( p_info->percent <= WARNING_PERCENT )
						{
							TB_dbg_led("WiSUN warning. %d, %d, %ddbm, %d%%\r\n", wisun_idx, i, p_info->dbm, p_info->percent );
							warning = TRUE;
						}
					}
				}
			}
		}
		else							//	Slave Mode
		{
			p_info = TB_rssi_get_comm_info( wisun_idx, 0 );
			if( p_info )
			{
				if( p_info->percent != 0 )
				{
					if( p_info->percent <= WARNING_PERCENT )
					{
						TB_dbg_led("WiSUN warning. %d, %d, %ddbm, %d%%\r\n", wisun_idx, 0, p_info->dbm, p_info->percent );
						warning = TRUE;
					}
				}
			}
		}
	}

	return warning;
}

void timer_proc_led( void *arg )
{
	if( TB_testmode_is_on() )
	{
		static TBBL	s_led_main 		= LED_OFF;
		static int 	s_expire_main 	= 0;
		
		/*
		*	period 500msec
		*/
		if( s_expire_main == 0 )
		{	
			TB_gpio_led( GPIO_TYPE_LTEM_LED, s_led_main );
			TB_gpio_led( GPIO_TYPE_WISUN_LED, s_led_main );
			TB_gpio_led( GPIO_TYPE_POWER_LED, s_led_main );
			TB_gpio_led( GPIO_TYPE_DIG_LED, s_led_main );
			
			s_led_main = !s_led_main;
			s_expire_main = 500 / TIMER_PERIOD_LED;
		}
		else
		{
			s_expire_main --;
		}

	}
	else
	{
		static TBBL s_led_wisun = LED_OFF;
		static TBBL s_led_ltem 	= LED_OFF;
		static TBBL	s_led_main 	= LED_OFF;
		static TBBL	s_led_pwr 	= LED_OFF;

		static int 	s_expire_wisun 	= 0;
		static int 	s_expire_main 	= 0;
		static int 	s_expire_pwr 	= 0;

		if( s_expire_wisun == 0 )
		{
			if( tb_led_get_wisun_dbm_warning(0) == TRUE )
			{
				s_led_wisun = !s_led_wisun;
				TB_gpio_led( GPIO_TYPE_WISUN_LED, s_led_wisun );
				s_expire_wisun = (100 / TIMER_PERIOD_LED);
			}
			else
			{
				if( s_led_wisun_work[0] == 1 )
				{
					s_led_wisun = !s_led_wisun;
					TB_gpio_led( GPIO_TYPE_WISUN_LED, s_led_wisun );
					s_expire_wisun = (500 / TIMER_PERIOD_LED);
				}
				else
				{
					TB_gpio_led( GPIO_TYPE_WISUN_LED, LED_OFF);
				}
			}

			////////////////////////////////////////////////////////////////////

			if( tb_led_get_wisun_dbm_warning(1) == TRUE )
			{
				s_led_wisun = !s_led_wisun;
				TB_gpio_led( GPIO_TYPE_WISUN_LED, s_led_wisun );
				s_expire_wisun = (100 / TIMER_PERIOD_LED);
			}
			else
			{
				if( s_led_wisun_work[1] == 1 )
				{
					s_led_ltem = !s_led_ltem;
					TB_gpio_led( GPIO_TYPE_LTEM_LED, s_led_ltem );
					s_expire_wisun = (500 / TIMER_PERIOD_LED);
				}
				else
				{
					TB_gpio_led( GPIO_TYPE_LTEM_LED, LED_OFF );
				}
			}
		}
		else
		{
			s_expire_wisun --;
		}

		/*
		*	period 500msec
		*/
		if( s_expire_main == 0 )
		{		
			TB_gpio_led( GPIO_TYPE_MAIN_LED, s_led_main );
			s_led_main = !s_led_main;
			s_expire_main = 500 / TIMER_PERIOD_LED;
		}
		else
		{
			s_expire_main --;
		}

		/*
		*	period 1000msec
		*/
		if( s_expire_pwr == 0 )
		{
			TB_gpio_led( GPIO_TYPE_POWER_LED, s_led_pwr );
			s_led_pwr = !s_led_pwr;
			s_expire_pwr = 1000 / TIMER_PERIOD_LED;
		}
		else
		{
			s_expire_pwr --;
		}
	}
}

#define	LED_DIG_NORMAL		0x00
#define	LED_DIG_CHECK		0x01
#define	LED_DIG_WARN		0x02
#define	LED_DIG_CRITICAL	0x04

static int s_led_dig_work = 0;

void TB_led_dig_set_wisun_state( TBUC percent )
{
	if( percent <= 100 )
	{
		if( percent <= 10 )
		{
			TB_led_dig_add_critical();
		}
		else if( percent <= 20 )
		{
			TB_led_dig_add_check();
		}
		else if( percent <= 30 )
		{
			TB_led_dig_add_warn();
		}
		else 
		{
			TB_led_dig_del_critical();
			TB_led_dig_del_check();
			TB_led_dig_del_warn();
		}
	}
	else
	{
		printf("[%s:%d] Invalid Wisun Percent = %d\r\n", __FILE__, __LINE__, percent );
	}
}

void TB_led_dig_add_critical( void )
{
	s_led_dig_work |= LED_DIG_CRITICAL;
}

void TB_led_dig_del_critical( void )
{
	s_led_dig_work &= ~LED_DIG_CRITICAL;
}

void TB_led_dig_add_warn( void )
{
	s_led_dig_work |= LED_DIG_WARN;
}

void TB_led_dig_del_warn( void )
{
	s_led_dig_work &= ~LED_DIG_WARN;
}

void TB_led_dig_add_check( void )
{
	s_led_dig_work |= LED_DIG_CHECK;
}

void TB_led_dig_del_check( void )
{
	s_led_dig_work &= ~LED_DIG_CHECK;
}

void TB_led_dig_set_normal( void )
{
	s_led_dig_work = LED_DIG_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

#define ACT_TIME	5	//sec

enum 
{
	ACT_TYPE_CHECK 	= 1,
	ACT_TYPE_WARN,
	ACT_TYPE_CRITICAL,
	ACT_TYPE_MAX,
};

#define ACT_TYPE_CHECK_NEXT		ACT_TYPE_WARN
#define ACT_TYPE_WARN_NEXT		ACT_TYPE_CRITICAL
#define ACT_TYPE_CRITICAL_NEXT	ACT_TYPE_CHECK
void timer_proc_led_dig( void *arg )
{
	static int s_expire_dig = 0;
	static int s_act_type = 0;
	static int s_led_off_state = 0;

	static TBBL	s_led_dig = LED_OFF;

	if( s_led_dig_work != LED_DIG_NORMAL )
	{	
		////////////////////////////////////////////////////////////////////////
		//						Dig LED work : Checking
		////////////////////////////////////////////////////////////////////////
		if( s_led_dig_work & LED_DIG_CHECK )
		{
			if( s_act_type == 0 || s_act_type == ACT_TYPE_CHECK )
			{
				static time_t t_old = 0;
				static time_t t_new = 0;

				if( t_old == 0 )
					t_old = time( NULL );
				
				if( s_expire_dig == 0 )
				{
					TB_gpio_led( GPIO_TYPE_DIG_LED, s_led_dig );
					s_led_dig = !s_led_dig;
					s_expire_dig = (1000 / TIMER_PERIOD_LED_DIG);
				}
				else
				{
					s_expire_dig --;
				}

				t_new = time( NULL );
				if( t_old + ACT_TIME < t_new )
				{
					s_act_type = ACT_TYPE_CHECK_NEXT;

					t_old = 0;
					t_new = 0;
				}
			}
		}
		else
		{
			s_act_type = ACT_TYPE_CHECK_NEXT;
		}

		////////////////////////////////////////////////////////////////////////
		//						Dig LED work : Warning
		////////////////////////////////////////////////////////////////////////
		if( s_led_dig_work & LED_DIG_WARN )
		{
			if( s_act_type == 0 || s_act_type == ACT_TYPE_WARN )
			{
				static time_t t_old = 0;
				static time_t t_new = 0;

				if( t_old == 0 )
					t_old = time( NULL );
				
				if( s_expire_dig == 0 )
				{
					TB_gpio_led( GPIO_TYPE_DIG_LED, s_led_dig );
					s_led_dig = !s_led_dig;
					s_expire_dig = (500 / TIMER_PERIOD_LED_DIG);
				}
				else
				{
					s_expire_dig --;
				}

				t_new = time( NULL );
				if( t_old + ACT_TIME < t_new )
				{
					s_act_type = ACT_TYPE_WARN_NEXT;

					t_old = 0;
					t_new = 0;
				}
			}
		}
		else
		{
			s_act_type = ACT_TYPE_WARN_NEXT;
		}

		////////////////////////////////////////////////////////////////////////
		//						Dig LED work : Critical
		////////////////////////////////////////////////////////////////////////
		if( s_led_dig_work & LED_DIG_CRITICAL )
		{
			if( s_act_type == 0 || s_act_type == ACT_TYPE_CRITICAL )
			{
				static time_t t_old = 0;
				static time_t t_new = 0;

				if( t_old == 0 )
					t_old = time( NULL );
				
				if( s_expire_dig == 0 )
				{
					TB_gpio_led( GPIO_TYPE_DIG_LED, s_led_dig );
					s_led_dig = !s_led_dig;
					s_expire_dig = (100 / TIMER_PERIOD_LED_DIG);
				}
				else
				{
					s_expire_dig --;
				}

				t_new = time( NULL );
				if( t_old + ACT_TIME < t_new )
				{
					s_act_type = ACT_TYPE_CRITICAL_NEXT;

					t_old = 0;
					t_new = 0;
				}
			}
		}
		else
		{
			s_act_type = ACT_TYPE_CRITICAL_NEXT;
		}

		s_led_off_state = 0;
	}
	else
	{
		if( s_led_off_state == 0 )
		{
			TB_gpio_led( GPIO_TYPE_DIG_LED, LED_OFF );
			s_led_off_state = 1;
		}
	}
}

int TB_led_wisun_work( int idx )
{
	int ret = -1;
	
	if( idx == 0 || idx == 1 )
	{
		s_led_wisun_work[idx] = 1;
		ret = 0;
	}
	
	return ret;
}

static TB_TIMER *s_timer_led 	= NULL;
static TB_TIMER *s_timer_led_dig = NULL;
void TB_led_init( void )
{
	if( TB_testmode_is_on() )
	{
		s_timer_led = TB_timer_handle_open( timer_proc_led, TIMER_PERIOD_LED );
	}
	else
	{
		s_timer_led 	= TB_timer_handle_open( timer_proc_led, TIMER_PERIOD_LED );
		s_timer_led_dig = TB_timer_handle_open( timer_proc_led_dig, TIMER_PERIOD_LED_DIG );
	}
}

void TB_led_deinit( void )
{
	s_timer_led 	= TB_timer_handle_close( s_timer_led );
	s_timer_led_dig = TB_timer_handle_close( s_timer_led_dig );
}


