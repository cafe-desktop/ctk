/* testgmenu.c
 * Copyright (C) 2011  Red Hat, Inc.
 * Written by Matthias Clasen
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

#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

/* TODO
 *
 * - Labeled sections
 *
 * - Focus changes. Verify that stopping subscriptions works.
 *
 * - Other attributes. What about icons ?
 */

/* The example menu {{{1 */

static const gchar menu_markup[] =
  "<interface>\n"
  "<menu id='edit-menu'>\n"
  "  <section>\n"
  "    <item>\n"
  "      <attribute name='action'>actions.undo</attribute>\n"
  "      <attribute name='label' translatable='yes' context='Stock label'>_Undo</attribute>\n"
  "    </item>\n"
  "    <item>\n"
  "      <attribute name='label' translatable='yes'>Redo</attribute>\n"
  "      <attribute name='action'>actions.redo</attribute>\n"
  "    </item>\n"
  "  </section>\n"
  "  <section/>\n"
  "  <section>\n"
  "    <attribute name='label' translatable='yes'>Copy &amp; Paste</attribute>\n"
  "    <item>\n"
  "      <attribute name='label' translatable='yes'>Cut</attribute>\n"
  "      <attribute name='action'>actions.cut</attribute>\n"
  "    </item>\n"
  "    <item>\n"
  "      <attribute name='label' translatable='yes'>Copy</attribute>\n"
  "      <attribute name='action'>actions.copy</attribute>\n"
  "    </item>\n"
  "    <item>\n"
  "      <attribute name='label' translatable='yes'>Paste</attribute>\n"
  "      <attribute name='action'>actions.paste</attribute>\n"
  "    </item>\n"
  "  </section>\n"
  "  <section>\n"
  "    <item>\n"
  "      <attribute name='label' translatable='yes'>Bold</attribute>\n"
  "      <attribute name='action'>actions.bold</attribute>\n"
  "    </item>\n"
  "    <section id=\"size-placeholder\">\n"
  "      <attribute name=\"label\">Size</attribute>"
  "    </section>\n"
  "    <submenu>\n"
  "      <attribute name='label' translatable='yes'>Language</attribute>\n"
  "      <item>\n"
  "        <attribute name='label' translatable='yes'>Latin</attribute>\n"
  "        <attribute name='action'>actions.lang</attribute>\n"
  "        <attribute name='target'>latin</attribute>\n"
  "      </item>\n"
  "      <item>\n"
  "        <attribute name='label' translatable='yes'>Greek</attribute>\n"
  "        <attribute name='action'>actions.lang</attribute>\n"
  "        <attribute name='target'>greek</attribute>\n"
  "      </item>\n"
  "      <item>\n"
  "        <attribute name='label' translatable='yes'>Urdu</attribute>\n"
  "        <attribute name='action'>actions.lang</attribute>\n"
  "        <attribute name='target'>urdu</attribute>\n"
  "      </item>\n"
  "    </submenu>\n"
  "  </section>\n"
  "</menu>\n"
  "</interface>\n";

static GMenuModel *
get_model (void)
{
  GError *error = NULL;
  CtkBuilder *builder;
  GMenuModel *menu, *section;
  float i;

  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder, menu_markup, -1, &error);
  g_assert_no_error (error);

  menu = G_MENU_MODEL (g_object_ref (ctk_builder_get_object (builder, "edit-menu")));

  section = G_MENU_MODEL (g_object_ref (ctk_builder_get_object (builder, "size-placeholder")));
  g_object_unref (builder);

  for (i = 0.5; i <= 2.0; i += 0.5)
    {
      GMenuItem *item;
      char *target;
      char *label;

      target = g_strdup_printf ("actions.size::%.1f", i);
      label = g_strdup_printf ("x %.1f", i);
      item = g_menu_item_new (label, target);
      g_menu_append_item (G_MENU (section), item);
      g_free (label);
      g_free (target);
    }

  return menu;
}

/* The example actions {{{1 */

static void
activate_action (GSimpleAction *action,
		 GVariant      *parameter G_GNUC_UNUSED,
		 gpointer       user_data G_GNUC_UNUSED)
{
  g_print ("Action %s activated\n", g_action_get_name (G_ACTION (action)));
}

static void
activate_toggle (GSimpleAction *action,
		 GVariant      *parameter G_GNUC_UNUSED,
		 gpointer       user_data G_GNUC_UNUSED)
{
  GVariant *old_state, *new_state;

  old_state = g_action_get_state (G_ACTION (action));
  new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));

  g_print ("Toggle action %s activated, state changes from %d to %d\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_boolean (old_state),
           g_variant_get_boolean (new_state));

  g_simple_action_set_state (action, new_state);
  g_variant_unref (old_state);
}

static void
activate_radio (GSimpleAction *action,
		GVariant      *parameter,
		gpointer       user_data G_GNUC_UNUSED)
{
  GVariant *old_state, *new_state;

  old_state = g_action_get_state (G_ACTION (action));
  new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));

  g_print ("Radio action %s activated, state changes from %s to %s\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_string (old_state, NULL),
           g_variant_get_string (new_state, NULL));

  g_simple_action_set_state (action, new_state);
  g_variant_unref (old_state);
}

static GActionEntry actions[] = {
  { .name = "undo",  .activate = activate_action },
  { .name = "redo",  .activate = activate_action },
  { .name = "cut",   .activate = activate_action },
  { .name = "copy",  .activate = activate_action },
  { .name = "paste", .activate = activate_action },
  { .name = "bold",  .activate = activate_toggle, .state = "true" },
  { .name = "lang",  .activate = activate_radio, .parameter_type = "s", .state = "'latin'" },
};

static GActionGroup *
get_group (void)
{
  GSimpleActionGroup *group;

  group = g_simple_action_group_new ();

  g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS (actions), NULL);

  return G_ACTION_GROUP (group);
}
 
/* The action treeview {{{1 */

static void
enabled_cell_func (CtkTreeViewColumn *column G_GNUC_UNUSED,
                   CtkCellRenderer   *cell,
                   CtkTreeModel      *model,
                   CtkTreeIter       *iter,
                   gpointer           data)
{
  GActionGroup *group = data;
  gchar *name;
  gboolean enabled;

  ctk_tree_model_get (model, iter, 0, &name, -1);
  enabled = g_action_group_get_action_enabled (group, name);
  g_free (name);

  ctk_cell_renderer_toggle_set_active (CTK_CELL_RENDERER_TOGGLE (cell), enabled);
}

static void
state_cell_func (CtkTreeViewColumn *column G_GNUC_UNUSED,
                 CtkCellRenderer   *cell,
                 CtkTreeModel      *model,
                 CtkTreeIter       *iter,
                 gpointer           data)
{
  GActionGroup *group = data;
  gchar *name;
  GVariant *state;

  ctk_tree_model_get (model, iter, 0, &name, -1);
  state = g_action_group_get_action_state (group, name);
  g_free (name);

  ctk_cell_renderer_set_visible (cell, FALSE);
  g_object_set (cell, "mode", CTK_CELL_RENDERER_MODE_INERT, NULL);

  if (state == NULL)
    return;

  if (g_variant_is_of_type (state, G_VARIANT_TYPE_BOOLEAN) &&
      CTK_IS_CELL_RENDERER_TOGGLE (cell))
    {
      ctk_cell_renderer_set_visible (cell, TRUE);
      g_object_set (cell, "mode", CTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
      ctk_cell_renderer_toggle_set_active (CTK_CELL_RENDERER_TOGGLE (cell),
                                           g_variant_get_boolean (state));
    }
  else if (g_variant_is_of_type (state, G_VARIANT_TYPE_STRING) &&
           CTK_IS_CELL_RENDERER_COMBO (cell))
    {
      ctk_cell_renderer_set_visible (cell, TRUE);
      g_object_set (cell, "mode", CTK_CELL_RENDERER_MODE_EDITABLE, NULL);
      g_object_set (cell, "text", g_variant_get_string (state, NULL), NULL);
    }

  g_variant_unref (state);
}

static void
enabled_cell_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
                      const gchar           *path_str,
                      CtkTreeModel          *model)
{
  GActionGroup *group;
  GAction *action;
  gchar *name;
  CtkTreePath *path;
  CtkTreeIter iter;
  gboolean enabled;

  group = g_object_get_data (G_OBJECT (model), "group");
  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, 0, &name, -1);

  enabled = g_action_group_get_action_enabled (group, name);
  action = g_action_map_lookup_action (G_ACTION_MAP (group), name);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !enabled);

  ctk_tree_model_row_changed (model, path, &iter);

  g_free (name);
  ctk_tree_path_free (path);
}

static void
state_cell_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
                    const gchar           *path_str,
                    CtkTreeModel          *model)
{
  GActionGroup *group;
  GAction *action;
  gchar *name;
  CtkTreePath *path;
  CtkTreeIter iter;
  GVariant *state;

  group = g_object_get_data (G_OBJECT (model), "group");
  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, 0, &name, -1);

  state = g_action_group_get_action_state (group, name);
  action = g_action_map_lookup_action (G_ACTION_MAP (group), name);
  if (state && g_variant_is_of_type (state, G_VARIANT_TYPE_BOOLEAN))
    {
      gboolean b;

      b = g_variant_get_boolean (state);
      g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (!b));
    }
  else
    {
      /* nothing to do */
    }

  ctk_tree_model_row_changed (model, path, &iter);

  g_free (name);
  ctk_tree_path_free (path);
  if (state)
    g_variant_unref (state);
}

static void
state_cell_edited (CtkCellRendererCombo *cell G_GNUC_UNUSED,
                   const gchar          *path_str,
                   const gchar          *new_text,
                   CtkTreeModel         *model)
{
  GActionGroup *group;
  GAction *action;
  gchar *name;
  CtkTreePath *path;
  CtkTreeIter iter;

  group = g_object_get_data (G_OBJECT (model), "group");
  path = ctk_tree_path_new_from_string (path_str);
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, 0, &name, -1);
  action = g_action_map_lookup_action (G_ACTION_MAP (group), name);
  g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_string (new_text));

  ctk_tree_model_row_changed (model, path, &iter);

  g_free (name);
  ctk_tree_path_free (path);
}

static CtkWidget *
create_action_treeview (GActionGroup *group)
{
  CtkWidget *tv;
  CtkListStore *store;
  CtkListStore *values;
  CtkTreeIter iter;
  CtkTreeViewColumn *column;
  CtkCellRenderer *cell;
  gchar **actions;
  gint i;

  store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  actions = g_action_group_list_actions (group);
  for (i = 0; actions[i]; i++)
    {
      ctk_list_store_append (store, &iter);
      ctk_list_store_set (store, &iter, 0, actions[i], -1);
    }
  g_strfreev (actions);
  g_object_set_data (G_OBJECT (store), "group", group);

  tv = ctk_tree_view_new ();

  g_signal_connect_swapped (group, "action-enabled-changed",
                            G_CALLBACK (ctk_widget_queue_draw), tv);
  g_signal_connect_swapped (group, "action-state-changed",
                            G_CALLBACK (ctk_widget_queue_draw), tv);

  ctk_tree_view_set_model (CTK_TREE_VIEW (tv), CTK_TREE_MODEL (store));

  cell = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Action", cell,
                                                     "text", 0,
                                                     NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tv), column);

  column = ctk_tree_view_column_new ();
  ctk_tree_view_column_set_title (column, "Enabled");
  cell = ctk_cell_renderer_toggle_new ();
  ctk_tree_view_column_pack_start (column, cell, FALSE);
  ctk_tree_view_column_set_cell_data_func (column, cell, enabled_cell_func, group, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (enabled_cell_toggled), store);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tv), column);

  column = ctk_tree_view_column_new ();
  ctk_tree_view_column_set_title (column, "State");
  cell = ctk_cell_renderer_toggle_new ();
  ctk_tree_view_column_pack_start (column, cell, FALSE);
  ctk_tree_view_column_set_cell_data_func (column, cell, state_cell_func, group, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (state_cell_toggled), store);
  cell = ctk_cell_renderer_combo_new ();
  values = ctk_list_store_new (1, G_TYPE_STRING);
  ctk_list_store_append (values, &iter);
  ctk_list_store_set (values, &iter, 0, "latin", -1);
  ctk_list_store_append (values, &iter);
  ctk_list_store_set (values, &iter, 0, "greek", -1);
  ctk_list_store_append (values, &iter);
  ctk_list_store_set (values, &iter, 0, "urdu", -1);
  ctk_list_store_append (values, &iter);
  ctk_list_store_set (values, &iter, 0, "sumerian", -1);
  g_object_set (cell,
                "has-entry", FALSE,
                "model", values,
                "text-column", 0,
                "editable", TRUE,
                NULL);
  ctk_tree_view_column_pack_start (column, cell, FALSE);
  ctk_tree_view_column_set_cell_data_func (column, cell, state_cell_func, group, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (state_cell_edited), store);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tv), column);

  return tv;
}

/* Dynamic menu changes {{{1 */

static void
toggle_sumerian (CtkToggleButton *button,
		 gpointer         data G_GNUC_UNUSED)
{
  GMenuModel *model;
  gboolean adding;
  GMenuModel *m;

  model = g_object_get_data (G_OBJECT (button), "model");

  adding = ctk_toggle_button_get_active (button);

  m = g_menu_model_get_item_link (model, g_menu_model_get_n_items (model) - 1, G_MENU_LINK_SECTION);
  m = g_menu_model_get_item_link (m, g_menu_model_get_n_items (m) - 1, G_MENU_LINK_SUBMENU);
  if (adding)
    g_menu_append (G_MENU (m), "Sumerian", "lang::sumerian");
  else
    g_menu_remove (G_MENU (m), g_menu_model_get_n_items (m) - 1);
}

static void
action_list_add (CtkTreeModel *store,
                 const gchar  *action)
{
  CtkTreeIter iter;

  ctk_list_store_append (CTK_LIST_STORE (store), &iter);
  ctk_list_store_set (CTK_LIST_STORE (store), &iter, 0, action, -1);
}

static void
action_list_remove (CtkTreeModel *store,
                    const gchar  *action)
{
  CtkTreeIter iter;
  gchar *text;

  ctk_tree_model_get_iter_first (store, &iter);
  do {
    ctk_tree_model_get (store, &iter, 0, &text, -1);
    if (g_strcmp0 (action, text) == 0)
      {
        g_free (text);
        ctk_list_store_remove (CTK_LIST_STORE (store), &iter);
        break;
      }
    g_free (text);
  } while (ctk_tree_model_iter_next (store, &iter));
}

static void
toggle_italic (CtkToggleButton *button, gpointer data)
{
  GMenuModel *model;
  GActionGroup *group;
  gboolean adding;
  GMenuModel *m;
  CtkTreeView *tv = data;
  CtkTreeModel *store;

  model = g_object_get_data (G_OBJECT (button), "model");
  group = g_object_get_data (G_OBJECT (button), "group");

  store = ctk_tree_view_get_model (tv);

  adding = ctk_toggle_button_get_active (button);

  m = g_menu_model_get_item_link (model, g_menu_model_get_n_items (model) - 1, G_MENU_LINK_SECTION);
  if (adding)
    {
      GSimpleAction *action;

      action = g_simple_action_new_stateful ("italic", NULL, g_variant_new_boolean (FALSE));
      g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
      g_signal_connect (action, "activate", G_CALLBACK (activate_toggle), NULL);
      g_object_unref (action);
      action_list_add (store, "italic");
      g_menu_insert (G_MENU (m), 1, "Italic", "italic");
    }
  else
    {
      g_action_map_remove_action (G_ACTION_MAP (group), "italic");
      action_list_remove (store, "italic");
      g_menu_remove (G_MENU (m), 1);
    }
}

static void
toggle_speed (CtkToggleButton *button, gpointer data)
{
  GMenuModel *model;
  GActionGroup *group;
  gboolean adding;
  GMenuModel *m;
  CtkTreeView *tv = data;
  CtkTreeModel *store;

  model = g_object_get_data (G_OBJECT (button), "model");
  group = g_object_get_data (G_OBJECT (button), "group");

  store = ctk_tree_view_get_model (tv);

  adding = ctk_toggle_button_get_active (button);

  m = g_menu_model_get_item_link (model, 1, G_MENU_LINK_SECTION);
  if (adding)
    {
      GSimpleAction *action;
      GMenu *submenu;

      action = g_simple_action_new ("faster", NULL);
      g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
      g_signal_connect (action, "activate", G_CALLBACK (activate_action), NULL);
      g_object_unref (action);

      action = g_simple_action_new ("slower", NULL);
      g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
      g_signal_connect (action, "activate", G_CALLBACK (activate_action), NULL);
      g_object_unref (action);

      action_list_add (store, "faster");
      action_list_add (store, "slower");

      submenu = g_menu_new ();
      g_menu_append (submenu, "Faster", "faster");
      g_menu_append (submenu, "Slower", "slower");
      g_menu_append_submenu (G_MENU (m), "Speed", G_MENU_MODEL (submenu));
    }
  else
    {
      g_action_map_remove_action (G_ACTION_MAP (group), "faster");
      g_action_map_remove_action (G_ACTION_MAP (group), "slower");

      action_list_remove (store, "faster");
      action_list_remove (store, "slower");

      g_menu_remove (G_MENU (m), g_menu_model_get_n_items (m) - 1);
    }
}
static CtkWidget *
create_add_remove_buttons (GActionGroup *group,
                           GMenuModel   *model,
                           CtkWidget    *treeview)
{
  CtkWidget *box;
  CtkWidget *button;

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);

  button = ctk_check_button_new_with_label ("Add Italic");
  ctk_container_add (CTK_CONTAINER (box), button);

  g_object_set_data  (G_OBJECT (button), "group", group);
  g_object_set_data  (G_OBJECT (button), "model", model);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (toggle_italic), treeview);

  button = ctk_check_button_new_with_label ("Add Sumerian");
  ctk_container_add (CTK_CONTAINER (box), button);

  g_object_set_data  (G_OBJECT (button), "group", group);
  g_object_set_data  (G_OBJECT (button), "model", model);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (toggle_sumerian), NULL);

  button = ctk_check_button_new_with_label ("Add Speed");
  ctk_container_add (CTK_CONTAINER (box), button);

  g_object_set_data  (G_OBJECT (button), "group", group);
  g_object_set_data  (G_OBJECT (button), "model", model);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (toggle_speed), treeview);
  return box;
}

/* main {{{1 */

#define BUS_NAME "org.ctk.TestMenus"
#define OBJ_PATH "/org/ctk/TestMenus"

static gboolean
on_delete_event (CtkWidget   *widget G_GNUC_UNUSED,
		 CdkEvent    *event G_GNUC_UNUSED,
		 gpointer     user_data G_GNUC_UNUSED)
{
  ctk_main_quit ();
  return TRUE;
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *box;
  GMenuModel *model;
  GActionGroup *group;
  GDBusConnection *bus;
  GError *error = NULL;
  gboolean do_export = FALSE;
  gboolean do_import = FALSE;
  GOptionEntry entries[] = {
    { "export", 0, 0, G_OPTION_ARG_NONE, &do_export, "Export actions and menus over D-Bus", NULL },
    { "import", 0, 0, G_OPTION_ARG_NONE, &do_import, "Use exported actions and menus", NULL },
    { NULL, }
  };

  ctk_init_with_args (&argc, &argv, NULL, entries, NULL, NULL);

  if (do_export && do_import)
    {
       g_error ("can't have it both ways\n");
       exit (1);
    }

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete-event", G_CALLBACK(on_delete_event), NULL);
  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_container_add (CTK_CONTAINER (window), box);

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  if (do_import)
    {
      g_print ("Getting menus from the bus...\n");
      model = (GMenuModel*)g_dbus_menu_model_get (bus, BUS_NAME, OBJ_PATH);
      g_print ("Getting actions from the bus...\n");
      group = (GActionGroup*)g_dbus_action_group_get (bus, BUS_NAME, OBJ_PATH);
    }
  else
    {
      CtkWidget *tv;
      CtkWidget *buttons;

      group = get_group ();
      model = get_model ();

      tv = create_action_treeview (group);
      ctk_container_add (CTK_CONTAINER (box), tv);
      buttons = create_add_remove_buttons (group, model, tv);
      ctk_container_add (CTK_CONTAINER (box), buttons);
    }

  if (do_export)
    {
      g_print ("Exporting menus on the bus...\n");
      if (!g_dbus_connection_export_menu_model (bus, OBJ_PATH, model, &error))
        {
          g_warning ("Menu export failed: %s", error->message);
          exit (1);
        }
      g_print ("Exporting actions on the bus...\n");
      if (!g_dbus_connection_export_action_group (bus, OBJ_PATH, group, &error))
        {
          g_warning ("Action export failed: %s", error->message);
          exit (1);
        }
      g_bus_own_name_on_connection (bus, BUS_NAME, 0, NULL, NULL, NULL, NULL);
    }
  else
    {
      CtkWidget *button;

      button = ctk_menu_button_new ();
      ctk_button_set_label (CTK_BUTTON (button), "Click here");
      ctk_menu_button_set_use_popover (CTK_MENU_BUTTON (button), TRUE);
      ctk_menu_button_set_menu_model (CTK_MENU_BUTTON (button), model);
      ctk_widget_insert_action_group (button, "actions", group);
      ctk_container_add (CTK_CONTAINER (box), button);
    }

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
