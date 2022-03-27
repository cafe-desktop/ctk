#include <ctk/ctk.h>

#include "exampleapp.h"
#include "exampleappwin.h"
#include "exampleappprefs.h"

struct _ExampleApp
{
  CtkApplication parent;
};

G_DEFINE_TYPE(ExampleApp, example_app, CTK_TYPE_APPLICATION);

static void
example_app_init (ExampleApp *app)
{
}

static void
preferences_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
  ExampleAppPrefs *prefs;
  CtkWindow *win;

  win = ctk_application_get_active_window (CTK_APPLICATION (app));
  prefs = example_app_prefs_new (EXAMPLE_APP_WINDOW (win));
  ctk_window_present (CTK_WINDOW (prefs));
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
  g_application_quit (G_APPLICATION (app));
}

static GActionEntry app_entries[] =
{
  { .name = "preferences", .activate = preferences_activated },
  { .name = "quit", .activate = quit_activated }
};

static void
example_app_startup (GApplication *app)
{
  CtkBuilder *builder;
  GMenuModel *app_menu;
  const gchar *quit_accels[2] = { "<Ctrl>Q", NULL };

  G_APPLICATION_CLASS (example_app_parent_class)->startup (app);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
  ctk_application_set_accels_for_action (CTK_APPLICATION (app),
                                         "app.quit",
                                         quit_accels);

  builder = ctk_builder_new_from_resource ("/org/ctk/exampleapp/app-menu.ui");
  app_menu = G_MENU_MODEL (ctk_builder_get_object (builder, "appmenu"));
  ctk_application_set_app_menu (CTK_APPLICATION (app), app_menu);
  g_object_unref (builder);
}

static void
example_app_activate (GApplication *app)
{
  ExampleAppWindow *win;

  win = example_app_window_new (EXAMPLE_APP (app));
  ctk_window_present (CTK_WINDOW (win));
}

static void
example_app_open (GApplication  *app,
                  GFile        **files,
                  gint           n_files,
                  const gchar   *hint)
{
  GList *windows;
  ExampleAppWindow *win;
  int i;

  windows = ctk_application_get_windows (CTK_APPLICATION (app));
  if (windows)
    win = EXAMPLE_APP_WINDOW (windows->data);
  else
    win = example_app_window_new (EXAMPLE_APP (app));

  for (i = 0; i < n_files; i++)
    example_app_window_open (win, files[i]);

  ctk_window_present (CTK_WINDOW (win));
}

static void
example_app_class_init (ExampleAppClass *class)
{
  G_APPLICATION_CLASS (class)->startup = example_app_startup;
  G_APPLICATION_CLASS (class)->activate = example_app_activate;
  G_APPLICATION_CLASS (class)->open = example_app_open;
}

ExampleApp *
example_app_new (void)
{
  return g_object_new (EXAMPLE_APP_TYPE,
                       "application-id", "org.ctk.exampleapp",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}
