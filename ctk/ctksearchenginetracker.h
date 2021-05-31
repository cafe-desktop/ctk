/*
 * Copyright (C) 2005 Mr Jamie McCracken
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
 * Author: Jamie McCracken (jamiemcc@gnome.org)
 *
 * Based on nautilus-search-engine-tracker.h
 */

#ifndef __CTK_SEARCH_ENGINE_TRACKER_H__
#define __CTK_SEARCH_ENGINE_TRACKER_H__

#include "ctksearchengine.h"

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_ENGINE_TRACKER		(_ctk_search_engine_tracker_get_type ())
#define CTK_SEARCH_ENGINE_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_ENGINE_TRACKER, GtkSearchEngineTracker))
#define CTK_SEARCH_ENGINE_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_ENGINE_TRACKER, GtkSearchEngineTrackerClass))
#define CTK_IS_SEARCH_ENGINE_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_ENGINE_TRACKER))
#define CTK_IS_SEARCH_ENGINE_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_ENGINE_TRACKER))
#define CTK_SEARCH_ENGINE_TRACKER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_ENGINE_TRACKER, GtkSearchEngineTrackerClass))

typedef struct _GtkSearchEngineTracker GtkSearchEngineTracker;
typedef struct _GtkSearchEngineTrackerClass GtkSearchEngineTrackerClass;

GType            _ctk_search_engine_tracker_get_type (void);

GtkSearchEngine* _ctk_search_engine_tracker_new      (void);

gboolean         _ctk_search_engine_tracker_is_indexed (GFile    *file,
                                                        gpointer  data);
G_END_DECLS

#endif /* __CTK_SEARCH_ENGINE_TRACKER_H__ */
