#include <ctk/ctk.h>

static void
test_type (GType t)
{
  GtkWidget *w;
  AtkObject *a;

  if (g_type_is_a (t, CTK_TYPE_WIDGET))
    {
      w = (GtkWidget *)g_object_new (t, NULL);
      a = ctk_widget_get_accessible (w);

      g_assert (CTK_IS_ACCESSIBLE (a));
      g_assert (ctk_accessible_get_widget (CTK_ACCESSIBLE (a)) == w);

      g_object_unref (w);
    }
}

int
main (int argc, char *argv[])
{
  const GType *tp;
  guint i, n;

  ctk_init (&argc, &argv);

  tp = ctk_test_list_all_types (&n);

  for (i = 0; i < n; i++)
    test_type (tp[i]);

  return 0;
}
