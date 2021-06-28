## Makefile.am for ctk+/cdk
include $(top_srcdir)/Makefile.decl
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ENV = \
   CC="$(CC)"
INTROSPECTION_SCANNER_ARGS = \
	--add-include-path=../cdk \
	--warn-all
INTROSPECTION_COMPILER_ARGS = \
   --includedir=$(srcdir) \
   --includedir=.

SUBDIRS = $(CDK_BACKENDS) .

DIST_SUBDIRS = win32 x11 quartz broadway wayland

CLEANFILES =

EXTRA_DIST += 			\
	keynames.txt		\
	keynames-translate.txt	\
	keyname-table.h		\
	cdkkeynames.c		\
	gen-keyname-table.pl	\
	cdkconfig.h.win32	\
	cdkconfig.h.win32_broadway	\
	cdkkeysyms-update.pl	\
	cdkmarshalers.list	\
	cdkwindowimpl.h		\
	makefile.msc		\
	cdkenumtypes.c.template	\
	cdkenumtypes.h.template	\
	cdkversionmacros.h.in	\
	meson.build \
	gen-cdk-gresources-xml.py \
	cdkconfig.h.meson

CDK_CFLAGS_DEFINES =	\
	-DG_LOG_USE_STRUCTURED=1	\
	-DCDK_COMPILATION


AM_CPPFLAGS =				\
	-DG_LOG_DOMAIN=\"Cdk\"		\
	$(CDK_CFLAGS_DEFINES)		\
	-I$(top_builddir)		\
	-I$(top_builddir)/cdk		\
	-I$(top_srcdir)			\
	$(CTK_DEBUG_FLAGS)		\
	$(CDK_DEP_CFLAGS)

# libtool stuff: set version and export symbols for resolving
# since automake doesn't support conditionalized libsomething_la_LDFLAGS
# we use the general approach here
LDADD = 					\
	$(CTK_LINK_FLAGS)			\
	-version-info $(LT_VERSION_INFO)	\
	-export-dynamic 			\
	-rpath $(libdir) 			\
	$(no_undefined)

#
# setup source file variables
#
#
# CDK header files for public installation (non-generated)
#
#
cdk_public_h_sources = 				\
	cdk.h					\
	cdk-autocleanup.h			\
	cdkapplaunchcontext.h			\
	cdkcairo.h				\
	cdkcursor.h				\
	cdkdevice.h				\
	cdkdevicepad.h				\
	cdkdevicetool.h				\
	cdkdevicemanager.h			\
	cdkdisplay.h				\
	cdkdisplaymanager.h			\
	cdkdnd.h				\
	cdkdrawingcontext.h			\
	cdkevents.h				\
	cdkframetimings.h			\
	cdkglcontext.h				\
	cdkkeys.h				\
	cdkkeysyms.h				\
	cdkkeysyms-compat.h			\
	cdkmain.h				\
	cdkmonitor.h				\
	cdkpango.h				\
	cdkframeclock.h				\
	cdkpixbuf.h				\
	cdkprivate.h				\
	cdkproperty.h				\
	cdkrectangle.h				\
	cdkrgba.h				\
	cdkscreen.h				\
	cdkseat.h				\
	cdkselection.h				\
	cdktestutils.h				\
	cdkthreads.h				\
	cdktypes.h				\
	cdkvisual.h				\
	cdkwindow.h

deprecated_h_sources =				\
	deprecated/cdkcolor.h

cdk_h_sources =					\
	$(cdk_public_h_sources)			\
	$(deprecated_h_sources)

cdk_private_headers = 				\
	cdk-private.h				\
	cdkapplaunchcontextprivate.h		\
	cdkconstructor.h		\
	cdkcursorprivate.h			\
	cdkdevicemanagerprivate.h		\
	cdkdeviceprivate.h			\
	cdkdevicepadprivate.h			\
	cdkdevicetoolprivate.h			\
	cdkdisplaymanagerprivate.h		\
	cdkdisplayprivate.h			\
	cdkdndprivate.h				\
	cdkdrawingcontextprivate.h		\
	cdkframeclockidle.h			\
	cdkframeclockprivate.h			\
	cdkglcontextprivate.h			\
	cdkmonitorprivate.h			\
	cdkprofilerprivate.h			\
	cdkscreenprivate.h			\
	cdkseatprivate.h			\
	cdkseatdefaultprivate.h			\
	cdkinternals.h				\
	cdkintl.h				\
	cdkkeysprivate.h			\
	cdkvisualprivate.h			\
	cdkx.h

deprecated_c_sources =				\
	deprecated/cdkcolor.c

cdk_c_sources = 				\
	$(deprecated_c_sources)			\
	cdk-private.c				\
	cdk.c					\
	cdkapplaunchcontext.c			\
	cdkcairo.c				\
	cdkcursor.c				\
	cdkdeprecated.c				\
	cdkdevice.c				\
	cdkdevicepad.c				\
	cdkdevicetool.c				\
	cdkdevicemanager.c			\
	cdkdisplay.c				\
	cdkdisplaymanager.c			\
	cdkdnd.c				\
	cdkdrawingcontext.c			\
	cdkevents.c     			\
	cdkframetimings.c			\
	cdkgl.c					\
	cdkglcontext.c				\
	cdkglobals.c				\
	cdkkeys.c				\
	cdkkeyuni.c				\
	cdkmonitor.c				\
	cdkoffscreenwindow.c			\
	cdkframeclock.c				\
	cdkframeclockidle.c			\
	cdkpango.c				\
	cdkpixbuf-drawable.c			\
	cdkprofiler.c				\
	cdkproperty.c				\
	cdkrectangle.c				\
	cdkrgba.c				\
	cdkscreen.c				\
	cdkseat.c				\
	cdkseatdefault.c			\
	cdkselection.c				\
	cdkvisual.c				\
	cdkwindow.c				\
	cdkwindowimpl.c

cdk_built_sources = 				\
	cdkenumtypes.h				\
	cdkenumtypes.c				\
	cdkmarshalers.h 			\
	cdkmarshalers.c				\
	cdkresources.h				\
	cdkresources.c				\
	cdkversionmacros.h

#
# setup CDK sources and their dependencies
#

cdkincludedir = $(includedir)/ctk-3.0/cdk
cdkinclude_HEADERS = $(cdk_public_h_sources)
nodist_cdkinclude_HEADERS = cdkconfig.h cdkenumtypes.h cdkversionmacros.h

deprecatedincludedir = $(includedir)/ctk-3.0/cdk/deprecated
deprecatedinclude_HEADERS = $(deprecated_h_sources)

common_sources = 		\
	$(cdk_private_headers)	\
	$(cdk_c_sources)	\
	cdkenumtypes.c		\
	cdkmarshalers.c		\
	cdkmarshalers.h		\
	cdkresources.h		\
	cdkresources.c

libcdk_3_la_SOURCES = $(common_sources)
libcdk_3_la_CFLAGS = $(AM_CFLAGS) $(CDK_HIDDEN_VISIBILITY_CFLAGS)
libcdk_3_la_LIBADD = $(CDK_DEP_LIBS) $(SHM_LIBS)
libcdk_3_la_LDFLAGS = $(LDADD)

if USE_X11
libcdk_3_la_LIBADD += x11/libcdk-x11.la
endif # USE_X11

if USE_QUARTZ
libcdk_3_la_LIBADD += quartz/libcdk-quartz.la
endif # USE_QUARTZ

if USE_WIN32
libcdk_3_la_LIBADD += win32/libcdk-win32.la
libcdk_3_la_DEPENDENCIES = win32/libcdk-win32.la win32/rc/cdk-win32-res.o
libcdk_3_la_LDFLAGS += -Wl,win32/rc/cdk-win32-res.o
endif # USE_WIN32

if USE_BROADWAY
libcdk_3_la_LIBADD += broadway/libcdk-broadway.la
endif # USE_BROADWAY

if USE_WAYLAND
libcdk_3_la_LIBADD += wayland/libcdk-wayland.la
endif

if HAVE_INTROSPECTION

introspection_files = 		\
	$(filter-out cdkkeysyms-compat.h, $(cdk_h_sources))	\
	$(cdk_c_sources)	\
	$(cdk_built_sources)

Cdk-3.0.gir: libcdk-3.la Makefile
Cdk_3_0_gir_SCANNERFLAGS = 	\
	--c-include="cdk/cdk.h"
Cdk_3_0_gir_INCLUDES = Gio-2.0 CdkPixbuf-2.0 Pango-1.0 cairo-1.0
Cdk_3_0_gir_LIBS = libcdk-3.la
Cdk_3_0_gir_FILES = $(introspection_files)
Cdk_3_0_gir_CFLAGS = $(AM_CPPFLAGS)
Cdk_3_0_gir_EXPORT_PACKAGES = cdk-3.0
INTROSPECTION_GIRS += Cdk-3.0.gir
if OS_WIN32
Cdk_3_0_gir_CFLAGS += -I$(srcdir)/win32 -I$(srcdir)
endif
if USE_X11
x11_introspection_files = 		\
	x11/cdkapplaunchcontext-x11.c	\
	x11/cdkasync.c			\
	x11/cdkcursor-x11.c		\
	x11/cdkdevice-core-x11.c	\
	x11/cdkdevicemanager-core-x11.c	\
	x11/cdkdevicemanager-x11.c	\
	x11/cdkdevicemanager-xi2.c	\
	x11/cdkdevice-xi2.c		\
	x11/cdkdisplay-x11.c		\
	x11/cdkdisplaymanager-x11.c	\
	x11/cdkdnd-x11.c		\
	x11/cdkeventsource.c		\
	x11/cdkeventtranslator.c	\
	x11/cdkgeometry-x11.c		\
	x11/cdkglcontext-x11.c		\
	x11/cdkkeys-x11.c		\
	x11/cdkmain-x11.c		\
	x11/cdkmonitor-x11.c		\
	x11/cdkproperty-x11.c		\
	x11/cdkscreen-x11.c		\
	x11/cdkselection-x11.c		\
	x11/cdksettings.c		\
	x11/cdktestutils-x11.c		\
	x11/cdkvisual-x11.c		\
	x11/cdkwindow-x11.c		\
	x11/cdkxftdefaults.c		\
	x11/cdkxid.c			\
	x11/xsettings-client.c		\
	x11/cdkx.h			\
	x11/cdkx11applaunchcontext.h	\
	x11/cdkx11cursor.h		\
	x11/cdkx11device.h		\
	x11/cdkx11device-core.h		\
	x11/cdkx11device-xi2.h		\
	x11/cdkx11devicemanager.h	\
	x11/cdkx11devicemanager-core.h	\
	x11/cdkx11devicemanager-xi2.h	\
	x11/cdkx11display.h		\
	x11/cdkx11displaymanager.h	\
	x11/cdkx11dnd.h			\
	x11/cdkx11glcontext.h		\
	x11/cdkx11keys.h		\
	x11/cdkx11monitor.h		\
	x11/cdkx11property.h		\
	x11/cdkx11screen.h		\
	x11/cdkx11selection.h		\
	x11/cdkx11utils.h		\
	x11/cdkx11visual.h		\
	x11/cdkx11window.h

CdkX11-3.0.gir: libcdk-3.la Cdk-3.0.gir Makefile
CdkX11_3_0_gir_SCANNERFLAGS = 		\
	--identifier-prefix=Cdk		\
	--c-include="cdk/cdkx.h"	\
	--include-uninstalled=$(top_builddir)/cdk/Cdk-3.0.gir
CdkX11_3_0_gir_INCLUDES = Gio-2.0 CdkPixbuf-2.0 Pango-1.0 xlib-2.0
CdkX11_3_0_gir_LIBS = libcdk-3.la
CdkX11_3_0_gir_FILES = $(x11_introspection_files)
CdkX11_3_0_gir_CFLAGS = $(AM_CPPFLAGS) -L$(top_builddir)/cdk
CdkX11_3_0_gir_EXPORT_PACKAGES = cdk-x11-3.0
INTROSPECTION_GIRS += CdkX11-3.0.gir

endif # USE_X11

w32_introspection_files =		\
	win32/cdkcursor-win32.c		\
	win32/cdkdevicemanager-win32.c	\
	win32/cdkdevice-virtual.c	\
	win32/cdkdevice-win32.c		\
	win32/cdkdevice-wintab.c	\
	win32/cdkdisplaymanager-win32.c	\
	win32/cdkdisplay-win32.c	\
	win32/cdkdnd-win32.c		\
	win32/cdkevents-win32.c		\
	win32/cdkgeometry-win32.c	\
	win32/cdkglobals-win32.c	\
	win32/cdkkeys-win32.c		\
	win32/cdkmain-win32.c		\
	win32/cdkproperty-win32.c	\
	win32/cdkscreen-win32.c		\
	win32/cdkselection-win32.c	\
	win32/cdktestutils-win32.c	\
	win32/cdkwin32.h		\
	win32/cdkwin32cursor.h		\
	win32/cdkwin32display.h		\
	win32/cdkwin32displaymanager.h	\
	win32/cdkwin32dnd.h		\
	win32/cdkwin32id.c		\
	win32/cdkwin32keys.h		\
	win32/cdkwin32misc.h		\
	win32/cdkwin32screen.h		\
	win32/cdkwin32window.h		\
	win32/cdkwindow-win32.c

if OS_WIN32
CdkWin32-3.0.gir: libcdk-3.la Cdk-3.0.gir Makefile
CdkWin32_3_0_gir_SCANNERFLAGS = \
	--identifier-prefix=Cdk		\
	--c-include="cdk/cdkwin32.h"	\
	--include-uninstalled=$(top_builddir)/cdk/Cdk-3.0.gir
CdkWin32_3_0_gir_INCLUDES = Gio-2.0 CdkPixbuf-2.0 Pango-1.0
CdkWin32_3_0_gir_LIBS = libcdk-3.la
CdkWin32_3_0_gir_FILES = $(w32_introspection_files)
CdkWin32_3_0_gir_CFLAGS = $(AM_CPPFLAGS) -L$(top_builddir)/cdk -I$(srcdir)/win32 -I$(srcdir)
INTROSPECTION_GIRS += CdkWin32-3.0.gir

endif # OS_WIN32

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)

endif # HAVE_INTROSPECTION

if OS_WIN32
cdk.def: libcdk-3.la
	echo "LIBRARY libcdk-$(CTK_MAJOR_VERSION)-@LT_CURRENT_MINUS_AGE@" >$@
	echo "EXPORTS" >>$@
	objdump -p .libs/libcdk-$(CTK_MAJOR_VERSION)-@LT_CURRENT_MINUS_AGE@.dll | sed -n -e 's/\t\[\s\+\([0-9]\+\)\]\s\([A-Za-z_][A-Za-z0-9_]\)/\2/p' >>$@
install-def-file: cdk.def
	mkdir -p $(DESTDIR)$(libdir)
	$(INSTALL) cdk.def $(DESTDIR)$(libdir)/cdk-win32-3.0.def
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/cdk-win32-3.0.def
else
install-def-file:
uninstall-def-file:
endif

if MS_LIB_AVAILABLE
noinst_DATA = cdk-win32-$(CTK_API_VERSION).lib

cdk-win32-$(CTK_API_VERSION).lib: libcdk-3.la cdk.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libcdk-win32-$(CTK_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:cdk.def -out:$@

install-ms-lib:
	mkdir -p $(DESTDIR)$(libdir)
	$(INSTALL) cdk-win32-$(CTK_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/cdk-win32-$(CTK_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

lib_LTLIBRARIES = libcdk-3.la

MAINTAINERCLEANFILES = $(cdk_built_sources) stamp-cdkenumtypes.h
EXTRA_DIST += \
	$(cdk_built_sources)	\
	fallback-c89.c

install-exec-hook:
if DISABLE_EXPLICIT_DEPS
	$(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/libcdk-3.la
endif

BUILT_SOURCES = \
	$(cdk_built_sources)			\
	cdkconfig.h

cdkenumtypes.h: stamp-cdkenumtypes.h
	@true
stamp-cdkenumtypes.h: $(cdk_h_sources) cdkenumtypes.h.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) --template cdkenumtypes.h.template \
		$(cdk_h_sources) ) >> xgen-geth \
	&& (cmp -s xgen-geth cdkenumtypes.h || cp xgen-geth cdkenumtypes.h ) \
	&& rm -f xgen-geth \
	&& echo timestamp > $(@F)
cdkenumtypes.c: $(cdk_h_sources) cdkenumtypes.c.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) --template cdkenumtypes.c.template \
		$(cdk_h_sources) ) > xgen-getc \
	&& cp xgen-getc cdkenumtypes.c  \
	&& rm -f xgen-getc

#
# Marshaller generation
#
cdkmarshalers.h: cdkmarshalers.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) \
		--prefix=_cdk_marshal \
		--valist-marshallers \
		--header \
		$(srcdir)/cdkmarshalers.list > cdkmarshalers-h.tmp \
	&& mv cdkmarshalers-h.tmp cdkmarshalers.h \
	|| ( rm -f cdkmarshalers-h.tmp && exit 1)
cdkmarshalers.c: cdkmarshalers.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) \
		--prefix=_cdk_marshal \
		--valist-marshallers \
		--body \
		$(srcdir)/cdkmarshalers.list > cdkmarshalers-c.tmp	\
	&& mv cdkmarshalers-c.tmp cdkmarshalers.c \
	|| ( rm -f cdkmarshalers-c.tmp && exit 1 )

cdkconfig.h: stamp-gc-h
	$(AM_V_GEN) if test -f cdkconfig.h; then :; \
	else rm -f stamp-gc-h; $(MAKE) stamp-gc-h; fi
stamp-gc-h: $(top_builddir)/config.status
	$(AM_V_at) cd $(top_builddir) && $(SHELL) ./config.status cdk/cdkconfig.h \
	&& echo timestamp > cdk/$(@F)

#
# Resources
#

glsl_sources := $(sort $(wildcard $(srcdir)/resources/glsl/*.glsl))

cdk.gresource.xml: Makefile.am
	$(AM_V_GEN) echo "<?xml version='1.0' encoding='UTF-8'?>" > $@; \
	echo "<gresources>" >> $@; \
	echo "  <gresource prefix='/org/ctk/libcdk'>" >> $@; \
	for f in $(glsl_sources); do \
	  n=`basename $$f`; \
	  echo "    <file alias='glsl/$$n'>resources/glsl/$$n</file>" >> $@; \
	done; \
	echo "  </gresource>" >> $@; \
	echo "</gresources>" >> $@;

EXTRA_DIST += $(glsl_sources)
CLEANFILES += cdk.gresource.xml

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(builddir)/cdk.gresource.xml)

cdkresources.h: cdk.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< --target=$@ \
		--sourcedir=$(srcdir) --c-name _cdk --generate-header --manual-register --internal
cdkresources.c: cdk.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< --target=$@ \
		--sourcedir=$(srcdir) --c-name _cdk --generate-source --manual-register --internal

# ------------------- MSVC Build Items ----------------
MSVCPROJS = cdk-3

cdk_3_FILES = $(libcdk_3_la_SOURCES)
cdk_3_EXCLUDES = dummy
cdk_3_HEADERS_DIR = $(cdkincludedir)

cdk_3_HEADERS_INST = \
	$(cdkinclude_HEADERS)		\
	$(deprecatedinclude_HEADERS)	\
	$(nodist_cdkinclude_HEADERS)

cdk_3_HEADERS_EXCLUDES = dummy

include $(top_srcdir)/win32/Makefile.msvcproj

if HAVE_INTROSPECTION
# Introspection Items for MSVC
MSVC_INTROSPECT_GIRS = Cdk-3.0.gir CdkWin32-3.0.gir

BASE_MSVC_GIR_CFLAGS =			\
	$(CDK_CFLAGS_DEFINES)		\
	-I.. -I../cdk -I../cdk/win32

INTROSPECTION_INTERMEDIATE_ITEMS = \
	$(top_builddir)/win32/Cdk-3.0.gir.msvc.introspect	\
	$(top_builddir)/win32/Cdk_3_0_gir_list			\
	$(top_builddir)/win32/CdkWin32-3.0.gir.msvc.introspect	\
	$(top_builddir)/win32/CdkWin32_3_0_gir_list

Cdk_3_0_gir_MSVC_FILES = $(introspection_files)
Cdk_3_0_gir_MSVC_EXPORT_PACKAGES = $(Cdk_3_0_gir_EXPORT_PACKAGES)
Cdk_3_0_gir_MSVC_INCLUDE_GIRS = $(Cdk_3_0_gir_INCLUDES)
Cdk_3_0_gir_MSVC_LIBS = cdk-3.0
Cdk_3_0_gir_MSVC_CFLAGS = $(BASE_MSVC_GIR_CFLAGS)
Cdk_3_0_gir_MSVC_SCANNERFLAGS = $(Cdk_3_0_gir_SCANNERFLAGS)

CdkWin32_3_0_gir_MSVC_FILES = $(w32_introspection_files)
CdkWin32_3_0_gir_MSVC_INCLUDE_GIRS = $(CdkWin32_3_0_gir_INCLUDES)
CdkWin32_3_0_gir_MSVC_LIBS = $(Cdk_3_0_gir_MSVC_LIBS)
CdkWin32_3_0_gir_MSVC_CFLAGS = $(BASE_MSVC_GIR_CFLAGS)
CdkWin32_3_0_gir_MSVC_SCANNERFLAGS =	\
	--identifier-prefix=Cdk		\
	--c-include="cdk/cdkwin32.h"	\
	--include-uninstalled='./vs$$$$(VSVER)/$$$$(CFG)/$$$$(PLAT)/bin/Cdk-3.0.gir'

include $(top_srcdir)/win32/Makefile.msvc-introspection

else
INTROSPECTION_INTERMEDIATE_ITEMS =
endif

dist-hook: \
	$(top_builddir)/win32/vs9/cdk-3.vcproj	\
	$(top_builddir)/win32/vs9/cdk-3.headers	\
	$(INTROSPECTION_INTERMEDIATE_ITEMS)

DISTCLEANFILES = cdkconfig.h stamp-gc-h

install-data-local: install-ms-lib install-def-file

uninstall-local: uninstall-ms-lib uninstall-def-file
	rm -f $(DESTDIR)$(configexecincludedir)/cdkconfig.h

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done

-include $(top_srcdir)/git.mk
