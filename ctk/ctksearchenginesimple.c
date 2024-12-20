/*
 * Copyright (C) 2005 Red Hat, Inc
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
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 * Based on nautilus-search-engine-simple.c
 */

#include "config.h"

#include <gio/gio.h>

#include <cdk/cdk.h>

#include "ctksearchenginesimple.h"
#include "ctkfilesystem.h"
#include "ctkprivate.h"

#include <string.h>

#define BATCH_SIZE 500

typedef struct
{
  CtkSearchEngineSimple *engine;
  GCancellable *cancellable;

  GQueue *directories;

  guint got_results : 1;
  gint n_processed_files;
  GList *hits;

  CtkQuery *query;
  gboolean recursive;
} SearchThreadData;


struct _CtkSearchEngineSimple
{
  CtkSearchEngine parent;

  CtkQuery *query;

  SearchThreadData *active_search;

  gboolean query_finished;

  CtkSearchEngineSimpleIsIndexed is_indexed_callback;
  gpointer                       is_indexed_data;
  GDestroyNotify                 is_indexed_data_destroy;
};

struct _CtkSearchEngineSimpleClass
{
  CtkSearchEngineClass parent_class;
};

G_DEFINE_TYPE (CtkSearchEngineSimple, _ctk_search_engine_simple, CTK_TYPE_SEARCH_ENGINE)

static void
ctk_search_engine_simple_dispose (GObject *object)
{
  CtkSearchEngineSimple *simple = CTK_SEARCH_ENGINE_SIMPLE (object);

  g_clear_object (&simple->query);

  if (simple->active_search)
    {
      g_cancellable_cancel (simple->active_search->cancellable);
      simple->active_search = NULL;
    }

  _ctk_search_engine_simple_set_indexed_cb (simple, NULL, NULL, NULL);

  G_OBJECT_CLASS (_ctk_search_engine_simple_parent_class)->dispose (object);
}

static void
queue_if_local (SearchThreadData *data,
                GFile            *file)
{
  if (file &&
      !_ctk_file_consider_as_remote (file) &&
      !g_file_has_uri_scheme (file, "recent"))
    g_queue_push_tail (data->directories, g_object_ref (file));
}

static SearchThreadData *
search_thread_data_new (CtkSearchEngineSimple *engine,
			CtkQuery              *query)
{
  SearchThreadData *data;

  data = g_new0 (SearchThreadData, 1);

  data->engine = g_object_ref (engine);
  data->directories = g_queue_new ();
  data->query = g_object_ref (query);
  /* Simple search engine is too slow to be recursive */
  data->recursive = FALSE;
  queue_if_local (data, ctk_query_get_location (query));

  data->cancellable = g_cancellable_new ();

  return data;
}

static void
search_thread_data_free (SearchThreadData *data)
{
  g_queue_foreach (data->directories, (GFunc)g_object_unref, NULL);
  g_queue_free (data->directories);
  g_object_unref (data->cancellable);
  g_object_unref (data->query);
  g_object_unref (data->engine);

  g_free (data);
}

static gboolean
search_thread_done_idle (gpointer user_data)
{
  SearchThreadData *data;

  data = user_data;

  if (!g_cancellable_is_cancelled (data->cancellable))
    {
      _ctk_search_engine_finished (CTK_SEARCH_ENGINE (data->engine),
                                   data->got_results);
    }

  data->engine->active_search = NULL;
  search_thread_data_free (data);

  return FALSE;
}

typedef struct
{
  GList *hits;
  SearchThreadData *thread_data;
} Batch;

static gboolean
search_thread_add_hits_idle (gpointer user_data)
{
  Batch *batch = user_data;

  if (!g_cancellable_is_cancelled (batch->thread_data->cancellable))
    _ctk_search_engine_hits_added (CTK_SEARCH_ENGINE (batch->thread_data->engine), batch->hits);

  g_list_free_full (batch->hits, (GDestroyNotify)_ctk_search_hit_free);
  g_free (batch);

  return FALSE;
}

static void
send_batch (SearchThreadData *data)
{
  data->n_processed_files = 0;

  if (data->hits)
    {
      Batch *batch;
      guint id;

      batch = g_new (Batch, 1);
      batch->hits = data->hits;
      batch->thread_data = data;

      id = cdk_threads_add_idle (search_thread_add_hits_idle, batch);
      g_source_set_name_by_id (id, "[ctk+] search_thread_add_hits_idle");
      data->got_results = TRUE;
    }

  data->hits = NULL;
}

static gboolean
is_indexed (CtkSearchEngineSimple *engine,
            GFile                 *location)
{
  if (engine->is_indexed_callback)
    {
      if (engine->is_indexed_callback (location, engine->is_indexed_data))
        {
          gchar *uri = g_file_get_uri (location);
          g_debug ("Simple search engine: Skipping indexed location: %s\n", uri);
          g_free (uri);

          return TRUE;
        }
    }

  return FALSE;
}

static void
visit_directory (GFile *dir, SearchThreadData *data)
{
  GFileEnumerator *enumerator;
  GFileInfo *info;
  GFile *child;

  enumerator = g_file_enumerate_children (dir,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP ","
                                          G_FILE_ATTRIBUTE_STANDARD_SIZE ","
                                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                                          G_FILE_ATTRIBUTE_STANDARD_TARGET_URI ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED ","
                                          G_FILE_ATTRIBUTE_TIME_ACCESS ","
                                          G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME ","
                                          G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH ","
                                          G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE,
                                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                          data->cancellable, NULL);
  if (enumerator == NULL)
    return;

  while (g_file_enumerator_iterate (enumerator, &info, &child, data->cancellable, NULL))
    {
      const gchar *display_name;

      if (info == NULL)
        break;

      display_name = g_file_info_get_display_name (info);
      if (display_name == NULL)
        continue;

      if (g_file_info_get_is_hidden (info))
        continue;

      if (ctk_query_matches_string (data->query, display_name))
        {
          CtkSearchHit *hit;

          hit = g_new (CtkSearchHit, 1);
          hit->file = g_object_ref (child);
          hit->info = g_object_ref (info);
          data->hits = g_list_prepend (data->hits, hit);
        }

      data->n_processed_files++;
      if (data->n_processed_files > BATCH_SIZE)
        send_batch (data);

      if (data->recursive &&
          g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY &&
          !is_indexed (data->engine, child))
        queue_if_local (data, child);
    }

  g_object_unref (enumerator);
}

static gpointer
search_thread_func (gpointer user_data)
{
  SearchThreadData *data;
  GFile *dir;
  guint id;

  data = user_data;

  while (!g_cancellable_is_cancelled (data->cancellable) &&
         (dir = g_queue_pop_head (data->directories)) != NULL)
    {
      visit_directory (dir, data);
      g_object_unref (dir);
    }

  if (!g_cancellable_is_cancelled (data->cancellable))
    send_batch (data);

  id = cdk_threads_add_idle (search_thread_done_idle, data);
  g_source_set_name_by_id (id, "[ctk+] search_thread_done_idle");

  return NULL;
}

static void
ctk_search_engine_simple_start (CtkSearchEngine *engine)
{
  CtkSearchEngineSimple *simple;
  SearchThreadData *data;

  simple = CTK_SEARCH_ENGINE_SIMPLE (engine);

  if (simple->active_search != NULL)
    return;

  if (simple->query == NULL)
    return;

  data = search_thread_data_new (simple, simple->query);

  g_thread_unref (g_thread_new ("file-search", search_thread_func, data));

  simple->active_search = data;
}

static void
ctk_search_engine_simple_stop (CtkSearchEngine *engine)
{
  CtkSearchEngineSimple *simple;

  simple = CTK_SEARCH_ENGINE_SIMPLE (engine);

  if (simple->active_search != NULL)
    {
      g_cancellable_cancel (simple->active_search->cancellable);
      simple->active_search = NULL;
    }
}

static void
ctk_search_engine_simple_set_query (CtkSearchEngine *engine,
				    CtkQuery        *query)
{
  CtkSearchEngineSimple *simple;

  simple = CTK_SEARCH_ENGINE_SIMPLE (engine);

  if (query)
    g_object_ref (query);

  if (simple->query)
    g_object_unref (simple->query);

  simple->query = query;
}

static void
_ctk_search_engine_simple_class_init (CtkSearchEngineSimpleClass *class)
{
  GObjectClass *gobject_class;
  CtkSearchEngineClass *engine_class;

  gobject_class = G_OBJECT_CLASS (class);
  gobject_class->dispose = ctk_search_engine_simple_dispose;

  engine_class = CTK_SEARCH_ENGINE_CLASS (class);
  engine_class->set_query = ctk_search_engine_simple_set_query;
  engine_class->start = ctk_search_engine_simple_start;
  engine_class->stop = ctk_search_engine_simple_stop;
}

static void
_ctk_search_engine_simple_init (CtkSearchEngineSimple *engine G_GNUC_UNUSED)
{
}

CtkSearchEngine *
_ctk_search_engine_simple_new (void)
{
  return g_object_new (CTK_TYPE_SEARCH_ENGINE_SIMPLE, NULL);
}

void
_ctk_search_engine_simple_set_indexed_cb (CtkSearchEngineSimple          *engine,
                                          CtkSearchEngineSimpleIsIndexed  callback,
                                          gpointer                        data,
                                          GDestroyNotify                  destroy)
{
  if (engine->is_indexed_data_destroy)
    engine->is_indexed_data_destroy (engine->is_indexed_data);

  engine->is_indexed_callback = callback;
  engine->is_indexed_data = data;
  engine->is_indexed_data_destroy = destroy;
}
