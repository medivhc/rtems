/*
 *  COPYRIGHT (c) 1989-2012.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  Copryight (c) 2012 Xiang Cui <medivhc@gmail.com>
 */

#include <rtems.h>

#include "rtems/bdbuf.h"
#include "rtems/bdbuf_clock.h"

#define CLOCK_IN_QUEUE BDBUF_PRIVATE1
#define CLOCK_HIT BDBUF_PRIVATE2

#define container_of(ptr, type, member) \
 ((type *)((char *)ptr - offsetof(type, member)))


static rtems_chain_control clock_queue;              /**< clock queue */
static rtems_chain_node *clock_hand ; 
static rtems_chain_node *start ;

rtems_status_code
_clock_init()
{
  rtems_chain_initialize_empty(&clock_queue);
  clock_hand = NULL;
  return RTEMS_SUCCESSFUL;
}

rtems_bdbuf_buffer* _clock_victim_next(rtems_bdbuf_buffer* pre)
{
  rtems_chain_node *chain_node;
  if (pre == NULL) {
    rtems_chain_node* first = rtems_chain_head (&clock_queue);
    rtems_chain_node* last = rtems_chain_tail (&clock_queue);
    first->previous = last;
    last-> next = first;
    chain_node = clock_hand;
    start = clock_hand ;
  } else {
    chain_node = &pre->node;
  }
  if (chain_node == NULL) {
    return NULL;
  }
  while (rtems_chain_next(chain_node) != start) {
     rtems_bdbuf_buffer * bd =
      container_of(chain_node,rtems_bdbuf_buffer,node);
     if (bd->state == RTEMS_BDBUF_STATE_CACHED &&
       bd->flags & CLOCK_HIT !=0 ) {
       return  bd;
     } else {
       chain_node  = rtems_chain_next(chain_node);
     }
  }
  return NULL;
}

void _clock_enqueue(rtems_bdbuf_buffer *bd)
{
  if (( bd->flags & CLOCK_IN_QUEUE ) == 0) {
    rtems_chain_append_unprotected (&clock_queue, &bd->node);
    bd->flags |= CLOCK_IN_QUEUE;
    clock_hand = &bd->node;
    start = &bd->node;
  } else {
    if (bd->state == RTEMS_BDBUF_STATE_TRANSFER) {
      bd->flags |= CLOCK_HIT;
    }
  }
}

void _clock_dequeue(rtems_bdbuf_buffer *bd)
{
  rtems_chain_node *chain_node;
  switch  (bd->state) {
    case RTEMS_BDBUF_STATE_EMPTY:
      chain_node = &bd->node;
      while (rtems_chain_next(chain_node) != start) {
        rtems_bdbuf_buffer * bd =
          container_of(chain_node,rtems_bdbuf_buffer,node);
        chain_node  = rtems_chain_previous(chain_node);
        bd->flags &= ~CLOCK_HIT;
      }
      clock_hand = &bd->node;
      break;
    case RTEMS_BDBUF_STATE_ACCESS_CACHED:
      bd->flags |= CLOCK_HIT;
      break;
    default:
      return;
  }
}
