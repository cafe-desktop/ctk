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
 * Based on nautilus-search-engine-simple.h
 */

#ifndef __CTK_SEARCH_ENGINE_SIMPLE_H__
#define __CTK_SEARCH_ENGINE_SIMPLE_H__

#include "ctksearchengine.h"

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_ENGINE_SIMPLE		(_ctk_search_engine_simple_get_type ())
#define CTK_SEARCH_ENGINE_SIMPLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_ENGINE_SIMPLE, CtkSearchEngineSimple))
#define CTK_SEARCH_ENGINE_SIMPLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_ENGINE_SIMPLE, CtkSearchEngineSimpleClass))
#define CTK_IS_SEARCH_ENGINE_SIMPLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_ENGINE_SIMPLE))
#define CTK_IS_SEARCH_ENGINE_SIMPLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_ENGINE_SIMPLE))
#define CTK_SEARCH_ENGINE_SIMPLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_ENGINE_SIMPLE, CtkSearchEngineSimpleClass))

typedef struct _CtkSearchEngineSimple CtkSearchEngineSimple;
typedef struct _CtkSearchEngineSimpleClass CtkSearchEngineSimpleClass;

GType            _ctk_search_engine_simple_get_type (void);

CtkSearchEngine* _ctk_search_engine_simple_new      (void);

typedef gboolean (*CtkSearchEngineSimpleIsIndexed) (GFile *location, gpointer data);

void             _ctk_search_engine_simple_set_indexed_cb (CtkSearchEngineSimple *engine,
                                                           CtkSearchEngineSimpleIsIndexed callback,
                                                           gpointer                       data,
                                                           GDestroyNotify                 destroy);

G_END_DECLS

#endif /* __CTK_SEARCH_ENGINE_SIMPLE_H__ */
