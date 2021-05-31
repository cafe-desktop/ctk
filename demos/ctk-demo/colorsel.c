/* Color Chooser
 *
 * A CtkColorChooser lets the user choose a color. There are several
 * implementations of the CtkColorChooser interface in GTK+. The
 * CtkColorChooserDialog is a prebuilt dialog containing a
 * CtkColorChooserWidget.
 */

#include <ctk/ctk.h>

static CtkWidget *window = NULL;
static CtkWidget *da;
static GdkRGBA color;
static CtkWidget *frame;

/* draw callback for the drawing area
 */
static gboolean
draw_callback (CtkWidget *widget,
               cairo_t   *cr,
               gpointer   data)
{
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_paint (cr);

  return TRUE;
}

static void
response_cb (CtkDialog *dialog,
             gint       response_id,
             gpointer   user_data)
{
  if (response_id == CTK_RESPONSE_OK)
    ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (dialog), &color);

  ctk_widget_destroy (CTK_WIDGET (dialog));
}

static void
change_color_callback (CtkWidget *button,
                       gpointer   data)
{
  CtkWidget *dialog;

  dialog = ctk_color_chooser_dialog_new ("Changing color", CTK_WINDOW (window));
  ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);
  ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (dialog), &color);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (response_cb), NULL);

  ctk_widget_show_all (dialog);
}

CtkWidget *
do_colorsel (CtkWidget *do_widget)
{
  CtkWidget *vbox;
  CtkWidget *button;

  if (!window)
    {
      color.red = 0;
      color.blue = 1;
      color.green = 0;
      color.alpha = 1;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Color Chooser");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_container_set_border_width (CTK_CONTAINER (window), 8);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      /*
       * Create the color swatch area
       */

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
      ctk_box_pack_start (CTK_BOX (vbox), frame, TRUE, TRUE, 0);

      da = ctk_drawing_area_new ();

      g_signal_connect (da, "draw", G_CALLBACK (draw_callback), NULL);

      /* set a minimum size */
      ctk_widget_set_size_request (da, 200, 200);

      ctk_container_add (CTK_CONTAINER (frame), da);

      button = ctk_button_new_with_mnemonic ("_Change the above color");
      ctk_widget_set_halign (button, CTK_ALIGN_END);
      ctk_widget_set_valign (button, CTK_ALIGN_CENTER);

      ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (change_color_callback), NULL);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
