#include <gtk/gtk.h>

static void
child_size_allocate (GtkWidget *child,
                     GdkRectangle *allocation,
                     gpointer user_data)
{
  GtkStyleContext *context;
  context = ctk_widget_get_style_context (child);

  g_print ("Child %p\nHas left? %d\nHas right? %d\nHas top? %d\nHas bottom? %d\n",
           child,
           ctk_style_context_has_class (context, "left"),
           ctk_style_context_has_class (context, "right"),
           ctk_style_context_has_class (context, "top"),
           ctk_style_context_has_class (context, "bottom"));
}

static gboolean
overlay_get_child_position (GtkOverlay *overlay,
                            GtkWidget *child,
                            GdkRectangle *allocation,
                            gpointer user_data)
{
  GtkWidget *custom_child = user_data;
  GtkRequisition req;

  if (child != custom_child)
    return FALSE;

  ctk_widget_get_preferred_size (child, NULL, &req);

  allocation->x = 120;
  allocation->y = 0;
  allocation->width = req.width;
  allocation->height = req.height;

  return TRUE;
}

int 
main (int argc, char *argv[])
{
  GtkWidget *win, *overlay, *grid, *main_child, *child, *label, *sw;
  GtkCssProvider *provider;
  gchar *str;

  ctk_init (&argc, &argv);

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider,
                                   "GtkLabel { border: 3px solid black; border-radius: 5px; padding: 2px; }"
                                   ".top { border-top-style: none; right-radius: 0px; border-top-left-radius: 0px; }"
                                   ".bottom { border-bottom-style: none; border-bottom-right-radius: 0px; border-bottom-left-radius: 0px; }"
                                   ".left { border-left-style: none; border-top-left-radius: 0px; border-bottom-left-radius: 0px; }"
                                   ".right { border-right-style: none; border-top-right-radius: 0px; border-bottom-right-radius: 0px; }",
                                   -1, NULL);
  ctk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (win), 600, 600);

  grid = ctk_grid_new ();
  child = ctk_event_box_new ();
  ctk_widget_set_hexpand (child, TRUE);
  ctk_widget_set_vexpand (child, TRUE);
  ctk_container_add (CTK_CONTAINER (grid), child);
  label = ctk_label_new ("Out of overlay");
  ctk_container_add (CTK_CONTAINER (child), label);

  overlay = ctk_overlay_new ();
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_ALWAYS,
                                  CTK_POLICY_ALWAYS);
  ctk_container_add (CTK_CONTAINER (overlay), sw);

  main_child = ctk_event_box_new ();
  ctk_container_add (CTK_CONTAINER (sw), main_child);
  ctk_widget_set_hexpand (main_child, TRUE);
  ctk_widget_set_vexpand (main_child, TRUE);
  label = ctk_label_new ("Main child");
  ctk_widget_set_halign (label, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_container_add (CTK_CONTAINER (main_child), label);

  child = ctk_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  ctk_label_set_text (CTK_LABEL (child), str);
  g_free (str);
  g_print ("Bottom/Right child: %p\n", child);
  ctk_widget_set_halign (child, CTK_ALIGN_END);
  ctk_widget_set_valign (child, CTK_ALIGN_END);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);

  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (child_size_allocate), overlay);

  child = ctk_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  ctk_label_set_text (CTK_LABEL (child), str);
  g_free (str);
  g_print ("Left/Top child: %p\n", child);
  ctk_widget_set_halign (child, CTK_ALIGN_START);
  ctk_widget_set_valign (child, CTK_ALIGN_START);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);

  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (child_size_allocate), overlay);

  child = ctk_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  ctk_label_set_text (CTK_LABEL (child), str);
  g_free (str);
  g_print ("Right/Center child: %p\n", child);
  ctk_widget_set_halign (child, CTK_ALIGN_END);
  ctk_widget_set_valign (child, CTK_ALIGN_CENTER);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);

  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (child_size_allocate), overlay);

  child = ctk_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  ctk_label_set_text (CTK_LABEL (child), str);
  g_free (str);
  ctk_widget_set_margin_start (child, 55);
  ctk_widget_set_margin_top (child, 4);
  g_print ("Left/Top margined child: %p\n", child);
  ctk_widget_set_halign (child, CTK_ALIGN_START);
  ctk_widget_set_valign (child, CTK_ALIGN_START);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);

  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (child_size_allocate), overlay);

  child = ctk_label_new (NULL);
  str = g_strdup_printf ("%p", child);
  ctk_label_set_text (CTK_LABEL (child), str);
  g_free (str);
  g_print ("Custom get-child-position child: %p\n", child);
  ctk_widget_set_halign (child, CTK_ALIGN_START);
  ctk_widget_set_valign (child, CTK_ALIGN_START);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), child);

  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (child_size_allocate), overlay);
  g_signal_connect (overlay, "get-child-position",
                    G_CALLBACK (overlay_get_child_position), child);

  ctk_grid_attach (CTK_GRID (grid), overlay, 1, 0, 1, 3);
  ctk_container_add (CTK_CONTAINER (win), grid);

  g_print ("\n");

  ctk_widget_show_all (win);

  ctk_main ();

  return 0;
}
