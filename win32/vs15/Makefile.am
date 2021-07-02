include $(top_srcdir)/Makefile.decl

EXTRA_DIST +=	\
	README.txt	\
	ctk+.sln	\
	ctk3-prebuild.vcxproj	\
	cdk3-win32.vcxproj	\
	cdk3-win32.vcxproj.filters	\
	cdk-3.vcxproj	\
	cdk-3.vcxproj.filters	\
	ctk-3.vcxproj	\
	ctk-3.vcxproj.filters	\
	ctk-builder-tool.vcxproj		\
	ctk-builder-tool.vcxproj.filters	\
	ctk-encode-symbolic-svg.vcxproj	\
	ctk-encode-symbolic-svg.vcxproj.filters	\
	ctk-query-settings.vcxproj		\
	ctk-query-settings.vcxproj.filters	\
	ctk-update-icon-cache.vcxproj		\
	ctk-update-icon-cache.vcxproj.filters	\
	ctk3-demo.vcxproj	\
	ctk3-demo.vcxproj.filters	\
	ctk3-demo-application.vcxproj	\
	ctk3-demo-application.vcxproj.filters	\
	ctk3-icon-browser.vcxproj	\
	ctk3-icon-browser.vcxproj.filters	\
	cailutil-3.vcxproj	\
	cailutil-3.vcxproj.filters	\
	ctk3-install.vcxproj	\
	ctk3-install.vcxproj.filters	\
	ctk3-introspect.vcxproj		\
	broadwayd.vcxproj	\
	broadwayd.vcxproj.filters	\
	cdk3-broadway.vcxproj	\
	cdk3-broadway.vcxproj.filters	\
	ctk3-build-defines.props	\
	ctk3-copy-cdk-broadway.props	\
	ctk3-gen-srcs.props	\
	ctk3-ignore-broadway.props	\
	ctk3-install.props	\
	ctk3-version-paths.props

DISTCLEANFILES = $(EXTRA_DIST)

MSVC_VER = 15
MSVC_VER_LONG = 15
MSVC_TOOLSET = 141
MSVC_FORMAT_VER = 12

include $(top_srcdir)/win32/Makefile-newvs.am

-include $(top_srcdir)/git.mk
