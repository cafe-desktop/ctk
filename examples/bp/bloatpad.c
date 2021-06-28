#include <stdlib.h>
#include <ctk/ctk.h>

typedef struct
{
  CtkApplication parent_instance;

  guint quit_inhibit;
  GMenu *time;
  guint timeout;
} BloatPad;

typedef CtkApplicationClass BloatPadClass;

G_DEFINE_TYPE (BloatPad, bloat_pad, CTK_TYPE_APPLICATION)

static void
activate_toggle (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GVariant *state;

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

static void
activate_radio (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  g_action_change_state (G_ACTION (action), parameter);
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

static void
change_busy_state (GSimpleAction *action,
                   GVariant      *state,
                   gpointer       user_data)
{
  CtkWindow *window = user_data;
  GApplication *application = G_APPLICATION (ctk_window_get_application (window));

  /* do this twice to test multiple busy counter increases */
  if (g_variant_get_boolean (state))
    {
      g_application_mark_busy (application);
      g_application_mark_busy (application);
    }
  else
    {
      g_application_unmark_busy (application);
      g_application_unmark_busy (application);
    }

  g_simple_action_set_state (action, state);
}

static void
change_justify_state (GSimpleAction *action,
                      GVariant      *state,
                      gpointer       user_data)
{
  CtkTextView *text = g_object_get_data (user_data, "bloatpad-text");
  const gchar *str;

  str = g_variant_get_string (state, NULL);

  if (g_str_equal (str, "left"))
    ctk_text_view_set_justification (text, CTK_JUSTIFY_LEFT);
  else if (g_str_equal (str, "center"))
    ctk_text_view_set_justification (text, CTK_JUSTIFY_CENTER);
  else if (g_str_equal (str, "right"))
    ctk_text_view_set_justification (text, CTK_JUSTIFY_RIGHT);
  else
    /* ignore this attempted change */
    return;

  g_simple_action_set_state (action, state);
}

static CtkClipboard *
get_clipboard (CtkWidget *widget)
{
  return ctk_widget_get_clipboard (widget, cdk_atom_intern_static_string ("CLIPBOARD"));
}

static void
window_copy (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CtkTextView *text = g_object_get_data ((GObject*)window, "bloatpad-text");

  ctk_text_buffer_copy_clipboard (ctk_text_view_get_buffer (text),
                                  get_clipboard ((CtkWidget*) text));
}

static void
window_paste (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CtkTextView *text = g_object_get_data ((GObject*)window, "bloatpad-text");
  
  ctk_text_buffer_paste_clipboard (ctk_text_view_get_buffer (text),
                                   get_clipboard ((CtkWidget*) text),
                                   NULL,
                                   TRUE);

}

static void
activate_clear (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CtkTextView *text = g_object_get_data ((GObject*)window, "bloatpad-text");

  ctk_text_buffer_set_text (ctk_text_view_get_buffer (text), "", -1);
}

static void
activate_clear_all (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  CtkApplication *app = CTK_APPLICATION (user_data);
  GList *iter;

  for (iter = ctk_application_get_windows (app); iter; iter = iter->next)
    g_action_group_activate_action (iter->data, "clear", NULL);
}

static void
text_buffer_changed_cb (CtkTextBuffer *buffer,
                        gpointer       user_data)
{
  CtkWindow *window = user_data;
  BloatPad *app;
  gint old_n, n;

  app = (BloatPad *) ctk_window_get_application (window);

  n = ctk_text_buffer_get_char_count (buffer);
  if (n > 0)
    {
      if (!app->quit_inhibit)
        app->quit_inhibit = ctk_application_inhibit (CTK_APPLICATION (app),
                                                     ctk_application_get_active_window (CTK_APPLICATION (app)),
                                                     CTK_APPLICATION_INHIBIT_LOGOUT,
                                                     "bloatpad can't save, so you can't logout; erase your text");
    }
  else
    {
      if (app->quit_inhibit)
        {
          ctk_application_uninhibit (CTK_APPLICATION (app), app->quit_inhibit);
          app->quit_inhibit = 0;
        }
    }

  g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (window), "clear")), n > 0);

  if (n > 0)
    {
      GSimpleAction *spellcheck;
      spellcheck = g_simple_action_new ("spell-check", NULL);
      g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (spellcheck));
    }
  else
    g_action_map_remove_action (G_ACTION_MAP (window), "spell-check");

  old_n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (buffer), "line-count"));
  n = ctk_text_buffer_get_line_count (buffer);
  g_object_set_data (G_OBJECT (buffer), "line-count", GINT_TO_POINTER (n));

  if (old_n < 3 && n == 3)
    {
      GNotification *n;
      n = g_notification_new ("Three lines of text");
      g_notification_set_body (n, "Keep up the good work!");
      g_notification_add_button (n, "Start over", "app.clear-all");
      g_application_send_notification (G_APPLICATION (app), "three-lines", n);
      g_object_unref (n);
    }
}

static GActionEntry win_entries[] = {
  { "copy", window_copy, NULL, NULL, NULL },
  { "paste", window_paste, NULL, NULL, NULL },
  { "fullscreen", activate_toggle, NULL, "false", change_fullscreen_state },
  { "busy", activate_toggle, NULL, "false", change_busy_state },
  { "justify", activate_radio, "s", "'left'", change_justify_state },
  { "clear", activate_clear, NULL, NULL, NULL }

};

static void
new_window (GApplication *app,
            GFile        *file)
{
  CtkWidget *window, *grid, *scrolled, *view;
  CtkWidget *toolbar;
  CtkToolItem *button;
  CtkWidget *sw, *box, *label;

  window = ctk_application_window_new (CTK_APPLICATION (app));
  ctk_window_set_default_size ((CtkWindow*)window, 640, 480);
  g_action_map_add_action_entries (G_ACTION_MAP (window), win_entries, G_N_ELEMENTS (win_entries), window);
  ctk_window_set_title (CTK_WINDOW (window), "Bloatpad");

  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);

  toolbar = ctk_toolbar_new ();
  button = ctk_toggle_tool_button_new ();
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (button), "format-justify-left");
  ctk_actionable_set_detailed_action_name (CTK_ACTIONABLE (button), "win.justify::left");
  ctk_container_add (CTK_CONTAINER (toolbar), CTK_WIDGET (button));

  button = ctk_toggle_tool_button_new ();
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (button), "format-justify-center");
  ctk_actionable_set_detailed_action_name (CTK_ACTIONABLE (button), "win.justify::center");
  ctk_container_add (CTK_CONTAINER (toolbar), CTK_WIDGET (button));

  button = ctk_toggle_tool_button_new ();
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (button), "format-justify-right");
  ctk_actionable_set_detailed_action_name (CTK_ACTIONABLE (button), "win.justify::right");
  ctk_container_add (CTK_CONTAINER (toolbar), CTK_WIDGET (button));

  button = ctk_separator_tool_item_new ();
  ctk_separator_tool_item_set_draw (CTK_SEPARATOR_TOOL_ITEM (button), FALSE);
  ctk_tool_item_set_expand (CTK_TOOL_ITEM (button), TRUE);
  ctk_container_add (CTK_CONTAINER (toolbar), CTK_WIDGET (button));

  button = ctk_tool_item_new ();
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_container_add (CTK_CONTAINER (button), box);
  label = ctk_label_new ("Fullscreen:");
  ctk_container_add (CTK_CONTAINER (box), label);
  sw = ctk_switch_new ();
  ctk_widget_set_valign (sw, CTK_ALIGN_CENTER);
  ctk_actionable_set_action_name (CTK_ACTIONABLE (sw), "win.fullscreen");
  ctk_container_add (CTK_CONTAINER (box), sw);
  ctk_container_add (CTK_CONTAINER (toolbar), CTK_WIDGET (button));

  ctk_grid_attach (CTK_GRID (grid), toolbar, 0, 0, 1, 1);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (scrolled, TRUE);
  ctk_widget_set_vexpand (scrolled, TRUE);
  view = ctk_text_view_new ();

  g_object_set_data ((GObject*)window, "bloatpad-text", view);

  ctk_container_add (CTK_CONTAINER (scrolled), view);

  ctk_grid_attach (CTK_GRID (grid), scrolled, 0, 1, 1, 1);

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
  g_signal_connect (ctk_text_view_get_buffer (CTK_TEXT_VIEW (view)), "changed",
                    G_CALLBACK (text_buffer_changed_cb), window);
  text_buffer_changed_cb (ctk_text_view_get_buffer (CTK_TEXT_VIEW (view)), window);

  ctk_widget_show_all (CTK_WIDGET (window));
}

static void
bloat_pad_activate (GApplication *application)
{
  new_window (application, NULL);
}

static void
bloat_pad_open (GApplication  *application,
                GFile        **files,
                gint           n_files,
                const gchar   *hint)
{
  gint i;

  for (i = 0; i < n_files; i++)
    new_window (application, files[i]);
}

static void
bloat_pad_finalize (GObject *object)
{
  G_OBJECT_CLASS (bloat_pad_parent_class)->finalize (object);
}

static void
new_activated (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GApplication *app = user_data;

  g_application_activate (app);
}

static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  ctk_show_about_dialog (NULL,
                         "program-name", "Bloatpad",
                         "title", "About Bloatpad",
                         "comments", "Not much to say, really.",
                         NULL);
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GApplication *app = user_data;

  g_application_quit (app);
}

static void
combo_changed (CtkComboBox *combo,
               gpointer     user_data)
{
  CtkEntry *entry = g_object_get_data (user_data, "entry");
  const gchar *action;
  gchar **accels;
  gchar *str;

  action = ctk_combo_box_get_active_id (combo);

  if (!action)
    return;

  accels = ctk_application_get_accels_for_action (ctk_window_get_application (user_data), action);
  str = g_strjoinv (",", accels);
  g_strfreev (accels);

  ctk_entry_set_text (entry, str);
}

static void
response (CtkDialog *dialog,
          guint      response_id,
          gpointer   user_data)
{
  CtkEntry *entry = g_object_get_data (user_data, "entry");
  CtkComboBox *combo = g_object_get_data (user_data, "combo");
  const gchar *action;
  const gchar *str;
  gchar **accels;

  if (response_id == CTK_RESPONSE_CLOSE)
    {
      ctk_widget_destroy (CTK_WIDGET (dialog));
      return;
    }

  action = ctk_combo_box_get_active_id (combo);

  if (!action)
    return;

  str = ctk_entry_get_text (entry);
  accels = g_strsplit (str, ",", 0);

  ctk_application_set_accels_for_action (ctk_window_get_application (user_data), action, (const gchar **) accels);
  g_strfreev (accels);
}

static void
edit_accels (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  CtkApplication *app = user_data;
  CtkWidget *combo;
  CtkWidget *entry;
  gchar **actions;
  CtkWidget *dialog;
  gint i;

  dialog = ctk_dialog_new ();
  ctk_window_set_application (CTK_WINDOW (dialog), app);
  actions = ctk_application_list_action_descriptions (app);
  combo = ctk_combo_box_text_new ();
  ctk_container_add (CTK_CONTAINER (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), combo);
  for (i = 0; actions[i]; i++)
    ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), actions[i], actions[i]);
  g_signal_connect (combo, "changed", G_CALLBACK (combo_changed), dialog);
  entry = ctk_entry_new ();
  ctk_container_add (CTK_CONTAINER (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), entry);
  ctk_dialog_add_button (CTK_DIALOG (dialog), "Close", CTK_RESPONSE_CLOSE);
  ctk_dialog_add_button (CTK_DIALOG (dialog), "Set", CTK_RESPONSE_APPLY);
  g_signal_connect (dialog, "response", G_CALLBACK (response), dialog);
  g_object_set_data (G_OBJECT (dialog), "combo", combo);
  g_object_set_data (G_OBJECT (dialog), "entry", entry);

  ctk_widget_show_all (dialog);
}

static gboolean
update_time (gpointer user_data)
{
  BloatPad *bloatpad = user_data;
  GDateTime *now;
  gchar *time;

  while (g_menu_model_get_n_items (G_MENU_MODEL (bloatpad->time)))
    g_menu_remove (bloatpad->time, 0);

  g_message ("Updating the time menu (which should be open now)...");

  now = g_date_time_new_now_local ();
  time = g_date_time_format (now, "%c");
  g_menu_append (bloatpad->time, time, NULL);
  g_date_time_unref (now);
  g_free (time);

  return G_SOURCE_CONTINUE;
}

static void
time_active_changed (GSimpleAction *action,
                     GVariant      *state,
                     gpointer       user_data)
{
  BloatPad *bloatpad = user_data;

  if (g_variant_get_boolean (state))
    {
      if (!bloatpad->timeout)
        {
          bloatpad->timeout = g_timeout_add (1000, update_time, bloatpad);
          update_time (bloatpad);
        }
    }
  else
    {
      if (bloatpad->timeout)
        {
          g_source_remove (bloatpad->timeout);
          bloatpad->timeout = 0;
        }
    }

  g_simple_action_set_state (action, state);
}

static GActionEntry app_entries[] = {
  { "new", new_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL },
  { "edit-accels", edit_accels },
  { "time-active", NULL, NULL, "false", time_active_changed },
  { "clear-all", activate_clear_all }
};

static void
dump_accels (CtkApplication *app)
{
  gchar **actions;
  gint i;

  actions = ctk_application_list_action_descriptions (app);
  for (i = 0; actions[i]; i++)
    {
      gchar **accels;
      gchar *str;

      accels = ctk_application_get_accels_for_action (app, actions[i]);

      str = g_strjoinv (",", accels);
      g_print ("%s -> %s\n", actions[i], str);
      g_strfreev (accels);
      g_free (str);
    }
  g_strfreev (actions);
}

static void
bloat_pad_startup (GApplication *application)
{
  BloatPad *bloatpad = (BloatPad*) application;
  CtkApplication *app = CTK_APPLICATION (application);
  GMenu *menu;
  GMenuItem *item;
  GBytes *bytes;
  GIcon *icon;
  GIcon *icon2;
  GEmblem *emblem;
  GFile *file;
  gint i;
  struct {
    const gchar *action_and_target;
    const gchar *accelerators[2];
  } accels[] = {
    { "app.new", { "<Primary>n", NULL } },
    { "app.quit", { "<Primary>q", NULL } },
    { "win.copy", { "<Primary>c", NULL } },
    { "win.paste", { "<Primary>p", NULL } },
    { "win.justify::left", { "<Primary>l", NULL } },
    { "win.justify::center", { "<Primary>m", NULL } },
    { "win.justify::right", { "<Primary>r", NULL } }
  };
  const gchar *new_accels[] = { "<Primary>n", "<Primary>t", NULL };

  G_APPLICATION_CLASS (bloat_pad_parent_class)
    ->startup (application);

  g_action_map_add_action_entries (G_ACTION_MAP (application), app_entries, G_N_ELEMENTS (app_entries), application);

  for (i = 0; i < G_N_ELEMENTS (accels); i++)
    ctk_application_set_accels_for_action (app, accels[i].action_and_target, accels[i].accelerators);

  menu = ctk_application_get_menu_by_id (CTK_APPLICATION (application), "icon-menu");

  file = g_file_new_for_uri ("resource:///org/ctk/libctk/icons/16x16/actions/ctk-select-color.png");
  icon = g_file_icon_new (file);
  item = g_menu_item_new ("File Icon", NULL);
  g_menu_item_set_icon (item, icon);
  g_menu_append_item (menu, item);
  g_object_unref (item);
  g_object_unref (icon);
  g_object_unref (file);

  icon = g_themed_icon_new ("edit-find");
  item = g_menu_item_new ("Themed Icon", NULL);
  g_menu_item_set_icon (item, icon);
  g_menu_append_item (menu, item);
  g_object_unref (item);
  g_object_unref (icon);

  bytes = g_resources_lookup_data ("/org/ctk/libctk/icons/16x16/actions/ctk-select-font.png", 0, NULL);
  icon = g_bytes_icon_new (bytes);
  item = g_menu_item_new ("Bytes Icon", NULL);
  g_menu_item_set_icon (item, icon);
  g_menu_append_item (menu, item);
  g_object_unref (item);
  g_object_unref (icon);
  g_bytes_unref (bytes);

  icon = G_ICON (cdk_pixbuf_new_from_resource ("/org/ctk/libctk/icons/16x16/actions/ctk-preferences.png", NULL));
  item = g_menu_item_new ("Pixbuf", NULL);
  g_menu_item_set_icon (item, icon);
  g_menu_append_item (menu, item);
  g_object_unref (item);
  g_object_unref (icon);

  file = g_file_new_for_uri ("resource:///org/ctk/libctk/icons/16x16/actions/ctk-page-setup.png");
  icon = g_file_icon_new (file);
  emblem = g_emblem_new (icon);
  g_object_unref (icon);
  g_object_unref (file);
  file = g_file_new_for_uri ("resource:///org/ctk/libctk/icons/16x16/actions/ctk-orientation-reverse-portrait.png");
  icon2 = g_file_icon_new (file);
  icon = g_emblemed_icon_new (icon2, emblem);
  item = g_menu_item_new ("Emblemed Icon", NULL);
  g_menu_item_set_icon (item, icon);
  g_menu_append_item (menu, item);
  g_object_unref (item);
  g_object_unref (icon);
  g_object_unref (icon2);
  g_object_unref (file);
  g_object_unref (emblem);

  icon = g_themed_icon_new ("weather-severe-alert-symbolic");
  item = g_menu_item_new ("Symbolic Icon", NULL);
  g_menu_item_set_icon (item, icon);
  g_menu_append_item (menu, item);
  g_object_unref (item);
  g_object_unref (icon);

  ctk_application_set_accels_for_action (CTK_APPLICATION (application), "app.new", new_accels);

  dump_accels (CTK_APPLICATION (application));
  //ctk_application_set_menubar (CTK_APPLICATION (application), G_MENU_MODEL (ctk_builder_get_object (builder, "app-menu")));
  bloatpad->time = ctk_application_get_menu_by_id (CTK_APPLICATION (application), "time-menu");
}

static void
bloat_pad_shutdown (GApplication *application)
{
  BloatPad *bloatpad = (BloatPad *) application;

  if (bloatpad->timeout)
    {
      g_source_remove (bloatpad->timeout);
      bloatpad->timeout = 0;
    }

  G_APPLICATION_CLASS (bloat_pad_parent_class)
    ->shutdown (application);
}

static void
bloat_pad_init (BloatPad *app)
{
}

static void
bloat_pad_class_init (BloatPadClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  application_class->startup = bloat_pad_startup;
  application_class->shutdown = bloat_pad_shutdown;
  application_class->activate = bloat_pad_activate;
  application_class->open = bloat_pad_open;

  object_class->finalize = bloat_pad_finalize;

}

BloatPad *
bloat_pad_new (void)
{
  BloatPad *bloat_pad;

  g_set_application_name ("Bloatpad");

  bloat_pad = g_object_new (bloat_pad_get_type (),
                            "application-id", "org.ctk.bloatpad",
                            "flags", G_APPLICATION_HANDLES_OPEN,
                            "inactivity-timeout", 30000,
                            "register-session", TRUE,
                            NULL);

  return bloat_pad;
}

int
main (int argc, char **argv)
{
  BloatPad *bloat_pad;
  int status;
  const gchar *accels[] = { "F11", NULL };

  bloat_pad = bloat_pad_new ();

  ctk_application_set_accels_for_action (CTK_APPLICATION (bloat_pad),
                                         "win.fullscreen", accels);

  status = g_application_run (G_APPLICATION (bloat_pad), argc, argv);

  g_object_unref (bloat_pad);

  return status;
}
