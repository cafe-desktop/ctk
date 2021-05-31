#include <gtk/gtk.h>

static void
on_text_changed (GtkEntry       *entry,
                 GParamSpec     *pspec,
                 GtkHeaderBar   *bar)
{
  const gchar *layout;

  layout = ctk_entry_get_text (entry);

  ctk_header_bar_set_decoration_layout (bar, layout);
}

static void
create_widgets (GtkHeaderBar *bar,
                GtkPackType   pack_type,
                gint          n)
{
  GList *children, *l;
  GtkWidget *child;
  gint i;
  gchar *label;

  children = ctk_container_get_children (CTK_CONTAINER (bar));
  for (l = children; l; l = l->next)
    {
      GtkPackType type;

      child = l->data;
      ctk_container_child_get (CTK_CONTAINER (bar), child, "pack-type", &type, NULL);
      if (type == pack_type)
        ctk_container_remove (CTK_CONTAINER (bar), child);
    }
  g_list_free (children);

  for (i = 0; i < n; i++)
    {
      label = g_strdup_printf ("%d", i);
      child = ctk_button_new_with_label (label);
      g_free (label);

      ctk_widget_show (child);
      if (pack_type == CTK_PACK_START)
        ctk_header_bar_pack_start (bar, child);
      else
        ctk_header_bar_pack_end (bar, child);
    }
}

static void
change_start (GtkSpinButton *button,
              GParamSpec    *pspec,
              GtkHeaderBar  *bar)
{
  create_widgets (bar,
                  CTK_PACK_START,
                  ctk_spin_button_get_value_as_int (button));
}

static void
change_end (GtkSpinButton *button,
            GParamSpec    *pspec,
            GtkHeaderBar  *bar)
{
  create_widgets (bar,
                  CTK_PACK_END,
                  ctk_spin_button_get_value_as_int (button));
}

static void
activate (GApplication *gapp)
{
  GtkApplication *app = CTK_APPLICATION (gapp);
  GtkWidget *window;
  GtkWidget *header;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *check;
  GtkWidget *spin;
  GtkBuilder *builder;
  GMenuModel *menu;
  gchar *layout;

  g_action_map_add_action (G_ACTION_MAP (gapp), G_ACTION (g_simple_action_new ("test", NULL)));
  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder,
                               "<interface>"
                               "  <menu id='app-menu'>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name='label'>Test item</attribute>"
                               "        <attribute name='action'>app.test</attribute>"
                               "      </item>"
                               "    </section>"
                               "  </menu>"
                               "</interface>", -1, NULL);
  window = ctk_application_window_new (app);
  ctk_window_set_icon_name (CTK_WINDOW (window), "preferences-desktop-font");

  menu = (GMenuModel*)ctk_builder_get_object (builder, "app-menu");
  ctk_application_add_window (app, CTK_WINDOW (window));
  ctk_application_set_app_menu (app, menu);

  header = ctk_header_bar_new ();
  ctk_window_set_titlebar (CTK_WINDOW (window), header);

  grid = ctk_grid_new ();
  g_object_set (grid,
                "halign", CTK_ALIGN_CENTER,
                "margin", 20,
                "row-spacing", 12,
                "column-spacing", 12,
                NULL);

  label = ctk_label_new ("Title");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  entry = ctk_entry_new ();
  g_object_bind_property (header, "title",
                          entry, "text",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 0, 1, 1);

  label = ctk_label_new ("Subtitle");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  entry = ctk_entry_new ();
  g_object_bind_property (header, "subtitle",
                          entry, "text",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 1, 1, 1);

  label = ctk_label_new ("Layout");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  entry = ctk_entry_new ();

  g_object_get (ctk_widget_get_settings (window), "gtk-decoration-layout", &layout, NULL);
  ctk_entry_set_text (CTK_ENTRY (entry), layout);
  g_free (layout);

  g_signal_connect (entry, "notify::text",
                    G_CALLBACK (on_text_changed), header);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 2, 1, 1);

  label = ctk_label_new ("Decorations");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  check = ctk_check_button_new ();
  g_object_bind_property (header, "show-close-button",
                          check, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label, 2, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check, 3, 0, 1, 1);

  label = ctk_label_new ("Has Subtitle");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  check = ctk_check_button_new ();
  g_object_bind_property (header, "has-subtitle",
                          check, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label, 2, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check, 3, 1, 1, 1);

  label = ctk_label_new ("Shell Shows Menu");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  check = ctk_check_button_new ();
  g_object_bind_property (ctk_settings_get_default (), "gtk-shell-shows-app-menu",
                          check, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label, 2, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check, 3, 2, 1, 1);

  label = ctk_label_new ("Custom");
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  spin = ctk_spin_button_new_with_range (0, 10, 1);
  g_signal_connect (spin, "notify::value",
                    G_CALLBACK (change_start), header);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 3, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), spin, 1, 3, 1, 1);
  spin = ctk_spin_button_new_with_range (0, 10, 1);
  g_signal_connect (spin, "notify::value",
                    G_CALLBACK (change_end), header);
  ctk_grid_attach (CTK_GRID (grid), spin, 2, 3, 2, 1);
  
  ctk_container_add (CTK_CONTAINER (window), grid);
  ctk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  GtkApplication *app;

  app = ctk_application_new ("org.gtk.Test.titlebar", 0);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
