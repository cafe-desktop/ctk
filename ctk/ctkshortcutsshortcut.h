/* ctkshortcutsshortcutprivate.h
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

#ifndef CTK_SHORTCUTS_SHORTCUT_H
#define CTK_SHORTCUTS_SHORTCUT_H

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_SHORTCUTS_SHORTCUT (ctk_shortcuts_shortcut_get_type())
#define CTK_SHORTCUTS_SHORTCUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SHORTCUTS_SHORTCUT, CtkShortcutsShortcut))
#define CTK_SHORTCUTS_SHORTCUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SHORTCUTS_SHORTCUT, CtkShortcutsShortcutClass))
#define CTK_IS_SHORTCUTS_SHORTCUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SHORTCUTS_SHORTCUT))
#define CTK_IS_SHORTCUTS_SHORTCUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SHORTCUTS_SHORTCUT))
#define CTK_SHORTCUTS_SHORTCUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SHORTCUTS_SHORTCUT, CtkShortcutsShortcutClass))


typedef struct _CtkShortcutsShortcut      CtkShortcutsShortcut;
typedef struct _CtkShortcutsShortcutClass CtkShortcutsShortcutClass;

/**
 * CtkShortcutType:
 * @CTK_SHORTCUT_ACCELERATOR:
 *   The shortcut is a keyboard accelerator. The #CtkShortcutsShortcut:accelerator
 *   property will be used.
 * @CTK_SHORTCUT_GESTURE_PINCH:
 *   The shortcut is a pinch gesture. CTK+ provides an icon and subtitle.
 * @CTK_SHORTCUT_GESTURE_STRETCH:
 *   The shortcut is a stretch gesture. CTK+ provides an icon and subtitle.
 * @CTK_SHORTCUT_GESTURE_ROTATE_CLOCKWISE:
 *   The shortcut is a clockwise rotation gesture. CTK+ provides an icon and subtitle.
 * @CTK_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE:
 *   The shortcut is a counterclockwise rotation gesture. CTK+ provides an icon and subtitle.
 * @CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT:
 *   The shortcut is a two-finger swipe gesture. CTK+ provides an icon and subtitle.
 * @CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT:
 *   The shortcut is a two-finger swipe gesture. CTK+ provides an icon and subtitle.
 * @CTK_SHORTCUT_GESTURE:
 *   The shortcut is a gesture. The #CtkShortcutsShortcut:icon property will be
 *   used.
 *
 * CtkShortcutType specifies the kind of shortcut that is being described.
 * More values may be added to this enumeration over time.
 *
 * Since: 3.20
 */
typedef enum {
  CTK_SHORTCUT_ACCELERATOR,
  CTK_SHORTCUT_GESTURE_PINCH,
  CTK_SHORTCUT_GESTURE_STRETCH,
  CTK_SHORTCUT_GESTURE_ROTATE_CLOCKWISE,
  CTK_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE,
  CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT,
  CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT,
  CTK_SHORTCUT_GESTURE
} CtkShortcutType;

CDK_AVAILABLE_IN_3_20
GType        ctk_shortcuts_shortcut_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* CTK_SHORTCUTS_SHORTCUT_H */
