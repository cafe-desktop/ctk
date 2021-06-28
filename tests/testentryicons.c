#include <ctk/ctk.h>
#include <stdio.h>

static void
clear_pressed (CtkEntry *entry, gint icon, CdkEvent *event, gpointer data)
{
   if (icon == CTK_ENTRY_ICON_SECONDARY)
     ctk_entry_set_text (entry, "");
}

static void
drag_begin_cb (CtkWidget      *widget,
               CdkDragContext *context,
               gpointer        user_data)
{
  gint pos;

  pos = ctk_entry_get_current_icon_drag_source (CTK_ENTRY (widget));
  if (pos != -1)
    ctk_drag_set_icon_name (context, "dialog-information", 2, 2);
}

static void
drag_data_get_cb (CtkWidget        *widget,
                  CdkDragContext   *context,
                  CtkSelectionData *data,
                  guint             info,
                  guint             time,
                  gpointer          user_data)
{
  gint pos;

  pos = ctk_entry_get_current_icon_drag_source (CTK_ENTRY (widget));

  if (pos == CTK_ENTRY_ICON_PRIMARY)
    {
      gint start, end;

      if (ctk_editable_get_selection_bounds (CTK_EDITABLE (widget), &start, &end))
        {
          gchar *str;

          str = ctk_editable_get_chars (CTK_EDITABLE (widget), start, end);
          ctk_selection_data_set_text (data, str, -1);
          g_free (str);
        }
      else
        ctk_selection_data_set_text (data, "XXX", -1);
    }
}

static void
set_blank (CtkWidget *button,
           CtkEntry  *entry)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    ctk_entry_set_icon_from_icon_name (entry, CTK_ENTRY_ICON_SECONDARY, NULL);
}

static void
set_icon_name (CtkWidget *button,
               CtkEntry  *entry)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    ctk_entry_set_icon_from_icon_name (entry, CTK_ENTRY_ICON_SECONDARY, "media-floppy");
}

static void
set_gicon (CtkWidget *button,
           CtkEntry  *entry)
{
  GIcon *icon;

 if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    {
      icon = g_themed_icon_new ("ctk-yes");
      ctk_entry_set_icon_from_gicon (entry, CTK_ENTRY_ICON_SECONDARY, icon);
      g_object_unref (icon);
    }
}

static void
set_pixbuf (CtkWidget *button,
            CtkEntry  *entry)
{
  CdkPixbuf *pixbuf;

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    {
      pixbuf = cdk_pixbuf_new_from_resource ("/org/ctk/libctk/inspector/logo.png", NULL);
      ctk_entry_set_icon_from_pixbuf (entry, CTK_ENTRY_ICON_SECONDARY, pixbuf);
      g_object_unref (pixbuf);
    }
}

int
main (int argc, char **argv)
{
  CtkWidget *window;
  CtkWidget *grid;
  CtkWidget *label;
  CtkWidget *entry;
  CtkWidget *box;
  CtkWidget *button1;
  CtkWidget *button2;
  CtkWidget *button3;
  CtkWidget *button4;
  GIcon *icon;
  CtkTargetList *tlist;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Ctk Entry Icons Test");
  ctk_container_set_border_width (CTK_CONTAINER (window), 12);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (ctk_main_quit), NULL);

  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);
  ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 6);

  /*
   * Open File - Sets the icon using a GIcon
   */
  label = ctk_label_new ("Open File:");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

  entry = ctk_entry_new ();
  ctk_widget_set_hexpand (entry, TRUE);
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 0, 1, 1);

  icon = g_themed_icon_new ("folder-symbolic");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "folder-symbolic");

  ctk_entry_set_icon_from_gicon (CTK_ENTRY (entry),
				 CTK_ENTRY_ICON_PRIMARY,
				 icon);
  ctk_entry_set_icon_sensitive (CTK_ENTRY (entry),
			        CTK_ENTRY_ICON_PRIMARY,
				FALSE);

  ctk_entry_set_icon_tooltip_text (CTK_ENTRY (entry),
				   CTK_ENTRY_ICON_PRIMARY,
				   "Open a file");

  /*
   * Save File - sets the icon using an icon name.
   */
  label = ctk_label_new ("Save File:");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

  entry = ctk_entry_new ();
  ctk_widget_set_hexpand (entry, TRUE);
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 1, 1, 1);
  ctk_entry_set_text (CTK_ENTRY (entry), "‏Right-to-left");
  ctk_widget_set_direction (entry, CTK_TEXT_DIR_RTL);
  
  ctk_entry_set_icon_from_icon_name (CTK_ENTRY (entry),
                                     CTK_ENTRY_ICON_PRIMARY,
                                     "document-save-symbolic");
  ctk_entry_set_icon_tooltip_text (CTK_ENTRY (entry),
				   CTK_ENTRY_ICON_PRIMARY,
				   "Save a file");
  tlist = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_text_targets (tlist, 0);
  ctk_entry_set_icon_drag_source (CTK_ENTRY (entry),
                                  CTK_ENTRY_ICON_PRIMARY,
                                  tlist, CDK_ACTION_COPY); 
  g_signal_connect_after (entry, "drag-begin", 
                          G_CALLBACK (drag_begin_cb), NULL);
  g_signal_connect (entry, "drag-data-get", 
                    G_CALLBACK (drag_data_get_cb), NULL);
  ctk_target_list_unref (tlist);

  /*
   * Search - Uses a helper function
   */
  label = ctk_label_new ("Search:");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 2, 1, 1);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

  entry = ctk_entry_new ();
  ctk_widget_set_hexpand (entry, TRUE);
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 2, 1, 1);

  ctk_entry_set_placeholder_text (CTK_ENTRY (entry),
                                  "Type some text, then click an icon");

  ctk_entry_set_icon_from_icon_name (CTK_ENTRY (entry),
                                     CTK_ENTRY_ICON_PRIMARY,
                                     "edit-find-symbolic");

  ctk_entry_set_icon_tooltip_text (CTK_ENTRY (entry),
                                   CTK_ENTRY_ICON_PRIMARY,
                                   "Clicking the other icon is more interesting!");

  ctk_entry_set_icon_from_icon_name (CTK_ENTRY (entry),
                                     CTK_ENTRY_ICON_SECONDARY,
                                     "edit-clear-symbolic");

  ctk_entry_set_icon_tooltip_text (CTK_ENTRY (entry),
                                   CTK_ENTRY_ICON_SECONDARY,
                                   "Clear");

  g_signal_connect (entry, "icon-press", G_CALLBACK (clear_pressed), NULL);

  /*
   * Password - Sets the icon using an icon name
   */
  label = ctk_label_new ("Password:");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 3, 1, 1);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

  entry = ctk_entry_new ();
  ctk_widget_set_hexpand (entry, TRUE);
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 3, 1, 1);
  ctk_entry_set_visibility (CTK_ENTRY (entry), FALSE);

  ctk_entry_set_icon_from_icon_name (CTK_ENTRY (entry),
                                     CTK_ENTRY_ICON_PRIMARY,
                                     "dialog-password-symbolic");

  ctk_entry_set_icon_activatable (CTK_ENTRY (entry),
				  CTK_ENTRY_ICON_PRIMARY,
				  FALSE);

  ctk_entry_set_icon_tooltip_text (CTK_ENTRY (entry),
                                   CTK_ENTRY_ICON_PRIMARY,
                                   "The password is hidden for security");

  /* Name - Does not set any icons. */
  label = ctk_label_new ("Name:");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 4, 1, 1);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);

  entry = ctk_entry_new ();
  ctk_widget_set_hexpand (entry, TRUE);
  ctk_entry_set_placeholder_text (CTK_ENTRY (entry),
                                  "Use the RadioButtons to choose an icon");
  ctk_entry_set_icon_tooltip_text (CTK_ENTRY (entry),
                                   CTK_ENTRY_ICON_SECONDARY,
                                   "Use the RadioButtons to change this icon");
  ctk_grid_attach (CTK_GRID (grid), entry, 1, 4, 1, 1);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_grid_attach (CTK_GRID (grid), box, 0, 5, 3, 1);

  button1 = ctk_radio_button_new_with_label (NULL, "Blank");
  g_signal_connect (button1, "toggled", G_CALLBACK (set_blank), entry);
  ctk_container_add (CTK_CONTAINER (box), button1);
  button2 = ctk_radio_button_new_with_label (NULL, "Icon Name");
  ctk_radio_button_join_group (CTK_RADIO_BUTTON (button2), CTK_RADIO_BUTTON (button1));
  g_signal_connect (button2, "toggled", G_CALLBACK (set_icon_name), entry);
  ctk_container_add (CTK_CONTAINER (box), button2);
  button3 = ctk_radio_button_new_with_label (NULL, "GIcon");
  ctk_radio_button_join_group (CTK_RADIO_BUTTON (button3), CTK_RADIO_BUTTON (button1));
  g_signal_connect (button3, "toggled", G_CALLBACK (set_gicon), entry);
  ctk_container_add (CTK_CONTAINER (box), button3);
  button4 = ctk_radio_button_new_with_label (NULL, "Pixbuf");
  ctk_radio_button_join_group (CTK_RADIO_BUTTON (button4), CTK_RADIO_BUTTON (button1));
  g_signal_connect (button4, "toggled", G_CALLBACK (set_pixbuf), entry);
  ctk_container_add (CTK_CONTAINER (box), button4);

  ctk_widget_show_all (window);

  ctk_main();

  return 0;
}
