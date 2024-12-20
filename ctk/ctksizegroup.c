/* CTK - The GIMP Toolkit
 * ctksizegroup.c: 
 * Copyright (C) 2001 Red Hat Software
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
#include <string.h>

#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkcontainer.h"
#include "ctkintl.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctksizegroup-private.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"


/**
 * SECTION:ctksizegroup
 * @Short_description: Grouping widgets so they request the same size
 * @Title: CtkSizeGroup
 *
 * #CtkSizeGroup provides a mechanism for grouping a number of widgets
 * together so they all request the same amount of space.  This is
 * typically useful when you want a column of widgets to have the same
 * size, but you can’t use a #CtkGrid widget.
 *
 * In detail, the size requested for each widget in a #CtkSizeGroup is
 * the maximum of the sizes that would have been requested for each
 * widget in the size group if they were not in the size group. The mode
 * of the size group (see ctk_size_group_set_mode()) determines whether
 * this applies to the horizontal size, the vertical size, or both sizes.
 *
 * Note that size groups only affect the amount of space requested, not
 * the size that the widgets finally receive. If you want the widgets in
 * a #CtkSizeGroup to actually be the same size, you need to pack them in
 * such a way that they get the size they request and not more. For
 * example, if you are packing your widgets into a table, you would not
 * include the %CTK_FILL flag.
 *
 * #CtkSizeGroup objects are referenced by each widget in the size group,
 * so once you have added all widgets to a #CtkSizeGroup, you can drop
 * the initial reference to the size group with g_object_unref(). If the
 * widgets in the size group are subsequently destroyed, then they will
 * be removed from the size group and drop their references on the size
 * group; when all widgets have been removed, the size group will be
 * freed.
 *
 * Widgets can be part of multiple size groups; CTK+ will compute the
 * horizontal size of a widget from the horizontal requisition of all
 * widgets that can be reached from the widget by a chain of size groups
 * of type %CTK_SIZE_GROUP_HORIZONTAL or %CTK_SIZE_GROUP_BOTH, and the
 * vertical size from the vertical requisition of all widgets that can be
 * reached from the widget by a chain of size groups of type
 * %CTK_SIZE_GROUP_VERTICAL or %CTK_SIZE_GROUP_BOTH.
 *
 * Note that only non-contextual sizes of every widget are ever consulted
 * by size groups (since size groups have no knowledge of what size a widget
 * will be allocated in one dimension, it cannot derive how much height
 * a widget will receive for a given width). When grouping widgets that
 * trade height for width in mode %CTK_SIZE_GROUP_VERTICAL or %CTK_SIZE_GROUP_BOTH:
 * the height for the minimum width will be the requested height for all
 * widgets in the group. The same is of course true when horizontally grouping
 * width for height widgets.
 *
 * Widgets that trade height-for-width should set a reasonably large minimum width
 * by way of #CtkLabel:width-chars for instance. Widgets with static sizes as well
 * as widgets that grow (such as ellipsizing text) need no such considerations.
 *
 * # CtkSizeGroup as CtkBuildable
 *
 * Size groups can be specified in a UI definition by placing an <object>
 * element with `class="CtkSizeGroup"` somewhere in the UI definition. The
 * widgets that belong to the size group are specified by a <widgets> element
 * that may contain multiple <widget> elements, one for each member of the
 * size group. The ”name” attribute gives the id of the widget.
 *
 * An example of a UI definition fragment with CtkSizeGroup:
 * |[
 * <object class="CtkSizeGroup">
 *   <property name="mode">CTK_SIZE_GROUP_HORIZONTAL</property>
 *   <widgets>
 *     <widget name="radio1"/>
 *     <widget name="radio2"/>
 *   </widgets>
 * </object>
 * ]|
 */


struct _CtkSizeGroupPrivate
{
  GSList         *widgets;

  guint8          mode;

  guint           ignore_hidden : 1;
};

enum {
  PROP_0,
  PROP_MODE,
  PROP_IGNORE_HIDDEN
};

static void ctk_size_group_set_property (GObject      *object,
					 guint         prop_id,
					 const GValue *value,
					 GParamSpec   *pspec);
static void ctk_size_group_get_property (GObject      *object,
					 guint         prop_id,
					 GValue       *value,
					 GParamSpec   *pspec);

/* CtkBuildable */
static void ctk_size_group_buildable_init (CtkBuildableIface *iface);
static gboolean ctk_size_group_buildable_custom_tag_start (CtkBuildable  *buildable,
							   CtkBuilder    *builder,
							   GObject       *child,
							   const gchar   *tagname,
							   GMarkupParser *parser,
							   gpointer      *data);
static void ctk_size_group_buildable_custom_finished (CtkBuildable  *buildable,
						      CtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *tagname,
						      gpointer       user_data);

G_STATIC_ASSERT (CTK_SIZE_GROUP_HORIZONTAL == (1 << CTK_ORIENTATION_HORIZONTAL));
G_STATIC_ASSERT (CTK_SIZE_GROUP_VERTICAL == (1 << CTK_ORIENTATION_VERTICAL));
G_STATIC_ASSERT (CTK_SIZE_GROUP_BOTH == (CTK_SIZE_GROUP_HORIZONTAL | CTK_SIZE_GROUP_VERTICAL));

G_DEFINE_TYPE_WITH_CODE (CtkSizeGroup, ctk_size_group, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkSizeGroup)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_size_group_buildable_init))

static void
add_widget_to_closure (GHashTable *widgets,
                       GHashTable *groups,
                       CtkWidget  *widget,
		       gint        orientation)
{
  GSList *tmp_groups, *tmp_widgets;
  gboolean hidden;

  if (g_hash_table_lookup (widgets, widget))
    return;

  g_hash_table_add (widgets, widget);
  hidden = !ctk_widget_is_visible (widget);

  for (tmp_groups = _ctk_widget_get_sizegroups (widget); tmp_groups; tmp_groups = tmp_groups->next)
    {
      CtkSizeGroup        *tmp_group = tmp_groups->data;
      CtkSizeGroupPrivate *tmp_priv  = tmp_group->priv;

      if (g_hash_table_lookup (groups, tmp_group))
        continue;

      if (tmp_priv->ignore_hidden && hidden)
        continue;

      if (orientation >= 0 && !(tmp_priv->mode & (1 << orientation)))
        continue;

      g_hash_table_add (groups, tmp_group);

      for (tmp_widgets = tmp_priv->widgets; tmp_widgets; tmp_widgets = tmp_widgets->next)
        add_widget_to_closure (widgets, groups, tmp_widgets->data, orientation);
    }
}

GHashTable *
_ctk_size_group_get_widget_peers (CtkWidget      *for_widget,
                                  CtkOrientation  orientation)
{
  GHashTable *widgets, *groups;

  widgets = g_hash_table_new (NULL, NULL);
  groups = g_hash_table_new (NULL, NULL);

  add_widget_to_closure (widgets, groups, for_widget, orientation);

  g_hash_table_unref (groups);

  return widgets;
}

static void
queue_resize_on_group (CtkSizeGroup *size_group)
{
  CtkSizeGroupPrivate *priv = size_group->priv;
  GSList *list;

  for (list = priv->widgets; list; list = list->next)
    {
      ctk_widget_queue_resize (list->data);
    }
}

static void
ctk_size_group_class_init (CtkSizeGroupClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = ctk_size_group_set_property;
  gobject_class->get_property = ctk_size_group_get_property;
  
  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      P_("Mode"),
                                                      P_("The directions in which the size group affects the requested sizes"
                                                         " of its component widgets"),
                                                      CTK_TYPE_SIZE_GROUP_MODE,
                                                      CTK_SIZE_GROUP_HORIZONTAL,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkSizeGroup:ignore-hidden:
   *
   * If %TRUE, unmapped widgets are ignored when determining
   * the size of the group.
   *
   * Since: 2.8
   *
   * Deprecated: 3.22: Measuring the size of hidden widgets has not worked
   *     reliably for a long time. In most cases, they will report a size
   *     of 0 nowadays, and thus, their size will not affect the other
   *     size group members. In effect, size groups will always operate
   *     as if this property was %TRUE. Use a #CtkStack instead to hide
   *     widgets while still having their size taken into account.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_IGNORE_HIDDEN,
                                   g_param_spec_boolean ("ignore-hidden",
                                                         P_("Ignore hidden"),
                                                         P_("If TRUE, unmapped widgets are ignored "
                                                            "when determining the size of the group"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED));
}

static void
ctk_size_group_init (CtkSizeGroup *size_group)
{
  CtkSizeGroupPrivate *priv;

  size_group->priv = ctk_size_group_get_instance_private (size_group);
  priv = size_group->priv;

  priv->widgets = NULL;
  priv->mode = CTK_SIZE_GROUP_HORIZONTAL;
  priv->ignore_hidden = FALSE;
}

static void
ctk_size_group_buildable_init (CtkBuildableIface *iface)
{
  iface->custom_tag_start = ctk_size_group_buildable_custom_tag_start;
  iface->custom_finished = ctk_size_group_buildable_custom_finished;
}

static void
ctk_size_group_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  CtkSizeGroup *size_group = CTK_SIZE_GROUP (object);

  switch (prop_id)
    {
    case PROP_MODE:
      ctk_size_group_set_mode (size_group, g_value_get_enum (value));
      break;
    case PROP_IGNORE_HIDDEN:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_size_group_set_ignore_hidden (size_group, g_value_get_boolean (value));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_size_group_get_property (GObject      *object,
			     guint         prop_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  CtkSizeGroup *size_group = CTK_SIZE_GROUP (object);
  CtkSizeGroupPrivate *priv = size_group->priv;

  switch (prop_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;
    case PROP_IGNORE_HIDDEN:
      g_value_set_boolean (value, priv->ignore_hidden);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_size_group_new:
 * @mode: the mode for the new size group.
 * 
 * Create a new #CtkSizeGroup.
 
 * Returns: a newly created #CtkSizeGroup
 **/
CtkSizeGroup *
ctk_size_group_new (CtkSizeGroupMode mode)
{
  CtkSizeGroup *size_group = g_object_new (CTK_TYPE_SIZE_GROUP, NULL);
  CtkSizeGroupPrivate *priv = size_group->priv;

  priv->mode = mode;

  return size_group;
}

/**
 * ctk_size_group_set_mode:
 * @size_group: a #CtkSizeGroup
 * @mode: the mode to set for the size group.
 * 
 * Sets the #CtkSizeGroupMode of the size group. The mode of the size
 * group determines whether the widgets in the size group should
 * all have the same horizontal requisition (%CTK_SIZE_GROUP_HORIZONTAL)
 * all have the same vertical requisition (%CTK_SIZE_GROUP_VERTICAL),
 * or should all have the same requisition in both directions
 * (%CTK_SIZE_GROUP_BOTH).
 **/
void
ctk_size_group_set_mode (CtkSizeGroup     *size_group,
			 CtkSizeGroupMode  mode)
{
  CtkSizeGroupPrivate *priv;

  g_return_if_fail (CTK_IS_SIZE_GROUP (size_group));

  priv = size_group->priv;

  if (priv->mode != mode)
    {
      if (priv->mode != CTK_SIZE_GROUP_NONE)
	queue_resize_on_group (size_group);
      priv->mode = mode;
      if (priv->mode != CTK_SIZE_GROUP_NONE)
	queue_resize_on_group (size_group);

      g_object_notify (G_OBJECT (size_group), "mode");
    }
}

/**
 * ctk_size_group_get_mode:
 * @size_group: a #CtkSizeGroup
 * 
 * Gets the current mode of the size group. See ctk_size_group_set_mode().
 * 
 * Returns: the current mode of the size group.
 **/
CtkSizeGroupMode
ctk_size_group_get_mode (CtkSizeGroup *size_group)
{
  g_return_val_if_fail (CTK_IS_SIZE_GROUP (size_group), CTK_SIZE_GROUP_BOTH);

  return size_group->priv->mode;
}

/**
 * ctk_size_group_set_ignore_hidden:
 * @size_group: a #CtkSizeGroup
 * @ignore_hidden: whether unmapped widgets should be ignored
 *   when calculating the size
 *
 * Sets whether unmapped widgets should be ignored when
 * calculating the size.
 *
 * Since: 2.8
 *
 * Deprecated: 3.22: Measuring the size of hidden widgets has not worked
 *     reliably for a long time. In most cases, they will report a size
 *     of 0 nowadays, and thus, their size will not affect the other
 *     size group members. In effect, size groups will always operate
 *     as if this property was %TRUE. Use a #CtkStack instead to hide
 *     widgets while still having their size taken into account.
 */
void
ctk_size_group_set_ignore_hidden (CtkSizeGroup *size_group,
				  gboolean      ignore_hidden)
{
  CtkSizeGroupPrivate *priv;

  g_return_if_fail (CTK_IS_SIZE_GROUP (size_group));

  priv = size_group->priv;

  ignore_hidden = ignore_hidden != FALSE;

  if (priv->ignore_hidden != ignore_hidden)
    {
      priv->ignore_hidden = ignore_hidden;

      g_object_notify (G_OBJECT (size_group), "ignore-hidden");
    }
}

/**
 * ctk_size_group_get_ignore_hidden:
 * @size_group: a #CtkSizeGroup
 *
 * Returns if invisible widgets are ignored when calculating the size.
 *
 * Returns: %TRUE if invisible widgets are ignored.
 *
 * Since: 2.8
 *
 * Deprecated: 3.22: Measuring the size of hidden widgets has not worked
 *     reliably for a long time. In most cases, they will report a size
 *     of 0 nowadays, and thus, their size will not affect the other
 *     size group members. In effect, size groups will always operate
 *     as if this property was %TRUE. Use a #CtkStack instead to hide
 *     widgets while still having their size taken into account.
 */
gboolean
ctk_size_group_get_ignore_hidden (CtkSizeGroup *size_group)
{
  g_return_val_if_fail (CTK_IS_SIZE_GROUP (size_group), FALSE);

  return size_group->priv->ignore_hidden;
}

/**
 * ctk_size_group_add_widget:
 * @size_group: a #CtkSizeGroup
 * @widget: the #CtkWidget to add
 * 
 * Adds a widget to a #CtkSizeGroup. In the future, the requisition
 * of the widget will be determined as the maximum of its requisition
 * and the requisition of the other widgets in the size group.
 * Whether this applies horizontally, vertically, or in both directions
 * depends on the mode of the size group. See ctk_size_group_set_mode().
 *
 * When the widget is destroyed or no longer referenced elsewhere, it will 
 * be removed from the size group.
 */
void
ctk_size_group_add_widget (CtkSizeGroup *size_group,
			   CtkWidget    *widget)
{
  CtkSizeGroupPrivate *priv;
  GSList *groups;
  
  g_return_if_fail (CTK_IS_SIZE_GROUP (size_group));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = size_group->priv;

  groups = _ctk_widget_get_sizegroups (widget);

  if (!g_slist_find (groups, size_group))
    {
      _ctk_widget_add_sizegroup (widget, size_group);

      priv->widgets = g_slist_prepend (priv->widgets, widget);

      g_object_ref (size_group);
    }
  
  queue_resize_on_group (size_group);
}

/**
 * ctk_size_group_remove_widget:
 * @size_group: a #CtkSizeGroup
 * @widget: the #CtkWidget to remove
 * 
 * Removes a widget from a #CtkSizeGroup.
 **/
void
ctk_size_group_remove_widget (CtkSizeGroup *size_group,
			      CtkWidget    *widget)
{
  CtkSizeGroupPrivate *priv;
  
  g_return_if_fail (CTK_IS_SIZE_GROUP (size_group));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = size_group->priv;

  g_return_if_fail (g_slist_find (priv->widgets, widget));

  _ctk_widget_remove_sizegroup (widget, size_group);

  priv->widgets = g_slist_remove (priv->widgets, widget);
  queue_resize_on_group (size_group);
  ctk_widget_queue_resize (widget);

  g_object_unref (size_group);
}

/**
 * ctk_size_group_get_widgets:
 * @size_group: a #CtkSizeGroup
 * 
 * Returns the list of widgets associated with @size_group.
 *
 * Returns:  (element-type CtkWidget) (transfer none): a #GSList of
 *   widgets. The list is owned by CTK+ and should not be modified.
 *
 * Since: 2.10
 **/
GSList *
ctk_size_group_get_widgets (CtkSizeGroup *size_group)
{
  return size_group->priv->widgets;
}

typedef struct {
  gchar *name;
  gint line;
  gint col;
} ItemData;

static void
item_data_free (gpointer data)
{
  ItemData *item_data = data;

  g_free (item_data->name);
  g_free (item_data);
}

typedef struct {
  GObject *object;
  CtkBuilder *builder;
  GSList *items;
} GSListSubParserData;

static void
size_group_start_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          const gchar         **names,
                          const gchar         **values,
                          gpointer              user_data,
                          GError              **error)
{
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  if (strcmp (element_name, "widget") == 0)
    {
      const gchar *name;
      ItemData *item_data;

      if (!_ctk_builder_check_parent (data->builder, context, "widgets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      item_data = g_new (ItemData, 1);
      item_data->name = g_strdup (name);
      g_markup_parse_context_get_position (context, &item_data->line, &item_data->col);
      data->items = g_slist_prepend (data->items, item_data);
    }
  else if (strcmp (element_name, "widgets") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkSizeGroup", element_name,
                                        error);
    }
}

static const GMarkupParser size_group_parser =
  {
    .start_element = size_group_start_element
  };

static gboolean
ctk_size_group_buildable_custom_tag_start (CtkBuildable  *buildable,
                                           CtkBuilder    *builder,
                                           GObject       *child,
                                           const gchar   *tagname,
                                           GMarkupParser *parser,
                                           gpointer      *parser_data)
{
  GSListSubParserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "widgets") == 0)
    {
      data = g_slice_new0 (GSListSubParserData);
      data->items = NULL;
      data->object = G_OBJECT (buildable);
      data->builder = builder;

      *parser = size_group_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

static void
ctk_size_group_buildable_custom_finished (CtkBuildable  *buildable G_GNUC_UNUSED,
                                          CtkBuilder    *builder,
                                          GObject       *child G_GNUC_UNUSED,
                                          const gchar   *tagname,
                                          gpointer       user_data)
{
  GSList *l;
  GSListSubParserData *data;
  GObject *object;

  if (strcmp (tagname, "widgets") != 0)
    return;

  data = (GSListSubParserData*)user_data;
  data->items = g_slist_reverse (data->items);

  for (l = data->items; l; l = l->next)
    {
      ItemData *item_data = l->data;
      object = _ctk_builder_lookup_object (builder, item_data->name, item_data->line, item_data->col);
      if (!object)
        continue;
      ctk_size_group_add_widget (CTK_SIZE_GROUP (data->object), CTK_WIDGET (object));
    }
  g_slist_free_full (data->items, item_data_free);
  g_slice_free (GSListSubParserData, data);
}
