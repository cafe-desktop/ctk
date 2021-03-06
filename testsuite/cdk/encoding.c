#include <cdk/cdk.h>
#ifdef CDK_WINDOWING_X11
#include <cdk/x11/cdkx.h>
#endif

static void
test_to_text_list (void)
{
#ifdef CDK_WINDOWING_X11
  CdkDisplay *display;

  display = cdk_display_get_default ();

  if (CDK_IS_X11_DISPLAY (display))
    {
      CdkAtom encoding;
      gint format;
      const guchar *text;
      gint length;
      gchar **list;
      gint n;

      encoding = cdk_atom_intern ("UTF8_STRING", FALSE);
      format = 8;
      text = (const guchar*)"abcdef \304\201 \304\205\0ABCDEF \304\200 \304\204";
      length = 25;
      n = cdk_x11_display_text_property_to_text_list (display, encoding, format, text, length, &list);
      g_assert_cmpint (n, ==, 2);
      g_assert (g_str_has_prefix (list[0], "abcdef "));
      g_assert (g_str_has_prefix (list[1], "ABCDEF "));

      cdk_x11_free_text_list (list);
    }
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  cdk_init (&argc, &argv);

  g_test_add_func ("/encoding/to-text-list", test_to_text_list);

  return g_test_run ();
}
