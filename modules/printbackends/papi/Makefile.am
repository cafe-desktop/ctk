include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/ctk				\
	-I$(top_builddir)/ctk				\
	-I$(top_srcdir)/cdk				\
	-I$(top_builddir)/cdk				\
	-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(CTK_DEP_CFLAGS)				\
	$(CTK_DEBUG_FLAGS)

LDADDS = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(CTK_DEP_LIBS)

backenddir = $(libdir)/ctk-3.0/$(CTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-papi.la

libprintbackend_papi_la_SOURCES =	\
	ctkprinterpapi.c		\
	ctkprintbackendpapi.c

noinst_HEADERS =			\
	ctkprinterpapi.h		\
	ctkprintbackendpapi.h

libprintbackend_papi_la_LDFLAGS =  -avoid-version -module $(no_undefined)
libprintbackend_papi_la_LIBADD = $(LDADDS) -lpapi

-include $(top_srcdir)/git.mk
