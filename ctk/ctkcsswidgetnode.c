/* GTK - The GIMP Toolkit
 * Copyright (C) 2014 Benjamin Otte <otte@gnome.org>
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

#include "gtkcsswidgetnodeprivate.h"

#include "gtkcontainerprivate.h"
#include "gtkcssanimatedstyleprivate.h"
#include "gtkprivate.h"
#include "gtksettingsprivate.h"
#include "gtkstylecontextprivate.h"
#include "gtkwidgetprivate.h"
/* widgets for special casing go here */
#include "gtkbox.h"

G_DEFINE_TYPE (GtkCssWidgetNode, ctk_css_widget_node, CTK_TYPE_CSS_NODE)

static void
ctk_css_widget_node_finalize (GObject *object)
{
  GtkCssWidgetNode *node = CTK_CSS_WIDGET_NODE (object);

  g_object_unref (node->last_updated_style);

  G_OBJECT_CLASS (ctk_css_widget_node_parent_class)->finalize (object);
}

static void
ctk_css_widget_node_style_changed (GtkCssNode        *cssnode,
                                   GtkCssStyleChange *change)
{
  GtkCssWidgetNode *node;

  node = CTK_CSS_WIDGET_NODE (cssnode);

  if (node->widget)
    ctk_widget_clear_path (node->widget);

  CTK_CSS_NODE_CLASS (ctk_css_widget_node_parent_class)->style_changed (cssnode, change);
}

static gboolean
ctk_css_widget_node_queue_callback (GtkWidget     *widget,
                                    GdkFrameClock *frame_clock,
                                    gpointer       user_data)
{
  GtkCssNode *node = user_data;

  ctk_css_node_invalidate_frame_clock (node, TRUE);
  _ctk_container_queue_restyle (CTK_CONTAINER (widget));

  return G_SOURCE_CONTINUE;
}

static GtkCssStyle *
ctk_css_widget_node_update_style (GtkCssNode   *cssnode,
                                  GtkCssChange  change,
                                  gint64        timestamp,
                                  GtkCssStyle  *style)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (cssnode);

  if (widget_node->widget != NULL)
    {
      GtkStyleContext *context = _ctk_widget_peek_style_context (widget_node->widget);
      if (context)
        ctk_style_context_clear_property_cache (context);
    }

  return CTK_CSS_NODE_CLASS (ctk_css_widget_node_parent_class)->update_style (cssnode, change, timestamp, style);
}

static void
ctk_css_widget_node_queue_validate (GtkCssNode *node)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (CTK_IS_RESIZE_CONTAINER (widget_node->widget))
    widget_node->validate_cb_id = ctk_widget_add_tick_callback (widget_node->widget,
                                                                ctk_css_widget_node_queue_callback,
                                                                node,
                                                                NULL);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
ctk_css_widget_node_dequeue_validate (GtkCssNode *node)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (CTK_IS_RESIZE_CONTAINER (widget_node->widget))
    ctk_widget_remove_tick_callback (widget_node->widget,
                                     widget_node->validate_cb_id);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
ctk_css_widget_node_validate (GtkCssNode *node)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);
  GtkCssStyleChange change;
  GtkCssStyle *style;

  if (widget_node->widget == NULL)
    return;

  style = ctk_css_node_get_style (node);

  ctk_css_style_change_init (&change, widget_node->last_updated_style, style);
  if (ctk_css_style_change_has_change (&change))
    {
      GtkStyleContext *context;

      context = _ctk_widget_peek_style_context (widget_node->widget);
      if (context)
        ctk_style_context_validate (context, &change);
      else
        _ctk_widget_style_context_invalidated (widget_node->widget);
      g_set_object (&widget_node->last_updated_style, style);
    }
  ctk_css_style_change_finish (&change);
}

typedef GtkWidgetPath * (* GetPathForChildFunc) (GtkContainer *, GtkWidget *);

static gboolean
widget_needs_widget_path (GtkWidget *widget)
{
  static GetPathForChildFunc funcs[2];
  GtkContainerClass *class;
  GtkWidget *parent;
  GetPathForChildFunc parent_func;
  guint i;

  if (G_UNLIKELY (funcs[0] == NULL))
    {
      i = 0;

      class = (GtkContainerClass*)g_type_class_ref (CTK_TYPE_CONTAINER);
      funcs[i++] = class->get_path_for_child;
      g_type_class_unref (class);

      class = (GtkContainerClass*)g_type_class_ref (CTK_TYPE_BOX);
      funcs[i++] = class->get_path_for_child;
      g_type_class_unref (class);

      g_assert (i == G_N_ELEMENTS (funcs));
    }

  parent = _ctk_widget_get_parent (widget);
  if (parent == NULL)
    return FALSE;

  parent_func = CTK_CONTAINER_GET_CLASS (CTK_CONTAINER (parent))->get_path_for_child;
  for (i = 0; i < G_N_ELEMENTS (funcs); i++)
    {
      if (funcs[i] == parent_func)
        return FALSE;
    }

  return TRUE;
}

gboolean
ctk_css_widget_node_init_matcher (GtkCssNode     *node,
                                  GtkCssMatcher  *matcher)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);

  if (widget_node->widget == NULL)
    return FALSE;

  if (!widget_needs_widget_path (widget_node->widget))
    return CTK_CSS_NODE_CLASS (ctk_css_widget_node_parent_class)->init_matcher (node, matcher);

  return _ctk_css_matcher_init (matcher,
                                ctk_widget_get_path (widget_node->widget),
                                ctk_css_node_get_declaration (node));
}

static GtkWidgetPath *
ctk_css_widget_node_create_widget_path (GtkCssNode *node)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);
  GtkWidgetPath *path;
  guint length;

  if (widget_node->widget == NULL)
    path = ctk_widget_path_new ();
  else
    path = _ctk_widget_create_path (widget_node->widget);
  
  length = ctk_widget_path_length (path);
  if (length > 0)
    {
      ctk_css_node_declaration_add_to_widget_path (ctk_css_node_get_declaration (node),
                                                   path,
                                                   length - 1);
    }

  return path;
}

static const GtkWidgetPath *
ctk_css_widget_node_get_widget_path (GtkCssNode *node)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);

  if (widget_node->widget == NULL)
    return NULL;

  return ctk_widget_get_path (widget_node->widget);
}

static GtkStyleProviderPrivate *
ctk_css_widget_node_get_style_provider (GtkCssNode *node)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);
  GtkStyleContext *context;
  GtkStyleCascade *cascade;
  GtkSettings *settings;

  if (widget_node->widget == NULL)
    return NULL;

  context = _ctk_widget_peek_style_context (widget_node->widget);
  if (context)
    return ctk_style_context_get_style_provider (context);

  settings = ctk_widget_get_settings (widget_node->widget);
  if (!settings)
    return NULL;

  cascade = _ctk_settings_get_style_cascade (ctk_widget_get_settings (widget_node->widget),
                                             ctk_widget_get_scale_factor (widget_node->widget));
  return CTK_STYLE_PROVIDER_PRIVATE (cascade);
}

static GdkFrameClock *
ctk_css_widget_node_get_frame_clock (GtkCssNode *node)
{
  GtkCssWidgetNode *widget_node = CTK_CSS_WIDGET_NODE (node);

  if (widget_node->widget == NULL)
    return NULL;

  if (!ctk_settings_get_enable_animations (ctk_widget_get_settings (widget_node->widget)))
    return NULL;

  return ctk_widget_get_frame_clock (widget_node->widget);
}

static void
ctk_css_widget_node_class_init (GtkCssWidgetNodeClass *klass)
{
  GtkCssNodeClass *node_class = CTK_CSS_NODE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_css_widget_node_finalize;
  node_class->update_style = ctk_css_widget_node_update_style;
  node_class->validate = ctk_css_widget_node_validate;
  node_class->queue_validate = ctk_css_widget_node_queue_validate;
  node_class->dequeue_validate = ctk_css_widget_node_dequeue_validate;
  node_class->init_matcher = ctk_css_widget_node_init_matcher;
  node_class->create_widget_path = ctk_css_widget_node_create_widget_path;
  node_class->get_widget_path = ctk_css_widget_node_get_widget_path;
  node_class->get_style_provider = ctk_css_widget_node_get_style_provider;
  node_class->get_frame_clock = ctk_css_widget_node_get_frame_clock;
  node_class->style_changed = ctk_css_widget_node_style_changed;
}

static void
ctk_css_widget_node_init (GtkCssWidgetNode *node)
{
  node->last_updated_style = g_object_ref (ctk_css_static_style_get_default ());
}

GtkCssNode *
ctk_css_widget_node_new (GtkWidget *widget)
{
  GtkCssWidgetNode *result;

  ctk_internal_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  result = g_object_new (CTK_TYPE_CSS_WIDGET_NODE, NULL);
  result->widget = widget;
  ctk_css_node_set_visible (CTK_CSS_NODE (result),
                            _ctk_widget_get_visible (widget));

  return CTK_CSS_NODE (result);
}

void
ctk_css_widget_node_widget_destroyed (GtkCssWidgetNode *node)
{
  ctk_internal_return_if_fail (CTK_IS_CSS_WIDGET_NODE (node));
  ctk_internal_return_if_fail (node->widget != NULL);

  node->widget = NULL;
  /* Contents of this node are now undefined.
   * So we don't clear the style or anything.
   */
}

GtkWidget *
ctk_css_widget_node_get_widget (GtkCssWidgetNode *node)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_WIDGET_NODE (node), NULL);

  return node->widget;
}

