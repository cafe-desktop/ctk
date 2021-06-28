/* ctkshortcutlabel.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkshortcutlabel.h"
#include "ctklabel.h"
#include "ctkframe.h"
#include "ctkstylecontext.h"
#include "ctkprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctkshortcutlabel
 * @Title: CtkShortcutLabel
 * @Short_description: Displays a keyboard shortcut
 * @See_also: #CtkCellRendererAccel
 *
 * #CtkShortcutLabel is a widget that represents a single keyboard shortcut or gesture
 * in the user interface.
 */

struct _CtkShortcutLabel
{
  CtkBox  parent_instance;
  gchar  *accelerator;
  gchar  *disabled_text;
};

struct _CtkShortcutLabelClass
{
  CtkBoxClass parent_class;
};

G_DEFINE_TYPE (CtkShortcutLabel, ctk_shortcut_label, CTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_ACCELERATOR,
  PROP_DISABLED_TEXT,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static gchar *
get_modifier_label (guint key)
{
  const gchar *subscript;
  const gchar *label;

  switch (key)
    {
    case CDK_KEY_Shift_L:
    case CDK_KEY_Control_L:
    case CDK_KEY_Alt_L:
    case CDK_KEY_Meta_L:
    case CDK_KEY_Super_L:
    case CDK_KEY_Hyper_L:
      /* Translators: This string is used to mark left/right variants of modifier
       * keys in the shortcut window (e.g. Control_L vs Control_R). Please keep
       * this string very short, ideally just a single character, since it will
       * be rendered as part of the key.
       */
      subscript = C_("keyboard side marker", "L");
      break;
    case CDK_KEY_Shift_R:
    case CDK_KEY_Control_R:
    case CDK_KEY_Alt_R:
    case CDK_KEY_Meta_R:
    case CDK_KEY_Super_R:
    case CDK_KEY_Hyper_R:
      /* Translators: This string is used to mark left/right variants of modifier
       * keys in the shortcut window (e.g. Control_L vs Control_R). Please keep
       * this string very short, ideally just a single character, since it will
       * be rendered as part of the key.
       */
      subscript = C_("keyboard side marker", "R");
      break;
    default:
      g_assert_not_reached ();
   }

 switch (key)
   {
   case CDK_KEY_Shift_L:   case CDK_KEY_Shift_R:
     label = C_("keyboard label", "Shift");
     break;
   case CDK_KEY_Control_L: case CDK_KEY_Control_R:
     label = C_("keyboard label", "Ctrl");
     break;
   case CDK_KEY_Alt_L:     case CDK_KEY_Alt_R:
     label = C_("keyboard label", "Alt");
     break;
   case CDK_KEY_Meta_L:    case CDK_KEY_Meta_R:
     label = C_("keyboard label", "Meta");
     break;
   case CDK_KEY_Super_L:   case CDK_KEY_Super_R:
     label = C_("keyboard label", "Super");
     break;
   case CDK_KEY_Hyper_L:   case CDK_KEY_Hyper_R:
     label = C_("keyboard label", "Hyper");
     break;
    default:
      g_assert_not_reached ();
   }

  return g_strdup_printf ("%s <small><b>%s</b></small>", label, subscript);
}

static gchar **
get_labels (guint key, CdkModifierType modifier, guint *n_mods)
{
  const gchar *labels[16];
  GList *freeme = NULL;
  gchar key_label[6];
  gchar *tmp;
  gunichar ch;
  gint i = 0;
  gchar **retval;

  if (modifier & CDK_SHIFT_MASK)
    labels[i++] = C_("keyboard label", "Shift");
  if (modifier & CDK_CONTROL_MASK)
    labels[i++] = C_("keyboard label", "Ctrl");
  if (modifier & CDK_MOD1_MASK)
    labels[i++] = C_("keyboard label", "Alt");
  if (modifier & CDK_MOD2_MASK)
    labels[i++] = "Mod2";
  if (modifier & CDK_MOD3_MASK)
    labels[i++] = "Mod3";
  if (modifier & CDK_MOD4_MASK)
    labels[i++] = "Mod4";
  if (modifier & CDK_MOD5_MASK)
    labels[i++] = "Mod5";
  if (modifier & CDK_SUPER_MASK)
    labels[i++] = C_("keyboard label", "Super");
  if (modifier & CDK_HYPER_MASK)
    labels[i++] = C_("keyboard label", "Hyper");
  if (modifier & CDK_META_MASK)
    labels[i++] = C_("keyboard label", "Meta");

  *n_mods = i;

  ch = cdk_keyval_to_unicode (key);
  if (ch && ch < 0x80 && g_unichar_isgraph (ch))
    {
      switch (ch)
        {
        case '<':
          labels[i++] = "&lt;";
          break;
        case '>':
          labels[i++] = "&gt;";
          break;
        case '&':
          labels[i++] = "&amp;";
          break;
        case '"':
          labels[i++] = "&quot;";
          break;
        case '\'':
          labels[i++] = "&apos;";
          break;
        case '\\':
          labels[i++] = C_("keyboard label", "Backslash");
          break;
        default:
          memset (key_label, 0, 6);
          g_unichar_to_utf8 (g_unichar_toupper (ch), key_label);
          labels[i++] = key_label;
          break;
        }
    }
  else
    {
      switch (key)
        {
        case CDK_KEY_Shift_L:   case CDK_KEY_Shift_R:
        case CDK_KEY_Control_L: case CDK_KEY_Control_R:
        case CDK_KEY_Alt_L:     case CDK_KEY_Alt_R:
        case CDK_KEY_Meta_L:    case CDK_KEY_Meta_R:
        case CDK_KEY_Super_L:   case CDK_KEY_Super_R:
        case CDK_KEY_Hyper_L:   case CDK_KEY_Hyper_R:
          freeme = g_list_prepend (freeme, get_modifier_label (key));
          labels[i++] = (const gchar*)freeme->data;
           break;
        case CDK_KEY_Left:
          labels[i++] = "\xe2\x86\x90";
          break;
        case CDK_KEY_Up:
          labels[i++] = "\xe2\x86\x91";
          break;
        case CDK_KEY_Right:
          labels[i++] = "\xe2\x86\x92";
          break;
        case CDK_KEY_Down:
          labels[i++] = "\xe2\x86\x93";
          break;
        case CDK_KEY_space:
          labels[i++] = "\xe2\x90\xa3";
          break;
        case CDK_KEY_Return:
          labels[i++] = "\xe2\x8f\x8e";
          break;
        case CDK_KEY_Page_Up:
          labels[i++] = C_("keyboard label", "Page_Up");
          break;
        case CDK_KEY_Page_Down:
          labels[i++] = C_("keyboard label", "Page_Down");
          break;
        default:
          tmp = cdk_keyval_name (cdk_keyval_to_lower (key));
          if (tmp != NULL)
            {
              if (tmp[0] != 0 && tmp[1] == 0)
                {
                  key_label[0] = g_ascii_toupper (tmp[0]);
                  key_label[1] = '\0';
                  labels[i++] = key_label;
                }
              else
                {
                  labels[i++] = g_dpgettext2 (GETTEXT_PACKAGE, "keyboard label", tmp);
                }
            }
        }
    }

  labels[i] = NULL;

  retval = g_strdupv ((gchar **)labels);

  g_list_free_full (freeme, g_free);

  return retval;
}

static CtkWidget *
dim_label (const gchar *text)
{
  CtkWidget *label;

  label = ctk_label_new (text);
  ctk_widget_show (label);
  ctk_style_context_add_class (ctk_widget_get_style_context (label), "dim-label");

  return label;
}

static void
display_shortcut (CtkContainer    *self,
                  guint            key,
                  CdkModifierType  modifier)
{
  gchar **keys = NULL;
  gint i;
  guint n_mods;

  keys = get_labels (key, modifier, &n_mods);
  for (i = 0; keys[i]; i++)
    {
      CtkWidget *disp;

      if (i > 0)
        ctk_container_add (self, dim_label ("+"));

      disp = ctk_label_new (keys[i]);
      if (i < n_mods)
        ctk_widget_set_size_request (disp, 50, -1);

      ctk_style_context_add_class (ctk_widget_get_style_context (disp), "keycap");
      ctk_label_set_use_markup (CTK_LABEL (disp), TRUE);

      ctk_widget_show (disp);
      ctk_container_add (self, disp);
    }
  g_strfreev (keys);
}

static gboolean
parse_combination (CtkShortcutLabel *self,
                   const gchar      *str)
{
  gchar **accels;
  gint k;
  CdkModifierType modifier = 0;
  guint key = 0;
  gboolean retval = TRUE;

  accels = g_strsplit (str, "&", 0);
  for (k = 0; accels[k]; k++)
    {
      ctk_accelerator_parse (accels[k], &key, &modifier);
      if (key == 0 && modifier == 0)
        {
          retval = FALSE;
          break;
        }
      if (k > 0)
        ctk_container_add (CTK_CONTAINER (self), dim_label ("+"));

      display_shortcut (CTK_CONTAINER (self), key, modifier);
    }
  g_strfreev (accels);

  return retval;
}

static gboolean
parse_sequence (CtkShortcutLabel *self,
                const gchar      *str)
{
  gchar **accels;
  gint k;
  gboolean retval = TRUE;

  accels = g_strsplit (str, "+", 0);
  for (k = 0; accels[k]; k++)
    {
      if (!parse_combination (self, accels[k]))
        {
          retval = FALSE;
          break;
        }
    }

  g_strfreev (accels);

  return retval;
}

static gboolean
parse_range (CtkShortcutLabel *self,
             const gchar      *str)
{
  gchar *dots;

  dots = strstr (str, "...");
  if (!dots)
    return parse_sequence (self, str);

  dots[0] = '\0';
  if (!parse_sequence (self, str))
    return FALSE;

  ctk_container_add (CTK_CONTAINER (self), dim_label ("⋯"));

  if (!parse_sequence (self, dots + 3))
    return FALSE;

  return TRUE;
}

static void
ctk_shortcut_label_rebuild (CtkShortcutLabel *self)
{
  gchar **accels;
  gint k;

  ctk_container_foreach (CTK_CONTAINER (self), (CtkCallback)ctk_widget_destroy, NULL);

  if (self->accelerator == NULL || self->accelerator[0] == '\0')
    {
      CtkWidget *label;

      label = dim_label (self->disabled_text);
      ctk_widget_show (label);

      ctk_container_add (CTK_CONTAINER (self), label);
      return;
    }

  accels = g_strsplit (self->accelerator, " ", 0);
  for (k = 0; accels[k]; k++)
    {
      if (k > 0)
        ctk_container_add (CTK_CONTAINER (self), dim_label ("/"));

      if (!parse_range (self, accels[k]))
        {
          g_warning ("Failed to parse %s, part of accelerator '%s'", accels[k], self->accelerator);
          break;
        }
    }
  g_strfreev (accels);
}

static void
ctk_shortcut_label_finalize (GObject *object)
{
  CtkShortcutLabel *self = (CtkShortcutLabel *)object;

  g_free (self->accelerator);
  g_free (self->disabled_text);

  G_OBJECT_CLASS (ctk_shortcut_label_parent_class)->finalize (object);
}

static void
ctk_shortcut_label_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CtkShortcutLabel *self = CTK_SHORTCUT_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      g_value_set_string (value, ctk_shortcut_label_get_accelerator (self));
      break;

    case PROP_DISABLED_TEXT:
      g_value_set_string (value, ctk_shortcut_label_get_disabled_text (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_shortcut_label_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CtkShortcutLabel *self = CTK_SHORTCUT_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      ctk_shortcut_label_set_accelerator (self, g_value_get_string (value));
      break;

    case PROP_DISABLED_TEXT:
      ctk_shortcut_label_set_disabled_text (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_shortcut_label_class_init (CtkShortcutLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_shortcut_label_finalize;
  object_class->get_property = ctk_shortcut_label_get_property;
  object_class->set_property = ctk_shortcut_label_set_property;

  /**
   * CtkShortcutLabel:accelerator:
   *
   * The accelerator that @self displays. See #CtkShortcutsShortcut:accelerator
   * for the accepted syntax.
   *
   * Since: 3.22
   */
  properties[PROP_ACCELERATOR] =
    g_param_spec_string ("accelerator", P_("Accelerator"), P_("Accelerator"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutLabel:disabled-text:
   *
   * The text that is displayed when no accelerator is set.
   *
   * Since: 3.22
   */
  properties[PROP_DISABLED_TEXT] =
    g_param_spec_string ("disabled-text", P_("Disabled text"), P_("Disabled text"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
ctk_shortcut_label_init (CtkShortcutLabel *self)
{
  ctk_box_set_spacing (CTK_BOX (self), 6);

  /* Always use LTR so that modifiers are always left to the keyval */
  ctk_widget_set_direction (CTK_WIDGET (self), CTK_TEXT_DIR_LTR);
}

/**
 * ctk_shortcut_label_new:
 * @accelerator: the initial accelerator
 *
 * Creates a new #CtkShortcutLabel with @accelerator set.
 *
 * Returns: (transfer full): a newly-allocated #CtkShortcutLabel
 *
 * Since: 3.22
 */
CtkWidget *
ctk_shortcut_label_new (const gchar *accelerator)
{
  return g_object_new (CTK_TYPE_SHORTCUT_LABEL,
                       "accelerator", accelerator,
                       NULL);
}

/**
 * ctk_shortcut_label_get_accelerator:
 * @self: a #CtkShortcutLabel
 *
 * Retrieves the current accelerator of @self.
 *
 * Returns: (transfer none)(nullable): the current accelerator.
 *
 * Since: 3.22
 */
const gchar *
ctk_shortcut_label_get_accelerator (CtkShortcutLabel *self)
{
  g_return_val_if_fail (CTK_IS_SHORTCUT_LABEL (self), NULL);

  return self->accelerator;
}

/**
 * ctk_shortcut_label_set_accelerator:
 * @self: a #CtkShortcutLabel
 * @accelerator: the new accelerator
 *
 * Sets the accelerator to be displayed by @self.
 *
 * Since: 3.22
 */
void
ctk_shortcut_label_set_accelerator (CtkShortcutLabel *self,
                                    const gchar      *accelerator)
{
  g_return_if_fail (CTK_IS_SHORTCUT_LABEL (self));

  if (g_strcmp0 (accelerator, self->accelerator) != 0)
    {
      g_free (self->accelerator);
      self->accelerator = g_strdup (accelerator);
      ctk_shortcut_label_rebuild (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACCELERATOR]);
    }
}

/**
 * ctk_shortcut_label_get_disabled_text:
 * @self: a #CtkShortcutLabel
 *
 * Retrieves the text that is displayed when no accelerator is set.
 *
 * Returns: (transfer none)(nullable): the current text displayed when no
 * accelerator is set.
 *
 * Since: 3.22
 */
const gchar *
ctk_shortcut_label_get_disabled_text (CtkShortcutLabel *self)
{
  g_return_val_if_fail (CTK_IS_SHORTCUT_LABEL (self), NULL);

  return self->disabled_text;
}

/**
 * ctk_shortcut_label_set_disabled_text:
 * @self: a #CtkShortcutLabel
 * @disabled_text: the text to be displayed when no accelerator is set
 *
 * Sets the text to be displayed by @self when no accelerator is set.
 *
 * Since: 3.22
 */
void
ctk_shortcut_label_set_disabled_text (CtkShortcutLabel *self,
                                      const gchar      *disabled_text)
{
  g_return_if_fail (CTK_IS_SHORTCUT_LABEL (self));

  if (g_strcmp0 (disabled_text, self->disabled_text) != 0)
    {
      g_free (self->disabled_text);
      self->disabled_text = g_strdup (disabled_text);
      ctk_shortcut_label_rebuild (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DISABLED_TEXT]);
    }
}
