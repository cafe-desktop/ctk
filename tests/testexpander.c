#include <ctk/ctk.h>

static void
expander_cb (CtkExpander *expander, GParamSpec *pspec, CtkWindow *dialog)
{
  ctk_window_set_resizable (dialog, ctk_expander_get_expanded (expander));
}

static void
do_not_expand (CtkWidget *child, gpointer data)
{
  ctk_container_child_set (CTK_CONTAINER (ctk_widget_get_parent (child)), child,
                           "expand", FALSE, "fill", FALSE, NULL);
}

static void
response_cb (CtkDialog *dialog, gint response_id)
{
  ctk_main_quit ();
}

int
main (int argc, char *argv[])
{
  CtkWidget *dialog;
  CtkWidget *area;
  CtkWidget *box;
  CtkWidget *expander;
  CtkWidget *sw;
  CtkWidget *tv;
  CtkTextBuffer *buffer;

  ctk_init (&argc, &argv);

  dialog = ctk_message_dialog_new_with_markup (NULL,
                       0,
                       CTK_MESSAGE_ERROR,
                       CTK_BUTTONS_CLOSE,
                       "<big><b>%s</b></big>",
                       "Something went wrong");
  ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                            "Here are some more details "
                                            "but not the full story.");

  area = ctk_message_dialog_get_message_area (CTK_MESSAGE_DIALOG (dialog));
  /* make the hbox expand */
  box = ctk_widget_get_parent (area);
  ctk_container_child_set (CTK_CONTAINER (ctk_widget_get_parent (box)), box,
                           "expand", TRUE, "fill", TRUE, NULL);
  /* make the labels not expand */
  ctk_container_foreach (CTK_CONTAINER (area), do_not_expand, NULL);

  expander = ctk_expander_new ("Details:");
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw), CTK_SHADOW_IN);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_NEVER,
                                  CTK_POLICY_AUTOMATIC);

  tv = ctk_text_view_new ();
  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (tv));
  ctk_text_view_set_editable (CTK_TEXT_VIEW (tv), FALSE);
  ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (tv), CTK_WRAP_WORD);
  ctk_text_buffer_set_text (CTK_TEXT_BUFFER (buffer),
                            "Finally, the full story with all details. "
                            "And all the inside information, including "
                            "error codes, etc etc. Pages of information, "
                            "you might have to scroll down to read it all, "
                            "or even resize the window - it works !\n"
                            "A second paragraph will contain even more "
                            "innuendo, just to make you scroll down or "
                            "resize the window. Do it already !", -1);
  ctk_container_add (CTK_CONTAINER (sw), tv);
  ctk_container_add (CTK_CONTAINER (expander), sw);
  ctk_box_pack_end (CTK_BOX (area), expander, TRUE, TRUE, 0);
  ctk_widget_show_all (expander);
  g_signal_connect (expander, "notify::expanded",
                    G_CALLBACK (expander_cb), dialog);

  g_signal_connect (dialog, "response", G_CALLBACK (response_cb), NULL);

  ctk_window_present (CTK_WINDOW (dialog));

  ctk_main ();

  return 0;
}

