#include <ctk/ctk.h>

static CtkWidget *antialias;
static CtkWidget *subpixel;
static CtkWidget *hintstyle;

static void
set_font_options (CtkWidget *label)
{
  cairo_antialias_t aa;
  cairo_subpixel_order_t sp;
  cairo_hint_style_t hs;
  cairo_font_options_t *options;

  aa = ctk_combo_box_get_active (CTK_COMBO_BOX (antialias));
  sp = ctk_combo_box_get_active (CTK_COMBO_BOX (subpixel));
  hs = ctk_combo_box_get_active (CTK_COMBO_BOX (hintstyle));

  options = cairo_font_options_create ();
  cairo_font_options_set_antialias (options, aa);
  cairo_font_options_set_subpixel_order (options, sp);
  cairo_font_options_set_hint_style (options, hs);

  ctk_widget_set_font_options (label, options);
  cairo_font_options_destroy (options);

  ctk_widget_queue_draw (label);
}

int
main (int   argc G_GNUC_UNUSED,
      char *argv[] G_GNUC_UNUSED)
{
  CtkWidget *window, *label, *grid, *demo;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  grid = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 10);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 10);
  ctk_container_set_border_width (CTK_CONTAINER (grid), 10);
  ctk_container_add (CTK_CONTAINER (window), grid);
  label = ctk_label_new ("Default font options");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 2, 1);
  demo = ctk_label_new ("Custom font options");
  ctk_grid_attach (CTK_GRID (grid), demo, 0, 1, 2, 1);

  antialias = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (antialias), "Default");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (antialias), "None");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (antialias), "Gray");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (antialias), "Subpixel");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (antialias), "Fast");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (antialias), "Good");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (antialias), "Best");
  g_signal_connect_swapped (antialias, "changed", G_CALLBACK (set_font_options), demo);
  label = ctk_label_new ("Antialias");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), antialias, 1, 2, 1, 1);

  subpixel = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (subpixel), "Default");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (subpixel), "RGB");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (subpixel), "BGR");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (subpixel), "Vertical RGB");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (subpixel), "Vertical BGR");
  g_signal_connect_swapped (subpixel, "changed", G_CALLBACK (set_font_options), demo);
  label = ctk_label_new ("Subpixel");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 3, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), subpixel, 1, 3, 1, 1);

  hintstyle = ctk_combo_box_text_new ();
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (hintstyle), "Default");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (hintstyle), "None");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (hintstyle), "Slight");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (hintstyle), "Medium");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (hintstyle), "Full");
  g_signal_connect_swapped (hintstyle, "changed", G_CALLBACK (set_font_options), demo);
  label = ctk_label_new ("Hintstyle");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 4, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), hintstyle, 1, 4, 1, 1);

  ctk_combo_box_set_active (CTK_COMBO_BOX (antialias), 0);
  ctk_combo_box_set_active (CTK_COMBO_BOX (subpixel), 0);
  ctk_combo_box_set_active (CTK_COMBO_BOX (hintstyle), 0);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
