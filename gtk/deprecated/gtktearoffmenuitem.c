/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#include "gtkmenu.h"
#include "gtkmenuitemprivate.h"
#include "gtkrender.h"
#include "gtkstylecontext.h"
#include "gtktearoffmenuitem.h"
#include "gtkintl.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:gtktearoffmenuitem
 * @Short_description: A menu item used to tear off and reattach its menu
 * @Title: GtkTearoffMenuItem
 * @See_also: #GtkMenu
 *
 * A #GtkTearoffMenuItem is a special #GtkMenuItem which is used to
 * tear off and reattach its menu.
 *
 * When its menu is shown normally, the #GtkTearoffMenuItem is drawn as a
 * dotted line indicating that the menu can be torn off.  Activating it
 * causes its menu to be torn off and displayed in its own window
 * as a tearoff menu.
 *
 * When its menu is shown as a tearoff menu, the #GtkTearoffMenuItem is drawn
 * as a dotted line which has a left pointing arrow graphic indicating that
 * the tearoff menu can be reattached.  Activating it will erase the tearoff
 * menu window.
 *
 * > #GtkTearoffMenuItem is deprecated and should not be used in newly
 * > written code. Menus are not meant to be torn around.
 */


#define ARROW_SIZE 10
#define TEAR_LENGTH 5
#define BORDER_SPACING  3

struct _GtkTearoffMenuItemPrivate
{
  guint torn_off : 1;
};

static void ctk_tearoff_menu_item_get_preferred_width  (GtkWidget      *widget,
                                                        gint           *minimum,
                                                        gint           *natural);
static void ctk_tearoff_menu_item_get_preferred_height (GtkWidget      *widget,
                                                        gint           *minimum,
                                                        gint           *natural);
static gboolean ctk_tearoff_menu_item_draw             (GtkWidget      *widget,
                                                        cairo_t        *cr);
static void ctk_tearoff_menu_item_activate             (GtkMenuItem    *menu_item);
static void ctk_tearoff_menu_item_parent_set           (GtkWidget      *widget,
                                                        GtkWidget      *previous);

G_DEFINE_TYPE_WITH_PRIVATE (GtkTearoffMenuItem, ctk_tearoff_menu_item, CTK_TYPE_MENU_ITEM)

/**
 * ctk_tearoff_menu_item_new:
 *
 * Creates a new #GtkTearoffMenuItem.
 *
 * Returns: a new #GtkTearoffMenuItem.
 *
 * Deprecated: 3.4: #GtkTearoffMenuItem is deprecated and should not be
 *     used in newly written code.
 */
GtkWidget*
ctk_tearoff_menu_item_new (void)
{
  return g_object_new (CTK_TYPE_TEAROFF_MENU_ITEM, NULL);
}

static void
ctk_tearoff_menu_item_class_init (GtkTearoffMenuItemClass *klass)
{
  GtkWidgetClass *widget_class;
  GtkMenuItemClass *menu_item_class;

  widget_class = (GtkWidgetClass*) klass;
  menu_item_class = (GtkMenuItemClass*) klass;

  widget_class->draw = ctk_tearoff_menu_item_draw;
  widget_class->get_preferred_width = ctk_tearoff_menu_item_get_preferred_width;
  widget_class->get_preferred_height = ctk_tearoff_menu_item_get_preferred_height;
  widget_class->parent_set = ctk_tearoff_menu_item_parent_set;

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_TEAR_OFF_MENU_ITEM);

  menu_item_class->activate = ctk_tearoff_menu_item_activate;
}

static void
ctk_tearoff_menu_item_init (GtkTearoffMenuItem *self)
{
  self->priv = ctk_tearoff_menu_item_get_instance_private (self);
  self->priv->torn_off = FALSE;
}

static void
ctk_tearoff_menu_item_get_preferred_width (GtkWidget      *widget,
                                           gint           *minimum,
                                           gint           *natural)
{
  GtkStyleContext *context;
  guint border_width;
  GtkBorder padding;
  GtkStateFlags state;

  context = ctk_widget_get_style_context (widget);
  state = ctk_widget_get_state_flags (widget);

  ctk_style_context_get_padding (context, state, &padding);
  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  *minimum = *natural = (border_width + BORDER_SPACING) * 2 + padding.left + padding.right;
}

static void
ctk_tearoff_menu_item_get_preferred_height (GtkWidget      *widget,
                                            gint           *minimum,
                                            gint           *natural)
{
  GtkStyleContext *context;
  GtkBorder padding;
  GtkStateFlags state;
  GtkWidget *parent;
  guint border_width;

  context = ctk_widget_get_style_context (widget);
  state = ctk_widget_get_state_flags (widget);

  ctk_style_context_get_padding (context, state, &padding);
  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  *minimum = *natural = (border_width * 2) + padding.top + padding.bottom;

  parent = ctk_widget_get_parent (widget);
  if (CTK_IS_MENU (parent) && ctk_menu_get_tearoff_state (CTK_MENU (parent)))
    {
      *minimum += ARROW_SIZE;
      *natural += ARROW_SIZE;
    }
  else
    {
      *minimum += padding.top + 4;
      *natural += padding.top + 4;
    }
}

static gboolean
ctk_tearoff_menu_item_draw (GtkWidget *widget,
                            cairo_t   *cr)
{
  GtkMenuItem *menu_item;
  GtkStateFlags state;
  GtkStyleContext *context;
  GtkBorder padding;
  gint x, y, width, height;
  gint right_max;
  guint border_width;
  GtkTextDirection direction;
  GtkWidget *parent;
  gdouble angle;

  menu_item = CTK_MENU_ITEM (widget);
  context = ctk_widget_get_style_context (widget);
  direction = ctk_widget_get_direction (widget);
  state = ctk_widget_get_state_flags (widget);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (menu_item));
  x = border_width;
  y = border_width;
  width = ctk_widget_get_allocated_width (widget) - border_width * 2;
  height = ctk_widget_get_allocated_height (widget) - border_width * 2;
  right_max = x + width;

  ctk_style_context_save (context);
  ctk_style_context_set_state (context, state);
  ctk_style_context_get_padding (context, state, &padding);

  if (state & CTK_STATE_FLAG_PRELIGHT)
    {
      ctk_render_background (context, cr, x, y, width, height);
      ctk_render_frame (context, cr, x, y, width, height);
    }

  parent = ctk_widget_get_parent (widget);
  if (CTK_IS_MENU (parent) && ctk_menu_get_tearoff_state (CTK_MENU (parent)))
    {
      gint arrow_x;

      if (menu_item->priv->toggle_size > ARROW_SIZE)
        {
          if (direction == CTK_TEXT_DIR_LTR)
            {
              arrow_x = x + (menu_item->priv->toggle_size - ARROW_SIZE)/2;
              angle = (3 * G_PI) / 2;
            }
          else
            {
              arrow_x = x + width - menu_item->priv->toggle_size + (menu_item->priv->toggle_size - ARROW_SIZE)/2;
              angle = G_PI / 2;
            }
          x += menu_item->priv->toggle_size + BORDER_SPACING;
        }
      else
        {
          if (direction == CTK_TEXT_DIR_LTR)
            {
              arrow_x = ARROW_SIZE / 2;
              angle = (3 * G_PI) / 2;
            }
          else
            {
              arrow_x = x + width - 2 * ARROW_SIZE + ARROW_SIZE / 2;
              angle = G_PI / 2;
            }
          x += 2 * ARROW_SIZE;
        }

      ctk_render_arrow (context, cr, angle,
                        arrow_x, height / 2 - 5,
                        ARROW_SIZE);
    }

  while (x < right_max)
    {
      gint x1, x2;

      if (direction == CTK_TEXT_DIR_LTR)
        {
          x1 = x;
          x2 = MIN (x + TEAR_LENGTH, right_max);
        }
      else
        {
          x1 = right_max - x;
          x2 = MAX (right_max - x - TEAR_LENGTH, 0);
        }

      ctk_render_line (context, cr,
                       x1, y + (height - padding.bottom) / 2,
                       x2, y + (height - padding.bottom) / 2);
      x += 2 * TEAR_LENGTH;
    }

  ctk_style_context_restore (context);

  return FALSE;
}

static void
ctk_tearoff_menu_item_activate (GtkMenuItem *menu_item)
{
  GtkWidget *parent;

  parent = ctk_widget_get_parent (CTK_WIDGET (menu_item));
  if (CTK_IS_MENU (parent))
    {
      GtkMenu *menu = CTK_MENU (parent);

      ctk_widget_queue_resize (CTK_WIDGET (menu_item));
      ctk_menu_set_tearoff_state (menu, !ctk_menu_get_tearoff_state (menu));
    }
}

static void
tearoff_state_changed (GtkMenu            *menu,
                       GParamSpec         *pspec,
                       gpointer            data)
{
  GtkTearoffMenuItem *tearoff_menu_item = CTK_TEAROFF_MENU_ITEM (data);
  GtkTearoffMenuItemPrivate *priv = tearoff_menu_item->priv;

  priv->torn_off = ctk_menu_get_tearoff_state (menu);
}

static void
ctk_tearoff_menu_item_parent_set (GtkWidget *widget,
                                  GtkWidget *previous)
{
  GtkTearoffMenuItem *tearoff_menu_item = CTK_TEAROFF_MENU_ITEM (widget);
  GtkTearoffMenuItemPrivate *priv = tearoff_menu_item->priv;
  GtkMenu *menu;
  GtkWidget *parent;

  parent = ctk_widget_get_parent (widget);
  menu = CTK_IS_MENU (parent) ? CTK_MENU (parent) : NULL;

  if (previous)
    g_signal_handlers_disconnect_by_func (previous,
                                          tearoff_state_changed,
                                          tearoff_menu_item);

  if (menu)
    {
      priv->torn_off = ctk_menu_get_tearoff_state (menu);
      g_signal_connect (menu, "notify::tearoff-state",
                        G_CALLBACK (tearoff_state_changed),
                        tearoff_menu_item);
    }
}
