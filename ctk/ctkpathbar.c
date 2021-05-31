/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* ctkpathbar.c
 * Copyright (C) 2004  Red Hat, Inc.,  Jonathan Blandford <jrb@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkpathbar.h"

#include <string.h>

#include "ctkbox.h"
#include "ctkcssnodeprivate.h"
#include "ctkdnd.h"
#include "ctkdragsource.h"
#include "ctkicontheme.h"
#include "ctkimage.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctksettings.h"
#include "ctktogglebutton.h"
#include "ctkwidgetpath.h"
#include "ctkwidgetprivate.h"

struct _GtkPathBarPrivate
{
  GtkFileSystem *file_system;
  GFile *root_file;
  GFile *home_file;
  GFile *desktop_file;

  /* List of running GCancellable.  When we cancel one, we remove it from this list.
   * The pathbar cancels all outstanding cancellables when it is disposed.
   *
   * In code that queues async I/O operations:
   *
   *   - Obtain a cancellable from the async I/O APIs, and call add_cancellable().
   *
   * To cancel a cancellable:
   *
   *   - Call cancel_cancellable().
   *
   * In async I/O callbacks:
   *
   *   - Check right away if g_cancellable_is_cancelled():  if true, just
   *     g_object_unref() the cancellable and return early (also free your
   *     closure data if you have one).
   *
   *   - If it was not cancelled, call cancellable_async_done().  This will
   *     unref the cancellable and unqueue it from the pathbar's outstanding
   *     cancellables.  Do your normal work to process the async result and free
   *     your closure data if you have one.
   */
  GList *cancellables;

  GCancellable *get_info_cancellable;

  GIcon *root_icon;
  GIcon *home_icon;
  GIcon *desktop_icon;

  GdkWindow *event_window;

  GList *button_list;
  GList *first_scrolled_button;
  GList *fake_root;
  GtkWidget *up_slider_button;
  GtkWidget *down_slider_button;
  guint settings_signal_id;
  gint16 slider_width;
  gint16 button_offset;
  guint timer;
  guint slider_visible : 1;
  guint need_timer     : 1;
  guint ignore_click   : 1;
  guint scrolling_up   : 1;
  guint scrolling_down : 1;
};

enum {
  PATH_CLICKED,
  LAST_SIGNAL
};

typedef enum {
  NORMAL_BUTTON,
  ROOT_BUTTON,
  HOME_BUTTON,
  DESKTOP_BUTTON
} ButtonType;

#define BUTTON_DATA(x) ((ButtonData *)(x))

#define SCROLL_DELAY_FACTOR 5
#define TIMEOUT_INITIAL     500
#define TIMEOUT_REPEAT      50

static guint path_bar_signals [LAST_SIGNAL] = { 0 };

/* Icon size for if we can't get it from the theme */
#define FALLBACK_ICON_SIZE 16

typedef struct _ButtonData ButtonData;

struct _ButtonData
{
  GtkWidget *button;
  ButtonType type;
  char *dir_name;
  GFile *file;
  GtkWidget *image;
  GtkWidget *label;
  GCancellable *cancellable;
  guint ignore_changes : 1;
  guint file_is_hidden : 1;
};
/* This macro is used to check if a button can be used as a fake root.
 * All buttons in front of a fake root are automatically hidden when in a
 * directory below a fake root and replaced with the "<" arrow button.
 */
#define BUTTON_IS_FAKE_ROOT(button) ((button)->type == HOME_BUTTON)

G_DEFINE_TYPE_WITH_PRIVATE (GtkPathBar, ctk_path_bar, CTK_TYPE_CONTAINER)

static void ctk_path_bar_finalize                 (GObject          *object);
static void ctk_path_bar_dispose                  (GObject          *object);
static void ctk_path_bar_realize                  (GtkWidget        *widget);
static void ctk_path_bar_unrealize                (GtkWidget        *widget);
static void ctk_path_bar_get_preferred_width      (GtkWidget        *widget,
                                                   gint             *minimum,
                                                   gint             *natural);
static void ctk_path_bar_get_preferred_height     (GtkWidget        *widget,
                                                   gint             *minimum,
                                                   gint             *natural);
static void ctk_path_bar_map                      (GtkWidget        *widget);
static void ctk_path_bar_unmap                    (GtkWidget        *widget);
static void ctk_path_bar_size_allocate            (GtkWidget        *widget,
						   GtkAllocation    *allocation);
static void ctk_path_bar_add                      (GtkContainer     *container,
						   GtkWidget        *widget);
static void ctk_path_bar_remove                   (GtkContainer     *container,
						   GtkWidget        *widget);
static void ctk_path_bar_forall                   (GtkContainer     *container,
						   gboolean          include_internals,
						   GtkCallback       callback,
						   gpointer          callback_data);
static gboolean ctk_path_bar_scroll               (GtkWidget        *widget,
						   GdkEventScroll   *event);
static void ctk_path_bar_scroll_up                (GtkPathBar       *path_bar);
static void ctk_path_bar_scroll_down              (GtkPathBar       *path_bar);
static void ctk_path_bar_stop_scrolling           (GtkPathBar       *path_bar);
static gboolean ctk_path_bar_slider_up_defocus    (GtkWidget        *widget,
						   GdkEventButton   *event,
						   GtkPathBar       *path_bar);
static gboolean ctk_path_bar_slider_down_defocus  (GtkWidget        *widget,
						   GdkEventButton   *event,
						   GtkPathBar       *path_bar);
static gboolean ctk_path_bar_slider_button_press  (GtkWidget        *widget,
						   GdkEventButton   *event,
						   GtkPathBar       *path_bar);
static gboolean ctk_path_bar_slider_button_release(GtkWidget        *widget,
						   GdkEventButton   *event,
						   GtkPathBar       *path_bar);
static void ctk_path_bar_grab_notify              (GtkWidget        *widget,
						   gboolean          was_grabbed);
static void ctk_path_bar_state_changed            (GtkWidget        *widget,
						   GtkStateType      previous_state);
static void ctk_path_bar_style_updated            (GtkWidget        *widget);
static void ctk_path_bar_screen_changed           (GtkWidget        *widget,
						   GdkScreen        *previous_screen);
static void ctk_path_bar_check_icon_theme         (GtkPathBar       *path_bar);
static void ctk_path_bar_update_button_appearance (GtkPathBar       *path_bar,
						   ButtonData       *button_data,
						   gboolean          current_dir);

static void
add_cancellable (GtkPathBar   *path_bar,
		 GCancellable *cancellable)
{
  g_assert (g_list_find (path_bar->priv->cancellables, cancellable) == NULL);
  path_bar->priv->cancellables = g_list_prepend (path_bar->priv->cancellables, cancellable);
}

static void
drop_node_for_cancellable (GtkPathBar *path_bar,
			   GCancellable *cancellable)
{
  GList *node;

  node = g_list_find (path_bar->priv->cancellables, cancellable);
  g_assert (node != NULL);
  node->data = NULL;
  path_bar->priv->cancellables = g_list_delete_link (path_bar->priv->cancellables, node);
}

static void
cancel_cancellable (GtkPathBar   *path_bar,
		    GCancellable *cancellable)
{
  drop_node_for_cancellable (path_bar, cancellable);
  g_cancellable_cancel (cancellable);
}

static void
cancellable_async_done (GtkPathBar   *path_bar,
			GCancellable *cancellable)
{
  drop_node_for_cancellable (path_bar, cancellable);
  g_object_unref (cancellable);
}

static void
cancel_all_cancellables (GtkPathBar *path_bar)
{
  while (path_bar->priv->cancellables)
    {
      GCancellable *cancellable = path_bar->priv->cancellables->data;
      cancel_cancellable (path_bar, cancellable);
    }
}

static void
on_slider_unmap (GtkWidget  *widget,
		 GtkPathBar *path_bar)
{
  if (path_bar->priv->timer &&
      ((widget == path_bar->priv->up_slider_button && path_bar->priv->scrolling_up) ||
       (widget == path_bar->priv->down_slider_button && path_bar->priv->scrolling_down)))
    ctk_path_bar_stop_scrolling (path_bar);
}

static void
ctk_path_bar_init (GtkPathBar *path_bar)
{
  GtkStyleContext *context;

  path_bar->priv = ctk_path_bar_get_instance_private (path_bar);

  ctk_widget_init_template (CTK_WIDGET (path_bar));

  /* Add the children manually because GtkPathBar derives from an abstract class,
   * Glade cannot edit a <template> in ctkpathbar.ui if it's only a GtkContainer.
   */
  ctk_container_add (CTK_CONTAINER (path_bar), path_bar->priv->up_slider_button);
  ctk_container_add (CTK_CONTAINER (path_bar), path_bar->priv->down_slider_button);

  /* GtkBuilder wont let us connect 'swapped' without specifying the signal's
   * user data in the .ui file
   */
  g_signal_connect_swapped (path_bar->priv->up_slider_button, "clicked",
			    G_CALLBACK (ctk_path_bar_scroll_up), path_bar);
  g_signal_connect_swapped (path_bar->priv->down_slider_button, "clicked",
			    G_CALLBACK (ctk_path_bar_scroll_down), path_bar);

  ctk_widget_set_has_window (CTK_WIDGET (path_bar), FALSE);

  context = ctk_widget_get_style_context (CTK_WIDGET (path_bar));
  ctk_style_context_add_class (context, "path-bar");
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_LINKED);

  path_bar->priv->get_info_cancellable = NULL;
  path_bar->priv->cancellables = NULL;
}

static void
ctk_path_bar_class_init (GtkPathBarClass *path_bar_class)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  gobject_class = (GObjectClass *) path_bar_class;
  widget_class = (GtkWidgetClass *) path_bar_class;
  container_class = (GtkContainerClass *) path_bar_class;

  gobject_class->finalize = ctk_path_bar_finalize;
  gobject_class->dispose = ctk_path_bar_dispose;

  widget_class->get_preferred_width = ctk_path_bar_get_preferred_width;
  widget_class->get_preferred_height = ctk_path_bar_get_preferred_height;
  widget_class->realize = ctk_path_bar_realize;
  widget_class->unrealize = ctk_path_bar_unrealize;
  widget_class->map = ctk_path_bar_map;
  widget_class->unmap = ctk_path_bar_unmap;
  widget_class->size_allocate = ctk_path_bar_size_allocate;
  widget_class->style_updated = ctk_path_bar_style_updated;
  widget_class->screen_changed = ctk_path_bar_screen_changed;
  widget_class->grab_notify = ctk_path_bar_grab_notify;
  widget_class->state_changed = ctk_path_bar_state_changed;
  widget_class->scroll_event = ctk_path_bar_scroll;

  container_class->add = ctk_path_bar_add;
  container_class->forall = ctk_path_bar_forall;
  container_class->remove = ctk_path_bar_remove;
  ctk_container_class_handle_border_width (container_class);
  /* FIXME: */
  /*  container_class->child_type = ctk_path_bar_child_type;*/

  path_bar_signals [PATH_CLICKED] =
    g_signal_new (I_("path-clicked"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkPathBarClass, path_clicked),
		  NULL, NULL,
		  _ctk_marshal_VOID__POINTER_POINTER_BOOLEAN,
		  G_TYPE_NONE, 3,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER,
		  G_TYPE_BOOLEAN);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkpathbar.ui");

  ctk_widget_class_bind_template_child_private (widget_class, GtkPathBar, up_slider_button);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPathBar, down_slider_button);

  ctk_widget_class_bind_template_callback (widget_class, ctk_path_bar_slider_button_press);
  ctk_widget_class_bind_template_callback (widget_class, ctk_path_bar_slider_button_release);
  ctk_widget_class_bind_template_callback (widget_class, ctk_path_bar_slider_up_defocus);
  ctk_widget_class_bind_template_callback (widget_class, ctk_path_bar_slider_down_defocus);
  ctk_widget_class_bind_template_callback (widget_class, ctk_path_bar_scroll_up);
  ctk_widget_class_bind_template_callback (widget_class, ctk_path_bar_scroll_down);
  ctk_widget_class_bind_template_callback (widget_class, on_slider_unmap);
}


static void
ctk_path_bar_finalize (GObject *object)
{
  GtkPathBar *path_bar;

  path_bar = CTK_PATH_BAR (object);

  cancel_all_cancellables (path_bar);
  ctk_path_bar_stop_scrolling (path_bar);

  g_list_free (path_bar->priv->button_list);
  g_clear_object (&path_bar->priv->root_file);
  g_clear_object (&path_bar->priv->home_file);
  g_clear_object (&path_bar->priv->desktop_file);

  g_clear_object (&path_bar->priv->root_icon);
  g_clear_object (&path_bar->priv->home_icon);
  g_clear_object (&path_bar->priv->desktop_icon);

  g_clear_object (&path_bar->priv->file_system);

  G_OBJECT_CLASS (ctk_path_bar_parent_class)->finalize (object);
}

/* Removes the settings signal handler.  It's safe to call multiple times */
static void
remove_settings_signal (GtkPathBar *path_bar,
			GdkScreen  *screen)
{
  if (path_bar->priv->settings_signal_id)
    {
      GtkSettings *settings;

      settings = ctk_settings_get_for_screen (screen);
      g_signal_handler_disconnect (settings,
				   path_bar->priv->settings_signal_id);
      path_bar->priv->settings_signal_id = 0;
    }
}

static void
ctk_path_bar_dispose (GObject *object)
{
  GtkPathBar *path_bar = CTK_PATH_BAR (object);

  remove_settings_signal (path_bar, ctk_widget_get_screen (CTK_WIDGET (object)));

  path_bar->priv->get_info_cancellable = NULL;
  cancel_all_cancellables (path_bar);

  G_OBJECT_CLASS (ctk_path_bar_parent_class)->dispose (object);
}

/* Size requisition:
 * 
 * Ideally, our size is determined by another widget, and we are just filling
 * available space.
 */
static void
ctk_path_bar_get_preferred_width (GtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  ButtonData *button_data;
  GtkPathBar *path_bar;
  GList *list;
  gint child_height;
  gint height;
  gint child_min, child_nat;

  path_bar = CTK_PATH_BAR (widget);

  *minimum = *natural = 0;
  height = 0;

  for (list = path_bar->priv->button_list; list; list = list->next)
    {
      button_data = BUTTON_DATA (list->data);
      ctk_widget_get_preferred_width (button_data->button, &child_min, &child_nat);
      ctk_widget_get_preferred_height (button_data->button, &child_height, NULL);
      height = MAX (height, child_height);

      if (button_data->type == NORMAL_BUTTON)
        {
          /* Use 2*Height as button width because of ellipsized label.  */
          child_min = MAX (child_min, child_height * 2);
          child_nat = MAX (child_min, child_height * 2);
        }

      *minimum = MAX (*minimum, child_min);
      *natural = *natural + child_nat;
    }

  /* Add space for slider, if we have more than one path */
  /* Theoretically, the slider could be bigger than the other button.  But we're
   * not going to worry about that now.
   */
  path_bar->priv->slider_width = 0;

  ctk_widget_get_preferred_width (path_bar->priv->up_slider_button, &child_min, &child_nat);
  if (path_bar->priv->button_list && path_bar->priv->button_list->next != NULL)
    {
      *minimum += child_min;
      *natural += child_nat;
    }
  path_bar->priv->slider_width = MAX (path_bar->priv->slider_width, child_min);

  ctk_widget_get_preferred_width (path_bar->priv->down_slider_button, &child_min, &child_nat);
  if (path_bar->priv->button_list && path_bar->priv->button_list->next != NULL)
    {
      *minimum += child_min;
      *natural += child_nat;
    }
  path_bar->priv->slider_width = MAX (path_bar->priv->slider_width, child_min);
}

static void
ctk_path_bar_get_preferred_height (GtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  ButtonData *button_data;
  GtkPathBar *path_bar;
  GList *list;
  gint child_min, child_nat;

  path_bar = CTK_PATH_BAR (widget);

  *minimum = *natural = 0;

  for (list = path_bar->priv->button_list; list; list = list->next)
    {
      button_data = BUTTON_DATA (list->data);
      ctk_widget_get_preferred_height (button_data->button, &child_min, &child_nat);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }

  ctk_widget_get_preferred_height (path_bar->priv->up_slider_button, &child_min, &child_nat);
  *minimum = MAX (*minimum, child_min);
  *natural = MAX (*natural, child_nat);

  ctk_widget_get_preferred_height (path_bar->priv->down_slider_button, &child_min, &child_nat);
  *minimum = MAX (*minimum, child_min);
  *natural = MAX (*natural, child_nat);
}

static void
ctk_path_bar_update_slider_buttons (GtkPathBar *path_bar)
{
  if (path_bar->priv->button_list)
    {
      GtkWidget *button;

      button = BUTTON_DATA (path_bar->priv->button_list->data)->button;
      if (ctk_widget_get_child_visible (button))
	{
	  ctk_path_bar_stop_scrolling (path_bar);
	  ctk_widget_set_sensitive (path_bar->priv->down_slider_button, FALSE);
	}
      else
	ctk_widget_set_sensitive (path_bar->priv->down_slider_button, TRUE);

      button = BUTTON_DATA (g_list_last (path_bar->priv->button_list)->data)->button;
      if (ctk_widget_get_child_visible (button))
	{
	  ctk_path_bar_stop_scrolling (path_bar);
	  ctk_widget_set_sensitive (path_bar->priv->up_slider_button, FALSE);
	}
      else
	ctk_widget_set_sensitive (path_bar->priv->up_slider_button, TRUE);
    }
}

static void
ctk_path_bar_map (GtkWidget *widget)
{
  gdk_window_show (CTK_PATH_BAR (widget)->priv->event_window);

  CTK_WIDGET_CLASS (ctk_path_bar_parent_class)->map (widget);
}

static void
ctk_path_bar_unmap (GtkWidget *widget)
{
  ctk_path_bar_stop_scrolling (CTK_PATH_BAR (widget));
  gdk_window_hide (CTK_PATH_BAR (widget)->priv->event_window);

  CTK_WIDGET_CLASS (ctk_path_bar_parent_class)->unmap (widget);
}

static void
ctk_path_bar_realize (GtkWidget *widget)
{
  GtkPathBar *path_bar;
  GtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  path_bar = CTK_PATH_BAR (widget);
  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= GDK_SCROLL_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  path_bar->priv->event_window = gdk_window_new (ctk_widget_get_parent_window (widget),
                                           &attributes, attributes_mask);
  ctk_widget_register_window (widget, path_bar->priv->event_window);
}

static void
ctk_path_bar_unrealize (GtkWidget *widget)
{
  GtkPathBar *path_bar;

  path_bar = CTK_PATH_BAR (widget);

  ctk_widget_unregister_window (widget, path_bar->priv->event_window);
  gdk_window_destroy (path_bar->priv->event_window);
  path_bar->priv->event_window = NULL;

  CTK_WIDGET_CLASS (ctk_path_bar_parent_class)->unrealize (widget);
}

/* This is a tad complicated
 */
static void
ctk_path_bar_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GtkWidget *child;
  GtkPathBar *path_bar = CTK_PATH_BAR (widget);
  GtkTextDirection direction;
  GtkAllocation child_allocation;
  GList *list, *first_button;
  gint width;
  gint allocation_width;
  gboolean need_sliders = TRUE;
  gint up_slider_offset = 0;
  gint down_slider_offset = 0;
  GtkRequisition child_requisition;

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    gdk_window_move_resize (path_bar->priv->event_window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  /* No path is set; we don't have to allocate anything. */
  if (path_bar->priv->button_list == NULL)
    {
      _ctk_widget_set_simple_clip (widget, NULL);

      return;
    }

  direction = ctk_widget_get_direction (widget);
  allocation_width = allocation->width;

  /* First, we check to see if we need the scrollbars. */
  if (path_bar->priv->fake_root)
    width = path_bar->priv->slider_width;
  else
    width = 0;

  for (list = path_bar->priv->button_list; list; list = list->next)
    {
      child = BUTTON_DATA (list->data)->button;

      ctk_widget_get_preferred_size (child, &child_requisition, NULL);

      width += child_requisition.width;
      if (list == path_bar->priv->fake_root)
	break;
    }

  if (width <= allocation_width)
    {
      if (path_bar->priv->fake_root)
	first_button = path_bar->priv->fake_root;
      else
	first_button = g_list_last (path_bar->priv->button_list);
    }
  else
    {
      gboolean reached_end = FALSE;
      gint slider_space = 2 * path_bar->priv->slider_width;

      if (path_bar->priv->first_scrolled_button)
	first_button = path_bar->priv->first_scrolled_button;
      else
	first_button = path_bar->priv->button_list;
      need_sliders = TRUE;

      /* To see how much space we have, and how many buttons we can display.
       * We start at the first button, count forward until hit the new
       * button, then count backwards.
       */
      /* Count down the path chain towards the end. */
      ctk_widget_get_preferred_size (BUTTON_DATA (first_button->data)->button,
                                     &child_requisition, NULL);

      width = child_requisition.width;
      list = first_button->prev;
      while (list && !reached_end)
	{
	  child = BUTTON_DATA (list->data)->button;

          ctk_widget_get_preferred_size (child, &child_requisition, NULL);

	  if (width + child_requisition.width + slider_space > allocation_width)
	    reached_end = TRUE;
	  else if (list == path_bar->priv->fake_root)
	    break;
	  else
	    width += child_requisition.width;

	  list = list->prev;
	}

      /* Finally, we walk up, seeing how many of the previous buttons we can
       * add */
      while (first_button->next && !reached_end)
	{
	  child = BUTTON_DATA (first_button->next->data)->button;

          ctk_widget_get_preferred_size (child, &child_requisition, NULL);

	  if (width + child_requisition.width + slider_space > allocation_width)
	    {
	      reached_end = TRUE;
	    }
	  else
	    {
	      width += child_requisition.width;
	      if (first_button == path_bar->priv->fake_root)
		break;
	      first_button = first_button->next;
	    }
	}
    }

  /* Now, we allocate space to the buttons */
  child_allocation.y = allocation->y;
  child_allocation.height = allocation->height;

  if (direction == CTK_TEXT_DIR_RTL)
    {
      child_allocation.x = allocation->x + allocation->width;
      if (need_sliders || path_bar->priv->fake_root)
	{
	  child_allocation.x -= path_bar->priv->slider_width;
	  up_slider_offset = allocation->width - path_bar->priv->slider_width;
	}
    }
  else
    {
      child_allocation.x = allocation->x;
      if (need_sliders || path_bar->priv->fake_root)
	{
	  up_slider_offset = 0;
	  child_allocation.x += path_bar->priv->slider_width;
	}
    }

  for (list = first_button; list; list = list->prev)
    {
      GtkAllocation widget_allocation;
      ButtonData *button_data;

      button_data = BUTTON_DATA (list->data);
      child = button_data->button;

      ctk_widget_get_preferred_size (child, &child_requisition, NULL);

      child_allocation.width = MIN (child_requisition.width,
				    allocation_width - 2 * path_bar->priv->slider_width);

      if (direction == CTK_TEXT_DIR_RTL)
	child_allocation.x -= child_allocation.width;

      /* Check to see if we've don't have any more space to allocate buttons */
      if (need_sliders && direction == CTK_TEXT_DIR_RTL)
	{
          ctk_widget_get_allocation (widget, &widget_allocation);
	  if (child_allocation.x - path_bar->priv->slider_width < widget_allocation.x)
	    break;
	}
      else if (need_sliders && direction == CTK_TEXT_DIR_LTR)
	{
          ctk_widget_get_allocation (widget, &widget_allocation);
	  if (child_allocation.x + child_allocation.width + path_bar->priv->slider_width >
	      widget_allocation.x + allocation_width)
	    break;
	}

      if (child_allocation.width < child_requisition.width)
	{
	  if (!ctk_widget_get_has_tooltip (child))
	    ctk_widget_set_tooltip_text (child, button_data->dir_name);
	}
      else if (ctk_widget_get_has_tooltip (child))
	ctk_widget_set_tooltip_text (child, NULL);
      
      ctk_widget_set_child_visible (child, TRUE);
      ctk_widget_size_allocate (child, &child_allocation);

      if (direction == CTK_TEXT_DIR_RTL)
        {
          down_slider_offset = child_allocation.x - allocation->x - path_bar->priv->slider_width;
        }
      else
        {
          down_slider_offset += child_allocation.width;
          child_allocation.x += child_allocation.width;
        }
    }
  /* Now we go hide all the widgets that don't fit */
  while (list)
    {
      child = BUTTON_DATA (list->data)->button;
      ctk_widget_set_child_visible (child, FALSE);
      list = list->prev;
    }
  for (list = first_button->next; list; list = list->next)
    {
      child = BUTTON_DATA (list->data)->button;
      ctk_widget_set_child_visible (child, FALSE);
    }

  if (need_sliders || path_bar->priv->fake_root)
    {
      child_allocation.width = path_bar->priv->slider_width;
      child_allocation.x = up_slider_offset + allocation->x;
      ctk_widget_size_allocate (path_bar->priv->up_slider_button, &child_allocation);

      ctk_widget_set_child_visible (path_bar->priv->up_slider_button, TRUE);
      ctk_widget_show_all (path_bar->priv->up_slider_button);

      if (direction == CTK_TEXT_DIR_LTR)
        down_slider_offset += path_bar->priv->slider_width;
    }
  else
    {
      ctk_widget_set_child_visible (path_bar->priv->up_slider_button, FALSE);
    }

  if (need_sliders)
    {
      child_allocation.width = path_bar->priv->slider_width;
      child_allocation.x = down_slider_offset + allocation->x;
      
      ctk_widget_size_allocate (path_bar->priv->down_slider_button, &child_allocation);

      ctk_widget_set_child_visible (path_bar->priv->down_slider_button, TRUE);
      ctk_widget_show_all (path_bar->priv->down_slider_button);
      ctk_path_bar_update_slider_buttons (path_bar);
    }
  else
    {
      ctk_widget_set_child_visible (path_bar->priv->down_slider_button, FALSE);
    }

  _ctk_widget_set_simple_clip (widget, NULL);
}

static void
ctk_path_bar_style_updated (GtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_path_bar_parent_class)->style_updated (widget);

  ctk_path_bar_check_icon_theme (CTK_PATH_BAR (widget));
}

static void
ctk_path_bar_screen_changed (GtkWidget *widget,
			     GdkScreen *previous_screen)
{
  if (CTK_WIDGET_CLASS (ctk_path_bar_parent_class)->screen_changed)
    CTK_WIDGET_CLASS (ctk_path_bar_parent_class)->screen_changed (widget, previous_screen);

  /* We might nave a new settings, so we remove the old one */
  if (previous_screen)
    remove_settings_signal (CTK_PATH_BAR (widget), previous_screen);

  ctk_path_bar_check_icon_theme (CTK_PATH_BAR (widget));
}

static gboolean
ctk_path_bar_scroll (GtkWidget      *widget,
		     GdkEventScroll *event)
{
  switch (event->direction)
    {
    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      ctk_path_bar_scroll_down (CTK_PATH_BAR (widget));
      break;
    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      ctk_path_bar_scroll_up (CTK_PATH_BAR (widget));
      break;
    case GDK_SCROLL_SMOOTH:
      break;
    }

  return TRUE;
}

static void
ctk_path_bar_add (GtkContainer *container,
		  GtkWidget    *widget)

{
  ctk_widget_set_parent (widget, CTK_WIDGET (container));
}

static void
ctk_path_bar_remove_1 (GtkContainer *container,
		       GtkWidget    *widget)
{
  gboolean was_visible = ctk_widget_get_visible (widget);
  ctk_widget_unparent (widget);
  if (was_visible)
    ctk_widget_queue_resize (CTK_WIDGET (container));
}

static void
ctk_path_bar_remove (GtkContainer *container,
		     GtkWidget    *widget)
{
  GtkPathBar *path_bar;
  GList *children;

  path_bar = CTK_PATH_BAR (container);

  if (widget == path_bar->priv->up_slider_button)
    {
      ctk_path_bar_remove_1 (container, widget);
      path_bar->priv->up_slider_button = NULL;
      return;
    }

  if (widget == path_bar->priv->down_slider_button)
    {
      ctk_path_bar_remove_1 (container, widget);
      path_bar->priv->down_slider_button = NULL;
      return;
    }

  children = path_bar->priv->button_list;
  while (children)
    {
      if (widget == BUTTON_DATA (children->data)->button)
	{
	  ctk_path_bar_remove_1 (container, widget);
	  path_bar->priv->button_list = g_list_remove_link (path_bar->priv->button_list, children);
	  g_list_free (children);
	  return;
	}
      
      children = children->next;
    }
}

static void
ctk_path_bar_forall (GtkContainer *container,
		     gboolean      include_internals,
		     GtkCallback   callback,
		     gpointer      callback_data)
{
  GtkPathBar *path_bar;
  GList *children;

  g_return_if_fail (callback != NULL);
  path_bar = CTK_PATH_BAR (container);

  children = path_bar->priv->button_list;
  while (children)
    {
      GtkWidget *child;
      child = BUTTON_DATA (children->data)->button;
      children = children->next;

      (* callback) (child, callback_data);
    }

  if (path_bar->priv->up_slider_button)
    (* callback) (path_bar->priv->up_slider_button, callback_data);

  if (path_bar->priv->down_slider_button)
    (* callback) (path_bar->priv->down_slider_button, callback_data);
}

static void
ctk_path_bar_scroll_down (GtkPathBar *path_bar)
{
  GtkAllocation allocation, button_allocation;
  GList *list;
  GList *down_button = NULL;
  gint space_available;

  if (path_bar->priv->ignore_click)
    {
      path_bar->priv->ignore_click = FALSE;
      return;   
    }

  if (ctk_widget_get_child_visible (BUTTON_DATA (path_bar->priv->button_list->data)->button))
    {
      /* Return if the last button is already visible */
      return;
    }

  ctk_widget_queue_resize (CTK_WIDGET (path_bar));

  /* We find the button at the 'down' end that we have to make
   * visible */
  for (list = path_bar->priv->button_list; list; list = list->next)
    {
      if (list->next && ctk_widget_get_child_visible (BUTTON_DATA (list->next->data)->button))
	{
	  down_button = list;
	  break;
	}
    }

  ctk_widget_get_allocation (CTK_WIDGET (path_bar), &allocation);
  ctk_widget_get_allocation (BUTTON_DATA (down_button->data)->button, &button_allocation);

  space_available = (allocation.width
		     - 2 * path_bar->priv->slider_width
                     - button_allocation.width);
  path_bar->priv->first_scrolled_button = down_button;
  
  /* We have space_available free space that's not being used.  
   * So we walk down from the end, adding buttons until we use all free space.
   */
  while (space_available > 0)
    {
      path_bar->priv->first_scrolled_button = down_button;
      down_button = down_button->next;
      if (!down_button)
	break;
      space_available -= button_allocation.width;
    }
}

static void
ctk_path_bar_scroll_up (GtkPathBar *path_bar)
{
  GList *list;

  if (path_bar->priv->ignore_click)
    {
      path_bar->priv->ignore_click = FALSE;
      return;   
    }

  list = g_list_last (path_bar->priv->button_list);

  if (ctk_widget_get_child_visible (BUTTON_DATA (list->data)->button))
    {
      /* Return if the first button is already visible */
      return;
    }

  ctk_widget_queue_resize (CTK_WIDGET (path_bar));

  for ( ; list; list = list->prev)
    {
      if (list->prev && ctk_widget_get_child_visible (BUTTON_DATA (list->prev->data)->button))
	{
	  if (list->prev == path_bar->priv->fake_root)
	    path_bar->priv->fake_root = NULL;
	  path_bar->priv->first_scrolled_button = list;
	  return;
	}
    }
}

static gboolean
ctk_path_bar_scroll_timeout (GtkPathBar *path_bar)
{
  gboolean retval = FALSE;

  if (path_bar->priv->timer)
    {
      if (path_bar->priv->scrolling_up)
	ctk_path_bar_scroll_up (path_bar);
      else if (path_bar->priv->scrolling_down)
	ctk_path_bar_scroll_down (path_bar);

      if (path_bar->priv->need_timer) 
	{
	  path_bar->priv->need_timer = FALSE;

	  path_bar->priv->timer = gdk_threads_add_timeout (TIMEOUT_REPEAT * SCROLL_DELAY_FACTOR,
					   (GSourceFunc)ctk_path_bar_scroll_timeout,
					   path_bar);
          g_source_set_name_by_id (path_bar->priv->timer, "[ctk+] ctk_path_bar_scroll_timeout");
	}
      else
	retval = TRUE;
    }

  return retval;
}

static void 
ctk_path_bar_stop_scrolling (GtkPathBar *path_bar)
{
  if (path_bar->priv->timer)
    {
      g_source_remove (path_bar->priv->timer);
      path_bar->priv->timer = 0;
      path_bar->priv->need_timer = FALSE;
    }
}

static gboolean
ctk_path_bar_slider_up_defocus (GtkWidget      *widget,
                                    GdkEventButton *event,
                                    GtkPathBar     *path_bar)
{
  GList *list;
  GList *up_button = NULL;

  if (event->type != GDK_FOCUS_CHANGE)
    return FALSE;

  for (list = g_list_last (path_bar->priv->button_list); list; list = list->prev)
    {
      if (ctk_widget_get_child_visible (BUTTON_DATA (list->data)->button))
        {
          up_button = list;
          break;
        }
    }

  /* don't let the focus vanish */
  if ((!ctk_widget_is_sensitive (path_bar->priv->up_slider_button)) ||
      (!ctk_widget_get_child_visible (path_bar->priv->up_slider_button)))
    ctk_widget_grab_focus (BUTTON_DATA (up_button->data)->button);

  return FALSE;
}

static gboolean
ctk_path_bar_slider_down_defocus (GtkWidget      *widget,
                                    GdkEventButton *event,
                                    GtkPathBar     *path_bar)
{
  GList *list;
  GList *down_button = NULL;

  if (event->type != GDK_FOCUS_CHANGE)
    return FALSE;

  for (list = path_bar->priv->button_list; list; list = list->next)
    {
      if (ctk_widget_get_child_visible (BUTTON_DATA (list->data)->button))
        {
          down_button = list;
          break;
        }
    }

  /* don't let the focus vanish */
  if ((!ctk_widget_is_sensitive (path_bar->priv->down_slider_button)) ||
      (!ctk_widget_get_child_visible (path_bar->priv->down_slider_button)))
    ctk_widget_grab_focus (BUTTON_DATA (down_button->data)->button);

  return FALSE;
}

static gboolean
ctk_path_bar_slider_button_press (GtkWidget      *widget, 
				  GdkEventButton *event,
				  GtkPathBar     *path_bar)
{
  if (event->type != GDK_BUTTON_PRESS || event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  path_bar->priv->ignore_click = FALSE;

  if (widget == path_bar->priv->up_slider_button)
    {
      path_bar->priv->scrolling_down = FALSE;
      path_bar->priv->scrolling_up = TRUE;
      ctk_path_bar_scroll_up (path_bar);
    }
  else if (widget == path_bar->priv->down_slider_button)
    {
      path_bar->priv->scrolling_up = FALSE;
      path_bar->priv->scrolling_down = TRUE;
      ctk_path_bar_scroll_down (path_bar);
    }

  if (!path_bar->priv->timer)
    {
      path_bar->priv->need_timer = TRUE;
      path_bar->priv->timer = gdk_threads_add_timeout (TIMEOUT_INITIAL,
				       (GSourceFunc)ctk_path_bar_scroll_timeout,
				       path_bar);
      g_source_set_name_by_id (path_bar->priv->timer, "[ctk+] ctk_path_bar_scroll_timeout");
    }

  return FALSE;
}

static gboolean
ctk_path_bar_slider_button_release (GtkWidget      *widget, 
				    GdkEventButton *event,
				    GtkPathBar     *path_bar)
{
  if (event->type != GDK_BUTTON_RELEASE)
    return FALSE;

  path_bar->priv->ignore_click = TRUE;
  ctk_path_bar_stop_scrolling (path_bar);

  return FALSE;
}

static void
ctk_path_bar_grab_notify (GtkWidget *widget,
			  gboolean   was_grabbed)
{
  if (!was_grabbed)
    ctk_path_bar_stop_scrolling (CTK_PATH_BAR (widget));
}

static void
ctk_path_bar_state_changed (GtkWidget    *widget,
			    GtkStateType  previous_state)
{
  if (!ctk_widget_is_sensitive (widget))
    ctk_path_bar_stop_scrolling (CTK_PATH_BAR (widget));
}


/* Changes the icons wherever it is needed */
static void
reload_icons (GtkPathBar *path_bar)
{
  GList *list;

  g_clear_object (&path_bar->priv->root_icon);
  g_clear_object (&path_bar->priv->home_icon);
  g_clear_object (&path_bar->priv->desktop_icon);

  for (list = path_bar->priv->button_list; list; list = list->next)
    {
      ButtonData *button_data;
      gboolean current_dir;

      button_data = BUTTON_DATA (list->data);
      if (button_data->type != NORMAL_BUTTON)
	{
	  current_dir = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button_data->button));
	  ctk_path_bar_update_button_appearance (path_bar, button_data, current_dir);
	}
    }
}

static void
change_icon_theme (GtkPathBar *path_bar)
{
  reload_icons (path_bar);
}

/* Callback used when a GtkSettings value changes */
static void
settings_notify_cb (GObject    *object,
		    GParamSpec *pspec,
		    GtkPathBar *path_bar)
{
  const char *name;

  name = g_param_spec_get_name (pspec);

  if (strcmp (name, "ctk-icon-theme-name") == 0)
    change_icon_theme (path_bar);
}

static void
ctk_path_bar_check_icon_theme (GtkPathBar *path_bar)
{
  if (path_bar->priv->settings_signal_id == 0)
    {
      GtkSettings *settings;

      settings = ctk_settings_get_for_screen (ctk_widget_get_screen (CTK_WIDGET (path_bar)));
      path_bar->priv->settings_signal_id = g_signal_connect (settings, "notify",
                                                             G_CALLBACK (settings_notify_cb), path_bar);
    }

  change_icon_theme (path_bar);
}

/* Public functions and their helpers */
static void
ctk_path_bar_clear_buttons (GtkPathBar *path_bar)
{
  while (path_bar->priv->button_list != NULL)
    {
      ctk_container_remove (CTK_CONTAINER (path_bar), BUTTON_DATA (path_bar->priv->button_list->data)->button);
    }
  path_bar->priv->first_scrolled_button = NULL;
  path_bar->priv->fake_root = NULL;
}

static void
button_clicked_cb (GtkWidget *button,
		   gpointer   data)
{
  ButtonData *button_data;
  GtkPathBar *path_bar;
  GList *button_list;
  gboolean child_is_hidden;
  GFile *child_file;

  button_data = BUTTON_DATA (data);
  if (button_data->ignore_changes)
    return;

  path_bar = CTK_PATH_BAR (ctk_widget_get_parent (button));

  button_list = g_list_find (path_bar->priv->button_list, button_data);
  g_assert (button_list != NULL);

  g_signal_handlers_block_by_func (button,
				   G_CALLBACK (button_clicked_cb), data);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_handlers_unblock_by_func (button,
				     G_CALLBACK (button_clicked_cb), data);

  if (button_list->prev)
    {
      ButtonData *child_data;

      child_data = BUTTON_DATA (button_list->prev->data);
      child_file = child_data->file;
      child_is_hidden = child_data->file_is_hidden;
    }
  else
    {
      child_file = NULL;
      child_is_hidden = FALSE;
    }

  g_signal_emit (path_bar, path_bar_signals [PATH_CLICKED], 0,
		 button_data->file, child_file, child_is_hidden);
}

struct SetButtonImageData
{
  GtkPathBar *path_bar;
  ButtonData *button_data;
};

static void
set_button_image_get_info_cb (GCancellable *cancellable,
			      GFileInfo    *info,
			      const GError *error,
			      gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  GIcon *icon;
  struct SetButtonImageData *data = user_data;

  if (cancelled)
    {
      g_free (data);
      g_object_unref (cancellable);
      return;
    }

  g_assert (CTK_IS_PATH_BAR (data->path_bar));
  g_assert (G_OBJECT (data->path_bar)->ref_count > 0);

  g_assert (cancellable == data->button_data->cancellable);
  cancellable_async_done (data->path_bar, cancellable);
  data->button_data->cancellable = NULL;

  if (error)
    goto out;

  icon = g_file_info_get_symbolic_icon (info);
  ctk_image_set_from_gicon (CTK_IMAGE (data->button_data->image), icon, CTK_ICON_SIZE_BUTTON);

  switch (data->button_data->type)
    {
      case HOME_BUTTON:
        g_set_object (&data->path_bar->priv->home_icon, icon);
	break;

      case DESKTOP_BUTTON:
        g_set_object (&data->path_bar->priv->desktop_icon, icon);
	break;

      default:
	break;
    };

out:
  g_free (data);
}

static void
set_button_image (GtkPathBar *path_bar,
		  ButtonData *button_data)
{
  GtkFileSystemVolume *volume;
  struct SetButtonImageData *data;

  switch (button_data->type)
    {
    case ROOT_BUTTON:

      if (path_bar->priv->root_icon != NULL)
        {
          ctk_image_set_from_gicon (CTK_IMAGE (button_data->image), path_bar->priv->root_icon, CTK_ICON_SIZE_BUTTON);
	  break;
	}

      volume = _ctk_file_system_get_volume_for_file (path_bar->priv->file_system, path_bar->priv->root_file);
      if (volume == NULL)
	return;

      path_bar->priv->root_icon = _ctk_file_system_volume_get_symbolic_icon (volume);
      _ctk_file_system_volume_unref (volume);
      ctk_image_set_from_gicon (CTK_IMAGE (button_data->image), path_bar->priv->root_icon, CTK_ICON_SIZE_BUTTON);

      break;

    case HOME_BUTTON:
      if (path_bar->priv->home_icon != NULL)
        {
          ctk_image_set_from_gicon (CTK_IMAGE (button_data->image), path_bar->priv->home_icon, CTK_ICON_SIZE_BUTTON);
	  break;
	}

      data = g_new0 (struct SetButtonImageData, 1);
      data->path_bar = path_bar;
      data->button_data = button_data;

      if (button_data->cancellable)
	{
	  cancel_cancellable (path_bar, button_data->cancellable);
	}

      button_data->cancellable =
        _ctk_file_system_get_info (path_bar->priv->file_system,
				   path_bar->priv->home_file,
				   "standard::symbolic-icon",
				   set_button_image_get_info_cb,
				   data);
      add_cancellable (path_bar, button_data->cancellable);
      break;

    case DESKTOP_BUTTON:
      if (path_bar->priv->desktop_icon != NULL)
        {
          ctk_image_set_from_gicon (CTK_IMAGE (button_data->image), path_bar->priv->desktop_icon, CTK_ICON_SIZE_BUTTON);
	  break;
	}

      data = g_new0 (struct SetButtonImageData, 1);
      data->path_bar = path_bar;
      data->button_data = button_data;

      if (button_data->cancellable)
	{
	  cancel_cancellable (path_bar, button_data->cancellable);
	}

      button_data->cancellable =
        _ctk_file_system_get_info (path_bar->priv->file_system,
				   path_bar->priv->desktop_file,
				   "standard::symbolic-icon",
				   set_button_image_get_info_cb,
				   data);
      add_cancellable (path_bar, button_data->cancellable);
      break;
    default:
      break;
    }
}

static void
button_data_free (ButtonData *button_data)
{
  if (button_data->file)
    g_object_unref (button_data->file);
  button_data->file = NULL;

  g_free (button_data->dir_name);
  button_data->dir_name = NULL;

  button_data->button = NULL;

  g_free (button_data);
}

static const char *
get_dir_name (ButtonData *button_data)
{
  return button_data->dir_name;
}

static void
ctk_path_bar_update_button_appearance (GtkPathBar *path_bar,
				       ButtonData *button_data,
				       gboolean    current_dir)
{
  const gchar *dir_name = get_dir_name (button_data);
  GtkStyleContext *context;

  context = ctk_widget_get_style_context (button_data->button);

  ctk_style_context_remove_class (context, "text-button");
  ctk_style_context_remove_class (context, "image-button");

  if (button_data->label != NULL)
    {
      ctk_label_set_text (CTK_LABEL (button_data->label), dir_name);
      if (button_data->image == NULL)
        ctk_style_context_add_class (context, "text-button");
    }

  if (button_data->image != NULL)
    {
      set_button_image (path_bar, button_data);
      if (button_data->label == NULL)
        ctk_style_context_add_class (context, "image-button");
    }

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button_data->button)) != current_dir)
    {
      button_data->ignore_changes = TRUE;
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button_data->button), current_dir);
      button_data->ignore_changes = FALSE;
    }
}

static ButtonType
find_button_type (GtkPathBar  *path_bar,
		  GFile       *file)
{
  if (path_bar->priv->root_file != NULL &&
      g_file_equal (file, path_bar->priv->root_file))
    return ROOT_BUTTON;
  if (path_bar->priv->home_file != NULL &&
      g_file_equal (file, path_bar->priv->home_file))
    return HOME_BUTTON;
  if (path_bar->priv->desktop_file != NULL &&
      g_file_equal (file, path_bar->priv->desktop_file))
    return DESKTOP_BUTTON;

 return NORMAL_BUTTON;
}

static void
button_drag_data_get_cb (GtkWidget        *widget,
                         GdkDragContext   *context,
                         GtkSelectionData *selection_data,
                         guint             info,
                         guint             time_,
                         gpointer          data)
{
  ButtonData *button_data;
  char *uris[2];

  button_data = data;

  uris[0] = g_file_get_uri (button_data->file);
  uris[1] = NULL;

  ctk_selection_data_set_uris (selection_data, uris);

  g_free (uris[0]);
}

static ButtonData *
make_directory_button (GtkPathBar  *path_bar,
		       const char  *dir_name,
		       GFile       *file,
		       gboolean     current_dir,
		       gboolean     file_is_hidden)
{
  AtkObject *atk_obj;
  GtkWidget *child = NULL;
  ButtonData *button_data;

  file_is_hidden = !! file_is_hidden;
  /* Is it a special button? */
  button_data = g_new0 (ButtonData, 1);
  button_data->type = find_button_type (path_bar, file);
  button_data->button = ctk_toggle_button_new ();
  atk_obj = ctk_widget_get_accessible (button_data->button);
  ctk_widget_set_focus_on_click (button_data->button, FALSE);
  ctk_widget_add_events (button_data->button, GDK_SCROLL_MASK);

  switch (button_data->type)
    {
    case ROOT_BUTTON:
      button_data->image = ctk_image_new ();
      child = button_data->image;
      button_data->label = NULL;
      atk_object_set_name (atk_obj, _("File System Root"));
      break;
    case HOME_BUTTON:
    case DESKTOP_BUTTON:
      button_data->image = ctk_image_new ();
      button_data->label = ctk_label_new (NULL);
      child = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_pack_start (CTK_BOX (child), button_data->image, FALSE, FALSE, 0);
      ctk_box_pack_start (CTK_BOX (child), button_data->label, FALSE, FALSE, 0);
      break;
    case NORMAL_BUTTON:
    default:
      button_data->label = ctk_label_new (NULL);
      child = button_data->label;
      button_data->image = NULL;
    }

  button_data->dir_name = g_strdup (dir_name);
  button_data->file = g_object_ref (file);
  button_data->file_is_hidden = file_is_hidden;

  ctk_container_add (CTK_CONTAINER (button_data->button), child);
  ctk_widget_show_all (button_data->button);

  ctk_path_bar_update_button_appearance (path_bar, button_data, current_dir);

  g_signal_connect (button_data->button, "clicked",
		    G_CALLBACK (button_clicked_cb),
		    button_data);
  g_object_weak_ref (G_OBJECT (button_data->button),
		     (GWeakNotify) button_data_free, button_data);

  ctk_drag_source_set (button_data->button,
		       GDK_BUTTON1_MASK,
		       NULL, 0,
		       GDK_ACTION_COPY);
  ctk_drag_source_add_uri_targets (button_data->button);
  g_signal_connect (button_data->button, "drag-data-get",
		    G_CALLBACK (button_drag_data_get_cb), button_data);

  return button_data;
}

static gboolean
ctk_path_bar_check_parent_path (GtkPathBar         *path_bar,
				GFile              *file)
{
  GList *list;
  GList *current_path = NULL;
  gboolean need_new_fake_root = FALSE;

  for (list = path_bar->priv->button_list; list; list = list->next)
    {
      ButtonData *button_data;

      button_data = list->data;
      if (g_file_equal (file, button_data->file))
	{
	  current_path = list;
	  break;
	}
      if (list == path_bar->priv->fake_root)
	need_new_fake_root = TRUE;
    }

  if (current_path)
    {
      if (need_new_fake_root)
	{
	  path_bar->priv->fake_root = NULL;
	  for (list = current_path; list; list = list->next)
	    {
	      ButtonData *button_data;

	      button_data = list->data;
	      if (BUTTON_IS_FAKE_ROOT (button_data))
		{
		  path_bar->priv->fake_root = list;
		  break;
		}
	    }
	}

      for (list = path_bar->priv->button_list; list; list = list->next)
	{
	  ctk_path_bar_update_button_appearance (path_bar,
						 BUTTON_DATA (list->data),
						 (list == current_path) ? TRUE : FALSE);
	}

      if (!ctk_widget_get_child_visible (BUTTON_DATA (current_path->data)->button))
	{
	  path_bar->priv->first_scrolled_button = current_path;
	  ctk_widget_queue_resize (CTK_WIDGET (path_bar));
	}

      return TRUE;
    }
  return FALSE;
}


struct SetFileInfo
{
  GFile *file;
  GFile *parent_file;
  GtkPathBar *path_bar;
  GList *new_buttons;
  GList *fake_root;
  gboolean first_directory;
};

static void
ctk_path_bar_set_file_finish (struct SetFileInfo *info,
                              gboolean            result)
{
  if (result)
    {
      GList *l;
      GtkCssNode *prev;

      ctk_path_bar_clear_buttons (info->path_bar);
      info->path_bar->priv->button_list = g_list_reverse (info->new_buttons);
      info->path_bar->priv->fake_root = info->fake_root;
      prev = ctk_widget_get_css_node (info->path_bar->priv->down_slider_button);

      for (l = info->path_bar->priv->button_list; l; l = l->next)
	{
	  GtkWidget *button = BUTTON_DATA (l->data)->button;
          GtkCssNode *node = ctk_widget_get_css_node (button);

          ctk_css_node_insert_before (ctk_widget_get_css_node (CTK_WIDGET (info->path_bar)),
                                      node,
                                      prev);
	  ctk_container_add (CTK_CONTAINER (info->path_bar), button);
          prev = node;
	}
    }
  else
    {
      GList *l;

      for (l = info->new_buttons; l; l = l->next)
	{
	  ButtonData *button_data;

	  button_data = BUTTON_DATA (l->data);
	  ctk_widget_destroy (button_data->button);
	}

      g_list_free (info->new_buttons);
    }

  if (info->file)
    g_object_unref (info->file);
  if (info->parent_file)
    g_object_unref (info->parent_file);

  g_free (info);
}

static void
ctk_path_bar_get_info_callback (GCancellable *cancellable,
			        GFileInfo    *info,
			        const GError *error,
			        gpointer      data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct SetFileInfo *file_info = data;
  ButtonData *button_data;
  const gchar *display_name;
  gboolean is_hidden;

  if (cancelled)
    {
      ctk_path_bar_set_file_finish (file_info, FALSE);
      g_object_unref (cancellable);
      return;
    }

  g_assert (CTK_IS_PATH_BAR (file_info->path_bar));
  g_assert (G_OBJECT (file_info->path_bar)->ref_count > 0);

  g_assert (cancellable == file_info->path_bar->priv->get_info_cancellable);
  cancellable_async_done (file_info->path_bar, cancellable);
  file_info->path_bar->priv->get_info_cancellable = NULL;

  if (!info)
    {
      ctk_path_bar_set_file_finish (file_info, FALSE);
      return;
    }

  display_name = g_file_info_get_display_name (info);
  is_hidden = g_file_info_get_is_hidden (info) || g_file_info_get_is_backup (info);

  button_data = make_directory_button (file_info->path_bar, display_name,
                                       file_info->file,
				       file_info->first_directory, is_hidden);
  g_clear_object (&file_info->file);

  file_info->new_buttons = g_list_prepend (file_info->new_buttons, button_data);

  if (BUTTON_IS_FAKE_ROOT (button_data))
    file_info->fake_root = file_info->new_buttons;

  /* We have assigned the info for the innermost button, i.e. the deepest directory.
   * Now, go on to fetch the info for this directory's parent.
   */

  file_info->file = file_info->parent_file;
  file_info->first_directory = FALSE;

  if (!file_info->file)
    {
      /* No parent?  Okay, we are done. */
      ctk_path_bar_set_file_finish (file_info, TRUE);
      return;
    }

  file_info->parent_file = g_file_get_parent (file_info->file);

  /* Recurse asynchronously */
  file_info->path_bar->priv->get_info_cancellable =
    _ctk_file_system_get_info (file_info->path_bar->priv->file_system,
			       file_info->file,
			       "standard::display-name,standard::is-hidden,standard::is-backup",
			       ctk_path_bar_get_info_callback,
			       file_info);
  add_cancellable (file_info->path_bar, file_info->path_bar->priv->get_info_cancellable);
}

void
_ctk_path_bar_set_file (GtkPathBar *path_bar,
                        GFile      *file,
                        gboolean    keep_trail)
{
  struct SetFileInfo *info;

  g_return_if_fail (CTK_IS_PATH_BAR (path_bar));
  g_return_if_fail (G_IS_FILE (file));

  /* Check whether the new path is already present in the pathbar as buttons.
   * This could be a parent directory or a previous selected subdirectory.
   */
  if (keep_trail && ctk_path_bar_check_parent_path (path_bar, file))
    return;

  info = g_new0 (struct SetFileInfo, 1);
  info->file = g_object_ref (file);
  info->path_bar = path_bar;
  info->first_directory = TRUE;
  info->parent_file = g_file_get_parent (info->file);

  if (path_bar->priv->get_info_cancellable)
    {
      cancel_cancellable (path_bar, path_bar->priv->get_info_cancellable);
    }

  path_bar->priv->get_info_cancellable =
    _ctk_file_system_get_info (path_bar->priv->file_system,
                               info->file,
                               "standard::display-name,standard::is-hidden,standard::is-backup",
                               ctk_path_bar_get_info_callback,
                               info);
  add_cancellable (path_bar, path_bar->priv->get_info_cancellable);
}

/* FIXME: This should be a construct-only property */
void
_ctk_path_bar_set_file_system (GtkPathBar    *path_bar,
			       GtkFileSystem *file_system)
{
  const char *home;

  g_return_if_fail (CTK_IS_PATH_BAR (path_bar));

  g_assert (path_bar->priv->file_system == NULL);

  path_bar->priv->file_system = g_object_ref (file_system);

  home = g_get_home_dir ();
  if (home != NULL)
    {
      const gchar *desktop;

      path_bar->priv->home_file = g_file_new_for_path (home);
      /* FIXME: Need file system backend specific way of getting the
       * Desktop path.
       */
      desktop = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
      if (desktop != NULL)
        path_bar->priv->desktop_file = g_file_new_for_path (desktop);
      else 
        path_bar->priv->desktop_file = NULL;
    }
  else
    {
      path_bar->priv->home_file = NULL;
      path_bar->priv->desktop_file = NULL;
    }
  path_bar->priv->root_file = g_file_new_for_path ("/");
}

/**
 * _ctk_path_bar_up:
 * @path_bar: a #GtkPathBar
 * 
 * If the selected button in the pathbar is not the furthest button “up” (in the
 * root direction), act as if the user clicked on the next button up.
 **/
void
_ctk_path_bar_up (GtkPathBar *path_bar)
{
  GList *l;

  for (l = path_bar->priv->button_list; l; l = l->next)
    {
      GtkWidget *button = BUTTON_DATA (l->data)->button;
      if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
	{
	  if (l->next)
	    {
	      GtkWidget *next_button = BUTTON_DATA (l->next->data)->button;
	      button_clicked_cb (next_button, l->next->data);
	    }
	  break;
	}
    }
}

/**
 * _ctk_path_bar_down:
 * @path_bar: a #GtkPathBar
 * 
 * If the selected button in the pathbar is not the furthest button “down” (in the
 * leaf direction), act as if the user clicked on the next button down.
 **/
void
_ctk_path_bar_down (GtkPathBar *path_bar)
{
  GList *l;

  for (l = path_bar->priv->button_list; l; l = l->next)
    {
      GtkWidget *button = BUTTON_DATA (l->data)->button;
      if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
	{
	  if (l->prev)
	    {
	      GtkWidget *prev_button = BUTTON_DATA (l->prev->data)->button;
	      button_clicked_cb (prev_button, l->prev->data);
	    }
	  break;
	}
    }
}
