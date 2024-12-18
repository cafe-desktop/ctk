/* Tool Palette
 *
 * A tool palette widget shows groups of toolbar items as a grid of icons
 * or a list of names.
 */

#include <string.h>
#include <ctk/ctk.h>

static CtkWidget *window = NULL;

static void load_icon_items (CtkToolPalette *palette);
static void load_toggle_items (CtkToolPalette *palette);
static void load_special_items (CtkToolPalette *palette);

typedef struct _CanvasItem CanvasItem;

struct _CanvasItem
{
  GdkPixbuf *pixbuf;
  gdouble x, y;
};

static gboolean drag_data_requested_for_drop = FALSE;
static CanvasItem *drop_item = NULL;
static GList *canvas_items = NULL;

/********************************/
/* ====== Canvas drawing ====== */
/********************************/

static CanvasItem*
canvas_item_new (CtkWidget     *widget,
                 CtkToolButton *button,
                 gdouble        x,
                 gdouble        y)
{
  CanvasItem *item = NULL;
  const gchar *icon_name;
  GdkPixbuf *pixbuf;
  CtkIconTheme *icon_theme;
  int width;

  icon_name = ctk_tool_button_get_icon_name (button);
  icon_theme = ctk_icon_theme_get_for_screen (ctk_widget_get_screen (widget));
  ctk_icon_size_lookup (CTK_ICON_SIZE_DIALOG, &width, NULL);
  pixbuf = ctk_icon_theme_load_icon (icon_theme,
                                     icon_name,
                                     width,
                                     CTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                     NULL);

  if (pixbuf)
    {
      item = g_slice_new0 (CanvasItem);
      item->pixbuf = pixbuf;
      item->x = x;
      item->y = y;
    }

  return item;
}

static void
canvas_item_free (CanvasItem *item)
{
  g_object_unref (item->pixbuf);
  g_slice_free (CanvasItem, item);
}

static void
canvas_item_draw (const CanvasItem *item,
                  cairo_t          *cr,
                  gboolean          preview)
{
  gdouble cx = gdk_pixbuf_get_width (item->pixbuf);
  gdouble cy = gdk_pixbuf_get_height (item->pixbuf);

  cdk_cairo_set_source_pixbuf (cr,
                               item->pixbuf,
                               item->x - cx * 0.5,
                               item->y - cy * 0.5);

  if (preview)
    cairo_paint_with_alpha (cr, 0.6);
  else
    cairo_paint (cr);
}

static gboolean
canvas_draw (CtkWidget *widget G_GNUC_UNUSED,
             cairo_t   *cr)
{
  GList *iter;

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  for (iter = canvas_items; iter; iter = iter->next)
    canvas_item_draw (iter->data, cr, FALSE);

  if (drop_item)
    canvas_item_draw (drop_item, cr, TRUE);

  return TRUE;
}

/*****************************/
/* ====== Palette DnD ====== */
/*****************************/

static void
palette_drop_item (CtkToolItem      *drag_item,
                   CtkToolItemGroup *drop_group,
                   gint              x,
                   gint              y)
{
  CtkWidget *drag_group = ctk_widget_get_parent (CTK_WIDGET (drag_item));
  CtkToolItem *drop_item = ctk_tool_item_group_get_drop_item (drop_group, x, y);
  gint drop_position = -1;

  if (drop_item)
    drop_position = ctk_tool_item_group_get_item_position (CTK_TOOL_ITEM_GROUP (drop_group), drop_item);

  if (CTK_TOOL_ITEM_GROUP (drag_group) != drop_group)
    {
      gboolean homogeneous, expand, fill, new_row;

      g_object_ref (drag_item);
      ctk_container_child_get (CTK_CONTAINER (drag_group), CTK_WIDGET (drag_item),
                               "homogeneous", &homogeneous,
                               "expand", &expand,
                               "fill", &fill,
                               "new-row", &new_row,
                               NULL);
      ctk_container_remove (CTK_CONTAINER (drag_group), CTK_WIDGET (drag_item));
      ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (drop_group),
                                  drag_item, drop_position);
      ctk_container_child_set (CTK_CONTAINER (drop_group), CTK_WIDGET (drag_item),
                               "homogeneous", homogeneous,
                               "expand", expand,
                               "fill", fill,
                               "new-row", new_row,
                               NULL);
      g_object_unref (drag_item);
    }
  else
    ctk_tool_item_group_set_item_position (CTK_TOOL_ITEM_GROUP (drop_group),
                                           drag_item, drop_position);
}

static void
palette_drop_group (CtkToolPalette   *palette,
                    CtkToolItemGroup *drag_group,
                    CtkToolItemGroup *drop_group)
{
  gint drop_position = -1;

  if (drop_group)
    drop_position = ctk_tool_palette_get_group_position (palette, drop_group);

  ctk_tool_palette_set_group_position (palette, drag_group, drop_position);
}

static void
palette_drag_data_received (CtkWidget        *widget,
                            CdkDragContext   *context,
                            gint              x,
                            gint              y,
                            CtkSelectionData *selection,
                            guint             info G_GNUC_UNUSED,
                            guint             time G_GNUC_UNUSED,
                            gpointer          data G_GNUC_UNUSED)
{
  CtkAllocation     allocation;
  CtkToolItemGroup *drop_group = NULL;
  CtkWidget        *drag_palette = ctk_drag_get_source_widget (context);
  CtkWidget        *drag_item = NULL;

  while (drag_palette && !CTK_IS_TOOL_PALETTE (drag_palette))
    drag_palette = ctk_widget_get_parent (drag_palette);

  if (drag_palette)
    {
      drag_item = ctk_tool_palette_get_drag_item (CTK_TOOL_PALETTE (drag_palette),
                                                  selection);
      drop_group = ctk_tool_palette_get_drop_group (CTK_TOOL_PALETTE (widget),
                                                    x, y);
    }

  if (CTK_IS_TOOL_ITEM_GROUP (drag_item))
    palette_drop_group (CTK_TOOL_PALETTE (drag_palette),
                        CTK_TOOL_ITEM_GROUP (drag_item),
                        drop_group);
  else if (CTK_IS_TOOL_ITEM (drag_item) && drop_group)
    {
      ctk_widget_get_allocation (CTK_WIDGET (drop_group), &allocation);
      palette_drop_item (CTK_TOOL_ITEM (drag_item),
                         drop_group,
                         x - allocation.x,
                         y - allocation.y);
    }
}

/********************************/
/* ====== Passive Canvas ====== */
/********************************/

static void
passive_canvas_drag_data_received (CtkWidget        *widget,
                                   CdkDragContext   *context,
                                   gint              x,
                                   gint              y,
                                   CtkSelectionData *selection,
                                   guint             info G_GNUC_UNUSED,
                                   guint             time G_GNUC_UNUSED,
                                   gpointer          data G_GNUC_UNUSED)
{
  /* find the tool button, which is the source of this DnD operation */

  CtkWidget *palette = ctk_drag_get_source_widget (context);
  CanvasItem *canvas_item = NULL;
  CtkWidget *tool_item = NULL;

  while (palette && !CTK_IS_TOOL_PALETTE (palette))
    palette = ctk_widget_get_parent (palette);

  if (palette)
    tool_item = ctk_tool_palette_get_drag_item (CTK_TOOL_PALETTE (palette),
                                                selection);

  g_assert (NULL == drop_item);

  /* append a new canvas item when a tool button was found */

  if (CTK_IS_TOOL_ITEM (tool_item))
    canvas_item = canvas_item_new (widget, CTK_TOOL_BUTTON (tool_item), x, y);

  if (canvas_item)
    {
      canvas_items = g_list_append (canvas_items, canvas_item);
      ctk_widget_queue_draw (widget);
    }
}

/************************************/
/* ====== Interactive Canvas ====== */
/************************************/

static gboolean
interactive_canvas_drag_motion (CtkWidget      *widget,
                                CdkDragContext *context,
                                gint            x,
                                gint            y,
                                guint           time,
                                gpointer        data G_GNUC_UNUSED)
{
  if (drop_item)
    {
      /* already have a drop indicator - just update position */

      drop_item->x = x;
      drop_item->y = y;

      ctk_widget_queue_draw (widget);
      cdk_drag_status (context, CDK_ACTION_COPY, time);
    }
  else
    {
      /* request DnD data for creating a drop indicator */

      CdkAtom target = ctk_drag_dest_find_target (widget, context, NULL);

      if (!target)
        return FALSE;

      drag_data_requested_for_drop = FALSE;
      ctk_drag_get_data (widget, context, target, time);
    }

  return TRUE;
}

static void
interactive_canvas_drag_data_received (CtkWidget        *widget,
                                       CdkDragContext   *context,
                                       gint              x,
                                       gint              y,
                                       CtkSelectionData *selection,
                                       guint             info G_GNUC_UNUSED,
                                       guint             time,
                                       gpointer          data G_GNUC_UNUSED)

{
  /* find the tool button which is the source of this DnD operation */

  CtkWidget *palette = ctk_drag_get_source_widget (context);
  CtkWidget *tool_item = NULL;
  CanvasItem *item;

  while (palette && !CTK_IS_TOOL_PALETTE (palette))
    palette = ctk_widget_get_parent (palette);

  if (palette)
    tool_item = ctk_tool_palette_get_drag_item (CTK_TOOL_PALETTE (palette),
                                                selection);

  /* create a canvas item when a tool button was found */

  g_assert (NULL == drop_item);

  if (!CTK_IS_TOOL_ITEM (tool_item))
    return;

  if (drop_item)
    {
      canvas_item_free (drop_item);
      drop_item = NULL;
    }

  item = canvas_item_new (widget, CTK_TOOL_BUTTON (tool_item), x, y);

  /* Either create a new item or just create a preview item, 
     depending on why the drag data was requested. */
  if(drag_data_requested_for_drop)
    {
      canvas_items = g_list_append (canvas_items, item);
      drop_item = NULL;

      ctk_drag_finish (context, TRUE, FALSE, time);
    } else
    {
      drop_item = item;
      cdk_drag_status (context, CDK_ACTION_COPY, time);
    }

  ctk_widget_queue_draw (widget);
}

static gboolean
interactive_canvas_drag_drop (CtkWidget      *widget,
                              CdkDragContext *context,
                              gint            x G_GNUC_UNUSED,
                              gint            y G_GNUC_UNUSED,
                              guint           time,
                              gpointer        data G_GNUC_UNUSED)
{
  CdkAtom target = ctk_drag_dest_find_target (widget, context, NULL);

  if (!target)
    return FALSE;

  drag_data_requested_for_drop = TRUE;
  ctk_drag_get_data (widget, context, target, time);

  return FALSE;
}

static void
interactive_canvas_drag_leave (gpointer data)
{
  if (drop_item)
    {
      CtkWidget *widget = CTK_WIDGET (data);

      canvas_item_free (drop_item);
      drop_item = NULL;

      if (widget)
        ctk_widget_queue_draw (widget);
    }
}

static void
on_combo_orientation_changed (CtkComboBox *combo_box,
                              gpointer     user_data)
{
  CtkToolPalette *palette = CTK_TOOL_PALETTE (user_data);
  CtkScrolledWindow *sw;
  CtkTreeModel *model = ctk_combo_box_get_model (combo_box);
  CtkTreeIter iter;
  gint val = 0;

  sw = CTK_SCROLLED_WINDOW (ctk_widget_get_parent (CTK_WIDGET (palette)));

  if (!ctk_combo_box_get_active_iter (combo_box, &iter))
    return;

  ctk_tree_model_get (model, &iter, 1, &val, -1);

  ctk_orientable_set_orientation (CTK_ORIENTABLE (palette), val);

  if (val == CTK_ORIENTATION_HORIZONTAL)
    ctk_scrolled_window_set_policy (sw, CTK_POLICY_AUTOMATIC, CTK_POLICY_NEVER);
  else
    ctk_scrolled_window_set_policy (sw, CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
}

static void
on_combo_style_changed (CtkComboBox *combo_box,
                        gpointer     user_data)
{
  CtkToolPalette *palette = CTK_TOOL_PALETTE (user_data);
  CtkTreeModel *model = ctk_combo_box_get_model (combo_box);
  CtkTreeIter iter;
  gint val = 0;

  if (!ctk_combo_box_get_active_iter (combo_box, &iter))
    return;

  ctk_tree_model_get (model, &iter, 1, &val, -1);

  if (val == -1)
    ctk_tool_palette_unset_style (palette);
  else
    ctk_tool_palette_set_style (palette, val);
}

CtkWidget *
do_toolpalette (CtkWidget *do_widget)
{
  CtkWidget *box = NULL;
  CtkWidget *hbox = NULL;
  CtkWidget *combo_orientation = NULL;
  CtkListStore *orientation_model = NULL;
  CtkWidget *combo_style = NULL;
  CtkListStore *style_model = NULL;
  CtkCellRenderer *cell_renderer = NULL;
  CtkTreeIter iter;
  CtkWidget *palette = NULL;
  CtkWidget *palette_scroller = NULL;
  CtkWidget *notebook = NULL;
  CtkWidget *contents = NULL;
  CtkWidget *contents_scroller = NULL;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Tool Palette");
      ctk_window_set_default_size (CTK_WINDOW (window), 200, 600);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);
      ctk_container_set_border_width (CTK_CONTAINER (window), 8);

      /* Add widgets to control the ToolPalette appearance: */
      box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
      ctk_container_add (CTK_CONTAINER (window), box);

      /* Orientation combo box: */
      orientation_model = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
      ctk_list_store_append (orientation_model, &iter);
      ctk_list_store_set (orientation_model, &iter,
                          0, "Horizontal",
                          1, CTK_ORIENTATION_HORIZONTAL,
                          -1);
      ctk_list_store_append (orientation_model, &iter);
      ctk_list_store_set (orientation_model, &iter,
                          0, "Vertical",
                          1, CTK_ORIENTATION_VERTICAL,
                          -1);

      combo_orientation =
        ctk_combo_box_new_with_model (CTK_TREE_MODEL (orientation_model));
      cell_renderer = ctk_cell_renderer_text_new ();
      ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo_orientation),
                                  cell_renderer,
                                  TRUE);
      ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo_orientation),
                                      cell_renderer,
                                      "text", 0,
                                      NULL);
      ctk_combo_box_set_active_iter (CTK_COMBO_BOX (combo_orientation), &iter);
      ctk_box_pack_start (CTK_BOX (box), combo_orientation, FALSE, FALSE, 0);

      /* Style combo box: */
      style_model = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
      ctk_list_store_append (style_model, &iter);
      ctk_list_store_set (style_model, &iter,
                          0, "Text",
                          1, CTK_TOOLBAR_TEXT,
                          -1);
      ctk_list_store_append (style_model, &iter);
      ctk_list_store_set (style_model, &iter,
                          0, "Both",
                          1, CTK_TOOLBAR_BOTH,
                          -1);
      ctk_list_store_append (style_model, &iter);
      ctk_list_store_set (style_model, &iter,
                          0, "Both: Horizontal",
                          1, CTK_TOOLBAR_BOTH_HORIZ,
                          -1);
      ctk_list_store_append (style_model, &iter);
      ctk_list_store_set (style_model, &iter,
                          0, "Icons",
                          1, CTK_TOOLBAR_ICONS,
                          -1);
      ctk_list_store_append (style_model, &iter);
      ctk_list_store_set (style_model, &iter,
                          0, "Default",
                          1, -1,  /* A custom meaning for this demo. */
                          -1);
      combo_style = ctk_combo_box_new_with_model (CTK_TREE_MODEL (style_model));
      cell_renderer = ctk_cell_renderer_text_new ();
      ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo_style),
                                  cell_renderer,
                                  TRUE);
      ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo_style),
                                      cell_renderer,
                                      "text", 0,
                                      NULL);
      ctk_combo_box_set_active_iter (CTK_COMBO_BOX (combo_style), &iter);
      ctk_box_pack_start (CTK_BOX (box), combo_style, FALSE, FALSE, 0);

      /* Add hbox */
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
      ctk_box_pack_start (CTK_BOX (box), hbox, TRUE, TRUE, 0);

      /* Add and fill the ToolPalette: */
      palette = ctk_tool_palette_new ();

      load_icon_items (CTK_TOOL_PALETTE (palette));
      load_toggle_items (CTK_TOOL_PALETTE (palette));
      load_special_items (CTK_TOOL_PALETTE (palette));

      palette_scroller = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (palette_scroller),
                                      CTK_POLICY_NEVER,
                                      CTK_POLICY_AUTOMATIC);
      ctk_container_set_border_width (CTK_CONTAINER (palette_scroller), 6);
      ctk_widget_set_hexpand (palette_scroller, TRUE);

      ctk_container_add (CTK_CONTAINER (palette_scroller), palette);
      ctk_container_add (CTK_CONTAINER (hbox), palette_scroller);

      ctk_widget_show_all (box);

      /* Connect signals: */
      g_signal_connect (combo_orientation, "changed",
                        G_CALLBACK (on_combo_orientation_changed), palette);
      g_signal_connect (combo_style, "changed",
                        G_CALLBACK (on_combo_style_changed), palette);

      /* Keep the widgets in sync: */
      on_combo_orientation_changed (CTK_COMBO_BOX (combo_orientation), palette);

      /* ===== notebook ===== */

      notebook = ctk_notebook_new ();
      ctk_container_set_border_width (CTK_CONTAINER (notebook), 6);
      ctk_box_pack_end (CTK_BOX(hbox), notebook, FALSE, FALSE, 0);

      /* ===== DnD for tool items ===== */

      g_signal_connect (palette, "drag-data-received",
                        G_CALLBACK (palette_drag_data_received), NULL);

      ctk_tool_palette_add_drag_dest (CTK_TOOL_PALETTE (palette),
                                      palette,
                                      CTK_DEST_DEFAULT_ALL,
                                      CTK_TOOL_PALETTE_DRAG_ITEMS |
                                      CTK_TOOL_PALETTE_DRAG_GROUPS,
                                      CDK_ACTION_MOVE);

      /* ===== passive DnD dest ===== */

      contents = ctk_drawing_area_new ();
      ctk_widget_set_app_paintable (contents, TRUE);

      g_object_connect (contents,
                        "signal::draw", canvas_draw, NULL,
                        "signal::drag-data-received", passive_canvas_drag_data_received, NULL,
                        NULL);

      ctk_tool_palette_add_drag_dest (CTK_TOOL_PALETTE (palette),
                                      contents,
                                      CTK_DEST_DEFAULT_ALL,
                                      CTK_TOOL_PALETTE_DRAG_ITEMS,
                                      CDK_ACTION_COPY);

      contents_scroller = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (contents_scroller),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_ALWAYS);
      ctk_container_add (CTK_CONTAINER (contents_scroller), contents);
      ctk_container_set_border_width (CTK_CONTAINER (contents_scroller), 6);

      ctk_notebook_append_page (CTK_NOTEBOOK (notebook),
                                contents_scroller,
                                ctk_label_new ("Passive DnD Mode"));

      /* ===== interactive DnD dest ===== */

      contents = ctk_drawing_area_new ();
      ctk_widget_set_app_paintable (contents, TRUE);

      g_object_connect (contents,
                        "signal::draw", canvas_draw, NULL,
                        "signal::drag-motion", interactive_canvas_drag_motion, NULL,
                        "signal::drag-data-received", interactive_canvas_drag_data_received, NULL,
                        "signal::drag-leave", interactive_canvas_drag_leave, contents,
                        "signal::drag-drop", interactive_canvas_drag_drop, NULL,
                        NULL);

      ctk_tool_palette_add_drag_dest (CTK_TOOL_PALETTE (palette),
                                      contents,
                                      CTK_DEST_DEFAULT_HIGHLIGHT,
                                      CTK_TOOL_PALETTE_DRAG_ITEMS,
                                      CDK_ACTION_COPY);

      contents_scroller = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (contents_scroller),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_ALWAYS);
      ctk_container_add (CTK_CONTAINER (contents_scroller), contents);
      ctk_container_set_border_width (CTK_CONTAINER (contents_scroller), 6);

      ctk_notebook_append_page (CTK_NOTEBOOK (notebook), contents_scroller,
                                ctk_label_new ("Interactive DnD Mode"));
    }

  if (!ctk_widget_get_visible (window))
    {
      ctk_widget_show_all (window);
    }
  else
    {
      ctk_widget_destroy (window);
      window = NULL;
    }

  return window;
}


static void
load_icon_items (CtkToolPalette *palette)
{
  GList *contexts;
  GList *l;
  CtkIconTheme *icon_theme;

  icon_theme = ctk_icon_theme_get_for_screen (ctk_widget_get_screen (CTK_WIDGET (palette)));

  contexts = ctk_icon_theme_list_contexts (icon_theme);
  for (l = contexts; l; l = l->next)
    {
      gchar *context = l->data;
      GList *icon_names;
      GList *ll;
      const guint max_icons = 10;
      guint icons_count = 0;

      CtkWidget *group = ctk_tool_item_group_new (context);
      ctk_container_add (CTK_CONTAINER (palette), group);

      if (g_strcmp0 (context, "Animations") == 0)
        continue;

      g_message ("Got context '%s'", context);
      icon_names = ctk_icon_theme_list_icons (icon_theme, context);
      icon_names = g_list_sort (icon_names, (GCompareFunc) strcmp);

      for (ll = icon_names; ll; ll = ll->next)
        {
          CtkToolItem *item;
          gchar *id = ll->data;

          if (g_str_equal (id, "emblem-desktop"))
            continue;

          if (g_str_has_suffix (id, "-symbolic"))
            continue;

          g_message ("Got id '%s'", id);

          item = ctk_tool_button_new (NULL, NULL);
          ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), id);
          ctk_tool_item_set_tooltip_text (CTK_TOOL_ITEM (item), id);
          ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);

          /* Prevent us having an insane number of icons: */
          ++icons_count;
          if(icons_count >= max_icons)
            break;
        }

      g_list_free_full (icon_names, g_free);
    }

  g_list_free_full (contexts, g_free);
}

static void
load_toggle_items (CtkToolPalette *palette)
{
  GSList *toggle_group = NULL;
  CtkWidget *group;
  int i;

  group = ctk_tool_item_group_new ("Radio Item");
  ctk_container_add (CTK_CONTAINER (palette), group);

  for (i = 1; i <= 10; ++i)
    {
      CtkToolItem *item;
      char *label;

      label = g_strdup_printf ("#%d", i);
      item = ctk_radio_tool_button_new (toggle_group);
      ctk_tool_button_set_label (CTK_TOOL_BUTTON (item), label);
      g_free (label);

      ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
      toggle_group = ctk_radio_tool_button_get_group (CTK_RADIO_TOOL_BUTTON (item));
    }
}

static CtkToolItem *
create_entry_item (const char *text)
{
  CtkToolItem *item;
  CtkWidget *entry;

  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), text);
  ctk_entry_set_width_chars (CTK_ENTRY (entry), 5);

  item = ctk_tool_item_new ();
  ctk_container_add (CTK_CONTAINER (item), entry);

  return item;
}

static void
load_special_items (CtkToolPalette *palette)
{
  CtkToolItem *item;
  CtkWidget *group;
  CtkWidget *label_button;

  group = ctk_tool_item_group_new (NULL);
  label_button = ctk_button_new_with_label ("Advanced Features");
  ctk_widget_show (label_button);
  ctk_tool_item_group_set_label_widget (CTK_TOOL_ITEM_GROUP (group),
                                        label_button);
  ctk_container_add (CTK_CONTAINER (palette), group);

  item = create_entry_item ("homogeneous=FALSE");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_container_child_set (CTK_CONTAINER (group), CTK_WIDGET (item),
                           "homogeneous", FALSE, NULL);

  item = create_entry_item ("homogeneous=FALSE, expand=TRUE");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_container_child_set (CTK_CONTAINER (group), CTK_WIDGET (item),
                           "homogeneous", FALSE, "expand", TRUE,
                           NULL);

  item = create_entry_item ("homogeneous=FALSE, expand=TRUE, fill=FALSE");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_container_child_set (CTK_CONTAINER (group), CTK_WIDGET (item),
                           "homogeneous", FALSE, "expand", TRUE,
                           "fill", FALSE, NULL);

  item = create_entry_item ("homogeneous=FALSE, expand=TRUE, new-row=TRUE");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_container_child_set (CTK_CONTAINER (group), CTK_WIDGET (item),
                           "homogeneous", FALSE, "expand", TRUE,
                           "new-row", TRUE, NULL);

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "go-up");
  ctk_tool_item_set_tooltip_text (item, "Show on vertical palettes only");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_tool_item_set_visible_horizontal (item, FALSE);

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "go-next");
  ctk_tool_item_set_tooltip_text (item, "Show on horizontal palettes only");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_tool_item_set_visible_vertical (item, FALSE);

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "edit-delete");
  ctk_tool_item_set_tooltip_text (item, "Do not show at all");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_widget_set_no_show_all (CTK_WIDGET (item), TRUE);

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "view-fullscreen");
  ctk_tool_item_set_tooltip_text (item, "Expanded this item");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
  ctk_container_child_set (CTK_CONTAINER (group), CTK_WIDGET (item),
                           "homogeneous", FALSE,
                           "expand", TRUE,
                           NULL);

  item = ctk_tool_button_new (NULL, NULL);
  ctk_tool_button_set_icon_name (CTK_TOOL_BUTTON (item), "help-browser");
  ctk_tool_item_set_tooltip_text (item, "A regular item");
  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (group), item, -1);
}
