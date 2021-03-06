/*
 * Copyright (C) 2020 Red Hat Inc
 *               2005 Mr Jamie McCracken
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
 * Author: Carlos Garnacho <carlosg@gnome.org>
 *         Jamie McCracken (jamiemcc@gnome.org)
 *
 * Based on nautilus-search-engine-tracker.h
 */

#ifndef __CTK_SEARCH_ENGINE_TRACKER3_H__
#define __CTK_SEARCH_ENGINE_TRACKER3_H__

#include "ctksearchengine.h"

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_ENGINE_TRACKER3 (ctk_search_engine_tracker3_get_type ())
G_DECLARE_FINAL_TYPE (CtkSearchEngineTracker3,
                      ctk_search_engine_tracker3,
                      CTK, SEARCH_ENGINE_TRACKER3,
                      CtkSearchEngine)

GType            ctk_search_engine_tracker3_get_type (void);

CtkSearchEngine* ctk_search_engine_tracker3_new      (void);

gboolean         ctk_search_engine_tracker3_is_indexed (GFile    *file,
                                                        gpointer  data);
G_END_DECLS

#endif /* __CTK_SEARCH_ENGINE_TRACKER3_H__ */
