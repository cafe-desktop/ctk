/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#ifndef __CDK_WINDOW_IMPL_H__
#define __CDK_WINDOW_IMPL_H__

#include <cdk/cdkwindow.h>
#include <cdk/cdkproperty.h>

G_BEGIN_DECLS

#define CDK_TYPE_WINDOW_IMPL           (cdk_window_impl_get_type ())
#define CDK_WINDOW_IMPL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WINDOW_IMPL, CdkWindowImpl))
#define CDK_WINDOW_IMPL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WINDOW_IMPL, CdkWindowImplClass))
#define CDK_IS_WINDOW_IMPL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WINDOW_IMPL))
#define CDK_IS_WINDOW_IMPL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WINDOW_IMPL))
#define CDK_WINDOW_IMPL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WINDOW_IMPL, CdkWindowImplClass))

typedef struct _CdkWindowImpl       CdkWindowImpl;
typedef struct _CdkWindowImplClass  CdkWindowImplClass;

struct _CdkWindowImpl
{
  GObject parent;
};

struct _CdkWindowImplClass
{
  GObjectClass parent_class;

  cairo_surface_t *
               (* ref_cairo_surface)    (CdkWindow       *window);
  cairo_surface_t *
               (* create_similar_image_surface) (CdkWindow *     window,
                                                 cairo_format_t  format,
                                                 int             width,
                                                 int             height);

  void         (* show)                 (CdkWindow       *window,
					 gboolean         already_mapped);
  void         (* hide)                 (CdkWindow       *window);
  void         (* withdraw)             (CdkWindow       *window);
  void         (* raise)                (CdkWindow       *window);
  void         (* lower)                (CdkWindow       *window);
  void         (* restack_under)        (CdkWindow       *window,
					 GList           *native_siblings);
  void         (* restack_toplevel)     (CdkWindow       *window,
					 CdkWindow       *sibling,
					 gboolean        above);

  void         (* move_resize)          (CdkWindow       *window,
                                         gboolean         with_move,
                                         gint             x,
                                         gint             y,
                                         gint             width,
                                         gint             height);
  void         (* move_to_rect)         (CdkWindow       *window,
                                         const CdkRectangle *rect,
                                         CdkGravity       rect_anchor,
                                         CdkGravity       window_anchor,
                                         CdkAnchorHints   anchor_hints,
                                         gint             rect_anchor_dx,
                                         gint             rect_anchor_dy);
  void         (* set_background)       (CdkWindow       *window,
                                         cairo_pattern_t *pattern);

  CdkEventMask (* get_events)           (CdkWindow       *window);
  void         (* set_events)           (CdkWindow       *window,
                                         CdkEventMask     event_mask);
  
  gboolean     (* reparent)             (CdkWindow       *window,
                                         CdkWindow       *new_parent,
                                         gint             x,
                                         gint             y);

  void         (* set_device_cursor)    (CdkWindow       *window,
                                         CdkDevice       *device,
                                         CdkCursor       *cursor);

  void         (* get_geometry)         (CdkWindow       *window,
                                         gint            *x,
                                         gint            *y,
                                         gint            *width,
                                         gint            *height);
  void         (* get_root_coords)      (CdkWindow       *window,
					 gint             x,
					 gint             y,
                                         gint            *root_x,
                                         gint            *root_y);
  gboolean     (* get_device_state)     (CdkWindow       *window,
                                         CdkDevice       *device,
                                         gdouble         *x,
                                         gdouble         *y,
                                         CdkModifierType *mask);
  gboolean    (* begin_paint)           (CdkWindow       *window);
  void        (* end_paint)             (CdkWindow       *window);

  cairo_region_t * (* get_shape)        (CdkWindow       *window);
  cairo_region_t * (* get_input_shape)  (CdkWindow       *window);
  void         (* shape_combine_region) (CdkWindow       *window,
                                         const cairo_region_t *shape_region,
                                         gint             offset_x,
                                         gint             offset_y);
  void         (* input_shape_combine_region) (CdkWindow       *window,
					       const cairo_region_t *shape_region,
					       gint             offset_x,
					       gint             offset_y);

  /* Called before processing updates for a window. This gives the windowing
   * layer a chance to save the region for later use in avoiding duplicate
   * exposes.
   */
  void     (* queue_antiexpose)     (CdkWindow       *window,
                                     cairo_region_t  *update_area);

/* Called to do the windowing system specific part of cdk_window_destroy(),
 *
 * window: The window being destroyed
 * recursing: If TRUE, then this is being called because a parent
 *     was destroyed. This generally means that the call to the windowing
 *     system to destroy the window can be omitted, since it will be
 *     destroyed as a result of the parent being destroyed.
 *     Unless @foreign_destroy
 * foreign_destroy: If TRUE, the window or a parent was destroyed by some
 *     external agency. The window has already been destroyed and no
 *     windowing system calls should be made. (This may never happen
 *     for some windowing systems.)
 */
  void         (* destroy)              (CdkWindow       *window,
					 gboolean         recursing,
					 gboolean         foreign_destroy);


 /* Called when cdk_window_destroy() is called on a foreign window
  * or an ancestor of the foreign window. It should generally reparent
  * the window out of it's current heirarchy, hide it, and then
  * send a message to the owner requesting that the window be destroyed.
  */
  void         (*destroy_foreign)       (CdkWindow       *window);

  /* optional */
  gboolean     (* beep)                 (CdkWindow       *window);

  void         (* focus)                (CdkWindow       *window,
					 guint32          timestamp);
  void         (* set_type_hint)        (CdkWindow       *window,
					 CdkWindowTypeHint hint);
  CdkWindowTypeHint (* get_type_hint)   (CdkWindow       *window);
  void         (* set_modal_hint)       (CdkWindow *window,
					 gboolean   modal);
  void         (* set_skip_taskbar_hint) (CdkWindow *window,
					  gboolean   skips_taskbar);
  void         (* set_skip_pager_hint)  (CdkWindow *window,
					 gboolean   skips_pager);
  void         (* set_urgency_hint)     (CdkWindow *window,
					 gboolean   urgent);
  void         (* set_geometry_hints)   (CdkWindow         *window,
					 const CdkGeometry *geometry,
					 CdkWindowHints     geom_mask);
  void         (* set_title)            (CdkWindow   *window,
					 const gchar *title);
  void         (* set_role)             (CdkWindow   *window,
					 const gchar *role);
  void         (* set_startup_id)       (CdkWindow   *window,
					 const gchar *startup_id);
  void         (* set_transient_for)    (CdkWindow *window,
					 CdkWindow *parent);
  void         (* get_frame_extents)    (CdkWindow    *window,
					 CdkRectangle *rect);
  void         (* set_override_redirect) (CdkWindow *window,
					  gboolean override_redirect);
  void         (* set_accept_focus)     (CdkWindow *window,
					 gboolean accept_focus);
  void         (* set_focus_on_map)     (CdkWindow *window,
					 gboolean focus_on_map);
  void         (* set_icon_list)        (CdkWindow *window,
					 GList     *pixbufs);
  void         (* set_icon_name)        (CdkWindow   *window,
					 const gchar *name);
  void         (* iconify)              (CdkWindow *window);
  void         (* deiconify)            (CdkWindow *window);
  void         (* stick)                (CdkWindow *window);
  void         (* unstick)              (CdkWindow *window);
  void         (* maximize)             (CdkWindow *window);
  void         (* unmaximize)           (CdkWindow *window);
  void         (* fullscreen)           (CdkWindow *window);
  void         (* fullscreen_on_monitor) (CdkWindow *window, gint monitor);
  void         (* apply_fullscreen_mode) (CdkWindow *window);
  void         (* unfullscreen)         (CdkWindow *window);
  void         (* set_keep_above)       (CdkWindow *window,
					 gboolean   setting);
  void         (* set_keep_below)       (CdkWindow *window,
					 gboolean   setting);
  CdkWindow *  (* get_group)            (CdkWindow *window);
  void         (* set_group)            (CdkWindow *window,
					 CdkWindow *leader);
  void         (* set_decorations)      (CdkWindow      *window,
					 CdkWMDecoration decorations);
  gboolean     (* get_decorations)      (CdkWindow       *window,
					 CdkWMDecoration *decorations);
  void         (* set_functions)        (CdkWindow    *window,
					 CdkWMFunction functions);
  void         (* begin_resize_drag)    (CdkWindow     *window,
                                         CdkWindowEdge  edge,
                                         CdkDevice     *device,
                                         gint           button,
                                         gint           root_x,
                                         gint           root_y,
                                         guint32        timestamp);
  void         (* begin_move_drag)      (CdkWindow *window,
                                         CdkDevice     *device,
                                         gint       button,
                                         gint       root_x,
                                         gint       root_y,
                                         guint32    timestamp);
  void         (* enable_synchronized_configure) (CdkWindow *window);
  void         (* configure_finished)   (CdkWindow *window);
  void         (* set_opacity)          (CdkWindow *window,
					 gdouble    opacity);
  void         (* set_composited)       (CdkWindow *window,
                                         gboolean   composited);
  void         (* destroy_notify)       (CdkWindow *window);
  CdkDragProtocol (* get_drag_protocol) (CdkWindow *window,
                                         CdkWindow **target);
  void         (* register_dnd)         (CdkWindow *window);
  CdkDragContext * (*drag_begin)        (CdkWindow *window,
                                         CdkDevice *device,
                                         GList     *targets,
                                         gint       x_root,
                                         gint       y_root);

  void         (*process_updates_recurse) (CdkWindow      *window,
                                           cairo_region_t *region);

  void         (*sync_rendering)          (CdkWindow      *window);
  gboolean     (*simulate_key)            (CdkWindow      *window,
                                           gint            x,
                                           gint            y,
                                           guint           keyval,
                                           CdkModifierType modifiers,
                                           CdkEventType    event_type);
  gboolean     (*simulate_button)         (CdkWindow      *window,
                                           gint            x,
                                           gint            y,
                                           guint           button,
                                           CdkModifierType modifiers,
                                           CdkEventType    event_type);

  gboolean     (*get_property)            (CdkWindow      *window,
                                           CdkAtom         property,
                                           CdkAtom         type,
                                           gulong          offset,
                                           gulong          length,
                                           gint            pdelete,
                                           CdkAtom        *actual_type,
                                           gint           *actual_format,
                                           gint           *actual_length,
                                           guchar        **data);
  void         (*change_property)         (CdkWindow      *window,
                                           CdkAtom         property,
                                           CdkAtom         type,
                                           gint            format,
                                           CdkPropMode     mode,
                                           const guchar   *data,
                                           gint            n_elements);
  void         (*delete_property)         (CdkWindow      *window,
                                           CdkAtom         property);

  gint         (* get_scale_factor)       (CdkWindow      *window);
  void         (* get_unscaled_size)      (CdkWindow      *window,
                                           int            *unscaled_width,
                                           int            *unscaled_height);

  void         (* set_opaque_region)      (CdkWindow      *window,
                                           cairo_region_t *region);
  void         (* set_shadow_width)       (CdkWindow      *window,
                                           gint            left,
                                           gint            right,
                                           gint            top,
                                           gint            bottom);
  gboolean     (* show_window_menu)       (CdkWindow      *window,
                                           CdkEvent       *event);
  CdkGLContext *(*create_gl_context)      (CdkWindow      *window,
					   gboolean        attached,
                                           CdkGLContext   *share,
                                           GError        **error);
  gboolean     (* realize_gl_context)     (CdkWindow      *window,
                                           CdkGLContext   *context,
                                           GError        **error);
  void         (*invalidate_for_new_frame)(CdkWindow      *window,
                                           cairo_region_t *update_area);

  CdkDrawingContext *(* create_draw_context)  (CdkWindow            *window,
                                               const cairo_region_t *region);
  void               (* destroy_draw_context) (CdkWindow            *window,
                                               CdkDrawingContext    *context);
};

/* Interface Functions */
GType cdk_window_impl_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CDK_WINDOW_IMPL_H__ */
