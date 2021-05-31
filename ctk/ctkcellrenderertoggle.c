/* gtkcellrenderertoggle.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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

#include "config.h"
#include <stdlib.h>
#include "gtkcellrenderertoggle.h"
#include "gtkintl.h"
#include "gtkmarshalers.h"
#include "gtkprivate.h"
#include "gtkstylecontextprivate.h"
#include "gtktreeprivate.h"
#include "a11y/gtkbooleancellaccessible.h"


/**
 * SECTION:gtkcellrenderertoggle
 * @Short_description: Renders a toggle button in a cell
 * @Title: GtkCellRendererToggle
 *
 * #GtkCellRendererToggle renders a toggle button in a cell. The
 * button is drawn as a radio or a checkbutton, depending on the
 * #GtkCellRendererToggle:radio property.
 * When activated, it emits the #GtkCellRendererToggle::toggled signal.
 */


static void ctk_cell_renderer_toggle_get_property  (GObject                    *object,
						    guint                       param_id,
						    GValue                     *value,
						    GParamSpec                 *pspec);
static void ctk_cell_renderer_toggle_set_property  (GObject                    *object,
						    guint                       param_id,
						    const GValue               *value,
						    GParamSpec                 *pspec);
static void ctk_cell_renderer_toggle_get_size   (GtkCellRenderer            *cell,
						 GtkWidget                  *widget,
						 const GdkRectangle         *cell_area,
						 gint                       *x_offset,
						 gint                       *y_offset,
						 gint                       *width,
						 gint                       *height);
static void ctk_cell_renderer_toggle_render     (GtkCellRenderer            *cell,
						 cairo_t                    *cr,
						 GtkWidget                  *widget,
						 const GdkRectangle         *background_area,
						 const GdkRectangle         *cell_area,
						 GtkCellRendererState        flags);
static gboolean ctk_cell_renderer_toggle_activate  (GtkCellRenderer            *cell,
						    GdkEvent                   *event,
						    GtkWidget                  *widget,
						    const gchar                *path,
						    const GdkRectangle         *background_area,
						    const GdkRectangle         *cell_area,
						    GtkCellRendererState        flags);


enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVATABLE,
  PROP_ACTIVE,
  PROP_RADIO,
  PROP_INCONSISTENT,
  PROP_INDICATOR_SIZE
};

#define TOGGLE_WIDTH 16

static guint toggle_cell_signals[LAST_SIGNAL] = { 0 };

struct _GtkCellRendererTogglePrivate
{
  gint indicator_size;

  guint active       : 1;
  guint activatable  : 1;
  guint inconsistent : 1;
  guint radio        : 1;
};


G_DEFINE_TYPE_WITH_PRIVATE (GtkCellRendererToggle, ctk_cell_renderer_toggle, CTK_TYPE_CELL_RENDERER)


static void
ctk_cell_renderer_toggle_init (GtkCellRendererToggle *celltoggle)
{
  GtkCellRendererTogglePrivate *priv;

  celltoggle->priv = ctk_cell_renderer_toggle_get_instance_private (celltoggle);
  priv = celltoggle->priv;

  priv->activatable = TRUE;
  priv->active = FALSE;
  priv->radio = FALSE;

  g_object_set (celltoggle, "mode", CTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
  ctk_cell_renderer_set_padding (CTK_CELL_RENDERER (celltoggle), 2, 2);

  priv->indicator_size = 0;
  priv->inconsistent = FALSE;
}

static void
ctk_cell_renderer_toggle_class_init (GtkCellRendererToggleClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkCellRendererClass *cell_class = CTK_CELL_RENDERER_CLASS (class);

  object_class->get_property = ctk_cell_renderer_toggle_get_property;
  object_class->set_property = ctk_cell_renderer_toggle_set_property;

  cell_class->get_size = ctk_cell_renderer_toggle_get_size;
  cell_class->render = ctk_cell_renderer_toggle_render;
  cell_class->activate = ctk_cell_renderer_toggle_activate;
  
  g_object_class_install_property (object_class,
				   PROP_ACTIVE,
				   g_param_spec_boolean ("active",
							 P_("Toggle state"),
							 P_("The toggle state of the button"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
		                   PROP_INCONSISTENT,
				   g_param_spec_boolean ("inconsistent",
					                 P_("Inconsistent state"),
							 P_("The inconsistent state of the button"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  g_object_class_install_property (object_class,
				   PROP_ACTIVATABLE,
				   g_param_spec_boolean ("activatable",
							 P_("Activatable"),
							 P_("The toggle button can be activated"),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_RADIO,
				   g_param_spec_boolean ("radio",
							 P_("Radio state"),
							 P_("Draw the toggle button as a radio button"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_INDICATOR_SIZE,
				   g_param_spec_int ("indicator-size",
						     P_("Indicator size"),
						     P_("Size of check or radio indicator"),
						     0,
						     G_MAXINT,
						     0,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED));

  
  /**
   * GtkCellRendererToggle::toggled:
   * @cell_renderer: the object which received the signal
   * @path: string representation of #GtkTreePath describing the 
   *        event location
   *
   * The ::toggled signal is emitted when the cell is toggled. 
   *
   * It is the responsibility of the application to update the model
   * with the correct value to store at @path.  Often this is simply the
   * opposite of the value currently stored at @path.
   **/
  toggle_cell_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkCellRendererToggleClass, toggled),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

  ctk_cell_renderer_class_set_accessible_type (cell_class, CTK_TYPE_BOOLEAN_CELL_ACCESSIBLE);
}

static void
ctk_cell_renderer_toggle_get_property (GObject     *object,
				       guint        param_id,
				       GValue      *value,
				       GParamSpec  *pspec)
{
  GtkCellRendererToggle *celltoggle = CTK_CELL_RENDERER_TOGGLE (object);
  GtkCellRendererTogglePrivate *priv = celltoggle->priv;

  switch (param_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->active);
      break;
    case PROP_INCONSISTENT:
      g_value_set_boolean (value, priv->inconsistent);
      break;
    case PROP_ACTIVATABLE:
      g_value_set_boolean (value, priv->activatable);
      break;
    case PROP_RADIO:
      g_value_set_boolean (value, priv->radio);
      break;
    case PROP_INDICATOR_SIZE:
      g_value_set_int (value, priv->indicator_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
ctk_cell_renderer_toggle_set_property (GObject      *object,
				       guint         param_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  GtkCellRendererToggle *celltoggle = CTK_CELL_RENDERER_TOGGLE (object);
  GtkCellRendererTogglePrivate *priv = celltoggle->priv;

  switch (param_id)
    {
    case PROP_ACTIVE:
      if (priv->active != g_value_get_boolean (value))
        {
          priv->active = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_INCONSISTENT:
      if (priv->inconsistent != g_value_get_boolean (value))
        {
          priv->inconsistent = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_ACTIVATABLE:
      if (priv->activatable != g_value_get_boolean (value))
        {
          priv->activatable = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_RADIO:
      if (priv->radio != g_value_get_boolean (value))
        {
          priv->radio = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_INDICATOR_SIZE:
      if (priv->indicator_size != g_value_get_int (value))
        {
          priv->indicator_size = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

/**
 * ctk_cell_renderer_toggle_new:
 *
 * Creates a new #GtkCellRendererToggle. Adjust rendering
 * parameters using object properties. Object properties can be set
 * globally (with g_object_set()). Also, with #GtkTreeViewColumn, you
 * can bind a property to a value in a #GtkTreeModel. For example, you
 * can bind the “active” property on the cell renderer to a boolean value
 * in the model, thus causing the check button to reflect the state of
 * the model.
 *
 * Returns: the new cell renderer
 **/
GtkCellRenderer *
ctk_cell_renderer_toggle_new (void)
{
  return g_object_new (CTK_TYPE_CELL_RENDERER_TOGGLE, NULL);
}

static GtkStyleContext *
ctk_cell_renderer_toggle_save_context (GtkCellRenderer *cell,
				       GtkWidget       *widget)
{
  GtkCellRendererTogglePrivate *priv = CTK_CELL_RENDERER_TOGGLE (cell)->priv;

  GtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  if (priv->radio)
    ctk_style_context_save_named (context, "radio");
  else
    ctk_style_context_save_named (context, "check");

  return context;
}
 
static void
calc_indicator_size (GtkStyleContext *context,
                     gint             indicator_size,
                     gint            *width,
                     gint            *height)
{
  if (indicator_size != 0)
    {
      *width = *height = indicator_size;
      return;
    }

  ctk_style_context_get (context, ctk_style_context_get_state (context),
                         "min-width", width,
                         "min-height", height,
                         NULL);

  if (*width == 0)
    *width = TOGGLE_WIDTH;
  if (*height == 0)
    *height = TOGGLE_WIDTH;
}

static void
ctk_cell_renderer_toggle_get_size (GtkCellRenderer    *cell,
				   GtkWidget          *widget,
				   const GdkRectangle *cell_area,
				   gint               *x_offset,
				   gint               *y_offset,
				   gint               *width,
				   gint               *height)
{
  GtkCellRendererTogglePrivate *priv;
  gint calc_width;
  gint calc_height;
  gint xpad, ypad;
  GtkStyleContext *context;
  GtkBorder border, padding;

  priv = CTK_CELL_RENDERER_TOGGLE (cell)->priv;

  ctk_cell_renderer_get_padding (cell, &xpad, &ypad);

  context = ctk_cell_renderer_toggle_save_context (cell, widget);
  ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &padding);
  ctk_style_context_get_border (context, ctk_style_context_get_state (context), &border);

  calc_indicator_size (context, priv->indicator_size, &calc_width, &calc_height);
  calc_width += xpad * 2 + padding.left + padding.right + border.left + border.right;
  calc_height += ypad * 2 + padding.top + padding.bottom + border.top + border.bottom;

  ctk_style_context_restore (context);

  if (width)
    *width = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
    {
      gfloat xalign, yalign;

      ctk_cell_renderer_get_alignment (cell, &xalign, &yalign);

      if (x_offset)
	{
	  *x_offset = ((ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL) ?
		       (1.0 - xalign) : xalign) * (cell_area->width - calc_width);
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = yalign * (cell_area->height - calc_height);
	  *y_offset = MAX (*y_offset, 0);
	}
    }
  else
    {
      if (x_offset) *x_offset = 0;
      if (y_offset) *y_offset = 0;
    }
}

static void
ctk_cell_renderer_toggle_render (GtkCellRenderer      *cell,
				 cairo_t              *cr,
				 GtkWidget            *widget,
				 const GdkRectangle   *background_area,
				 const GdkRectangle   *cell_area,
				 GtkCellRendererState  flags)
{
  GtkCellRendererToggle *celltoggle = CTK_CELL_RENDERER_TOGGLE (cell);
  GtkCellRendererTogglePrivate *priv = celltoggle->priv;
  GtkStyleContext *context;
  gint width, height;
  gint x_offset, y_offset;
  gint xpad, ypad;
  GtkStateFlags state;
  GtkBorder padding, border;

  context = ctk_widget_get_style_context (widget);
  ctk_cell_renderer_toggle_get_size (cell, widget, cell_area,
				     &x_offset, &y_offset,
				     &width, &height);
  ctk_cell_renderer_get_padding (cell, &xpad, &ypad);
  width -= xpad * 2;
  height -= ypad * 2;

  if (width <= 0 || height <= 0)
    return;

  state = ctk_cell_renderer_get_state (cell, widget, flags);

  if (!priv->activatable)
    state |= CTK_STATE_FLAG_INSENSITIVE;

  state &= ~(CTK_STATE_FLAG_INCONSISTENT | CTK_STATE_FLAG_CHECKED);

  if (priv->inconsistent)
    state |= CTK_STATE_FLAG_INCONSISTENT;
  
  if (priv->active)
    state |= CTK_STATE_FLAG_CHECKED;

  cairo_save (cr);

  gdk_cairo_rectangle (cr, cell_area);
  cairo_clip (cr);

  context = ctk_cell_renderer_toggle_save_context (cell, widget);
  ctk_style_context_set_state (context, state);

  ctk_render_background (context, cr,
                         cell_area->x + x_offset + xpad,
                         cell_area->y + y_offset + ypad,
                         width, height);
  ctk_render_frame (context, cr,
                    cell_area->x + x_offset + xpad,
                    cell_area->y + y_offset + ypad,
                    width, height);

  ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &padding);
  ctk_style_context_get_border (context, ctk_style_context_get_state (context), &border);

  if (priv->radio)
    {
      ctk_render_option (context, cr,
                         cell_area->x + x_offset + xpad + padding.left + border.left,
                         cell_area->y + y_offset + ypad + padding.top + border.top,
                         width - padding.left - padding.right - border.left - border.right,
                         height - padding.top - padding.bottom - border.top - border.bottom);
    }
  else
    {
      ctk_render_check (context, cr,
                        cell_area->x + x_offset + xpad + padding.left + border.left,
                        cell_area->y + y_offset + ypad + padding.top + border.top,
                        width - padding.left - padding.right - border.left - border.right,
                        height - padding.top - padding.bottom - border.top - border.bottom);
    }

  ctk_style_context_restore (context);
  cairo_restore (cr);
}

static gint
ctk_cell_renderer_toggle_activate (GtkCellRenderer      *cell,
				   GdkEvent             *event,
				   GtkWidget            *widget,
				   const gchar          *path,
				   const GdkRectangle   *background_area,
				   const GdkRectangle   *cell_area,
				   GtkCellRendererState  flags)
{
  GtkCellRendererTogglePrivate *priv;
  GtkCellRendererToggle *celltoggle;

  celltoggle = CTK_CELL_RENDERER_TOGGLE (cell);
  priv = celltoggle->priv;

  if (priv->activatable)
    {
      g_signal_emit (cell, toggle_cell_signals[TOGGLED], 0, path);
      return TRUE;
    }

  return FALSE;
}

/**
 * ctk_cell_renderer_toggle_set_radio:
 * @toggle: a #GtkCellRendererToggle
 * @radio: %TRUE to make the toggle look like a radio button
 * 
 * If @radio is %TRUE, the cell renderer renders a radio toggle
 * (i.e. a toggle in a group of mutually-exclusive toggles).
 * If %FALSE, it renders a check toggle (a standalone boolean option).
 * This can be set globally for the cell renderer, or changed just
 * before rendering each cell in the model (for #GtkTreeView, you set
 * up a per-row setting using #GtkTreeViewColumn to associate model
 * columns with cell renderer properties).
 **/
void
ctk_cell_renderer_toggle_set_radio (GtkCellRendererToggle *toggle,
				    gboolean               radio)
{
  GtkCellRendererTogglePrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER_TOGGLE (toggle));

  priv = toggle->priv;

  priv->radio = radio;
}

/**
 * ctk_cell_renderer_toggle_get_radio:
 * @toggle: a #GtkCellRendererToggle
 *
 * Returns whether we’re rendering radio toggles rather than checkboxes. 
 * 
 * Returns: %TRUE if we’re rendering radio toggles rather than checkboxes
 **/
gboolean
ctk_cell_renderer_toggle_get_radio (GtkCellRendererToggle *toggle)
{
  g_return_val_if_fail (CTK_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return toggle->priv->radio;
}

/**
 * ctk_cell_renderer_toggle_get_active:
 * @toggle: a #GtkCellRendererToggle
 *
 * Returns whether the cell renderer is active. See
 * ctk_cell_renderer_toggle_set_active().
 *
 * Returns: %TRUE if the cell renderer is active.
 **/
gboolean
ctk_cell_renderer_toggle_get_active (GtkCellRendererToggle *toggle)
{
  g_return_val_if_fail (CTK_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return toggle->priv->active;
}

/**
 * ctk_cell_renderer_toggle_set_active:
 * @toggle: a #GtkCellRendererToggle.
 * @setting: the value to set.
 *
 * Activates or deactivates a cell renderer.
 **/
void
ctk_cell_renderer_toggle_set_active (GtkCellRendererToggle *toggle,
				     gboolean               setting)
{
  g_return_if_fail (CTK_IS_CELL_RENDERER_TOGGLE (toggle));

  g_object_set (toggle, "active", setting ? TRUE : FALSE, NULL);
}

/**
 * ctk_cell_renderer_toggle_get_activatable:
 * @toggle: a #GtkCellRendererToggle
 *
 * Returns whether the cell renderer is activatable. See
 * ctk_cell_renderer_toggle_set_activatable().
 *
 * Returns: %TRUE if the cell renderer is activatable.
 *
 * Since: 2.18
 **/
gboolean
ctk_cell_renderer_toggle_get_activatable (GtkCellRendererToggle *toggle)
{
  g_return_val_if_fail (CTK_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return toggle->priv->activatable;
}

/**
 * ctk_cell_renderer_toggle_set_activatable:
 * @toggle: a #GtkCellRendererToggle.
 * @setting: the value to set.
 *
 * Makes the cell renderer activatable.
 *
 * Since: 2.18
 **/
void
ctk_cell_renderer_toggle_set_activatable (GtkCellRendererToggle *toggle,
                                          gboolean               setting)
{
  GtkCellRendererTogglePrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER_TOGGLE (toggle));

  priv = toggle->priv;

  if (priv->activatable != setting)
    {
      priv->activatable = setting ? TRUE : FALSE;
      g_object_notify (G_OBJECT (toggle), "activatable");
    }
}
