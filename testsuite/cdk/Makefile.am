include $(top_srcdir)/Makefile.decl

NULL=

noinst_PROGRAMS = $(TEST_PROGS)

AM_CPPFLAGS = 				\
	$(CDK_DEP_CFLAGS) 		\
	-I$(top_srcdir) 		\
	-I$(top_builddir)/cdk 		\
	$(NULL)

LDADD = 				\
	$(CDK_DEP_LIBS) 		\
	$(top_builddir)/cdk/libcdk-3.la \
	$(NULL)

#TEST_PROGS              += check-cdk-cairo

TEST_PROGS += 				\
	cairo				\
	display				\
	encoding			\
	keysyms				\
	rectangle			\
	rgba				\
	seat				\
	visual				\
	$(NULL)

CLEANFILES = 			\
	cairosurface.png	\
	cdksurface.png		\
	$(NULL)

EXTRA_DIST += \
	check-cdk-cairo.c	\
	meson.build

if BUILDOPT_INSTALL_TESTS
insttestdir=$(libexecdir)/installed-tests/$(PACKAGE)
insttest_PROGRAMS = $(TEST_PROGS)

%.test: %$(EXEEXT) Makefile
	$(AM_V_GEN) (echo '[Test]' > $@.tmp; \
	echo 'Type=session' >> $@.tmp; \
	echo 'Output=TAP' >> $@.tmp; \
	echo 'Exec=$(insttestdir)/$< --tap' >> $@.tmp; \
	mv $@.tmp $@)

test_files = $(TEST_PROGRS:=.test)

DISTCLEANFILES = $(test_files)

testmetadir = $(datadir)/installed-tests/$(PACKAGE)
testmeta_DATA = $(test_files)
endif

-include $(top_srcdir)/git.mk
