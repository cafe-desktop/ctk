include $(top_srcdir)/Makefile.decl

check_PROGRAMS = $(TEST_PROGS)

AM_CPPFLAGS =                           \
        -I$(top_srcdir)                 \
        -I$(top_builddir)/cdk           \
        -I$(top_srcdir)/cdk             \
        -DCDK_DISABLE_DEPRECATED        \
        -DCTK_DISABLE_DEPRECATED        \
        -DCTK_VERSION=\"$(CTK_VERSION)\"\
        $(CTK_DEBUG_FLAGS)              \
        $(CTK_DEP_CFLAGS)

LDADD = \
        $(top_builddir)/cdk/libcdk-3.la \
        $(top_builddir)/ctk/libctk-3.la \
        $(CTK_DEP_LIBS)

TESTS_ENVIRONMENT =                     \
        GIO_USE_VOLUME_MONITOR=unix     \
        GSETTINGS_BACKEND=memory        \
        G_ENABLE_DIAGNOSTIC=0

test_programs = state-record

testdata = \
	focus1.ui focus1.in focus1.out \
	focus2.ui focus2.in focus2.out

EXTRA_DIST += $(testdata) \
	meson.build \
	a11ystate.test.in

if BUILDOPT_INSTALL_TESTS
insttestdir = $(libexecdir)/installed-tests/$(PACKAGE)
insttest_PROGRAMS = $(test_programs)

statetestdir = $(insttestdir)/state
statetest_DATA = $(testdata)

a11ystate.test: Makefile
	$(AM_V_GEN) (echo '[Test]' > $@.tmp; \
	echo 'Type=session-exclusive' >> $@.tmp; \
	echo 'Output=TAP' >> $@.tmp; \
	echo 'Exec=env G_ENABLE_DIAGNOSTIC=0 $(insttestdir)/state-record --tap --directory $(statetestdir)' >> $@.tmp; \
	mv $@.tmp $@)

testfiles = a11ystate.test

DISTCLEANFILES = \
	$(testfiles)

testmetadir = $(datadir)/installed-tests/$(PACKAGE)
testmeta_DATA = $(testfiles)
endif

-include $(top_srcdir)/git.mk
