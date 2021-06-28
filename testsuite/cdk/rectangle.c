#include <cdk/cdk.h>

static void
test_rectangle_equal (void)
{
  CdkRectangle a = { 0, 0, 1, 1 };
  CdkRectangle b = { 1, 1, 2, 2 };
  CdkRectangle c = { 0, 0, 2, 2 };
  CdkRectangle d = { 0, 0, 1, 1 };
  CdkRectangle e = { 0, 0, 0, 0 };
  CdkRectangle f = { 1, 1, 0, 0 };

  g_assert_true (!cdk_rectangle_equal (&a, &b));
  g_assert_true (!cdk_rectangle_equal (&a, &c));
  g_assert_true (!cdk_rectangle_equal (&b, &c));
  g_assert_true ( cdk_rectangle_equal (&a, &d));
  g_assert_true (!cdk_rectangle_equal (&e, &f));
}

static void
test_rectangle_intersect (void)
{
  CdkRectangle a = { 0, 0, 10, 10 };
  CdkRectangle b = { 5, 5, 10, 10 };
  CdkRectangle c = { 0, 0, 0, 0 };
  CdkRectangle d = { 5, 5, 5, 5 };
  CdkRectangle e = { 0, 0, 10, 10 };
  CdkRectangle f = { 20, 20, 10, 10 };
  CdkRectangle g = { 0, 0, 0, 0 };
  CdkRectangle h = { 10, 10, 0, 0 };
  gboolean res;

  res = cdk_rectangle_intersect (&a, &b, &c);
  g_assert_true (res);
  g_assert_true (cdk_rectangle_equal (&c, &d));

  /* non-empty, non-intersecting rectangles */
  res = cdk_rectangle_intersect (&e, &f, &f);
  g_assert_cmpint (f.width, ==, 0);
  g_assert_cmpint (f.height, ==, 0);

  /* empty rectangles */
  res = cdk_rectangle_intersect (&g, &h, NULL);
  g_assert_true (!res);
}

static void
test_rectangle_union (void)
{
  CdkRectangle a = { 0, 0, 10, 10 };
  CdkRectangle b = { 5, 5, 10, 10 };
  CdkRectangle c = { 0, 0, 0, 0 };
  CdkRectangle d = { 0, 0, 15, 15 };
  CdkRectangle e = { 0, 0, 0, 0 };
  CdkRectangle f = { 50, 50, 0, 0 };
  CdkRectangle g = { 0, 0, 50, 50 };

  cdk_rectangle_union (&a, &b, &c);
  g_assert_true (cdk_rectangle_equal (&c, &d));

  cdk_rectangle_union (&a, &b, &b);
  g_assert_true (cdk_rectangle_equal (&b, &d));

  cdk_rectangle_union (&e, &f, &f);
  g_assert_true (cdk_rectangle_equal (&f, &g));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  cdk_init (NULL, NULL);

  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/rectangle/equal", test_rectangle_equal);
  g_test_add_func ("/rectangle/intersect", test_rectangle_intersect);
  g_test_add_func ("/rectangle/union", test_rectangle_union);

  return g_test_run ();
}
