/* Assistant
 *
 * Demonstrates a sample multi-step assistant. Assistants are used to divide
 * an operation into several simpler sequential steps, and to guide the user
 * through these steps.
 */

#include <gtk/gtk.h>

static GtkWidget *assistant = NULL;
static GtkWidget *progress_bar = NULL;

static gboolean
apply_changes_gradually (gpointer data)
{
  gdouble fraction;

  /* Work, work, work... */
  fraction = ctk_progress_bar_get_fraction (CTK_PROGRESS_BAR (progress_bar));
  fraction += 0.05;

  if (fraction < 1.0)
    {
      ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (progress_bar), fraction);
      return G_SOURCE_CONTINUE;
    }
  else
    {
      /* Close automatically once changes are fully applied. */
      ctk_widget_destroy (assistant);
      assistant = NULL;
      return G_SOURCE_REMOVE;
    }
}

static void
on_assistant_apply (GtkWidget *widget, gpointer data)
{
  /* Start a timer to simulate changes taking a few seconds to apply. */
  g_timeout_add (100, apply_changes_gradually, NULL);
}

static void
on_assistant_close_cancel (GtkWidget *widget, gpointer data)
{
  GtkWidget **assistant = (GtkWidget **) data;

  ctk_widget_destroy (*assistant);
  *assistant = NULL;
}

static void
on_assistant_prepare (GtkWidget *widget, GtkWidget *page, gpointer data)
{
  gint current_page, n_pages;
  gchar *title;

  current_page = ctk_assistant_get_current_page (CTK_ASSISTANT (widget));
  n_pages = ctk_assistant_get_n_pages (CTK_ASSISTANT (widget));

  title = g_strdup_printf ("Sample assistant (%d of %d)", current_page + 1, n_pages);
  ctk_window_set_title (CTK_WINDOW (widget), title);
  g_free (title);

  /* The fourth page (counting from zero) is the progress page.  The
  * user clicked Apply to get here so we tell the assistant to commit,
  * which means the changes up to this point are permanent and cannot
  * be cancelled or revisited. */
  if (current_page == 3)
      ctk_assistant_commit (CTK_ASSISTANT (widget));
}

static void
on_entry_changed (GtkWidget *widget, gpointer data)
{
  GtkAssistant *assistant = CTK_ASSISTANT (data);
  GtkWidget *current_page;
  gint page_number;
  const gchar *text;

  page_number = ctk_assistant_get_current_page (assistant);
  current_page = ctk_assistant_get_nth_page (assistant, page_number);
  text = ctk_entry_get_text (CTK_ENTRY (widget));

  if (text && *text)
    ctk_assistant_set_page_complete (assistant, current_page, TRUE);
  else
    ctk_assistant_set_page_complete (assistant, current_page, FALSE);
}

static void
create_page1 (GtkWidget *assistant)
{
  GtkWidget *box, *label, *entry;

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_container_set_border_width (CTK_CONTAINER (box), 12);

  label = ctk_label_new ("You must fill out this entry to continue:");
  ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 0);

  entry = ctk_entry_new ();
  ctk_entry_set_activates_default (CTK_ENTRY (entry), TRUE);
  ctk_widget_set_valign (entry, CTK_ALIGN_CENTER);
  ctk_box_pack_start (CTK_BOX (box), entry, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (on_entry_changed), assistant);

  ctk_widget_show_all (box);
  ctk_assistant_append_page (CTK_ASSISTANT (assistant), box);
  ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), box, "Page 1");
  ctk_assistant_set_page_type (CTK_ASSISTANT (assistant), box, CTK_ASSISTANT_PAGE_INTRO);
}

static void
create_page2 (GtkWidget *assistant)
{
  GtkWidget *box, *checkbutton;

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  ctk_container_set_border_width (CTK_CONTAINER (box), 12);

  checkbutton = ctk_check_button_new_with_label ("This is optional data, you may continue "
                                                 "even if you do not check this");
  ctk_box_pack_start (CTK_BOX (box), checkbutton, FALSE, FALSE, 0);

  ctk_widget_show_all (box);
  ctk_assistant_append_page (CTK_ASSISTANT (assistant), box);
  ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), box, TRUE);
  ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), box, "Page 2");
}

static void
create_page3 (GtkWidget *assistant)
{
  GtkWidget *label;

  label = ctk_label_new ("This is a confirmation page, press 'Apply' to apply changes");

  ctk_widget_show (label);
  ctk_assistant_append_page (CTK_ASSISTANT (assistant), label);
  ctk_assistant_set_page_type (CTK_ASSISTANT (assistant), label, CTK_ASSISTANT_PAGE_CONFIRM);
  ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), label, TRUE);
  ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), label, "Confirmation");
}

static void
create_page4 (GtkWidget *assistant)
{
  progress_bar = ctk_progress_bar_new ();
  ctk_widget_set_halign (progress_bar, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (progress_bar, CTK_ALIGN_CENTER);

  ctk_widget_show (progress_bar);
  ctk_assistant_append_page (CTK_ASSISTANT (assistant), progress_bar);
  ctk_assistant_set_page_type (CTK_ASSISTANT (assistant), progress_bar, CTK_ASSISTANT_PAGE_PROGRESS);
  ctk_assistant_set_page_title (CTK_ASSISTANT (assistant), progress_bar, "Applying changes");

  /* This prevents the assistant window from being
   * closed while we're "busy" applying changes.
   */
  ctk_assistant_set_page_complete (CTK_ASSISTANT (assistant), progress_bar, FALSE);
}

GtkWidget*
do_assistant (GtkWidget *do_widget)
{
  if (!assistant)
    {
      assistant = ctk_assistant_new ();

      ctk_window_set_default_size (CTK_WINDOW (assistant), -1, 300);

      ctk_window_set_screen (CTK_WINDOW (assistant),
                             ctk_widget_get_screen (do_widget));

      create_page1 (assistant);
      create_page2 (assistant);
      create_page3 (assistant);
      create_page4 (assistant);

      g_signal_connect (G_OBJECT (assistant), "cancel",
                        G_CALLBACK (on_assistant_close_cancel), &assistant);
      g_signal_connect (G_OBJECT (assistant), "close",
                        G_CALLBACK (on_assistant_close_cancel), &assistant);
      g_signal_connect (G_OBJECT (assistant), "apply",
                        G_CALLBACK (on_assistant_apply), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
                        G_CALLBACK (on_assistant_prepare), NULL);
    }

  if (!ctk_widget_get_visible (assistant))
    ctk_widget_show (assistant);
  else
    {
      ctk_widget_destroy (assistant);
      assistant = NULL;
    }

  return assistant;
}
