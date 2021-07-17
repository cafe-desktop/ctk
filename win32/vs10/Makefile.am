include $(top_srcdir)/Makefile.decl

GENERATED_ITEMS = \
	cdk3-win32.vcxproj			\
	cdk3-win32.vcxproj.filters		\
	cdk3-broadway.vcxproj			\
	cdk3-broadway.vcxproj.filters		\
	cdkbroadwayd.vcxproj			\
	cdkbroadwayd.vcxproj.filters		\
	cdk-3.vcxproj				\
	cdk-3.vcxproj.filters			\
	ctk-3.vcxproj				\
	ctk-3.vcxproj.filters			\
	ctk3-demo.vcxproj			\
	ctk3-demo.vcxproj.filters		\
	ctk3-demo-application.vcxproj		\
	ctk3-demo-application.vcxproj.filters	\
	ctk3-icon-browser.vcxproj		\
	ctk3-icon-browser.vcxproj.filters	\
	cailutil-3.vcxproj			\
	cailutil-3.vcxproj.filters		\
	ctk3-install.props			\
	ctk3-version-paths.props

MSVC10_HEADERS_LISTS =	\
	cdk3-win32.vs10.headers		\
	cdk3-broadway.vs10.headers	\
	cdk-3.vs10.headers		\
	ctk-3.vs10.headers		\
	cailutil-3.vs10.headers

EXTRA_DIST += \
	README.txt				\
	ctk+.sln				\
	ctk3-prebuild.vcxproj			\
	cdk3-win32.vcxprojin			\
	cdk3-win32.vcxproj.filtersin		\
	cdk3-broadway.vcxprojin			\
	cdk3-broadway.vcxproj.filtersin		\
	cdkbroadwayd.vcxprojin			\
	cdkbroadwayd.vcxproj.filtersin		\
	cdk-3.vcxprojin				\
	cdk-3.vcxproj.filtersin			\
	ctk-3.vcxprojin				\
	ctk-3.vcxproj.filtersin			\
	ctk-builder-tool.vcxproj		\
	ctk-builder-tool.vcxproj.filters	\
	ctk-encode-symbolic-svg.vcxproj		\
	ctk-encode-symbolic-svg.vcxproj.filters	\
	ctk-query-settings.vcxproj		\
	ctk-query-settings.vcxproj.filters	\
	ctk-update-icon-cache.vcxproj		\
	ctk-update-icon-cache.vcxproj.filters	\
	ctk3-demo.vcxprojin			\
	ctk3-demo.vcxproj.filtersin		\
	ctk3-demo-application.vcxprojin		\
	ctk3-demo-application.vcxproj.filtersin	\
	ctk3-icon-browser.vcxprojin		\
	ctk3-icon-browser.vcxproj.filtersin	\
	cailutil-3.vcxprojin			\
	cailutil-3.vcxproj.filtersin		\
	ctk3-install.vcxproj			\
	ctk3-install.vcxproj.filters		\
	ctk3-introspect.vcxproj			\
	ctk3-build-defines.props		\
	ctk3-copy-cdk-broadway.props		\
	ctk3-gen-srcs.props			\
	ctk3-ignore-broadway.props		\
	ctk3-install.propsin			\
	ctk3-version-paths.props.in		\
	$(GENERATED_ITEMS)

DISTCLEANFILES = \
	$(GENERATED_ITEMS)

ctk3-install.props: $(top_srcdir)/win32/vs10/ctk3-install.propsin $(MSVC10_HEADERS_LISTS)
	-$(RM) $(top_builddir)/win32/vs11/ctk3-install.props
	-$(RM) $(top_builddir)/win32/vs12/ctk3-install.props
	-$(RM) $(top_builddir)/win32/vs14/ctk3-install.props
	-$(RM) $(top_builddir)/win32/vs15/ctk3-install.props
	-$(RM) $(top_builddir)/win32/vs16/ctk3-install.props
	$(CPP) -P - <$(top_srcdir)/win32/vs10/ctk3-install.propsin >$@
	rm $(MSVC10_HEADERS_LISTS)

-include $(top_srcdir)/git.mk
