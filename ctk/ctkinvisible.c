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

#include "config.h"
#include <cdk/cdk.h>
#include "ctkinvisible.h"
#include "ctkwidgetprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"


/**
 * SECTION:ctkinvisible
 * @Short_description: A widget which is not displayed
 * @Title: CtkInvisible
 *
 * The #CtkInvisible widget is used internally in CTK+, and is probably not
 * very useful for application developers.
 *
 * It is used for reliable pointer grabs and selection handling in the code
 * for drag-and-drop.
 */


struct _CtkInvisiblePrivate
{
  CdkScreen    *screen;
  gboolean      has_user_ref_count;
};

enum {
  PROP_0,
  PROP_SCREEN,
  LAST_ARG
};

static void ctk_invisible_destroy       (CtkWidget         *widget);
static void ctk_invisible_realize       (CtkWidget         *widget);
static void ctk_invisible_style_updated (CtkWidget         *widget);
static void ctk_invisible_show          (CtkWidget         *widget);
static void ctk_invisible_size_allocate (CtkWidget         *widget,
					 CtkAllocation     *allocation);
static void ctk_invisible_set_property  (GObject           *object,
					 guint              prop_id,
					 const GValue      *value,
					 GParamSpec        *pspec);
static void ctk_invisible_get_property  (GObject           *object,
					 guint              prop_id,
					 GValue		   *value,
					 GParamSpec        *pspec);
static void ctk_invisible_constructed   (GObject           *object);

G_DEFINE_TYPE_WITH_PRIVATE (CtkInvisible, ctk_invisible, CTK_TYPE_WIDGET)

static void
ctk_invisible_class_init (CtkInvisibleClass *class)
{
  GObjectClass	 *gobject_class;
  CtkWidgetClass *widget_class;

  widget_class = (CtkWidgetClass*) class;
  gobject_class = (GObjectClass*) class;

  widget_class->realize = ctk_invisible_realize;
  widget_class->style_updated = ctk_invisible_style_updated;
  widget_class->show = ctk_invisible_show;
  widget_class->size_allocate = ctk_invisible_size_allocate;
  widget_class->destroy = ctk_invisible_destroy;

  gobject_class->set_property = ctk_invisible_set_property;
  gobject_class->get_property = ctk_invisible_get_property;
  gobject_class->constructed = ctk_invisible_constructed;

  g_object_class_install_property (gobject_class,
				   PROP_SCREEN,
				   g_param_spec_object ("screen",
 							P_("Screen"),
 							P_("The screen where this window will be displayed"),
							GDK_TYPE_SCREEN,
 							CTK_PARAM_READWRITE));
}

static void
ctk_invisible_init (CtkInvisible *invisible)
{
  CtkInvisiblePrivate *priv;

  invisible->priv = ctk_invisible_get_instance_private (invisible);
  priv = invisible->priv;

  ctk_widget_set_has_window (CTK_WIDGET (invisible), TRUE);
  _ctk_widget_set_is_toplevel (CTK_WIDGET (invisible), TRUE);

  g_object_ref_sink (invisible);

  priv->has_user_ref_count = TRUE;
  priv->screen = cdk_screen_get_default ();
}

static void
ctk_invisible_destroy (CtkWidget *widget)
{
  CtkInvisible *invisible = CTK_INVISIBLE (widget);
  CtkInvisiblePrivate *priv = invisible->priv;

  if (priv->has_user_ref_count)
    {
      priv->has_user_ref_count = FALSE;
      g_object_unref (invisible);
    }

  CTK_WIDGET_CLASS (ctk_invisible_parent_class)->destroy (widget);
}

/**
 * ctk_invisible_new_for_screen:
 * @screen: a #CdkScreen which identifies on which
 *	    the new #CtkInvisible will be created.
 *
 * Creates a new #CtkInvisible object for a specified screen
 *
 * Returns: a newly created #CtkInvisible object
 *
 * Since: 2.2
 **/
CtkWidget* 
ctk_invisible_new_for_screen (CdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  
  return g_object_new (CTK_TYPE_INVISIBLE, "screen", screen, NULL);
}

/**
 * ctk_invisible_new:
 * 
 * Creates a new #CtkInvisible.
 * 
 * Returns: a new #CtkInvisible.
 **/
CtkWidget*
ctk_invisible_new (void)
{
  return g_object_new (CTK_TYPE_INVISIBLE, NULL);
}

/**
 * ctk_invisible_set_screen:
 * @invisible: a #CtkInvisible.
 * @screen: a #CdkScreen.
 *
 * Sets the #CdkScreen where the #CtkInvisible object will be displayed.
 *
 * Since: 2.2
 **/ 
void
ctk_invisible_set_screen (CtkInvisible *invisible,
			  CdkScreen    *screen)
{
  CtkInvisiblePrivate *priv;
  CtkWidget *widget;
  CdkScreen *previous_screen;
  gboolean was_realized;

  g_return_if_fail (CTK_IS_INVISIBLE (invisible));
  g_return_if_fail (GDK_IS_SCREEN (screen));

  priv = invisible->priv;

  if (screen == priv->screen)
    return;

  widget = CTK_WIDGET (invisible);

  previous_screen = priv->screen;
  was_realized = ctk_widget_get_realized (widget);

  if (was_realized)
    ctk_widget_unrealize (widget);

  priv->screen = screen;
  if (screen != previous_screen)
    _ctk_widget_propagate_screen_changed (widget, previous_screen);
  g_object_notify (G_OBJECT (invisible), "screen");
  
  if (was_realized)
    ctk_widget_realize (widget);
}

/**
 * ctk_invisible_get_screen:
 * @invisible: a #CtkInvisible.
 *
 * Returns the #CdkScreen object associated with @invisible
 *
 * Returns: (transfer none): the associated #CdkScreen.
 *
 * Since: 2.2
 **/
CdkScreen *
ctk_invisible_get_screen (CtkInvisible *invisible)
{
  g_return_val_if_fail (CTK_IS_INVISIBLE (invisible), NULL);

  return invisible->priv->screen;
}

static void
ctk_invisible_realize (CtkWidget *widget)
{
  CdkWindow *parent;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  parent = ctk_widget_get_parent_window (widget);
  if (parent == NULL)
    parent = cdk_screen_get_root_window (ctk_widget_get_screen (widget));

  attributes.x = -100;
  attributes.y = -100;
  attributes.width = 10;
  attributes.height = 10;
  attributes.window_type = GDK_WINDOW_TEMP;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.override_redirect = TRUE;
  attributes.event_mask = ctk_widget_get_events (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_NOREDIR;

  window = cdk_window_new (parent, &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);
}

static void
ctk_invisible_style_updated (CtkWidget *widget)
{
  /* Don't chain up to parent implementation */
}

static void
ctk_invisible_show (CtkWidget *widget)
{
  _ctk_widget_set_visible_flag (widget, TRUE);
  ctk_widget_map (widget);
}

static void
ctk_invisible_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
  ctk_widget_set_allocation (widget, allocation);
}


static void 
ctk_invisible_set_property  (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  CtkInvisible *invisible = CTK_INVISIBLE (object);
  
  switch (prop_id)
    {
    case PROP_SCREEN:
      ctk_invisible_set_screen (invisible, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
ctk_invisible_get_property  (GObject      *object,
			     guint         prop_id,
			     GValue	  *value,
			     GParamSpec   *pspec)
{
  CtkInvisible *invisible = CTK_INVISIBLE (object);
  CtkInvisiblePrivate *priv = invisible->priv;

  switch (prop_id)
    {
    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* We use a constructor here so that we can realize the invisible on
 * the correct screen after the “screen” property has been set
 */
static void
ctk_invisible_constructed (GObject *object)
{
  G_OBJECT_CLASS (ctk_invisible_parent_class)->constructed (object);

  ctk_widget_realize (CTK_WIDGET (object));
}
