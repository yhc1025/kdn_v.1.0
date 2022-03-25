#if !defined(__WDTLIB_H)
#define __WDTLIB_H

#include "TB_type.h"

#define WDT_TIMEOUT_MIN			1	//	unit : sec
#define WDT_TIMEOUT_MAX			10
#define WDT_TIMEOUT_DEFAULT		5

////////////////////////////////////////////////////////////////////////////////

typedef enum tagWDT_COND
{
	WDT_COND_NO_LOG 			= -1,
	WDT_COND_CHANGE_CONFIG 		= 0,
	WDT_COND_WISUN_CONNECTION_FAIL,
	WDT_COND_PING_FAIL,
	WDT_COND_SETUP_REBOOT,
	WDT_COND_EMS_REBOOT,
	WDT_COND_SET_TEST_MODE,
	WDT_COND_COMM_STATE_CHANGED,

	WDT_COND_UNKNOWN
} TB_WDT_COND;

////////////////////////////////////////////////////////////////////////////////

extern int 	TB_wdt_init( int timeout );
extern int 	TB_wdt_enable( void );
extern int 	TB_wdt_disable( void );
extern int 	TB_wdt_deinit( void );
extern int 	TB_wdt_keepalive( void );
extern void	__TB_wdt_reboot( TB_WDT_COND cond, char *file, int line );
#define TB_wdt_reboot(c)	__TB_wdt_reboot(c,__FILE__,__LINE__)

#endif

