/* ctktrayicon.h
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
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

#ifndef __CTK_TRAY_ICON_H__
#define __CTK_TRAY_ICON_H__

#include "ctkplug.h"

G_BEGIN_DECLS

#define CTK_TYPE_TRAY_ICON		(ctk_tray_icon_get_type ())
#define CTK_TRAY_ICON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TRAY_ICON, CtkTrayIcon))
#define CTK_TRAY_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TRAY_ICON, CtkTrayIconClass))
#define CTK_IS_TRAY_ICON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TRAY_ICON))
#define CTK_IS_TRAY_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TRAY_ICON))
#define CTK_TRAY_ICON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TRAY_ICON, CtkTrayIconClass))
	
typedef struct _CtkTrayIcon	   CtkTrayIcon;
typedef struct _CtkTrayIconPrivate CtkTrayIconPrivate;
typedef struct _CtkTrayIconClass   CtkTrayIconClass;

struct _CtkTrayIcon
{
  CtkPlug parent_instance;

  CtkTrayIconPrivate *priv;
};

struct _CtkTrayIconClass
{
  CtkPlugClass parent_class;

  /* Padding for future expansion */
  void (*__ctk_reserved1) (void);
  void (*__ctk_reserved2) (void);
  void (*__ctk_reserved3) (void);
  void (*__ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType          ctk_tray_icon_get_type         (void) G_GNUC_CONST;

CtkTrayIcon   *_ctk_tray_icon_new_for_screen  (CdkScreen   *screen,
					       const gchar *name);

CtkTrayIcon   *_ctk_tray_icon_new             (const gchar *name);

guint          _ctk_tray_icon_send_message    (CtkTrayIcon *icon,
					       gint         timeout,
					       const gchar *message,
					       gint         len);
void           _ctk_tray_icon_cancel_message  (CtkTrayIcon *icon,
					       guint        id);

CtkOrientation _ctk_tray_icon_get_orientation (CtkTrayIcon *icon);
gint           _ctk_tray_icon_get_padding     (CtkTrayIcon *icon);
gint           _ctk_tray_icon_get_icon_size   (CtkTrayIcon *icon);

G_END_DECLS

#endif /* __CTK_TRAY_ICON_H__ */
