/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __GDK_DEVICE_PRIVATE_H__
#define __GDK_DEVICE_PRIVATE_H__

#include "cdkdevice.h"
#include "cdkdevicetool.h"
#include "cdkdevicemanager.h"
#include "cdkevents.h"
#include "cdkseat.h"

G_BEGIN_DECLS

#define GDK_DEVICE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_DEVICE, CdkDeviceClass))
#define GDK_IS_DEVICE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_DEVICE))
#define GDK_DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_DEVICE, CdkDeviceClass))

typedef struct _CdkDeviceClass CdkDeviceClass;
typedef struct _CdkDeviceKey CdkDeviceKey;

struct _CdkDeviceKey
{
  guint keyval;
  CdkModifierType modifiers;
};

struct _CdkDevice
{
  GObject parent_instance;

  gchar *name;
  CdkInputSource source;
  CdkInputMode mode;
  gboolean has_cursor;
  gint num_keys;
  CdkAxisFlags axis_flags;
  CdkDeviceKey *keys;
  CdkDeviceManager *manager;
  CdkDisplay *display;
  /* Paired master for master,
   * associated master for slaves
   */
  CdkDevice *associated;
  GList *slaves;
  CdkDeviceType type;
  GArray *axes;
  guint num_touches;

  gchar *vendor_id;
  gchar *product_id;

  CdkSeat *seat;
  CdkDeviceTool *last_tool;
};

struct _CdkDeviceClass
{
  GObjectClass parent_class;

  gboolean (* get_history)   (CdkDevice      *device,
                              CdkWindow      *window,
                              guint32         start,
                              guint32         stop,
                              CdkTimeCoord ***events,
                              gint           *n_events);

  void (* get_state)         (CdkDevice       *device,
                              CdkWindow       *window,
                              gdouble         *axes,
                              CdkModifierType *mask);

  void (* set_window_cursor) (CdkDevice *device,
                              CdkWindow *window,
                              CdkCursor *cursor);

  void (* warp)              (CdkDevice  *device,
                              CdkScreen  *screen,
                              gdouble     x,
                              gdouble     y);
  void (* query_state)       (CdkDevice       *device,
                              CdkWindow       *window,
                              CdkWindow      **root_window,
                              CdkWindow      **child_window,
                              gdouble          *root_x,
                              gdouble          *root_y,
                              gdouble          *win_x,
                              gdouble          *win_y,
                              CdkModifierType  *mask);
  CdkGrabStatus (* grab)     (CdkDevice        *device,
                              CdkWindow        *window,
                              gboolean          owner_events,
                              CdkEventMask      event_mask,
                              CdkWindow        *confine_to,
                              CdkCursor        *cursor,
                              guint32           time_);
  void          (*ungrab)    (CdkDevice        *device,
                              guint32           time_);

  CdkWindow * (* window_at_position) (CdkDevice       *device,
                                      double          *win_x,
                                      double          *win_y,
                                      CdkModifierType *mask,
                                      gboolean         get_toplevel);
  void (* select_window_events)      (CdkDevice       *device,
                                      CdkWindow       *window,
                                      CdkEventMask     event_mask);
};

void  _cdk_device_set_associated_device (CdkDevice *device,
                                         CdkDevice *relative);

void  _cdk_device_reset_axes (CdkDevice   *device);
guint _cdk_device_add_axis   (CdkDevice   *device,
                              CdkAtom      label_atom,
                              CdkAxisUse   use,
                              gdouble      min_value,
                              gdouble      max_value,
                              gdouble      resolution);
void _cdk_device_get_axis_info (CdkDevice  *device,
				guint       index,
				CdkAtom    *label_atom,
				CdkAxisUse *use,
				gdouble    *min_value,
				gdouble    *max_value,
				gdouble    *resolution);

void _cdk_device_set_keys    (CdkDevice   *device,
                              guint        num_keys);

gboolean   _cdk_device_translate_window_coord (CdkDevice *device,
                                               CdkWindow *window,
                                               guint      index,
                                               gdouble    value,
                                               gdouble   *axis_value);

gboolean   _cdk_device_translate_screen_coord (CdkDevice *device,
                                               CdkWindow *window,
                                               gdouble    window_root_x,
                                               gdouble    window_root_y,
                                               guint      index,
                                               gdouble    value,
                                               gdouble   *axis_value);

gboolean   _cdk_device_translate_axis         (CdkDevice *device,
                                               guint      index,
                                               gdouble    value,
                                               gdouble   *axis_value);

CdkTimeCoord ** _cdk_device_allocate_history  (CdkDevice *device,
                                               gint       n_events);

void _cdk_device_add_slave (CdkDevice *device,
                            CdkDevice *slave);
void _cdk_device_remove_slave (CdkDevice *device,
                               CdkDevice *slave);
void _cdk_device_query_state                  (CdkDevice        *device,
                                               CdkWindow        *window,
                                               CdkWindow       **root_window,
                                               CdkWindow       **child_window,
                                               gdouble          *root_x,
                                               gdouble          *root_y,
                                               gdouble          *win_x,
                                               gdouble          *win_y,
                                               CdkModifierType  *mask);
CdkWindow * _cdk_device_window_at_position    (CdkDevice        *device,
                                               gdouble          *win_x,
                                               gdouble          *win_y,
                                               CdkModifierType  *mask,
                                               gboolean          get_toplevel);

void  cdk_device_set_seat  (CdkDevice *device,
                            CdkSeat   *seat);

void           cdk_device_update_tool (CdkDevice     *device,
                                       CdkDeviceTool *tool);

CdkInputMode cdk_device_get_input_mode (CdkDevice *device);

G_END_DECLS

#endif /* __GDK_DEVICE_PRIVATE_H__ */
