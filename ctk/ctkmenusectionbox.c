/*
 * Copyright © 2014 Canonical Limited
 * Copyright © 2013 Carlos Garnacho
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "ctkmenusectionbox.h"

#include "ctkwidgetprivate.h"
#include "ctklabel.h"
#include "ctkmenutracker.h"
#include "ctkmodelbutton.h"
#include "ctkseparator.h"
#include "ctksizegroup.h"
#include "ctkstack.h"
#include "ctkstylecontext.h"
#include "ctkpopover.h"
#include "ctkorientable.h"

typedef CtkBoxClass CtkMenuSectionBoxClass;

struct _CtkMenuSectionBox
{
  CtkBox             parent_instance;

  CtkMenuSectionBox *toplevel;
  CtkMenuTracker    *tracker;
  CtkBox            *item_box;
  CtkWidget         *separator;
  guint              separator_sync_idle;
  gboolean           iconic;
  gint               depth;
};

typedef struct
{
  gint     n_items;
  gboolean previous_is_iconic;
} MenuData;

G_DEFINE_TYPE (CtkMenuSectionBox, ctk_menu_section_box, CTK_TYPE_BOX)

static void        ctk_menu_section_box_sync_separators (CtkMenuSectionBox  *box,
                                                         MenuData           *data);
static void        ctk_menu_section_box_new_submenu     (CtkMenuTrackerItem *item,
                                                         CtkMenuSectionBox  *toplevel,
                                                         CtkWidget          *focus,
                                                         const gchar        *name);
static CtkWidget * ctk_menu_section_box_new_section     (CtkMenuTrackerItem *item,
                                                         CtkMenuSectionBox  *parent);

static void
ctk_menu_section_box_sync_item (CtkWidget *widget,
                                gpointer   user_data)
{
  MenuData *data = (MenuData *)user_data;

  if (CTK_IS_MENU_SECTION_BOX (widget))
    ctk_menu_section_box_sync_separators (CTK_MENU_SECTION_BOX (widget), data);
  else
    data->n_items++;
}

/* We are trying to implement the following rules here:
 *
 * rule 1: never ever show separators for empty sections
 * rule 2: always show a separator if there is a label
 * rule 3: don't show a separator for the first section
 * rule 4: don't show a separator for the following sections if there are
 *         no items before it
 * rule 5: never show separators directly above or below an iconic box
 * (rule 6: these rules don't apply exactly the same way for subsections)
 */
static void
ctk_menu_section_box_sync_separators (CtkMenuSectionBox *box,
                                      MenuData          *data)
{
  gboolean previous_section_is_iconic;
  gboolean should_have_separator;
  gboolean should_have_top_margin = FALSE;
  gboolean is_not_empty_item;
  gboolean has_separator;
  gboolean has_label;
  gboolean separator_condition;
  gint n_items_before;

  n_items_before =  data->n_items;
  previous_section_is_iconic = data->previous_is_iconic;

  ctk_container_foreach (CTK_CONTAINER (box->item_box), ctk_menu_section_box_sync_item, data);

  is_not_empty_item = (data->n_items > n_items_before);

  if (is_not_empty_item)
    data->previous_is_iconic = box->iconic;

  if (box->separator == NULL)
    return;

  has_separator = ctk_widget_get_parent (box->separator) != NULL;
  has_label = !CTK_IS_SEPARATOR (box->separator);

  separator_condition = has_label ? TRUE : n_items_before > 0 &&
                                           box->depth <= 1 &&
                                           !previous_section_is_iconic &&
                                           !box->iconic;

  should_have_separator = separator_condition && is_not_empty_item;

  should_have_top_margin = !should_have_separator &&
                           (box->depth <= 1 || box->iconic) &&
                           n_items_before > 0 &&
                           is_not_empty_item;

  ctk_widget_set_margin_top (CTK_WIDGET (box->item_box), should_have_top_margin ? 10 : 0);

  if (should_have_separator == has_separator)
    return;

  if (should_have_separator)
    ctk_box_pack_start (CTK_BOX (box), box->separator, FALSE, FALSE, 0);
  else
    ctk_container_remove (CTK_CONTAINER (box), box->separator);
}

static gboolean
ctk_menu_section_box_handle_sync_separators (gpointer user_data)
{
  CtkMenuSectionBox *box = user_data;
  MenuData data;

  data.n_items = 0;
  data.previous_is_iconic = FALSE;
  ctk_menu_section_box_sync_separators (box, &data);

  box->separator_sync_idle = 0;

  return G_SOURCE_REMOVE;
}

static void
ctk_menu_section_box_schedule_separator_sync (CtkMenuSectionBox *box)
{
  box = box->toplevel;

  if (!box->separator_sync_idle)
    box->separator_sync_idle = cdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE, /* before resize... */
                                                          ctk_menu_section_box_handle_sync_separators,
                                                          box, NULL);
}

static void
ctk_popover_item_activate (CtkWidget *button,
                           gpointer   user_data)
{
  CtkMenuTrackerItem *item = user_data;
  CtkWidget *popover = NULL;

  if (ctk_menu_tracker_item_get_role (item) == CTK_MENU_TRACKER_ITEM_ROLE_NORMAL)
    {
      /* Activating the item could cause the popover
       * to be free'd, for example if it is a Quit item
       */
      popover = g_object_ref (ctk_widget_get_ancestor (button,
                                                       CTK_TYPE_POPOVER));
    }

  ctk_menu_tracker_item_activated (item);

  if (popover != NULL)
    {
      ctk_widget_hide (popover);
      g_object_unref (popover);
    }
}

static void
ctk_menu_section_box_remove_func (gint     position,
                                  gpointer user_data)
{
  CtkMenuSectionBox *box = user_data;
  CtkMenuTrackerItem *item;
  CtkWidget *widget;
  GList *children;

  children = ctk_container_get_children (CTK_CONTAINER (box->item_box));

  widget = g_list_nth_data (children, position);

  item = g_object_get_data (G_OBJECT (widget), "CtkMenuTrackerItem");
  if (ctk_menu_tracker_item_get_has_link (item, G_MENU_LINK_SUBMENU))
    {
      CtkWidget *stack, *subbox;

      stack = ctk_widget_get_ancestor (CTK_WIDGET (box->toplevel), CTK_TYPE_STACK);
      subbox = ctk_stack_get_child_by_name (CTK_STACK (stack), ctk_menu_tracker_item_get_label (item));
      if (subbox != NULL)
        ctk_container_remove (CTK_CONTAINER (stack), subbox);
    }

  ctk_widget_destroy (g_list_nth_data (children, position));
  g_list_free (children);

  ctk_menu_section_box_schedule_separator_sync (box);
}

static gboolean
get_ancestors (CtkWidget  *widget,
               GType       widget_type,
               CtkWidget **ancestor,
               CtkWidget **below)
{
  CtkWidget *a, *b;

  a = NULL;
  b = widget;
  while (b != NULL)
    {
      a = ctk_widget_get_parent (b);
      if (!a)
        return FALSE;
      if (g_type_is_a (G_OBJECT_TYPE (a), widget_type))
        break;
      b = a;
    }

  *below = b;
  *ancestor = a;

  return TRUE;
}

static void
close_submenu (CtkWidget *button,
               gpointer   data)
{
  CtkMenuTrackerItem *item = data;
  CtkWidget *focus;

  if (ctk_menu_tracker_item_get_should_request_show (item))
    ctk_menu_tracker_item_request_submenu_shown (item, FALSE);

  focus = CTK_WIDGET (g_object_get_data (G_OBJECT (button), "focus"));
  ctk_widget_grab_focus (focus);
}

static void
open_submenu (CtkWidget *button,
              gpointer   data)
{
  CtkMenuTrackerItem *item = data;
  CtkWidget *focus;

  if (ctk_menu_tracker_item_get_should_request_show (item))
    ctk_menu_tracker_item_request_submenu_shown (item, TRUE);

  focus = CTK_WIDGET (g_object_get_data (G_OBJECT (button), "focus"));
  ctk_widget_grab_focus (focus);
}

static void
ctk_menu_section_box_insert_func (CtkMenuTrackerItem *item,
                                  gint                position,
                                  gpointer            user_data)
{
  CtkMenuSectionBox *box = user_data;
  CtkWidget *widget;

  if (ctk_menu_tracker_item_get_is_separator (item))
    {
      widget = ctk_menu_section_box_new_section (item, box);
    }
  else if (ctk_menu_tracker_item_get_has_link (item, G_MENU_LINK_SUBMENU))
    {
      CtkWidget *stack = NULL;
      CtkWidget *parent = NULL;
      gchar *name;

      widget = g_object_new (CTK_TYPE_MODEL_BUTTON,
                             "menu-name", ctk_menu_tracker_item_get_label (item),
                             NULL);
      g_object_bind_property (item, "label", widget, "text", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "icon", widget, "icon", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "sensitive", widget, "sensitive", G_BINDING_SYNC_CREATE);

      get_ancestors (CTK_WIDGET (box->toplevel), CTK_TYPE_STACK, &stack, &parent);
      ctk_container_child_get (CTK_CONTAINER (stack), parent, "name", &name, NULL);
      ctk_menu_section_box_new_submenu (item, box->toplevel, widget, name);
      g_free (name);
    }
  else
    {
      widget = ctk_model_button_new ();
      g_object_bind_property (item, "label", widget, "text", G_BINDING_SYNC_CREATE);

      if (box->iconic)
        {
          g_object_bind_property (item, "verb-icon", widget, "icon", G_BINDING_SYNC_CREATE);
          g_object_set (widget, "iconic", TRUE, "centered", TRUE, NULL);
        }
      else
        g_object_bind_property (item, "icon", widget, "icon", G_BINDING_SYNC_CREATE);

      g_object_bind_property (item, "sensitive", widget, "sensitive", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "role", widget, "role", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "toggled", widget, "active", G_BINDING_SYNC_CREATE);
      g_signal_connect (widget, "clicked", G_CALLBACK (ctk_popover_item_activate), item);
    }

  ctk_widget_show (widget);

  g_object_set_data_full (G_OBJECT (widget), "CtkMenuTrackerItem", g_object_ref (item), g_object_unref);

  ctk_widget_set_halign (widget, CTK_ALIGN_FILL);
  if (box->iconic)
    ctk_box_pack_start (CTK_BOX (box->item_box), widget, TRUE, TRUE, 0);
  else
    ctk_container_add (CTK_CONTAINER (box->item_box), widget);
  ctk_box_reorder_child (CTK_BOX (box->item_box), widget, position);

  ctk_menu_section_box_schedule_separator_sync (box);
}

static void
ctk_menu_section_box_init (CtkMenuSectionBox *box)
{
  CtkWidget *item_box;

  ctk_orientable_set_orientation (CTK_ORIENTABLE (box), CTK_ORIENTATION_VERTICAL);

  box->toplevel = box;

  item_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  box->item_box = CTK_BOX (item_box);
  ctk_box_pack_end (CTK_BOX (box), item_box, FALSE, FALSE, 0);
  ctk_widget_set_halign (CTK_WIDGET (item_box), CTK_ALIGN_FILL);
  ctk_widget_show (item_box);

  ctk_widget_set_halign (CTK_WIDGET (box), CTK_ALIGN_FILL);
  g_object_set (box, "margin", 0, NULL);

}

static void
ctk_menu_section_box_dispose (GObject *object)
{
  CtkMenuSectionBox *box = CTK_MENU_SECTION_BOX (object);

  if (box->separator_sync_idle)
    {
      g_source_remove (box->separator_sync_idle);
      box->separator_sync_idle = 0;
    }

  g_clear_object (&box->separator);

  if (box->tracker)
    {
      ctk_menu_tracker_free (box->tracker);
      box->tracker = NULL;
    }

  G_OBJECT_CLASS (ctk_menu_section_box_parent_class)->dispose (object);
}

static void
ctk_menu_section_box_class_init (CtkMenuSectionBoxClass *class)
{
  G_OBJECT_CLASS (class)->dispose = ctk_menu_section_box_dispose;
}

static void
update_popover_position_cb (GObject    *source,
                            GParamSpec *spec G_GNUC_UNUSED,
                            gpointer   *user_data)
{
  CtkPopover *popover = CTK_POPOVER (source);
  CtkMenuSectionBox *box = CTK_MENU_SECTION_BOX (user_data);

  CtkPositionType new_pos = ctk_popover_get_position (popover);

  GList *children = ctk_container_get_children (CTK_CONTAINER (ctk_widget_get_parent (CTK_WIDGET (box))));
  GList *l;

  for (l = children;
       l != NULL;
       l = l->next)
    {
      CtkWidget *w = l->data;

      if (new_pos == CTK_POS_BOTTOM)
        ctk_widget_set_valign (w, CTK_ALIGN_START);
      else if (new_pos == CTK_POS_TOP)
        ctk_widget_set_valign (w, CTK_ALIGN_END);
      else
        ctk_widget_set_valign (w, CTK_ALIGN_CENTER);
    }

  g_list_free (children);
}

void
ctk_menu_section_box_new_toplevel (CtkStack    *stack,
                                   GMenuModel  *model,
                                   const gchar *action_namespace,
                                   CtkPopover  *popover)
{
  CtkMenuSectionBox *box;

  box = g_object_new (CTK_TYPE_MENU_SECTION_BOX, "margin", 10,  NULL);
  ctk_stack_add_named (stack, CTK_WIDGET (box), "main");

  box->tracker = ctk_menu_tracker_new (CTK_ACTION_OBSERVABLE (_ctk_widget_get_action_muxer (CTK_WIDGET (box), TRUE)),
                                       model, TRUE, FALSE, FALSE, action_namespace,
                                       ctk_menu_section_box_insert_func,
                                       ctk_menu_section_box_remove_func, box);

  g_signal_connect (G_OBJECT (popover), "notify::position", G_CALLBACK (update_popover_position_cb), box);


  ctk_widget_show (CTK_WIDGET (box));
}

static void
ctk_menu_section_box_new_submenu (CtkMenuTrackerItem *item,
                                  CtkMenuSectionBox  *toplevel,
                                  CtkWidget          *focus,
                                  const gchar        *name)
{
  CtkMenuSectionBox *box;
  CtkWidget *button;

  box = g_object_new (CTK_TYPE_MENU_SECTION_BOX, "margin", 10, NULL);

  button = g_object_new (CTK_TYPE_MODEL_BUTTON,
                         "menu-name", name,
                         "inverted", TRUE,
                         "centered", TRUE,
                         NULL);

  g_object_bind_property (item, "label", button, "text", G_BINDING_SYNC_CREATE);
  g_object_bind_property (item, "icon", button, "icon", G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (button), "focus", focus);
  g_object_set_data (G_OBJECT (focus), "focus", button);

  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);
  ctk_widget_show (button);

  g_signal_connect (focus, "clicked", G_CALLBACK (open_submenu), item);
  g_signal_connect (button, "clicked", G_CALLBACK (close_submenu), item);

  ctk_stack_add_named (CTK_STACK (ctk_widget_get_ancestor (CTK_WIDGET (toplevel), CTK_TYPE_STACK)),
                       CTK_WIDGET (box), ctk_menu_tracker_item_get_label (item));
  ctk_widget_show (CTK_WIDGET (box));

  box->tracker = ctk_menu_tracker_new_for_item_link (item, G_MENU_LINK_SUBMENU, FALSE, FALSE,
                                                     ctk_menu_section_box_insert_func,
                                                     ctk_menu_section_box_remove_func,
                                                     box);
}

static CtkWidget *
ctk_menu_section_box_new_section (CtkMenuTrackerItem *item,
                                  CtkMenuSectionBox  *parent)
{
  CtkMenuSectionBox *box;
  const gchar *label;
  const gchar *hint;
  const gchar *text_direction;

  box = g_object_new (CTK_TYPE_MENU_SECTION_BOX, NULL);
  box->toplevel = parent->toplevel;
  box->depth = parent->depth + 1;

  label = ctk_menu_tracker_item_get_label (item);
  hint = ctk_menu_tracker_item_get_display_hint (item);
  text_direction = ctk_menu_tracker_item_get_text_direction (item);

  if (hint && g_str_equal (hint, "horizontal-buttons"))
    {
      ctk_orientable_set_orientation (CTK_ORIENTABLE (box->item_box), CTK_ORIENTATION_HORIZONTAL);
      ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (box->item_box)), CTK_STYLE_CLASS_LINKED);
      box->iconic = TRUE;

      if (text_direction)
        {
          CtkTextDirection dir = CTK_TEXT_DIR_NONE;

          if (g_str_equal (text_direction, "rtl"))
            dir = CTK_TEXT_DIR_RTL;
          else if (g_str_equal (text_direction, "ltr"))
            dir = CTK_TEXT_DIR_LTR;

          ctk_widget_set_direction (CTK_WIDGET (box->item_box), dir);
        }
    }

  if (label != NULL)
    {
      CtkWidget *separator;
      CtkWidget *title;

      box->separator = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
      g_object_ref_sink (box->separator);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_widget_set_valign (separator, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (box->separator), separator, TRUE, TRUE, 0);

      title = ctk_label_new (label);
      g_object_bind_property (item, "label", title, "label", G_BINDING_SYNC_CREATE);
      ctk_style_context_add_class (ctk_widget_get_style_context (title), CTK_STYLE_CLASS_SEPARATOR);
      ctk_widget_set_halign (title, CTK_ALIGN_START);
      ctk_box_pack_start (CTK_BOX (box->separator), title, FALSE, FALSE, 0);

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_widget_set_valign (separator, CTK_ALIGN_CENTER);
      ctk_box_pack_start (CTK_BOX (box->separator), separator, TRUE, TRUE, 0);

      ctk_widget_show_all (box->separator);
    }
  else
    {
      box->separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      g_object_ref_sink (box->separator);

      ctk_widget_show (box->separator);
    }

  box->tracker = ctk_menu_tracker_new_for_item_link (item, G_MENU_LINK_SECTION, FALSE, FALSE,
                                                     ctk_menu_section_box_insert_func,
                                                     ctk_menu_section_box_remove_func,
                                                     box);

  return CTK_WIDGET (box);
}
