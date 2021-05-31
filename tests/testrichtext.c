/* testrichtext.c
 * Copyright (C) 2006 Imendio AB
 * Authors: Michael Natterer, Tim Janik
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

#include <string.h>
#include <ctk/ctk.h>

static guint32 quick_rand32_accu = 2147483563;

static inline guint32
quick_rand32 (void)
{
  quick_rand32_accu = 1664525 * quick_rand32_accu + 1013904223;
  return quick_rand32_accu;
}

static gboolean
delete_event (GtkWidget   *widget,
              GdkEventAny *event,
              gpointer     user_data)
{
  ctk_main_quit ();

  return TRUE;
}

static void
text_tag_enqueue (GtkTextTag *tag,
                  gpointer    data)
{
  GSList **slist_p = data;
  *slist_p = g_slist_prepend (*slist_p, tag);
}

static const gchar *example_text =
"vkndsk vfds vkfds vkdsv fdlksnvkfdvnkfdvnkdsnvs\n"
"kmvofdmvfdsvkv fdskvnkfdv nnd.mckfdvnknsknvdnvs"
"fdlvmfdsvlkfdsmvnskdnvfdsnvf sbskjnvlknfd cvdvnd"
"mvlfdsv vfdkjv m, ds vkfdks v df,v j kfds v d\n"
"vnfdskv kjvnfv  cfdkvndfnvcm fd,vk kdsf vj d\n"
"KLJHkjh kjh klhjKLJH Kjh kjl h34kj h34kj3h klj 23 "
"kjlkjlhsdjk 34kljh klj hklj 23k4jkjkjh234kjh 52kj "
"2h34 sdaf ukklj kjl32l jkkjl 23j jkl ljk23 jkl\n"
"hjhjhj2hj23jh jh jk jk2h3 hj kjj jk jh21 jhhj32.";

static GdkAtom
setup_buffer (GtkTextBuffer *buffer)
{
  const guint tlen = strlen (example_text);
  const guint tcount = 17;
  GtkTextTag **tags;
  GtkTextTagTable *ttable = ctk_text_buffer_get_tag_table (buffer);
  GSList *node, *slist = NULL;
  GdkAtom atom;
  guint i;

  tags = g_malloc (sizeof (GtkTextTag *) * tcount);

  /* cleanup */
  ctk_text_buffer_set_text (buffer, "", 0);
  ctk_text_tag_table_foreach (ttable, text_tag_enqueue, &slist);
  for (node = slist; node; node = node->next)
    ctk_text_tag_table_remove (ttable, node->data);
  g_slist_free (slist);

  /* create new tags */
  for (i = 0; i < tcount; i++)
    {
      char *s = g_strdup_printf ("tag%u", i);
      tags[i] = ctk_text_buffer_create_tag (buffer, s,
                                            "weight", quick_rand32() >> 31 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
                                            "style", quick_rand32() >> 31 ? PANGO_STYLE_OBLIQUE : PANGO_STYLE_NORMAL,
                                            "underline", quick_rand32() >> 31,
                                            NULL);
      g_free (s);
    }

  /* assign text and tags */
  ctk_text_buffer_set_text (buffer, example_text, -1);
  for (i = 0; i < tcount * 5; i++)
    {
      gint a = quick_rand32() % tlen, b = quick_rand32() % tlen;
      GtkTextIter start, end;
      ctk_text_buffer_get_iter_at_offset (buffer, &start, MIN (a, b));
      ctk_text_buffer_get_iter_at_offset (buffer, &end,   MAX (a, b));
      ctk_text_buffer_apply_tag (buffer, tags[i % tcount], &start, &end);
    }

  /* return serialization format */
  atom = ctk_text_buffer_register_deserialize_tagset (buffer, NULL);
  ctk_text_buffer_deserialize_set_can_create_tags (buffer, atom, TRUE);

  g_free (tags);

  return atom;
}

static gboolean
test_serialize_deserialize (GtkTextBuffer *buffer,
                            GdkAtom        atom,
                            GError       **error)
{
  GtkTextIter  start, end;
  guint8      *spew;
  gsize        spew_length;
  gboolean     success;

  ctk_text_buffer_get_bounds (buffer, &start, &end);

  spew = ctk_text_buffer_serialize (buffer, buffer, atom,
                                    &start, &end, &spew_length);

  success = ctk_text_buffer_deserialize (buffer, buffer, atom, &end,
                                         spew, spew_length, error);

  g_free (spew);

  return success;
}

gint
main (gint   argc,
      gchar *argv[])
{
  GtkWidget     *window;
  GtkWidget     *sw;
  GtkWidget     *view;
  GtkTextBuffer *buffer;
  GdkAtom        atom;
  guint          i, broken = 0;

  ctk_init (&argc, &argv);

  /* initialize random numbers, disable this for deterministic testing */
  if (1)        
    quick_rand32_accu = g_random_int();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_size_request (window, 400, 300);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw),
                                       CTK_SHADOW_IN);
  ctk_container_set_border_width (CTK_CONTAINER (sw), 12);
  ctk_container_add (CTK_CONTAINER (window), sw);

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (delete_event),
                    NULL);

  buffer = ctk_text_buffer_new (NULL);
  view = ctk_text_view_new_with_buffer (buffer);
  g_object_unref (buffer);

  ctk_container_add (CTK_CONTAINER (sw), view);

  ctk_widget_show_all (window);
  if (0)
    ctk_main ();

  for (i = 0; i < 250; i++)
    {
      GError *error = NULL;
      g_printerr ("creating randomly tagged text buffer with accu=0x%x...\n", quick_rand32_accu);
      atom = setup_buffer (buffer);
      if (test_serialize_deserialize (buffer, atom, &error))
        g_printerr ("ok.\n");
      else
        {
          g_printerr ("FAIL: serialization/deserialization failed:\n  %s\n", error->message);
          broken += 1;
        }
      g_clear_error (&error);
    }

  return broken > 0;
}
