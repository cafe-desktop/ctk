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
 * Based on nautilus-search-engine.h
 */

#ifndef __CTK_SEARCH_ENGINE_H__
#define __CTK_SEARCH_ENGINE_H__

#include "gtkquery.h"
#include "gtkfilesystemmodel.h"
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_ENGINE		(_ctk_search_engine_get_type ())
#define CTK_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_ENGINE, GtkSearchEngine))
#define CTK_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_ENGINE, GtkSearchEngineClass))
#define CTK_IS_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_ENGINE))
#define CTK_IS_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_ENGINE))
#define CTK_SEARCH_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_ENGINE, GtkSearchEngineClass))

typedef struct _GtkSearchEngine GtkSearchEngine;
typedef struct _GtkSearchEngineClass GtkSearchEngineClass;
typedef struct _GtkSearchEnginePrivate GtkSearchEnginePrivate;
typedef struct _GtkSearchHit GtkSearchHit;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkSearchEngine, g_object_unref)

struct _GtkSearchHit
{
  GFile *file;
  GFileInfo *info; /* may be NULL */
};

struct _GtkSearchEngine
{
  GObject parent;

  GtkSearchEnginePrivate *priv;
};

struct _GtkSearchEngineClass 
{
  GObjectClass parent_class;
  
  /* VTable */
  void     (*set_query)       (GtkSearchEngine *engine, 
			       GtkQuery        *query);
  void     (*start)           (GtkSearchEngine *engine);
  void     (*stop)            (GtkSearchEngine *engine);
  
  /* Signals */
  void     (*hits_added)      (GtkSearchEngine *engine, 
			       GList           *hits);
  void     (*finished)        (GtkSearchEngine *engine);
  void     (*error)           (GtkSearchEngine *engine, 
			       const gchar     *error_message);
};

GType            _ctk_search_engine_get_type        (void);

GtkSearchEngine* _ctk_search_engine_new             (void);

void             _ctk_search_engine_set_query       (GtkSearchEngine *engine, 
                                                     GtkQuery        *query);
void	         _ctk_search_engine_start           (GtkSearchEngine *engine);
void	         _ctk_search_engine_stop            (GtkSearchEngine *engine);

void	         _ctk_search_engine_hits_added      (GtkSearchEngine *engine, 
						     GList           *hits);
void             _ctk_search_engine_finished        (GtkSearchEngine *engine,
                                                     gboolean         got_results);
void	         _ctk_search_engine_error           (GtkSearchEngine *engine, 
						     const gchar     *error_message);
void             _ctk_search_engine_set_recursive   (GtkSearchEngine *engine,
                                                     gboolean         recursive);
gboolean         _ctk_search_engine_get_recursive   (GtkSearchEngine *engine);

void             _ctk_search_hit_free (GtkSearchHit *hit);
GtkSearchHit    *_ctk_search_hit_dup (GtkSearchHit *hit);

void             _ctk_search_engine_set_model       (GtkSearchEngine    *engine,
                                                     GtkFileSystemModel *model);

G_END_DECLS

#endif /* __CTK_SEARCH_ENGINE_H__ */