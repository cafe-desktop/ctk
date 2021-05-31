#include <gtk/gtk.h>

static GtkTargetEntry entries[] = {
  { "CTK_LIST_BOX_ROW", CTK_TARGET_SAME_APP, 0 }
};

static void
drag_begin (GtkWidget      *widget,
            GdkDragContext *context,
            gpointer        data)
{
  GtkWidget *row;
  GtkAllocation alloc;
  cairo_surface_t *surface;
  cairo_t *cr;
  int x, y;
  double sx, sy;

  row = ctk_widget_get_ancestor (widget, CTK_TYPE_LIST_BOX_ROW);
  ctk_widget_get_allocation (row, &alloc);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, alloc.width, alloc.height);
  cr = cairo_create (surface);

  ctk_style_context_add_class (ctk_widget_get_style_context (row), "drag-icon");
  ctk_widget_draw (row, cr);
  ctk_style_context_remove_class (ctk_widget_get_style_context (row), "drag-icon");

  ctk_widget_translate_coordinates (widget, row, 0, 0, &x, &y);
  cairo_surface_get_device_scale (surface, &sx, &sy);
  cairo_surface_set_device_offset (surface, -x * sx, -y * sy);
  ctk_drag_set_icon_surface (context, surface);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

void
drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *selection_data,
               guint             info,
               guint             time,
               gpointer          data)
{
  ctk_selection_data_set (selection_data,
                          gdk_atom_intern_static_string ("CTK_LIST_BOX_ROW"),
                          32,
                          (const guchar *)&widget,
                          sizeof (gpointer));
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
  GtkWidget *target;
  GtkWidget *row;
  GtkWidget *source;
  int pos;

  target = widget;

  pos = ctk_list_box_row_get_index (CTK_LIST_BOX_ROW (target));
  row = (gpointer)* (gpointer*)ctk_selection_data_get_data (selection_data);
  source = ctk_widget_get_ancestor (row, CTK_TYPE_LIST_BOX_ROW);

  if (source == target)
    return;

  g_object_ref (source);
  ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (source)), source);
  ctk_list_box_insert (CTK_LIST_BOX (ctk_widget_get_parent (target)), source, pos);
  g_object_unref (source);
}

static GtkWidget *
create_row (const gchar *text)
{
  GtkWidget *row, *handle, *box, *label, *image;

  row = ctk_list_box_row_new ();

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box, "margin-start", 10, "margin-end", 10, NULL);
  ctk_container_add (CTK_CONTAINER (row), box);

  handle = ctk_event_box_new ();
  image = ctk_image_new_from_icon_name ("open-menu-symbolic", 1);
  ctk_container_add (CTK_CONTAINER (handle), image);
  ctk_container_add (CTK_CONTAINER (box), handle);

  label = ctk_label_new (text);
  ctk_container_add_with_properties (CTK_CONTAINER (box), label, "expand", TRUE, NULL);

  ctk_drag_source_set (handle, GDK_BUTTON1_MASK, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (handle, "drag-begin", G_CALLBACK (drag_begin), NULL);
  g_signal_connect (handle, "drag-data-get", G_CALLBACK (drag_data_get), NULL);

  ctk_drag_dest_set (row, CTK_DEST_DEFAULT_ALL, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (row, "drag-data-received", G_CALLBACK (drag_data_received), NULL);

  return row;
}

static const char *css =
  ".drag-icon { "
  "  background: white; "
  "  border: 1px solid black; "
  "}";

int
main (int argc, char *argv[])
{
  GtkWidget *window, *list, *sw, *row;
  gint i;
  gchar *text;
  GtkCssProvider *provider;

  ctk_init (NULL, NULL);

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider, css, -1, NULL);
  ctk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), -1, 300);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (sw, TRUE);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw), CTK_POLICY_NEVER, CTK_POLICY_ALWAYS);
  ctk_container_add (CTK_CONTAINER (window), sw);

  list = ctk_list_box_new ();
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (list), CTK_SELECTION_NONE);
  ctk_container_add (CTK_CONTAINER (sw), list);

  for (i = 0; i < 20; i++)
    {
      text = g_strdup_printf ("Row %d", i);
      row = create_row (text);
      ctk_list_box_insert (CTK_LIST_BOX (list), row, -1);
    }

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
