#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include "TB_util.h"
#include "TB_uart.h"
#include "TB_sys_gpio.h"
#include "TB_msg_queue.h"
#include "TB_modbus.h"
#include "TB_psu.h"
#include "TB_elapse.h"
#include "TB_debug.h"
#include "TB_setup.h"
#include "TB_wisun.h"
#include "TB_log.h"

#include "TB_modem.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct tagMODEM_DNP
{
	TBUC buffer[MAX_DNP_BUF];
	TBUS length;
} TB_MODEM_DNP;

////////////////////////////////////////////////////////////////////////////////

static TB_MODEM_DNP	s_modem_dnp;

////////////////////////////////////////////////////////////////////////////////

static pthread_mutex_t		s_modem_write_mutex;
static pthread_mutexattr_t 	s_modem_mutex_attr;
int TB_modem_init( void )
{
	int ret = -1;

	bzero( &s_modem_dnp, sizeof(s_modem_dnp) );	
	if( TB_uart_mdm_init() == 0 )
	{
		pthread_mutexattr_init( &s_modem_mutex_attr );
		pthread_mutexattr_settype( &s_modem_mutex_attr, PTHREAD_MUTEX_ERRORCHECK );
		pthread_mutex_init( &s_modem_write_mutex, &s_modem_mutex_attr );
		
		TB_modem_proc_start();
		ret = 0;
	}
	
	return 0;
}

TBUS TB_modem_read( void )
{
	s_modem_dnp.length = TB_uart_mdm_read( s_modem_dnp.buffer, sizeof(s_modem_dnp.buffer) );
	return s_modem_dnp.length;
}

TBUS TB_modem_write( TBUC *buf, TBUS len )
{
	TBUS ret = 0;
	static volatile int s_count = 0;

	if( buf && len > 0 )
	{
		while( s_count > 1 )
		{
			TB_util_delay( 10 );
		}

		s_count ++;		
		pthread_mutex_lock( &s_modem_write_mutex );

		ret = TB_uart_mdm_write( buf, len );

		pthread_mutex_unlock( &s_modem_write_mutex );
		s_count --;
	}

	return ret;
}

void TB_modem_deinit( void )
{
	TB_uart_mdm_deinit();
}

void TB_modem_work( TBUC *data, TBUS length )
{
	if( data && length > 0 )
	{
		TB_SETUP_WISUN *p_wisun_info = TB_setup_get_wisun_info( 0 );
		TB_ROLE role = TB_setup_get_role();

		TB_MESSAGE send_msg;
		
		TBUS frtu_dnp = TB_setup_get_product_info_frtp_dnp();
		TBUC len = data[2];
		TBUS dest_dnp_addr = (data[4] << 0) |	\
							 (data[5] << 8);

		TB_dbg_modem("DEST dnp addr = %d, external FRTU dnp = %d\r\n", dest_dnp_addr, frtu_dnp );

		/*******************************************************
		*					외부 FRTU로 전송
		*******************************************************/
		if( dest_dnp_addr == frtu_dnp )
		{
			send_msg.type = MSG_TYPE_FRTU;
			send_msg.id	  = MSG_CMD_FRTU_WRITE;
			send_msg.data = data;
			send_msg.size = length;

			if( TB_dm_is_on(DMID_MODEM) )
				TB_dbg_modem("Send Message : From MODEM to external FRTU. msgid=0x%X\r\n", send_msg.id );

			TB_msgq_send( &send_msg );
		}
		/*******************************************************
		*				내부 하위 네트워크로 전송
		*******************************************************/
		else
		{
			if( role == ROLE_GRPGW )
			{
				for( int i=0; i<p_wisun_info->max_connect; i++ )
				{	
					send_msg.type = MSG_TYPE_WISUN_GGW2TERM;
					send_msg.id	  = MSG_CMD_WISUN_SEND_GGW2TERM1 + i;
					send_msg.data = data;
					send_msg.size = length;

					TB_dbg_modem("Send Message : From MODEM to Terminal. msgid=0x%X\r\n", send_msg.id );
					//TB_util_data_dump2(send_msg.data, send_msg.size);
					TB_msgq_send( &send_msg );

					TB_util_delay( 1000 );
				}
			}
			else if( role >= ROLE_TERM_MIN && role <= ROLE_TERM_MAX )
			{
				send_msg.type 	= MSG_TYPE_PSU_DNP;
				send_msg.id		= MSG_CMD_PSU_DNP_WRITE;
				send_msg.data	= data;
				send_msg.size	= length;

				TB_dbg_modem("Send Message : From MODEM to PSU_DNP. msgid=0x%X\r\n", send_msg.id );
				//TB_util_data_dump2(send_msg.data, send_msg.size);
				TB_msgq_send( &send_msg );
			}
		}
	}
}

#define RT_STX_1ST		0x4B
#define RT_STX_2ND		0x4B
#define RT_STX_SIZE		2
#define RT_LEN_SIZE		2
#define RT_HEADER_SIZE	(RT_STX_SIZE + RT_LEN_SIZE)

#define DNP_STX_1ST		0x05
#define DNP_STX_2ND		0x64

#define DNP_STX_SIZE	2
#define DNP_LEN_SIZE	1

#if 1
int TB_modem_dnp_packet_check( TBUC *p_dnp_packet, TBUC dnp_packet_len )
{
	int error_check = 1;

	if( p_dnp_packet && dnp_packet_len > 0 )
	{
		if( p_dnp_packet[0] == DNP_STX_1ST && p_dnp_packet[1] == DNP_STX_2ND )
		{
			TBUC dnp_data_len = p_dnp_packet[2];	//	DATA1 + DATA2 + DATA3 + ......
			/************************************************
			*	   |  STX  |  LEN |     DATA1      |  CRC1  |
			*************************************************
			*  IDX | 00 01 |  02  | 03 04 05 06 07 |  08 09 |
			*************************************************
			*	ex | 05 64 |  05  | C0 03 00 FE FF |  B3 FD |
			************************************************/
			
			/****************************************************************************
			*	   |  STX  |  LEN |     DATA1      |  CRC1  |      DATA2        |  CRC2 |
			*****************************************************************************
			*  IDX | 00 01 |  02  | 03 04 05 06 07 |  08 09 | 0A 0B 0C 0D 0E 0F | 10 11 |
			*****************************************************************************
			*	ex | 05 64 |  0B  | C4 01 27 FE FF |  A4 E4 | C1 C1 01 3C 01 06 | 1E C6 |
			****************************************************************************/
			
			/********************************************************************************************************************************
			*	   |  STX  |  LEN |     DATA1      |  CRC1  |               DATA2                             |  CRC2 |    DATA3    |  CRC3 |
			*********************************************************************************************************************************
			*  IDX | 00 01 |  02  | 03 04 05 06 07 |  08 09 | 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 | 1A 1B | 1C 1D 1E 1F | 20 21 |
			*********************************************************************************************************************************
			*   ex | 05 64 |  19  | C4 FF FF FE FF |  2C 37 | C0 CA 0C 32 02 07 01 98 CC E6 34 7E 01 00 00 00 | 2F 4C | 00 14 00 06 | 22 FA |
			********************************************************************************************************************************/
			int  delimit_cnt = 0;
			TBUC offset = 0;
			while( dnp_data_len > 0 )
			{
				if( delimit_cnt == 0 )
				{
					error_check = 1;
					if( TB_crc16_dnp_check( &p_dnp_packet[offset], (DNP_STX_SIZE + DNP_LEN_SIZE + 5) + CRC_SIZE ) == TRUE )
					{
						offset += (DNP_STX_SIZE + DNP_LEN_SIZE + 5) + CRC_SIZE;
						dnp_data_len -= 5;
						delimit_cnt ++;

						error_check = 0;
					}
					else
					{
						break;
					}
				}
				else
				{
					error_check = 1;
					if( dnp_data_len - 16 >= 0 )
					{
						if( TB_crc16_dnp_check( &p_dnp_packet[offset], 16 + CRC_SIZE) == TRUE )
						{
							offset += (16 + CRC_SIZE); 
							dnp_data_len -= 16;
							delimit_cnt ++;

							error_check = 0;
						}
						else
						{
							break;
						}
					}
					else
					{
						TBUC remain = dnp_data_len;
						if( TB_crc16_dnp_check( &p_dnp_packet[offset], remain + CRC_SIZE) == TRUE )
						{
							offset += (remain + CRC_SIZE); 
							dnp_data_len -= remain;
							delimit_cnt ++;

							error_check = 0;
						}
						else
						{
							break;
						}
					}
				}					
			}			
		}
	}

	return error_check;
}
#endif

TBUC s_proc_flag_modem = 1;
static void *tb_modem_proc( void *arg )
{
	TB_MESSAGE send_msg;
	TB_MESSAGE recv_msg;

	TB_SETUP_WISUN *p_wisun_info = TB_setup_get_wisun_info( 0 );
	TB_ROLE role = TB_setup_get_role();

	/***************************************************************************
	*	20211201 - KDN 요청사항 : 	GGW와 하위간의 통신상태와 별개로, 주장치와
	*								외부 FRTU와 통신이 이루어지도록 함.
	***************************************************************************/
	while( 0 )
	{
		TB_util_delay( 500 );
		
		TB_ROLE role = TB_setup_get_role();
		if( role == ROLE_GRPGW )
		{
			if( TB_wisun_init_complete(WISUN_FIRST) == FALSE )
				continue;

			break;
		}
		else if( role >= ROLE_TERM1 && role <= ROLE_TERM3 )
		{
			if( TB_setup_get_comm_type_master() == COMM_MODE_MASTER_WISUN )
				if( TB_wisun_init_complete(WISUN_SECOND) == FALSE )
					continue;

			if( TB_setup_get_comm_type_slave() == COMM_MODE_SLAVE_WISUN )
				if( TB_wisun_init_complete(WISUN_FIRST) == FALSE )
					continue;

			break;
		}
	}

	int check = 0;
	TB_MODEM_DNP modem_dnp;

	printf("===========================================\r\n" );
	printf("              Start Modem Proc             \r\n" );
	printf("===========================================\r\n" );
	while( s_proc_flag_modem )
	{
		check = 0;

		while( TB_modem_read() > 0 )
		{
			if( check == 0 )
				bzero( &modem_dnp, sizeof(modem_dnp) );
			
			check = 1;
			wmemcpy( &modem_dnp.buffer[modem_dnp.length],		\
					 sizeof(modem_dnp.buffer)-modem_dnp.length,	\
					 s_modem_dnp.buffer,						\
					 s_modem_dnp.length );
			modem_dnp.length += s_modem_dnp.length;
			TB_util_delay( 300 );
		}

		if( check == 1 )
		{
			TBUS rt_packet_len = 0;
			TBUI offset = 0;
			if( TB_dm_is_on(DMID_MODEM) )
				TB_util_data_dump( "Modem Read data", modem_dnp.buffer, modem_dnp.length );

			int dnp_packet_cnt = 0;
			for( int i=0; i<modem_dnp.length; i++ )
			{
				if( modem_dnp.buffer[i] == RT_STX_1ST && modem_dnp.buffer[i+1] == RT_STX_2ND )
				{
					rt_packet_len = (modem_dnp.buffer[i+RT_STX_SIZE+0] << 0) |	\
								    (modem_dnp.buffer[i+RT_STX_SIZE+1] << 8);

					if( rt_packet_len <= modem_dnp.length - i )
					{
						int error_check = 1;

						TBUC dnp_packet_len = (TBUC)rt_packet_len;						
						offset = i + RT_HEADER_SIZE;

						error_check = TB_modem_dnp_packet_check( &modem_dnp.buffer[offset], dnp_packet_len );
						if( error_check )
						{
							TB_dbg_modem( "ERROR. Read data from MODEM\r\n" );
							TB_log_append( LOG_TYPE_MODEM_DATA_ERROR, NULL, -1 );
							break;
						}
						else
						{
							TB_modem_work( &modem_dnp.buffer[offset], dnp_packet_len );
							i += dnp_packet_len;
						}
					}
					else
					{
						TB_dbg_modem( "ERROR. Read data from MODEM\r\n" );
						TB_log_append( LOG_TYPE_MODEM_DATA_ERROR, NULL, -1 );
						break;
					}
				}
				else if( modem_dnp.buffer[i] == DNP_STX_1ST && modem_dnp.buffer[i+1] == DNP_STX_2ND )
				{
					if( TB_modem_dnp_packet_check( &modem_dnp.buffer[i], modem_dnp.length-i ) == 0 )
					{
						TB_modem_work( &modem_dnp.buffer[i], modem_dnp.length-i );
						i += modem_dnp.length;
					}
					else
					{
						TB_dbg_modem( "ERROR. Read data from MODEM\r\n" );
						TB_log_append( LOG_TYPE_MODEM_DATA_ERROR, NULL, -1 );
						break;
					}
				}
			}

		}

		////////////////////////////////////////////////////////////////////////

		if( TB_msgq_recv( &recv_msg, MSG_TYPE_MODEM, NOWAIT) > 0 )
		{
			switch( recv_msg.id )
			{
				case MSG_CMD_MODEM_WRITE 	:
					if( recv_msg.data && recv_msg.size > 0 )
					{
						if( TB_dm_is_on(DMID_MODEM) )
							TB_util_data_dump( "Modem Write data", recv_msg.data, recv_msg.size );
						TB_modem_write( (TBUC *)recv_msg.data, recv_msg.size );
					}
					break;
					
				default	:
					TB_dbg_modem("Unknown Message ID : %d\r\n", recv_msg.id );
					break;
			}
		}

		TB_util_delay( 500 );
	}
}

pthread_t s_thid_modem;
void TB_modem_proc_start( void )
{
	pthread_create( &s_thid_modem, NULL, tb_modem_proc, NULL );
}

void TB_modem_proc_stop( void )
{
	s_proc_flag_modem = 0;
	pthread_join( s_thid_modem, NULL );
}

////////////////////////////////////////////////////////////////////////////////
#include "TB_test.h"
pthread_t 	g_thid_modem_test;
int 		g_proc_flag_modem_test = 1;
static void *tb_modem_test_proc( void *arg )
{
	time_t cur_t, old_t;
    cur_t = old_t = time(NULL);
	
	printf("===========================================\r\n" );
	printf("            Start MODEM TEST Proc          \r\n" );
	printf("===========================================\r\n" );

	int check = 0;
	TBUC buf1[32] = "1234567890";
	TBUC buf2[32] = "12345";
	TBUC buf_read[128];
	int normal = 0;
	TBUS len = 0;

	while( g_proc_flag_modem_test )
	{		
		cur_t = time(NULL);
        if( cur_t != old_t )
		{			
			TBUC buffer[128] = {0, };
			TBUC *p	= (check == 0) ? (TBUC *)&buf1[0] :	\
									 (TBUC *)&buf2[2];
			check = !check;
			
			snprintf( buffer, sizeof(buffer), "[%s] %s\r\n", "MODEM", p );
			len = TB_uart_mdm_write( buffer, wstrlen(buffer) );
			if( len == wstrlen(buffer) )
			{
				TB_dbg_modem( "MODEM WRITE = %s\r\n", buffer );
				normal |= SUCCESS_WRITE;
			}
			else
			{
				TB_dbg_modem( "ERROR. MODEM WRITE = %d vs %d\r\n", len, wstrlen(buffer) );
			}
			
			old_t = cur_t;
        }

		bzero( buf_read, sizeof(buf_read) );
		len = TB_uart_mdm_read( buf_read, sizeof(buf_read) ) ;
        if( len > 1 )
        {
			TB_dbg_modem( "MODEM READ = %s\r\n", buf_read );
			normal |= SUCCESS_READ;
        }

		if( (normal & (SUCCESS_WRITE|SUCCESS_READ)) == (SUCCESS_WRITE|SUCCESS_READ) )
		{
			TB_testmode_set_test_result( TEST_ITEM_MODEM, TRUE );
		}

		TB_util_delay( 2000 );
	}
}

void TB_modem_test_proc_start( void )
{
	pthread_create( &g_thid_modem_test, NULL, tb_modem_test_proc, NULL );
}

void TB_modem_test_proc_stop( void )
{
	g_proc_flag_modem_test = 0;
	pthread_join( g_thid_modem_test, NULL );
}


