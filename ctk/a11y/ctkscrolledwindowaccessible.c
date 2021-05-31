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
#include "ctkscrolledwindowaccessible.h"


G_DEFINE_TYPE (CtkScrolledWindowAccessible, ctk_scrolled_window_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE)

static void
visibility_changed (GObject    *object,
                    GParamSpec *pspec,
                    gpointer    user_data)
{
  if (!g_strcmp0 (pspec->name, "visible"))
    {
      gint index;
      gint n_children;
      gboolean child_added = FALSE;
      GList *children;
      AtkObject *child;
      CtkWidget *widget;
      CtkScrolledWindow *scrolled_window;
      CtkWidget *hscrollbar, *vscrollbar;
      CtkAccessible *accessible = CTK_ACCESSIBLE (user_data);

      widget = ctk_accessible_get_widget (user_data);
      if (widget == NULL)
        return;

      scrolled_window = CTK_SCROLLED_WINDOW (widget);
      children = ctk_container_get_children (CTK_CONTAINER (widget));
      index = n_children = g_list_length (children);
      g_list_free (children);

      hscrollbar = ctk_scrolled_window_get_hscrollbar (scrolled_window);
      vscrollbar = ctk_scrolled_window_get_vscrollbar (scrolled_window);

      if ((gpointer) object == (gpointer) (hscrollbar))
        {
          if (ctk_scrolled_window_get_hscrollbar (scrolled_window))
            child_added = TRUE;

          child = ctk_widget_get_accessible (hscrollbar);
        }
      else if ((gpointer) object == (gpointer) (vscrollbar))
        {
          if (ctk_scrolled_window_get_vscrollbar (scrolled_window))
            child_added = TRUE;

          child = ctk_widget_get_accessible (vscrollbar);
          if (ctk_scrolled_window_get_hscrollbar (scrolled_window))
            index = n_children + 1;
        }
      else
        {
          g_assert_not_reached ();
          return;
        }

      if (child_added)
        g_signal_emit_by_name (accessible, "children-changed::add", index, child, NULL);
      else
        g_signal_emit_by_name (accessible, "children-changed::remove", index, child, NULL);

    }
}

static void
ctk_scrolled_window_accessible_initialize (AtkObject *obj,
                                           gpointer  data)
{
  CtkScrolledWindow *window;

  ATK_OBJECT_CLASS (ctk_scrolled_window_accessible_parent_class)->initialize (obj, data);

  window = CTK_SCROLLED_WINDOW (data);

  g_signal_connect_object (ctk_scrolled_window_get_hscrollbar (window), "notify::visible",
                           G_CALLBACK (visibility_changed),
                           obj, 0);
  g_signal_connect_object (ctk_scrolled_window_get_vscrollbar (window), "notify::visible",
                           G_CALLBACK (visibility_changed),
                           obj, 0);

  obj->role = ATK_ROLE_SCROLL_PANE;
}

static gint
ctk_scrolled_window_accessible_get_n_children (AtkObject *object)
{
  CtkWidget *widget;
  CtkScrolledWindow *scrolled_window;
  GList *children;
  gint n_children;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (object));
  if (widget == NULL)
    return 0;

  scrolled_window = CTK_SCROLLED_WINDOW (widget);

  children = ctk_container_get_children (CTK_CONTAINER (widget));
  n_children = g_list_length (children);
  g_list_free (children);

  if (ctk_scrolled_window_get_hscrollbar (scrolled_window))
    n_children++;
  if (ctk_scrolled_window_get_vscrollbar (scrolled_window))
    n_children++;

  return n_children;
}

static AtkObject *
ctk_scrolled_window_accessible_ref_child (AtkObject *obj,
                                          gint       child)
{
  CtkWidget *widget;
  CtkScrolledWindow *scrolled_window;
  CtkWidget *hscrollbar, *vscrollbar;
  GList *children, *tmp_list;
  gint n_children;
  AtkObject  *accessible = NULL;

  g_return_val_if_fail (child >= 0, NULL);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  scrolled_window = CTK_SCROLLED_WINDOW (widget);
  hscrollbar = ctk_scrolled_window_get_hscrollbar (scrolled_window);
  vscrollbar = ctk_scrolled_window_get_vscrollbar (scrolled_window);

  children = ctk_container_get_children (CTK_CONTAINER (widget));
  n_children = g_list_length (children);

  if (child == n_children)
    {
      if (ctk_scrolled_window_get_hscrollbar (scrolled_window))
        accessible = ctk_widget_get_accessible (hscrollbar);
      else if (ctk_scrolled_window_get_vscrollbar (scrolled_window))
        accessible = ctk_widget_get_accessible (vscrollbar);
    }
  else if (child == n_children + 1 &&
           ctk_scrolled_window_get_hscrollbar (scrolled_window) &&
           ctk_scrolled_window_get_vscrollbar (scrolled_window))
    accessible = ctk_widget_get_accessible (vscrollbar);
  else if (child < n_children)
    {
      tmp_list = g_list_nth (children, child);
      if (tmp_list)
        accessible = ctk_widget_get_accessible (CTK_WIDGET (tmp_list->data));
    }

  g_list_free (children);
  if (accessible)
    g_object_ref (accessible);

  return accessible;
}

static void
ctk_scrolled_window_accessible_class_init (CtkScrolledWindowAccessibleClass *klass)
{
  AtkObjectClass  *class = ATK_OBJECT_CLASS (klass);

  class->get_n_children = ctk_scrolled_window_accessible_get_n_children;
  class->ref_child = ctk_scrolled_window_accessible_ref_child;
  class->initialize = ctk_scrolled_window_accessible_initialize;
}

static void
ctk_scrolled_window_accessible_init (CtkScrolledWindowAccessible *window)
{
}
