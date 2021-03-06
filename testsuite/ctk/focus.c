#include <ctk/ctk.h>

static void
test_window_focus (void)
{
  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *entry1;
  CtkWidget *entry2;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new ("label1"));
  entry1 = ctk_entry_new ();
  ctk_container_add (CTK_CONTAINER (box), entry1);
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new ("label2"));
  entry2 = ctk_entry_new ();
  ctk_container_add (CTK_CONTAINER (box), entry2);

  ctk_widget_show_all (box);

  g_assert_null (ctk_window_get_focus (CTK_WINDOW (window)));

  ctk_window_set_focus (CTK_WINDOW (window), entry1);

  g_assert (ctk_window_get_focus (CTK_WINDOW (window)) == entry1);

  ctk_widget_show_now (window);

  g_assert (ctk_window_get_focus (CTK_WINDOW (window)) == entry1);

  ctk_widget_grab_focus (entry2);

  g_assert (ctk_window_get_focus (CTK_WINDOW (window)) == entry2);

  ctk_widget_hide (window);

  g_assert (ctk_window_get_focus (CTK_WINDOW (window)) == entry2);

  ctk_window_set_focus (CTK_WINDOW (window), entry1);

  g_assert (ctk_window_get_focus (CTK_WINDOW (window)) == entry1);

  ctk_widget_destroy (window);
}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/focus/window", test_window_focus);

  return g_test_run ();
}
