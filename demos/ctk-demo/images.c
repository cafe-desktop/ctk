/* Images
 *
 * CtkImage is used to display an image; the image can be in a number of formats.
 * Typically, you load an image into a GdkPixbuf, then display the pixbuf.
 *
 * This demo code shows some of the more obscure cases, in the simple
 * case a call to ctk_image_new_from_file() is all you need.
 *
 * If you want to put image data in your program as a C variable,
 * use the make-inline-pixbuf program that comes with CTK+.
 * This way you won't need to depend on loading external files, your
 * application binary can be self-contained.
 */

#include <ctk/ctk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <errno.h>

static CtkWidget *window = NULL;
static GdkPixbufLoader *pixbuf_loader = NULL;
static guint load_timeout = 0;
static GInputStream * image_stream = NULL;

static void
progressive_prepared_callback (GdkPixbufLoader *loader,
                               gpointer         data)
{
  GdkPixbuf *pixbuf;
  CtkWidget *image;

  image = CTK_WIDGET (data);

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

  /* Avoid displaying random memory contents, since the pixbuf
   * isn't filled in yet.
   */
  gdk_pixbuf_fill (pixbuf, 0xaaaaaaff);

  ctk_image_set_from_pixbuf (CTK_IMAGE (image), pixbuf);
}

static void
progressive_updated_callback (GdkPixbufLoader *loader G_GNUC_UNUSED,
                              gint                 x G_GNUC_UNUSED,
                              gint                 y G_GNUC_UNUSED,
                              gint                 width G_GNUC_UNUSED,
                              gint                 height G_GNUC_UNUSED,
                              gpointer     data)
{
  CtkWidget *image;
  GdkPixbuf *pixbuf;

  image = CTK_WIDGET (data);

  /* We know the pixbuf inside the CtkImage has changed, but the image
   * itself doesn't know this; so give it a hint by setting the pixbuf
   * again. Queuing a redraw used to be sufficient, but nowadays CtkImage
   * uses CtkIconHelper which caches the pixbuf state and will just redraw
   * from the cache.
   */

  pixbuf = ctk_image_get_pixbuf (CTK_IMAGE (image));
  g_object_ref (pixbuf);
  ctk_image_set_from_pixbuf (CTK_IMAGE (image), pixbuf);
  g_object_unref (pixbuf);
}

static gint
progressive_timeout (gpointer data)
{
  CtkWidget *image;

  image = CTK_WIDGET (data);

  /* This shows off fully-paranoid error handling, so looks scary.
   * You could factor out the error handling code into a nice separate
   * function to make things nicer.
   */

  if (image_stream)
    {
      gssize bytes_read;
      guchar buf[256];
      GError *error = NULL;

      bytes_read = g_input_stream_read (image_stream, buf, 256, NULL, &error);

      if (bytes_read < 0)
        {
          CtkWidget *dialog;

          dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                           CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_CLOSE,
                                           "Failure reading image file 'alphatest.png': %s",
                                           error->message);
          g_error_free (error);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (ctk_widget_destroy), NULL);

          g_object_unref (image_stream);
          image_stream = NULL;

          ctk_widget_show (dialog);

          load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (!gdk_pixbuf_loader_write (pixbuf_loader,
                                    buf, bytes_read,
                                    &error))
        {
          CtkWidget *dialog;

          dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                           CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_CLOSE,
                                           "Failed to load image: %s",
                                           error->message);

          g_error_free (error);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (ctk_widget_destroy), NULL);

          g_object_unref (image_stream);
          image_stream = NULL;

          ctk_widget_show (dialog);

          load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (bytes_read == 0)
        {
          /* Errors can happen on close, e.g. if the image
           * file was truncated we'll know on close that
           * it was incomplete.
           */
          error = NULL;
          if (!g_input_stream_close (image_stream, NULL, &error))
            {
              CtkWidget *dialog;

              dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                               CTK_DIALOG_DESTROY_WITH_PARENT,
                                               CTK_MESSAGE_ERROR,
                                               CTK_BUTTONS_CLOSE,
                                               "Failed to load image: %s",
                                               error->message);

              g_error_free (error);

              g_signal_connect (dialog, "response",
                                G_CALLBACK (ctk_widget_destroy), NULL);

              ctk_widget_show (dialog);

              g_object_unref (image_stream);
              image_stream = NULL;
              g_object_unref (pixbuf_loader);
              pixbuf_loader = NULL;

              load_timeout = 0;

              return FALSE; /* uninstall the timeout */
            }

          g_object_unref (image_stream);
          image_stream = NULL;

          /* Errors can happen on close, e.g. if the image
           * file was truncated we'll know on close that
           * it was incomplete.
           */
          error = NULL;
          if (!gdk_pixbuf_loader_close (pixbuf_loader,
                                        &error))
            {
              CtkWidget *dialog;

              dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                               CTK_DIALOG_DESTROY_WITH_PARENT,
                                               CTK_MESSAGE_ERROR,
                                               CTK_BUTTONS_CLOSE,
                                               "Failed to load image: %s",
                                               error->message);

              g_error_free (error);

              g_signal_connect (dialog, "response",
                                G_CALLBACK (ctk_widget_destroy), NULL);

              ctk_widget_show (dialog);

              g_object_unref (pixbuf_loader);
              pixbuf_loader = NULL;

              load_timeout = 0;

              return FALSE; /* uninstall the timeout */
            }

          g_object_unref (pixbuf_loader);
          pixbuf_loader = NULL;
        }
    }
  else
    {
      GError *error = NULL;

      image_stream = g_resources_open_stream ("/images/alphatest.png", 0, &error);

      if (image_stream == NULL)
        {
          CtkWidget *dialog;

          dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                           CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_CLOSE,
                                           "%s", error->message);
          g_error_free (error);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (ctk_widget_destroy), NULL);

          ctk_widget_show (dialog);

          load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (pixbuf_loader)
        {
          gdk_pixbuf_loader_close (pixbuf_loader, NULL);
          g_object_unref (pixbuf_loader);
        }

      pixbuf_loader = gdk_pixbuf_loader_new ();

      g_signal_connect (pixbuf_loader, "area-prepared",
                        G_CALLBACK (progressive_prepared_callback), image);

      g_signal_connect (pixbuf_loader, "area-updated",
                        G_CALLBACK (progressive_updated_callback), image);
    }

  /* leave timeout installed */
  return TRUE;
}

static void
start_progressive_loading (CtkWidget *image)
{
  /* This is obviously totally contrived (we slow down loading
   * on purpose to show how incremental loading works).
   * The real purpose of incremental loading is the case where
   * you are reading data from a slow source such as the network.
   * The timeout simply simulates a slow data source by inserting
   * pauses in the reading process.
   */
  load_timeout = cdk_threads_add_timeout (150,
                                progressive_timeout,
                                image);
  g_source_set_name_by_id (load_timeout, "[ctk+] progressive_timeout");
}

static void
cleanup_callback (GObject   *object G_GNUC_UNUSED,
                  gpointer   data G_GNUC_UNUSED)
{
  if (load_timeout)
    {
      g_source_remove (load_timeout);
      load_timeout = 0;
    }

  if (pixbuf_loader)
    {
      gdk_pixbuf_loader_close (pixbuf_loader, NULL);
      g_object_unref (pixbuf_loader);
      pixbuf_loader = NULL;
    }

  if (image_stream)
    {
      g_object_unref (image_stream);
      image_stream = NULL;
    }
}

static void
toggle_sensitivity_callback (CtkWidget *togglebutton,
                             gpointer   user_data)
{
  CtkContainer *container = user_data;
  GList *list;
  GList *tmp;

  list = ctk_container_get_children (container);

  tmp = list;
  while (tmp != NULL)
    {
      /* don't disable our toggle */
      if (CTK_WIDGET (tmp->data) != togglebutton)
        ctk_widget_set_sensitive (CTK_WIDGET (tmp->data),
                                  !ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (togglebutton)));

      tmp = tmp->next;
    }

  g_list_free (list);
}


CtkWidget *
do_images (CtkWidget *do_widget)
{
  if (!window)
    {
      CtkWidget *frame;
      CtkWidget *vbox;
      CtkWidget *image;
      CtkWidget *label;
      CtkWidget *button;
      GIcon     *gicon;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Images");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (cleanup_callback), NULL);

      ctk_container_set_border_width (CTK_CONTAINER (window), 8);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
                            "<u>Image loaded from a file</u>");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
      ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      image = ctk_image_new_from_icon_name ("ctk3-demo", CTK_ICON_SIZE_DIALOG);

      ctk_container_add (CTK_CONTAINER (frame), image);


      /* Animation */

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
                            "<u>Animation loaded from a file</u>");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
      ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      image = ctk_image_new_from_resource ("/images/floppybuddy.gif");

      ctk_container_add (CTK_CONTAINER (frame), image);

      /* Symbolic icon */

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
                            "<u>Symbolic themed icon</u>");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
      ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      gicon = g_themed_icon_new_with_default_fallbacks ("battery-caution-charging-symbolic");
      image = ctk_image_new_from_gicon (gicon, CTK_ICON_SIZE_DIALOG);

      ctk_container_add (CTK_CONTAINER (frame), image);


      /* Progressive */

      label = ctk_label_new (NULL);
      ctk_label_set_markup (CTK_LABEL (label),
                            "<u>Progressive image loading</u>");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
      ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

      /* Create an empty image for now; the progressive loader
       * will create the pixbuf and fill it in.
       */
      image = ctk_image_new_from_pixbuf (NULL);
      ctk_container_add (CTK_CONTAINER (frame), image);

      start_progressive_loading (image);

      /* Sensitivity control */
      button = ctk_toggle_button_new_with_mnemonic ("_Insensitive");
      ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (toggle_sensitivity_callback),
                        vbox);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
