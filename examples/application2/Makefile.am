CTK_CFLAGS =	 				\
	-I$(top_srcdir)				\
	$(CTK_DEP_CFLAGS)

CTK_LIBS = 					\
        $(top_builddir)/ctk/libctk-3.la 	\
        $(top_builddir)/cdk/libcdk-3.la 	\
        $(CTK_DEP_LIBS)


noinst_PROGRAMS = exampleapp

exampleapp_CFLAGS = $(CTK_CFLAGS)
exampleapp_LDADD = $(CTK_LIBS)
exampleapp_SOURCES = 				\
	main.c 					\
	exampleapp.c exampleapp.h 		\
	exampleappwin.c exampleappwin.h
nodist_exampleapp_SOURCES =			\
	resources.c

BUILT_SOURCES = $(nodist_exampleapp_SOURCES)

resources.c: exampleapp.gresource.xml window.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $(srcdir)/exampleapp.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source

EXTRA_DIST = 					\
	window.ui 				\
	exampleapp.gresource.xml	\
	meson.build

CLEANFILES = $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
