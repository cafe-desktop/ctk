include $(top_srcdir)/Makefile.decl

backenddir = $(libdir)/ctk-3.0/$(CTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-file.la

libprintbackend_file_la_SOURCES = \
	ctkprintbackendfile.c

libprintbackend_file_la_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/ctk				\
	-I$(top_builddir)/ctk				\
	-I$(top_srcdir)/gdk				\
	-I$(top_builddir)/gdk				\
	-DCTK_COMPILATION				\
	-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(AM_CPPFLAGS)

libprintbackend_file_la_CFLAGS = \
	$(CTK_DEP_CFLAGS)				\
	$(AM_CFLAGS)

libprintbackend_file_la_LDFLAGS =  \
	-avoid-version -module $(no_undefined)

libprintbackend_file_la_LIBADD = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(CTK_DEP_LIBS)

noinst_HEADERS =			\
	ctkprintbackendfile.h

-include $(top_srcdir)/git.mk
