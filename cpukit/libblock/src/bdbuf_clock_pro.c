
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
#include <rtems/rbtree.h>

#include "rtems/bdbuf.h"
#include "rtems/bdbuf_clock_pro.h"

#define CLOCK_PRO_HOT  BDBUF_PRIVATE1
#define CLOCK_PRO_TEST  BDBUF_PRIVATE2
#define CLOCK_PRO_VALID BDBUF_PRIVATE3
#define CLOCK_PRO_REF BDBUF_REFERENCE

#define container_of rtems_rbtree_container_of
/*
 * Turn on assertions when RTEMS_DEBUG is defined.
 */
#ifdef RTEMS_DEBUG
#include <assert.h>
#define CLOCK_ASSERT(_x) assert(_x)
#else
#define CLOCK_ASSERT(_x)
#endif



struct clock_pro_block;
typedef struct
{
  rtems_rbtree_node rbtree_node;
  uintptr_t dd;
  rtems_blkdev_bnum block;
  struct clock_pro_block *ptr;
} non_resident_buffer;

typedef struct clock_pro_block
{
  union
  {
    non_resident_buffer *non_resident;
    rtems_bdbuf_buffer *bd;
  } ptr;
  rtems_chain_node link;
  uint8_t flags;
} clock_pro_block;

typedef struct
{
  int cached_buffer_num;
  int cachde_buffer_min;
  int non_resident_num;
  int non_resident_max;
  int hot_num;
  int hot_max;
  rtems_rbtree_control non_resident_rbtree;
  rtems_chain_control free_non_resident_list;
  rtems_chain_control free_resident_list;
  rtems_chain_node *hand_hot;
  rtems_chain_node *hand_cold;
  rtems_chain_node *hand_test;
  clock_pro_block *init_block;
  clock_pro_block *blocks;
  non_resident_buffer *non_resident;
} clock_pro_state;

static clock_pro_state clock_pro;

static int
non_resident_compare_function (const rtems_rbtree_node * n1,
                               const rtems_rbtree_node * n2)
{
  non_resident_buffer *buffer1 =
    rtems_rbtree_container_of (n1, non_resident_buffer, rbtree_node);
  non_resident_buffer *buffer2 =
    rtems_rbtree_container_of (n2, non_resident_buffer, rbtree_node);

  uintptr_t diff = buffer1->dd - buffer2->dd;
  if (diff != 0)
    return diff;
  else {
    return buffer1->block - buffer2->block;
  }
}


static inline bool
is_cached_buffer (clock_pro_block * cb)
{
  return ((cb->flags & CLOCK_PRO_VALID) && !(cb->flags & CLOCK_PRO_TEST));
}

static inline bool
is_hot_buffer (clock_pro_block * cb)
{
  return cb->flags & CLOCK_PRO_HOT;
}

static inline bool
is_ref_buffer (clock_pro_block * cb)
{
  return (cb->ptr.bd->flags) & CLOCK_PRO_REF;
}

static inline bool
is_test_buffer (clock_pro_block * cb)
{
  return cb->flags & CLOCK_PRO_TEST;
}

static inline void
set_buffer_cached (clock_pro_block * cb, bool flag)
{
  if (flag) {
    cb->flags |= CLOCK_PRO_VALID;
  } else {
    cb->flags &= ~CLOCK_PRO_VALID;
  }
}

static inline void
set_buffer_hot (clock_pro_block * cb, bool flag)
{
  if (flag) {
    cb->flags |= CLOCK_PRO_HOT;
  } else {
    cb->flags &= ~CLOCK_PRO_HOT;
  }
}

static inline void
set_buffer_ref (clock_pro_block * cb, bool flag)
{
  if (flag) {
    (cb->ptr.bd->flags) |= CLOCK_PRO_REF;
  } else {
    (cb->ptr.bd->flags) &= ~CLOCK_PRO_REF;
  }
}


static rtems_chain_node *
find_test_block_from (rtems_chain_node * node)
{
  rtems_chain_node *chain_node = node;
  do {
    clock_pro_block *cb = container_of (chain_node, clock_pro_block, link);
    if (is_test_buffer (cb))
      return chain_node;
    chain_node = rtems_chain_next (chain_node);
  } while (!rtems_chain_are_nodes_equal (chain_node, node));
  return NULL;
}

static rtems_chain_node *
find_hot_block_from (rtems_chain_node * node)
{
  rtems_chain_node *chain_node = node;
  do {
    clock_pro_block *cb = container_of (chain_node, clock_pro_block, link);
    if (is_cached_buffer (cb) && is_hot_buffer (cb))
      return chain_node;
    chain_node = rtems_chain_next (chain_node);
  } while (!rtems_chain_are_nodes_equal (chain_node, node));
  return NULL;
}

static rtems_chain_node *
find_cold_block_from (rtems_chain_node * node)
{
  rtems_chain_node *chain_node = node;
  do {
    clock_pro_block *cb = container_of (chain_node, clock_pro_block, link);
    if (is_cached_buffer (cb) && !is_hot_buffer (cb))
      return chain_node;
    chain_node = rtems_chain_next (chain_node);
  } while (!rtems_chain_are_nodes_equal (chain_node, node));
  return NULL;
}

static rtems_chain_node *
get_hand_hot (void)
{
  rtems_chain_node *hand_hot;
  hand_hot = find_hot_block_from (clock_pro.hand_hot);
  if (hand_hot == NULL) {
    hand_hot = &clock_pro.init_block->link;
  }
  clock_pro.hand_hot = hand_hot;
  return hand_hot;
}

static rtems_chain_node *
get_hand_cold (void)
{
  rtems_chain_node *hand_cold;
  hand_cold = find_cold_block_from (clock_pro.hand_cold);
  if (hand_cold == NULL) {
    hand_cold = &clock_pro.init_block->link;
  }
  clock_pro.hand_cold = hand_cold;
  return hand_cold;
}

static rtems_chain_node *
get_hand_test (void)
{
  rtems_chain_node *hand_test;
  hand_test = find_test_block_from (clock_pro.hand_test);
  if (hand_test == NULL) {
    hand_test = &clock_pro.init_block->link;
  }
  clock_pro.hand_test = hand_test;
  return hand_test;
}
static void check_conflict_with_hands (rtems_chain_node *node)
{

  if (node == clock_pro.hand_cold) {
    clock_pro.hand_cold = rtems_chain_next(clock_pro.hand_cold);
  }
  if (node == clock_pro.hand_hot) {
    clock_pro.hand_hot = rtems_chain_next(clock_pro.hand_hot);
  }
  if (node == clock_pro.hand_test){
    clock_pro.hand_test = rtems_chain_next(clock_pro.hand_test);
  }

}

static void
remove_non_resident_buffer_from_clock (clock_pro_block * cb)
{
  CLOCK_ASSERT (cb != clock_pro.init_block);
  check_conflict_with_hands(&cb->link);
  CLOCK_ASSERT (is_test_buffer (cb));
  rtems_rbtree_extract_unprotected (&clock_pro.non_resident_rbtree,&cb->ptr.non_resident->rbtree_node);
  rtems_chain_extract_unprotected (&cb->link);
  rtems_chain_append_unprotected (&clock_pro.free_non_resident_list,
                                  &cb->link);
  --clock_pro.non_resident_num;
}


/*
 * if the hot_num bigger than the max, degrade the hot to cold
 */
static void
check_resident_num (void)
{
  clock_pro_block *cb;
  rtems_chain_node *hand_hot = get_hand_hot ();
  rtems_chain_node *chain_node = hand_hot;
  while (rtems_chain_next (chain_node) != hand_hot) {
    cb = container_of (chain_node, clock_pro_block, link);
    if (is_cached_buffer(cb) && is_hot_buffer (cb)) {
      if (clock_pro.hot_num >= clock_pro.hot_max) {
        if (!is_ref_buffer (cb)) {
          set_buffer_hot (cb, false);
          --clock_pro.hot_num;
        }
        set_buffer_ref(cb,false);
      } else {
        break;
      }
    } else if (is_test_buffer (cb)) {
      chain_node = rtems_chain_previous (chain_node);
      remove_non_resident_buffer_from_clock (cb);
    }
    chain_node = rtems_chain_next (chain_node);
  }
  clock_pro.hand_hot = (chain_node);
}

static void
show_ring_begin_with (rtems_chain_node * node)
{
  rtems_chain_node *begin = node;
  rtems_chain_node *chain_node = rtems_chain_next (node);
  clock_pro_block *cb;

  while ((chain_node) != begin) {
    chain_node = rtems_chain_next (chain_node);
    cb = container_of (chain_node, clock_pro_block, link);
    if (&cb->link == clock_pro.hand_test) {
      printf (" hand test->");
    if (&cb->link == clock_pro.hand_hot) {
      printf (" hand hot->");
    }
    if (&cb->link == clock_pro.hand_cold) {
      printf (" hand cold->");
    }
    }
    if (is_cached_buffer(cb)&&is_ref_buffer (cb)) {
      printf ("i");
    }
    if (is_test_buffer (cb)) {
      CLOCK_ASSERT (cb != clock_pro.init_block);
      printf ("T");
    } else if (is_hot_buffer (cb)) {
      CLOCK_ASSERT (cb != clock_pro.init_block);
      printf ("H");
    } else if (is_cached_buffer (cb) && !is_hot_buffer (cb)) {
      CLOCK_ASSERT (cb != clock_pro.init_block);
      printf ("C");
    }else {
      printf ("M");
    }
  }
}

static void
show_usage (void)
{

  printf ("test :");
  show_ring_begin_with (clock_pro.hand_test);
  printf ("\nhot  :");
  show_ring_begin_with (clock_pro.hand_hot);
  printf ("\ncold :");
  show_ring_begin_with (clock_pro.hand_cold);
  printf ("\n");
}

static void
print_state (const char *where)
{
  printf ("%s : cached num: %d, hot num: %d,non residemt num: %d \n", where,
          clock_pro.cached_buffer_num, clock_pro.hot_num,
          clock_pro.non_resident_num);
  show_usage ();
}

rtems_status_code
_clock_pro_init (const rtems_bdbuf_config * config,
                 rtems_chain_control * free_list)
{
  int i = 0;
  int buffer_min_count = config->size / config->buffer_min;
  clock_pro.non_resident_max = buffer_min_count;

  clock_pro_block *blocks =
    calloc (2 * buffer_min_count+1, sizeof (clock_pro_block));
  if (blocks == NULL)
    return RTEMS_UNSATISFIED;

  clock_pro.blocks = blocks;

  non_resident_buffer *non_resident =
    calloc (buffer_min_count, sizeof (non_resident_buffer));
  clock_pro.non_resident = non_resident;
  if (non_resident == NULL) {
    free (blocks);
    return RTEMS_UNSATISFIED;
  }

  rtems_chain_initialize_empty (&clock_pro.free_resident_list);
  rtems_chain_initialize_empty (&clock_pro.free_non_resident_list);
  rtems_rbtree_initialize_empty (&clock_pro.non_resident_rbtree,
                                 &non_resident_compare_function, true);

  for (i = 0; i < buffer_min_count; ++i) {
    clock_pro_block *cb = &blocks[i];
    cb->flags |= CLOCK_PRO_TEST;
    cb->ptr.non_resident = &non_resident[i];
    non_resident[i].ptr = cb;
    rtems_chain_append_unprotected (&clock_pro.free_non_resident_list,
                                   &cb->link);
  }
  for (i = 0; i < buffer_min_count; ++i) {
    rtems_chain_append_unprotected (&clock_pro.free_resident_list,
                                    &blocks[i + buffer_min_count].link);
  }

  clock_pro_block *init_block = &blocks[2*buffer_min_count];
  init_block->link.next = &init_block->link;
  init_block->link.previous = &init_block->link;

  clock_pro.init_block = init_block;

  clock_pro.hand_cold = &init_block->link;
  clock_pro.hand_hot = &init_block->link;
  clock_pro.hand_test = &init_block->link;
  clock_pro.hot_max = clock_pro.non_resident_max / 8;
  clock_pro.cachde_buffer_min = 2 *clock_pro.hot_max;
  return RTEMS_SUCCESSFUL;
}

/*
 * This function should return the cold buffer, and will not 
 * reset the ref flag
 */
rtems_bdbuf_buffer *
_clock_pro_victim_next (rtems_bdbuf_buffer * pre)
{
  rtems_chain_node *chain_node;
  clock_pro_block *cb;
  static rtems_chain_node *victim_start = NULL;

  print_state ("victim");
  if (clock_pro.cached_buffer_num <= clock_pro.cachde_buffer_min) {
    return NULL;
  }

  if (pre == NULL) {            /* first call */
    victim_start = chain_node = get_hand_cold ();
  } else {
    cb = pre->user;
    chain_node = rtems_chain_next (&cb->link);
  }

  while (rtems_chain_next (chain_node) != victim_start) {
    cb = container_of (chain_node, clock_pro_block, link);
    if (is_cached_buffer (cb)
        && !is_ref_buffer (cb)
        && !is_hot_buffer (cb)
        && cb->ptr.bd->state == RTEMS_BDBUF_STATE_CACHED) {
      return cb->ptr.bd;
    } else {
      chain_node = rtems_chain_next (chain_node);
    }
  }
  chain_node = rtems_chain_next (chain_node);
  while (rtems_chain_next (chain_node) != victim_start) {
    cb = container_of (chain_node, clock_pro_block, link);
    if (is_cached_buffer (cb)
        && !is_hot_buffer (cb)
        && cb->ptr.bd->state == RTEMS_BDBUF_STATE_CACHED) {
      return cb->ptr.bd;
    } else {
      chain_node = rtems_chain_next (chain_node);
    }
  }
  return NULL;
}

static void
move_after_hand_hot (clock_pro_block * cb, bool as_hot_buffer)
{
  rtems_chain_node *hand_hot = get_hand_hot ();

  check_conflict_with_hands(&cb->link);
  rtems_chain_extract_unprotected (&cb->link);
  rtems_chain_insert_unprotected ((rtems_chain_previous (hand_hot)),
                                  &cb->link);

  if (as_hot_buffer) {
    ++clock_pro.hot_num;
    set_buffer_hot (cb, true);
    check_resident_num ();
  } else {
    set_buffer_hot (cb, false);
  }
}

static void
add_after_hand_hot (rtems_bdbuf_buffer * bd, bool as_hot_buffer)
{
  clock_pro_block *cb;

  if (bd->user != NULL) {
    cb = bd->user;
    if (is_hot_buffer(cb)) {
      -- clock_pro.hot_num;
    }
  } else {
    rtems_chain_node *chain_node =
      rtems_chain_get_unprotected (&clock_pro.free_resident_list);
    CLOCK_ASSERT (chain_node != NULL);
    cb = container_of (chain_node, clock_pro_block, link);
  }

  CLOCK_ASSERT (cb != clock_pro.init_block);

  set_buffer_hot (cb,false);
  set_buffer_cached (cb, true);

  cb->ptr.bd = bd;
  bd->user = cb;

  move_after_hand_hot (cb, as_hot_buffer);
}

/*
 * Rotale the cold hand over the bd position.
 * The cold with ref pass through should be upgraded to hot.
 */
static void
rotate_hand_cold_over (clock_pro_block * cb)
{
  rtems_chain_node *chain_node = get_hand_cold ();
  clock_pro_block *current = container_of (chain_node, clock_pro_block, link);

  while (!rtems_chain_are_nodes_equal (chain_node, &cb->link)) {
    if (is_cached_buffer (cb)
        && !is_hot_buffer (cb)) {
      CLOCK_ASSERT (!is_test_buffer (cb));
      if (is_ref_buffer (cb)) {
        /*
         * this buffer should upgrade to hot, we get the previous node first.
         */

        move_after_hand_hot (current, true);
      } else {
        /*
         * This happens that the buffer is not swap out because it is waiting, 
         * So put the hand_cold here.
         */
        clock_pro.hand_cold = (chain_node);
        return;
      }
    }
    chain_node = rtems_chain_next (chain_node);
    current = container_of (chain_node, clock_pro_block, link);
  }
  clock_pro.hand_cold = rtems_chain_next(&cb->link);
}

static non_resident_buffer *
find_non_resident_buffer (rtems_bdbuf_buffer * bd)
{
  non_resident_buffer target;
  target.dd = (uintptr_t) bd->dd;
  target.block = bd->block;
  rtems_rbtree_node *node = rtems_rbtree_find_unprotected (&clock_pro.non_resident_rbtree,
                                               &target.rbtree_node);
  if (node == NULL) {
    return NULL;
  } else {
    return container_of (node, non_resident_buffer, rbtree_node);
  }
}

static void
change_to_non_resident (rtems_bdbuf_buffer * bd)
{
  non_resident_buffer *nrb;
  clock_pro_block *cb = bd->user;
  rtems_chain_node *chain_node;

  chain_node =
    rtems_chain_get_unprotected (&clock_pro.free_non_resident_list);

  if (chain_node == NULL) {     /*non_resident_buffer is full */
    chain_node = get_hand_test ();
    CLOCK_ASSERT (chain_node != &clock_pro.init_block->link);
    /* move hand_test  */
    clock_pro.hand_test =  rtems_chain_next (chain_node);;
    check_conflict_with_hands(chain_node);
    rtems_chain_extract_unprotected (chain_node);
  } else {
    ++clock_pro.non_resident_num;
  }

  /* intert the non_resident after the bd, the bd will be removed then */

  rtems_chain_insert_unprotected (&cb->link, chain_node);
  cb = container_of (chain_node, clock_pro_block, link);
  CLOCK_ASSERT (is_test_buffer (cb));
  nrb = cb->ptr.non_resident;
  nrb->dd = (uintptr_t) bd->dd;
  nrb->block = bd->block;
  rtems_rbtree_insert (&clock_pro.non_resident_rbtree, &nrb->rbtree_node);
}

void
_clock_pro_enqueue (rtems_bdbuf_buffer * bd)
{
  bool as_hot_buffer = false;
  non_resident_buffer *find_non_resident;
  clock_pro_block *cb;
  print_state ("enqueue");
  switch (bd->state) {
  case RTEMS_BDBUF_STATE_TRANSFER:
    find_non_resident = find_non_resident_buffer (bd);
    if (find_non_resident != NULL) {
      cb = find_non_resident->ptr;
      remove_non_resident_buffer_from_clock (cb);
      as_hot_buffer = true;
    }
    add_after_hand_hot (bd, as_hot_buffer);
    break;

  case RTEMS_BDBUF_STATE_ACCESS_CACHED:
  cb = bd->user;
  set_buffer_cached(cb,true);
  set_buffer_ref(cb,true);
  break;
  default:
    CLOCK_ASSERT (false);
    break;
  }
  ++clock_pro.cached_buffer_num;
}


static void
remove_resident_buffer_from_clock (clock_pro_block * cb)
{
  check_conflict_with_hands (&cb->link);
  CLOCK_ASSERT (is_cached_buffer (cb));
  set_buffer_ref(cb,false);
  if (is_hot_buffer (cb)) {
    --clock_pro.hot_num;
    set_buffer_hot(cb,false);
  }
  CLOCK_ASSERT (&cb->link != clock_pro.hand_cold);
  CLOCK_ASSERT (cb != clock_pro.init_block);
  rtems_chain_extract_unprotected (&cb->link);
  rtems_chain_append_unprotected (&clock_pro.free_resident_list, &cb->link);
}


static void
show_users (const char* where, rtems_bdbuf_buffer* bd)
{
  const char* states[] =
    { "FR", "EM", "CH", "AC", "AM", "AE", "AP", "MD", "SY", "TR", "TP" };

  printf ("bdbuf:users: %15s: [%ld (%s)] n",
          where,
          bd->block, states[bd->state]);
}
void
_clock_pro_dequeue (rtems_bdbuf_buffer * bd)
{
  clock_pro_block *cb = bd->user;
  print_state ("dequeue");

  CLOCK_ASSERT (cb != NULL);
  CLOCK_ASSERT (!is_test_buffer (cb));

  switch (bd->state) {
  case RTEMS_BDBUF_STATE_EMPTY:
    if (is_cached_buffer(cb)) {
      /*
       * The buffer is going to be swapped out
       * We firstly try to rotalte the cold hand over it.
       */
      rotate_hand_cold_over (cb);
      /*
       * the buffer which is going to be swap out will be add to non_resident
       */
      change_to_non_resident (bd);
    }
    /* fall through */
  case RTEMS_BDBUF_STATE_FREE:
  case RTEMS_BDBUF_STATE_CACHED:
    remove_resident_buffer_from_clock (cb);
    bd->user = NULL;
  break;
  case RTEMS_BDBUF_STATE_ACCESS_CACHED:
  break;

  default:
    show_users("dequeue",bd);
    CLOCK_ASSERT(false);
    break;
  }
  set_buffer_cached (cb, false);
  --clock_pro.cached_buffer_num;
}
