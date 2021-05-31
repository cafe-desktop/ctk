/*
 * Copyright (C) 2005 Novell, Inc.
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
 *
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 * Based on nautilus-query.c
 */

#include "config.h"
#include <string.h>

#include "ctkquery.h"

struct _CtkQueryPrivate
{
  gchar *text;
  GFile *location;
  GList *mime_types;
  gchar **words;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkQuery, ctk_query, G_TYPE_OBJECT)

static void
finalize (GObject *object)
{
  CtkQuery *query;

  query = CTK_QUERY (object);

  g_clear_object (&query->priv->location);
  g_free (query->priv->text);
  g_strfreev (query->priv->words);

  G_OBJECT_CLASS (ctk_query_parent_class)->finalize (object);
}

static void
ctk_query_class_init (CtkQueryClass *class)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (class);
  gobject_class->finalize = finalize;
}

static void
ctk_query_init (CtkQuery *query)
{
  query->priv = ctk_query_get_instance_private (query);
}

CtkQuery *
ctk_query_new (void)
{
  return g_object_new (CTK_TYPE_QUERY,  NULL);
}


const gchar *
ctk_query_get_text (CtkQuery *query)
{
  return query->priv->text;
}

void
ctk_query_set_text (CtkQuery    *query,
                    const gchar *text)
{
  g_free (query->priv->text);
  query->priv->text = g_strdup (text);

  g_strfreev (query->priv->words);
  query->priv->words = NULL;
}

GFile *
ctk_query_get_location (CtkQuery *query)
{
  return query->priv->location;
}

void
ctk_query_set_location (CtkQuery *query,
                        GFile    *file)
{
  g_set_object (&query->priv->location, file);
}

static gchar *
prepare_string_for_compare (const gchar *string)
{
  gchar *normalized, *res;

  normalized = g_utf8_normalize (string, -1, G_NORMALIZE_NFD);
  res = g_utf8_strdown (normalized, -1);
  g_free (normalized);

  return res;
}

gboolean
ctk_query_matches_string (CtkQuery    *query,
                          const gchar *string)
{
  gchar *prepared;
  gboolean found;
  gint i;

  if (!query->priv->text)
    return FALSE;

  if (!query->priv->words)
    {
      prepared = prepare_string_for_compare (query->priv->text);
      query->priv->words = g_strsplit (prepared, " ", -1);
      g_free (prepared);
    }

  prepared = prepare_string_for_compare (string);

  found = TRUE;
  for (i = 0; query->priv->words[i]; i++)
    {
      if (strstr (prepared, query->priv->words[i]) == NULL)
        {
          found = FALSE;
          break;
        }
    }

  g_free (prepared);

  return found;
}
