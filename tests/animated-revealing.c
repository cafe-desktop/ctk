/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include <ctk/ctk.h>

#include "frame-stats.h"

double reveal_time = 5;

static GOptionEntry options[] = {
  { "time", 't', 0, G_OPTION_ARG_DOUBLE, &reveal_time, "Reveal time", "SECONDS" },
  { NULL }
};

static void
toggle_reveal (CtkRevealer *revealer)
{
  ctk_revealer_set_reveal_child (revealer, !ctk_revealer_get_reveal_child (revealer));
}

int
main(int argc, char **argv)
{
  CtkWidget *window, *revealer, *grid, *widget;
  CtkCssProvider *cssprovider;
  GError *error = NULL;
  guint x, y;

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

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);
  frame_stats_ensure (CTK_WINDOW (window));

  revealer = ctk_revealer_new ();
  ctk_widget_set_valign (revealer, CTK_ALIGN_START);
  ctk_revealer_set_transition_type (CTK_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
  ctk_revealer_set_transition_duration (CTK_REVEALER (revealer), reveal_time * 1000);
  ctk_revealer_set_reveal_child (CTK_REVEALER (revealer), TRUE);
  g_signal_connect_after (revealer, "map", G_CALLBACK (toggle_reveal), NULL);
  g_signal_connect_after (revealer, "notify::child-revealed", G_CALLBACK (toggle_reveal), NULL);
  ctk_container_add (CTK_CONTAINER (window), revealer);

  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (revealer), grid);

  cssprovider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (cssprovider, "* { padding: 2px; text-shadow: 5px 5px 2px grey; }", -1, NULL);

  for (x = 0; x < 10; x++)
    {
      for (y = 0; y < 20; y++)
        {
          widget = ctk_label_new ("Hello World");
          ctk_style_context_add_provider (ctk_widget_get_style_context (widget),
                                          CTK_STYLE_PROVIDER (cssprovider),
                                          CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
          ctk_grid_attach (CTK_GRID (grid), widget, x, y, 1, 1);
        }
    }

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
