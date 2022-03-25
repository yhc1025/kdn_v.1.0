#ifndef	__TB_RES_ENG_H__
#define __TB_RES_ENG_H__

#include "TB_resource.h"

char *sp_message_eng[] = 
{
	"Press any key within 5 seconds to run SETUP.\n",
	"There is no master key.\nYou must set the master key.\nPress any key to run SETUP.\n",
	"It is the same as the current password.\n",
	"Password has been changed.\n",
	"You must change your password.\n",
	"Login failed 5 times.\nYou cannot login for 5 minutes.\n",
	"You cannot login for 5 minutes.\nThere are ",
	"seconds left to retry the login.\n",
	"Restart after 5 seconds. Press any key to cancel.\n",
	"Restart after 5 seconds\n",
	"All account information will be reset.\nDo you want to proceed? (Y/N)\n",
	"A request to change inverter configuration\n has been received. After the changes \nare complete, the device will reboot.\n",
	"Unknown.\n"
	
	
};

char *sp_res_eng[RESID_MAX] = 
{
	" 1. Setting (Comm, System, DER)   ",
	" 2. Comm Status                   ",	
	" 3. Role                          ",
	" 4. Wi-SUN                        ",
	" 5. Wi-SUN                        ",	
	" 6. History                       ",	
	" 7. Time                          ",
	" 8. Factory Initialization        ",
	" 9. Firmware Download             ",	
	"10. Master Key                    ",
	"11. Information                   ",
	"12. Reboot                        ",
	"13. Admin Setting                 ",	

	" 2. ------------------------------",
	
	////////////////////////////////////////////////////////////////////////////
	
	"           <Main Menu>          ",
	"            <Setting>           ",
	"          <Comm Status>         ",
	"          <Wi-SUN Role>         ",
	"            <Wi-SUN#1>          ",
	"            <Wi-SUN#2>          ",
	"             <History>          ",
	"        <Communication Log>     ",
	"          <Security Log>        ",
	"    <Communication Log Detail>  ",
	"      <Security Log Detail>     ",
	"            <Time Set>          ",
	"    <Factory Initialization>    ",
	"      <Firmware Download>       ",
	"           <Master Key>         ",
	"           <Master Key>         ",
	"          <LIBRARY INFO>        ",
	"                                ",
	"          <AMDIN SETTING>       ",
	"            <DEBUG MODE>        ",
	"         <CHANGE PASSWORD>      ",
	"           <INFORMATION>        ",	

	"     <CHANGE ADMIN PASSWORD>    ",
	"      <CHANGE USER PASSWORD>    ",
	"            <LOGIN>             ",

	////////////////////////////////////////////////////////////////////////////

	"1. COMMUNICATION",
	"2. SECURITY     ",
	
	"1. INPUT PASSWORD  ",	
	"1. CURRENT ",
	"2. NEW     ",
	"3. CONFIRM ",

	"MASTER MODULE  ",
	"REPEATER MODULE",
	"SLAVE MODULE   ",

	"GROUP G/W    ",		
	"DER-FRTU-1   ",
	"DER-FRTU-2   ",
	"DER-FRTU-3   ",	
	"DER-MID-1    ",
	"DER-MID-2    ",
	"DER-MID-3    ",
	"DER-REPEATER ",

	" 4. Wi-SUN 1st(Between DER-MID)   ",
	" 4. Wi-SUN 1st(Between DER-FRTU)  ",
	" 4. Wi-SUN 1st(Between GROUP G/W) ",
	" 5. Wi-SUN 2nd(Between GROUP G/W) ",
	" 5. REPEATER                      ",
	" 5. ------------------------------",
	"                                  ",

	"1. Start Factory Initialization",
	"1. Start Download\n",

	"1. ENABLE         ",
	"2. MODE           ",
	"3. MAX CONNECT[EA]",
	"4. FREQUENCY      ",

	"1. Comm Status          ",
	"2. Comm success rate/dBm",

	"1. Master Key edit",
	"2. Library Info   ",

	"1. Library Name     ",
	"2. Interface Version",
	"3. Library Version  ",
	
	"1. Master Key edit  ",
	"2. Key file upload  ",
	"3. Key file download",

	"1. ACCOUNT SETTING ",
	"2. ACCOUNT CLEAR   ",
	"3. DEBUG MODE      ",
	"1. ADMIN ",
	"2. USER  ",

	" 1. VERSION            ",
	" 2. VOLTAGE TYPE       ",
	" 3. EMS ADDRESS        ",
	" 4. SERIAL NUMBER      ",
	" 5. DATE OF MANUFACTURE",
	" 6. COT IP ADDRESS     ",
	" 7. RT PORT NUMBER     ",
	" 8. FRTU DNP ADDRESS   ",
	" 9. EMS                ",
	"10. SESSION KEY UPDATE ",

	" 1. DER-FRTU MODBUS Comm Speed",
	" 2. Inverter MODBUS Comm Speed",
	" 3. Inverter Number [EA]      ",
	" 4. Inverter Comm Timeout     ",
	" 5. Inverter Comm Retry       ",
	" 6. Inverter Read Delay       ",
	" 7. Inverter Write Delay      ",
	" 8. Auto Locking              ",
	" 9. Optical Sensor            ",
	"10. Master Comm Mode          ",
	"11. Slave  Comm Mode          ",

	////////////////////////////////////////////////////////////////////////////

	"BOOT COMPLETE                ",                     //	SYSTEM LOG
	"WATCHDOG                     ",
	"CHANGE SETUP DATA            ",
	"TIME SYNC                    ",
	"LAST WORKING TIME            ",
	"WORKING NORMAL               ",
	"FW UPDATE FAILED             ",
	"FW UPDATE SUCCESSFUL         ",
	"INITIALIZEATION FAILED       ",
	"INITIALIZEATION SUCCESSFUL   ",
	"LOGIN FAILED                 ",
	"LOGIN SUCCESS                ",
	"ADMIN PW CHANGED             ",
	"USER PW CHANGED              ",
	"CONFIG FILE CORRUPTION       ",
	"SECUTIRY LOG FILE CORRUPTION ",
	"COMMUNICATION FILE CORRUPTION",
	"CONNECTION STATE CHANGED.    "
	"UNKNOWN LOG CODE             ",

	"COMM CONNECTION SUCCESS  Group GW : FRTU     ",	//	COMMUNICATION LOG
	"COMM CONNECTION SUCCESS  Group GW : REPEATER ",
	"COMM CONNECTION SUCCESS  FRTU     : MID      ",
	"COMM CONNECTION FAILED   Group GW : FRTU     ",
	"COMM CONNECTION FAILED   Group GW : REPEATER ",
	"COMM CONNECTION FAILED   FRTU     : MID      ",
	"DATA ERROR               Group GW : MAIN UNIT",
	"WiSUN DATA ERROR                             ",
	"UNKNOWN LOG CODE                             ",

	"NO DETAILED DATA ",                                //	Detail for LOG
	"TIME SYNC",
	"UNKNOWN",
	"WORKING TIME",
	"Config data has been changed."	,					//	Detail for WATCHDOG
	"Unable to connect WiSUN to master device.",
	"No ping response from master device.",
	"Reboot command is executed in SETUP program.",
	"Reboot command received from EMS.",
	"Test mode is enabled.",
	"Connection state changed.",
	"File open Error",
	"File data Error",
	"File read Error",
	"File write Error",
	
	////////////////////////////////////////////////////////////////////////////

	"ENTER",
	"ENABLE ",
	"DISABLE",
	"HOUR",
	"MIN",
	"HIGH VOLTAGE",
	"LOW VOLTAGE ",

	////////////////////////////////////////////////////////////////////////////

	"\"LEFT\"  : move focus to left",
	"\"RIGHT\" : move focus to right",
	"\"UP\"    : increment value",
	"\"DOWN\"  : decrement value",
	"\"ESC\"   : return",
	"\"ENTER\" : run",

	"\"HOME\"  : move to first page   ",
	"\"END\"   : move to last page    ",
	"\"LEFT\"  : move to previous page",
	"\"RIGHT\" : move to next page    ",
	"\"UP\"    : move to previous line",
	"\"DOWN\"  : move to next line    ",
	"\"ENTER\" : Show details         ",

	"UNKNOWN"

};

#endif//__TB_RES_ENG_H__

