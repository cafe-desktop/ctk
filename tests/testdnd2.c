#include <gtk/gtk.h>

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

enum {
  TARGET_IMAGE,
  TARGET_TEXT
};

enum {
  TOP_LEFT,
  CENTER,
  BOTTOM_RIGHT
};

static void
image_drag_begin (GtkWidget      *widget,
                  GdkDragContext *context,
                  gpointer        data)
{
  GdkPixbuf *pixbuf;
  gint hotspot;
  gint hot_x, hot_y;

  pixbuf = get_image_pixbuf (CTK_IMAGE (data));
  hotspot = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data), "hotspot"));
  switch (hotspot)
    {
    default:
    case TOP_LEFT:
      hot_x = 0;
      hot_y = 0;
      break;
    case CENTER:
      hot_x = gdk_pixbuf_get_width (pixbuf) / 2;
      hot_y = gdk_pixbuf_get_height (pixbuf) / 2;
      break;
    case BOTTOM_RIGHT:
      hot_x = gdk_pixbuf_get_width (pixbuf);
      hot_y = gdk_pixbuf_get_height (pixbuf);
      break;
    }
  ctk_drag_set_icon_pixbuf (context, pixbuf, hot_x, hot_y);
  g_object_unref (pixbuf);
}

static void
window_destroyed (GtkWidget *window, gpointer data)
{
  GtkWidget *widget = data;

  g_print ("drag widget destroyed\n");
  g_object_unref (window);
  g_object_set_data (G_OBJECT (widget), "drag window", NULL);
}

static void
window_drag_end (GtkWidget *ebox, GdkDragContext *context, gpointer data)
{
  GtkWidget *window = data;

  ctk_widget_destroy (window);
  g_signal_handlers_disconnect_by_func (ebox, window_drag_end, data);
}

static void
window_drag_begin (GtkWidget      *widget,
                   GdkDragContext *context,
                   gpointer        data)
{
  GdkPixbuf *pixbuf;
  GtkWidget *window;
  GtkWidget *image;
  int hotspot;

  hotspot = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data), "hotspot"));

  window = g_object_get_data (G_OBJECT (widget), "drag window");
  if (window == NULL)
    {
      window = ctk_window_new (CTK_WINDOW_POPUP);
      g_print ("creating new drag widget\n");
      pixbuf = get_image_pixbuf (CTK_IMAGE (data));
      image = ctk_image_new_from_pixbuf (pixbuf);
      g_object_unref (pixbuf);
      ctk_widget_show (image);
      ctk_container_add (CTK_CONTAINER (window), image);
      g_object_ref (window);
      g_object_set_data (G_OBJECT (widget), "drag window", window);
      g_signal_connect (window, "destroy", G_CALLBACK (window_destroyed), widget);
    }
  else
    g_print ("reusing drag widget\n");

  ctk_drag_set_icon_widget (context, window, 0, 0);

  if (hotspot == CENTER)
    g_signal_connect (widget, "drag-end", G_CALLBACK (window_drag_end), window);
}

static void
update_source_target_list (GtkWidget *ebox, GtkWidget *image)
{
  GtkTargetList *target_list;

  target_list = ctk_target_list_new (NULL, 0);

  ctk_target_list_add_image_targets (target_list, TARGET_IMAGE, FALSE);
  if (ctk_image_get_storage_type (CTK_IMAGE (image)) == CTK_IMAGE_ICON_NAME)
    ctk_target_list_add_text_targets (target_list, TARGET_TEXT);

  ctk_drag_source_set_target_list (ebox, target_list);

  ctk_target_list_unref (target_list);
}

static void
update_dest_target_list (GtkWidget *ebox)
{
  GtkTargetList *target_list;

  target_list = ctk_target_list_new (NULL, 0);

  ctk_target_list_add_image_targets (target_list, TARGET_IMAGE, FALSE);
  ctk_target_list_add_text_targets (target_list, TARGET_TEXT);

  ctk_drag_dest_set_target_list (ebox, target_list);

  ctk_target_list_unref (target_list);
}

void
image_drag_data_get (GtkWidget        *widget,
                     GdkDragContext   *context,
                     GtkSelectionData *selection_data,
                     guint             info,
                     guint             time,
                     gpointer          data)
{
  GdkPixbuf *pixbuf;
  const gchar *name;

  switch (info)
    {
    case TARGET_IMAGE:
      pixbuf = get_image_pixbuf (CTK_IMAGE (data));
      ctk_selection_data_set_pixbuf (selection_data, pixbuf);
      g_object_unref (pixbuf);
      break;
    case TARGET_TEXT:
      if (ctk_image_get_storage_type (CTK_IMAGE (data)) == CTK_IMAGE_ICON_NAME)
        ctk_image_get_icon_name (CTK_IMAGE (data), &name, NULL);
      else
        name = "Boo!";
      ctk_selection_data_set_text (selection_data, name, -1);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
image_drag_data_received (GtkWidget        *widget,
                          GdkDragContext   *context,
                          gint              x,
                          gint              y,
                          GtkSelectionData *selection_data,
                          guint             info,
                          guint32           time,
                          gpointer          data)
{
  GdkPixbuf *pixbuf;
  gchar *text;

  if (ctk_selection_data_get_length (selection_data) == 0)
    return;

  switch (info)
    {
    case TARGET_IMAGE:
      pixbuf = ctk_selection_data_get_pixbuf (selection_data);
      ctk_image_set_from_pixbuf (CTK_IMAGE (data), pixbuf);
      g_object_unref (pixbuf);
      break;
    case TARGET_TEXT:
      text = (gchar *)ctk_selection_data_get_text (selection_data);
      ctk_image_set_from_icon_name (CTK_IMAGE (data), text, CTK_ICON_SIZE_DIALOG);
      g_free (text);
      break;
    default:
      g_assert_not_reached ();
    }
}


GtkWidget *
make_image (const gchar *icon_name, int hotspot)
{
  GtkWidget *image, *ebox;

  image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_DIALOG);
  ebox = ctk_event_box_new ();

  ctk_drag_source_set (ebox, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_COPY);
  update_source_target_list (ebox, image);

  g_object_set_data  (G_OBJECT (image), "hotspot", GINT_TO_POINTER (hotspot));

  g_signal_connect (ebox, "drag-begin", G_CALLBACK (image_drag_begin), image);
  g_signal_connect (ebox, "drag-data-get", G_CALLBACK (image_drag_data_get), image);

  ctk_drag_dest_set (ebox, CTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
  g_signal_connect (ebox, "drag-data-received", G_CALLBACK (image_drag_data_received), image);
  update_dest_target_list (ebox);

  ctk_container_add (CTK_CONTAINER (ebox), image);

  return ebox;
}

GtkWidget *
make_image2 (const gchar *icon_name, int hotspot)
{
  GtkWidget *image, *ebox;

  image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_DIALOG);
  ebox = ctk_event_box_new ();

  ctk_drag_source_set (ebox, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_COPY);
  update_source_target_list (ebox, image);

  g_object_set_data  (G_OBJECT (image), "hotspot", GINT_TO_POINTER (hotspot));

  g_signal_connect (ebox, "drag-begin", G_CALLBACK (window_drag_begin), image);
  g_signal_connect (ebox, "drag-data-get", G_CALLBACK (image_drag_data_get), image);

  ctk_drag_dest_set (ebox, CTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
  g_signal_connect (ebox, "drag-data-received", G_CALLBACK (image_drag_data_received), image);
  update_dest_target_list (ebox);

  ctk_container_add (CTK_CONTAINER (ebox), image);

  return ebox;
}

static void
spinner_drag_begin (GtkWidget      *widget,
                    GdkDragContext *context,
                    gpointer        data)
{
  GtkWidget *spinner;

  g_print ("GtkWidget::drag-begin\n");
  spinner = g_object_new (CTK_TYPE_SPINNER,
                          "visible", TRUE,
                          "active",  TRUE,
                          NULL);
  ctk_drag_set_icon_widget (context, spinner, 0, 0);
  g_object_set_data (G_OBJECT (context), "spinner", spinner);
}

static void
spinner_drag_end (GtkWidget      *widget,
                  GdkDragContext *context,
                  gpointer        data)
{
  GtkWidget *spinner;

  g_print ("GtkWidget::drag-end\n");
  spinner = g_object_get_data (G_OBJECT (context), "spinner");
  ctk_widget_destroy (spinner);
}

static gboolean
spinner_drag_failed (GtkWidget      *widget,
                     GdkDragContext *context,
                     GtkDragResult   result,
                     gpointer        data)
{
  GTypeClass *class;
  GEnumValue *value;

  class = g_type_class_ref (CTK_TYPE_DRAG_RESULT);
  value = g_enum_get_value (G_ENUM_CLASS (class), result);
  g_print ("GtkWidget::drag-failed %s\n", value->value_nick);
  g_type_class_unref (class);

  return FALSE;
}

void
spinner_drag_data_get (GtkWidget        *widget,
                       GdkDragContext   *context,
                       GtkSelectionData *selection_data,
                       guint             info,
                       guint             time,
                       gpointer          data)
{
  g_print ("GtkWidget::drag-data-get\n");
  ctk_selection_data_set_text (selection_data, "ACTIVE", -1);
}

static GtkWidget *
make_spinner (void)
{
  GtkWidget *spinner, *ebox;

  spinner = ctk_spinner_new ();
  ctk_spinner_start (CTK_SPINNER (spinner));
  ebox = ctk_event_box_new ();

  ctk_drag_source_set (ebox, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_COPY);
  ctk_drag_source_add_text_targets (ebox);

  g_signal_connect (ebox, "drag-begin", G_CALLBACK (spinner_drag_begin), spinner);
  g_signal_connect (ebox, "drag-end", G_CALLBACK (spinner_drag_end), spinner);
  g_signal_connect (ebox, "drag-failed", G_CALLBACK (spinner_drag_failed), spinner);
  g_signal_connect (ebox, "drag-data-get", G_CALLBACK (spinner_drag_data_get), spinner);

  ctk_container_add (CTK_CONTAINER (ebox), spinner);

  return ebox;
}

int
main (int argc, char *Argv[])
{
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *entry;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Drag And Drop");
  ctk_window_set_resizable (CTK_WINDOW (window), FALSE);

  grid = ctk_grid_new ();
  g_object_set (grid,
                "margin", 20,
                "row-spacing", 20,
                "column-spacing", 20,
                NULL);
  ctk_container_add (CTK_CONTAINER (window), grid);
  ctk_grid_attach (CTK_GRID (grid), make_image ("dialog-warning", TOP_LEFT), 0, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), make_image ("process-stop", BOTTOM_RIGHT), 1, 0, 1, 1);

  entry = ctk_entry_new ();
  ctk_grid_attach (CTK_GRID (grid), entry, 0, 1, 2, 1);

  ctk_grid_attach (CTK_GRID (grid), make_spinner (), 0, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), make_image ("weather-clear", CENTER), 1, 2, 1, 1);

  ctk_grid_attach (CTK_GRID (grid), make_image2 ("dialog-question", TOP_LEFT), 0, 3, 1, 1);

  ctk_grid_attach (CTK_GRID (grid), make_image2 ("dialog-information", CENTER), 1, 3, 1, 1);

  ctk_widget_show_all (window);
  ctk_main ();

  return 0;
}
