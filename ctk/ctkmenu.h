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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_MENU_H__
#define __CTK_MENU_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkaccelgroup.h>
#include <ctk/ctkmenushell.h>

G_BEGIN_DECLS

#define CTK_TYPE_MENU			(ctk_menu_get_type ())
#define CTK_MENU(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MENU, CtkMenu))
#define CTK_MENU_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MENU, CtkMenuClass))
#define CTK_IS_MENU(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MENU))
#define CTK_IS_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MENU))
#define CTK_MENU_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MENU, CtkMenuClass))


typedef struct _CtkMenu        CtkMenu;
typedef struct _CtkMenuClass   CtkMenuClass;
typedef struct _CtkMenuPrivate CtkMenuPrivate;

/**
 * CtkArrowPlacement:
 * @CTK_ARROWS_BOTH: Place one arrow on each end of the menu.
 * @CTK_ARROWS_START: Place both arrows at the top of the menu.
 * @CTK_ARROWS_END: Place both arrows at the bottom of the menu.
 *
 * Used to specify the placement of scroll arrows in scrolling menus.
 */
typedef enum
{
  CTK_ARROWS_BOTH,
  CTK_ARROWS_START,
  CTK_ARROWS_END
} CtkArrowPlacement;

/**
 * CtkMenuPositionFunc:
 * @menu: a #CtkMenu.
 * @x: (inout): address of the #gint representing the horizontal
 *     position where the menu shall be drawn.
 * @y: (inout): address of the #gint representing the vertical position
 *     where the menu shall be drawn.  This is an output parameter.
 * @push_in: (out): This parameter controls how menus placed outside
 *     the monitor are handled.  If this is set to %TRUE and part of
 *     the menu is outside the monitor then CTK+ pushes the window
 *     into the visible area, effectively modifying the popup
 *     position.  Note that moving and possibly resizing the menu
 *     around will alter the scroll position to keep the menu items
 *     “in place”, i.e. at the same monitor position they would have
 *     been without resizing.  In practice, this behavior is only
 *     useful for combobox popups or option menus and cannot be used
 *     to simply confine a menu to monitor boundaries.  In that case,
 *     changing the scroll offset is not desirable.
 * @user_data: the data supplied by the user in the ctk_menu_popup()
 *     @data parameter.
 *
 * A user function supplied when calling ctk_menu_popup() which
 * controls the positioning of the menu when it is displayed.  The
 * function sets the @x and @y parameters to the coordinates where the
 * menu is to be drawn.  To make the menu appear on a different
 * monitor than the mouse pointer, ctk_menu_set_monitor() must be
 * called.
 */
typedef void (*CtkMenuPositionFunc) (CtkMenu   *menu,
				     gint      *x,
				     gint      *y,
				     gboolean  *push_in,
				     gpointer	user_data);

/**
 * CtkMenuDetachFunc:
 * @attach_widget: the #CtkWidget that the menu is being detached from.
 * @menu: the #CtkMenu being detached.
 *
 * A user function supplied when calling ctk_menu_attach_to_widget() which 
 * will be called when the menu is later detached from the widget.
 */
typedef void (*CtkMenuDetachFunc)   (CtkWidget *attach_widget,
				     CtkMenu   *menu);

struct _CtkMenu
{
  CtkMenuShell menu_shell;

  /*< private >*/
  CtkMenuPrivate *priv;
};

struct _CtkMenuClass
{
  CtkMenuShellClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType	   ctk_menu_get_type		  (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_new			  (void);
GDK_AVAILABLE_IN_3_4
CtkWidget* ctk_menu_new_from_model        (GMenuModel *model);

/* Display the menu onscreen */
GDK_DEPRECATED_IN_3_22_FOR((ctk_menu_popup_at_widget, ctk_menu_popup_at_pointer, ctk_menu_popup_at_rect))
void	   ctk_menu_popup		  (CtkMenu	       *menu,
					   CtkWidget	       *parent_menu_shell,
					   CtkWidget	       *parent_menu_item,
					   CtkMenuPositionFunc	func,
					   gpointer		data,
					   guint		button,
					   guint32		activate_time);
GDK_DEPRECATED_IN_3_22_FOR((ctk_menu_popup_at_widget, ctk_menu_popup_at_pointer, ctk_menu_popup_at_rect))
void       ctk_menu_popup_for_device      (CtkMenu             *menu,
                                           GdkDevice           *device,
                                           CtkWidget           *parent_menu_shell,
                                           CtkWidget           *parent_menu_item,
                                           CtkMenuPositionFunc  func,
                                           gpointer             data,
                                           GDestroyNotify       destroy,
                                           guint                button,
                                           guint32              activate_time);
GDK_AVAILABLE_IN_3_22
void       ctk_menu_popup_at_rect         (CtkMenu             *menu,
                                           GdkWindow           *rect_window,
                                           const GdkRectangle  *rect,
                                           GdkGravity           rect_anchor,
                                           GdkGravity           menu_anchor,
                                           const GdkEvent      *trigger_event);
GDK_AVAILABLE_IN_3_22
void       ctk_menu_popup_at_widget       (CtkMenu             *menu,
                                           CtkWidget           *widget,
                                           GdkGravity           widget_anchor,
                                           GdkGravity           menu_anchor,
                                           const GdkEvent      *trigger_event);
GDK_AVAILABLE_IN_3_22
void       ctk_menu_popup_at_pointer      (CtkMenu             *menu,
                                           const GdkEvent      *trigger_event);

/* Position the menu according to its position function. Called
 * from ctkmenuitem.c when a menu-item changes its allocation
 */
GDK_AVAILABLE_IN_ALL
void	   ctk_menu_reposition		  (CtkMenu	       *menu);

GDK_AVAILABLE_IN_ALL
void	   ctk_menu_popdown		  (CtkMenu	       *menu);

/* Keep track of the last menu item selected. (For the purposes
 * of the option menu
 */
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_get_active		  (CtkMenu	       *menu);
GDK_AVAILABLE_IN_ALL
void	   ctk_menu_set_active		  (CtkMenu	       *menu,
					   guint		index);

/* set/get the accelerator group that holds global accelerators (should
 * be added to the corresponding toplevel with ctk_window_add_accel_group().
 */
GDK_AVAILABLE_IN_ALL
void	       ctk_menu_set_accel_group	  (CtkMenu	       *menu,
					   CtkAccelGroup       *accel_group);
GDK_AVAILABLE_IN_ALL
CtkAccelGroup* ctk_menu_get_accel_group	  (CtkMenu	       *menu);
GDK_AVAILABLE_IN_ALL
void           ctk_menu_set_accel_path    (CtkMenu             *menu,
					   const gchar         *accel_path);
GDK_AVAILABLE_IN_ALL
const gchar*   ctk_menu_get_accel_path    (CtkMenu             *menu);

/* A reference count is kept for a widget when it is attached to
 * a particular widget. This is typically a menu item; it may also
 * be a widget with a popup menu - for instance, the Notebook widget.
 */
GDK_AVAILABLE_IN_ALL
void	   ctk_menu_attach_to_widget	  (CtkMenu	       *menu,
					   CtkWidget	       *attach_widget,
					   CtkMenuDetachFunc	detacher);
GDK_AVAILABLE_IN_ALL
void	   ctk_menu_detach		  (CtkMenu	       *menu);

/* This should be dumped in favor of data set when the menu is popped
 * up - that is currently in the ItemFactory code, but should be
 * in the Menu code.
 */
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_menu_get_attach_widget	  (CtkMenu	       *menu);

GDK_DEPRECATED_IN_3_10
void       ctk_menu_set_tearoff_state     (CtkMenu             *menu,
                                           gboolean             torn_off);
GDK_DEPRECATED_IN_3_10
gboolean   ctk_menu_get_tearoff_state     (CtkMenu             *menu);

/* This sets the window manager title for the window that
 * appears when a menu is torn off
 */
GDK_DEPRECATED_IN_3_10
void          ctk_menu_set_title          (CtkMenu             *menu,
                                           const gchar         *title);
GDK_DEPRECATED_IN_3_10
const gchar * ctk_menu_get_title          (CtkMenu             *menu);

GDK_AVAILABLE_IN_ALL
void       ctk_menu_reorder_child         (CtkMenu             *menu,
                                           CtkWidget           *child,
                                           gint                position);

GDK_AVAILABLE_IN_ALL
void	   ctk_menu_set_screen		  (CtkMenu	       *menu,
					   GdkScreen	       *screen);

GDK_AVAILABLE_IN_ALL
void       ctk_menu_attach                (CtkMenu             *menu,
                                           CtkWidget           *child,
                                           guint                left_attach,
                                           guint                right_attach,
                                           guint                top_attach,
                                           guint                bottom_attach);

GDK_AVAILABLE_IN_ALL
void       ctk_menu_set_monitor           (CtkMenu             *menu,
                                           gint                 monitor_num);
GDK_AVAILABLE_IN_ALL
gint       ctk_menu_get_monitor           (CtkMenu             *menu);

GDK_AVAILABLE_IN_3_22
void       ctk_menu_place_on_monitor      (CtkMenu             *menu,
                                           GdkMonitor          *monitor);

GDK_AVAILABLE_IN_ALL
GList*     ctk_menu_get_for_attach_widget (CtkWidget           *widget); 

GDK_AVAILABLE_IN_ALL
void     ctk_menu_set_reserve_toggle_size (CtkMenu  *menu,
                                          gboolean   reserve_toggle_size);
GDK_AVAILABLE_IN_ALL
gboolean ctk_menu_get_reserve_toggle_size (CtkMenu  *menu);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkMenu, g_object_unref)

G_END_DECLS

#endif /* __CTK_MENU_H__ */
