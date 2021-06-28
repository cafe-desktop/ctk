/* cdkkeys-quartz.h
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#ifndef __CDK_KEYS_QUARTZ_H__
#define __CDK_KEYS_QUARTZ_H__
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
typedef enum
  {
    CDK_QUARTZ_FLAGS_CHANGED = NSFlagsChanged,
    CDK_QUARTZ_KEY_UP = NSKeyUp,
    CDK_QUARTZ_KEY_DOWN = NSKeyDown,
    CDK_QUARTZ_MOUSE_ENTERED = NSMouseEntered,
    CDK_QUARTZ_MOUSE_EXITED = NSMouseExited,
    CDK_QUARTZ_SCROLL_WHEEL = NSScrollWheel,
    CDK_QUARTZ_MOUSE_MOVED = NSMouseMoved,
    CDK_QUARTZ_OTHER_MOUSE_DRAGGED = NSOtherMouseDragged,
    CDK_QUARTZ_RIGHT_MOUSE_DRAGGED = NSRightMouseDragged,
    CDK_QUARTZ_LEFT_MOUSE_DRAGGED = NSLeftMouseDragged,
    CDK_QUARTZ_OTHER_MOUSE_UP = NSOtherMouseUp,
    CDK_QUARTZ_RIGHT_MOUSE_UP = NSRightMouseUp,
    CDK_QUARTZ_LEFT_MOUSE_UP = NSLeftMouseUp,
    CDK_QUARTZ_OTHER_MOUSE_DOWN = NSOtherMouseDown,
    CDK_QUARTZ_RIGHT_MOUSE_DOWN = NSRightMouseDown,
    CDK_QUARTZ_LEFT_MOUSE_DOWN = NSLeftMouseDown,
  } CdkQuartzEventType;

typedef enum
  {
    CDK_QUARTZ_ALTERNATE_KEY_MASK = NSAlternateKeyMask,
    CDK_QUARTZ_CONTROL_KEY_MASK = NSControlKeyMask,
    CDK_QUARTZ_SHIFT_KEY_MASK = NSShiftKeyMask,
    CDK_QUARTZ_ALPHA_SHIFT_KEY_MASK = NSAlphaShiftKeyMask,
    CDK_QUARTZ_COMMAND_KEY_MASK = NSCommandKeyMask,
    CDK_QUARTZ_ANY_EVENT_MASK = NSAnyEventMask,
  } CdkQuartzEventModifierFlags;


#else
typedef enum
  {
    CDK_QUARTZ_FLAGS_CHANGED = NSEventTypeFlagsChanged,
    CDK_QUARTZ_KEY_UP = NSEventTypeKeyUp,
    CDK_QUARTZ_KEY_DOWN = NSEventTypeKeyDown,
    CDK_QUARTZ_MOUSE_ENTERED = NSEventTypeMouseEntered,
    CDK_QUARTZ_MOUSE_EXITED = NSEventTypeMouseExited,
    CDK_QUARTZ_SCROLL_WHEEL = NSEventTypeScrollWheel,
    CDK_QUARTZ_MOUSE_MOVED = NSEventTypeMouseMoved,
    CDK_QUARTZ_OTHER_MOUSE_DRAGGED = NSEventTypeOtherMouseDragged,
    CDK_QUARTZ_RIGHT_MOUSE_DRAGGED = NSEventTypeRightMouseDragged,
    CDK_QUARTZ_LEFT_MOUSE_DRAGGED = NSEventTypeLeftMouseDragged,
    CDK_QUARTZ_OTHER_MOUSE_UP = NSEventTypeOtherMouseUp,
    CDK_QUARTZ_RIGHT_MOUSE_UP = NSEventTypeRightMouseUp,
    CDK_QUARTZ_LEFT_MOUSE_UP = NSEventTypeLeftMouseUp,
    CDK_QUARTZ_OTHER_MOUSE_DOWN = NSEventTypeOtherMouseDown,
    CDK_QUARTZ_RIGHT_MOUSE_DOWN = NSEventTypeRightMouseDown,
    CDK_QUARTZ_LEFT_MOUSE_DOWN = NSEventTypeLeftMouseDown,
  } CdkQuartzEventType;

typedef enum
  {
   CDK_QUARTZ_ALTERNATE_KEY_MASK = NSEventModifierFlagOption,
   CDK_QUARTZ_CONTROL_KEY_MASK = NSEventModifierFlagControl,
   CDK_QUARTZ_SHIFT_KEY_MASK = NSEventModifierFlagShift,
   CDK_QUARTZ_ALPHA_SHIFT_KEY_MASK = NSEventModifierFlagCapsLock,
   CDK_QUARTZ_COMMAND_KEY_MASK = NSEventModifierFlagCommand,
  } CdkQuartzEventModifierFlags;


#endif
#endif /* __CDK_KEYS_QUARTZ_H__ */
