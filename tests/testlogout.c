#include <gtk/gtk.h>

static GtkWidget *win;
static GtkWidget *inhibit_entry;
static GtkWidget *inhibit_logout;
static GtkWidget *inhibit_switch;
static GtkWidget *inhibit_suspend;
static GtkWidget *inhibit_idle;
static GtkWidget *inhibit_label;

static void
inhibitor_toggled (GtkToggleButton *button, GtkApplication *app)
{
  gboolean active;
  const gchar *reason;
  GtkApplicationInhibitFlags flags;
  GtkWidget *toplevel;
  guint cookie;

  active = ctk_toggle_button_get_active (button);
  reason = ctk_entry_get_text (CTK_ENTRY (inhibit_entry));

  flags = 0;
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (inhibit_logout)))
    flags |= CTK_APPLICATION_INHIBIT_LOGOUT;
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (inhibit_switch)))
    flags |= CTK_APPLICATION_INHIBIT_SWITCH;
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (inhibit_suspend)))
    flags |= CTK_APPLICATION_INHIBIT_SUSPEND;
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (inhibit_idle)))
    flags |= CTK_APPLICATION_INHIBIT_IDLE;

  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (button));

  if (active)
    {
      gchar *text;

      g_print ("Calling ctk_application_inhibit: %d, '%s'\n", flags, reason);

      cookie = ctk_application_inhibit (app, CTK_WINDOW (toplevel), flags, reason);
      g_object_set_data (G_OBJECT (button), "cookie", GUINT_TO_POINTER (cookie));
      if (cookie == 0)
        {
          g_signal_handlers_block_by_func (button, inhibitor_toggled, app);
          ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), FALSE);
          g_signal_handlers_unblock_by_func (button, inhibitor_toggled, app);
          active = FALSE;
        }
      else
        {
          text = g_strdup_printf ("%#x", cookie);
          ctk_label_set_label (CTK_LABEL (inhibit_label), text);
          g_free (text);
        }
    }
  else
    {
      cookie = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button), "cookie"));
      g_print ("Calling ctk_application_uninhibit: %#x\n", cookie);
      ctk_application_uninhibit (app, cookie);
      ctk_label_set_label (CTK_LABEL (inhibit_label), "");
    }

  ctk_widget_set_sensitive (inhibit_entry, !active);
  ctk_widget_set_sensitive (inhibit_logout, !active);
  ctk_widget_set_sensitive (inhibit_switch, !active);
  ctk_widget_set_sensitive (inhibit_suspend, !active);
  ctk_widget_set_sensitive (inhibit_idle, !active);
}

static void
activate (GtkApplication *app,
          gpointer        data)
{
  GtkWidget *box;
  GtkWidget *separator;
  GtkWidget *grid;
  GtkWidget *button;
  GtkWidget *label;

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  g_object_set (box, "margin", 12, NULL);
  ctk_container_add (CTK_CONTAINER (win), box);

  grid = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 6);

  ctk_container_add (CTK_CONTAINER (box), grid);

  label = ctk_label_new ("Inhibitor");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);

  inhibit_label = ctk_label_new ("");
  ctk_grid_attach (CTK_GRID (grid), inhibit_label, 1, 0, 1, 1);

  inhibit_logout = ctk_check_button_new_with_label ("Logout");
  ctk_grid_attach (CTK_GRID (grid), inhibit_logout, 1, 1, 1, 1);

  inhibit_switch = ctk_check_button_new_with_label ("User switching");
  ctk_grid_attach (CTK_GRID (grid), inhibit_switch, 1, 2, 1, 1);

  inhibit_suspend = ctk_check_button_new_with_label ("Suspend");
  ctk_grid_attach (CTK_GRID (grid), inhibit_suspend, 1, 4, 1, 1);

  inhibit_idle = ctk_check_button_new_with_label ("Idle");
  ctk_grid_attach (CTK_GRID (grid), inhibit_idle, 1, 5, 1, 1);

  inhibit_entry = ctk_entry_new ();
  ctk_grid_attach (CTK_GRID (grid), inhibit_entry, 1, 6, 1, 1);

  button = ctk_toggle_button_new_with_label ("Inhibit");
  g_signal_connect (button, "toggled",
                    G_CALLBACK (inhibitor_toggled), app);
  ctk_grid_attach (CTK_GRID (grid), button, 2, 6, 1, 1);

  separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_container_add (CTK_CONTAINER (box), separator);

  grid = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 6);

  ctk_widget_show_all (win);

  ctk_application_add_window (app, CTK_WINDOW (win));
}

static void
quit (GtkApplication *app,
      gpointer        data)
{
  g_print ("Received quit\n");
  ctk_widget_destroy (win);
}

int
main (int argc, char *argv[])
{
  GtkApplication *app;

  app = ctk_application_new ("org.gtk.Test.session", 0);
  g_object_set (app, "register-session", TRUE, NULL);

  g_signal_connect (app, "activate",
                    G_CALLBACK (activate), NULL);
  g_signal_connect (app, "quit",
                    G_CALLBACK (quit), NULL);

  g_application_run (G_APPLICATION (app), argc, argv);

  return 0;
}
