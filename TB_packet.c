#include "TB_util.h"
#include "TB_j11.h"
#include "TB_j11_util.h"
#include "TB_wisun.h"
#include "TB_debug.h"
#include "TB_kcmvp.h"
#include "TB_packet.h"

////////////////////////////////////////////////////////////////////////////////

const TBUS MAX_PAYLOAD_SIZE = 1000;

////////////////////////////////////////////////////////////////////////////////

TB_PACKET	g_packet;
TBUC 		s_packet_dir[2] = {WISUN_DIR_MAX, WISUN_DIR_MAX};
TBUC		s_packet_dest_ip[2][16];

////////////////////////////////////////////////////////////////////////////////

int TB_packet_dump( TB_PACKET *p_packet )
{
	int ret = -1;

	if( p_packet )
	{
		void *p = p_packet->data;
		int	idx = 0;
		int pl_size = 0;

		TB_PACKET_HEAD *p_head = (TB_PACKET_HEAD *)(p + idx);

		TBUS 		TLENGTH = p_head->TLENGTH;
		TB_SERIAL	SN		= p_head->SN;

		printf("***** PACKET DUMP START ***** \r\n" );
		printf( "	SOP         = 0x%04X\r\n", p_head->SOP );
		printf( "	VER         = 0x%02X\r\n", p_head->VER );
		printf( "	CTL.FSN     = 0x%02X\r\n", p_head->CTL.FSN );
		printf( "	CTL.PTYPE   = 0x%02X\r\n", p_head->CTL.PTYPE );
		printf( "	LENGTH      = %d\r\n",     p_head->LENGTH );
		printf( "	TLENGTH     = %d\r\n",     TLENGTH );
		printf( "	SN.MSB      = 0x%02X\r\n", SN.MSB );
		printf( "	SN.num      = 0x%02X\r\n", SN.NUM );
		printf( "	CMD         = 0x%02X\r\n", p_head->CMD );
		idx += sizeof(TB_PACKET_HEAD);

		pl_size = p_head->TLENGTH;
		printf( "	payload     = %d byte\r\n", pl_size );
		idx += pl_size;

		TB_PACKET_TAIL *p_tail = (TB_PACKET_TAIL *)(p + idx);
		printf( "	CRC         = 0x%04X\r\n", p_tail->CRC16 );
		printf( "	EOP         = 0x%04X\r\n", p_tail->EOP );
		printf("***** PACKET DUMP END *****\r\n");

		ret = 0;
	}

	return ret;
}

int TB_packet_make( TB_PACKET *p_packet, TBUS cmd, void *p_pl, TBUS pl_size, TBUS pl_tsize, int pk_cnt, int pk_idx )
{
	int ret = -1;	

	if( p_packet )
	{
		int idx = 0;
		TB_PACKET_HEAD head;
		TB_PACKET_TAIL tail;

		TB_dbg_packet("Make Packet : CMD = 0x%02X, pl_size=%d, pl_tsize=%d\r\n", cmd, pl_size, pl_tsize );

		bzero( p_packet, sizeof(TB_PACKET) );
		
		head.SOP 		= PACKET_SOP;
		head.VER		= PACKET_VER;		
		head.CTL.FSN	= (pk_cnt - pk_idx) - 1;
		head.CTL.PTYPE 	= (pk_cnt > 1) ? 1 : 0;
		head.LENGTH		= 6 + pl_size;	//	6 is Total LENGTH size + SN size + CMD size
		head.TLENGTH	= pl_tsize;
		head.SN.MSB		= (pk_idx != 0) ? 1 : 0;
		head.SN.NUM		= 0;
		head.CMD		= cmd;

		/*
		*	Copy a packet head data
		*/		
		wmemcpy( &p_packet->data[idx], sizeof(p_packet->data)-idx, &head, sizeof(head) );
		idx += sizeof(head);

		/*
		*	Copy a payload data
		*/
		if( p_pl && pl_size > 0 )
		{
			wmemcpy( &p_packet->data[idx], sizeof(p_packet->data[idx])-idx, p_pl, pl_size );
			idx += pl_size;			
		}

		/*
		*	Copy a packet tail data
		*/
		tail.CRC16 	= TB_crc16_modbus_get( &p_packet->data[2], sizeof(head) + pl_size - 2 );
		tail.EOP 	= PACKET_EOP;
		wmemcpy( &p_packet->data[idx], sizeof(p_packet->data)-idx, &tail, sizeof(tail) );
		idx += sizeof(tail);

		/*
		*	Set packet's size data
		*/
		p_packet->size = idx;

//		if( TB_dm_is_on(DMID_WISUN) )
//			TB_packet_dump( p_packet );

		ret = 0;
	}

	return ret;
}

void TB_packet_set_send_direction( int idx, TBUC dir )
{
	if( dir > WISUN_DIR_MIN && dir < WISUN_DIR_MAX )
	{		
		s_packet_dir[idx] = dir;
		TB_packet_set_dest_ip( idx, dir );
	}
}

TBUC TB_packet_get_send_direction( int idx )
{
	return s_packet_dir[idx];
}

void TB_packet_set_dest_ip( int idx, TBUC dir )
{
	int i;
	int endd_idx;
	switch( dir )
	{
		case WISUN_DIR_TERM2GGW  	:	endd_idx = 0;	break;
		
		case WISUN_DIR_GGW2TERM1  	:	endd_idx = 0;	break;
		case WISUN_DIR_GGW2TERM2  	:	endd_idx = 1;	break;
		case WISUN_DIR_GGW2TERM3  	:	endd_idx = 2;	break;
		case WISUN_DIR_GGW2REPEATER	:	endd_idx = 3;	break;
		
		case WISUN_DIR_TERM2RELAY1 	:	endd_idx = 0;	break;
		case WISUN_DIR_TERM2RELAY2 	:	endd_idx = 1;	break;
		case WISUN_DIR_TERM2RELAY3 	:	endd_idx = 2;	break;
		
		case WISUN_DIR_RELAY2TERM  	:	endd_idx = 0;	break;
		default						: 	TB_dbg_j11( "Unknow wisun send direction = %d\r\n", dir );
										return;
	}

#if 1
	wmemcpy( s_packet_dest_ip[idx], sizeof(s_packet_dest_ip[idx]), &g_ip_adr[idx][endd_idx][0], LEN_IP_ADDR );
#else
	for( i=0; i<16; i++ )
	{
		s_packet_dest_ip[idx][i] = g_ip_adr[idx][endd_idx][i];
	}
#endif

	if( TB_dm_is_on(DMID_PACKET) )
	{
		TB_dbg_packet( "idx=%d, SET DEST IP = \n", idx );
		
		TB_dbg_packet( "     s_packet_dest_ip = " );
		for( i=0; i<16; i++ )	TB_prt_packet( "%02X ", s_packet_dest_ip[idx][i] );		TB_prt_packet("\r\n" );
		
		TB_dbg_packet( "     g_ip_adr         = " );
		for( i=0; i<16; i++ )	TB_prt_packet( "%02X ", g_ip_adr[idx][endd_idx][i] );	TB_prt_packet("\r\n" );
	}	
}

TBUC *TB_packet_get_dest_ip( int idx )
{
	return &s_packet_dest_ip[idx][0];
}

int TB_packet_make_kcmvp( TB_PACKET *p_packet )
{
	int ret = -1;

	if( p_packet && p_packet->size > 0 )
	{
		int offset = 0;
		TB_KCMVP_DATA in, out;
		TB_KCMVP_KEY *p_tag = NULL;

		bzero( &in,  sizeof(in) );
		bzero( &out, sizeof(out) );

		wmemcpy( in.data, sizeof(in.data), p_packet->data, p_packet->size );
		in.size = p_packet->size;

		TB_kcmvp_encryption( &in, &out );
		p_tag = TB_kcmvp_get_keyinfo_tag();

		if( p_tag && p_tag->size > 0 )
		{
			wmemcpy( &p_packet->data[offset], sizeof(p_packet->data)-offset, &p_tag->data[0], p_tag->size );
			offset += p_tag->size;

			if( out.data && out.size > 0 )
			{
				wmemcpy( &p_packet->data[offset], sizeof(p_packet->data)-offset, &out.data[0], out.size );
				offset += out.size;

				ret = 0;
			}
		}
		
		p_packet->size = offset;
	}

	return ret;
}

int TB_packet_send( int idx, TBUS cmd, void *data, TBUS size )
{
	int		ret 	= -1;
	void *	p_pl 	= data;
	TBUS	pl_size = size;

	if( pl_size > MAX_PAYLOAD_SIZE )
	{
		TBUC *frag_data = (TBUC *)p_pl;
		TBUS frag_size  = 0;
		TBUC pk_cnt     = (pl_size / MAX_PAYLOAD_SIZE) + 1;
		int i;

		for( i=0; i<pk_cnt; i++ )
		{
			if( i < pk_cnt-1 )
				frag_size = MAX_PAYLOAD_SIZE;
			else
				frag_size = (pl_size % MAX_PAYLOAD_SIZE);				

			if( TB_packet_make( &g_packet, cmd, frag_data, frag_size, pl_size, pk_cnt, i ) == 0 )
			{
				TB_packet_make_kcmvp( &g_packet );
				ret = TB_j11_cmd_udpsend( idx, g_packet.data, g_packet.size );
			}

			frag_data += frag_size;
		}

		ret = 0;
	}
	else
	{
		if( TB_packet_make( &g_packet, cmd, p_pl, pl_size, pl_size, 1, 0 ) == 0 )
		{
			TB_packet_make_kcmvp( &g_packet );
			ret = TB_j11_cmd_udpsend( idx, g_packet.data, g_packet.size );
		}
	}

	return ret;
}

TBBL TB_packet_check( TB_PACKET *p_packet )
{
	TBBL check = FALSE;

	if( p_packet )
	{	
		TB_PACKET_HEAD *p_head = (TB_PACKET_HEAD *)p_packet->data;
		if( p_head )
		{
			if( p_head->SOP == PACKET_SOP && p_head->VER == PACKET_VER )
			{
				TBUS pl_size = p_head->LENGTH - 6;
				TB_PACKET_TAIL *p_tail = (TB_PACKET_TAIL *)&p_packet->data[sizeof(TB_PACKET_HEAD) + pl_size];
				if( p_tail->EOP == PACKET_EOP )
				{
					TBUS crc = TB_crc16_modbus_get( &p_packet->data[2], sizeof(TB_PACKET_HEAD) + pl_size - 2 );

					if( p_tail->CRC16 == crc )
					{
						TB_dbg_packet("Valid Packet : CMD = 0x%02X\r\n", p_head->CMD );
						check = TRUE;
					}
					else
					{
						TB_dbg_packet("Invalid Packet3\r\n" );
					}
				}
				else
				{
					TB_dbg_packet("Invalid Packet2(0x%02X : 0x%02X)\r\n", p_tail->EOP, PACKET_EOP );
				}
			}
			else
			{
				TB_dbg_packet("Invalid Packet1\r\n" );
			}

			if( check == FALSE )
			{
				TB_dbg_packet("Invalid Packet : CMD = 0x%02X\r\n", p_head->CMD );
			}
		}
	}

	return check;
}

