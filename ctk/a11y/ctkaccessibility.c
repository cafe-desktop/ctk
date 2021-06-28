/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#include "ctkaccessibility.h"
#include "ctkaccessibilityutil.h"
#include "ctkaccessibilitymisc.h"

#include "ctkwindowaccessible.h"

#include <stdio.h>
#include <stdlib.h>

#include <cdk/cdk.h>
#include <ctk/ctkcombobox.h>
#include <ctk/ctkentry.h>
#include <ctk/ctknotebook.h>
#include <ctk/ctkmenuitem.h>
#include <ctk/ctkmenu.h>
#include <ctk/ctkmenubar.h>
#include <ctk/ctksocket.h>
#include <ctk/ctktogglebutton.h>
#include <ctk/ctkaccessible.h>

#ifdef GDK_WINDOWING_X11
#include <atk-bridge.h>
#endif

static gboolean gail_focus_watcher      (GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static gboolean gail_select_watcher     (GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static gboolean gail_deselect_watcher   (GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static gboolean gail_switch_page_watcher(GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static void     gail_finish_select       (CtkWidget            *widget);
static void     gail_map_cb              (CtkWidget            *widget);
static void     gail_map_submenu_cb      (CtkWidget            *widget);
static gint     gail_focus_idle_handler  (gpointer             data);
static void     gail_focus_notify        (CtkWidget            *widget);
static void     gail_focus_notify_when_idle (CtkWidget            *widget);

static void     gail_focus_tracker_init (void);
static void     gail_focus_object_destroyed (gpointer data);
static void     gail_focus_tracker (AtkObject *object);
static void     gail_set_focus_widget (CtkWidget *focus_widget,
                                       CtkWidget *widget);
static void     gail_set_focus_object (AtkObject *focus_obj,
                                       AtkObject *obj);

CtkWidget* _focus_widget = NULL;
static CtkWidget* next_focus_widget = NULL;
static gboolean was_deselect = FALSE;
static CtkWidget* subsequent_focus_widget = NULL;
static CtkWidget* focus_before_menu = NULL;
static guint focus_notify_handler = 0;
static guint focus_tracker_id = 0;
static GQuark quark_focus_object = 0;
static int initialized = FALSE;

static AtkObject*
get_accessible_for_widget (CtkWidget *widget,
                           gboolean  *transient)
{
  AtkObject *obj = NULL;

  *transient = FALSE;
  if (!widget)
    return NULL;

  if (CTK_IS_ENTRY (widget))
    ;
  else if (CTK_IS_NOTEBOOK (widget)) 
    {
      CtkNotebook *notebook;
      gint page_num = -1;

      notebook = CTK_NOTEBOOK (widget);
      page_num = ctk_notebook_get_current_page (notebook);
      if (page_num != -1)
        {
          obj = ctk_widget_get_accessible (widget);
          obj = atk_object_ref_accessible_child (obj, page_num);
          g_object_unref (obj);
        }
    }
  else if (CTK_IS_TOGGLE_BUTTON (widget))
    {
      CtkWidget *other_widget = ctk_widget_get_parent (widget);
      if (CTK_IS_COMBO_BOX (other_widget))
        {
          gail_set_focus_widget (other_widget, widget);
          widget = other_widget;
        }
    }
  if (obj == NULL)
    {
      AtkObject *focus_object;

      obj = ctk_widget_get_accessible (widget);
      focus_object = g_object_get_qdata (G_OBJECT (obj), quark_focus_object);
      /*
       * We check whether the object for this focus_object has been deleted.
       * This can happen when navigating to an empty directory in nautilus. 
       * See bug #141907.
       */
      if (ATK_IS_GOBJECT_ACCESSIBLE (focus_object))
        {
          if (!atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (focus_object)))
            focus_object = NULL;
        }
      if (focus_object)
        obj = focus_object;
    }

  return obj;
}

static gboolean
gail_focus_watcher (GSignalInvocationHint *ihint,
                    guint                  n_param_values,
                    const GValue          *param_values,
                    gpointer               data)
{
  GObject *object;
  CtkWidget *widget;
  CdkEvent *event;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (CTK_IS_WIDGET(object), FALSE);

  event = g_value_get_boxed (param_values + 1);
  widget = CTK_WIDGET (object);

  if (event->type == GDK_FOCUS_CHANGE) 
    {
      if (event->focus_change.in)
        {
          if (CTK_IS_WINDOW (widget))
            {
              CtkWidget *focus_widget;
              CtkWindow *window;
              CtkWindowType type;

              window = CTK_WINDOW (widget);
              focus_widget = ctk_window_get_focus (window);
              g_object_get (window, "type", &type, NULL);

              if (focus_widget)
                {
                  /*
                   * If we already have a potential focus widget set this
                   * windows's focus widget to focus_before_menu so that 
                   * it will be reported when menu item is unset.
                   */
                  if (next_focus_widget)
                    {
                      if (CTK_IS_MENU_ITEM (next_focus_widget) &&
                          !focus_before_menu)
                        {
                          void *vp_focus_before_menu = &focus_before_menu;
                          focus_before_menu = focus_widget;
                          g_object_add_weak_pointer (G_OBJECT (focus_before_menu), vp_focus_before_menu);
                        }

                      return TRUE;
                    }
                  widget = focus_widget;
                }
              else if (type == CTK_WINDOW_POPUP)
                {
	          if (CTK_IS_BIN (widget))
		    {
		      CtkWidget *child = ctk_bin_get_child (CTK_BIN (widget));

		      if (CTK_IS_WIDGET (child) && ctk_widget_has_grab (child))
			{
			  if (CTK_IS_MENU_SHELL (child))
			    {
			      if (ctk_menu_shell_get_selected_item (CTK_MENU_SHELL (child)))
				{
				  /*
				   * We have a menu which has a menu item selected
				   * so we do not report focus on the menu.
				   */ 
				  return TRUE; 
				}
			    }
			  widget = child;
			} 
		    }
		  else /* popup window has no children; this edge case occurs in some custom code (OOo for instance) */
		    {
		      return TRUE;
		    }
                }
	      else /* Widget is a non-popup toplevel with no focus children; 
		      don't emit for this case either, as it's useless */
		{
		  return TRUE;
		}
            }
        }
      else
        {
          if (next_focus_widget)
            {
               CtkWidget *toplevel;

               toplevel = ctk_widget_get_toplevel (next_focus_widget);
               if (toplevel == widget)
                 next_focus_widget = NULL; 
            }
          /* focus out */
          widget = NULL;
        }
    }
  else
    {
      if (event->type == GDK_MOTION_NOTIFY && ctk_widget_has_focus (widget))
        {
          if (widget == _focus_widget)
            {
              return TRUE;
            }
        }
      else
        {
          return TRUE;
        }
    }

#ifdef GDK_WINDOWING_X11
  /*
   * If the focus widget is a CtkSocket without a plug
   * then ignore the focus notification as the embedded
   * plug will report a focus notification.
   */
  if (CTK_IS_SOCKET (widget) &&
      ctk_socket_get_plug_window (CTK_SOCKET (widget)) != NULL)
    return TRUE;
#endif

  /*
   * The widget may not yet be visible on the screen so we wait until it is.
   */
  gail_focus_notify_when_idle (widget);
  return TRUE; 
}

static gboolean
gail_select_watcher (GSignalInvocationHint *ihint,
                     guint                  n_param_values,
                     const GValue          *param_values,
                     gpointer               data)
{
  GObject *object;
  CtkWidget *widget;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (CTK_IS_WIDGET(object), FALSE);

  widget = CTK_WIDGET (object);

  if (!ctk_widget_get_mapped (widget))
    {
      g_signal_connect (widget, "map",
                        G_CALLBACK (gail_map_cb),
                        NULL);
    }
  else
    gail_finish_select (widget);

  return TRUE;
}

static void
gail_finish_select (CtkWidget *widget)
{
  if (CTK_IS_MENU_ITEM (widget))
    {
      CtkMenuItem* menu_item;
      CtkWidget *submenu;

      menu_item = CTK_MENU_ITEM (widget);
      submenu = ctk_menu_item_get_submenu (menu_item);
      if (submenu &&
          !ctk_widget_get_mapped (submenu))
        {
          /*
           * If the submenu is not visble, wait until it is before
           * reporting focus on the menu item.
           */
          gulong handler_id;

          handler_id = g_signal_handler_find (submenu,
                                              G_SIGNAL_MATCH_FUNC,
                                              g_signal_lookup ("map",
                                                               CTK_TYPE_WINDOW),
                                              0,
                                              NULL,
                                              (gpointer) gail_map_submenu_cb,
                                              NULL); 
          if (!handler_id)
            g_signal_connect (submenu, "map",
                              G_CALLBACK (gail_map_submenu_cb),
                              NULL);

          return;
        }
      /*
       * If we are waiting to report focus on a menubar or a menu item
       * because of a previous deselect, cancel it.
       */
      if (was_deselect &&
          focus_notify_handler &&
          next_focus_widget &&
          (CTK_IS_MENU_BAR (next_focus_widget) ||
           CTK_IS_MENU_ITEM (next_focus_widget)))
        {
          void *vp_next_focus_widget = &next_focus_widget;
          g_source_remove (focus_notify_handler);
          g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
	  next_focus_widget = NULL;
          focus_notify_handler = 0;
          was_deselect = FALSE;
        }
    } 
  /*
   * If previously focused widget is not a CtkMenuItem or a CtkMenu,
   * keep track of it so we can return to it after menubar is deactivated
   */
  if (_focus_widget && 
      !CTK_IS_MENU_ITEM (_focus_widget) && 
      !CTK_IS_MENU (_focus_widget))
    {
      void *vp_focus_before_menu = &focus_before_menu;
      focus_before_menu = _focus_widget;
      g_object_add_weak_pointer (G_OBJECT (focus_before_menu), vp_focus_before_menu);

    } 
  gail_focus_notify_when_idle (widget);

  return; 
}

static void
gail_map_cb (CtkWidget *widget)
{
  gail_finish_select (widget);
}

static void
gail_map_submenu_cb (CtkWidget *widget)
{
  if (CTK_IS_MENU (widget))
    {
      CtkWidget *parent_menu_item;

      parent_menu_item = ctk_menu_get_attach_widget (CTK_MENU (widget));
      if (parent_menu_item)
        gail_finish_select (parent_menu_item);
    }
}


static gboolean
gail_deselect_watcher (GSignalInvocationHint *ihint,
                       guint                  n_param_values,
                       const GValue          *param_values,
                       gpointer               data)
{
  GObject *object;
  CtkWidget *widget;
  CtkWidget *menu_shell;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (CTK_IS_WIDGET(object), FALSE);

  widget = CTK_WIDGET (object);

  if (!CTK_IS_MENU_ITEM (widget))
    return TRUE;

  if (subsequent_focus_widget == widget)
    subsequent_focus_widget = NULL;

  menu_shell = ctk_widget_get_parent (widget);
  if (CTK_IS_MENU_SHELL (menu_shell))
    {
      CtkWidget *parent_menu_shell;

      parent_menu_shell = ctk_menu_shell_get_parent_shell (CTK_MENU_SHELL (menu_shell));
      if (parent_menu_shell)
        {
          CtkWidget *active_menu_item;

          active_menu_item = ctk_menu_shell_get_selected_item (CTK_MENU_SHELL (parent_menu_shell));
          if (active_menu_item)
            {
              gail_focus_notify_when_idle (active_menu_item);
            }
        }
      else
        {
          if (!CTK_IS_MENU_BAR (menu_shell))
            {
              gail_focus_notify_when_idle (menu_shell);
            }
        }
    }
  was_deselect = TRUE;
  return TRUE; 
}

static gboolean 
gail_switch_page_watcher (GSignalInvocationHint *ihint,
                          guint                  n_param_values,
                          const GValue          *param_values,
                          gpointer               data)
{
  GObject *object;
  CtkWidget *widget;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (CTK_IS_WIDGET(object), FALSE);

  widget = CTK_WIDGET (object);

  if (!CTK_IS_NOTEBOOK (widget))
    return TRUE;

  if (ctk_notebook_get_current_page (CTK_NOTEBOOK (widget)) == -1)
    return TRUE;

  gail_focus_notify_when_idle (widget);
  return TRUE;
}

static gboolean
gail_focus_idle_handler (gpointer data)
{
  focus_notify_handler = 0;
  /*
   * The widget which was to receive focus may have been removed
   */
  if (!next_focus_widget)
    {
      if (next_focus_widget != data)
        return FALSE;
    }
  else
    {
      void *vp_next_focus_widget = &next_focus_widget;
      g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
      next_focus_widget = NULL;
    }
    
  gail_focus_notify (data);

  return FALSE;
}

static void
gail_focus_notify (CtkWidget *widget)
{
  AtkObject *atk_obj;
  gboolean transient;

  if (widget != _focus_widget)
    {
      if (_focus_widget)
        {
          void *vp_focus_widget = &_focus_widget;
          g_object_remove_weak_pointer (G_OBJECT (_focus_widget), vp_focus_widget);
        }
      _focus_widget = widget;
      if (_focus_widget)
        {
          void *vp_focus_widget = &_focus_widget;
          g_object_add_weak_pointer (G_OBJECT (_focus_widget), vp_focus_widget);
          /*
           * The UI may not have been updated yet; e.g. in ctkhtml2
           * html_view_layout() is called in a idle handler
           */
          if (_focus_widget == focus_before_menu)
            {
              void *vp_focus_before_menu = &focus_before_menu;
              g_object_remove_weak_pointer (G_OBJECT (focus_before_menu), vp_focus_before_menu);
              focus_before_menu = NULL;
            }
        }
      gail_focus_notify_when_idle (_focus_widget);
    }
  else
    {
      if (_focus_widget)
        atk_obj  = get_accessible_for_widget (_focus_widget, &transient);
      else
        atk_obj = NULL;
      /*
       * Do not report focus on redundant object
       */
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      if (atk_obj &&
	  (atk_object_get_role(atk_obj) != ATK_ROLE_REDUNDANT_OBJECT))
	  atk_focus_tracker_notify (atk_obj);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      if (atk_obj && transient)
        g_object_unref (atk_obj);
      if (subsequent_focus_widget)
        {
          CtkWidget *tmp_widget = subsequent_focus_widget;
          subsequent_focus_widget = NULL;
          gail_focus_notify_when_idle (tmp_widget);
        }
    }
}

static void
gail_focus_notify_when_idle (CtkWidget *widget)
{
  if (focus_notify_handler)
    {
      if (widget)
        {
          /*
           * Ignore focus request when menu item is going to be focused.
           * See bug #124232.
           */
          if (CTK_IS_MENU_ITEM (next_focus_widget) && !CTK_IS_MENU_ITEM (widget))
            return;

          if (next_focus_widget)
            {
              if (CTK_IS_MENU_ITEM (next_focus_widget) && CTK_IS_MENU_ITEM (widget))
                {
                  if (ctk_menu_item_get_submenu (CTK_MENU_ITEM (next_focus_widget)) == ctk_widget_get_parent (widget))
                    {
                      if (subsequent_focus_widget)
                        g_assert_not_reached ();
                      subsequent_focus_widget = widget;
                      return;
                    } 
                }
            }
          g_source_remove (focus_notify_handler);
          if (next_focus_widget)
	    {
	      void *vp_next_focus_widget = &next_focus_widget;
	      g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
	      next_focus_widget = NULL;
	    }
        }
      else
        /*
         * Ignore if focus is being set to NULL and we are waiting to set focus
         */
        return;
    }

  if (widget)
    {
      void *vp_next_focus_widget = &next_focus_widget;
      next_focus_widget = widget;
      g_object_add_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
    }
  else
    {
      /*
       * We are about to report focus as NULL so remove the weak pointer
       * for the widget we were waiting to report focus on.
       */ 
      if (next_focus_widget)
        {
          void *vp_next_focus_widget = &next_focus_widget;
          g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
          next_focus_widget = NULL;
        }
    }

  focus_notify_handler = cdk_threads_add_idle (gail_focus_idle_handler, widget);
  g_source_set_name_by_id (focus_notify_handler, "[ctk+] gail_focus_idle_handler");
}

static gboolean
gail_deactivate_watcher (GSignalInvocationHint *ihint,
                         guint                  n_param_values,
                         const GValue          *param_values,
                         gpointer               data)
{
  GObject *object;
  CtkWidget *widget;
  CtkMenuShell *shell;
  CtkWidget *focus = NULL;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (CTK_IS_WIDGET(object), FALSE);
  widget = CTK_WIDGET (object);

  g_return_val_if_fail (CTK_IS_MENU_SHELL(widget), TRUE);
  shell = CTK_MENU_SHELL(widget);
  if (! ctk_menu_shell_get_parent_shell (shell))
    focus = focus_before_menu;
      
  /*
   * If we are waiting to report focus on a menubar or a menu item
   * because of a previous deselect, cancel it.
   */
  if (was_deselect &&
      focus_notify_handler &&
      next_focus_widget &&
      (CTK_IS_MENU_BAR (next_focus_widget) ||
       CTK_IS_MENU_ITEM (next_focus_widget)))
    {
      void *vp_next_focus_widget = &next_focus_widget;
      g_source_remove (focus_notify_handler);
      g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
      next_focus_widget = NULL;
      focus_notify_handler = 0;
      was_deselect = FALSE;
    }
  gail_focus_notify_when_idle (focus);

  return TRUE; 
}

static void
gail_focus_tracker_init (void)
{
  static gboolean  emission_hooks_added = FALSE;

  if (!emission_hooks_added)
    {
      /*
       * We cannot be sure that the classes exist so we make sure that they do.
       */
      g_type_class_ref (CTK_TYPE_WIDGET);
      g_type_class_ref (CTK_TYPE_MENU_ITEM);
      g_type_class_ref (CTK_TYPE_MENU_SHELL);
      g_type_class_ref (CTK_TYPE_NOTEBOOK);

      /*
       * We listen for event_after signal and then check that the
       * event was a focus in event so we get called after the event.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("event-after", CTK_TYPE_WIDGET), 0,
             gail_focus_watcher, NULL, (GDestroyNotify) NULL);
      /*
       * A "select" signal is emitted when arrow key is used to
       * move to a list item in the popup window of a CtkCombo or
       * a menu item in a menu.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("select", CTK_TYPE_MENU_ITEM), 0,
             gail_select_watcher, NULL, (GDestroyNotify) NULL);

      /*
       * A "deselect" signal is emitted when arrow key is used to
       * move from a menu item in a menu to the parent menu.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("deselect", CTK_TYPE_MENU_ITEM), 0,
             gail_deselect_watcher, NULL, (GDestroyNotify) NULL);

      /*
       * We listen for deactivate signals on menushells to determine
       * when the "focus" has left the menus.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("deactivate", CTK_TYPE_MENU_SHELL), 0,
             gail_deactivate_watcher, NULL, (GDestroyNotify) NULL);

      /*
       * We listen for "switch-page" signal on a CtkNotebook to notify
       * when page has changed because of clicking on a notebook tab.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("switch-page", CTK_TYPE_NOTEBOOK), 0,
             gail_switch_page_watcher, NULL, (GDestroyNotify) NULL);
      emission_hooks_added = TRUE;
    }
}

static void
gail_focus_object_destroyed (gpointer data)
{
  GObject *obj;

  obj = G_OBJECT (data);
  g_object_set_qdata (obj, quark_focus_object, NULL);
  g_object_unref (obj); 
}

static void
gail_focus_tracker (AtkObject *focus_object)
{
  /*
   * Do not report focus on redundant object
   */
  if (focus_object && 
      (atk_object_get_role(focus_object) != ATK_ROLE_REDUNDANT_OBJECT))
    {
      AtkObject *old_focus_object;

      if (!CTK_IS_ACCESSIBLE (focus_object))
        {
          AtkObject *parent;

          parent = focus_object;
          while (1)
            {
              parent = atk_object_get_parent (parent);
              if (parent == NULL)
                break;
              if (CTK_IS_ACCESSIBLE (parent))
                break;
            }

          if (parent)
            {
              gail_set_focus_object (focus_object, parent);
            }
        }
      else
        {
          old_focus_object = g_object_get_qdata (G_OBJECT (focus_object), quark_focus_object);
          if (old_focus_object)
            {
              g_object_weak_unref (G_OBJECT (old_focus_object),
                                   (GWeakNotify) gail_focus_object_destroyed,
                                   focus_object);
              g_object_set_qdata (G_OBJECT (focus_object), quark_focus_object, NULL);
              g_object_unref (G_OBJECT (focus_object));
            }
        }
    }
}

static void 
gail_set_focus_widget (CtkWidget *focus_widget,
                       CtkWidget *widget)
{
  AtkObject *focus_obj;
  AtkObject *obj;

  focus_obj = ctk_widget_get_accessible (focus_widget);
  obj = ctk_widget_get_accessible (widget);
  gail_set_focus_object (focus_obj, obj);
}

static void 
gail_set_focus_object (AtkObject *focus_obj,
                       AtkObject *obj)
{
  AtkObject *old_focus_obj;

  old_focus_obj = g_object_get_qdata (G_OBJECT (obj), quark_focus_object);
  if (old_focus_obj != obj)
    {
      if (old_focus_obj)
        g_object_weak_unref (G_OBJECT (old_focus_obj),
                             (GWeakNotify) gail_focus_object_destroyed,
                             obj);
      else
        /*
         * We call g_object_ref as if obj is destroyed 
         * while the weak reference exists then destroying the 
         * focus_obj would cause gail_focus_object_destroyed to be 
         * called when obj is not a valid GObject.
         */
        g_object_ref (obj);

      g_object_weak_ref (G_OBJECT (focus_obj),
                         (GWeakNotify) gail_focus_object_destroyed,
                         obj);
      g_object_set_qdata (G_OBJECT (obj), quark_focus_object, focus_obj);
    }
}

static gboolean
state_event_watcher (GSignalInvocationHint *hint,
                     guint                  n_param_values,
                     const GValue          *param_values,
                     gpointer               data)
{
  GObject *object;
  CtkWidget *widget;
  AtkObject *atk_obj;
  AtkObject *parent;
  CdkEventWindowState *event;
  gchar *signal_name;

  object = g_value_get_object (param_values + 0);
  if (!CTK_IS_WINDOW (object))
    return FALSE;

  event = g_value_get_boxed (param_values + 1);
  if (event->type == GDK_WINDOW_STATE)
    return FALSE;
  widget = CTK_WIDGET (object);

  if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
    signal_name = "maximize";
  else if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
    signal_name = "minimize";
  else if (event->new_window_state == 0)
    signal_name = "restore";
  else
    return TRUE;

  atk_obj = ctk_widget_get_accessible (widget);
  if (CTK_IS_WINDOW_ACCESSIBLE (atk_obj))
    {
      parent = atk_object_get_parent (atk_obj);
      if (parent == atk_get_root ())
        g_signal_emit_by_name (atk_obj, signal_name);

      return TRUE;
    }

  return FALSE;
}

static gboolean
configure_event_watcher (GSignalInvocationHint *hint,
                         guint                  n_param_values,
                         const GValue          *param_values,
                         gpointer               data)
{
  CtkAllocation allocation;
  GObject *object;
  CtkWidget *widget;
  AtkObject *atk_obj;
  AtkObject *parent;
  CdkEvent *event;
  gchar *signal_name;

  object = g_value_get_object (param_values + 0);
  if (!CTK_IS_WINDOW (object))
    return FALSE;

  event = g_value_get_boxed (param_values + 1);
  if (event->type != GDK_CONFIGURE)
    return FALSE;
  widget = CTK_WIDGET (object);
  ctk_widget_get_allocation (widget, &allocation);
  if (allocation.x == ((CdkEventConfigure *)event)->x &&
      allocation.y == ((CdkEventConfigure *)event)->y &&
      allocation.width == ((CdkEventConfigure *)event)->width &&
      allocation.height == ((CdkEventConfigure *)event)->height)
    return TRUE;

  if (allocation.width != ((CdkEventConfigure *)event)->width ||
      allocation.height != ((CdkEventConfigure *)event)->height)
    signal_name = "resize";
  else
    signal_name = "move";

  atk_obj = ctk_widget_get_accessible (widget);
  if (CTK_IS_WINDOW_ACCESSIBLE (atk_obj))
    {
      parent = atk_object_get_parent (atk_obj);
      if (parent == atk_get_root ())
        g_signal_emit_by_name (atk_obj, signal_name);

      return TRUE;
    }

  return FALSE;
}

static gboolean
window_focus (CtkWidget     *widget,
              CdkEventFocus *event)
{
  AtkObject *atk_obj;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  atk_obj = ctk_widget_get_accessible (widget);
  g_signal_emit_by_name (atk_obj, event->in ? "activate" : "deactivate");

  return FALSE;
}

static void
window_added (AtkObject *atk_obj,
              guint      index,
              AtkObject *child)
{
  CtkWidget *widget;

  if (!CTK_IS_WINDOW_ACCESSIBLE (child))
    return;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (child));
  if (!widget)
    return;

  g_signal_connect (widget, "focus-in-event", (GCallback) window_focus, NULL);
  g_signal_connect (widget, "focus-out-event", (GCallback) window_focus, NULL);
  g_signal_emit_by_name (child, "create");
}

static void
window_removed (AtkObject *atk_obj,
                guint      index,
                AtkObject *child)
{
  CtkWidget *widget;
  CtkWindow *window;

  if (!CTK_IS_WINDOW_ACCESSIBLE (child))
    return;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (child));
  if (!widget)
    return;

  window = CTK_WINDOW (widget);
  /*
   * Deactivate window if it is still focused and we are removing it. This
   * can happen when a dialog displayed by gok is removed.
   */
  if (ctk_window_is_active (window) && ctk_window_has_toplevel_focus (window))
    g_signal_emit_by_name (child, "deactivate");

  g_signal_handlers_disconnect_by_func (widget, (gpointer) window_focus, NULL);
  g_signal_emit_by_name (child, "destroy");
}

static void
do_window_event_initialization (void)
{
  AtkObject *root;

  g_type_class_ref (CTK_TYPE_WINDOW_ACCESSIBLE);
  g_signal_add_emission_hook (g_signal_lookup ("window-state-event", CTK_TYPE_WIDGET),
                              0, state_event_watcher, NULL, (GDestroyNotify) NULL);
  g_signal_add_emission_hook (g_signal_lookup ("configure-event", CTK_TYPE_WIDGET),
                              0, configure_event_watcher, NULL, (GDestroyNotify) NULL);

  root = atk_get_root ();
  g_signal_connect (root, "children-changed::add", (GCallback) window_added, NULL);
  g_signal_connect (root, "children-changed::remove", (GCallback) window_removed, NULL);
}

void
_ctk_accessibility_init (void)
{
  if (initialized)
    return;

  initialized = TRUE;
  quark_focus_object = g_quark_from_static_string ("gail-focus-object");

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  atk_focus_tracker_init (gail_focus_tracker_init);
  focus_tracker_id = atk_add_focus_tracker (gail_focus_tracker);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  _ctk_accessibility_override_atk_util ();
  do_window_event_initialization ();

#ifdef GDK_WINDOWING_X11
  atk_bridge_adaptor_init (NULL, NULL);
#endif

  atk_misc_instance = g_object_new (CTK_TYPE_MISC_IMPL, NULL);
}
