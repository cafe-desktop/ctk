#include <string.h>
#include <ctk/ctk.h>

static gboolean
draw_cb_checks (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "check");
  ctk_style_context_set_state (context, 0);
  ctk_render_check (context, cr, 12, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_ACTIVE);
  ctk_render_check (context, cr, 36, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_INCONSISTENT);
  ctk_render_check (context, cr, 60, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_INSENSITIVE);
  ctk_render_check (context, cr, 84, 12, 12, 12);

  ctk_style_context_restore (context);

  return TRUE;
}


static gboolean
draw_cb_options (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "radio");
  ctk_style_context_set_state (context, 0);
  ctk_render_option (context, cr, 12, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_ACTIVE);
  ctk_render_option (context, cr, 36, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_INCONSISTENT);
  ctk_render_option (context, cr, 60, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_INSENSITIVE);
  ctk_render_option (context, cr, 84, 12, 12, 12);

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_arrows (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_set_state (context, 0);
  ctk_render_arrow (context, cr, 0,        12, 12, 12);
  ctk_render_arrow (context, cr, G_PI/2,   36, 12, 12);
  ctk_render_arrow (context, cr, G_PI,     60, 12, 12);
  ctk_render_arrow (context, cr, G_PI*3/2, 84, 12, 12);

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_expanders (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "expander");
  ctk_style_context_set_state (context, 0);
  ctk_render_expander (context, cr, 12, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_PRELIGHT);
  ctk_render_expander (context, cr, 36, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_ACTIVE);
  ctk_render_expander (context, cr, 60, 12, 12, 12);
  ctk_style_context_set_state (context, CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_ACTIVE);
  ctk_render_expander (context, cr, 84, 12, 12, 12);

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_background (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "background");
  ctk_style_context_set_junction_sides (context, 0);
  ctk_render_background (context, cr, 12, 12, 100, 100);
  ctk_style_context_remove_class (context, "background");

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_frame (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "frame1");
  ctk_style_context_set_junction_sides (context, 0);
  ctk_render_frame (context, cr, 12, 12, 50, 50);
  ctk_style_context_remove_class (context, "frame1");

  ctk_style_context_add_class (context, "frame2");
  ctk_render_frame (context, cr, 74, 12, 50, 50);
  ctk_style_context_remove_class (context, "frame2");

  ctk_style_context_add_class (context, "frame3");
  ctk_style_context_set_junction_sides (context, CTK_JUNCTION_RIGHT);
  ctk_render_frame (context, cr, 12, 74, 56, 50);
  ctk_style_context_set_junction_sides (context, CTK_JUNCTION_LEFT);
  ctk_render_frame (context, cr, 68, 74, 56, 50);
  ctk_style_context_remove_class (context, "frame3");

  ctk_style_context_restore (context);

  return TRUE;
}

/* FIXME: this doesn't work */
static gboolean
draw_cb_activity (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;
  CtkWidgetPath *path;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save (context);

  path = ctk_widget_path_new ();
  ctk_widget_path_append_type (path, CTK_TYPE_SPINNER);
  ctk_widget_path_iter_add_class (path, 0, "spinner");
  ctk_style_context_set_path (context, path);
  ctk_widget_path_free (path);

  ctk_style_context_set_state (context, CTK_STATE_FLAG_ACTIVE);
  ctk_render_activity (context, cr, 12, 12, 12, 12);

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_slider (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;
  CtkWidgetPath *path;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save (context);

  path = ctk_widget_path_new ();
  ctk_widget_path_append_type (path, CTK_TYPE_SCALE);
  ctk_widget_path_iter_add_class (path, 0, "slider");
  ctk_widget_path_iter_add_class (path, 0, "scale");
  ctk_style_context_set_path (context, path);
  ctk_widget_path_free (path);

  ctk_render_slider (context, cr, 12, 22, 30, 10, CTK_ORIENTATION_HORIZONTAL);
  ctk_render_slider (context, cr, 54, 12, 10, 30, CTK_ORIENTATION_VERTICAL);

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_focus (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_render_focus (context, cr, 12, 12, 50, 50);

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_extension (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "notebook");
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_style_context_add_region (context, CTK_STYLE_REGION_TAB, 0);
G_GNUC_END_IGNORE_DEPRECATIONS

  ctk_style_context_set_state (context, 0);
  ctk_render_extension (context, cr, 26, 12, 24, 12, CTK_POS_BOTTOM);
  ctk_render_extension (context, cr, 12, 26, 12, 24, CTK_POS_RIGHT);
  ctk_render_extension (context, cr, 26, 52, 24, 12, CTK_POS_TOP);
  ctk_render_extension (context, cr, 52, 26, 12, 24, CTK_POS_LEFT);

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_frame_gap (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "frame");
  ctk_style_context_set_junction_sides (context, 0);
  ctk_render_frame_gap (context, cr, 12, 12, 50, 50, CTK_POS_TOP, 15, 35);
  ctk_style_context_remove_class (context, "frame");

  ctk_style_context_restore (context);

  return TRUE;
}

static gboolean
draw_cb_handles (CtkWidget *widget, cairo_t *cr)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save (context);

  ctk_style_context_add_class (context, "paned");
  ctk_render_handle (context, cr, 12, 22, 20, 10);
  ctk_render_handle (context, cr, 44, 12, 10, 20);
  ctk_style_context_remove_class (context, "paned");

  ctk_style_context_add_class (context, "grip");
  ctk_style_context_set_junction_sides (context, CTK_JUNCTION_CORNER_BOTTOMLEFT);
  ctk_render_handle (context, cr, 12, 48, 12, 12);

  ctk_style_context_set_junction_sides (context, CTK_JUNCTION_CORNER_BOTTOMRIGHT);
  ctk_render_handle (context, cr, 40, 48, 12, 12);

  ctk_style_context_restore (context);

  return TRUE;
}

static char *what;

static gboolean
draw_cb (CtkWidget *widget, cairo_t *cr)
{
  if (strcmp (what, "check") == 0)
    return draw_cb_checks (widget, cr);
  else if (strcmp (what, "option") == 0)
    return draw_cb_options (widget, cr);
  else if (strcmp (what, "arrow") == 0)
    return draw_cb_arrows (widget, cr);
  else if (strcmp (what, "expander") == 0)
    return draw_cb_expanders (widget, cr);
  else if (strcmp (what, "background") == 0)
    return draw_cb_background (widget, cr);
  else if (strcmp (what, "frame") == 0)
    return draw_cb_frame (widget, cr);
  else if (strcmp (what, "activity") == 0)
    return draw_cb_activity (widget, cr);
  else if (strcmp (what, "slider") == 0)
    return draw_cb_slider (widget, cr);
  else if (strcmp (what, "focus") == 0)
    return draw_cb_focus (widget, cr);
  else if (strcmp (what, "extension") == 0)
    return draw_cb_extension (widget, cr);
  else if (strcmp (what, "frame-gap") == 0)
    return draw_cb_frame_gap (widget, cr);
  else if (strcmp (what, "handle") == 0)
    return draw_cb_handles (widget, cr);

  return FALSE;
}

int main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *ebox;
  CtkStyleContext *context;
  CtkStyleProvider *provider;

  ctk_init (&argc, &argv);

  if (argc > 1)
    what = argv[1];
  else
    what = "check";

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ebox = ctk_event_box_new ();
  ctk_event_box_set_visible_window (CTK_EVENT_BOX (ebox), TRUE);
  ctk_container_add (CTK_CONTAINER (window), ebox);
  ctk_widget_set_name (ebox, "ebox");

  context = ctk_widget_get_style_context (ebox);
  provider = (CtkStyleProvider *)ctk_css_provider_new ();
  ctk_css_provider_load_from_data (CTK_CSS_PROVIDER (provider),
                                   ".frame1 {\n"
                                   "   border-image: url('gradient1.png') 10 10 10 10 stretch;\n"
                                   "}\n"
                                   ".frame2 {\n"
                                   "   border-style: solid;\n"
                                   "   border-color: rgb(255,0,0);\n"
                                   "   border-width: 10;\n"
                                   "   border-radius: 10;\n"
                                   "}\n"
                                   ".frame3 {\n"
                                   "   border-style: solid;\n"
                                   "   border-color: rgb(0,0,0);\n"
                                   "   border-width: 2;\n"
                                   "   border-radius: 10;\n"
                                   "}\n"
                                   ".background {\n"
                                   "   border-radius: 10;\n"
                                   "   border-width: 0;\n"
                                   "   background-image: -ctk-gradient (linear, left top, right bottom, from(#ff00ff), to(#aabbcc));\n"
                                   "}\n"
                                   ".frame {\n"
                                   "   border-style: solid;\n"
                                   "   border-width: 1;\n"
                                   "   border-radius: 0;\n"
                                   "}\n", -1, NULL);
  ctk_style_context_add_provider (context, provider, CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_signal_connect_after (ebox, "draw", G_CALLBACK (draw_cb), NULL);

  ctk_widget_show_all (window);

  ctk_main ();

  ctk_style_context_remove_provider (context, provider);

  return 0;
}

