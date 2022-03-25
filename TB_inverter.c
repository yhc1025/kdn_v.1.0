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
#include "TB_inverter.h"
#include "TB_wisun.h"
#include "TB_debug.h"
#include "TB_setup.h"

////////////////////////////////////////////////////////////////////////////////

TB_MB_RESPONSE *s_mb_response = NULL;

////////////////////////////////////////////////////////////////////////////////

#define SLAVEID_485A_MIN	1
#define SLAVEID_485A_MAX	20
#define SLAVEID_485B_MIN	21
#define SLAVEID_485B_MAX	40

////////////////////////////////////////////////////////////////////////////////

pthread_t 		s_thid_invt;
pthread_t 		s_thid_invt_each;
TB_CONDMUTEX	s_cm_invt;

////////////////////////////////////////////////////////////////////////////////

TB_INVT_INFO g_invt_each[MAX_INVT_CNT];
TBBL g_invt_each_run_flag = FALSE;

////////////////////////////////////////////////////////////////////////////////

int TB_rs485_init( void )
{
	int ret = -1;
	int init = 0;

	TB_ROLE role = TB_setup_get_role();
	
	if( role >= ROLE_RELAY_MIN && role <= ROLE_RELAY_MAX )
	{
		init = 1;
	}
	else if( role >= ROLE_TERM_MIN && role <= ROLE_TERM_MAX )
	{
		TB_COMM_TYPE comm_type = TB_setup_get_comm_type();
		if( comm_type.slave == COMM_MODE_SLAVE_RS485 )
		{
			init = 1;
		}
		else
		{
			TB_VOLTAGE vol = TB_setup_get_product_info_voltage();
			if( vol == VOLTAGE_HIGH )
				init = 1;
		}
	}

	if( init )
	{
		g_op_uart_invta.init();
		if( role >= ROLE_TERM_MIN && role <= ROLE_TERM_MAX )
		{
			if( TB_setup_get_optical_sensor() == FALSE )
				g_op_uart_invtb.init();
		}

		s_mb_response = TB_modbus_init();

		INIT_COND_MUTEX_V( s_cm_invt );
		TB_rs485_proc_start();
		ret = 0;
	}

	return ret;
}

static TBUC pkt_recv[MAX_MODBUS_BUF];
TBUC TB_rs485_read( TBUC rs485 )
{
	TBUS (*inverter_read) (TBUC *, TBUI) = (rs485 == 0) ? g_op_uart_invta.read :	\
														  g_op_uart_invtb.read;

	TBUS len_recv 		= 0;
	TBUS len_recv_total = 0;
	
	int wait_time 		= 0;
	int sleep_msec 		= 100;
	int timeout_cnt 	= 0;
	
	int setup_timeout 	= TB_setup_get_invt_timeout() * 100;
	int setup_retry   	= TB_setup_get_invt_retry();
	
	while( 1 )
	{
		len_recv = inverter_read( pkt_recv, sizeof(pkt_recv) );
		if( len_recv > 1 )	//	R/TX 전환 시 1byte가 읽힌다.
		{
			if( s_mb_response )
			{
				if( s_mb_response->push(pkt_recv, len_recv) == TRUE )
				{
#if 0
					if( TB_dm_is_on(DMID_INVERTER) )
						TB_util_data_dump( "INVERTER RECEIVE", s_mb_response->buffers(), s_mb_response->length() );
#endif
					len_recv_total += len_recv;

					/***********************************************************
					*	다음 slave id를 처리하기 전에 반드시 지연을 줘야한다.
					*
					*	Delay가 없으면 다음 slave id를 읽을 때 읽히지가 않는다.
					*	OK -> 0 -> OK -> 0 -> OK -> 0 .........
					*
					***********************************************************/
					TB_util_delay( TB_setup_get_invt_delay_read() );
					break;
				}
			}
		}
		else
		{
			len_recv = 0;
			TB_util_delay( sleep_msec );
			wait_time += sleep_msec;
		}

		////////////////////////////////////////////////////////////////////////

		if( wait_time >= setup_timeout )
		{
			timeout_cnt ++;
			if( timeout_cnt >= setup_retry )
			{
				break;
			}
			else
			{
				wait_time = 0;
			}
		}
	}

	return len_recv_total;
}

TBUS TB_rs485_write( TBUC rs485, TBUC *buf, int len )
{
	TBUS ret = 0;

	if( buf && len > 0 )
	{
		TBUS (*inverter_write) (void *, int) = (rs485 == 0) ? g_op_uart_invta.write : 	\
														      g_op_uart_invtb.write;
	
		ret = inverter_write( buf, len );
	}

	return ret;
}

int TB_rs485_run_cmd( TB_MB_FUNC func, TBUC *payload, TBUS pl_size, TBUC *buf, TBUC rs485 )
{
	int ret = 0;
	
	switch( func )
	{
		case FUNC_READ_HOLDING_REG :
			if( buf )
			{
				TBUC cmd[32] 	  = {0, };
				TBUC read_buf[64] = {0, };
				int  offset = 0;
				TBUC slvid = payload[0];
				TBUC invt_cmd = payload[1];
				
				cmd[offset++] = payload[0];
				cmd[offset++] = payload[1];
				cmd[offset++] = payload[2];
				cmd[offset++] = payload[3];
				cmd[offset++] = payload[4];
				cmd[offset++] = payload[5];
				
				TB_crc16_modbus_fill( cmd, offset );
				offset += CRC_SIZE;

				////////////////////////////////////////////////////////////////

				if( s_mb_response )
				{
					g_invt_each[slvid-1].count_try ++;
					if( g_invt_each[slvid-1].count_try == 0 )
					{
						g_invt_each[slvid-1].count_success = 0;
						g_invt_each[slvid-1].count_fail	= 0;
					}
					
					s_mb_response->filter( slvid, invt_cmd );
					TB_rs485_write( rs485, cmd, offset );
					if( TB_rs485_read(rs485) > 0 )
					{
						if( s_mb_response->length() > 1 )	//	RS485를 읽으면 가끔씩 1byte가 넘어온다.
						{
							bzero( buf, INVT_EACH_BUF_SIZE );
							wmemcpy( buf, INVT_EACH_BUF_SIZE, s_mb_response->buffers(), s_mb_response->length() );
							ret = s_mb_response->length();
						}
						else
						{
							ret = 0;
						}
						s_mb_response->reset();

						TB_dbg_inverter( "[%s] INVERTER READ : slaveid=%2d, read size=%2d\r\n", rs485==0 ? "RS485A":"RS485B", slvid, ret );
						g_invt_each[slvid-1].count_success ++;
					}
					else
					{
						TB_dbg_inverter( "[%s] Inverter Read TIMEOUT --> slvid = %d\r\n", rs485==0 ? "RS485A":"RS485B", slvid );
						g_invt_each[slvid-1].count_fail ++;
					}
				}
			}
			else
			{
				TB_dbg_inverter( "ERROR. inverter read buffer is NULL\r\n" );
			}
			break;
			
		default :
			TB_dbg_inverter( "UNKNOWN INVERTER COMMAND : %2d\r\n", func );
			ret = -1;
			break;
	}

	return ret;
}

void TB_rs485_deinit( void )
{
	g_op_uart_invta.deinit();
	g_op_uart_invtb.deinit();
}

////////////////////////////////////////////////////////////////////////////////

TBUC s_proc_flag_inverter = 1;

TBUC g_send_invt_buf[512] = {0, };
TBUS g_send_invt_buf_idx;

static void *tb_rs485_each_proc( void *arg )
{
	printf("===========================================\r\n" );
	printf("            Start RS485 Each Proc          \r\n" );
	printf("===========================================\r\n" );

	bzero( g_invt_each, sizeof(g_invt_each) );

	TB_MESSAGE 	msg;
	TBUC 		immedi_flag = 0;

	TB_INVT_INFO invt_buf;
	int invt_cnt = TB_setup_get_invt_count();
	while( s_proc_flag_inverter )
	{
		for( TBUC slvid=1; slvid<=invt_cnt; slvid++ )
		{
			TBUC rs485port = 0;

			if( slvid <= SLAVEID_485A_MAX )
			{
				rs485port = 0;
			}
			else
			{
				rs485port = 1;
			}

			TBUS reg_addr = (0x03E8);	//	1000
			TBUS reg_cont = (0x0016);	//	22

			TBUC payload[32];
			TBUS pl_size = 0;

			bzero( payload, sizeof(payload) );
			
			payload[pl_size++] = slvid;
			payload[pl_size++] = FUNC_READ_HOLDING_REG;
			payload[pl_size++] = (reg_addr >> 8) & 0xFF;
			payload[pl_size++] = (reg_addr >> 0) & 0xFF;
			payload[pl_size++] = (reg_cont >> 8) & 0xFF;
			payload[pl_size++] = (reg_cont >> 0) & 0xFF;

			MUTEX_LOCK( &s_cm_invt.mutex );
			
			invt_buf.size = TB_rs485_run_cmd( FUNC_READ_HOLDING_REG, payload, pl_size, invt_buf.buf, rs485port );
			if( invt_buf.size > 0 )
			{
				//	실제 데이터만 저장
				wmemcpy( g_invt_each[slvid-1].buf, sizeof(g_invt_each[slvid-1].buf), &invt_buf.buf[3], 44 );
				g_invt_each[slvid-1].size = 44;
			}
			else
			{
				g_invt_each[slvid-1].size = -1;
			}

			COND_SIGNAL( &s_cm_invt.cond );
			MUTEX_UNLOCK( &s_cm_invt.mutex );
			TB_util_delay( 50 );	//	Condition Wait와 Signal이 동작하기 위함.

			/*******************************************************************
			*	FUNC_DOUBLE_READ_REC_REG_IMMEDI(0x67) 명령 처리 순서
			*	2. Inverter 데이터를 읽고 있는 도중
			*	   "즉시인버터읽음" 명령이 수신한다.
			*      SlaveID 1부터 다시 읽도록 한다.
			*******************************************************************/
			if( TB_msgq_recv(&msg, MSG_TYPE_INVERTER_EACH, NOWAIT) > 0 )
			{
				if( msg.id == MSG_CMD_INVERTER_READ_IMMEDI_REQ )
				{
					TB_dbg_inverter("Recv Message --> MSG_TYPE_INVERTER_EACH : MSG_CMD_INVERTER_READ_IMMEDI_REQ\n" );
					immedi_flag = 1;
					slvid = 0;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////

		g_invt_each_run_flag = TRUE;

		////////////////////////////////////////////////////////////////////////

		if( immedi_flag == 1 )
		{
			/*******************************************************************
			*	FUNC_DOUBLE_READ_REC_REG_IMMEDI(0x67) 명령 처리 순서
			*	3. "즉시인버터읽음" 명령을 수행 후 완료되었음을 전송한다.
			*******************************************************************/
			static TBUC data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
			msg.type    = MSG_TYPE_INVERTER;
			msg.id		= FUNC_DOUBLE_READ_REC_REG_IMMEDI;
			msg.data	= data;
			msg.size	= 4;
			TB_msgq_send( &msg );

			TB_dbg_inverter("Send Message --> MSG_TYPE_INVERTER : FUNC_DOUBLE_READ_REC_REG_IMMEDI\n" );

			immedi_flag = 0;
		}

		////////////////////////////////////////////////////////////////////////
#if 0
		TB_util_delay( 10000 );
#else
		{
			TBUI sum = 0;
			TBUI delay = 100;
			while( 1 )
			{
				/***************************************************************
				*	FUNC_DOUBLE_READ_REC_REG_IMMEDI(0x67) 명령 처리 순서
				*	2. Inverter 데이터를 읽고 대기하는 중
				*		"즉시인버터읽음" 명령이 수신한다.
				*      	SlaveID 1부터 다시 읽도록 한다.
				***************************************************************/
				if( TB_msgq_recv( &msg, MSG_TYPE_INVERTER_EACH, NOWAIT) > 0 )
				{
					if( msg.id == MSG_CMD_INVERTER_READ_IMMEDI_REQ )
					{
						TB_dbg_inverter("Recv Message --> MSG_TYPE_INVERTER_EACH : MSG_CMD_INVERTER_READ_IMMEDI_REQ\n" );
						immedi_flag = 1;
						break;
					}
				}

				////////////////////////////////////////////////////////////////
				
				TB_util_delay( delay );
				sum += delay;
				if( sum >= 10000 )
					break;

				/***************************************************************
				*	CAUTION1. COND_WAIT를 해제하기 위함.
				***************************************************************/
				COND_SIGNAL( &s_cm_invt.cond );
			}
		}
#endif
	}
}

static int tb_rs485_send_msg_data_write( TBUC *p_buf, TBUS size )
{
	int ret = -1;

	if( p_buf && size > 0 )
	{
		int try_cnt = 0;
		int check_recv = 0;

		TB_MESSAGE msg_send;
		TB_MESSAGE msg_recv;

		TB_VOLTAGE 	 vol 	   = TB_setup_get_product_info_voltage();
		TB_COMM_TYPE comm_type = TB_setup_get_comm_type();
		while( 1 )
		{
			check_recv = 0;

			if( comm_type.master == COMM_MODE_MASTER_RS232 || vol == VOLTAGE_HIGH )
			{			
				msg_send.type 	= MSG_TYPE_PSU_MOD;
				msg_send.id		= MSG_CMD_PSU_MOD_WRITE;

				TB_dbg_inverter("Send Message : MSG_TYPE_PSU_MOD, MSG_CMD_PSU_MOD_WRITE (try count = %d) \r\n", try_cnt+1);
			}
			else if( comm_type.master == COMM_MODE_MASTER_WISUN )
			{
				TB_ROLE role = TB_setup_get_role();
				
				if( role >= ROLE_RELAY_MIN && role <= ROLE_RELAY_MAX )
				{
					msg_send.type 	= MSG_TYPE_WISUN_TERM2RELAY;
					msg_send.id	 	= MSG_CMD_WISUN_INVERTER_DATA_RES;

					TB_dbg_inverter("Send Message : MSG_TYPE_WISUN_TERM2RELAY, MSG_CMD_WISUN_INVERTER_DATA_RES (try count = %d) \r\n", try_cnt+1);
				}
				else if( role >= ROLE_TERM_MIN && role <= ROLE_TERM_MAX )
				{
					msg_send.type 	= MSG_TYPE_WISUN_GGW2TERM;
					msg_send.id	 	= MSG_CMD_WISUN_INVERTER_DATA_RES;

					TB_dbg_inverter("Send Message : MSG_TYPE_WISUN_TERM2RELAY, MSG_CMD_WISUN_INVERTER_DATA_RES (try count = %d) \r\n", try_cnt+1);
				}
			}
			else if( comm_type.master == COMM_MODE_MASTER_LTE )
			{
				msg_send.type 	= MSG_TYPE_LTE;
				msg_send.id		= MSG_CMD_LTE_WRITE;

				TB_dbg_inverter("Send Message : MSG_TYPE_LTE, MSG_CMD_LTE_WRITE (try count = %d) \r\n", try_cnt+1);
			}

			msg_send.data	= p_buf;
			msg_send.size	= size;
			TB_msgq_send( &msg_send );

			////////////////////////////////////////////////////////////////////

			/*******************************************************************
			* 	CAUTION2. PSU-MODBUS에서 Message를 받고, write를 마칠 때까지 기다린다.
			*******************************************************************/
			if( comm_type.master == COMM_MODE_MASTER_RS232 || vol == VOLTAGE_HIGH )
			{			
				if( TB_msgq_recv( &msg_recv, MSG_TYPE_PSU_MOD_READY, WAIT) > 0 )
				{
					if( msg_recv.id == MSG_CMD_PSU_MOD_WRITE_READY )
					{
						check_recv = 1;
						ret = 0;

						/******************************************************
						* PSU에서 수신 및 수신데이터 처리시간을 확보하기 위해서.
						*******************************************************/
						TB_util_delay( TB_setup_get_invt_delay_write() );
						break;
					}
				}
			}
			/*******************************************************************
			* CAUTION3. Wisun에서 Message를 받은 후, 상위 Wisun으로 전송할 때까지 기다린다.
			*******************************************************************/
			else if( comm_type.master == COMM_MODE_MASTER_WISUN )
			{
				if( TB_msgq_recv( &msg_recv, MSG_TYPE_INVERTER, WAIT) > 0 )
				{
					if( msg_recv.id == MSG_CMD_INVERTER_WISUN_READY )
					{
						check_recv = 1;
						ret = 0;

						TB_dbg_inverter("OK : Ready ******** MSG_TYPE_WISUN_TERM2RELAY, MSG_CMD_WISUN_INVERTER_DATA_RES\r\n");

						/*******************************************************
						*	20210403 - Message Queue 이상 디버깅
						* 		Wisun에서 수신 및 수신데이터를 단말장치로
						* 		전송하고, 단말장치에서 수신한 데이터를 PSU-Modbus로
						* 		Write하기까지 소요되는 시간동안 지연을 둔다.
						*******************************************************/
						TB_util_delay( TB_setup_get_invt_delay_write() );
						break;
					}
				}
			}
			/*******************************************************************
			* LTE Module에서 Message를 받은 후, LTE에서 전송할 때까지 기다린다.
			*******************************************************************/
			else if( comm_type.master == COMM_MODE_MASTER_LTE )
			{
				if( TB_msgq_recv( &msg_recv, MSG_TYPE_LTE_READY, WAIT) > 0 )
				{
					if( msg_recv.id == MSG_CMD_LTE_WRITE_READY )
					{
						check_recv = 1;
						ret = 0;

						/******************************************************
						* LTE에서 수신 및 수신데이터 처리시간을 확보하기 위해서.
						*******************************************************/
						TB_util_delay( TB_setup_get_invt_delay_write() );
						break;
					}
				}
			}

			////////////////////////////////////////////////////////////////////

			if( check_recv == 1 )
				break;
			else
				try_cnt ++;
		}
	}

	return ret;
}

#define IDX_SLAVE_ADDR		0
#define IDX_FUNC_CODE		1
#define IDX_FRAME_TYPE		2
#define IDX_FRAME_SEQ_NUM	3
#define IDX_INVT_CNT_LOW	4
#define IDX_INVT_CNT_HIGH	5
#define IDX_INVT_BYTE_CNT	6
#define IDX_DATA_START		7

#define FRM_TYPE_SINGLE 		0x01
#define FRM_TYPE_MULTI_START 	0x02
#define FRM_TYPE_MULTI_MIDDLE	0x03
#define FRM_TYPE_MULTI_END 		0x04

static void tb_rs485_init_invt_total_buffer( TBUC func, TBUC *buf, TBUS buf_size )
{
	if( buf )
	{
		TB_ROLE role = TB_get_role();
		
		bzero( buf, buf_size );
		buf[IDX_SLAVE_ADDR] = (role == ROLE_RELAY1) ? SLAVEID_RELAY1 :	\
							  (role == ROLE_RELAY2) ? SLAVEID_RELAY2 :	\
							  (role == ROLE_RELAY3) ? SLAVEID_RELAY3 :	\
										              SLAVEID_RELAY1;
		buf[IDX_FUNC_CODE] 		= func;		//	FUNC_DOUBLE_READ_REC_REG;
		buf[IDX_FRAME_TYPE] 	= 0x01;		//	Single Frame
		buf[IDX_FRAME_SEQ_NUM] 	= 0x01;		//	First Frame
		buf[IDX_INVT_CNT_LOW] 	= 0;		//	interter count - low
		buf[IDX_INVT_CNT_HIGH] 	= 0;		//	interter count - high
		buf[IDX_INVT_BYTE_CNT] 	= 0;		//	Byte count
	}
}

int TB_rs485_send_invt_info( TBUC func, TBUS invt_slave_id )
{
	int ret = -1;
	
	if( g_invt_each_run_flag == TRUE )
	{
		const TBUS MAX_INVT_SEND_CNT = 5;	//	1개의 패킷에 보내는 Inverter 정보의 갯수

		int i;
		int start, end;
		
		TBUC send_cnt	 = 0;	//	Frame Send Count
		int  check_cnt	 = 0;	//	Invert Count
		int  invt_cnt 	 = 0;
		TBUC invt_byte_cnt = 0;

		TBUC invt_total_buf[512] = {0, };
		TBUS invt_total_buf_idx = 0;		

		MUTEX_LOCK( &s_cm_invt.mutex );
		/***********************************************************************
		*	CAUTION1. COND_WAIT를 해제하기 위함.
		***********************************************************************/
		COND_WAIT( &s_cm_invt.cond, &s_cm_invt.mutex );
		
		/***********************************************************************
		*						Inverter Buffer Init
		***********************************************************************/
		tb_rs485_init_invt_total_buffer( func, invt_total_buf, sizeof(invt_total_buf) );
		if( invt_slave_id == 0x00 )	//	그룹AI계측
		{
			start = SLAVEID_485A_MIN;
			end	  = SLAVEID_485B_MAX;
		}
		else
		{
			start = invt_slave_id;
			end   = invt_slave_id;
		}
		
		invt_total_buf_idx = IDX_DATA_START;

		////////////////////////////////////////////////////////////////////////
		//				전체 전송할 횟수를 미리 계산한다.
		////////////////////////////////////////////////////////////////////////
		int valid_count = 0 ;
		int total_send_count = 0;
		for( i=start; i<=end; i++ )
		{
			if( g_invt_each[i-1].size != 0 )
			{
				valid_count ++;
			}
		}

		total_send_count = valid_count / MAX_INVT_SEND_CNT;
		if( (valid_count % MAX_INVT_SEND_CNT) != 0 )
			total_send_count ++;

		TB_dbg_inverter( "total_send_count=%d\r\n", total_send_count );

		////////////////////////////////////////////////////////////////////////
		
		for( i=start; i<=end; i++ )
		{
			TBSC each_size = g_invt_each[i-1].size;

			if( each_size == 0 )
				continue;
			
			invt_total_buf[invt_total_buf_idx++] = (i>>8) & 0xFF;		//	Slave ID
			invt_total_buf[invt_total_buf_idx++] = (i>>0) & 0xFF;		//	Slave ID
			if( each_size > 0 )
			{	
				invt_total_buf[invt_total_buf_idx++] = (0>>8) & 0xFF;;	//	Success : 0, Fail : 1
				invt_total_buf[invt_total_buf_idx++] = (0>>0) & 0xFF;;	//	Success : 0, Fail : 1
				wmemcpy( &invt_total_buf[invt_total_buf_idx], sizeof(invt_total_buf)-invt_total_buf_idx, g_invt_each[i-1].buf, g_invt_each[i-1].size );

				check_cnt ++;
			}
			else
			{
				each_size = 44;
				invt_total_buf[invt_total_buf_idx++] = (1>>8) & 0xFF;	//	Success : 0, Fail : 1
				invt_total_buf[invt_total_buf_idx++] = (1>>0) & 0xFF;	//	Success : 0, Fail : 1
				bzero( &invt_total_buf[invt_total_buf_idx], each_size );
			}								
			invt_total_buf_idx += each_size;

			////////////////////////////////////////////////////////////////////
			
			invt_cnt ++;
			invt_byte_cnt += (each_size + 2 + 2);	//	44+2+2 : 2, 2 is Slave ID + Success/Fail
			
			invt_total_buf[IDX_INVT_CNT_LOW]  = (invt_cnt>>8) & 0xFF;
			invt_total_buf[IDX_INVT_CNT_HIGH] = (invt_cnt>>0) & 0xFF;
			invt_total_buf[IDX_INVT_BYTE_CNT] = invt_byte_cnt;

			////////////////////////////////////////////////////////////////////
			
			check_cnt = (invt_total_buf[IDX_INVT_CNT_LOW]  << 8) |	\
						(invt_total_buf[IDX_INVT_CNT_HIGH] << 0);
			if( check_cnt != 0 && check_cnt % MAX_INVT_SEND_CNT == 0 )
			{
				TB_MESSAGE msg;

				send_cnt ++;
				TB_dbg_inverter( "******** total_send_count=%d, check_cnt=%d, send_cnt=%d\r\n", total_send_count, check_cnt, send_cnt );

				/***************************************************************
				*	invt_slave_id는 0x00 또는 Inverter SlaveID를 지정하여 들어온다.
				*	즉, 그룹(0x00) 또는 1,...,40까지의 숫자가 지정된다.
				****************************************************************/

				switch( send_cnt )
				{
					////////////////////////////////////////////////////////////
					//	if invt_slave_id == 0 then group inverter, multi frame
					//	if invt_slave_id != 0 then each inverter, single frame
					////////////////////////////////////////////////////////////
					case 1	:	if( invt_slave_id == 0x00 )
								{
									if( total_send_count == send_cnt )
										invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_SINGLE;
									else
										invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_MULTI_START;
								}
								else
								{
									invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_SINGLE;
								}
								break;
					case 2	:
					case 3	:
					case 4	:
					case 5	:
					case 6	:
					case 7	:   invt_total_buf[IDX_FRAME_TYPE] = ( total_send_count != send_cnt ) ? FRM_TYPE_MULTI_MIDDLE :	\
																									FRM_TYPE_MULTI_END;
								break;
					case 8	:	invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_MULTI_END;
								break;
					default :	TB_dbg_inverter( "Error. Invalid count = %d\r\n", send_cnt );
								break;
				}

				////////////////////////////////////////////////////////////////

				invt_total_buf[IDX_FRAME_SEQ_NUM] = send_cnt;
				
				////////////////////////////////////////////////////////////////

				TB_crc16_modbus_fill( invt_total_buf, invt_total_buf_idx );
				invt_total_buf_idx += CRC_SIZE;

				g_send_invt_buf[0] = 0;	//	NULL
				wmemcpy( g_send_invt_buf, sizeof(g_send_invt_buf), invt_total_buf, invt_total_buf_idx );
				g_send_invt_buf_idx = invt_total_buf_idx;
			
				if( TB_dm_is_on(DMID_INVERTER) )
					TB_util_data_dump1( g_send_invt_buf, g_send_invt_buf_idx );

				tb_rs485_send_msg_data_write( g_send_invt_buf, g_send_invt_buf_idx );

				/***************************************************************
				*				Inverter Buffer Init
				***************************************************************/
				invt_cnt 	  = 0;
				invt_byte_cnt = 0;
				
				tb_rs485_init_invt_total_buffer( func, invt_total_buf, sizeof(invt_total_buf) );
				invt_total_buf_idx = IDX_DATA_START;
			}
		}

		////////////////////////////////////////////////////////////////////////

		check_cnt = (invt_total_buf[IDX_INVT_CNT_LOW] << 8) | (invt_total_buf[IDX_INVT_CNT_HIGH] << 0);							
		if( check_cnt != 0 && check_cnt % 5 != 0 )
		{
			TB_MESSAGE msg;

			send_cnt ++;
			TB_dbg_inverter( "******** check_cnt=%d, send_cnt=%d\r\n", check_cnt, send_cnt );
			switch( send_cnt )
			{
				////////////////////////////////////////////////////////////////
				//	if invt_slave_id == 0 then group inverter, multi frame
				//	if invt_slave_id != 0 then each inverter, single frame
				////////////////////////////////////////////////////////////////
				case 1	:	if( invt_slave_id == 0x00 )
							{
								if( total_send_count == send_cnt )
									invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_SINGLE;
								else
									invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_MULTI_START;
							}
							else
							{
								invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_SINGLE;
							}
							break;
				case 2	:
				case 3	:
				case 4	:
				case 5	:
				case 6	:
				case 7	:   invt_total_buf[IDX_FRAME_TYPE] = ( total_send_count != send_cnt ) ? FRM_TYPE_MULTI_MIDDLE : \
																								FRM_TYPE_MULTI_END;
							break;
				case 8	:	invt_total_buf[IDX_FRAME_TYPE] = FRM_TYPE_MULTI_END;
							break;
				default :	TB_dbg_inverter( "Error. Invalid count = %d\r\n", send_cnt );
							break;
			}

			////////////////////////////////////////////////////////////////////

			invt_total_buf[IDX_FRAME_SEQ_NUM] = send_cnt;

			////////////////////////////////////////////////////////////////////
			
			TB_crc16_modbus_fill( invt_total_buf, invt_total_buf_idx );
			invt_total_buf_idx += CRC_SIZE;

			g_send_invt_buf[0] = 0; //NULL;
			wmemcpy( g_send_invt_buf, sizeof(g_send_invt_buf), invt_total_buf, invt_total_buf_idx );
			g_send_invt_buf_idx = invt_total_buf_idx;

			////////////////////////////////////////////////////////////////////
		
			if( TB_dm_is_on(DMID_INVERTER) )
				TB_util_data_dump1( g_send_invt_buf, g_send_invt_buf_idx );

			tb_rs485_send_msg_data_write( g_send_invt_buf, g_send_invt_buf_idx );
		}

		MUTEX_UNLOCK( &s_cm_invt.mutex );

		ret = 0;
	}
	else
	{
		TB_dbg_inverter( "Inverter data is Not ready yet\r\n" );
	}

	return ret;
}

static void *tb_rs485_proc( void *arg )
{
	TB_MESSAGE msg;

	printf("===========================================\r\n" );
	printf("              Start RS485 Proc             \r\n" );
	printf("===========================================\r\n" );

	while( s_proc_flag_inverter )
	{
		if( TB_msgq_recv( &msg, MSG_TYPE_INVERTER, NOWAIT) > 0 )
		{
			TBUC *payload = msg.data;
			TBUS pl_size  = msg.size;

			TB_dbg_inverter( "Message Recived : 0x%02X\r\n", msg.id );
			
			switch( msg.id )
			{
				case FUNC_DOUBLE_READ_REC_REG_IMMEDI :
					{
						/*******************************************************
						*	FUNC_DOUBLE_READ_REC_REG_IMMEDI(0x67) 명령 처리 순서
						*	4. inverter_each_proc에서 인버터 데이터를 모두
						*	읽은 후 완료되었음을 알린다.
						*	즉시 업데이트된 인버터 데이터를 전송한다.
						********************************************************/
						if( payload[0] == 0xFF && payload[1] == 0xFF && payload[2] == 0xFF && payload[3] == 0xFF )
						{
							TB_rs485_send_invt_info( FUNC_DOUBLE_READ_REC_REG_IMMEDI, 0 );
						}
						else
						{
							/***************************************************
							*	FUNC_DOUBLE_READ_REC_REG_IMMEDI(0x67) 명령 처리 순서
							*	1. 단말장치에서 보낸 명령을 수신 후 
							*	inverter_each_proc 로 명령 메세지를 전송한다.
							***************************************************/
							TB_MESSAGE msg1;

							msg1.type   = MSG_TYPE_INVERTER_EACH;
							msg1.id		= MSG_CMD_INVERTER_READ_IMMEDI_REQ;
							msg1.data	= NULL;
							msg1.size	= 0;
							TB_msgq_send( &msg1 );

							TB_dbg_inverter("Send Message --> MSG_TYPE_INVERTER_EACH : MSG_CMD_INVERTER_READ_IMMEDI_REQ\n" );
						}
					}
					break;
					
				case FUNC_DOUBLE_READ_REC_REG :
					{
						TBUS invt_slave_id = (payload[3] << 8) | payload[2];
						TB_rs485_send_invt_info( FUNC_DOUBLE_READ_REC_REG, invt_slave_id );
					}
					break;
					
				case FUNC_WRITE_SINGLE_REG		:
					TB_dbg_inverter( "COMMAND : FUNC_WRITE_SINGLE_REG to Inverters(GROUP / NO return)\r\n" );
					if( TB_dm_is_on(DMID_INVERTER) )
						TB_util_data_dump1( payload, pl_size );
					/*
					*	그룹제어 이기 때문에 485포트에 모두 Write 한다.
					*	현재는 응답을 하지 않는다.
					*/
					TB_rs485_write( 0, payload, pl_size );
					TB_rs485_write( 1, payload, pl_size );
					break;

				case FUNC_WRITE_MULTIPLE_REG    :
					TB_dbg_inverter( "COMMAND : FUNC_WRITE_MULTIPLE_REG to Inverters(GROUP / NO return)\r\n" );
					if( TB_dm_is_on(DMID_INVERTER) )
						TB_util_data_dump1( payload, pl_size );
					/*
					*	그룹제어 이기 때문에 485포트에 모두 Write 한다.
					*	현재는 응답을 하지 않는다.
					*/
					TB_rs485_write( 0, payload, pl_size );
					TB_rs485_write( 1, payload, pl_size );
					break;
				default :
					TB_dbg_inverter( "Error. Invalid Function Code = 0x%02X\r\n", msg.id );
					break;
			}
		}
		
		TB_util_delay( 500 );
	}
}

void TB_rs485_proc_start( void )
{
	pthread_create( &s_thid_invt, NULL, tb_rs485_proc, NULL );
	pthread_create( &s_thid_invt_each, NULL, tb_rs485_each_proc, NULL );
}

void TB_rs485_proc_stop( void )
{
	s_proc_flag_inverter = 0;
	pthread_join( s_thid_invt, NULL );
	pthread_join( s_thid_invt_each, NULL );
}

TBSC TB_rs485_each_info_datasize( int idx )
{
	TBSC size = -1;
	if( idx < MAX_INVT_CNT )
	{
		size = g_invt_each[idx].size;
	}

	return size;
}

TB_INVT_INFO *TB_rs485_each_info( int idx )
{
	TB_INVT_INFO *info = NULL;
	if( idx < MAX_INVT_CNT )
	{
		info = &g_invt_each[idx];
	}

	return info;
}

////////////////////////////////////////////////////////////////////////////////
#include "TB_test.h"
int 		g_proc_flag_inverter_test = 1;
pthread_t 	g_thid_inverter_test;
static void *tb_rs485_test_proc( void *arg )
{
	TB_MESSAGE msg;

	time_t cur_t, old_t;
    cur_t = old_t = time(NULL);
	
	printf("===========================================\r\n" );
	printf("           Start INVERTER TEST Proc        \r\n" );
	printf("===========================================\r\n" );

	static int s_check = 0;
	TBUC buf1[32] = "1234567890";
	TBUC buf2[32] = "12345";
	TBUC bufa[128];
	TBUC bufb[128];
	int normala = 0;
	int normalb = 0;
	while( g_proc_flag_inverter_test )
	{
		cur_t = time( NULL );
        if( cur_t != old_t )
		{
			TBUC buffer[128] = {0, };
			int  s = 0;
			TBUC *p	= (s_check == 0) ? (TBUC *)&buf1[0] :	\
									   (TBUC *)&buf2[0];
			s_check = !s_check;

			snprintf( buffer, sizeof(buffer), "[%s] %s\r\n", "RS485A", p );
			s = wstrlen( buffer );
			if( g_op_uart_invta.write( buffer, s ) > 1  )
			{
				normalb |= 0x01;
			}

			snprintf( buffer, sizeof(buffer), "[%s] %s\r\n", "RS485B", p );
			s = wstrlen( buffer );
			if( g_op_uart_invtb.write( buffer, s ) > 1 )
			{
				normalb |= 0x01;
			}
			
			old_t = cur_t;
        }

		bzero( bufa, sizeof(bufa) );
		TBUS lena = g_op_uart_invta.read( bufa, sizeof(bufa) );
		if( lena > 1 )
		{
			normala |= 0x02;
			//TB_util_data_dump( "[READ 485A]", bufa, lena );
		}

		bzero( bufb, sizeof(bufb) );
		TBUS lenb = g_op_uart_invtb.read( bufb, sizeof(bufb) );
		if( lenb > 1 )
		{
			normalb |= 0x02;
			//TB_util_data_dump( "[READ 485B]", bufb, lenb );
		}

		if( normala & (0x01 | 0x02) )
		{
			TB_testmode_set_test_result( TEST_ITEM_RS485A, TRUE );
		}

		if( normalb & (0x01 | 0x02) )
		{
			TB_testmode_set_test_result( TEST_ITEM_RS485B, TRUE );
		}

		TB_util_delay( 500 );
	}
}
void TB_rs485_test_proc_start( void )
{
	pthread_create( &g_thid_inverter_test, NULL, tb_rs485_test_proc, NULL );
}

void TB_rs485_test_proc_stop( void )
{
	g_proc_flag_inverter_test = 0;
	pthread_join( g_thid_inverter_test, NULL );
}


