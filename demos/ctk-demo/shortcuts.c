/* Shortcuts Window
 *
 * CtkShortcutsWindow is a window that provides a help overlay
 * for shortcuts and gestures in an application.
 */

#include <ctk/ctk.h>

static void
show_shortcuts (CtkWidget   *window,
                const gchar *id,
                const gchar *view)
{
  CtkBuilder *builder;
  CtkWidget *overlay;
  gchar *path;

  path = g_strdup_printf ("/shortcuts/%s.ui", id);
  builder = ctk_builder_new_from_resource (path);
  g_free (path);
  overlay = CTK_WIDGET (ctk_builder_get_object (builder, id));
  ctk_window_set_transient_for (CTK_WINDOW (overlay), CTK_WINDOW (window));
  g_object_set (overlay, "view-name", view, NULL);
  ctk_widget_show (overlay);
  g_object_unref (builder);
}

static void
builder_shortcuts (CtkWidget *window)
{
  show_shortcuts (window, "shortcuts-builder", NULL);
}

static void
gedit_shortcuts (CtkWidget *window)
{
  show_shortcuts (window, "shortcuts-gedit", NULL);
}

static void
clocks_shortcuts (CtkWidget *window)
{
  show_shortcuts (window, "shortcuts-clocks", NULL);
}

static void
clocks_shortcuts_stopwatch (CtkWidget *window)
{
  show_shortcuts (window, "shortcuts-clocks", "stopwatch");
}

static void
boxes_shortcuts (CtkWidget *window)
{
  show_shortcuts (window, "shortcuts-boxes", NULL);
}

static void
boxes_shortcuts_wizard (CtkWidget *window)
{
  show_shortcuts (window, "shortcuts-boxes", "wizard");
}

static void
boxes_shortcuts_display (CtkWidget *window)
{
  show_shortcuts (window, "shortcuts-boxes", "display");
}

CtkWidget *
do_shortcuts (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  static gboolean icons_added = FALSE;

  if (!icons_added)
    {
      icons_added = TRUE;
      ctk_icon_theme_add_resource_path (ctk_icon_theme_get_default (), "/icons");
    }

  g_type_ensure (G_TYPE_FILE_ICON);

  if (!window)
    {
      CtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/shortcuts/shortcuts.ui");
      ctk_builder_add_callback_symbols (builder,
                                        "builder_shortcuts", G_CALLBACK (builder_shortcuts),
                                        "gedit_shortcuts", G_CALLBACK (gedit_shortcuts),
                                        "clocks_shortcuts", G_CALLBACK (clocks_shortcuts),
                                        "clocks_shortcuts_stopwatch", G_CALLBACK (clocks_shortcuts_stopwatch),
                                        "boxes_shortcuts", G_CALLBACK (boxes_shortcuts),
                                        "boxes_shortcuts_wizard", G_CALLBACK (boxes_shortcuts_wizard),
                                        "boxes_shortcuts_display", G_CALLBACK (boxes_shortcuts_display),
                                        NULL);
      ctk_builder_connect_signals (builder, NULL);
      window = CTK_WIDGET (ctk_builder_get_object (builder, "window1"));
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      g_object_unref (builder);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
