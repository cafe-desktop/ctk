/* Clipboard
 *
 * GtkClipboard is used for clipboard handling. This demo shows how to
 * copy and paste text to and from the clipboard.
 *
 * It also shows how to transfer images via the clipboard or via
 * drag-and-drop, and how to make clipboard contents persist after
 * the application exits. Clipboard persistence requires a clipboard
 * manager to run.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

void
copy_button_clicked (GtkWidget *button,
                     gpointer   user_data)
{
  GtkWidget *entry;
  GtkClipboard *clipboard;

  entry = CTK_WIDGET (user_data);

  /* Get the clipboard object */
  clipboard = ctk_widget_get_clipboard (entry,
                                        GDK_SELECTION_CLIPBOARD);

  /* Set clipboard text */
  ctk_clipboard_set_text (clipboard, ctk_entry_get_text (CTK_ENTRY (entry)), -1);
}

void
paste_received (GtkClipboard *clipboard,
                const gchar  *text,
                gpointer      user_data)
{
  GtkWidget *entry;

  entry = CTK_WIDGET (user_data);

  /* Set the entry text */
  if(text)
    ctk_entry_set_text (CTK_ENTRY (entry), text);
}

void
paste_button_clicked (GtkWidget *button,
                     gpointer   user_data)
{
  GtkWidget *entry;
  GtkClipboard *clipboard;

  entry = CTK_WIDGET (user_data);

  /* Get the clipboard object */
  clipboard = ctk_widget_get_clipboard (entry,
                                        GDK_SELECTION_CLIPBOARD);

  /* Request the contents of the clipboard, contents_received will be
     called when we do get the contents.
   */
  ctk_clipboard_request_text (clipboard,
                              paste_received, entry);
}

static GdkPixbuf *
get_image_pixbuf (GtkImage *image)
{
  const gchar *icon_name;
  GtkIconSize size;
  GtkIconTheme *icon_theme;
  int width;

  switch (ctk_image_get_storage_type (image))
    {
    case CTK_IMAGE_PIXBUF:
      return g_object_ref (ctk_image_get_pixbuf (image));
    case CTK_IMAGE_ICON_NAME:
      ctk_image_get_icon_name (image, &icon_name, &size);
      icon_theme = ctk_icon_theme_get_for_screen (ctk_widget_get_screen (CTK_WIDGET (image)));
      ctk_icon_size_lookup (size, &width, NULL);
      return ctk_icon_theme_load_icon (icon_theme,
                                       icon_name,
                                       width,
                                       CTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                       NULL);
    default:
      g_warning ("Image storage type %d not handled",
                 ctk_image_get_storage_type (image));
      return NULL;
    }
}

static void
drag_begin (GtkWidget      *widget,
            GdkDragContext *context,
            gpointer        data)
{
  GdkPixbuf *pixbuf;

  pixbuf = get_image_pixbuf (CTK_IMAGE (data));
  ctk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

void
drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *selection_data,
               guint             info,
               guint             time,
               gpointer          data)
{
  GdkPixbuf *pixbuf;

  pixbuf = get_image_pixbuf (CTK_IMAGE (data));
  ctk_selection_data_set_pixbuf (selection_data, pixbuf);
  g_object_unref (pixbuf);
}

static void
drag_data_received (GtkWidget        *widget,
                    GdkDragContext   *context,
                    gint              x,
                    gint              y,
                    GtkSelectionData *selection_data,
                    guint             info,
                    guint32           time,
                    gpointer          data)
{
  GdkPixbuf *pixbuf;

  if (ctk_selection_data_get_length (selection_data) > 0)
    {
      pixbuf = ctk_selection_data_get_pixbuf (selection_data);
      ctk_image_set_from_pixbuf (CTK_IMAGE (data), pixbuf);
      g_object_unref (pixbuf);
    }
}

static void
copy_image (GtkMenuItem *item,
            gpointer     data)
{
  GtkClipboard *clipboard;
  GdkPixbuf *pixbuf;

  clipboard = ctk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  pixbuf = get_image_pixbuf (CTK_IMAGE (data));

  ctk_clipboard_set_image (clipboard, pixbuf);
  g_object_unref (pixbuf);
}

static void
paste_image (GtkMenuItem *item,
             gpointer     data)
{
  GtkClipboard *clipboard;
  GdkPixbuf *pixbuf;

  clipboard = ctk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  pixbuf = ctk_clipboard_wait_for_image (clipboard);

  if (pixbuf)
    {
      ctk_image_set_from_pixbuf (CTK_IMAGE (data), pixbuf);
      g_object_unref (pixbuf);
    }
}

static gboolean
button_press (GtkWidget      *widget,
              GdkEventButton *button,
              gpointer        data)
{
  GtkWidget *menu;
  GtkWidget *item;

  if (button->button != GDK_BUTTON_SECONDARY)
    return FALSE;

  menu = ctk_menu_new ();

  item = ctk_menu_item_new_with_mnemonic (_("_Copy"));
  g_signal_connect (item, "activate", G_CALLBACK (copy_image), data);
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  item = ctk_menu_item_new_with_mnemonic (_("_Paste"));
  g_signal_connect (item, "activate", G_CALLBACK (paste_image), data);
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);

  ctk_menu_popup_at_pointer (CTK_MENU (menu), (GdkEvent *) button);
  return TRUE;
}

GtkWidget *
do_clipboard (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *vbox, *hbox;
      GtkWidget *label;
      GtkWidget *entry, *button;
      GtkWidget *ebox, *image;
      GtkClipboard *clipboard;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Clipboard");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);

      ctk_container_add (CTK_CONTAINER (window), vbox);

      label = ctk_label_new ("\"Copy\" will copy the text\nin the entry to the clipboard");

      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 8);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      /* Create the first entry */
      entry = ctk_entry_new ();
      ctk_box_pack_start (CTK_BOX (hbox), entry, TRUE, TRUE, 0);

      /* Create the button */
      button = ctk_button_new_with_mnemonic (_("_Copy"));
      ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (copy_button_clicked), entry);

      label = ctk_label_new ("\"Paste\" will paste the text from the clipboard to the entry");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 8);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      /* Create the second entry */
      entry = ctk_entry_new ();
      ctk_box_pack_start (CTK_BOX (hbox), entry, TRUE, TRUE, 0);

      /* Create the button */
      button = ctk_button_new_with_mnemonic (_("_Paste"));
      ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (paste_button_clicked), entry);

      label = ctk_label_new ("Images can be transferred via the clipboard, too");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
      ctk_container_set_border_width (CTK_CONTAINER (hbox), 8);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      /* Create the first image */
      image = ctk_image_new_from_icon_name ("dialog-warning",
                                            CTK_ICON_SIZE_BUTTON);
      ebox = ctk_event_box_new ();
      ctk_container_add (CTK_CONTAINER (ebox), image);
      ctk_container_add (CTK_CONTAINER (hbox), ebox);

      /* make ebox a drag source */
      ctk_drag_source_set (ebox, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_COPY);
      ctk_drag_source_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-begin",
                        G_CALLBACK (drag_begin), image);
      g_signal_connect (ebox, "drag-data-get",
                        G_CALLBACK (drag_data_get), image);

      /* accept drops on ebox */
      ctk_drag_dest_set (ebox, CTK_DEST_DEFAULT_ALL,
                         NULL, 0, GDK_ACTION_COPY);
      ctk_drag_dest_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-data-received",
                        G_CALLBACK (drag_data_received), image);

      /* context menu on ebox */
      g_signal_connect (ebox, "button-press-event",
                        G_CALLBACK (button_press), image);

      /* Create the second image */
      image = ctk_image_new_from_icon_name ("process-stop",
                                            CTK_ICON_SIZE_BUTTON);
      ebox = ctk_event_box_new ();
      ctk_container_add (CTK_CONTAINER (ebox), image);
      ctk_container_add (CTK_CONTAINER (hbox), ebox);

      /* make ebox a drag source */
      ctk_drag_source_set (ebox, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_COPY);
      ctk_drag_source_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-begin",
                        G_CALLBACK (drag_begin), image);
      g_signal_connect (ebox, "drag-data-get",
                        G_CALLBACK (drag_data_get), image);

      /* accept drops on ebox */
      ctk_drag_dest_set (ebox, CTK_DEST_DEFAULT_ALL,
                         NULL, 0, GDK_ACTION_COPY);
      ctk_drag_dest_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-data-received",
                        G_CALLBACK (drag_data_received), image);

      /* context menu on ebox */
      g_signal_connect (ebox, "button-press-event",
                        G_CALLBACK (button_press), image);

      /* tell the clipboard manager to make the data persistent */
      clipboard = ctk_clipboard_get (GDK_SELECTION_CLIPBOARD);
      ctk_clipboard_set_can_store (clipboard, NULL, 0);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
