include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_builddir)/cdk		\
	-I$(top_srcdir)/cdk		\
	-I$(top_srcdir)/cdk/x11		\
	-I$(top_srcdir)/tests		\
	$(CTK_DEBUG_FLAGS)		\
	$(CTK_DEP_CFLAGS)

DEPS = \
	$(top_builddir)/ctk/libctk-3.la

LDADDS = \
	$(top_builddir)/ctk/libctk-3.la	\
	$(top_builddir)/cdk/libcdk-3.la	\
	$(top_builddir)/tests/ctkgears.o \
	$(CTK_DEP_LIBS)			\
	$(CDK_DEP_LIBS)			\
	-lm

EXTRA_DIST += \
	meson.build

if USE_X11
noinst_PROGRAMS = 		\
	doc-shooter
endif

doc_shooter_DEPENDENCIES = $(DEPS)
doc_shooter_LDADD = $(LDADDS)
doc_shooter_SOURCES =	\
	shadow.c	\
	shadow.h	\
	shooter.c	\
	widgets.c	\
	widgets.h

clean-local:
	for file in *.png; do \
	  case "$$file" in \
	    folder.png|gnome.png) ;; \
	    *) rm -f $$file ;; \
	  esac ; \
	done

-include $(top_srcdir)/git.mk
