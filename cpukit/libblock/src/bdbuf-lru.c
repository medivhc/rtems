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
}

void rtems_bdbuf_enqueue(rtems_bdbuf_buffer *bd)
{
}

void rtems_bdbuf_dequeue(rtems_bdbuf_buffer *bd)
{
}

bool rtems_bdbuf_isqueued(rtems_bdbuf_buffer *bd)
{
}
