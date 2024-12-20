/*
 * Copyright (C) 2011 Red Hat Inc.
 *
 * Author:
 *      Matthias Clasen <mclasen@redhat.com>
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

#define CDK_DISABLE_DEPRECATION_WARNINGS
#undef CTK_DISABLE_DEPRECATED

#include <ctk/ctk.h>
#include <string.h>

typedef struct
{
  CtkWidget *widget;
  gpointer child[3];
} STATE;

static void
test_scrolled_window_child_count (void)
{
  CtkWidget *sw;
  AtkObject *accessible;

  sw = ctk_scrolled_window_new (NULL, NULL);
  g_object_ref_sink (sw);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_ALWAYS, CTK_POLICY_ALWAYS);
  ctk_container_add (CTK_CONTAINER (sw), ctk_label_new ("Bla"));

  accessible = ctk_widget_get_accessible (sw);
  g_assert_cmpint (atk_object_get_n_accessible_children (accessible), ==, 3);

  g_object_unref (sw);
}

typedef struct {
  gint count;
  gint index;
  gint n_children;
  gpointer parent;
} SignalData;

static void
children_changed (AtkObject  *accessible,
                  guint       index,
                  gpointer    child G_GNUC_UNUSED,
                  SignalData *data)
{
  data->count++;
  data->index = index;
  data->n_children = atk_object_get_n_accessible_children (accessible);
}

static void
remove_child (STATE *state,
              gint i)
{
  CtkWidget *child;

  if (CTK_IS_ENTRY (state->widget))
    {
      switch (i)
        {
        case 0:
          ctk_entry_set_icon_from_gicon (CTK_ENTRY (state->widget),
                                         CTK_ENTRY_ICON_PRIMARY,
                                         NULL);
        return;
        case 1:
          ctk_entry_set_icon_from_gicon (CTK_ENTRY (state->widget),
                                         CTK_ENTRY_ICON_SECONDARY,
                                         NULL);
        return;
        default:
          return;
        }
    }

  child = state->child [i];
  if (CTK_IS_SCROLLED_WINDOW (state->widget))
    {
      if (ctk_widget_get_parent (child) != state->widget)
        child = ctk_widget_get_parent (child);
    }

  ctk_container_remove (CTK_CONTAINER (state->widget), child);
}

static void
parent_notify (AtkObject  *obj,
	       GParamSpec *pspec G_GNUC_UNUSED,
	       SignalData *data)
{
  data->count++;
  data->parent = atk_object_get_parent (obj);
}

gboolean
do_create_child (STATE *state, gint i)
{
  if (CTK_IS_ENTRY (state->widget))
    {
      switch (i)
        {
        case 0:
          ctk_entry_set_icon_from_icon_name (CTK_ENTRY (state->widget),
                                             CTK_ENTRY_ICON_PRIMARY,
                                             "dialog-warning-symbolic");
        return TRUE;
        case 1:
          ctk_entry_set_icon_from_icon_name (CTK_ENTRY (state->widget),
                                             CTK_ENTRY_ICON_SECONDARY,
                                             "edit-clear");
        return TRUE;
        default:
          return FALSE;
        }
    }
  else if (ctk_container_child_type (CTK_CONTAINER (state->widget)) == G_TYPE_NONE)
    return FALSE;

  state->child[i] = ctk_label_new ("bla");
  return TRUE;
}

static void
test_add_remove (CtkWidget *widget)
{
  AtkObject *accessible;
  AtkObject *child_accessible;
  SignalData add_data;
  SignalData remove_data;
  SignalData parent_data[3] = { { 0, }, };
  STATE state;
  gint i, j;
  gint step_children;

  state.widget = widget;
  accessible = ctk_widget_get_accessible (widget);

  add_data.count = 0;
  remove_data.count = 0;
  g_signal_connect (accessible, "children_changed::add",
                    G_CALLBACK (children_changed), &add_data);
  g_signal_connect (accessible, "children_changed::remove",
                    G_CALLBACK (children_changed), &remove_data);

  step_children = atk_object_get_n_accessible_children (accessible);

  for (i = 0; i < 3; i++)
    {
      if (!do_create_child (&state, i))
        break;
      if (!CTK_IS_ENTRY (widget))
        {
          parent_data[i].count = 0;
          child_accessible = ctk_widget_get_accessible (state.child[i]);
          g_signal_connect (child_accessible, "notify::accessible-parent",
                            G_CALLBACK (parent_notify), &(parent_data[i]));
          ctk_container_add (CTK_CONTAINER (widget), state.child[i]);
        }
      else
        child_accessible = atk_object_ref_accessible_child (accessible, i);

      g_assert_cmpint (add_data.count, ==, i + 1);
      g_assert_cmpint (add_data.n_children, ==, step_children + i + 1);
      g_assert_cmpint (remove_data.count, ==, 0);
      if (!CTK_IS_ENTRY (widget))
        g_assert_cmpint (parent_data[i].count, ==, 1);
      if (CTK_IS_SCROLLED_WINDOW (widget) ||
          CTK_IS_NOTEBOOK (widget))
        g_assert (atk_object_get_parent (ATK_OBJECT (parent_data[i].parent)) == accessible);
      else if (CTK_IS_ENTRY (widget))
        g_assert (atk_object_get_parent (child_accessible) == accessible);
      else
        g_assert (parent_data[i].parent == accessible);

      if (CTK_IS_ENTRY (widget))
        g_object_unref (child_accessible);
    }
  for (j = 0 ; j < i; j++)
    {
      remove_child (&state, j);
      g_assert_cmpint (add_data.count, ==, i);
      g_assert_cmpint (remove_data.count, ==, j + 1);
      g_assert_cmpint (remove_data.n_children, ==, step_children + i - j - 1);
      if (parent_data[j].count == 2)
        g_assert (parent_data[j].parent == NULL);
      else if (!CTK_IS_ENTRY (widget))
        {
          AtkStateSet *set;
          set = atk_object_ref_state_set (ATK_OBJECT (parent_data[j].parent));
          g_assert (atk_state_set_contains_state (set, ATK_STATE_DEFUNCT));
          g_object_unref (set);
        }
    }

  g_signal_handlers_disconnect_by_func (accessible, G_CALLBACK (children_changed), &add_data);
  g_signal_handlers_disconnect_by_func (accessible, G_CALLBACK (children_changed), &remove_data);
}

static void
add_child_test (const gchar      *prefix,
                GTestFixtureFunc  test_func,
                CtkWidget        *widget)
{
  gchar *path;

  path = g_strdup_printf ("%s/%s", prefix, G_OBJECT_TYPE_NAME (widget));
  g_test_add_vtable (path,
                     0,
                     g_object_ref (widget),
                     0,
                     (GTestFixtureFunc) test_func,
                     (GTestFixtureFunc) g_object_unref);
  g_free (path);
}

static void
add_child_tests (CtkWidget *widget)
{
  g_object_ref_sink (widget);
  add_child_test ("/child/add-remove", (GTestFixtureFunc)test_add_remove, widget);
  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/scrolledwindow/child-count", test_scrolled_window_child_count);

  add_child_tests (ctk_scrolled_window_new (NULL, NULL));
  add_child_tests (ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0));
  add_child_tests (ctk_paned_new (CTK_ORIENTATION_HORIZONTAL));
  add_child_tests (ctk_grid_new ());
  add_child_tests (ctk_event_box_new ());
  add_child_tests (ctk_window_new (CTK_WINDOW_TOPLEVEL));
  add_child_tests (ctk_assistant_new ());
  add_child_tests (ctk_frame_new ("frame"));
  add_child_tests (ctk_expander_new ("expander"));
  add_child_tests (ctk_table_new (2, 2, FALSE));
  add_child_tests (ctk_text_view_new ());
  add_child_tests (ctk_tree_view_new ());
#if 0
  /* cail doesn't handle non-label children in these */
  add_child_tests (ctk_button_new ());
  add_child_tests (ctk_statusbar_new ());
#endif
  add_child_tests (ctk_notebook_new ());
  add_child_tests (ctk_entry_new ());

  return g_test_run ();
}

