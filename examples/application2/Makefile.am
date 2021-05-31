CTK_CFLAGS =	 				\
	-I$(top_srcdir)				\
	$(CTK_DEP_CFLAGS)

CTK_LIBS = 					\
        $(top_builddir)/gtk/libgtk-3.la 	\
        $(top_builddir)/gdk/libgdk-3.la 	\
        $(CTK_DEP_LIBS)


noinst_PROGRAMS = exampleapp

exampleapp_CFLAGS = $(CTK_CFLAGS)
exampleapp_LDADD = $(CTK_LIBS)
exampleapp_SOURCES = 				\
	main.c 					\
	exampleapp.c exampleapp.h 		\
	exampleappwin.c exampleappwin.h 	\
	resources.c

BUILT_SOURCES = resources.c

resources.c: exampleapp.gresource.xml window.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $(srcdir)/exampleapp.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source

EXTRA_DIST = 					\
	window.ui 				\
	exampleapp.gresource.xml	\
	meson.build

-include $(top_srcdir)/git.mk
