include $(top_srcdir)/Makefile.decl

libcdkincludedir = $(includedir)/ctk-3.0/cdk
libcdkquartzincludedir = $(includedir)/ctk-3.0/cdk/quartz

AM_CPPFLAGS = 				\
	-DG_LOG_DOMAIN=\"Gdk\"		\
	-DG_LOG_USE_STRUCTURED=1	\
	-DGDK_COMPILATION		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/cdk		\
	-I$(top_builddir)/cdk		\
	$(GDK_HIDDEN_VISIBILITY_CFLAGS) \
	$(CTK_DEBUG_FLAGS) 		\
	$(GDK_DEP_CFLAGS)		\
	"-xobjective-c"

LDADDS = $(GDK_DEP_LIBS)

noinst_LTLIBRARIES = libcdk-quartz.la

libcdk_quartz_la_SOURCES =    	\
	GdkQuartzView.c		\
	GdkQuartzView.h		\
	GdkQuartzNSWindow.c	\
	GdkQuartzNSWindow.h	\
	cdkcursor-quartz.c	\
	cdkdevice-core-quartz.c	\
	cdkdevicemanager-core-quartz.c	\
	cdkdevicemanager-core-quartz.h	\
	cdkdisplay-quartz.c	\
	cdkdisplay-quartz.h	\
	cdkdisplaymanager-quartz.c	\
	cdkdnd-quartz.c		\
	cdkdnd-quartz.h		\
	cdkevents-quartz.c	\
	cdkeventloop-quartz.c	\
	cdkglcontext-quartz.c	\
	cdkglcontext-quartz.h	\
	cdkglobals-quartz.c	\
	cdkinternal-quartz.h	\
	cdkkeys-quartz.c	\
	cdkkeys-quartz.h	\
	cdkmonitor-quartz.c     \
	cdkmonitor-quartz.h     \
	cdkprivate-quartz.h	\
	cdkproperty-quartz.c	\
	cdkquartz.h		\
	cdkscreen-quartz.c	\
	cdkscreen-quartz.h	\
	cdkselection-quartz.c	\
	cdktestutils-quartz.c	\
	cdkutils-quartz.c	\
	cdkvisual-quartz.c	\
	cdkwindow-quartz.c	\
	cdkwindow-quartz.h	\
	xcursors.h

libcdkinclude_HEADERS = 	\
	cdkquartz.h

libcdkquartzinclude_HEADERS =		\
	cdkquartz-ctk-only.h            \
	cdkquartzcursor.h		\
	cdkquartzdevice-core.h		\
	cdkquartzdevicemanager-core.h	\
	cdkquartzdisplay.h		\
	cdkquartzdisplaymanager.h	\
	cdkquartzdnd.h			\
	cdkquartzglcontext.h		\
	cdkquartzkeys.h			\
	cdkquartzmonitor.h		\
	cdkquartzscreen.h		\
	cdkquartzutils.h		\
	cdkquartzvisual.h		\
	cdkquartzwindow.h

EXTRA_DIST += \
	meson.build

-include $(top_srcdir)/git.mk
