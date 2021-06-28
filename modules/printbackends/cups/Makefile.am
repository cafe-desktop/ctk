include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/ctk				\
	-I$(top_builddir)/ctk				\
	-I$(top_srcdir)/cdk				\
	-I$(top_builddir)/cdk				\
	$(CUPS_CFLAGS)					\
	$(COLORD_CFLAGS)				\
	-DCTK_COMPILATION				\
	-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(CTK_DEP_CFLAGS)				\
	$(CTK_DEBUG_FLAGS)

LDADDS = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(COLORD_LIBS)			\
	$(CTK_DEP_LIBS)

backenddir = $(libdir)/ctk-3.0/$(CTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-cups.la

libprintbackend_cups_la_SOURCES =	\
	ctkprintbackendcups.c		\
	ctkprintercups.c		\
	ctkcupsutils.c			\
	ctkcupssecretsutils.c

noinst_HEADERS =			\
	ctkprintbackendcups.h		\
	ctkprintercups.h		\
	ctkcupsutils.h			\
	ctkcupssecretsutils.h

libprintbackend_cups_la_LDFLAGS =  -avoid-version -module $(no_undefined)
libprintbackend_cups_la_LIBADD = $(LDADDS) $(CUPS_LIBS)

-include $(top_srcdir)/git.mk
