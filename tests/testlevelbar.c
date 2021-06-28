#include <ctk/ctk.h>

static CtkWidget *
create_level_bar (void)
{
  CtkWidget *bar;

  bar = ctk_level_bar_new ();
  ctk_level_bar_set_min_value (CTK_LEVEL_BAR (bar), 0.0);
  ctk_level_bar_set_max_value (CTK_LEVEL_BAR (bar), 10.0);

  ctk_level_bar_add_offset_value (CTK_LEVEL_BAR (bar),
                                  CTK_LEVEL_BAR_OFFSET_LOW, 1.0);

  ctk_level_bar_add_offset_value (CTK_LEVEL_BAR (bar),
                                  CTK_LEVEL_BAR_OFFSET_HIGH, 9.0);

  ctk_level_bar_add_offset_value (CTK_LEVEL_BAR (bar),
                                  "full", 10.0);

  ctk_level_bar_add_offset_value (CTK_LEVEL_BAR (bar),
                                  "my-offset", 5.0);

  return bar;
}

static void
add_custom_css (void)
{
  CtkCssProvider *provider;
  const gchar data[] =
  "levelbar block.my-offset {"
  "   background: magenta;"
  "}";

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider, data, -1, NULL);
  ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static gboolean
increase_level (gpointer data)
{
  CtkLevelBar *bar = data;
  gdouble value;

  value = ctk_level_bar_get_value (bar);
  value += 0.1;
  if (value >= ctk_level_bar_get_max_value (bar))
    value = ctk_level_bar_get_min_value (bar);
  ctk_level_bar_set_value (bar, value);

  return G_SOURCE_CONTINUE;
}

static gboolean
window_delete_event (CtkWidget *widget,
                     CdkEvent *event,
                     gpointer _data)
{
  ctk_main_quit ();
  return FALSE;
}

static void
toggle (CtkSwitch *sw, GParamSpec *pspec, CtkLevelBar *bar)
{
  if (ctk_switch_get_active (sw))
    ctk_level_bar_set_mode (bar, CTK_LEVEL_BAR_MODE_DISCRETE);
  else
    ctk_level_bar_set_mode (bar, CTK_LEVEL_BAR_MODE_CONTINUOUS);
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *bar;
  CtkWidget *box2;
  CtkWidget *sw;

  ctk_init (&argc, &argv);

  add_custom_css ();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 500, 100);
  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
  g_object_set (box, "margin", 20, NULL);
  bar = create_level_bar ();
  ctk_container_add (CTK_CONTAINER (window), box);
  ctk_container_add (CTK_CONTAINER (box), bar);
  box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (box), box2);
  ctk_container_add (CTK_CONTAINER (box2), ctk_label_new ("Discrete"));
  sw = ctk_switch_new ();
  ctk_container_add (CTK_CONTAINER (box2), sw);
  g_signal_connect (sw, "notify::active", G_CALLBACK (toggle), bar);

  ctk_widget_show_all (window);

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (window_delete_event), NULL);

  g_timeout_add (100, increase_level, bar);
  ctk_main ();

  return 0;
}

