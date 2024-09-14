/* Application Class
 *
 * Demonstrates a simple application.
 *
 * This example uses CtkApplication, CtkApplicationWindow, CtkBuilder
 * as well as GMenu and GResource. Due to the way CtkApplication is structured,
 * it is run as a separate process.
 */

#include "config.h"

#include <ctk/ctk.h>

static gboolean name_seen;
static CtkWidget *placeholder;

static void
on_name_appeared (GDBusConnection *connection G_GNUC_UNUSED,
                  const gchar     *name G_GNUC_UNUSED,
                  const gchar     *name_owner G_GNUC_UNUSED,
                  gpointer         user_data G_GNUC_UNUSED)
{
  name_seen = TRUE;
}

static void
on_name_vanished (GDBusConnection *connection G_GNUC_UNUSED,
                  const gchar     *name G_GNUC_UNUSED,
                  gpointer         user_data G_GNUC_UNUSED)
{
  if (!name_seen)
    return;

  if (placeholder)
    {
      ctk_widget_destroy (placeholder);
      g_object_unref (placeholder);
      placeholder = NULL;
    }
}

#ifdef G_OS_WIN32
#define APP_EXTENSION ".exe"
#else
#define APP_EXTENSION
#endif

CtkWidget *
do_application_demo (CtkWidget *toplevel G_GNUC_UNUSED)
{
  static guint watch = 0;

  if (watch == 0)
    watch = g_bus_watch_name (G_BUS_TYPE_SESSION,
                              "org.ctk.Demo2",
                              0,
                              on_name_appeared,
                              on_name_vanished,
                              NULL, NULL);

  if (placeholder == NULL)
    {
      const gchar *command;
      GError *error = NULL;

      if (g_file_test ("./ctk3-demo-application" APP_EXTENSION, G_FILE_TEST_IS_EXECUTABLE))
        command = "./ctk3-demo-application" APP_EXTENSION;
      else
        command = "ctk3-demo-application";

      if (!g_spawn_command_line_async (command, &error))
        {
          g_warning ("%s", error->message);
          g_error_free (error);
        }

      placeholder = ctk_label_new ("");
      g_object_ref_sink (placeholder);
    }
  else
    {
      g_dbus_connection_call_sync (g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL),
                                   "org.ctk.Demo2",
                                   "/org/ctk/Demo2",
                                   "org.ctk.Actions",
                                   "Activate",
                                   g_variant_new ("(sava{sv})", "quit", NULL, NULL),
                                   NULL,
                                   0,
                                   G_MAXINT,
                                   NULL, NULL);
    }

  return placeholder;
}
