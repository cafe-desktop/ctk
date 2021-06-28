/* CTK - The GIMP Toolkit
 * Copyright (C) 2011 Alberto Ruiz <aruiz@gnome.org>
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "config.h"

#include <stdlib.h>
#include <glib/gprintf.h>
#include <string.h>

#include <atk/atk.h>

#include "ctkfontchooserdialog.h"
#include "ctkfontchooser.h"
#include "ctkfontchooserwidget.h"
#include "ctkfontchooserwidgetprivate.h"
#include "ctkfontchooserutils.h"
#include "ctkbox.h"
#include "deprecated/ctkstock.h"
#include "ctkintl.h"
#include "ctkaccessible.h"
#include "ctkbuildable.h"
#include "ctkprivate.h"
#include "ctkwidget.h"
#include "ctksettings.h"
#include "ctkdialogprivate.h"
#include "ctktogglebutton.h"
#include "ctkheaderbar.h"
#include "ctkactionable.h"

struct _CtkFontChooserDialogPrivate
{
  CtkWidget *fontchooser;

  CtkWidget *select_button;
  CtkWidget *cancel_button;
  CtkWidget *tweak_button;
};

/**
 * SECTION:ctkfontchooserdialog
 * @Short_description: A dialog for selecting fonts
 * @Title: CtkFontChooserDialog
 * @See_also: #CtkFontChooser, #CtkDialog
 *
 * The #CtkFontChooserDialog widget is a dialog for selecting a font.
 * It implements the #CtkFontChooser interface.
 *
 * # CtkFontChooserDialog as CtkBuildable
 *
 * The CtkFontChooserDialog implementation of the #CtkBuildable
 * interface exposes the buttons with the names “select_button”
 * and “cancel_button”.
 *
 * Since: 3.2
 */

static void     ctk_font_chooser_dialog_buildable_interface_init     (CtkBuildableIface *iface);
static GObject *ctk_font_chooser_dialog_buildable_get_internal_child (CtkBuildable *buildable,
                                                                      CtkBuilder   *builder,
                                                                      const gchar  *childname);

G_DEFINE_TYPE_WITH_CODE (CtkFontChooserDialog, ctk_font_chooser_dialog, CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkFontChooserDialog)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_FONT_CHOOSER,
                                                _ctk_font_chooser_delegate_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_font_chooser_dialog_buildable_interface_init))

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_font_chooser_dialog_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  CtkFontChooserDialog *dialog = CTK_FONT_CHOOSER_DIALOG (object);
  CtkFontChooserDialogPrivate *priv = dialog->priv;

  switch (prop_id)
    {
    default:
      g_object_set_property (G_OBJECT (priv->fontchooser), pspec->name, value);
      break;
    }
}

static void
ctk_font_chooser_dialog_get_property (GObject      *object,
                                      guint         prop_id,
                                      GValue       *value,
                                      GParamSpec   *pspec)
{
  CtkFontChooserDialog *dialog = CTK_FONT_CHOOSER_DIALOG (object);
  CtkFontChooserDialogPrivate *priv = dialog->priv;

  switch (prop_id)
    {
    default:
      g_object_get_property (G_OBJECT (priv->fontchooser), pspec->name, value);
      break;
    }
}

static void
font_activated_cb (CtkFontChooser *fontchooser,
                   const gchar    *fontname,
                   gpointer        user_data)
{
  CtkDialog *dialog = user_data;

  ctk_dialog_response (dialog, CTK_RESPONSE_OK);
}

static gboolean
ctk_font_chooser_dialog_key_press_event (CtkWidget   *dialog,
                                         CdkEventKey *event)
{
  CtkFontChooserDialog *fdialog = CTK_FONT_CHOOSER_DIALOG (dialog);
  gboolean handled = FALSE;

  handled = CTK_WIDGET_CLASS (ctk_font_chooser_dialog_parent_class)->key_press_event (dialog, event);

  if (!handled)
    handled = ctk_font_chooser_widget_handle_event (fdialog->priv->fontchooser, event);

  return handled;
}

static void
update_tweak_button (CtkFontChooserDialog *dialog)
{
  CtkFontChooserLevel level;

  if (!dialog->priv->tweak_button)
    return;

  g_object_get (dialog->priv->fontchooser, "level", &level, NULL);
  if ((level & (CTK_FONT_CHOOSER_LEVEL_VARIATIONS | CTK_FONT_CHOOSER_LEVEL_FEATURES)) != 0)
    ctk_widget_show (dialog->priv->tweak_button);
  else
    ctk_widget_hide (dialog->priv->tweak_button);
}

static void
setup_tweak_button (CtkFontChooserDialog *dialog)
{
  gboolean use_header;

  if (dialog->priv->tweak_button)
    return;

  g_object_get (dialog, "use-header-bar", &use_header, NULL);
  if (use_header)
    {
      CtkWidget *button;
      CtkWidget *image;
      CtkWidget *header;
      GActionGroup *actions;

      actions = G_ACTION_GROUP (g_simple_action_group_new ());
      g_action_map_add_action (G_ACTION_MAP (actions), ctk_font_chooser_widget_get_tweak_action (dialog->priv->fontchooser));
      ctk_widget_insert_action_group (CTK_WIDGET (dialog), "font", actions);
      g_object_unref (actions);

      button = ctk_toggle_button_new ();
      ctk_actionable_set_action_name (CTK_ACTIONABLE (button), "font.tweak");
      ctk_widget_set_focus_on_click (button, FALSE);
      ctk_widget_set_valign (button, CTK_ALIGN_CENTER);

      image = ctk_image_new_from_icon_name ("emblem-system-symbolic", CTK_ICON_SIZE_BUTTON);
      ctk_widget_show (image);
      ctk_container_add (CTK_CONTAINER (button), image);

      header = ctk_dialog_get_header_bar (CTK_DIALOG (dialog));
      ctk_header_bar_pack_end (CTK_HEADER_BAR (header), button);

      dialog->priv->tweak_button = button;
      update_tweak_button (dialog);
    }
}

static void
ctk_font_chooser_dialog_map (CtkWidget *widget)
{
  CtkFontChooserDialog *dialog = CTK_FONT_CHOOSER_DIALOG (widget);

  setup_tweak_button (dialog);

  CTK_WIDGET_CLASS (ctk_font_chooser_dialog_parent_class)->map (widget);
}

static void
ctk_font_chooser_dialog_class_init (CtkFontChooserDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  gobject_class->get_property = ctk_font_chooser_dialog_get_property;
  gobject_class->set_property = ctk_font_chooser_dialog_set_property;

  widget_class->key_press_event = ctk_font_chooser_dialog_key_press_event;
  widget_class->map = ctk_font_chooser_dialog_map;

  _ctk_font_chooser_install_properties (gobject_class);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
                                               "/org/ctk/libctk/ui/ctkfontchooserdialog.ui");

  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserDialog, fontchooser);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserDialog, select_button);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontChooserDialog, cancel_button);
  ctk_widget_class_bind_template_callback (widget_class, font_activated_cb);
}

static void
update_button (CtkFontChooserDialog *dialog)
{
  CtkFontChooserDialogPrivate *priv = dialog->priv;
  PangoFontDescription *desc;

  desc = ctk_font_chooser_get_font_desc (CTK_FONT_CHOOSER (priv->fontchooser));

  ctk_widget_set_sensitive (priv->select_button, desc != NULL);

  if (desc)
    pango_font_description_free (desc);
}

static void
ctk_font_chooser_dialog_init (CtkFontChooserDialog *fontchooserdiag)
{
  CtkFontChooserDialogPrivate *priv;

  fontchooserdiag->priv = ctk_font_chooser_dialog_get_instance_private (fontchooserdiag);
  priv = fontchooserdiag->priv;

  ctk_widget_init_template (CTK_WIDGET (fontchooserdiag));
  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (fontchooserdiag));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_dialog_set_alternative_button_order (CTK_DIALOG (fontchooserdiag),
                                           CTK_RESPONSE_OK,
                                           CTK_RESPONSE_CANCEL,
                                           -1);
G_GNUC_END_IGNORE_DEPRECATIONS
  _ctk_font_chooser_set_delegate (CTK_FONT_CHOOSER (fontchooserdiag),
                                  CTK_FONT_CHOOSER (priv->fontchooser));

  g_signal_connect_swapped (priv->fontchooser, "notify::font-desc",
                            G_CALLBACK (update_button), fontchooserdiag);
  update_button (fontchooserdiag);

  g_signal_connect_swapped (priv->fontchooser, "notify::level",
                            G_CALLBACK (update_tweak_button), fontchooserdiag);
}

/**
 * ctk_font_chooser_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 *
 * Creates a new #CtkFontChooserDialog.
 *
 * Returns: a new #CtkFontChooserDialog
 *
 * Since: 3.2
 */
CtkWidget*
ctk_font_chooser_dialog_new (const gchar *title,
                             CtkWindow   *parent)
{
  CtkFontChooserDialog *dialog;

  dialog = g_object_new (CTK_TYPE_FONT_CHOOSER_DIALOG,
                         "title", title,
                         "transient-for", parent,
                         NULL);

  return CTK_WIDGET (dialog);
}

static void
ctk_font_chooser_dialog_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = ctk_font_chooser_dialog_buildable_get_internal_child;
}

static GObject *
ctk_font_chooser_dialog_buildable_get_internal_child (CtkBuildable *buildable,
                                                      CtkBuilder   *builder,
                                                      const gchar  *childname)
{
  CtkFontChooserDialogPrivate *priv;

  priv = CTK_FONT_CHOOSER_DIALOG (buildable)->priv;

  if (g_strcmp0 (childname, "select_button") == 0)
    return G_OBJECT (priv->select_button);
  else if (g_strcmp0 (childname, "cancel_button") == 0)
    return G_OBJECT (priv->cancel_button);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}
