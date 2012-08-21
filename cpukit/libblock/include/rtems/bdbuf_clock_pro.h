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
 *  Clock-Pro is a advanced repalcement policy which keeps extra information on paged-out buffer
 *  to hlep to select victim.
 *  Clock-pro keeps a circular list of information about recently-referenced
 *  pages, including all M pages in memory as well as the most recent M pages
 *  that have been paged out. I uses more memory than LRU.
 *
 *  More information can be found:
 *  <a herf="http://www.cse.ohio-state.edu/
 *  hpcs/WWW/HTML/publications/abs05-3.html"> Clock-Pro </a>
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

#define BDBUF_POLICY_CLOCK_PRO_ENTRY_POINTS { \
       _clock_pro_init,\
       _clock_pro_victim_next,\
       _clock_pro_enqueue, \
       _clock_pro_dequeue \
}

rtems_status_code _clock_pro_init (const rtems_bdbuf_config *config,
  rtems_chain_control *free_list);

rtems_bdbuf_buffer* _clock_pro_victim_next (rtems_bdbuf_buffer *bd);

void _clock_pro_enqueue (rtems_bdbuf_buffer *bd);

void _clock_pro_dequeue (rtems_bdbuf_buffer *bd);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif
