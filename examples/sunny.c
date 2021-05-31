#include <stdlib.h>
#include <gtk/gtk.h>

static void
new_window (GApplication *app,
            GFile        *file)
{
  GtkWidget *window, *scrolled, *view, *overlay;
  GtkWidget *header;

  window = ctk_application_window_new (CTK_APPLICATION (app));
  ctk_application_window_set_show_menubar (CTK_APPLICATION_WINDOW (window), FALSE);
  ctk_window_set_default_size ((GtkWindow*)window, 640, 480);
  ctk_window_set_title (CTK_WINDOW (window), "Sunny");
  ctk_window_set_icon_name (CTK_WINDOW (window), "sunny");

  header = ctk_header_bar_new ();
  ctk_widget_show (header);
  ctk_header_bar_set_title (CTK_HEADER_BAR (header), "Sunny");
  ctk_header_bar_set_show_close_button (CTK_HEADER_BAR (header), TRUE);
  ctk_window_set_titlebar (CTK_WINDOW (window), header);

  overlay = ctk_overlay_new ();
  ctk_container_add (CTK_CONTAINER (window), overlay);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (scrolled, TRUE);
  ctk_widget_set_vexpand (scrolled, TRUE);
  view = ctk_text_view_new ();

  ctk_container_add (CTK_CONTAINER (scrolled), view);
  ctk_container_add (CTK_CONTAINER (overlay), scrolled);

  if (file != NULL)
    {
      gchar *contents;
      gsize length;

      if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
        {
          GtkTextBuffer *buffer;

          buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));
          ctk_text_buffer_set_text (buffer, contents, length);
          g_free (contents);
        }
    }

  ctk_widget_show_all (CTK_WIDGET (window));
}

static void
activate (GApplication *application)
{
  new_window (application, NULL);
}

static void
open (GApplication  *application,
      GFile        **files,
      gint           n_files,
      const gchar   *hint)
{
  gint i;

  for (i = 0; i < n_files; i++)
    new_window (application, files[i]);
}

typedef GtkApplication MenuButton;
typedef GtkApplicationClass MenuButtonClass;

G_DEFINE_TYPE (MenuButton, menu_button, CTK_TYPE_APPLICATION)

static void
show_about (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  ctk_show_about_dialog (NULL,
                         "program-name", "Sunny",
                         "title", "About Sunny",
                         "logo-icon-name", "sunny",
                         "comments", "A cheap Bloatpad clone.",
                         NULL);
}


static void
quit_app (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  GList *list, *next;
  GtkWindow *win;

  g_print ("Going down...\n");

  list = ctk_application_get_windows (CTK_APPLICATION (g_application_get_default ()));
  while (list)
    {
      win = list->data;
      next = list->next;

      ctk_widget_destroy (CTK_WIDGET (win));

      list = next;
    }
}

static void
new_activated (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GApplication *app = user_data;

  g_application_activate (app);
}

static GActionEntry app_entries[] = {
  { "about", show_about, NULL, NULL, NULL },
  { "quit", quit_app, NULL, NULL, NULL },
  { "new", new_activated, NULL, NULL, NULL }
};

static void
startup (GApplication *application)
{
  GtkBuilder *builder;

  G_APPLICATION_CLASS (menu_button_parent_class)->startup (application);

  g_action_map_add_action_entries (G_ACTION_MAP (application), app_entries, G_N_ELEMENTS (app_entries), application);

  if (g_getenv ("APP_MENU_FALLBACK"))
    g_object_set (ctk_settings_get_default (), "gtk-shell-shows-app-menu", FALSE, NULL);
 
  builder = ctk_builder_new ();
  ctk_builder_add_from_string (builder,
                               "<interface>"
                               "  <menu id='app-menu'>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name='label' translatable='yes'>_New Window</attribute>"
                               "        <attribute name='action'>app.new</attribute>"
                               "      </item>"
                               "      <item>"
                               "        <attribute name='label' translatable='yes'>_About Sunny</attribute>"
                               "        <attribute name='action'>app.about</attribute>"
                               "      </item>"
                               "      <item>"
                               "        <attribute name='label' translatable='yes'>_Quit</attribute>"
                               "        <attribute name='action'>app.quit</attribute>"
                               "        <attribute name='accel'>&lt;Primary&gt;q</attribute>"
                               "      </item>"
                               "    </section>"
                               "  </menu>"
                               "</interface>", -1, NULL);
  ctk_application_set_app_menu (CTK_APPLICATION (application), G_MENU_MODEL (ctk_builder_get_object (builder, "app-menu")));
  g_object_unref (builder);
}

static void
menu_button_init (MenuButton *app)
{
}

static void
menu_button_class_init (MenuButtonClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);

  application_class->startup = startup;
  application_class->activate = activate;
  application_class->open = open;
}

MenuButton *
menu_button_new (void)
{
  return g_object_new (menu_button_get_type (),
                       "application-id", "org.gtk.Test.Sunny",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

int
main (int argc, char **argv)
{
  MenuButton *menu_button;
  int status;

  menu_button = menu_button_new ();
  status = g_application_run (G_APPLICATION (menu_button), argc, argv);
  g_object_unref (menu_button);

  return status;
}
