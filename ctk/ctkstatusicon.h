/* ctkstatusicon.h:
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *      Mark McLoughlin <mark@skynet.ie>
 */

#ifndef __CTK_STATUS_ICON_H__
#define __CTK_STATUS_ICON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkimage.h>
#include <ctk/ctkmenu.h>

G_BEGIN_DECLS

#define CTK_TYPE_STATUS_ICON         (ctk_status_icon_get_type ())
#define CTK_STATUS_ICON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_STATUS_ICON, CtkStatusIcon))
#define CTK_STATUS_ICON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_STATUS_ICON, CtkStatusIconClass))
#define CTK_IS_STATUS_ICON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_STATUS_ICON))
#define CTK_IS_STATUS_ICON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_STATUS_ICON))
#define CTK_STATUS_ICON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_STATUS_ICON, CtkStatusIconClass))

typedef struct _CtkStatusIcon	     CtkStatusIcon;
typedef struct _CtkStatusIconClass   CtkStatusIconClass;
typedef struct _CtkStatusIconPrivate CtkStatusIconPrivate;

struct _CtkStatusIcon
{
  GObject               parent_instance;

  CtkStatusIconPrivate *priv;
};

struct _CtkStatusIconClass
{
  GObjectClass parent_class;

  void     (* activate)             (CtkStatusIcon  *status_icon);
  void     (* popup_menu)           (CtkStatusIcon  *status_icon,
                                     guint           button,
                                     guint32         activate_time);
  gboolean (* size_changed)         (CtkStatusIcon  *status_icon,
                                     gint            size);
  gboolean (* button_press_event)   (CtkStatusIcon  *status_icon,
                                     CdkEventButton *event);
  gboolean (* button_release_event) (CtkStatusIcon  *status_icon,
                                     CdkEventButton *event);
  gboolean (* scroll_event)         (CtkStatusIcon  *status_icon,
                                     CdkEventScroll *event);
  gboolean (* query_tooltip)        (CtkStatusIcon  *status_icon,
                                     gint            x,
                                     gint            y,
                                     gboolean        keyboard_mode,
                                     CtkTooltip     *tooltip);

  void (*__ctk_reserved1) (void);
  void (*__ctk_reserved2) (void);
  void (*__ctk_reserved3) (void);
  void (*__ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType                 ctk_status_icon_get_type           (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkStatusIcon        *ctk_status_icon_new                (void);
CDK_AVAILABLE_IN_ALL
CtkStatusIcon        *ctk_status_icon_new_from_pixbuf    (GdkPixbuf          *pixbuf);
CDK_AVAILABLE_IN_ALL
CtkStatusIcon        *ctk_status_icon_new_from_file      (const gchar        *filename);
CDK_AVAILABLE_IN_ALL
CtkStatusIcon        *ctk_status_icon_new_from_stock     (const gchar        *stock_id);
CDK_AVAILABLE_IN_ALL
CtkStatusIcon        *ctk_status_icon_new_from_icon_name (const gchar        *icon_name);
CDK_AVAILABLE_IN_ALL
CtkStatusIcon        *ctk_status_icon_new_from_gicon     (GIcon              *icon);

CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_from_pixbuf    (CtkStatusIcon      *status_icon,
							  GdkPixbuf          *pixbuf);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_from_file      (CtkStatusIcon      *status_icon,
							  const gchar        *filename);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_from_stock     (CtkStatusIcon      *status_icon,
							  const gchar        *stock_id);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_from_icon_name (CtkStatusIcon      *status_icon,
							  const gchar        *icon_name);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_from_gicon     (CtkStatusIcon      *status_icon,
                                                          GIcon              *icon);

CDK_AVAILABLE_IN_ALL
CtkImageType          ctk_status_icon_get_storage_type   (CtkStatusIcon      *status_icon);

CDK_AVAILABLE_IN_ALL
GdkPixbuf            *ctk_status_icon_get_pixbuf         (CtkStatusIcon      *status_icon);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_status_icon_get_stock          (CtkStatusIcon      *status_icon);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_status_icon_get_icon_name      (CtkStatusIcon      *status_icon);
CDK_AVAILABLE_IN_ALL
GIcon                *ctk_status_icon_get_gicon          (CtkStatusIcon      *status_icon);

CDK_AVAILABLE_IN_ALL
gint                  ctk_status_icon_get_size           (CtkStatusIcon      *status_icon);

CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_screen         (CtkStatusIcon      *status_icon,
                                                          CdkScreen          *screen);
CDK_AVAILABLE_IN_ALL
CdkScreen            *ctk_status_icon_get_screen         (CtkStatusIcon      *status_icon);

CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_has_tooltip    (CtkStatusIcon      *status_icon,
                                                          gboolean            has_tooltip);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_tooltip_text   (CtkStatusIcon      *status_icon,
                                                          const gchar        *text);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_tooltip_markup (CtkStatusIcon      *status_icon,
                                                          const gchar        *markup);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_title          (CtkStatusIcon      *status_icon,
                                                          const gchar        *title);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_status_icon_get_title          (CtkStatusIcon      *status_icon);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_name           (CtkStatusIcon      *status_icon,
                                                          const gchar        *name);
CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_set_visible        (CtkStatusIcon      *status_icon,
							  gboolean            visible);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_status_icon_get_visible        (CtkStatusIcon      *status_icon);

CDK_AVAILABLE_IN_ALL
gboolean              ctk_status_icon_is_embedded        (CtkStatusIcon      *status_icon);

CDK_AVAILABLE_IN_ALL
void                  ctk_status_icon_position_menu      (CtkMenu            *menu,
							  gint               *x,
							  gint               *y,
							  gboolean           *push_in,
							  gpointer            user_data);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_status_icon_get_geometry       (CtkStatusIcon      *status_icon,
							  CdkScreen         **screen,
							  CdkRectangle       *area,
							  CtkOrientation     *orientation);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_status_icon_get_has_tooltip    (CtkStatusIcon      *status_icon);
CDK_AVAILABLE_IN_ALL
gchar                *ctk_status_icon_get_tooltip_text   (CtkStatusIcon      *status_icon);
CDK_AVAILABLE_IN_ALL
gchar                *ctk_status_icon_get_tooltip_markup (CtkStatusIcon      *status_icon);

CDK_AVAILABLE_IN_ALL
guint32               ctk_status_icon_get_x11_window_id  (CtkStatusIcon      *status_icon);

G_END_DECLS

#endif /* __CTK_STATUS_ICON_H__ */
