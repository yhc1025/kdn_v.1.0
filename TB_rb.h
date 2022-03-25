#ifndef	__TB_RINGBUFFER_H__
#define __TB_RINGBUFFER_H__

#include "TB_type.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct tagRB_H
{
    void * data;   			// pointer to external buffer
    int size;               // size of external buffer
    int head;               // index of free space in external buffer
    int tail;               // index of last item to be read
    int count;              // hold number of items stored but not read

	int item_size;
} TB_RB_H;

////////////////////////////////////////////////////////////////////////////////


// interface
extern int TB_rb_init( TB_RB_H *p_handle, void *p_buffer, int buf_size, int item_size );
extern int TB_rb_reset( TB_RB_H *p_handle );
extern int TB_rb_is_empty( TB_RB_H *p_handle );
extern int TB_rb_is_full( TB_RB_H *p_handle );
extern int TB_rb_item_push( TB_RB_H *p_handle, void *p_item );
extern int TB_rb_item_pop( TB_RB_H *p_handle, void *p_item );
extern int TB_rb_item_get( TB_RB_H * p_handle );
extern int TB_rb_get_count( TB_RB_H *p_handle );

#endif//__TB_RINGBUFFER_H__

