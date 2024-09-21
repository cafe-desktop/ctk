/* Entry/Delayed Search Entry
 *
 * CtkSearchEntry sets up CtkEntries ready for search. Search entries
 * have their "changed" signal delayed and should be used
 * when the searched operation is slow such as loads of entries
 * to search, or online searches.
 */

#include <ctk/ctk.h>

static void
search_changed_cb (CtkSearchEntry *entry,
                   CtkLabel       *result_label)
{
  const char *text;
  text = ctk_entry_get_text (CTK_ENTRY (entry));
  g_message ("search changed: %s", text);
  ctk_label_set_text (result_label, text ? text : "");
}

static void
changed_cb (CtkEditable *editable)
{
  const char *text;
  text = ctk_entry_get_text (CTK_ENTRY (editable));
  g_message ("changed: %s", text);
}

static gboolean
window_key_press_event_cb (CtkWidget    *widget G_GNUC_UNUSED,
			   CdkEvent     *event,
			   CtkSearchBar *bar)
{
  return ctk_search_bar_handle_event (bar, event);
}

static void
search_changed (CtkSearchEntry *entry G_GNUC_UNUSED,
                CtkLabel       *label)
{
  ctk_label_set_text (label, "search-changed");
}

static void
next_match (CtkSearchEntry *entry G_GNUC_UNUSED,
            CtkLabel       *label)
{
  ctk_label_set_text (label, "next-match");
}

static void
previous_match (CtkSearchEntry *entry G_GNUC_UNUSED,
                CtkLabel       *label)
{
  ctk_label_set_text (label, "previous-match");
}

static void
stop_search (CtkSearchEntry *entry G_GNUC_UNUSED,
             CtkLabel       *label)
{
  ctk_label_set_text (label, "stop-search");
}

CtkWidget *
do_search_entry2 (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *vbox;
      CtkWidget *hbox;
      CtkWidget *label;
      CtkWidget *entry;
      CtkWidget *container;
      CtkWidget *searchbar;
      CtkWidget *button;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window), "Delayed Search Entry");
      ctk_window_set_transient_for (CTK_WINDOW (window), CTK_WINDOW (do_widget));
      ctk_window_set_resizable (CTK_WINDOW (window), TRUE);
      ctk_widget_set_size_request (window, 200, -1);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 0);

      entry = ctk_search_entry_new ();
      container = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_widget_set_halign (container, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (container), entry, FALSE, FALSE, 0);
      searchbar = ctk_search_bar_new ();
      ctk_search_bar_connect_entry (CTK_SEARCH_BAR (searchbar), CTK_ENTRY (entry));
      ctk_search_bar_set_show_close_button (CTK_SEARCH_BAR (searchbar), FALSE);
      ctk_container_add (CTK_CONTAINER (searchbar), container);
      ctk_box_pack_start (CTK_BOX (vbox), searchbar, FALSE, FALSE, 0);

      /* Hook the search bar to key presses */
      g_signal_connect (window, "key-press-event",
                        G_CALLBACK (window_key_press_event_cb), searchbar);

      /* Help */
      label = ctk_label_new ("Start Typing to search");
      ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);

      /* Toggle button */
      button = ctk_toggle_button_new_with_label ("Search");
      g_object_bind_property (button, "active",
                              searchbar, "search-mode-enabled",
                              G_BINDING_BIDIRECTIONAL);
      ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);

      /* Result */
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 0);

      label = ctk_label_new ("Result:");
      ctk_label_set_xalign (CTK_LABEL (label), 0.0);
      ctk_widget_set_margin_start (label, 6);
      ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);

      label = ctk_label_new ("");
      ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);

      g_signal_connect (entry, "search-changed",
                        G_CALLBACK (search_changed_cb), label);
      g_signal_connect (entry, "changed",
                        G_CALLBACK (changed_cb), label);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 0);

      label = ctk_label_new ("Signal:");
      ctk_label_set_xalign (CTK_LABEL (label), 0.0);
      ctk_widget_set_margin_start (label, 6);
      ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);

      label = ctk_label_new ("");
      ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);

      g_signal_connect (entry, "search-changed",
                        G_CALLBACK (search_changed), label);
      g_signal_connect (entry, "next-match",
                        G_CALLBACK (next_match), label);
      g_signal_connect (entry, "previous-match",
                        G_CALLBACK (previous_match), label);
      g_signal_connect (entry, "stop-search",
                        G_CALLBACK (stop_search), label);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
