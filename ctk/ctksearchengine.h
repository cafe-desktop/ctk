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

#include "ctkquery.h"
#include "ctkfilesystemmodel.h"
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_ENGINE		(_ctk_search_engine_get_type ())
#define CTK_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_ENGINE, CtkSearchEngine))
#define CTK_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_ENGINE, CtkSearchEngineClass))
#define CTK_IS_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_ENGINE))
#define CTK_IS_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_ENGINE))
#define CTK_SEARCH_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_ENGINE, CtkSearchEngineClass))

typedef struct _CtkSearchEngine CtkSearchEngine;
typedef struct _CtkSearchEngineClass CtkSearchEngineClass;
typedef struct _CtkSearchEnginePrivate CtkSearchEnginePrivate;
typedef struct _CtkSearchHit CtkSearchHit;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CtkSearchEngine, g_object_unref)

struct _CtkSearchHit
{
  GFile *file;
  GFileInfo *info; /* may be NULL */
};

struct _CtkSearchEngine
{
  GObject parent;

  CtkSearchEnginePrivate *priv;
};

struct _CtkSearchEngineClass 
{
  GObjectClass parent_class;
  
  /* VTable */
  void     (*set_query)       (CtkSearchEngine *engine, 
			       CtkQuery        *query);
  void     (*start)           (CtkSearchEngine *engine);
  void     (*stop)            (CtkSearchEngine *engine);
  
  /* Signals */
  void     (*hits_added)      (CtkSearchEngine *engine, 
			       GList           *hits);
  void     (*finished)        (CtkSearchEngine *engine);
  void     (*error)           (CtkSearchEngine *engine, 
			       const gchar     *error_message);
};

GType            _ctk_search_engine_get_type        (void);

CtkSearchEngine* _ctk_search_engine_new             (void);

void             _ctk_search_engine_set_query       (CtkSearchEngine *engine, 
                                                     CtkQuery        *query);
void	         _ctk_search_engine_start           (CtkSearchEngine *engine);
void	         _ctk_search_engine_stop            (CtkSearchEngine *engine);

void	         _ctk_search_engine_hits_added      (CtkSearchEngine *engine, 
						     GList           *hits);
void             _ctk_search_engine_finished        (CtkSearchEngine *engine,
                                                     gboolean         got_results);
void	         _ctk_search_engine_error           (CtkSearchEngine *engine, 
						     const gchar     *error_message);
void             _ctk_search_engine_set_recursive   (CtkSearchEngine *engine,
                                                     gboolean         recursive);
gboolean         _ctk_search_engine_get_recursive   (CtkSearchEngine *engine);

void             _ctk_search_hit_free (CtkSearchHit *hit);
CtkSearchHit    *_ctk_search_hit_dup (CtkSearchHit *hit);

void             _ctk_search_engine_set_model       (CtkSearchEngine    *engine,
                                                     CtkFileSystemModel *model);

G_END_DECLS

#endif /* __CTK_SEARCH_ENGINE_H__ */
