/* ctkshortcutssection.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkshortcutssection.h"

#include "ctkshortcutsgroup.h"
#include "ctkbutton.h"
#include "ctklabel.h"
#include "ctkstack.h"
#include "ctkstackswitcher.h"
#include "ctkstylecontext.h"
#include "ctkorientable.h"
#include "ctksizegroup.h"
#include "ctkwidget.h"
#include "ctkbindings.h"
#include "ctkprivate.h"
#include "ctkmarshalers.h"
#include "ctkgesturepan.h"
#include "ctkwidgetprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctkshortcutssection
 * @Title: CtkShortcutsSection
 * @Short_description: Represents an application mode in a CtkShortcutsWindow
 *
 * A CtkShortcutsSection collects all the keyboard shortcuts and gestures
 * for a major application mode. If your application needs multiple sections,
 * you should give each section a unique #CtkShortcutsSection:section-name and
 * a #CtkShortcutsSection:title that can be shown in the section selector of
 * the CtkShortcutsWindow.
 *
 * The #CtkShortcutsSection:max-height property can be used to influence how
 * the groups in the section are distributed over pages and columns.
 *
 * This widget is only meant to be used with #CtkShortcutsWindow.
 */

struct _CtkShortcutsSection
{
  CtkBox            parent_instance;

  gchar            *name;
  gchar            *title;
  gchar            *view_name;
  guint             max_height;

  CtkStack         *stack;
  CtkStackSwitcher *switcher;
  CtkWidget        *show_all;
  CtkWidget        *footer;
  GList            *groups;

  gboolean          has_filtered_group;
  gboolean          need_reflow;

  CtkGesture       *pan_gesture;
};

struct _CtkShortcutsSectionClass
{
  CtkBoxClass parent_class;

  gboolean (* change_current_page) (CtkShortcutsSection *self,
                                    gint                 offset);

};

G_DEFINE_TYPE (CtkShortcutsSection, ctk_shortcuts_section, CTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_TITLE,
  PROP_SECTION_NAME,
  PROP_VIEW_NAME,
  PROP_MAX_HEIGHT,
  LAST_PROP
};

enum {
  CHANGE_CURRENT_PAGE,
  LAST_SIGNAL
};

static GParamSpec *properties[LAST_PROP];
static guint signals[LAST_SIGNAL];

static void ctk_shortcuts_section_set_view_name    (CtkShortcutsSection *self,
                                                    const gchar         *view_name);
static void ctk_shortcuts_section_set_max_height   (CtkShortcutsSection *self,
                                                    guint                max_height);
static void ctk_shortcuts_section_add_group        (CtkShortcutsSection *self,
                                                    CtkShortcutsGroup   *group);

static void ctk_shortcuts_section_show_all         (CtkShortcutsSection *self);
static void ctk_shortcuts_section_filter_groups    (CtkShortcutsSection *self);
static void ctk_shortcuts_section_reflow_groups    (CtkShortcutsSection *self);
static void ctk_shortcuts_section_maybe_reflow     (CtkShortcutsSection *self);

static gboolean ctk_shortcuts_section_change_current_page (CtkShortcutsSection *self,
                                                           gint                 offset);

static void ctk_shortcuts_section_pan_gesture_pan (CtkGesturePan       *gesture,
                                                   CtkPanDirection      direction,
                                                   gdouble              offset,
                                                   CtkShortcutsSection *self);

static void
ctk_shortcuts_section_add (CtkContainer *container,
                           CtkWidget    *child)
{
  CtkShortcutsSection *self = CTK_SHORTCUTS_SECTION (container);

  if (CTK_IS_SHORTCUTS_GROUP (child))
    ctk_shortcuts_section_add_group (self, CTK_SHORTCUTS_GROUP (child));
  else
    g_warning ("Can't add children of type %s to %s",
               G_OBJECT_TYPE_NAME (child),
               G_OBJECT_TYPE_NAME (container));
}

static void
ctk_shortcuts_section_remove (CtkContainer *container,
                              CtkWidget    *child)
{
  CtkShortcutsSection *self = (CtkShortcutsSection *)container;

  if (CTK_IS_SHORTCUTS_GROUP (child) &&
      ctk_widget_is_ancestor (child, CTK_WIDGET (container)))
    {
      self->groups = g_list_remove (self->groups, child);
      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (child)), child);
    }
  else
    CTK_CONTAINER_CLASS (ctk_shortcuts_section_parent_class)->remove (container, child);
}

static void
ctk_shortcuts_section_forall (CtkContainer *container,
                              gboolean      include_internal,
                              CtkCallback   callback,
                              gpointer      callback_data)
{
  CtkShortcutsSection *self = (CtkShortcutsSection *)container;
  GList *l;

  if (include_internal)
    {
      CTK_CONTAINER_CLASS (ctk_shortcuts_section_parent_class)->forall (container, include_internal, callback, callback_data);
    }
  else
    {
      for (l = self->groups; l; l = l->next)
        {
          CtkWidget *group = l->data;
          callback (group, callback_data);
        }
    }
}

static void
map_child (CtkWidget *child)
{
  if (_ctk_widget_get_visible (child) &&
      _ctk_widget_get_child_visible (child) &&
      !_ctk_widget_get_mapped (child))
    ctk_widget_map (child);
}

static void
ctk_shortcuts_section_map (CtkWidget *widget)
{
  CtkShortcutsSection *self = CTK_SHORTCUTS_SECTION (widget);

  if (self->need_reflow)
    ctk_shortcuts_section_reflow_groups (self);

  ctk_widget_set_mapped (widget, TRUE);

  map_child (CTK_WIDGET (self->stack));
  map_child (CTK_WIDGET (self->footer));
}

static void
ctk_shortcuts_section_unmap (CtkWidget *widget)
{
  CtkShortcutsSection *self = CTK_SHORTCUTS_SECTION (widget);

  ctk_widget_set_mapped (widget, FALSE);

  ctk_widget_unmap (CTK_WIDGET (self->footer));
  ctk_widget_unmap (CTK_WIDGET (self->stack));
}

static void
ctk_shortcuts_section_destroy (CtkWidget *widget)
{
  CtkShortcutsSection *self = CTK_SHORTCUTS_SECTION (widget);

  if (self->stack)
    {
      ctk_widget_destroy (CTK_WIDGET (self->stack));
      self->stack = NULL;
    }

  if (self->footer)
    {
      ctk_widget_destroy (CTK_WIDGET (self->footer));
      self->footer = NULL;
    }

  g_list_free (self->groups);
  self->groups = NULL;

  CTK_WIDGET_CLASS (ctk_shortcuts_section_parent_class)->destroy (widget);
}

static void
ctk_shortcuts_section_finalize (GObject *object)
{
  CtkShortcutsSection *self = (CtkShortcutsSection *)object;

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->title, g_free);
  g_clear_pointer (&self->view_name, g_free);
  g_clear_object (&self->pan_gesture);

  G_OBJECT_CLASS (ctk_shortcuts_section_parent_class)->finalize (object);
}

static void
ctk_shortcuts_section_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  CtkShortcutsSection *self = (CtkShortcutsSection *)object;

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_VIEW_NAME:
      g_value_set_string (value, self->view_name);
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_MAX_HEIGHT:
      g_value_set_uint (value, self->max_height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_shortcuts_section_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  CtkShortcutsSection *self = (CtkShortcutsSection *)object;

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      g_free (self->name);
      self->name = g_value_dup_string (value);
      break;

    case PROP_VIEW_NAME:
      ctk_shortcuts_section_set_view_name (self, g_value_get_string (value));
      break;

    case PROP_TITLE:
      g_free (self->title);
      self->title = g_value_dup_string (value);
      break;

    case PROP_MAX_HEIGHT:
      ctk_shortcuts_section_set_max_height (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GType
ctk_shortcuts_section_child_type (CtkContainer *container)
{
  return CTK_TYPE_SHORTCUTS_GROUP;
}

static void
ctk_shortcuts_section_class_init (CtkShortcutsSectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);
  CtkBindingSet *binding_set;

  object_class->finalize = ctk_shortcuts_section_finalize;
  object_class->get_property = ctk_shortcuts_section_get_property;
  object_class->set_property = ctk_shortcuts_section_set_property;

  widget_class->map = ctk_shortcuts_section_map;
  widget_class->unmap = ctk_shortcuts_section_unmap;
  widget_class->destroy = ctk_shortcuts_section_destroy;

  container_class->add = ctk_shortcuts_section_add;
  container_class->remove = ctk_shortcuts_section_remove;
  container_class->forall = ctk_shortcuts_section_forall;
  container_class->child_type = ctk_shortcuts_section_child_type;

  klass->change_current_page = ctk_shortcuts_section_change_current_page;

  /**
   * CtkShortcutsSection:section-name:
   *
   * A unique name to identify this section among the sections
   * added to the CtkShortcutsWindow. Setting the #CtkShortcutsWindow:section-name
   * property to this string will make this section shown in the
   * CtkShortcutsWindow.
   */
  properties[PROP_SECTION_NAME] =
    g_param_spec_string ("section-name", P_("Section Name"), P_("Section Name"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsSection:view-name:
   *
   * A view name to filter the groups in this section by.
   * See #CtkShortcutsGroup:view.
   *
   * Applications are expected to use the #CtkShortcutsWindow:view-name
   * property for this purpose.
   */
  properties[PROP_VIEW_NAME] =
    g_param_spec_string ("view-name", P_("View Name"), P_("View Name"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkShortcutsSection:title:
   *
   * The string to show in the section selector of the CtkShortcutsWindow
   * for this section. If there is only one section, you don't need to
   * set a title, since the section selector will not be shown in this case.
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title", P_("Title"), P_("Title"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsSection:max-height:
   *
   * The maximum number of lines to allow per column. This property can
   * be used to influence how the groups in this section are distributed
   * across pages and columns. The default value of 15 should work in
   * most cases.
   */
  properties[PROP_MAX_HEIGHT] =
    g_param_spec_uint ("max-height", P_("Maximum Height"), P_("Maximum Height"),
                       0, G_MAXUINT, 15,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  signals[CHANGE_CURRENT_PAGE] =
    g_signal_new (I_("change-current-page"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkShortcutsSectionClass, change_current_page),
                  NULL, NULL,
                  _ctk_marshal_BOOLEAN__INT,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_INT);

  binding_set = ctk_binding_set_by_class (klass);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_Page_Up, 0,
                                "change-current-page", 1,
                                G_TYPE_INT, -1);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_Page_Down, 0,
                                "change-current-page", 1,
                                G_TYPE_INT, 1);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_Page_Up, GDK_CONTROL_MASK,
                                "change-current-page", 1,
                                G_TYPE_INT, -1);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_Page_Down, GDK_CONTROL_MASK,
                                "change-current-page", 1,
                                G_TYPE_INT, 1);
}

static void
ctk_shortcuts_section_init (CtkShortcutsSection *self)
{
  self->max_height = 15;

  ctk_orientable_set_orientation (CTK_ORIENTABLE (self), CTK_ORIENTATION_VERTICAL);
  ctk_box_set_homogeneous (CTK_BOX (self), FALSE);
  ctk_box_set_spacing (CTK_BOX (self), 22);
  ctk_container_set_border_width (CTK_CONTAINER (self), 24);

  self->stack = g_object_new (CTK_TYPE_STACK,
                              "homogeneous", TRUE,
                              "transition-type", CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT,
                              "vexpand", TRUE,
                              "visible", TRUE,
                              NULL);
  CTK_CONTAINER_CLASS (ctk_shortcuts_section_parent_class)->add (CTK_CONTAINER (self), CTK_WIDGET (self->stack));

  self->switcher = g_object_new (CTK_TYPE_STACK_SWITCHER,
                                 "halign", CTK_ALIGN_CENTER,
                                 "stack", self->stack,
                                 "spacing", 12,
                                 "no-show-all", TRUE,
                                 NULL);

  ctk_style_context_remove_class (ctk_widget_get_style_context (CTK_WIDGET (self->switcher)), CTK_STYLE_CLASS_LINKED);

  self->show_all = ctk_button_new_with_mnemonic (_("_Show All"));
  ctk_widget_set_no_show_all (self->show_all, TRUE);
  g_signal_connect_swapped (self->show_all, "clicked",
                            G_CALLBACK (ctk_shortcuts_section_show_all), self);

  self->footer = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 20);
  CTK_CONTAINER_CLASS (ctk_shortcuts_section_parent_class)->add (CTK_CONTAINER (self), self->footer);

  ctk_box_set_center_widget (CTK_BOX (self->footer), CTK_WIDGET (self->switcher));
  ctk_box_pack_end (CTK_BOX (self->footer), self->show_all, TRUE, TRUE, 0);
  ctk_widget_set_halign (self->show_all, CTK_ALIGN_END);

  self->pan_gesture = ctk_gesture_pan_new (CTK_WIDGET (self->stack), CTK_ORIENTATION_HORIZONTAL);
  g_signal_connect (self->pan_gesture, "pan",
                    G_CALLBACK (ctk_shortcuts_section_pan_gesture_pan), self);
}

static void
ctk_shortcuts_section_set_view_name (CtkShortcutsSection *self,
                                     const gchar         *view_name)
{
  if (g_strcmp0 (self->view_name, view_name) == 0)
    return;

  g_free (self->view_name);
  self->view_name = g_strdup (view_name);

  ctk_shortcuts_section_filter_groups (self);
  ctk_shortcuts_section_reflow_groups (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VIEW_NAME]);
}

static void
ctk_shortcuts_section_set_max_height (CtkShortcutsSection *self,
                                      guint                max_height)
{
  if (self->max_height == max_height)
    return;

  self->max_height = max_height;

  ctk_shortcuts_section_maybe_reflow (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAX_HEIGHT]);
}

static void
ctk_shortcuts_section_add_group (CtkShortcutsSection *self,
                                 CtkShortcutsGroup   *group)
{
  GList *children;
  CtkWidget *page, *column;

  children = ctk_container_get_children (CTK_CONTAINER (self->stack));
  if (children)
    page = g_list_last (children)->data;
  else
    {
      page = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 22);
      ctk_stack_add_named (self->stack, page, "1");
    }
  g_list_free (children);

  children = ctk_container_get_children (CTK_CONTAINER (page));
  if (children)
    column = g_list_last (children)->data;
  else
    {
      column = ctk_box_new (CTK_ORIENTATION_VERTICAL, 22);
      ctk_container_add (CTK_CONTAINER (page), column);
    }
  g_list_free (children);

  ctk_container_add (CTK_CONTAINER (column), CTK_WIDGET (group));
  self->groups = g_list_append (self->groups, group);

  ctk_shortcuts_section_maybe_reflow (self);
}

static void
ctk_shortcuts_section_show_all (CtkShortcutsSection *self)
{
  ctk_shortcuts_section_set_view_name (self, NULL);
}

static void
update_group_visibility (CtkWidget *child, gpointer data)
{
  CtkShortcutsSection *self = data;

  if (CTK_IS_SHORTCUTS_GROUP (child))
    {
      gchar *view;
      gboolean match;

      g_object_get (child, "view", &view, NULL);
      match = view == NULL ||
              self->view_name == NULL ||
              strcmp (view, self->view_name) == 0;

      ctk_widget_set_visible (child, match);
      self->has_filtered_group |= !match;

      g_free (view);
    }
  else if (CTK_IS_CONTAINER (child))
    {
      ctk_container_foreach (CTK_CONTAINER (child), update_group_visibility, data);
    }
}

static void
ctk_shortcuts_section_filter_groups (CtkShortcutsSection *self)
{
  self->has_filtered_group = FALSE;

  ctk_container_foreach (CTK_CONTAINER (self), update_group_visibility, self);

  ctk_widget_set_visible (CTK_WIDGET (self->show_all), self->has_filtered_group);
  ctk_widget_set_visible (ctk_widget_get_parent (CTK_WIDGET (self->show_all)),
                          ctk_widget_get_visible (CTK_WIDGET (self->show_all)) ||
                          ctk_widget_get_visible (CTK_WIDGET (self->switcher)));
}

static void
ctk_shortcuts_section_maybe_reflow (CtkShortcutsSection *self)
{
  if (ctk_widget_get_mapped (CTK_WIDGET (self)))
    ctk_shortcuts_section_reflow_groups (self);
  else
    self->need_reflow = TRUE;
}

static void
adjust_page_buttons (CtkWidget *widget,
                     gpointer   data)
{
  CtkWidget *label;

  ctk_style_context_add_class (ctk_widget_get_style_context (widget), "circular");

  label = ctk_bin_get_child (CTK_BIN (widget));
  ctk_label_set_use_underline (CTK_LABEL (label), TRUE);
}

static void
ctk_shortcuts_section_reflow_groups (CtkShortcutsSection *self)
{
  GList *pages, *p;
  GList *columns, *c;
  GList *groups, *g;
  GList *children;
  guint n_rows;
  guint n_columns;
  guint n_pages;
  CtkWidget *current_page, *current_column;

  /* collect all groups from the current pages */
  groups = NULL;
  pages = ctk_container_get_children (CTK_CONTAINER (self->stack));
  for (p = pages; p; p = p->next)
    {
      columns = ctk_container_get_children (CTK_CONTAINER (p->data));
      for (c = columns; c; c = c->next)
        {
          children = ctk_container_get_children (CTK_CONTAINER (c->data));
          groups = g_list_concat (groups, children);
        }
      g_list_free (columns);
    }
  g_list_free (pages);

  /* create new pages */
  current_page = NULL;
  current_column = NULL;
  pages = NULL;
  n_rows = 0;
  n_columns = 0;
  n_pages = 0;
  for (g = groups; g; g = g->next)
    {
      CtkShortcutsGroup *group = g->data;
      guint height;
      gboolean visible;

      g_object_get (group,
                    "visible", &visible,
                    "height", &height,
                    NULL);
      if (!visible)
        height = 0;

      if (current_column == NULL || n_rows + height > self->max_height)
        {
          CtkWidget *column;
          CtkSizeGroup *group;

          column = ctk_box_new (CTK_ORIENTATION_VERTICAL, 22);
          ctk_widget_show (column);

          group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          ctk_size_group_set_ignore_hidden (group, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS
          g_object_set_data_full (G_OBJECT (column), "accel-size-group", group, g_object_unref);

          group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          ctk_size_group_set_ignore_hidden (group, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS
          g_object_set_data_full (G_OBJECT (column), "title-size-group", group, g_object_unref);

          if (n_columns % 2 == 0)
            {
              CtkWidget *page;

              page = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 22);
              ctk_widget_show (page);

              pages = g_list_append (pages, page);
              current_page = page;
            }

          ctk_container_add (CTK_CONTAINER (current_page), column);
          current_column = column;
          n_columns += 1;
          n_rows = 0;
        }

      n_rows += height;

      g_object_set (group,
                    "accel-size-group", g_object_get_data (G_OBJECT (current_column), "accel-size-group"),
                    "title-size-group", g_object_get_data (G_OBJECT (current_column), "title-size-group"),
                    NULL);

      g_object_ref (group);
      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (CTK_WIDGET (group))), CTK_WIDGET (group));
      ctk_container_add (CTK_CONTAINER (current_column), CTK_WIDGET (group));
      g_object_unref (group);
    }

  /* balance the last page */
  if (n_columns % 2 == 1)
    {
      CtkWidget *column;
      CtkSizeGroup *group;
      GList *content;
      guint n;

      column = ctk_box_new (CTK_ORIENTATION_VERTICAL, 22);
      ctk_widget_show (column);

      group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_size_group_set_ignore_hidden (group, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS
      g_object_set_data_full (G_OBJECT (column), "accel-size-group", group, g_object_unref);
      group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_size_group_set_ignore_hidden (group, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS
      g_object_set_data_full (G_OBJECT (column), "title-size-group", group, g_object_unref);

      ctk_container_add (CTK_CONTAINER (current_page), column);

      content = ctk_container_get_children (CTK_CONTAINER (current_column));
      n = 0;

      for (g = g_list_last (content); g; g = g->prev)
        {
          CtkShortcutsGroup *group = g->data;
          guint height;
          gboolean visible;

          g_object_get (group,
                        "visible", &visible,
                        "height", &height,
                        NULL);
          if (!visible)
            height = 0;

          if (n_rows - height == 0)
            break;
          if (ABS (n_rows - n) < ABS ((n_rows - height) - (n + height)))
            break;

          n_rows -= height;
          n += height;
        }

      for (g = g->next; g; g = g->next)
        {
          CtkShortcutsGroup *group = g->data;

          g_object_set (group,
                        "accel-size-group", g_object_get_data (G_OBJECT (column), "accel-size-group"),
                        "title-size-group", g_object_get_data (G_OBJECT (column), "title-size-group"),
                        NULL);

          g_object_ref (group);
          ctk_container_remove (CTK_CONTAINER (current_column), CTK_WIDGET (group));
          ctk_container_add (CTK_CONTAINER (column), CTK_WIDGET (group));
          g_object_unref (group);
        }

      g_list_free (content);
    }

  /* replace the current pages with the new pages */
  children = ctk_container_get_children (CTK_CONTAINER (self->stack));
  g_list_free_full (children, (GDestroyNotify)ctk_widget_destroy);

  for (p = pages, n_pages = 0; p; p = p->next, n_pages++)
    {
      CtkWidget *page = p->data;
      gchar *title;

      title = g_strdup_printf ("_%u", n_pages + 1);
      ctk_stack_add_titled (self->stack, page, title, title);
      g_free (title);
    }

  /* fix up stack switcher */
  ctk_container_foreach (CTK_CONTAINER (self->switcher), adjust_page_buttons, NULL);
  ctk_widget_set_visible (CTK_WIDGET (self->switcher), (n_pages > 1));
  ctk_widget_set_visible (ctk_widget_get_parent (CTK_WIDGET (self->switcher)),
                          ctk_widget_get_visible (CTK_WIDGET (self->show_all)) ||
                          ctk_widget_get_visible (CTK_WIDGET (self->switcher)));

  /* clean up */
  g_list_free (groups);
  g_list_free (pages);

  self->need_reflow = FALSE;
}

static gboolean
ctk_shortcuts_section_change_current_page (CtkShortcutsSection *self,
                                           gint                 offset)
{
  CtkWidget *child;
  GList *children, *l;

  child = ctk_stack_get_visible_child (self->stack);
  children = ctk_container_get_children (CTK_CONTAINER (self->stack));
  l = g_list_find (children, child);

  if (offset == 1)
    l = l->next;
  else if (offset == -1)
    l = l->prev;
  else
    g_assert_not_reached ();

  if (l)
    ctk_stack_set_visible_child (self->stack, CTK_WIDGET (l->data));
  else
    ctk_widget_error_bell (CTK_WIDGET (self));

  g_list_free (children);

  return TRUE;
}

static void
ctk_shortcuts_section_pan_gesture_pan (CtkGesturePan       *gesture,
                                       CtkPanDirection      direction,
                                       gdouble              offset,
                                       CtkShortcutsSection *self)
{
  if (offset < 50)
    return;

  if (direction == CTK_PAN_DIRECTION_LEFT)
    ctk_shortcuts_section_change_current_page (self, 1);
  else if (direction == CTK_PAN_DIRECTION_RIGHT)
    ctk_shortcuts_section_change_current_page (self, -1);
  else
    g_assert_not_reached ();

  ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
}
