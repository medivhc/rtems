/*
 *  COPYRIGHT (c) 1989-2011.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <unistd.h>

#include "fstest.h"
#include "pmacros.h"

static const mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;

static const char databuf [] =
  "Happy days are here again.  Happy days are here again.1Happy "
  "days are here again.2Happy days are here again.3Happy days are here again."
  "4Happy days are here again.5Happy days are here again.6Happy days are here "
  "again.7Happy days are here again.";

static const size_t len = sizeof (databuf) - 1;

static void
test_case_enter (const char *wd)
{
  int status;

  printf ("test case: %s\n", wd);

  status = mkdir (wd, mode);
  rtems_test_assert (status == 0);

  status = chdir (wd);
  rtems_test_assert (status == 0);
}

static void
test_case_leave (void)
{
  int status;

  status = chdir ("..");
  rtems_test_assert (status == 0);
}

static bool init(const char *device,rtems_disk_device *dd)
{
  int fd;
  bool ok;

  fd = open(device, O_RDWR);
  ok = fd >= 0;
  if (ok) {
    int rv;
    rv = rtems_disk_fd_get_disk_device(fd, &dd);
    ok = rv == 0;
    }
  }
  return ok;
}

static void print_dev_stats(void)
{
  rtems_disk_device *dd;
  bool is_ok;

  is_ok = init(RAMDISK_PATH,dd);
  if (is_ok) {
    

  }
  

}

  void
test ()
{
  char *file_name = "test_file";
  char *dir_name = "dir";
  const char *wd ="workspace";
  int status;
  int fd; 
  int i;
  int n;

  test_case_enter("write file");

  fd = open (file_name, O_CREAT | O_WRONLY, mode);
  status = close (fd);
  rtems_test_assert (status == 0);

  fd = open (file_name, O_WRONLY);
  rtems_test_assert (fd >= 0);
  for (i = 0; i < 10; ++i) {
    n = write (fd, databuf, len);
    rtems_test_assert (n == len);
  }
  status = close (fd);
  rtems_test_assert (status == 0);


  status = mkdir (wd, mode);
  rtems_test_assert (status == 0);


  char *readbuf = (char *) malloc (len + 1);
  rtems_test_assert (readbuf);

  fd = open (file_name, O_RDONLY);
  rtems_test_assert (fd >= 0);
  for (i = 0; i < 10; ++i) {
    n = read (fd, readbuf, len);
    rtems_test_assert (n == len);
  }

  status = close (fd);
  rtems_test_assert (status == 0);

  test_case_leave();
}
