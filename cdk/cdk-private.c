#include "config.h"
#include "cdk-private.h"
#include "cdkprofilerprivate.h"

CdkPrivateVTable *
cdk__private__ (void)
{
  static CdkPrivateVTable table = {
    cdk_device_grab_info,
    cdk_display_open_default,
    cdk_add_option_entries,
    cdk_pre_parse,
    cdk_gl_get_flags,
    cdk_gl_set_flags,
    cdk_window_freeze_toplevel_updates,
    cdk_window_thaw_toplevel_updates,
    cdk_display_get_rendering_mode,
    cdk_display_set_rendering_mode,
    cdk_display_get_debug_updates,
    cdk_display_set_debug_updates,
    cdk_get_desktop_startup_id,
    cdk_get_desktop_autostart_id,
    cdk_profiler_is_running,
    cdk_profiler_start,
    cdk_profiler_stop
  };

  return &table;
}
