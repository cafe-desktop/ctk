include $(top_srcdir)/Makefile.decl

EXTRA_DIST += \
	cdk.rc \
	cdk.rc.in \
	ctk.ico

if USE_WIN32
noinst_DATA = cdk-win32-res.o
endif

cdk-win32-res.o : cdk.rc ctk.ico
	$(WINDRES) -I $(srcdir) cdk.rc $@

-include $(top_srcdir)/git.mk
