/* GTK+ - accessibility implementations
 * Copyright 2011 Red Hat, Inc.
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

#include <gtk/gtk.h>
#include "gtklinkbuttonaccessible.h"

typedef struct _GtkLinkButtonAccessibleLink GtkLinkButtonAccessibleLink;
typedef struct _GtkLinkButtonAccessibleLinkClass GtkLinkButtonAccessibleLinkClass;

struct _GtkLinkButtonAccessiblePrivate
{
  AtkHyperlink *link;
};

struct _GtkLinkButtonAccessibleLink
{
  AtkHyperlink parent;

  GtkLinkButtonAccessible *button;
};

struct _GtkLinkButtonAccessibleLinkClass
{
  AtkHyperlinkClass parent_class;
};

static void atk_action_interface_init (AtkActionIface *iface);

GType _ctk_link_button_accessible_link_get_type (void);

G_DEFINE_TYPE_WITH_CODE (GtkLinkButtonAccessibleLink, _ctk_link_button_accessible_link, ATK_TYPE_HYPERLINK,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init))

static AtkHyperlink *
ctk_link_button_accessible_link_new (GtkLinkButtonAccessible *button)
{
  GtkLinkButtonAccessibleLink *l;

  l = g_object_new (_ctk_link_button_accessible_link_get_type (), NULL);
  l->button = button;

  return ATK_HYPERLINK (l);
}

static gchar *
ctk_link_button_accessible_link_get_uri (AtkHyperlink *atk_link,
                                         gint          i)
{
  GtkLinkButtonAccessibleLink *l = (GtkLinkButtonAccessibleLink *)atk_link;
  GtkWidget *widget;
  const gchar *uri;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (l->button));
  uri = ctk_link_button_get_uri (CTK_LINK_BUTTON (widget));

  return g_strdup (uri);
}

static gint
ctk_link_button_accessible_link_get_n_anchors (AtkHyperlink *atk_link)
{
  return 1;
}

static gboolean
ctk_link_button_accessible_link_is_valid (AtkHyperlink *atk_link)
{
  return TRUE;
}

static AtkObject *
ctk_link_button_accessible_link_get_object (AtkHyperlink *atk_link,
                                            gint          i)
{
  GtkLinkButtonAccessibleLink *l = (GtkLinkButtonAccessibleLink *)atk_link;

  return ATK_OBJECT (l->button);
}

static void
_ctk_link_button_accessible_link_init (GtkLinkButtonAccessibleLink *l)
{
}

static void
_ctk_link_button_accessible_link_class_init (GtkLinkButtonAccessibleLinkClass *class)
{
  AtkHyperlinkClass *atk_link_class = ATK_HYPERLINK_CLASS (class);

  atk_link_class->get_uri = ctk_link_button_accessible_link_get_uri;
  atk_link_class->get_n_anchors = ctk_link_button_accessible_link_get_n_anchors;
  atk_link_class->is_valid = ctk_link_button_accessible_link_is_valid;
  atk_link_class->get_object = ctk_link_button_accessible_link_get_object;
}

static gboolean
ctk_link_button_accessible_link_do_action (AtkAction *action,
                                           gint       i)
{
  GtkLinkButtonAccessibleLink *l = (GtkLinkButtonAccessibleLink *)action;
  GtkWidget *widget;

  widget = CTK_WIDGET (l->button);
  if (widget == NULL)
    return FALSE;

  if (i != 0)
    return FALSE;

  if (!ctk_widget_is_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  ctk_button_clicked (CTK_BUTTON (widget));

  return TRUE;
}

static gint
ctk_link_button_accessible_link_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
ctk_link_button_accessible_link_get_name (AtkAction *action,
                                          gint       i)
{
  if (i != 0)
    return NULL;

  return "activate";
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_link_button_accessible_link_do_action;
  iface->get_n_actions = ctk_link_button_accessible_link_get_n_actions;
  iface->get_name = ctk_link_button_accessible_link_get_name;
}

static gboolean
activate_link (GtkLinkButton *button,
               AtkHyperlink  *atk_link)
{
  g_signal_emit_by_name (atk_link, "link-activated");

  return FALSE;
}

static AtkHyperlink *
ctk_link_button_accessible_get_hyperlink (AtkHyperlinkImpl *impl)
{
  GtkLinkButtonAccessible *button = CTK_LINK_BUTTON_ACCESSIBLE (impl);

  if (!button->priv->link)
    {
      button->priv->link = ctk_link_button_accessible_link_new (button);
      g_signal_connect (ctk_accessible_get_widget (CTK_ACCESSIBLE (button)),
                        "activate-link", G_CALLBACK (activate_link), button->priv->link);
    }

  return g_object_ref (button->priv->link);
}

static void atk_hypertext_impl_interface_init (AtkHyperlinkImplIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkLinkButtonAccessible, ctk_link_button_accessible, CTK_TYPE_BUTTON_ACCESSIBLE,
                         G_ADD_PRIVATE (GtkLinkButtonAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_HYPERLINK_IMPL, atk_hypertext_impl_interface_init))

static void
ctk_link_button_accessible_init (GtkLinkButtonAccessible *button)
{
  button->priv = ctk_link_button_accessible_get_instance_private (button);
}

static void
ctk_link_button_accessible_finalize (GObject *object)
{
  GtkLinkButtonAccessible *button = CTK_LINK_BUTTON_ACCESSIBLE (object);

  if (button->priv->link)
    g_object_unref (button->priv->link);

  G_OBJECT_CLASS (ctk_link_button_accessible_parent_class)->finalize (object);
}

static AtkStateSet *
ctk_link_button_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  GtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_link_button_accessible_parent_class)->ref_state_set (accessible);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget != NULL)
    {
      if (ctk_link_button_get_visited (CTK_LINK_BUTTON (widget)))
        atk_state_set_add_state (state_set, ATK_STATE_VISITED);
    }

  return state_set;
}

static void
ctk_link_button_accessible_class_init (GtkLinkButtonAccessibleClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = ctk_link_button_accessible_finalize;
  ATK_OBJECT_CLASS (klass)->ref_state_set = ctk_link_button_ref_state_set;
}

static void
atk_hypertext_impl_interface_init (AtkHyperlinkImplIface *iface)
{
  iface->get_hyperlink = ctk_link_button_accessible_get_hyperlink;
}
