AUTOMAKE_OPTIONS = subdir-objects

include $(top_srcdir)/Makefile.decl

CTK_BASE_CFLAGS_DEFINES =				\
	-DG_LOG_USE_STRUCTURED=1			\
	-DCTK_VERSION=\"$(CTK_VERSION)\"		\
	-DCTK_BINARY_VERSION=\"$(CTK_BINARY_VERSION)\"	\
	-DCTK_COMPILATION				\
	-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED

CTK_PLAT_CFLAGS_DEFINES =				\
	-DCTK_LIBDIR=\"$(libdir)\"			\
	-DCTK_LOCALEDIR=\"$(localedir)\"		\
	-DCTK_DATADIR=\"$(datadir)\"			\
	-DCTK_DATA_PREFIX=\"$(prefix)\"			\
	-DCTK_SYSCONFDIR=\"$(sysconfdir)\"		\
	-DCTK_HOST=\"$(host)\"				\
	-DCTK_PRINT_BACKENDS=\"$(CTK_PRINT_BACKENDS)\"

AM_CPPFLAGS =						\
	-DG_LOG_DOMAIN=\"Ctk\"				\
	$(CTK_BASE_CFLAGS_DEFINES)			\
	$(CTK_PLAT_CFLAGS_DEFINES)			\
	-DX11_DATA_PREFIX=\"$(X11_PREFIX)\"		\
	-DISO_CODES_PREFIX=\"$(ISO_CODES_PREFIX)\"	\
	-I$(top_builddir)				\
	-I$(top_builddir)/ctk				\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/cdk				\
	$(GMODULE_CFLAGS)				\
	$(CTK_DEBUG_FLAGS)				\
	$(CTK_DEP_CFLAGS)				\
	$(INCLUDED_IMMODULE_DEFINE)

if OS_WIN32
ctk_def = ctk.def
ctk_win32_symbols = -export-symbols $(srcdir)/ctk.def

ctk_win32_res = ctk-win32-res.o
ctk_win32_res_ldflag = -Wl,ctk-win32-res.o

ctk-win32-res.o : ctk-win32.rc libctk3.manifest
	$(WINDRES) ctk-win32.rc $@

ctk-win32.rc: ctk-win32.rc.body
	echo "#include <winuser.h>" >>$@
	cat $< >>$@
	echo "ISOLATIONAWARE_MANIFEST_RESOURCE_ID RT_MANIFEST libctk3.manifest" >>$@

ctk.def: libctk-3.la
	echo "LIBRARY libctk-$(CTK_MAJOR_VERSION)-@LT_CURRENT_MINUS_AGE@" >$@
	echo "EXPORTS" >>$@
	objdump -p .libs/libctk-$(CTK_MAJOR_VERSION)-@LT_CURRENT_MINUS_AGE@.dll | sed -n -e 's/\t\[\s\+\([0-9]\+\)\]\s\([A-Za-z_][A-Za-z0-9_]\)/\2/p' >>$@

install-def-file: ctk.def
	$(INSTALL) ctk.def $(DESTDIR)$(libdir)/ctk-win32-3.0.def
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/ctk-win32-3.0.def
else
install-def-file:
uninstall-def-file:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ctk-win32-$(CTK_API_VERSION).lib

ctk-win32-$(CTK_API_VERSION).lib: libctk-3.la ctk.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libctk-win32-$(CTK_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ctk.def -out:$@

install-ms-lib:
	$(INSTALL) ctk-win32-$(CTK_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ctk-win32-$(CTK_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

libadd = 				\
	$(top_builddir)/cdk/libcdk-3.la	\
	$(GMODULE_LIBS) 		\
	$(CTK_DEP_LIBS)
deps = 					\
	$(top_builddir)/cdk/libcdk-3.la

# libtool stuff: set version and export symbols for resolving
# since automake doesn't support conditionalized libsomething_la_LDFLAGS
# we use the general approach here
libtool_opts =							\
  $(CTK_LINK_FLAGS)						\
  -version-info $(LT_VERSION_INFO)				\
  -export-dynamic $(no_undefined)				\
  -rpath $(libdir)


included-modules:
if HAVE_INCLUDED_IMMMODULES
	@cd $(top_builddir)/modules/input && $(MAKE) $(AM_MAKEFLAGS) included-modules

libadd += $(INCLUDED_IMMODULE_OBJ)
deps += $(INCLUDED_IMMODULE_OBJ)

$(INCLUDED_IMMODULE_OBJ): included-modules
	@true
endif

.PHONY: included-modules

#
# setup source file variables
#

include $(srcdir)/a11y/Makefile.inc
include $(srcdir)/deprecated/Makefile.inc
include $(srcdir)/inspector/Makefile.inc

# CTK+ header files for public installation (non-generated, or generated
# by configure)

ctk_public_h_sources = 		\
	ctk.h			\
	ctk-autocleanups.h	\
	ctkx.h			\
	ctkx-autocleanups.h	\
	ctk-a11y.h		\
	ctkaboutdialog.h	\
	ctkaccelgroup.h		\
	ctkaccellabel.h		\
	ctkaccelmap.h		\
	ctkaccessible.h		\
	ctkactionable.h		\
	ctkactionbar.h		\
	ctkadjustment.h		\
	ctkappchooser.h		\
	ctkappchooserbutton.h	\
	ctkappchooserdialog.h	\
	ctkappchooserwidget.h	\
	ctkapplication.h	\
	ctkapplicationwindow.h	\
	ctkaspectframe.h	\
	ctkassistant.h		\
	ctkbbox.h		\
	ctkbin.h		\
	ctkbindings.h		\
	ctkborder.h		\
	ctkbox.h		\
	ctkbuilder.h		\
	ctkbuildable.h		\
	ctkbutton.h		\
	ctkcalendar.h		\
	ctkcellarea.h		\
	ctkcellareacontext.h	\
	ctkcellareabox.h	\
	ctkcelleditable.h	\
	ctkcelllayout.h		\
	ctkcellrenderer.h	\
	ctkcellrendereraccel.h	\
	ctkcellrenderercombo.h	\
	ctkcellrendererpixbuf.h	\
	ctkcellrendererprogress.h \
	ctkcellrendererspin.h	\
	ctkcellrendererspinner.h\
	ctkcellrenderertext.h	\
	ctkcellrenderertoggle.h	\
	ctkcellview.h		\
	ctkcheckbutton.h	\
	ctkcheckmenuitem.h	\
	ctkclipboard.h		\
	ctkcolorbutton.h	\
	ctkcolorchooser.h	\
	ctkcolorchooserwidget.h	\
	ctkcolorchooserdialog.h	\
	ctkcolorutils.h		\
	ctkcombobox.h		\
	ctkcomboboxtext.h	\
	ctkcontainer.h		\
	ctkcssprovider.h	\
	ctkcsssection.h 	\
	ctkdebug.h		\
	ctkdialog.h		\
	ctkdnd.h		\
	ctkdragdest.h		\
	ctkdragsource.h		\
	ctkdrawingarea.h	\
	ctkeditable.h		\
	ctkentry.h		\
	ctkentrybuffer.h	\
	ctkentrycompletion.h	\
	ctkenums.h		\
	ctkeventbox.h		\
	ctkeventcontroller.h	\
	ctkeventcontrollerkey.h	\
	ctkeventcontrollermotion.h	\
	ctkeventcontrollerscroll.h	\
	ctkexpander.h		\
	ctkfilechooser.h	\
	ctkfilechooserbutton.h	\
	ctkfilechooserdialog.h	\
	ctkfilechoosernative.h	\
	ctkfilechooserwidget.h	\
	ctkfilefilter.h		\
	ctkfixed.h		\
	ctkflowbox.h		\
	ctkfontbutton.h		\
	ctkfontchooser.h	\
	ctkfontchooserdialog.h  \
	ctkfontchooserwidget.h	\
	ctkframe.h		\
	ctkgesture.h		\
	ctkgesturedrag.h	\
	ctkgesturelongpress.h	\
	ctkgesturemultipress.h	\
	ctkgesturepan.h		\
	ctkgesturerotate.h	\
	ctkgesturesingle.h	\
	ctkgesturestylus.h	\
	ctkgestureswipe.h	\
	ctkgesturezoom.h	\
	ctkglarea.h		\
	ctkgrid.h		\
	ctkheaderbar.h		\
	ctkicontheme.h		\
	ctkiconview.h		\
	ctkimage.h		\
	ctkimcontext.h		\
	ctkimcontextinfo.h	\
	ctkimcontextsimple.h	\
	ctkimmodule.h		\
	ctkimmulticontext.h	\
	ctkinfobar.h		\
	ctkinvisible.h		\
	ctklabel.h		\
	ctklayout.h		\
	ctklevelbar.h		\
	ctklinkbutton.h		\
	ctklistbox.h		\
	ctkliststore.h		\
	ctklockbutton.h		\
	ctkmain.h		\
	ctkmenu.h		\
	ctkmenubar.h		\
	ctkmenubutton.h		\
	ctkmenuitem.h		\
	ctkmenushell.h		\
	ctkmenutoolbutton.h	\
	ctkmessagedialog.h	\
	ctkmodelbutton.h	\
	ctkmodules.h		\
	ctkmountoperation.h	\
	ctknativedialog.h	\
	ctknotebook.h		\
	ctkoffscreenwindow.h	\
	ctkorientable.h		\
	ctkoverlay.h		\
	ctkpadcontroller.h	\
	ctkpagesetup.h		\
	ctkpaned.h		\
	ctkpapersize.h		\
	ctkplacessidebar.h	\
	ctkplug.h		\
	ctkpopover.h		\
	ctkpopovermenu.h	\
	ctkprintcontext.h	\
	ctkprintoperation.h	\
	ctkprintoperationpreview.h \
	ctkprintsettings.h	\
	ctkprogressbar.h	\
	ctkradiobutton.h	\
	ctkradiomenuitem.h	\
	ctkradiotoolbutton.h	\
	ctkrange.h		\
	ctkrecentchooser.h	\
	ctkrecentchooserdialog.h \
	ctkrecentchoosermenu.h	\
	ctkrecentchooserwidget.h \
	ctkrecentfilter.h	\
	ctkrecentmanager.h	\
	ctkrender.h		\
	ctkrevealer.h		\
	ctkscale.h		\
	ctkscalebutton.h	\
	ctkscrollable.h		\
	ctkscrollbar.h		\
	ctkscrolledwindow.h	\
	ctksearchbar.h		\
	ctksearchentry.h	\
	ctkselection.h		\
	ctkseparator.h		\
	ctkseparatormenuitem.h	\
	ctkseparatortoolitem.h	\
	ctksettings.h		\
	ctkshortcutlabel.h	\
	ctkshortcutsgroup.h	\
	ctkshortcutssection.h	\
	ctkshortcutsshortcut.h	\
	ctkshortcutswindow.h	\
	ctkshow.h		\
	ctkstacksidebar.h	\
	ctksizegroup.h		\
	ctksizerequest.h	\
	ctksocket.h		\
	ctkspinbutton.h		\
	ctkspinner.h		\
	ctkstack.h		\
	ctkstackswitcher.h	\
	ctkstatusbar.h		\
	ctkstylecontext.h	\
	ctkstyleprovider.h	\
	ctkswitch.h		\
	ctktestutils.h		\
	ctktextattributes.h	\
	ctktextbuffer.h		\
	ctktextbufferrichtext.h	\
	ctktextchild.h		\
	ctktextdisplay.h	\
	ctktextiter.h		\
	ctktextmark.h		\
	ctktexttag.h		\
	ctktexttagtable.h	\
	ctktextview.h		\
	ctktogglebutton.h	\
	ctktoggletoolbutton.h	\
	ctktoolbar.h		\
	ctktoolbutton.h		\
	ctktoolitem.h		\
	ctktoolitemgroup.h	\
	ctktoolpalette.h	\
	ctktoolshell.h		\
	ctktooltip.h		\
	ctktreednd.h		\
	ctktreemodel.h		\
	ctktreemodelfilter.h	\
	ctktreemodelsort.h	\
	ctktreeselection.h	\
	ctktreesortable.h	\
	ctktreestore.h		\
	ctktreeview.h		\
	ctktreeviewcolumn.h	\
	ctktypes.h		\
	ctkviewport.h		\
	ctkvolumebutton.h	\
	ctkwidget.h		\
	ctkwidgetpath.h		\
	ctkwindow.h		\
	ctkwindowgroup.h

if OS_UNIX
ctk_unix_print_public_h_sources = 	\
	ctkpagesetupunixdialog.h	\
	ctkprintunixdialog.h		\
	ctkprinter.h			\
	ctkprintjob.h			\
	ctkunixprint-autocleanups.h	\
	ctkunixprint.h
endif

# Installed header files without compatibility guarantees
# that are not included in ctk/ctk.h
ctk_semi_private_h_sources = \
	ctktextlayout.h

ctk_private_type_h_sources =	\
	ctkcsstypesprivate.h	\
	ctktexthandleprivate.h


# CTK+ header files that don't get installed
ctk_private_h_sources =		\
	$(inspector_h_sources)	\
	$(ctk_private_type_h_sources) \
	ctkactionmuxer.h	\
	ctkactionobserver.h	\
	ctkactionobservable.h	\
	ctkadjustmentprivate.h	\
	ctkapplicationaccelsprivate.h	\
	ctkapplicationprivate.h	\
	ctkaccelgroupprivate.h	\
	ctkaccelmapprivate.h	\
	ctkactionhelper.h	\
	ctkallocatedbitmaskprivate.h	\
	ctkappchooserprivate.h	\
	ctkbindingsprivate.h	\
	ctkbitmaskprivate.h	\
	ctkbitmaskprivateimpl.h	\
	ctkbookmarksmanager.h	\
	ctkboxprivate.h         \
	ctkboxgadgetprivate.h	\
	ctkbuilderprivate.h	\
	ctkbuiltiniconprivate.h	\
	ctkbuttonprivate.h	\
	ctkcairoblurprivate.h	\
	ctkcellareaboxcontextprivate.h	\
	ctkcheckbuttonprivate.h	\
	ctkcheckmenuitemprivate.h	\
	ctkclipboardprivate.h		\
	ctkcolorswatchprivate.h	\
	ctkcoloreditorprivate.h	\
	ctkcolorpickerprivate.h	\
	ctkcolorpickerkwinprivate.h	\
	ctkcolorpickerportalprivate.h	\
	ctkcolorpickershellprivate.h	\
	ctkcolorplaneprivate.h	\
	ctkcolorscaleprivate.h	\
	ctkcolorchooserprivate.h	\
	ctkcomboboxprivate.h	\
	ctkcomposetable.h	\
	ctkcontainerprivate.h   \
	ctkcssanimationprivate.h	\
	ctkcssanimatedstyleprivate.h	\
	ctkcssarrayvalueprivate.h	\
	ctkcssbgsizevalueprivate.h	\
	ctkcssbordervalueprivate.h	\
	ctkcsscalcvalueprivate.h	\
	ctkcsscolorvalueprivate.h	\
	ctkcsscornervalueprivate.h	\
	ctkcsscustomgadgetprivate.h	\
	ctkcsscustompropertyprivate.h 	\
	ctkcssdimensionvalueprivate.h	\
	ctkcsseasevalueprivate.h	\
	ctkcssenginevalueprivate.h	\
	ctkcssenumvalueprivate.h	\
	ctkcssgadgetprivate.h		\
	ctkcssiconthemevalueprivate.h	\
	ctkcssimagebuiltinprivate.h	\
	ctkcssimagecrossfadeprivate.h	\
	ctkcssimagefallbackprivate.h	\
	ctkcssimagegradientprivate.h	\
	ctkcssimageiconthemeprivate.h	\
	ctkcssimagelinearprivate.h	\
	ctkcssimageradialprivate.h	\
	ctkcssimageprivate.h	\
	ctkcssimagesurfaceprivate.h	\
	ctkcssimageurlprivate.h	\
	ctkcssimagerecolorprivate.h	\
	ctkcssimagescaledprivate.h	\
	ctkcssimagevalueprivate.h	\
	ctkcssimagewin32private.h	\
	ctkcssinheritvalueprivate.h	\
	ctkcssinitialvalueprivate.h	\
	ctkcsskeyframesprivate.h	\
	ctkcsslookupprivate.h	\
	ctkcssmatcherprivate.h	\
	ctkcssnodeprivate.h		\
	ctkcssnodedeclarationprivate.h	\
	ctkcssnodestylecacheprivate.h	\
	ctkcssnumbervalueprivate.h	\
	ctkcsspalettevalueprivate.h	\
	ctkcssparserprivate.h	\
	ctkcsspathnodeprivate.h	\
	ctkcsspositionvalueprivate.h	\
	ctkcssproviderprivate.h	\
	ctkcssrepeatvalueprivate.h	\
	ctkcssrgbavalueprivate.h	\
	ctkcsssectionprivate.h 	\
	ctkcssselectorprivate.h	\
	ctkcssshadowsvalueprivate.h	\
	ctkcssshadowvalueprivate.h      \
	ctkcssshorthandpropertyprivate.h \
	ctkcssstaticstyleprivate.h	\
	ctkcssstringvalueprivate.h	\
	ctkcssstylefuncsprivate.h \
	ctkcssstylechangeprivate.h 	\
	ctkcssstyleprivate.h 	\
	ctkcssstylepropertyprivate.h \
	ctkcsstransformvalueprivate.h	\
	ctkcsstransientnodeprivate.h	\
	ctkcsstransitionprivate.h	\
	ctkcsstypedvalueprivate.h	\
	ctkcssunsetvalueprivate.h	\
	ctkcssvalueprivate.h	\
	ctkcsswin32sizevalueprivate.h   \
	ctkcsswidgetnodeprivate.h	\
	ctkcustompaperunixdialog.h \
	ctkdialogprivate.h 	\
	ctkdndprivate.h		\
	ctkemojichooser.h	\
	ctkemojicompletion.h	\
	ctkentryprivate.h	\
	ctkeventcontrollerprivate.h	\
	ctkfilechooserembed.h	\
	ctkfilechooserentry.h	\
	ctkfilechooserprivate.h	\
	ctkfilechoosernativeprivate.h	\
	ctkfilechooserwidgetprivate.h	\
	ctkfilechooserutils.h	\
	ctkfilefilterprivate.h	\
	ctkfilesystem.h		\
	ctkfilesystemmodel.h	\
	ctkfontchooserprivate.h	\
	ctkfontchooserutils.h	\
	ctkfontchooserwidgetprivate.h \
	ctkgestureprivate.h	\
	ctkgesturedragprivate.h	\
	ctkgesturelongpressprivate.h	\
	ctkgesturemultipressprivate.h	\
	ctkgesturepanprivate.h	\
	ctkgesturerotateprivate.h	\
	ctkgesturesingleprivate.h	\
	ctkgesturestylusprivate.h	\
	ctkgestureswipeprivate.h	\
	ctkgesturezoomprivate.h	\
	ctkheaderbarprivate.h	\
	ctkhslaprivate.h	\
	ctkiconcache.h		\
	ctkiconhelperprivate.h  \
	ctkiconprivate.h	\
	ctkiconthemeprivate.h  \
	ctkiconviewprivate.h	\
	ctkimagedefinitionprivate.h	\
	ctkimageprivate.h	\
	ctkimcontextsimpleprivate.h	\
	ctkimmoduleprivate.h	\
	ctkimcontextsimpleseqs.h \
	ctkintl.h		\
	ctkkeyhash.h		\
	ctkkineticscrolling.h	\
	ctklabelprivate.h	\
	ctklockbuttonprivate.h	\
	ctkmagnifierprivate.h	\
	ctkmenubuttonprivate.h	\
	ctkmenuprivate.h	\
	ctkmenuitemprivate.h	\
	ctkmenusectionbox.h	\
	ctkmenushellprivate.h	\
	ctkmenutracker.h	\
	ctkmenutrackeritem.h	\
	ctkmnemonichash.h	\
	ctkmodelmenuitem.h	\
	ctkmodifierstyle.h	\
	ctkmodulesprivate.h	\
	ctkmountoperationprivate.h \
	ctknativedialogprivate.h \
	ctkorientableprivate.h	\
	ctkpango.h		\
	ctkpathbar.h		\
	ctkplacessidebarprivate.h	\
	ctkplacesviewprivate.h	\
	ctkplacesviewrowprivate.h	\
	ctkpopoverprivate.h	\
	ctkprintoperation-private.h \
	ctkprintutils.h		\
	ctkprivate.h		\
	ctkpixelcacheprivate.h	\
	ctkprogresstrackerprivate.h	\
	ctkquery.h		\
	ctkrangeprivate.h	\
	ctkrbtree.h		\
	ctkrecentchooserdefault.h \
	ctkrecentchooserprivate.h \
	ctkrecentchooserutils.h	\
	ctkrenderbackgroundprivate.h \
	ctkrenderborderprivate.h \
	ctkrendericonprivate.h	\
	ctkrenderprivate.h	\
	ctkroundedboxprivate.h	\
	ctksearchengine.h	\
	ctksearchenginesimple.h	\
	ctksearchenginemodel.h	\
	ctksearchentryprivate.h \
	ctkselectionprivate.h	\
	ctksettingsprivate.h	\
	ctkshortcutswindowprivate.h	\
	ctkshortcutsshortcutprivate.h	\
	ctksidebarrowprivate.h	\
	ctksizegroup-private.h	\
	ctksizerequestcacheprivate.h	\
	ctksocketprivate.h	\
	ctkstyleanimationprivate.h \
	ctkstylecascadeprivate.h \
	ctkstylecontextprivate.h \
	ctkstylepropertyprivate.h \
	ctkstyleproviderprivate.h \
	ctktextattributesprivate.h \
	ctktextbtree.h		\
	ctktextbufferprivate.h \
	ctktextbufferserialize.h \
	ctktextchildprivate.h	\
	ctktexthandleprivate.h	\
	ctktextiterprivate.h	\
	ctktextmarkprivate.h	\
	ctktextsegment.h	\
	ctktexttagprivate.h	\
	ctktexttagtableprivate.h	\
	ctktexttypes.h		\
	ctktextutil.h		\
	ctktrashmonitor.h	\
	ctktogglebuttonprivate.h \
	ctktoolbarprivate.h	\
	ctktoolpaletteprivate.h	\
	ctktooltipprivate.h	\
	ctktooltipwindowprivate.h \
	ctktreedatalist.h	\
	ctktreeprivate.h	\
	ctkutilsprivate.h	\
	ctkwidgetprivate.h	\
	ctkwidgetpathprivate.h	\
	ctkwin32drawprivate.h	\
	ctkwin32themeprivate.h	\
	ctkwindowprivate.h	\
	ctktreemenu.h		\
	gdkpixbufutilsprivate.h \
	open-type-layout.h	\
	language-names.h	\
	script-names.h

# CTK+ C sources to build the library from
ctk_base_c_sources = 		\
	$(a11y_c_sources)	\
	$(deprecated_c_sources)	\
	$(inspector_c_sources)	\
	ctkactionmuxer.c	\
	ctkactionobserver.c	\
	ctkactionobservable.c	\
	ctkactionable.c		\
	ctkquery.c		\
	ctksearchbar.c		\
	ctksearchentry.c	\
	ctksearchengine.c	\
	ctksearchenginesimple.c	\
	ctksearchenginemodel.c	\
	fnmatch.c		\
	ctkaboutdialog.c	\
	ctkaccelgroup.c		\
	ctkaccellabel.c		\
	ctkaccelmap.c		\
	ctkaccessible.c		\
	ctkactionbar.c		\
	ctkactionhelper.c	\
	ctkadjustment.c		\
	ctkallocatedbitmask.c	\
	ctkappchooser.c		\
	ctkappchooserwidget.c	\
	ctkappchooserbutton.c	\
	ctkappchooserdialog.c	\
	ctkapplication.c	\
	ctkapplicationaccels.c	\
	ctkapplicationimpl.c	\
	ctkapplicationwindow.c	\
	ctkaspectframe.c	\
	ctkassistant.c		\
	ctkbbox.c		\
	ctkbin.c		\
	ctkbindings.c		\
	ctkbookmarksmanager.c	\
	ctkborder.c		\
	ctkbox.c		\
	ctkboxgadget.c		\
	ctkbuildable.c		\
	ctkbuilder.c		\
	ctkbuilderparser.c	\
	ctkbuilder-menus.c	\
	ctkbuiltinicon.c	\
	ctkbutton.c		\
	ctkcairoblur.c		\
	ctkcalendar.c		\
	ctkcellarea.c		\
	ctkcellareabox.c	\
	ctkcellareaboxcontext.c	\
	ctkcellareacontext.c	\
	ctkcelleditable.c	\
	ctkcelllayout.c		\
	ctkcellrenderer.c	\
	ctkcellrendereraccel.c	\
	ctkcellrenderercombo.c	\
	ctkcellrendererpixbuf.c	\
	ctkcellrendererprogress.c \
	ctkcellrendererspin.c	\
	ctkcellrendererspinner.c \
	ctkcellrenderertext.c	\
	ctkcellrenderertoggle.c	\
	ctkcellview.c		\
	ctkcheckbutton.c	\
	ctkcheckmenuitem.c	\
	ctkcolorbutton.c	\
	ctkcolorchooser.c	\
	ctkcolorchooserwidget.c	\
	ctkcolorchooserdialog.c	\
	ctkcoloreditor.c	\
	ctkcolorpicker.c	\
	ctkcolorpickerkwin.c	\
	ctkcolorpickerportal.c	\
	ctkcolorpickershell.c	\
	ctkcolorplane.c		\
	ctkcolorscale.c		\
	ctkcolorswatch.c	\
	ctkcolorutils.c		\
	ctkcombobox.c		\
	ctkcomboboxtext.c	\
	ctkcomposetable.c	\
	ctkcontainer.c		\
	ctkcssanimation.c	\
	ctkcssanimatedstyle.c	\
	ctkcssarrayvalue.c	\
	ctkcssbgsizevalue.c	\
	ctkcssbordervalue.c	\
	ctkcsscalcvalue.c	\
	ctkcsscolorvalue.c	\
	ctkcsscornervalue.c	\
	ctkcsscustomgadget.c	\
	ctkcsscustomproperty.c	\
	ctkcssdimensionvalue.c	\
	ctkcsseasevalue.c	\
	ctkcssenumvalue.c	\
	ctkcssenginevalue.c	\
	ctkcssgadget.c		\
	ctkcssiconthemevalue.c	\
	ctkcssimage.c		\
	ctkcssimagebuiltin.c	\
	ctkcssimagecrossfade.c	\
	ctkcssimagefallback.c	\
	ctkcssimagegradient.c	\
	ctkcssimageicontheme.c	\
	ctkcssimagelinear.c	\
	ctkcssimageradial.c	\
	ctkcssimagesurface.c	\
	ctkcssimageurl.c	\
	ctkcssimagerecolor.c	\
	ctkcssimagescaled.c	\
	ctkcssimagevalue.c	\
	ctkcssimagewin32.c	\
	ctkcssinheritvalue.c	\
	ctkcssinitialvalue.c	\
	ctkcsskeyframes.c	\
	ctkcsslookup.c		\
	ctkcssmatcher.c		\
	ctkcssnode.c		\
	ctkcssnodedeclaration.c	\
	ctkcssnodestylecache.c	\
	ctkcssnumbervalue.c	\
	ctkcsspalettevalue.c	\
	ctkcssparser.c		\
	ctkcsspathnode.c	\
	ctkcsspositionvalue.c	\
	ctkcssprovider.c	\
	ctkcssrepeatvalue.c	\
	ctkcssrgbavalue.c	\
	ctkcsssection.c 	\
	ctkcssselector.c	\
	ctkcssstringvalue.c	\
	ctkcssstyle.c		\
	ctkcssstylechange.c 	\
	ctkcssshadowsvalue.c	\
	ctkcssshadowvalue.c	\
	ctkcssshorthandproperty.c \
	ctkcssshorthandpropertyimpl.c \
	ctkcssstaticstyle.c	\
	ctkcssstylefuncs.c	\
	ctkcssstyleproperty.c	\
	ctkcssstylepropertyimpl.c \
	ctkcsstransformvalue.c	\
	ctkcsstransientnode.c	\
	ctkcsstransition.c	\
	ctkcsstypedvalue.c	\
	ctkcssunsetvalue.c	\
	ctkcsstypes.c		\
	ctkcssvalue.c		\
	ctkcsswidgetnode.c	\
        ctkcsswin32sizevalue.c  \
	ctkdialog.c		\
	ctkdragsource.c		\
	ctkdrawingarea.c	\
	ctkeditable.c		\
	ctkemojichooser.c	\
	ctkemojicompletion.c	\
	ctkentry.c		\
	ctkentrybuffer.c	\
	ctkentrycompletion.c	\
	ctkeventbox.c		\
	ctkeventcontroller.c	\
	ctkeventcontrollerkey.c	\
	ctkeventcontrollermotion.c	\
	ctkeventcontrollerscroll.c	\
	ctkexpander.c		\
	ctkfilechooser.c	\
	ctkfilechooserbutton.c	\
	ctkfilechooserdialog.c	\
	ctkfilechooserembed.c	\
	ctkfilechooserentry.c	\
	ctkfilechoosernative.c	\
	ctkfilechoosernativeportal.c	\
	ctkfilechooserutils.c	\
	ctkfilechooserwidget.c	\
	ctkfilefilter.c		\
	ctkfilesystem.c		\
	ctkfilesystemmodel.c	\
	ctkfixed.c		\
	ctkflowbox.c		\
	ctkfontbutton.c		\
	ctkfontchooser.c	\
	ctkfontchooserdialog.c	\
	ctkfontchooserutils.c	\
	ctkfontchooserwidget.c	\
	ctkframe.c		\
	ctkgladecatalog.c	\
	ctkgesture.c		\
	ctkgesturedrag.c	\
	ctkgesturelongpress.c	\
	ctkgesturemultipress.c	\
	ctkgesturepan.c		\
	ctkgesturerotate.c	\
	ctkgesturesingle.c	\
	ctkgesturestylus.c	\
	ctkgestureswipe.c	\
	ctkgesturezoom.c	\
	ctkglarea.c		\
	ctkgrid.c		\
	ctkheaderbar.c		\
	ctkhsla.c		\
	ctkicon.c		\
	ctkiconcache.c		\
	ctkiconcachevalidator.c	\
	ctkiconhelper.c		\
	ctkicontheme.c		\
	ctkiconview.c		\
	ctkimage.c		\
	ctkimagedefinition.c	\
	ctkimcontext.c		\
	ctkimcontextsimple.c	\
	ctkimmodule.c		\
	ctkimmulticontext.c	\
	ctkinfobar.c		\
	ctkinvisible.c		\
	ctkkeyhash.c		\
	ctkkineticscrolling.c	\
	ctklabel.c		\
	ctklayout.c		\
	ctklevelbar.c		\
	ctklinkbutton.c		\
	ctklistbox.c		\
	ctkliststore.c		\
	ctklockbutton.c		\
	ctkmain.c		\
	ctkmagnifier.c		\
	ctkmenu.c		\
	ctkmenubar.c		\
	ctkmenubutton.c		\
	ctkmenuitem.c		\
	ctkmenusectionbox.c	\
	ctkmenushell.c		\
	ctkmenutracker.c	\
	ctkmenutrackeritem.c	\
	ctkmenutoolbutton.c	\
	ctkmessagedialog.c	\
	ctkmnemonichash.c	\
	ctkmodelmenuitem.c	\
	ctkmodelbutton.c	\
	ctkmodifierstyle.c	\
	ctkmodules.c		\
	ctkmountoperation.c	\
	ctknativedialog.c	\
	ctknotebook.c		\
	ctkoffscreenwindow.c	\
	ctkorientable.c		\
	ctkoverlay.c		\
	ctkpadcontroller.c	\
	ctkpagesetup.c		\
	ctkpaned.c		\
	ctkpango.c		\
	ctkpapersize.c		\
	ctkpathbar.c		\
	ctkplacessidebar.c	\
	ctkplacesview.c		\
	ctkplacesviewrow.c	\
	ctkprintcontext.c	\
	ctkprintoperation.c	\
	ctkprintoperationpreview.c \
	ctkprintsettings.c	\
	ctkprintutils.c		\
	ctkprivate.c		\
	ctkprogressbar.c	\
	ctkprogresstracker.c	\
	ctkpixelcache.c		\
	ctkpopover.c		\
	ctkpopovermenu.c	\
	ctkradiobutton.c	\
	ctkradiomenuitem.c	\
	ctkradiotoolbutton.c	\
	ctkrange.c		\
	ctkrbtree.c		\
	ctkrecentchooserdefault.c \
	ctkrecentchooserdialog.c \
	ctkrecentchoosermenu.c	\
	ctkrecentchooserwidget.c \
	ctkrecentchooserutils.c	\
	ctkrecentchooser.c	\
	ctkrecentfilter.c	\
	ctkrecentmanager.c	\
	ctkrender.c		\
	ctkrenderbackground.c	\
	ctkrenderborder.c	\
	ctkrendericon.c		\
	ctkrevealer.c		\
	ctkroundedbox.c		\
	ctkscale.c		\
	ctkscalebutton.c	\
	ctkscrollable.c		\
	ctkscrollbar.c		\
	ctkscrolledwindow.c	\
	ctkselection.c		\
	ctkseparator.c		\
	ctkseparatormenuitem.c	\
	ctkseparatortoolitem.c	\
	ctksettings.c		\
	ctkshortcutsgroup.c	\
	ctkshortcutlabel.c	\
	ctkshortcutsshortcut.c	\
	ctkshortcutssection.c	\
	ctkshortcutswindow.c	\
	ctksidebarrow.c		\
	ctksizegroup.c		\
	ctksizerequest.c	\
	ctksizerequestcache.c	\
	ctkshow.c		\
	ctkstacksidebar.c	\
	ctkspinbutton.c		\
	ctkspinner.c		\
	ctkstack.c		\
	ctkstackswitcher.c	\
	ctkstatusbar.c		\
	ctkstyleanimation.c	\
	ctkstylecascade.c	\
	ctkstylecontext.c	\
	ctkstyleproperty.c	\
	ctkstyleprovider.c	\
	ctkstyleproviderprivate.c	\
	ctkswitch.c		\
	ctktestutils.c		\
	ctktextattributes.c	\
	ctktextbtree.c		\
	ctktextbuffer.c		\
	ctktextbufferrichtext.c	\
	ctktextbufferserialize.c \
	ctktextchild.c		\
	ctktextdisplay.c	\
	ctktexthandle.c		\
	ctktextiter.c		\
	ctktextlayout.c		\
	ctktextmark.c		\
	ctktextsegment.c	\
	ctktexttag.c		\
	ctktexttagtable.c	\
	ctktexttypes.c		\
	ctktextutil.c		\
	ctktextview.c		\
	ctktogglebutton.c	\
	ctktoggletoolbutton.c	\
	ctktoolbar.c		\
	ctktoolbutton.c		\
	ctktoolitem.c		\
	ctktoolitemgroup.c	\
	ctktoolpalette.c	\
	ctktoolshell.c		\
	ctktooltip.c		\
	ctktooltipwindow.c	\
	ctktrashmonitor.c	\
	ctktreedatalist.c	\
	ctktreednd.c		\
	ctktreemenu.c		\
	ctktreemodel.c		\
	ctktreemodelfilter.c	\
	ctktreemodelsort.c	\
	ctktreeselection.c	\
	ctktreesortable.c	\
	ctktreestore.c		\
	ctktreeview.c		\
	ctktreeviewcolumn.c	\
	ctkutils.c		\
	ctkvolumebutton.c	\
	ctkviewport.c		\
	ctkwidget.c		\
	ctkwidgetpath.c		\
	ctkwindow.c		\
	ctkwindowgroup.c	\
	ctkwin32draw.c		\
	ctkwin32theme.c		\
	gdkpixbufutils.c	\
	language-names.c	\
	script-names.c

if USE_QUARTZ
ctk_base_c_sources += 		\
	ctkclipboard-quartz.c	\
	ctkdnd-quartz.c
else
ctk_base_c_sources += 		\
	ctkclipboard.c 		\
	ctkdnd.c		\
	ctkdragdest.c
endif

nodist_ctk_c_sources =
ctk_c_sources = $(ctk_base_c_sources)

ctk_dbus_built_sources = ctkdbusgenerated.c ctkdbusgenerated.h

$(ctk_dbus_built_sources) : Makefile.am ctkdbusinterfaces.xml
	$(AM_V_GEN) gdbus-codegen \
	--interface-prefix org.Ctk. \
	--c-namespace _Ctk \
	--generate-c-code ctkdbusgenerated \
	$(srcdir)/ctkdbusinterfaces.xml

nodist_ctk_c_sources += $(ctk_dbus_built_sources)

ctk_os_unix_c_sources = 		\
	ctkcustompaperunixdialog.c	\
	ctkpagesetupunixdialog.c	\
	ctkprinter.c			\
	ctkprinteroption.c		\
	ctkprinteroptionset.c		\
	ctkprinteroptionwidget.c	\
	ctkprintjob.c			\
	ctkprintoperation-unix.c	\
	ctkprintoperation-portal.h	\
	ctkprintoperation-portal.c	\
	ctkprintunixdialog.c		\
        ctkprintbackend.c		\
	ctksearchenginetracker.c

if OS_UNIX
ctk_private_h_sources += \
	ctkiconcachevalidator.h		\
	ctkprintbackend.h		\
	ctkprinter-private.h		\
	ctkprinteroption.h		\
	ctkprinteroptionset.h		\
	ctkprinteroptionwidget.h	\
	ctksearchenginetracker.h

if HAVE_TRACKER3
ctk_private_h_sources += \
	ctksearchenginetracker3.h
ctk_os_unix_c_sources += \
	ctksearchenginetracker3.c
endif

ctk_c_sources += $(ctk_os_unix_c_sources)
endif

ctk_os_win32_c_sources = \
	ctkprint-win32.c		\
	ctkprintoperation-win32.c	\
	ctkfilechoosernativewin32.c	\
	ctkwin32.c
if OS_WIN32
ctk_private_h_sources += ctkprint-win32.h
ctk_c_sources += $(ctk_os_win32_c_sources)
endif

ctk_use_x11_c_sources = 	\
	ctkplug.c		\
	ctksocket.c		\
	ctkxembed.c		\
	deprecated/ctktrayicon-x11.c	\
	ctkapplication-x11.c	\
	ctkmountoperation-x11.c

ctk_use_wayland_c_sources = \
	ctkapplication-wayland.c

ctk_use_wayland_or_x11_c_sources = \
	ctkapplication-dbus.c

ctk_use_win32_c_sources = 	\
	ctkwin32embed.c		\
	ctkwin32embedwidget.c	\
	ctkmountoperation-stub.c
ctk_use_quartz_c_sources = 	\
	ctksearchenginequartz.c	\
	ctkmountoperation-stub.c \
	ctkapplication-quartz.c \
	ctkapplication-quartz-menu.c \
	ctkfilechoosernativequartz.c \
	ctkquartz.c
ctk_use_stub_c_sources = 	\
	ctkmountoperation-stub.c
ctk_use_x11_private_h_sources = 	\
	ctkxembed.h		\
	deprecated/ctktrayicon.h	\
	xembed.h
if USE_X11
ctk_c_sources += $(ctk_use_x11_c_sources)
ctk_private_h_sources += $(ctk_use_x11_private_h_sources)
endif

if USE_WAYLAND
ctk_c_sources += $(ctk_use_wayland_c_sources)
endif

# pretty tricky way to write USE_WAYLAND || USE_X11...
if USE_WAYLAND
ctk_c_sources += $(ctk_use_wayland_or_x11_c_sources)
else
if USE_X11
ctk_c_sources += $(ctk_use_wayland_or_x11_c_sources)
else
endif
endif

ctk_use_win32_private_h_sources = 	\
	ctkwin32embed.h		\
	ctkwin32embedwidget.h
if USE_WIN32
ctk_c_sources += $(ctk_use_win32_c_sources)
ctk_private_h_sources += $(ctk_use_win32_private_h_sources)
endif

ctk_use_quartz_private_h_sources = 	\
	ctksearchenginequartz.h	\
	ctkquartz.h
if USE_QUARTZ
ctk_c_sources += $(ctk_use_quartz_c_sources)
ctk_private_h_sources += $(ctk_use_quartz_private_h_sources)
endif

ctk_all_private_h_sources = \
	$(a11y_private_h_sources) \
	$(deprecated_private_h_sources) \
	$(ctk_private_h_sources) \
	$(ctk_use_x11_private_h_sources) \
	$(ctk_use_win32_private_h_sources) \
	$(ctk_use_quartz_private_h_sources)

if !USE_X11
if !USE_WIN32
if !USE_QUARTZ
ctk_c_sources += $(ctk_use_stub_c_sources)
endif
endif
endif

# we use our own built_sources variable rules to avoid automake's
# BUILT_SOURCES oddities
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content

# built sources that get installed with the header files
ctk_built_public_sources =			\
	ctktypebuiltins.h

# built headers that don't get installed
ctk_built_private_headers =			\
	ctkresources.h				\
	ctkmarshalers.h				\
	ctkprivatetypebuiltins.h

ctk_built_sources =				\
	$(ctk_built_private_headers)		\
	$(ctk_built_public_sources)		\
	ctkresources.c				\
	ctk.gresource.xml			\
	ctktypebuiltins.c			\
	ctkmarshalers.c				\
	ctkprivatetypebuiltins.c

nodist_ctk_c_sources += $(ctk_built_sources)

stamp_files =					\
	stamp-ctkmarshalers.h			\
	stamp-ctktypebuiltins.h			\
	stamp-ctkprivatetypebuiltins.h

# non-header sources (headers should be specified in the above variables)
# that don't serve as direct make target sources, i.e. they don't have
# their own .lo rules and don't get publically installed
ctk_extra_sources =				\
	paper_names.c				\
	paper_names_offsets.c			\
	gen-paper-names.c			\
	deprecated/ctkstatusicon-quartz.c	\
	ctkversion.h.in				\
	ctkmarshalers.list			\
	fallback-c89.c

#
# setup CTK+ sources and their dependencies
#
DISTCLEANFILES = \
	$(ctk_built_sources) 			\
	$(ctk_dbus_built_sources)		\
	$(print_portal_built_sources)		\
	$(stamp_files)				\
	ctktypefuncs.inc

if OS_WIN32
DISTCLEANFILES += ctk-win32.rc
endif

EXTRA_DIST += $(ctk_all_private_h_sources) $(ctk_extra_sources)


pkgdatadir = $(datadir)/ctk-$(CTK_API_VERSION)
pkgdata_DATA = ctkbuilder.rng

EXTRA_DIST += ctkbuilder.rnc ctkbuilder.rng

itsdir = $(datadir)/gettext/its
its_DATA = ctkbuilder.loc ctkbuilder.its

EXTRA_DIST += $(its_DATA)

templates =				\
	$(inspector_templates)		\
	ui/ctkapplication-quartz.ui	\
	ui/ctkaboutdialog.ui		\
	ui/ctkactionbar.ui		\
	ui/ctkappchooserdialog.ui	\
	ui/ctkappchooserwidget.ui	\
	ui/ctkassistant.ui		\
	ui/ctkcolorchooserdialog.ui	\
	ui/ctkcoloreditor.ui		\
	ui/ctkcombobox.ui		\
	ui/ctkdialog.ui		\
	ui/ctkfilechooserbutton.ui	\
	ui/ctkemojichooser.ui		\
	ui/ctkfilechooserwidget.ui	\
	ui/ctkfilechooserdialog.ui	\
	ui/ctkfontbutton.ui		\
	ui/ctkfontchooserdialog.ui	\
	ui/ctkfontchooserwidget.ui	\
	ui/ctkinfobar.ui		\
	ui/ctklockbutton.ui		\
	ui/ctkmessagedialog.ui	\
	ui/ctkpagesetupunixdialog.ui	\
	ui/ctkpathbar.ui		\
	ui/ctkplacesview.ui		\
	ui/ctkplacesviewrow.ui		\
	ui/ctkprintunixdialog.ui	\
	ui/ctkrecentchooserdefault.ui	\
	ui/ctksearchbar.ui		\
	ui/ctkscalebutton.ui		\
	ui/ctkstatusbar.ui		\
	ui/ctktooltipwindow.ui		\
	ui/ctkvolumebutton.ui		\
	ui/ctksidebarrow.ui

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-gdef xgen-gtbh xgen-gtic xgen-gmh xgen-gmc xgen-gmlh xgen-gmlc xgen-gtfsrc.c xgen-gtf
CLEANFILES = $(gen_sources)

BUILT_SOURCES = \
	$(ctk_built_sources) \
	$(ctk_dbus_built_sources) \
	$(print_portal_built_sources)

# all autogenerated files need to be generated in the srcdir,
# so old versions get remade and are not confused with newer
# versions in the build dir. thus a development setup requires
# srcdir to be writable, passing --disable-rebuilds to
# ../configure will supress all autogeneration rules.
ctkmarshalers.h: stamp-ctkmarshalers.h
	@true
stamp-ctkmarshalers.h: ctkmarshalers.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) \
		--prefix=_ctk_marshal \
		--header \
		--valist-marshallers \
		$(srcdir)/ctkmarshalers.list >> xgen-gmlh \
	&& (cmp -s xgen-gmlh ctkmarshalers.h || cp xgen-gmlh ctkmarshalers.h) \
	&& rm -f xgen-gmlh \
	&& echo timestamp > $(@F)
ctkmarshalers.c: ctkmarshalers.list
	$(AM_V_GEN) (echo "#undef G_ENABLE_DEBUG"; \
	 $(GLIB_GENMARSHAL) \
	 	--prefix=_ctk_marshal \
		--body \
		--valist-marshallers $(srcdir)/ctkmarshalers.list) >> xgen-gmlc \
	&& cp xgen-gmlc ctkmarshalers.c \
	&& rm -f xgen-gmlc

ctktypebuiltins.h: stamp-ctktypebuiltins.h
	@true
stamp-ctktypebuiltins.h: $(ctk_public_h_sources) $(a11y_h_sources) $(deprecated_h_sources) ctktypebuiltins.h.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) --template ctktypebuiltins.h.template \
		$(ctk_public_h_sources) $(a11y_h_sources) $(deprecated_h_sources) ) > xgen-gtbh \
	&& (cmp -s xgen-gtbh ctktypebuiltins.h || cp xgen-gtbh ctktypebuiltins.h ) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)
ctktypebuiltins.c: $(ctk_public_h_sources) $(a11y_h_sources) $(deprecated_h_sources) ctktypebuiltins.c.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) --template ctktypebuiltins.c.template \
		$(ctk_public_h_sources) $(a11y_h_sources) $(deprecated_h_sources) ) > xgen-gtbc \
	&& cp xgen-gtbc ctktypebuiltins.c  \
	&& rm -f xgen-gtbc

ctk.gresource.xml: Makefile.am inspector/Makefile.inc
	$(AM_V_GEN) echo "<?xml version='1.0' encoding='UTF-8'?>" > $@; \
	echo "<gresources>" >> $@; \
	echo "  <gresource prefix='/org/ctk/libctk'>" >> $@; \
        echo "    <file>theme/Adwaita/ctk.css</file>" >> $@; \
        echo "    <file>theme/Adwaita/ctk-dark.css</file>" >> $@; \
        echo "    <file>theme/Adwaita/ctk-contained.css</file>" >> $@; \
        echo "    <file>theme/Adwaita/ctk-contained-dark.css</file>" >> $@; \
        for f in $(srcdir)/theme/Adwaita/assets/*.png; do \
          n=`basename $$f`; \
          echo "    <file preprocess='to-pixdata'>theme/Adwaita/assets/$$n</file>" >> $@; \
        done; \
        for f in $(srcdir)/theme/Adwaita/assets/*.svg; do \
          n=`basename $$f`; \
          echo "    <file>theme/Adwaita/assets/$$n</file>" >> $@; \
        done; \
	echo "    <file>theme/HighContrast/ctk.css</file>" >> $@; \
	echo "    <file alias='theme/HighContrastInverse/ctk.css'>theme/HighContrast/ctk-inverse.css</file>" >> $@; \
	echo "    <file>theme/HighContrast/ctk-contained.css</file>" >> $@; \
	echo "    <file>theme/HighContrast/ctk-contained-inverse.css</file>" >> $@; \
	      for f in $(srcdir)/theme/HighContrast/assets/*.png; do \
          n=`basename $$f`; \
          echo "    <file preprocess='to-pixdata'>theme/HighContrast/assets/$$n</file>" >> $@; \
        done; \
        for f in $(srcdir)/theme/HighContrast/assets/*.svg; do \
          n=`basename $$f`; \
          echo "    <file>theme/HighContrast/assets/$$n</file>" >> $@; \
        done; \
	echo "    <file>theme/win32/ctk-win32-base.css</file>" >> $@; \
	echo "    <file>theme/win32/ctk.css</file>" >> $@; \
	for f in $(srcdir)/cursor/*.png; do \
	  n=`basename $$f`; \
	  echo "    <file>cursor/$$n</file>" >> $@; \
	done; \
	for f in $(srcdir)/gesture/*.symbolic.png; do \
	  n=`basename $$f`; \
	  echo "    <file alias='icons/64x64/actions/$$n'>gesture/$$n</file>" >> $@; \
	done; \
	for f in $(srcdir)/ui/*.ui; do \
	  n=`basename $$f`; \
	  echo "    <file preprocess='xml-stripblanks'>ui/$$n</file>" >> $@; \
	done; \
	for s in 16x16 22x22 24x24 32x32 48x48; do \
	  for c in actions status categories; do \
	    for f in $(srcdir)/icons/$$s/$$c/*.png; do \
	      test "$$f" = "$(srcdir)/icons/$$s/$$c/*.png" && continue; \
	      n=`basename $$f`; \
	      echo "    <file>icons/$$s/$$c/$$n</file>" >> $@; \
	    done; \
	  done; \
	done; \
	for s in scalable; do \
	  for c in status; do \
	    for f in $(srcdir)/icons/$$s/$$c/*.svg; do \
	      test "$$f" = "$(srcdir)/icons/$$s/$$c/*.svg" && continue; \
	      n=`basename $$f`; \
	      echo "    <file>icons/$$s/$$c/$$n</file>" >> $@; \
	    done; \
	  done; \
	done; \
	for f in $(srcdir)/inspector/*.ui; do \
	  n=`basename $$f`; \
	  echo "    <file compressed='true' preprocess='xml-stripblanks'>inspector/$$n</file>" >> $@; \
	done; \
	echo "    <file>inspector/logo.png</file>" >> $@; \
	echo "    <file>emoji/emoji.data</file>" >> $@; \
	echo "  </gresource>" >> $@; \
	echo "</gresources>" >> $@;

gesture_sources = 				\
	gesture/gesture-pinch-symbolic.svg			\
	gesture/gesture-stretch-symbolic.svg			\
	gesture/gesture-rotate-clockwise-symbolic.svg		\
	gesture/gesture-rotate-anticlockwise-symbolic.svg	\
	gesture/gesture-two-finger-swipe-left-symbolic.svg	\
	gesture/gesture-two-finger-swipe-right-symbolic.svg

adwaita_theme_scss = \
	theme/Adwaita/_colors-public.scss 	\
	theme/Adwaita/_colors.scss 		\
	theme/Adwaita/_common.scss 		\
	theme/Adwaita/_drawing.scss 		\
	theme/Adwaita/ctk-contained-dark.scss 	\
	theme/Adwaita/ctk-contained.scss 	\
	$()

highcontrast_theme_scss = \
	theme/HighContrast/_colors-hc.scss 	\
	theme/HighContrast/ctk-contained-inverse.scss \
	theme/HighContrast/ctk-contained.scss 	\
	$()

if REBUILD_SCSS
# If we have sassc then we can rebuild the theme CSS as soon as the SCSS
# files have been changed

scss_verbose = $(scss_verbose_@AM_V@)
scss_verbose_ = $(scss_verbose_@AM_DEFAULT_V@)
scss_verbose_0 = @echo "  SCSS     $@";

SASSC_OPTS = -a -M -t compact

theme/Adwaita/ctk-contained.css: $(top_srcdir)/ctk/theme/Adwaita/ctk-contained.scss $(adwaita_theme_scss)
	$(scss_verbose) $(MKDIR_P) $(dir $@) && $(SASSC) $(SASSC_OPTS) $< $@ || rm -f $@

theme/Adwaita/ctk-contained-dark.css: $(top_srcdir)/ctk/theme/Adwaita/ctk-contained-dark.scss $(adwaita_theme_scss)
	$(scss_verbose) $(MKDIR_P) $(dir $@) && $(SASSC) $(SASSC_OPTS) $< $@ || rm -f $@

theme/HighContrast/ctk-contained.css: $(top_srcdir)/ctk/theme/HighContrast/ctk-contained.scss $(highcontrast_theme_scss)
	$(scss_verbose) $(MKDIR_P) $(dir $@) && $(SASSC) $(SASSC_OPTS) $< $@ || rm -f $@

theme/HighContrast/ctk-contained-inverse.css: $(top_srcdir)/ctk/theme/HighContrast/ctk-contained-inverse.scss $(highcontrast_theme_scss)
	$(scss_verbose) $(MKDIR_P) $(dir $@) && $(SASSC) $(SASSC_OPTS) $< $@ || rm -f $@

endif # REBUILD_SCSS

theme_sources = 				\
	theme/win32/ctk.css	 		\
	theme/win32/ctk-win32-base.css 		\
	$(adwaita_theme_scss)			\
	theme/Adwaita/assets.txt 		\
	theme/Adwaita/assets.svg 		\
	theme/Adwaita/README 			\
	theme/Adwaita/ctk-contained.css 	\
	theme/Adwaita/ctk-contained-dark.css 	\
	theme/Adwaita/ctk.css 			\
	theme/Adwaita/ctk-dark.css 		\
	$(highcontrast_theme_scss)		\
	theme/HighContrast/ctk-contained.css 	\
	theme/HighContrast/ctk-contained-inverse.css \
	theme/HighContrast/ctk.css 		\
	theme/HighContrast/ctk-inverse.css

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(builddir)/ctk.gresource.xml)

ctkresources.h: ctk.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< \
		--target=$@ --sourcedir=$(srcdir) --c-name _ctk --generate-header --manual-register --internal
ctkresources.c: ctk.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $< \
		--target=$@ --sourcedir=$(srcdir) --c-name _ctk --generate-source --manual-register --internal

ctkprivatetypebuiltins.h: stamp-ctkprivatetypebuiltins.h
	@true
stamp-ctkprivatetypebuiltins.h: $(ctk_private_type_h_sources) ctkprivatetypebuiltins.h.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) --template ctkprivatetypebuiltins.h.template \
		$(ctk_private_type_h_sources) ) > xgen-gptbh \
	&& (cmp -s xgen-gptbh ctkprivatetypebuiltins.h || cp xgen-gptbh ctkprivatetypebuiltins.h ) \
	&& rm -f xgen-gptbh \
	&& echo timestamp > $(@F)
ctkprivatetypebuiltins.c: $(ctk_private_type_h_sources)  ctkprivatetypebuiltins.c.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) --template ctkprivatetypebuiltins.c.template \
		$(ctk_private_type_h_sources) ) > xgen-gptbc \
	&& cp xgen-gptbc ctkprivatetypebuiltins.c  \
	&& rm -f xgen-gptbc


ctktypefuncs.inc: stamp-ctktypebuiltins.h stamp-ctkprivatetypebuiltins.h $(top_srcdir)/ctk/*.h $(top_srcdir)/ctk/a11y/*.h $(top_srcdir)/ctk/deprecated/*.h $(top_srcdir)/cdk/*.h Makefile
	$(AM_V_GEN) export LC_ALL=C ; \
	  (echo '#undef CTK_COMPILATION' && echo '#include <ctk/ctkx.h>') > xgen-gtfsrc.c && \
	  echo 'G_GNUC_BEGIN_IGNORE_DEPRECATIONS' > xgen-gtf && \
	  ${CPP} $(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) xgen-gtfsrc.c | \
	  $(GREP) -o '\bg[td]k_[a-zA-Z0-9_]*_get_type\b' | \
	  sort | uniq | \
	  $(SED) '{ s/^/*tp++ = /; s/$$/();/; s/^.*\(cdk_x11\|ctk_plug_\|ctk_socket_\).*$$/#ifdef CDK_WINDOWING_X11\n&\n#endif/; }' >> xgen-gtf \
	&& cp xgen-gtf $@ && rm -f xgen-gtf
$(srcdir)/ctktestutils.c: ctktypefuncs.inc

# target platform:
lib_LTLIBRARIES = libctk-3.la

ctkincludedir = $(includedir)/ctk-3.0/ctk
ctkinclude_HEADERS = $(ctk_public_h_sources) $(ctk_semi_private_h_sources)
nodist_ctkinclude_HEADERS = $(ctk_built_public_sources) ctkversion.h

a11yincludedir = $(includedir)/ctk-3.0/ctk/a11y
a11yinclude_HEADERS= $(a11y_h_sources)

deprecatedincludedir = $(includedir)/ctk-3.0/ctk/deprecated
deprecatedinclude_HEADERS= $(deprecated_h_sources)

ctkunixprintincludedir = $(includedir)/ctk-3.0/unix-print/ctk
ctkunixprintinclude_HEADERS = $(ctk_unix_print_public_h_sources)

nodist_libctk_3_la_SOURCES = $(nodist_ctk_c_sources)
libctk_3_la_SOURCES = $(ctk_c_sources)
libctk_3_la_CFLAGS = $(AM_CFLAGS) $(CDK_HIDDEN_VISIBILITY_CFLAGS)
libctk_3_la_LDFLAGS = $(libtool_opts)
libctk_3_la_LIBADD = $(libadd)
libctk_3_la_DEPENDENCIES = $(deps)

if USE_QUARTZ
libctk_3_la_CFLAGS += "-xobjective-c"
endif

if USE_WIN32
libctk_3_la_LIBADD += -lole32 -lgdi32 -lcomdlg32 -lwinspool -lcomctl32
libctk_3_la_LDFLAGS += -Wl,-luuid $(ctk_win32_res_ldflag)
libctk_3_la_DEPENDENCIES += $(ctk_win32_res)
endif

install-exec-hook:
if DISABLE_EXPLICIT_DEPS
	$(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/libctk-3.la
endif

if USE_QUARTZ
install-mac-key-theme:
	$(MKDIR_P) $(DESTDIR)$(datadir)/themes/Mac/ctk-3.0
	$(INSTALL_DATA) $(srcdir)/ctk-keys.css.mac $(DESTDIR)$(datadir)/themes/Mac/ctk-3.0/ctk-keys.css
uninstall-mac-key-theme:
	rm -f $(DESTDIR)$(datadir)/themes/Mac/ctk-3.0/ctk-keys.css
else
install-mac-key-theme:
uninstall-mac-key-theme:
endif

# ------------------- MSVC Build Items ----------------
MSVCPROJS = ctk-3

ctk_3_FILES = \
	$(ctk_base_c_sources)		\
	$(ctk_os_win32_c_sources)	\
	$(ctk_use_win32_c_sources)	\
	$(ctk_dbus_built_sources)	\
	$(ctk_built_sources)

ctk_3_EXCLUDES = dummy
ctk_3_HEADERS_DIR = $(ctkincludedir)

ctk_3_HEADERS_INST = \
	$(ctkinclude_HEADERS)	\
	$(nodist_ctkinclude_HEADERS)	\
	$(a11y_h_sources)	\
	$(deprecated_h_sources)

ctk_3_HEADERS_EXCLUDES = dummy

include $(top_srcdir)/win32/Makefile.msvcproj

if HAVE_INTROSPECTION
# Introspection Items for MSVC
MSVC_INTROSPECT_GIRS = Ctk-3.0.gir

CTK_MSVC_GIR_CFLAGS =						\
	$(CTK_BASE_CFLAGS_DEFINES)				\
	-DCTK_LIBDIR=\\\"/dummy/lib\\\"				\
	-DCTK_DATADIR=\\\"/dummy/share\\\"			\
	-DCTK_DATA_PREFIX=\\\"/dummy\\\"			\
	-DCTK_SYSCONFDIR=\\\"/dummy/etc\\\"			\
	-DCTK_HOST=\\\"'$$$$(AT_PLAT)'-pc-vs'$$$$(VSVER)'\\\"	\
	-DCTK_PRINT_BACKENDS=\\\"file\\\"			\
	-DINCLUDE_IM_am_et			\
	-DINCLUDE_IM_cedilla			\
	-DINCLUDE_IM_cyrillic_translit		\
	-DINCLUDE_IM_ime			\
	-DINCLUDE_IM_inuktitu			\
	-DINCLUDE_IM_ipa			\
	-DINCLUDE_IM_multipress			\
	-DINCLUDE_IM_thai			\
	-DINCLUDE_IM_ti_er			\
	-DINCLUDE_IM_ti_et			\
	-DINCLUDE_IM_viqr			\
	-DCTK_TEXT_USE_INTERNAL_UNSUPPORTED_API	\
	-I.. -I../ctk -I../cdk

INTROSPECTION_INTERMEDIATE_ITEMS = \
	$(top_builddir)/win32/Ctk-3.0.gir.msvc.introspect		\
	$(top_builddir)/win32/Ctk_3_0_gir_list

Ctk_3_0_gir_MSVC_FILES =		\
	$(introspected_pub_headers)	\
	$(ctk_base_c_sources)		\
	$(ctk_os_win32_c_sources)	\
	ctktypebuiltins.h		\
	ctktypebuiltins.c

Ctk_3_0_gir_MSVC_EXPORT_PACKAGES = $(Ctk_3_0_gir_EXPORT_PACKAGES)
Ctk_3_0_gir_MSVC_INCLUDE_GIRS = Atk-1.0
Ctk_3_0_gir_MSVC_LIBS = ctk-3.0 cdk-3.0
Ctk_3_0_gir_MSVC_CFLAGS = $(CTK_MSVC_GIR_CFLAGS)
Ctk_3_0_gir_MSVC_SCANNERFLAGS = --warn-all --include-uninstalled='./vs$$$$(VSVER)/$$$$(CFG)/$$$$(PLAT)/bin/Cdk-3.0.gir'

include $(top_srcdir)/win32/Makefile.msvc-introspection

else
INTROSPECTION_INTERMEDIATE_ITEMS =

endif

dist-hook: \
	$(top_builddir)/win32/vs9/ctk-3.vcproj	\
	$(top_builddir)/win32/vs9/ctk-3.headers	\
	$(INTROSPECTION_INTERMEDIATE_ITEMS)

# Install a RC file for the default CTK+ theme, and key themes
install-data-local: install-ms-lib install-def-file install-mac-key-theme
	$(MKDIR_P) $(DESTDIR)$(datadir)/themes/Default/ctk-3.0
	$(INSTALL_DATA) $(srcdir)/ctk-keys.css.default $(DESTDIR)$(datadir)/themes/Default/ctk-3.0/ctk-keys.css
	$(MKDIR_P) $(DESTDIR)$(datadir)/themes/Emacs/ctk-3.0
	$(INSTALL_DATA) $(srcdir)/ctk-keys.css.emacs $(DESTDIR)$(datadir)/themes/Emacs/ctk-3.0/ctk-keys.css

uninstall-local: uninstall-ms-lib uninstall-def-file uninstall-mac-key-theme
	rm -f $(DESTDIR)$(datadir)/themes/Default/ctk-3.0/ctk-keys.css
	rm -f $(DESTDIR)$(datadir)/themes/Emacs/ctk-3.0/ctk-keys.css

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

if HAVE_INTROSPECTION
introspected_pub_headers = $(filter-out %private.h ctktextdisplay.h ctktextlayout.h ctkx.h, $(ctkinclude_HEADERS) $(a11yinclude_HEADERS) $(deprecatedinclude_HEADERS) $(nodist_ctkinclude_HEADERS))

introspection_files = \
    $(introspected_pub_headers) \
    $(filter-out %win32.c, $(ctk_base_c_sources)) \
    ctkprintoperation-unix.c \
    ctktypebuiltins.h \
    ctktypebuiltins.c

if USE_X11
introspection_files += \
    ctksocket.c \
    ctkplug.c
endif

include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_SCANNER_ENV = CC="$(CC)"
INTROSPECTION_COMPILER_ARGS = \
	--includedir=$(srcdir) \
	--includedir=. \
	--includedir=../cdk

Ctk-3.0.gir: $(INTROSPECTION_SCANNER) libctk-3.la $(top_builddir)/cdk/Cdk-3.0.gir Makefile
Ctk_3_0_gir_SCANNERFLAGS = 			\
	--warn-all				\
	--add-include-path=$(top_builddir)/cdk	\
	--include-uninstalled=$(top_builddir)/cdk/Cdk-3.0.gir
Ctk_3_0_gir_INCLUDES = Atk-1.0
Ctk_3_0_gir_C_INCLUDES = ctk/ctk.h ctk/ctk-a11y.h
if USE_X11
Ctk_3_0_gir_SCANNERFLAGS += --add-include-path=$(top_builddir)/cdk/x11
Ctk_3_0_gir_INCLUDES += xlib-2.0
Ctk_3_0_gir_C_INCLUDES += ctk/ctkx.h
endif
Ctk_3_0_gir_CFLAGS = \
		$(AM_CPPFLAGS) \
		-DCTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
Ctk_3_0_gir_FILES = $(introspection_files)
Ctk_3_0_gir_LIBS = libctk-3.la $(top_builddir)/cdk/libcdk-3.la
Ctk_3_0_gir_EXPORT_PACKAGES = ctk+-3.0
INTROSPECTION_GIRS = Ctk-3.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
endif

#
# Installed tools
#
bin_PROGRAMS = \
	ctk-query-immodules-3.0	\
	ctk-update-icon-cache \
	ctk-encode-symbolic-svg \
	ctk-builder-tool \
	ctk-query-settings \
	ctk-launch

ctk_query_immodules_3_0_SOURCES = queryimmodules.c ctkutils.c
ctk_query_immodules_3_0_LDADD = 		\
	libctk-3.la				\
	$(top_builddir)/cdk/libcdk-3.la		\
	$(GMODULE_LIBS)				\
	$(CTK_DEP_LIBS)

ctk_update_icon_cache_SOURCES = updateiconcache.c
ctk_update_icon_cache_LDADD = $(GDK_PIXBUF_LIBS)

ctk_encode_symbolic_svg_SOURCES = encodesymbolic.c
ctk_encode_symbolic_svg_LDADD =			\
	$(GDK_PIXBUF_LIBS)			\
	$(top_builddir)/cdk/libcdk-3.la		\
	$(CTK_DEP_LIBS)

ctk_builder_tool_SOURCES = ctk-builder-tool.c
ctk_builder_tool_LDADD =			\
	libctk-3.la				\
	$(top_builddir)/cdk/libcdk-3.la		\
	$(CTK_DEP_LIBS)

ctk_query_settings_SOURCES = ctk-query-settings.c
ctk_query_settings_LDADD= 			\
	libctk-3.la				\
	$(top_builddir)/cdk/libcdk-3.la		\
	$(CTK_DEP_LIBS)

ctk_launch_SOURCES = ctk-launch.c
ctk_launch_LDADD =				\
	libctk-3.la				\
	$(top_builddir)/cdk/libcdk-3.la		\
	$(CTK_DEP_LIBS)

if OS_WIN32

# Workaround for UAC silliness: programs with "update" in their name
# are believed to be installers and require elevated privileges to be
# used... Embed a manifest file into executable to tell Windows that
# ctk-update-icon-cache.exe doesn't require any special privileges.

CTK_UPDATE_ICON_CACHE_MANIFEST = ctk-update-icon-cache.exe.manifest
CTK_UPDATE_ICON_CACHE_RC = ctk-update-icon-cache.rc
CTK_UPDATE_ICON_CACHE_MANIFEST_OBJECT = ctk-update-icon-cache_manifest.o

$(CTK_UPDATE_ICON_CACHE_MANIFEST):
	(echo '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' ; \
	 echo '<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">' ; \
	 echo '  <assemblyIdentity version="1.0.0.0"' ; \
	 echo '     processorArchitecture="'$(EXE_MANIFEST_ARCHITECTURE)'"' ; \
	 echo '     name="ctk-update-icon-cache.exe"' ; \
	 echo '     type="win32"/>' ; \
	 echo '  <!-- Identify the application security requirements. -->' ; \
	 echo '  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">' ; \
	 echo '    <security>' ; \
	 echo '      <requestedPrivileges>' ; \
	 echo '        <requestedExecutionLevel' ; \
	 echo '          level="asInvoker"' ; \
	 echo '          uiAccess="false"/>' ; \
	 echo '        </requestedPrivileges>' ; \
	 echo '       </security>' ; \
	 echo '  </trustInfo>' ; \
	 echo '</assembly>' ) >$@

$(CTK_UPDATE_ICON_CACHE_RC):
	(echo -e '#include <winuser.h>\nCREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST '$(CTK_UPDATE_ICON_CACHE_MANIFEST)) >$@

$(CTK_UPDATE_ICON_CACHE_MANIFEST_OBJECT): $(CTK_UPDATE_ICON_CACHE_RC) $(CTK_UPDATE_ICON_CACHE_MANIFEST)
	$(WINDRES) --input $< --output $@ --output-format=coff

ctk_update_icon_cache_LDADD += $(CTK_UPDATE_ICON_CACHE_MANIFEST_OBJECT)

endif

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done

gsettings_SCHEMAS = \
	org.ctk.Settings.FileChooser.gschema.xml \
	org.ctk.Settings.ColorChooser.gschema.xml \
	org.ctk.Settings.EmojiChooser.gschema.xml \
	org.ctk.Settings.Debug.gschema.xml

@GSETTINGS_RULES@

EXTRA_DIST +=                   \
	$(resource_files)	\
	$(gesture_sources)	\
	$(theme_sources)	\
	$(gsettings_SCHEMAS)	\
	compose-parse.py	\
	emoji/convert-emoji.c	\
	emoji/emoji.json	\
	emoji/emoji1.json	\
	a11y/Makefile.inc	\
	deprecated/Makefile.inc	\
	glade/README.glade	\
	glade/ctk-private-widgets.xml \
	ctk-compose-lookaside.txt \
	inspector/Makefile.inc	\
	libctk3.manifest.in	\
	ctk-win32.rc.body.in	\
	ctk-win32.rc.body	\
	ctkwin32embed.h		\
	ctkwin32embedwidget.h	\
	ctkwin32embedwidget.c	\
	ctkprint-win32.h	\
	ctkprint-win32.c	\
	ctksearchenginetracker3.c \
	ctksearchenginetracker3.h \
	ctksearchenginequartz.h \
	ctkdbusinterfaces.xml	\
	ctk-keys.css.default	\
	ctk-keys.css.emacs	\
	ctk-keys.css.mac	\
	makefile.msc		\
	makefile.msc.in		\
	ctktypebuiltins.c.template \
	ctktypebuiltins.h.template \
	ctkprivatetypebuiltins.c.template \
	ctkprivatetypebuiltins.h.template \
	meson.build \
	gen-ctk-gresources-xml.py \
	gen-rc.py \
	gentypefuncs.py \
	a11y/meson.build \
	deprecated/meson.build \
	inspector/meson.build \
	theme/Adwaita/meson.build \
	theme/Adwaita/parse-sass.sh \
	theme/Adwaita/render-assets.sh \
	theme/HighContrast/assets.svg \
	theme/HighContrast/assets.txt \
	theme/HighContrast/meson.build \
	theme/HighContrast/parse-sass.sh \
	theme/HighContrast/render-assets.sh


-include $(top_srcdir)/git.mk
