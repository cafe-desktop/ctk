/* GDK - The GIMP Drawing Kit
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

#ifndef __GDK_SCREEN_PRIVATE_H__
#define __GDK_SCREEN_PRIVATE_H__

#include "cdkscreen.h"
#include "cdkvisual.h"

G_BEGIN_DECLS

#define GDK_SCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_SCREEN, CdkScreenClass))
#define GDK_IS_SCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_SCREEN))
#define GDK_SCREEN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_SCREEN, CdkScreenClass))

typedef struct _CdkScreenClass CdkScreenClass;

struct _CdkScreen
{
  GObject parent_instance;

  cairo_font_options_t *font_options;
  gdouble resolution; /* pixels/points scale factor for fonts */
  guint resolution_set : 1; /* resolution set through public API */
  guint closed : 1;
};

struct _CdkScreenClass
{
  GObjectClass parent_class;

  CdkDisplay * (* get_display)           (CdkScreen *screen);
  gint         (* get_width)             (CdkScreen *screen);
  gint         (* get_height)            (CdkScreen *screen);
  gint         (* get_width_mm)          (CdkScreen *screen);
  gint         (* get_height_mm)         (CdkScreen *screen);
  gint         (* get_number)            (CdkScreen *screen);
  CdkWindow *  (* get_root_window)       (CdkScreen *screen);
  gint         (* get_n_monitors)        (CdkScreen *screen);
  gint         (* get_primary_monitor)   (CdkScreen *screen);
  gint         (* get_monitor_width_mm)  (CdkScreen *screen,
                                          gint       monitor_num);
  gint         (* get_monitor_height_mm) (CdkScreen *screen,
                                          gint       monitor_num);
  gchar *      (* get_monitor_plug_name) (CdkScreen *screen,
                                          gint       monitor_num);
  void         (* get_monitor_geometry)  (CdkScreen    *screen,
                                          gint          monitor_num,
                                          CdkRectangle *dest);
  void         (* get_monitor_workarea)  (CdkScreen    *screen,
                                          gint          monitor_num,
                                          CdkRectangle *dest);
  GList *      (* list_visuals)          (CdkScreen *screen);
  CdkVisual *  (* get_system_visual)     (CdkScreen *screen);
  CdkVisual *  (* get_rgba_visual)       (CdkScreen *screen);
  gboolean     (* is_composited)         (CdkScreen *screen);
  gchar *      (* make_display_name)     (CdkScreen *screen);
  CdkWindow *  (* get_active_window)     (CdkScreen *screen);
  GList *      (* get_window_stack)      (CdkScreen *screen);
  void         (* broadcast_client_message) (CdkScreen *screen,
                                             CdkEvent  *event);
  gboolean     (* get_setting)           (CdkScreen   *screen,
                                          const gchar *name,
                                          GValue      *value);
  gint         (* visual_get_best_depth) (CdkScreen   *screen);
  CdkVisualType (* visual_get_best_type) (CdkScreen   *screen);
  CdkVisual *  (* visual_get_best)       (CdkScreen   *screen);
  CdkVisual *  (* visual_get_best_with_depth) (CdkScreen   *screen,
                                               gint depth);
  CdkVisual *  (* visual_get_best_with_type) (CdkScreen   *screen,
                                              CdkVisualType visual_type);
  CdkVisual *  (* visual_get_best_with_both) (CdkScreen   *screen,
                                              gint depth,
                                              CdkVisualType visual_type);
  void         (* query_depths)          (CdkScreen   *screen,
                                          gint **depths,
                                          gint  *count);
  void         (* query_visual_types)    (CdkScreen   *screen,
                                          CdkVisualType **visual_types,
                                          gint           *count);
  gint         (* get_monitor_scale_factor) (CdkScreen *screen,
                                             gint       monitor_num);

  /* Signals: */
  void (*size_changed) (CdkScreen *screen);
  void (*composited_changed) (CdkScreen *screen);
  void (*monitors_changed) (CdkScreen *screen);
};

G_END_DECLS

#endif /* __GDK_SCREEN_PRIVATE_H__ */
