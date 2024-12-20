/* Change Display
 *
 * Demonstrates migrating a window between different displays.
 * A display is a mouse and keyboard with some number of
 * associated monitors. The neat thing about having multiple
 * displays is that they can be on a completely separate
 * computers, as long as there is a network connection to the
 * computer where the application is running.
 *
 * Only some of the windowing systems where CTK+ runs have the
 * concept of multiple displays. (The X Window System is the
 * main example.) Other windowing systems can only handle one
 * keyboard and mouse, and combine all monitors into
 * a single display.
 *
 * This is a moderately complex example, and demonstrates:
 *
 *  - Tracking the currently open displays
 *
 *  - Changing the display for a window
 *
 *  - Letting the user choose a window by clicking on it
 *
 *  - Using CtkListStore and CtkTreeView
 *
 *  - Using CtkDialog
 */
#include <string.h>
#include <glib/gi18n.h>
#include <ctk/ctk.h>

/* The ChangeDisplayInfo structure corresponds to a toplevel window and
 * holds pointers to widgets inside the toplevel window along with other
 * information about the contents of the window.
 * This is a common organizational structure in real applications.
 */
typedef struct _ChangeDisplayInfo ChangeDisplayInfo;

struct _ChangeDisplayInfo
{
  CtkWidget *window;
  CtkSizeGroup *size_group;

  CtkTreeModel *display_model;

  CdkDisplay *current_display;
};

/* These enumerations provide symbolic names for the columns
 * in the two CtkListStore models.
 */
enum
{
  DISPLAY_COLUMN_NAME,
  DISPLAY_COLUMN_DISPLAY,
  DISPLAY_NUM_COLUMNS
};

enum
{
  SCREEN_COLUMN_NUMBER,
  SCREEN_COLUMN_SCREEN,
  SCREEN_NUM_COLUMNS
};

/* Finds the toplevel window under the mouse pointer, if any.
 */
static CtkWidget *
find_toplevel_at_pointer (CdkDisplay *display G_GNUC_UNUSED)
{
  CdkWindow *pointer_window;
  CtkWidget *widget = NULL;

  pointer_window = cdk_device_get_window_at_position (ctk_get_current_event_device (),
                                                      NULL, NULL);

  /* The user data field of a CdkWindow is used to store a pointer
   * to the widget that created it.
   */
  if (pointer_window)
    {
      gpointer widget_ptr;
      cdk_window_get_user_data (pointer_window, &widget_ptr);
      widget = widget_ptr;
    }

  return widget ? ctk_widget_get_toplevel (widget) : NULL;
}

static gboolean
button_release_event_cb (CtkWidget      *widget G_GNUC_UNUSED,
                         CdkEventButton *event G_GNUC_UNUSED,
                         gboolean       *clicked)
{
  *clicked = TRUE;
  return TRUE;
}

/* Asks the user to click on a window, then waits for them click
 * the mouse. When the mouse is released, returns the toplevel
 * window under the pointer, or NULL, if there is none.
 */
static CtkWidget *
query_for_toplevel (CdkScreen  *screen,
                    const char *prompt)
{
  CdkDisplay *display = cdk_screen_get_display (screen);
  CtkWidget *popup, *label, *frame;
  CdkCursor *cursor;
  CtkWidget *toplevel = NULL;
  CdkDevice *device;

  popup = ctk_window_new (CTK_WINDOW_POPUP);
  ctk_window_set_screen (CTK_WINDOW (popup), screen);
  ctk_window_set_modal (CTK_WINDOW (popup), TRUE);
  ctk_window_set_position (CTK_WINDOW (popup), CTK_WIN_POS_CENTER);

  frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_OUT);
  ctk_container_add (CTK_CONTAINER (popup), frame);

  label = ctk_label_new (prompt);
  g_object_set (label, "margin", 10, NULL);
  ctk_container_add (CTK_CONTAINER (frame), label);

  ctk_widget_show_all (popup);
  cursor = cdk_cursor_new_from_name (display, "crosshair");
  device = ctk_get_current_event_device ();

  if (cdk_seat_grab (cdk_device_get_seat (device),
                     ctk_widget_get_window (popup),
                     CDK_SEAT_CAPABILITY_ALL_POINTING,
                     FALSE, cursor, NULL, NULL, NULL) == CDK_GRAB_SUCCESS)
    {
      gboolean clicked = FALSE;

      g_signal_connect (popup, "button-release-event",
                        G_CALLBACK (button_release_event_cb), &clicked);

      /* Process events until clicked is set by button_release_event_cb.
       * We pass in may_block=TRUE since we want to wait if there
       * are no events currently.
       */
      while (!clicked)
        g_main_context_iteration (NULL, TRUE);

      toplevel = find_toplevel_at_pointer (cdk_screen_get_display (screen));
      if (toplevel == popup)
        toplevel = NULL;
    }

  g_object_unref (cursor);
  ctk_widget_destroy (popup);
  cdk_display_flush (display);                 /* Really release the grab */

  return toplevel;
}

/* Prompts the user for a toplevel window to move, and then moves
 * that window to the currently selected display
 */
static void
query_change_display (ChangeDisplayInfo *info)
{
  CdkScreen *screen = ctk_widget_get_screen (info->window);
  CtkWidget *toplevel;

  toplevel = query_for_toplevel (screen,
                                 "Please select the toplevel\n"
                                 "to move to the new screen");

  if (toplevel)
    ctk_window_set_screen (CTK_WINDOW (toplevel), cdk_display_get_default_screen (info->current_display));
  else
    cdk_display_beep (cdk_screen_get_display (screen));
}

/* Called when the user clicks on a button in our dialog or
 * closes the dialog through the window manager. Unless the
 * "Change" button was clicked, we destroy the dialog.
 */
static void
response_cb (CtkDialog         *dialog,
             gint               response_id,
             ChangeDisplayInfo *info)
{
  if (response_id == CTK_RESPONSE_OK)
    query_change_display (info);
  else
    ctk_widget_destroy (CTK_WIDGET (dialog));
}

/* Called when the user clicks on "Open..." in the display
 * frame. Prompts for a new display, and then opens a connection
 * to that display.
 */
static void
open_display_cb (CtkWidget         *button G_GNUC_UNUSED,
                 ChangeDisplayInfo *info)
{
  CtkWidget *content_area;
  CtkWidget *dialog;
  CtkWidget *display_entry;
  CtkWidget *dialog_label;
  gchar *new_screen_name = NULL;
  CdkDisplay *result = NULL;

  dialog = ctk_dialog_new_with_buttons ("Open Display",
                                        CTK_WINDOW (info->window),
                                        CTK_DIALOG_MODAL,
                                        _("_Cancel"), CTK_RESPONSE_CANCEL,
                                        _("_OK"), CTK_RESPONSE_OK,
                                        NULL);

  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
  display_entry = ctk_entry_new ();
  ctk_entry_set_activates_default (CTK_ENTRY (display_entry), TRUE);
  dialog_label =
    ctk_label_new ("Please enter the name of\nthe new display\n");

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));

  ctk_container_add (CTK_CONTAINER (content_area), dialog_label);
  ctk_container_add (CTK_CONTAINER (content_area), display_entry);

  ctk_widget_grab_focus (display_entry);
  ctk_widget_show_all (ctk_bin_get_child (CTK_BIN (dialog)));

  while (!result)
    {
      gint response_id = ctk_dialog_run (CTK_DIALOG (dialog));
      if (response_id != CTK_RESPONSE_OK)
        break;

      new_screen_name = ctk_editable_get_chars (CTK_EDITABLE (display_entry),
                                                0, -1);

      if (strcmp (new_screen_name, "") != 0)
        {
          result = cdk_display_open (new_screen_name);
          if (!result)
            {
              gchar *error_msg =
                g_strdup_printf  ("Can't open display:\n\t%s\nplease try another one\n",
                                  new_screen_name);
              ctk_label_set_text (CTK_LABEL (dialog_label), error_msg);
              g_free (error_msg);
            }

          g_free (new_screen_name);
        }
    }

  ctk_widget_destroy (dialog);
}

/* Called when the user clicks on the "Close" button in the
 * "Display" frame. Closes the selected display.
 */
static void
close_display_cb (CtkWidget         *button G_GNUC_UNUSED,
                  ChangeDisplayInfo *info)
{
  if (info->current_display)
    cdk_display_close (info->current_display);
}

/* Called when the selected row in the display list changes.
 * Updates info->current_display, then refills the list of
 * screens.
 */
static void
display_changed_cb (CtkTreeSelection  *selection,
                    ChangeDisplayInfo *info)
{
  CtkTreeModel *model;
  CtkTreeIter iter;

  if (info->current_display)
    g_object_unref (info->current_display);
  if (ctk_tree_selection_get_selected (selection, &model, &iter))
    ctk_tree_model_get (model, &iter,
                        DISPLAY_COLUMN_DISPLAY, &info->current_display,
                        -1);
  else
    info->current_display = NULL;
}

/* This function is used both for creating the "Display" and
 * "Screen" frames, since they have a similar structure. The
 * caller hooks up the right context for the value returned
 * in tree_view, and packs any relevant buttons into button_vbox.
 */
static void
create_frame (ChangeDisplayInfo *info,
              const char        *title,
              CtkWidget        **frame,
              CtkWidget        **tree_view,
              CtkWidget        **button_vbox)
{
  CtkTreeSelection *selection;
  CtkWidget *scrollwin;
  CtkWidget *hbox;

  *frame = ctk_frame_new (title);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (hbox), 8);
  ctk_container_add (CTK_CONTAINER (*frame), hbox);

  scrollwin = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrollwin),
                                  CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrollwin),
                                       CTK_SHADOW_IN);
  ctk_box_pack_start (CTK_BOX (hbox), scrollwin, TRUE, TRUE, 0);

  *tree_view = ctk_tree_view_new ();
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (*tree_view), FALSE);
  ctk_container_add (CTK_CONTAINER (scrollwin), *tree_view);

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (*tree_view));
  ctk_tree_selection_set_mode (selection, CTK_SELECTION_BROWSE);

  *button_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_box_pack_start (CTK_BOX (hbox), *button_vbox, FALSE, FALSE, 0);

  if (!info->size_group)
    info->size_group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);

  ctk_size_group_add_widget (CTK_SIZE_GROUP (info->size_group), *button_vbox);
}

/* If we have a stack of buttons, it often looks better if their contents
 * are left-aligned, rather than centered. This function creates a button
 * and left-aligns it contents.
 */
CtkWidget *
left_align_button_new (const char *label)
{
  CtkWidget *button = ctk_button_new_with_mnemonic (label);
  CtkWidget *child = ctk_bin_get_child (CTK_BIN (button));

  ctk_widget_set_halign (child, CTK_ALIGN_START);
  ctk_widget_set_valign (child, CTK_ALIGN_CENTER);

  return button;
}

/* Creates the "Display" frame in the main window.
 */
CtkWidget *
create_display_frame (ChangeDisplayInfo *info)
{
  CtkWidget *frame;
  CtkWidget *tree_view;
  CtkWidget *button_vbox;
  CtkTreeViewColumn *column;
  CtkTreeSelection *selection;
  CtkWidget *button;

  create_frame (info, "Display", &frame, &tree_view, &button_vbox);

  button = left_align_button_new ("_Open...");
  g_signal_connect (button, "clicked",  G_CALLBACK (open_display_cb), info);
  ctk_box_pack_start (CTK_BOX (button_vbox), button, FALSE, FALSE, 0);

  button = left_align_button_new ("_Close");
  g_signal_connect (button, "clicked",  G_CALLBACK (close_display_cb), info);
  ctk_box_pack_start (CTK_BOX (button_vbox), button, FALSE, FALSE, 0);

  info->display_model = (CtkTreeModel *)ctk_list_store_new (DISPLAY_NUM_COLUMNS,
                                                            G_TYPE_STRING,
                                                            CDK_TYPE_DISPLAY);

  ctk_tree_view_set_model (CTK_TREE_VIEW (tree_view), info->display_model);

  column = ctk_tree_view_column_new_with_attributes ("Name",
                                                     ctk_cell_renderer_text_new (),
                                                     "text", DISPLAY_COLUMN_NAME,
                                                     NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view));
  g_signal_connect (selection, "changed",
                    G_CALLBACK (display_changed_cb), info);

  return frame;
}

/* Called when one of the currently open displays is closed.
 * Remove it from our list of displays.
 */
static void
display_closed_cb (CdkDisplay        *display,
                   gboolean           is_error G_GNUC_UNUSED,
                   ChangeDisplayInfo *info)
{
  CtkTreeIter iter;
  gboolean valid;

  for (valid = ctk_tree_model_get_iter_first (info->display_model, &iter);
       valid;
       valid = ctk_tree_model_iter_next (info->display_model, &iter))
    {
      CdkDisplay *tmp_display;

      ctk_tree_model_get (info->display_model, &iter,
                          DISPLAY_COLUMN_DISPLAY, &tmp_display,
                          -1);
      if (tmp_display == display)
        {
          ctk_list_store_remove (CTK_LIST_STORE (info->display_model), &iter);
          break;
        }
    }
}

/* Adds a new display to our list of displays, and connects
 * to the "closed" signal so that we can remove it from the
 * list of displays again.
 */
static void
add_display (ChangeDisplayInfo *info,
             CdkDisplay        *display)
{
  const gchar *name = cdk_display_get_name (display);
  CtkTreeIter iter;

  ctk_list_store_append (CTK_LIST_STORE (info->display_model), &iter);
  ctk_list_store_set (CTK_LIST_STORE (info->display_model), &iter,
                      DISPLAY_COLUMN_NAME, name,
                      DISPLAY_COLUMN_DISPLAY, display,
                      -1);

  g_signal_connect (display, "closed",
                    G_CALLBACK (display_closed_cb), info);
}

/* Called when a new display is opened
 */
static void
display_opened_cb (CdkDisplayManager *manager G_GNUC_UNUSED,
                   CdkDisplay        *display,
                   ChangeDisplayInfo *info)
{
  add_display (info, display);
}

/* Adds all currently open displays to our list of displays,
 * and set up a signal connection so that we'll be notified
 * when displays are opened in the future as well.
 */
static void
initialize_displays (ChangeDisplayInfo *info)
{
  CdkDisplayManager *manager = cdk_display_manager_get ();
  GSList *displays = cdk_display_manager_list_displays (manager);
  GSList *tmp_list;

  for (tmp_list = displays; tmp_list; tmp_list = tmp_list->next)
    add_display (info, tmp_list->data);

  g_slist_free (tmp_list);

  g_signal_connect (manager, "display-opened",
                    G_CALLBACK (display_opened_cb), info);
}

/* Cleans up when the toplevel is destroyed; we remove the
 * connections we use to track currently open displays, then
 * free the ChangeDisplayInfo structure.
 */
static void
destroy_info (ChangeDisplayInfo *info)
{
  CdkDisplayManager *manager = cdk_display_manager_get ();
  GSList *displays = cdk_display_manager_list_displays (manager);
  GSList *tmp_list;

  g_signal_handlers_disconnect_by_func (manager,
                                        display_opened_cb,
                                        info);

  for (tmp_list = displays; tmp_list; tmp_list = tmp_list->next)
    g_signal_handlers_disconnect_by_func (tmp_list->data,
                                          display_closed_cb,
                                          info);

  g_slist_free (tmp_list);

  g_object_unref (info->size_group);
  g_object_unref (info->display_model);

  if (info->current_display)
    g_object_unref (info->current_display);

  g_free (info);
}

static void
destroy_cb (GObject            *object G_GNUC_UNUSED,
            ChangeDisplayInfo **info)
{
  destroy_info (*info);
  *info = NULL;
}

/* Main entry point. If the dialog for this demo doesn't yet exist, creates
 * it. Otherwise, destroys it.
 */
CtkWidget *
do_changedisplay (CtkWidget *do_widget)
{
  static ChangeDisplayInfo *info = NULL;

  if (!info)
    {
      CtkWidget *content_area;
      CtkWidget *vbox;
      CtkWidget *frame;

      info = g_new0 (ChangeDisplayInfo, 1);

      info->window = ctk_dialog_new_with_buttons ("Change Display",
                                                  CTK_WINDOW (do_widget),
                                                  0,
                                                  "Close", CTK_RESPONSE_CLOSE,
                                                  "Change", CTK_RESPONSE_OK,
                                                  NULL);

      ctk_window_set_default_size (CTK_WINDOW (info->window), 300, 400);

      g_signal_connect (info->window, "response",
                        G_CALLBACK (response_cb), info);
      g_signal_connect (info->window, "destroy",
                        G_CALLBACK (destroy_cb), &info);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (info->window));

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
      ctk_box_pack_start (CTK_BOX (content_area), vbox, TRUE, TRUE, 0);

      frame = create_display_frame (info);
      ctk_box_pack_start (CTK_BOX (vbox), frame, TRUE, TRUE, 0);

      initialize_displays (info);

      ctk_widget_show_all (info->window);
      return info->window;
    }
  else
    {
      ctk_widget_destroy (info->window);
      return NULL;
    }
}
