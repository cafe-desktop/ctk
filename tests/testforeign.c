#include <gtk/gtk.h>
#include <gdk/wayland/gdkwayland.h>

static GtkWidget *window;
static GtkWidget *label;
static GtkWidget *entry;
static GtkWidget *unexport_button;
static char *export_handle;
int export_count;

static void
update_ui (void)
{
  gboolean can_unexport;
  char *label_text;

  ctk_entry_set_text (GTK_ENTRY (entry), export_handle ? export_handle : "");

  label_text = g_strdup_printf ("Export count: %d", export_count);
  ctk_label_set_text (GTK_LABEL (label), label_text);
  g_free (label_text);

  can_unexport = !!export_handle;
  ctk_widget_set_sensitive (unexport_button, can_unexport);
}

static void
exported_callback (GdkWindow  *window,
                   const char *handle,
                   gpointer    user_data)
{
  if (!export_handle)
    export_handle = g_strdup (handle);

  g_assert (g_str_equal (handle, export_handle));

  export_count++;

  update_ui ();
}

static void
export_callback (GtkWidget *widget,
                 gpointer   data)
{
  if (!gdk_wayland_window_export_handle (ctk_widget_get_window (window),
                                         exported_callback,
                                         g_strdup ("user_data"), g_free))
    g_error ("Failed to export window");

  update_ui ();
}

static void
unexport_callback (GtkWidget *widget,
                   gpointer   data)
{
  gdk_wayland_window_unexport_handle (ctk_widget_get_window (window));

  export_count--;
  if (export_count == 0)
    g_clear_pointer (&export_handle, g_free);

  update_ui ();
}

int
main (int   argc,
      char *argv[])
{
  GdkDisplay *display;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *export_button;

  ctk_init (&argc, &argv);


  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);

  display = ctk_widget_get_display (window);
  if (!GDK_IS_WAYLAND_DISPLAY (display))
    g_error ("This test is only works on Wayland");

  vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);

  label = ctk_label_new (NULL);
  entry = ctk_entry_new ();
  ctk_editable_set_editable (GTK_EDITABLE (entry), FALSE);

  export_button = ctk_button_new_with_label ("Export");
  unexport_button = ctk_button_new_with_label("Unexport");
  g_signal_connect (export_button, "clicked",
                    G_CALLBACK (export_callback), NULL);
  g_signal_connect (unexport_button, "clicked",
                    G_CALLBACK (unexport_callback), NULL);

  ctk_container_add (GTK_CONTAINER (hbox), export_button);
  ctk_container_add (GTK_CONTAINER (hbox), unexport_button);

  ctk_container_add (GTK_CONTAINER (vbox), entry);
  ctk_container_add (GTK_CONTAINER (vbox), label);
  ctk_container_add (GTK_CONTAINER (vbox), hbox);

  ctk_container_add (GTK_CONTAINER (window), vbox);

  update_ui ();

  g_signal_connect (window, "delete-event", G_CALLBACK (ctk_main_quit), NULL);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
