/* CTK+ - accessibility implementations
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
#ifdef CDK_WINDOWING_X11
#include <cdk/x11/cdkx.h>
#endif
#include "ctkwidgetaccessibleprivate.h"
#include "ctknotebookpageaccessible.h"

struct _CtkWidgetAccessiblePrivate
{
  AtkLayer layer;
};

#define TOOLTIP_KEY "tooltip"

extern CtkWidget *_focus_widget;


static gboolean ctk_widget_accessible_on_screen           (CtkWidget *widget);
static gboolean ctk_widget_accessible_all_parents_visible (CtkWidget *widget);

static void atk_component_interface_init (AtkComponentIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkWidgetAccessible, ctk_widget_accessible, CTK_TYPE_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkWidgetAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, atk_component_interface_init))

/* Translate CtkWidget::focus-in/out-event to AtkObject::focus-event */
static gboolean
focus_cb (CtkWidget     *widget,
          CdkEventFocus *event)
{
  AtkObject *obj;

  obj = ctk_widget_get_accessible (widget);

  g_signal_emit_by_name (obj, "focus-event", event->in);

  return FALSE;
}

/* Translate CtkWidget property change notification to the notify_ctk vfunc */
static void
notify_cb (GObject    *obj,
           GParamSpec *pspec)
{
  CtkWidgetAccessible *widget;
  CtkWidgetAccessibleClass *klass;

  widget = CTK_WIDGET_ACCESSIBLE (ctk_widget_get_accessible (CTK_WIDGET (obj)));
  klass = CTK_WIDGET_ACCESSIBLE_GET_CLASS (widget);
  if (klass->notify_ctk)
    klass->notify_ctk (obj, pspec);
}

/* Translate CtkWidget::size-allocate to AtkComponent::bounds-changed */
static void
size_allocate_cb (CtkWidget     *widget,
                  CtkAllocation *allocation)
{
  AtkObject* accessible;
  AtkRectangle rect;

  accessible = ctk_widget_get_accessible (widget);
  if (ATK_IS_COMPONENT (accessible))
    {
      rect.x = allocation->x;
      rect.y = allocation->y;
      rect.width = allocation->width;
      rect.height = allocation->height;
      g_signal_emit_by_name (accessible, "bounds-changed", &rect);
    }
}

/* Translate CtkWidget mapped state into AtkObject showing */
static gint
map_cb (CtkWidget *widget)
{
  AtkObject *accessible;

  accessible = ctk_widget_get_accessible (widget);
  atk_object_notify_state_change (accessible, ATK_STATE_SHOWING,
                                  ctk_widget_get_mapped (widget));
  return 1;
}

static void
ctk_widget_accessible_focus_event (AtkObject *obj,
                                   gboolean   focus_in)
{
  AtkObject *focus_obj;

  focus_obj = g_object_get_data (G_OBJECT (obj), "cail-focus-object");
  if (focus_obj == NULL)
    focus_obj = obj;
  atk_object_notify_state_change (focus_obj, ATK_STATE_FOCUSED, focus_in);
}

static void
ctk_widget_accessible_update_tooltip (CtkWidgetAccessible *accessible,
                                      CtkWidget *widget)
{
  g_object_set_data_full (G_OBJECT (accessible),
                          TOOLTIP_KEY,
                          ctk_widget_get_tooltip_text (widget),
                          g_free);
}

static void
ctk_widget_accessible_initialize (AtkObject *obj,
                                  gpointer   data)
{
  CtkWidget *widget;

  widget = CTK_WIDGET (data);

  g_signal_connect_after (widget, "focus-in-event", G_CALLBACK (focus_cb), NULL);
  g_signal_connect_after (widget, "focus-out-event", G_CALLBACK (focus_cb), NULL);
  g_signal_connect (widget, "notify", G_CALLBACK (notify_cb), NULL);
  g_signal_connect (widget, "size-allocate", G_CALLBACK (size_allocate_cb), NULL);
  g_signal_connect (widget, "map", G_CALLBACK (map_cb), NULL);
  g_signal_connect (widget, "unmap", G_CALLBACK (map_cb), NULL);

  CTK_WIDGET_ACCESSIBLE (obj)->priv->layer = ATK_LAYER_WIDGET;
  obj->role = ATK_ROLE_UNKNOWN;

  ctk_widget_accessible_update_tooltip (CTK_WIDGET_ACCESSIBLE (obj), widget);
}

static const gchar *
ctk_widget_accessible_get_description (AtkObject *accessible)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  if (accessible->description)
    return accessible->description;

  return g_object_get_data (G_OBJECT (accessible), TOOLTIP_KEY);
}

static AtkObject *
ctk_widget_accessible_get_parent (AtkObject *accessible)
{
  AtkObject *parent;
  CtkWidget *widget, *parent_widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  parent = accessible->accessible_parent;
  if (parent != NULL)
    return parent;

  parent_widget = ctk_widget_get_parent (widget);
  if (parent_widget == NULL)
    return NULL;

  /* For a widget whose parent is a CtkNoteBook, we return the
   * accessible object corresponding the CtkNotebookPage containing
   * the widget as the accessible parent.
   */
  if (CTK_IS_NOTEBOOK (parent_widget))
    {
      gint page_num;
      CtkWidget *child;
      CtkNotebook *notebook;

      page_num = 0;
      notebook = CTK_NOTEBOOK (parent_widget);
      while (TRUE)
        {
          child = ctk_notebook_get_nth_page (notebook, page_num);
          if (!child)
            break;
          if (child == widget)
            {
              parent = ctk_widget_get_accessible (parent_widget);
              parent = atk_object_ref_accessible_child (parent, page_num);
              g_object_unref (parent);
              return parent;
            }
          page_num++;
        }
    }
  parent = ctk_widget_get_accessible (parent_widget);
  return parent;
}

static CtkWidget *
find_label (CtkWidget *widget)
{
  GList *labels;
  CtkWidget *label;
  CtkWidget *temp_widget;
  GList *ptr;

  labels = ctk_widget_list_mnemonic_labels (widget);
  label = NULL;
  ptr = labels;
  while (ptr)
    {
      if (ptr->data)
        {
          label = ptr->data;
          break;
        }
      ptr = ptr->next;
    }
  g_list_free (labels);

  /* Ignore a label within a button; bug #136602 */
  if (label && CTK_IS_BUTTON (widget))
    {
      temp_widget = label;
      while (temp_widget)
        {
          if (temp_widget == widget)
            {
              label = NULL;
              break;
            }
          temp_widget = ctk_widget_get_parent (temp_widget);
        }
    }
  return label;
}

static AtkRelationSet *
ctk_widget_accessible_ref_relation_set (AtkObject *obj)
{
  CtkWidget *widget;
  AtkRelationSet *relation_set;
  CtkWidget *label;
  AtkObject *array[1];
  AtkRelation* relation;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  relation_set = ATK_OBJECT_CLASS (ctk_widget_accessible_parent_class)->ref_relation_set (obj);

  if (CTK_IS_BOX (widget))
    return relation_set;

  if (!atk_relation_set_contains (relation_set, ATK_RELATION_LABELLED_BY))
    {
      label = find_label (widget);
      if (label == NULL)
        {
          if (CTK_IS_BUTTON (widget) && ctk_widget_get_mapped (widget))
            /*
             * Handle the case where GnomeIconEntry is the mnemonic widget.
             * The CtkButton which is a grandchild of the GnomeIconEntry
             * should really be the mnemonic widget. See bug #133967.
             */
            {
              CtkWidget *temp_widget;

              temp_widget = ctk_widget_get_parent (widget);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              if (CTK_IS_ALIGNMENT (temp_widget))
                {
                  temp_widget = ctk_widget_get_parent (temp_widget);
                  if (CTK_IS_BOX (temp_widget))
                    {
                      label = find_label (temp_widget);
                      if (!label)
                        label = find_label (ctk_widget_get_parent (temp_widget));
                    }
                }
G_GNUC_END_IGNORE_DEPRECATIONS
            }
          else if (CTK_IS_COMBO_BOX (widget))
            /*
             * Handle the case when CtkFileChooserButton is the mnemonic
             * widget.  The CtkComboBox which is a child of the
             * CtkFileChooserButton should be the mnemonic widget.
             * See bug #359843.
             */
            {
              CtkWidget *temp_widget;

              temp_widget = ctk_widget_get_parent (widget);
              if (CTK_IS_BOX (temp_widget))
                {
                  label = find_label (temp_widget);
                }
            }
        }

      if (label)
        {
          array[0] = ctk_widget_get_accessible (label);

          relation = atk_relation_new (array, 1, ATK_RELATION_LABELLED_BY);
          atk_relation_set_add (relation_set, relation);
          g_object_unref (relation);
        }
    }

  return relation_set;
}

static AtkStateSet *
ctk_widget_accessible_ref_state_set (AtkObject *accessible)
{
  CtkWidget *widget;
  AtkStateSet *state_set;

  state_set = ATK_OBJECT_CLASS (ctk_widget_accessible_parent_class)->ref_state_set (accessible);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);
  else
    {
      if (ctk_widget_is_sensitive (widget))
        {
          atk_state_set_add_state (state_set, ATK_STATE_SENSITIVE);
          atk_state_set_add_state (state_set, ATK_STATE_ENABLED);
        }
  
      if (ctk_widget_get_can_focus (widget))
        {
          atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);
        }
      /*
       * We do not currently generate notifications when an ATK object
       * corresponding to a CtkWidget changes visibility by being scrolled
       * on or off the screen.  The testcase for this is the main window
       * of the testctk application in which a set of buttons in a CtkVBox
       * is in a scrolled window with a viewport.
       *
       * To generate the notifications we would need to do the following:
       * 1) Find the CtkViewport among the ancestors of the objects
       * 2) Create an accessible for the viewport
       * 3) Connect to the value-changed signal on the viewport
       * 4) When the signal is received we need to traverse the children
       *    of the viewport and check whether the children are visible or not
       *    visible; we may want to restrict this to the widgets for which
       *    accessible objects have been created.
       * 5) We probably need to store a variable on_screen in the
       *    CtkWidgetAccessible data structure so we can determine whether
       *    the value has changed.
       */
      if (ctk_widget_get_visible (widget))
        {
          atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);
          if (ctk_widget_accessible_on_screen (widget) &&
              ctk_widget_get_mapped (widget) &&
              ctk_widget_accessible_all_parents_visible (widget))
            atk_state_set_add_state (state_set, ATK_STATE_SHOWING);
        }

      if (ctk_widget_has_focus (widget) && (widget == _focus_widget))
        {
          AtkObject *focus_obj;

          focus_obj = g_object_get_data (G_OBJECT (accessible), "cail-focus-object");
          if (focus_obj == NULL)
            atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);
        }

      if (ctk_widget_has_default (widget))
        atk_state_set_add_state (state_set, ATK_STATE_DEFAULT);

      if (CTK_IS_ORIENTABLE (widget))
        {
          if (ctk_orientable_get_orientation (CTK_ORIENTABLE (widget)) == CTK_ORIENTATION_HORIZONTAL)
            atk_state_set_add_state (state_set, ATK_STATE_HORIZONTAL);
          else
            atk_state_set_add_state (state_set, ATK_STATE_VERTICAL);
        }

      if (ctk_widget_get_has_tooltip (widget))
        atk_state_set_add_state (state_set, ATK_STATE_HAS_TOOLTIP);
    }
  return state_set;
}

static gint
ctk_widget_accessible_get_index_in_parent (AtkObject *accessible)
{
  CtkWidget *widget;
  CtkWidget *parent_widget;
  gint index;
  GList *children;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));

  if (widget == NULL)
    return -1;

  if (accessible->accessible_parent)
    {
      AtkObject *parent;

      parent = accessible->accessible_parent;

      if (CTK_IS_NOTEBOOK_PAGE_ACCESSIBLE (parent))
        return 0;
      else
        {
          gint n_children, i;
          gboolean found = FALSE;

          n_children = atk_object_get_n_accessible_children (parent);
          for (i = 0; i < n_children; i++)
            {
              AtkObject *child;

              child = atk_object_ref_accessible_child (parent, i);
              if (child == accessible)
                found = TRUE;

              g_object_unref (child);
              if (found)
                return i;
            }
        }
    }

  if (!CTK_IS_WIDGET (widget))
    return -1;
  parent_widget = ctk_widget_get_parent (widget);
  if (!CTK_IS_CONTAINER (parent_widget))
    return -1;

  children = ctk_container_get_children (CTK_CONTAINER (parent_widget));

  index = g_list_index (children, widget);
  g_list_free (children);
  return index;
}

/* This function is the default implementation for the notify_ctk
 * vfunc which gets called when a property changes value on the
 * CtkWidget associated with a CtkWidgetAccessible. It constructs
 * an AtkPropertyValues structure and emits a “property_changed”
 * signal which causes the user specified AtkPropertyChangeHandler
 * to be called.
 */
static void
ctk_widget_accessible_notify_ctk (GObject    *obj,
                                  GParamSpec *pspec)
{
  CtkWidget* widget = CTK_WIDGET (obj);
  AtkObject* atk_obj = ctk_widget_get_accessible (widget);
  AtkState state;
  gboolean value;

  if (g_strcmp0 (pspec->name, "has-focus") == 0)
    /*
     * We use focus-in-event and focus-out-event signals to catch
     * focus changes so we ignore this.
     */
    return;
  else if (g_strcmp0 (pspec->name, "tooltip-text") == 0)
    {
      ctk_widget_accessible_update_tooltip (CTK_WIDGET_ACCESSIBLE (atk_obj),
                                            widget);

      if (atk_obj->description == NULL)
        g_object_notify (G_OBJECT (atk_obj), "accessible-description");
      return;
    }
  else if (g_strcmp0 (pspec->name, "visible") == 0)
    {
      state = ATK_STATE_VISIBLE;
      value = ctk_widget_get_visible (widget);
    }
  else if (g_strcmp0 (pspec->name, "sensitive") == 0)
    {
      state = ATK_STATE_SENSITIVE;
      value = ctk_widget_get_sensitive (widget);
    }
  else if (g_strcmp0 (pspec->name, "orientation") == 0 &&
           CTK_IS_ORIENTABLE (widget))
    {
      CtkOrientable *orientable;

      orientable = CTK_ORIENTABLE (widget);

      state = ATK_STATE_HORIZONTAL;
      value = (ctk_orientable_get_orientation (orientable) == CTK_ORIENTATION_HORIZONTAL);
    }
  else if (g_strcmp0 (pspec->name, "has-tooltip") == 0)
    {
      state = ATK_STATE_HAS_TOOLTIP;
      value = ctk_widget_get_has_tooltip (widget);
    }
  else
    return;

  atk_object_notify_state_change (atk_obj, state, value);
  if (state == ATK_STATE_SENSITIVE)
    atk_object_notify_state_change (atk_obj, ATK_STATE_ENABLED, value);

  if (state == ATK_STATE_HORIZONTAL)
    atk_object_notify_state_change (atk_obj, ATK_STATE_VERTICAL, !value);
}

static AtkAttributeSet *
ctk_widget_accessible_get_attributes (AtkObject *obj)
{
  AtkAttributeSet *attributes;
  AtkAttribute *toolkit;

  toolkit = g_new (AtkAttribute, 1);
  toolkit->name = g_strdup ("toolkit");
  toolkit->value = g_strdup ("ctk");

  attributes = g_slist_append (NULL, toolkit);

  return attributes;
}

static void
ctk_widget_accessible_class_init (CtkWidgetAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  klass->notify_ctk = ctk_widget_accessible_notify_ctk;

  class->get_description = ctk_widget_accessible_get_description;
  class->get_parent = ctk_widget_accessible_get_parent;
  class->ref_relation_set = ctk_widget_accessible_ref_relation_set;
  class->ref_state_set = ctk_widget_accessible_ref_state_set;
  class->get_index_in_parent = ctk_widget_accessible_get_index_in_parent;
  class->initialize = ctk_widget_accessible_initialize;
  class->get_attributes = ctk_widget_accessible_get_attributes;
  class->focus_event = ctk_widget_accessible_focus_event;
}

static void
ctk_widget_accessible_init (CtkWidgetAccessible *accessible)
{
  accessible->priv = ctk_widget_accessible_get_instance_private (accessible);
}

static void
ctk_widget_accessible_get_extents (AtkComponent   *component,
                                   gint           *x,
                                   gint           *y,
                                   gint           *width,
                                   gint           *height,
                                   AtkCoordType    coord_type)
{
  CdkWindow *window;
  gint x_window, y_window;
  gint x_toplevel, y_toplevel;
  CtkWidget *widget;
  CtkAllocation allocation;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (widget == NULL)
    return;

  ctk_widget_get_allocation (widget, &allocation);
  *width = allocation.width;
  *height = allocation.height;
  if (!ctk_widget_accessible_on_screen (widget) || (!ctk_widget_is_drawable (widget)))
    {
      *x = G_MININT;
      *y = G_MININT;
      return;
    }

  if (ctk_widget_get_parent (widget))
    {
      *x = allocation.x;
      *y = allocation.y;
      window = ctk_widget_get_parent_window (widget);
    }
  else
    {
      *x = 0;
      *y = 0;
      window = ctk_widget_get_window (widget);
    }
  cdk_window_get_origin (window, &x_window, &y_window);
  *x += x_window;
  *y += y_window;

  if (coord_type == ATK_XY_WINDOW)
    {
      window = cdk_window_get_toplevel (ctk_widget_get_window (widget));
      cdk_window_get_origin (window, &x_toplevel, &y_toplevel);

      *x -= x_toplevel;
      *y -= y_toplevel;
    }
}

static AtkLayer
ctk_widget_accessible_get_layer (AtkComponent *component)
{
  CtkWidgetAccessible *accessible = CTK_WIDGET_ACCESSIBLE (component);

  return accessible->priv->layer;
}

static gboolean
ctk_widget_accessible_grab_focus (AtkComponent *component)
{
  CtkWidget *widget;
  CtkWidget *toplevel;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (!widget)
    return FALSE;

  if (!ctk_widget_get_can_focus (widget))
    return FALSE;

  ctk_widget_grab_focus (widget);
  toplevel = ctk_widget_get_toplevel (widget);
  if (ctk_widget_is_toplevel (toplevel))
    {
#ifdef CDK_WINDOWING_X11
      if (CDK_IS_X11_DISPLAY (ctk_widget_get_display (toplevel)))
        ctk_window_present_with_time (CTK_WINDOW (toplevel),
                                      cdk_x11_get_server_time (ctk_widget_get_window (widget)));
      else
#endif
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          ctk_window_present (CTK_WINDOW (toplevel));
          G_GNUC_END_IGNORE_DEPRECATIONS
        }
    }

  return TRUE;
}

static gboolean
ctk_widget_accessible_set_extents (AtkComponent *component,
                                   gint          x,
                                   gint          y,
                                   gint          width,
                                   gint          height,
                                   AtkCoordType  coord_type)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (widget == NULL)
    return FALSE;

  if (!ctk_widget_is_toplevel (widget))
    return FALSE;

  if (coord_type == ATK_XY_WINDOW)
    {
      gint x_current, y_current;
      CdkWindow *window = ctk_widget_get_window (widget);

      cdk_window_get_origin (window, &x_current, &y_current);
      x_current += x;
      y_current += y;
      if (x_current < 0 || y_current < 0)
        return FALSE;
      else
        {
          ctk_window_move (CTK_WINDOW (widget), x_current, y_current);
          ctk_widget_set_size_request (widget, width, height);
          return TRUE;
        }
    }
  else if (coord_type == ATK_XY_SCREEN)
    {
      ctk_window_move (CTK_WINDOW (widget), x, y);
      ctk_widget_set_size_request (widget, width, height);
      return TRUE;
    }
  return FALSE;
}

static gboolean
ctk_widget_accessible_set_position (AtkComponent *component,
                                    gint          x,
                                    gint          y,
                                    AtkCoordType  coord_type)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (widget == NULL)
    return FALSE;

  if (ctk_widget_is_toplevel (widget))
    {
      if (coord_type == ATK_XY_WINDOW)
        {
          gint x_current, y_current;
          CdkWindow *window = ctk_widget_get_window (widget);

          cdk_window_get_origin (window, &x_current, &y_current);
          x_current += x;
          y_current += y;
          if (x_current < 0 || y_current < 0)
            return FALSE;
          else
            {
              ctk_window_move (CTK_WINDOW (widget), x_current, y_current);
              return TRUE;
            }
        }
      else if (coord_type == ATK_XY_SCREEN)
        {
          ctk_window_move (CTK_WINDOW (widget), x, y);
          return TRUE;
        }
    }
  return FALSE;
}

static gboolean
ctk_widget_accessible_set_size (AtkComponent *component,
                                gint          width,
                                gint          height)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (widget == NULL)
    return FALSE;

  if (ctk_widget_is_toplevel (widget))
    {
      ctk_widget_set_size_request (widget, width, height);
      return TRUE;
    }
  else
   return FALSE;
}

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = ctk_widget_accessible_get_extents;
  iface->get_layer = ctk_widget_accessible_get_layer;
  iface->grab_focus = ctk_widget_accessible_grab_focus;
  iface->set_extents = ctk_widget_accessible_set_extents;
  iface->set_position = ctk_widget_accessible_set_position;
  iface->set_size = ctk_widget_accessible_set_size;
}

/* This function checks whether the widget has an ancestor which is
 * a CtkViewport and, if so, whether any part of the widget intersects
 * the visible rectangle of the CtkViewport.
 */
static gboolean
ctk_widget_accessible_on_screen (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkWidget *viewport;
  gboolean return_value;

  ctk_widget_get_allocation (widget, &allocation);

  if (!ctk_widget_get_mapped (widget))
    return FALSE;

  viewport = ctk_widget_get_ancestor (widget, CTK_TYPE_VIEWPORT);
  if (viewport)
    {
      CtkAllocation viewport_allocation;
      CtkAdjustment *adjustment;
      CdkRectangle visible_rect;

      ctk_widget_get_allocation (viewport, &viewport_allocation);

      adjustment = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (viewport));
      visible_rect.y = ctk_adjustment_get_value (adjustment);
      adjustment = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (viewport));
      visible_rect.x = ctk_adjustment_get_value (adjustment);
      visible_rect.width = viewport_allocation.width;
      visible_rect.height = viewport_allocation.height;

      if (((allocation.x + allocation.width) < visible_rect.x) ||
         ((allocation.y + allocation.height) < visible_rect.y) ||
         (allocation.x > (visible_rect.x + visible_rect.width)) ||
         (allocation.y > (visible_rect.y + visible_rect.height)))
        return_value = FALSE;
      else
        return_value = TRUE;
    }
  else
    {
      /* Check whether the widget has been placed of the screen.
       * The widget may be MAPPED as when toolbar items do not
       * fit on the toolbar.
       */
      if (allocation.x + allocation.width <= 0 &&
          allocation.y + allocation.height <= 0)
        return_value = FALSE;
      else
        return_value = TRUE;
    }

  return return_value;
}

/* Checks if all the predecessors (the parent widget, his parent, etc)
 * are visible Used to check properly the SHOWING state.
 */
static gboolean
ctk_widget_accessible_all_parents_visible (CtkWidget *widget)
{
  CtkWidget *iter_parent = NULL;
  gboolean result = TRUE;

  for (iter_parent = ctk_widget_get_parent (widget); iter_parent;
       iter_parent = ctk_widget_get_parent (iter_parent))
    {
      if (!ctk_widget_get_visible (iter_parent))
        {
          result = FALSE;
          break;
        }
    }

  return result;
}

void
_ctk_widget_accessible_set_layer (CtkWidgetAccessible *accessible,
                                  AtkLayer             layer)
{
  accessible->priv->layer = layer;
}
