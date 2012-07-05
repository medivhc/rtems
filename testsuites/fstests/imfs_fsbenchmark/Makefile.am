
rtems_tests_PROGRAMS = imfs_fsbenchmark
imfs_fsbenchmark_SOURCES  = ../fsbenchmark/init.c
imfs_fsbenchmark_SOURCES += ../support/fstest_support.c
imfs_fsbenchmark_SOURCES += ../support/fstest_support.h
imfs_fsbenchmark_SOURCES += ../support/fstest.h
imfs_fsbenchmark_SOURCES += ../../psxtests/include/pmacros.h
imfs_fsbenchmark_SOURCES += ../imfs_support/fs_support.c
imfs_fsbenchmark_SOURCES += ../imfs_support/fs_config.h 

#dist_rtems_tests_DATA = imfs_fsbenchmark.scn
#dist_rtems_tests_DATA += imfs_fsbenchmark.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am


AM_CPPFLAGS += -I$(top_srcdir)/support
AM_CPPFLAGS += -I$(top_srcdir)/imfs_support
AM_CPPFLAGS += -I$(top_srcdir)/../support/include
AM_CPPFLAGS += -I$(top_srcdir)/../psxtests/include

LINK_OBJS = $(imfs_fsbenchmark_OBJECTS)
LINK_LIBS = $(imfs_fsbenchmark_LDLIBS)

imfs_fsbenchmark$(EXEEXT): $(imfs_fsbenchmark_OBJECTS) $(imfs_fsbenchmark_DEPENDENCIES)
	@rm -f imfs_fsbenchmark$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
