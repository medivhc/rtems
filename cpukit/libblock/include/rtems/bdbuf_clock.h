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

#ifndef _RTEMS_BDBUF_CLOCK_H
#define _RTEMS_BDBUF_CLOCK_H

/**
 *  @addtogroup rtems_bdbuf_policy
 *
 *  Clock algorithm keeps a circular list of buffers. It provides almost the
 *  same hit rate as the LRU here, but uses more memory.
 *
 *  More information can be found:
 *  <a herf="http://en.wikipedia.org/wiki/
 *  Page_replacement_algorithm#Clock">Clock</a>
 *
 */
/**@{*/

#include <rtems.h>
#include <rtems/libio.h>
#include <rtems/chain.h>

#include <rtems/blkdev.h>
#include <rtems/diskdevs.h>

struct rtems_bdbuf_config;

#ifdef __cplusplus
extern "C" {
#endif

#define BDBUF_POLICY_CLOCK_ENTRY_POINTS { \
       _clock_init,\
       _clock_victim_next,\
       _clock_enqueue, \
       _clock_dequeue \
}


rtems_status_code _clock_init (const rtems_bdbuf_config *config,
  rtems_chain_control *free_list);

rtems_bdbuf_buffer* _clock_victim_next (rtems_bdbuf_buffer *bd);

void _clock_enqueue (rtems_bdbuf_buffer *bd);

void _clock_dequeue (rtems_bdbuf_buffer *bd);

#ifdef __cplusplus
}
#endif
/**@}*/
#endif
