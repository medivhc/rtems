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

#define CLOCK_TRACE 1

#define container_of(ptr, type, member) \
 ((type *)((char *)ptr - offsetof(type, member)))

#define CLOCK_REF BDBUF_REFERENCE

typedef struct clock_block {
  rtems_chain_node node;
  rtems_bdbuf_buffer *bd;
} clock_block;
static clock_block * blocks;
rtems_chain_node *clock_hand ;
static uint32_t clock_size;
static bool is_second_round;


#if CLOCK_TRACE
static void print_usage(const char *where)
{
  const char* states[] =
    { "FR", "EM", "CH", "AC", "AM", "AE", "AP", "MD", "SY", "TR", "TP" };

  rtems_chain_node* chain_node = clock_hand;
  clock_block *cb;
  printf("%s\n",where);

  do {
    cb = container_of (chain_node, clock_block,node);
    printf("%s ",states[cb->bd->state]);
    chain_node = rtems_chain_next( chain_node);
  } while (chain_node!= clock_hand);
  printf("\n");
}
#else
#define print_usage(_a) ((void)0)
#endif

static uint32_t rtems_bdbuf_list_count (rtems_chain_control* list)
{
  rtems_chain_node* node = rtems_chain_first (list);
  uint32_t          count = 0;
  while (!rtems_chain_is_tail (list, node))
  {
    count++;
    node = rtems_chain_next (node);
  }
  return count;
}

rtems_status_code
_clock_init(const rtems_bdbuf_config  *config,
  rtems_chain_control *free_list)
{
  rtems_chain_node first_node;
  rtems_chain_node *chain_node = rtems_chain_first (free_list);
  uint32_t count = rtems_bdbuf_list_count (free_list);
  clock_size = count;
  blocks = calloc (count, sizeof (clock_block));
  if (blocks == NULL) {
    return RTEMS_UNSATISFIED;
  }
  uint32_t i = 0;
  rtems_bdbuf_buffer *bd;
  first_node.next = &first_node;
  first_node.previous = &first_node;

  while (!rtems_chain_is_tail(free_list,chain_node)){
    bd = container_of(chain_node,rtems_bdbuf_buffer,link);
    bd->user = &blocks[i];
    blocks[i].bd = bd;
    rtems_chain_insert_unprotected(&first_node,&blocks[i].node);
    chain_node = rtems_chain_next(chain_node);
    ++ i;
  }

  clock_hand = rtems_chain_next(&first_node);
  rtems_chain_extract_unprotected (&first_node);

  is_second_round = false;
  printf("clock_size %ld\n", clock_size);

  return RTEMS_SUCCESSFUL;
}

rtems_bdbuf_buffer* _clock_victim_next(rtems_bdbuf_buffer* pre)
{
  print_usage("victim");
  clock_block *cb;
  rtems_chain_node* chain_node;
  rtems_chain_node *start;
  if ( pre == NULL) {
    chain_node = start = clock_hand ;
  } else {
    cb = pre->user;
    chain_node = start  = rtems_chain_next(&cb->node);
  }
  if (!is_second_round){
    do {
      cb = container_of (chain_node,clock_block,node);
      if (cb->bd->state == RTEMS_BDBUF_STATE_CACHED
        &&(cb->bd->flags & CLOCK_REF) ==0) {
          return cb->bd;
      }
      chain_node = rtems_chain_next(chain_node);
    } while (chain_node!= start);
    is_second_round =true;
  }

  do {
    cb = container_of (chain_node,clock_block,node);
    if (cb->bd->state == RTEMS_BDBUF_STATE_CACHED) {
      return cb->bd;
    }
    chain_node = rtems_chain_next(chain_node);
  } while (chain_node!= start);

  is_second_round =false;

  return NULL;
}

void _clock_enqueue(rtems_bdbuf_buffer *bd)
{
  print_usage("enqueue");
  rtems_chain_node *chain_node;
  clock_block *cb = bd->user;
  chain_node = &cb->node;
  switch (bd->state) {
    case RTEMS_BDBUF_STATE_TRANSFER:
    if (chain_node == clock_hand){
      clock_hand = rtems_chain_next(clock_hand);
    }else {
      rtems_chain_extract_unprotected(chain_node);
      rtems_chain_insert_unprotected(rtems_chain_previous(clock_hand),chain_node);
    }
    case RTEMS_BDBUF_STATE_ACCESS_CACHED:
    default:
    break;
  }
}

void _clock_dequeue(rtems_bdbuf_buffer *bd)
{
  print_usage("dequeue");
  clock_block *cb;
  clock_block *current;
  rtems_chain_node *chain_node;
  switch (bd->state){
    case RTEMS_BDBUF_STATE_EMPTY:
      cb = bd->user;
      if (!is_second_round){
        chain_node = clock_hand;
        do {
          current = container_of (chain_node,clock_block,node);
          (current->bd->flags) &= ~CLOCK_REF;
          chain_node = rtems_chain_next (chain_node);
        } while (chain_node!= &cb->node);
      } else {
        chain_node = &cb->node;
        do {
          current = container_of (chain_node,clock_block,node);
          (current->bd->flags) &= ~CLOCK_REF;
          chain_node = rtems_chain_next (chain_node);
        } while (chain_node!= &cb->node);
        is_second_round =false;
      }
      clock_hand = &cb->node;
    case RTEMS_BDBUF_STATE_CACHED:
    case RTEMS_BDBUF_STATE_FREE:
    case RTEMS_BDBUF_STATE_ACCESS_CACHED:
    default:
    break;
  }
}
