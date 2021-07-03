CTK_CFLAGS =			 		\
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
	exampleappwin.c exampleappwin.h 	\
	exampleappprefs.c exampleappprefs.h
nodist_exampleapp_SOURCES =			\
	resources.c

BUILT_SOURCES = 				\
	resources.c 				\
	gschemas.compiled

resources.c: exampleapp.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/exampleapp.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $(srcdir)/exampleapp.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source

gsettings_SCHEMAS = 				\
	org.ctk.exampleapp.gschema.xml

@GSETTINGS_RULES@

gschemas.compiled: org.ctk.exampleapp.gschema.xml
	$(GLIB_COMPILE_SCHEMAS) .

EXTRA_DIST = 					\
	window.ui 				\
	app-menu.ui 				\
	prefs.ui 				\
	gears-menu.ui 				\
	exampleapp.gresource.xml 		\
	org.ctk.exampleapp.gschema.xml	\
	meson.build

CLEANFILES = 					\
	$(nodist_exampleapp_SOURCES)		\
	gschemas.compiled

-include $(top_srcdir)/git.mk
