#include <cdk/cdk.h>

/* We don't technically guarantee that the visual returned by
 * cdk_screen_get_rgba_visual is ARGB8888. But if it isn't, lots
 * of code will break, so test this here anyway.
 * The main point of this test is to ensure that the pixel_details
 * functions return meaningful values for TrueColor visuals.
 */
static void
test_rgba_visual (void)
{
  CdkScreen *screen;
  CdkVisual *visual;
  guint32 r_mask, g_mask, b_mask;
  gint r_shift, g_shift, b_shift;
  gint r_precision, g_precision, b_precision;
  gint depth;
  CdkVisualType type;

  g_test_bug ("764210");

  screen = cdk_screen_get_default ();
  visual = cdk_screen_get_rgba_visual (screen);

  if (visual == NULL)
    {
      g_test_skip ("no rgba visual");
      return;
    }

  depth = cdk_visual_get_depth (visual);
  type = cdk_visual_get_visual_type (visual);
  cdk_visual_get_red_pixel_details (visual, &r_mask, &r_shift, &r_precision);
  cdk_visual_get_green_pixel_details (visual, &g_mask, &g_shift, &g_precision);
  cdk_visual_get_blue_pixel_details (visual, &b_mask, &b_shift, &b_precision);

  g_assert_cmpint (depth, ==, 32);
  g_assert_cmpint (type, ==, GDK_VISUAL_TRUE_COLOR);

  g_assert_cmphex (r_mask, ==, 0x00ff0000);
  g_assert_cmphex (g_mask, ==, 0x0000ff00);
  g_assert_cmphex (b_mask, ==, 0x000000ff);

  g_assert_cmpint (r_shift, ==, 16);
  g_assert_cmpint (g_shift, ==,  8);
  g_assert_cmpint (b_shift, ==,  0);

  g_assert_cmpint (r_precision, ==, 8);
  g_assert_cmpint (g_precision, ==, 8);
  g_assert_cmpint (b_precision, ==, 8);
}

static void
test_list_visuals (void)
{
  CdkScreen *screen;
  CdkVisual *visual;
  CdkVisual *rgba_visual;
  CdkVisual *system_visual;
  GList *list, *l;
  gboolean found_system, found_rgba;

  screen = cdk_screen_get_default ();
  system_visual = cdk_screen_get_system_visual (screen);
  rgba_visual = cdk_screen_get_rgba_visual (screen);

  found_system = FALSE;
  found_rgba = FALSE;

  list = cdk_screen_list_visuals (screen);
  for (l = list; l; l = l->next)
    {
      visual = l->data;
      if (visual == system_visual)
        found_system = TRUE;
      if (visual == rgba_visual)
        found_rgba = TRUE;
      g_assert_true (GDK_IS_VISUAL (visual));

      g_assert_true (cdk_visual_get_screen (visual) == screen);
    }
  g_list_free (list);

  g_assert (system_visual != NULL && found_system);
  g_assert (rgba_visual == NULL || found_rgba);
}

static void
test_depth (void)
{
  CdkVisual *visual;
  gint *depths;
  gint n_depths;
  gint i, j;
  gboolean is_depth;

  cdk_query_depths (&depths, &n_depths);
  g_assert_cmpint (n_depths, >, 0);
  for (i = 0; i < n_depths; i++)
    {
      g_assert_cmpint (depths[i], >, 0);
      g_assert_cmpint (depths[i], <=, 32);

      visual = cdk_visual_get_best_with_depth (depths[i]);

      g_assert_nonnull (visual);
      g_assert_cmpint (cdk_visual_get_depth (visual), ==, depths[i]);
    }

  for (i = 1; i <= 32; i++)
    {
      is_depth = FALSE;
      for (j = 0; j < n_depths; j++)
        {
          if (i == depths[j])
            is_depth = TRUE;
        }

      visual = cdk_visual_get_best_with_depth (i);
      if (!is_depth)
        g_assert_null (visual);
      else
        {
          g_assert_nonnull (visual);
          g_assert_cmpint (cdk_visual_get_depth (visual), ==, i);
        }
    }
}

static void
test_type (void)
{
  CdkVisual *visual;
  CdkVisualType *types;
  gint n_types;
  gint i, j;
  gboolean is_type;

  cdk_query_visual_types (&types, &n_types);
  g_assert_cmpint (n_types, >, 0);
  for (i = 0; i < n_types; i++)
    {
      g_assert_cmpint (types[i], >=, GDK_VISUAL_STATIC_GRAY);
      g_assert_cmpint (types[i], <=, GDK_VISUAL_DIRECT_COLOR);

      visual = cdk_visual_get_best_with_type (types[i]);

      g_assert_nonnull (visual);
      g_assert_cmpint (cdk_visual_get_visual_type (visual), ==, types[i]);
    }

  for (i = GDK_VISUAL_STATIC_GRAY; i <= GDK_VISUAL_DIRECT_COLOR; i++)
    {
      is_type = FALSE;
      for (j = 0; j < n_types; j++)
        {
          if (i == types[j])
            is_type = TRUE;
        }

      visual = cdk_visual_get_best_with_type (i);
      if (!is_type)
        g_assert_null (visual);
      else
        {
          g_assert_nonnull (visual);
          g_assert_cmpint (cdk_visual_get_visual_type (visual), ==, i);
        }
    }
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  cdk_init (NULL, NULL);

  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/visual/list", test_list_visuals);
  g_test_add_func ("/visual/rgba", test_rgba_visual);
  g_test_add_func ("/visual/depth", test_depth);
  g_test_add_func ("/visual/type", test_type);

  return g_test_run ();
}
