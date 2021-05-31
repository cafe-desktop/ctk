/*
 * Copyright (C) 2015 Red Hat, Inc
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
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include <gio/gio.h>

#include <gdk/gdk.h>

#include "ctksearchenginemodel.h"
#include "ctkprivate.h"

#include <string.h>

#define BATCH_SIZE 500

struct _CtkSearchEngineModel
{
  CtkSearchEngine parent;

  CtkFileSystemModel *model;
  CtkQuery *query;

  gboolean query_finished;
  guint idle;
};

struct _CtkSearchEngineModelClass
{
  CtkSearchEngineClass parent_class;
};

G_DEFINE_TYPE (CtkSearchEngineModel, _ctk_search_engine_model, CTK_TYPE_SEARCH_ENGINE)

static void
ctk_search_engine_model_dispose (GObject *object)
{
  CtkSearchEngineModel *model = CTK_SEARCH_ENGINE_MODEL (object);

  g_clear_object (&model->query);
  g_clear_object (&model->model);

  G_OBJECT_CLASS (_ctk_search_engine_model_parent_class)->dispose (object);
}

gboolean
info_matches_query (CtkQuery  *query,
                    GFileInfo *info)
{
  const gchar *display_name;

  display_name = g_file_info_get_display_name (info);
  if (display_name == NULL)
    return FALSE;

  if (g_file_info_get_is_hidden (info))
    return FALSE;

  if (!ctk_query_matches_string (query, display_name))
    return FALSE;

  return TRUE;
}

static gboolean
do_search (gpointer data)
{
  CtkSearchEngineModel *model = data;
  CtkTreeIter iter;
  GList *hits = NULL;

  if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (model->model), &iter))
    {
      do
        {
          GFileInfo *info;

          info = _ctk_file_system_model_get_info (model->model, &iter);
          if (info_matches_query (model->query, info))
            {
              GFile *file;
              CtkSearchHit *hit;

              file = _ctk_file_system_model_get_file (model->model, &iter);
              hit = g_new (CtkSearchHit, 1);
              hit->file = g_object_ref (file);
              hit->info = g_object_ref (info);
              hits = g_list_prepend (hits, hit);
            }
        }
      while (ctk_tree_model_iter_next (CTK_TREE_MODEL (model->model), &iter));

      if (hits)
        {
          _ctk_search_engine_hits_added (CTK_SEARCH_ENGINE (model), hits);
          g_list_free_full (hits, (GDestroyNotify)_ctk_search_hit_free);
        }
    }

  model->idle = 0;

  return G_SOURCE_REMOVE;
}

static void
ctk_search_engine_model_start (CtkSearchEngine *engine)
{
  CtkSearchEngineModel *model;

  model = CTK_SEARCH_ENGINE_MODEL (engine);

  if (model->query == NULL)
    return;

  model->idle = gdk_threads_add_idle (do_search, engine);
  g_source_set_name_by_id (model->idle, "[ctk+] ctk_search_engine_model_start");
}

static void
ctk_search_engine_model_stop (CtkSearchEngine *engine)
{
  CtkSearchEngineModel *model;

  model = CTK_SEARCH_ENGINE_MODEL (engine);

  if (model->idle != 0)
    {
      g_source_remove (model->idle);
      model->idle = 0;
    }
}

static void
ctk_search_engine_model_set_query (CtkSearchEngine *engine,
                                   CtkQuery        *query)
{
  CtkSearchEngineModel *model = CTK_SEARCH_ENGINE_MODEL (engine);

  g_set_object (&model->query, query);
}

static void
_ctk_search_engine_model_class_init (CtkSearchEngineModelClass *class)
{
  GObjectClass *gobject_class;
  CtkSearchEngineClass *engine_class;

  gobject_class = G_OBJECT_CLASS (class);
  gobject_class->dispose = ctk_search_engine_model_dispose;

  engine_class = CTK_SEARCH_ENGINE_CLASS (class);
  engine_class->set_query = ctk_search_engine_model_set_query;
  engine_class->start = ctk_search_engine_model_start;
  engine_class->stop = ctk_search_engine_model_stop;
}

static void
_ctk_search_engine_model_init (CtkSearchEngineModel *engine)
{
}

CtkSearchEngine *
_ctk_search_engine_model_new (CtkFileSystemModel *model)
{
  CtkSearchEngineModel *engine;

  engine = CTK_SEARCH_ENGINE_MODEL (g_object_new (CTK_TYPE_SEARCH_ENGINE_MODEL, NULL));
  engine->model = g_object_ref (model);

  return CTK_SEARCH_ENGINE (engine);
}
