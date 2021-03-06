/* ctkshortcutsgroupprivate.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_SHORTCUTS_GROUP_H__
#define __CTK_SHORTCUTS_GROUP_H__

#include <ctk/ctkbox.h>

G_BEGIN_DECLS

#define CTK_TYPE_SHORTCUTS_GROUP            (ctk_shortcuts_group_get_type ())
#define CTK_SHORTCUTS_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SHORTCUTS_GROUP, CtkShortcutsGroup))
#define CTK_SHORTCUTS_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SHORTCUTS_GROUP, CtkShortcutsGroupClass))
#define CTK_IS_SHORTCUTS_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SHORTCUTS_GROUP))
#define CTK_IS_SHORTCUTS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SHORTCUTS_GROUP))
#define CTK_SHORTCUTS_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SHORTCUTS_GROUP, CtkShortcutsGroupClass))


typedef struct _CtkShortcutsGroup         CtkShortcutsGroup;
typedef struct _CtkShortcutsGroupClass    CtkShortcutsGroupClass;

CDK_AVAILABLE_IN_3_20
GType ctk_shortcuts_group_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_SHORTCUTS_GROUP_H__ */
