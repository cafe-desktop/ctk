/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include <ctk/ctk.h>
#include <string.h>

#include "ctk/fallback-c89.c"
#include "frame-stats.h"

#define RADIUS 64
#define DIAMETER (2*RADIUS)
#define WIDTH 600
#define HEIGHT 600
#define WINDOW_SIZE_JITTER 200
#define CYCLE_TIME 5.

static CtkWidget *window;
static int window_width = WIDTH, window_height = HEIGHT;

gint64 start_frame_time;
static double angle;

static double load_factor = 1.0;
static double cb_no_resize = FALSE;

static cairo_surface_t *source_surface;

static void
ensure_resources(cairo_surface_t *target)
{
  cairo_t *cr;
  int i, j;

  if (source_surface != NULL)
    return;

  source_surface = cairo_surface_create_similar (target, CAIRO_CONTENT_COLOR_ALPHA,
                                                 16 * DIAMETER, 16 * DIAMETER);
  cr = cairo_create(source_surface);

  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(cr);
  cairo_restore(cr);

  cairo_set_line_width(cr, 1.0);

  for (j = 0; j < 16; j++)
    for (i = 0; i < 16; i++)
      {
        cairo_set_source_rgba(cr,
                              ((i * 41) % 16) / 15.,
                              ((i * 31) % 16) / 15.,
                              ((i * 23) % 16) / 15.,
                              0.25);
        cairo_arc(cr,
                  i * DIAMETER + RADIUS, j * DIAMETER + RADIUS,
                  RADIUS - 0.5, 0, 2 * M_PI);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr,
                              ((i * 41) % 16) / 15.,
                              ((i * 31) % 16) / 15.,
                              ((i * 23) % 16) / 15.,
                              1.0);
        cairo_stroke(cr);
      }
}

static gboolean
on_window_draw (CtkWidget *widget,
                cairo_t   *cr)

{
  GRand *rand = g_rand_new_with_seed(0);
  int i;
  int width, height;

  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);

  ensure_resources (cairo_get_target (cr));

  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, 1.0);
  cairo_rectangle (cr, 0.5, 0.5, width - 1, height - 1);
  cairo_stroke (cr);

  for(i = 0; i < load_factor * 150; i++)
    {
      int source = g_rand_int_range(rand, 0, 255);
      double phi = g_rand_double_range(rand, 0, 2 * M_PI) + angle;
      double r = g_rand_double_range(rand, 0, width / 2 - RADIUS);
      int x, y;

      int source_x = (source % 16) * DIAMETER;
      int source_y = (source / 16) * DIAMETER;

      x = round(width / 2 + r * cos(phi) - RADIUS);
      y = round(height / 2 - r * sin(phi) - RADIUS);

      cairo_set_source_surface(cr, source_surface,
                               x - source_x, y - source_y);
      cairo_rectangle(cr, x, y, DIAMETER, DIAMETER);
      cairo_fill(cr);
    }

  g_rand_free(rand);

  return FALSE;
}

static void
on_frame (double progress)
{
  int jitter;

  angle = 2 * M_PI * progress;
  jitter = WINDOW_SIZE_JITTER * sin(angle);

  if (!cb_no_resize)
    {
      window_width = WIDTH + jitter;
      window_height = HEIGHT + jitter;
    }

  ctk_window_resize (CTK_WINDOW (window),
                     window_width, window_height);

  ctk_widget_queue_draw (window);
}

static gboolean
tick_callback (CtkWidget     *widget G_GNUC_UNUSED,
               CdkFrameClock *frame_clock,
               gpointer       user_data G_GNUC_UNUSED)
{
  gint64 frame_time = cdk_frame_clock_get_frame_time (frame_clock);
  double scaled_time;

  if (start_frame_time == 0)
    start_frame_time = frame_time;

  scaled_time = (frame_time - start_frame_time) / (CYCLE_TIME * 1000000);
  on_frame (scaled_time - floor (scaled_time));

  return G_SOURCE_CONTINUE;
}

static gboolean
on_map_event (CtkWidget	  *widget G_GNUC_UNUSED,
              CdkEventAny *event G_GNUC_UNUSED)
{
  ctk_widget_add_tick_callback (window, tick_callback, NULL, NULL);

  return FALSE;
}

static GOptionEntry options[] = {
  { "factor", 'f', 0, G_OPTION_ARG_DOUBLE, &load_factor, "Load factor", "FACTOR" },
  { "no-resize", 'n', 0, G_OPTION_ARG_NONE, &cb_no_resize, "No Resize", NULL },
  { NULL }
};

int
main(int argc, char **argv)
{
  GError *error = NULL;
  CdkMonitor *monitor;
  CdkRectangle monitor_bounds;

  GOptionContext *context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, options, NULL);
  frame_stats_add_options (g_option_context_get_main_group (context));
  g_option_context_add_group (context,
                              ctk_get_option_group (TRUE));

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("Option parsing failed: %s\n", error->message);
      return 1;
    }

  g_print ("# Load factor: %g\n",
           load_factor);
  g_print ("# Resizing?: %s\n",
           cb_no_resize ? "no" : "yes");

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  frame_stats_ensure (CTK_WINDOW (window));

  ctk_window_set_keep_above (CTK_WINDOW (window), TRUE);
  ctk_window_set_gravity (CTK_WINDOW (window), CDK_GRAVITY_CENTER);
  ctk_widget_set_app_paintable (window, TRUE);

  g_signal_connect (window, "draw",
                    G_CALLBACK (on_window_draw), NULL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (ctk_main_quit), NULL);

  g_signal_connect (window, "map-event",
                    G_CALLBACK (on_map_event), NULL);
  on_frame (0.);

  monitor = cdk_display_get_primary_monitor (ctk_widget_get_display (window));
  cdk_monitor_get_geometry (monitor, &monitor_bounds);

  ctk_window_move (CTK_WINDOW (window),
                   monitor_bounds.x + (monitor_bounds.width - window_width) / 2,
                   monitor_bounds.y + (monitor_bounds.height - window_height) / 2);

  ctk_widget_show (window);

  ctk_main ();

  return 0;
}
