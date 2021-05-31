include $(top_srcdir)/Makefile.decl

backenddir = $(libdir)/gtk-3.0/$(CTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-cloudprint.la

libprintbackend_cloudprint_la_SOURCES = \
	gtkprintbackendcloudprint.h \
	gtkprintbackendcloudprint.c \
	gtkprintercloudprint.h \
	gtkprintercloudprint.c \
	gtkcloudprintaccount.h \
	gtkcloudprintaccount.c

libprintbackend_cloudprint_la_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/gtk				\
	-I$(top_builddir)/gtk				\
	-I$(top_srcdir)/gdk				\
	-I$(top_builddir)/gdk				\
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
	$(top_builddir)/gtk/libgtk-3.la			\
	$(REST_LIBS)					\
	$(JSON_GLIB_LIBS)				\
	$(CTK_DEP_LIBS)

-include $(top_srcdir)/git.mk
