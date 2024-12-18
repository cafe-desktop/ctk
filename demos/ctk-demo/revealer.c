/* Revealer
 *
 * CtkRevealer is a container that animates showing and hiding
 * of its sole child with nice transitions.
 */

#include <ctk/ctk.h>

static gint count = 0;
static guint timeout = 0;

static void
change_direction (CtkRevealer *revealer)
{
  if (ctk_widget_get_mapped (CTK_WIDGET (revealer)))
    {
      gboolean revealed;

      revealed = ctk_revealer_get_child_revealed (revealer);
      ctk_revealer_set_reveal_child (revealer, !revealed);
    }
}

static gboolean
reveal_one (gpointer data)
{
  CtkWidget *window = data;
  CtkBuilder *builder;
  gchar *name;
  CtkRevealer *revealer;

  builder = CTK_BUILDER (g_object_get_data (G_OBJECT (window), "builder"));
  name = g_strdup_printf ("revealer%d", count);
  revealer = (CtkRevealer *)ctk_builder_get_object (builder, name);

  ctk_revealer_set_reveal_child (revealer, TRUE);

  g_signal_connect (revealer, "notify::child-revealed",
                    G_CALLBACK (change_direction), NULL);
  count++;

  if (count >= 9)
    {
      timeout = 0;
      return FALSE;
    }
  else
    return TRUE;
}

static CtkWidget *window = NULL;

static void
on_destroy (gpointer data G_GNUC_UNUSED)
{
  window = NULL;
  if (timeout != 0)
    {
      g_source_remove (timeout);
      timeout = 0;
    }
}

CtkWidget *
do_revealer (CtkWidget *do_widget)
{
  if (!window)
    {
      CtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/revealer/revealer.ui");
      ctk_builder_connect_signals (builder, NULL);
      window = CTK_WIDGET (ctk_builder_get_object (builder, "window"));
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (on_destroy), NULL);
      g_object_set_data_full (G_OBJECT (window), "builder", builder, g_object_unref);
    }

  if (!ctk_widget_get_visible (window))
    {
      count = 0;
      timeout = g_timeout_add (690, reveal_one, window);
      ctk_widget_show_all (window);
    }
  else
    {
      ctk_widget_destroy (window);
    }


  return window;
}
