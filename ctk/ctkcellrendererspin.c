/* CtkCellRendererSpin
 * Copyright (C) 2004 Lorenzo Gil Sanchez
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
 *
 * Authors: Lorenzo Gil Sanchez    <lgs@sicem.biz>
 *          Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#include "config.h"

#include "ctkcellrendererspin.h"

#include "ctkadjustment.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkspinbutton.h"


/**
 * SECTION:ctkcellrendererspin
 * @Short_description: Renders a spin button in a cell
 * @Title: CtkCellRendererSpin
 * @See_also: #CtkCellRendererText, #CtkSpinButton
 *
 * #CtkCellRendererSpin renders text in a cell like #CtkCellRendererText from
 * which it is derived. But while #CtkCellRendererText offers a simple entry to
 * edit the text, #CtkCellRendererSpin offers a #CtkSpinButton widget. Of course,
 * that means that the text has to be parseable as a floating point number.
 *
 * The range of the spinbutton is taken from the adjustment property of the
 * cell renderer, which can be set explicitly or mapped to a column in the
 * tree model, like all properties of cell renders. #CtkCellRendererSpin
 * also has properties for the #CtkCellRendererSpin:climb-rate and the number
 * of #CtkCellRendererSpin:digits to display. Other #CtkSpinButton properties
 * can be set in a handler for the #CtkCellRenderer::editing-started signal.
 *
 * The #CtkCellRendererSpin cell renderer was added in GTK+ 2.10.
 */


struct _CtkCellRendererSpinPrivate
{
  CtkAdjustment *adjustment;
  gdouble climb_rate;
  guint   digits;
};

static void ctk_cell_renderer_spin_finalize   (GObject                  *object);

static void ctk_cell_renderer_spin_get_property (GObject      *object,
						 guint         prop_id,
						 GValue       *value,
						 GParamSpec   *spec);
static void ctk_cell_renderer_spin_set_property (GObject      *object,
						 guint         prop_id,
						 const GValue *value,
						 GParamSpec   *spec);

static CtkCellEditable * ctk_cell_renderer_spin_start_editing (CtkCellRenderer     *cell,
							       GdkEvent            *event,
							       CtkWidget           *widget,
							       const gchar         *path,
							       const GdkRectangle  *background_area,
							       const GdkRectangle  *cell_area,
							       CtkCellRendererState flags);
enum {
  PROP_0,
  PROP_ADJUSTMENT,
  PROP_CLIMB_RATE,
  PROP_DIGITS
};

#define CTK_CELL_RENDERER_SPIN_PATH "ctk-cell-renderer-spin-path"

G_DEFINE_TYPE_WITH_PRIVATE (CtkCellRendererSpin, ctk_cell_renderer_spin, CTK_TYPE_CELL_RENDERER_TEXT)


static void
ctk_cell_renderer_spin_class_init (CtkCellRendererSpinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkCellRendererClass *cell_class = CTK_CELL_RENDERER_CLASS (klass);

  object_class->finalize     = ctk_cell_renderer_spin_finalize;
  object_class->get_property = ctk_cell_renderer_spin_get_property;
  object_class->set_property = ctk_cell_renderer_spin_set_property;

  cell_class->start_editing  = ctk_cell_renderer_spin_start_editing;

  /**
   * CtkCellRendererSpin:adjustment:
   *
   * The adjustment that holds the value of the spinbutton. 
   * This must be non-%NULL for the cell renderer to be editable.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
				   PROP_ADJUSTMENT,
				   g_param_spec_object ("adjustment",
							P_("Adjustment"),
							P_("The adjustment that holds the value of the spin button"),
							CTK_TYPE_ADJUSTMENT,
							CTK_PARAM_READWRITE));


  /**
   * CtkCellRendererSpin:climb-rate:
   *
   * The acceleration rate when you hold down a button.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
				   PROP_CLIMB_RATE,
				   g_param_spec_double ("climb-rate",
							P_("Climb rate"),
							P_("The acceleration rate when you hold down a button"),
							0.0, G_MAXDOUBLE, 0.0,
							CTK_PARAM_READWRITE));  
  /**
   * CtkCellRendererSpin:digits:
   *
   * The number of decimal places to display.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
				   PROP_DIGITS,
				   g_param_spec_uint ("digits",
						      P_("Digits"),
						      P_("The number of decimal places to display"),
						      0, 20, 0,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY)); 
}

static void
ctk_cell_renderer_spin_init (CtkCellRendererSpin *self)
{
  CtkCellRendererSpinPrivate *priv;

  self->priv = ctk_cell_renderer_spin_get_instance_private (self);
  priv = self->priv;

  priv->adjustment = NULL;
  priv->climb_rate = 0.0;
  priv->digits = 0;
}

static void
ctk_cell_renderer_spin_finalize (GObject *object)
{
  CtkCellRendererSpinPrivate *priv;

  priv = CTK_CELL_RENDERER_SPIN (object)->priv;

  if (priv && priv->adjustment)
    g_object_unref (priv->adjustment);

  G_OBJECT_CLASS (ctk_cell_renderer_spin_parent_class)->finalize (object);
}

static void
ctk_cell_renderer_spin_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec)
{
  CtkCellRendererSpin *renderer;
  CtkCellRendererSpinPrivate *priv;

  renderer = CTK_CELL_RENDERER_SPIN (object);
  priv = renderer->priv;

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, priv->adjustment);
      break;
    case PROP_CLIMB_RATE:
      g_value_set_double (value, priv->climb_rate);
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, priv->digits);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_cell_renderer_spin_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  CtkCellRendererSpin *renderer;
  CtkCellRendererSpinPrivate *priv;
  GObject *obj;

  renderer = CTK_CELL_RENDERER_SPIN (object);
  priv = renderer->priv;

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      obj = g_value_get_object (value);

      if (priv->adjustment)
	{
	  g_object_unref (priv->adjustment);
	  priv->adjustment = NULL;
	}

      if (obj)
	priv->adjustment = CTK_ADJUSTMENT (g_object_ref_sink (obj));
      break;
    case PROP_CLIMB_RATE:
      priv->climb_rate = g_value_get_double (value);
      break;
    case PROP_DIGITS:
      if (priv->digits != g_value_get_uint (value))
        {
          priv->digits = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
ctk_cell_renderer_spin_focus_out_event (CtkWidget *widget,
					GdkEvent  *event,
					gpointer   data)
{
  const gchar *path;
  const gchar *new_text;
  gboolean canceled;

  g_object_get (widget,
                "editing-canceled", &canceled,
                NULL);

  g_signal_handlers_disconnect_by_func (widget,
					ctk_cell_renderer_spin_focus_out_event,
					data);

  ctk_cell_renderer_stop_editing (CTK_CELL_RENDERER (data), canceled);

  if (!canceled)
    {
      path = g_object_get_data (G_OBJECT (widget), CTK_CELL_RENDERER_SPIN_PATH);

      new_text = ctk_entry_get_text (CTK_ENTRY (widget));
      g_signal_emit_by_name (data, "edited", path, new_text);
    }
  
  return FALSE;
}

static gboolean
ctk_cell_renderer_spin_key_press_event (CtkWidget   *widget,
					GdkEventKey *event,
					gpointer     data)
{
  if (event->state == 0)
    {
      if (event->keyval == GDK_KEY_Up)
	{
	  ctk_spin_button_spin (CTK_SPIN_BUTTON (widget), CTK_SPIN_STEP_FORWARD, 1);
	  return TRUE;
	}
      else if (event->keyval == GDK_KEY_Down)
	{
	  ctk_spin_button_spin (CTK_SPIN_BUTTON (widget), CTK_SPIN_STEP_BACKWARD, 1);
	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
ctk_cell_renderer_spin_button_press_event (CtkWidget      *widget,
                                           GdkEventButton *event,
                                           gpointer        user_data)
{
  /* Block 2BUTTON and 3BUTTON here, so that they won't be eaten
   * by tree view.
   */
  if (event->type == GDK_2BUTTON_PRESS
      || event->type == GDK_3BUTTON_PRESS)
    return TRUE;

  return FALSE;
}

static CtkCellEditable *
ctk_cell_renderer_spin_start_editing (CtkCellRenderer      *cell,
				      GdkEvent             *event,
				      CtkWidget            *widget,
				      const gchar          *path,
				      const GdkRectangle   *background_area,
				      const GdkRectangle   *cell_area,
				      CtkCellRendererState  flags)
{
  CtkCellRendererSpinPrivate *priv;
  CtkCellRendererText *cell_text;
  CtkWidget *spin;
  gboolean editable;
  gchar *text;

  cell_text = CTK_CELL_RENDERER_TEXT (cell);
  priv = CTK_CELL_RENDERER_SPIN (cell)->priv;

  g_object_get (cell_text, "editable", &editable, NULL);
  if (!editable)
    return NULL;

  if (!priv->adjustment)
    return NULL;

  spin = ctk_spin_button_new (priv->adjustment,
			      priv->climb_rate, priv->digits);

  g_signal_connect (spin, "button-press-event",
                    G_CALLBACK (ctk_cell_renderer_spin_button_press_event),
                    NULL);

  g_object_get (cell_text, "text", &text, NULL);
  if (text)
    {
      ctk_spin_button_set_value (CTK_SPIN_BUTTON (spin),
                                 g_strtod (text, NULL));
      g_free (text);
    }

  g_object_set_data_full (G_OBJECT (spin), CTK_CELL_RENDERER_SPIN_PATH,
			  g_strdup (path), g_free);

  g_signal_connect (G_OBJECT (spin), "focus-out-event",
		    G_CALLBACK (ctk_cell_renderer_spin_focus_out_event),
		    cell);
  g_signal_connect (G_OBJECT (spin), "key-press-event",
		    G_CALLBACK (ctk_cell_renderer_spin_key_press_event),
		    cell);

  ctk_widget_show (spin);

  return CTK_CELL_EDITABLE (spin);
}

/**
 * ctk_cell_renderer_spin_new:
 *
 * Creates a new #CtkCellRendererSpin. 
 *
 * Returns: a new #CtkCellRendererSpin
 *
 * Since: 2.10
 */
CtkCellRenderer *
ctk_cell_renderer_spin_new (void)
{
  return g_object_new (CTK_TYPE_CELL_RENDERER_SPIN, NULL);
}
