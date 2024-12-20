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
 * Based on nautilus-search-engine.c
 */

#include "config.h"
#include "ctksearchengine.h"
#include "ctksearchenginesimple.h"
#include "ctksearchenginemodel.h"
#include "ctksearchenginequartz.h"
#include "ctkintl.h"

#if defined(HAVE_TRACKER3)
#include "ctksearchenginetracker3.h"
#endif
#if !defined G_OS_WIN32 /* No tracker on windows */
#include "ctksearchenginetracker.h"
#define HAVE_TRACKER 1
#endif

#include <cdk/cdk.h> /* for CDK_WINDOWING_QUARTZ */

struct _CtkSearchEnginePrivate {
  CtkSearchEngine *native;
  gboolean native_running;
  gchar *native_error;

  CtkSearchEngine *simple;
  gboolean simple_running;
  gboolean got_results;
  gchar *simple_error;

  CtkSearchEngine *model;
  gboolean model_running;
  gchar *model_error;

  gboolean running;
  gboolean recursive;
  GHashTable *hits;

  CtkQuery *query;
};

enum
{
  HITS_ADDED,
  FINISHED,
  ERROR,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE (CtkSearchEngine, _ctk_search_engine, G_TYPE_OBJECT);

static void
set_query (CtkSearchEngine *engine,
           CtkQuery        *query)
{
  g_set_object (&engine->priv->query, query);

  if (engine->priv->native)
    _ctk_search_engine_set_query (engine->priv->native, query);

  if (engine->priv->simple)
    _ctk_search_engine_set_query (engine->priv->simple, query);

  if (engine->priv->model)
    _ctk_search_engine_set_query (engine->priv->model, query);
}

static void
start (CtkSearchEngine *engine)
{
  g_hash_table_remove_all (engine->priv->hits);

  if (engine->priv->native)
    {
      g_clear_pointer (&engine->priv->native_error, g_free);
      _ctk_search_engine_start (engine->priv->native);
      engine->priv->native_running = TRUE;
    }

  if (engine->priv->simple)
    {
      g_clear_pointer (&engine->priv->simple_error, g_free);
      _ctk_search_engine_start (engine->priv->simple);
      engine->priv->simple_running = TRUE;
    }

  if (engine->priv->model)
    {
      g_clear_pointer (&engine->priv->model_error, g_free);
      _ctk_search_engine_start (engine->priv->model);
      engine->priv->model_running = TRUE;
    }

  engine->priv->running = TRUE;
}

static void
stop (CtkSearchEngine *engine)
{
  if (engine->priv->native)
    {
      _ctk_search_engine_stop (engine->priv->native);
      engine->priv->native_running = FALSE;
    }

  if (engine->priv->simple)
    {
      _ctk_search_engine_stop (engine->priv->simple);
      engine->priv->simple_running = FALSE;
    }

  if (engine->priv->model)
    {
      _ctk_search_engine_stop (engine->priv->model);
      engine->priv->model_running = FALSE;
    }

  engine->priv->running = FALSE;

  g_hash_table_remove_all (engine->priv->hits);
}

static void
finalize (GObject *object)
{
  CtkSearchEngine *engine = CTK_SEARCH_ENGINE (object);

  g_clear_object (&engine->priv->native);
  g_free (engine->priv->native_error);

  g_clear_object (&engine->priv->simple);
  g_free (engine->priv->simple_error);

  g_clear_object (&engine->priv->model);
  g_free (engine->priv->model_error);

  g_clear_pointer (&engine->priv->hits, g_hash_table_unref);

  g_clear_object (&engine->priv->query);

  G_OBJECT_CLASS (_ctk_search_engine_parent_class)->finalize (object);
}

static void
_ctk_search_engine_class_init (CtkSearchEngineClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = finalize;

  class->set_query = set_query;
  class->start = start;
  class->stop = stop;

  signals[HITS_ADDED] =
    g_signal_new (I_("hits-added"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSearchEngineClass, hits_added),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  signals[FINISHED] =
    g_signal_new (I_("finished"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSearchEngineClass, finished),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  signals[ERROR] =
    g_signal_new (I_("error"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSearchEngineClass, error),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
_ctk_search_engine_init (CtkSearchEngine *engine)
{
  engine->priv = _ctk_search_engine_get_instance_private (engine);

  engine->priv->recursive = TRUE;
}

static void
hits_added (CtkSearchEngine *engine G_GNUC_UNUSED,
            GList           *hits,
            gpointer         data)
{
  CtkSearchEngine *composite = CTK_SEARCH_ENGINE (data);
  GList *added, *l;

  added = NULL;

  for (l = hits; l; l = l->next)
    {
      CtkSearchHit *hit;

      hit = l->data;

      if (!g_hash_table_contains (composite->priv->hits, hit))
        {
          hit = _ctk_search_hit_dup (hit);
          g_hash_table_add (composite->priv->hits, hit);
          added = g_list_prepend (added, hit);
        }
    }

  if (added)
    {
      _ctk_search_engine_hits_added (composite, added);
      g_list_free (added);
    }
}

static void
update_status (CtkSearchEngine *engine)
{
  gboolean running;

  running = engine->priv->native_running || engine->priv->simple_running;

  if (running != engine->priv->running)
    {
      engine->priv->running = running;

      if (!running)
        {
          if (engine->priv->native_error)
            _ctk_search_engine_error (engine, engine->priv->native_error);
          else if (engine->priv->simple_error)
            _ctk_search_engine_error (engine, engine->priv->simple_error);
          else if (engine->priv->model_error)
            _ctk_search_engine_error (engine, engine->priv->model_error);
          else
            _ctk_search_engine_finished (engine, engine->priv->got_results);

          engine->priv->got_results = FALSE;
        }
    }
}

static void
finished (CtkSearchEngine *engine,
          gboolean         got_results,
          gpointer         data)
{
  CtkSearchEngine *composite = CTK_SEARCH_ENGINE (data);

  if (engine == composite->priv->native)
    composite->priv->native_running = FALSE;
  else if (engine == composite->priv->simple)
    composite->priv->simple_running = FALSE;
  else if (engine == composite->priv->model)
    composite->priv->model_running = FALSE;

  composite->priv->got_results |= got_results;
  update_status (composite);
}

static void
error (CtkSearchEngine *engine,
       const gchar     *message,
       gpointer         data)
{
  CtkSearchEngine *composite = CTK_SEARCH_ENGINE (data);

  if (engine == composite->priv->native)
    {
      g_free (composite->priv->native_error);
      composite->priv->native_error = g_strdup (message);
      composite->priv->native_running = FALSE;
    }
  else if (engine == composite->priv->simple)
    {
      g_free (composite->priv->simple_error);
      composite->priv->simple_error = g_strdup (message);
      composite->priv->simple_running = FALSE;
    }
  else if (engine == composite->priv->model)
    {
      g_free (composite->priv->model_error);
      composite->priv->model_error = g_strdup (message);
      composite->priv->model_running = FALSE;
    }

  update_status (composite);
}

static gboolean
search_hit_equal (gconstpointer a, gconstpointer b)
{
  const CtkSearchHit *ha = (const CtkSearchHit *)a;
  const CtkSearchHit *hb = (const CtkSearchHit *)b;

  return g_file_equal (ha->file, hb->file);
}


static guint
search_hit_hash (gconstpointer a)
{
  const CtkSearchHit *ha = (const CtkSearchHit *)a;

  return g_file_hash (ha->file);
}

CtkSearchHit *
_ctk_search_hit_dup (CtkSearchHit *hit)
{
  CtkSearchHit *dup;

  dup = g_new (CtkSearchHit, 1);
  dup->file = g_object_ref (hit->file);
  if (hit->info)
    dup->info = g_object_ref (hit->info);
  else
    dup->info = NULL;

  return dup;
}

void
_ctk_search_hit_free (CtkSearchHit *hit)
{
  g_clear_object (&hit->file);
  g_clear_object (&hit->info);
  g_free (hit);
}

static void
connect_engine_signals (CtkSearchEngine *engine,
                        gpointer         data)
{
  g_signal_connect_object (engine, "hits-added", G_CALLBACK (hits_added), data, 0);
  g_signal_connect_object (engine, "finished", G_CALLBACK (finished), data, 0);
  g_signal_connect_object (engine, "error", G_CALLBACK (error), data, 0);
}

CtkSearchEngine *
_ctk_search_engine_new (void)
{
  CtkSearchEngine *engine;

  engine = g_object_new (CTK_TYPE_SEARCH_ENGINE, NULL);

  engine->priv->simple = _ctk_search_engine_simple_new ();
  g_debug ("Using simple search engine");
  connect_engine_signals (engine->priv->simple, engine);

#if defined(HAVE_TRACKER3)
  engine->priv->native = ctk_search_engine_tracker3_new ();
  if (engine->priv->native)
    {
      g_debug ("Using Tracker3 search engine");
      connect_engine_signals (engine->priv->native, engine);
      _ctk_search_engine_simple_set_indexed_cb (CTK_SEARCH_ENGINE_SIMPLE (engine->priv->simple),
                                                ctk_search_engine_tracker3_is_indexed,
                                                g_object_ref (engine->priv->native),
                                                g_object_unref);
    }
#endif

#ifdef HAVE_TRACKER
  if (!engine->priv->native)
    {
      engine->priv->native = _ctk_search_engine_tracker_new ();
      if (engine->priv->native)
        {
          g_debug ("Using Tracker search engine");
          connect_engine_signals (engine->priv->native, engine);
          _ctk_search_engine_simple_set_indexed_cb (CTK_SEARCH_ENGINE_SIMPLE (engine->priv->simple),
                                                    _ctk_search_engine_tracker_is_indexed,
                                                    g_object_ref (engine->priv->native),
                                                    g_object_unref);
        }
    }
#endif

#ifdef CDK_WINDOWING_QUARTZ
  engine->priv->native = _ctk_search_engine_quartz_new ();
  if (engine->priv->native)
    {
      g_debug ("Using Quartz search engine");
      connect_engine_signals (engine->priv->native, engine);
    }
#endif

  engine->priv->hits = g_hash_table_new_full (search_hit_hash, search_hit_equal,
                                              (GDestroyNotify)_ctk_search_hit_free, NULL);

  return engine;
}

void
_ctk_search_engine_set_query (CtkSearchEngine *engine,
                              CtkQuery        *query)
{
  g_return_if_fail (CTK_IS_SEARCH_ENGINE (engine));
  g_return_if_fail (CTK_SEARCH_ENGINE_GET_CLASS (engine)->set_query != NULL);

  CTK_SEARCH_ENGINE_GET_CLASS (engine)->set_query (engine, query);
}

void
_ctk_search_engine_start (CtkSearchEngine *engine)
{
  g_return_if_fail (CTK_IS_SEARCH_ENGINE (engine));
  g_return_if_fail (CTK_SEARCH_ENGINE_GET_CLASS (engine)->start != NULL);

  CTK_SEARCH_ENGINE_GET_CLASS (engine)->start (engine);
}

void
_ctk_search_engine_stop (CtkSearchEngine *engine)
{
  g_return_if_fail (CTK_IS_SEARCH_ENGINE (engine));
  g_return_if_fail (CTK_SEARCH_ENGINE_GET_CLASS (engine)->stop != NULL);

  CTK_SEARCH_ENGINE_GET_CLASS (engine)->stop (engine);
}

void
_ctk_search_engine_hits_added (CtkSearchEngine *engine,
                               GList           *hits)
{
  g_return_if_fail (CTK_IS_SEARCH_ENGINE (engine));

  g_signal_emit (engine, signals[HITS_ADDED], 0, hits);
}

void
_ctk_search_engine_finished (CtkSearchEngine *engine,
                             gboolean         got_results)
{
  g_return_if_fail (CTK_IS_SEARCH_ENGINE (engine));

  g_signal_emit (engine, signals[FINISHED], 0, got_results);
}

void
_ctk_search_engine_error (CtkSearchEngine *engine,
                          const gchar     *error_message)
{
  g_return_if_fail (CTK_IS_SEARCH_ENGINE (engine));

  g_signal_emit (engine, signals[ERROR], 0, error_message);
}

void
_ctk_search_engine_set_recursive (CtkSearchEngine *engine,
                                  gboolean         recursive)
{
  g_return_if_fail (CTK_IS_SEARCH_ENGINE (engine));

  g_assert (!engine->priv->running);

  engine->priv->recursive = recursive;

  if (engine->priv->native)
    _ctk_search_engine_set_recursive (engine->priv->native, recursive);

  if (engine->priv->simple)
    _ctk_search_engine_set_recursive (engine->priv->simple, recursive);
}

gboolean
_ctk_search_engine_get_recursive (CtkSearchEngine *engine)
{
  g_return_val_if_fail (CTK_IS_SEARCH_ENGINE (engine), TRUE);

  return engine->priv->recursive;
}

void
_ctk_search_engine_set_model (CtkSearchEngine    *engine,
                              CtkFileSystemModel *model)
{
  g_clear_object (&engine->priv->model);
  if (model)
    {
      engine->priv->model = _ctk_search_engine_model_new (model);
      connect_engine_signals (engine->priv->model, engine);
      if (engine->priv->query)
        _ctk_search_engine_set_query (engine->priv->model, engine->priv->query);
    }
}
