
/* testpixbuf -- test program for cdk-pixbuf code
 * Copyright (C) 1999 Mark Crichton, Larry Ewing
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctk/ctk.h>

#ifndef G_OS_WIN32
# include <unistd.h>
#endif

typedef struct _LoadContext LoadContext;

struct _LoadContext
{
  gchar *filename;
  CtkWidget *window;
  GdkPixbufLoader *pixbuf_loader;
  guint load_timeout;
  FILE* image_stream;
};

static void
destroy_context (gpointer data)
{
  LoadContext *lc = data;

  g_free (lc->filename);
  
  if (lc->load_timeout)
    g_source_remove (lc->load_timeout);

  if (lc->image_stream)
    fclose (lc->image_stream);

  if (lc->pixbuf_loader)
    {
      gdk_pixbuf_loader_close (lc->pixbuf_loader, NULL);
      g_object_unref (lc->pixbuf_loader);
    }
  
  g_free (lc);
}

static LoadContext*
get_load_context (CtkWidget *image)
{
  LoadContext *lc;

  lc = g_object_get_data (G_OBJECT (image), "lc");

  if (lc == NULL)
    {
      lc = g_new0 (LoadContext, 1);

      g_object_set_data_full (G_OBJECT (image),        
                              "lc",
                              lc,
                              destroy_context);
    }

  return lc;
}

static void
progressive_prepared_callback (GdkPixbufLoader* loader,
                               gpointer         data)
{
  GdkPixbuf* pixbuf;
  CtkWidget* image;

  image = CTK_WIDGET (data);
    
  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

  /* Avoid displaying random memory contents, since the pixbuf
   * isn't filled in yet.
   */
  gdk_pixbuf_fill (pixbuf, 0xaaaaaaff);

  /* Could set the pixbuf instead, if we only wanted to display
   * static images.
   */
  ctk_image_set_from_animation (CTK_IMAGE (image),
                                gdk_pixbuf_loader_get_animation (loader));
}

static void
progressive_updated_callback (GdkPixbufLoader* loader,
                              gint x, gint y, gint width, gint height,
                              gpointer data)
{
  CtkWidget* image;
  
  image = CTK_WIDGET (data);

  /* We know the pixbuf inside the CtkImage has changed, but the image
   * itself doesn't know this; so queue a redraw.  If we wanted to be
   * really efficient, we could use a drawing area or something
   * instead of a CtkImage, so we could control the exact position of
   * the pixbuf on the display, then we could queue a draw for only
   * the updated area of the image.
   */

  /* We only really need to redraw if the image's animation iterator
   * is gdk_pixbuf_animation_iter_on_currently_loading_frame(), but
   * who cares.
   */
  
  ctk_widget_queue_draw (image);
}

static gint
progressive_timeout (gpointer data)
{
  CtkWidget *image;
  LoadContext *lc;
  
  image = CTK_WIDGET (data);
  lc = get_load_context (image);
  
  /* This shows off fully-paranoid error handling, so looks scary.
   * You could factor out the error handling code into a nice separate
   * function to make things nicer.
   */
  
  if (lc->image_stream)
    {
      size_t bytes_read;
      guchar buf[256];
      GError *error = NULL;
      
      bytes_read = fread (buf, 1, 256, lc->image_stream);

      if (ferror (lc->image_stream))
        {
          CtkWidget *dialog;
          
          dialog = ctk_message_dialog_new (CTK_WINDOW (lc->window),
                                           CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_CLOSE,
                                           "Failure reading image file 'alphatest.png': %s",
                                           g_strerror (errno));

          g_signal_connect (dialog, "response",
			    G_CALLBACK (ctk_widget_destroy), NULL);

          fclose (lc->image_stream);
          lc->image_stream = NULL;

          ctk_widget_show (dialog);
          
          lc->load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (!gdk_pixbuf_loader_write (lc->pixbuf_loader,
                                    buf, bytes_read,
                                    &error))
        {
          CtkWidget *dialog;
          
          dialog = ctk_message_dialog_new (CTK_WINDOW (lc->window),
                                           CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_CLOSE,
                                           "Failed to load image: %s",
                                           error->message);

          g_error_free (error);
          
          g_signal_connect (dialog, "response",
			    G_CALLBACK (ctk_widget_destroy), NULL);

          fclose (lc->image_stream);
          lc->image_stream = NULL;
          
          ctk_widget_show (dialog);

          lc->load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (feof (lc->image_stream))
        {
          fclose (lc->image_stream);
          lc->image_stream = NULL;

          /* Errors can happen on close, e.g. if the image
           * file was truncated we'll know on close that
           * it was incomplete.
           */
          error = NULL;
          if (!gdk_pixbuf_loader_close (lc->pixbuf_loader,
                                        &error))
            {
              CtkWidget *dialog;
              
              dialog = ctk_message_dialog_new (CTK_WINDOW (lc->window),
                                               CTK_DIALOG_DESTROY_WITH_PARENT,
                                               CTK_MESSAGE_ERROR,
                                               CTK_BUTTONS_CLOSE,
                                               "Failed to load image: %s",
                                               error->message);
              
              g_error_free (error);
              
              g_signal_connect (dialog, "response",
				G_CALLBACK (ctk_widget_destroy), NULL);
              
              ctk_widget_show (dialog);

              g_object_unref (lc->pixbuf_loader);
              lc->pixbuf_loader = NULL;
              
              lc->load_timeout = 0;
              
              return FALSE; /* uninstall the timeout */
            }
          
          g_object_unref (lc->pixbuf_loader);
          lc->pixbuf_loader = NULL;
        }
    }
  else
    {
      lc->image_stream = fopen (lc->filename, "r");

      if (lc->image_stream == NULL)
        {
          CtkWidget *dialog;
          
          dialog = ctk_message_dialog_new (CTK_WINDOW (lc->window),
                                           CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_CLOSE,
                                           "Unable to open image file '%s': %s",
                                           lc->filename,
                                           g_strerror (errno));

          g_signal_connect (dialog, "response",
			    G_CALLBACK (ctk_widget_destroy), NULL);
          
          ctk_widget_show (dialog);

          lc->load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (lc->pixbuf_loader)
        {
          gdk_pixbuf_loader_close (lc->pixbuf_loader, NULL);
          g_object_unref (lc->pixbuf_loader);
          lc->pixbuf_loader = NULL;
        }
      
      lc->pixbuf_loader = gdk_pixbuf_loader_new ();
      
      g_signal_connect (lc->pixbuf_loader, "area_prepared",
			G_CALLBACK (progressive_prepared_callback), image);
      g_signal_connect (lc->pixbuf_loader, "area_updated",
			G_CALLBACK (progressive_updated_callback), image);
    }

  /* leave timeout installed */
  return TRUE;
}

static void
start_progressive_loading (CtkWidget *image)
{
  LoadContext *lc;

  lc = get_load_context (image);
  
  /* This is obviously totally contrived (we slow down loading
   * on purpose to show how incremental loading works).
   * The real purpose of incremental loading is the case where
   * you are reading data from a slow source such as the network.
   * The timeout simply simulates a slow data source by inserting
   * pauses in the reading process.
   */
  lc->load_timeout = cdk_threads_add_timeout (100,
                                    progressive_timeout,
                                    image);
}

static CtkWidget *
do_image (const char *filename)
{
  CtkWidget *frame;
  CtkWidget *vbox;
  CtkWidget *image;
  CtkWidget *label;
  CtkWidget *window;
  gchar *str, *escaped;
  LoadContext *lc;
  
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Image Loading");

  ctk_container_set_border_width (CTK_CONTAINER (window), 8);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  label = ctk_label_new (NULL);
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  escaped = g_markup_escape_text (filename, -1);
  str = g_strdup_printf ("Progressively loading: <b>%s</b>", escaped);
  ctk_label_set_markup (CTK_LABEL (label),
                        str);
  g_free (escaped);
  g_free (str);
  
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);
      
  frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
  ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
  ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

  image = ctk_image_new_from_pixbuf (NULL);
  ctk_container_add (CTK_CONTAINER (frame), image);

  lc = get_load_context (image);

  lc->window = window;
  lc->filename = g_strdup (filename);
  
  start_progressive_loading (image);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (ctk_main_quit), NULL);
  
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (ctk_main_quit), NULL);

  ctk_widget_show_all (window);

  return window;
}

static void
do_nonprogressive (const gchar *filename)
{
  CtkWidget *frame;
  CtkWidget *vbox;
  CtkWidget *image;
  CtkWidget *label;
  CtkWidget *window;
  gchar *str, *escaped;
  
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Animation");

  ctk_container_set_border_width (CTK_CONTAINER (window), 8);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  label = ctk_label_new (NULL);
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  escaped = g_markup_escape_text (filename, -1);
  str = g_strdup_printf ("Loaded from file: <b>%s</b>", escaped);
  ctk_label_set_markup (CTK_LABEL (label),
                        str);
  g_free (escaped);
  g_free (str);
  
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);
      
  frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
  ctk_widget_set_halign (frame, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (frame, CTK_ALIGN_CENTER);
  ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

  image = ctk_image_new_from_file (filename);
  ctk_container_add (CTK_CONTAINER (frame), image);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (ctk_main_quit), NULL);
  
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (ctk_main_quit), NULL);

  ctk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
  gint i;
  
  ctk_init (&argc, &argv);

  i = 1;
  while (i < argc)
    {
      do_image (argv[i]);
      do_nonprogressive (argv[i]);
      
      ++i;
    }

  ctk_main ();
  
  return 0;
}

