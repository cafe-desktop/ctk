#include <cdk/cdk.h>
#include <ctk/ctk.h>
#include <cdkx.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/extensions/shape.h>

#include <cdk-pixbuf/cdk-pixbuf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include "widgets.h"
#include "shadow.h"

#define MAXIMUM_WM_REPARENTING_DEPTH 4
#ifndef _
#define _(x) (x)
#endif

static void queue_show (void);

static Window
find_toplevel_window (Window xid)
{
  Window root, parent, *children;
  guint nchildren;

  do
    {
      if (XQueryTree (CDK_DISPLAY_XDISPLAY (cdk_display_get_default ()), xid, &root,
		      &parent, &children, &nchildren) == 0)
	{
	  g_warning ("Couldn't find window manager window");
	  return 0;
	}

      if (root == parent)
	return xid;

      xid = parent;
    }
  while (TRUE);
}

static CdkPixbuf *
add_border_to_shot (CdkPixbuf *pixbuf)
{
  CdkPixbuf *retval;
  CdkColorspace colorspace;
  int bits;

  colorspace = cdk_pixbuf_get_colorspace (pixbuf);
  bits = cdk_pixbuf_get_bits_per_sample (pixbuf);
  retval = cdk_pixbuf_new (colorspace, TRUE, bits,
			   cdk_pixbuf_get_width (pixbuf) + 2,
			   cdk_pixbuf_get_height (pixbuf) + 2);

  /* Fill with solid black */
  cdk_pixbuf_fill (retval, 0xFF);
  cdk_pixbuf_copy_area (pixbuf,
			0, 0,
			cdk_pixbuf_get_width (pixbuf),
			cdk_pixbuf_get_height (pixbuf),
			retval, 1, 1);

  return retval;
}

static CdkPixbuf *
remove_shaped_area (CdkPixbuf *pixbuf,
		    Window     window)
{
  CdkPixbuf *retval;
  XRectangle *rectangles;
  int rectangle_count, rectangle_order;
  int i;
  CdkColorspace colorspace;
  int bits;

  colorspace = cdk_pixbuf_get_colorspace (pixbuf);
  bits = cdk_pixbuf_get_bits_per_sample (pixbuf);
  retval = cdk_pixbuf_new (colorspace, TRUE, bits,
			   cdk_pixbuf_get_width (pixbuf),
			   cdk_pixbuf_get_height (pixbuf));
  
  cdk_pixbuf_fill (retval, 0);
  rectangles = XShapeGetRectangles (CDK_DISPLAY_XDISPLAY (cdk_display_get_default ()), window,
				    ShapeBounding, &rectangle_count, &rectangle_order);

  for (i = 0; i < rectangle_count; i++)
    {
      int y, x;

      for (y = rectangles[i].y; y < rectangles[i].y + rectangles[i].height; y++)
	{
	  guchar *src_pixels, *dest_pixels;

	  src_pixels = cdk_pixbuf_get_pixels (pixbuf) +
	    y * cdk_pixbuf_get_rowstride (pixbuf) +
	    rectangles[i].x * (cdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3);
	  dest_pixels = cdk_pixbuf_get_pixels (retval) +
	    y * cdk_pixbuf_get_rowstride (retval) +
	    rectangles[i].x * 4;

	  for (x = rectangles[i].x; x < rectangles[i].x + rectangles[i].width; x++)
	    {
	      *dest_pixels++ = *src_pixels ++;
	      *dest_pixels++ = *src_pixels ++;
	      *dest_pixels++ = *src_pixels ++;
	      *dest_pixels++ = 255;

	      if (cdk_pixbuf_get_has_alpha (pixbuf))
		src_pixels++;
	    }
	}
    }

  return retval;
}

typedef enum {
  DECOR_NONE,
  DECOR_FRAME,
  DECOR_WINDOW_FRAME
} DecorationType;

static CdkPixbuf *
take_window_shot (Window         child,
                  DecorationType decor)
{
  CdkWindow *window;
  Window xid;
  gint x_orig, y_orig;
  gint x = 0, y = 0;
  gint width, height;

  CdkPixbuf *tmp, *tmp2;
  CdkPixbuf *retval = NULL;

  if (decor == DECOR_WINDOW_FRAME)
    xid = find_toplevel_window (child);
  else
    xid = child;

  window = cdk_x11_window_foreign_new_for_display (cdk_display_get_default (), xid);

  width = cdk_window_get_width (window);
  height = cdk_window_get_height (window);
  cdk_window_get_origin (window, &x_orig, &y_orig);

  if (x_orig < 0)
    {
      x = - x_orig;
      width = width + x_orig;
      x_orig = 0;
    }

  if (y_orig < 0)
    {
      y = - y_orig;
      height = height + y_orig;
      y_orig = 0;
    }

  if (x_orig + width > cdk_screen_width ())
    width = cdk_screen_width () - x_orig;

  if (y_orig + height > cdk_screen_height ())
    height = cdk_screen_height () - y_orig;

  tmp = cdk_pixbuf_get_from_window (window,
				    x, y, width, height);

  if (tmp != NULL)
    {
      if (decor == DECOR_WINDOW_FRAME)
        tmp2 = remove_shaped_area (tmp, xid);
      else if (decor == DECOR_FRAME)
        tmp2 = add_border_to_shot (tmp);
      else
        tmp2 = g_object_ref (tmp);

      g_object_unref (tmp);

      if (tmp2 != NULL)
        {
          retval = create_shadowed_pixbuf (tmp2);
          g_object_unref (tmp2);
        }
    }

  return retval;
}

static GList *toplevels;
static guint shot_id;
static gboolean

window_is_csd (CdkWindow *window)
{
  gboolean set;
  CdkWMDecoration decorations = 0;

  /* FIXME: is this accurate? */
  set = cdk_window_get_decorations (window, &decorations);
  return (set && (decorations == 0));
}

static gboolean
shoot_one (WidgetInfo *info)
{
  CdkWindow *window;
  XID id;
  CdkPixbuf *screenshot = NULL;
  DecorationType decor = DECOR_FRAME;

  if (g_list_find (toplevels, info) == NULL)
    {
      g_warning ("Widget not found in queue");
      ctk_main_quit ();
    }

  window = ctk_widget_get_window (info->window);
  id = cdk_x11_window_get_xid (window);
  if (window_is_csd (window))
    decor = (info->include_decorations) ? DECOR_NONE : DECOR_WINDOW_FRAME;
  screenshot = take_window_shot (id, decor);
  if (screenshot != NULL)
    {
      char *filename;
      filename = g_strdup_printf ("./%s.png", info->name);
      cdk_pixbuf_save (screenshot, filename, "png", NULL, NULL);
      g_free (filename);
      g_object_unref (screenshot);
    }
  else
    {
      g_warning ("unable to save shot of %s", info->name);
    }
  ctk_widget_destroy (info->window);

  shot_id = 0;

  /* remove from the queue and try to load up another */
  toplevels = g_list_remove (toplevels, info);
  if (toplevels == NULL)
    ctk_main_quit ();
  else
    queue_show ();

  return G_SOURCE_REMOVE;
}

static void
on_show (WidgetInfo *info)
{
  if (shot_id != 0)
    return;

  shot_id = g_timeout_add (500, (GSourceFunc) shoot_one, info);
}

static gboolean
show_one (void)
{
  WidgetInfo *info = toplevels->data;

  g_message ("shooting %s", info->name);

  g_signal_connect_swapped (info->window,
                            "show",
                            G_CALLBACK (on_show),
                            info);

  ctk_widget_show (info->window);

  return G_SOURCE_REMOVE;
}

static void
queue_show (void)
{
  g_idle_add ((GSourceFunc) show_one, NULL);
}

int main (int argc, char **argv)
{
  /* If there's no DISPLAY, we silently error out.  We don't want to break
   * headless builds. */
  if (! ctk_init_check (&argc, &argv))
    return 0;

  toplevels = get_all_widgets ();

  queue_show ();
  ctk_main ();

  return 0;
}
