
rtems_tests_PROGRAMS = mimfs_fsbenchmark
mimfs_fsbenchmark_SOURCES  = ../fsbenchmark/init.c
mimfs_fsbenchmark_SOURCES += ../support/fstest_support.c
mimfs_fsbenchmark_SOURCES += ../support/fstest_support.h
mimfs_fsbenchmark_SOURCES += ../support/fstest.h
mimfs_fsbenchmark_SOURCES += ../../psxtests/include/pmacros.h
mimfs_fsbenchmark_SOURCES += ../mimfs_support/fs_support.c
mimfs_fsbenchmark_SOURCES += ../mimfs_support/fs_config.h

#dist_rtems_tests_DATA = mimfs_fsbenchmark.scn
#dist_rtems_tests_DATA += mimfs_fsbenchmark.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am


AM_CPPFLAGS += -I$(top_srcdir)/support
AM_CPPFLAGS += -I$(top_srcdir)/mimfs_support
AM_CPPFLAGS += -I$(top_srcdir)/../support/include
AM_CPPFLAGS += -I$(top_srcdir)/../psxtests/include

LINK_OBJS = $(mimfs_fsbenchmark_OBJECTS)
LINK_LIBS = $(mimfs_fsbenchmark_LDLIBS)

mimfs_fsbenchmark$(EXEEXT): $(mimfs_fsbenchmark_OBJECTS) $(mimfs_fsbenchmark_DEPENDENCIES)
	@rm -f mimfs_fsbenchmark$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
