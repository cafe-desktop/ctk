/*
 * Copyright (C) 2011 SUSE Linux Products GmbH, Nurenberg, Germany
 *
 * Author:
 *      Mike Gorse <mgorse@suse.com>
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

#include <ctk/ctk.h>

const gchar list_ui[] =
  "<interface>"
  "  <object class='CtkListStore' id='liststore1'>"
  "    <columns>"
  "      <column type='gchararray'/>"
  "      <column type='gchararray'/>"
  "      <column type='gchararray'/>"
  "      <column type='gboolean'/>"
  "      <column type='gint'/>"
  "      <column type='gint'/>"
  "    </columns>"
  "    <data>"
  "      <row><col id='0'>One</col><col id='1'>Two</col><col id='2'>Three</col><col id='3'>True</col><col id='4'>50</col><col id='5'>50</col></row>"
  "    </data>"
  "  </object>"
  "  <object class='CtkWindow' id='window1'>"
  "    <child>"
  "      <object class='CtkTreeView' id='treeview1'>"
  "        <property name='visible'>True</property>"
  "        <property name='model'>liststore1</property>"
  "        <child>"
  "          <object class='CtkTreeViewColumn' id='column1'>"
  "            <property name='title' translatable='yes'>First column</property>"
  "            <child>"
  "              <object class='CtkCellRendererText' id='renderer1'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='text'>0</attribute>"
  "              </attributes>"
  "            </child>"
  "            <child>"
  "              <object class='CtkCellRendererToggle' id='renderer2'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='active'>3</attribute>"
  "              </attributes>"
  "            </child>"
  "          </object>"
  "        </child>"
  "        <child>"
  "          <object class='CtkTreeViewColumn' id='column2'>"
  "            <property name='title' translatable='yes'>Second column</property>"
  "            <child>"
  "              <object class='CtkCellRendererText' id='renderer3'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='text'>1</attribute>"
  "              </attributes>"
  "            </child>"
  "            <child>"
  "              <object class='CtkCellRendererProgress' id='renderer4'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='value'>4</attribute>"
  "              </attributes>"
  "            </child>"
  "          </object>"
  "        </child>"
  "      </object>"
  "    </child>"
  "  </object>"
  "</interface>";

static CtkWidget *
builder_get_toplevel (CtkBuilder *builder)
{
  GSList *list, *walk;
  CtkWidget *window = NULL;

  list = ctk_builder_get_objects (builder);
  for (walk = list; walk; walk = walk->next)
    {
      if (CTK_IS_WINDOW (walk->data) &&
          ctk_widget_get_parent (walk->data) == NULL)
        {
          window = walk->data;
          break;
        }
    }

  g_slist_free (list);

  return window;
}

const gchar tree_ui[] =
  "<interface>"
  "  <object class='CtkTreeStore' id='treestore1'>"
  "    <columns>"
  "      <column type='gchararray'/>"
  "      <column type='gchararray'/>"
  "      <column type='gchararray'/>"
  "      <column type='gboolean'/>"
  "      <column type='gint'/>"
  "      <column type='gint'/>"
  "    </columns>"
  "  </object>"
  "  <object class='CtkWindow' id='window1'>"
  "    <child>"
  "      <object class='CtkTreeView' id='treeview1'>"
  "        <property name='visible'>True</property>"
  "        <property name='model'>treestore1</property>"
  "        <child>"
  "          <object class='CtkTreeViewColumn' id='column1'>"
  "            <property name='title' translatable='yes'>First column</property>"
  "            <child>"
  "              <object class='CtkCellRendererText' id='renderer1'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='text'>0</attribute>"
  "              </attributes>"
  "            </child>"
  "            <child>"
  "              <object class='CtkCellRendererToggle' id='renderer2'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='active'>3</attribute>"
  "              </attributes>"
  "            </child>"
  "          </object>"
  "        </child>"
  "        <child>"
  "          <object class='CtkTreeViewColumn' id='column2'>"
  "            <property name='title' translatable='yes'>Second column</property>"
  "            <child>"
  "              <object class='CtkCellRendererText' id='renderer3'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='text'>1</attribute>"
  "              </attributes>"
  "            </child>"
  "            <child>"
  "              <object class='CtkCellRendererProgress' id='renderer4'>"
  "              </object>"
  "              <attributes>"
  "                <attribute name='value'>4</attribute>"
  "              </attributes>"
  "            </child>"
  "          </object>"
  "        </child>"
  "      </object>"
  "    </child>"
  "  </object>"
  "</interface>";

static void
populate_tree (CtkBuilder *builder)
{
  CtkTreeView *tv;
  CtkTreeStore *store;
  CtkTreeIter iter;

  tv = (CtkTreeView *)ctk_builder_get_object (builder, "treeview1");
  store = (CtkTreeStore *)ctk_tree_view_get_model (tv);

  /* append some rows */
  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "a(1)", 1, "a(2)", 2, "a(3)", 3, TRUE, 4, 0, 5, 0, -1);
  ctk_tree_store_append (store, &iter, &iter);
  ctk_tree_store_set (store, &iter, 0, "aa(1)", 1, "aa(2)", 2, "aa(3)", 3, TRUE, 4, 0, 5, 0, -1);
      ctk_tree_store_append (store, &iter, &iter);
  ctk_tree_store_set (store, &iter, 0, "aaa(1)", 1, "aaa(2)", 2, "aaa(3)", 3, TRUE, 4, 0, 5, 0, -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "b(1)", 1, "b(2)", 2, "b(3)", 3, TRUE, 4, 0, 5, 0, -1);
  ctk_tree_store_append (store, &iter, &iter);
  ctk_tree_store_set (store, &iter, 0, "bb(1)", 1, "bb(2)", 2, "bb(3)", 3, TRUE, 4, 0, 5, 0, -1);
      ctk_tree_store_append (store, &iter, &iter);
  ctk_tree_store_set (store, &iter, 0, "bbb(1)", 1, "bbb(2)", 2, "bbb(3)", 3, TRUE, 4, 0, 5, 0, -1);
}

typedef struct {
  gint count;
  AtkObject *descendant;
} SignalData;

static void
active_descendant_changed (AtkObject  *accessible G_GNUC_UNUSED,
			   AtkObject  *descendant,
			   SignalData *data)
{
  data->count++;
  if (data->descendant)
    g_object_unref (data->descendant);
  data->descendant = g_object_ref (descendant);
}

static gboolean
quit_loop (gpointer data)
{
  GMainLoop *loop = data;
  g_main_loop_quit (loop);
  return FALSE;
}

static void
process_pending_idles ()
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  cdk_threads_add_idle (quit_loop, loop);
  g_main_loop_run (loop);
}

static void
test_a11y_tree_focus (void)
{
  CtkBuilder *builder;
  CtkWidget *window;
  GError *error = NULL;
  CtkTreeView *tv;
  CtkTreePath *path = NULL;
  CtkTreeViewColumn *focus_column = NULL;
  SignalData data;
  AtkObject *accessible;
  AtkObject *child;
  gchar *text;

  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder, tree_ui, -1, &error);
  g_assert_no_error (error);
  window = builder_get_toplevel (builder);
  g_assert (window);

  populate_tree (builder);

  tv = (CtkTreeView *)ctk_builder_get_object (builder, "treeview1");
  ctk_tree_view_expand_all (tv);

  ctk_widget_show (window);

  ctk_tree_view_get_cursor (tv, &path, &focus_column);
  ctk_tree_path_down (path);
  data.count = 0;
  data.descendant = NULL;
  accessible = ctk_widget_get_accessible (CTK_WIDGET (tv));
  g_signal_connect (accessible, "active_descendant_changed",
                    G_CALLBACK (active_descendant_changed), &data);
  ctk_tree_view_set_cursor (tv, path, focus_column, FALSE);
  /* hack: active_descendant_change gets fired in an idle handler */
  process_pending_idles ();
  /* Getting only one signal might be ideal, although we get three or so */
  g_assert_cmpint (data.count, >=, 1);
  child = atk_object_ref_accessible_child (data.descendant, 0);
  text = atk_text_get_text (ATK_TEXT (child), 0, -1);
  g_assert_cmpstr (text, ==, "aa(1)");
  g_free (text);

  g_object_unref (child);

  g_object_unref (data.descendant);
  g_object_unref (builder);

}

static AtkObject *
find_root_accessible (CtkTreeView *tv, const char *name)
{
  AtkObject *tvaccessible = ctk_widget_get_accessible (CTK_WIDGET (tv));
  int i = 0;

  for (i = 0;;i++)
  {
    AtkObject *child = atk_object_ref_accessible_child (tvaccessible, i);
    AtkObject *item;
    gchar *text;
    if (!child)
      return NULL;
    item = atk_object_ref_accessible_child (child, 0);
    if (!item)
      continue;
    text = atk_text_get_text (ATK_TEXT (item), 0, -1);
    g_object_unref (item);
    if (!g_strcmp0 (text, name))
    {
      g_free (text);
      return child;
    }
    g_free (text);
    g_object_unref (child);
  }
}

static void
test_node_child_of (AtkObject *child, AtkObject *parent)
{
  AtkRelationSet *set = atk_object_ref_relation_set (child);
  AtkRelation *relation = atk_relation_set_get_relation_by_type (set, ATK_RELATION_NODE_CHILD_OF);

  g_assert (relation != NULL);
  g_assert_cmpint (relation->target->len, ==, 1);
  g_assert (g_ptr_array_index (relation->target, 0) == parent);
  g_object_unref (set);
}

static void
test_node_parent_of (AtkObject *parent, AtkObject *child)
{
  AtkRelationSet *set = atk_object_ref_relation_set (parent);
  AtkRelation *relation = atk_relation_set_get_relation_by_type (set, ATK_RELATION_NODE_PARENT_OF);

  g_assert (relation != NULL);
  g_assert_cmpint (relation->target->len, ==, 1);
  g_assert (g_ptr_array_index (relation->target, 0) == child);
  g_object_unref (set);
}

static void
test_relations (AtkObject *parent, AtkObject *child)
{
  test_node_parent_of (parent, child);
  test_node_child_of (child, parent);
}

static void
test_a11y_tree_relations (void)
{
  CtkBuilder *builder;
  CtkWidget *window;
  GError *error = NULL;
  CtkTreeView *tv;
  AtkObject *parent, *child;

  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder, tree_ui, -1, &error);
  g_assert_no_error (error);
  window = builder_get_toplevel (builder);
  g_assert (window);

  populate_tree (builder);

  tv = (CtkTreeView *)ctk_builder_get_object (builder, "treeview1");
  ctk_tree_view_expand_all (tv);

  ctk_widget_show (window);

  parent = find_root_accessible (tv, "a(1)");
child = find_root_accessible (tv, "aa(1)");
  test_relations (parent, child);
  g_object_unref (parent);
  parent = child;
child = find_root_accessible (tv, "aaa(1)");
  test_relations (parent, child);
  g_object_unref (parent);
  g_object_unref (child);

  g_object_unref (builder);

}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/tree/focus", test_a11y_tree_focus);
  g_test_add_func ("/a11y/tree/relations", test_a11y_tree_relations);

  return g_test_run ();
}

