## Makefile.am for ctk+/demos
include $(top_srcdir)/Makefile.decl

## These should be in the order you want them to appear in the
## demo app, which means alphabetized by demo title, not filename
demos_base =					\
	application_demo.c			\
	assistant.c				\
	builder.c				\
	button_box.c				\
	changedisplay.c				\
	clipboard.c				\
	colorsel.c				\
	combobox.c				\
	css_accordion.c                         \
	css_basics.c				\
	css_blendmodes.c			\
	css_multiplebgs.c			\
	css_pixbufs.c				\
	css_shadows.c				\
	cursors.c				\
	dialog.c				\
	drawingarea.c				\
	editable_cells.c			\
	entry_buffer.c				\
	entry_completion.c			\
	event_axes.c				\
	expander.c  				\
        filtermodel.c				\
	fishbowl.c				\
	foreigndrawing.c			\
	gestures.c				\
	glarea.c				\
	headerbar.c				\
	hypertext.c				\
	iconview.c				\
	iconview_edit.c				\
	images.c				\
	infobar.c				\
	links.c					\
	listbox.c				\
	flowbox.c				\
	list_store.c				\
	markup.c				\
	menus.c					\
	modelbutton.c				\
	offscreen_window.c			\
	offscreen_window2.c			\
	overlay.c				\
	overlay2.c				\
	paint.c					\
	panes.c					\
	pickers.c				\
	pixbufs.c				\
	popover.c				\
	printing.c				\
	revealer.c				\
	rotated_text.c				\
	scale.c					\
	search_entry.c				\
	search_entry2.c				\
	shortcuts.c				\
	sidebar.c				\
	sizegroup.c				\
	spinbutton.c				\
	spinner.c				\
	stack.c					\
	tabs.c					\
	textmask.c				\
	textview.c				\
	textscroll.c				\
	theming_style_classes.c			\
	toolpalette.c				\
	transparent.c				\
	tree_store.c

demos_opt =

if BUILD_FONT_DEMO
demos_opt += font_features.c
endif

if OS_UNIX
demos_opt += pagesetup.c
endif

demos = $(demos_base) $(demos_opt)

AM_CPPFLAGS = \
	-I$(top_srcdir) 			\
	-I$(top_builddir)/cdk			\
	-DGDK_DISABLE_DEPRECATED		\
	-DCTK_DISABLE_DEPRECATED 		\
	$(CTK_DEBUG_FLAGS)			\
	$(CTK_DEP_CFLAGS)

DEPS = \
	$(top_builddir)/ctk/libctk-3.la

LDADDS = \
	$(top_builddir)/ctk/libctk-3.la		\
	$(top_builddir)/cdk/libcdk-3.la		\
	$(CTK_DEP_LIBS)				\
	$(GDK_DEP_LIBS)				\
	-lm

bin_PROGRAMS = ctk3-demo ctk3-demo-application

desktopdir = $(datadir)/applications
dist_desktop_DATA = ctk3-demo.desktop

BUILT_SOURCES = demos.h demo_resources.c

EXTRA_DIST += 				\
	data/source.svg			\
	data/symbolic-source.svg	\
	demo.gresource.xml		\
	$(resource_files)		\
	makefile.msc.in			\
	org.ctk.Demo.gschema.xml	\
	demos.h.win32	\
	meson.build	\
	geninclude.py

gsettings_SCHEMAS = \
	org.ctk.Demo.gschema.xml

@GSETTINGS_RULES@

demos.h: $(demos) geninclude.pl
	 $(AM_V_GEN) (here=`pwd` ; cd $(srcdir) && $(PERL) $$here/geninclude.pl $(demos)) > demos.h

demos.h.win32: $(demos_base) geninclude.pl
	 $(AM_V_GEN) (here=`pwd` ; cd $(srcdir) && $(PERL) $$here/geninclude.pl $(demos_base)) > demos.h.win32

nodist_ctk3_demo_SOURCES = demos.h

ctk3_demo_SOURCES = 		\
	$(demos)		\
	ctkfishbowl.c		\
	ctkfishbowl.h		\
	demo_resources.c	\
	main.c

ctk3_demo_DEPENDENCIES = $(DEPS)
ctk3_demo_LDADD = $(LDADDS)
ctk3_demo_LDFLAGS = -export-dynamic

ctk3_demo_application_SOURCES = \
	application.c 		\
	demo_resources.c

ctk3_demo_application_LDADD = $(LDADDS)

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/demo.gresource.xml)

demo_resources.c: demo.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $(srcdir)/demo.gresource.xml

iconthemedir = $(datadir)/icons/hicolor

appsicon16dir = $(iconthemedir)/16x16/apps
appsicon22dir = $(iconthemedir)/22x22/apps
appsicon24dir = $(iconthemedir)/24x24/apps
appsicon32dir = $(iconthemedir)/32x32/apps
appsicon48dir = $(iconthemedir)/48x48/apps
appsicon256dir = $(iconthemedir)/256x256/apps
appsiconscalabledir = $(iconthemedir)/scalable/apps

dist_appsicon16_DATA = data/16x16/ctk3-demo.png data/16x16/ctk3-demo-symbolic.symbolic.png
dist_appsicon22_DATA = data/22x22/ctk3-demo.png data/22x22/ctk3-demo-symbolic.symbolic.png
dist_appsicon24_DATA = data/24x24/ctk3-demo.png data/24x24/ctk3-demo-symbolic.symbolic.png
dist_appsicon32_DATA = data/32x32/ctk3-demo.png data/32x32/ctk3-demo-symbolic.symbolic.png
dist_appsicon48_DATA = data/48x48/ctk3-demo.png data/48x48/ctk3-demo-symbolic.symbolic.png
dist_appsicon256_DATA = data/256x256/ctk3-demo.png data/256x256/ctk3-demo-symbolic.symbolic.png

update_icon_cache = $(top_builddir)/ctk/ctk-update-icon-cache$(EXEEXT) --ignore-theme-index --force

install-data-hook: install-update-icon-cache
uninstall-hook: uninstall-update-icon-cache

install-update-icon-cache:
	$(AM_V_at)$(POST_INSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

uninstall-update-icon-cache:
	$(AM_V_at)$(POST_UNINSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

# ------------------- MSVC Build Items ----------------
MSVCPROJS = ctk3-demo ctk3-demo-application

ctk3_demo_FILES = $(ctk3_demo_SOURCES)
ctk3_demo_EXCLUDES = font_features.c|pagesetup.c

ctk3_demo_application_FILES = $(ctk3_demo_application_SOURCES)
ctk3_demo_application_EXCLUDES = dummy

include $(top_srcdir)/win32/Makefile.msvcproj

dist-hook: \
	$(top_builddir)/win32/vs9/ctk3-demo.vcproj	\
	$(top_builddir)/win32/vs9/ctk3-demo-application.vcproj

DISTCLEANFILES = demos.h demos.h.win32

-include $(top_srcdir)/git.mk
