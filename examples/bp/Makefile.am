AM_CPPFLAGS = \
	-DSRCDIR=\"$(abs_srcdir)\"	\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/cdk		\
	-I$(top_builddir)/cdk		\
	$(CTK_DEBUG_FLAGS)		\
	$(CTK_DEP_CFLAGS)

LDADD = \
        $(top_builddir)/ctk/libctk-3.la \
        $(top_builddir)/cdk/libcdk-3.la \
        $(CTK_DEP_LIBS)

noinst_PROGRAMS = bloatpad


bloatpad_SOURCES = bloatpad.c
nodist_bloatpad_SOURCES = bloatpad-gresources.c

resource_files = \
	bloatpad.gresources.xml		\
	ctk/menus.ui			\
	ctk/help-overlay.ui

bloatpad-gresources.c: $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

CLEANFILES = bloatpad-gresources.c

EXTRA_DIST = \
	$(resource_files) \
	meson.build

-include $(top_srcdir)/git.mk
