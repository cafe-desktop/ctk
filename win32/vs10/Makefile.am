include $(top_srcdir)/Makefile.decl

GENERATED_ITEMS = \
	gdk3-win32.vcxproj			\
	gdk3-win32.vcxproj.filters		\
	gdk3-broadway.vcxproj			\
	gdk3-broadway.vcxproj.filters		\
	broadwayd.vcxproj			\
	broadwayd.vcxproj.filters		\
	gdk-3.vcxproj				\
	gdk-3.vcxproj.filters			\
	ctk-3.vcxproj				\
	ctk-3.vcxproj.filters			\
	ctk3-demo.vcxproj			\
	ctk3-demo.vcxproj.filters		\
	ctk3-demo-application.vcxproj		\
	ctk3-demo-application.vcxproj.filters	\
	ctk3-icon-browser.vcxproj		\
	ctk3-icon-browser.vcxproj.filters	\
	gailutil-3.vcxproj			\
	gailutil-3.vcxproj.filters		\
	ctk3-install.props			\
	ctk3-version-paths.props

MSVC10_HEADERS_LISTS =	\
	gdk3-win32.vs10.headers		\
	gdk3-broadway.vs10.headers	\
	gdk-3.vs10.headers		\
	ctk-3.vs10.headers		\
	gailutil-3.vs10.headers

EXTRA_DIST += \
	README.txt				\
	ctk+.sln				\
	ctk3-prebuild.vcxproj			\
	gdk3-win32.vcxprojin			\
	gdk3-win32.vcxproj.filtersin		\
	gdk3-broadway.vcxprojin			\
	gdk3-broadway.vcxproj.filtersin		\
	broadwayd.vcxprojin			\
	broadwayd.vcxproj.filtersin		\
	gdk-3.vcxprojin				\
	gdk-3.vcxproj.filtersin			\
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
	gailutil-3.vcxprojin			\
	gailutil-3.vcxproj.filtersin		\
	ctk3-install.vcxproj			\
	ctk3-install.vcxproj.filters		\
	ctk3-introspect.vcxproj			\
	ctk3-build-defines.props		\
	ctk3-copy-gdk-broadway.props		\
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
