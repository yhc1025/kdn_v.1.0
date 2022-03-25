#include "TB_util.h"
#include "TB_rb.h"

int TB_rb_init( TB_RB_H *p_handle, void *p_buffer, int buf_size, int item_size )
{
	int ret = -1;

	if( p_handle && p_buffer )
	{
		if( buf_size > 0 && item_size > 0 )	
		{
		    p_handle->data 	= p_buffer;
		    p_handle->size 	= buf_size;
		    p_handle->head 	= 0;
		    p_handle->tail 	= 0;
		    p_handle->count	= 0;

			p_handle->item_size = item_size;

			ret = 0;
		}
	}
	else
	{
		printf( "[%s:%d] ERROR. Init a Ring Buffer\r\n", __FILE__, __LINE__ );
	}

	return ret;
}

int TB_rb_reset( TB_RB_H *p_handle )
{
	int ret = -1;
	
    if( p_handle )
    { 
	    p_handle->head = 0;
	    p_handle->tail = 0;
	    p_handle->count = 0;

		ret = 0;
    }
	else
	{
		printf( "[%s:%d] ERROR. Reset a Ring Buffer\r\n", __FILE__, __LINE__ );
	}

	return ret;
}
 
int TB_rb_is_empty( TB_RB_H *p_handle )
{
    int is_empty = 0;
 
    if( p_handle )
    { 
	    if( p_handle->count == 0 )
	    {
	        is_empty = 1;
	    }
    }
	else
	{
		is_empty = -1;
		printf( "[%s:%d] ERROR. Empty Check a Ring Buffer\r\n", __FILE__, __LINE__ );
	}
 
    return is_empty;
}
 
int  TB_rb_is_full( TB_RB_H *p_handle )
{
    int is_full = 0;
 
    if( p_handle )
    { 
	    if( (p_handle->count * p_handle->item_size) >= p_handle->size )
	    {
	        is_full = 1;
	    }
    }
	else
	{
		is_full = -1;
		printf( "[%s:%d] ERROR. Full Check a Ring Buffer\r\n", __FILE__, __LINE__ );
	}

    return is_full;
}
 
int TB_rb_item_push( TB_RB_H *p_handle, void *p_item )
{
    int is_full = 0;

    if( p_handle && p_item )
    {
	    unsigned char *p_dst = ((unsigned char *)p_handle->data) + p_handle->head;
		if( p_dst )
		{
			wmemcpy( p_dst, p_handle->item_size, p_item, p_handle->item_size );
		    p_handle->head += p_handle->item_size;
		    p_handle->count++;

		    if( p_handle->head >= p_handle->size )
		    {
		        p_handle->head = 0;
		    }

		    is_full = TB_rb_is_full( p_handle );
		    if( is_full == 1 )
		    {
				TB_rb_item_get( p_handle );
		    }
		}
    }
	else
	{
		is_full = -1;
		printf( "[%s:%d] ERROR. Push Item a Ring Buffer\r\n", __FILE__, __LINE__ );
	}
 
    return is_full;
}

/*******************************************************************************
*		First-In Data를 읽어오고, tail index를 이동시킨다.
*******************************************************************************/
int  TB_rb_item_pop( TB_RB_H *p_handle, void *p_item )
{
    int is_empty = 0;
 
    if( p_handle && p_item )
    {
	    is_empty = TB_rb_is_empty( p_handle );
	 
	    if( !is_empty )
	    {
			unsigned char *p_src = (unsigned char *)p_handle->data + p_handle->tail;
			wmemcpy( p_item, p_handle->item_size, p_src, p_handle->item_size );
	        p_handle->tail += p_handle->item_size;
	        p_handle->count--;
	 
	        if( p_handle->tail >= p_handle->size )
	        {
	            p_handle->tail = 0;
	        }
	    }
    }
	else
	{
		is_empty = -1;
		printf( "[%s:%d] ERROR. Pop Item a Ring Buffer\r\n", __FILE__, __LINE__ );
	}
 
    return is_empty;
}

/*******************************************************************************
*	First-In Data를 읽기 위한 파라메터가 없고, 단지 tail index만 이동시킨다.
*******************************************************************************/
int  TB_rb_item_get( TB_RB_H *p_handle )
{
    int is_empty = 0;
 
    if( p_handle )
    { 
	    is_empty = TB_rb_is_empty( p_handle );
	 
	    if( !is_empty )
	    {
	        p_handle->tail += p_handle->item_size;
	        p_handle->count--;
	 
	        if( p_handle->tail >= p_handle->size )
	        {
	            p_handle->tail = 0;
	        }
	    }
    }
	else
	{
		is_empty = -1;
		printf( "[%s:%d] ERROR. Get Item a Ring Buffer\r\n", __FILE__, __LINE__ );
	}
 
    return is_empty;
}

int TB_rb_get_count( TB_RB_H *p_handle )
{
    int count = 0;

    if( p_handle )
    {
	    count = p_handle->count;
    }
	else
	{
		printf( "handle is NULL\r\n" );
	}
 
    return count;
}

