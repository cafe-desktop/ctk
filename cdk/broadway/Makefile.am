## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl
NULL =

libcdkincludedir = $(includedir)/ctk-3.0/cdk
libcdkbroadwayincludedir = $(includedir)/ctk-3.0/cdk/broadway

AM_CPPFLAGS =				\
	-DG_LOG_DOMAIN=\"Cdk\"		\
	-DG_LOG_USE_STRUCTURED=1	\
	-DCDK_COMPILATION		\
	-DCDK_DISABLE_DEPRECATION_WARNINGS \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/cdk		\
	-I$(top_builddir)/cdk		\
	$(CDK_HIDDEN_VISIBILITY_CFLAGS) \
	$(CTK_DEBUG_FLAGS)		\
	$(CDK_DEP_CFLAGS)

LDADDS = $(CDK_DEP_LIBS) -lrt

noinst_LTLIBRARIES = libcdk-broadway.la

bin_PROGRAMS = broadwayd

libcdkinclude_HEADERS = 	\
	cdkbroadway.h

libcdkbroadwayinclude_HEADERS = 	\
	cdkbroadwaydisplay.h		\
	cdkbroadwaywindow.h		\
	cdkbroadwaycursor.h		\
	cdkbroadwaymonitor.h		\
	cdkbroadwayvisual.h

EXTRA_DIST += toarray.pl

clienthtml.h: client.html
	$(AM_V_GEN) $(PERL) $(srcdir)/toarray.pl client_html $(srcdir)/client.html > $@

EXTRA_DIST += client.html

broadwayjs.h: broadway.js rawinflate.min.js
	$(AM_V_GEN) $(PERL) $(srcdir)/toarray.pl broadway_js $(srcdir)/broadway.js $(srcdir)/rawinflate.min.js  > $@

EXTRA_DIST += broadway.js rawinflate.min.js

# built headers that don't get installed
broadway_built_private_headers =	\
	clienthtml.h			\
	broadwayjs.h

broadway_built_sources =		\
	${broadway_built_private_headers}

libcdk_broadway_la_SOURCES =		\
	${broadway_built_private_headers}\
	cdkcursor-broadway.c	\
	cdkdevice-broadway.h	\
	cdkdevice-broadway.c	\
	cdkdevicemanager-broadway.h \
	cdkdevicemanager-broadway.c \
	cdkdisplay-broadway.c	\
	cdkdisplay-broadway.h	\
	cdkdnd-broadway.c	\
	cdkeventsource.c	\
	cdkeventsource.h	\
	cdkglobals-broadway.c	\
	cdkkeys-broadway.c	\
	cdkmonitor-broadway.c	\
	cdkmonitor-broadway.h	\
	cdkproperty-broadway.c	\
	cdkscreen-broadway.c	\
	cdkscreen-broadway.h	\
	cdkselection-broadway.c	\
	cdktestutils-broadway.c	\
	cdkvisual-broadway.c	\
	cdkwindow-broadway.c	\
	cdkwindow-broadway.h	\
	cdkprivate-broadway.h   \
	cdkbroadway-server.h	\
	cdkbroadway-server.c

if OS_UNIX
libcdk_broadway_la_LIBADD = \
	$(SHM_LIBS)         \
	$(NULL)
endif

broadwayd_SOURCES = \
	broadway-protocol.h		\
	broadwayd.c 			\
	broadway-server.h		\
	broadway-server.c		\
	broadway-buffer.c		\
	broadway-buffer.h		\
	broadway-output.h		\
	broadway-output.c

if OS_WIN32
broadwayd_LDADD = $(CDK_DEP_LIBS) -lws2_32
else
broadwayd_LDADD = $(CDK_DEP_LIBS) @SHM_LIBS@
endif

MAINTAINERCLEANFILES = $(broadway_built_sources)
EXTRA_DIST += $(broadway_built_sources)

BUILT_SOURCES = $(broadway_built_sources)

EXTRA_DIST += \
	meson.build \
	gen-c-array.py

# ------------------- MSVC Build Items ----------------
MSVCPROJS = cdk3-broadway broadwayd

cdk3_broadway_FILES = $(libcdk_broadway_la_SOURCES)
cdk3_broadway_EXCLUDES = dummy
cdk3_broadway_HEADERS_DIR = $(libcdkbroadwayincludedir)

cdk3_broadway_HEADERS_INST = $(libcdkbroadwayinclude_HEADERS)

cdk3_broadway_HEADERS_EXCLUDES = dummy

broadwayd_FILES = $(broadwayd_SOURCES)
broadwayd_EXCLUDES = dummy

include $(top_srcdir)/win32/Makefile.msvcproj

dist-hook: \
	$(top_builddir)/win32/vs9/cdk3-broadway.vcproj		\
	$(top_builddir)/win32/vs9/broadwayd.vcproj		\
	$(top_builddir)/win32/vs9/cdk3-broadway.headers

-include $(top_srcdir)/git.mk
