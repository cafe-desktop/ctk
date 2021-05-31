#include <ctk/ctk.h>

static char *cursor_names[] = {
  /*** resize cursors that we're using for csd, from ctkwindow.c ***/
  "nw-resize",
  "n-resize",
  "ne-resize",
  "w-resize",
  "e-resize",
  "sw-resize",
  "s-resize",
  "se-resize",

  /*** resize cursors, from ctkpaned.c ***/
  "col-resize",
  "row-resize",

  /*** dnd cursors, from ctkdnd.c ***/
  "dnd-ask",
  "copy",
  "move",
  "alias",
  "no-drop",

  "none",      /* used e.g. in ctkentry.c */
  "pointer",   /* used e.g. in ctklinkbutton.c */
  "text",      /* used e.g. in ctkentry.c */
  "crosshair", /* used e.g. in ctkcolorplane.c */
  "progress",  /* used e.g. in ctkfilechooserwidget.c */
};

static void
test_cursor_existence (gconstpointer name)
{
  GdkDisplay *display;
  GdkCursor *cursor;

  display = gdk_display_get_default ();
  cursor = gdk_cursor_new_from_name (display, name);
  g_assert (cursor != NULL);
  g_object_unref (cursor);
}

int
main (int argc, char *argv[])
{
  guint i;
  char *test_name;
  char *theme;

  ctk_test_init (&argc, &argv);

  g_object_get (ctk_settings_get_default (), "ctk-cursor-theme-name", &theme, NULL);
  g_test_message ("Testing cursor theme: %s", theme);
  g_free (theme);

  for (i = 0; i < G_N_ELEMENTS (cursor_names); i++)
    {
      test_name = g_strdup_printf ("/check-cursor-names/%s", cursor_names[i]);
      g_test_add_data_func (test_name, cursor_names[i], test_cursor_existence);
      g_free (test_name);
    }

  return g_test_run();
}
