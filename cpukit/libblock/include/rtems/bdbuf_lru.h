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

#ifndef _RTEMS_BDBUF_LRU_H
#define _RTEMS_BDBUF_LRU_H

/**
 *  @addtogroup rtems_bdbuf_policy
 *
 *  This is the default policy for bdbuf. It uses the minimum memory and
 *  suits for most cases.
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

/**
 *  Entry points for LRU
 */
#define BDBUF_POLICY_LRU_ENTRY_POINTS { \
       _lru_init,\
       _lru_victim_next,\
       _lru_enqueue, \
       _lru_dequeue \
}


rtems_status_code _lru_init (const rtems_bdbuf_config *config,
  rtems_chain_control *free_list);

rtems_bdbuf_buffer* _lru_victim_next (rtems_bdbuf_buffer *bd);

void _lru_enqueue (rtems_bdbuf_buffer *bd);

void _lru_dequeue (rtems_bdbuf_buffer *bd);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif
