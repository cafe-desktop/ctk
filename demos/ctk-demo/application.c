
#include "config.h"

#include <ctk/ctk.h>

typedef CtkApplication DemoApplication;
typedef CtkApplicationClass DemoApplicationClass;

G_DEFINE_TYPE (DemoApplication, demo_application, CTK_TYPE_APPLICATION)

typedef struct {
  CtkApplicationWindow parent_instance;

  CtkWidget *message;
  CtkWidget *infobar;
  CtkWidget *status;
  CtkWidget *menutool;
  GMenuModel *toolmenu;
  CtkTextBuffer *buffer;

  int width;
  int height;
  gboolean maximized;
  gboolean fullscreen;
} DemoApplicationWindow;
typedef CtkApplicationWindowClass DemoApplicationWindowClass;

G_DEFINE_TYPE (DemoApplicationWindow, demo_application_window, CTK_TYPE_APPLICATION_WINDOW)

static void create_window (GApplication *app, const char *contents);

static void
show_action_dialog (GSimpleAction *action)
{
  const gchar *name;
  CtkWidget *dialog;

  name = g_action_get_name (G_ACTION (action));

  dialog = ctk_message_dialog_new (NULL,
                                   CTK_DIALOG_DESTROY_WITH_PARENT,
                                   CTK_MESSAGE_INFO,
                                   CTK_BUTTONS_CLOSE,
                                   "You activated action: \"%s\"",
                                    name);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (ctk_widget_destroy), NULL);

  ctk_widget_show (dialog);
}

static void
show_action_infobar (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       data)
{
  DemoApplicationWindow *window = data;
  gchar *text;
  const gchar *name;
  const gchar *value;

  name = g_action_get_name (G_ACTION (action));
  value = g_variant_get_string (parameter, NULL);

  text = g_strdup_printf ("You activated radio action: \"%s\".\n"
                          "Current value: %s", name, value);
  ctk_label_set_text (CTK_LABEL (window->message), text);
  ctk_widget_show (window->infobar);
  g_free (text);
}

static void
activate_action (GSimpleAction *action,
                 GVariant      *parameter G_GNUC_UNUSED,
                 gpointer       user_data G_GNUC_UNUSED)
{
  show_action_dialog (action);
}

static void
activate_new (GSimpleAction *action G_GNUC_UNUSED,
              GVariant      *parameter G_GNUC_UNUSED,
              gpointer       user_data)
{
  GApplication *app = user_data;

  create_window (app, NULL);
}

static void
open_response_cb (CtkNativeDialog *dialog G_GNUC_UNUSED,
                  gint             response_id,
                  gpointer         user_data)
{
  CtkFileChooserNative *native = user_data;
  GApplication *app = g_object_get_data (G_OBJECT (native), "app");
  CtkWidget *message_dialog;
  GFile *file;
  char *contents;
  GError *error = NULL;

  if (response_id == CTK_RESPONSE_ACCEPT)
    {
      file = ctk_file_chooser_get_file (CTK_FILE_CHOOSER (native));

      if (g_file_load_contents (file, NULL, &contents, NULL, NULL, &error))
        {
          create_window (app, contents);
          g_free (contents);
        }
      else
        {
          message_dialog = ctk_message_dialog_new (NULL,
                                                   CTK_DIALOG_DESTROY_WITH_PARENT,
                                                   CTK_MESSAGE_ERROR,
                                                   CTK_BUTTONS_CLOSE,
                                                   "Error loading file: \"%s\"",
                                                   error->message);
          g_signal_connect (message_dialog, "response",
                            G_CALLBACK (ctk_widget_destroy), NULL);
          ctk_widget_show (message_dialog);
          g_error_free (error);
        }
    }

  ctk_native_dialog_destroy (CTK_NATIVE_DIALOG (native));
  g_object_unref (native);
}


static void
activate_open (GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *parameter G_GNUC_UNUSED,
               gpointer       user_data)
{
  GApplication *app = user_data;
  CtkFileChooserNative *native;

  native = ctk_file_chooser_native_new ("Open File",
                                        NULL,
                                        CTK_FILE_CHOOSER_ACTION_OPEN,
                                        "_Open",
                                        "_Cancel");

  g_object_set_data_full (G_OBJECT (native), "app", g_object_ref (app), g_object_unref);
  g_signal_connect (native,
                    "response",
                    G_CALLBACK (open_response_cb),
                    native);

  ctk_native_dialog_show (CTK_NATIVE_DIALOG (native));
}

static void
activate_toggle (GSimpleAction *action,
                 GVariant      *parameter G_GNUC_UNUSED,
                 gpointer       user_data G_GNUC_UNUSED)
{
  GVariant *state;

  show_action_dialog (action);

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

static void
activate_radio (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  show_action_infobar (action, parameter, user_data);

  g_action_change_state (G_ACTION (action), parameter);
}

static void
activate_about (GSimpleAction *action G_GNUC_UNUSED,
                GVariant      *parameter G_GNUC_UNUSED,
                gpointer       user_data)
{
  CtkWidget *window = user_data;

  const gchar *authors[] = {
    "Peter Mattis",
    "Spencer Kimball",
    "Josh MacDonald",
    "and many more...",
    NULL
  };

  const gchar *documentors[] = {
    "Owen Taylor",
    "Tony Gale",
    "Matthias Clasen <mclasen@redhat.com>",
    "and many more...",
    NULL
  };

  ctk_show_about_dialog (CTK_WINDOW (window),
                         "program-name", "CTK+ Code Demos",
                         "version", g_strdup_printf ("%s,\nRunning against CTK+ %d.%d.%d",
                                                     PACKAGE_VERSION,
                                                     ctk_get_major_version (),
                                                     ctk_get_minor_version (),
                                                     ctk_get_micro_version ()),
                         "copyright", "(C) 1997-2013 The CTK+ Team",
                         "license-type", CTK_LICENSE_LGPL_2_1,
                         "website", "http://github.com/cafe-desktop/ctk",
                         "comments", "Program to demonstrate CTK+ functions.",
                         "authors", authors,
                         "documenters", documentors,
                         "logo-icon-name", "ctk3-demo",
                         "title", "About CTK+ Code Demos",
                         NULL);
}

static void
activate_quit (GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *parameter G_GNUC_UNUSED,
               gpointer       user_data)
{
  CtkApplication *app = user_data;
  CtkWidget *win;
  GList *list, *next;

  list = ctk_application_get_windows (app);
  while (list)
    {
      win = list->data;
      next = list->next;

      ctk_widget_destroy (CTK_WIDGET (win));

      list = next;
    }
}

static void
update_statusbar (CtkTextBuffer         *buffer,
                  DemoApplicationWindow *window)
{
  gchar *msg;
  gint row, col;
  gint count;
  CtkTextIter iter;

  /* clear any previous message, underflow is allowed */
  ctk_statusbar_pop (CTK_STATUSBAR (window->status), 0);

  count = ctk_text_buffer_get_char_count (buffer);

  ctk_text_buffer_get_iter_at_mark (buffer,
                                    &iter,
                                    ctk_text_buffer_get_insert (buffer));

  row = ctk_text_iter_get_line (&iter);
  col = ctk_text_iter_get_line_offset (&iter);

  msg = g_strdup_printf ("Cursor at row %d column %d - %d chars in document",
                         row, col, count);

  ctk_statusbar_push (CTK_STATUSBAR (window->status), 0, msg);

  g_free (msg);
}

static void
mark_set_callback (CtkTextBuffer         *buffer,
                   const CtkTextIter     *new_location G_GNUC_UNUSED,
                   CtkTextMark           *mark G_GNUC_UNUSED,
                   DemoApplicationWindow *window)
{
  update_statusbar (buffer, window);
}

static void
change_theme_state (GSimpleAction *action,
                    GVariant      *state,
                    gpointer       user_data G_GNUC_UNUSED)
{
  CtkSettings *settings = ctk_settings_get_default ();

  g_object_set (G_OBJECT (settings),
                "ctk-application-prefer-dark-theme",
                g_variant_get_boolean (state),
                NULL);

  g_simple_action_set_state (action, state);
}

static void
change_titlebar_state (GSimpleAction *action,
                       GVariant      *state,
                       gpointer       user_data)
{
  CtkWindow *window = user_data;

  ctk_window_set_hide_titlebar_when_maximized (CTK_WINDOW (window),
                                               g_variant_get_boolean (state));

  g_simple_action_set_state (action, state);
}

static void
change_radio_state (GSimpleAction *action,
                    GVariant      *state,
                    gpointer       user_data G_GNUC_UNUSED)
{
  g_simple_action_set_state (action, state);
}

static GActionEntry app_entries[] = {
  { .name = "new", .activate = activate_new },
  { .name = "open", .activate = activate_open },
  { .name = "save", .activate = activate_action },
  { .name = "save-as", .activate = activate_action },
  { .name = "quit", .activate = activate_quit },
  { .name = "dark", .activate = activate_toggle, .state = "false", .change_state = change_theme_state }
};

static GActionEntry win_entries[] = {
  { .name = "titlebar", .activate = activate_toggle, .state = "false", .change_state = change_titlebar_state },
  { .name = "shape", .activate = activate_radio, .parameter_type = "s", .state = "'oval'", .change_state = change_radio_state },
  { .name = "bold", .activate = activate_toggle, .state = "false" },
  { .name = "about", .activate = activate_about },
  { .name = "file1", .activate = activate_action },
  { .name = "logo", .activate = activate_action }
};

static void
clicked_cb (CtkWidget             *widget G_GNUC_UNUSED,
	    DemoApplicationWindow *window)
{
  ctk_widget_hide (window->infobar);
}

static void
startup (GApplication *app)
{
  CtkBuilder *builder;
  GMenuModel *appmenu;
  GMenuModel *menubar;

  G_APPLICATION_CLASS (demo_application_parent_class)->startup (app);

  builder = ctk_builder_new ();
  ctk_builder_add_from_resource (builder, "/application_demo/menus.ui", NULL);

  appmenu = (GMenuModel *)ctk_builder_get_object (builder, "appmenu");
  menubar = (GMenuModel *)ctk_builder_get_object (builder, "menubar");

  ctk_application_set_app_menu (CTK_APPLICATION (app), appmenu);
  ctk_application_set_menubar (CTK_APPLICATION (app), menubar);

  g_object_unref (builder);
}

static void
create_window (GApplication *app,
               const char   *content)
{
  DemoApplicationWindow *window;

  window = (DemoApplicationWindow *)g_object_new (demo_application_window_get_type (),
                                                  "application", app,
                                                  NULL);
  if (content)
    ctk_text_buffer_set_text (window->buffer, content, -1);

  ctk_window_present (CTK_WINDOW (window));
}

static void
activate (GApplication *app)
{
  create_window (app, NULL);
}

static void
demo_application_init (DemoApplication *app)
{
  GSettings *settings;
  GAction *action;

  settings = g_settings_new ("org.ctk.Demo");

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);

  action = g_settings_create_action (settings, "color");

  g_action_map_add_action (G_ACTION_MAP (app), action);

  g_object_unref (settings);
}

static void
demo_application_class_init (DemoApplicationClass *class)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (class);

  app_class->startup = startup;
  app_class->activate = activate;
}

static void
demo_application_window_store_state (DemoApplicationWindow *win)
{
  GSettings *settings;

  settings = g_settings_new ("org.ctk.Demo");
  g_settings_set (settings, "window-size", "(ii)", win->width, win->height);
  g_settings_set_boolean (settings, "maximized", win->maximized);
  g_settings_set_boolean (settings, "fullscreen", win->fullscreen);
  g_object_unref (settings);
}

static void
demo_application_window_load_state (DemoApplicationWindow *win)
{
  GSettings *settings;

  settings = g_settings_new ("org.ctk.Demo");
  g_settings_get (settings, "window-size", "(ii)", &win->width, &win->height);
  win->maximized = g_settings_get_boolean (settings, "maximized");
  win->fullscreen = g_settings_get_boolean (settings, "fullscreen");
  g_object_unref (settings);
}

static void
demo_application_window_init (DemoApplicationWindow *window)
{
  CtkWidget *menu;

  window->width = -1;
  window->height = -1;
  window->maximized = FALSE;
  window->fullscreen = FALSE;

  ctk_widget_init_template (CTK_WIDGET (window));

  menu = ctk_menu_new_from_model (window->toolmenu);
  ctk_menu_tool_button_set_menu (CTK_MENU_TOOL_BUTTON (window->menutool), menu);

  g_action_map_add_action_entries (G_ACTION_MAP (window),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   window);
}

static void
demo_application_window_constructed (GObject *object)
{
  DemoApplicationWindow *window = (DemoApplicationWindow *)object;

  demo_application_window_load_state (window);

  ctk_window_set_default_size (CTK_WINDOW (window), window->width, window->height);

  if (window->maximized)
    ctk_window_maximize (CTK_WINDOW (window));

  if (window->fullscreen)
    ctk_window_fullscreen (CTK_WINDOW (window));

  G_OBJECT_CLASS (demo_application_window_parent_class)->constructed (object);
}

static void
demo_application_window_size_allocate (CtkWidget     *widget,
                                       CtkAllocation *allocation)
{
  DemoApplicationWindow *window = (DemoApplicationWindow *)widget;

  CTK_WIDGET_CLASS (demo_application_window_parent_class)->size_allocate (widget, allocation);

  if (!window->maximized && !window->fullscreen)
    ctk_window_get_size (CTK_WINDOW (window), &window->width, &window->height);
}

static gboolean
demo_application_window_state_event (CtkWidget           *widget,
                                     CdkEventWindowState *event)
{
  DemoApplicationWindow *window = (DemoApplicationWindow *)widget;
  gboolean res = CDK_EVENT_PROPAGATE;

  if (CTK_WIDGET_CLASS (demo_application_window_parent_class)->window_state_event)
    res = CTK_WIDGET_CLASS (demo_application_window_parent_class)->window_state_event (widget, event);

  window->maximized = (event->new_window_state & CDK_WINDOW_STATE_MAXIMIZED) != 0;
  window->fullscreen = (event->new_window_state & CDK_WINDOW_STATE_FULLSCREEN) != 0;

  return res;
}

static void
demo_application_window_destroy (CtkWidget *widget)
{
  DemoApplicationWindow *window = (DemoApplicationWindow *)widget;

  demo_application_window_store_state (window);

  CTK_WIDGET_CLASS (demo_application_window_parent_class)->destroy (widget);
}

static void
demo_application_window_class_init (DemoApplicationWindowClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->constructed = demo_application_window_constructed;

  widget_class->size_allocate = demo_application_window_size_allocate;
  widget_class->window_state_event = demo_application_window_state_event;
  widget_class->destroy = demo_application_window_destroy;

  ctk_widget_class_set_template_from_resource (widget_class, "/application_demo/application.ui");
  ctk_widget_class_bind_template_child (widget_class, DemoApplicationWindow, message);
  ctk_widget_class_bind_template_child (widget_class, DemoApplicationWindow, infobar);
  ctk_widget_class_bind_template_child (widget_class, DemoApplicationWindow, status);
  ctk_widget_class_bind_template_child (widget_class, DemoApplicationWindow, buffer);
  ctk_widget_class_bind_template_child (widget_class, DemoApplicationWindow, menutool);
  ctk_widget_class_bind_template_child (widget_class, DemoApplicationWindow, toolmenu);
  ctk_widget_class_bind_template_callback (widget_class, clicked_cb);
  ctk_widget_class_bind_template_callback (widget_class, update_statusbar);
  ctk_widget_class_bind_template_callback (widget_class, mark_set_callback);
}

int
main (int   argc G_GNUC_UNUSED,
      char *argv[] G_GNUC_UNUSED)
{
  CtkApplication *app;

  app = CTK_APPLICATION (g_object_new (demo_application_get_type (),
                                       "application-id", "org.ctk.Demo2",
                                       "flags", G_APPLICATION_HANDLES_OPEN,
                                       NULL));

  return g_application_run (G_APPLICATION (app), 0, NULL);
}
