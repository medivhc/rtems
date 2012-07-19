#include <rtems.h>
#include <rtems/error.h>
#include <rtems/malloc.h>

#include "rtems/bdbuf.h"
#define container_of(ptr, type, member) \
 ((type *)((char *)ptr - offsetof(type, member)))


#define CLOCK_IN BUF_PRIVATE1
#define CLOCK_INIT BUF_PRIVATE2
#define CLOCK_HIT BUF_PRIVATE3
#define CLOCK_MODIFIED  BUF_PRIVATE4
#define CLOCK_ROUND_1  BUF_PRIVATE5
#define CLOCK_ROUND_2  BUF_PRIVATE6


static rtems_chain_control clock_list;               /**< Least recently used list */
static rtems_chain_node *current_node; 
static rtems_chain_node *node; 

rtems_status_code
rtems_bdbuf_init_policy()
{
  rtems_chain_initialize_empty(&clock_list);
  return RTEMS_SUCCESSFUL;
}

rtems_bdbuf_buffer* rtems_bdbuf_select_victim(rtems_bdbuf_buffer* pre)
{
  rtems_chain_node *start;
  
  if ( pre == NULL) {
    node = current_node;
    start = node;
    rtems_chain_node* first = rtems_chain_head (&clock_list);
    rtems_chain_node* last = rtems_chain_tail (&clock_list);
    first->previous = last;
    last-> next = first;
  } else {
    node = &pre->node;
  }
  while ( rtems_chain_next(node) != start){
     rtems_bdbuf_buffer * bd = container_of(node,rtems_bdbuf_buffer,node);
     if (bd->state != RTEMS_BDBUF_STATE_CACHED) {
       node = rtems_chain_next(node);
     } else {
       current_node = node; 
       return  bd;
     }
  }
  return NULL;
}

void rtems_bdbuf_enqueue(rtems_bdbuf_buffer *bd)
{
  if ((bd->flags & CLOCK_IN) ==0) {
    rtems_chain_append_unprotected (&clock_list, &bd->node);
    current_node = &bd->node;
    bd->flags |= CLOCK_IN;
  }
}

void rtems_bdbuf_dequeue(rtems_bdbuf_buffer *bd)
{
}

bool rtems_bdbuf_isqueued(rtems_bdbuf_buffer *bd)
{
  return (bd->flags & CLOCK_IN != 0);
}
