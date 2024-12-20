#include <ctk/ctk.h>

static gint value_changed_count;

static void
value_changed_cb (CtkSpinButton *spin G_GNUC_UNUSED)
{
  value_changed_count++;
}

static void
test_value_changed (void)
{
  CtkWidget *spin;
  CtkAdjustment *adj;

  spin = ctk_spin_button_new_with_range (0.0, 10.0, 1.0);

  g_signal_connect (spin, "value-changed", G_CALLBACK (value_changed_cb), NULL);

  value_changed_count = 0;
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (spin), 1.0);
  g_assert_cmpint (value_changed_count, ==, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (spin), 2.0);
  g_assert_cmpint (value_changed_count, ==, 2);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (spin), 2.0);
  g_assert_cmpint (value_changed_count, ==, 2);

  ctk_spin_button_spin (CTK_SPIN_BUTTON (spin), CTK_SPIN_STEP_FORWARD, 0.5);
  g_assert_cmpint (value_changed_count, ==, 3);

  ctk_spin_button_configure (CTK_SPIN_BUTTON (spin), NULL, 1.0, 0);
  g_assert_cmpint (value_changed_count, ==, 4);

  adj = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (spin));
  ctk_adjustment_set_value (adj, 0.0);
  g_assert_cmpint (value_changed_count, ==, 5);
}

static gint adjustment_changed_count;

static void
adjustment_changed_cb (GObject    *object G_GNUC_UNUSED,
		       GParamSpec *pspec G_GNUC_UNUSED)
{
  adjustment_changed_count++;
}

static void
test_adjustment_changed (void)
{
  CtkWidget *spin;
  CtkAdjustment *adj;

  spin = ctk_spin_button_new_with_range (0.0, 10.0, 1.0);

  g_signal_connect (spin, "notify::adjustment", G_CALLBACK (adjustment_changed_cb), NULL);

  adjustment_changed_count = 0;
  adj = ctk_adjustment_new (50.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  ctk_spin_button_configure (CTK_SPIN_BUTTON (spin), adj, 1.0, 0);
  g_assert_cmpint (adjustment_changed_count, ==, 1);

  adj = ctk_adjustment_new (51.0, 1.0, 101.0, 1.0, 1.0, 0.0);
  ctk_spin_button_set_adjustment (CTK_SPIN_BUTTON (spin), adj);
  g_assert_cmpint (adjustment_changed_count, ==, 2);

  ctk_spin_button_set_range (CTK_SPIN_BUTTON (spin), 2.0, 102.0);
  g_assert_cmpint (adjustment_changed_count, ==, 2);
}

static void
test_adjustment_null (void)
{
  CtkWidget *spin;
  CtkAdjustment *adj;

  spin = ctk_spin_button_new_with_range (0.0, 10.0, 1.0);

  adj = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (spin));
  ctk_spin_button_configure (CTK_SPIN_BUTTON (spin), NULL, 1.0, 0);
  g_assert (adj == ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (spin)));

  ctk_spin_button_set_adjustment (CTK_SPIN_BUTTON (spin), NULL);
  adj = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (spin));
  g_assert_cmpfloat (ctk_adjustment_get_lower (adj), ==, 0.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (adj), ==, 0.0);
  g_assert_cmpfloat (ctk_adjustment_get_upper (adj), ==, 0.0);
}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/spinbutton/value-changed", test_value_changed);
  g_test_add_func ("/spinbutton/adjustment-changed", test_adjustment_changed);
  g_test_add_func ("/spinbutton/adjustment-null", test_adjustment_null);

  return g_test_run ();
}
