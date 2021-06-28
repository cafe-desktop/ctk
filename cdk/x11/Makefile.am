## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

libcdkincludedir = $(includedir)/ctk-3.0/cdk
libcdkx11includedir = $(includedir)/ctk-3.0/cdk/x11

AM_CPPFLAGS = 				\
	-DG_LOG_DOMAIN=\"Cdk\"		\
	-DG_LOG_USE_STRUCTURED=1	\
	-DGDK_COMPILATION		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/cdk		\
	-I$(top_builddir)/cdk		\
	$(GDK_HIDDEN_VISIBILITY_CFLAGS) \
	$(CTK_DEBUG_FLAGS) 		\
	$(GDK_DEP_CFLAGS)

LDADDS = $(GDK_DEP_LIBS)

noinst_LTLIBRARIES = libcdk-x11.la

libcdk_x11_la_SOURCES = 	\
	MwmUtil.h		\
	cdkapplaunchcontext-x11.c \
	cdkasync.c		\
	cdkasync.h		\
	cdkcursor-x11.c		\
	cdkdevice-core-x11.c	\
	cdkdevice-xi2.c		\
	cdkdevicemanager-core-x11.c \
	cdkdevicemanager-x11.c	\
	cdkdevicemanager-xi2.c	\
	cdkdevicemanagerprivate-core.h \
	cdkdisplaymanager-x11.c	\
	cdkdisplay-x11.c	\
	cdkdisplay-x11.h	\
	cdkdnd-x11.c		\
	cdkeventsource.c	\
	cdkeventsource.h	\
	cdkeventtranslator.c	\
	cdkeventtranslator.h	\
	cdkgeometry-x11.c  	\
	cdkglcontext-x11.c	\
	cdkglcontext-x11.h	\
	cdkkeys-x11.c		\
	cdkmain-x11.c		\
	cdkmonitor-x11.c	\
	cdkmonitor-x11.h	\
	cdkproperty-x11.c	\
	cdkscreen-x11.c		\
	cdkscreen-x11.h		\
	cdkselection-x11.c	\
	cdktestutils-x11.c	\
	cdkvisual-x11.c		\
	cdkwindow-x11.c		\
	cdkwindow-x11.h		\
	cdkxftdefaults.c	\
	cdkxid.c		\
	cdkx.h			\
	cdkprivate-x11.h	\
	xsettings-client.h	\
	xsettings-client.c

libcdkinclude_HEADERS = 	\
	cdkx.h

libcdkx11include_HEADERS = 	\
	cdkx-autocleanups.h	\
	cdkx11applaunchcontext.h \
	cdkx11cursor.h		\
	cdkx11device.h		\
	cdkx11device-core.h	\
	cdkx11device-xi2.h	\
	cdkx11devicemanager.h   \
	cdkx11devicemanager-core.h \
	cdkx11devicemanager-xi2.h \
	cdkx11display.h		\
	cdkx11displaymanager.h	\
	cdkx11dnd.h		\
	cdkx11glcontext.h	\
	cdkx11keys.h		\
	cdkx11monitor.h		\
	cdkx11property.h	\
	cdkx11screen.h		\
	cdkx11selection.h	\
	cdkx11utils.h		\
	cdkx11visual.h		\
	cdkx11window.h

# We need to include all these C files here since the conditionals
# don't seem to be correctly expanded for the dist files.
EXTRA_DIST += 			\
	cdksettings.c	\
	meson.build

-include $(top_srcdir)/git.mk
