/* CTK - The GIMP Toolkit
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

#ifndef __CTK_WINDOW_H__
#define __CTK_WINDOW_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkapplication.h>
#include <ctk/ctkaccelgroup.h>
#include <ctk/ctkbin.h>

G_BEGIN_DECLS

#define CTK_TYPE_WINDOW			(ctk_window_get_type ())
#define CTK_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_WINDOW, CtkWindow))
#define CTK_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_WINDOW, CtkWindowClass))
#define CTK_IS_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_WINDOW))
#define CTK_IS_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_WINDOW))
#define CTK_WINDOW_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_WINDOW, CtkWindowClass))

typedef struct _CtkWindowPrivate      CtkWindowPrivate;
typedef struct _CtkWindowClass        CtkWindowClass;
typedef struct _CtkWindowGeometryInfo CtkWindowGeometryInfo;
typedef struct _CtkWindowGroup        CtkWindowGroup;
typedef struct _CtkWindowGroupClass   CtkWindowGroupClass;
typedef struct _CtkWindowGroupPrivate CtkWindowGroupPrivate;

struct _CtkWindow
{
  CtkBin bin;

  CtkWindowPrivate *priv;
};

/**
 * CtkWindowClass:
 * @parent_class: The parent class.
 * @set_focus: Sets child as the focus widget for the window.
 * @activate_focus: Activates the current focused widget within the window.
 * @activate_default: Activates the default widget for the window.
 * @keys_changed: Signal gets emitted when the set of accelerators or
 *   mnemonics that are associated with window changes.
 * @enable_debugging: Class handler for the #CtkWindow::enable-debugging
 *   keybinding signal. Since: 3.14
 */
struct _CtkWindowClass
{
  CtkBinClass parent_class;

  /*< public >*/

  void     (* set_focus)   (CtkWindow *window,
                            CtkWidget *focus);

  /* G_SIGNAL_ACTION signals for keybindings */

  void     (* activate_focus)   (CtkWindow *window);
  void     (* activate_default) (CtkWindow *window);
  void	   (* keys_changed)     (CtkWindow *window);
  gboolean (* enable_debugging) (CtkWindow *window,
                                 gboolean   toggle);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
};

/**
 * CtkWindowType:
 * @CTK_WINDOW_TOPLEVEL: A regular window, such as a dialog.
 * @CTK_WINDOW_POPUP: A special window such as a tooltip.
 *
 * A #CtkWindow can be one of these types. Most things you’d consider a
 * “window” should have type #CTK_WINDOW_TOPLEVEL; windows with this type
 * are managed by the window manager and have a frame by default (call
 * ctk_window_set_decorated() to toggle the frame).  Windows with type
 * #CTK_WINDOW_POPUP are ignored by the window manager; window manager
 * keybindings won’t work on them, the window manager won’t decorate the
 * window with a frame, many CTK+ features that rely on the window
 * manager will not work (e.g. resize grips and
 * maximization/minimization). #CTK_WINDOW_POPUP is used to implement
 * widgets such as #CtkMenu or tooltips that you normally don’t think of
 * as windows per se. Nearly all windows should be #CTK_WINDOW_TOPLEVEL.
 * In particular, do not use #CTK_WINDOW_POPUP just to turn off
 * the window borders; use ctk_window_set_decorated() for that.
 */
typedef enum
{
  CTK_WINDOW_TOPLEVEL,
  CTK_WINDOW_POPUP
} CtkWindowType;

/**
 * CtkWindowPosition:
 * @CTK_WIN_POS_NONE: No influence is made on placement.
 * @CTK_WIN_POS_CENTER: Windows should be placed in the center of the screen.
 * @CTK_WIN_POS_MOUSE: Windows should be placed at the current mouse position.
 * @CTK_WIN_POS_CENTER_ALWAYS: Keep window centered as it changes size, etc.
 * @CTK_WIN_POS_CENTER_ON_PARENT: Center the window on its transient
 *  parent (see ctk_window_set_transient_for()).
 *
 * Window placement can be influenced using this enumeration. Note that
 * using #CTK_WIN_POS_CENTER_ALWAYS is almost always a bad idea.
 * It won’t necessarily work well with all window managers or on all windowing systems.
 */
typedef enum
{
  CTK_WIN_POS_NONE,
  CTK_WIN_POS_CENTER,
  CTK_WIN_POS_MOUSE,
  CTK_WIN_POS_CENTER_ALWAYS,
  CTK_WIN_POS_CENTER_ON_PARENT
} CtkWindowPosition;


CDK_AVAILABLE_IN_ALL
GType      ctk_window_get_type                 (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_window_new                      (CtkWindowType        type);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_title                (CtkWindow           *window,
						const gchar         *title);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_window_get_title             (CtkWindow           *window);
CDK_DEPRECATED_IN_3_22
void       ctk_window_set_wmclass              (CtkWindow           *window,
						const gchar         *wmclass_name,
						const gchar         *wmclass_class);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_role                 (CtkWindow           *window,
                                                const gchar         *role);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_startup_id           (CtkWindow           *window,
                                                const gchar         *startup_id);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_window_get_role              (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_add_accel_group          (CtkWindow           *window,
						CtkAccelGroup	    *accel_group);
CDK_AVAILABLE_IN_ALL
void       ctk_window_remove_accel_group       (CtkWindow           *window,
						CtkAccelGroup	    *accel_group);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_position             (CtkWindow           *window,
						CtkWindowPosition    position);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_activate_focus	       (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_focus                (CtkWindow           *window,
						CtkWidget           *focus);
CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_window_get_focus                (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_default              (CtkWindow           *window,
						CtkWidget           *default_widget);
CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_window_get_default_widget       (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_activate_default	       (CtkWindow           *window);

CDK_AVAILABLE_IN_ALL
void       ctk_window_set_transient_for        (CtkWindow           *window, 
						CtkWindow           *parent);
CDK_AVAILABLE_IN_ALL
CtkWindow *ctk_window_get_transient_for        (CtkWindow           *window);
CDK_AVAILABLE_IN_3_4
void       ctk_window_set_attached_to          (CtkWindow           *window, 
                                                CtkWidget           *attach_widget);
CDK_AVAILABLE_IN_3_4
CtkWidget *ctk_window_get_attached_to          (CtkWindow           *window);
CDK_DEPRECATED_IN_3_8_FOR(ctk_widget_set_opacity)
void       ctk_window_set_opacity              (CtkWindow           *window, 
						gdouble              opacity);
CDK_DEPRECATED_IN_3_8_FOR(ctk_widget_get_opacity)
gdouble    ctk_window_get_opacity              (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_type_hint            (CtkWindow           *window, 
						CdkWindowTypeHint    hint);
CDK_AVAILABLE_IN_ALL
CdkWindowTypeHint ctk_window_get_type_hint     (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_skip_taskbar_hint    (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_skip_taskbar_hint    (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_skip_pager_hint      (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_skip_pager_hint      (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_urgency_hint         (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_urgency_hint         (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_accept_focus         (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_accept_focus         (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_focus_on_map         (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_focus_on_map         (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_destroy_with_parent  (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_destroy_with_parent  (CtkWindow           *window);
CDK_AVAILABLE_IN_3_4
void       ctk_window_set_hide_titlebar_when_maximized (CtkWindow   *window,
                                                        gboolean     setting);
CDK_AVAILABLE_IN_3_4
gboolean   ctk_window_get_hide_titlebar_when_maximized (CtkWindow   *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_mnemonics_visible    (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_mnemonics_visible    (CtkWindow           *window);
CDK_AVAILABLE_IN_3_2
void       ctk_window_set_focus_visible        (CtkWindow           *window,
                                                gboolean             setting);
CDK_AVAILABLE_IN_3_2
gboolean   ctk_window_get_focus_visible        (CtkWindow           *window);

CDK_AVAILABLE_IN_ALL
void       ctk_window_set_resizable            (CtkWindow           *window,
                                                gboolean             resizable);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_resizable            (CtkWindow           *window);

CDK_AVAILABLE_IN_ALL
void       ctk_window_set_gravity              (CtkWindow           *window,
                                                CdkGravity           gravity);
CDK_AVAILABLE_IN_ALL
CdkGravity ctk_window_get_gravity              (CtkWindow           *window);


CDK_AVAILABLE_IN_ALL
void       ctk_window_set_geometry_hints       (CtkWindow           *window,
						CtkWidget           *geometry_widget,
						CdkGeometry         *geometry,
						CdkWindowHints       geom_mask);

CDK_AVAILABLE_IN_ALL
void	   ctk_window_set_screen	       (CtkWindow	    *window,
						CdkScreen	    *screen);
CDK_AVAILABLE_IN_ALL
CdkScreen* ctk_window_get_screen	       (CtkWindow	    *window);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_is_active                (CtkWindow           *window);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_has_toplevel_focus       (CtkWindow           *window);

CDK_AVAILABLE_IN_ALL
void       ctk_window_set_decorated            (CtkWindow *window,
                                                gboolean   setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_decorated            (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_deletable            (CtkWindow *window,
                                                gboolean   setting);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_deletable            (CtkWindow *window);

CDK_AVAILABLE_IN_ALL
void       ctk_window_set_icon_list                (CtkWindow  *window,
                                                    GList      *list);
CDK_AVAILABLE_IN_ALL
GList*     ctk_window_get_icon_list                (CtkWindow  *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_icon                     (CtkWindow  *window,
                                                    GdkPixbuf  *icon);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_icon_name                (CtkWindow   *window,
						    const gchar *name);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_set_icon_from_file           (CtkWindow   *window,
						    const gchar *filename,
						    GError     **err);
CDK_AVAILABLE_IN_ALL
GdkPixbuf* ctk_window_get_icon                     (CtkWindow  *window);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_window_get_icon_name             (CtkWindow  *window);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_default_icon_list        (GList      *list);
CDK_AVAILABLE_IN_ALL
GList*     ctk_window_get_default_icon_list        (void);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_default_icon             (GdkPixbuf  *icon);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_default_icon_name        (const gchar *name);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_window_get_default_icon_name     (void);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_set_default_icon_from_file   (const gchar *filename,
						    GError     **err);

CDK_AVAILABLE_IN_ALL
void       ctk_window_set_auto_startup_notification (gboolean setting);

/* If window is set modal, input will be grabbed when show and released when hide */
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_modal      (CtkWindow *window,
				      gboolean   modal);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_window_get_modal      (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
GList*     ctk_window_list_toplevels (void);
CDK_AVAILABLE_IN_ALL
void       ctk_window_set_has_user_ref_count (CtkWindow *window,
                                              gboolean   setting);

CDK_AVAILABLE_IN_ALL
void     ctk_window_add_mnemonic          (CtkWindow       *window,
					   guint            keyval,
					   CtkWidget       *target);
CDK_AVAILABLE_IN_ALL
void     ctk_window_remove_mnemonic       (CtkWindow       *window,
					   guint            keyval,
					   CtkWidget       *target);
CDK_AVAILABLE_IN_ALL
gboolean ctk_window_mnemonic_activate     (CtkWindow       *window,
					   guint            keyval,
					   CdkModifierType  modifier);
CDK_AVAILABLE_IN_ALL
void     ctk_window_set_mnemonic_modifier (CtkWindow       *window,
					   CdkModifierType  modifier);
CDK_AVAILABLE_IN_ALL
CdkModifierType ctk_window_get_mnemonic_modifier (CtkWindow *window);

CDK_AVAILABLE_IN_ALL
gboolean ctk_window_activate_key          (CtkWindow        *window,
					   CdkEventKey      *event);
CDK_AVAILABLE_IN_ALL
gboolean ctk_window_propagate_key_event   (CtkWindow        *window,
					   CdkEventKey      *event);

CDK_AVAILABLE_IN_ALL
void     ctk_window_present            (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_present_with_time  (CtkWindow *window,
				        guint32    timestamp);
CDK_AVAILABLE_IN_ALL
void     ctk_window_iconify       (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_deiconify     (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_stick         (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_unstick       (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_maximize      (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_unmaximize    (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_fullscreen    (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_unfullscreen  (CtkWindow *window);
CDK_AVAILABLE_IN_3_18
void     ctk_window_fullscreen_on_monitor(CtkWindow *window,
                                          CdkScreen *screen,
                                          gint monitor);
CDK_AVAILABLE_IN_3_10
void     ctk_window_close         (CtkWindow *window);
CDK_AVAILABLE_IN_ALL
void     ctk_window_set_keep_above    (CtkWindow *window, gboolean setting);
CDK_AVAILABLE_IN_ALL
void     ctk_window_set_keep_below    (CtkWindow *window, gboolean setting);

CDK_AVAILABLE_IN_ALL
void ctk_window_begin_resize_drag (CtkWindow     *window,
                                   CdkWindowEdge  edge,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);
CDK_AVAILABLE_IN_ALL
void ctk_window_begin_move_drag   (CtkWindow     *window,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);

/* Set initial default size of the window (does not constrain user
 * resize operations)
 */
CDK_AVAILABLE_IN_ALL
void     ctk_window_set_default_size (CtkWindow   *window,
                                      gint         width,
                                      gint         height);
CDK_AVAILABLE_IN_ALL
void     ctk_window_get_default_size (CtkWindow   *window,
                                      gint        *width,
                                      gint        *height);
CDK_AVAILABLE_IN_ALL
void     ctk_window_resize           (CtkWindow   *window,
                                      gint         width,
                                      gint         height);
CDK_AVAILABLE_IN_ALL
void     ctk_window_get_size         (CtkWindow   *window,
                                      gint        *width,
                                      gint        *height);
CDK_AVAILABLE_IN_ALL
void     ctk_window_move             (CtkWindow   *window,
                                      gint         x,
                                      gint         y);
CDK_AVAILABLE_IN_ALL
void     ctk_window_get_position     (CtkWindow   *window,
                                      gint        *root_x,
                                      gint        *root_y);
CDK_DEPRECATED_IN_3_20
gboolean ctk_window_parse_geometry   (CtkWindow   *window,
                                      const gchar *geometry);

CDK_DEPRECATED_IN_3_20_FOR(ctk_window_set_default_size)
void ctk_window_set_default_geometry (CtkWindow *window,
                                      gint       width,
                                      gint       height);
CDK_DEPRECATED_IN_3_20_FOR(ctk_window_resize)
void ctk_window_resize_to_geometry   (CtkWindow *window,
                                      gint       width,
                                      gint       height);

CDK_AVAILABLE_IN_ALL
CtkWindowGroup *ctk_window_get_group (CtkWindow   *window);
CDK_AVAILABLE_IN_ALL
gboolean ctk_window_has_group        (CtkWindow   *window);

/* Ignore this unless you are writing a GUI builder */
CDK_DEPRECATED_IN_3_10
void     ctk_window_reshow_with_initial_size (CtkWindow *window);

CDK_AVAILABLE_IN_ALL
CtkWindowType ctk_window_get_window_type     (CtkWindow     *window);


CDK_AVAILABLE_IN_ALL
CtkApplication *ctk_window_get_application      (CtkWindow          *window);
CDK_AVAILABLE_IN_ALL
void            ctk_window_set_application      (CtkWindow          *window,
                                                 CtkApplication     *application);


/* Window grips
 */
CDK_DEPRECATED_IN_3_14
void     ctk_window_set_has_resize_grip    (CtkWindow    *window,
                                            gboolean      value);
CDK_DEPRECATED_IN_3_14
gboolean ctk_window_get_has_resize_grip    (CtkWindow    *window);
CDK_DEPRECATED_IN_3_14
gboolean ctk_window_resize_grip_is_visible (CtkWindow    *window);
CDK_DEPRECATED_IN_3_14
gboolean ctk_window_get_resize_grip_area   (CtkWindow    *window,
                                            CdkRectangle *rect);

CDK_AVAILABLE_IN_3_10
void     ctk_window_set_titlebar           (CtkWindow    *window,
                                            CtkWidget    *titlebar);
CDK_AVAILABLE_IN_3_16
CtkWidget *ctk_window_get_titlebar         (CtkWindow    *window);

CDK_AVAILABLE_IN_3_12
gboolean ctk_window_is_maximized           (CtkWindow    *window);

CDK_AVAILABLE_IN_3_14
void     ctk_window_set_interactive_debugging (gboolean enable);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkWindow, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkWindowGroup, g_object_unref)

G_END_DECLS

#endif /* __CTK_WINDOW_H__ */
