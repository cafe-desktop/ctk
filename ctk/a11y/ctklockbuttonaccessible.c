/* CTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include "ctklockbuttonaccessibleprivate.h"

#include "ctk/ctklockbuttonprivate.h"
#include "ctk/ctkwidgetprivate.h"

G_DEFINE_TYPE (CtkLockButtonAccessible, ctk_lock_button_accessible, CTK_TYPE_BUTTON_ACCESSIBLE)

static const gchar *
ctk_lock_button_accessible_get_name (AtkObject *obj)
{
  CtkLockButton *lockbutton;

  lockbutton = CTK_LOCK_BUTTON (ctk_accessible_get_widget (CTK_ACCESSIBLE (obj)));
  if (lockbutton == NULL)
    return NULL;

  return _ctk_lock_button_get_current_text (lockbutton);
}

static void
ctk_lock_button_accessible_class_init (CtkLockButtonAccessibleClass *klass)
{
  AtkObjectClass *atk_object_class = ATK_OBJECT_CLASS (klass);

  atk_object_class->get_name = ctk_lock_button_accessible_get_name;
}

static void
ctk_lock_button_accessible_init (CtkLockButtonAccessible *lockbutton G_GNUC_UNUSED)
{
}

void
_ctk_lock_button_accessible_name_changed (CtkLockButton *lockbutton)
{
  AtkObject *obj;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (lockbutton));
  if (obj == NULL)
    return;

  g_object_notify (G_OBJECT (obj), "accessible-name");
}

