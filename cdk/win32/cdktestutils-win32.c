/* Ctk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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
#include "config.h"

#include <cdk/cdktestutils.h>
#include <cdk/cdkkeysyms.h>
#include <win32/cdkwin32.h>


gboolean
_cdk_win32_window_simulate_key (CdkWindow      *window,
                       gint            x,
                       gint            y,
                       guint           keyval,
                       CdkModifierType modifiers,
                       CdkEventType    key_pressrelease)
{
  gboolean      success = FALSE;
  CdkKeymapKey *keys    = NULL;
  gint          n_keys  = 0;
  INPUT         ip;
  gint          i;

  g_return_val_if_fail (key_pressrelease == CDK_KEY_PRESS || key_pressrelease == CDK_KEY_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;

  switch (key_pressrelease)
    {
    case CDK_KEY_PRESS:
      ip.ki.dwFlags = 0;
      break;
    case CDK_KEY_RELEASE:
      ip.ki.dwFlags = KEYEVENTF_KEYUP;
      break;
    default:
      /* Not a key event. */
      return FALSE;
    }
  if (cdk_keymap_get_entries_for_keyval (cdk_keymap_get_default (), keyval, &keys, &n_keys))
    {
      for (i = 0; i < n_keys; i++)
        {
          if (key_pressrelease == CDK_KEY_PRESS)
            {
              /* AltGr press. */
              if (keys[i].group)
                {
                  /* According to some virtualbox code I found, AltGr is
                   * simulated on win32 with LCtrl+RAlt */
                  ip.ki.wVk = VK_CONTROL;
                  SendInput(1, &ip, sizeof(INPUT));
                  ip.ki.wVk = VK_MENU;
                  SendInput(1, &ip, sizeof(INPUT));
                }
              /* Shift press. */
              if (keys[i].level || (modifiers & CDK_SHIFT_MASK))
                {
                  ip.ki.wVk = VK_SHIFT;
                  SendInput(1, &ip, sizeof(INPUT));
                }
            }

          /* Key pressed/released. */
          ip.ki.wVk = keys[i].keycode;
          SendInput(1, &ip, sizeof(INPUT));

          if (key_pressrelease == CDK_KEY_RELEASE)
            {
              /* Shift release. */
              if (keys[i].level || (modifiers & CDK_SHIFT_MASK))
                {
                  ip.ki.wVk = VK_SHIFT;
                  SendInput(1, &ip, sizeof(INPUT));
                }
              /* AltrGr release. */
              if (keys[i].group)
                {
                  ip.ki.wVk = VK_MENU;
                  SendInput(1, &ip, sizeof(INPUT));
                  ip.ki.wVk = VK_CONTROL;
                  SendInput(1, &ip, sizeof(INPUT));
                }
            }

          /* No need to loop for alternative keycodes. We want only one
           * key generated. */
          success = TRUE;
          break;
        }
      g_free (keys);
    }
  return success;
}

gboolean
_cdk_win32_window_simulate_button (CdkWindow      *window,
                          gint            x,
                          gint            y,
                          guint           button, /*1..3*/
                          CdkModifierType modifiers,
                          CdkEventType    button_pressrelease)
{
  g_return_val_if_fail (button_pressrelease == CDK_BUTTON_PRESS || button_pressrelease == CDK_BUTTON_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  return FALSE;
}
