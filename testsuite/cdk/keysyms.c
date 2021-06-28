#include <locale.h>
#include <cdk/cdk.h>

static void
test_keysyms_basic (void)
{
  struct {
    guint keyval;
    const gchar *name;
    const gchar *other_name;
  } tests[] = {
    { CDK_KEY_space, "space", NULL },
    { CDK_KEY_a, "a", NULL },
    { CDK_KEY_Thorn, "Thorn", "THORN" },
    { CDK_KEY_Hangul_J_RieulTieut, "Hangul_J_RieulTieut", NULL },
    { CDK_KEY_Page_Up, "Page_Up", NULL },
    { CDK_KEY_KP_Multiply, "KP_Multiply", NULL },
    { CDK_KEY_MonBrightnessUp, "MonBrightnessUp", NULL },
    { 0, NULL }
  };
  gint i;

 for (i = 0; tests[i].keyval != 0; i++)
   {
     g_assert_cmpstr (cdk_keyval_name (tests[i].keyval), ==, tests[i].name);
     g_assert_cmpuint (cdk_keyval_from_name (tests[i].name), ==, tests[i].keyval);
     if (tests[i].other_name)
       g_assert_cmpuint (cdk_keyval_from_name (tests[i].other_name), ==, tests[i].keyval);
   }
}

static void
test_keysyms_void (void)
{
  g_assert_cmpuint (cdk_keyval_from_name ("NoSuchKeysym"), ==, CDK_KEY_VoidSymbol);
  g_assert_cmpstr (cdk_keyval_name (CDK_KEY_VoidSymbol), ==, "0xffffff");
}

static void
test_keysyms_xf86 (void)
{
  g_assert_cmpuint (cdk_keyval_from_name ("XF86MonBrightnessUp"), ==, CDK_KEY_MonBrightnessUp);
  g_assert_cmpuint (cdk_keyval_from_name ("XF86MonBrightnessDown"), ==, CDK_KEY_MonBrightnessDown);
  g_assert_cmpuint (cdk_keyval_from_name ("XF86KbdBrightnessUp"), ==, CDK_KEY_KbdBrightnessUp);
  g_assert_cmpuint (cdk_keyval_from_name ("XF86KbdBrightnessDown"), ==, CDK_KEY_KbdBrightnessDown);
  g_assert_cmpuint (cdk_keyval_from_name ("XF86Battery"), ==, CDK_KEY_Battery);
  g_assert_cmpuint (cdk_keyval_from_name ("XF86Display"), ==, CDK_KEY_Display);

  g_assert_cmpuint (cdk_keyval_from_name ("MonBrightnessUp"), ==, CDK_KEY_MonBrightnessUp);
  g_assert_cmpuint (cdk_keyval_from_name ("MonBrightnessDown"), ==, CDK_KEY_MonBrightnessDown);
  g_assert_cmpuint (cdk_keyval_from_name ("KbdBrightnessUp"), ==, CDK_KEY_KbdBrightnessUp);
  g_assert_cmpuint (cdk_keyval_from_name ("KbdBrightnessDown"), ==, CDK_KEY_KbdBrightnessDown);
  g_assert_cmpuint (cdk_keyval_from_name ("Battery"), ==, CDK_KEY_Battery);
  g_assert_cmpuint (cdk_keyval_from_name ("Display"), ==, CDK_KEY_Display);
}

int main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);
  cdk_init (&argc, &argv);

  g_test_add_func ("/keysyms/basic", test_keysyms_basic);
  g_test_add_func ("/keysyms/void", test_keysyms_void);
  g_test_add_func ("/keysyms/xf86", test_keysyms_xf86);

  return g_test_run ();
}
