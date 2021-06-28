#ifndef __GDK__PRIVATE_H__
#define __GDK__PRIVATE_H__

#include <cdk/cdk.h>
#include "cdk/cdkinternals.h"

#define GDK_PRIVATE_CALL(symbol)        (cdk__private__ ()->symbol)

CdkDisplay *    cdk_display_open_default        (void);

gboolean        cdk_device_grab_info            (CdkDisplay  *display,
                                                 CdkDevice   *device,
                                                 CdkWindow  **grab_window,
                                                 gboolean    *owner_events);

void            cdk_add_option_entries          (GOptionGroup *group);

void            cdk_pre_parse                   (void);

CdkGLFlags      cdk_gl_get_flags                (void);
void            cdk_gl_set_flags                (CdkGLFlags flags);

void            cdk_window_freeze_toplevel_updates      (CdkWindow *window);
void            cdk_window_thaw_toplevel_updates        (CdkWindow *window);

CdkRenderingMode cdk_display_get_rendering_mode (CdkDisplay       *display);
void             cdk_display_set_rendering_mode (CdkDisplay       *display,
                                                 CdkRenderingMode  mode);

gboolean         cdk_display_get_debug_updates (CdkDisplay *display);
void             cdk_display_set_debug_updates (CdkDisplay *display,
                                                gboolean    debug_updates);

const gchar *   cdk_get_desktop_startup_id   (void);
const gchar *   cdk_get_desktop_autostart_id (void);

typedef struct {
  /* add all private functions here, initialize them in cdk-private.c */
  gboolean (* cdk_device_grab_info) (CdkDisplay  *display,
                                     CdkDevice   *device,
                                     CdkWindow  **grab_window,
                                     gboolean    *owner_events);

  CdkDisplay *(* cdk_display_open_default) (void);

  void (* cdk_add_option_entries) (GOptionGroup *group);
  void (* cdk_pre_parse) (void);

  CdkGLFlags (* cdk_gl_get_flags) (void);
  void       (* cdk_gl_set_flags) (CdkGLFlags flags);

  void (* cdk_window_freeze_toplevel_updates) (CdkWindow *window);
  void (* cdk_window_thaw_toplevel_updates) (CdkWindow *window);

  CdkRenderingMode (* cdk_display_get_rendering_mode) (CdkDisplay       *display);
  void             (* cdk_display_set_rendering_mode) (CdkDisplay       *display,
                                                       CdkRenderingMode  mode);

  gboolean         (* cdk_display_get_debug_updates) (CdkDisplay *display);
  void             (* cdk_display_set_debug_updates) (CdkDisplay *display,
                                                      gboolean    debug_updates);

  const gchar * (* cdk_get_desktop_startup_id)   (void);
  const gchar * (* cdk_get_desktop_autostart_id) (void);

  gboolean (* cdk_profiler_is_running) (void);
  void     (* cdk_profiler_start)      (int fd);
  void     (* cdk_profiler_stop)       (void);
} CdkPrivateVTable;

GDK_AVAILABLE_IN_ALL
CdkPrivateVTable *      cdk__private__  (void);

gboolean cdk_running_in_sandbox (void);
gboolean cdk_should_use_portal (void);

#endif /* __GDK__PRIVATE_H__ */
