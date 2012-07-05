
rtems_tests_PROGRAMS = mrfs_fsbenchmark
mrfs_fsbenchmark_SOURCES  = ../fsbenchmark/init.c
mrfs_fsbenchmark_SOURCES += ../support/ramdisk_support.c
mrfs_fsbenchmark_SOURCES += ../support/fstest_support.c
mrfs_fsbenchmark_SOURCES += ../support/fstest_support.h
mrfs_fsbenchmark_SOURCES += ../support/ramdisk_support.h
mrfs_fsbenchmark_SOURCES += ../support/fstest.h
mrfs_fsbenchmark_SOURCES += ../../psxtests/include/pmacros.h
mrfs_fsbenchmark_SOURCES += ../mrfs_support/fs_support.c
mrfs_fsbenchmark_SOURCES += ../mrfs_support/fs_config.h

#dist_rtems_tests_DATA = mrfs_fsbenchmark.scn
#dist_rtems_tests_DATA += mrfs_fsbenchmark.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am


AM_CPPFLAGS += -I$(top_srcdir)/support
AM_CPPFLAGS += -I$(top_srcdir)/mrfs_support
AM_CPPFLAGS += -I$(top_srcdir)/../support/include
AM_CPPFLAGS += -I$(top_srcdir)/../psxtests/include

LINK_OBJS = $(mrfs_fsbenchmark_OBJECTS)
LINK_LIBS = $(mrfs_fsbenchmark_LDLIBS)

mrfs_fsbenchmark$(EXEEXT): $(mrfs_fsbenchmark_OBJECTS) $(mrfs_fsbenchmark_DEPENDENCIES)
	@rm -f mrfs_fsbenchmark$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
