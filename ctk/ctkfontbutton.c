/* 
 * GTK - The GIMP Toolkit
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved.
 *
 * Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Modified by the GTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctkfontbutton.h"

#include "ctkmain.h"
#include "ctkbox.h"
#include "ctklabel.h"
#include "ctkfontchooser.h"
#include "ctkfontchooserdialog.h"
#include "ctkimage.h"
#include "ctkmarshalers.h"
#include "ctkseparator.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkcssprovider.h"

#include <string.h>
#include <stdio.h>
#include "ctkfontchooserutils.h"


/**
 * SECTION:ctkfontbutton
 * @Short_description: A button to launch a font chooser dialog
 * @Title: CtkFontButton
 * @See_also: #CtkFontChooserDialog, #CtkColorButton.
 *
 * The #CtkFontButton is a button which displays the currently selected
 * font an allows to open a font chooser dialog to change the font.
 * It is suitable widget for selecting a font in a preference dialog.
 *
 * # CSS nodes
 *
 * CtkFontButton has a single CSS node with name button and style class .font.
 */


struct _CtkFontButtonPrivate
{
  gchar         *title;

  gchar         *fontname;

  guint         use_font : 1;
  guint         use_size : 1;
  guint         show_style : 1;
  guint         show_size : 1;
  guint         show_preview_entry : 1;

  CtkWidget     *font_dialog;
  CtkWidget     *font_label;
  CtkWidget     *size_label;
  CtkWidget     *font_size_box;

  PangoFontDescription *font_desc;
  PangoFontFamily      *font_family;
  PangoFontFace        *font_face;
  PangoFontMap         *font_map;
  gint                  font_size;
  char                 *font_features;
  PangoLanguage        *language;
  gchar                *preview_text;
  CtkFontFilterFunc     font_filter;
  gpointer              font_filter_data;
  GDestroyNotify        font_filter_data_destroy;
  CtkCssProvider       *provider;

  CtkFontChooserLevel   level;
};

/* Signals */
enum
{
  FONT_SET,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_TITLE,
  PROP_FONT_NAME,
  PROP_USE_FONT,
  PROP_USE_SIZE,
  PROP_SHOW_STYLE,
  PROP_SHOW_SIZE
};

/* Prototypes */
static void ctk_font_button_finalize               (GObject            *object);
static void ctk_font_button_get_property           (GObject            *object,
                                                    guint               param_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void ctk_font_button_set_property           (GObject            *object,
                                                    guint               param_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);

static void ctk_font_button_clicked                 (CtkButton         *button);

/* Dialog response functions */
static void response_cb                             (CtkDialog         *dialog,
                                                     gint               response_id,
                                                     gpointer           data);
static void dialog_destroy                          (CtkWidget         *widget,
                                                     gpointer           data);

/* Auxiliary functions */
static void ctk_font_button_label_use_font          (CtkFontButton     *gfs);
static void ctk_font_button_update_font_info        (CtkFontButton     *gfs);

static void        font_button_set_font_name (CtkFontButton *button,
                                              const char    *fontname);
static void        ctk_font_button_set_level     (CtkFontButton       *font_button,
                                                  CtkFontChooserLevel  level);
static void        ctk_font_button_set_language  (CtkFontButton *button,
                                                  const char    *language);

static guint font_button_signals[LAST_SIGNAL] = { 0 };

static void
clear_font_data (CtkFontButton *font_button)
{
  CtkFontButtonPrivate *priv = font_button->priv;

  if (priv->font_family)
    g_object_unref (priv->font_family);
  priv->font_family = NULL;

  if (priv->font_face)
    g_object_unref (priv->font_face);
  priv->font_face = NULL;

  if (priv->font_desc)
    pango_font_description_free (priv->font_desc);
  priv->font_desc = NULL;

  g_free (priv->fontname);
  priv->fontname = NULL;

  g_free (priv->font_features);
  priv->font_features = NULL;
}

static void
clear_font_filter_data (CtkFontButton *font_button)
{
  CtkFontButtonPrivate *priv = font_button->priv;

  if (priv->font_filter_data_destroy)
    priv->font_filter_data_destroy (priv->font_filter_data);
  priv->font_filter = NULL;
  priv->font_filter_data = NULL;
  priv->font_filter_data_destroy = NULL;
}

static gboolean
font_description_style_equal (const PangoFontDescription *a,
                              const PangoFontDescription *b)
{
  return (pango_font_description_get_weight (a) == pango_font_description_get_weight (b) &&
          pango_font_description_get_style (a) == pango_font_description_get_style (b) &&
          pango_font_description_get_stretch (a) == pango_font_description_get_stretch (b) &&
          pango_font_description_get_variant (a) == pango_font_description_get_variant (b));
}

static void
ctk_font_button_update_font_data (CtkFontButton *font_button)
{
  CtkFontButtonPrivate *priv = font_button->priv;
  PangoFontFamily **families;
  PangoFontFace **faces;
  gint n_families, n_faces, i;
  const gchar *family;

  g_assert (priv->font_desc != NULL);

  priv->fontname = pango_font_description_to_string (priv->font_desc);

  family = pango_font_description_get_family (priv->font_desc);
  if (family == NULL)
    return;

  n_families = 0;
  families = NULL;
  pango_context_list_families (ctk_widget_get_pango_context (CTK_WIDGET (font_button)),
                               &families, &n_families);
  n_faces = 0;
  faces = NULL;
  for (i = 0; i < n_families; i++)
    {
      const gchar *name = pango_font_family_get_name (families[i]);

      if (!g_ascii_strcasecmp (name, family))
        {
          priv->font_family = g_object_ref (families[i]);

          pango_font_family_list_faces (families[i], &faces, &n_faces);
          break;
        }
    }
  g_free (families);

  for (i = 0; i < n_faces; i++)
    {
      PangoFontDescription *tmp_desc = pango_font_face_describe (faces[i]);

      if (font_description_style_equal (tmp_desc, priv->font_desc))
        {
          priv->font_face = g_object_ref (faces[i]);

          pango_font_description_free (tmp_desc);
          break;
        }
      else
        pango_font_description_free (tmp_desc);
    }

  g_free (faces);
}

static gchar *
ctk_font_button_get_preview_text (CtkFontButton *font_button)
{
  CtkFontButtonPrivate *priv = font_button->priv;

  if (priv->font_dialog)
    return ctk_font_chooser_get_preview_text (CTK_FONT_CHOOSER (priv->font_dialog));

  return g_strdup (priv->preview_text);
}

static void
ctk_font_button_set_preview_text (CtkFontButton *font_button,
                                  const gchar   *preview_text)
{
  CtkFontButtonPrivate *priv = font_button->priv;

  if (priv->font_dialog)
    {
      ctk_font_chooser_set_preview_text (CTK_FONT_CHOOSER (priv->font_dialog),
                                         preview_text);
      return;
    }

  g_free (priv->preview_text);
  priv->preview_text = g_strdup (preview_text);
}


static gboolean
ctk_font_button_get_show_preview_entry (CtkFontButton *font_button)
{
  CtkFontButtonPrivate *priv = font_button->priv;

  if (priv->font_dialog)
    return ctk_font_chooser_get_show_preview_entry (CTK_FONT_CHOOSER (priv->font_dialog));

  return priv->show_preview_entry;
}

static void
ctk_font_button_set_show_preview_entry (CtkFontButton *font_button,
                                        gboolean       show)
{
  CtkFontButtonPrivate *priv = font_button->priv;

  show = show != FALSE;

  if (priv->show_preview_entry != show)
    {
      priv->show_preview_entry = show;
      if (priv->font_dialog)
        ctk_font_chooser_set_show_preview_entry (CTK_FONT_CHOOSER (priv->font_dialog), show);
      g_object_notify (G_OBJECT (font_button), "show-preview-entry");
    }
}

static PangoFontFamily *
ctk_font_button_font_chooser_get_font_family (CtkFontChooser *chooser)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (chooser);
  CtkFontButtonPrivate *priv = font_button->priv;

  return priv->font_family;
}

static PangoFontFace *
ctk_font_button_font_chooser_get_font_face (CtkFontChooser *chooser)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (chooser);
  CtkFontButtonPrivate *priv = font_button->priv;

  return priv->font_face;
}

static int
ctk_font_button_font_chooser_get_font_size (CtkFontChooser *chooser)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (chooser);
  CtkFontButtonPrivate *priv = font_button->priv;

  return priv->font_size;
}

static void
ctk_font_button_font_chooser_set_filter_func (CtkFontChooser    *chooser,
                                              CtkFontFilterFunc  filter_func,
                                              gpointer           filter_data,
                                              GDestroyNotify     data_destroy)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (chooser);
  CtkFontButtonPrivate *priv = font_button->priv;

  if (priv->font_dialog)
    {
      ctk_font_chooser_set_filter_func (CTK_FONT_CHOOSER (priv->font_dialog),
                                        filter_func,
                                        filter_data,
                                        data_destroy);
      return;
    }

  clear_font_filter_data (font_button);
  priv->font_filter = filter_func;
  priv->font_filter_data = filter_data;
  priv->font_filter_data_destroy = data_destroy;
}

static void
ctk_font_button_take_font_desc (CtkFontButton        *font_button,
                                PangoFontDescription *font_desc)
{
  CtkFontButtonPrivate *priv = font_button->priv;
  GObject *object = G_OBJECT (font_button);

  if (priv->font_desc && font_desc &&
      pango_font_description_equal (priv->font_desc, font_desc))
    {
      pango_font_description_free (font_desc);
      return;
    }

  g_object_freeze_notify (object);

  clear_font_data (font_button);

  if (font_desc)
    priv->font_desc = font_desc; /* adopted */
  else
    priv->font_desc = pango_font_description_from_string (_("Sans 12"));

  if (pango_font_description_get_size_is_absolute (priv->font_desc))
    priv->font_size = pango_font_description_get_size (priv->font_desc);
  else
    priv->font_size = pango_font_description_get_size (priv->font_desc) / PANGO_SCALE;

  ctk_font_button_update_font_data (font_button);
  ctk_font_button_update_font_info (font_button);

  if (priv->font_dialog)
    ctk_font_chooser_set_font_desc (CTK_FONT_CHOOSER (priv->font_dialog),
                                    priv->font_desc);

  g_object_notify (G_OBJECT (font_button), "font");
  g_object_notify (G_OBJECT (font_button), "font-desc");
  g_object_notify (G_OBJECT (font_button), "font-name");

  g_object_thaw_notify (object);
}

static const PangoFontDescription *
ctk_font_button_get_font_desc (CtkFontButton *font_button)
{
  return font_button->priv->font_desc;
}

static void
ctk_font_button_font_chooser_set_font_map (CtkFontChooser *chooser,
                                           PangoFontMap   *font_map)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (chooser);

  if (g_set_object (&font_button->priv->font_map, font_map))
    {
      PangoContext *context;

      if (!font_map)
        font_map = pango_cairo_font_map_get_default ();

      context = ctk_widget_get_pango_context (font_button->priv->font_label);
      pango_context_set_font_map (context, font_map);
    }
}

static PangoFontMap *
ctk_font_button_font_chooser_get_font_map (CtkFontChooser *chooser)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (chooser);

  return font_button->priv->font_map;
}

static void
ctk_font_button_font_chooser_notify (GObject    *object,
                                     GParamSpec *pspec,
                                     gpointer    user_data)
{
  /* We do not forward the notification of the "font" property to the dialog! */
  if (pspec->name == I_("preview-text") ||
      pspec->name == I_("show-preview-entry"))
    g_object_notify_by_pspec (user_data, pspec);
}

static void
ctk_font_button_font_chooser_iface_init (CtkFontChooserIface *iface)
{
  iface->get_font_family = ctk_font_button_font_chooser_get_font_family;
  iface->get_font_face = ctk_font_button_font_chooser_get_font_face;
  iface->get_font_size = ctk_font_button_font_chooser_get_font_size;
  iface->set_filter_func = ctk_font_button_font_chooser_set_filter_func;
  iface->set_font_map = ctk_font_button_font_chooser_set_font_map;
  iface->get_font_map = ctk_font_button_font_chooser_get_font_map;
}

G_DEFINE_TYPE_WITH_CODE (CtkFontButton, ctk_font_button, CTK_TYPE_BUTTON,
                         G_ADD_PRIVATE (CtkFontButton)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_FONT_CHOOSER,
                                                ctk_font_button_font_chooser_iface_init))

static void
ctk_font_button_class_init (CtkFontButtonClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkButtonClass *button_class;
  
  gobject_class = (GObjectClass *) klass;
  widget_class = (CtkWidgetClass *) klass;
  button_class = (CtkButtonClass *) klass;

  gobject_class->finalize = ctk_font_button_finalize;
  gobject_class->set_property = ctk_font_button_set_property;
  gobject_class->get_property = ctk_font_button_get_property;
  
  button_class->clicked = ctk_font_button_clicked;
  
  klass->font_set = NULL;

  _ctk_font_chooser_install_properties (gobject_class);

  /**
   * CtkFontButton:title:
   * 
   * The title of the font chooser dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        P_("Title"),
                                                        P_("The title of the font chooser dialog"),
                                                        _("Pick a Font"),
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkFontButton:font-name:
   * 
   * The name of the currently selected font.
   *
   * Since: 2.4
   *
   * Deprecated: 3.22: Use the #CtkFontChooser::font property instead
   */
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        P_("Font name"),
                                                        P_("The name of the selected font"),
                                                        _("Sans 12"),
                                                        CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * CtkFontButton:use-font:
   * 
   * If this property is set to %TRUE, the label will be drawn 
   * in the selected font.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_FONT,
                                   g_param_spec_boolean ("use-font",
                                                         P_("Use font in label"),
                                                         P_("Whether the label is drawn in the selected font"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkFontButton:use-size:
   * 
   * If this property is set to %TRUE, the label will be drawn 
   * with the selected font size.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_SIZE,
                                   g_param_spec_boolean ("use-size",
                                                         P_("Use size in label"),
                                                         P_("Whether the label is drawn with the selected font size"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkFontButton:show-style:
   * 
   * If this property is set to %TRUE, the name of the selected font style 
   * will be shown in the label. For a more WYSIWYG way to show the selected 
   * style, see the ::use-font property. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_STYLE,
                                   g_param_spec_boolean ("show-style",
                                                         P_("Show style"),
                                                         P_("Whether the selected font style is shown in the label"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  /**
   * CtkFontButton:show-size:
   * 
   * If this property is set to %TRUE, the selected font size will be shown 
   * in the label. For a more WYSIWYG way to show the selected size, see the 
   * ::use-size property. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_SIZE,
                                   g_param_spec_boolean ("show-size",
                                                         P_("Show size"),
                                                         P_("Whether selected font size is shown in the label"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkFontButton::font-set:
   * @widget: the object which received the signal.
   * 
   * The ::font-set signal is emitted when the user selects a font. 
   * When handling this signal, use ctk_font_chooser_get_font()
   * to find out which font was just selected.
   *
   * Note that this signal is only emitted when the user
   * changes the font. If you need to react to programmatic font changes
   * as well, use the notify::font signal.
   *
   * Since: 2.4
   */
  font_button_signals[FONT_SET] = g_signal_new (I_("font-set"),
                                                G_TYPE_FROM_CLASS (gobject_class),
                                                G_SIGNAL_RUN_FIRST,
                                                G_STRUCT_OFFSET (CtkFontButtonClass, font_set),
                                                NULL, NULL,
                                                NULL,
                                                G_TYPE_NONE, 0);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkfontbutton.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontButton, font_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontButton, size_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFontButton, font_size_box);

  ctk_widget_class_set_css_name (widget_class, "button");
}

static void
ctk_font_button_init (CtkFontButton *font_button)
{
  CtkStyleContext *context;

  font_button->priv = ctk_font_button_get_instance_private (font_button);

  /* Initialize fields */
  font_button->priv->use_font = FALSE;
  font_button->priv->use_size = FALSE;
  font_button->priv->show_style = TRUE;
  font_button->priv->show_size = TRUE;
  font_button->priv->show_preview_entry = TRUE;
  font_button->priv->font_dialog = NULL;
  font_button->priv->font_family = NULL;
  font_button->priv->font_face = NULL;
  font_button->priv->font_size = -1;
  font_button->priv->title = g_strdup (_("Pick a Font"));
  font_button->priv->level = CTK_FONT_CHOOSER_LEVEL_FAMILY |
                             CTK_FONT_CHOOSER_LEVEL_STYLE |
                             CTK_FONT_CHOOSER_LEVEL_SIZE;
  font_button->priv->language = pango_language_get_default ();

  ctk_widget_init_template (CTK_WIDGET (font_button));

  ctk_font_button_take_font_desc (font_button, NULL);

  context = ctk_widget_get_style_context (CTK_WIDGET (font_button));
  ctk_style_context_add_class (context, "font");
}

static void
ctk_font_button_finalize (GObject *object)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (object);
  CtkFontButtonPrivate *priv = font_button->priv;

  if (priv->font_dialog != NULL) 
    ctk_widget_destroy (priv->font_dialog);

  g_free (priv->title);

  clear_font_data (font_button);
  clear_font_filter_data (font_button);

  g_free (priv->preview_text);

  g_clear_object (&priv->provider);

  G_OBJECT_CLASS (ctk_font_button_parent_class)->finalize (object);
}

static void
ctk_font_button_set_property (GObject      *object,
                              guint         param_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (object);

  switch (param_id) 
    {
    case CTK_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      ctk_font_button_set_preview_text (font_button, g_value_get_string (value));
      break;
    case CTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      ctk_font_button_set_show_preview_entry (font_button, g_value_get_boolean (value));
      break;
    case PROP_TITLE:
      ctk_font_button_set_title (font_button, g_value_get_string (value));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT_DESC:
      ctk_font_button_take_font_desc (font_button, g_value_dup_boxed (value));
      break;
    case CTK_FONT_CHOOSER_PROP_LANGUAGE:
      ctk_font_button_set_language (font_button, g_value_get_string (value));
      break;
    case CTK_FONT_CHOOSER_PROP_LEVEL:
      ctk_font_button_set_level (font_button, g_value_get_flags (value));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT:
    case PROP_FONT_NAME:
      font_button_set_font_name (font_button, g_value_get_string (value));
      break;
    case PROP_USE_FONT:
      ctk_font_button_set_use_font (font_button, g_value_get_boolean (value));
      break;
    case PROP_USE_SIZE:
      ctk_font_button_set_use_size (font_button, g_value_get_boolean (value));
      break;
    case PROP_SHOW_STYLE:
      ctk_font_button_set_show_style (font_button, g_value_get_boolean (value));
      break;
    case PROP_SHOW_SIZE:
      ctk_font_button_set_show_size (font_button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
ctk_font_button_get_property (GObject    *object,
                              guint       param_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (object);
  CtkFontButtonPrivate *priv = font_button->priv;
  
  switch (param_id) 
    {
    case CTK_FONT_CHOOSER_PROP_PREVIEW_TEXT:
      g_value_set_string (value, ctk_font_button_get_preview_text (font_button));
      break;
    case CTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY:
      g_value_set_boolean (value, ctk_font_button_get_show_preview_entry (font_button));
      break;
    case PROP_TITLE:
      g_value_set_string (value, ctk_font_button_get_title (font_button));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT_DESC:
      g_value_set_boxed (value, ctk_font_button_get_font_desc (font_button));
      break;
    case CTK_FONT_CHOOSER_PROP_FONT_FEATURES:
      g_value_set_string (value, priv->font_features);
      break;
    case CTK_FONT_CHOOSER_PROP_LANGUAGE:
      g_value_set_string (value, pango_language_to_string (priv->language));
      break;
    case CTK_FONT_CHOOSER_PROP_LEVEL:
      g_value_set_flags (value, priv->level);
      break;
    case CTK_FONT_CHOOSER_PROP_FONT:
    case PROP_FONT_NAME:
      g_value_set_string (value, font_button->priv->fontname);
      break;
    case PROP_USE_FONT:
      g_value_set_boolean (value, ctk_font_button_get_use_font (font_button));
      break;
    case PROP_USE_SIZE:
      g_value_set_boolean (value, ctk_font_button_get_use_size (font_button));
      break;
    case PROP_SHOW_STYLE:
      g_value_set_boolean (value, ctk_font_button_get_show_style (font_button));
      break;
    case PROP_SHOW_SIZE:
      g_value_set_boolean (value, ctk_font_button_get_show_size (font_button));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


/**
 * ctk_font_button_new:
 *
 * Creates a new font picker widget.
 *
 * Returns: a new font picker widget.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_font_button_new (void)
{
  return g_object_new (CTK_TYPE_FONT_BUTTON, NULL);
}

/**
 * ctk_font_button_new_with_font:
 * @fontname: Name of font to display in font chooser dialog
 *
 * Creates a new font picker widget.
 *
 * Returns: a new font picker widget.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_font_button_new_with_font (const gchar *fontname)
{
  return g_object_new (CTK_TYPE_FONT_BUTTON, "font", fontname, NULL);
} 

/**
 * ctk_font_button_set_title:
 * @font_button: a #CtkFontButton
 * @title: a string containing the font chooser dialog title
 *
 * Sets the title for the font chooser dialog.  
 *
 * Since: 2.4
 */
void
ctk_font_button_set_title (CtkFontButton *font_button, 
                           const gchar   *title)
{
  gchar *old_title;
  g_return_if_fail (CTK_IS_FONT_BUTTON (font_button));
  
  old_title = font_button->priv->title;
  font_button->priv->title = g_strdup (title);
  g_free (old_title);
  
  if (font_button->priv->font_dialog)
    ctk_window_set_title (CTK_WINDOW (font_button->priv->font_dialog),
                          font_button->priv->title);

  g_object_notify (G_OBJECT (font_button), "title");
} 

/**
 * ctk_font_button_get_title:
 * @font_button: a #CtkFontButton
 *
 * Retrieves the title of the font chooser dialog.
 *
 * Returns: an internal copy of the title string which must not be freed.
 *
 * Since: 2.4
 */
const gchar*
ctk_font_button_get_title (CtkFontButton *font_button)
{
  g_return_val_if_fail (CTK_IS_FONT_BUTTON (font_button), NULL);

  return font_button->priv->title;
} 

/**
 * ctk_font_button_get_use_font:
 * @font_button: a #CtkFontButton
 *
 * Returns whether the selected font is used in the label.
 *
 * Returns: whether the selected font is used in the label.
 *
 * Since: 2.4
 */
gboolean
ctk_font_button_get_use_font (CtkFontButton *font_button)
{
  g_return_val_if_fail (CTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->use_font;
} 

/**
 * ctk_font_button_set_use_font:
 * @font_button: a #CtkFontButton
 * @use_font: If %TRUE, font name will be written using font chosen.
 *
 * If @use_font is %TRUE, the font name will be written using the selected font.  
 *
 * Since: 2.4
 */
void  
ctk_font_button_set_use_font (CtkFontButton *font_button,
			      gboolean       use_font)
{
  g_return_if_fail (CTK_IS_FONT_BUTTON (font_button));
  
  use_font = (use_font != FALSE);
  
  if (font_button->priv->use_font != use_font) 
    {
      font_button->priv->use_font = use_font;

      ctk_font_button_label_use_font (font_button);
 
      g_object_notify (G_OBJECT (font_button), "use-font");
    }
} 


/**
 * ctk_font_button_get_use_size:
 * @font_button: a #CtkFontButton
 *
 * Returns whether the selected size is used in the label.
 *
 * Returns: whether the selected size is used in the label.
 *
 * Since: 2.4
 */
gboolean
ctk_font_button_get_use_size (CtkFontButton *font_button)
{
  g_return_val_if_fail (CTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->use_size;
} 

/**
 * ctk_font_button_set_use_size:
 * @font_button: a #CtkFontButton
 * @use_size: If %TRUE, font name will be written using the selected size.
 *
 * If @use_size is %TRUE, the font name will be written using the selected size.
 *
 * Since: 2.4
 */
void  
ctk_font_button_set_use_size (CtkFontButton *font_button,
                              gboolean       use_size)
{
  g_return_if_fail (CTK_IS_FONT_BUTTON (font_button));
  
  use_size = (use_size != FALSE);
  if (font_button->priv->use_size != use_size) 
    {
      font_button->priv->use_size = use_size;

      ctk_font_button_label_use_font (font_button);

      g_object_notify (G_OBJECT (font_button), "use-size");
    }
} 

/**
 * ctk_font_button_get_show_style:
 * @font_button: a #CtkFontButton
 * 
 * Returns whether the name of the font style will be shown in the label.
 * 
 * Returns: whether the font style will be shown in the label.
 *
 * Since: 2.4
 **/
gboolean 
ctk_font_button_get_show_style (CtkFontButton *font_button)
{
  g_return_val_if_fail (CTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->show_style;
}

/**
 * ctk_font_button_set_show_style:
 * @font_button: a #CtkFontButton
 * @show_style: %TRUE if font style should be displayed in label.
 *
 * If @show_style is %TRUE, the font style will be displayed along with name of the selected font.
 *
 * Since: 2.4
 */
void
ctk_font_button_set_show_style (CtkFontButton *font_button,
                                gboolean       show_style)
{
  g_return_if_fail (CTK_IS_FONT_BUTTON (font_button));
  
  show_style = (show_style != FALSE);
  if (font_button->priv->show_style != show_style) 
    {
      font_button->priv->show_style = show_style;
      
      ctk_font_button_update_font_info (font_button);
  
      g_object_notify (G_OBJECT (font_button), "show-style");
    }
} 


/**
 * ctk_font_button_get_show_size:
 * @font_button: a #CtkFontButton
 * 
 * Returns whether the font size will be shown in the label.
 * 
 * Returns: whether the font size will be shown in the label.
 *
 * Since: 2.4
 **/
gboolean 
ctk_font_button_get_show_size (CtkFontButton *font_button)
{
  g_return_val_if_fail (CTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->show_size;
}

/**
 * ctk_font_button_set_show_size:
 * @font_button: a #CtkFontButton
 * @show_size: %TRUE if font size should be displayed in dialog.
 *
 * If @show_size is %TRUE, the font size will be displayed along with the name of the selected font.
 *
 * Since: 2.4
 */
void
ctk_font_button_set_show_size (CtkFontButton *font_button,
                               gboolean       show_size)
{
  g_return_if_fail (CTK_IS_FONT_BUTTON (font_button));
  
  show_size = (show_size != FALSE);

  if (font_button->priv->show_size != show_size) 
    {
      font_button->priv->show_size = show_size;

      if (font_button->priv->show_size)
	ctk_widget_show (font_button->priv->font_size_box);
      else
	ctk_widget_hide (font_button->priv->font_size_box);
      
      ctk_font_button_update_font_info (font_button);

      g_object_notify (G_OBJECT (font_button), "show-size");
    }
} 


/**
 * ctk_font_button_get_font_name:
 * @font_button: a #CtkFontButton
 *
 * Retrieves the name of the currently selected font. This name includes
 * style and size information as well. If you want to render something
 * with the font, use this string with pango_font_description_from_string() .
 * If you’re interested in peeking certain values (family name,
 * style, size, weight) just query these properties from the
 * #PangoFontDescription object.
 *
 * Returns: an internal copy of the font name which must not be freed.
 *
 * Since: 2.4
 * Deprecated: 3.22: Use ctk_font_chooser_get_font() instead
 */
const gchar *
ctk_font_button_get_font_name (CtkFontButton *font_button)
{
  g_return_val_if_fail (CTK_IS_FONT_BUTTON (font_button), NULL);

  return font_button->priv->fontname;
}

static void
font_button_set_font_name (CtkFontButton *font_button,
                           const char    *fontname)
{
  PangoFontDescription *font_desc;

  font_desc = pango_font_description_from_string (fontname);
  ctk_font_button_take_font_desc (font_button, font_desc);
}

/**
 * ctk_font_button_set_font_name:
 * @font_button: a #CtkFontButton
 * @fontname: Name of font to display in font chooser dialog
 *
 * Sets or updates the currently-displayed font in font picker dialog.
 *
 * Returns: %TRUE
 *
 * Since: 2.4
 * Deprecated: 3.22: Use ctk_font_chooser_set_font() instead
 */
gboolean 
ctk_font_button_set_font_name (CtkFontButton *font_button,
                               const gchar    *fontname)
{
  g_return_val_if_fail (CTK_IS_FONT_BUTTON (font_button), FALSE);
  g_return_val_if_fail (fontname != NULL, FALSE);

  font_button_set_font_name (font_button, fontname);

  return TRUE;
}

static void
ctk_font_button_clicked (CtkButton *button)
{
  CtkFontChooser *font_dialog;
  CtkFontButton  *font_button = CTK_FONT_BUTTON (button);
  CtkFontButtonPrivate *priv = font_button->priv;
  
  if (!font_button->priv->font_dialog) 
    {
      CtkWidget *parent;
      
      parent = ctk_widget_get_toplevel (CTK_WIDGET (font_button));

      priv->font_dialog = ctk_font_chooser_dialog_new (priv->title, NULL);
      font_dialog = CTK_FONT_CHOOSER (font_button->priv->font_dialog);

      if (priv->font_map)
        ctk_font_chooser_set_font_map (font_dialog, priv->font_map);
      ctk_font_chooser_set_show_preview_entry (font_dialog, priv->show_preview_entry);
      ctk_font_chooser_set_level (CTK_FONT_CHOOSER (font_dialog), priv->level);
      ctk_font_chooser_set_language (CTK_FONT_CHOOSER (font_dialog), pango_language_to_string 
(priv->language));

      if (priv->preview_text)
        {
          ctk_font_chooser_set_preview_text (font_dialog, priv->preview_text);
          g_free (priv->preview_text);
          priv->preview_text = NULL;
        }

      if (priv->font_filter)
        {
          ctk_font_chooser_set_filter_func (font_dialog,
                                            priv->font_filter,
                                            priv->font_filter_data,
                                            priv->font_filter_data_destroy);
          priv->font_filter = NULL;
          priv->font_filter_data = NULL;
          priv->font_filter_data_destroy = NULL;
        }

      if (ctk_widget_is_toplevel (parent) && CTK_IS_WINDOW (parent))
        {
          if (CTK_WINDOW (parent) != ctk_window_get_transient_for (CTK_WINDOW (font_dialog)))
            ctk_window_set_transient_for (CTK_WINDOW (font_dialog), CTK_WINDOW (parent));

          ctk_window_set_modal (CTK_WINDOW (font_dialog),
                                ctk_window_get_modal (CTK_WINDOW (parent)));
        }

      g_signal_connect (font_dialog, "notify",
                        G_CALLBACK (ctk_font_button_font_chooser_notify), button);

      g_signal_connect (font_dialog, "response",
                        G_CALLBACK (response_cb), font_button);

      g_signal_connect (font_dialog, "destroy",
                        G_CALLBACK (dialog_destroy), font_button);

      g_signal_connect (font_dialog, "delete-event",
                        G_CALLBACK (ctk_widget_hide_on_delete), NULL);
    }
  
  if (!ctk_widget_get_visible (font_button->priv->font_dialog))
    {
      font_dialog = CTK_FONT_CHOOSER (font_button->priv->font_dialog);
      ctk_font_chooser_set_font_desc (font_dialog, font_button->priv->font_desc);
    } 

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_window_present (CTK_WINDOW (font_button->priv->font_dialog));
  G_GNUC_END_IGNORE_DEPRECATIONS
}


static void
response_cb (CtkDialog *dialog,
             gint       response_id,
             gpointer   data)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (data);
  CtkFontButtonPrivate *priv = font_button->priv;
  CtkFontChooser *font_chooser;
  GObject *object;

  ctk_widget_hide (font_button->priv->font_dialog);

  if (response_id != CTK_RESPONSE_OK)
    return;

  font_chooser = CTK_FONT_CHOOSER (priv->font_dialog);
  object = G_OBJECT (font_chooser);

  g_object_freeze_notify (object);

  clear_font_data (font_button);

  priv->font_desc = ctk_font_chooser_get_font_desc (font_chooser);
  if (priv->font_desc)
    priv->fontname = pango_font_description_to_string (priv->font_desc);
  priv->font_family = ctk_font_chooser_get_font_family (font_chooser);
  if (priv->font_family)
    g_object_ref (priv->font_family);
  priv->font_face = ctk_font_chooser_get_font_face (font_chooser);
  if (priv->font_face)
    g_object_ref (priv->font_face);
  priv->font_size = ctk_font_chooser_get_font_size (font_chooser);
  g_free (priv->font_features);
  priv->font_features = ctk_font_chooser_get_font_features (font_chooser);
  priv->language = pango_language_from_string (ctk_font_chooser_get_language (font_chooser));

  /* Set label font */
  ctk_font_button_update_font_info (font_button);

  g_object_notify (G_OBJECT (font_button), "font");
  g_object_notify (G_OBJECT (font_button), "font-desc");
  g_object_notify (G_OBJECT (font_button), "font-name");
  g_object_notify (G_OBJECT (font_button), "font-features");

  g_object_thaw_notify (object);

  /* Emit font_set signal */
  g_signal_emit (font_button, font_button_signals[FONT_SET], 0);
}

static void
dialog_destroy (CtkWidget *widget,
                gpointer   data)
{
  CtkFontButton *font_button = CTK_FONT_BUTTON (data);
    
  /* Dialog will get destroyed so reference is not valid now */
  font_button->priv->font_dialog = NULL;
} 

static gchar *
pango_font_description_to_css (PangoFontDescription *desc)
{
  GString *s;
  PangoFontMask set;

  s = g_string_new ("* { ");

  set = pango_font_description_get_set_fields (desc);
  if (set & PANGO_FONT_MASK_FAMILY)
    {
      g_string_append (s, "font-family: ");
      g_string_append (s, pango_font_description_get_family (desc));
      g_string_append (s, "; ");
    }
  if (set & PANGO_FONT_MASK_STYLE)
    {
      switch (pango_font_description_get_style (desc))
        {
        case PANGO_STYLE_NORMAL:
          g_string_append (s, "font-style: normal; ");
          break;
        case PANGO_STYLE_OBLIQUE:
          g_string_append (s, "font-style: oblique; ");
          break;
        case PANGO_STYLE_ITALIC:
          g_string_append (s, "font-style: italic; ");
          break;
        }
    }
  if (set & PANGO_FONT_MASK_VARIANT)
    {
      switch (pango_font_description_get_variant (desc))
        {
        case PANGO_VARIANT_NORMAL:
          g_string_append (s, "font-variant: normal; ");
          break;
        case PANGO_VARIANT_SMALL_CAPS:
          g_string_append (s, "font-variant: small-caps; ");
          break;
        }
    }
  if (set & PANGO_FONT_MASK_WEIGHT)
    {
      switch (pango_font_description_get_weight (desc))
        {
        case PANGO_WEIGHT_THIN:
          g_string_append (s, "font-weight: 100; ");
          break;
        case PANGO_WEIGHT_ULTRALIGHT:
          g_string_append (s, "font-weight: 200; ");
          break;
        case PANGO_WEIGHT_LIGHT:
        case PANGO_WEIGHT_SEMILIGHT:
          g_string_append (s, "font-weight: 300; ");
          break;
        case PANGO_WEIGHT_BOOK:
        case PANGO_WEIGHT_NORMAL:
          g_string_append (s, "font-weight: 400; ");
          break;
        case PANGO_WEIGHT_MEDIUM:
          g_string_append (s, "font-weight: 500; ");
          break;
        case PANGO_WEIGHT_SEMIBOLD:
          g_string_append (s, "font-weight: 600; ");
          break;
        case PANGO_WEIGHT_BOLD:
          g_string_append (s, "font-weight: 700; ");
          break;
        case PANGO_WEIGHT_ULTRABOLD:
          g_string_append (s, "font-weight: 800; ");
          break;
        case PANGO_WEIGHT_HEAVY:
        case PANGO_WEIGHT_ULTRAHEAVY:
          g_string_append (s, "font-weight: 900; ");
          break;
        }
    }
  if (set & PANGO_FONT_MASK_STRETCH)
    {
      switch (pango_font_description_get_stretch (desc))
        {
        case PANGO_STRETCH_ULTRA_CONDENSED:
          g_string_append (s, "font-stretch: ultra-condensed; ");
          break;
        case PANGO_STRETCH_EXTRA_CONDENSED:
          g_string_append (s, "font-stretch: extra-condensed; ");
          break;
        case PANGO_STRETCH_CONDENSED:
          g_string_append (s, "font-stretch: condensed; ");
          break;
        case PANGO_STRETCH_SEMI_CONDENSED:
          g_string_append (s, "font-stretch: semi-condensed; ");
          break;
        case PANGO_STRETCH_NORMAL:
          g_string_append (s, "font-stretch: normal; ");
          break;
        case PANGO_STRETCH_SEMI_EXPANDED:
          g_string_append (s, "font-stretch: semi-expanded; ");
          break;
        case PANGO_STRETCH_EXPANDED:
          g_string_append (s, "font-stretch: expanded; ");
          break;
        case PANGO_STRETCH_EXTRA_EXPANDED:
          g_string_append (s, "font-stretch: extra-expanded; ");
          break;
        case PANGO_STRETCH_ULTRA_EXPANDED:
          g_string_append (s, "font-stretch: ultra-expanded; ");
          break;
        }
    }
  if (set & PANGO_FONT_MASK_SIZE)
    {
      g_string_append_printf (s, "font-size: %dpt", pango_font_description_get_size (desc) / PANGO_SCALE);
    }

  g_string_append (s, "}");

  return g_string_free (s, FALSE);
}

static void
ctk_font_button_label_use_font (CtkFontButton *font_button)
{
  CtkFontButtonPrivate *priv = font_button->priv;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (priv->font_label);

  if (!priv->use_font)
    {
      if (priv->provider)
        {
          ctk_style_context_remove_provider (context, CTK_STYLE_PROVIDER (priv->provider));
          g_clear_object (&priv->provider);
        }
    }
  else
    {
      PangoFontDescription *desc;
      gchar *data;

      if (!priv->provider)
        {
          priv->provider = ctk_css_provider_new ();
          ctk_style_context_add_provider (context,
                                          CTK_STYLE_PROVIDER (priv->provider),
                                          CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }

      desc = pango_font_description_copy (priv->font_desc);

      if (!priv->use_size)
        pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);

      data = pango_font_description_to_css (desc);
      ctk_css_provider_load_from_data (priv->provider, data, -1, NULL);

      g_free (data);
      pango_font_description_free (desc);
    }
}

static void
ctk_font_button_update_font_info (CtkFontButton *font_button)
{
  CtkFontButtonPrivate *priv = font_button->priv;
  const gchar *fam_name;
  const gchar *face_name;
  gchar *family_style;

  if (priv->font_family)
    fam_name = pango_font_family_get_name (priv->font_family);
  else
    fam_name = C_("font", "None");
  if (priv->font_face)
    face_name = pango_font_face_get_face_name (priv->font_face);
  else
    face_name = "";

  if (priv->show_style)
    family_style = g_strconcat (fam_name, " ", face_name, NULL);
  else
    family_style = g_strdup (fam_name);

  ctk_label_set_text (CTK_LABEL (font_button->priv->font_label), family_style);
  g_free (family_style);

  if (font_button->priv->show_size) 
    {
      /* mirror Pango, which doesn't translate this either */
      gchar *size = g_strdup_printf ("%2.4g%s",
                                     pango_font_description_get_size (priv->font_desc) / (double)PANGO_SCALE,
                                     pango_font_description_get_size_is_absolute (priv->font_desc) ? "px" : "");
      
      ctk_label_set_text (CTK_LABEL (font_button->priv->size_label), size);
      
      g_free (size);
    }

  ctk_font_button_label_use_font (font_button);
} 

static void
ctk_font_button_set_level (CtkFontButton       *button,
                           CtkFontChooserLevel  level)
{
  CtkFontButtonPrivate *priv = button->priv;

  if (priv->level == level)
    return;

  priv->level = level;

  if (priv->font_dialog)
    g_object_set (priv->font_dialog, "level", level, NULL);

  g_object_set (button,
                "show-size", (level & CTK_FONT_CHOOSER_LEVEL_SIZE) != 0,
                "show-style", (level & CTK_FONT_CHOOSER_LEVEL_STYLE) != 0,
                NULL);

  g_object_notify (G_OBJECT (button), "level");
}

static void
ctk_font_button_set_language (CtkFontButton *button,
                              const char    *language)
{
  CtkFontButtonPrivate *priv = button->priv;

  priv->language = pango_language_from_string (language);

  if (priv->font_dialog)
    ctk_font_chooser_set_language (CTK_FONT_CHOOSER (priv->font_dialog), language);

  g_object_notify (G_OBJECT (button), "language");
}
