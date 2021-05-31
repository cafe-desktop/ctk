#include <ctk/ctk.h>

static CtkTargetEntry entries[] = {
  { "CTK_LIST_BOX_ROW", CTK_TARGET_SAME_APP, 0 }
};

static void
drag_begin (CtkWidget      *widget,
            GdkDragContext *context,
            gpointer        data)
{
  CtkWidget *row;
  CtkAllocation alloc;
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
  cairo_surface_set_device_offset (surface, -x * sy, -y * sy);
  ctk_drag_set_icon_surface (context, surface);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  g_object_set_data (G_OBJECT (ctk_widget_get_parent (row)), "drag-row", row);
  ctk_style_context_add_class (ctk_widget_get_style_context (row), "drag-row");
}

static void
drag_end (CtkWidget      *widget,
          GdkDragContext *context,
          gpointer        data)
{
  CtkWidget *row;

  row = ctk_widget_get_ancestor (widget, CTK_TYPE_LIST_BOX_ROW);
  g_object_set_data (G_OBJECT (ctk_widget_get_parent (row)), "drag-row", NULL);
  ctk_style_context_remove_class (ctk_widget_get_style_context (row), "drag-row");
  ctk_style_context_remove_class (ctk_widget_get_style_context (row), "drag-hover");
}

void
drag_data_get (CtkWidget        *widget,
               GdkDragContext   *context,
               CtkSelectionData *selection_data,
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

static CtkListBoxRow *
get_last_row (CtkListBox *list)
{
  int i;
  CtkListBoxRow *row;

  row = NULL;
  for (i = 0; ; i++)
    {
      CtkListBoxRow *tmp;
      tmp = ctk_list_box_get_row_at_index (list, i);
      if (tmp == NULL)
        return row;
      row = tmp;
    }
  return row;
}

static CtkListBoxRow *
get_row_before (CtkListBox    *list,
                CtkListBoxRow *row)
{
  int pos = ctk_list_box_row_get_index (row);
  return ctk_list_box_get_row_at_index (list, pos - 1);
}

static CtkListBoxRow *
get_row_after (CtkListBox    *list,
               CtkListBoxRow *row)
{
  int pos = ctk_list_box_row_get_index (row);
  return ctk_list_box_get_row_at_index (list, pos + 1);
}

static void
drag_data_received (CtkWidget        *widget,
                    GdkDragContext   *context,
                    gint              x,
                    gint              y,
                    CtkSelectionData *selection_data,
                    guint             info,
                    guint32           time,
                    gpointer          data)
{
  CtkWidget *row_before;
  CtkWidget *row_after;
  CtkWidget *row;
  CtkWidget *source;
  int pos;

  row_before = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "row-before"));
  row_after = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "row-after"));

  g_object_set_data (G_OBJECT (widget), "row-before", NULL);
  g_object_set_data (G_OBJECT (widget), "row-after", NULL);

  if (row_before)
    ctk_style_context_remove_class (ctk_widget_get_style_context (row_before), "drag-hover-bottom");
  if (row_after)
    ctk_style_context_remove_class (ctk_widget_get_style_context (row_after), "drag-hover-top");

  row = (gpointer)* (gpointer*)ctk_selection_data_get_data (selection_data);
  source = ctk_widget_get_ancestor (row, CTK_TYPE_LIST_BOX_ROW);

  if (source == row_after)
    return;

  g_object_ref (source);
  ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (source)), source);

  if (row_after)
    pos = ctk_list_box_row_get_index (CTK_LIST_BOX_ROW (row_after));
  else
    pos = ctk_list_box_row_get_index (CTK_LIST_BOX_ROW (row_before)) + 1;

  ctk_list_box_insert (CTK_LIST_BOX (widget), source, pos);
  g_object_unref (source);
}

static gboolean
drag_motion (CtkWidget      *widget,
             GdkDragContext *context,
             int             x,
             int             y,
             guint           time)
{
  CtkAllocation alloc;
  CtkWidget *row;
  int hover_row_y;
  int hover_row_height;
  CtkWidget *drag_row;
  CtkWidget *row_before;
  CtkWidget *row_after;

  row = CTK_WIDGET (ctk_list_box_get_row_at_y (CTK_LIST_BOX (widget), y));

  drag_row = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "drag-row"));
  row_before = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "row-before"));
  row_after = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "row-after"));

  ctk_style_context_remove_class (ctk_widget_get_style_context (drag_row), "drag-hover");
  if (row_before)
    ctk_style_context_remove_class (ctk_widget_get_style_context (row_before), "drag-hover-bottom");
  if (row_after)
    ctk_style_context_remove_class (ctk_widget_get_style_context (row_after), "drag-hover-top");

  if (row)
    {
      ctk_widget_get_allocation (row, &alloc);
      hover_row_y = alloc.y;
      hover_row_height = alloc.height;

      if (y < hover_row_y + hover_row_height/2)
        {
          row_after = row;
          row_before = CTK_WIDGET (get_row_before (CTK_LIST_BOX (widget), CTK_LIST_BOX_ROW (row)));
        }
      else
        {
          row_before = row;
          row_after = CTK_WIDGET (get_row_after (CTK_LIST_BOX (widget), CTK_LIST_BOX_ROW (row)));
        }
    }
  else
    {
      row_before = CTK_WIDGET (get_last_row (CTK_LIST_BOX (widget)));
      row_after = NULL;
    }

  g_object_set_data (G_OBJECT (widget), "row-before", row_before);
  g_object_set_data (G_OBJECT (widget), "row-after", row_after);

  if (drag_row == row_before || drag_row == row_after)
    {
      ctk_style_context_add_class (ctk_widget_get_style_context (drag_row), "drag-hover");
      return FALSE;
    }

  if (row_before)
    ctk_style_context_add_class (ctk_widget_get_style_context (row_before), "drag-hover-bottom");
  if (row_after)
    ctk_style_context_add_class (ctk_widget_get_style_context (row_after), "drag-hover-top");

  return TRUE;
}

static void
drag_leave (CtkWidget      *widget,
            GdkDragContext *context,
            guint           time)
{
  CtkWidget *drag_row;
  CtkWidget *row_before;
  CtkWidget *row_after;

  drag_row = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "drag-row"));
  row_before = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "row-before"));
  row_after = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "row-after"));

  ctk_style_context_remove_class (ctk_widget_get_style_context (drag_row), "drag-hover");
  if (row_before)
    ctk_style_context_remove_class (ctk_widget_get_style_context (row_before), "drag-hover-bottom");
  if (row_after)
    ctk_style_context_remove_class (ctk_widget_get_style_context (row_after), "drag-hover-top");
}

static CtkWidget *
create_row (const gchar *text)
{
  CtkWidget *row, *ebox, *box, *label, *image;

  row = ctk_list_box_row_new ();
  ebox = ctk_event_box_new ();
  image = ctk_image_new_from_icon_name ("open-menu-symbolic", 1);
  ctk_container_add (CTK_CONTAINER (ebox), image);
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box, "margin-start", 10, "margin-end", 10, NULL);
  label = ctk_label_new (text);
  ctk_container_add (CTK_CONTAINER (row), box);
  ctk_box_pack_start (CTK_BOX (box), label, TRUE, FALSE, 0);
  ctk_container_add (CTK_CONTAINER (box), ebox);

  ctk_style_context_add_class (ctk_widget_get_style_context (row), "row");
  ctk_drag_source_set (ebox, GDK_BUTTON1_MASK, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (ebox, "drag-begin", G_CALLBACK (drag_begin), NULL);
  g_signal_connect (ebox, "drag-end", G_CALLBACK (drag_end), NULL);
  g_signal_connect (ebox, "drag-data-get", G_CALLBACK (drag_data_get), NULL);

  return row;
}

static void
on_row_activated (CtkListBox *self,
                  CtkWidget  *child)
{
  const char *id;
  id = g_object_get_data (G_OBJECT (ctk_bin_get_child (CTK_BIN (child))), "id");
  g_message ("Row activated %p: %s", child, id);
}

static void
on_selected_children_changed (CtkListBox *self)
{
  g_message ("Selection changed");
}

static void
a11y_selection_changed (AtkObject *obj)
{
  g_message ("Accessible selection changed");
}

static void
selection_mode_changed (CtkComboBox *combo, gpointer data)
{
  CtkListBox *list = data;

  ctk_list_box_set_selection_mode (list, ctk_combo_box_get_active (combo));
}

static const char *css =
  ".row:not(:first-child) { "
  "  border-top: 1px solid alpha(gray,0.5); "
  "  border-bottom: 1px solid transparent; "
  "}"
  ".row:first-child { "
  "  border-top: 1px solid transparent; "
  "  border-bottom: 1px solid transparent; "
  "}"
  ".row:last-child { "
  "  border-top: 1px solid alpha(gray,0.5); "
  "  border-bottom: 1px solid alpha(gray,0.5); "
  "}"
  ".row.drag-icon { "
  "  background: white; "
  "  border: 1px solid black; "
  "}"
  ".row.drag-row { "
  "  color: gray; "
  "  background: alpha(gray,0.2); "
  "}"
  ".row.drag-row.drag-hover { "
  "  border-top: 1px solid #4e9a06; "
  "  border-bottom: 1px solid #4e9a06; "
  "}"
  ".row.drag-hover image, "
  ".row.drag-hover label { "
  "  color: #4e9a06; "
  "}"
  ".row.drag-hover-top {"
  "  border-top: 1px solid #4e9a06; "
  "}"
  ".row.drag-hover-bottom {"
  "  border-bottom: 1px solid #4e9a06; "
  "}"
;

int
main (int argc, char *argv[])
{
  CtkWidget *window, *list, *sw, *row;
  CtkWidget *hbox, *vbox, *combo, *button;
  gint i;
  gchar *text;
  CtkCssProvider *provider;

  ctk_init (NULL, NULL);

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider, css, -1, NULL);
  ctk_style_context_add_provider_for_screen (gdk_screen_get_default (), CTK_STYLE_PROVIDER (provider), 800);
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), -1, 300);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_container_add (CTK_CONTAINER (window), hbox);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  g_object_set (vbox, "margin", 12, NULL);
  ctk_container_add (CTK_CONTAINER (hbox), vbox);

  list = ctk_list_box_new ();
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (list), CTK_SELECTION_NONE);

  ctk_drag_dest_set (list, CTK_DEST_DEFAULT_MOTION | CTK_DEST_DEFAULT_DROP, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (list, "drag-data-received", G_CALLBACK (drag_data_received), NULL);
  g_signal_connect (list, "drag-motion", G_CALLBACK (drag_motion), NULL);
  g_signal_connect (list, "drag-leave", G_CALLBACK (drag_leave), NULL);

  g_signal_connect (list, "row-activated", G_CALLBACK (on_row_activated), NULL);
  g_signal_connect (list, "selected-rows-changed", G_CALLBACK (on_selected_children_changed), NULL);
  g_signal_connect (ctk_widget_get_accessible (list), "selection-changed", G_CALLBACK (a11y_selection_changed), NULL);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (sw, TRUE);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw), CTK_POLICY_NEVER, CTK_POLICY_ALWAYS);
  ctk_container_add (CTK_CONTAINER (hbox), sw);
  ctk_container_add (CTK_CONTAINER (sw), list);

  button = ctk_check_button_new_with_label ("Activate on single click");
  g_object_bind_property (list, "activate-on-single-click",
                          button, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  ctk_container_add (CTK_CONTAINER (vbox), button);

  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "None");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Single");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Browse");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Multiple");
  g_signal_connect (combo, "changed", G_CALLBACK (selection_mode_changed), list);
  ctk_container_add (CTK_CONTAINER (vbox), combo);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), ctk_list_box_get_selection_mode (CTK_LIST_BOX (list)));

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
