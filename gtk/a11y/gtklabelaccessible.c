/* GTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

/* Preamble {{{1 */

#include "config.h"

#include <gtk/gtk.h>
#include <gtk/gtkpango.h>
#include "gtkwidgetprivate.h"
#include "gtklabelprivate.h"
#include "gtklabelaccessible.h"
#include "gtklabelaccessibleprivate.h"
#include "gtkstylecontextprivate.h"

struct _GtkLabelAccessiblePrivate
{
  gint cursor_position;
  gint selection_bound;

  GList *links;
};

typedef struct _GtkLabelAccessibleLink      GtkLabelAccessibleLink;
typedef struct _GtkLabelAccessibleLinkClass GtkLabelAccessibleLinkClass;

struct _GtkLabelAccessibleLink
{
  AtkHyperlink parent;
  
  GtkLabelAccessible *label;
  gint index;
  gboolean focused;
};

struct _GtkLabelAccessibleLinkClass
{
  AtkHyperlinkClass parent_class;
};

static GtkLabelAccessibleLink *ctk_label_accessible_link_new (GtkLabelAccessible *label,
                                                              gint                idx);

typedef struct _GtkLabelAccessibleLinkImpl      GtkLabelAccessibleLinkImpl;
typedef struct _GtkLabelAccessibleLinkImplClass GtkLabelAccessibleLinkImplClass;

struct _GtkLabelAccessibleLinkImpl
{
  AtkObject parent;

  GtkLabelAccessibleLink *link;
};

struct _GtkLabelAccessibleLinkImplClass
{
  AtkObjectClass parent_class;
};

/* GtkLabelAccessibleLinkImpl {{{1 */

GType _ctk_label_accessible_link_impl_get_type (void);

static void atk_hyperlink_impl_interface_init (AtkHyperlinkImplIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkLabelAccessibleLinkImpl, _ctk_label_accessible_link_impl, ATK_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_HYPERLINK_IMPL, atk_hyperlink_impl_interface_init))

static AtkHyperlink *
ctk_label_accessible_link_impl_get_hyperlink (AtkHyperlinkImpl *atk_impl)
{
  GtkLabelAccessibleLinkImpl *impl = (GtkLabelAccessibleLinkImpl *)atk_impl;

  return ATK_HYPERLINK (g_object_ref (impl->link));
}

static void
atk_hyperlink_impl_interface_init (AtkHyperlinkImplIface *iface)
{
  iface->get_hyperlink = ctk_label_accessible_link_impl_get_hyperlink;
}

static AtkStateSet *
ctk_label_accessible_link_impl_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  GtkLabelAccessibleLink *link;
  GtkWidget *widget;

  link = ((GtkLabelAccessibleLinkImpl *)obj)->link;

  state_set = atk_object_ref_state_set (atk_object_get_parent (obj));

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_object_get_parent (obj)));
  if (widget)
    {
      if (ctk_widget_get_can_focus (widget))
        {
          atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);
          if (_ctk_label_get_link_focused (CTK_LABEL (widget), link->index))
            atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);
          else
            atk_state_set_remove_state (state_set, ATK_STATE_FOCUSED);
        }

      if (_ctk_label_get_link_visited (CTK_LABEL (widget), link->index))
        atk_state_set_add_state (state_set, ATK_STATE_VISITED);
    }

  return state_set;
}

static void
_ctk_label_accessible_link_impl_init (GtkLabelAccessibleLinkImpl *impl)
{
  atk_object_set_role (ATK_OBJECT (impl), ATK_ROLE_LINK);
}

static void
_ctk_label_accessible_link_impl_finalize (GObject *obj)
{
  GtkLabelAccessibleLinkImpl *impl = (GtkLabelAccessibleLinkImpl *)obj;

  g_object_unref (impl->link);

  G_OBJECT_CLASS (_ctk_label_accessible_link_impl_parent_class)->finalize (obj);
}

static void
_ctk_label_accessible_link_impl_class_init (GtkLabelAccessibleLinkImplClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  AtkObjectClass *atk_obj_class = ATK_OBJECT_CLASS (class);

  object_class->finalize = _ctk_label_accessible_link_impl_finalize;
  atk_obj_class->ref_state_set = ctk_label_accessible_link_impl_ref_state_set;
}

/* 'Public' API {{{2 */

static GtkLabelAccessibleLinkImpl *
ctk_label_accessible_link_impl_new (GtkLabelAccessible *label,
                                    gint                idx)
{
  GtkLabelAccessibleLinkImpl *impl;

  impl = g_object_new (_ctk_label_accessible_link_impl_get_type (), NULL);
  impl->link = ctk_label_accessible_link_new (label, idx);
  atk_object_set_parent (ATK_OBJECT (impl), ATK_OBJECT (label));

  return impl;
}

/* GtkLabelAccessibleLink {{{1 */

GType _ctk_label_accessible_link_get_type (void);

static void atk_action_interface_init (AtkActionIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkLabelAccessibleLink, _ctk_label_accessible_link, ATK_TYPE_HYPERLINK,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init))

static gchar *
ctk_label_accessible_link_get_uri (AtkHyperlink *atk_link,
                                   gint          i)
{
  GtkLabelAccessibleLink *link = (GtkLabelAccessibleLink *)atk_link;
  GtkWidget *widget;
  const gchar *uri;

  g_return_val_if_fail (i == 0, NULL);

  if (link->label == NULL)
    return NULL;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (link->label));
  uri = _ctk_label_get_link_uri (CTK_LABEL (widget), link->index);

  return g_strdup (uri);
}

static gint
ctk_label_accessible_link_get_n_anchors (AtkHyperlink *atk_link)
{
  return 1;
}

static gboolean
ctk_label_accessible_link_is_valid (AtkHyperlink *atk_link)
{
  return TRUE;
}

static AtkObject *
ctk_label_accessible_link_get_object (AtkHyperlink *atk_link,
                                      gint          i)
{
  GtkLabelAccessibleLink *link = (GtkLabelAccessibleLink *)atk_link;

  g_return_val_if_fail (i == 0, NULL);

  return ATK_OBJECT (link->label);
}

static gint
ctk_label_accessible_link_get_start_index (AtkHyperlink *atk_link)
{
  GtkLabelAccessibleLink *link = (GtkLabelAccessibleLink *)atk_link;
  GtkWidget *widget;
  gint start, end;

  if (link->label == NULL)
    return 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (link->label));
  _ctk_label_get_link_extent (CTK_LABEL (widget), link->index, &start, &end);

  return start;
}

static gint
ctk_label_accessible_link_get_end_index (AtkHyperlink *atk_link)
{
  GtkLabelAccessibleLink *link = (GtkLabelAccessibleLink *)atk_link;
  GtkWidget *widget;
  gint start, end;

  if (link->label == NULL)
    return 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (link->label));
  _ctk_label_get_link_extent (CTK_LABEL (widget), link->index, &start, &end);

  return end;
}

static void
_ctk_label_accessible_link_init (GtkLabelAccessibleLink *link)
{
}

static void
_ctk_label_accessible_link_class_init (GtkLabelAccessibleLinkClass *class)
{
  AtkHyperlinkClass *atk_link_class = ATK_HYPERLINK_CLASS (class);

  atk_link_class->get_uri = ctk_label_accessible_link_get_uri;
  atk_link_class->get_n_anchors = ctk_label_accessible_link_get_n_anchors;
  atk_link_class->is_valid = ctk_label_accessible_link_is_valid;
  atk_link_class->get_object = ctk_label_accessible_link_get_object;
  atk_link_class->get_start_index = ctk_label_accessible_link_get_start_index;
  atk_link_class->get_end_index = ctk_label_accessible_link_get_end_index;
}

/* 'Public' API {{{2 */

static GtkLabelAccessibleLink *
ctk_label_accessible_link_new (GtkLabelAccessible *label,
                               gint                idx)
{
  GtkLabelAccessibleLink *link;

  link = g_object_new (_ctk_label_accessible_link_get_type (), NULL);
  link->label = label;
  link->index = idx;

  return link;
}

/* AtkAction implementation {{{2 */

static gboolean
ctk_label_accessible_link_do_action (AtkAction *action,
                                     gint       i)
{
  GtkLabelAccessibleLink *link = (GtkLabelAccessibleLink *)action;
  GtkWidget *widget;

  if (i != 0)
    return FALSE;

  if (link->label == NULL)
    return FALSE;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (link->label));
  if (widget == NULL)
    return FALSE;

  if (!ctk_widget_is_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  _ctk_label_activate_link (CTK_LABEL (widget), link->index);

  return TRUE;
}

static gint
ctk_label_accessible_link_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
ctk_label_accessible_link_get_name (AtkAction *action,
                                    gint       i)
{
  if (i != 0)
    return NULL;

  return "activate";
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_label_accessible_link_do_action;
  iface->get_n_actions = ctk_label_accessible_link_get_n_actions;
  iface->get_name = ctk_label_accessible_link_get_name;
}

/* GtkLabelAccessible {{{1 */

static void atk_text_interface_init       (AtkTextIface      *iface);
static void atk_hypertext_interface_init (AtkHypertextIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkLabelAccessible, ctk_label_accessible, CTK_TYPE_WIDGET_ACCESSIBLE,
                         G_ADD_PRIVATE (GtkLabelAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_TEXT, atk_text_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_HYPERTEXT, atk_hypertext_interface_init))

static void
ctk_label_accessible_init (GtkLabelAccessible *label)
{
  label->priv = ctk_label_accessible_get_instance_private (label);
}

static void
ctk_label_accessible_initialize (AtkObject *obj,
                                 gpointer   data)
{
  GtkWidget  *widget;

  ATK_OBJECT_CLASS (ctk_label_accessible_parent_class)->initialize (obj, data);

  widget = CTK_WIDGET (data);

  _ctk_label_accessible_update_links (CTK_LABEL (widget));

  /* Check whether ancestor of GtkLabel is a GtkButton
   * and if so set accessible parent for GtkLabelAccessible
   */
  while (widget != NULL)
    {
      widget = ctk_widget_get_parent (widget);
      if (CTK_IS_BUTTON (widget))
        {
          atk_object_set_parent (obj, ctk_widget_get_accessible (widget));
          break;
        }
    }

  obj->role = ATK_ROLE_LABEL;
}

static gboolean
check_for_selection_change (GtkLabelAccessible *accessible,
                            GtkLabel           *label)
{
  gboolean ret_val = FALSE;
  gint start, end;

  if (ctk_label_get_selection_bounds (label, &start, &end))
    {
      if (end != accessible->priv->cursor_position ||
          start != accessible->priv->selection_bound)
        ret_val = TRUE;
    }
  else
    {
      ret_val = (accessible->priv->cursor_position != accessible->priv->selection_bound);
    }

  accessible->priv->cursor_position = end;
  accessible->priv->selection_bound = start;

  return ret_val;
}

static void
ctk_label_accessible_notify_gtk (GObject    *obj,
                                 GParamSpec *pspec)
{
  GtkWidget *widget = CTK_WIDGET (obj);
  AtkObject* atk_obj = ctk_widget_get_accessible (widget);
  GtkLabelAccessible *accessible;

  accessible = CTK_LABEL_ACCESSIBLE (atk_obj);

  if (g_strcmp0 (pspec->name, "cursor-position") == 0)
    {
      g_signal_emit_by_name (atk_obj, "text-caret-moved",
                             _ctk_label_get_cursor_position (CTK_LABEL (widget)));
      if (check_for_selection_change (accessible, CTK_LABEL (widget)))
        g_signal_emit_by_name (atk_obj, "text-selection-changed");
    }
  else if (g_strcmp0 (pspec->name, "selection-bound") == 0)
    {
      if (check_for_selection_change (accessible, CTK_LABEL (widget)))
        g_signal_emit_by_name (atk_obj, "text-selection-changed");
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_label_accessible_parent_class)->notify_gtk (obj, pspec);
}

/* atkobject.h */

static AtkStateSet *
ctk_label_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (ctk_label_accessible_parent_class)->ref_state_set (accessible);
  atk_state_set_add_state (state_set, ATK_STATE_MULTI_LINE);

  return state_set;
}

static AtkRelationSet *
ctk_label_accessible_ref_relation_set (AtkObject *obj)
{
  GtkWidget *widget;
  AtkRelationSet *relation_set;

  g_return_val_if_fail (CTK_IS_LABEL_ACCESSIBLE (obj), NULL);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  relation_set = ATK_OBJECT_CLASS (ctk_label_accessible_parent_class)->ref_relation_set (obj);

  if (!atk_relation_set_contains (relation_set, ATK_RELATION_LABEL_FOR))
    {
      /* Get the mnemonic widget.
       * The relation set is not updated if the mnemonic widget is changed
       */
      GtkWidget *mnemonic_widget;

      mnemonic_widget = ctk_label_get_mnemonic_widget (CTK_LABEL (widget));

      if (mnemonic_widget)
        {
          AtkObject *accessible_array[1];
          AtkRelation* relation;

          if (!ctk_widget_get_can_focus (mnemonic_widget))
            {
            /*
             * Handle the case where a GtkFileChooserButton is specified
             * as the mnemonic widget. use the combobox which is a child of the
             * GtkFileChooserButton as the mnemonic widget. See bug #359843.
             */
             if (CTK_IS_BOX (mnemonic_widget))
               {
                  GList *list, *tmpl;

                  list = ctk_container_get_children (CTK_CONTAINER (mnemonic_widget));
                  if (g_list_length (list) == 2)
                    {
                      tmpl = g_list_last (list);
                      if (CTK_IS_COMBO_BOX(tmpl->data))
                        {
                          mnemonic_widget = CTK_WIDGET(tmpl->data);
                        }
                    }
                  g_list_free (list);
                }
            }
          accessible_array[0] = ctk_widget_get_accessible (mnemonic_widget);
          relation = atk_relation_new (accessible_array, 1,
                                       ATK_RELATION_LABEL_FOR);
          atk_relation_set_add (relation_set, relation);
          /*
           * Unref the relation so that it is not leaked.
           */
          g_object_unref (relation);
        }
    }
  return relation_set;
}

static const gchar*
ctk_label_accessible_get_name (AtkObject *accessible)
{
  const gchar *name;

  g_return_val_if_fail (CTK_IS_LABEL_ACCESSIBLE (accessible), NULL);

  name = ATK_OBJECT_CLASS (ctk_label_accessible_parent_class)->get_name (accessible);
  if (name != NULL)
    return name;
  else
    {
      /*
       * Get the text on the label
       */
      GtkWidget *widget;

      widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
      if (widget == NULL)
        return NULL;

      g_return_val_if_fail (CTK_IS_LABEL (widget), NULL);

      return ctk_label_get_text (CTK_LABEL (widget));
    }
}

static gint
ctk_label_accessible_get_n_children (AtkObject *obj)
{
  GtkLabelAccessible *accessible = CTK_LABEL_ACCESSIBLE (obj);

  return g_list_length (accessible->priv->links);
}

static AtkObject *
ctk_label_accessible_ref_child (AtkObject *obj,
                                gint       idx)
{
  GtkLabelAccessible *accessible = CTK_LABEL_ACCESSIBLE (obj);
  AtkObject *child;

  child = g_list_nth_data (accessible->priv->links, idx);

  if (child)
    g_object_ref (child);

  return child;
}

static void clear_links (GtkLabelAccessible *accessible);

static void
ctk_label_accessible_finalize (GObject *obj)
{
  GtkLabelAccessible *accessible = CTK_LABEL_ACCESSIBLE (obj);

  clear_links (accessible);

  G_OBJECT_CLASS (ctk_label_accessible_parent_class)->finalize (obj);
}

static void
ctk_label_accessible_class_init (GtkLabelAccessibleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  GtkWidgetAccessibleClass *widget_class = CTK_WIDGET_ACCESSIBLE_CLASS (klass);

  object_class->finalize = ctk_label_accessible_finalize;

  class->get_name = ctk_label_accessible_get_name;
  class->ref_state_set = ctk_label_accessible_ref_state_set;
  class->ref_relation_set = ctk_label_accessible_ref_relation_set;
  class->initialize = ctk_label_accessible_initialize;

  class->get_n_children = ctk_label_accessible_get_n_children;
  class->ref_child = ctk_label_accessible_ref_child;

  widget_class->notify_gtk = ctk_label_accessible_notify_gtk;
}

/* 'Public' API {{{2 */

void
_ctk_label_accessible_text_deleted (GtkLabel *label)
{
  AtkObject *obj;
  const char *text;
  guint length;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (label));
  if (obj == NULL)
    return;

  text = ctk_label_get_text (label);
  length = g_utf8_strlen (text, -1);
  if (length > 0)
    g_signal_emit_by_name (obj, "text-changed::delete", 0, length);
}

void
_ctk_label_accessible_text_inserted (GtkLabel *label)
{
  AtkObject *obj;
  const char *text;
  guint length;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (label));
  if (obj == NULL)
    return;

  text = ctk_label_get_text (label);
  length = g_utf8_strlen (text, -1);
  if (length > 0)
    g_signal_emit_by_name (obj, "text-changed::insert", 0, length);

  if (obj->name == NULL)
    /* The label has changed so notify a change in accessible-name */
    g_object_notify (G_OBJECT (obj), "accessible-name");

  g_signal_emit_by_name (obj, "visible-data-changed");
}

static void
clear_links (GtkLabelAccessible *accessible)
{
  GList *l;
  gint i;
  GtkLabelAccessibleLinkImpl *impl;

  for (l = accessible->priv->links, i = 0; l; l = l->next, i++)
    {
      impl = l->data;
      g_signal_emit_by_name (accessible, "children-changed::remove", i, impl, NULL);
      atk_object_set_parent (ATK_OBJECT (impl), NULL);
      impl->link->label = NULL;
    }
  g_list_free_full (accessible->priv->links, g_object_unref);
  accessible->priv->links = NULL;
}

static void
create_links (GtkLabelAccessible *accessible)
{
  GtkWidget *widget;
  gint n, i;
  GtkLabelAccessibleLinkImpl *impl;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));

  n = _ctk_label_get_n_links (CTK_LABEL (widget));
  for (i = 0; i < n; i++)
    {
      impl = ctk_label_accessible_link_impl_new (accessible, i);
      accessible->priv->links = g_list_append (accessible->priv->links, impl);
      g_signal_emit_by_name (accessible, "children-changed::add", i, impl, NULL);
    }
}

void
_ctk_label_accessible_update_links (GtkLabel *label)
{
  AtkObject *obj;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (label));
  if (obj == NULL)
    return;

  clear_links (CTK_LABEL_ACCESSIBLE (obj));
  create_links (CTK_LABEL_ACCESSIBLE (obj));
}

void
_ctk_label_accessible_focus_link_changed (GtkLabel *label)
{
  AtkObject *obj;
  GtkLabelAccessible *accessible;
  GList *l;
  GtkLabelAccessibleLinkImpl *impl;
  gboolean focused;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (label));
  if (obj == NULL)
    return;

  accessible = CTK_LABEL_ACCESSIBLE (obj);

  for (l = accessible->priv->links; l; l = l->next)
    {
      impl = l->data;
      focused = _ctk_label_get_link_focused (label, impl->link->index);
      if (impl->link->focused != focused)
        {
          impl->link->focused = focused;
          atk_object_notify_state_change (ATK_OBJECT (impl), ATK_STATE_FOCUSED, focused);
        }
    }
}

/* AtkText implementation {{{2 */

static gchar*
ctk_label_accessible_get_text (AtkText *atk_text,
                               gint     start_pos,
                               gint     end_pos)
{
  GtkWidget *widget;
  const gchar *text;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return NULL;

  text = ctk_label_get_text (CTK_LABEL (widget));

  if (text)
    {
      guint length;
      const gchar *start, *end;

      length = g_utf8_strlen (text, -1);
      if (end_pos < 0 || end_pos > length)
        end_pos = length;
      if (start_pos > length)
        start_pos = length;
      if (end_pos <= start_pos)
        return g_strdup ("");
      start = g_utf8_offset_to_pointer (text, start_pos);
      end = g_utf8_offset_to_pointer (start, end_pos - start_pos);
      return g_strndup (start, end - start);
    }

  return NULL;
}

static gchar *
ctk_label_accessible_get_text_before_offset (AtkText         *text,
                                             gint             offset,
                                             AtkTextBoundary  boundary_type,
                                             gint            *start_offset,
                                             gint            *end_offset)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  return _ctk_pango_get_text_before (ctk_label_get_layout (CTK_LABEL (widget)),
                                     boundary_type, offset,
                                     start_offset, end_offset);
}

static gchar*
ctk_label_accessible_get_text_at_offset (AtkText         *text,
                                         gint             offset,
                                         AtkTextBoundary  boundary_type,
                                         gint            *start_offset,
                                         gint            *end_offset)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  return _ctk_pango_get_text_at (ctk_label_get_layout (CTK_LABEL (widget)),
                                 boundary_type, offset,
                                 start_offset, end_offset);
}

static gchar*
ctk_label_accessible_get_text_after_offset (AtkText         *text,
                                            gint             offset,
                                            AtkTextBoundary  boundary_type,
                                            gint            *start_offset,
                                            gint            *end_offset)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  return _ctk_pango_get_text_after (ctk_label_get_layout (CTK_LABEL (widget)),
                                    boundary_type, offset,
                                    start_offset, end_offset);
}

static gint
ctk_label_accessible_get_character_count (AtkText *atk_text)
{
  GtkWidget *widget;
  const gchar *text;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return 0;

  text = ctk_label_get_text (CTK_LABEL (widget));

  if (text)
    return g_utf8_strlen (text, -1);

  return 0;
}

static gint
ctk_label_accessible_get_caret_offset (AtkText *text)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return 0;

  return _ctk_label_get_cursor_position (CTK_LABEL (widget));
}

static gboolean
ctk_label_accessible_set_caret_offset (AtkText *text,
                                       gint     offset)
{
  GtkWidget *widget;
  GtkLabel *label;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  label = CTK_LABEL (widget);

  if (!ctk_label_get_selectable (label))
    return FALSE;

  ctk_label_select_region (label, offset, offset);

  return TRUE;
}

static gint
ctk_label_accessible_get_n_selections (AtkText *text)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return 0;

  if (ctk_label_get_selection_bounds (CTK_LABEL (widget), NULL, NULL))
    return 1;

  return 0;
}

static gchar *
ctk_label_accessible_get_selection (AtkText *atk_text,
                                    gint     selection_num,
                                    gint    *start_pos,
                                    gint    *end_pos)
{
  GtkWidget *widget;
  GtkLabel  *label;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return NULL;

  if (selection_num != 0)
    return NULL;

  label = CTK_LABEL (widget);

  if (ctk_label_get_selection_bounds (label, start_pos, end_pos))
    {
      const gchar *text;

      text = ctk_label_get_text (label);

      if (text)
        return g_utf8_substring (text, *start_pos, *end_pos);
    }

  return NULL;
}

static gboolean
ctk_label_accessible_add_selection (AtkText *text,
                                    gint     start_pos,
                                    gint     end_pos)
{
  GtkWidget *widget;
  GtkLabel  *label;
  gint start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  label = CTK_LABEL (widget);

  if (!ctk_label_get_selectable (label))
    return FALSE;

  if (!ctk_label_get_selection_bounds (label, &start, &end))
    {
      ctk_label_select_region (label, start_pos, end_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_label_accessible_remove_selection (AtkText *text,
                                       gint     selection_num)
{
  GtkWidget *widget;
  GtkLabel  *label;
  gint start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  label = CTK_LABEL (widget);

  if (!ctk_label_get_selectable (label))
     return FALSE;

  if (ctk_label_get_selection_bounds (label, &start, &end))
    {
      ctk_label_select_region (label, end, end);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_label_accessible_set_selection (AtkText *text,
                                    gint     selection_num,
                                    gint     start_pos,
                                    gint     end_pos)
{
  GtkWidget *widget;
  GtkLabel  *label;
  gint start, end;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return FALSE;

  if (selection_num != 0)
    return FALSE;

  label = CTK_LABEL (widget);

  if (!ctk_label_get_selectable (label))
    return FALSE;

  if (ctk_label_get_selection_bounds (label, &start, &end))
    {
      ctk_label_select_region (label, start_pos, end_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_label_accessible_get_character_extents (AtkText      *text,
                                            gint          offset,
                                            gint         *x,
                                            gint         *y,
                                            gint         *width,
                                            gint         *height,
                                            AtkCoordType  coords)
{
  GtkWidget *widget;
  GtkLabel *label;
  PangoRectangle char_rect;
  const gchar *label_text;
  gint index, x_layout, y_layout;
  GdkWindow *window;
  gint x_window, y_window;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return;

  label = CTK_LABEL (widget);

  ctk_label_get_layout_offsets (label, &x_layout, &y_layout);
  label_text = ctk_label_get_text (label);
  index = g_utf8_offset_to_pointer (label_text, offset) - label_text;
  pango_layout_index_to_pos (ctk_label_get_layout (label), index, &char_rect);
  pango_extents_to_pixels (&char_rect, NULL);

  window = ctk_widget_get_window (widget);
  gdk_window_get_origin (window, &x_window, &y_window);

  *x = x_window + x_layout + char_rect.x;
  *y = y_window + y_layout + char_rect.y;
  *width = char_rect.width;
  *height = char_rect.height;

  if (coords == ATK_XY_WINDOW)
    {
      window = gdk_window_get_toplevel (window);
      gdk_window_get_origin (window, &x_window, &y_window);

      *x -= x_window;
      *y -= y_window;
    }
}

static gint
ctk_label_accessible_get_offset_at_point (AtkText      *atk_text,
                                          gint          x,
                                          gint          y,
                                          AtkCoordType  coords)
{
  GtkWidget *widget;
  GtkLabel *label;
  const gchar *text;
  gint index, x_layout, y_layout;
  gint x_window, y_window;
  gint x_local, y_local;
  GdkWindow *window;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return -1;

  label = CTK_LABEL (widget);

  ctk_label_get_layout_offsets (label, &x_layout, &y_layout);

  window = ctk_widget_get_window (widget);
  gdk_window_get_origin (window, &x_window, &y_window);

  x_local = x - x_layout - x_window;
  y_local = y - y_layout - y_window;

  if (coords == ATK_XY_WINDOW)
    {
      window = gdk_window_get_toplevel (window);
      gdk_window_get_origin (window, &x_window, &y_window);

      x_local += x_window;
      y_local += y_window;
    }

  if (!pango_layout_xy_to_index (ctk_label_get_layout (label),
                                 x_local * PANGO_SCALE,
                                 y_local * PANGO_SCALE,
                                 &index, NULL))
    {
      if (x_local < 0 || y_local < 0)
        index = 0;
      else
        index = -1;
    }

  if (index != -1)
    {
      text = ctk_label_get_text (label);
      return g_utf8_pointer_to_offset (text, text + index);
    }

  return -1;
}

static AtkAttributeSet *
add_attribute (AtkAttributeSet  *attributes,
               AtkTextAttribute  attr,
               const gchar      *value)
{
  AtkAttribute *at;

  at = g_new (AtkAttribute, 1);
  at->name = g_strdup (atk_text_attribute_get_name (attr));
  at->value = g_strdup (value);

  return g_slist_prepend (attributes, at);
}

static AtkAttributeSet*
ctk_label_accessible_get_run_attributes (AtkText *text,
                                         gint     offset,
                                         gint    *start_offset,
                                         gint    *end_offset)
{
  GtkWidget *widget;
  AtkAttributeSet *attributes;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  attributes = NULL;
  attributes = add_attribute (attributes, ATK_TEXT_ATTR_DIRECTION,
                   atk_text_attribute_get_value (ATK_TEXT_ATTR_DIRECTION,
                                                 ctk_widget_get_direction (widget)));
  attributes = _ctk_pango_get_run_attributes (attributes,
                                              ctk_label_get_layout (CTK_LABEL (widget)),
                                              offset,
                                              start_offset,
                                              end_offset);

  return attributes;
}

static AtkAttributeSet *
ctk_label_accessible_get_default_attributes (AtkText *text)
{
  GtkWidget *widget;
  AtkAttributeSet *attributes;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (text));
  if (widget == NULL)
    return NULL;

  attributes = NULL;
  attributes = add_attribute (attributes, ATK_TEXT_ATTR_DIRECTION,
                   atk_text_attribute_get_value (ATK_TEXT_ATTR_DIRECTION,
                                                 ctk_widget_get_direction (widget)));
  attributes = _ctk_pango_get_default_attributes (attributes,
                                                  ctk_label_get_layout (CTK_LABEL (widget)));
  attributes = _ctk_style_context_get_attributes (attributes,
                                                  ctk_widget_get_style_context (widget),
                                                  ctk_widget_get_state_flags (widget));

  return attributes;
}

static gunichar
ctk_label_accessible_get_character_at_offset (AtkText *atk_text,
                                              gint     offset)
{
  GtkWidget *widget;
  const gchar *text;
  gchar *index;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (atk_text));
  if (widget == NULL)
    return '\0';

  text = ctk_label_get_text (CTK_LABEL (widget));
  if (offset >= g_utf8_strlen (text, -1))
    return '\0';

  index = g_utf8_offset_to_pointer (text, offset);

  return g_utf8_get_char (index);
}

static void
atk_text_interface_init (AtkTextIface *iface)
{
  iface->get_text = ctk_label_accessible_get_text;
  iface->get_character_at_offset = ctk_label_accessible_get_character_at_offset;
  iface->get_text_before_offset = ctk_label_accessible_get_text_before_offset;
  iface->get_text_at_offset = ctk_label_accessible_get_text_at_offset;
  iface->get_text_after_offset = ctk_label_accessible_get_text_after_offset;
  iface->get_character_count = ctk_label_accessible_get_character_count;
  iface->get_caret_offset = ctk_label_accessible_get_caret_offset;
  iface->set_caret_offset = ctk_label_accessible_set_caret_offset;
  iface->get_n_selections = ctk_label_accessible_get_n_selections;
  iface->get_selection = ctk_label_accessible_get_selection;
  iface->add_selection = ctk_label_accessible_add_selection;
  iface->remove_selection = ctk_label_accessible_remove_selection;
  iface->set_selection = ctk_label_accessible_set_selection;
  iface->get_character_extents = ctk_label_accessible_get_character_extents;
  iface->get_offset_at_point = ctk_label_accessible_get_offset_at_point;
  iface->get_run_attributes = ctk_label_accessible_get_run_attributes;
  iface->get_default_attributes = ctk_label_accessible_get_default_attributes;
}

/* AtkHypertext implementation {{{2 */

static AtkHyperlink *
ctk_label_accessible_get_link (AtkHypertext *hypertext,
                               gint          idx)
{
  GtkLabelAccessible *label = CTK_LABEL_ACCESSIBLE (hypertext);
  GtkLabelAccessibleLinkImpl *impl;

  impl = (GtkLabelAccessibleLinkImpl *)g_list_nth_data (label->priv->links, idx);

  if (impl)
    return ATK_HYPERLINK (impl->link);

  return NULL;
}

static gint
ctk_label_accessible_get_n_links (AtkHypertext *hypertext)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (hypertext));

  return _ctk_label_get_n_links (CTK_LABEL (widget));
}

static gint
ctk_label_accessible_get_link_index (AtkHypertext *hypertext,
                                     gint          char_index)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (hypertext));

  return _ctk_label_get_link_at (CTK_LABEL (widget), char_index);
}

static void
atk_hypertext_interface_init (AtkHypertextIface *iface)
{
  iface->get_link = ctk_label_accessible_get_link;
  iface->get_n_links = ctk_label_accessible_get_n_links;
  iface->get_link_index = ctk_label_accessible_get_link_index;
}

/* vim:set foldmethod=marker expandtab: */
