/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __CDK_DISPLAY_PRIVATE_H__
#define __CDK_DISPLAY_PRIVATE_H__

#include "cdkdisplay.h"
#include "cdkwindow.h"
#include "cdkcursor.h"
#include "cdkmonitor.h"
#include "cdkinternals.h"

G_BEGIN_DECLS

#define CDK_DISPLAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_DISPLAY, CdkDisplayClass))
#define CDK_IS_DISPLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_DISPLAY))
#define CDK_DISPLAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_DISPLAY, CdkDisplayClass))


typedef struct _CdkDisplayClass CdkDisplayClass;

/* Tracks information about the device grab on this display */
typedef struct
{
  CdkWindow *window;
  CdkWindow *native_window;
  gulong serial_start;
  gulong serial_end; /* exclusive, i.e. not active on serial_end */
  guint event_mask;
  guint32 time;
  CdkGrabOwnership ownership;

  guint activated : 1;
  guint implicit_ungrab : 1;
  guint owner_events : 1;
  guint implicit : 1;
} CdkDeviceGrabInfo;

/* Tracks information about a touch implicit grab on this display */
typedef struct
{
  CdkDevice *device;
  CdkEventSequence *sequence;

  CdkWindow *window;
  CdkWindow *native_window;
  gulong serial;
  guint event_mask;
  guint32 time;
} CdkTouchGrabInfo;

/* Tracks information about which window and position the pointer last was in.
 * This is useful when we need to synthesize events later.
 * Note that we track toplevel_under_pointer using enter/leave events,
 * so in the case of a grab, either with owner_events==FALSE or with the
 * pointer in no clients window the x/y coordinates may actually be outside
 * the window.
 */
typedef struct
{
  CdkWindow *toplevel_under_pointer; /* toplevel window containing the pointer, */
                                     /* tracked via native events */
  CdkWindow *window_under_pointer;   /* window that last got a normal enter event */
  gdouble toplevel_x, toplevel_y;
  guint32 state;
  guint32 button;
  CdkDevice *last_slave;
  guint need_touch_press_enter : 1;
} CdkPointerWindowInfo;

typedef struct
{
  guint32 button_click_time[2]; /* last 2 button click times */
  CdkWindow *button_window[2];  /* last 2 windows to receive button presses */
  gint button_number[2];        /* last 2 buttons to be pressed */
  gint button_x[2];             /* last 2 button click positions */
  gint button_y[2];
  CdkDevice *last_slave;
} CdkMultipleClickInfo;

struct _CdkDisplay
{
  GObject parent_instance;

  GList *queued_events;
  GList *queued_tail;

  /* Information for determining if the latest button click
   * is part of a double-click or triple-click
   */
  GHashTable *multiple_click_info;

  guint event_pause_count;       /* How many times events are blocked */

  guint closed             : 1;  /* Whether this display has been closed */

  GArray *touch_implicit_grabs;
  GHashTable *device_grabs;
  GHashTable *motion_hint_info;
  CdkDeviceManager *device_manager;
  GList *input_devices; /* Deprecated, only used to keep cdk_display_list_devices working */

  GHashTable *pointers_info;  /* CdkPointerWindowInfo for each device */
  guint32 last_event_time;    /* Last reported event time from server */

  guint double_click_time;  /* Maximum time between clicks in msecs */
  guint double_click_distance;   /* Maximum distance between clicks in pixels */

  guint has_gl_extension_texture_non_power_of_two : 1;
  guint has_gl_extension_texture_rectangle : 1;

  guint debug_updates     : 1;
  guint debug_updates_set : 1;

  CdkRenderingMode rendering_mode;

  GList *seats;
};

struct _CdkDisplayClass
{
  GObjectClass parent_class;

  GType window_type;          /* type for native windows for this display, set in class_init */

  const gchar *              (*get_name)           (CdkDisplay *display);
  CdkScreen *                (*get_default_screen) (CdkDisplay *display);
  void                       (*beep)               (CdkDisplay *display);
  void                       (*sync)               (CdkDisplay *display);
  void                       (*flush)              (CdkDisplay *display);
  gboolean                   (*has_pending)        (CdkDisplay *display);
  void                       (*queue_events)       (CdkDisplay *display);
  void                       (*make_default)       (CdkDisplay *display);
  CdkWindow *                (*get_default_group)  (CdkDisplay *display);
  gboolean                   (*supports_selection_notification) (CdkDisplay *display);
  gboolean                   (*request_selection_notification)  (CdkDisplay *display,
                                                                 CdkAtom     selection);
  gboolean                   (*supports_shapes)       (CdkDisplay *display);
  gboolean                   (*supports_input_shapes) (CdkDisplay *display);
  gboolean                   (*supports_composite)    (CdkDisplay *display);
  gboolean                   (*supports_cursor_alpha) (CdkDisplay *display);
  gboolean                   (*supports_cursor_color) (CdkDisplay *display);

  gboolean                   (*supports_clipboard_persistence)  (CdkDisplay *display);
  void                       (*store_clipboard)    (CdkDisplay    *display,
                                                    CdkWindow     *clipboard_window,
                                                    guint32        time_,
                                                    const CdkAtom *targets,
                                                    gint           n_targets);

  void                       (*get_default_cursor_size) (CdkDisplay *display,
                                                         guint      *width,
                                                         guint      *height);
  void                       (*get_maximal_cursor_size) (CdkDisplay *display,
                                                         guint      *width,
                                                         guint      *height);
  CdkCursor *                (*get_cursor_for_type)     (CdkDisplay    *display,
                                                         CdkCursorType  type);
  CdkCursor *                (*get_cursor_for_name)     (CdkDisplay    *display,
                                                         const gchar   *name);
  CdkCursor *                (*get_cursor_for_surface)  (CdkDisplay    *display,
                                                         cairo_surface_t *surface,
                                                         gdouble          x,
                                                         gdouble          y);

  CdkAppLaunchContext *      (*get_app_launch_context) (CdkDisplay *display);

  void                       (*before_process_all_updates) (CdkDisplay *display);
  void                       (*after_process_all_updates)  (CdkDisplay *display);

  gulong                     (*get_next_serial) (CdkDisplay *display);

  void                       (*notify_startup_complete) (CdkDisplay  *display,
                                                         const gchar *startup_id);
  void                       (*event_data_copy) (CdkDisplay     *display,
                                                 const CdkEvent *event,
                                                 CdkEvent       *new_event);
  void                       (*event_data_free) (CdkDisplay     *display,
                                                 CdkEvent       *event);
  void                       (*create_window_impl) (CdkDisplay    *display,
                                                    CdkWindow     *window,
                                                    CdkWindow     *real_parent,
                                                    CdkScreen     *screen,
                                                    CdkEventMask   event_mask,
                                                    CdkWindowAttr *attributes,
                                                    gint           attributes_mask);

  CdkKeymap *                (*get_keymap)         (CdkDisplay    *display);
  void                       (*push_error_trap)    (CdkDisplay    *display);
  gint                       (*pop_error_trap)     (CdkDisplay    *display,
                                                    gboolean       ignore);

  CdkWindow *                (*get_selection_owner) (CdkDisplay   *display,
                                                     CdkAtom       selection);
  gboolean                   (*set_selection_owner) (CdkDisplay   *display,
                                                     CdkWindow    *owner,
                                                     CdkAtom       selection,
                                                     guint32       time,
                                                     gboolean      send_event);
  void                       (*send_selection_notify) (CdkDisplay *dispay,
                                                       CdkWindow        *requestor,
                                                       CdkAtom          selection,
                                                       CdkAtom          target,
                                                       CdkAtom          property,
                                                       guint32          time);
  gint                       (*get_selection_property) (CdkDisplay  *display,
                                                        CdkWindow   *requestor,
                                                        guchar     **data,
                                                        CdkAtom     *type,
                                                        gint        *format);
  void                       (*convert_selection)      (CdkDisplay  *display,
                                                        CdkWindow   *requestor,
                                                        CdkAtom      selection,
                                                        CdkAtom      target,
                                                        guint32      time);

  gint                   (*text_property_to_utf8_list) (CdkDisplay     *display,
                                                        CdkAtom         encoding,
                                                        gint            format,
                                                        const guchar   *text,
                                                        gint            length,
                                                        gchar        ***list);
  gchar *                (*utf8_to_string_target)      (CdkDisplay     *display,
                                                        const gchar    *text);

  gboolean               (*make_gl_context_current)    (CdkDisplay        *display,
                                                        CdkGLContext      *context);

  CdkSeat *              (*get_default_seat)           (CdkDisplay     *display);

  int                    (*get_n_monitors)             (CdkDisplay     *display);
  CdkMonitor *           (*get_monitor)                (CdkDisplay     *display,
                                                        int             index);
  CdkMonitor *           (*get_primary_monitor)        (CdkDisplay     *display);
  CdkMonitor *           (*get_monitor_at_window)      (CdkDisplay     *display,
                                                        CdkWindow      *window);

  /* Signals */
  void                   (*opened)                     (CdkDisplay     *display);
  void (*closed) (CdkDisplay *display,
                  gboolean    is_error);
};


typedef void (* CdkDisplayPointerInfoForeach) (CdkDisplay           *display,
                                               CdkDevice            *device,
                                               CdkPointerWindowInfo *device_info,
                                               gpointer              user_data);

void                _cdk_display_update_last_event    (CdkDisplay     *display,
                                                       const CdkEvent *event);
void                _cdk_display_device_grab_update   (CdkDisplay *display,
                                                       CdkDevice  *device,
                                                       CdkDevice  *source_device,
                                                       gulong      current_serial);
CdkDeviceGrabInfo * _cdk_display_get_last_device_grab (CdkDisplay *display,
                                                       CdkDevice  *device);
CdkDeviceGrabInfo * _cdk_display_add_device_grab      (CdkDisplay       *display,
                                                       CdkDevice        *device,
                                                       CdkWindow        *window,
                                                       CdkWindow        *native_window,
                                                       CdkGrabOwnership  grab_ownership,
                                                       gboolean          owner_events,
                                                       CdkEventMask      event_mask,
                                                       gulong            serial_start,
                                                       guint32           time,
                                                       gboolean          implicit);
CdkDeviceGrabInfo * _cdk_display_has_device_grab      (CdkDisplay       *display,
                                                       CdkDevice        *device,
                                                       gulong            serial);
gboolean            _cdk_display_end_device_grab      (CdkDisplay       *display,
                                                       CdkDevice        *device,
                                                       gulong            serial,
                                                       CdkWindow        *if_child,
                                                       gboolean          implicit);
gboolean            _cdk_display_check_grab_ownership (CdkDisplay       *display,
                                                       CdkDevice        *device,
                                                       gulong            serial);
void                _cdk_display_add_touch_grab       (CdkDisplay       *display,
                                                       CdkDevice        *device,
                                                       CdkEventSequence *sequence,
                                                       CdkWindow        *window,
                                                       CdkWindow        *native_window,
                                                       CdkEventMask      event_mask,
                                                       unsigned long     serial_start,
                                                       guint32           time);
CdkTouchGrabInfo *  _cdk_display_has_touch_grab       (CdkDisplay       *display,
                                                       CdkDevice        *device,
                                                       CdkEventSequence *sequence,
                                                       gulong            serial);
gboolean            _cdk_display_end_touch_grab       (CdkDisplay       *display,
                                                       CdkDevice        *device,
                                                       CdkEventSequence *sequence);
void                _cdk_display_enable_motion_hints  (CdkDisplay       *display,
                                                       CdkDevice        *device);
CdkPointerWindowInfo * _cdk_display_get_pointer_info  (CdkDisplay       *display,
                                                       CdkDevice        *device);
void                _cdk_display_pointer_info_foreach (CdkDisplay       *display,
                                                       CdkDisplayPointerInfoForeach func,
                                                       gpointer          user_data);
gulong              _cdk_display_get_next_serial      (CdkDisplay       *display);
void                _cdk_display_pause_events         (CdkDisplay       *display);
void                _cdk_display_unpause_events       (CdkDisplay       *display);
void                _cdk_display_event_data_copy      (CdkDisplay       *display,
                                                       const CdkEvent   *event,
                                                       CdkEvent         *new_event);
void                _cdk_display_event_data_free      (CdkDisplay       *display,
                                                       CdkEvent         *event);
void                _cdk_display_create_window_impl   (CdkDisplay       *display,
                                                       CdkWindow        *window,
                                                       CdkWindow        *real_parent,
                                                       CdkScreen        *screen,
                                                       CdkEventMask      event_mask,
                                                       CdkWindowAttr    *attributes,
                                                       gint              attributes_mask);
CdkWindow *         _cdk_display_create_window        (CdkDisplay       *display);

gboolean            cdk_display_make_gl_context_current  (CdkDisplay        *display,
                                                          CdkGLContext      *context);

void                cdk_display_add_seat              (CdkDisplay       *display,
                                                       CdkSeat          *seat);
void                cdk_display_remove_seat           (CdkDisplay       *display,
                                                       CdkSeat          *seat);
void                cdk_display_monitor_added         (CdkDisplay       *display,
                                                       CdkMonitor       *monitor);
void                cdk_display_monitor_removed       (CdkDisplay       *display,
                                                       CdkMonitor       *monitor);

G_END_DECLS

#endif  /* __CDK_DISPLAY_PRIVATE_H__ */
