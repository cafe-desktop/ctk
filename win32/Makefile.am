include $(top_srcdir)/Makefile.decl

if HAVE_INTROSPECTION
GENERATED_ITEMS = \
	introspection.body.mak	\
	Cdk_3_0_gir_list		\
	CdkWin32_3_0_gir_list	\
	Ctk_3_0_gir_list

MSVC_INTROSPECTION_INTERMEDIATE_FILES = Cdk-3.0.gir.msvc.introspect CdkWin32-3.0.gir.msvc.introspect Ctk-3.0.gir.msvc.introspect

introspection.body.mak: $(MSVC_INTROSPECTION_INTERMEDIATE_FILES)
	-$(RM) introspection.body.mak
	for F in `ls *.msvc.introspect`; do \
		case $$F in \
			*)	cat $(top_builddir)/win32/$$F >>introspection.body.mak \
			;; \
		esac; \
	done
	$(RM) $(MSVC_INTROSPECTION_INTERMEDIATE_FILES)

DISTCLEANFILES = $(GENERATED_ITEMS)

else
GENERATED_ITEMS =
DISTCLEANFILES =
endif

SUBDIRS =	\
	vs9 	\
	vs10	\
	vs11	\
	vs12	\
	vs14	\
	vs15	\
	vs16

EXTRA_DIST +=	\
	config-msvc.mak		\
	config-msvc.mak.in		\
	create-lists.bat	\
	create-lists-msvc.mak		\
	detectenv-msvc.mak		\
	gen-cdkversionmacros-h.py	\
	generate-msvc.mak	\
	ctk-introspection-msvc.mak	\
	introspection-msvc.mak		\
	replace.py			\
	pc_base.py			\
	ctkpc.py			\
	README_EGL_MSVC.txt		\
	$(GENERATED_ITEMS)

-include $(top_srcdir)/git.mk
