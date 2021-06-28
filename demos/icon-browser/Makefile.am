AM_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/cdk		\
	-I$(top_builddir)/cdk		\
	$(CTK_DEBUG_FLAGS)		\
	$(CTK_DEP_CFLAGS)

LDADD = \
        $(top_builddir)/ctk/libctk-3.la \
        $(top_builddir)/cdk/libcdk-3.la \
        $(CTK_DEP_LIBS)


bin_PROGRAMS = ctk3-icon-browser

desktopdir = $(datadir)/applications
dist_desktop_DATA = ctk3-icon-browser.desktop

ctk3_icon_browser_SOURCES = \
	main.c \
	iconbrowserapp.c iconbrowserapp.h \
	iconbrowserwin.c iconbrowserwin.h \
	iconstore.c iconstore.h \
	resources.c

BUILT_SOURCES = \
	resources.c

resources.c: iconbrowser.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/iconbrowser.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $(srcdir)/iconbrowser.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source

EXTRA_DIST = \
	menus.ui \
	iconbrowser.gresource.xml \
	window.ui \
	icon.list \
	meson.build

# ------------------- MSVC Build Items ----------------
MSVCPROJS = ctk3-icon-browser

ctk3_icon_browser_FILES = $(ctk3_icon_browser_SOURCES)
ctk3_icon_browser_EXCLUDES = dummy

include $(top_srcdir)/win32/Makefile.msvcproj

dist-hook: $(top_builddir)/win32/vs9/ctk3-icon-browser.vcproj

-include $(top_srcdir)/git.mk
