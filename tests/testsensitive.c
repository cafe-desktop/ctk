#include <gtk/gtk.h>

static void
set_insensitive (GtkButton *b, GtkWidget *w)
{
  ctk_widget_set_sensitive (w, FALSE);
}

static void
state_changed (GtkWidget *widget)
{
  GtkStateFlags flags;
  const gchar *sep;

  g_print ("state changed: \n");

  flags = ctk_widget_get_state_flags (widget);
  sep = "";
  if (flags & CTK_STATE_FLAG_ACTIVE)
    {
      g_print ("%sactive", sep);
      sep = "|";
    }
  if (flags & CTK_STATE_FLAG_PRELIGHT)
    {
      g_print ("%sprelight", sep);
      sep = "|";
    }
  if (flags & CTK_STATE_FLAG_SELECTED)
    {
      g_print ("%sselected", sep);
      sep = "|";
    }
  if (flags & CTK_STATE_FLAG_INSENSITIVE)
    {
      g_print ("%sinsensitive", sep);
      sep = "|";
    }
  if (flags & CTK_STATE_FLAG_INCONSISTENT)
    {
      g_print ("%sinconsistent", sep);
      sep = "|";
    }
  if (flags & CTK_STATE_FLAG_FOCUSED)
    {
      g_print ("%sfocused", sep);
      sep = "|";
    }
  if (sep[0] == 0)
    g_print ("normal");
  g_print ("\n");
}

int main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *bu;
  GtkWidget *w, *c;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_container_add (CTK_CONTAINER (window), box);

  w = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 15);
  ctk_box_pack_start (CTK_BOX (box), w, TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (w), ctk_entry_new (), TRUE, TRUE, 0);
  bu = ctk_button_new_with_label ("Bu");
  ctk_box_pack_start (CTK_BOX (w), bu, TRUE, TRUE, 0);
  c = ctk_switch_new ();
  ctk_switch_set_active (CTK_SWITCH (c), TRUE);
  ctk_widget_set_halign (c, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (c, CTK_ALIGN_CENTER);
  ctk_box_pack_start (CTK_BOX (box), c, TRUE, TRUE, 0);
  g_signal_connect (bu, "clicked", G_CALLBACK (set_insensitive), w);
  g_signal_connect (bu, "state-changed", G_CALLBACK (state_changed), NULL);

  g_object_bind_property (c, "active", w, "sensitive", G_BINDING_BIDIRECTIONAL);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
