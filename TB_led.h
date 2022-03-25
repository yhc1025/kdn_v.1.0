#ifndef	__TB_LED_H__
#define __TB_LED_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include "TB_sys_gpio.h"

extern void TB_led_init( void );
extern void TB_led_deinit( void );
extern int  TB_led_wisun_work( int idx );

extern void TB_led_dig_set_wisun_state( TBUC percent );
extern void TB_led_dig_add_critical( void );
extern void TB_led_dig_del_critical( void );
extern void TB_led_dig_add_warn( void );
extern void TB_led_dig_del_warn( void );
extern void TB_led_dig_add_check( void );
extern void TB_led_dig_del_check( void );
extern void TB_led_dig_set_normal( void );

#endif//__TB_LED_H__

