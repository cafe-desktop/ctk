#include <ctk/ctk.h>

static void
print_hello (CtkWidget *widget G_GNUC_UNUSED,
             gpointer   data G_GNUC_UNUSED)
{
  g_print ("Hello World\n");
}

int
main (int   argc,
      char *argv[])
{
  CtkBuilder *builder;
  GObject *window;
  GObject *button;
  GError *error = NULL;

  ctk_init (&argc, &argv);

  /* Construct a CtkBuilder instance and load our UI description */
  builder = ctk_builder_new ();
  if (ctk_builder_add_from_file (builder, "builder.ui", &error) == 0)
    {
      g_printerr ("Error loading file: %s\n", error->message);
      g_clear_error (&error);
      return 1;
    }

  /* Connect signal handlers to the constructed widgets. */
  window = ctk_builder_get_object (builder, "window");
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);

  button = ctk_builder_get_object (builder, "button1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = ctk_builder_get_object (builder, "button2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = ctk_builder_get_object (builder, "quit");
  g_signal_connect (button, "clicked", G_CALLBACK (ctk_main_quit), NULL);

  ctk_main ();

  return 0;
}
