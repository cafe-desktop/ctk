/*
 * testoffscreen.c
 */

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtkoffscreenbox.h"


static void
combo_changed_cb (GtkWidget *combo,
		  gpointer   data)
{
  GtkWidget *label = GTK_WIDGET (data);
  gint active;

  active = ctk_combo_box_get_active (GTK_COMBO_BOX (combo));

  ctk_label_set_ellipsize (GTK_LABEL (label), (PangoEllipsizeMode)active);
}

static gboolean
layout_draw_handler (GtkWidget *widget,
                     cairo_t   *cr)
{
  GtkLayout *layout = GTK_LAYOUT (widget);
  GdkWindow *bin_window = ctk_layout_get_bin_window (layout);
  GdkRectangle clip;

  gint i, j, x, y;
  gint imin, imax, jmin, jmax;

  if (!ctk_cairo_should_draw_window (cr, bin_window))
    return FALSE;

  gdk_window_get_position (bin_window, &x, &y);
  cairo_translate (cr, x, y);

  gdk_cairo_get_clip_rectangle (cr, &clip);

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
  GtkWidget *layout = data;
  GtkAdjustment *adj;

  adj = ctk_scrollable_get_hadjustment (GTK_SCROLLABLE (layout));
  ctk_adjustment_set_value (adj, ctk_adjustment_get_value (adj) + 5.0);
  return G_SOURCE_CONTINUE;
}

static guint layout_timeout;

static void
create_layout (GtkWidget *vbox)
{
  GtkAdjustment *hadjustment, *vadjustment;
  GtkLayout *layout;
  GtkWidget *layout_widget;
  GtkWidget *scrolledwindow;
  GtkWidget *button;
  gchar buf[16];
  gint i, j;

  scrolledwindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_IN);
  ctk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolledwindow),
				     GTK_CORNER_TOP_RIGHT);

  ctk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

  layout_widget = ctk_layout_new (NULL, NULL);
  layout = GTK_LAYOUT (layout_widget);
  ctk_container_add (GTK_CONTAINER (scrolledwindow), layout_widget);

  /* We set step sizes here since GtkLayout does not set
   * them itself.
   */
  hadjustment = ctk_scrollable_get_hadjustment (GTK_SCROLLABLE (layout));
  vadjustment = ctk_scrollable_get_vadjustment (GTK_SCROLLABLE (layout));
  ctk_adjustment_set_step_increment (hadjustment, 10.0);
  ctk_adjustment_set_step_increment (vadjustment, 10.0);
  ctk_scrollable_set_hadjustment (GTK_SCROLLABLE (layout), hadjustment);
  ctk_scrollable_set_vadjustment (GTK_SCROLLABLE (layout), vadjustment);

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
create_treeview (GtkWidget *vbox)
{
  GtkWidget *scrolledwindow;
  GtkListStore *store;
  GtkWidget *tree_view;
  GtkIconTheme *icon_theme;
  GList *icon_names;
  GList *list;

  scrolledwindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_IN);

  ctk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

  store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  tree_view = ctk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  ctk_container_add (GTK_CONTAINER (scrolledwindow), tree_view);

  ctk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view), -1,
                                               "Icon",
                                               ctk_cell_renderer_pixbuf_new (),
                                               "icon-name", 0,
                                               NULL);
  ctk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view), -1,
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

static GtkWidget *
create_widgets (void)
{
  GtkWidget *main_hbox, *main_vbox;
  GtkWidget *vbox, *hbox, *label, *combo, *entry, *button, *cb;
  GtkWidget *sw, *text_view;

  main_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  main_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (GTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 0);

  vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  ctk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);

  hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = ctk_label_new ("This label may be ellipsized\nto make it fit.");
  ctk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "NONE");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "START");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "MIDDLE");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "END");
  ctk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  ctk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (combo_changed_cb),
                    label);

  entry = ctk_entry_new ();
  ctk_entry_set_text (GTK_ENTRY (entry), "an entry - lots of text.... lots of text.... lots of text.... lots of text.... ");
  ctk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  label = ctk_label_new ("Label after entry.");
  ctk_label_set_selectable (GTK_LABEL (label), TRUE);
  ctk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Button");
  ctk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);

  button = ctk_check_button_new_with_mnemonic ("_Check button");
  ctk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  cb = ctk_combo_box_text_new ();
  entry = ctk_entry_new ();
  ctk_widget_show (entry);
  ctk_container_add (GTK_CONTAINER (cb), entry);

  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb), "item0");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb), "item1");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb), "item1");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb), "item2");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb), "item2");
  ctk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb), "item2");
  ctk_entry_set_text (GTK_ENTRY (entry), "hello world ♥ foo");
  ctk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
  ctk_box_pack_start (GTK_BOX (vbox), cb, TRUE, TRUE, 0);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  text_view = ctk_text_view_new ();
  ctk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  ctk_container_add (GTK_CONTAINER (sw), text_view);

  create_layout (vbox);

  create_treeview (main_hbox);

  return main_vbox;
}


static void
scale_changed (GtkRange        *range,
	       GtkOffscreenBox *offscreen_box)
{
  ctk_offscreen_box_set_angle (offscreen_box, ctk_range_get_value (range));
}

static GtkWidget *scale = NULL;

static void
remove_clicked (GtkButton *button,
                GtkWidget *widget)
{
  ctk_widget_destroy (widget);
  g_source_remove (layout_timeout);

  ctk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
  ctk_widget_set_sensitive (scale, FALSE);
}

int
main (int   argc,
      char *argv[])
{
  GtkWidget *window, *widget, *vbox, *button;
  GtkWidget *offscreen = NULL;
  gboolean use_offscreen;

  ctk_init (&argc, &argv);

  use_offscreen = argc == 1;

  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (GTK_WINDOW (window), 300,300);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (ctk_main_quit),
                    NULL);

  vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (GTK_CONTAINER (window), vbox);

  scale = ctk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
                                    0, G_PI * 2, 0.01);
  ctk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Remove child 2");
  ctk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  if (use_offscreen)
    {
      offscreen = ctk_offscreen_box_new ();

      g_signal_connect (scale, "value_changed",
                        G_CALLBACK (scale_changed),
                        offscreen);
    }
  else
    {
      offscreen = ctk_paned_new (GTK_ORIENTATION_VERTICAL);
    }

  ctk_box_pack_start (GTK_BOX (vbox), offscreen, TRUE, TRUE, 0);

  widget = create_widgets ();
  if (use_offscreen)
    ctk_offscreen_box_add1 (GTK_OFFSCREEN_BOX (offscreen),
			    widget);
  else
    ctk_paned_add1 (GTK_PANED (offscreen), widget);

  widget = create_widgets ();
  if (1)
    {
      GtkWidget *widget2, *box2, *offscreen2;

      offscreen2 = ctk_offscreen_box_new ();
      ctk_box_pack_start (GTK_BOX (widget), offscreen2, FALSE, FALSE, 0);

      g_signal_connect (scale, "value_changed",
                        G_CALLBACK (scale_changed),
                        offscreen2);

      box2 = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      ctk_offscreen_box_add2 (GTK_OFFSCREEN_BOX (offscreen2), box2);

      widget2 = ctk_button_new_with_label ("Offscreen in offscreen");
      ctk_box_pack_start (GTK_BOX (box2), widget2, FALSE, FALSE, 0);

      widget2 = ctk_entry_new ();
      ctk_entry_set_text (GTK_ENTRY (widget2), "Offscreen in offscreen");
      ctk_box_pack_start (GTK_BOX (box2), widget2, FALSE, FALSE, 0);
    }

  if (use_offscreen)
    ctk_offscreen_box_add2 (GTK_OFFSCREEN_BOX (offscreen), widget);
  else
    ctk_paned_add2 (GTK_PANED (offscreen), widget);

  ctk_widget_show_all (window);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (remove_clicked),
                    widget);

  ctk_main ();

  return 0;
}
