#include <gtk/gtk.h>

static void
fill_text_view (GtkWidget *tv, const gchar *text)
{
  gint i;
  GString *s;

  s = g_string_new ("");
  for (i = 0; i < 200; i++)
    g_string_append_printf (s, "%s %d\n", text, i);

  ctk_text_buffer_set_text (ctk_text_view_get_buffer (GTK_TEXT_VIEW (tv)),
                            g_string_free (s, FALSE), -1); 
}

int
main (int argc, char *argv[])
{
  GtkWidget *win, *box, *tv, *sw, *sb;
  GtkAdjustment *adj;

  ctk_init (NULL, NULL);

  win = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (GTK_WINDOW (win), 640, 480);

  box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  ctk_container_add (GTK_CONTAINER (win), box);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_EXTERNAL);
  ctk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);
  tv = ctk_text_view_new ();
  fill_text_view (tv, "Left");
  ctk_container_add (GTK_CONTAINER (sw), tv);

  adj = ctk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));

  sw = ctk_scrolled_window_new (NULL, adj);
  ctk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_EXTERNAL);
  ctk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);
  tv = ctk_text_view_new ();
  fill_text_view (tv, "Middle");
  ctk_container_add (GTK_CONTAINER (sw), tv);

  sw = ctk_scrolled_window_new (NULL, adj);
  ctk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_EXTERNAL);
  ctk_box_pack_start (GTK_BOX (box), sw, TRUE, TRUE, 0);
  tv = ctk_text_view_new ();
  fill_text_view (tv, "Right");
  ctk_container_add (GTK_CONTAINER (sw), tv);

  sb = ctk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adj);

  ctk_container_add (GTK_CONTAINER (box), sb);

  ctk_widget_show_all (win);

  ctk_main ();

  return 0;
}
