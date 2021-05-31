/* Text View/Automatic Scrolling
 *
 * This example demonstrates how to use the gravity of
 * GtkTextMarks to keep a text view scrolled to the bottom
 * while appending text.
 */

#include <gtk/gtk.h>

/* Scroll to the end of the buffer.
 */
static gboolean
scroll_to_end (GtkTextView *textview)
{
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  GtkTextMark *mark;
  char *spaces;
  char *text;
  static int count;

  buffer = ctk_text_view_get_buffer (textview);

  /* Get "end" mark. It's located at the end of buffer because
   * of right gravity
   */
  mark = ctk_text_buffer_get_mark (buffer, "end");
  ctk_text_buffer_get_iter_at_mark (buffer, &iter, mark);

  /* and insert some text at its position, the iter will be
   * revalidated after insertion to point to the end of inserted text
   */
  spaces = g_strnfill (count++, ' ');
  ctk_text_buffer_insert (buffer, &iter, "\n", -1);
  ctk_text_buffer_insert (buffer, &iter, spaces, -1);
  text = g_strdup_printf ("Scroll to end scroll to end scroll "
                          "to end scroll to end %d", count);
  ctk_text_buffer_insert (buffer, &iter, text, -1);
  g_free (spaces);
  g_free (text);

  /* Now scroll the end mark onscreen.
   */
  ctk_text_view_scroll_mark_onscreen (textview, mark);

  /* Emulate typewriter behavior, shift to the left if we
   * are far enough to the right.
   */
  if (count > 150)
    count = 0;

  return G_SOURCE_CONTINUE;
}

/* Scroll to the bottom of the buffer.
 */
static gboolean
scroll_to_bottom (GtkTextView *textview)
{
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  GtkTextMark *mark;
  char *spaces;
  char *text;
  static int count;

  buffer = ctk_text_view_get_buffer (textview);

  /* Get end iterator */
  ctk_text_buffer_get_end_iter (buffer, &iter);

  /* and insert some text at it, the iter will be revalidated
   * after insertion to point to the end of inserted text
   */
  spaces = g_strnfill (count++, ' ');
  ctk_text_buffer_insert (buffer, &iter, "\n", -1);
  ctk_text_buffer_insert (buffer, &iter, spaces, -1);
  text = g_strdup_printf ("Scroll to bottom scroll to bottom scroll "
                          "to bottom scroll to bottom %d", count);
  ctk_text_buffer_insert (buffer, &iter, text, -1);
  g_free (spaces);
  g_free (text);

  /* Move the iterator to the beginning of line, so we don't scroll
   * in horizontal direction
   */
  ctk_text_iter_set_line_offset (&iter, 0);

  /* and place the mark at iter. the mark will stay there after we
   * insert some text at the end because it has left gravity.
   */
  mark = ctk_text_buffer_get_mark (buffer, "scroll");
  ctk_text_buffer_move_mark (buffer, mark, &iter);

  /* Scroll the mark onscreen.
   */
  ctk_text_view_scroll_mark_onscreen (textview, mark);

  /* Shift text back if we got enough to the right.
   */
  if (count > 40)
    count = 0;

  return G_SOURCE_CONTINUE;
}

static guint
setup_scroll (GtkTextView *textview,
              gboolean     to_end)
{
  GtkTextBuffer *buffer;
  GtkTextIter iter;

  buffer = ctk_text_view_get_buffer (textview);
  ctk_text_buffer_get_end_iter (buffer, &iter);

  if (to_end)
    {
      /* If we want to scroll to the end, including horizontal scrolling,
       * then we just create a mark with right gravity at the end of the
       * buffer. It will stay at the end unless explicitly moved with
       * ctk_text_buffer_move_mark.
       */
      ctk_text_buffer_create_mark (buffer, "end", &iter, FALSE);

      /* Add scrolling timeout. */
      return g_timeout_add (50, (GSourceFunc) scroll_to_end, textview);
    }
  else
    {
      /* If we want to scroll to the bottom, but not scroll horizontally,
       * then an end mark won't do the job. Just create a mark so we can
       * use it with ctk_text_view_scroll_mark_onscreen, we'll position it
       * explicitly when needed. Use left gravity so the mark stays where
       * we put it after inserting new text.
       */
      ctk_text_buffer_create_mark (buffer, "scroll", &iter, TRUE);

      /* Add scrolling timeout. */
      return g_timeout_add (100, (GSourceFunc) scroll_to_bottom, textview);
    }
}

static void
remove_timeout (GtkWidget *window,
                gpointer   timeout)
{
  g_source_remove (GPOINTER_TO_UINT (timeout));
}

static void
create_text_view (GtkWidget *hbox,
                  gboolean   to_end)
{
  GtkWidget *swindow;
  GtkWidget *textview;
  guint timeout;

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_box_pack_start (CTK_BOX (hbox), swindow, TRUE, TRUE, 0);
  textview = ctk_text_view_new ();
  ctk_container_add (CTK_CONTAINER (swindow), textview);

  timeout = setup_scroll (CTK_TEXT_VIEW (textview), to_end);

  /* Remove the timeout in destroy handler, so we don't try to
   * scroll destroyed widget.
   */
  g_signal_connect (textview, "destroy",
                    G_CALLBACK (remove_timeout),
                    GUINT_TO_POINTER (timeout));
}

GtkWidget *
do_textscroll (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *hbox;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window), "Automatic Scrolling");
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);
      ctk_window_set_default_size (CTK_WINDOW (window), 600, 400);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
      ctk_box_set_homogeneous (CTK_BOX (hbox), TRUE);
      ctk_container_add (CTK_CONTAINER (window), hbox);

      create_text_view (hbox, TRUE);
      create_text_view (hbox, FALSE);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
