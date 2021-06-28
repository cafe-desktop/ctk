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

#ifndef __GDK_DEVICE_H__
#define __GDK_DEVICE_H__

#if !defined (__GDK_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>


G_BEGIN_DECLS

#define GDK_TYPE_DEVICE         (cdk_device_get_type ())
#define GDK_DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_DEVICE, CdkDevice))
#define GDK_IS_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_DEVICE))

typedef struct _CdkTimeCoord CdkTimeCoord;

/**
 * CdkInputSource:
 * @GDK_SOURCE_MOUSE: the device is a mouse. (This will be reported for the core
 *                    pointer, even if it is something else, such as a trackball.)
 * @GDK_SOURCE_PEN: the device is a stylus of a graphics tablet or similar device.
 * @GDK_SOURCE_ERASER: the device is an eraser. Typically, this would be the other end
 *                     of a stylus on a graphics tablet.
 * @GDK_SOURCE_CURSOR: the device is a graphics tablet “puck” or similar device.
 * @GDK_SOURCE_KEYBOARD: the device is a keyboard.
 * @GDK_SOURCE_TOUCHSCREEN: the device is a direct-input touch device, such
 *     as a touchscreen or tablet. This device type has been added in 3.4.
 * @GDK_SOURCE_TOUCHPAD: the device is an indirect touch device, such
 *     as a touchpad. This device type has been added in 3.4.
 * @GDK_SOURCE_TRACKPOINT: the device is a trackpoint. This device type has been
 *     added in 3.22
 * @GDK_SOURCE_TABLET_PAD: the device is a "pad", a collection of buttons,
 *     rings and strips found in drawing tablets. This device type has been
 *     added in 3.22.
 *
 * An enumeration describing the type of an input device in general terms.
 */
typedef enum
{
  GDK_SOURCE_MOUSE,
  GDK_SOURCE_PEN,
  GDK_SOURCE_ERASER,
  GDK_SOURCE_CURSOR,
  GDK_SOURCE_KEYBOARD,
  GDK_SOURCE_TOUCHSCREEN,
  GDK_SOURCE_TOUCHPAD,
  GDK_SOURCE_TRACKPOINT,
  GDK_SOURCE_TABLET_PAD
} CdkInputSource;

/**
 * CdkInputMode:
 * @GDK_MODE_DISABLED: the device is disabled and will not report any events.
 * @GDK_MODE_SCREEN: the device is enabled. The device’s coordinate space
 *                   maps to the entire screen.
 * @GDK_MODE_WINDOW: the device is enabled. The device’s coordinate space
 *                   is mapped to a single window. The manner in which this window
 *                   is chosen is undefined, but it will typically be the same
 *                   way in which the focus window for key events is determined.
 *
 * An enumeration that describes the mode of an input device.
 */
typedef enum
{
  GDK_MODE_DISABLED,
  GDK_MODE_SCREEN,
  GDK_MODE_WINDOW
} CdkInputMode;

/**
 * CdkDeviceType:
 * @GDK_DEVICE_TYPE_MASTER: Device is a master (or virtual) device. There will
 *                          be an associated focus indicator on the screen.
 * @GDK_DEVICE_TYPE_SLAVE: Device is a slave (or physical) device.
 * @GDK_DEVICE_TYPE_FLOATING: Device is a physical device, currently not attached to
 *                            any virtual device.
 *
 * Indicates the device type. See [above][CdkDeviceManager.description]
 * for more information about the meaning of these device types.
 */
typedef enum {
  GDK_DEVICE_TYPE_MASTER,
  GDK_DEVICE_TYPE_SLAVE,
  GDK_DEVICE_TYPE_FLOATING
} CdkDeviceType;

/* We don't allocate each coordinate this big, but we use it to
 * be ANSI compliant and avoid accessing past the defined limits.
 */
#define GDK_MAX_TIMECOORD_AXES 128

/**
 * CdkTimeCoord:
 * @time: The timestamp for this event.
 * @axes: the values of the device’s axes.
 *
 * A #CdkTimeCoord stores a single event in a motion history.
 */
struct _CdkTimeCoord
{
  guint32 time;
  gdouble axes[GDK_MAX_TIMECOORD_AXES];
};

GDK_AVAILABLE_IN_ALL
GType                 cdk_device_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
const gchar *         cdk_device_get_name       (CdkDevice *device);
GDK_AVAILABLE_IN_ALL
gboolean              cdk_device_get_has_cursor (CdkDevice *device);

/* Functions to configure a device */
GDK_AVAILABLE_IN_ALL
CdkInputSource cdk_device_get_source    (CdkDevice      *device);

GDK_AVAILABLE_IN_ALL
CdkInputMode   cdk_device_get_mode      (CdkDevice      *device);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_device_set_mode      (CdkDevice      *device,
                                         CdkInputMode    mode);

GDK_AVAILABLE_IN_ALL
gint           cdk_device_get_n_keys    (CdkDevice       *device);
GDK_AVAILABLE_IN_ALL
gboolean       cdk_device_get_key       (CdkDevice       *device,
                                         guint            index_,
                                         guint           *keyval,
                                         CdkModifierType *modifiers);
GDK_AVAILABLE_IN_ALL
void           cdk_device_set_key       (CdkDevice      *device,
                                         guint           index_,
                                         guint           keyval,
                                         CdkModifierType modifiers);

GDK_AVAILABLE_IN_ALL
CdkAxisUse     cdk_device_get_axis_use  (CdkDevice         *device,
                                         guint              index_);
GDK_AVAILABLE_IN_ALL
void           cdk_device_set_axis_use  (CdkDevice         *device,
                                         guint              index_,
                                         CdkAxisUse         use);


GDK_AVAILABLE_IN_ALL
void     cdk_device_get_state    (CdkDevice         *device,
                                  CdkWindow         *window,
                                  gdouble           *axes,
                                  CdkModifierType   *mask);
GDK_AVAILABLE_IN_ALL
void     cdk_device_get_position (CdkDevice         *device,
                                  CdkScreen        **screen,
                                  gint              *x,
                                  gint              *y);
GDK_AVAILABLE_IN_ALL
CdkWindow *
         cdk_device_get_window_at_position
                                 (CdkDevice         *device,
                                  gint              *win_x,
                                  gint              *win_y);
GDK_AVAILABLE_IN_3_10
void     cdk_device_get_position_double (CdkDevice         *device,
                                         CdkScreen        **screen,
                                         gdouble           *x,
                                         gdouble           *y);
GDK_AVAILABLE_IN_3_10
CdkWindow *
         cdk_device_get_window_at_position_double
                                 (CdkDevice         *device,
                                  gdouble           *win_x,
                                  gdouble           *win_y);
GDK_AVAILABLE_IN_ALL
gboolean cdk_device_get_history  (CdkDevice         *device,
                                  CdkWindow         *window,
                                  guint32            start,
                                  guint32            stop,
                                  CdkTimeCoord    ***events,
                                  gint              *n_events);
GDK_AVAILABLE_IN_ALL
void     cdk_device_free_history (CdkTimeCoord     **events,
                                  gint               n_events);

GDK_AVAILABLE_IN_ALL
gint     cdk_device_get_n_axes     (CdkDevice       *device);
GDK_AVAILABLE_IN_ALL
GList *  cdk_device_list_axes      (CdkDevice       *device);
GDK_AVAILABLE_IN_ALL
gboolean cdk_device_get_axis_value (CdkDevice       *device,
                                    gdouble         *axes,
                                    CdkAtom          axis_label,
                                    gdouble         *value);

GDK_AVAILABLE_IN_ALL
gboolean cdk_device_get_axis     (CdkDevice         *device,
                                  gdouble           *axes,
                                  CdkAxisUse         use,
                                  gdouble           *value);
GDK_AVAILABLE_IN_ALL
CdkDisplay * cdk_device_get_display (CdkDevice      *device);

GDK_AVAILABLE_IN_ALL
CdkDevice  * cdk_device_get_associated_device (CdkDevice     *device);
GDK_AVAILABLE_IN_ALL
GList *      cdk_device_list_slave_devices    (CdkDevice     *device);

GDK_AVAILABLE_IN_ALL
CdkDeviceType cdk_device_get_device_type (CdkDevice *device);

GDK_DEPRECATED_IN_3_20_FOR(cdk_seat_grab)
CdkGrabStatus cdk_device_grab        (CdkDevice        *device,
                                      CdkWindow        *window,
                                      CdkGrabOwnership  grab_ownership,
                                      gboolean          owner_events,
                                      CdkEventMask      event_mask,
                                      CdkCursor        *cursor,
                                      guint32           time_);

GDK_DEPRECATED_IN_3_20_FOR(cdk_seat_ungrab)
void          cdk_device_ungrab      (CdkDevice        *device,
                                      guint32           time_);

GDK_AVAILABLE_IN_ALL
void          cdk_device_warp        (CdkDevice        *device,
                                      CdkScreen        *screen,
                                      gint              x,
                                      gint              y);

GDK_DEPRECATED_IN_3_16
gboolean cdk_device_grab_info_libctk_only (CdkDisplay  *display,
                                           CdkDevice   *device,
                                           CdkWindow  **grab_window,
                                           gboolean    *owner_events);

GDK_AVAILABLE_IN_3_12
CdkWindow *cdk_device_get_last_event_window (CdkDevice *device);

GDK_AVAILABLE_IN_3_16
const gchar *cdk_device_get_vendor_id       (CdkDevice *device);
GDK_AVAILABLE_IN_3_16
const gchar *cdk_device_get_product_id      (CdkDevice *device);

GDK_AVAILABLE_IN_3_20
CdkSeat     *cdk_device_get_seat            (CdkDevice *device);

GDK_AVAILABLE_IN_3_22
CdkAxisFlags cdk_device_get_axes            (CdkDevice *device);

G_END_DECLS

#endif /* __GDK_DEVICE_H__ */
