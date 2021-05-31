/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include <gtk/gtk.h>
#include <math.h>

#include "frame-stats.h"


/* Stub definition of MyTextView which is used in the
 * widget-factory.ui file. We just need this so the
 * test keeps working
 */
typedef struct
{
  GtkTextView tv;
} MyTextView;

typedef GtkTextViewClass MyTextViewClass;

G_DEFINE_TYPE (MyTextView, my_text_view, GTK_TYPE_TEXT_VIEW)

static void
my_text_view_init (MyTextView *tv) {}

static void
my_text_view_class_init (MyTextViewClass *tv_class) {}



GtkWidget *
create_widget_factory_content (void)
{
  GError *error = NULL;
  GtkBuilder *builder;
  GtkWidget *result;

  g_type_ensure (my_text_view_get_type ());
  builder = ctk_builder_new ();
  ctk_builder_add_from_file (builder,
                             "../demos/widget-factory/widget-factory.ui",
                             &error);
  if (error != NULL)
    g_error ("Failed to create widgets: %s", error->message);

  result = GTK_WIDGET (ctk_builder_get_object (builder, "box1"));
  g_object_ref (result);
  ctk_container_remove (GTK_CONTAINER (ctk_widget_get_parent (result)),
                        result);
  g_object_unref (builder);

  return result;
}

static void
set_adjustment_to_fraction (GtkAdjustment *adjustment,
                            gdouble        fraction)
{
  gdouble upper = ctk_adjustment_get_upper (adjustment);
  gdouble lower = ctk_adjustment_get_lower (adjustment);
  gdouble page_size = ctk_adjustment_get_page_size (adjustment);

  ctk_adjustment_set_value (adjustment,
                            (1 - fraction) * lower +
                            fraction * (upper - page_size));
}

gboolean
scroll_viewport (GtkWidget     *viewport,
                 GdkFrameClock *frame_clock,
                 gpointer       user_data)
{
  static gint64 start_time;
  gint64 now = gdk_frame_clock_get_frame_time (frame_clock);
  gdouble elapsed;
  GtkAdjustment *hadjustment, *vadjustment;

  if (start_time == 0)
    start_time = now;

  elapsed = (now - start_time) / 1000000.;

  hadjustment = ctk_scrollable_get_hadjustment (GTK_SCROLLABLE (viewport));
  vadjustment = ctk_scrollable_get_vadjustment (GTK_SCROLLABLE (viewport));

  set_adjustment_to_fraction (hadjustment, 0.5 + 0.5 * sin (elapsed));
  set_adjustment_to_fraction (vadjustment, 0.5 + 0.5 * cos (elapsed));

  return TRUE;
}

static GOptionEntry options[] = {
  { NULL }
};

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *scrolled_window;
  GtkWidget *viewport;
  GtkWidget *grid;
  GError *error = NULL;
  int i;

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

  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  frame_stats_ensure (GTK_WINDOW (window));
  ctk_window_set_default_size (GTK_WINDOW (window), 800, 600);

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (GTK_CONTAINER (window), scrolled_window);

  viewport = ctk_viewport_new (NULL, NULL);
  ctk_container_add (GTK_CONTAINER (scrolled_window), viewport);

  grid = ctk_grid_new ();
  ctk_container_add (GTK_CONTAINER (viewport), grid);

  for (i = 0; i < 4; i++)
    {
      GtkWidget *content = create_widget_factory_content ();
      ctk_grid_attach (GTK_GRID (grid), content,
                       i % 2, i / 2, 1, 1);
      g_object_unref (content);
    }

  ctk_widget_add_tick_callback (viewport,
                                scroll_viewport,
                                NULL,
                                NULL);

  ctk_widget_show_all (window);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (ctk_main_quit), NULL);
  ctk_main ();

  return 0;
}
