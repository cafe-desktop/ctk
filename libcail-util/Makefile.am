include $(top_srcdir)/Makefile.decl

EXTRA_DIST += \
	cailutil.def \
	meson.build

if OS_WIN32
export_symbols = -export-symbols $(srcdir)/cailutil.def
cailutil.def: libcailutil-3.la
	echo "LIBRARY libcailutil-$(CTK_MAJOR_VERSION)-$(GAIL_LT_CURRENT_MINUS_AGE)" >$@
	echo "EXPORTS" >>$@
	objdump -p .libs/libcailutil-$(CTK_MAJOR_VERSION)-$(GAIL_LT_CURRENT_MINUS_AGE).dll | sed -n -e 's/\t\[\s\+\([0-9]\+\)\]\s\([A-Za-z_][A-Za-z0-9_]\)/\2/p' >>$@
install-def-file: cailutil.def
	$(INSTALL) cailutil.def $(DESTDIR)$(libdir)
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/cailutil.def
else
install-def-file:
uninstall-def-file:
endif

if MS_LIB_AVAILABLE
noinst_DATA = cailutil.lib

install-ms-lib:
	$(INSTALL) cailutil.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/cailutil.lib
else
install-ms-lib:
uninstall-ms-lib:
endif


lib_LTLIBRARIES = libcailutil-3.la

util_c_sources =		\
	cailmisc.c		\
	cailtextutil.c

libcailutilincludedir=$(includedir)/cail-3.0/libcail-util

util_public_h_sources =		\
	cailmisc.h		\
	cailtextutil.h		\
	cail-util.h

libcailutil_3_la_SOURCES = 	\
	$(util_c_sources)

libcailutilinclude_HEADERS =	\
	$(util_public_h_sources)

libcailutil_3_la_CPPFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/cdk	\
	-I$(top_builddir)/cdk	\
	-I$(top_srcdir)/ctk	\
	-I$(top_builddir)/ctk	\
	$(AM_CPPFLAGS) 		\
	-DCDK_DISABLE_DEPRECATED\
	-DCTK_DISABLE_DEPRECATED

libcailutil_3_la_CFLAGS = \
	$(CTK_DEP_CFLAGS)	\
	$(CTK_DEBUG_FLAGS)	\
	$(AM_CFLAGS)

libcailutil_3_la_LIBADD = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(top_builddir)/cdk/libcdk-3.la	\
	$(CTK_DEP_LIBS)

libcailutil_3_la_LDFLAGS = \
	-version-info $(GAIL_LT_VERSION_INFO)	\
	$(no_undefined)				\
        $(LDFLAGS)

cailutil.lib: libcailutil-3.la cailutil.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libcailutil-3.0-$(GAIL_LT_CURRENT_MINUS_AGE).dll -def:cailutil.def -out:$@

install-data-local: install-ms-lib install-def-file

uninstall-local: uninstall-ms-lib uninstall-def-file

# ------------------- MSVC Build Items ----------------
MSVCPROJS = cailutil-3

cailutil_3_FILES = $(libcailutil_3_la_SOURCES)

cailutil_3_EXCLUDES = dummy
cailutil_3_HEADERS_DIR = $(libcailutilincludedir)

cailutil_3_HEADERS_INST = $(libcailutilinclude_HEADERS)

cailutil_3_HEADERS_EXCLUDES = dummy

include $(top_srcdir)/win32/Makefile.msvcproj

dist-hook: \
	$(top_builddir)/win32/vs9/cailutil-3.vcproj	\
	$(top_builddir)/win32/vs9/cailutil-3.headers

-include $(top_srcdir)/git.mk
