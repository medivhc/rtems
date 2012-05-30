/*
 * Copyright (c) 2012 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.com/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "tmacros.h"

#include <errno.h>
#include <string.h>
#include <math.h>

#include <rtems/blkdev.h>
#include <rtems/bdbuf.h>

#define BLOCK_COUNT 15
#define BLOCK_SIZE 1
#define CACHE_SIZE 4
static rtems_bdbuf_stats stats;

static const rtems_blkdev_bnum sequence_a [] = {
   1, 2, 3, 4,5,1, 2, 3, 4,1, 2, 3, 4,1, 2, 3, 4,1, 2, 3, 4,BLOCK_COUNT
};
static const rtems_blkdev_bnum sequence_b [] = {
  0,1,2,3,0,1,2,3,0,1,2,3,BLOCK_COUNT
};


void print_stats(rtems_bdbuf_stats *stats);
static int test_disk_ioctl(rtems_disk_device *dd, uint32_t req, void *arg)
{
  int rv = 0;
  if (req == RTEMS_BLKIO_REQUEST) {
    rtems_blkdev_request *breq = arg;
    rtems_blkdev_sg_buffer *sg = breq->bufs;
    uint32_t i;

    for (i = 0; i < breq->bufnum; ++i) {
      rtems_blkdev_bnum block = sg [i].block;
      rtems_test_assert(block < BLOCK_COUNT);
    }
    breq->req_done(breq->done_arg, RTEMS_SUCCESSFUL);
  } else {
    errno = EINVAL;
    rv = -1;
  }
  return rv;
}


static void do_read_sequence(rtems_disk_device *dd,const rtems_blkdev_bnum *sequence)
{
  size_t i =0;

  while (sequence[i] <BLOCK_COUNT){
    rtems_status_code sc;
    rtems_bdbuf_buffer *bd;

    sc = rtems_bdbuf_read(dd, sequence [i], &bd);
    rtems_test_assert(sc == RTEMS_SUCCESSFUL);
    sc = rtems_bdbuf_release(bd);
    rtems_test_assert(sc == RTEMS_SUCCESSFUL);

    ++i;
  }
}

static void do_write_esquence(rtems_disk_device *dd,const rtems_blkdev_bnum *sequence )
{
  size_t i = 0;

  while (sequence[i] <BLOCK_COUNT){
    rtems_status_code sc;
    rtems_bdbuf_buffer *bd;

    sc = rtems_bdbuf_get(dd, sequence [i], &bd);
    rtems_test_assert(sc == RTEMS_SUCCESSFUL);
    sc = rtems_bdbuf_release_modified(bd);
    rtems_test_assert(sc == RTEMS_SUCCESSFUL);

    ++i;
  }
}

void print_stats(rtems_bdbuf_stats *stats)
{

  printf("Total read:%2"PRIu32 " Total write:%2"PRIu32 
      " Total read hit:%2"PRIu32" Swapout Execute Count:%2"PRIu32"\n",
      stats->total_read_count,
      stats->total_write_count,
      stats->total_read_hit,stats->cache.swapout_count);
}

static void test(void)
{
  rtems_status_code sc;
  dev_t dev_a = 0;
  dev_t dev_b = 1;
  rtems_disk_device *dd_a;
  rtems_disk_device *dd_b;

  sc = rtems_disk_io_initialize();
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  sc = rtems_disk_create_phys(
      dev_a,
      BLOCK_SIZE,
      BLOCK_COUNT,
      test_disk_ioctl,
      NULL,
      NULL
      );
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  sc=rtems_disk_create_log(
      dev_b,
      dev_a,
      1,
      BLOCK_COUNT-1,
      NULL
      );
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  dd_a = rtems_disk_obtain(dev_a);
  rtems_test_assert(dd_a != NULL);
  dd_b = rtems_disk_obtain(dev_b);
  rtems_test_assert(dd_b != NULL);

  do_read_sequence(dd_a,sequence_a);
  do_read_sequence(dd_b,sequence_b);

  rtems_bdbuf_get_stats(&stats);
  print_stats (&stats);

  do_write_esquence(dd_a,sequence_a);
  do_write_esquence(dd_b,sequence_b);

  do_read_sequence(dd_a,sequence_a);
  do_read_sequence(dd_b,sequence_b);

  sc =rtems_bdbuf_syncdev (dd_a);
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  sc =rtems_bdbuf_syncdev (dd_b);
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  rtems_bdbuf_get_stats(&stats);
  print_stats (&stats);

  sc = rtems_disk_release(dd_a);
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  sc = rtems_disk_release(dd_b);
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  sc = rtems_disk_delete(dev_a);
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);

  sc = rtems_disk_delete(dev_b);
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);
}

static void Init(rtems_task_argument arg)
{
  puts("\n\n*** TEST BLOCK 13 ***");

  test();

  puts("*** END OF TEST BLOCK 13 ***");

  rtems_test_exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#define CONFIGURE_BDBUF_BUFFER_MIN_SIZE 1
#define CONFIGURE_BDBUF_BUFFER_MAX_SIZE 1
#define CONFIGURE_BDBUF_CACHE_MEMORY_SIZE CACHE_SIZE

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_MAXIMUM_TASKS 2

#define CONFIGURE_EXTRA_TASK_STACKS (8 * 1024)

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
