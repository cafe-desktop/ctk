#include <gtk/gtk.h>

static void
assert_section_is_not_null (GtkCssProvider *provider,
                            GtkCssSection  *section,
                            const GError   *error,
                            gpointer        unused)
{
  g_assert (section != NULL);
}

static void
test_section_in_load_from_data (void)
{
  GtkCssProvider *provider;

  provider = ctk_css_provider_new ();
  g_signal_connect (provider, "parsing-error",
                    G_CALLBACK (assert_section_is_not_null), NULL);
  ctk_css_provider_load_from_data (provider, "random garbage goes here", -1, NULL);
  g_object_unref (provider);
}

static void
test_section_in_style_property (void)
{
  GtkCssProvider *provider;
  GtkWidgetClass *widget_class;
  GtkWidgetPath *path;
  GParamSpec *pspec;
  GValue value = G_VALUE_INIT;

  provider = ctk_css_provider_new ();
  g_signal_connect (provider, "parsing-error",
                    G_CALLBACK (assert_section_is_not_null), NULL);
  ctk_css_provider_load_from_data (provider, "* { -GtkWidget-interior-focus: random garbage goes here; }", -1, NULL);

  widget_class = g_type_class_ref (CTK_TYPE_WIDGET);
  pspec = ctk_widget_class_find_style_property (widget_class, "interior-focus");
  g_assert (pspec);
  path = ctk_widget_path_new ();
  ctk_widget_path_append_type (path, CTK_TYPE_WIDGET);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_style_provider_get_style_property (CTK_STYLE_PROVIDER (provider), path, 0, pspec, &value);
G_GNUC_END_IGNORE_DEPRECATIONS;

  ctk_widget_path_unref (path);
  g_type_class_unref (widget_class);
  g_object_unref (provider);
}

static void
test_section_load_nonexisting_file (void)
{
  GtkCssProvider *provider;

  provider = ctk_css_provider_new ();
  g_signal_connect (provider, "parsing-error",
                    G_CALLBACK (assert_section_is_not_null), NULL);
  ctk_css_provider_load_from_path (provider, "this/path/does/absolutely/not/exist.css", NULL);
  g_object_unref (provider);
}

int
main (int argc, char *argv[])
{
  ctk_init (NULL, NULL);
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/cssprovider/section-in-load-from-data", test_section_in_load_from_data);
  g_test_add_func ("/cssprovider/section-in-style-property", test_section_in_style_property);
  g_test_add_func ("/cssprovider/load-nonexisting-file", test_section_load_nonexisting_file);

  return g_test_run ();
}
