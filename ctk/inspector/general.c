/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "config.h"
#include <glib/gi18n-lib.h>

#include "general.h"

#include "ctkdebug.h"
#include "ctklabel.h"
#include "ctkscale.h"
#include "ctkswitch.h"
#include "ctklistbox.h"
#include "ctkprivate.h"
#include "ctksizegroup.h"
#include "ctkimage.h"
#include "ctkadjustment.h"
#include "ctkbox.h"

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#include <epoxy/glx.h>
#endif

#ifdef CDK_WINDOWING_WIN32
#include "win32/cdkwin32.h"
#endif

#ifdef CDK_WINDOWING_QUARTZ
#include "quartz/cdkquartz.h"
#endif

#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkwayland.h"
#include <epoxy/egl.h>
#endif

#ifdef CDK_WINDOWING_BROADWAY
#include "broadway/cdkbroadway.h"
#endif

struct _CtkInspectorGeneralPrivate
{
  CtkWidget *version_box;
  CtkWidget *env_box;
  CtkWidget *display_box;
  CtkWidget *gl_box;
  CtkWidget *device_box;
  CtkWidget *ctk_version;
  CtkWidget *cdk_backend;
  CtkWidget *gl_version;
  CtkWidget *gl_vendor;
  CtkWidget *prefix;
  CtkWidget *xdg_data_home;
  CtkWidget *xdg_data_dirs;
  CtkWidget *ctk_path;
  CtkWidget *ctk_exe_prefix;
  CtkWidget *ctk_data_prefix;
  CtkWidget *gsettings_schema_dir;
  CtkWidget *display_name;
  CtkWidget *display_rgba;
  CtkWidget *display_composited;
  CtkSizeGroup *labels;
  CtkAdjustment *focus_adjustment;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorGeneral, ctk_inspector_general, CTK_TYPE_SCROLLED_WINDOW)

static void
init_version (CtkInspectorGeneral *gen)
{
  const gchar *backend;
  CdkDisplay *display;

  display = cdk_display_get_default ();

#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (display))
    backend = "X11";
  else
#endif
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (display))
    backend = "Wayland";
  else
#endif
#ifdef CDK_WINDOWING_BROADWAY
  if (CDK_IS_BROADWAY_DISPLAY (display))
    backend = "Broadway";
  else
#endif
#ifdef CDK_WINDOWING_WIN32
  if (CDK_IS_WIN32_DISPLAY (display))
    backend = "Windows";
  else
#endif
#ifdef CDK_WINDOWING_QUARTZ
  if (CDK_IS_QUARTZ_DISPLAY (display))
    backend = "Quartz";
  else
#endif
    backend = "Unknown";

  ctk_label_set_text (CTK_LABEL (gen->priv->ctk_version), CTK_VERSION);
  ctk_label_set_text (CTK_LABEL (gen->priv->cdk_backend), backend);
}

static G_GNUC_UNUSED void
add_check_row (CtkInspectorGeneral *gen,
               CtkListBox          *list,
               const gchar         *name,
               gboolean             value,
               gint                 indent)
{
  CtkWidget *row, *box, *label, *check;

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 40);
  g_object_set (box,
                "margin", 10,
                "margin-start", 10 + indent,
                NULL);

  label = ctk_label_new (name);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 0);

  check = ctk_image_new_from_icon_name ("object-select-symbolic", CTK_ICON_SIZE_MENU);
  ctk_widget_set_halign (check, CTK_ALIGN_END);
  ctk_widget_set_valign (check, CTK_ALIGN_BASELINE);
  ctk_widget_set_opacity (check, value ? 1.0 : 0.0);
  ctk_box_pack_start (CTK_BOX (box), check, TRUE, TRUE, 0);

  row = ctk_list_box_row_new ();
  ctk_container_add (CTK_CONTAINER (row), box);
  ctk_list_box_row_set_activatable (CTK_LIST_BOX_ROW (row), FALSE);
  ctk_widget_show_all (row);

  ctk_list_box_insert (list, row, -1);

  ctk_size_group_add_widget (CTK_SIZE_GROUP (gen->priv->labels), label);
}

static void
add_label_row (CtkInspectorGeneral *gen,
               CtkListBox          *list,
               const char          *name,
               const char          *value,
               gint                 indent)
{
  CtkWidget *box;
  CtkWidget *label;
  CtkWidget *row;

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 40);
  g_object_set (box,
                "margin", 10,
                "margin-start", 10 + indent,
                NULL);

  label = ctk_label_new (name);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 0);

  label = ctk_label_new (value);
  ctk_label_set_selectable (CTK_LABEL (label), TRUE);
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_label_set_xalign (CTK_LABEL (label), 1.0);
  ctk_box_pack_start (CTK_BOX (box), label, TRUE, TRUE, 0);

  row = ctk_list_box_row_new ();
  ctk_container_add (CTK_CONTAINER (row), box);
  ctk_list_box_row_set_activatable (CTK_LIST_BOX_ROW (row), FALSE);
  ctk_widget_show_all (row);

  ctk_list_box_insert (CTK_LIST_BOX (list), row, -1);

  ctk_size_group_add_widget (CTK_SIZE_GROUP (gen->priv->labels), label);
}

#ifdef CDK_WINDOWING_X11
static void
append_glx_extension_row (CtkInspectorGeneral *gen,
                          Display             *dpy,
                          const gchar         *ext)
{
  add_check_row (gen, CTK_LIST_BOX (gen->priv->gl_box), ext, epoxy_has_glx_extension (dpy, 0, ext), 0);
}
#endif

#ifdef CDK_WINDOWING_WAYLAND
static void
append_egl_extension_row (CtkInspectorGeneral *gen,
                          EGLDisplay          dpy,
                          const gchar         *ext)
{
  add_check_row (gen, CTK_LIST_BOX (gen->priv->gl_box), ext, epoxy_has_egl_extension (dpy, ext), 0);
}

static EGLDisplay
wayland_get_display (struct wl_display *wl_display)
{
  EGLDisplay dpy = NULL;

  if (epoxy_has_egl_extension (NULL, "EGL_KHR_platform_base"))
    {
      PFNEGLGETPLATFORMDISPLAYPROC getPlatformDisplay =
        (void *) eglGetProcAddress ("eglGetPlatformDisplay");

      if (getPlatformDisplay)
        dpy = getPlatformDisplay (EGL_PLATFORM_WAYLAND_EXT,
                                  wl_display,
                                  NULL);
      if (dpy)
        return dpy;
    }

  if (epoxy_has_egl_extension (NULL, "EGL_EXT_platform_base"))
    {
      PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
        (void *) eglGetProcAddress ("eglGetPlatformDisplayEXT");

      if (getPlatformDisplay)
        dpy = getPlatformDisplay (EGL_PLATFORM_WAYLAND_EXT,
                                  wl_display,
                                  NULL);
      if (dpy)
        return dpy;
    }

  return eglGetDisplay ((EGLNativeDisplayType)wl_display);
}
#endif


static void
init_gl (CtkInspectorGeneral *gen)
{
#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (cdk_display_get_default ()))
    {
      CdkDisplay *display = cdk_display_get_default ();
      Display *dpy = CDK_DISPLAY_XDISPLAY (display);
      int error_base, event_base;
      gchar *version;
      if (!glXQueryExtension (dpy, &error_base, &event_base))
        return;

      version = g_strconcat ("GLX ", glXGetClientString (dpy, GLX_VERSION), NULL);
      ctk_label_set_text (CTK_LABEL (gen->priv->gl_version), version);
      g_free (version);
      ctk_label_set_text (CTK_LABEL (gen->priv->gl_vendor), glXGetClientString (dpy, GLX_VENDOR));

      append_glx_extension_row (gen, dpy, "GLX_ARB_create_context_profile");
      append_glx_extension_row (gen, dpy, "GLX_SGI_swap_control");
      append_glx_extension_row (gen, dpy, "GLX_EXT_texture_from_pixmap");
      append_glx_extension_row (gen, dpy, "GLX_SGI_video_sync");
      append_glx_extension_row (gen, dpy, "GLX_EXT_buffer_age");
      append_glx_extension_row (gen, dpy, "GLX_OML_sync_control");
      append_glx_extension_row (gen, dpy, "GLX_ARB_multisample");
      append_glx_extension_row (gen, dpy, "GLX_EXT_visual_rating");
    }
  else
#endif
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (cdk_display_get_default ()))
    {
      CdkDisplay *display = cdk_display_get_default ();
      EGLDisplay dpy;
      EGLint major, minor;
      gchar *version;

      dpy = wayland_get_display (cdk_wayland_display_get_wl_display (display));

      if (!eglInitialize (dpy, &major, &minor))
        return;

      version = g_strconcat ("EGL ", eglQueryString (dpy, EGL_VERSION), NULL);
      ctk_label_set_text (CTK_LABEL (gen->priv->gl_version), version);
      g_free (version);
      ctk_label_set_text (CTK_LABEL (gen->priv->gl_vendor), eglQueryString (dpy, EGL_VENDOR));

      append_egl_extension_row (gen, dpy, "EGL_KHR_create_context");
      append_egl_extension_row (gen, dpy, "EGL_EXT_buffer_age");
      append_egl_extension_row (gen, dpy, "EGL_EXT_swap_buffers_with_damage");
      append_egl_extension_row (gen, dpy, "EGL_KHR_surfaceless_context");
    }
  else
#endif
    {
      ctk_label_set_text (CTK_LABEL (gen->priv->gl_version), C_("GL version", "None"));
      ctk_label_set_text (CTK_LABEL (gen->priv->gl_vendor), C_("GL vendor", "None"));
    }
}

static void
set_monospace_font (CtkWidget *w)
{
  PangoAttrList *attrs;

  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_fallback_new (FALSE));
  pango_attr_list_insert (attrs, pango_attr_family_new ("Monospace"));
  ctk_label_set_attributes (CTK_LABEL (w), attrs);
  pango_attr_list_unref (attrs);
}

static void
set_path_label (CtkWidget   *w,
                const gchar *var)
{
  const gchar *v;

  v = g_getenv (var);
  if (v != NULL)
    {
      set_monospace_font (w);
      ctk_label_set_text (CTK_LABEL (w), v);
    }
  else
    {
       CtkWidget *r;
       r = ctk_widget_get_ancestor (w, CTK_TYPE_LIST_BOX_ROW);
       ctk_widget_hide (r);
    }
}

static void
init_env (CtkInspectorGeneral *gen)
{
  set_monospace_font (gen->priv->prefix);
  ctk_label_set_text (CTK_LABEL (gen->priv->prefix), _ctk_get_data_prefix ());
  set_path_label (gen->priv->xdg_data_home, "XDG_DATA_HOME");
  set_path_label (gen->priv->xdg_data_dirs, "XDG_DATA_DIRS");
  set_path_label (gen->priv->ctk_path, "CTK_PATH");
  set_path_label (gen->priv->ctk_exe_prefix, "CTK_EXE_PREFIX");
  set_path_label (gen->priv->ctk_data_prefix, "CTK_DATA_PREFIX");
  set_path_label (gen->priv->gsettings_schema_dir, "GSETTINGS_SCHEMA_DIR");
}

static const char *
translate_subpixel_layout (CdkSubpixelLayout subpixel)
{
  switch (subpixel)
    {
    case CDK_SUBPIXEL_LAYOUT_NONE: return "none";
    case CDK_SUBPIXEL_LAYOUT_UNKNOWN: return "unknown";
    case CDK_SUBPIXEL_LAYOUT_HORIZONTAL_RGB: return "horizontal rgb";
    case CDK_SUBPIXEL_LAYOUT_HORIZONTAL_BGR: return "horizontal bgr";
    case CDK_SUBPIXEL_LAYOUT_VERTICAL_RGB: return "vertical rgb";
    case CDK_SUBPIXEL_LAYOUT_VERTICAL_BGR: return "vertical bgr";
    default: g_assert_not_reached ();
    }
}

static void
populate_display (CdkScreen *screen, CtkInspectorGeneral *gen)
{
  gchar *name;
  gint i;
  GList *children, *l;
  CtkWidget *child;
  CdkDisplay *display;
  int n_monitors;
  CtkListBox *list;

  list = CTK_LIST_BOX (gen->priv->display_box);
  children = ctk_container_get_children (CTK_CONTAINER (list));
  for (l = children; l; l = l->next)
    {
      child = l->data;
      if (ctk_widget_is_ancestor (gen->priv->display_name, child) ||
          ctk_widget_is_ancestor (gen->priv->display_rgba, child) ||
          ctk_widget_is_ancestor (gen->priv->display_composited, child))
        continue;

      ctk_widget_destroy (child);
    }
  g_list_free (children);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  name = cdk_screen_make_display_name (screen);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_label_set_label (CTK_LABEL (gen->priv->display_name), name);
  g_free (name);

  if (cdk_screen_get_rgba_visual (screen) != NULL)
    ctk_widget_show (gen->priv->display_rgba);

  if (cdk_screen_is_composited (screen))
    ctk_widget_show (gen->priv->display_composited);

  display = cdk_screen_get_display (screen);
  n_monitors = cdk_display_get_n_monitors (display);
  for (i = 0; i < n_monitors; i++)
    {
      gchar *name;
      gchar *value;
      CdkRectangle rect;
      gint scale;
      const char *manufacturer;
      const char *model;
      CdkMonitor *monitor;

      monitor = cdk_display_get_monitor (display, i);

      name = g_strdup_printf ("Monitor %d", i);
      manufacturer = cdk_monitor_get_manufacturer (monitor);
      model = cdk_monitor_get_model (monitor);
      value = g_strdup_printf ("%s%s%s",
                               manufacturer ? manufacturer : "",
                               manufacturer || model ? " " : "",
                               model ? model : "");
      add_label_row (gen, list, name, value, 0);
      g_free (name);
      g_free (value);

      cdk_monitor_get_geometry (monitor, &rect);
      scale = cdk_monitor_get_scale_factor (monitor);

      value = g_strdup_printf ("%d × %d%s at %d, %d",
                               rect.width, rect.height,
                               scale == 2 ? " @ 2" : "",
                               rect.x, rect.y);
      add_label_row (gen, list, "Geometry", value, 10);
      g_free (value);

      value = g_strdup_printf ("%d × %d mm²",
                               cdk_monitor_get_width_mm (monitor),
                               cdk_monitor_get_height_mm (monitor));
      add_label_row (gen, list, "Size", value, 10);
      g_free (value);

      add_check_row (gen, list, "Primary", cdk_monitor_is_primary (monitor), 10);

      if (cdk_monitor_get_refresh_rate (monitor) != 0)
        value = g_strdup_printf ("%.2f Hz",
                                 0.001 * cdk_monitor_get_refresh_rate (monitor));
      else
        value = g_strdup ("unknown");
      add_label_row (gen, list, "Refresh rate", value, 10);
      g_free (value);

      value = g_strdup (translate_subpixel_layout (cdk_monitor_get_subpixel_layout (monitor)));
      add_label_row (gen, list, "Subpixel layout", value, 10);
      g_free (value);
    }
}

static void
init_display (CtkInspectorGeneral *gen)
{
  CdkScreen *screen;

  screen = cdk_screen_get_default ();

  g_signal_connect (screen, "size-changed", G_CALLBACK (populate_display), gen);
  g_signal_connect (screen, "composited-changed", G_CALLBACK (populate_display), gen);
  g_signal_connect (screen, "monitors-changed", G_CALLBACK (populate_display), gen);

  populate_display (screen, gen);
}

static void populate_seats (CtkInspectorGeneral *gen);

static void
add_device (CtkInspectorGeneral *gen,
            CdkDevice           *device)
{
  const gchar *name, *value;
  GString *str;
  int i;
  guint n_touches;
  gchar *text;
  CdkAxisFlags axes;
  const char *axis_name[] = {
    "Ignore",
    "X",
    "Y",
    "Pressure",
    "X Tilt",
    "Y Tilt",
    "Wheel",
    "Distance",
    "Rotation",
    "Slider"
  };
  const char *source_name[] = {
    "Mouse",
    "Pen",
    "Eraser",
    "Cursor",
    "Keyboard",
    "Touchscreen",
    "Touchpad",
    "Trackpoint",
    "Pad"
  };

  name = cdk_device_get_name (device);
  value = source_name[cdk_device_get_source (device)];
  add_label_row (gen, CTK_LIST_BOX (gen->priv->device_box), name, value, 10);

  str = g_string_new ("");

  axes = cdk_device_get_axes (device);
  for (i = CDK_AXIS_X; i < CDK_AXIS_LAST; i++)
    {
      if ((axes & (1 << i)) != 0)
        {
          if (str->len > 0)
            g_string_append (str, ", ");
          g_string_append (str, axis_name[i]);
        }
    }

  if (str->len > 0)
    add_label_row (gen, CTK_LIST_BOX (gen->priv->device_box), "Axes", str->str, 20);

  g_string_free (str, TRUE);

  g_object_get (device, "num-touches", &n_touches, NULL);
  if (n_touches > 0)
    {
      text = g_strdup_printf ("%d", n_touches);
      add_label_row (gen, CTK_LIST_BOX (gen->priv->device_box), "Touches", text, 20);
      g_free (text);
    }
}

static char *
get_seat_capabilities (CdkSeat *seat)
{
  struct {
    CdkSeatCapabilities cap;
    const char *name;
  } caps[] = {
    { CDK_SEAT_CAPABILITY_POINTER,       "Pointer" },
    { CDK_SEAT_CAPABILITY_TOUCH,         "Touch" },
    { CDK_SEAT_CAPABILITY_TABLET_STYLUS, "Tablet" },
    { CDK_SEAT_CAPABILITY_KEYBOARD,      "Keyboard" },
    { 0, NULL }
  };
  GString *str;
  CdkSeatCapabilities capabilities;
  int i;

  str = g_string_new ("");
  capabilities = cdk_seat_get_capabilities (seat);
  for (i = 0; caps[i].cap != 0; i++)
    {
      if (capabilities & caps[i].cap)
        {
          if (str->len > 0)
            g_string_append (str, ", ");
          g_string_append (str, caps[i].name);
        }
    }

  return g_string_free (str, FALSE);
}

static void
add_seat (CtkInspectorGeneral *gen,
          CdkSeat             *seat,
          int                  num)
{
  char *text;
  char *caps;
  GList *list, *l;

  if (!g_object_get_data (G_OBJECT (seat), "inspector-connected"))
    {
      g_object_set_data (G_OBJECT (seat), "inspector-connected", GINT_TO_POINTER (1));
      g_signal_connect_swapped (seat, "device-added", G_CALLBACK (populate_seats), gen);
      g_signal_connect_swapped (seat, "device-removed", G_CALLBACK (populate_seats), gen);
    }

  text = g_strdup_printf ("Seat %d", num);
  caps = get_seat_capabilities (seat);

  add_label_row (gen, CTK_LIST_BOX (gen->priv->device_box), text, caps, 0);
  g_free (text);
  g_free (caps);

  list = cdk_seat_get_slaves (seat, CDK_SEAT_CAPABILITY_ALL);

  for (l = list; l; l = l->next)
    add_device (gen, CDK_DEVICE (l->data));

  g_list_free (list);
}

static void
populate_seats (CtkInspectorGeneral *gen)
{
  CdkDisplay *display = cdk_display_get_default ();
  GList *list, *l;
  int i;

  list = ctk_container_get_children (CTK_CONTAINER (gen->priv->device_box));
  for (l = list; l; l = l->next)
    ctk_widget_destroy (CTK_WIDGET (l->data));
  g_list_free (list);

  list = cdk_display_list_seats (display);

  for (l = list, i = 0; l; l = l->next, i++)
    add_seat (gen, CDK_SEAT (l->data), i);

  g_list_free (list);
}

static void
init_device (CtkInspectorGeneral *gen)
{
  CdkDisplay *display = cdk_display_get_default ();

  g_signal_connect_swapped (display, "seat-added", G_CALLBACK (populate_seats), gen);
  g_signal_connect_swapped (display, "seat-removed", G_CALLBACK (populate_seats), gen);

  populate_seats (gen);
}

static void
ctk_inspector_general_init (CtkInspectorGeneral *gen)
{
  gen->priv = ctk_inspector_general_get_instance_private (gen);
  ctk_widget_init_template (CTK_WIDGET (gen));
  init_version (gen);
  init_env (gen);
  init_display (gen);
  init_gl (gen);
  init_device (gen);
}

static gboolean
keynav_failed (CtkWidget *widget, CtkDirectionType direction, CtkInspectorGeneral *gen)
{
  CtkWidget *next;
  gdouble value, lower, upper, page;

  if (direction == CTK_DIR_DOWN && widget == gen->priv->version_box)
    next = gen->priv->env_box;
  else if (direction == CTK_DIR_DOWN && widget == gen->priv->env_box)
    next = gen->priv->display_box;
  else if (direction == CTK_DIR_DOWN && widget == gen->priv->display_box)
    next = gen->priv->gl_box;
  else if (direction == CTK_DIR_DOWN && widget == gen->priv->gl_box)
    next = gen->priv->device_box;
  else if (direction == CTK_DIR_UP && widget == gen->priv->device_box)
    next = gen->priv->gl_box;
  else if (direction == CTK_DIR_UP && widget == gen->priv->gl_box)
    next = gen->priv->display_box;
  else if (direction == CTK_DIR_UP && widget == gen->priv->display_box)
    next = gen->priv->env_box;
  else if (direction == CTK_DIR_UP && widget == gen->priv->env_box)
    next = gen->priv->version_box;
  else
    next = NULL;

  if (next)
    {
      ctk_widget_child_focus (next, direction);
      return TRUE;
    }

  value = ctk_adjustment_get_value (gen->priv->focus_adjustment);
  lower = ctk_adjustment_get_lower (gen->priv->focus_adjustment);
  upper = ctk_adjustment_get_upper (gen->priv->focus_adjustment);
  page  = ctk_adjustment_get_page_size (gen->priv->focus_adjustment);

  if (direction == CTK_DIR_UP && value > lower)
    {
      ctk_adjustment_set_value (gen->priv->focus_adjustment, lower);
      return TRUE;
    }
  else if (direction == CTK_DIR_DOWN && value < upper - page)
    {
      ctk_adjustment_set_value (gen->priv->focus_adjustment, upper - page);
      return TRUE;
    }

  return FALSE;
}

static void
ctk_inspector_general_constructed (GObject *object)
{
  CtkInspectorGeneral *gen = CTK_INSPECTOR_GENERAL (object);

  G_OBJECT_CLASS (ctk_inspector_general_parent_class)->constructed (object);

  gen->priv->focus_adjustment = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (gen));
  ctk_container_set_focus_vadjustment (CTK_CONTAINER (ctk_bin_get_child (CTK_BIN (gen))),
                                       gen->priv->focus_adjustment);

   g_signal_connect (gen->priv->version_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->priv->env_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->priv->display_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->priv->gl_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->priv->device_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
}

static void
ctk_inspector_general_class_init (CtkInspectorGeneralClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->constructed = ctk_inspector_general_constructed;

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/general.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, version_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, env_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, display_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, gl_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, ctk_version);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, cdk_backend);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, gl_version);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, gl_vendor);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, prefix);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, xdg_data_home);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, xdg_data_dirs);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, ctk_path);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, ctk_exe_prefix);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, ctk_data_prefix);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, gsettings_schema_dir);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, labels);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, display_name);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, display_composited);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, display_rgba);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorGeneral, device_box);
}

// vim: set et sw=2 ts=2:
