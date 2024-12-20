/* testtooltips.c: Test application for CTK+ >= 2.12 tooltips code
 *
 * Copyright (C) 2006-2007  Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <ctk/ctk.h>

static gboolean
query_tooltip_cb (CtkWidget  *widget,
		  gint        x G_GNUC_UNUSED,
		  gint        y G_GNUC_UNUSED,
		  gboolean    keyboard_tip G_GNUC_UNUSED,
		  CtkTooltip *tooltip,
		  gpointer    data G_GNUC_UNUSED)
{
  ctk_tooltip_set_markup (tooltip, ctk_button_get_label (CTK_BUTTON (widget)));
  ctk_tooltip_set_icon_from_icon_name (tooltip, "edit-delete",
                                       CTK_ICON_SIZE_MENU);

  return TRUE;
}

static gboolean
draw_tooltip (CtkWidget *widget G_GNUC_UNUSED,
              cairo_t   *cr,
              gpointer   unused G_GNUC_UNUSED)
{
  cairo_set_source_rgb (cr, 0, 0, 1);
  cairo_paint (cr);

  return FALSE;
}

static gboolean
query_tooltip_custom_cb (CtkWidget  *widget,
			 gint        x G_GNUC_UNUSED,
			 gint        y G_GNUC_UNUSED,
			 gboolean    keyboard_tip G_GNUC_UNUSED,
			 CtkTooltip *tooltip G_GNUC_UNUSED,
			 gpointer    data G_GNUC_UNUSED)
{
  CtkWindow *window = ctk_widget_get_tooltip_window (widget);

  ctk_widget_set_app_paintable (CTK_WIDGET (window), TRUE);
  g_signal_connect (window, "draw", G_CALLBACK (draw_tooltip), NULL);

  return TRUE;
}

static gboolean
query_tooltip_text_view_cb (CtkWidget  *widget,
			    gint        x,
			    gint        y,
			    gboolean    keyboard_tip,
			    CtkTooltip *tooltip,
			    gpointer    data)
{
  CtkTextTag *tag = data;
  CtkTextIter iter;
  CtkTextView *text_view = CTK_TEXT_VIEW (widget);
  CtkTextBuffer *buffer = ctk_text_view_get_buffer (text_view);

  if (keyboard_tip)
    {
      gint offset;

      g_object_get (buffer, "cursor-position", &offset, NULL);
      ctk_text_buffer_get_iter_at_offset (buffer, &iter, offset);
    }
  else
    {
      gint bx, by, trailing;

      ctk_text_view_window_to_buffer_coords (text_view, CTK_TEXT_WINDOW_TEXT,
					     x, y, &bx, &by);
      ctk_text_view_get_iter_at_position (text_view, &iter, &trailing, bx, by);
    }

  if (ctk_text_iter_has_tag (&iter, tag))
    ctk_tooltip_set_text (tooltip, "Tooltip on text tag");
  else
   return FALSE;

  return TRUE;
}

static gboolean
query_tooltip_tree_view_cb (CtkWidget  *widget,
			    gint        x,
			    gint        y,
			    gboolean    keyboard_tip,
			    CtkTooltip *tooltip,
			    gpointer    data G_GNUC_UNUSED)
{
  CtkTreeIter iter;
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CtkTreeModel *model = ctk_tree_view_get_model (tree_view);
  CtkTreePath *path = NULL;
  gchar *tmp;
  gchar *pathstring;

  char buffer[512];

  if (!ctk_tree_view_get_tooltip_context (tree_view, &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  ctk_tree_model_get (model, &iter, 0, &tmp, -1);
  pathstring = ctk_tree_path_to_string (path);

  g_snprintf (buffer, 511, "<b>Path %s:</b> %s", pathstring, tmp);
  ctk_tooltip_set_markup (tooltip, buffer);

  ctk_tree_view_set_tooltip_row (tree_view, tooltip, path);

  ctk_tree_path_free (path);
  g_free (pathstring);
  g_free (tmp);

  return TRUE;
}

static CtkTreeModel *
create_model (void)
{
  CtkTreeStore *store;
  CtkTreeIter iter;

  store = ctk_tree_store_new (1, G_TYPE_STRING);

  /* A tree store with some random words ... */
  ctk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "File Manager", -1);
  ctk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Gossip", -1);
  ctk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "System Settings", -1);
  ctk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "The GIMP", -1);
  ctk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Terminal", -1);
  ctk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Word Processor", -1);

  return CTK_TREE_MODEL (store);
}

static void
selection_changed_cb (CtkTreeSelection *selection G_GNUC_UNUSED,
		      CtkWidget        *tree_view)
{
  ctk_widget_trigger_tooltip_query (tree_view);
}

static struct Rectangle
{
  gint x;
  gint y;
  gfloat r;
  gfloat g;
  gfloat b;
  const char *tooltip;
}
rectangles[] =
{
  { 10, 10, 0.0, 0.0, 0.9, "Blue box!" },
  { 200, 170, 1.0, 0.0, 0.0, "Red thing" },
  { 100, 50, 0.8, 0.8, 0.0, "Yellow thing" }
};

static gboolean
query_tooltip_drawing_area_cb (CtkWidget  *widget G_GNUC_UNUSED,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_tip,
			       CtkTooltip *tooltip,
			       gpointer    data G_GNUC_UNUSED)
{
  gint i;

  if (keyboard_tip)
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (rectangles); i++)
    {
      struct Rectangle *r = &rectangles[i];

      if (r->x < x && x < r->x + 50
	  && r->y < y && y < r->y + 50)
        {
	  ctk_tooltip_set_markup (tooltip, r->tooltip);
	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
drawing_area_draw (CtkWidget *drawing_area G_GNUC_UNUSED,
		   cairo_t   *cr,
		   gpointer   data G_GNUC_UNUSED)
{
  gint i;

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

  for (i = 0; i < G_N_ELEMENTS (rectangles); i++)
    {
      struct Rectangle *r = &rectangles[i];

      cairo_rectangle (cr, r->x, r->y, 50, 50);
      cairo_set_source_rgb (cr, r->r, r->g, r->b);
      cairo_stroke (cr);

      cairo_rectangle (cr, r->x, r->y, 50, 50);
      cairo_set_source_rgba (cr, r->r, r->g, r->b, 0.5);
      cairo_fill (cr);
    }

  return FALSE;
}

static gboolean
query_tooltip_label_cb (CtkWidget  *widget G_GNUC_UNUSED,
			gint        x G_GNUC_UNUSED,
			gint        y G_GNUC_UNUSED,
			gboolean    keyboard_tip G_GNUC_UNUSED,
			CtkTooltip *tooltip,
			gpointer    data)
{
  CtkWidget *custom = data;

  ctk_tooltip_set_custom (tooltip, custom);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *drawing_area;
  CtkWidget *button;
  CtkWidget *label;

  CtkWidget *tooltip_window;
  CtkWidget *tooltip_button;

  CtkWidget *tree_view;
  CtkTreeViewColumn *column;

  CtkWidget *text_view;
  CtkTextBuffer *buffer;
  CtkTextIter iter;
  CtkTextTag *tag;

  gchar *text, *markup;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Tooltips test");
  ctk_container_set_border_width (CTK_CONTAINER (window), 10);
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (ctk_main_quit), NULL);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 3);
  ctk_container_add (CTK_CONTAINER (window), box);

  /* A check button using the tooltip-markup property */
  button = ctk_check_button_new_with_label ("This one uses the tooltip-markup property");
  ctk_widget_set_tooltip_text (button, "Hello, I am a static tooltip.");
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);

  text = ctk_widget_get_tooltip_text (button);
  markup = ctk_widget_get_tooltip_markup (button);
  g_assert (g_str_equal ("Hello, I am a static tooltip.", text));
  g_assert (g_str_equal ("Hello, I am a static tooltip.", markup));
  g_free (text); g_free (markup);

  /* A check button using the query-tooltip signal */
  button = ctk_check_button_new_with_label ("I use the query-tooltip signal");
  g_object_set (button, "has-tooltip", TRUE, NULL);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_cb), NULL);
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);

  /* A label */
  button = ctk_label_new ("I am just a label");
  ctk_label_set_selectable (CTK_LABEL (button), FALSE);
  ctk_widget_set_tooltip_text (button, "Label & and tooltip");
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);

  text = ctk_widget_get_tooltip_text (button);
  markup = ctk_widget_get_tooltip_markup (button);
  g_assert (g_str_equal ("Label & and tooltip", text));
  g_assert (g_str_equal ("Label &amp; and tooltip", markup));
  g_free (text); g_free (markup);

  /* A selectable label */
  button = ctk_label_new ("I am a selectable label");
  ctk_label_set_selectable (CTK_LABEL (button), TRUE);
  ctk_widget_set_tooltip_markup (button, "<b>Another</b> Label tooltip");
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);

  text = ctk_widget_get_tooltip_text (button);
  markup = ctk_widget_get_tooltip_markup (button);
  g_assert (g_str_equal ("Another Label tooltip", text));
  g_assert (g_str_equal ("<b>Another</b> Label tooltip", markup));
  g_free (text); g_free (markup);

  /* Another one, with a custom tooltip window */
  button = ctk_check_button_new_with_label ("This one has a custom tooltip window!");
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);

  tooltip_window = ctk_window_new (CTK_WINDOW_POPUP);
  tooltip_button = ctk_label_new ("blaat!");
  ctk_container_add (CTK_CONTAINER (tooltip_window), tooltip_button);
  ctk_widget_show (tooltip_button);

  ctk_widget_set_tooltip_window (button, CTK_WINDOW (tooltip_window));
  ctk_window_set_type_hint (CTK_WINDOW (tooltip_window),
                            CDK_WINDOW_TYPE_HINT_TOOLTIP);
  ctk_window_set_transient_for (CTK_WINDOW (tooltip_window),
                                CTK_WINDOW (window));

  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_custom_cb), NULL);
  g_object_set (button, "has-tooltip", TRUE, NULL);

  /* An insensitive button */
  button = ctk_button_new_with_label ("This one is insensitive");
  ctk_widget_set_sensitive (button, FALSE);
  g_object_set (button, "tooltip-text", "Insensitive!", NULL);
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);

  /* Testcases from Kris without a tree view don't exist. */
  tree_view = ctk_tree_view_new_with_model (create_model ());
  ctk_widget_set_size_request (tree_view, 200, 240);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       0, "Test",
					       ctk_cell_renderer_text_new (),
					       "text", 0,
					       NULL);

  g_object_set (tree_view, "has-tooltip", TRUE, NULL);
  g_signal_connect (tree_view, "query-tooltip",
		    G_CALLBACK (query_tooltip_tree_view_cb), NULL);
  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view)),
		    "changed", G_CALLBACK (selection_changed_cb), tree_view);

  /* Set a tooltip on the column */
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), 0);
  ctk_tree_view_column_set_clickable (column, TRUE);
  g_object_set (ctk_tree_view_column_get_button (column), "tooltip-text", "Header", NULL);

  ctk_box_pack_start (CTK_BOX (box), tree_view, FALSE, FALSE, 2);

  /* And a text view for Matthias */
  buffer = ctk_text_buffer_new (NULL);

  ctk_text_buffer_get_end_iter (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, "Hello, the text ", -1);

  tag = ctk_text_buffer_create_tag (buffer, "bold", NULL);
  g_object_set (tag, "weight", PANGO_WEIGHT_BOLD, NULL);

  ctk_text_buffer_get_end_iter (buffer, &iter);
  ctk_text_buffer_insert_with_tags (buffer, &iter, "in bold", -1, tag, NULL);

  ctk_text_buffer_get_end_iter (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, " has a tooltip!", -1);

  text_view = ctk_text_view_new_with_buffer (buffer);
  ctk_widget_set_size_request (text_view, 200, 50);

  g_object_set (text_view, "has-tooltip", TRUE, NULL);
  g_signal_connect (text_view, "query-tooltip",
		    G_CALLBACK (query_tooltip_text_view_cb), tag);

  ctk_box_pack_start (CTK_BOX (box), text_view, FALSE, FALSE, 2);

  /* Drawing area */
  drawing_area = ctk_drawing_area_new ();
  ctk_widget_set_size_request (drawing_area, 320, 240);
  g_object_set (drawing_area, "has-tooltip", TRUE, NULL);
  g_signal_connect (drawing_area, "draw",
		    G_CALLBACK (drawing_area_draw), NULL);
  g_signal_connect (drawing_area, "query-tooltip",
		    G_CALLBACK (query_tooltip_drawing_area_cb), NULL);
  ctk_box_pack_start (CTK_BOX (box), drawing_area, FALSE, FALSE, 2);

  button = ctk_label_new ("Custom tooltip I");
  label = ctk_label_new ("See, custom");
  g_object_ref_sink (label);
  g_object_set (button, "has-tooltip", TRUE, NULL);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_label_cb), label);
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 2);

  button = ctk_label_new ("Custom tooltip II");
  label = ctk_label_new ("See, custom, too");
  g_object_ref_sink (label);
  g_object_set (button, "has-tooltip", TRUE, NULL);
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 2);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_label_cb), label);

  /* Done! */
  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
