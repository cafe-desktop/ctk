/* Copyright (C) 2016 Red Hat, Inc.
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

#include "cdkpixbufutilsprivate.h"

static CdkPixbuf *
load_from_stream (CdkPixbufLoader  *loader,
                  GInputStream     *stream,
                  GCancellable     *cancellable,
                  GError          **error)
{
  CdkPixbuf *pixbuf;
  gssize n_read;
  guchar buffer[65536];
  gboolean res;

  res = TRUE;
  while (1)
    {
      n_read = g_input_stream_read (stream, buffer, sizeof (buffer), cancellable, error);
      if (n_read < 0)
        {
          res = FALSE;
          error = NULL; /* Ignore further errors */
          break;
        }

      if (n_read == 0)
        break;

      if (!cdk_pixbuf_loader_write (loader, buffer, n_read, error))
        {
          res = FALSE;
          error = NULL;
          break;
        }
    }

  if (!cdk_pixbuf_loader_close (loader, error))
    {
      res = FALSE;
      error = NULL;
    }

  pixbuf = NULL;

  if (res)
    {
      pixbuf = cdk_pixbuf_loader_get_pixbuf (loader);
      if (pixbuf)
        g_object_ref (pixbuf);
    }

  return pixbuf;
}

static void
size_prepared_cb (CdkPixbufLoader *loader,
                  gint             width,
                  gint             height,
                  gpointer         data)
{
  gdouble *scale = data;

  width = MAX (*scale * width, 1);
  height = MAX (*scale * height, 1);

  cdk_pixbuf_loader_set_size (loader, width, height);
}

/* Like cdk_pixbuf_new_from_stream_at_scale, but
 * load the image at its original size times the
 * given scale.
 */
CdkPixbuf *
_cdk_pixbuf_new_from_stream_scaled (GInputStream  *stream,
                                    gdouble        scale,
                                    GCancellable  *cancellable,
                                    GError       **error)
{
  CdkPixbufLoader *loader;
  CdkPixbuf *pixbuf;

  loader = cdk_pixbuf_loader_new ();

  g_signal_connect (loader, "size-prepared",
                    G_CALLBACK (size_prepared_cb), &scale);

  pixbuf = load_from_stream (loader, stream, cancellable, error);

  g_object_unref (loader);

  return pixbuf;
}

/* Like cdk_pixbuf_new_from_resource_at_scale, but
 * load the image at its original size times the
 * given scale.
 */
CdkPixbuf *
_cdk_pixbuf_new_from_resource_scaled (const gchar  *resource_path,
                                      gdouble       scale,
                                      GError      **error)
{
  GInputStream *stream;
  CdkPixbuf *pixbuf;

  stream = g_resources_open_stream (resource_path, 0, error);
  if (stream == NULL)
    return NULL;

  pixbuf = _cdk_pixbuf_new_from_stream_scaled (stream, scale, NULL, error);
  g_object_unref (stream);

  return pixbuf;
}

