/* GTK - The GIMP Toolkit
 * gtktrashmonitor.h: Monitor the trash:/// folder to see if there is trash or not
 * Copyright (C) 2011 Suse
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Federico Mena Quintero <federico@gnome.org>
 */

#ifndef __CTK_TRASH_MONITOR_H__
#define __CTK_TRASH_MONITOR_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_TRASH_MONITOR			(_ctk_trash_monitor_get_type ())
#define CTK_TRASH_MONITOR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TRASH_MONITOR, GtkTrashMonitor))
#define CTK_TRASH_MONITOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TRASH_MONITOR, GtkTrashMonitorClass))
#define CTK_IS_TRASH_MONITOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TRASH_MONITOR))
#define CTK_IS_TRASH_MONITOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TRASH_MONITOR))
#define CTK_TRASH_MONITOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TRASH_MONITOR, GtkTrashMonitorClass))

typedef struct _GtkTrashMonitor GtkTrashMonitor;
typedef struct _GtkTrashMonitorClass GtkTrashMonitorClass;

GType _ctk_trash_monitor_get_type (void);
GtkTrashMonitor *_ctk_trash_monitor_get (void);

GIcon *_ctk_trash_monitor_get_icon (GtkTrashMonitor *monitor);

gboolean _ctk_trash_monitor_get_has_trash (GtkTrashMonitor *monitor);

G_END_DECLS

#endif /* __CTK_TRASH_MONITOR_H__ */
