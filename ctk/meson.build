subdir('deprecated')
subdir('a11y')
subdir('inspector')

ctk_cargs = [
  '-DCTK_COMPILATION',
  '-DG_LOG_DOMAIN="Ctk"',
  '-DCTK_PRINT_BACKEND_ENABLE_UNSUPPORTED',
  '-DCTK_BINARY_VERSION="@0@"'.format(ctk_binary_version),
  '-DCTK_HOST="@0@"'.format(host_machine.system()),
  '-DCTK_DATA_PREFIX="@0@"'.format(ctk_prefix),
  '-DCTK_LIBDIR="@0@"'.format(ctk_libdir),
  '-DCTK_LOCALEDIR="@0@"'.format(ctk_localedir),
  '-DCTK_DATADIR="@0@"'.format(ctk_datadir),
  '-DCTK_SYSCONFDIR="@0@"'.format(ctk_sysconfdir),
]

# List of sources to build the library from
ctk_sources = files(
  'ctkaction.c',
  'ctkactiongroup.c',
  'ctkactionmuxer.c',
  'ctkactionobserver.c',
  'ctkactionobservable.c',
  'ctkactionable.c',
  'ctkquery.c',
  'ctksearchbar.c',
  'ctksearchentry.c',
  'ctksearchengine.c',
  'ctksearchenginesimple.c',
  'ctksearchenginemodel.c',
  'fnmatch.c',
  'ctkaboutdialog.c',
  'ctkaccelgroup.c',
  'ctkaccellabel.c',
  'ctkaccelmap.c',
  'ctkaccessible.c',
  'ctkactionbar.c',
  'ctkactionhelper.c',
  'ctkadjustment.c',
  'ctkallocatedbitmask.c',
  'ctkappchooser.c',
  'ctkappchooserwidget.c',
  'ctkappchooserbutton.c',
  'ctkappchooserdialog.c',
  'ctkapplication.c',
  'ctkapplicationaccels.c',
  'ctkapplicationimpl.c',
  'ctkapplicationwindow.c',
  'ctkaspectframe.c',
  'ctkassistant.c',
  'ctkbbox.c',
  'ctkbin.c',
  'ctkbindings.c',
  'ctkbookmarksmanager.c',
  'ctkborder.c',
  'ctkbox.c',
  'ctkboxgadget.c',
  'ctkbuildable.c',
  'ctkbuilder.c',
  'ctkbuilderparser.c',
  'ctkbuilder-menus.c',
  'ctkbuiltinicon.c',
  'ctkbutton.c',
  'ctkcairoblur.c',
  'ctkcalendar.c',
  'ctkcellarea.c',
  'ctkcellareabox.c',
  'ctkcellareaboxcontext.c',
  'ctkcellareacontext.c',
  'ctkcelleditable.c',
  'ctkcelllayout.c',
  'ctkcellrenderer.c',
  'ctkcellrendereraccel.c',
  'ctkcellrenderercombo.c',
  'ctkcellrendererpixbuf.c',
  'ctkcellrendererprogress.c',
  'ctkcellrendererspin.c',
  'ctkcellrendererspinner.c',
  'ctkcellrenderertext.c',
  'ctkcellrenderertoggle.c',
  'ctkcellview.c',
  'ctkcheckbutton.c',
  'ctkcheckmenuitem.c',
  'ctkcolorbutton.c',
  'ctkcolorchooser.c',
  'ctkcolorchooserwidget.c',
  'ctkcolorchooserdialog.c',
  'ctkcoloreditor.c',
  'ctkcolorpicker.c',
  'ctkcolorpickerkwin.c',
  'ctkcolorpickerportal.c',
  'ctkcolorpickershell.c',
  'ctkcolorplane.c',
  'ctkcolorscale.c',
  'ctkcolorswatch.c',
  'ctkcolorutils.c',
  'ctkcombobox.c',
  'ctkcomboboxtext.c',
  'ctkcomposetable.c',
  'ctkcontainer.c',
  'ctkcssanimation.c',
  'ctkcssanimatedstyle.c',
  'ctkcssarrayvalue.c',
  'ctkcssbgsizevalue.c',
  'ctkcssbordervalue.c',
  'ctkcsscalcvalue.c',
  'ctkcsscolorvalue.c',
  'ctkcsscornervalue.c',
  'ctkcsscustomgadget.c',
  'ctkcsscustomproperty.c',
  'ctkcssdimensionvalue.c',
  'ctkcsseasevalue.c',
  'ctkcssenumvalue.c',
  'ctkcssenginevalue.c',
  'ctkcssgadget.c',
  'ctkcssiconthemevalue.c',
  'ctkcssimage.c',
  'ctkcssimagebuiltin.c',
  'ctkcssimagecrossfade.c',
  'ctkcssimagefallback.c',
  'ctkcssimagegradient.c',
  'ctkcssimageicontheme.c',
  'ctkcssimagelinear.c',
  'ctkcssimageradial.c',
  'ctkcssimagesurface.c',
  'ctkcssimageurl.c',
  'ctkcssimagerecolor.c',
  'ctkcssimagescaled.c',
  'ctkcssimagevalue.c',
  'ctkcssimagewin32.c',
  'ctkcssinheritvalue.c',
  'ctkcssinitialvalue.c',
  'ctkcsskeyframes.c',
  'ctkcsslookup.c',
  'ctkcssmatcher.c',
  'ctkcssnode.c',
  'ctkcssnodedeclaration.c',
  'ctkcssnodestylecache.c',
  'ctkcssnumbervalue.c',
  'ctkcsspalettevalue.c',
  'ctkcssparser.c',
  'ctkcsspathnode.c',
  'ctkcsspositionvalue.c',
  'ctkcssprovider.c',
  'ctkcssrepeatvalue.c',
  'ctkcssrgbavalue.c',
  'ctkcsssection.c',
  'ctkcssselector.c',
  'ctkcssstringvalue.c',
  'ctkcssstyle.c',
  'ctkcssstylechange.c',
  'ctkcssshadowsvalue.c',
  'ctkcssshadowvalue.c',
  'ctkcssshorthandproperty.c',
  'ctkcssshorthandpropertyimpl.c',
  'ctkcssstaticstyle.c',
  'ctkcssstylefuncs.c',
  'ctkcssstyleproperty.c',
  'ctkcssstylepropertyimpl.c',
  'ctkcsstransformvalue.c',
  'ctkcsstransientnode.c',
  'ctkcsstransition.c',
  'ctkcsstypedvalue.c',
  'ctkcssunsetvalue.c',
  'ctkcsstypes.c',
  'ctkcssvalue.c',
  'ctkcsswidgetnode.c',
  'ctkcsswin32sizevalue.c',
  'ctkdialog.c',
  'ctkdragsource.c',
  'ctkdrawingarea.c',
  'ctkeditable.c',
  'ctkemojichooser.c',
  'ctkemojicompletion.c',
  'ctkentry.c',
  'ctkentrybuffer.c',
  'ctkentrycompletion.c',
  'ctkeventbox.c',
  'ctkeventcontroller.c',
  'ctkeventcontrollerkey.c',
  'ctkeventcontrollermotion.c',
  'ctkeventcontrollerscroll.c',
  'ctkexpander.c',
  'ctkfilechooser.c',
  'ctkfilechooserbutton.c',
  'ctkfilechooserdialog.c',
  'ctkfilechooserembed.c',
  'ctkfilechooserentry.c',
  'ctkfilechoosernative.c',
  'ctkfilechoosernativeportal.c',
  'ctkfilechooserutils.c',
  'ctkfilechooserwidget.c',
  'ctkfilefilter.c',
  'ctkfilesystem.c',
  'ctkfilesystemmodel.c',
  'ctkfixed.c',
  'ctkflowbox.c',
  'ctkfontbutton.c',
  'ctkfontchooser.c',
  'ctkfontchooserdialog.c',
  'ctkfontchooserutils.c',
  'ctkfontchooserwidget.c',
  'ctkframe.c',
  'ctkgladecatalog.c',
  'ctkgesture.c',
  'ctkgesturedrag.c',
  'ctkgesturelongpress.c',
  'ctkgesturemultipress.c',
  'ctkgesturepan.c',
  'ctkgesturerotate.c',
  'ctkgesturesingle.c',
  'ctkgesturestylus.c',
  'ctkgestureswipe.c',
  'ctkgesturezoom.c',
  'ctkglarea.c',
  'ctkgrid.c',
  'ctkheaderbar.c',
  'ctkhsla.c',
  'ctkicon.c',
  'ctkiconcache.c',
  'ctkiconcachevalidator.c',
  'ctkiconfactory.c',
  'ctkiconhelper.c',
  'ctkicontheme.c',
  'ctkiconview.c',
  'ctkimage.c',
  'ctkimagedefinition.c',
  'ctkimcontext.c',
  'ctkimcontextsimple.c',
  'ctkimmodule.c',
  'ctkimmulticontext.c',
  'ctkinfobar.c',
  'ctkinvisible.c',
  'ctkkeyhash.c',
  'ctkkineticscrolling.c',
  'ctklabel.c',
  'ctklayout.c',
  'ctklevelbar.c',
  'ctklinkbutton.c',
  'ctklistbox.c',
  'ctkliststore.c',
  'ctklockbutton.c',
  'ctkmain.c',
  'ctkmagnifier.c',
  'ctkmenu.c',
  'ctkmenubar.c',
  'ctkmenubutton.c',
  'ctkmenuitem.c',
  'ctkmenusectionbox.c',
  'ctkmenushell.c',
  'ctkmenutracker.c',
  'ctkmenutrackeritem.c',
  'ctkmenutoolbutton.c',
  'ctkmessagedialog.c',
  'ctkmnemonichash.c',
  'ctkmodelmenuitem.c',
  'ctkmodelbutton.c',
  'ctkmodifierstyle.c',
  'ctkmodules.c',
  'ctkmountoperation.c',
  'ctknativedialog.c',
  'ctknotebook.c',
  'ctkoffscreenwindow.c',
  'ctkorientable.c',
  'ctkoverlay.c',
  'ctkpadcontroller.c',
  'ctkpagesetup.c',
  'ctkpaned.c',
  'ctkpango.c',
  'ctkpapersize.c',
  'ctkpathbar.c',
  'ctkplacessidebar.c',
  'ctkplacesview.c',
  'ctkplacesviewrow.c',
  'ctkprintcontext.c',
  'ctkprintoperation.c',
  'ctkprintoperationpreview.c',
  'ctkprintsettings.c',
  'ctkprintutils.c',
  'ctkprivate.c',
  'ctkprogressbar.c',
  'ctkprogresstracker.c',
  'ctkpixelcache.c',
  'ctkpopover.c',
  'ctkpopovermenu.c',
  'ctkradiobutton.c',
  'ctkradiomenuitem.c',
  'ctkradiotoolbutton.c',
  'ctkrange.c',
  'ctkrbtree.c',
  'ctkrc.c',
  'ctkrecentchooserdefault.c',
  'ctkrecentchooserdialog.c',
  'ctkrecentchoosermenu.c',
  'ctkrecentchooserwidget.c',
  'ctkrecentchooserutils.c',
  'ctkrecentchooser.c',
  'ctkrecentfilter.c',
  'ctkrecentmanager.c',
  'ctkrender.c',
  'ctkrenderbackground.c',
  'ctkrenderborder.c',
  'ctkrendericon.c',
  'ctkrevealer.c',
  'ctkroundedbox.c',
  'ctkscale.c',
  'ctkscalebutton.c',
  'ctkscrollable.c',
  'ctkscrollbar.c',
  'ctkscrolledwindow.c',
  'ctkselection.c',
  'ctkseparator.c',
  'ctkseparatormenuitem.c',
  'ctkseparatortoolitem.c',
  'ctksettings.c',
  'ctkshortcutsgroup.c',
  'ctkshortcutlabel.c',
  'ctkshortcutsshortcut.c',
  'ctkshortcutssection.c',
  'ctkshortcutswindow.c',
  'ctksidebarrow.c',
  'ctksizegroup.c',
  'ctksizerequest.c',
  'ctksizerequestcache.c',
  'ctkshow.c',
  'ctkstacksidebar.c',
  'ctkspinbutton.c',
  'ctkspinner.c',
  'ctkstack.c',
  'ctkstackswitcher.c',
  'ctkstatusbar.c',
  'ctkstock.c',
  'ctkstyleanimation.c',
  'ctkstylecascade.c',
  'ctkstylecontext.c',
  'ctkstyleproperty.c',
  'ctkstyleprovider.c',
  'ctkstyleproviderprivate.c',
  'ctkstyle.c',
  'ctkswitch.c',
  'ctktestutils.c',
  'ctktextattributes.c',
  'ctktextbtree.c',
  'ctktextbuffer.c',
  'ctktextbufferrichtext.c',
  'ctktextbufferserialize.c',
  'ctktextchild.c',
  'ctktextdisplay.c',
  'ctktexthandle.c',
  'ctktextiter.c',
  'ctktextlayout.c',
  'ctktextmark.c',
  'ctktextsegment.c',
  'ctktexttag.c',
  'ctktexttagtable.c',
  'ctktexttypes.c',
  'ctktextutil.c',
  'ctktextview.c',
  'ctktoggleaction.c',
  'ctktogglebutton.c',
  'ctktoggletoolbutton.c',
  'ctktoolbar.c',
  'ctktoolbutton.c',
  'ctktoolitem.c',
  'ctktoolitemgroup.c',
  'ctktoolpalette.c',
  'ctktoolshell.c',
  'ctktooltip.c',
  'ctktooltipwindow.c',
  'ctktrashmonitor.c',
  'ctktreedatalist.c',
  'ctktreednd.c',
  'ctktreemenu.c',
  'ctktreemodel.c',
  'ctktreemodelfilter.c',
  'ctktreemodelsort.c',
  'ctktreeselection.c',
  'ctktreesortable.c',
  'ctktreestore.c',
  'ctktreeview.c',
  'ctktreeviewcolumn.c',
  'ctkutils.c',
  'ctkvolumebutton.c',
  'ctkviewport.c',
  'ctkwidget.c',
  'ctkwidgetpath.c',
  'ctkwindow.c',
  'ctkwindowgroup.c',
  'ctkwin32draw.c',
  'ctkwin32theme.c',
  'gdkpixbufutils.c',
  'language-names.c',
  'script-names.c',
)

ctk_private_type_headers = files(
  'ctkcsstypesprivate.h',
  'ctktexthandleprivate.h',
)

ctk_gir_public_headers = files(
  'ctk.h',
  'ctk-autocleanups.h',
  'ctkx-autocleanups.h',
  'ctk-a11y.h',
  'ctkaboutdialog.h',
  'ctkaccelgroup.h',
  'ctkaccellabel.h',
  'ctkaccelmap.h',
  'ctkaccessible.h',
  'ctkaction.h',
  'ctkactiongroup.h',
  'ctkactionable.h',
  'ctkactionbar.h',
  'ctkadjustment.h',
  'ctkappchooser.h',
  'ctkappchooserbutton.h',
  'ctkappchooserdialog.h',
  'ctkappchooserwidget.h',
  'ctkapplication.h',
  'ctkapplicationwindow.h',
  'ctkaspectframe.h',
  'ctkassistant.h',
  'ctkbbox.h',
  'ctkbin.h',
  'ctkbindings.h',
  'ctkborder.h',
  'ctkbox.h',
  'ctkbuilder.h',
  'ctkbuildable.h',
  'ctkbutton.h',
  'ctkcalendar.h',
  'ctkcellarea.h',
  'ctkcellareacontext.h',
  'ctkcellareabox.h',
  'ctkcelleditable.h',
  'ctkcelllayout.h',
  'ctkcellrenderer.h',
  'ctkcellrendereraccel.h',
  'ctkcellrenderercombo.h',
  'ctkcellrendererpixbuf.h',
  'ctkcellrendererprogress.h',
  'ctkcellrendererspin.h',
  'ctkcellrendererspinner.h',
  'ctkcellrenderertext.h',
  'ctkcellrenderertoggle.h',
  'ctkcellview.h',
  'ctkcheckbutton.h',
  'ctkcheckmenuitem.h',
  'ctkclipboard.h',
  'ctkcolorbutton.h',
  'ctkcolorchooser.h',
  'ctkcolorchooserwidget.h',
  'ctkcolorchooserdialog.h',
  'ctkcolorutils.h',
  'ctkcombobox.h',
  'ctkcomboboxtext.h',
  'ctkcontainer.h',
  'ctkcssprovider.h',
  'ctkcsssection.h',
  'ctkdebug.h',
  'ctkdialog.h',
  'ctkdnd.h',
  'ctkdragdest.h',
  'ctkdragsource.h',
  'ctkdrawingarea.h',
  'ctkeditable.h',
  'ctkentry.h',
  'ctkentrybuffer.h',
  'ctkentrycompletion.h',
  'ctkenums.h',
  'ctkeventbox.h',
  'ctkeventcontroller.h',
  'ctkeventcontrollerkey.h',
  'ctkeventcontrollermotion.h',
  'ctkeventcontrollerscroll.h',
  'ctkexpander.h',
  'ctkfilechooser.h',
  'ctkfilechooserbutton.h',
  'ctkfilechooserdialog.h',
  'ctkfilechoosernative.h',
  'ctkfilechooserwidget.h',
  'ctkfilefilter.h',
  'ctkfixed.h',
  'ctkflowbox.h',
  'ctkfontbutton.h',
  'ctkfontchooser.h',
  'ctkfontchooserdialog.h',
  'ctkfontchooserwidget.h',
  'ctkframe.h',
  'ctkgesture.h',
  'ctkgesturedrag.h',
  'ctkgesturelongpress.h',
  'ctkgesturemultipress.h',
  'ctkgesturepan.h',
  'ctkgesturerotate.h',
  'ctkgesturesingle.h',
  'ctkgesturestylus.h',
  'ctkgestureswipe.h',
  'ctkgesturezoom.h',
  'ctkglarea.h',
  'ctkgrid.h',
  'ctkheaderbar.h',
  'ctkiconfactory.h',
  'ctkicontheme.h',
  'ctkiconview.h',
  'ctkimage.h',
  'ctkimcontext.h',
  'ctkimcontextinfo.h',
  'ctkimcontextsimple.h',
  'ctkimmodule.h',
  'ctkimmulticontext.h',
  'ctkinfobar.h',
  'ctkinvisible.h',
  'ctklabel.h',
  'ctklayout.h',
  'ctklevelbar.h',
  'ctklinkbutton.h',
  'ctklistbox.h',
  'ctkliststore.h',
  'ctklockbutton.h',
  'ctkmain.h',
  'ctkmenu.h',
  'ctkmenubar.h',
  'ctkmenubutton.h',
  'ctkmenuitem.h',
  'ctkmenushell.h',
  'ctkmenutoolbutton.h',
  'ctkmessagedialog.h',
  'ctkmodelbutton.h',
  'ctkmodules.h',
  'ctkmountoperation.h',
  'ctknativedialog.h',
  'ctknotebook.h',
  'ctkoffscreenwindow.h',
  'ctkorientable.h',
  'ctkoverlay.h',
  'ctkpadcontroller.h',
  'ctkpagesetup.h',
  'ctkpaned.h',
  'ctkpapersize.h',
  'ctkplacessidebar.h',
  'ctkplug.h',
  'ctkpopover.h',
  'ctkpopovermenu.h',
  'ctkprintcontext.h',
  'ctkprintoperation.h',
  'ctkprintoperationpreview.h',
  'ctkprintsettings.h',
  'ctkprogressbar.h',
  'ctkradiobutton.h',
  'ctkradiomenuitem.h',
  'ctkradiotoolbutton.h',
  'ctkrange.h',
  'ctkrc.h',
  'ctkrecentchooser.h',
  'ctkrecentchooserdialog.h',
  'ctkrecentchoosermenu.h',
  'ctkrecentchooserwidget.h',
  'ctkrecentfilter.h',
  'ctkrecentmanager.h',
  'ctkrender.h',
  'ctkrevealer.h',
  'ctkscale.h',
  'ctkscalebutton.h',
  'ctkscrollable.h',
  'ctkscrollbar.h',
  'ctkscrolledwindow.h',
  'ctksearchbar.h',
  'ctksearchentry.h',
  'ctkselection.h',
  'ctkseparator.h',
  'ctkseparatormenuitem.h',
  'ctkseparatortoolitem.h',
  'ctksettings.h',
  'ctkshortcutlabel.h',
  'ctkshortcutsgroup.h',
  'ctkshortcutssection.h',
  'ctkshortcutsshortcut.h',
  'ctkshortcutswindow.h',
  'ctkshow.h',
  'ctkstacksidebar.h',
  'ctksizegroup.h',
  'ctksizerequest.h',
  'ctksocket.h',
  'ctkspinbutton.h',
  'ctkspinner.h',
  'ctkstack.h',
  'ctkstackswitcher.h',
  'ctkstatusbar.h',
  'ctkstock.h',
  'ctkstylecontext.h',
  'ctkstyleprovider.h',
  'ctkstyle.h',
  'ctkswitch.h',
  'ctktestutils.h',
  'ctktextattributes.h',
  'ctktextbuffer.h',
  'ctktextbufferrichtext.h',
  'ctktextchild.h',
  'ctktextiter.h',
  'ctktextmark.h',
  'ctktexttag.h',
  'ctktexttagtable.h',
  'ctktextview.h',
  'ctktoggleaction.h',
  'ctktogglebutton.h',
  'ctktoggletoolbutton.h',
  'ctktoolbar.h',
  'ctktoolbutton.h',
  'ctktoolitem.h',
  'ctktoolitemgroup.h',
  'ctktoolpalette.h',
  'ctktoolshell.h',
  'ctktooltip.h',
  'ctktreednd.h',
  'ctktreemodel.h',
  'ctktreemodelfilter.h',
  'ctktreemodelsort.h',
  'ctktreeselection.h',
  'ctktreesortable.h',
  'ctktreestore.h',
  'ctktreeview.h',
  'ctktreeviewcolumn.h',
  'ctktypes.h',
  'ctkviewport.h',
  'ctkvolumebutton.h',
  'ctkwidget.h',
  'ctkwidgetpath.h',
  'ctkwindow.h',
  'ctkwindowgroup.h',
)

ctk_nogir_public_headers = files(
  'ctkx.h',
  'ctktextdisplay.h',
)

# Installed headers without compatibility guarantees that are not included in ctk/ctk.h
ctk_semi_private_headers = files('ctktextlayout.h')
ctk_public_headers = ctk_gir_public_headers + ctk_nogir_public_headers
install_headers(ctk_public_headers + ctk_semi_private_headers, subdir: 'ctk-3.0/ctk')

if os_unix
  ctk_unix_print_headers = [
    'ctkpagesetupunixdialog.h',
    'ctkprintunixdialog.h',
    'ctkprinter.h',
    'ctkprintjob.h',
    'ctkunixprint-autocleanups.h',
    'ctkunixprint.h',
  ]
  install_headers(ctk_unix_print_headers, subdir: 'ctk-3.0/unix-print/ctk')
endif

ctk_unix_sources = files(
  'ctkcustompaperunixdialog.c',
  'ctkpagesetupunixdialog.c',
  'ctkprinter.c',
  'ctkprinteroption.c',
  'ctkprinteroptionset.c',
  'ctkprinteroptionwidget.c',
  'ctkprintjob.c',
  'ctkprintoperation-unix.c',
  'ctkprintoperation-portal.c',
  'ctkprintunixdialog.c',
  'ctkprintbackend.c',
  'ctksearchenginetracker.c',
)

if os_unix and tracker3_enabled
  ctk_unix_sources += 'ctksearchenginetracker3.c'
endif

if os_unix
  ctk_sources += ctk_unix_sources
endif

ctk_win32_sources = files(
  'ctkprint-win32.c',
	'ctkprintoperation-win32.c',
	'ctkfilechoosernativewin32.c',
	'ctkwin32.c',
)

if os_win32
  # FIXME these sources should not be added to generate_gir?
  ctk_sources += ctk_win32_sources
endif

gen_ctk_gresources_xml = find_program('gen-ctk-gresources-xml.py')
ctk_gresources_xml = configure_file(output: 'ctk.gresources.xml',
                                    command: [
                                      gen_ctk_gresources_xml,
                                      meson.current_source_dir(),
                                      '@OUTPUT@'
                                    ])

# Re-build the theme files if sassc is available
theme_deps = []
sassc = find_program('sassc', required: false)
if sassc.found()
  sassc_opts = [ '-a', '-M', '-t', 'compact' ]

  subdir('theme/Adwaita')
  subdir('theme/HighContrast')
endif

ctkresources = gnome.compile_resources('ctkresources',
                                       ctk_gresources_xml,
                                       dependencies: theme_deps,
                                       source_dir: [
                                         # List in order of preference
                                         meson.current_build_dir(),
                                         meson.current_source_dir(),
                                       ],
                                       c_name: '_ctk',
                                       extra_args: '--manual-register')

ctk_use_x11_sources = files([
  'ctkplug.c',
  'ctksocket.c',
  'ctkxembed.c',
  'deprecated/ctktrayicon-x11.c',
  'ctkapplication-x11.c',
  'ctkmountoperation-x11.c',
])

ctk_use_wayland_sources = files([
  'ctkapplication-wayland.c'
])

if quartz_enabled
  ctk_sources += files(
    'ctkclipboard-quartz.c',
    'ctkdnd-quartz.c',
  )
else
  ctk_sources += files(
    'ctkclipboard.c',
    'ctkdnd.c',
    'ctkdragdest.c',
  )
endif

ctk_use_wayland_or_x11_sources = files( 'ctkapplication-dbus.c')

ctk_use_win32_sources = files(
	'ctkwin32embed.c',
	'ctkwin32embedwidget.c',
	'ctkmountoperation-stub.c',
)

ctk_use_quartz_sources = files(
  'ctksearchenginequartz.c',
  'ctkmountoperation-stub.c',
  'ctkapplication-quartz.c',
  'ctkapplication-quartz-menu.c',
  'ctkfilechoosernativequartz.c',
  'ctkquartz.c',
)

ctk_stub_sources = files('ctkmountoperation-stub.c')

ctk_dbus_src = gnome.gdbus_codegen('ctkdbusgenerated', 'ctkdbusinterfaces.xml',
                                   interface_prefix: 'org.Ctk.',
                                   namespace: '_Ctk')

ctkmarshalers = gnome.genmarshal('ctkmarshalers',
                                 sources: 'ctkmarshalers.list',
                                 prefix: '_ctk_marshal',
                                 valist_marshallers: true)

ctkmarshal_h = ctkmarshalers[1]

ctktypebuiltins = gnome.mkenums('ctktypebuiltins',
                                sources: ctk_public_headers + ctk_deprecated_headers + a11y_headers,
                                c_template: 'ctktypebuiltins.c.template',
                                h_template: 'ctktypebuiltins.h.template',
                                install_dir: join_paths(ctk_includedir, 'ctk-3.0/ctk'),
                                install_header: true)

ctktypebuiltins_h = ctktypebuiltins[1]

ctkprivatetypebuiltins = gnome.mkenums('ctkprivatetypebuiltins',
                                       sources: ctk_private_type_headers,
                                       c_template: 'ctkprivatetypebuiltins.c.template',
                                       h_template: 'ctkprivatetypebuiltins.h.template')

ctkprivatetypebuiltins_h = ctkprivatetypebuiltins[1]

# Generate ctktypefuncs.inc
typefuncs = custom_target('ctktypefuncs.inc',
                          depends: [ cdkenum_h ],
                          output: 'ctktypefuncs.inc',
                          input: cdk_public_headers +
                            ctk_public_headers +
                            ctk_deprecated_headers +
                            [ ctktypebuiltins_h, cdkenum_h ],
                          command: [
                            find_program('gentypefuncs.py'),
                            '@OUTPUT@',
                            '@INPUT@',
                          ],
                          install: false)


ctkversion_cdata = configuration_data()
ctkversion_cdata.set('CTK_MAJOR_VERSION', ctk_major_version)
ctkversion_cdata.set('CTK_MINOR_VERSION', ctk_minor_version)
ctkversion_cdata.set('CTK_MICRO_VERSION', ctk_micro_version)
ctkversion_cdata.set('CTK_BINARY_AGE', ctk_binary_age)
ctkversion_cdata.set('CTK_INTERFACE_AGE', ctk_interface_age)
ctkversion_cdata.set('CTK_API_VERSION', ctk_api_version)
ctkversion_cdata.set('CTK_BINARY_VERSION', ctk_binary_version)
ctkversion_cdata.set('CTK_VERSION', meson.project_version())
ctkversion_cdata.set('EXE_MANIFEST_ARCHITECTURE', '*')
ctkversion_cdata.set('LT_CURRENT_MINUS_AGE', ctk_soversion)

ctkversion = configure_file(input: 'ctkversion.h.in',
                            output: 'ctkversion.h',
                            configuration: ctkversion_cdata,
                            install_dir: join_paths(ctk_includedir, 'ctk-3.0/ctk'))

ctk_gen_headers = [
  ctkmarshalers,
  ctkprivatetypebuiltins,
  ctktypebuiltins,
  ctkversion,
]

ctk_sources += [
  ctk_dbus_src,
  ctk_deprecated_sources,
  inspector_sources,
  a11y_sources,
  ctkresources,
  ctkmarshalers,
  ctkprivatetypebuiltins,
  ctktypebuiltins,
]

ctk_deps = [
  gmodule_dep,
  glib_dep,
  gobject_dep,
  platform_gio_dep,
  pangocairo_dep,
  pango_dep,
  pangoft_dep,
  harfbuzz_dep,
  fribidi_dep,
  cairogobj_dep,
  cairo_dep,
  fontconfig_dep,
  pixbuf_dep,
  atk_dep,
  epoxy_dep,
  libm,
]

if x11_enabled
  x11_data_prefix = dependency('x11').get_pkgconfig_variable('prefix')

  ctk_cargs += [ '-DX11_DATA_PREFIX="@0@"'.format(x11_data_prefix), ]
  ctk_sources += ctk_use_x11_sources
  ctk_deps += [ xi_dep, x11_dep, ]
endif

if wayland_enabled
  ctk_sources += ctk_use_wayland_sources
endif

if win32_enabled
  ctk_cargs += []
  ctk_sources += ctk_use_win32_sources
  ctk_deps += [ giowin32_dep, pangowin32_dep ]

  ctk_rc_body = configure_file(
    input: 'ctk-win32.rc.body.in',
    output: 'ctk-win32.rc.body',
	configuration: ctkversion_cdata,
  )
  ctk_rc_manifest = configure_file(
    input: 'libctk3.manifest.in',
    output: 'libctk3.manifest',
    configuration: ctkversion_cdata,
  )

  # Unfortunately, an extra directive in the .rc file is required for earlier
  # Visual Studio for embedding manifests via .rc files (2010 [v16] and earlier)
  # using ISOLATIONAWARE_MANIFEST_RESOURCE_ID to really work.  Somehow for
  # Visual Studio 2008 builds, this does not yet work.
  old_msvc = false

  if cc.get_id() == 'msvc' and cc.version().split('.')[0].to_int() < 17
    old_msvc = true
  endif

  ctk_rc = custom_target(
             'ctk.rc',
             output: 'ctk.rc',
             input: ctk_rc_body,
             command: [find_program('gen-rc.py'),
               '@OUTPUT@',
               '@INPUT@',
               old_msvc ? '1' : ''
             ],
             install: false,
           )
  ctk_res = import('windows').compile_resources(ctk_rc)
  ctk_sources += ctk_res
else
  ctk_deps += [ atkbridge_dep, ]
endif

if quartz_enabled
  ctk_sources += ctk_use_quartz_sources
  ctk_cargs += ['-xobjective-c']
endif

# So we don't add these twice
if x11_enabled or wayland_enabled
  ctk_sources += ctk_use_wayland_or_x11_sources
  ctk_deps += pangoft_dep
endif

if not (x11_enabled or win32_enabled or quartz_enabled)
  ctk_sources += ctk_stub_sources
endif

if cloudproviders_enabled
  ctk_deps += cloudproviders_dep
endif

if os_unix and tracker3_enabled
  ctk_deps += tracker3_dep
endif

# Unconditional. If libintl isn't found,
# the object just does nothing being in the deplist
ctk_deps += libintl_dep

ctk_settings_schemas = [
  'org.ctk.Settings.FileChooser.gschema.xml',
  'org.ctk.Settings.ColorChooser.gschema.xml',
  'org.ctk.Settings.EmojiChooser.gschema.xml',
  'org.ctk.Settings.Debug.gschema.xml',
]
install_data(ctk_settings_schemas, install_dir: ctk_schemasdir)
gnome.compile_schemas(depend_files: files(ctk_settings_schemas),
                      build_by_default: true)
ctk_schema_build_dir = meson.current_build_dir()

# Check for more things
if cc.has_header('langinfo.h')
  foreach nl_enum: [ '_NL_MEASUREMENT_MEASUREMENT',
                     '_NL_PAPER_HEIGHT',
                     '_NL_PAPER_WIDTH',
                     '_NL_TIME_FIRST_WEEKDAY', ]
    cdata.set('HAVE_' + nl_enum, cc.has_header_symbol('langinfo.h', nl_enum) ? 1 : false)
  endforeach
endif

ctk_dep_sources = [ctkversion, ctktypebuiltins_h]

# Static immodules
ctk_included_im_deps = []
foreach l: immodules
  name = l[0]
  sources = l[1]
  cond = l.get(2, true)
  cflags = l.get(3, [])

  if cond and builtin_immodules
    ctk_cargs += ['-DINCLUDE_IM_@0@'.format(name.underscorify())]
    mod = static_library('staticimmodule-@0@'.format(name),
                         sources + ctk_dep_sources,
                         dependencies: ctk_deps + [libcdk_dep] ,
                         c_args: ctk_cargs + common_cflags + cflags,
                         include_directories: [confinc, cdkinc, ctkinc])

    ctk_included_im_deps += declare_dependency(link_with: mod)
  endif
endforeach

# Library
libctk = library('ctk-3',
                 soversion: ctk_soversion,
                 version: ctk_library_version,
                 sources: [typefuncs, ctk_sources, ctkmarshal_h, ctkprivatetypebuiltins_h],
                 c_args: ctk_cargs + common_cflags,
                 include_directories: [confinc, cdkinc, ctkinc],
                 dependencies: ctk_deps + [libcdk_dep] + ctk_included_im_deps,
                 link_with: libcdk,
                 link_args: common_ldflags,
                 darwin_versions : ctk_osxversions,
                 install: true)

# Introspection
if build_gir
  gir_args = ['--quiet']

  ctk_introspection_sources = [
    ctk_gir_public_headers,
    ctk_deprecated_headers,
    ctk_sources,
    a11y_headers,
    a11y_sources,
    ctktypebuiltins_h,
    ctkversion,
  ]

  ctk_gir_includes = [cdk_gir[0], 'Atk-1.0']
  ctk_gir_extra_args = gir_args + ['-DCTK_COMPILATION',
                                   '--c-include=ctk/ctk-a11y.h']
  if x11_enabled
    ctk_gir_includes += ['xlib-2.0']
    ctk_gir_extra_args += ['--c-include=ctk/ctkx.h']
  endif

  ctk_gir = gnome.generate_gir(libctk,
                               sources: ctk_introspection_sources,
                               namespace: 'Ctk',
                               nsversion: ctk_api_version,
                               identifier_prefix: 'Ctk',
                               symbol_prefix: 'ctk',
                               export_packages: 'ctk+-3.0',
                               includes: ctk_gir_includes,
                               header: 'ctk/ctk.h',
                               install: true,
                               dependencies: cdk_gir_dep,
                               extra_args: ctk_gir_extra_args)
  ctk_dep_sources += ctk_gir
endif

libctk_dep = declare_dependency(sources: ctk_dep_sources,
                                include_directories: [confinc, ctkinc],
                                dependencies: ctk_deps + [libcdk_dep],
                                link_with: libctk,
                                link_args: common_ldflags)

if quartz_enabled
  install_data(['ctk-keys.css.mac'],
               install_dir:  join_paths(get_option('datadir'), 'themes/Mac/ctk-3.0'),
               rename: ['ctk-keys.css'])
endif

install_data(['ctk-keys.css.default'],
             install_dir:  join_paths(get_option('datadir'), 'themes/Default/ctk-3.0'),
             rename: ['ctk-keys.css'])

install_data(['ctk-keys.css.emacs'],
             install_dir:  join_paths(get_option('datadir'), 'themes/Emacs/ctk-3.0'),
             rename: ['ctk-keys.css'])

# Data to install
install_data('ctkbuilder.rng',
             install_dir: join_paths(ctk_datadir, 'ctk-3.0'))

install_data('ctkbuilder.loc', 'ctkbuilder.its',
             install_dir: join_paths(ctk_datadir, 'gettext/its'))

####### Utils ######
ctk_query_settings = executable(
  'ctk-query-settings',
  'ctk-query-settings.c',
  c_args: ctk_cargs,
  dependencies: libctk_dep,
  install: true
)

ctk_builder_tool = executable(
  'ctk-builder-tool',
  'ctk-builder-tool.c',
  c_args: ctk_cargs,
  dependencies: libctk_dep,
  install: true
)

ctk_update_icon_cache = executable(
  'ctk-update-icon-cache',
  'updateiconcache.c',
  c_args: ctk_cargs,
  dependencies: libctk_dep,
  install: true
)

 ctk_query_immodules = executable(
  'ctk-query-immodules-3.0',
  'queryimmodules.c',
  'ctkutils.c',
  c_args: ctk_cargs,
  dependencies: libctk_dep,
  install: true
)

ctk_encode_symbolic_svg = executable(
  'ctk-encode-symbolic-svg',
  'encodesymbolic.c',
  c_args: ctk_cargs,
  dependencies: libctk_dep,
  install: true
)

ctk_launch = executable(
  'ctk-launch',
  'ctk-launch.c',
  c_args: ctk_cargs,
  dependencies: libctk_dep,
  install: true
)
