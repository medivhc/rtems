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
#include "rtems/bdbuf_lru.h"


#define container_of(ptr, type, member) \
 ((type *)((char *)ptr - offsetof(type, member)))

rtems_chain_control lru;               /**< Least recently used list */

rtems_status_code
_lru_init()
{
  rtems_chain_initialize_empty(&lru);
  return RTEMS_SUCCESSFUL;
}

rtems_bdbuf_buffer* _lru_victim_next(rtems_bdbuf_buffer* pre)
{
  rtems_chain_node *chain_node; 
  if (pre == NULL) {
    chain_node = rtems_chain_first (&lru);
  }
  else { 
    chain_node = rtems_chain_next (&pre->node);
  }
  if  (!rtems_chain_is_tail (&lru, chain_node)) {
    rtems_bdbuf_buffer *bd = container_of(chain_node,rtems_bdbuf_buffer,node);
    return bd;
  }
  return NULL;
}

void _lru_enqueue(rtems_bdbuf_buffer *bd)
{
  rtems_chain_append_unprotected (&lru, &bd->node);
}

void _lru_dequeue(rtems_bdbuf_buffer *bd)
{
  rtems_chain_extract_unprotected (&bd->node);
}
