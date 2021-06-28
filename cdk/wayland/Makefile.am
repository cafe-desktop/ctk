## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

libcdkincludedir = $(includedir)/ctk-3.0/cdk
libcdkwaylandincludedir = $(includedir)/ctk-3.0/cdk/wayland

AM_CPPFLAGS =					\
	-DG_LOG_DOMAIN=\"Cdk\"			\
	-DG_LOG_USE_STRUCTURED=1		\
	-DGDK_COMPILATION			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/cdk			\
	-I$(top_builddir)/cdk			\
	$(GDK_HIDDEN_VISIBILITY_CFLAGS)		\
	$(CTK_DEBUG_FLAGS)			\
	$(GDK_DEP_CFLAGS)

LDADDS = $(GDK_DEP_LIBS) $(SHM_LIBS)

noinst_LTLIBRARIES =				\
	libcdk-wayland.la

BUILT_SOURCES = 				\
	pointer-gestures-unstable-v1-client-protocol.h		\
	pointer-gestures-unstable-v1-protocol.c			\
	xdg-shell-client-protocol.h				\
	xdg-shell-protocol.c					\
	xdg-shell-unstable-v6-client-protocol.h			\
	xdg-shell-unstable-v6-protocol.c			\
	xdg-foreign-unstable-v1-client-protocol.h		\
	xdg-foreign-unstable-v1-protocol.c			\
	ctk-primary-selection-client-protocol.h			\
	ctk-primary-selection-protocol.c			\
	tablet-unstable-v2-client-protocol.h	\
	tablet-unstable-v2-protocol.c		\
	xdg-output-unstable-v1-protocol.c			\
	xdg-output-unstable-v1-client-protocol.h		\
	keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h \
	keyboard-shortcuts-inhibit-unstable-v1-protocol.c	\
	server-decoration-client-protocol.h		\
	server-decoration-protocol.c			\
	ctk-shell-client-protocol.h		\
	ctk-shell-protocol.c			\
	primary-selection-unstable-v1-client-protocol.h		\
	primary-selection-unstable-v1-protocol.c

nodist_libcdk_wayland_la_SOURCES =		\
	$(BUILT_SOURCES)

DISTCLEANFILES =				\
	$(BUILT_SOURCES)

libcdk_wayland_la_SOURCES =			\
	cdkapplaunchcontext-wayland.c		\
	cdkcursor-wayland.c			\
	cdkdevice-wayland.c			\
	cdkdisplay-wayland.c			\
	cdkdisplay-wayland.h			\
	cdkdnd-wayland.c			\
	cdkeventsource.c			\
	cdkkeys-wayland.c                       \
	cdkglcontext-wayland.c			\
	cdkglcontext-wayland.h			\
	cdkmonitor-wayland.c			\
	cdkmonitor-wayland.h			\
	cdkscreen-wayland.c			\
	cdkseat-wayland.h			\
	cdkselection-wayland.c			\
	cdkwindow-wayland.c			\
	cdkwayland.h				\
	cdkprivate-wayland.h			\
	wm-button-layout-translation.c		\
	wm-button-layout-translation.h

libcdkinclude_HEADERS =				\
	cdkwayland.h

libcdkwaylandinclude_HEADERS = 			\
	cdkwaylanddevice.h			\
	cdkwaylanddisplay.h			\
	cdkwaylandglcontext.h			\
	cdkwaylandmonitor.h			\
	cdkwaylandselection.h			\
	cdkwaylandwindow.h

.SECONDEXPANSION:

define protostability
$(if $(findstring unstable,$1),unstable,stable)
endef

define protoname
$(shell echo $1 | sed 's/\([a-z\-]\{1,\}\)-[a-z]\{1,\}-v[0-9]\{1,\}/\1/')
endef

%-protocol.c : $(WAYLAND_PROTOCOLS_DATADIR)/$$(call protostability,$$*)/$$(call protoname,$$*)/$$*.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) private-code < $< > $@
%-client-protocol.h : $(WAYLAND_PROTOCOLS_DATADIR)/$$(call protostability,$$*)/$$(call protoname,$$*)/$$*.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header < $< > $@

%-protocol.c : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) private-code < $< > $@

%-server-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) server-header < $< > $@
%-client-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header < $< > $@

EXTRA_DIST += 					\
	protocol/ctk-primary-selection.xml	\
	protocol/ctk-shell.xml			\
	protocol/server-decoration.xml	\
	meson.build

-include $(top_srcdir)/git.mk
