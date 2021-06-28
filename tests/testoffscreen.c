/*
 * testoffscreen.c
 */

#include <math.h>
#include <string.h>
#include <ctk/ctk.h>
#include "ctkoffscreenbox.h"


static void
combo_changed_cb (CtkWidget *combo,
		  gpointer   data)
{
  CtkWidget *label = CTK_WIDGET (data);
  gint active;

  active = ctk_combo_box_get_active (CTK_COMBO_BOX (combo));

  ctk_label_set_ellipsize (CTK_LABEL (label), (PangoEllipsizeMode)active);
}

static gboolean
layout_draw_handler (CtkWidget *widget,
                     cairo_t   *cr)
{
  CtkLayout *layout = CTK_LAYOUT (widget);
  CdkWindow *bin_window = ctk_layout_get_bin_window (layout);
  CdkRectangle clip;

  gint i, j, x, y;
  gint imin, imax, jmin, jmax;

  if (!ctk_cairo_should_draw_window (cr, bin_window))
    return FALSE;

  cdk_window_get_position (bin_window, &x, &y);
  cairo_translate (cr, x, y);

  cdk_cairo_get_clip_rectangle (cr, &clip);

  imin = (clip.x) / 10;
  imax = (clip.x + clip.width + 9) / 10;

  jmin = (clip.y) / 10;
  jmax = (clip.y + clip.height + 9) / 10;

  for (i = imin; i < imax; i++)
    for (j = jmin; j < jmax; j++)
      if ((i + j) % 2)
          cairo_rectangle (cr,
                           10 * i, 10 * j,
                           1 + i % 10, 1 + j % 10);

  cairo_fill (cr);

  return FALSE;
}

static gboolean
scroll_layout (gpointer data)
{
  CtkWidget *layout = data;
  CtkAdjustment *adj;

  adj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (layout));
  ctk_adjustment_set_value (adj, ctk_adjustment_get_value (adj) + 5.0);
  return G_SOURCE_CONTINUE;
}

static guint layout_timeout;

static void
create_layout (CtkWidget *vbox)
{
  CtkAdjustment *hadjustment, *vadjustment;
  CtkLayout *layout;
  CtkWidget *layout_widget;
  CtkWidget *scrolledwindow;
  CtkWidget *button;
  gchar buf[16];
  gint i, j;

  scrolledwindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolledwindow),
				       CTK_SHADOW_IN);
  ctk_scrolled_window_set_placement (CTK_SCROLLED_WINDOW (scrolledwindow),
				     CTK_CORNER_TOP_RIGHT);

  ctk_box_pack_start (CTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

  layout_widget = ctk_layout_new (NULL, NULL);
  layout = CTK_LAYOUT (layout_widget);
  ctk_container_add (CTK_CONTAINER (scrolledwindow), layout_widget);

  /* We set step sizes here since CtkLayout does not set
   * them itself.
   */
  hadjustment = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (layout));
  vadjustment = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (layout));
  ctk_adjustment_set_step_increment (hadjustment, 10.0);
  ctk_adjustment_set_step_increment (vadjustment, 10.0);
  ctk_scrollable_set_hadjustment (CTK_SCROLLABLE (layout), hadjustment);
  ctk_scrollable_set_vadjustment (CTK_SCROLLABLE (layout), vadjustment);

  ctk_widget_set_events (layout_widget, GDK_EXPOSURE_MASK);
  g_signal_connect (layout, "draw",
		    G_CALLBACK (layout_draw_handler),
                    NULL);

  ctk_layout_set_size (layout, 1600, 128000);

  for (i = 0 ; i < 16 ; i++)
    for (j = 0 ; j < 16 ; j++)
      {
	g_snprintf (buf, sizeof (buf), "Button %d, %d", i, j);

	if ((i + j) % 2)
	  button = ctk_button_new_with_label (buf);
	else
	  button = ctk_label_new (buf);

	ctk_layout_put (layout, button,	j * 100, i * 100);
      }

  for (i = 16; i < 1280; i++)
    {
      g_snprintf (buf, sizeof (buf), "Button %d, %d", i, 0);

      if (i % 2)
	button = ctk_button_new_with_label (buf);
      else
	button = ctk_label_new (buf);

      ctk_layout_put (layout, button, 0, i * 100);
    }

  layout_timeout = g_timeout_add (1000, scroll_layout, layout);
}

static void
create_treeview (CtkWidget *vbox)
{
  CtkWidget *scrolledwindow;
  CtkListStore *store;
  CtkWidget *tree_view;
  CtkIconTheme *icon_theme;
  GList *icon_names;
  GList *list;

  scrolledwindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolledwindow),
				       CTK_SHADOW_IN);

  ctk_box_pack_start (CTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

  store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (store));
  g_object_unref (store);

  ctk_container_add (CTK_CONTAINER (scrolledwindow), tree_view);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view), -1,
                                               "Icon",
                                               ctk_cell_renderer_pixbuf_new (),
                                               "icon-name", 0,
                                               NULL);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view), -1,
                                               "Label",
                                               ctk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);

  icon_theme = ctk_icon_theme_get_for_screen (ctk_widget_get_screen (vbox));
  icon_names = ctk_icon_theme_list_icons (icon_theme, NULL);
  icon_names = g_list_sort (icon_names, (GCompareFunc) strcmp);

  for (list = icon_names; list; list = g_list_next (list))
    {
      const gchar *name = list->data;

      ctk_list_store_insert_with_values (store, NULL, -1,
                                         0, name,
                                         1, name,
                                         -1);
    }

  g_list_free_full (icon_names, g_free);
}

static CtkWidget *
create_widgets (void)
{
  CtkWidget *main_hbox, *main_vbox;
  CtkWidget *vbox, *hbox, *label, *combo, *entry, *button, *cb;
  CtkWidget *sw, *text_view;

  main_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  main_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 0);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_box_pack_start (CTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = ctk_label_new ("This label may be ellipsized\nto make it fit.");
  ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);

  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "NONE");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "START");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "MIDDLE");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "END");
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);
  ctk_box_pack_start (CTK_BOX (hbox), combo, TRUE, TRUE, 0);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (combo_changed_cb),
                    label);

  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "an entry - lots of text.... lots of text.... lots of text.... lots of text.... ");
  ctk_box_pack_start (CTK_BOX (vbox), entry, FALSE, FALSE, 0);

  label = ctk_label_new ("Label after entry.");
  ctk_label_set_selectable (CTK_LABEL (label), TRUE);
  ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Button");
  ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);

  button = ctk_check_button_new_with_mnemonic ("_Check button");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  cb = ctk_combo_box_text_new ();
  entry = ctk_entry_new ();
  ctk_widget_show (entry);
  ctk_container_add (CTK_CONTAINER (cb), entry);

  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (cb), "item0");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (cb), "item1");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (cb), "item1");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (cb), "item2");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (cb), "item2");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (cb), "item2");
  ctk_entry_set_text (CTK_ENTRY (entry), "hello world ♥ foo");
  ctk_editable_select_region (CTK_EDITABLE (entry), 0, -1);
  ctk_box_pack_start (CTK_BOX (vbox), cb, TRUE, TRUE, 0);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
				  CTK_POLICY_AUTOMATIC,
				  CTK_POLICY_AUTOMATIC);
  text_view = ctk_text_view_new ();
  ctk_box_pack_start (CTK_BOX (vbox), sw, TRUE, TRUE, 0);
  ctk_container_add (CTK_CONTAINER (sw), text_view);

  create_layout (vbox);

  create_treeview (main_hbox);

  return main_vbox;
}


static void
scale_changed (CtkRange        *range,
	       CtkOffscreenBox *offscreen_box)
{
  ctk_offscreen_box_set_angle (offscreen_box, ctk_range_get_value (range));
}

static CtkWidget *scale = NULL;

static void
remove_clicked (CtkButton *button,
                CtkWidget *widget)
{
  ctk_widget_destroy (widget);
  g_source_remove (layout_timeout);

  ctk_widget_set_sensitive (CTK_WIDGET (button), FALSE);
  ctk_widget_set_sensitive (scale, FALSE);
}

int
main (int   argc,
      char *argv[])
{
  CtkWidget *window, *widget, *vbox, *button;
  CtkWidget *offscreen = NULL;
  gboolean use_offscreen;

  ctk_init (&argc, &argv);

  use_offscreen = argc == 1;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 300,300);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (ctk_main_quit),
                    NULL);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL,
                                    0, G_PI * 2, 0.01);
  ctk_box_pack_start (CTK_BOX (vbox), scale, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Remove child 2");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  if (use_offscreen)
    {
      offscreen = ctk_offscreen_box_new ();

      g_signal_connect (scale, "value_changed",
                        G_CALLBACK (scale_changed),
                        offscreen);
    }
  else
    {
      offscreen = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
    }

  ctk_box_pack_start (CTK_BOX (vbox), offscreen, TRUE, TRUE, 0);

  widget = create_widgets ();
  if (use_offscreen)
    ctk_offscreen_box_add1 (CTK_OFFSCREEN_BOX (offscreen),
			    widget);
  else
    ctk_paned_add1 (CTK_PANED (offscreen), widget);

  widget = create_widgets ();
  if (1)
    {
      CtkWidget *widget2, *box2, *offscreen2;

      offscreen2 = ctk_offscreen_box_new ();
      ctk_box_pack_start (CTK_BOX (widget), offscreen2, FALSE, FALSE, 0);

      g_signal_connect (scale, "value_changed",
                        G_CALLBACK (scale_changed),
                        offscreen2);

      box2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_offscreen_box_add2 (CTK_OFFSCREEN_BOX (offscreen2), box2);

      widget2 = ctk_button_new_with_label ("Offscreen in offscreen");
      ctk_box_pack_start (CTK_BOX (box2), widget2, FALSE, FALSE, 0);

      widget2 = ctk_entry_new ();
      ctk_entry_set_text (CTK_ENTRY (widget2), "Offscreen in offscreen");
      ctk_box_pack_start (CTK_BOX (box2), widget2, FALSE, FALSE, 0);
    }

  if (use_offscreen)
    ctk_offscreen_box_add2 (CTK_OFFSCREEN_BOX (offscreen), widget);
  else
    ctk_paned_add2 (CTK_PANED (offscreen), widget);

  ctk_widget_show_all (window);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (remove_clicked),
                    widget);

  ctk_main ();

  return 0;
}
