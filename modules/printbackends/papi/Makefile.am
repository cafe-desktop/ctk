include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/gtk				\
	-I$(top_builddir)/gtk				\
	-I$(top_srcdir)/gdk				\
	-I$(top_builddir)/gdk				\
	-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(CTK_DEP_CFLAGS)				\
	$(CTK_DEBUG_FLAGS)

LDADDS = \
	$(top_builddir)/gtk/libgtk-3.la	\
	$(CTK_DEP_LIBS)

backenddir = $(libdir)/gtk-3.0/$(CTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-papi.la

libprintbackend_papi_la_SOURCES =	\
	gtkprinterpapi.c		\
	gtkprintbackendpapi.c

noinst_HEADERS =			\
	gtkprinterpapi.h		\
	gtkprintbackendpapi.h

libprintbackend_papi_la_LDFLAGS =  -avoid-version -module $(no_undefined)
libprintbackend_papi_la_LIBADD = $(LDADDS) -lpapi

-include $(top_srcdir)/git.mk
