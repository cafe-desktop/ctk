/* Pixbufs
 *
 * A GdkPixbuf represents an image, normally in RGB or RGBA format.
 * Pixbufs are normally used to load files from disk and perform
 * image scaling.
 *
 * This demo is not all that educational, but looks cool. It was written
 * by Extreme Pixbuf Hacker Federico Mena Quintero. It also shows
 * off how to use CtkDrawingArea to do a simple animation.
 *
 * Look at the Image demo for additional pixbuf usage examples.
 *
 */

#include <stdlib.h>
#include <ctk/ctk.h>
#include <math.h>

#define BACKGROUND_NAME "/pixbufs/background.jpg"

static const char *image_names[] = {
  "/pixbufs/apple-red.png",
  "/pixbufs/gnome-applets.png",
  "/pixbufs/gnome-calendar.png",
  "/pixbufs/gnome-foot.png",
  "/pixbufs/gnome-gmush.png",
  "/pixbufs/gnome-gimp.png",
  "/pixbufs/gnome-gsame.png",
  "/pixbufs/gnu-keys.png"
};

#define N_IMAGES G_N_ELEMENTS (image_names)

/* demo window */
static CtkWidget *window = NULL;

/* Current frame */
static GdkPixbuf *frame;

/* Background image */
static GdkPixbuf *background;
static gint back_width, back_height;

/* Images */
static GdkPixbuf *images[N_IMAGES];

/* Widgets */
static CtkWidget *da;

/* Loads the images for the demo and returns whether the operation succeeded */
static gboolean
load_pixbufs (GError **error)
{
  gint i;

  if (background)
    return TRUE; /* already loaded earlier */

  background = gdk_pixbuf_new_from_resource (BACKGROUND_NAME, error);
  if (!background)
    return FALSE; /* Note that "error" was filled with a GError */

  back_width = gdk_pixbuf_get_width (background);
  back_height = gdk_pixbuf_get_height (background);

  for (i = 0; i < N_IMAGES; i++)
    {
      images[i] = gdk_pixbuf_new_from_resource (image_names[i], error);

      if (!images[i])
        return FALSE; /* Note that "error" was filled with a GError */
    }

  return TRUE;
}

/* Expose callback for the drawing area */
static gint
draw_cb (CtkWidget *widget G_GNUC_UNUSED,
         cairo_t   *cr,
         gpointer   data G_GNUC_UNUSED)
{
  cdk_cairo_set_source_pixbuf (cr, frame, 0, 0);
  cairo_paint (cr);

  return TRUE;
}

#define CYCLE_TIME 3000000 /* 3 seconds */

static gint64 start_time;

/* Handler to regenerate the frame */
static gboolean
on_tick (CtkWidget     *widget G_GNUC_UNUSED,
         CdkFrameClock *frame_clock,
         gpointer       data G_GNUC_UNUSED)
{
  gint64 current_time;
  double f;
  int i;
  double xmid, ymid;
  double radius;

  gdk_pixbuf_copy_area (background, 0, 0, back_width, back_height,
                        frame, 0, 0);

  if (start_time == 0)
    start_time = cdk_frame_clock_get_frame_time (frame_clock);

  current_time = cdk_frame_clock_get_frame_time (frame_clock);
  f = ((current_time - start_time) % CYCLE_TIME) / (double)CYCLE_TIME;

  xmid = back_width / 2.0;
  ymid = back_height / 2.0;

  radius = MIN (xmid, ymid) / 2.0;

  for (i = 0; i < N_IMAGES; i++)
    {
      double ang;
      int xpos, ypos;
      int iw, ih;
      double r;
      CdkRectangle r1, r2, dest;
      double k;

      ang = 2.0 * G_PI * (double) i / N_IMAGES - f * 2.0 * G_PI;

      iw = gdk_pixbuf_get_width (images[i]);
      ih = gdk_pixbuf_get_height (images[i]);

      r = radius + (radius / 3.0) * sin (f * 2.0 * G_PI);

      xpos = floor (xmid + r * cos (ang) - iw / 2.0 + 0.5);
      ypos = floor (ymid + r * sin (ang) - ih / 2.0 + 0.5);

      k = (i & 1) ? sin (f * 2.0 * G_PI) : cos (f * 2.0 * G_PI);
      k = 2.0 * k * k;
      k = MAX (0.25, k);

      r1.x = xpos;
      r1.y = ypos;
      r1.width = iw * k;
      r1.height = ih * k;

      r2.x = 0;
      r2.y = 0;
      r2.width = back_width;
      r2.height = back_height;

      if (cdk_rectangle_intersect (&r1, &r2, &dest))
        gdk_pixbuf_composite (images[i],
                              frame,
                              dest.x, dest.y,
                              dest.width, dest.height,
                              xpos, ypos,
                              k, k,
                              GDK_INTERP_NEAREST,
                              ((i & 1)
                               ? MAX (127, fabs (255 * sin (f * 2.0 * G_PI)))
                               : MAX (127, fabs (255 * cos (f * 2.0 * G_PI)))));
    }

  ctk_widget_queue_draw (da);

  return G_SOURCE_CONTINUE;
}

CtkWidget *
do_pixbufs (CtkWidget *do_widget)
{
  if (!window)
    {
      GError *error;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Pixbufs");
      ctk_window_set_resizable (CTK_WINDOW (window), FALSE);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      error = NULL;
      if (!load_pixbufs (&error))
        {
          CtkWidget *dialog;

          dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                           CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_CLOSE,
                                           "Failed to load an image: %s",
                                           error->message);

          g_error_free (error);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (ctk_widget_destroy), NULL);

          ctk_widget_show (dialog);
        }
      else
        {
          ctk_widget_set_size_request (window, back_width, back_height);

          frame = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, back_width, back_height);

          da = ctk_drawing_area_new ();

          g_signal_connect (da, "draw",
                            G_CALLBACK (draw_cb), NULL);

          ctk_container_add (CTK_CONTAINER (window), da);

          ctk_widget_add_tick_callback (da, on_tick, NULL, NULL);
        }
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    {
      ctk_widget_destroy (window);
      g_object_unref (frame);
    }

  return window;
}
