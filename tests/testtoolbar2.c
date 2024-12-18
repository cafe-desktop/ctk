#include <ctk/ctk.h>

int main (int   argc G_GNUC_UNUSED,
	  char *argv[] G_GNUC_UNUSED)
{
  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *frame;
  CtkWidget *box3;
  CtkWidget *view;
  CtkWidget *button;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 600, 400);
  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);
  frame = ctk_frame_new (NULL);
  ctk_container_add (CTK_CONTAINER (box), frame);
  view = ctk_text_view_new ();
  ctk_widget_set_vexpand (view, TRUE);
  ctk_container_add (CTK_CONTAINER (box), view);
  box3 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (box3, "margin", 10, NULL);
  ctk_style_context_add_class (ctk_widget_get_style_context (box3), CTK_STYLE_CLASS_LINKED);
  button = ctk_button_new_from_icon_name ("document-new-symbolic", CTK_ICON_SIZE_BUTTON);
  ctk_container_add (CTK_CONTAINER (box3), button);
  button = ctk_button_new_from_icon_name ("document-open-symbolic", CTK_ICON_SIZE_BUTTON);
  ctk_container_add (CTK_CONTAINER (box3), button);
  button = ctk_button_new_from_icon_name ("document-save-symbolic", CTK_ICON_SIZE_BUTTON);
  ctk_container_add (CTK_CONTAINER (box3), button);

  ctk_container_add (CTK_CONTAINER (frame), box3);
  
  ctk_widget_show_all (CTK_WIDGET (window));

  ctk_main ();

  return 0;
}
