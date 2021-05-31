CTK_CFLAGS = 					\
	-I$(top_srcdir)				\
	$(CTK_DEP_CFLAGS)

CTK_LIBS = 					\
        $(top_builddir)/ctk/libctk-3.la 	\
        $(top_builddir)/gdk/libgdk-3.la 	\
        $(CTK_DEP_LIBS)


noinst_PROGRAMS = exampleapp

exampleapp_CFLAGS = $(CTK_CFLAGS)
exampleapp_LDADD = $(CTK_LIBS)
exampleapp_SOURCES = 				\
	main.c 					\
	exampleapp.c exampleapp.h 		\
	exampleappwin.c exampleappwin.h

EXTRA_DIST = \
	exampleapp.desktop \
	meson.build

-include $(top_srcdir)/git.mk
