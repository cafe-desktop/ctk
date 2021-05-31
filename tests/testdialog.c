#include <ctk/ctk.h>

static void
show_message_dialog1 (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = CTK_WIDGET (ctk_message_dialog_new (parent,
                                               CTK_DIALOG_MODAL|
                                               CTK_DIALOG_DESTROY_WITH_PARENT|
                                               CTK_DIALOG_USE_HEADER_BAR,
                                               CTK_MESSAGE_INFO,
                                               CTK_BUTTONS_OK,
                                               "Oops! Something went wrong."));
  ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                            "Unhandled error message: SSH program unexpectedly exited");

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_message_dialog1a (GtkWindow *parent)
{
  GtkWidget *dialog;
  GtkWidget *image;

  dialog = CTK_WIDGET (ctk_message_dialog_new (parent,
                                               CTK_DIALOG_MODAL|
                                               CTK_DIALOG_DESTROY_WITH_PARENT|
                                               CTK_DIALOG_USE_HEADER_BAR,
                                               CTK_MESSAGE_INFO,
                                               CTK_BUTTONS_OK,
                                               "The system network services are not compatible with this version."));

  image = ctk_image_new_from_icon_name ("computer-fail", CTK_ICON_SIZE_DIALOG);
  ctk_widget_show (image);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_message_dialog_set_image (CTK_MESSAGE_DIALOG (dialog), image);
  G_GNUC_END_IGNORE_DEPRECATIONS;


  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_message_dialog2 (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = CTK_WIDGET (ctk_message_dialog_new (parent,
                                               CTK_DIALOG_MODAL|
                                               CTK_DIALOG_DESTROY_WITH_PARENT|
                                               CTK_DIALOG_USE_HEADER_BAR,
                                               CTK_MESSAGE_INFO,
                                               CTK_BUTTONS_NONE,
                                               "Empty all items from Wastebasket?"));
  ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                            "All items in the Wastebasket will be permanently deleted");
  ctk_dialog_add_buttons (CTK_DIALOG (dialog), 
                          "Cancel", CTK_RESPONSE_CANCEL,
                          "Empty Wastebasket", CTK_RESPONSE_OK,
                          NULL);  

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_color_chooser (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = ctk_color_chooser_dialog_new ("Builtin", parent);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_color_chooser_generic (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = g_object_new (CTK_TYPE_COLOR_CHOOSER_DIALOG,
                         "title", "Generic Builtin",
                         "transient-for", parent,
                         NULL);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
add_content (GtkWidget *dialog)
{
  GtkWidget *label;

  label = ctk_label_new ("content");
  g_object_set (label, "margin", 50, NULL);
  ctk_widget_show (label);

  ctk_container_add (CTK_CONTAINER (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), label);
}

static void
add_buttons (GtkWidget *dialog)
{
  ctk_dialog_add_button (CTK_DIALOG (dialog), "Done", CTK_RESPONSE_OK);
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
}

static void
show_dialog (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = ctk_dialog_new_with_buttons ("Simple", parent, 
					CTK_DIALOG_MODAL|CTK_DIALOG_DESTROY_WITH_PARENT,
				        "Close", CTK_RESPONSE_CLOSE,
                                        NULL);

  add_content (dialog);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_dialog_with_header (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = ctk_dialog_new_with_buttons ("With Header", parent, 
					CTK_DIALOG_MODAL|CTK_DIALOG_DESTROY_WITH_PARENT|CTK_DIALOG_USE_HEADER_BAR,
				        "Close", CTK_RESPONSE_CLOSE,
                                        NULL);

  add_content (dialog);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_dialog_with_buttons (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = ctk_dialog_new_with_buttons ("With Buttons", parent, 
					CTK_DIALOG_MODAL|CTK_DIALOG_DESTROY_WITH_PARENT,
				        "Close", CTK_RESPONSE_CLOSE,
				        "Frob", 25,
                                        NULL);

  add_content (dialog);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_dialog_with_header_buttons (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = ctk_dialog_new_with_buttons ("Header & Buttons", parent, 
					CTK_DIALOG_MODAL|CTK_DIALOG_DESTROY_WITH_PARENT|CTK_DIALOG_USE_HEADER_BAR,
				        "Close", CTK_RESPONSE_CLOSE,
				        "Frob", 25,
                                        NULL);

  add_content (dialog);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_dialog_with_header_buttons2 (GtkWindow *parent)
{
  GtkBuilder *builder;
  GtkWidget *dialog;

  builder = ctk_builder_new ();
  ctk_builder_add_from_file (builder, "dialog.ui", NULL);
  dialog = (GtkWidget *)ctk_builder_get_object (builder, "dialog");
  g_object_unref (builder);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

typedef struct {
  GtkDialog parent;
} MyDialog;

typedef struct {
  GtkDialogClass parent_class;
} MyDialogClass;

G_DEFINE_TYPE (MyDialog, my_dialog, CTK_TYPE_DIALOG);

static void
my_dialog_init (MyDialog *dialog)
{
  ctk_widget_init_template (CTK_WIDGET (dialog));
}

static void
my_dialog_class_init (MyDialogClass *class)
{
  gchar *buffer;
  gsize size;
  GBytes *bytes;

  if (!g_file_get_contents ("mydialog.ui", &buffer, &size, NULL))
    g_error ("Template file mydialog.ui not found");

  bytes = g_bytes_new_static (buffer, size);
  ctk_widget_class_set_template (CTK_WIDGET_CLASS (class), bytes);
  g_bytes_unref (bytes);
}

static void
show_dialog_from_template (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = g_object_new (my_dialog_get_type (),
                         "title", "Template",
                         "transient-for", parent,
                         NULL);

  add_content (dialog);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

static void
show_dialog_flex_template (GtkWindow *parent)
{
  GtkWidget *dialog;
  gboolean use_header;

  g_object_get (ctk_settings_get_default (),
                "ctk-dialogs-use-header", &use_header,
                NULL);
  dialog = g_object_new (my_dialog_get_type (),
                         "title", "Flexible Template",
                         "transient-for", parent,
                         "use-header-bar", use_header,
                         NULL);

  add_content (dialog);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

typedef struct {
  GtkDialog parent;

  GtkWidget *content;
} MyDialog2;

typedef struct {
  GtkDialogClass parent_class;
} MyDialog2Class;

G_DEFINE_TYPE (MyDialog2, my_dialog2, CTK_TYPE_DIALOG);

static void
my_dialog2_init (MyDialog2 *dialog)
{
  ctk_widget_init_template (CTK_WIDGET (dialog));
}

static void
my_dialog2_class_init (MyDialog2Class *class)
{
  gchar *buffer;
  gsize size;
  GBytes *bytes;

  if (!g_file_get_contents ("mydialog2.ui", &buffer, &size, NULL))
    g_error ("Template file mydialog2.ui not found");

  bytes = g_bytes_new_static (buffer, size);
  ctk_widget_class_set_template (CTK_WIDGET_CLASS (class), bytes);
  g_bytes_unref (bytes);

  ctk_widget_class_bind_template_child (CTK_WIDGET_CLASS (class), MyDialog2, content);
}

static void
show_dialog_from_template_with_header (GtkWindow *parent)
{
  GtkWidget *dialog;

  dialog = g_object_new (my_dialog2_get_type (),
                         "transient-for", parent,
                         "use-header-bar", TRUE,
                         NULL);

  add_buttons (dialog);
  add_content (dialog);

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *button;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 600, 400);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_widget_set_halign (vbox, CTK_ALIGN_FILL);
  ctk_widget_set_valign (vbox, CTK_ALIGN_CENTER);
  ctk_widget_show (vbox);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  
  box = ctk_flow_box_new ();
  ctk_flow_box_set_selection_mode (CTK_FLOW_BOX (box), CTK_SELECTION_NONE);
  ctk_widget_set_hexpand (box, TRUE);
  ctk_widget_show (box);
  ctk_container_add (CTK_CONTAINER (vbox), box);

  button = ctk_button_new_with_label ("Message dialog");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_message_dialog1), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Message with icon");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_message_dialog1a), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Confirmation dialog");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_message_dialog2), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Builtin");
  button = ctk_button_new_with_label ("Builtin");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_color_chooser), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Generic Builtin");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_color_chooser_generic), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Simple");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("With Header");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_header), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("With Buttons");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_buttons), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Header & Buttons");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_header_buttons), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Header & Buttons & Builder");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_header_buttons2), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Template");
  button = ctk_button_new_with_label ("Template");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_from_template), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Template With Header");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_from_template_with_header), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_button_new_with_label ("Flexible Template");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_flex_template), window);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);

  button = ctk_check_button_new_with_label ("Dialogs have headers");
  g_object_bind_property (ctk_settings_get_default (), "ctk-dialogs-use-header",
                          button, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  ctk_widget_show (button);
  ctk_widget_set_halign (button, CTK_ALIGN_CENTER);
  ctk_container_add (CTK_CONTAINER (vbox), button);

  button = ctk_spinner_new ();
  ctk_spinner_start (CTK_SPINNER (button));
  ctk_widget_show (button);
  ctk_widget_set_halign (button, CTK_ALIGN_CENTER);
  ctk_container_add (CTK_CONTAINER (vbox), button);

  ctk_widget_show (window);
  ctk_main ();
  
  return 0;
}

