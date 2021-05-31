#include "config.h"
#include <gtk/gtk.h>

GtkWidget *notebook;


static void
remove_notebook_page (GtkWidget *button,
		      GtkWidget *toplevel)
{
  ctk_container_remove (GTK_CONTAINER (notebook), toplevel);
  ctk_widget_show (toplevel);
}

GtkWidget *
create_tab_label (GtkWidget *toplevel)
{
  GtkWidget *box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget *label = ctk_label_new (G_OBJECT_TYPE_NAME (toplevel));
  GtkWidget *button = ctk_button_new ();
  GtkWidget *image = ctk_image_new_from_icon_name ("window-close", GTK_ICON_SIZE_MENU);

  ctk_container_add (GTK_CONTAINER (button), image);
  ctk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  ctk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (remove_notebook_page), toplevel);

  ctk_widget_show_all (box);
  return box;
}

static void
toplevel_delete_event (GtkWidget *toplevel,
		       GdkEvent  *event,
		       gpointer   none)
{
  GdkWindow *gdk_win;
  GtkWidget *label = create_tab_label (toplevel);

  gdk_win = ctk_widget_get_window (notebook);
  g_assert (gdk_win);

  ctk_widget_hide (toplevel);
  ctk_widget_unrealize (toplevel);

  ctk_widget_set_parent_window (toplevel, gdk_win);
  ctk_notebook_append_page (GTK_NOTEBOOK (notebook), toplevel, label);

  ctk_widget_show (toplevel);
}

gint
main (gint argc, gchar **argv)
{
  GtkWidget *window;
  GtkWidget *widget;
  
  ctk_init (&argc, &argv);

  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (GTK_WINDOW (window), "Toplevel widget embedding example");
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);

  notebook = ctk_notebook_new ();
  ctk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  ctk_container_add (GTK_CONTAINER (window), notebook);

  ctk_widget_realize (notebook);

  widget = ctk_about_dialog_new ();
  toplevel_delete_event (widget, NULL, NULL);
  g_signal_connect (widget, "delete-event", G_CALLBACK (toplevel_delete_event), NULL);

  widget = ctk_file_chooser_dialog_new ("the chooser", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, NULL, NULL);
  toplevel_delete_event (widget, NULL, NULL);
  g_signal_connect (widget, "delete-event", G_CALLBACK (toplevel_delete_event), NULL);

  widget = ctk_color_chooser_dialog_new ("the colorsel", NULL);
  toplevel_delete_event (widget, NULL, NULL);
  g_signal_connect (widget, "delete-event", G_CALLBACK (toplevel_delete_event), NULL);

  widget = ctk_font_chooser_dialog_new ("the fontsel", NULL);
  toplevel_delete_event (widget, NULL, NULL);
  g_signal_connect (widget, "delete-event", G_CALLBACK (toplevel_delete_event), NULL);

  widget = ctk_recent_chooser_dialog_new ("the recent chooser", NULL,
					  "_Cancel", GTK_RESPONSE_CANCEL,
					  "_Open", GTK_RESPONSE_ACCEPT,
					  NULL);
  toplevel_delete_event (widget, NULL, NULL);
  g_signal_connect (widget, "delete-event", G_CALLBACK (toplevel_delete_event), NULL);

  widget = ctk_message_dialog_new (NULL, GTK_DIALOG_MODAL, 
				   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				   "Do you have any questions ?");
  toplevel_delete_event (widget, NULL, NULL);
  g_signal_connect (widget, "delete-event", G_CALLBACK (toplevel_delete_event), NULL);

  ctk_widget_show_all (window);
  ctk_main ();

  return 0;
}
