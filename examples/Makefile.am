SUBDIRS = \
	bp \
	application1 application2 application3 \
	application4 application5 application6 \
	application7 application8 application9 \
	application10

AM_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/cdk			\
	-I$(top_builddir)/cdk			\
	-DCDK_DISABLE_DEPRECATED		\
	-DCTK_DISABLE_DEPRECATED		\
	$(CTK_DEBUG_FLAGS)			\
	$(CTK_DEP_CFLAGS)

LDADD = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(top_builddir)/cdk/libcdk-3.la \
	$(CTK_DEP_LIBS)

noinst_PROGRAMS = \
	hello-world				\
	window-default				\
	plugman					\
	sunny					\
	action-namespace			\
	grid-packing				\
	drawing					\
	builder					\
	search-bar				\
	listbox-dnd

EXTRA_DIST = \
	builder.ui \
	meson.build

-include $(top_srcdir)/git.mk
