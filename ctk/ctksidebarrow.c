/* ctksidebarrow.c
 *
 * Copyright (C) 2015 Carlos Soriano <csoriano@gnome.org>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctksidebarrowprivate.h"
/* For section and place type enums */
#include "ctkplacessidebarprivate.h"
#include "ctkplacessidebar.h"
#include "ctkwidget.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctkstylecontext.h"
#include "ctkrevealer.h"
#include "ctkselection.h"
#include "ctkspinner.h"

#ifdef HAVE_CLOUDPROVIDERS
#include <cloudproviders/cloudprovidersaccount.h>
#endif

struct _CtkSidebarRow
{
  CtkListBoxRow parent_instance;
  GIcon *start_icon;
  GIcon *end_icon;
  CtkWidget *start_icon_widget;
  CtkWidget *end_icon_widget;
  gchar *label;
  gchar *tooltip;
  CtkWidget *label_widget;
  gboolean ejectable;
  CtkWidget *eject_button;
  gint order_index;
  CtkPlacesSidebarSectionType section_type;
  CtkPlacesSidebarPlaceType place_type;
  gchar *uri;
  GDrive *drive;
  GVolume *volume;
  GMount *mount;
  GObject *cloud_provider_account;
  gboolean placeholder;
  CtkPlacesSidebar *sidebar;
  CtkWidget *event_box;
  CtkWidget *revealer;
  CtkWidget *busy_spinner;
};

G_DEFINE_TYPE (CtkSidebarRow, ctk_sidebar_row, CTK_TYPE_LIST_BOX_ROW)

enum
{
  PROP_0,
  PROP_START_ICON,
  PROP_END_ICON,
  PROP_LABEL,
  PROP_TOOLTIP,
  PROP_EJECTABLE,
  PROP_SIDEBAR,
  PROP_ORDER_INDEX,
  PROP_SECTION_TYPE,
  PROP_PLACE_TYPE,
  PROP_URI,
  PROP_DRIVE,
  PROP_VOLUME,
  PROP_MOUNT,
  PROP_CLOUD_PROVIDER_ACCOUNT,
  PROP_PLACEHOLDER,
  LAST_PROP
};

static GParamSpec *properties [LAST_PROP];

#ifdef HAVE_CLOUDPROVIDERS

static void
cloud_row_update (CtkSidebarRow *self)
{
  CloudProvidersAccount *account;
  GIcon *end_icon;
  gint provider_status;

  account = CLOUD_PROVIDERS_ACCOUNT (self->cloud_provider_account);
  provider_status = cloud_providers_account_get_status (account);
  switch (provider_status)
    {
      case CLOUD_PROVIDERS_ACCOUNT_STATUS_IDLE:
        end_icon = NULL;
        break;

      case CLOUD_PROVIDERS_ACCOUNT_STATUS_SYNCING:
        end_icon = g_themed_icon_new ("emblem-synchronizing-symbolic");
        break;

      case CLOUD_PROVIDERS_ACCOUNT_STATUS_ERROR:
        end_icon = g_themed_icon_new ("dialog-warning-symbolic");
        break;

      default:
        return;
    }

  g_object_set (self,
                "label", cloud_providers_account_get_name (account),
                NULL);
  g_object_set (self,
                "tooltip", cloud_providers_account_get_status_details (account),
                NULL);
  g_object_set (self,
                "end-icon", end_icon,
                NULL);

  if (end_icon != NULL)
    g_object_unref (end_icon);
}

#endif

static void
ctk_sidebar_row_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkSidebarRow *self = CTK_SIDEBAR_ROW (object);

  switch (prop_id)
    {
    case PROP_SIDEBAR:
      g_value_set_object (value, self->sidebar);
      break;

    case PROP_START_ICON:
      g_value_set_object (value, self->start_icon);
      break;

    case PROP_END_ICON:
      g_value_set_object (value, self->end_icon);
      break;

    case PROP_LABEL:
      g_value_set_string (value, self->label);
      break;

    case PROP_TOOLTIP:
      g_value_set_string (value, self->tooltip);
      break;

    case PROP_EJECTABLE:
      g_value_set_boolean (value, self->ejectable);
      break;

    case PROP_ORDER_INDEX:
      g_value_set_int (value, self->order_index);
      break;

    case PROP_SECTION_TYPE:
      g_value_set_int (value, self->section_type);
      break;

    case PROP_PLACE_TYPE:
      g_value_set_int (value, self->place_type);
      break;

    case PROP_URI:
      g_value_set_string (value, self->uri);
      break;

    case PROP_DRIVE:
      g_value_set_object (value, self->drive);
      break;

    case PROP_VOLUME:
      g_value_set_object (value, self->volume);
      break;

    case PROP_MOUNT:
      g_value_set_object (value, self->mount);
      break;

    case PROP_CLOUD_PROVIDER_ACCOUNT:
      g_value_set_object (value, self->cloud_provider_account);
      break;

    case PROP_PLACEHOLDER:
      g_value_set_boolean (value, self->placeholder);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_sidebar_row_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkSidebarRow *self = CTK_SIDEBAR_ROW (object);
  CtkStyleContext *context;

  switch (prop_id)
    {
    case PROP_SIDEBAR:
      self->sidebar = g_value_get_object (value);
      break;

    case PROP_START_ICON:
      {
        g_clear_object (&self->start_icon);
        object = g_value_get_object (value);
        if (object != NULL)
          {
            self->start_icon = G_ICON (g_object_ref (object));
            ctk_image_set_from_gicon (CTK_IMAGE (self->start_icon_widget),
                                      self->start_icon,
                                      CTK_ICON_SIZE_MENU);
          }
        else
          {
            ctk_image_clear (CTK_IMAGE (self->start_icon_widget));
          }
        break;
      }

    case PROP_END_ICON:
      {
        g_clear_object (&self->end_icon);
        object = g_value_get_object (value);
        if (object != NULL)
          {
            self->end_icon = G_ICON (g_object_ref (object));
            ctk_image_set_from_gicon (CTK_IMAGE (self->end_icon_widget),
                                      self->end_icon,
                                      CTK_ICON_SIZE_MENU);
            ctk_widget_show (self->end_icon_widget);
          }
        else
          {
            ctk_image_clear (CTK_IMAGE (self->end_icon_widget));
            ctk_widget_hide (self->end_icon_widget);
          }
        break;
      }

    case PROP_LABEL:
      g_free (self->label);
      self->label = g_strdup (g_value_get_string (value));
      ctk_label_set_text (CTK_LABEL (self->label_widget), self->label);
      break;

    case PROP_TOOLTIP:
      g_free (self->tooltip);
      self->tooltip = g_strdup (g_value_get_string (value));
      ctk_widget_set_tooltip_text (CTK_WIDGET (self), self->tooltip);
      break;

    case PROP_EJECTABLE:
      self->ejectable = g_value_get_boolean (value);
      if (self->ejectable)
        ctk_widget_show (self->eject_button);
      else
        ctk_widget_hide (self->eject_button);
      break;

    case PROP_ORDER_INDEX:
      self->order_index = g_value_get_int (value);
      break;

    case PROP_SECTION_TYPE:
      self->section_type = g_value_get_int (value);
      if (self->section_type == SECTION_COMPUTER ||
          self->section_type == SECTION_OTHER_LOCATIONS)
        ctk_label_set_ellipsize (CTK_LABEL (self->label_widget), PANGO_ELLIPSIZE_NONE);
      else
        ctk_label_set_ellipsize (CTK_LABEL (self->label_widget), PANGO_ELLIPSIZE_END);
      break;

    case PROP_PLACE_TYPE:
      self->place_type = g_value_get_int (value);
      break;

    case PROP_URI:
      g_free (self->uri);
      self->uri = g_strdup (g_value_get_string (value));
      break;

    case PROP_DRIVE:
      g_set_object (&self->drive, g_value_get_object (value));
      break;

    case PROP_VOLUME:
      g_set_object (&self->volume, g_value_get_object (value));
      break;

    case PROP_MOUNT:
      g_set_object (&self->mount, g_value_get_object (value));
      break;

    case PROP_CLOUD_PROVIDER_ACCOUNT:
#ifdef HAVE_CLOUDPROVIDERS
      if (self->cloud_provider_account != NULL)
        g_signal_handlers_disconnect_by_data (self->cloud_provider_account, self);

      self->cloud_provider_account = g_value_dup_object (value);

      if (self->cloud_provider_account != NULL)
        {
          g_signal_connect_swapped (self->cloud_provider_account, "notify::name",
                                    G_CALLBACK (cloud_row_update), self);
          g_signal_connect_swapped (self->cloud_provider_account, "notify::status",
                                    G_CALLBACK (cloud_row_update), self);
          g_signal_connect_swapped (self->cloud_provider_account, "notify::status-details",
                                    G_CALLBACK (cloud_row_update), self);
        }
#endif
      break;

    case PROP_PLACEHOLDER:
      {
        self->placeholder = g_value_get_boolean (value);
        if (self->placeholder)
          {
            g_clear_object (&self->start_icon);
            g_clear_object (&self->end_icon);
            g_free (self->label);
            self->label = NULL;
            g_free (self->tooltip);
            self->tooltip = NULL;
            ctk_widget_set_tooltip_text (CTK_WIDGET (self), NULL);
            self->ejectable = FALSE;
            self->section_type = SECTION_BOOKMARKS;
            self->place_type = PLACES_BOOKMARK_PLACEHOLDER;
            g_free (self->uri);
            self->uri = NULL;
            g_clear_object (&self->drive);
            g_clear_object (&self->volume);
            g_clear_object (&self->mount);
            g_clear_object (&self->cloud_provider_account);

            ctk_container_foreach (CTK_CONTAINER (self),
                                   (CtkCallback) ctk_widget_destroy,
                                   NULL);

            context = ctk_widget_get_style_context (CTK_WIDGET (self));
            ctk_style_context_add_class (context, "sidebar-placeholder-row");
          }

        break;
      }

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_child_revealed (GObject    *self,
                   GParamSpec *pspec G_GNUC_UNUSED,
                   gpointer    user_data)
{
 /* We need to hide the actual widget because if not the CtkListBoxRow will
  * still allocate the paddings, even if the revealer is not revealed, and
  * therefore the row will be still somewhat visible. */
  if (!ctk_revealer_get_reveal_child (CTK_REVEALER (self)))
    ctk_widget_hide (CTK_WIDGET (CTK_SIDEBAR_ROW (user_data)));
}

void
ctk_sidebar_row_reveal (CtkSidebarRow *self)
{
  ctk_widget_show_all (CTK_WIDGET (self));
  ctk_revealer_set_reveal_child (CTK_REVEALER (self->revealer), TRUE);
}

void
ctk_sidebar_row_hide (CtkSidebarRow *self,
                      gboolean       inmediate)
{
  guint transition_duration;

  transition_duration = ctk_revealer_get_transition_duration (CTK_REVEALER (self->revealer));
  if (inmediate)
      ctk_revealer_set_transition_duration (CTK_REVEALER (self->revealer), 0);

  ctk_revealer_set_reveal_child (CTK_REVEALER (self->revealer), FALSE);

  ctk_revealer_set_transition_duration (CTK_REVEALER (self->revealer), transition_duration);
}

void
ctk_sidebar_row_set_start_icon (CtkSidebarRow *self,
                                GIcon         *icon)
{
  g_return_if_fail (CTK_IS_SIDEBAR_ROW (self));

  if (self->start_icon != icon)
    {
      g_set_object (&self->start_icon, icon);
      if (self->start_icon != NULL)
        ctk_image_set_from_gicon (CTK_IMAGE (self->start_icon_widget), self->start_icon,
                                  CTK_ICON_SIZE_MENU);
      else
        ctk_image_clear (CTK_IMAGE (self->start_icon_widget));

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_START_ICON]);
    }
}

void
ctk_sidebar_row_set_end_icon (CtkSidebarRow *self,
                              GIcon         *icon)
{
  g_return_if_fail (CTK_IS_SIDEBAR_ROW (self));

  if (self->end_icon != icon)
    {
      g_set_object (&self->end_icon, icon);
      if (self->end_icon != NULL)
        ctk_image_set_from_gicon (CTK_IMAGE (self->end_icon_widget), self->end_icon,
                                  CTK_ICON_SIZE_MENU);
      else
        if (self->end_icon_widget != NULL)
          ctk_image_clear (CTK_IMAGE (self->end_icon_widget));

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_END_ICON]);
    }
}

static void
ctk_sidebar_row_finalize (GObject *object)
{
  CtkSidebarRow *self = CTK_SIDEBAR_ROW (object);

  g_clear_object (&self->start_icon);
  g_clear_object (&self->end_icon);
  g_free (self->label);
  self->label = NULL;
  g_free (self->tooltip);
  self->tooltip = NULL;
  g_free (self->uri);
  self->uri = NULL;
  g_clear_object (&self->drive);
  g_clear_object (&self->volume);
  g_clear_object (&self->mount);
#ifdef HAVE_CLOUDPROVIDERS
  if (self->cloud_provider_account != NULL)
    g_signal_handlers_disconnect_by_data (self->cloud_provider_account, self);
  g_clear_object (&self->cloud_provider_account);
#endif

  G_OBJECT_CLASS (ctk_sidebar_row_parent_class)->finalize (object);
}

static void
ctk_sidebar_row_init (CtkSidebarRow *self)
{
  ctk_widget_init_template (CTK_WIDGET (self));
}

static void
ctk_sidebar_row_class_init (CtkSidebarRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->get_property = ctk_sidebar_row_get_property;
  object_class->set_property = ctk_sidebar_row_set_property;
  object_class->finalize = ctk_sidebar_row_finalize;

  properties [PROP_SIDEBAR] =
    g_param_spec_object ("sidebar",
                         "Sidebar",
                         "Sidebar",
                         CTK_TYPE_PLACES_SIDEBAR,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_START_ICON] =
    g_param_spec_object ("start-icon",
                         "start-icon",
                         "The start icon.",
                         G_TYPE_ICON,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_END_ICON] =
    g_param_spec_object ("end-icon",
                         "end-icon",
                         "The end icon.",
                         G_TYPE_ICON,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_LABEL] =
    g_param_spec_string ("label",
                         "label",
                         "The label text.",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_TOOLTIP] =
    g_param_spec_string ("tooltip",
                         "Tooltip",
                         "Tooltip",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_EJECTABLE] =
    g_param_spec_boolean ("ejectable",
                          "Ejectable",
                          "Ejectable",
                          FALSE,
                          (G_PARAM_READWRITE |
                           G_PARAM_STATIC_STRINGS));

  properties [PROP_ORDER_INDEX] =
    g_param_spec_int ("order-index",
                      "OrderIndex",
                      "Order Index",
                      0, G_MAXINT, 0,
                      (G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS));

  properties [PROP_SECTION_TYPE] =
    g_param_spec_int ("section-type",
                      "section type",
                      "The section type.",
                      SECTION_INVALID, N_SECTIONS, SECTION_INVALID,
                      (G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_CONSTRUCT_ONLY));

  properties [PROP_PLACE_TYPE] =
    g_param_spec_int ("place-type",
                      "place type",
                      "The place type.",
                      PLACES_INVALID, N_PLACES, PLACES_INVALID,
                      (G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_CONSTRUCT_ONLY));

  properties [PROP_URI] =
    g_param_spec_string ("uri",
                         "Uri",
                         "Uri",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_DRIVE] =
    g_param_spec_object ("drive",
                         "Drive",
                         "Drive",
                         G_TYPE_DRIVE,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_VOLUME] =
    g_param_spec_object ("volume",
                         "Volume",
                         "Volume",
                         G_TYPE_VOLUME,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_MOUNT] =
    g_param_spec_object ("mount",
                         "Mount",
                         "Mount",
                         G_TYPE_MOUNT,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_CLOUD_PROVIDER_ACCOUNT] =
    g_param_spec_object ("cloud-provider-account",
                         "CloudProvidersAccount",
                         "CloudProvidersAccount",
                         G_TYPE_OBJECT,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_PLACEHOLDER] =
    g_param_spec_boolean ("placeholder",
                          "Placeholder",
                          "Placeholder",
                          FALSE,
                          (G_PARAM_READWRITE |
                           G_PARAM_CONSTRUCT_ONLY |
                           G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  ctk_widget_class_set_template_from_resource (widget_class,
                                               "/org/ctk/libctk/ui/ctksidebarrow.ui");

  ctk_widget_class_bind_template_child (widget_class, CtkSidebarRow, start_icon_widget);
  ctk_widget_class_bind_template_child (widget_class, CtkSidebarRow, end_icon_widget);
  ctk_widget_class_bind_template_child (widget_class, CtkSidebarRow, label_widget);
  ctk_widget_class_bind_template_child (widget_class, CtkSidebarRow, eject_button);
  ctk_widget_class_bind_template_child (widget_class, CtkSidebarRow, event_box);
  ctk_widget_class_bind_template_child (widget_class, CtkSidebarRow, revealer);
  ctk_widget_class_bind_template_child (widget_class, CtkSidebarRow, busy_spinner);

  ctk_widget_class_bind_template_callback (widget_class, on_child_revealed);
  ctk_widget_class_set_css_name (widget_class, "row");
}

CtkSidebarRow*
ctk_sidebar_row_clone (CtkSidebarRow *self)
{
 return g_object_new (CTK_TYPE_SIDEBAR_ROW,
                      "sidebar", self->sidebar,
                      "start-icon", self->start_icon,
                      "end-icon", self->end_icon,
                      "label", self->label,
                      "tooltip", self->tooltip,
                      "ejectable", self->ejectable,
                      "order-index", self->order_index,
                      "section-type", self->section_type,
                      "place-type", self->place_type,
                      "uri", self->uri,
                      "drive", self->drive,
                      "volume", self->volume,
                      "mount", self->mount,
                      "cloud-provider-account", self->cloud_provider_account,
                      NULL);
}

CtkWidget*
ctk_sidebar_row_get_eject_button (CtkSidebarRow *self)
{
  return self->eject_button;
}

CtkWidget*
ctk_sidebar_row_get_event_box (CtkSidebarRow *self)
{
  return self->event_box;
}

void
ctk_sidebar_row_set_busy (CtkSidebarRow *row,
                          gboolean       is_busy)
{
  g_return_if_fail (CTK_IS_SIDEBAR_ROW (row));

  ctk_widget_set_visible (row->busy_spinner, is_busy);
}
