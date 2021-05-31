#include <ctk/ctk.h>

static void
test_init_of_theme (void)
{
  CtkStyleContext *context;
  CtkCssProvider *provider;
  CtkWidgetPath *path;
  GdkRGBA before, after;
  char *css;

  /* Test that a style context actually uses the theme loaded for the 
   * screen it is using. If no screen is set, it's the default one.
   */
  context = ctk_style_context_new ();
  path = ctk_widget_path_new ();

  /* Set a path that will have a color set.
   * (This could actually fail if style classes change, so if this test
   *  fails, make sure to have this path represent something sane.)
   */
  ctk_widget_path_append_type (path, CTK_TYPE_WINDOW);
  ctk_widget_path_iter_add_class (path, -1, CTK_STYLE_CLASS_BACKGROUND);
  ctk_style_context_set_path (context, path);
  ctk_widget_path_free (path);

  /* Get the color. This should be initialized by the theme and not be
   * the default. */
  ctk_style_context_get_color (context, ctk_style_context_get_state (context), &before);

  /* Add a style that sets a different color for this widget.
   * This style has a higher priority than fallback, but a lower 
   * priority than the theme. */
  css = g_strdup_printf (".background { color: %s; }",
                         before.alpha < 0.5 ? "black" : "transparent");
  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider, css, -1, NULL);
  ctk_style_context_add_provider (context,
                                  CTK_STYLE_PROVIDER (provider),
                                  CTK_STYLE_PROVIDER_PRIORITY_FALLBACK + 1);
  g_object_unref (provider);

  /* Get the color again. */
  ctk_style_context_get_color (context, ctk_style_context_get_state (context), &after);

  /* Because the style we added does not influence the color,
   * the before and after colors should be identical. */
  g_assert (gdk_rgba_equal (&before, &after));

  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  /* If gdk_init() is called before ctk_init() the CTK code takes
   * a different path (why?)
   */
  gdk_init (NULL, NULL);
  ctk_init (NULL, NULL);
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/style/init_of_theme", test_init_of_theme);

  return g_test_run ();
}
