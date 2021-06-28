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
#include <cdk/cdkinternals.h>
#include "cdkprivate-broadway.h"

void
_cdk_broadway_window_sync_rendering (CdkWindow *window)
{
  /* FIXME: Find out if there is a way to implement this on broadway. */
}

gboolean
_cdk_broadway_window_simulate_key (CdkWindow      *window,
				   gint            x,
				   gint            y,
				   guint           keyval,
				   CdkModifierType modifiers,
				   CdkEventType    key_pressrelease)
{
  g_return_val_if_fail (key_pressrelease == CDK_KEY_PRESS || key_pressrelease == CDK_KEY_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  if (!CDK_WINDOW_IS_MAPPED (window))
    return FALSE;

  /* FIXME: Implement. */

  return FALSE;
}

gboolean
_cdk_broadway_window_simulate_button (CdkWindow      *window,
				      gint            x,
				      gint            y,
				      guint           button, /*1..3*/
				      CdkModifierType modifiers,
				      CdkEventType    button_pressrelease)
{
  g_return_val_if_fail (button_pressrelease == CDK_BUTTON_PRESS || button_pressrelease == CDK_BUTTON_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  if (!CDK_WINDOW_IS_MAPPED (window))
    return FALSE;

  /* FIXME: Implement. */

  return FALSE;
}
