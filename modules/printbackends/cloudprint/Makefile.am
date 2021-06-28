include $(top_srcdir)/Makefile.decl

backenddir = $(libdir)/ctk-3.0/$(CTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-cloudprint.la

libprintbackend_cloudprint_la_SOURCES = \
	ctkprintbackendcloudprint.h \
	ctkprintbackendcloudprint.c \
	ctkprintercloudprint.h \
	ctkprintercloudprint.c \
	ctkcloudprintaccount.h \
	ctkcloudprintaccount.c

libprintbackend_cloudprint_la_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/ctk				\
	-I$(top_builddir)/ctk				\
	-I$(top_srcdir)/cdk				\
	-I$(top_builddir)/cdk				\
	-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(AM_CPPFLAGS)

libprintbackend_cloudprint_la_CFLAGS = \
	$(CTK_DEP_CFLAGS)				\
	$(CTK_DEBUG_FLAGS)				\
	$(REST_CFLAGS)					\
	$(JSON_GLIB_CFLAGS)				\
	$(AM_CFLAGS)

libprintbackend_cloudprint_la_LDFLAGS =  \
	-avoid-version -module $(no_undefined)

libprintbackend_cloudprint_la_LIBADD =			\
	$(top_builddir)/ctk/libctk-3.la			\
	$(REST_LIBS)					\
	$(JSON_GLIB_LIBS)				\
	$(CTK_DEP_LIBS)

-include $(top_srcdir)/git.mk
