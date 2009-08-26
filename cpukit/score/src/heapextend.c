/*
 *  Heap Handler
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id$
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/system.h>
#include <rtems/score/sysstate.h>
#include <rtems/score/heap.h>

Heap_Extend_status _Heap_Extend(
  Heap_Control *heap,
  void *area_begin_ptr,
  uintptr_t area_size,
  uintptr_t *amount_extended
)
{
  Heap_Statistics *const stats = &heap->stats;
  uintptr_t const area_begin = (uintptr_t) area_begin_ptr;
  uintptr_t const heap_area_begin = heap->begin;
  uintptr_t const heap_area_end = heap->end;
  uintptr_t const new_heap_area_end = heap_area_end + area_size;
  uintptr_t extend_size = 0;
  Heap_Block *const old_final = heap->final;
  Heap_Block *new_final = NULL;

  /*
   *  There are five possibilities for the location of starting
   *  address:
   *
   *    1. non-contiguous lower address     (NOT SUPPORTED)
   *    2. contiguous lower address         (NOT SUPPORTED)
   *    3. in the heap                      (ERROR)
   *    4. contiguous higher address        (SUPPORTED)
   *    5. non-contiguous higher address    (NOT SUPPORTED)
   *
   *  As noted, this code only supports (4).
   */

  if ( area_begin >= heap_area_begin && area_begin < heap_area_end ) {
    return HEAP_EXTEND_ERROR; /* case 3 */
  } else if ( area_begin != heap_area_end ) {
    return HEAP_EXTEND_NOT_IMPLEMENTED; /* cases 1, 2, and 5 */
  }

  /*
   *  Currently only case 4 should make it to this point.
   *  The basic trick is to make the extend area look like a used
   *  block and free it.
   */

  heap->end = new_heap_area_end;

  extend_size = new_heap_area_end
    - (uintptr_t) old_final - HEAP_LAST_BLOCK_OVERHEAD;
  extend_size = _Heap_Align_down( extend_size, heap->page_size );

  *amount_extended = extend_size;

  if( extend_size >= heap->min_block_size ) {
    old_final->size_and_flag = extend_size
      | (old_final->size_and_flag & HEAP_PREV_BLOCK_USED);
    new_final = _Heap_Block_at( old_final, extend_size );
    new_final->size_and_flag = heap->page_size | HEAP_PREV_BLOCK_USED;

    heap->final = new_final;

    stats->size += area_size;
    ++stats->used_blocks;
    --stats->frees; /* Do not count subsequent call as actual free() */

    _Heap_Free( heap, (void *) _Heap_Alloc_area_of_block( old_final ));
  }

  return HEAP_EXTEND_SUCCESSFUL;
}
