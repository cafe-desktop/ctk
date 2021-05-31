## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module.
DOC_MODULE=ctk3

# The top-level SGML file.
DOC_MAIN_SGML_FILE=gtk-docs.sgml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS=--deprecated-guards="CTK_ENABLE_BROKEN|CTK_DISABLE_DEPRECATED"

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=$(top_srcdir)/ctk

# Used for dependencies
HFILE_GLOB=$(top_srcdir)/ctk/*.h
CFILE_GLOB=$(top_srcdir)/ctk/*.c

# Header files to ignore when scanning
IGNORE_HFILES = \
	inspector			\
	a11y				\
	ctkaccelgroupprivate.h		\
	ctkaccelmapprivate.h		\
	ctkadjustmentprivate.h		\
	ctkallocatedbitmaskprivate.h	\
	ctkappchooserprivate.h		\
	ctkapplicationaccelsprivate.h	\
	ctkapplicationprivate.h		\
	ctkbindingsprivate.h		\
	ctkbitmaskprivate.h		\
	ctkboxprivate.h		        \
	ctkbuilderprivate.h		\
	ctkbuttonprivate.h		\
	ctkcairoblurprivate.h		\
	ctkcellareaboxcontextprivate.h	\
	ctkclipboardprivate.h		\
	ctkcolorchooserprivate.h	\
	ctkcoloreditorprivate.h		\
	ctkcolorplaneprivate.h		\
	ctkcolorscaleprivate.h		\
	ctkcolorswatchprivate.h		\
	ctkcomboboxprivate.h		\
	ctkcontainerprivate.h		\
	ctkcssanimatedstyleprivate.h	\
	ctkcssanimationprivate.h	\
	ctkcssarrayvalueprivate.h	\
	ctkcssbgsizevalueprivate.h	\
	ctkcssbordervalueprivate.h	\
	ctkcsscolorvalueprivate.h	\
	ctkcsscornervalueprivate.h	\
	ctkcsscustompropertyprivate.h	\
	ctkcsseasevalueprivate.h	\
	ctkcssenginevalueprivate.h	\
	ctkcssenumvalueprivate.h	\
	ctkcssiconthemevalueprivate.h	\
	ctkcssimagebuiltinprivate.h	\
	ctkcssimagecrossfadeprivate.h	\
	ctkcssimagegradientprivate.h	\
	ctkcssimageiconthemeprivate.h	\
	ctkcssimagelinearprivate.h	\
	ctkcssimageprivate.h		\
	ctkcssimagescaledprivate.h	\
	ctkcssimagesurfaceprivate.h	\
	ctkcssimageurlprivate.h		\
	ctkcssimagevalueprivate.h	\
	ctkcssimagewin32private.h	\
	ctkcssinheritvalueprivate.h	\
	ctkcssinitialvalueprivate.h	\
	ctkcsskeyframesprivate.h	\
	ctkcsslookupprivate.h		\
	ctkcssmatcherprivate.h		\
	ctkcssnodeprivate.h		\
	ctkcssnodedeclarationprivate.h	\
	ctkcssnumbervalueprivate.h	\
	ctkcsspathnodeprivate.h		\
	ctkcssparserprivate.h		\
	ctkcsspositionvalueprivate.h	\
	ctkcssproviderprivate.h		\
	ctkcssrepeatvalueprivate.h	\
	ctkcssrgbavalueprivate.h	\
	ctkcsssectionprivate.h		\
	ctkcssselectorprivate.h		\
	ctkcssshadowsvalueprivate.h	\
	ctkcssshadowvalueprivate.h	\
	ctkcssshorthandpropertyprivate.h\
	ctkcssstaticstyleprivate.h	\
	ctkcssstringvalueprivate.h	\
	ctkcssstylefuncsprivate.h	\
	ctkcssstyleprivate.h		\
	ctkcssstylepropertyprivate.h	\
	ctkcsstransformvalueprivate.h	\
	ctkcsstransientnodeprivate.h	\
	ctkcsstransitionprivate.h	\
	ctkcsstypedvalueprivate.h	\
	ctkcsstypesprivate.h		\
	ctkcssunsetvalueprivate.h	\
	ctkcssvalueprivate.h		\
	ctkcsswidgetnodeprivate.h	\
	ctkdialogprivate.h		\
	ctkentryprivate.h		\
	ctkeventcontrollerprivate.h	\
	ctkfilechooserprivate.h		\
	ctkfilechooserwidgetprivate.h	\
	ctkfontchooserprivate.h		\
	ctkgesturedragprivate.h		\
	ctkgesturelongpressprivate.h	\
	ctkgesturemultipressprivate.h	\
	ctkgesturepanprivate.h		\
	ctkgestureprivate.h		\
	ctkgesturerotateprivate.h	\
	ctkgesturesingleprivate.h	\
	ctkgestureswipeprivate.h	\
	ctkgesturezoomprivate.h		\
	ctkheaderbarprivate.h		\
	ctkhslaprivate.h		\
	ctkiconhelperprivate.h		\
	ctkiconviewprivate.h		\
	ctkimageprivate.h		\
	ctkimmoduleprivate.h		\
	ctklabelprivate.h		\
	ctklockbuttonprivate.h		\
	ctkmagnifierprivate.h		\
	ctkmenubuttonprivate.h		\
	ctkmenuitemprivate.h		\
	ctkmenuprivate.h		\
	ctkmenushellprivate.h		\
	ctkmodulesprivate.h		\
	ctkmountoperationprivate.h	\
	ctkorientableprivate.h		\
	ctkpixelcacheprivate.h		\
	ctkplacessidebarprivate.h	\
	ctkplacesviewprivate.h		\
	ctkplacesviewrowprivate.h	\
	ctkpopoverprivate.h		\
	ctkprinter-private.h		\
	ctkprintoperation-private.h	\
	ctkprivate.h		        \
	ctkrangeprivate.h		\
	ctkrecentchooserprivate.h	\
	ctkrenderbackgroundprivate.h	\
	ctkrenderborderprivate.h	\
	ctkrendericonprivate.h		\
	ctkrenderprivate.h		\
	ctkroundedboxprivate.h		\
	ctkscaleprivate.h		\
	ctksearchengine.h		\
	ctksearchenginemodel.h		\
	ctksearchenginequartz.h		\
	ctksearchenginesimple.h		\
	ctksearchenginetracker.h	\
	ctksearchentryprivate.h		\
	ctkselectionprivate.h		\
	ctksettingsprivate.h		\
	ctksidebarrowprivate.h		\
	ctksizegroup-private.h		\
	ctksizerequestcacheprivate.h	\
	ctksocketprivate.h		\
	ctkstyleanimationprivate.h	\
	ctkstylecascadeprivate.h	\
	ctkstylecontextprivate.h	\
	ctkstylepropertyprivate.h	\
	ctkstyleproviderprivate.h	\
	ctktextattributesprivate.h	\
	ctktextchildprivate.h		\
	ctktexthandleprivate.h		\
	ctktextiterprivate.h		\
	ctktextmarkprivate.h		\
	ctktexttagprivate.h		\
	ctktogglebuttonprivate.h	\
	ctktoolbarprivate.h		\
	ctktoolpaletteprivate.h		\
	ctktooltipprivate.h		\
	ctktooltipwindowprivate.h	\
	ctktreeprivate.h		\
	ctkwidgetprivate.h		\
	ctkwin32themeprivate.h		\
	ctkwindowprivate.h		\
	fnmatch.h			\
	ctkactionmuxer.h		\
	ctkactionobserver.h		\
	ctkactionobservable.h		\
	ctk9slice.h			\
	ctkanimationdescription.h	\
	ctkbitmaskprivateimpl.h		\
	ctkbookmarksmanager.h		\
	ctkdbusgenerated.c		\
	ctkdbusgenerated.h		\
	ctkdebug.h			\
	ctkactionhelper.h		\
	ctkdndcursors.h			\
	ctkfilechooserdefault.h		\
	ctkfilechooserembed.h		\
	ctkfilechooserentry.h		\
	ctkfilechoosersettings.h	\
	ctkfilechooserutils.h		\
	ctkfilesystem.h			\
	ctkfilesystemmodel.h		\
	ctkfilesystemunix.h		\
	ctkfilesystemwin32.h		\
	ctkfontchooserutils.h		\
	ctkiconcache.h			\
        ctkiconcachevalidator.h         \
	ctkiconthemeparser.h		\
	ctkintl.h			\
	ctkkeyhash.h			\
	ctkkineticscrolling.h		\
	ctkmarshal.h			\
	ctkmnemonichash.h		\
	ctkmenutracker.h		\
	ctkmenutrackeritem.h		\
	ctkmenusectionbox.h		\
	ctkmodelmenu.h			\
	ctkmodelmenuitem.h		\
	ctkmodifierstyle.h		\
	ctkpathbar.h			\
	ctkprintbackend.h		\
	ctkprinteroption.h		\
	ctkprinteroptionset.h		\
	ctkprinteroptionwidget.h	\
	ctkprint-win32.h		\
	ctkprintutils.h			\
	ctkprivate.h			\
	ctkprivatetypebuiltins.h	\
	ctkquery.h			\
	ctkrbtree.h			\
	ctkrecentchooserdefault.h	\
	ctkrecentchooserutils.h		\
	ctksearchengine.h		\
	ctksearchenginetracker.h	\
	ctksearchenginesimple.h		\
	ctksearchenginequartz.h		\
	ctksequence.h			\
	ctksocketprivate.h		\
	ctktextbtree.h			\
	ctktextbufferserialize.h	\
	ctktextdisplay.h		\
	ctktextlayout.h			\
	ctktextsegment.h		\
	ctktexttypes.h			\
	ctktextutil.h			\
	ctktimeline.h			\
	ctkthemes.h			\
	ctktrashmonitor.h		\
	ctktrayicon.h			\
	ctktreedatalist.h		\
	ctktreemenu.h			\
	ctktypebuiltins.h		\
	ctkxembed.h			\
	ctkwin32embed.h			\
	ctkwin32embedwidget.h		\
	ctkwindow-decorate.h		\
	xdgmime				\
	xembed.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	-I$(top_builddir) 	\
	-I$(top_builddir)/gdk	\
	-DCTK_ENABLE_BROKEN	\
	-Wno-deprecated-declarations \
	$(CTK_DEBUG_FLAGS)	\
	$(CTK_DEP_CFLAGS)

CPPFLAGS += \
	-UCTK_DISABLE_SINGLE_INCLUDES

GTKDOC_LIBS = \
	$(top_builddir)/gdk/libgdk-3.la \
	$(top_builddir)/ctk/libctk-3.la	\
	$(CTK_DEP_LIBS)


# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--output-format=xml --name-space=ctk --default-includes=ctk/ctk.h

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =					\
	broadway.xml				\
	broadwayd.xml				\
	building.sgml				\
	compiling.sgml				\
	css-overview.xml			\
	css-properties.xml			\
	drawing-model.xml			\
	$(builddir)/getting_started.xml			\
	glossary.xml				\
	ctk3-demo-application.xml		\
	ctk3-demo.xml				\
	ctk3-icon-browser.xml			\
	ctk3-widget-factory.xml			\
	ctk-builder-tool.xml			\
	ctk-encode-symbolic-svg.xml		\
	ctk-launch.xml				\
	ctk-query-immodules-3.0.xml		\
	ctk-query-settings.xml			\
	ctk-update-icon-cache.xml		\
	input-handling.xml			\
	migrating-2to3.xml			\
	migrating-3xtoy.xml			\
	migrating-checklist.sgml		\
	migrating-GtkGrid.xml			\
	migrating-GtkStyleContext.xml		\
	migrating-smclient-GtkApplication.xml	\
	migrating-unique-GtkApplication.xml	\
	osx.sgml				\
	overview.xml 				\
	question_index.sgml			\
	resources.sgml				\
	running.sgml				\
	text_widget.sgml			\
	tree_widget.sgml			\
	version.xml				\
	visual_index.xml			\
	wayland.xml				\
	windows.sgml				\
	x11.sgml

expand_content_files = 				\
	compiling.sgml				\
	drawing-model.xml			\
	$(builddir)/getting_started.xml			\
	glossary.xml				\
	input-handling.xml			\
	migrating-2to3.xml			\
	migrating-3xtoy.xml			\
	migrating-checklist.sgml		\
	migrating-GtkGrid.xml			\
	migrating-GtkStyleContext.xml		\
	migrating-smclient-GtkApplication.xml	\
	migrating-unique-GtkApplication.xml	\
	question_index.sgml			\
	text_widget.sgml			\
	tree_widget.sgml

# Images to copy into HTML directory
HTML_IMAGES = \
	$(srcdir)/images/aboutdialog.png				\
	$(srcdir)/images/accel-label.png				\
	$(srcdir)/images/action-bar.png					\
	$(srcdir)/images/appchooserbutton.png				\
	$(srcdir)/images/appchooserdialog.png				\
	$(srcdir)/images/assistant.png					\
	$(srcdir)/images/box-packing.png				\
	$(srcdir)/images/box-expand.png					\
	$(srcdir)/images/button.png					\
	$(srcdir)/images/check-button.png				\
	$(srcdir)/images/color-button.png				\
	$(srcdir)/images/colorchooser.png				\
	$(srcdir)/images/combo-box.png					\
	$(srcdir)/images/combo-box-entry.png				\
	$(srcdir)/images/combo-box-text.png				\
	$(srcdir)/images/entry.png					\
	$(srcdir)/images/figure-hierarchical-drawing.png		\
	$(srcdir)/images/figure-windowed-label.png			\
	$(srcdir)/images/file-button.png				\
	$(srcdir)/images/filechooser.png				\
	$(srcdir)/images/font-button.png				\
	$(srcdir)/images/fontchooser.png				\
	$(srcdir)/images/frame.png					\
	$(srcdir)/images/glarea.png					\
	$(srcdir)/images/headerbar.png					\
	$(srcdir)/images/icon-view.png					\
	$(srcdir)/images/image.png					\
	$(srcdir)/images/info-bar.png					\
	$(srcdir)/images/label.png					\
	$(srcdir)/images/levelbar.png					\
	$(srcdir)/images/link-button.png				\
	$(srcdir)/images/list-and-tree.png				\
	$(srcdir)/images/lock-button.png				\
	$(srcdir)/images/lockbutton.png	        			\
	$(srcdir)/images/lockbutton-locked.png				\
	$(srcdir)/images/lockbutton-unlocked.png			\
	$(srcdir)/images/lockbutton-sorry.png				\
	$(srcdir)/images/menubar.png					\
	$(srcdir)/images/menu-button.png				\
	$(srcdir)/images/messagedialog.png				\
	$(srcdir)/images/multiline-text.png				\
	$(srcdir)/images/notebook.png					\
	$(srcdir)/images/panes.png					\
	$(srcdir)/images/pagesetupdialog.png				\
	$(srcdir)/images/placessidebar.png				\
	$(srcdir)/images/popup-anchors.png				\
	$(srcdir)/images/popup-at.svg					\
	$(srcdir)/images/popup-flip.png					\
	$(srcdir)/images/popup-slide.png				\
	$(srcdir)/images/printdialog.png				\
	$(srcdir)/images/progressbar.png				\
	$(srcdir)/images/radio-group.png				\
	$(srcdir)/images/recentchooserdialog.png			\
	$(srcdir)/images/scales.png					\
	$(srcdir)/images/scrollbar.png					\
	$(srcdir)/images/scrolledwindow.png				\
	$(srcdir)/images/search-bar.png					\
	$(srcdir)/images/search-entry.png				\
	$(srcdir)/images/separator.png					\
	$(srcdir)/images/sidebar.png					\
	$(srcdir)/images/spinbutton.png					\
	$(srcdir)/images/spinner.png					\
	$(srcdir)/images/stack.png					\
	$(srcdir)/images/stackswitcher.png				\
	$(srcdir)/images/statusbar.png					\
	$(srcdir)/images/toggle-button.png				\
	$(srcdir)/images/toolbar.png					\
	$(srcdir)/images/toolpalette.png				\
	$(srcdir)/images/tree-view-coordinates.png			\
	$(srcdir)/images/volumebutton.png				\
	$(srcdir)/images/window.png					\
	$(srcdir)/images/layout-btlr.png				\
	$(srcdir)/images/layout-btrl.png				\
	$(srcdir)/images/layout-lrbt.png				\
	$(srcdir)/images/layout-lrtb.png				\
	$(srcdir)/images/layout-rlbt.png				\
	$(srcdir)/images/layout-rltb.png				\
	$(srcdir)/images/layout-tblr.png				\
	$(srcdir)/images/layout-tbrl.png				\
	$(srcdir)/images/widget-hvalign.png				\
	$(srcdir)/images/window-default.png				\
	$(srcdir)/images/hello-world.png				\
	$(srcdir)/images/grid-packing.png				\
	$(srcdir)/images/drawing.png					\
	$(srcdir)/images/switch.png					\
	$(srcdir)/images/linear.png					\
	$(srcdir)/images/ease.png					\
	$(srcdir)/images/ease-in-out.png				\
	$(srcdir)/images/ease-in.png					\
	$(srcdir)/images/ease-out.png					\
	$(srcdir)/images/gradient1.png					\
	$(srcdir)/images/gradient2.png					\
	$(srcdir)/images/gradient3.png					\
	$(srcdir)/images/gradient4.png					\
	$(srcdir)/images/border1.png					\
	$(srcdir)/images/border2.png					\
	$(srcdir)/images/border3.png					\
	$(srcdir)/images/slices.png					\
	$(srcdir)/images/checks.png					\
	$(srcdir)/images/options.png					\
	$(srcdir)/images/arrows.png					\
	$(srcdir)/images/expanders.png					\
	$(srcdir)/images/background.png					\
	$(srcdir)/images/frames.png					\
	$(srcdir)/images/frame-gap.png					\
	$(srcdir)/images/sliders.png					\
	$(srcdir)/images/focus.png					\
	$(srcdir)/images/handles.png					\
	$(srcdir)/images/extensions.png					\
	$(srcdir)/images/numerableicon.png				\
	$(srcdir)/images/numerableicon2.png				\
	$(srcdir)/images/bloatpad-osx.png				\
	$(srcdir)/images/bloatpad-gnome.png				\
	$(srcdir)/images/bloatpad-xfce.png				\
	$(srcdir)/images/down-center.png				\
	$(srcdir)/images/down-end.png					\
	$(srcdir)/images/down-start.png					\
	$(srcdir)/images/left-center.png				\
	$(srcdir)/images/left-end.png					\
	$(srcdir)/images/left-start.png					\
	$(srcdir)/images/right-center.png				\
	$(srcdir)/images/right-end.png					\
	$(srcdir)/images/right-start.png				\
	$(srcdir)/images/up-center.png					\
	$(srcdir)/images/up-end.png					\
	$(srcdir)/images/up-start.png					\
	$(srcdir)/images/list-box.png					\
	$(srcdir)/images/getting-started-app1.png			\
	$(srcdir)/images/getting-started-app2.png			\
	$(srcdir)/images/getting-started-app3.png			\
	$(srcdir)/images/getting-started-app4.png			\
	$(srcdir)/images/getting-started-app6.png			\
	$(srcdir)/images/getting-started-app7.png			\
	$(srcdir)/images/getting-started-app8.png			\
	$(srcdir)/images/getting-started-app9.png			\
	$(srcdir)/images/getting-started-app10.png			\
	$(srcdir)/images/exampleapp.png					\
	$(srcdir)/images/flow-box.png					\
	$(srcdir)/images/inspector.png					\
	$(srcdir)/images/gedit-shortcuts.png				\
	$(srcdir)/images/clocks-shortcuts.png				\
	$(srcdir)/images/builder-shortcuts.png

if ENABLE_DOC_CROSS_REFERENCES
# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=--extra-dir=../gdk/html \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gobject \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/glib \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gio \
	--extra-dir=$(ATK_PREFIX)/share/gtk-doc/html/atk \
	--extra-dir=$(PANGO_PREFIX)/share/gtk-doc/html/pango \
	--extra-dir=$(CAIRO_PREFIX)/share/gtk-doc/html/cairo
endif

include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += version.xml.in ctk3.types.in getting_started.xml.in

########################################################################

man_MANS = 				\
	ctk-query-immodules-3.0.1	\
	ctk-update-icon-cache.1		\
	ctk-encode-symbolic-svg.1	\
	ctk-launch.1			\
	ctk3-demo.1			\
	ctk3-demo-application.1		\
	ctk3-widget-factory.1		\
	ctk3-icon-browser.1		\
	broadwayd.1			\
	ctk-builder-tool.1 		\
	ctk-query-settings.1

if ENABLE_MAN

XSLTPROC_FLAGS = \
        --nonet \
        --stringparam man.output.quietly 1 \
        --stringparam funcsynopsis.style ansi \
        --stringparam man.th.extra1.suppress 1 \
        --stringparam man.authors.section.enabled 0 \
        --stringparam man.copyright.section.enabled 0

.xml.1:
	$(AM_V_GEN) $(XSLTPROC) $(XSLTPROC_FLAGS) http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

dist-local-check-mans-enabled:
	if grep "Man generation disabled" $(man_MANS) >/dev/null; then $(RM) $(man_MANS); fi

else

$(man_MANS):
	echo Man generation disabled.  Creating dummy $@.  Configure with --enable-man to enable it.
	echo Man generation disabled.  Remove this file, configure with --enable-man, and rebuild > $@

dist-local-check-mans-enabled:
	echo "*** --enable-man must be used in order to make dist"
	false

endif

MAINTAINERCLEANFILES = $(man_MANS) $(BUILT_SOURCES)

EXTRA_DIST += \
	$(man_MANS) \
	meson.build

if ENABLE_CTK_DOC
TESTS_ENVIRONMENT = cd $(srcdir) && \
  DOC_MODULE=$(DOC_MODULE) DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
  SRCDIR=$(abs_srcdir) BUILDDIR=$(abs_builddir)
#TESTS = $(GTKDOC_CHECK)
endif

dist-hook-local: dist-local-check-mans-enabled all-local

gtk-docs-clean: clean
	cd $(srcdir) && rm -rf xml html

-include $(top_srcdir)/git.mk
