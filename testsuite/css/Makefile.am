include $(top_srcdir)/Makefile.decl

NULL =

SUBDIRS = parser nodes style

check_PROGRAMS = $(TEST_PROGS)
test_in_files =

AM_CPPFLAGS =				\
	-I$(top_srcdir)                 \
	-I$(top_builddir)/cdk           \
	-I$(top_srcdir)/cdk             \
	-DGDK_DISABLE_DEPRECATED        \
	-DCTK_DISABLE_DEPRECATED        \
	-DCTK_VERSION=\"$(CTK_VERSION)\"\
	$(CTK_DEBUG_FLAGS)              \
	$(CTK_DEP_CFLAGS)

LDADD = \
	$(top_builddir)/cdk/libcdk-3.la \
	$(top_builddir)/ctk/libctk-3.la \
	$(CTK_DEP_LIBS)

TEST_PROGS += api
test_in_files += api.test.in

EXTRA_DIST += \
	$(test_in_files) \
	meson.build

if BUILDOPT_INSTALL_TESTS
insttestdir=$(libexecdir)/installed-tests/$(PACKAGE)/css
insttest_PROGRAMS = $(TEST_PROGS)

substitutions = \
	-e s,@libexecdir\@,$(libexecdir),g \
	$(NULL)

test_files = $(test_in_files:.test.in=.test)

$(test_files): %.test: %.test.in
	$(AM_V_GEN) sed $(substitutions) $< > $@.tmp && mv $@.tmp $@

DISTCLEANFILES = $(test_files)

testmetadir = $(datadir)/installed-tests/$(PACKAGE)/css
testmeta_DATA = $(test_files)
endif

-include $(top_srcdir)/git.mk
