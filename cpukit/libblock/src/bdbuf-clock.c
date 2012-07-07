
#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <rtems.h>
#include <rtems/error.h>
#include <rtems/malloc.h>

#include "rtems/bdbuf.h"
#define container_of(ptr, type, member) \
 ((type *)((char *)ptr - offsetof(type, member)))


rtems_chain_control lru;               /**< Least recently used list */

rtems_status_code
rtems_bdbuf_init_policy()
{
  rtems_chain_initialize_empty(&lru);
  return RTEMS_SUCCESSFUL;
}

rtems_bdbuf_buffer* rtems_bdbuf_select_victim(rtems_bdbuf_buffer* last)
{
  rtems_chain_node *chain_node; 
  if (last == NULL) {
    chain_node = rtems_chain_first (&lru);
  }
  else { 
    chain_node = rtems_chain_next (&last->node);
  }

  if  (!rtems_chain_is_tail (&lru, chain_node)) {
    rtems_bdbuf_buffer *bd = container_of(chain_node,rtems_bdbuf_buffer,node);
    return bd;
  }
  return NULL;
}

void rtems_bdbuf_enqueue(rtems_bdbuf_buffer *bd)
{
  rtems_chain_append_unprotected (&lru, &bd->node);
}

void rtems_bdbuf_dequeue(rtems_bdbuf_buffer *bd)
{
  rtems_chain_extract_unprotected (&bd->node);
}

bool rtems_bdbuf_isqueued(rtems_bdbuf_buffer *bd)
{
  return true;
}
