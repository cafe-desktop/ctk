/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 * Copyright (C) 2009 Bastien Nocera, David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

/*
 * Modified by the CTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkspinner.h"

#include "ctkimage.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkstylecontext.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidgetprivate.h"
#include "a11y/ctkspinneraccessible.h"
#include "ctkbuiltiniconprivate.h"


/**
 * SECTION:ctkspinner
 * @Short_description: Show a spinner animation
 * @Title: CtkSpinner
 * @See_also: #CtkCellRendererSpinner, #CtkProgressBar
 *
 * A CtkSpinner widget displays an icon-size spinning animation.
 * It is often used as an alternative to a #CtkProgressBar for
 * displaying indefinite activity, instead of actual progress.
 *
 * To start the animation, use ctk_spinner_start(), to stop it
 * use ctk_spinner_stop().
 *
 * # CSS nodes
 *
 * CtkSpinner has a single CSS node with the name spinner. When the animation is
 * active, the :checked pseudoclass is added to this node.
 */


enum {
  PROP_0,
  PROP_ACTIVE
};

struct _CtkSpinnerPrivate
{
  CtkCssGadget *gadget;
  gboolean active;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkSpinner, ctk_spinner, CTK_TYPE_WIDGET)

static void
ctk_spinner_finalize (GObject *object)
{
  CtkSpinner *spinner = CTK_SPINNER (object);

  g_clear_object (&spinner->priv->gadget);

  G_OBJECT_CLASS (ctk_spinner_parent_class)->finalize (object);
}

static void
ctk_spinner_get_preferred_width (CtkWidget *widget,
                                 gint      *minimum,
                                 gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SPINNER (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_spinner_get_preferred_height (CtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SPINNER (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_spinner_size_allocate (CtkWidget     *widget,
                           CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CTK_SPINNER (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static gboolean
ctk_spinner_draw (CtkWidget *widget,
                  cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_SPINNER (widget)->priv->gadget, cr);

  return FALSE;
}

static void
ctk_spinner_set_active (CtkSpinner *spinner,
                        gboolean    active)
{
  CtkSpinnerPrivate *priv = spinner->priv;

  active = !!active;

  if (priv->active != active)
    {
      priv->active = active;

      g_object_notify (G_OBJECT (spinner), "active");

      if (active)
        ctk_widget_set_state_flags (CTK_WIDGET (spinner),
                                    CTK_STATE_FLAG_CHECKED, FALSE);
      else
        ctk_widget_unset_state_flags (CTK_WIDGET (spinner),
                                      CTK_STATE_FLAG_CHECKED);
    }
}

static void
ctk_spinner_get_property (GObject    *object,
                          guint       param_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  CtkSpinnerPrivate *priv;

  priv = CTK_SPINNER (object)->priv;

  switch (param_id)
    {
      case PROP_ACTIVE:
        g_value_set_boolean (value, priv->active);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
ctk_spinner_set_property (GObject      *object,
                          guint         param_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  switch (param_id)
    {
      case PROP_ACTIVE:
        ctk_spinner_set_active (CTK_SPINNER (object), g_value_get_boolean (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
ctk_spinner_class_init (CtkSpinnerClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = ctk_spinner_finalize;
  gobject_class->get_property = ctk_spinner_get_property;
  gobject_class->set_property = ctk_spinner_set_property;

  widget_class = CTK_WIDGET_CLASS(klass);
  widget_class->size_allocate = ctk_spinner_size_allocate;
  widget_class->draw = ctk_spinner_draw;
  widget_class->get_preferred_width = ctk_spinner_get_preferred_width;
  widget_class->get_preferred_height = ctk_spinner_get_preferred_height;

  /* CtkSpinner:active:
   *
   * Whether the spinner is active
   *
   * Since: 2.20
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the spinner is active"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SPINNER_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "spinner");
}

static void
ctk_spinner_init (CtkSpinner *spinner)
{
  CtkCssNode *widget_node;

  spinner->priv = ctk_spinner_get_instance_private (spinner);

  ctk_widget_set_has_window (CTK_WIDGET (spinner), FALSE);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (spinner));
  spinner->priv->gadget = ctk_builtin_icon_new_for_node (widget_node, CTK_WIDGET (spinner));
  ctk_builtin_icon_set_image (CTK_BUILTIN_ICON (spinner->priv->gadget), CTK_CSS_IMAGE_BUILTIN_SPINNER);
  ctk_builtin_icon_set_default_size (CTK_BUILTIN_ICON (spinner->priv->gadget), 16);
}

/**
 * ctk_spinner_new:
 *
 * Returns a new spinner widget. Not yet started.
 *
 * Returns: a new #CtkSpinner
 *
 * Since: 2.20
 */
CtkWidget *
ctk_spinner_new (void)
{
  return g_object_new (CTK_TYPE_SPINNER, NULL);
}

/**
 * ctk_spinner_start:
 * @spinner: a #CtkSpinner
 *
 * Starts the animation of the spinner.
 *
 * Since: 2.20
 */
void
ctk_spinner_start (CtkSpinner *spinner)
{
  g_return_if_fail (CTK_IS_SPINNER (spinner));

  ctk_spinner_set_active (spinner, TRUE);
}

/**
 * ctk_spinner_stop:
 * @spinner: a #CtkSpinner
 *
 * Stops the animation of the spinner.
 *
 * Since: 2.20
 */
void
ctk_spinner_stop (CtkSpinner *spinner)
{
  g_return_if_fail (CTK_IS_SPINNER (spinner));

  ctk_spinner_set_active (spinner, FALSE);
}
