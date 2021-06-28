#ifndef __GDK__PRIVATE_H__
#define __GDK__PRIVATE_H__

#include <cdk/cdk.h>
#include "cdk/cdkinternals.h"

#define GDK_PRIVATE_CALL(symbol)        (cdk__private__ ()->symbol)

GdkDisplay *    cdk_display_open_default        (void);

gboolean        cdk_device_grab_info            (GdkDisplay  *display,
                                                 GdkDevice   *device,
                                                 GdkWindow  **grab_window,
                                                 gboolean    *owner_events);

void            cdk_add_option_entries          (GOptionGroup *group);

void            cdk_pre_parse                   (void);

GdkGLFlags      cdk_gl_get_flags                (void);
void            cdk_gl_set_flags                (GdkGLFlags flags);

void            cdk_window_freeze_toplevel_updates      (GdkWindow *window);
void            cdk_window_thaw_toplevel_updates        (GdkWindow *window);

GdkRenderingMode cdk_display_get_rendering_mode (GdkDisplay       *display);
void             cdk_display_set_rendering_mode (GdkDisplay       *display,
                                                 GdkRenderingMode  mode);

gboolean         cdk_display_get_debug_updates (GdkDisplay *display);
void             cdk_display_set_debug_updates (GdkDisplay *display,
                                                gboolean    debug_updates);

const gchar *   cdk_get_desktop_startup_id   (void);
const gchar *   cdk_get_desktop_autostart_id (void);

typedef struct {
  /* add all private functions here, initialize them in cdk-private.c */
  gboolean (* cdk_device_grab_info) (GdkDisplay  *display,
                                     GdkDevice   *device,
                                     GdkWindow  **grab_window,
                                     gboolean    *owner_events);

  GdkDisplay *(* cdk_display_open_default) (void);

  void (* cdk_add_option_entries) (GOptionGroup *group);
  void (* cdk_pre_parse) (void);

  GdkGLFlags (* cdk_gl_get_flags) (void);
  void       (* cdk_gl_set_flags) (GdkGLFlags flags);

  void (* cdk_window_freeze_toplevel_updates) (GdkWindow *window);
  void (* cdk_window_thaw_toplevel_updates) (GdkWindow *window);

  GdkRenderingMode (* cdk_display_get_rendering_mode) (GdkDisplay       *display);
  void             (* cdk_display_set_rendering_mode) (GdkDisplay       *display,
                                                       GdkRenderingMode  mode);

  gboolean         (* cdk_display_get_debug_updates) (GdkDisplay *display);
  void             (* cdk_display_set_debug_updates) (GdkDisplay *display,
                                                      gboolean    debug_updates);

  const gchar * (* cdk_get_desktop_startup_id)   (void);
  const gchar * (* cdk_get_desktop_autostart_id) (void);

  gboolean (* cdk_profiler_is_running) (void);
  void     (* cdk_profiler_start)      (int fd);
  void     (* cdk_profiler_stop)       (void);
} GdkPrivateVTable;

GDK_AVAILABLE_IN_ALL
GdkPrivateVTable *      cdk__private__  (void);

gboolean cdk_running_in_sandbox (void);
gboolean cdk_should_use_portal (void);

#endif /* __GDK__PRIVATE_H__ */
