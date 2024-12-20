#include <stdlib.h>
#include <ctk/ctk.h>

static void
activate_toggle (GSimpleAction *action,
                 GVariant      *parameter G_GNUC_UNUSED,
                 gpointer       user_data G_GNUC_UNUSED)
{
  GVariant *state;

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

static void
change_fullscreen_state (GSimpleAction *action,
                         GVariant      *state,
                         gpointer       user_data)
{
  if (g_variant_get_boolean (state))
    ctk_window_fullscreen (user_data);
  else
    ctk_window_unfullscreen (user_data);

  g_simple_action_set_state (action, state);
}

static CtkClipboard *
get_clipboard (CtkWidget *widget)
{
  return ctk_widget_get_clipboard (widget, cdk_atom_intern_static_string ("CLIPBOARD"));
}

static void
window_copy (GSimpleAction *action G_GNUC_UNUSED,
             GVariant      *parameter G_GNUC_UNUSED,
             gpointer       user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CtkTextView *text = g_object_get_data ((GObject*)window, "plugman-text");

  ctk_text_buffer_copy_clipboard (ctk_text_view_get_buffer (text),
                                  get_clipboard ((CtkWidget*) text));
}

static void
window_paste (GSimpleAction *action G_GNUC_UNUSED,
              GVariant      *parameter G_GNUC_UNUSED,
              gpointer       user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CtkTextView *text = g_object_get_data ((GObject*)window, "plugman-text");
  
  ctk_text_buffer_paste_clipboard (ctk_text_view_get_buffer (text),
                                   get_clipboard ((CtkWidget*) text),
                                   NULL,
                                   TRUE);

}

static GActionEntry win_entries[] = {
  { .name = "copy", .activate = window_copy },
  { .name = "paste", .activate = window_paste },
  { .name = "fullscreen", .activate = activate_toggle, .state = "false", .change_state = change_fullscreen_state }
};

static void
new_window (GApplication *app,
            GFile        *file)
{
  CtkWidget *window, *grid, *scrolled, *view;

  window = ctk_application_window_new (CTK_APPLICATION (app));
  ctk_window_set_default_size ((CtkWindow*)window, 640, 480);
  g_action_map_add_action_entries (G_ACTION_MAP (window), win_entries, G_N_ELEMENTS (win_entries), window);
  ctk_window_set_title (CTK_WINDOW (window), "Plugman");

  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (scrolled, TRUE);
  ctk_widget_set_vexpand (scrolled, TRUE);
  view = ctk_text_view_new ();

  g_object_set_data ((GObject*)window, "plugman-text", view);

  ctk_container_add (CTK_CONTAINER (scrolled), view);

  ctk_grid_attach (CTK_GRID (grid), scrolled, 0, 0, 1, 1);

  if (file != NULL)
    {
      gchar *contents;
      gsize length;

      if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
        {
          CtkTextBuffer *buffer;

          buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));
          ctk_text_buffer_set_text (buffer, contents, length);
          g_free (contents);
        }
    }

  ctk_widget_show_all (CTK_WIDGET (window));
}

static void
plug_man_activate (GApplication *application)
{
  new_window (application, NULL);
}

static void
plug_man_open (GApplication  *application,
                GFile        **files,
                gint           n_files,
                const gchar   *hint G_GNUC_UNUSED)
{
  gint i;

  for (i = 0; i < n_files; i++)
    new_window (application, files[i]);
}

typedef CtkApplication PlugMan;
typedef CtkApplicationClass PlugManClass;

G_DEFINE_TYPE (PlugMan, plug_man, CTK_TYPE_APPLICATION)

static void
plug_man_finalize (GObject *object)
{
  G_OBJECT_CLASS (plug_man_parent_class)->finalize (object);
}

static void
show_about (GSimpleAction *action G_GNUC_UNUSED,
            GVariant      *parameter G_GNUC_UNUSED,
            gpointer       user_data G_GNUC_UNUSED)
{
  ctk_show_about_dialog (NULL,
                         "program-name", "Plugman",
                         "title", "About Plugman",
                         "comments", "A cheap Bloatpad clone.",
                         NULL);
}


static void
quit_app (GSimpleAction *action G_GNUC_UNUSED,
          GVariant      *parameter G_GNUC_UNUSED,
          gpointer       user_data G_GNUC_UNUSED)
{
  GList *list, *next;

  g_print ("Going down...\n");

  list = ctk_application_get_windows (CTK_APPLICATION (g_application_get_default ()));
  while (list)
    {
      CtkWindow *win;

      win = list->data;
      next = list->next;

      ctk_widget_destroy (CTK_WIDGET (win));

      list = next;
    }
}

static gboolean is_red_plugin_enabled;
static gboolean is_black_plugin_enabled;

static gboolean
plugin_enabled (const gchar *name)
{
  if (g_strcmp0 (name, "red") == 0)
    return is_red_plugin_enabled;
  else
    return is_black_plugin_enabled;
}

static GMenuModel *
find_plugin_menu (void)
{
  return (GMenuModel*) g_object_get_data (G_OBJECT (g_application_get_default ()), "plugin-menu");
}

static void
plugin_action (GAction  *action,
               GVariant *parameter G_GNUC_UNUSED,
               gpointer  data G_GNUC_UNUSED)
{
  GApplication *app;
  GList *list;
  CtkWindow *window;
  CtkWidget *text;
  CdkRGBA color;

  app = g_application_get_default ();
  list = ctk_application_get_windows (CTK_APPLICATION (app));
  window = CTK_WINDOW (list->data);
  text = g_object_get_data ((GObject*)window, "plugman-text");

  cdk_rgba_parse (&color, g_action_get_name (action));

  ctk_widget_override_color (text, 0, &color);
}

static void
enable_plugin (const gchar *name)
{
  GMenuModel *plugin_menu;
  GAction *action;

  g_print ("Enabling '%s' plugin\n", name);

  action = (GAction *)g_simple_action_new (name, NULL);
  g_signal_connect (action, "activate", G_CALLBACK (plugin_action), (gpointer)name);
  g_action_map_add_action (G_ACTION_MAP (g_application_get_default ()), action);
  g_print ("Actions of '%s' plugin added\n", name);
  g_object_unref (action);

  plugin_menu = find_plugin_menu ();
  if (plugin_menu)
    {
      GMenu *section;
      GMenuItem *item;
      gchar *label;
      gchar *action_name;

      section = g_menu_new ();
      label = g_strdup_printf ("Turn text %s", name);
      action_name = g_strconcat ("app.", name, NULL);
      g_menu_insert (section, 0, label, action_name);
      g_free (label);
      g_free (action_name);
      item = g_menu_item_new_section (NULL, (GMenuModel*)section);
      g_menu_item_set_attribute (item, "id", "s", name);
      g_menu_append_item (G_MENU (plugin_menu), item);
      g_object_unref (item);
      g_object_unref (section);
      g_print ("Menus of '%s' plugin added\n", name);
    }
  else
    g_warning ("Plugin menu not found");

  if (g_strcmp0 (name, "red") == 0)
    is_red_plugin_enabled = TRUE;
  else
    is_black_plugin_enabled = TRUE;
}

static void
disable_plugin (const gchar *name)
{
  GMenuModel *plugin_menu;

  g_print ("Disabling '%s' plugin\n", name);

  plugin_menu = find_plugin_menu ();
  if (plugin_menu)
    {
      gint i;

      for (i = 0; i < g_menu_model_get_n_items (plugin_menu); i++)
        {
           gchar *id;
           if (g_menu_model_get_item_attribute (plugin_menu, i, "id", "s", &id))
             {
               if (g_strcmp0 (id, name) == 0)
                 {
                   g_menu_remove (G_MENU (plugin_menu), i);
                   g_print ("Menus of '%s' plugin removed\n", name);
                 }
               g_free (id);
             }
        }
    }
  else
    g_warning ("Plugin menu not found");

  g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()), name);
  g_print ("Actions of '%s' plugin removed\n", name);

  if (g_strcmp0 (name, "red") == 0)
    is_red_plugin_enabled = FALSE;
  else
    is_black_plugin_enabled = FALSE;
}

static void
enable_or_disable_plugin (CtkToggleButton *button G_GNUC_UNUSED,
                          const gchar     *name)
{
  if (plugin_enabled (name))
    disable_plugin (name);
  else
    enable_plugin (name);
}


static void
configure_plugins (GSimpleAction *action G_GNUC_UNUSED,
                   GVariant      *parameter G_GNUC_UNUSED,
                   gpointer       user_data G_GNUC_UNUSED)
{
  CtkBuilder *builder;
  CtkWidget *dialog;
  CtkWidget *check;
  GError *error = NULL;

  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder,
                               "<interface>"
                               "  <object class='CtkDialog' id='plugin-dialog'>"
                               "    <property name='border-width'>12</property>"
                               "    <property name='title'>Plugins</property>"
                               "    <child internal-child='vbox'>"
                               "      <object class='CtkBox' id='content-area'>"
                               "        <property name='visible'>True</property>"
                               "        <child>"
                               "          <object class='CtkCheckButton' id='red-plugin'>"
                               "            <property name='label' translatable='yes'>Red Plugin - turn your text red</property>"
                               "            <property name='visible'>True</property>"
                               "          </object>"
                               "        </child>"
                               "        <child>"
                               "          <object class='CtkCheckButton' id='black-plugin'>"
                               "            <property name='label' translatable='yes'>Black Plugin - turn your text black</property>"
                               "            <property name='visible'>True</property>"
                               "          </object>"
                               "        </child>"
                               "      </object>"
                               "    </child>"
                               "    <child internal-child='action_area'>"
                               "      <object class='CtkButtonBox' id='action-area'>"
                               "        <property name='visible'>True</property>"
                               "        <child>"
                               "          <object class='CtkButton' id='close-button'>"
                               "            <property name='label' translatable='yes'>Close</property>"
                               "            <property name='visible'>True</property>"
                               "          </object>"
                               "        </child>"
                               "      </object>"
                               "    </child>"
                               "    <action-widgets>"
                               "      <action-widget response='-5'>close-button</action-widget>"
                               "    </action-widgets>"
                               "  </object>"
                               "</interface>", -1, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      goto out;
    }

  dialog = (CtkWidget *)ctk_builder_get_object (builder, "plugin-dialog");
  check = (CtkWidget *)ctk_builder_get_object (builder, "red-plugin");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), plugin_enabled ("red"));
  g_signal_connect (check, "toggled", G_CALLBACK (enable_or_disable_plugin), "red");
  check = (CtkWidget *)ctk_builder_get_object (builder, "black-plugin");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (check), plugin_enabled ("black"));
  g_signal_connect (check, "toggled", G_CALLBACK (enable_or_disable_plugin), "black");

  g_signal_connect (dialog, "response", G_CALLBACK (ctk_widget_destroy), NULL);

  ctk_window_present (CTK_WINDOW (dialog));

out:
  g_object_unref (builder);
}

static GActionEntry app_entries[] = {
  { .name = "about", .activate = show_about },
  { .name = "quit", .activate = quit_app },
  { .name = "plugins", .activate = configure_plugins },
};

static void
plug_man_startup (GApplication *application)
{
  CtkBuilder *builder;

  G_APPLICATION_CLASS (plug_man_parent_class)
    ->startup (application);

  g_action_map_add_action_entries (G_ACTION_MAP (application), app_entries, G_N_ELEMENTS (app_entries), application);

  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder,
                               "<interface>"
                               "  <menu id='app-menu'>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name='label' translatable='yes'>_About Plugman</attribute>"
                               "        <attribute name='action'>app.about</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name='label' translatable='yes'>_Quit</attribute>"
                               "        <attribute name='action'>app.quit</attribute>"
                               "        <attribute name='accel'>&lt;Primary&gt;q</attribute>"
                               "      </item>"
                               "    </section>"
                               "  </menu>"
                               "  <menu id='menubar'>"
                               "    <submenu>"
                               "      <attribute name='label' translatable='yes'>_Edit</attribute>"
                               "      <section>"
                               "        <item>"
                               "          <attribute name='label' translatable='yes'>_Copy</attribute>"
                               "          <attribute name='action'>win.copy</attribute>"
                               "        </item>"
                               "        <item>"
                               "          <attribute name='label' translatable='yes'>_Paste</attribute>"
                               "          <attribute name='action'>win.paste</attribute>"
                               "        </item>"
                               "      </section>"
                               "      <item><link name='section' id='plugins'>"
                               "      </link></item>"
                               "      <section>"
                               "        <item>"
                               "          <attribute name='label' translatable='yes'>Plugins</attribute>"
                               "          <attribute name='action'>app.plugins</attribute>"
                               "        </item>"
                               "      </section>"
                               "    </submenu>"
                               "    <submenu>"
                               "      <attribute name='label' translatable='yes'>_View</attribute>"
                               "      <section>"
                               "        <item>"
                               "          <attribute name='label' translatable='yes'>_Fullscreen</attribute>"
                               "          <attribute name='action'>win.fullscreen</attribute>"
                               "        </item>"
                               "      </section>"
                               "    </submenu>"
                               "  </menu>"
                               "</interface>", -1, NULL);
  ctk_application_set_app_menu (CTK_APPLICATION (application), G_MENU_MODEL (ctk_builder_get_object (builder, "app-menu")));
  ctk_application_set_menubar (CTK_APPLICATION (application), G_MENU_MODEL (ctk_builder_get_object (builder, "menubar")));
  g_object_set_data_full (G_OBJECT (application), "plugin-menu", ctk_builder_get_object (builder, "plugins"), g_object_unref);
  g_object_unref (builder);
}

static void
plug_man_init (PlugMan *app G_GNUC_UNUSED)
{
}

static void
plug_man_class_init (PlugManClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  application_class->startup = plug_man_startup;
  application_class->activate = plug_man_activate;
  application_class->open = plug_man_open;

  object_class->finalize = plug_man_finalize;

}

PlugMan *
plug_man_new (void)
{
  return g_object_new (plug_man_get_type (),
                       "application-id", "org.ctk.Test.plugman",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

int
main (int argc, char **argv)
{
  PlugMan *plug_man;
  int status;
  const gchar *accels[] = { "F11", NULL };

  plug_man = plug_man_new ();
  ctk_application_set_accels_for_action (CTK_APPLICATION (plug_man),
                                         "win.fullscreen", accels);
  status = g_application_run (G_APPLICATION (plug_man), argc, argv);
  g_object_unref (plug_man);

  return status;
}
