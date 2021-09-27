/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_H__
#define __CTK_H__

#define __CTK_H_INSIDE__

#include <cdk/cdk.h>
#include <ctk/ctkaboutdialog.h>
#include <ctk/ctkaccelgroup.h>
#include <ctk/ctkaccellabel.h>
#include <ctk/ctkaccelmap.h>
#include <ctk/ctkaccessible.h>
#include <ctk/ctkaction.h>
#include <ctk/ctkactiongroup.h>
#include <ctk/ctkactionable.h>
#include <ctk/ctkactionbar.h>
#include <ctk/ctkadjustment.h>
#include <ctk/ctkappchooser.h>
#include <ctk/ctkappchooserdialog.h>
#include <ctk/ctkappchooserwidget.h>
#include <ctk/ctkappchooserbutton.h>
#include <ctk/ctkapplication.h>
#include <ctk/ctkapplicationwindow.h>
#include <ctk/ctkaspectframe.h>
#include <ctk/ctkassistant.h>
#include <ctk/ctkbbox.h>
#include <ctk/ctkbin.h>
#include <ctk/ctkbindings.h>
#include <ctk/ctkborder.h>
#include <ctk/ctkbox.h>
#include <ctk/ctkbuildable.h>
#include <ctk/ctkbuilder.h>
#include <ctk/ctkbutton.h>
#include <ctk/ctkcalendar.h>
#include <ctk/ctkcellarea.h>
#include <ctk/ctkcellareabox.h>
#include <ctk/ctkcellareacontext.h>
#include <ctk/ctkcelleditable.h>
#include <ctk/ctkcelllayout.h>
#include <ctk/ctkcellrenderer.h>
#include <ctk/ctkcellrendereraccel.h>
#include <ctk/ctkcellrenderercombo.h>
#include <ctk/ctkcellrendererpixbuf.h>
#include <ctk/ctkcellrendererprogress.h>
#include <ctk/ctkcellrendererspin.h>
#include <ctk/ctkcellrendererspinner.h>
#include <ctk/ctkcellrenderertext.h>
#include <ctk/ctkcellrenderertoggle.h>
#include <ctk/ctkcellview.h>
#include <ctk/ctkcheckbutton.h>
#include <ctk/ctkcheckmenuitem.h>
#include <ctk/ctkclipboard.h>
#include <ctk/ctkcolorbutton.h>
#include <ctk/ctkcolorchooser.h>
#include <ctk/ctkcolorchooserdialog.h>
#include <ctk/ctkcolorchooserwidget.h>
#include <ctk/ctkcolorutils.h>
#include <ctk/ctkcombobox.h>
#include <ctk/ctkcomboboxtext.h>
#include <ctk/ctkcontainer.h>
#include <ctk/ctkcssprovider.h>
#include <ctk/ctkcsssection.h>
#include <ctk/ctkdebug.h>
#include <ctk/ctkdialog.h>
#include <ctk/ctkdnd.h>
#include <ctk/ctkdragdest.h>
#include <ctk/ctkdragsource.h>
#include <ctk/ctkdrawingarea.h>
#include <ctk/ctkeditable.h>
#include <ctk/ctkentry.h>
#include <ctk/ctkentrybuffer.h>
#include <ctk/ctkentrycompletion.h>
#include <ctk/ctkenums.h>
#include <ctk/ctkeventbox.h>
#include <ctk/ctkeventcontroller.h>
#include <ctk/ctkeventcontrollerkey.h>
#include <ctk/ctkeventcontrollermotion.h>
#include <ctk/ctkeventcontrollerscroll.h>
#include <ctk/ctkexpander.h>
#include <ctk/ctkfixed.h>
#include <ctk/ctkfilechooser.h>
#include <ctk/ctkfilechooserbutton.h>
#include <ctk/ctkfilechooserdialog.h>
#include <ctk/ctkfilechoosernative.h>
#include <ctk/ctkfilechooserwidget.h>
#include <ctk/ctkfilefilter.h>
#include <ctk/ctkflowbox.h>
#include <ctk/ctkfontbutton.h>
#include <ctk/ctkfontchooser.h>
#include <ctk/ctkfontchooserdialog.h>
#include <ctk/ctkfontchooserwidget.h>
#include <ctk/ctkframe.h>
#include <ctk/ctkgesture.h>
#include <ctk/ctkgesturedrag.h>
#include <ctk/ctkgesturelongpress.h>
#include <ctk/ctkgesturemultipress.h>
#include <ctk/ctkgesturepan.h>
#include <ctk/ctkgesturerotate.h>
#include <ctk/ctkgesturesingle.h>
#include <ctk/ctkgesturestylus.h>
#include <ctk/ctkgestureswipe.h>
#include <ctk/ctkgesturezoom.h>
#include <ctk/ctkglarea.h>
#include <ctk/ctkgrid.h>
#include <ctk/ctkheaderbar.h>
#include <ctk/ctkiconfactory.h>
#include <ctk/ctkicontheme.h>
#include <ctk/ctkiconview.h>
#include <ctk/ctkimage.h>
#include <ctk/ctkimcontext.h>
#include <ctk/ctkimcontextinfo.h>
#include <ctk/ctkimcontextsimple.h>
#include <ctk/ctkimmulticontext.h>
#include <ctk/ctkinfobar.h>
#include <ctk/ctkinvisible.h>
#include <ctk/ctklabel.h>
#include <ctk/ctklayout.h>
#include <ctk/ctklevelbar.h>
#include <ctk/ctklinkbutton.h>
#include <ctk/ctklistbox.h>
#include <ctk/ctkliststore.h>
#include <ctk/ctklockbutton.h>
#include <ctk/ctkmain.h>
#include <ctk/ctkmenu.h>
#include <ctk/ctkmenubar.h>
#include <ctk/ctkmenubutton.h>
#include <ctk/ctkmenuitem.h>
#include <ctk/ctkmenushell.h>
#include <ctk/ctkmenutoolbutton.h>
#include <ctk/ctkmessagedialog.h>
#include <ctk/ctkmodelbutton.h>
#include <ctk/ctkmodules.h>
#include <ctk/ctkmountoperation.h>
#include <ctk/ctknativedialog.h>
#include <ctk/ctknotebook.h>
#include <ctk/ctkoffscreenwindow.h>
#include <ctk/ctkorientable.h>
#include <ctk/ctkoverlay.h>
#include <ctk/ctkpadcontroller.h>
#include <ctk/ctkpagesetup.h>
#include <ctk/ctkpapersize.h>
#include <ctk/ctkpaned.h>
#include <ctk/ctkplacessidebar.h>
#include <ctk/ctkpopover.h>
#include <ctk/ctkpopovermenu.h>
#include <ctk/ctkprintcontext.h>
#include <ctk/ctkprintoperation.h>
#include <ctk/ctkprintoperationpreview.h>
#include <ctk/ctkprintsettings.h>
#include <ctk/ctkprogressbar.h>
#include <ctk/ctkradioaction.h>
#include <ctk/ctkradiobutton.h>
#include <ctk/ctkradiomenuitem.h>
#include <ctk/ctkradiotoolbutton.h>
#include <ctk/ctkrange.h>
#include <ctk/ctkrc.h>
#include <ctk/ctkrecentchooser.h>
#include <ctk/ctkrecentchooserdialog.h>
#include <ctk/ctkrecentchoosermenu.h>
#include <ctk/ctkrecentchooserwidget.h>
#include <ctk/ctkrecentfilter.h>
#include <ctk/ctkrecentmanager.h>
#include <ctk/ctkrender.h>
#include <ctk/ctkrevealer.h>
#include <ctk/ctkscale.h>
#include <ctk/ctkscalebutton.h>
#include <ctk/ctkscrollable.h>
#include <ctk/ctkscrollbar.h>
#include <ctk/ctkscrolledwindow.h>
#include <ctk/ctksearchbar.h>
#include <ctk/ctksearchentry.h>
#include <ctk/ctkselection.h>
#include <ctk/ctkseparator.h>
#include <ctk/ctkseparatormenuitem.h>
#include <ctk/ctkseparatortoolitem.h>
#include <ctk/ctksettings.h>
#include <ctk/ctkshortcutlabel.h>
#include <ctk/ctkshortcutsgroup.h>
#include <ctk/ctkshortcutssection.h>
#include <ctk/ctkshortcutsshortcut.h>
#include <ctk/ctkshortcutswindow.h>
#include <ctk/ctkshow.h>
#include <ctk/ctkstacksidebar.h>
#include <ctk/ctksizegroup.h>
#include <ctk/ctksizerequest.h>
#include <ctk/ctkspinbutton.h>
#include <ctk/ctkspinner.h>
#include <ctk/ctkstack.h>
#include <ctk/ctkstackswitcher.h>
#include <ctk/ctkstatusbar.h>
#include <ctk/ctkstock.h>
#include <ctk/ctkstylecontext.h>
#include <ctk/ctkstyleprovider.h>
#include <ctk/ctkstyle.h>
#include <ctk/ctkswitch.h>
#include <ctk/ctktextattributes.h>
#include <ctk/ctktextbuffer.h>
#include <ctk/ctktextbufferrichtext.h>
#include <ctk/ctktextchild.h>
#include <ctk/ctktextiter.h>
#include <ctk/ctktextmark.h>
#include <ctk/ctktexttag.h>
#include <ctk/ctktexttagtable.h>
#include <ctk/ctktextview.h>
#include <ctk/ctktoggleaction.h>
#include <ctk/ctktogglebutton.h>
#include <ctk/ctktoggletoolbutton.h>
#include <ctk/ctktoolbar.h>
#include <ctk/ctktoolbutton.h>
#include <ctk/ctktoolitem.h>
#include <ctk/ctktoolitemgroup.h>
#include <ctk/ctktoolpalette.h>
#include <ctk/ctktoolshell.h>
#include <ctk/ctktooltip.h>
#include <ctk/ctktestutils.h>
#include <ctk/ctktreednd.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreemodelfilter.h>
#include <ctk/ctktreemodelsort.h>
#include <ctk/ctktreeselection.h>
#include <ctk/ctktreesortable.h>
#include <ctk/ctktreestore.h>
#include <ctk/ctktreeview.h>
#include <ctk/ctktreeviewcolumn.h>
#include <ctk/ctktypebuiltins.h>
#include <ctk/ctktypes.h>
#include <ctk/ctkversion.h>
#include <ctk/ctkviewport.h>
#include <ctk/ctkvolumebutton.h>
#include <ctk/ctkwidget.h>
#include <ctk/ctkwidgetpath.h>
#include <ctk/ctkwindow.h>
#include <ctk/ctkwindowgroup.h>

#ifndef CTK_DISABLE_DEPRECATED
#include <ctk/deprecated/ctkarrow.h>
#include <ctk/deprecated/ctkactivatable.h>
#include <ctk/deprecated/ctkalignment.h>
#include <ctk/deprecated/ctkcolorsel.h>
#include <ctk/deprecated/ctkcolorseldialog.h>
#include <ctk/deprecated/ctkfontsel.h>
#include <ctk/deprecated/ctkgradient.h>
#include <ctk/deprecated/ctkhandlebox.h>
#include <ctk/deprecated/ctkhbbox.h>
#include <ctk/deprecated/ctkhbox.h>
#include <ctk/deprecated/ctkhpaned.h>
#include <ctk/deprecated/ctkhsv.h>
#include <ctk/deprecated/ctkhscale.h>
#include <ctk/deprecated/ctkhscrollbar.h>
#include <ctk/deprecated/ctkhseparator.h>
#include <ctk/deprecated/ctkimagemenuitem.h>
#include <ctk/deprecated/ctkmisc.h>
#include <ctk/deprecated/ctknumerableicon.h>
#include <ctk/ctkradioaction.h>
#include <ctk/deprecated/ctkrecentaction.h>
#include <ctk/deprecated/ctkstatusicon.h>
#include <ctk/deprecated/ctkstyleproperties.h>
#include <ctk/deprecated/ctksymboliccolor.h>
#include <ctk/deprecated/ctktable.h>
#include <ctk/deprecated/ctktearoffmenuitem.h>
#include <ctk/deprecated/ctkthemingengine.h>
#include <ctk/deprecated/ctkuimanager.h>
#include <ctk/deprecated/ctkvbbox.h>
#include <ctk/deprecated/ctkvbox.h>
#include <ctk/deprecated/ctkvpaned.h>
#include <ctk/deprecated/ctkvscale.h>
#include <ctk/deprecated/ctkvscrollbar.h>
#include <ctk/deprecated/ctkvseparator.h>
#endif /* CTK_DISABLE_DEPRECATED */

#include <ctk/ctk-autocleanups.h>

#undef __CTK_H_INSIDE__

#endif /* __CTK_H__ */
