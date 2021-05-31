#include <gtk/gtk.h>

int main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *frame;
  GtkWidget *box3;
  GtkWidget *view;
  GtkWidget *button;

  ctk_init (NULL, NULL);

  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (GTK_WINDOW (window), 600, 400);
  box = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (GTK_CONTAINER (window), box);
  frame = ctk_frame_new (NULL);
  ctk_container_add (GTK_CONTAINER (box), frame);
  view = ctk_text_view_new ();
  ctk_widget_set_vexpand (view, TRUE);
  ctk_container_add (GTK_CONTAINER (box), view);
  box3 = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (box3, "margin", 10, NULL);
  ctk_style_context_add_class (ctk_widget_get_style_context (box3), GTK_STYLE_CLASS_LINKED);
  button = ctk_button_new_from_icon_name ("document-new-symbolic", GTK_ICON_SIZE_BUTTON);
  ctk_container_add (GTK_CONTAINER (box3), button);
  button = ctk_button_new_from_icon_name ("document-open-symbolic", GTK_ICON_SIZE_BUTTON);
  ctk_container_add (GTK_CONTAINER (box3), button);
  button = ctk_button_new_from_icon_name ("document-save-symbolic", GTK_ICON_SIZE_BUTTON);
  ctk_container_add (GTK_CONTAINER (box3), button);

  ctk_container_add (GTK_CONTAINER (frame), box3);
  
  ctk_widget_show_all (GTK_WIDGET (window));

  ctk_main ();

  return 0;
}
