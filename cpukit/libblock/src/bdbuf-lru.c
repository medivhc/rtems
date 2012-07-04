#include <rtems.h>
#include <rtems/error.h>
#include <rtems/malloc.h>

#include "rtems/bdbuf.h"
#define container_of(ptr, type, member) \
 ((type *)((char *)ptr - offsetof(type, member)))


#define CLOCK_IN BUF_PRIVATE1
#define CLOCK_HIT BUF_PRIVATE2
#define CLOCK_MODIFIED  BUF_PRIVATE3
#define CLOCK_ROUND_1  BUF_PRIVATE4
#define CLOCK_ROUND_2  BUF_PRIVATE5


rtems_chain_control clock_list;               /**< Least recently used list */
rtems_chain_node *current_node; 

rtems_status_code
rtems_bdbuf_init_policy()
{
  rtems_chain_initialize_empty(&clock_list);
  return RTEMS_SUCCESSFUL;
}

rtems_bdbuf_buffer* rtems_bdbuf_select_victim(rtems_bdbuf_buffer* last)
{
  rtems_chain_node *chain_node; 
  if (last == NULL) {
    chain_node = current_node;
    rtems_bdbuf_buffer *bd = container_of(chain_node,rtems_bdbuf_buffer,node);
    bd->flags  |= CLOCK_ROUND_1;
  }
  else { 
    if (last->flags & CLOCK_MODIFIED ){
      last->flags &= ~CLOCK_MODIFIED;
      last->flags |= CLOCK_HIT;
    }
    chain_node = rtems_chain_next (&last->node);
  }
  bool is_end = false;
  while  ( true ) {
    rtems_bdbuf_buffer *bd = container_of(chain_node,rtems_bdbuf_buffer,node);
    if (bd->flags & CLOCK_HIT ) {
      bd->flags |= CLOCK_MODIFIED;
      bd->flags &= ~CLOCK_HIT;
    } else {
      //Kbd->flags |= CLOCK_HIT;
    }
    if (bd->state ==RTEMS_BDBUF_STATE_FREE || bd->state == RTEMS_BDBUF_STATE_CACHED){
      bd->flags &= ~(CLOCK_ROUND_1 | CLOCK_ROUND_2);
      return bd;
    }
    is_end = rtems_chain_is_tail (&clock_list, chain_node);
    if (is_end){
      chain_node = rtems_chain_first (&clock_list);
    }else {
      chain_node = rtems_chain_next (&bd->node);
    }

    /*
     * Walk around and still not find the buffer
     */
    if (bd->flags & CLOCK_ROUND_1) {
      bd->flags |= CLOCK_ROUND_2 ;
    } else if (bd->flags & CLOCK_ROUND_2) {
      bd->flags &= ~(CLOCK_ROUND_1 | CLOCK_ROUND_2);
      return NULL;
    }
  }
  return NULL;
}

void rtems_bdbuf_enqueue(rtems_bdbuf_buffer *bd)
{
  if (bd->flags == 0)
  {
    bd->flags |= CLOCK_IN;
    rtems_chain_append_unprotected (&clock_list, &bd->node);
    current_node = &bd->node;
  }else {
    bd->flags |= CLOCK_HIT;
  }

}

void rtems_bdbuf_dequeue(rtems_bdbuf_buffer *bd)
{
  if (bd->state == RTEMS_BDBUF_STATE_CACHED) {
    bd->flags |= CLOCK_HIT;
  }
  //rtems_chain_extract_unprotected (&bd->node);

}

bool rtems_bdbuf_isqueued(rtems_bdbuf_buffer *bd)
{
  return true;
}
