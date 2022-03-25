#ifndef	__TB_DEBUG_H__
#define __TB_DEBUG_H__

#include "TB_type.h"

typedef enum tagDMID
{
	DMID_ALL 	= 0,
	DMID_DEBUG,
	DMID_J11,
	DMID_WISUN,
	DMID_LED,
	DMID_GPIO,
	DMID_UART,
	DMID_WDT,
	DMID_MODBUS,
	DMID_MODEM,
	///////////////////////////////////	--> 10
	DMID_LTE,
	DMID_PSU,
	DMID_PACKET,
	DMID_INVERTER,
	DMID_LOG,
	DMID_NETQ,
	DMID_KCMVP,
	DMID_PING,
	DMID_MSGQ,
	DMID_COMMON,
	/////////////////////////////////// --> 20
	DMID_ELAPSE,
	DMID_SETUP,
	DMID_LOGIN,
	DMID_RSSI,
	DMID_EMS,
	DMID_FRTU,
	DMID_OPTICAL,
	
	DMID_COUNT
} TB_DMID;

////////////////////////////////////////////////////////////////////////////////

#define DM_ON		1
#define DM_OFF		0

////////////////////////////////////////////////////////////////////////////////

extern int 	TB_dm_print( TB_DMID mode, const char *fmt, ... );
extern int 	TB_dm_debug( const char *file, int line, TB_DMID mode, const char *fmt, ... );
extern int 	TB_dm_set_mode( TB_DMID mode, char onoff );
extern int 	TB_dm_is_on( TB_DMID mode );
extern int 	TB_dm_init( void );

extern void TB_dm_dbg_mode_all_off( void );
extern void TB_dm_dbg_mode_restore( void );
extern TBUC *TB_dm_dbg_get_mode( void );
extern TBUC *TB_dm_dbg_get_mode_backup( void );

////////////////////////////////////////////////////////////////////////////////

#define TB_dbg_j11(fmt,args...)			TB_dm_debug(__FILE__,__LINE__,DMID_J11,fmt,##args)
#define TB_dbg_wisun(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_WISUN,fmt,##args)
#define TB_dbg_modbus(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_MODBUS,fmt,##args)
#define TB_dbg_modem(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_MODEM,fmt,##args)
#define TB_dbg_psu(fmt,args...)			TB_dm_debug(__FILE__,__LINE__,DMID_PSU,fmt,##args)
#define TB_dbg_uart(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_UART,fmt,##args)
#define TB_dbg_inverter(fmt,args...)	TB_dm_debug(__FILE__,__LINE__,DMID_INVERTER,fmt,##args)
#define TB_dbg_log(fmt,args...)			TB_dm_debug(__FILE__,__LINE__,DMID_LOG,fmt,##args)
#define TB_dbg_gpio(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_GPIO,fmt,##args)
#define TB_dbg_netq(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_NETQ,fmt,##args)
#define TB_dbg_packet(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_PACKET,fmt,##args)
#define TB_dbg_kcmvp(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_KCMVP,fmt,##args)
#define TB_dbg_ping(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_PING,fmt,##args)
#define TB_dbg_msgq(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_MSGQ,fmt,##args)
#define TB_dbg_common(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_COMMON,fmt,##args)
#define TB_dbg_elapse(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_ELAPSE,fmt,##args)
#define TB_dbg_setup(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_SETUP,fmt,##args)
#define TB_dbg_lte(fmt,args...)			TB_dm_debug(__FILE__,__LINE__,DMID_LTE,fmt,##args)
#define TB_dbg_login(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_LOGIN,fmt,##args)
#define TB_dbg_rssi(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_RSSI,fmt,##args)
#define TB_dbg_ems(fmt,args...)			TB_dm_debug(__FILE__,__LINE__,DMID_EMS,fmt,##args)
#define TB_dbg_frtu(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_FRTU,fmt,##args)
#define TB_dbg_optical(fmt,args...)		TB_dm_debug(__FILE__,__LINE__,DMID_OPTICAL,fmt,##args)
#define TB_dbg_led(fmt,args...)			TB_dm_debug(__FILE__,__LINE__,DMID_LED,fmt,##args)

#define TB_prt_j11(fmt,args...)			TB_dm_print(DMID_J11,fmt,##args)
#define TB_prt_wisun(fmt,args...)		TB_dm_print(DMID_WISUN,fmt,##args)
#define TB_prt_modbus(fmt,args...)		TB_dm_print(DMID_MODBUS,fmt,##args)
#define TB_prt_modem(fmt,args...)		TB_dm_print(DMID_MODEM,fmt,##args)
#define TB_prt_psu(fmt,args...)			TB_dm_print(DMID_PSU,fmt,##args)
#define TB_prt_uart(fmt,args...)		TB_dm_print(DMID_UART,fmt,##args)
#define TB_prt_inverter(fmt,args...)	TB_dm_print(DMID_INVERTER,fmt,##args)
#define TB_prt_log(fmt,args...)			TB_dm_print(DMID_LOG,fmt,##args)
#define TB_prt_gpio(fmt,args...)		TB_dm_print(DMID_GPIO,fmt,##args)
#define TB_prt_netq(fmt,args...)		TB_dm_print(DMID_NETQ,fmt,##args)
#define TB_prt_packet(fmt,args...)		TB_dm_print(DMID_PACKET,fmt,##args)
#define TB_prt_kcmvp(fmt,args...)		TB_dm_print(DMID_KCMVP,fmt,##args)
#define TB_prt_ping(fmt,args...)		TB_dm_print(DMID_PING,fmt,##args)
#define TB_prt_msgq(fmt,args...)		TB_dm_print(DMID_MSGQ,fmt,##args)
#define TB_prt_common(fmt,args...)		TB_dm_print(DMID_COMMON,fmt,##args)
#define TB_prt_elapse(fmt,args...)		TB_dm_print(DMID_ELAPSE,fmt,##args)
#define TB_prt_setup(fmt,args...)		TB_dm_print(DMID_SETUP,fmt,##args)
#define TB_prt_lte(fmt,args...)			TB_dm_print(DMID_LTE,fmt,##args)
#define TB_prt_login(fmt,args...)		TB_dm_print(DMID_LOGIN,fmt,##args)
#define TB_prt_rssi(fmt,args...)		TB_dm_print(DMID_RSSI,fmt,##args)
#define TB_prt_ems(fmt,args...)			TB_dm_print(DMID_EMS,fmt,##args)
#define TB_prt_frtu(fmt,args...)		TB_dm_print(DMID_FRTU,fmt,##args)
#define TB_prt_optical(fmt,args...)		TB_dm_print(DMID_OPTICAL,fmt,##args)
#define TB_prt_led(fmt,args...)			TB_dm_print(DMID_LED,fmt,##args)

#endif//__TB_DEBUG_H__

