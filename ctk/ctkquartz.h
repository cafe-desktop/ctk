/* ctkquartz.h: Utility functions used by the Quartz port
 *
 * Copyright (C) 2006 Imendio AB
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

#ifndef __CTK_QUARTZ_H__
#define __CTK_QUARTZ_H__

#import <Cocoa/Cocoa.h>
#include <ctk/ctkselection.h>

G_BEGIN_DECLS

NSSet   *_ctk_quartz_target_list_to_pasteboard_types    (CtkTargetList *target_list);
NSSet   *_ctk_quartz_target_entries_to_pasteboard_types (const CtkTargetEntry *targets,
							 guint                 n_targets);

GList   *_ctk_quartz_pasteboard_types_to_atom_list (NSArray  *array);

CtkSelectionData *_ctk_quartz_get_selection_data_from_pasteboard (NSPasteboard *pasteboard,
								  CdkAtom       target,
								  CdkAtom       selection);

void _ctk_quartz_set_selection_data_for_pasteboard (NSPasteboard *pasteboard,
						    CtkSelectionData *selection_data);

NSImage *_ctk_quartz_create_image_from_surface (cairo_surface_t *surface);

G_END_DECLS

#endif /* __CTK_QUARTZ_H__ */
