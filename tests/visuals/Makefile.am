AM_CPPFLAGS =				\
	-I$(top_srcdir)			\
	-I$(top_builddir)/gdk		\
	-I$(top_srcdir)/gdk		\
	$(CTK_DEBUG_FLAGS)		\
	$(CTK_DEP_CFLAGS)		\
	$(GDK_DEP_CFLAGS)

AM_CXXFLAGS = $(AM_CPPFLAGS)

DEPS = \
	$(top_builddir)/ctk/libctk-3.la

LDADDS = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(top_builddir)/gdk/libgdk-3.la	\
	$(CTK_DEP_LIBS)			\
	-lm

noinst_PROGRAMS =  visuals

visuals_DEPENDENCIES = $(TEST_DEPS)
visuals_LDADD = $(LDADDS)
visuals_SOURCES = visuals.c

EXTRA_DIST = \
	inline-toolbar-horizontal.ui \
	inline-toolbar-vertical.ui \
	level-bar.ui \
	linked-buttons-horizontal.ui \
	linked-buttons-vertical.ui \
	osd-toolbars.ui \
	primary-toolbar.ui \
	suggested-action-buttons.ui \
	meson.build

-include $(top_srcdir)/git.mk
