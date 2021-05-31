/* GTK - The GIMP Toolkit
 * gtkrecentchooserdefault.h
 * Copyright (C) 2006 Emmanuele Bassi
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

#ifndef __CTK_RECENT_CHOOSER_DEFAULT_H__
#define __CTK_RECENT_CHOOSER_DEFAULT_H__

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS


#define CTK_TYPE_RECENT_CHOOSER_DEFAULT    (_ctk_recent_chooser_default_get_type ())
#define CTK_RECENT_CHOOSER_DEFAULT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_CHOOSER_DEFAULT, GtkRecentChooserDefault))
#define CTK_IS_RECENT_CHOOSER_DEFAULT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_CHOOSER_DEFAULT))


typedef struct _GtkRecentChooserDefault GtkRecentChooserDefault;

GType      _ctk_recent_chooser_default_get_type (void) G_GNUC_CONST;
GtkWidget *_ctk_recent_chooser_default_new      (GtkRecentManager *recent_manager);


G_END_DECLS

#endif /* __CTK_RECENT_CHOOSER_DEFAULT_H__ */