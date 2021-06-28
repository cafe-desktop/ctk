include $(top_srcdir)/Makefile.decl

backenddir = $(libdir)/ctk-3.0/$(CTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-test.la

libprintbackend_test_la_SOURCES = \
	ctkprintbackendtest.c

libprintbackend_test_la_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/ctk				\
	-I$(top_builddir)/ctk				\
	-I$(top_srcdir)/cdk				\
	-I$(top_builddir)/cdk				\
	-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(AM_CPPFLAGS)

libprintbackend_test_la_CFLAGS = \
	$(CTK_DEP_CFLAGS)				\
	$(AM_CFLAGS)

libprintbackend_test_la_LDFLAGS =  \
	-avoid-version -module $(no_undefined)

libprintbackend_test_la_LIBADD = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(CTK_DEP_LIBS)

noinst_HEADERS =			\
	ctkprintbackendtest.h

-include $(top_srcdir)/git.mk
