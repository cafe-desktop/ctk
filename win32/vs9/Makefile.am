include $(top_srcdir)/Makefile.decl

MSVC_HEADERS_LISTS =	\
	cdk3-win32.headers	\
	cdk3-broadway.headers	\
	cdk-3.headers		\
	ctk-3.headers		\
	gailutil-3.headers

GENERATED_ITEMS = \
	cdk3-win32.vcproj		\
	cdk3-broadway.vcproj		\
	broadwayd.vcproj		\
	cdk-3.vcproj			\
	ctk-3.vcproj			\
	ctk3-demo.vcproj		\
	ctk3-demo-application.vcproj	\
	ctk3-icon-browser.vcproj	\
	gailutil-3.vcproj		\
	ctk3-install.vsprops		\
	ctk3-version-paths.vsprops

EXTRA_DIST += \
	README.txt			\
	ctk+.sln			\
	ctk3-prebuild.vcproj		\
	cdk3-win32.vcprojin		\
	cdk-3.vcprojin			\
	ctk-3.vcprojin			\
	ctk-builder-tool.vcproj	\
	ctk-encode-symbolic-svg.vcproj	\
	ctk-query-settings.vcproj	\
	ctk-update-icon-cache.vcproj	\
	ctk3-demo.vcprojin		\
	ctk3-demo-application.vcprojin	\
	ctk3-icon-browser.vcprojin	\
	gailutil-3.vcprojin		\
	ctk3-install.vcproj		\
	ctk3-introspect.vcproj		\
	broadwayd.vcprojin		\
	cdk3-broadway.vcprojin		\
	ctk3-build-defines.vsprops	\
	ctk3-copy-cdk-broadway.vsprops	\
	ctk3-gen-srcs.vsprops		\
	ctk3-ignore-broadway.vsprops	\
	ctk3-install.vspropsin		\
	ctk3-version-paths.vsprops.in	\
	$(GENERATED_ITEMS)

ctk3-install.vsprops: $(top_srcdir)/win32/vs9/ctk3-install.vspropsin $(MSVC_HEADERS_LISTS)
	$(CPP) -P - <$(top_srcdir)/win32/vs9/ctk3-install.vspropsin >$@
	rm $(MSVC_HEADERS_LISTS)

DISTCLEANFILES = $(GENERATED_ITEMS)

-include $(top_srcdir)/git.mk
