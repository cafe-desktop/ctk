/* Entry/Search Entry
 *
 * GtkEntry allows to display icons and progress information.
 * This demo shows how to use these features in a search entry.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

static GtkWidget *window = NULL;
static GtkWidget *menu = NULL;
static GtkWidget *notebook = NULL;

static guint search_progress_id = 0;
static guint finish_search_id = 0;

static void
show_find_button (void)
{
  ctk_notebook_set_current_page (CTK_NOTEBOOK (notebook), 0);
}

static void
show_cancel_button (void)
{
  ctk_notebook_set_current_page (CTK_NOTEBOOK (notebook), 1);
}

static gboolean
search_progress (gpointer data)
{
  ctk_entry_progress_pulse (CTK_ENTRY (data));
  return G_SOURCE_CONTINUE;
}

static void
search_progress_done (GtkEntry *entry)
{
  ctk_entry_set_progress_fraction (entry, 0.0);
}

static gboolean
finish_search (GtkButton *button)
{
  show_find_button ();
  if (search_progress_id)
    {
      g_source_remove (search_progress_id);
      search_progress_id = 0;
    }
  return G_SOURCE_REMOVE;
}

static gboolean
start_search_feedback (gpointer data)
{
  search_progress_id = g_timeout_add_full (G_PRIORITY_DEFAULT, 100,
                                           (GSourceFunc)search_progress, data,
                                           (GDestroyNotify)search_progress_done);
  return G_SOURCE_REMOVE;
}

static void
start_search (GtkButton *button,
              GtkEntry  *entry)
{
  show_cancel_button ();
  search_progress_id = g_timeout_add_seconds (1, (GSourceFunc)start_search_feedback, entry);
  finish_search_id = g_timeout_add_seconds (15, (GSourceFunc)finish_search, button);
}


static void
stop_search (GtkButton *button,
             gpointer   data)
{
  if (finish_search_id)
    {
      g_source_remove (finish_search_id);
      finish_search_id = 0;
    }
  finish_search (button);
}

static void
clear_entry (GtkEntry *entry)
{
  ctk_entry_set_text (entry, "");
}

static void
search_by_name (GtkWidget *item,
                GtkEntry  *entry)
{
  ctk_entry_set_icon_tooltip_text (entry,
                                   CTK_ENTRY_ICON_PRIMARY,
                                   "Search by name\n"
                                   "Click here to change the search type");
  ctk_entry_set_placeholder_text (entry, "name");
}

static void
search_by_description (GtkWidget *item,
                       GtkEntry  *entry)
{

  ctk_entry_set_icon_tooltip_text (entry,
                                   CTK_ENTRY_ICON_PRIMARY,
                                   "Search by description\n"
                                   "Click here to change the search type");
  ctk_entry_set_placeholder_text (entry, "description");
}

static void
search_by_file (GtkWidget *item,
                GtkEntry  *entry)
{
  ctk_entry_set_icon_tooltip_text (entry,
                                   CTK_ENTRY_ICON_PRIMARY,
                                   "Search by file name\n"
                                   "Click here to change the search type");
  ctk_entry_set_placeholder_text (entry, "file name");
}

GtkWidget *
create_search_menu (GtkWidget *entry)
{
  GtkWidget *menu;
  GtkWidget *item;

  menu = ctk_menu_new ();

  item = ctk_menu_item_new_with_mnemonic ("Search by _name");
  g_signal_connect (item, "activate",
                    G_CALLBACK (search_by_name), entry);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  item = ctk_menu_item_new_with_mnemonic ("Search by _description");
  g_signal_connect (item, "activate",
                    G_CALLBACK (search_by_description), entry);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  item = ctk_menu_item_new_with_mnemonic ("Search by _file name");
  g_signal_connect (item, "activate",
                    G_CALLBACK (search_by_file), entry);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  ctk_widget_show_all (menu);

  return menu;
}

static void
icon_press_cb (GtkEntry       *entry,
               gint            position,
               GdkEventButton *event,
               gpointer        data)
{
  if (position == CTK_ENTRY_ICON_PRIMARY)
    ctk_menu_popup_at_pointer (CTK_MENU (menu), (GdkEvent *) event);
}

static void
activate_cb (GtkEntry  *entry,
             GtkButton *button)
{
  if (search_progress_id != 0)
    return;

  start_search (button, entry);

}

static void
search_entry_destroyed (GtkWidget *widget)
{
  if (finish_search_id != 0)
    {
      g_source_remove (finish_search_id);
      finish_search_id = 0;
    }

  if (search_progress_id != 0)
    {
      g_source_remove (search_progress_id);
      search_progress_id = 0;
    }

  window = NULL;
}

static void
entry_populate_popup (GtkEntry *entry,
                      GtkMenu  *menu,
                      gpointer user_data)
{
  GtkWidget *item;
  GtkWidget *search_menu;
  gboolean has_text;

  has_text = ctk_entry_get_text_length (entry) > 0;

  item = ctk_separator_menu_item_new ();
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  item = ctk_menu_item_new_with_mnemonic ("C_lear");
  ctk_widget_show (item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (clear_entry), entry);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);
  ctk_widget_set_sensitive (item, has_text);

  search_menu = create_search_menu (CTK_WIDGET (entry));
  item = ctk_menu_item_new_with_label ("Search by");
  ctk_widget_show (item);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (item), search_menu);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);
}

GtkWidget *
do_search_entry (GtkWidget *do_widget)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *find_button;
  GtkWidget *cancel_button;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window), ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Search Entry");
      ctk_window_set_resizable (CTK_WINDOW (window), FALSE);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (search_entry_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label), "Search entry demo");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 0);

      /* Create our entry */
      entry = ctk_search_entry_new ();
      ctk_box_pack_start (CTK_BOX (hbox), entry, FALSE, FALSE, 0);

      /* Create the find and cancel buttons */
      notebook = ctk_notebook_new ();
      ctk_notebook_set_show_tabs (CTK_NOTEBOOK (notebook), FALSE);
      ctk_notebook_set_show_border (CTK_NOTEBOOK (notebook), FALSE);
      ctk_box_pack_start (CTK_BOX (hbox), notebook, FALSE, FALSE, 0);

      find_button = ctk_button_new_with_label ("Find");
      g_signal_connect (find_button, "clicked",
                        G_CALLBACK (start_search), entry);
      ctk_notebook_append_page (CTK_NOTEBOOK (notebook), find_button, NULL);
      ctk_widget_show (find_button);

      cancel_button = ctk_button_new_with_label ("Cancel");
      g_signal_connect (cancel_button, "clicked",
                        G_CALLBACK (stop_search), NULL);
      ctk_notebook_append_page (CTK_NOTEBOOK (notebook), cancel_button, NULL);
      ctk_widget_show (cancel_button);

      /* Set up the search icon */
      search_by_name (NULL, CTK_ENTRY (entry));

      /* Set up the clear icon */
      g_signal_connect (entry, "icon-press",
                        G_CALLBACK (icon_press_cb), NULL);
      g_signal_connect (entry, "activate",
                        G_CALLBACK (activate_cb), NULL);

      /* Create the menu */
      menu = create_search_menu (entry);
      ctk_menu_attach_to_widget (CTK_MENU (menu), entry, NULL);

      /* add accessible alternatives for icon functionality */
      g_object_set (entry, "populate-all", TRUE, NULL);
      g_signal_connect (entry, "populate-popup",
                        G_CALLBACK (entry_populate_popup), NULL);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    {
      ctk_widget_destroy (menu);
      ctk_widget_destroy (window);
    }

  return window;
}
