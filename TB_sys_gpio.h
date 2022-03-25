#if !defined(__GPIOLIB_H)
#define __GPIOLIB_H

#include "TB_type.h"

typedef enum
{
	GPIO_TYPE_LTEM_LED	= 0,	// PC21
	GPIO_TYPE_WISUN_LED,		// PC22
	GPIO_TYPE_LTE_PWR_EN,		// PC23	LTE PWR ENABLE
	GPIO_TYPE_WISUN_RESET2,		// PC24	LTE RESET
	GPIO_TYPE_MAIN_LED,			// PC26
	GPIO_TYPE_485A_RE,			// PC29
	GPIO_TYPE_485B_RE,			// PC30
	GPIO_TYPE_DIG_LED,			// PC31
	GPIO_TYPE_POWER_LED,		// PD25
	GPIO_TYPE_WISUN_RESET1,		// PD06
	GPIO_TYPE_RF_CH1,			// PD01
	GPIO_TYPE_RF_CH2,			// PA27
	GPIO_TYPE_RF_CH3,			// PD19
	GPIO_TYPE_RF_CH4,			// PD20
	NUM_GPIO_TYPE
} GPIO_TYPE_E;

int		TB_gpio_init( void );
void 	TB_gpio_deinit(void);
void 	TB_gpio_exit( void );
int 	TB_gpio_set( int gpio_type, TBBL high );
int 	TB_gpio_get( int gpio_type );
int 	TB_gpio_led( int gpio_type, TBBL led_on );

#endif	// __GPIOLIB_H

