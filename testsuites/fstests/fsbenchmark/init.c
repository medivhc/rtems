/*
 *  COPYRIGHT (c) 1989-2011.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */




/*
 * Every operation divided by '\n'. Reading 4 bytes from file 2 starting
 * at 3  could be expressed as  "1 R 2 3 4". The order of the parameters 
 * is index operation file_name start size. index here have no meaning now.
 * operation  could be R W L C U which mean read write lseek close and
 * unlink respectively. file_name could be [0-MAX_FILE_NUM-1] or L which
 * means last opened file. start and size could be positive integer or r
 * which means random. If start is 0, it will do nothing, else I will call
 * lseek SEEK_SET before read or write. If file with file_name does not exists,
 * W R L will create file with O_WRONLY O_RDONLY O_RDWR respectively. 
 * The test will not handle any error or check any data integrity. If error
 * occurs, it just use perror() to print them.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/libio.h>
#include <rtems/blkdev.h>
#include <rtems/bdbuf.h>

#include "fstest.h"
#include "pmacros.h"
#include "ramdisk_support.h"

#define MAX_OPEN_FILE (10)
#define MAX_LEN 1024*64

static int fd_array[MAX_OPEN_FILE];
const static  int RUN_TIMES = 5;

const static mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
char workload_input[] = {
  "1 W 0 0 r\n"
    "1 W L r r\nn"
    "1 W L 0 r\n"
    "1 W L 0 r\n"
    "1 W L 0 1024\n"
    "1 W L 0 1024\n"
    "1 L L r 1024\n"
    "1 C L 0 1024\n"
    "1 R 0 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 r\n"
    "1 L L r 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L r r\n"
    "1 R L 0 1024\n"
    "1 R L r 1024\n"
    "1 R L 0 r\n"
    "1 L L r r\n"
    "1 R L 0 r\n"
    "1 C L 0 1024\n"
    "1 W 0 0 r\n"
    "1 W L r r\nn"
    "1 L L r 1024\n"
    "1 W L 0 r\n"
    "1 W L 0 r\n"
    "1 W L 0 1024\n"
    "1 W L 0 1024\n"
    "1 C L 0 r\n"
    "1 L L r r\n"
    "1 L 1 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 r\n"
    "1 W L 0 r\n"
    "1 W L 0 r\n"
    "1 L L r 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 r\n"
    "1 R L 0 1024\n"
    "1 R L r r\n"
    "1 R L 0 1024\n"
    "1 L 0 0 1024\n"
    "1 R L 0 r\n"
    "1 R L 0 r\n"
    "1 R L 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 1024\n"
    "1 R L 0 r\n"
    "1 R L 0 1024\n"
    "1 R L 0 r\n"
    "1 R L 0 1024\n"
    "1 L 3 0 1024\n"
    "1 L L r 1024\n"
    "1 W L 0 1024\n"
    "1 W L 0 1024\n"
    "1 W L 0 1024\n"
    "1 W L 0 r\n"
    "1 L L r 1024\n"
    "1 W L 0 1024\n"
    "1 W L 0 r\n"
    "1 L L 0 r\n"
    "1 L L 0 1024\n"
    "1 C 3\n"
};

  static void
check_status (char *message, int result)
{
  if (!result)
  {
    perror (message);
  }
}



typedef struct benchmark_test_state
{
  int is_colsed;
  int previous_fd;
  int previous_name;
  off_t pos;
} benchmark_test_state;
benchmark_test_state state;

typedef enum test_arg_type
{
  INDEX,
  OPERATION,
  FILE_NAME,
  START,
  SIZE,
  OTHER
} test_arg_type;

struct test_item
{
  int index;
  char name[3];
  char op;
  off_t begin;
  size_t size;
  int fd;
};

  static long
prase_long (const char*str)
{
  if (isdigit ((int) *str))
  {
    return atol (str);
  }
  else
  {
    switch (*str)
    {
      case 'l':
        return state.pos;
        break;
      case 'r':
        return (lrand48 () % (long)MAX_LEN)+1;
      case 'u':
        return 0;
    }
    return 0;

  }
}
  static void
fill_test_entry (test_arg_type arg, char *pch, struct test_item * it)
{

  switch (arg)
  {
    case INDEX:
      it->index = atoi (pch);
      break;
    case OPERATION:
      it->op = *pch;
      break;
    case FILE_NAME:
      strncpy (it->name, pch, strlen (pch));
      it->name[strlen (pch)] = '\0';
      break;
    case START:
      it->begin = prase_long (pch);
      break;
    case SIZE:
      it->size = prase_long (pch);
      break;
    case OTHER:
      break;
    default:
      exit (1);
  }
}
static void do_common_before (struct test_item *it)
{
  int fd;
  int rv = 0;
  char first_char = it->name[0];
  if (isdigit ((int )first_char))
  { /* file name */
    if (state.previous_fd != -1)
    {
      rv = close (state.previous_fd);
      check_status ("close ", rv == 0);
      fd_array[state.previous_name] = -1;
    }

    switch (it->op)
    {
      case 'R':
        fd = open (it->name, O_CREAT | O_RDONLY, mode);
        break;
      case 'W':
        fd = open (it->name, O_CREAT | O_WRONLY, mode);
        break;
      case 'L':
        fd = open (it->name, O_CREAT | O_RDWR, mode);
    }
    check_status ("create ", fd != -1);

    state.previous_name = atoi (it->name);
  }
  else
  {
    fd = state.previous_fd;
  }
  it->fd = fd;
  if (it->begin != 0)
  {
    lseek (fd, it->begin, SEEK_SET);
  }
}

static  void do_common_afer (struct test_item *it)
{

  state.previous_fd = it->fd;
  fd_array[state.previous_name] = it->fd;
}

static   off_t
do_read (struct test_item *it)
{
  do_common_before (it);


  void * buffer = malloc (it->size);

  size_t n;
  size_t actual = 0;

  while ((n = read (it->fd, buffer, it->size - actual)) != -1)
  {
    if (n == 0)
      break;
    else
    {
      actual += n;
    }
  }
  check_status ("read ", n != -1);

  printf ("read  file: %d size: %d\n", state.previous_name,actual);

  free (buffer);
  do_common_afer (it);
  return actual;
}


static off_t
do_write (struct test_item *it)
{
  size_t n;
  size_t actual = 0;

  do_common_before (it);
  void * buffer = malloc (it->size);
  while ((n = write (it->fd, buffer, it->size - actual)) != -1)
  {
    if (n == 0)
      break;
    else
    {
      actual += n;
    }
  }
  free (buffer);
  check_status ("write", n != -1);
  printf ("write file: %d size: %d\n", state.previous_name,actual);
  do_common_afer (it);
  return actual;

}

static off_t
do_lseek (struct test_item *it)
{
  do_common_before (it);
  do_common_afer (it);
  return it->begin;
}

static void 
do_unlink (struct test_item *it)
{
  int rv = unlink (it->name);
  check_status ("unlink", rv == 0);
}
static void
do_close (struct test_item *it)
{
  int rv = 0;
  char first_char = it->name[0];
  if (isdigit ((int)first_char))
  { /* file name */
    rv = close (fd_array[atoi (it->name)]);
    check_status ("close", rv == 0);

    fd_array[atoi (it->name)] = -1;
  }
  else
  { /* last file */
    rv = close (state.previous_fd);
    check_status ("close", rv == 0);
  }
  it->fd = -1;

  do_common_afer(it);

}

static void
execute_entry (struct test_item *it)
{
  off_t result;
  switch (it->op)
  {
    case 'R':
      result = do_read (it);
      break;
    case 'W':
      result = do_write (it);
      break;
    case 'L':
      result = do_lseek (it);
      break;
    case 'U':
      do_unlink (it);
      break;
    case 'C':
      do_close (it);
      break;
  }
  state.pos = result;
}

static  void parse (char *str)
{
  char *pch;
  pch = strtok (str, " ");
  test_arg_type arg = INDEX;
  struct test_item it;
  while (pch != NULL)
  {
    fill_test_entry (arg, pch, &it);
    pch = strtok (NULL, " ");
    arg++;
  }
  execute_entry (&it);
}

static void process (const char* str)
{
  char * pch;
  const char * start = str;
  char buffer[256];
   /*
    * separate the string by '\n'
    */
  while (strlen (start))
  {
    pch = (char*) memchr (start, '\n', strlen (start));
    size_t len = pch - start;

    strncpy (buffer, start, len);
    buffer[len] = '\0';
    parse (buffer);
    start = pch + 1;
  }
}

static void init (void )
{
  int i = 1;
  seed48 ((short unsigned int*) &i);
  state.previous_name = 0;
  state.pos = 0;
  state.previous_fd =-1;
}



static void print_dev_stats(void)
{
  int status;
  rtems_status_code sc;
  int fd;
  rtems_blkdev_stats stats;
  rtems_disk_device *dd;


  status = chroot("/");
  rtems_test_assert (status == 0);

  fd = open(RAMDISK_PATH, O_RDWR);
  rtems_test_assert (fd != 0);
  sc = rtems_disk_fd_get_disk_device(fd, &dd);
  rtems_test_assert (sc == 0);
  status = rtems_bdbuf_syncdev (dd);
  rtems_test_assert (status == 0);
  rtems_bdbuf_get_device_stats (dd, &stats);
  rtems_blkdev_print_stats(&stats, rtems_printf_plugin, NULL);
}

static void reset_dev_stats(void )
{

  /*
   * We have to chroot to get the ramdisk fd
   */
  int rv;
  int fd;
  rtems_status_code sc;
  rv = chroot("/");
  rtems_test_assert (rv == 0);
  fd = open( RAMDISK_PATH,O_RDWR);
  rtems_test_assert (fd != 0);
  sc = rtems_disk_fd_reset_device_stats(fd);
  rtems_test_assert (sc == 0);
  rv = close (fd);
  rtems_test_assert (rv == 0);
  rv = chroot(BASE_FOR_TEST);
  rtems_test_assert (rv == 0);
}

void test ()
{
  int i =0;
  reset_dev_stats();
  init ();
  for ( i = 0; i < RUN_TIMES; i++){
    process (workload_input);
  }
  sync();
  print_dev_stats();
}
