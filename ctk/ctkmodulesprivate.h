/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_MODULES_PRIVATE_H__
#define __CTK_MODULES_PRIVATE_H__

G_BEGIN_DECLS

#include "ctksettings.h"

gchar  * _ctk_find_module              (const gchar  *name,
                                        const gchar  *type);
gchar ** _ctk_get_module_path          (const gchar  *type);

void     _ctk_modules_init             (gint          *argc,
                                        gchar       ***argv,
                                        const gchar   *ctk_modules_args);
void     _ctk_modules_settings_changed (CtkSettings   *settings,
                                        const gchar   *modules);

gboolean _ctk_module_has_mixed_deps    (GModule       *module);

G_END_DECLS

#endif /* __CTK_MODULES_PRIVATE_H__ */
