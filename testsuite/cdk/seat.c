#include <cdk/cdk.h>

static void
test_list_seats (void)
{
  CdkDisplay *display;
  CdkSeat *seat0, *seat;
  GList *list, *l;
  gboolean found_default;

  display = cdk_display_get_default ();
  seat0 = cdk_display_get_default_seat (display);

  g_assert_true (CDK_IS_SEAT (seat0));

  found_default = FALSE;
  list = cdk_display_list_seats (display);
  for (l = list; l; l = l->next)
    {
      seat = l->data;

      g_assert_true (CDK_IS_SEAT (seat));
      g_assert (cdk_seat_get_display (seat) == display);

      if (seat == seat0)
        found_default = TRUE;
    }
  g_list_free (list);

  g_assert_true (found_default);
}

static void
test_default_seat (void)
{
  CdkDisplay *display;
  CdkSeat *seat0;
  CdkSeatCapabilities caps;
  CdkDevice *pointer0, *keyboard0, *device;
  GList *slaves, *l;

  display = cdk_display_get_default ();
  seat0 = cdk_display_get_default_seat (display);

  g_assert_true (CDK_IS_SEAT (seat0));

  caps = cdk_seat_get_capabilities (seat0);

  g_assert (caps != CDK_SEAT_CAPABILITY_NONE);

  pointer0 = cdk_seat_get_pointer (seat0);
  slaves = cdk_seat_get_slaves (seat0, CDK_SEAT_CAPABILITY_POINTER);

  if ((caps & CDK_SEAT_CAPABILITY_POINTER) != 0)
    {
      g_assert_nonnull (pointer0);
      g_assert (cdk_device_get_device_type (pointer0) == CDK_DEVICE_TYPE_MASTER);
      g_assert (cdk_device_get_display (pointer0) == display);
      g_assert (cdk_device_get_seat (pointer0) == seat0);

      g_assert_nonnull (slaves);
      for (l = slaves; l; l = l->next)
        {
          device = l->data;
          g_assert (cdk_device_get_device_type (device) == CDK_DEVICE_TYPE_SLAVE);
          g_assert (cdk_device_get_display (device) == display);
          g_assert (cdk_device_get_seat (device) == seat0);
        }
      g_list_free (slaves);
    }
  else
    {
      g_assert_null (pointer0);
      g_assert_null (slaves);
    }

  keyboard0 = cdk_seat_get_keyboard (seat0);
  slaves = cdk_seat_get_slaves (seat0, CDK_SEAT_CAPABILITY_KEYBOARD);

  if ((caps & CDK_SEAT_CAPABILITY_KEYBOARD) != 0)
    {
      g_assert_nonnull (keyboard0);
      g_assert (cdk_device_get_device_type (keyboard0) == CDK_DEVICE_TYPE_MASTER);
      g_assert (cdk_device_get_display (keyboard0) == display);
      g_assert (cdk_device_get_seat (keyboard0) == seat0);
      g_assert (cdk_device_get_source (keyboard0) == CDK_SOURCE_KEYBOARD);

      g_assert_nonnull (slaves);
      for (l = slaves; l; l = l->next)
        {
          device = l->data;
          g_assert (cdk_device_get_device_type (device) == CDK_DEVICE_TYPE_SLAVE);
          g_assert (cdk_device_get_display (device) == display);
          g_assert (cdk_device_get_seat (device) == seat0);
          g_assert (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD);
        }
      g_list_free (slaves);
    }
  else
    {
      g_assert_null (keyboard0);
      g_assert_null (slaves);
    }

}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  cdk_init (NULL, NULL);

  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/seat/list", test_list_seats);
  g_test_add_func ("/seat/default", test_default_seat);

  return g_test_run ();
}
