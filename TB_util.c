#include <sys/types.h>		//	for umask
#include <sys/stat.h>		//	for umask

#include "TB_setup.h"
#include "TB_util.h"

////////////////////////////////////////////////////////////////////////////////

TBUL TB_util_ascii2decimal( TBUC* p_ascii, TBUC length )
{
	TBUL decimal = 0xFFFFFFFF;

	while( length-- )
	{
		if( ( p_ascii[0] >= '0' )&& ( p_ascii[0] <= '9' ) )
		{
			if( decimal == 0xFFFFFFFF )
				decimal = 0;
			else
				decimal *= 10L;

			decimal += (TBUL)(p_ascii[0] - '0');
			p_ascii++;
		}
		else
			break;
	}

	return decimal;
}


void TB_util_delay( TBUL delay_msec )
{
	usleep( delay_msec * 1000 );
}

//	ex. 00 01 02 03 04 05
void TB_util_data_dump2( TBUC *data, TBUS length )
{
	if( data && length > 0 )
	{
		for( int i=0; i<length; i++ )
		{
			printf("%02X ", data[i]);
			if( (i % 16) == 15 )
			{
				if( i < length-1 )
					printf("\r\n");
			}
		}
		printf("\r\n");
	}
	else
	{
		printf( "[%s:%d] ERROR. data dump\r\n", __FILE__, __LINE__ );
	}
}

//	ex. 0001| 00 01 02 03 04 05
void TB_util_data_dump1( TBUC *data, TBUS length )
{
	if( data && length > 0 )
	{
		for( int i=0; i<length; i++ )
		{
			if( (i % 16) ==  0 )
				printf("%04X| ", i);
			
			printf("%02X ", data[i]);
			
			if( (i % 16) == 15 )
			{
				if( i < length-1 )
					printf("\r\n");
			}
		}
		printf("\r\n");
	}
	else
	{
		printf( "[%s:%d] ERROR. data dump\r\n", __FILE__, __LINE__ );
	}
}

void TB_util_data_dump( TBUC *msg, TBUC *data, TBUS length )
{
	if( msg )
	{
		printf( "%s ", msg );
	}

	if( data && length > 0 )
	{
		printf( "(%d byte)\r\n", length );
		printf( "------------------------------------------------------\r\n" );
		printf( "    | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F \r\n" );
		printf( "------------------------------------------------------\r\n" );
		TB_util_data_dump1( data, length );
		printf( "------------------------------------------------------\r\n" );
	}
	else
	{
		printf( "[%s:%d] ERROR. data dump : data=%x, len=%d\r\n", __FILE__, __LINE__, data, length );
	}
}

int TB_util_run_cmd( TBUC *command )
{
	int ret = -1;

	if( command )
	{
		printf("\r\nSYSTEM COMMAND = %s\r\n", command );
		system( command );
		printf("\r\n");
		ret = 0;
	}

	return ret;
}

void TB_util_print_time1( int YY, int MM, int DD, int hh, int mm, int ss)
{
	printf( "time --> [ %04d/%02d/%02d %02d:%02d:%02d ]\r\n", YY, MM, DD, hh, mm, ss );
}

void TB_util_print_time2( time_t t )
{
	struct tm *tm = localtime( &t );
	if( tm )
	{
		TB_util_print_time1(tm->tm_year+1900,	\
							tm->tm_mon+1,		\
							tm->tm_mday,		\
							tm->tm_hour,		\
							tm->tm_min,			\
							tm->tm_sec );
	}
}

int TB_util_set_systemtime1( int YY, int MM, int DD, int hh, int mm, int ss)
{
	int ret = -1;
	
	if( YY >= 2000 && YY < 2100 &&	\
		MM >=    1 && MM <=  12 &&	\
		DD >=    1 && DD <=  31 &&	\
		hh >=    0 && hh <=  23 &&	\
		mm >=    0 && mm <=  59 &&	\
		ss >=    0 && ss <=  59 )
	{
		char command[32];

		snprintf( command, sizeof(command), "date -s \"%d-%d-%d %d:%d:%d\"", YY, MM, DD, hh, mm, ss );	
		TB_util_run_cmd( command );
		
		snprintf( command, sizeof(command), "hwclock -w" );	
		TB_util_run_cmd( command );	
		
		snprintf( command, sizeof(command), "date" );
		TB_util_run_cmd( command );

		ret = 0;
	}
	else
	{
		printf( "Invalid Time info --> [ %04d/%02d/%02d %02d:%02d:%02d ]\r\n", YY, MM, DD, hh, mm, ss );
	}

	return ret;
}

int TB_util_set_systemtime2( time_t t )
{
	int ret = -1;
	struct tm *tm = localtime( &t );
	if( tm )
	{
		ret = TB_util_set_systemtime1( tm->tm_year+1900,	\
										tm->tm_mon+1,		\
										tm->tm_mday,		\
										tm->tm_hour,		\
										tm->tm_min,			\
										tm->tm_sec );
	}

	return ret;
}

TBBL TB_util_time_check_validation( time_t t )
{
	TBBL check = FALSE;
	struct tm *tm = localtime( &t );
	if( tm )
	{
		tm->tm_year += 1900;
		tm->tm_mon	+= 1;

		if( tm->tm_year >= 2000 && tm->tm_year <= 2100 &&
			tm->tm_mon	>= 	  1 && tm->tm_mon  <= 	12 &&
			tm->tm_mday >=	  1 && tm->tm_mday <=   31 &&
			tm->tm_hour >=    0 && tm->tm_hour <=   23 &&
			tm->tm_min  >=    0 && tm->tm_min  <=   59 &&
			tm->tm_sec  >=    0 && tm->tm_sec  <=   59 )
		{
			check = TRUE;
		}
	}

	return check;
}

int TB_util_is_leap_year( int year )
{
	int check1 = ((year %   4) == 0) && ((year % 100) != 0) ? 1 : 0;
	int check2 = ((year % 400) == 0) 						? 1 : 0;
	return ( check1 || check2 ) ? 1 : 0;
}

int TB_util_get_lastday_of_month( int mon, int year )
{
	static int s_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int		   days = -1;

	if( mon != 2 )
	{
		days = s_days[mon-1];
	}
	else
	{
		if( TB_util_is_leap_year(year) )
		{
			days = s_days[mon-1] + 1;
		}
		else
		{
			days = s_days[mon-1];
		}
	}

	return days;
}

void TB_util_print_fw_version_info( void )
{
	extern TBUC *get_product_info_string_fw_version( void );
	
	printf("\r\n\r\n\r\n" );
	printf("========================================================\r\n" );
	printf("        %s\r\n", get_product_info_string_fw_version() );
	printf("========================================================\r\n" );
	printf("\r\n\r\n\r\n" );
}

int TB_util_switch_bg_proc( void )
{
	pid_t pid = fork();
    if( pid < 0 )
    {
        printf( "error fork\n" );
    }
    else if( pid != 0 ) // parent process
    {
		exit(0);
    }
	
    setsid();
	
	umask( 0 );

	return 0;
}

TBUC *TB_util_get_datetime_string( time_t t )
{
	static TBUC s_time_str[128];
	struct tm *p_tm = localtime( &t );

	int YY = p_tm->tm_year+1900;
	int MM = p_tm->tm_mon+1;
	int DD = p_tm->tm_mday;
	int hh = p_tm->tm_hour;
	int mm = p_tm->tm_min;
	int ss = p_tm->tm_sec;

	bzero( s_time_str, sizeof(s_time_str) );

	if( YY >= 2000 && YY < 2100 &&	\
		MM >=    1 && MM <=  12 &&	\
		DD >=    1 && DD <=  31 &&	\
		hh >=    0 && hh <   24 &&	\
		mm >=    0 && mm <   60 &&	\
		ss >=    0 && ss <   60 )
	{
		strftime( s_time_str, sizeof(s_time_str), "%Y/%m/%d %H:%M:%S", p_tm );
	}
	else
	{
		char *null_time = "----/--/-- --:--:--";
		wstrncpy( s_time_str, sizeof(s_time_str), null_time, wstrlen(null_time) );
	}

	return s_time_str;
}

int TB_util_get_file_size( TBUC *p_path )
{
	int size = -1;

	if( p_path )
	{
		FILE *fp = fopen( p_path, "r" );
		if( fp )
		{
			fseek( fp, 0, SEEK_END );
			size = ftell( fp );

			fclose( fp );
		}
	}

	return size;
}

int TB_util_get_rand( int range_min, int range_max )
{
	int r = ((double)rand() / RAND_MAX) * (range_max - range_min) + range_min;
	return r;
}

