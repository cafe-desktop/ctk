/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __CTK_ACCESSIBILITY_H__
#define __CTK_ACCESSIBILITY_H__

#include <glib.h>
#include "ctk/ctkwidget.h"

G_BEGIN_DECLS

void      _ctk_accessibility_init        (void);

gboolean  _ctk_accessibility_key_snooper (CtkWidget   *widget,
                                          CdkEventKey *event);

G_END_DECLS

#endif /* __CTK_ACCESSIBILITY_H__ */
