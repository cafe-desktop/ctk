#include <ctk/ctk.h>

#define MIN_SIZE 150
#define MAX_SIZE 300
#define BOX_SIZE 600

typedef enum
{
  MINIMUM_CONTENT = 1 << 0,
  MAXIMUM_CONTENT = 1 << 1
} TestProperty;

static void
test_size (gboolean       overlay,
           CtkPolicyType  policy,
           CtkOrientation orientation,
           TestProperty   prop)
{
  CtkWidget *scrolledwindow, *box;
  int min_size, max_size, child_size;
  int scrollbar_size = 0;

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_set_hexpand (box, TRUE);
  ctk_widget_set_vexpand (box, TRUE);

  scrolledwindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_propagate_natural_width (CTK_SCROLLED_WINDOW (scrolledwindow), TRUE);
  ctk_scrolled_window_set_propagate_natural_height (CTK_SCROLLED_WINDOW (scrolledwindow), TRUE);
  ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (scrolledwindow), overlay);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolledwindow), policy, policy);
  ctk_container_add (CTK_CONTAINER (scrolledwindow), box);
  ctk_widget_show_all (scrolledwindow);

  /* Testing the content-width property */
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (prop & MINIMUM_CONTENT)
        {
          ctk_scrolled_window_set_min_content_width (CTK_SCROLLED_WINDOW (scrolledwindow), MIN_SIZE);

          ctk_widget_get_preferred_width (scrolledwindow, &min_size, NULL);
        }

      if (prop & MAXIMUM_CONTENT)
        {
          ctk_scrolled_window_set_max_content_width (CTK_SCROLLED_WINDOW (scrolledwindow), MAX_SIZE);
          ctk_widget_set_size_request (box, BOX_SIZE, -1);

          /*
           * Here, the content is purposely bigger than the scrolled window,
           * so it should grow up to max-content-width.
           */
          ctk_widget_get_preferred_width (scrolledwindow, NULL, &max_size);
          ctk_widget_get_preferred_width (box, &child_size, NULL);
        }

      /* If the relevant scrollbar is non-overlay and always shown, it is added
       * to the preferred size. When comparing to the expected size, we need to
       * to exclude that extra, as we are only interested in the content size */
      if (!overlay && policy == CTK_POLICY_ALWAYS)
        {
          CtkWidget *scrollbar = ctk_scrolled_window_get_vscrollbar (CTK_SCROLLED_WINDOW (scrolledwindow));
          ctk_widget_get_preferred_width (scrollbar, &scrollbar_size, NULL);
        }
    }
  /* Testing the content-height property */
  else
    {
      if (prop & MINIMUM_CONTENT)
        {
          ctk_scrolled_window_set_min_content_height (CTK_SCROLLED_WINDOW (scrolledwindow), MIN_SIZE);

          ctk_widget_get_preferred_height (scrolledwindow, &min_size, NULL);
        }

      if (prop & MAXIMUM_CONTENT)
        {
          ctk_scrolled_window_set_max_content_height (CTK_SCROLLED_WINDOW (scrolledwindow), MAX_SIZE);
          ctk_widget_set_size_request (box, -1, BOX_SIZE);

          /*
           * Here, the content is purposely bigger than the scrolled window,
           * so it should grow up to max-content-height.
           */
          ctk_widget_get_preferred_height (scrolledwindow, NULL, &max_size);
          ctk_widget_get_preferred_height (box, &child_size, NULL);
        }

      if (!overlay && policy == CTK_POLICY_ALWAYS)
        {
          CtkWidget *scrollbar = ctk_scrolled_window_get_hscrollbar (CTK_SCROLLED_WINDOW (scrolledwindow));
          ctk_widget_get_preferred_height (scrollbar, &scrollbar_size, NULL);
        }
    }

  if (prop & MINIMUM_CONTENT)
    {
      min_size -= scrollbar_size;
      g_assert_cmpint (min_size, ==, MIN_SIZE);
    }

  if (prop & MAXIMUM_CONTENT)
    {
      g_assert_cmpint (child_size, ==, BOX_SIZE);

      max_size -= scrollbar_size;
      g_assert_cmpint (max_size, ==, MAX_SIZE);
    }
}


static void
overlay_automatic_width_min (void)
{
  test_size (TRUE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT);
}

static void
overlay_automatic_height_min (void)
{
  test_size (TRUE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT);
}

static void
overlay_automatic_width_max (void)
{
  test_size (TRUE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_HORIZONTAL, MAXIMUM_CONTENT);
}

static void
overlay_automatic_height_max (void)
{
  test_size (TRUE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_VERTICAL, MAXIMUM_CONTENT);
}

static void
overlay_automatic_width_min_max (void)
{
  test_size (TRUE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}

static void
overlay_automatic_height_min_max (void)
{
  test_size (TRUE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}


static void
nonoverlay_automatic_width_min (void)
{
  test_size (FALSE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT);
}

static void
nonoverlay_automatic_height_min (void)
{
  test_size (FALSE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT);
}

static void
nonoverlay_automatic_width_max (void)
{
  test_size (FALSE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_HORIZONTAL, MAXIMUM_CONTENT);
}

static void
nonoverlay_automatic_height_max (void)
{
  test_size (FALSE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_VERTICAL, MAXIMUM_CONTENT);
}

static void
nonoverlay_automatic_width_min_max (void)
{
  test_size (FALSE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}

static void
nonoverlay_automatic_height_min_max (void)
{
  test_size (FALSE, CTK_POLICY_AUTOMATIC, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}


static void
overlay_always_width_min (void)
{
  test_size (TRUE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT);
}

static void
overlay_always_height_min (void)
{
  test_size (TRUE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT);
}

static void
overlay_always_width_max (void)
{
  test_size (TRUE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_HORIZONTAL, MAXIMUM_CONTENT);
}

static void
overlay_always_height_max (void)
{
  test_size (TRUE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_VERTICAL, MAXIMUM_CONTENT);
}

static void
overlay_always_width_min_max (void)
{
  test_size (TRUE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}

static void
overlay_always_height_min_max (void)
{
  test_size (TRUE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}


static void
nonoverlay_always_width_min (void)
{
  test_size (FALSE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT);
}

static void
nonoverlay_always_height_min (void)
{
  test_size (FALSE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT);
}

static void
nonoverlay_always_width_max (void)
{
  test_size (FALSE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_HORIZONTAL, MAXIMUM_CONTENT);
}

static void
nonoverlay_always_height_max (void)
{
  test_size (FALSE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_VERTICAL, MAXIMUM_CONTENT);
}

static void
nonoverlay_always_width_min_max (void)
{
  test_size (FALSE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_HORIZONTAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}

static void
nonoverlay_always_height_min_max (void)
{
  test_size (FALSE, CTK_POLICY_ALWAYS, CTK_ORIENTATION_VERTICAL, MINIMUM_CONTENT | MAXIMUM_CONTENT);
}


int
main (int argc, char **argv)
{
  ctk_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/sizing/scrolledwindow/overlay_automatic_width_min", overlay_automatic_width_min);
  g_test_add_func ("/sizing/scrolledwindow/overlay_automatic_height_min", overlay_automatic_height_min);
  g_test_add_func ("/sizing/scrolledwindow/overlay_automatic_width_max", overlay_automatic_width_max);
  g_test_add_func ("/sizing/scrolledwindow/overlay_automatic_height_max", overlay_automatic_height_max);
  g_test_add_func ("/sizing/scrolledwindow/overlay_automatic_width_min_max", overlay_automatic_width_min_max);
  g_test_add_func ("/sizing/scrolledwindow/overlay_automatic_height_min_max", overlay_automatic_height_min_max);

  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_automatic_width_min", nonoverlay_automatic_width_min);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_automatic_height_min", nonoverlay_automatic_height_min);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_automatic_width_max", nonoverlay_automatic_width_max);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_automatic_height_max", nonoverlay_automatic_height_max);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_automatic_width_min_max", nonoverlay_automatic_width_min_max);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_automatic_height_min_max", nonoverlay_automatic_height_min_max);

  g_test_add_func ("/sizing/scrolledwindow/overlay_always_width_min", overlay_always_width_min);
  g_test_add_func ("/sizing/scrolledwindow/overlay_always_height_min", overlay_always_height_min);
  g_test_add_func ("/sizing/scrolledwindow/overlay_always_width_max", overlay_always_width_max);
  g_test_add_func ("/sizing/scrolledwindow/overlay_always_height_max", overlay_always_height_max);
  g_test_add_func ("/sizing/scrolledwindow/overlay_always_width_min_max", overlay_always_width_min_max);
  g_test_add_func ("/sizing/scrolledwindow/overlay_always_height_min_max", overlay_always_height_min_max);

  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_always_width_min", nonoverlay_always_width_min);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_always_height_min", nonoverlay_always_height_min);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_always_width_max", nonoverlay_always_width_max);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_always_height_max", nonoverlay_always_height_max);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_always_width_min_max", nonoverlay_always_width_min_max);
  g_test_add_func ("/sizing/scrolledwindow/nonoverlay_always_height_min_max", nonoverlay_always_height_min_max);

  return g_test_run ();
}
