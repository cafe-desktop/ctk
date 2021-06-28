/* CTK+ - accessibility implementations
 * Copyright 2011, F123 Consulting & Mais Diferenças
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include <ctk/ctk.h>

#include "ctkwidgetaccessibleprivate.h"
#include "ctkwindowaccessible.h"
#include "ctktoplevelaccessible.h"
#include "ctkwindowprivate.h"

/* atkcomponent.h */

static void                  ctk_window_accessible_get_extents      (AtkComponent         *component,
                                                           gint                 *x,
                                                           gint                 *y,
                                                           gint                 *width,
                                                           gint                 *height,
                                                           AtkCoordType         coord_type);
static void                  ctk_window_accessible_get_size         (AtkComponent         *component,
                                                           gint                 *width,
                                                           gint                 *height);

static void atk_component_interface_init (AtkComponentIface *iface);
static void atk_window_interface_init (AtkWindowIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkWindowAccessible,
                         ctk_window_accessible,
                         CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT,
                                                atk_component_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_WINDOW,
                                                atk_window_interface_init));


static void
ctk_window_accessible_focus_event (AtkObject *obj,
                                   gboolean   focus_in)
{
  atk_object_notify_state_change (obj, ATK_STATE_ACTIVE, focus_in);
}

static void
ctk_window_accessible_notify_ctk (GObject    *obj,
                                  GParamSpec *pspec)
{
  CtkWidget *widget = CTK_WIDGET (obj);
  AtkObject* atk_obj = ctk_widget_get_accessible (widget);

  if (g_strcmp0 (pspec->name, "title") == 0)
    {
      g_object_notify (G_OBJECT (atk_obj), "accessible-name");
      g_signal_emit_by_name (atk_obj, "visible-data-changed");
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_window_accessible_parent_class)->notify_ctk (obj, pspec);
}

static gboolean
window_state_event_cb (CtkWidget           *widget,
                       CdkEventWindowState *event)
{
  AtkObject* obj;

  obj = ctk_widget_get_accessible (widget);
  atk_object_notify_state_change (obj, ATK_STATE_ICONIFIED,
                                  (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) != 0);

  return FALSE;
}

static void
ctk_window_accessible_initialize (AtkObject *obj,
                                  gpointer   data)
{
  CtkWidget *widget = CTK_WIDGET (data);

  ATK_OBJECT_CLASS (ctk_window_accessible_parent_class)->initialize (obj, data);

  g_signal_connect (data, "window-state-event", G_CALLBACK (window_state_event_cb), NULL);
  _ctk_widget_accessible_set_layer (CTK_WIDGET_ACCESSIBLE (obj), ATK_LAYER_WINDOW);

  if (ctk_window_get_window_type (CTK_WINDOW (widget)) == CTK_WINDOW_POPUP)
    obj->role = ATK_ROLE_WINDOW;
  else
    obj->role = ATK_ROLE_FRAME;
}

static CtkWidget *
find_label_child (CtkContainer *container)
{
  GList *children, *tmp_list;
  CtkWidget *child;

  children = ctk_container_get_children (container);

  child = NULL;
  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next)
    {
      if (CTK_IS_LABEL (tmp_list->data))
        {
          child = CTK_WIDGET (tmp_list->data);
          break;
        }
      else if (CTK_IS_CONTAINER (tmp_list->data))
        {
          child = find_label_child (CTK_CONTAINER (tmp_list->data));
          if (child)
            break;
        }
   }
  g_list_free (children);
  return child;
}

static const gchar *
ctk_window_accessible_get_name (AtkObject *accessible)
{
  const gchar* name;
  CtkWidget* widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (ctk_window_accessible_parent_class)->get_name (accessible);
  if (name != NULL)
    return name;

  if (CTK_IS_WINDOW (widget))
    {
      CtkWindow *window = CTK_WINDOW (widget);

      name = ctk_window_get_title (window);
      if (name == NULL && accessible->role == ATK_ROLE_TOOL_TIP)
        {
          CtkWidget *child;

          child = find_label_child (CTK_CONTAINER (window));
          if (CTK_IS_LABEL (child))
            name = ctk_label_get_text (CTK_LABEL (child));
        }
    }
  return name;
}

static gint
ctk_window_accessible_get_index_in_parent (AtkObject *accessible)
{
  CtkWidget* widget;
  AtkObject* atk_obj;
  gint index = -1;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return -1;

  index = ATK_OBJECT_CLASS (ctk_window_accessible_parent_class)->get_index_in_parent (accessible);
  if (index != -1)
    return index;

  atk_obj = atk_get_root ();

  if (CTK_IS_WINDOW (widget))
    {
      CtkWindow *window = CTK_WINDOW (widget);
      if (CTK_IS_TOPLEVEL_ACCESSIBLE (atk_obj))
        {
          CtkToplevelAccessible *toplevel = CTK_TOPLEVEL_ACCESSIBLE (atk_obj);
          index = g_list_index (ctk_toplevel_accessible_get_children (toplevel), window);
        }
      else
        {
          gint i, sibling_count;

          sibling_count = atk_object_get_n_accessible_children (atk_obj);
          for (i = 0; i < sibling_count && index == -1; ++i)
            {
              AtkObject *child = atk_object_ref_accessible_child (atk_obj, i);
              if (accessible == child)
                index = i;
              g_object_unref (G_OBJECT (child));
            }
        }
    }
  return index;
}

static AtkRelationSet *
ctk_window_accessible_ref_relation_set (AtkObject *obj)
{
  CtkWidget *widget;
  AtkRelationSet *relation_set;
  AtkObject *array[1];
  AtkRelation* relation;
  CtkWidget *current_widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  relation_set = ATK_OBJECT_CLASS (ctk_window_accessible_parent_class)->ref_relation_set (obj);

  if (atk_object_get_role (obj) == ATK_ROLE_TOOL_TIP)
    {
      relation = atk_relation_set_get_relation_by_type (relation_set, ATK_RELATION_POPUP_FOR);
      if (relation)
        atk_relation_set_remove (relation_set, relation);

      if (0) /* FIXME need a way to go from tooltip window to widget */
        {
          array[0] = ctk_widget_get_accessible (current_widget);
          relation = atk_relation_new (array, 1, ATK_RELATION_POPUP_FOR);
          atk_relation_set_add (relation_set, relation);
          g_object_unref (relation);
        }
    }
  return relation_set;
}

static AtkStateSet *
ctk_window_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  CtkWidget *widget;
  CtkWindow *window;
  CdkWindow *cdk_window;
  CdkWindowState state;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (ctk_window_accessible_parent_class)->ref_state_set (accessible);

  window = CTK_WINDOW (widget);

  if (ctk_window_has_toplevel_focus (window) && ctk_window_is_active (window))
    atk_state_set_add_state (state_set, ATK_STATE_ACTIVE);

  cdk_window = ctk_widget_get_window (widget);
  if (cdk_window)
    {
      state = cdk_window_get_state (cdk_window);
      if (state & GDK_WINDOW_STATE_ICONIFIED)
        atk_state_set_add_state (state_set, ATK_STATE_ICONIFIED);
    }
  if (ctk_window_get_modal (window))
    atk_state_set_add_state (state_set, ATK_STATE_MODAL);

  if (ctk_window_get_resizable (window))
    atk_state_set_add_state (state_set, ATK_STATE_RESIZABLE);

  return state_set;
}

static void
count_widget (CtkWidget *widget,
              gint      *count)
{
  (*count)++;
}

static void
prepend_widget (CtkWidget  *widget,
		GList     **list)
{
  *list = g_list_prepend (*list, widget);
}

static gint
ctk_window_accessible_get_n_children (AtkObject *object)
{
  CtkWidget *window;
  gint count = 0;

  window = ctk_accessible_get_widget (CTK_ACCESSIBLE (object));
  ctk_container_forall (CTK_CONTAINER (window),
			(CtkCallback) count_widget, &count);
  return count;
}

static AtkObject *
ctk_window_accessible_ref_child (AtkObject *object,
                                 gint       i)
{
  CtkWidget *window, *ref_child;
  GList *children = NULL;

  window = ctk_accessible_get_widget (CTK_ACCESSIBLE (object));
  ctk_container_forall (CTK_CONTAINER (window),
			(CtkCallback) prepend_widget, &children);
  ref_child = g_list_nth_data (children, i);
  g_list_free (children);

  if (!ref_child)
    return NULL;

  return g_object_ref (ctk_widget_get_accessible (ref_child));
}

static AtkAttributeSet *
ctk_widget_accessible_get_attributes (AtkObject *obj)
{
  CtkWidget *window;
  CdkWindowTypeHint hint;
  AtkAttributeSet *attributes;
  AtkAttribute *attr;
  GEnumClass *class;
  GEnumValue *value;

  attributes = ATK_OBJECT_CLASS (ctk_window_accessible_parent_class)->get_attributes (obj);

  attr = g_new (AtkAttribute, 1);
  attr->name = g_strdup ("window-type");

  window = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  hint = ctk_window_get_type_hint (CTK_WINDOW (window));

  class = g_type_class_ref (GDK_TYPE_WINDOW_TYPE_HINT);
  for (value = class->values; value->value_name; value++)
    {
      if (hint == value->value)
        {
          attr->value = g_strdup (value->value_nick);
          break;
        }
    }
  g_type_class_unref (class);

  attributes = g_slist_append (attributes, attr);

  return attributes;
}

static void
ctk_window_accessible_class_init (CtkWindowAccessibleClass *klass)
{
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  widget_class->notify_ctk = ctk_window_accessible_notify_ctk;

  class->get_name = ctk_window_accessible_get_name;
  class->get_index_in_parent = ctk_window_accessible_get_index_in_parent;
  class->ref_relation_set = ctk_window_accessible_ref_relation_set;
  class->ref_state_set = ctk_window_accessible_ref_state_set;
  class->initialize = ctk_window_accessible_initialize;
  class->focus_event = ctk_window_accessible_focus_event;
  class->get_n_children = ctk_window_accessible_get_n_children;
  class->ref_child = ctk_window_accessible_ref_child;
  class->get_attributes = ctk_widget_accessible_get_attributes;
}

static void
ctk_window_accessible_init (CtkWindowAccessible *accessible)
{
}

static void
ctk_window_accessible_get_extents (AtkComponent  *component,
                                   gint          *x,
                                   gint          *y,
                                   gint          *width,
                                   gint          *height,
                                   AtkCoordType   coord_type)
{
  CtkWidget *widget;
  CdkWindow *window;
  CdkRectangle rect;
  gint x_toplevel, y_toplevel;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (widget == NULL)
    return;

  if (!ctk_widget_is_toplevel (widget))
    {
      AtkComponentIface *parent_iface;

      parent_iface = (AtkComponentIface *) g_type_interface_peek_parent (ATK_COMPONENT_GET_IFACE (component));
      parent_iface->get_extents (component, x, y, width, height, coord_type);
      return;
    }

  window = ctk_widget_get_window (widget);
  if (window == NULL)
    return;

  cdk_window_get_frame_extents (window, &rect);

  *width = rect.width;
  *height = rect.height;
  if (!ctk_widget_is_drawable (widget))
    {
      *x = G_MININT;
      *y = G_MININT;
      return;
    }

  *x = rect.x;
  *y = rect.y;
  if (coord_type == ATK_XY_WINDOW)
    {
      cdk_window_get_origin (window, &x_toplevel, &y_toplevel);
      *x -= x_toplevel;
      *y -= y_toplevel;
    }
}

static void
ctk_window_accessible_get_size (AtkComponent *component,
                                gint         *width,
                                gint         *height)
{
  CtkWidget *widget;
  CdkWindow *window;
  CdkRectangle rect;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (widget == NULL)
    return;

  if (!ctk_widget_is_toplevel (widget))
    {
      AtkComponentIface *parent_iface;

      parent_iface = (AtkComponentIface *) g_type_interface_peek_parent (ATK_COMPONENT_GET_IFACE (component));
      parent_iface->get_size (component, width, height);
      return;
    }

  window = ctk_widget_get_window (widget);
  if (window == NULL)
    return;

  cdk_window_get_frame_extents (window, &rect);

  *width = rect.width;
  *height = rect.height;
}

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = ctk_window_accessible_get_extents;
  iface->get_size = ctk_window_accessible_get_size;
}

static void
atk_window_interface_init (AtkWindowIface *iface)
{
  /* At this moment AtkWindow is just about signals */
}
