MAINTAINERCLEANFILES=Makefile.in
AM_CXXFLAGS=@CXXFLAGS@ @SYNFIG_CFLAGS@ -I$(top_builddir) -I$(top_srcdir)/src -I$(top_srcdir)/test
AM_LDFLAGS = \
	$(top_builddir)/src/synfigapp/libsynfigapp.la \
	@SYNFIG_LIBS@

check_PROGRAMS=$(TESTS)

TESTS=app_layerduplicate smach

app_layerduplicate_SOURCES=app_layerduplicate.cpp test_base.h

smach_SOURCES=smach.cpp

