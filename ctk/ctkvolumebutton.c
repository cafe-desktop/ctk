/* CTK - The GIMP Toolkit
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
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
 * Modified by the CTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkvolumebutton.h"

#include "ctkadjustment.h"
#include "ctkintl.h"
#include "ctktooltip.h"


/**
 * SECTION:ctkvolumebutton
 * @Short_description: A button which pops up a volume control
 * @Title: CtkVolumeButton
 *
 * #CtkVolumeButton is a subclass of #CtkScaleButton that has
 * been tailored for use as a volume control widget with suitable
 * icons, tooltips and accessible labels.
 */

#define EPSILON (1e-10)

static const gchar * const icons[] =
{
  "audio-volume-muted",
  "audio-volume-high",
  "audio-volume-low",
  "audio-volume-medium",
  NULL
};

static const gchar * const icons_symbolic[] =
{
  "audio-volume-muted-symbolic",
  "audio-volume-high-symbolic",
  "audio-volume-low-symbolic",
  "audio-volume-medium-symbolic",
  NULL
};

enum
{
  PROP_0,
  PROP_SYMBOLIC
};

static gboolean	cb_query_tooltip (CtkWidget       *button,
                                  gint             x,
                                  gint             y,
                                  gboolean         keyboard_mode,
                                  CtkTooltip      *tooltip,
                                  gpointer         user_data);
static void	cb_value_changed (CtkVolumeButton *button,
                                  gdouble          value,
                                  gpointer         user_data);

G_DEFINE_TYPE (CtkVolumeButton, ctk_volume_button, CTK_TYPE_SCALE_BUTTON)

static gboolean
get_symbolic (CtkScaleButton *button)
{
  gchar **icon_list;
  gboolean ret;

  g_object_get (button, "icons", &icon_list, NULL);
  if (icon_list != NULL &&
      icon_list[0] != NULL &&
      g_str_equal (icon_list[0], icons_symbolic[0]))
    ret = TRUE;
  else
    ret = FALSE;
  g_strfreev (icon_list);

  return ret;
}

static void
ctk_volume_button_set_property (GObject       *object,
				guint          prop_id,
				const GValue  *value,
				GParamSpec    *pspec)
{
  CtkScaleButton *button = CTK_SCALE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_SYMBOLIC:
      if (get_symbolic (button) != g_value_get_boolean (value))
        {
          if (g_value_get_boolean (value))
            ctk_scale_button_set_icons (button, (const char **) icons_symbolic);
          else
	    ctk_scale_button_set_icons (button, (const char **) icons);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_volume_button_get_property (GObject     *object,
			        guint        prop_id,
			        GValue      *value,
			        GParamSpec  *pspec)
{
  switch (prop_id)
    {
    case PROP_SYMBOLIC:
      g_value_set_boolean (value, get_symbolic (CTK_SCALE_BUTTON (object)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_volume_button_class_init (CtkVolumeButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  gobject_class->set_property = ctk_volume_button_set_property;
  gobject_class->get_property = ctk_volume_button_get_property;

  /**
   * CtkVolumeButton:use-symbolic:
   *
   * Whether to use symbolic icons as the icons. Note that
   * if the symbolic icons are not available in your installed
   * theme, then the normal (potentially colorful) icons will
   * be used.
   *
   * Since: 3.0
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SYMBOLIC,
                                   g_param_spec_boolean ("use-symbolic",
                                                         P_("Use symbolic icons"),
                                                         P_("Whether to use symbolic icons"),
                                                         TRUE,
                                                         G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkvolumebutton.ui");
  ctk_widget_class_bind_template_callback (widget_class, cb_query_tooltip);
  ctk_widget_class_bind_template_callback (widget_class, cb_value_changed);
}

static void
ctk_volume_button_init (CtkVolumeButton *button)
{
  CtkWidget *widget = CTK_WIDGET (button);

  ctk_widget_init_template (widget);

  /* The atk action description is not supported by CtkBuilder */
  atk_action_set_description (ATK_ACTION (ctk_widget_get_accessible (CTK_WIDGET (widget))),
			      1, _("Adjusts the volume"));
}

/**
 * ctk_volume_button_new:
 *
 * Creates a #CtkVolumeButton, with a range between 0.0 and 1.0, with
 * a stepping of 0.02. Volume values can be obtained and modified using
 * the functions from #CtkScaleButton.
 *
 * Returns: a new #CtkVolumeButton
 *
 * Since: 2.12
 */
CtkWidget *
ctk_volume_button_new (void)
{
  GObject *button;
  button = g_object_new (CTK_TYPE_VOLUME_BUTTON, NULL);
  return CTK_WIDGET (button);
}

static gboolean
cb_query_tooltip (CtkWidget  *button,
		  gint        x G_GNUC_UNUSED,
		  gint        y G_GNUC_UNUSED,
		  gboolean    keyboard_mode G_GNUC_UNUSED,
		  CtkTooltip *tooltip,
		  gpointer    user_data G_GNUC_UNUSED)
{
  CtkScaleButton *scale_button = CTK_SCALE_BUTTON (button);
  CtkAdjustment *adjustment;
  gdouble val;
  char *str;
  AtkImage *image;

  image = ATK_IMAGE (ctk_widget_get_accessible (button));

  adjustment = ctk_scale_button_get_adjustment (scale_button);
  val = ctk_scale_button_get_value (scale_button);

  if (val < (ctk_adjustment_get_lower (adjustment) + EPSILON))
    {
      str = g_strdup (_("Muted"));
    }
  else if (val >= (ctk_adjustment_get_upper (adjustment) - EPSILON))
    {
      str = g_strdup (_("Full Volume"));
    }
  else
    {
      int percent;

      percent = (int) (100. * val / (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_lower (adjustment)) + .5);

      /* Translators: this is the percentage of the current volume,
       * as used in the tooltip, eg. "49 %".
       * Translate the "%d" to "%Id" if you want to use localised digits,
       * or otherwise translate the "%d" to "%d".
       */
      str = g_strdup_printf (C_("volume percentage", "%d %%"), percent);
    }

  ctk_tooltip_set_text (tooltip, str);
  atk_image_set_image_description (image, str);
  g_free (str);

  return TRUE;
}

static void
cb_value_changed (CtkVolumeButton *button,
		  gdouble          value G_GNUC_UNUSED,
		  gpointer         user_data G_GNUC_UNUSED)
{
  ctk_widget_trigger_tooltip_query (CTK_WIDGET (button));
}
