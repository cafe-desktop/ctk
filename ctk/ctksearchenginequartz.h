/*
 * Copyright (C) 2007  Kristian Rietveld  <kris@ctk.org>
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

#ifndef __CTK_SEARCH_ENGINE_QUARTZ_H__
#define __CTK_SEARCH_ENGINE_QUARTZ_H__

#include "ctksearchengine.h"

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_ENGINE_QUARTZ			(_ctk_search_engine_quartz_get_type ())
#define CTK_SEARCH_ENGINE_QUARTZ(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_ENGINE_QUARTZ, CtkSearchEngineQuartz))
#define CTK_SEARCH_ENGINE_QUARTZ_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_ENGINE_QUARTZ, CtkSearchEngineQuartzClass))
#define CTK_IS_SEARCH_ENGINE_QUARTZ(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_ENGINE_QUARTZ))
#define CTK_IS_SEARCH_ENGINE_QUARTZ_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_ENGINE_QUARTZ))
#define CTK_SEARCH_ENGINE_QUARTZ_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_ENGINE_QUARTZ, CtkSearchEngineQuartzClass))

typedef struct _CtkSearchEngineQuartz CtkSearchEngineQuartz;
typedef struct _CtkSearchEngineQuartzClass CtkSearchEngineQuartzClass;
typedef struct _CtkSearchEngineQuartzPrivate CtkSearchEngineQuartzPrivate;

struct _CtkSearchEngineQuartz
{
  CtkSearchEngine parent;

  CtkSearchEngineQuartzPrivate *priv;
};

struct _CtkSearchEngineQuartzClass
{
  CtkSearchEngineClass parent_class;
};

GType            _ctk_search_engine_quartz_get_type (void);
CtkSearchEngine *_ctk_search_engine_quartz_new      (void);

G_END_DECLS

#endif /* __CTK_SEARCH_ENGINE_QUARTZ_H__ */
