/*
 * Copyright (C) 2020 Red Hat Inc
 * Copyright (C) 2009-2011 Nokia <ivan.frade@nokia.com>
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
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *          Jürg Billeter <juerg.billeter@codethink.co.uk>
 *          Martyn Russell <martyn@lanedo.com>
 *
 * Based on nautilus-search-engine-tracker.c
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>
#include <gmodule.h>
#include <gdk/gdk.h>
#include <ctk/ctk.h>
#include <libtracker-sparql/tracker-sparql.h>

#include "ctksearchenginetracker3.h"

#define MINER_FS_BUS_NAME "org.freedesktop.Tracker3.Miner.Files"

#define SEARCH_QUERY_BASE(__PATTERN__)                                 \
  "SELECT ?url "                                                       \
  "       nfo:fileName(?urn) "					       \
  "       nie:mimeType(?urn)"					       \
  "       nfo:fileSize(?urn)"					       \
  "       nfo:fileLastModified(?urn)"				       \
  "FROM tracker:FileSystem "                                           \
  "WHERE {"                                                            \
  "  ?urn a nfo:FileDataObject ;"                                      \
  "       nie:url ?url ; "                                             \
  "       fts:match ~match . "                                         \
  __PATTERN__                                                          \
  "} "                                                                 \
  "ORDER BY DESC(fts:rank(?urn)) DESC(?url)"

#define SEARCH_QUERY SEARCH_QUERY_BASE("")
#define SEARCH_RECURSIVE_QUERY SEARCH_QUERY_BASE("?urn (nfo:belongsToContainer/nie:isStoredAs)+/nie:url ~location")
#define SEARCH_LOCATION_QUERY SEARCH_QUERY_BASE("?urn nfo:belongsToContainer/nie:isStoredAs/nie:url ~location")
#define FILE_CHECK_QUERY "ASK { ?urn nie:url ~url }"

struct _CtkSearchEngineTracker3
{
  CtkSearchEngine parent;
  TrackerSparqlConnection *sparql_conn;
  TrackerSparqlStatement *search_query;
  TrackerSparqlStatement *search_recursive_query;
  TrackerSparqlStatement *search_location_query;
  TrackerSparqlStatement *file_check_query;
  GCancellable *cancellable;
  CtkQuery *query;
  gboolean query_pending;
};

struct _CtkSearchEngineTracker3Class
{
  CtkSearchEngineClass parent_class;
};

static void ctk_search_engine_tracker3_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkSearchEngineTracker3,
                         ctk_search_engine_tracker3,
                         CTK_TYPE_SEARCH_ENGINE,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                ctk_search_engine_tracker3_initable_iface_init))

static void
finalize (GObject *object)
{
  CtkSearchEngineTracker3 *engine;

  g_debug ("Finalizing CtkSearchEngineTracker3");

  engine = CTK_SEARCH_ENGINE_TRACKER3 (object);

  if (engine->cancellable)
    {
      g_cancellable_cancel (engine->cancellable);
      g_object_unref (engine->cancellable);
    }

  g_clear_object (&engine->search_query);
  g_clear_object (&engine->search_location_query);
  g_clear_object (&engine->file_check_query);
  tracker_sparql_connection_close (engine->sparql_conn);
  g_clear_object (&engine->sparql_conn);

  G_OBJECT_CLASS (ctk_search_engine_tracker3_parent_class)->finalize (object);
}

static void
free_hit (gpointer data)
{
  CtkSearchHit *hit = data;

  g_clear_object (&hit->file);
  g_clear_object (&hit->info);
  g_slice_free (CtkSearchHit, hit);
}

static GFileInfo *
create_file_info (TrackerSparqlCursor *cursor)
{
  GFileInfo *info;
  const gchar *str;
  GDateTime *creation;

  info = g_file_info_new ();
  str = tracker_sparql_cursor_get_string (cursor, 1, NULL);
  if (str)
    g_file_info_set_display_name (info, str);

  str = tracker_sparql_cursor_get_string (cursor, 2, NULL);
  if (str)
    g_file_info_set_content_type (info, str);

  g_file_info_set_size (info,
                        tracker_sparql_cursor_get_integer (cursor, 3));

  str = tracker_sparql_cursor_get_string (cursor, 4, NULL);
  if (str)
    {
      creation = g_date_time_new_from_iso8601 (str, NULL);
      g_file_info_set_modification_date_time (info, creation);
      g_date_time_unref (creation);
    }

  return info;
}

static void
query_callback (TrackerSparqlStatement *statement,
                GAsyncResult           *res,
                gpointer                user_data)
{
  CtkSearchEngineTracker3 *engine;
  TrackerSparqlCursor *cursor;
  GList *hits = NULL;
  GError *error = NULL;
  CtkSearchHit *hit;

  engine = CTK_SEARCH_ENGINE_TRACKER3 (user_data);

  engine->query_pending = FALSE;

  cursor = tracker_sparql_statement_execute_finish (statement, res, &error);

  if (!cursor)
    {
      _ctk_search_engine_error (CTK_SEARCH_ENGINE (engine), error->message);
      g_error_free (error);
      g_object_unref (engine);
      return;
    }

  while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      const gchar *url;

      url = tracker_sparql_cursor_get_string (cursor, 0, NULL);
      hit = g_slice_new0 (CtkSearchHit);
      hit->file = g_file_new_for_uri (url);
      hit->info = create_file_info (cursor);
      hits = g_list_prepend (hits, hit);
    }

  tracker_sparql_cursor_close (cursor);

  _ctk_search_engine_hits_added (CTK_SEARCH_ENGINE (engine), hits);
  _ctk_search_engine_finished (CTK_SEARCH_ENGINE (engine), hits != NULL);

  g_list_free_full (hits, free_hit);
  g_object_unref (engine);
  g_object_unref (cursor);
}

static void
ctk_search_engine_tracker3_start (CtkSearchEngine *engine)
{
  CtkSearchEngineTracker3 *tracker;
  TrackerSparqlStatement *statement;
  const gchar *search_text;
  gboolean recursive;
  gchar *match;
  GFile *location;

  tracker = CTK_SEARCH_ENGINE_TRACKER3 (engine);

  if (tracker->query_pending)
    {
      g_debug ("Attempt to start a new search while one is pending, doing nothing");
      return;
    }

  if (tracker->query == NULL)
    {
      g_debug ("Attempt to start a new search with no CtkQuery, doing nothing");
      return;
    }

  tracker->query_pending = TRUE;
  search_text = ctk_query_get_text (tracker->query);
  location = ctk_query_get_location (tracker->query);
  recursive = _ctk_search_engine_get_recursive (engine);

  if (location)
    {
      gchar *location_uri = g_file_get_uri (location);

      if (recursive)
        {
          g_debug ("Recursive search query in location: %s", location_uri);
          statement = tracker->search_recursive_query;
        }
      else
        {
          g_debug ("Search query in location: %s", location_uri);
          statement = tracker->search_location_query;
        }

      tracker_sparql_statement_bind_string (statement,
                                            "location",
                                            location_uri);
      g_free (location_uri);
    }
  else
    {
      g_debug ("Search query");
      statement = tracker->search_query;
    }

  match = g_strdup_printf ("%s*", search_text);
  tracker_sparql_statement_bind_string (statement, "match", match);
  g_debug ("search text: %s\n", match);
  tracker_sparql_statement_execute_async (statement, tracker->cancellable,
                                          (GAsyncReadyCallback) query_callback,
                                          g_object_ref (tracker));
  g_free (match);
}

static void
ctk_search_engine_tracker3_stop (CtkSearchEngine *engine)
{
  CtkSearchEngineTracker3 *tracker;

  tracker = CTK_SEARCH_ENGINE_TRACKER3 (engine);

  if (tracker->query && tracker->query_pending)
    {
      g_cancellable_cancel (tracker->cancellable);
      tracker->query_pending = FALSE;
    }
}

static void
ctk_search_engine_tracker3_set_query (CtkSearchEngine *engine,
                                      CtkQuery        *query)
{
  CtkSearchEngineTracker3 *tracker;

  tracker = CTK_SEARCH_ENGINE_TRACKER3 (engine);

  if (query)
    g_object_ref (query);

  if (tracker->query)
    g_object_unref (tracker->query);

  tracker->query = query;
}

static void
ctk_search_engine_tracker3_class_init (CtkSearchEngineTracker3Class *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkSearchEngineClass *engine_class = CTK_SEARCH_ENGINE_CLASS (class);

  gobject_class->finalize = finalize;

  engine_class->set_query = ctk_search_engine_tracker3_set_query;
  engine_class->start = ctk_search_engine_tracker3_start;
  engine_class->stop = ctk_search_engine_tracker3_stop;
}

static void
ctk_search_engine_tracker3_init (CtkSearchEngineTracker3 *engine)
{
  engine->cancellable = g_cancellable_new ();
  engine->query_pending = FALSE;
}

static gboolean
ctk_search_engine_tracker3_initable_init (GInitable     *initable,
                                          GCancellable  *cancellable,
                                          GError       **error)
{
  CtkSearchEngineTracker3 *engine;

  engine = CTK_SEARCH_ENGINE_TRACKER3 (initable);

  engine->sparql_conn = tracker_sparql_connection_bus_new (MINER_FS_BUS_NAME,
                                                           NULL, NULL,
                                                           error);
  if (!engine->sparql_conn)
    return FALSE;

  engine->search_query =
    tracker_sparql_connection_query_statement (engine->sparql_conn,
                                               SEARCH_QUERY,
                                               cancellable,
                                               error);
  if (!engine->search_query)
    return FALSE;

  engine->search_recursive_query =
    tracker_sparql_connection_query_statement (engine->sparql_conn,
                                               SEARCH_RECURSIVE_QUERY,
                                               cancellable,
                                               error);
  if (!engine->search_recursive_query)
    return FALSE;

  engine->search_location_query =
    tracker_sparql_connection_query_statement (engine->sparql_conn,
                                               SEARCH_LOCATION_QUERY,
                                               cancellable,
                                               error);
  if (!engine->search_location_query)
    return FALSE;

  engine->file_check_query =
    tracker_sparql_connection_query_statement (engine->sparql_conn,
                                               FILE_CHECK_QUERY,
                                               cancellable,
                                               error);
  if (!engine->file_check_query)
    return FALSE;

  return TRUE;
}

static void
ctk_search_engine_tracker3_initable_iface_init (GInitableIface *iface)
{
  iface->init = ctk_search_engine_tracker3_initable_init;
}

CtkSearchEngine *
ctk_search_engine_tracker3_new (void)
{
  CtkSearchEngineTracker3 *engine;
  GError *error = NULL;
  GModule *self;

  self = g_module_open (NULL, G_MODULE_BIND_LAZY);

  /* Avoid hell from breaking loose if the application links to Tracker 2.x */
  if (self)
    {
      gpointer symbol;
      gboolean found;

      found = g_module_symbol (self, "tracker_sparql_builder_new", &symbol);
      g_module_close (self);

      if (found)
        return NULL;
    }

  g_debug ("Creating CtkSearchEngineTracker3...");

  engine = g_initable_new (CTK_TYPE_SEARCH_ENGINE_TRACKER3,
                           NULL, &error, NULL);
  if (!engine)
    {
      g_critical ("Could not init tracker3 search engine: %s",
                  error->message);
      g_error_free (error);
    }

  return CTK_SEARCH_ENGINE (engine);
}

gboolean
ctk_search_engine_tracker3_is_indexed (GFile    *location,
                                       gpointer  data)
{
  CtkSearchEngineTracker3 *engine = data;
  TrackerSparqlCursor *cursor;
  GError *error = NULL;
  gboolean indexed;
  gchar *uri;

  uri = g_file_get_uri (location);
  tracker_sparql_statement_bind_string (engine->file_check_query,
                                        "url", uri);
  cursor = tracker_sparql_statement_execute (engine->file_check_query,
                                             engine->cancellable, &error);
  g_free (uri);

  if (!cursor ||
      !tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      g_warning ("Error checking indexed file '%s': %s",
                 uri, error->message);
      g_error_free (error);
      g_free (uri);
      return FALSE;
    }

  indexed = tracker_sparql_cursor_get_boolean (cursor, 0);
  tracker_sparql_cursor_close (cursor);
  g_object_unref (cursor);

  return indexed;
}
