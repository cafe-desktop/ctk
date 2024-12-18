/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Red Hat, Inc.
 * Author: Matthias Clasen
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

#include "ctklockbuttonprivate.h"
#include "ctkbox.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctksizegroup.h"
#include "ctkintl.h"
#include "a11y/ctklockbuttonaccessibleprivate.h"

/**
 * SECTION:ctklockbutton
 * @title: CtkLockButton
 * @short_description: A widget to unlock or lock privileged operations
 *
 * CtkLockButton is a widget that can be used in control panels or
 * preference dialogs to allow users to obtain and revoke authorizations
 * needed to operate the controls. The required authorization is represented
 * by a #GPermission object. Concrete implementations of #GPermission may use
 * PolicyKit or some other authorization framework. To obtain a PolicyKit-based
 * #GPermission, use polkit_permission_new().
 *
 * If the user is not currently allowed to perform the action, but can obtain
 * the permission, the widget looks like this:
 *
 * ![](lockbutton-locked.png)
 *
 * and the user can click the button to request the permission. Depending
 * on the platform, this may pop up an authentication dialog or ask the user
 * to authenticate in some other way. Once the user has obtained the permission,
 * the widget changes to this:
 *
 * ![](lockbutton-unlocked.png)
 *
 * and the permission can be dropped again by clicking the button. If the user
 * is not able to obtain the permission at all, the widget looks like this:
 *
 * ![](lockbutton-sorry.png)
 *
 * If the user has the permission and cannot drop it, the button is hidden.
 *
 * The text (and tooltips) that are shown in the various cases can be adjusted
 * with the #CtkLockButton:text-lock, #CtkLockButton:text-unlock,
 * #CtkLockButton:tooltip-lock, #CtkLockButton:tooltip-unlock and
 * #CtkLockButton:tooltip-not-authorized properties.
 */

struct _CtkLockButtonPrivate
{
  GPermission *permission;
  GCancellable *cancellable;

  gchar *tooltip_lock;
  gchar *tooltip_unlock;
  gchar *tooltip_not_authorized;
  GIcon *icon_lock;
  GIcon *icon_unlock;

  CtkWidget *box;
  CtkWidget *image;
  CtkWidget *stack;
  CtkWidget *label_lock;
  CtkWidget *label_unlock;
};

enum
{
  PROP_0,
  PROP_PERMISSION,
  PROP_TEXT_LOCK,
  PROP_TEXT_UNLOCK,
  PROP_TOOLTIP_LOCK,
  PROP_TOOLTIP_UNLOCK,
  PROP_TOOLTIP_NOT_AUTHORIZED
};

static void update_state (CtkLockButton *button);
static void ctk_lock_button_clicked (CtkButton *button);

static void on_permission_changed (GPermission *permission,
                                   GParamSpec  *pspec,
                                   gpointer     user_data);

G_DEFINE_TYPE_WITH_PRIVATE (CtkLockButton, ctk_lock_button, CTK_TYPE_BUTTON)

static void
ctk_lock_button_finalize (GObject *object)
{
  CtkLockButton *button = CTK_LOCK_BUTTON (object);
  CtkLockButtonPrivate *priv = button->priv;

  g_free (priv->tooltip_lock);
  g_free (priv->tooltip_unlock);
  g_free (priv->tooltip_not_authorized);

  g_object_unref (priv->icon_lock);
  g_object_unref (priv->icon_unlock);

  if (priv->cancellable != NULL)
    {
      g_cancellable_cancel (priv->cancellable);
      g_object_unref (priv->cancellable);
    }

  if (priv->permission)
    {
      g_signal_handlers_disconnect_by_func (priv->permission,
                                            on_permission_changed,
                                            button);
      g_object_unref (priv->permission);
    }

  G_OBJECT_CLASS (ctk_lock_button_parent_class)->finalize (object);
}

static void
ctk_lock_button_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkLockButton *button = CTK_LOCK_BUTTON (object);
  CtkLockButtonPrivate *priv = button->priv;

  switch (property_id)
    {
    case PROP_PERMISSION:
      g_value_set_object (value, priv->permission);
      break;

    case PROP_TEXT_LOCK:
      g_value_set_string (value, ctk_label_get_text (CTK_LABEL (priv->label_lock)));
      break;

    case PROP_TEXT_UNLOCK:
      g_value_set_string (value, ctk_label_get_text (CTK_LABEL (priv->label_unlock)));
      break;

    case PROP_TOOLTIP_LOCK:
      g_value_set_string (value, priv->tooltip_lock);
      break;

    case PROP_TOOLTIP_UNLOCK:
      g_value_set_string (value, priv->tooltip_unlock);
      break;

    case PROP_TOOLTIP_NOT_AUTHORIZED:
      g_value_set_string (value, priv->tooltip_not_authorized);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_lock_button_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkLockButton *button = CTK_LOCK_BUTTON (object);
  CtkLockButtonPrivate *priv = button->priv;

  switch (property_id)
    {
    case PROP_PERMISSION:
      ctk_lock_button_set_permission (button, g_value_get_object (value));
      break;

    case PROP_TEXT_LOCK:
      ctk_label_set_text (CTK_LABEL (priv->label_lock), g_value_get_string (value));
      _ctk_lock_button_accessible_name_changed (button);
      break;

    case PROP_TEXT_UNLOCK:
      ctk_label_set_text (CTK_LABEL (priv->label_unlock), g_value_get_string (value));
      _ctk_lock_button_accessible_name_changed (button);
      break;

    case PROP_TOOLTIP_LOCK:
      g_free (priv->tooltip_lock);
      priv->tooltip_lock = g_value_dup_string (value);
      break;

    case PROP_TOOLTIP_UNLOCK:
      g_free (priv->tooltip_unlock);
      priv->tooltip_unlock = g_value_dup_string (value);
      break;

    case PROP_TOOLTIP_NOT_AUTHORIZED:
      g_free (priv->tooltip_not_authorized);
      priv->tooltip_not_authorized = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  update_state (button);
}

static void
ctk_lock_button_init (CtkLockButton *button)
{
  CtkLockButtonPrivate *priv;
  gchar *names[3];
  CtkStyleContext *context;

  button->priv = priv = ctk_lock_button_get_instance_private (button);

  ctk_widget_init_template (CTK_WIDGET (button));

  names[0] = "changes-allow-symbolic";
  names[1] = "changes-allow";
  names[2] = NULL;
  priv->icon_unlock = g_themed_icon_new_from_names (names, -1);

  names[0] = "changes-prevent-symbolic";
  names[1] = "changes-prevent";
  names[2] = NULL;
  priv->icon_lock = g_themed_icon_new_from_names (names, -1);

  update_state (button);

  context = ctk_widget_get_style_context (CTK_WIDGET (button));
  ctk_style_context_add_class (context, "lock");
}

static void
ctk_lock_button_class_init (CtkLockButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkButtonClass *button_class = CTK_BUTTON_CLASS (klass);

  gobject_class->finalize     = ctk_lock_button_finalize;
  gobject_class->get_property = ctk_lock_button_get_property;
  gobject_class->set_property = ctk_lock_button_set_property;

  button_class->clicked = ctk_lock_button_clicked;

  g_object_class_install_property (gobject_class, PROP_PERMISSION,
    g_param_spec_object ("permission",
                         P_("Permission"),
                         P_("The GPermission object controlling this button"),
                         G_TYPE_PERMISSION,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TEXT_LOCK,
    g_param_spec_string ("text-lock",
                         P_("Lock Text"),
                         P_("The text to display when prompting the user to lock"),
                         _("Lock"),
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TEXT_UNLOCK,
    g_param_spec_string ("text-unlock",
                         P_("Unlock Text"),
                         P_("The text to display when prompting the user to unlock"),
                         _("Unlock"),
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TOOLTIP_LOCK,
    g_param_spec_string ("tooltip-lock",
                         P_("Lock Tooltip"),
                         P_("The tooltip to display when prompting the user to lock"),
                         _("Dialog is unlocked.\nClick to prevent further changes"),
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TOOLTIP_UNLOCK,
    g_param_spec_string ("tooltip-unlock",
                         P_("Unlock Tooltip"),
                         P_("The tooltip to display when prompting the user to unlock"),
                         _("Dialog is locked.\nClick to make changes"),
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TOOLTIP_NOT_AUTHORIZED,
    g_param_spec_string ("tooltip-not-authorized",
                         P_("Not Authorized Tooltip"),
                         P_("The tooltip to display when prompting the user cannot obtain authorization"),
                         _("System policy prevents changes.\nContact your system administrator"),
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctklockbutton.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkLockButton, box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkLockButton, image);
  ctk_widget_class_bind_template_child_private (widget_class, CtkLockButton, label_lock);
  ctk_widget_class_bind_template_child_private (widget_class, CtkLockButton, label_unlock);
  ctk_widget_class_bind_template_child_private (widget_class, CtkLockButton, stack);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_LOCK_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "button");
}

static void
update_state (CtkLockButton *button)
{
  CtkLockButtonPrivate *priv = button->priv;
  gboolean allowed;
  gboolean can_acquire;
  gboolean can_release;
  gboolean sensitive;
  gboolean visible;
  GIcon *icon;
  const gchar *tooltip;

  if (priv->permission)
    {
      allowed = g_permission_get_allowed (priv->permission);
      can_acquire = g_permission_get_can_acquire (priv->permission);
      can_release = g_permission_get_can_release (priv->permission);
    }
  else
    {
      allowed = TRUE;
      can_acquire = FALSE;
      can_release = FALSE;
    }

  if (allowed && can_release)
    {
      visible = TRUE;
      sensitive = TRUE;
      icon = priv->icon_lock;
      tooltip = priv->tooltip_lock;
    }
  else if (allowed && !can_release)
    {
      visible = FALSE;
      sensitive = TRUE;
      icon = priv->icon_lock;
      tooltip = priv->tooltip_lock;
    }
  else if (!allowed && can_acquire)
    {
      visible = TRUE;
      sensitive = TRUE;
      icon = priv->icon_unlock;
      tooltip = priv->tooltip_unlock;
    }
  else if (!allowed && !can_acquire)
    {
      visible = TRUE;
      sensitive = FALSE;
      icon = priv->icon_unlock;
      tooltip = priv->tooltip_not_authorized;
    }
  else
    {
      g_assert_not_reached ();
    }

  ctk_image_set_from_gicon (CTK_IMAGE (priv->image), icon, CTK_ICON_SIZE_MENU);
  ctk_stack_set_visible_child (CTK_STACK (priv->stack),
                               allowed ? priv->label_lock : priv->label_unlock);
  _ctk_lock_button_accessible_name_changed (button);
  ctk_widget_set_tooltip_markup (CTK_WIDGET (button), tooltip);
  ctk_widget_set_sensitive (CTK_WIDGET (button), sensitive);
  ctk_widget_set_visible (CTK_WIDGET (button), visible);
}

static void
on_permission_changed (GPermission *permission G_GNUC_UNUSED,
                       GParamSpec  *pspec G_GNUC_UNUSED,
                       gpointer     user_data)
{
  CtkLockButton *button = CTK_LOCK_BUTTON (user_data);

  update_state (button);
}

static void
acquire_cb (GObject      *source G_GNUC_UNUSED,
            GAsyncResult *result,
            gpointer      user_data)
{
  CtkLockButton *button = CTK_LOCK_BUTTON (user_data);
  CtkLockButtonPrivate *priv = button->priv;
  GError *error;

  error = NULL;
  if (!g_permission_acquire_finish (priv->permission, result, &error))
    {
      g_warning ("Error acquiring permission: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (priv->cancellable);
  priv->cancellable = NULL;

  update_state (button);
}

static void
release_cb (GObject      *source G_GNUC_UNUSED,
            GAsyncResult *result,
            gpointer      user_data)
{
  CtkLockButton *button = CTK_LOCK_BUTTON (user_data);
  CtkLockButtonPrivate *priv = button->priv;
  GError *error;

  error = NULL;
  if (!g_permission_release_finish (priv->permission, result, &error))
    {
      g_warning ("Error releasing permission: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (priv->cancellable);
  priv->cancellable = NULL;

  update_state (button);
}

static void
ctk_lock_button_clicked (CtkButton *button)
{
  CtkLockButtonPrivate *priv = CTK_LOCK_BUTTON (button)->priv;

  /* if we already have a pending interactive check or permission is not set,
   * then do nothing
   */
  if (priv->cancellable != NULL || priv->permission == NULL)
    return;

  if (g_permission_get_allowed (priv->permission))
    {
      if (g_permission_get_can_release (priv->permission))
        {
          priv->cancellable = g_cancellable_new ();

          g_permission_release_async (priv->permission,
                                      priv->cancellable,
                                      release_cb,
                                      button);
        }
    }
  else
    {
      if (g_permission_get_can_acquire (priv->permission))
        {
          priv->cancellable = g_cancellable_new ();

          g_permission_acquire_async (priv->permission,
                                      priv->cancellable,
                                      acquire_cb,
                                      button);
        }
    }
}

/**
 * ctk_lock_button_new:
 * @permission: (allow-none): a #GPermission
 *
 * Creates a new lock button which reflects the @permission.
 *
 * Returns: a new #CtkLockButton
 *
 * Since: 3.2
 */
CtkWidget *
ctk_lock_button_new (GPermission *permission)
{
  return CTK_WIDGET (g_object_new (CTK_TYPE_LOCK_BUTTON,
                                   "permission", permission,
                                   NULL));
}

/**
 * ctk_lock_button_get_permission:
 * @button: a #CtkLockButton
 *
 * Obtains the #GPermission object that controls @button.
 *
 * Returns: (transfer none): the #GPermission of @button
 *
 * Since: 3.2
 */
GPermission *
ctk_lock_button_get_permission (CtkLockButton *button)
{
  g_return_val_if_fail (CTK_IS_LOCK_BUTTON (button), NULL);

  return button->priv->permission;
}

/**
 * ctk_lock_button_set_permission:
 * @button: a #CtkLockButton
 * @permission: (allow-none): a #GPermission object, or %NULL
 *
 * Sets the #GPermission object that controls @button.
 *
 * Since: 3.2
 */
void
ctk_lock_button_set_permission (CtkLockButton *button,
                                GPermission   *permission)
{
  CtkLockButtonPrivate *priv;

  g_return_if_fail (CTK_IS_LOCK_BUTTON (button));
  g_return_if_fail (permission == NULL || G_IS_PERMISSION (permission));

  priv = button->priv;

  if (priv->permission != permission)
    {
      if (priv->permission)
        {
          g_signal_handlers_disconnect_by_func (priv->permission,
                                                on_permission_changed,
                                                button);
          g_object_unref (priv->permission);
        }

      priv->permission = permission;

      if (priv->permission)
        {
          g_object_ref (priv->permission);
          g_signal_connect (priv->permission, "notify",
                            G_CALLBACK (on_permission_changed), button);
        }

      update_state (button);

      g_object_notify (G_OBJECT (button), "permission");
    }
}

const char *
_ctk_lock_button_get_current_text (CtkLockButton *button)
{
  CtkWidget *label;

  g_return_val_if_fail (CTK_IS_LOCK_BUTTON (button), NULL);

  label = ctk_stack_get_visible_child (CTK_STACK (button->priv->stack));

  return ctk_label_get_text (CTK_LABEL (label));
}

