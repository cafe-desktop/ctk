/* ctkshortcutsshortcut.c
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

#include "ctkshortcutsshortcut.h"

#include "ctkshortcutlabel.h"
#include "ctkshortcutswindowprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctkshortcutsshortcut
 * @Title: CtkShortcutsShortcut
 * @Short_description: Represents a keyboard shortcut in a CtkShortcutsWindow
 *
 * A CtkShortcutsShortcut represents a single keyboard shortcut or gesture
 * with a short text. This widget is only meant to be used with #CtkShortcutsWindow.
 */

struct _CtkShortcutsShortcut
{
  CtkBox            parent_instance;

  CtkImage         *image;
  CtkShortcutLabel *accelerator;
  CtkLabel         *title;
  CtkLabel         *subtitle;
  CtkLabel         *title_box;

  CtkSizeGroup *accel_size_group;
  CtkSizeGroup *title_size_group;

  gboolean subtitle_set;
  gboolean icon_set;
  CtkTextDirection direction;
  gchar *action_name;
  CtkShortcutType  shortcut_type;
};

struct _CtkShortcutsShortcutClass
{
  CtkBoxClass parent_class;
};

G_DEFINE_TYPE (CtkShortcutsShortcut, ctk_shortcuts_shortcut, CTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_ACCELERATOR,
  PROP_ICON,
  PROP_ICON_SET,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_SUBTITLE_SET,
  PROP_ACCEL_SIZE_GROUP,
  PROP_TITLE_SIZE_GROUP,
  PROP_DIRECTION,
  PROP_SHORTCUT_TYPE,
  PROP_ACTION_NAME,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static void
ctk_shortcuts_shortcut_set_accelerator (CtkShortcutsShortcut *self,
                                        const gchar          *accelerator)
{
  ctk_shortcut_label_set_accelerator (self->accelerator, accelerator);
}

static void
ctk_shortcuts_shortcut_set_accel_size_group (CtkShortcutsShortcut *self,
                                             CtkSizeGroup         *group)
{
  if (self->accel_size_group)
    {
      ctk_size_group_remove_widget (self->accel_size_group, CTK_WIDGET (self->accelerator));
      ctk_size_group_remove_widget (self->accel_size_group, CTK_WIDGET (self->image));
    }

  if (group)
    {
      ctk_size_group_add_widget (group, CTK_WIDGET (self->accelerator));
      ctk_size_group_add_widget (group, CTK_WIDGET (self->image));
    }

  g_set_object (&self->accel_size_group, group);
}

static void
ctk_shortcuts_shortcut_set_title_size_group (CtkShortcutsShortcut *self,
                                             CtkSizeGroup         *group)
{
  if (self->title_size_group)
    ctk_size_group_remove_widget (self->title_size_group, CTK_WIDGET (self->title_box));
  if (group)
    ctk_size_group_add_widget (group, CTK_WIDGET (self->title_box));

  g_set_object (&self->title_size_group, group);
}

static void
update_subtitle_from_type (CtkShortcutsShortcut *self)
{
  const gchar *subtitle;

  if (self->subtitle_set)
    return;

  switch (self->shortcut_type)
    {
    case CTK_SHORTCUT_ACCELERATOR:
    case CTK_SHORTCUT_GESTURE:
      subtitle = NULL;
      break;

    case CTK_SHORTCUT_GESTURE_PINCH:
      subtitle = _("Two finger pinch");
      break;

    case CTK_SHORTCUT_GESTURE_STRETCH:
      subtitle = _("Two finger stretch");
      break;

    case CTK_SHORTCUT_GESTURE_ROTATE_CLOCKWISE:
      subtitle = _("Rotate clockwise");
      break;

    case CTK_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE:
      subtitle = _("Rotate counterclockwise");
      break;

    case CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT:
      subtitle = _("Two finger swipe left");
      break;

    case CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT:
      subtitle = _("Two finger swipe right");
      break;

    default:
      subtitle = NULL;
      break;
    }

  ctk_label_set_label (self->subtitle, subtitle);
  ctk_widget_set_visible (CTK_WIDGET (self->subtitle), subtitle != NULL);
  g_object_notify (G_OBJECT (self), "subtitle");
}

static void
ctk_shortcuts_shortcut_set_subtitle_set (CtkShortcutsShortcut *self,
                                         gboolean              subtitle_set)
{
  if (self->subtitle_set != subtitle_set)
    {
      self->subtitle_set = subtitle_set;
      g_object_notify (G_OBJECT (self), "subtitle-set");
    }
  update_subtitle_from_type (self);
}

static void
ctk_shortcuts_shortcut_set_subtitle (CtkShortcutsShortcut *self,
                                     const gchar          *subtitle)
{
  ctk_label_set_label (self->subtitle, subtitle);
  ctk_widget_set_visible (CTK_WIDGET (self->subtitle), subtitle && subtitle[0]);
  ctk_shortcuts_shortcut_set_subtitle_set (self, subtitle && subtitle[0]);

  g_object_notify (G_OBJECT (self), "subtitle");
}

static void
update_icon_from_type (CtkShortcutsShortcut *self)
{
  GIcon *icon;

  if (self->icon_set)
    return;

  switch (self->shortcut_type)
    {
    case CTK_SHORTCUT_GESTURE_PINCH:
      icon = g_themed_icon_new ("gesture-pinch-symbolic");
      break;

    case CTK_SHORTCUT_GESTURE_STRETCH:
      icon = g_themed_icon_new ("gesture-stretch-symbolic");
      break;

    case CTK_SHORTCUT_GESTURE_ROTATE_CLOCKWISE:
      icon = g_themed_icon_new ("gesture-rotate-clockwise-symbolic");
      break;

    case CTK_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE:
      icon = g_themed_icon_new ("gesture-rotate-anticlockwise-symbolic");
      break;

    case CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT:
      icon = g_themed_icon_new ("gesture-two-finger-swipe-left-symbolic");
      break;

    case CTK_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT:
      icon = g_themed_icon_new ("gesture-two-finger-swipe-right-symbolic");
      break;

    default: ;
      icon = NULL;
      break;
    }

  if (icon)
    {
      ctk_image_set_from_gicon (self->image, icon, CTK_ICON_SIZE_DIALOG);
      ctk_image_set_pixel_size (self->image, 64);
      g_object_unref (icon);
    }
}

static void
ctk_shortcuts_shortcut_set_icon_set (CtkShortcutsShortcut *self,
                                     gboolean              icon_set)
{
  if (self->icon_set != icon_set)
    {
      self->icon_set = icon_set;
      g_object_notify (G_OBJECT (self), "icon-set");
    }
  update_icon_from_type (self);
}

static void
ctk_shortcuts_shortcut_set_icon (CtkShortcutsShortcut *self,
                                 GIcon                *gicon)
{
  ctk_image_set_from_gicon (self->image, gicon, CTK_ICON_SIZE_DIALOG);
  ctk_shortcuts_shortcut_set_icon_set (self, gicon != NULL);
  g_object_notify (G_OBJECT (self), "icon");
}

static void
update_visible_from_direction (CtkShortcutsShortcut *self)
{
  if (self->direction == CTK_TEXT_DIR_NONE ||
      self->direction == ctk_widget_get_direction (CTK_WIDGET (self)))
    {
      ctk_widget_set_visible (CTK_WIDGET (self), TRUE);
      ctk_widget_set_no_show_all (CTK_WIDGET (self), FALSE);
    }
  else
    {
      ctk_widget_set_visible (CTK_WIDGET (self), FALSE);
      ctk_widget_set_no_show_all (CTK_WIDGET (self), TRUE);
    }
}

static void
ctk_shortcuts_shortcut_set_direction (CtkShortcutsShortcut *self,
                                      CtkTextDirection      direction)
{
  if (self->direction == direction)
    return;

  self->direction = direction;

  update_visible_from_direction (self);

  g_object_notify (G_OBJECT (self), "direction");
}

static void
ctk_shortcuts_shortcut_direction_changed (CtkWidget        *widget,
                                          CtkTextDirection  previous_dir)
{
  update_visible_from_direction (CTK_SHORTCUTS_SHORTCUT (widget));

  CTK_WIDGET_CLASS (ctk_shortcuts_shortcut_parent_class)->direction_changed (widget, previous_dir);
}

static void
ctk_shortcuts_shortcut_set_type (CtkShortcutsShortcut *self,
                                 CtkShortcutType       type)
{
  if (self->shortcut_type == type)
    return;

  self->shortcut_type = type;

  update_subtitle_from_type (self);
  update_icon_from_type (self);

  ctk_widget_set_visible (CTK_WIDGET (self->accelerator), type == CTK_SHORTCUT_ACCELERATOR);
  ctk_widget_set_visible (CTK_WIDGET (self->image), type != CTK_SHORTCUT_ACCELERATOR);


  g_object_notify (G_OBJECT (self), "shortcut-type");
}

static void
ctk_shortcuts_shortcut_set_action_name (CtkShortcutsShortcut *self,
                                        const gchar          *action_name)
{
  g_free (self->action_name);
  self->action_name = g_strdup (action_name);

  g_object_notify (G_OBJECT (self), "action-name");
}

static void
ctk_shortcuts_shortcut_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CtkShortcutsShortcut *self = CTK_SHORTCUTS_SHORTCUT (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, ctk_label_get_label (self->title));
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, ctk_label_get_label (self->subtitle));
      break;

    case PROP_SUBTITLE_SET:
      g_value_set_boolean (value, self->subtitle_set);
      break;

    case PROP_ACCELERATOR:
      g_value_set_string (value, ctk_shortcut_label_get_accelerator (self->accelerator));
      break;

    case PROP_ICON:
      {
        GIcon *icon;

        ctk_image_get_gicon (self->image, &icon, NULL);
        g_value_set_object (value, icon);
      }
      break;

    case PROP_ICON_SET:
      g_value_set_boolean (value, self->icon_set);
      break;

    case PROP_DIRECTION:
      g_value_set_enum (value, self->direction);
      break;

    case PROP_SHORTCUT_TYPE:
      g_value_set_enum (value, self->shortcut_type);
      break;

    case PROP_ACTION_NAME:
      g_value_set_string (value, self->action_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_shortcuts_shortcut_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkShortcutsShortcut *self = CTK_SHORTCUTS_SHORTCUT (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      ctk_shortcuts_shortcut_set_accelerator (self, g_value_get_string (value));
      break;

    case PROP_ICON:
      ctk_shortcuts_shortcut_set_icon (self, g_value_get_object (value));
      break;

    case PROP_ICON_SET:
      ctk_shortcuts_shortcut_set_icon_set (self, g_value_get_boolean (value));
      break;

    case PROP_ACCEL_SIZE_GROUP:
      ctk_shortcuts_shortcut_set_accel_size_group (self, CTK_SIZE_GROUP (g_value_get_object (value)));
      break;

    case PROP_TITLE:
      ctk_label_set_label (self->title, g_value_get_string (value));
      break;

    case PROP_SUBTITLE:
      ctk_shortcuts_shortcut_set_subtitle (self, g_value_get_string (value));
      break;

    case PROP_SUBTITLE_SET:
      ctk_shortcuts_shortcut_set_subtitle_set (self, g_value_get_boolean (value));
      break;

    case PROP_TITLE_SIZE_GROUP:
      ctk_shortcuts_shortcut_set_title_size_group (self, CTK_SIZE_GROUP (g_value_get_object (value)));
      break;

    case PROP_DIRECTION:
      ctk_shortcuts_shortcut_set_direction (self, g_value_get_enum (value));
      break;

    case PROP_SHORTCUT_TYPE:
      ctk_shortcuts_shortcut_set_type (self, g_value_get_enum (value));
      break;

    case PROP_ACTION_NAME:
      ctk_shortcuts_shortcut_set_action_name (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_shortcuts_shortcut_finalize (GObject *object)
{
  CtkShortcutsShortcut *self = CTK_SHORTCUTS_SHORTCUT (object);

  g_clear_object (&self->accel_size_group);
  g_clear_object (&self->title_size_group);
  g_free (self->action_name);

  G_OBJECT_CLASS (ctk_shortcuts_shortcut_parent_class)->finalize (object);
}

static void
ctk_shortcuts_shortcut_add (CtkContainer *container,
                            CtkWidget    *widget)
{
  g_warning ("Can't add children to %s", G_OBJECT_TYPE_NAME (container));
}

static GType
ctk_shortcuts_shortcut_child_type (CtkContainer *container)
{
  return G_TYPE_NONE;
}

void
ctk_shortcuts_shortcut_update_accel (CtkShortcutsShortcut *self,
                                     CtkWindow            *window)
{
  CtkApplication *app;
  gchar **accels;
  gchar *str;

  if (self->action_name == NULL)
    return;

  app = ctk_window_get_application (window);
  if (app == NULL)
    return;

  accels = ctk_application_get_accels_for_action (app, self->action_name);
  str = g_strjoinv (" ", accels);

  ctk_shortcuts_shortcut_set_accelerator (self, str);

  g_free (str);
  g_strfreev (accels);
}

static void
ctk_shortcuts_shortcut_class_init (CtkShortcutsShortcutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  object_class->finalize = ctk_shortcuts_shortcut_finalize;
  object_class->get_property = ctk_shortcuts_shortcut_get_property;
  object_class->set_property = ctk_shortcuts_shortcut_set_property;

  widget_class->direction_changed = ctk_shortcuts_shortcut_direction_changed;

  container_class->add = ctk_shortcuts_shortcut_add;
  container_class->child_type = ctk_shortcuts_shortcut_child_type;

  /**
   * CtkShortcutsShortcut:accelerator:
   *
   * The accelerator(s) represented by this object. This property is used
   * if #CtkShortcutsShortcut:shortcut-type is set to #CTK_SHORTCUT_ACCELERATOR.
   *
   * The syntax of this property is (an extension of) the syntax understood by
   * ctk_accelerator_parse(). Multiple accelerators can be specified by separating
   * them with a space, but keep in mind that the available width is limited.
   * It is also possible to specify ranges of shortcuts, using ... between the keys.
   * Sequences of keys can be specified using a + or & between the keys.
   *
   * Examples:
   * - A single shortcut: <ctl><alt>delete
   * - Two alternative shortcuts: <shift>a Home
   * - A range of shortcuts: <alt>1...<alt>9
   * - Several keys pressed together: Control_L&Control_R
   * - A sequence of shortcuts or keys: <ctl>c+<ctl>x
   *
   * Use + instead of & when the keys may (or have to be) pressed sequentially (e.g
   * use t+t for 'press the t key twice').
   *
   * Note that <, > and & need to be escaped as &lt;, &gt; and &amp; when used
   * in .ui files.
   */
  properties[PROP_ACCELERATOR] =
    g_param_spec_string ("accelerator",
                         P_("Accelerator"),
                         P_("The accelerator keys for shortcuts of type 'Accelerator'"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:icon:
   *
   * An icon to represent the shortcut or gesture. This property is used if
   * #CtkShortcutsShortcut:shortcut-type is set to #CTK_SHORTCUT_GESTURE.
   * For the other predefined gesture types, CTK+ provides an icon on its own.
   */
  properties[PROP_ICON] =
    g_param_spec_object ("icon",
                         P_("Icon"),
                         P_("The icon to show for shortcuts of type 'Other Gesture'"),
                         G_TYPE_ICON,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:icon-set:
   *
   * %TRUE if an icon has been set.
   */
  properties[PROP_ICON_SET] =
    g_param_spec_boolean ("icon-set",
                          P_("Icon Set"),
                          P_("Whether an icon has been set"),
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:title:
   *
   * The textual description for the shortcut or gesture represented by
   * this object. This should be a short string that can fit in a single line.
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         P_("Title"),
                         P_("A short description for the shortcut"),
                         "",
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:subtitle:
   *
   * The subtitle for the shortcut or gesture.
   *
   * This is typically used for gestures and should be a short, one-line
   * text that describes the gesture itself. For the predefined gesture
   * types, CTK+ provides a subtitle on its own.
   */
  properties[PROP_SUBTITLE] =
    g_param_spec_string ("subtitle",
                         P_("Subtitle"),
                         P_("A short description for the gesture"),
                         "",
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:subtitle-set:
   *
   * %TRUE if a subtitle has been set.
   */
  properties[PROP_SUBTITLE_SET] =
    g_param_spec_boolean ("subtitle-set",
                          P_("Subtitle Set"),
                          P_("Whether a subtitle has been set"),
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:accel-size-group:
   *
   * The size group for the accelerator portion of this shortcut.
   *
   * This is used internally by CTK+, and must not be modified by applications.
   */
  properties[PROP_ACCEL_SIZE_GROUP] =
    g_param_spec_object ("accel-size-group",
                         P_("Accelerator Size Group"),
                         P_("Accelerator Size Group"),
                         CTK_TYPE_SIZE_GROUP,
                         (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:title-size-group:
   *
   * The size group for the textual portion of this shortcut.
   *
   * This is used internally by CTK+, and must not be modified by applications.
   */
  properties[PROP_TITLE_SIZE_GROUP] =
    g_param_spec_object ("title-size-group",
                         P_("Title Size Group"),
                         P_("Title Size Group"),
                         CTK_TYPE_SIZE_GROUP,
                         (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkShortcutsShortcut:direction:
   *
   * The text direction for which this shortcut is active. If the shortcut
   * is used regardless of the text direction, set this property to
   * #CTK_TEXT_DIR_NONE.
   */
  properties[PROP_DIRECTION] =
    g_param_spec_enum ("direction",
                       P_("Direction"),
                       P_("Text direction for which this shortcut is active"),
                       CTK_TYPE_TEXT_DIRECTION,
                       CTK_TEXT_DIR_NONE,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkShortcutsShortcut:shortcut-type:
   *
   * The type of shortcut that is represented.
   */
  properties[PROP_SHORTCUT_TYPE] =
    g_param_spec_enum ("shortcut-type",
                       P_("Shortcut Type"),
                       P_("The type of shortcut that is represented"),
                       CTK_TYPE_SHORTCUT_TYPE,
                       CTK_SHORTCUT_ACCELERATOR,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkShortcutsShortcut:action-name:
   *
   * A detailed action name. If this is set for a shortcut
   * of type %CTK_SHORTCUT_ACCELERATOR, then CTK+ will use
   * the accelerators that are associated with the action
   * via ctk_application_set_accels_for_action(), and setting
   * #CtkShortcutsShortcut::accelerator is not necessary.
   *
   * Since: 3.22
   */
  properties[PROP_ACTION_NAME] =
    g_param_spec_string ("action-name",
                         P_("Action Name"),
                         P_("The name of the action"),
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
  ctk_widget_class_set_css_name (widget_class, "shortcut");
}

static void
ctk_shortcuts_shortcut_init (CtkShortcutsShortcut *self)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (self), CTK_ORIENTATION_HORIZONTAL);
  ctk_box_set_spacing (CTK_BOX (self), 12);

  self->direction = CTK_TEXT_DIR_NONE;
  self->shortcut_type = CTK_SHORTCUT_ACCELERATOR;

  self->image = g_object_new (CTK_TYPE_IMAGE,
                              "visible", FALSE,
                              "valign", CTK_ALIGN_CENTER,
                              "no-show-all", TRUE,
                              NULL);
  CTK_CONTAINER_CLASS (ctk_shortcuts_shortcut_parent_class)->add (CTK_CONTAINER (self), CTK_WIDGET (self->image));

  self->accelerator = g_object_new (CTK_TYPE_SHORTCUT_LABEL,
                                    "visible", TRUE,
                                    "valign", CTK_ALIGN_CENTER,
                                    "no-show-all", TRUE,
                                    NULL);
  CTK_CONTAINER_CLASS (ctk_shortcuts_shortcut_parent_class)->add (CTK_CONTAINER (self), CTK_WIDGET (self->accelerator));

  self->title_box = g_object_new (CTK_TYPE_BOX,
                                  "visible", TRUE,
                                  "valign", CTK_ALIGN_CENTER,
                                  "hexpand", TRUE,
                                  "orientation", CTK_ORIENTATION_VERTICAL,
                                  NULL);
  CTK_CONTAINER_CLASS (ctk_shortcuts_shortcut_parent_class)->add (CTK_CONTAINER (self), CTK_WIDGET (self->title_box));

  self->title = g_object_new (CTK_TYPE_LABEL,
                              "visible", TRUE,
                              "xalign", 0.0f,
                              NULL);
  ctk_container_add (CTK_CONTAINER (self->title_box), CTK_WIDGET (self->title));

  self->subtitle = g_object_new (CTK_TYPE_LABEL,
                                 "visible", FALSE,
                                 "no-show-all", TRUE,
                                 "xalign", 0.0f,
                                 NULL);
  ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (self->subtitle)),
                               CTK_STYLE_CLASS_DIM_LABEL);
  ctk_container_add (CTK_CONTAINER (self->title_box), CTK_WIDGET (self->subtitle));
}
