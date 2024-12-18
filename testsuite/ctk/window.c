#include <ctk/ctk.h>

#ifdef CDK_WINDOWING_X11
#include <cdk/cdkx.h>
#include <X11/Xatom.h>
#endif

static gboolean interactive = FALSE;

static gboolean
stop_main (gpointer data G_GNUC_UNUSED)
{
  ctk_main_quit ();

  return G_SOURCE_REMOVE;
}

static gboolean
on_draw (CtkWidget *widget, cairo_t *cr)
{
  gint i, j;

  for (i = 0; 20 * i < ctk_widget_get_allocated_width (widget); i++)
    {
      for (j = 0; 20 * j < ctk_widget_get_allocated_height (widget); j++)
        {
          if ((i + j) % 2 == 1)
            cairo_set_source_rgb (cr, 1., 1., 1.);
          else
            cairo_set_source_rgb (cr, 0., 0., 0.);

          cairo_rectangle (cr, 20. * i, 20. *j, 20., 20.);
          cairo_fill (cr);
        }
    }

  return FALSE;
}

static gboolean
on_keypress (CtkWidget *widget G_GNUC_UNUSED)
{
  ctk_main_quit ();

  return TRUE;
}

static void
test_default_size (void)
{
  CtkWidget *window;
  CtkWidget *box;
  gint w, h;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "draw", G_CALLBACK (on_draw), NULL);
  if (interactive)
    g_signal_connect (window, "key-press-event", G_CALLBACK (on_keypress), NULL);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);

  /* check that default size is unset initially */
  ctk_window_get_default_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, -1);
  g_assert_cmpint (h, ==, -1);

  /* check that setting default size before realize works */
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);

  ctk_window_get_default_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 300);
  g_assert_cmpint (h, ==, 300);

  /* check that the window size is also reported accordingly */
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 300);
  g_assert_cmpint (h, ==, 300);

  ctk_widget_show_all (window);

  if (!interactive)
    g_timeout_add (200, stop_main, NULL);
  ctk_main ();

  /* check that the window and its content actually gets the right size */
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 300);
  g_assert_cmpint (h, ==, 300);

  g_assert_cmpint (ctk_widget_get_allocated_width (box), ==, 300);
  g_assert_cmpint (ctk_widget_get_allocated_height (box), ==, 300);

  /* check that setting default size after the fact does not change
   * window size
   */
  ctk_window_set_default_size (CTK_WINDOW (window), 100, 600);
  ctk_window_get_default_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 100);
  g_assert_cmpint (h, ==, 600);

  if (!interactive)
    g_timeout_add (200, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 300);
  g_assert_cmpint (h, ==, 300);

  /* check that even hide/show does not pull in the new default */
  ctk_widget_hide (window);
  ctk_widget_show (window);

  if (!interactive)
    g_timeout_add (200, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 300);
  g_assert_cmpint (h, ==, 300);

  ctk_widget_destroy (window);
}

static void
test_resize (void)
{
  CtkWidget *window;
  CtkWidget *box;
  gint w, h;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "draw", G_CALLBACK (on_draw), NULL);
  if (interactive)
    g_signal_connect (window, "key-press-event", G_CALLBACK (on_keypress), NULL);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);

  /* test that resize before show overrides default size */
  ctk_window_set_default_size (CTK_WINDOW (window), 500, 500);

  ctk_window_resize (CTK_WINDOW (window), 1, 1);

  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 1);
  g_assert_cmpint (h, ==, 1);

  ctk_window_resize (CTK_WINDOW (window), 400, 200);

  ctk_widget_show_all (window);

  if (!interactive)
    g_timeout_add (200, stop_main, NULL);
  ctk_main ();

  /* test that resize before show works */
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 400);
  g_assert_cmpint (h, ==, 200);

  /* test that resize after show works, both
   * for making things bigger and for making things
   * smaller
   */
  ctk_window_resize (CTK_WINDOW (window), 200, 400);

  if (!interactive)
    g_timeout_add (200, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (w, ==, 200);
  g_assert_cmpint (h, ==, 400);

  ctk_widget_destroy (window);
}

static void
test_resize_popup (void)
{
  CtkWidget *window;
  gint x, y, w, h;

  /* testcase for the dnd window */
  window = ctk_window_new (CTK_WINDOW_POPUP);
  ctk_window_set_screen (CTK_WINDOW (window), cdk_screen_get_default ());
  ctk_window_resize (CTK_WINDOW (window), 1, 1);
  ctk_window_move (CTK_WINDOW (window), -99, -99);

  ctk_window_get_position (CTK_WINDOW (window), &x, &y);
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (x, ==, -99);
  g_assert_cmpint (y, ==, -99);
  g_assert_cmpint (w, ==, 1);
  g_assert_cmpint (h, ==, 1);

  ctk_widget_show (window);

  g_timeout_add (200, stop_main, NULL);
  ctk_main ();

  ctk_window_get_position (CTK_WINDOW (window), &x, &y);
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  g_assert_cmpint (x, ==, -99);
  g_assert_cmpint (y, ==, -99);
  g_assert_cmpint (w, ==, 1);
  g_assert_cmpint (h, ==, 1);

  ctk_widget_destroy (window);
}

static void
test_show_hide (void)
{
  CtkWidget *window;
  gint w, h, w1, h1;

  g_test_bug ("696882");

  /* test that hide/show does not affect the size */

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  ctk_widget_show (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w, &h);

  ctk_widget_hide (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w1, &h1);
  g_assert_cmpint (w, ==, w1);
  g_assert_cmpint (h, ==, h1);

  ctk_widget_show (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w1, &h1);
  g_assert_cmpint (w, ==, w1);
  g_assert_cmpint (h, ==, h1);

  ctk_widget_destroy (window);
}

static void
test_show_hide2 (void)
{
  CtkWidget *window;
  gint x, y, w, h, w1, h1;

  g_test_bug ("696882");

  /* test that hide/show does not affect the size,
   * even when get_position/move is called
   */

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  ctk_widget_show (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_position (CTK_WINDOW (window), &x, &y);
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  ctk_widget_hide (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w1, &h1);
  g_assert_cmpint (w, ==, w1);
  g_assert_cmpint (h, ==, h1);

  ctk_window_move (CTK_WINDOW (window), x, y);
  ctk_widget_show (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w1, &h1);
  g_assert_cmpint (w, ==, w1);
  g_assert_cmpint (h, ==, h1);

  ctk_widget_destroy (window);
}

static void
test_show_hide3 (void)
{
  CtkWidget *window;
  gint x, y, w, h, w1, h1;

  g_test_bug ("696882");

  /* test that hide/show does not affect the size,
   * even when get_position/move is called and
   * a default size is set
   */

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 200, 200);

  ctk_widget_show (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_position (CTK_WINDOW (window), &x, &y);
  ctk_window_get_size (CTK_WINDOW (window), &w, &h);
  ctk_widget_hide (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w1, &h1);
  g_assert_cmpint (w, ==, w1);
  g_assert_cmpint (h, ==, h1);

  ctk_window_move (CTK_WINDOW (window), x, y);
  ctk_widget_show (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

  ctk_window_get_size (CTK_WINDOW (window), &w1, &h1);
  g_assert_cmpint (w, ==, w1);
  g_assert_cmpint (h, ==, h1);

  ctk_widget_destroy (window);
}

static gboolean
on_map_event (CtkWidget *window G_GNUC_UNUSED)
{
  ctk_main_quit ();

  return FALSE;
}

static void
test_hide_titlebar_when_maximized (void)
{
  CtkWidget *window;

  g_test_bug ("740287");

  /* test that hide-titlebar-when-maximized gets set appropriately
   * on the window, if it's set before the window is realized.
   */

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  g_signal_connect (window,
                    "map-event",
                    G_CALLBACK (on_map_event),
                    NULL);

  ctk_window_set_hide_titlebar_when_maximized (CTK_WINDOW (window), TRUE);

  ctk_widget_show (window);

  g_timeout_add (100, stop_main, NULL);
  ctk_main ();

#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_SCREEN (ctk_widget_get_screen (window)))
    {
      Atom type;
      gint format;
      gulong nitems;
      gulong bytes_after;
      gulong *hide = NULL;

      XGetWindowProperty (cdk_x11_get_default_xdisplay (),
                          CDK_WINDOW_XID (ctk_widget_get_window (window)),
                          cdk_x11_get_xatom_by_name ("_CTK_HIDE_TITLEBAR_WHEN_MAXIMIZED"),
                          0,
                          G_MAXLONG,
                          False,
                          XA_CARDINAL,
                          &type,
                          &format,
                          &nitems,
                          &bytes_after,
                          (guchar **) &hide);

      g_assert_cmpint (type, !=, None);
      g_assert_cmpint (type, ==, XA_CARDINAL);
      g_assert_cmpint (format, ==, 32);
      g_assert_cmpint (nitems, ==, 1);
      g_assert_cmpint (hide[0], ==, 1);

      XFree (hide);
    }
#endif

  ctk_widget_destroy (window);
}

int
main (int argc, char *argv[])
{
  gint i;

  ctk_test_init (&argc, &argv);
  g_test_bug_base ("http://bugzilla.gnome.org/");

  for (i = 0; i < argc; i++)
    {
      if (g_strcmp0 (argv[i], "--interactive") == 0)
        interactive = TRUE;
    }

  g_test_add_func ("/window/default-size", test_default_size);
  g_test_add_func ("/window/resize", test_resize);
  g_test_add_func ("/window/resize-popup", test_resize_popup);
  g_test_add_func ("/window/show-hide", test_show_hide);
  g_test_add_func ("/window/show-hide2", test_show_hide2);
  g_test_add_func ("/window/show-hide3", test_show_hide3);
  g_test_add_func ("/window/hide-titlebar-when-maximized", test_hide_titlebar_when_maximized);

  return g_test_run ();
}
