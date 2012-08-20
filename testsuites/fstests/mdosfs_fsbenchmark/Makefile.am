
rtems_tests_PROGRAMS = mdosfs_fsbenchmark
mdosfs_fsbenchmark_SOURCES  = ../fsbenchmark/init.c
mdosfs_fsbenchmark_SOURCES += ../support/ramdisk_support.c
mdosfs_fsbenchmark_SOURCES += ../support/fstest_support.c
mdosfs_fsbenchmark_SOURCES += ../support/fstest_support.h
mdosfs_fsbenchmark_SOURCES += ../support/ramdisk_support.h
mdosfs_fsbenchmark_SOURCES += ../support/fstest.h
mdosfs_fsbenchmark_SOURCES += ../../psxtests/include/pmacros.h
mdosfs_fsbenchmark_SOURCES += ../mdosfs_support/fs_support.c
mdosfs_fsbenchmark_SOURCES += ../mdosfs_support/fs_config.h

#dist_rtems_tests_DATA = mdosfs_fsbenchmark.scn
#dist_rtems_tests_DATA += mdosfs_fsbenchmark.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am


AM_CPPFLAGS += -I$(top_srcdir)/support
AM_CPPFLAGS += -I$(top_srcdir)/mdosfs_support
AM_CPPFLAGS += -I$(top_srcdir)/../support/include
AM_CPPFLAGS += -I$(top_srcdir)/../psxtests/include

LINK_OBJS = $(mdosfs_fsbenchmark_OBJECTS)
LINK_LIBS = $(mdosfs_fsbenchmark_LDLIBS)

mdosfs_fsbenchmark$(EXEEXT): $(mdosfs_fsbenchmark_OBJECTS) $(mdosfs_fsbenchmark_DEPENDENCIES)
	@rm -f mdosfs_fsbenchmark$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
