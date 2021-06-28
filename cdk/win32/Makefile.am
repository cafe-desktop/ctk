## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

libcdkincludedir = $(includedir)/ctk-3.0/cdk
libcdkwin32includedir = $(includedir)/ctk-3.0/cdk/win32

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Cdk\"		\
	-DG_LOG_USE_STRUCTURED=1	\
	-DINSIDE_CDK_WIN32		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/cdk		\
	-I$(top_builddir)/cdk		\
	$(CDK_HIDDEN_VISIBILITY_CFLAGS) \
	$(CTK_DEBUG_FLAGS) 		\
	$(CDK_DEP_CFLAGS)		\
	$(CDK_WIN32_EXTRA_CFLAGS)	\
	-DCDK_COMPILATION

if WIN32_GLES
AM_CPPFLAGS += "-DCDK_WIN32_ENABLE_EGL=1"
endif #WIN32_GLES

LDADDS = $(CDK_DEP_LIBS)

noinst_LTLIBRARIES = libcdk-win32.la

SUBDIRS=rc

EXTRA_DIST += \
	bdfcursor.c \
	makefile.msc \
	meson.build

libcdk_win32_la_SOURCES = \
	cdkcursor-win32.c \
	cdkdevicemanager-win32.c \
	cdkdevicemanager-win32.h \
	cdkdevice-virtual.c \
	cdkdevice-virtual.h \
	cdkdevice-win32.c \
	cdkdevice-win32.h \
	cdkdevice-wintab.c \
	cdkdevice-wintab.h \
	cdkdisplay-win32.c \
	cdkdisplay-win32.h \
	cdkdisplaymanager-win32.c \
	cdkdnd-win32.c \
	cdkevents-win32.c \
	cdkgeometry-win32.c \
	cdkglcontext-win32.c \
	cdkglcontext-win32.h \
	cdkglobals-win32.c \
	cdkkeys-win32.c \
	cdkmain-win32.c \
	cdkmonitor-win32.c \
	cdkmonitor-win32.h \
	cdkprivate-win32.h \
	cdkproperty-win32.c \
	cdkscreen-win32.c \
	cdkselection-win32.c \
	cdkselection-win32.h \
	cdktestutils-win32.c \
	cdkwin32cursor.h \
	cdkwin32display.h \
	cdkwin32displaymanager.h \
	cdkwin32dnd.h \
	cdkwin32dnd-private.h \
	cdkwin32glcontext.h \
	cdkwin32langnotification.c \
	cdkwin32langnotification.h \
	cdkwin32.h \
	cdkwin32id.c \
	cdkwin32keys.h \
	cdkwin32monitor.h \
	cdkwin32screen.h \
	cdkwin32window.h \
	cdkwindow-win32.c \
	cdkwindow-win32.h \
	pktdef.h \
	wintab.h \
	xcursors.h

libcdkinclude_HEADERS =		\
	cdkwin32.h

libcdkwin32include_HEADERS =	\
	cdkwin32cursor.h	\
	cdkwin32display.h	\
	cdkwin32displaymanager.h\
	cdkwin32dnd.h		\
	cdkwin32glcontext.h	\
	cdkwin32keys.h		\
	cdkwin32misc.h		\
	cdkwin32monitor.h	\
	cdkwin32screen.h	\
	cdkwin32window.h

# ------------------- MSVC Build Items ----------------
MSVCPROJS = cdk3-win32

cdk3_win32_FILES = $(libcdk_win32_la_SOURCES)
cdk3_win32_EXCLUDES = dummy
cdk3_win32_HEADERS_DIR = $(libcdkwin32includedir)

cdk3_win32_HEADERS_INST = $(libcdkwin32include_HEADERS)

cdk3_win32_HEADERS_EXCLUDES = dummy

include $(top_srcdir)/win32/Makefile.msvcproj

dist-hook: \
	$(top_builddir)/win32/vs9/cdk3-win32.vcproj	\
	$(top_builddir)/win32/vs9/cdk3-win32.headers

-include $(top_srcdir)/git.mk
