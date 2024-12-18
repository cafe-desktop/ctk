/* CTK - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
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

#include "config.h"

#include "ctkdialog.h"
#include "ctkdialogprivate.h"
#include "ctkbutton.h"
#include "ctkbox.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctksettings.h"

#include "ctkcolorchooserprivate.h"
#include "ctkcolorchooserdialog.h"
#include "ctkcolorchooserwidget.h"

/**
 * SECTION:ctkcolorchooserdialog
 * @Short_description: A dialog for choosing colors
 * @Title: CtkColorChooserDialog
 * @See_also: #CtkColorChooser, #CtkDialog
 *
 * The #CtkColorChooserDialog widget is a dialog for choosing
 * a color. It implements the #CtkColorChooser interface.
 *
 * Since: 3.4
 */

struct _CtkColorChooserDialogPrivate
{
  CtkWidget *chooser;
};

enum
{
  PROP_ZERO,
  PROP_RGBA,
  PROP_USE_ALPHA,
  PROP_SHOW_EDITOR
};

static void ctk_color_chooser_dialog_iface_init (CtkColorChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkColorChooserDialog, ctk_color_chooser_dialog, CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkColorChooserDialog)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_COLOR_CHOOSER,
                                                ctk_color_chooser_dialog_iface_init))

static void
propagate_notify (GObject               *o G_GNUC_UNUSED,
                  GParamSpec            *pspec,
                  CtkColorChooserDialog *cc)
{
  g_object_notify (G_OBJECT (cc), pspec->name);
}

static void
save_color (CtkColorChooserDialog *dialog)
{
  CdkRGBA color;

  /* This causes the color chooser widget to save the
   * selected and custom colors to GSettings.
   */
  ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (dialog), &color);
  ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (dialog), &color);
}

static void
color_activated_cb (CtkColorChooser *chooser G_GNUC_UNUSED,
                    CdkRGBA         *color G_GNUC_UNUSED,
                    CtkDialog       *dialog)
{
  save_color (CTK_COLOR_CHOOSER_DIALOG (dialog));
  ctk_dialog_response (dialog, CTK_RESPONSE_OK);
}

static void
ctk_color_chooser_dialog_response (CtkDialog *dialog,
                                   gint       response_id,
                                   gpointer   user_data G_GNUC_UNUSED)
{
  if (response_id == CTK_RESPONSE_OK)
    save_color (CTK_COLOR_CHOOSER_DIALOG (dialog));
}

static void
ctk_color_chooser_dialog_init (CtkColorChooserDialog *cc)
{
  cc->priv = ctk_color_chooser_dialog_get_instance_private (cc);

  ctk_widget_init_template (CTK_WIDGET (cc));
  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (cc));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_dialog_set_alternative_button_order (CTK_DIALOG (cc),
                                           CTK_RESPONSE_OK,
                                           CTK_RESPONSE_CANCEL,
                                           -1);
G_GNUC_END_IGNORE_DEPRECATIONS

  g_signal_connect (cc, "response",
                    G_CALLBACK (ctk_color_chooser_dialog_response), NULL);
}

static void
ctk_color_chooser_dialog_unmap (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_color_chooser_dialog_parent_class)->unmap (widget);

  /* We never want the dialog to come up with the editor,
   * even if it was showing the editor the last time it was used.
   */
  g_object_set (widget, "show-editor", FALSE, NULL);
}

static void
ctk_color_chooser_dialog_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  CtkColorChooserDialog *cd = CTK_COLOR_CHOOSER_DIALOG (object);
  CtkColorChooser *cc = CTK_COLOR_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      {
        CdkRGBA color;

        ctk_color_chooser_get_rgba (cc, &color);
        g_value_set_boxed (value, &color);
      }
      break;
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, ctk_color_chooser_get_use_alpha (CTK_COLOR_CHOOSER (cd->priv->chooser)));
      break;
    case PROP_SHOW_EDITOR:
      {
        gboolean show_editor;
        g_object_get (cd->priv->chooser, "show-editor", &show_editor, NULL);
        g_value_set_boolean (value, show_editor);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_color_chooser_dialog_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  CtkColorChooserDialog *cd = CTK_COLOR_CHOOSER_DIALOG (object);
  CtkColorChooser *cc = CTK_COLOR_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      ctk_color_chooser_set_rgba (cc, g_value_get_boxed (value));
      break;
    case PROP_USE_ALPHA:
      if (ctk_color_chooser_get_use_alpha (CTK_COLOR_CHOOSER (cd->priv->chooser)) != g_value_get_boolean (value))
        {
          ctk_color_chooser_set_use_alpha (CTK_COLOR_CHOOSER (cd->priv->chooser), g_value_get_boolean (value));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SHOW_EDITOR:
      g_object_set (cd->priv->chooser,
                    "show-editor", g_value_get_boolean (value),
                    NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_color_chooser_dialog_class_init (CtkColorChooserDialogClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->get_property = ctk_color_chooser_dialog_get_property;
  object_class->set_property = ctk_color_chooser_dialog_set_property;

  widget_class->unmap = ctk_color_chooser_dialog_unmap;

  g_object_class_override_property (object_class, PROP_RGBA, "rgba");
  g_object_class_override_property (object_class, PROP_USE_ALPHA, "use-alpha");
  g_object_class_install_property (object_class, PROP_SHOW_EDITOR,
      g_param_spec_boolean ("show-editor", P_("Show editor"), P_("Show editor"),
                            FALSE, CTK_PARAM_READWRITE));

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkcolorchooserdialog.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorChooserDialog, chooser);
  ctk_widget_class_bind_template_callback (widget_class, propagate_notify);
  ctk_widget_class_bind_template_callback (widget_class, color_activated_cb);
}

static void
ctk_color_chooser_dialog_get_rgba (CtkColorChooser *chooser,
                                   CdkRGBA         *color)
{
  CtkColorChooserDialog *cc = CTK_COLOR_CHOOSER_DIALOG (chooser);

  ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (cc->priv->chooser), color);
}

static void
ctk_color_chooser_dialog_set_rgba (CtkColorChooser *chooser,
                                   const CdkRGBA   *color)
{
  CtkColorChooserDialog *cc = CTK_COLOR_CHOOSER_DIALOG (chooser);

  ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (cc->priv->chooser), color);
}

static void
ctk_color_chooser_dialog_add_palette (CtkColorChooser *chooser,
                                      CtkOrientation   orientation,
                                      gint             colors_per_line,
                                      gint             n_colors,
                                      CdkRGBA         *colors)
{
  CtkColorChooserDialog *cc = CTK_COLOR_CHOOSER_DIALOG (chooser);

  ctk_color_chooser_add_palette (CTK_COLOR_CHOOSER (cc->priv->chooser),
                                 orientation, colors_per_line, n_colors, colors);
}

static void
ctk_color_chooser_dialog_iface_init (CtkColorChooserInterface *iface)
{
  iface->get_rgba = ctk_color_chooser_dialog_get_rgba;
  iface->set_rgba = ctk_color_chooser_dialog_set_rgba;
  iface->add_palette = ctk_color_chooser_dialog_add_palette;
}

/**
 * ctk_color_chooser_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 *
 * Creates a new #CtkColorChooserDialog.
 *
 * Returns: a new #CtkColorChooserDialog
 *
 * Since: 3.4
 */
CtkWidget *
ctk_color_chooser_dialog_new (const gchar *title,
                              CtkWindow   *parent)
{
  CtkColorChooserDialog *dialog;

  dialog = g_object_new (CTK_TYPE_COLOR_CHOOSER_DIALOG,
                         "title", title,
                         "transient-for", parent,
                         NULL);

  return CTK_WIDGET (dialog);
}
