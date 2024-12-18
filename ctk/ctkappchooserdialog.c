/*
 * ctkappchooserdialog.c: an app-chooser dialog
 *
 * Copyright (C) 2004 Novell, Inc.
 * Copyright (C) 2007, 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Dave Camp <dave@novell.com>
 *          Alexander Larsson <alexl@redhat.com>
 *          Cosimo Cecchi <ccecchi@redhat.com>
 */

/**
 * SECTION:ctkappchooserdialog
 * @Title: CtkAppChooserDialog
 * @Short_description: An application chooser dialog
 *
 * #CtkAppChooserDialog shows a #CtkAppChooserWidget inside a #CtkDialog.
 *
 * Note that #CtkAppChooserDialog does not have any interesting methods
 * of its own. Instead, you should get the embedded #CtkAppChooserWidget
 * using ctk_app_chooser_dialog_get_widget() and call its methods if
 * the generic #CtkAppChooser interface is not sufficient for your needs.
 *
 * To set the heading that is shown above the #CtkAppChooserWidget,
 * use ctk_app_chooser_dialog_set_heading().
 */
#include "config.h"

#include "ctkappchooserdialog.h"

#include "ctkintl.h"
#include "ctkappchooser.h"
#include "ctkappchooserprivate.h"

#include "ctkmessagedialog.h"
#include "ctksettings.h"
#include "ctklabel.h"
#include "ctkbbox.h"
#include "ctkbutton.h"
#include "ctkentry.h"
#include "ctktogglebutton.h"
#include "ctkstylecontext.h"
#include "ctkmenuitem.h"
#include "ctkheaderbar.h"
#include "ctkdialogprivate.h"
#include "ctksearchbar.h"
#include "ctksizegroup.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

struct _CtkAppChooserDialogPrivate {
  char *content_type;
  GFile *gfile;
  char *heading;

  CtkWidget *label;
  CtkWidget *inner_box;

  CtkWidget *open_label;

  CtkWidget *search_bar;
  CtkWidget *search_entry;
  CtkWidget *app_chooser_widget;
  CtkWidget *show_more_button;
  CtkWidget *software_button;

  CtkSizeGroup *buttons;

  gboolean show_more_clicked;
  gboolean dismissed;
};

enum {
  PROP_GFILE = 1,
  PROP_CONTENT_TYPE,
  PROP_HEADING
};

static void ctk_app_chooser_dialog_iface_init (CtkAppChooserIface *iface);
G_DEFINE_TYPE_WITH_CODE (CtkAppChooserDialog, ctk_app_chooser_dialog, CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkAppChooserDialog)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_APP_CHOOSER,
                                                ctk_app_chooser_dialog_iface_init));


static void
add_or_find_application (CtkAppChooserDialog *self)
{
  GAppInfo *app;

  app = ctk_app_chooser_get_app_info (CTK_APP_CHOOSER (self));

  if (app)
    {
      /* we don't care about reporting errors here */
      if (self->priv->content_type)
        g_app_info_set_as_last_used_for_type (app,
                                              self->priv->content_type,
                                              NULL);
      g_object_unref (app);
    }
}

static void
ctk_app_chooser_dialog_response (CtkDialog *dialog,
                                 gint       response_id,
                                 gpointer   user_data G_GNUC_UNUSED)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (dialog);

  switch (response_id)
    {
    case CTK_RESPONSE_OK:
      add_or_find_application (self);
      break;
    case CTK_RESPONSE_CANCEL:
    case CTK_RESPONSE_DELETE_EVENT:
      self->priv->dismissed = TRUE;
    default:
      break;
    }
}

static void
widget_application_selected_cb (CtkAppChooserWidget *widget G_GNUC_UNUSED,
                                GAppInfo            *app_info G_GNUC_UNUSED,
                                gpointer             user_data)
{
  CtkDialog *self = user_data;

  ctk_dialog_set_response_sensitive (self, CTK_RESPONSE_OK, TRUE);
}

static void
widget_application_activated_cb (CtkAppChooserWidget *widget G_GNUC_UNUSED,
                                 GAppInfo            *app_info G_GNUC_UNUSED,
                                 gpointer             user_data)
{
  CtkAppChooserDialog *self = user_data;

  ctk_dialog_response (CTK_DIALOG (self), CTK_RESPONSE_OK);
}

static char *
get_extension (const char *basename)
{
  char *p;

  p = strrchr (basename, '.');

  if (p && *(p + 1) != '\0')
    return g_strdup (p + 1);

  return NULL;
}

static void
set_dialog_properties (CtkAppChooserDialog *self)
{
  gchar *name;
  gchar *extension;
  gchar *description;
  gchar *string;
  gboolean unknown;
  gchar *title;
  gchar *subtitle;
  gboolean use_header;
  CtkWidget *header;

  name = NULL;
  extension = NULL;
  description = NULL;
  unknown = TRUE;

  if (self->priv->gfile != NULL)
    {
      name = g_file_get_basename (self->priv->gfile);
      extension = get_extension (name);
    }

  if (self->priv->content_type)
    {
      description = g_content_type_get_description (self->priv->content_type);
      unknown = g_content_type_is_unknown (self->priv->content_type);
    }

  title = g_strdup (_("Select Application"));
  subtitle = NULL;
  string = NULL;

  if (name != NULL)
    {
      /* Translators: %s is a filename */
      subtitle = g_strdup_printf (_("Opening “%s”."), name);
      string = g_strdup_printf (_("No applications found for “%s”"), name);
    }
  else if (self->priv->content_type)
    {
      /* Translators: %s is a file type description */
      subtitle = g_strdup_printf (_("Opening “%s” files."), 
                                  unknown ? self->priv->content_type : description);
      string = g_strdup_printf (_("No applications found for “%s” files"),
                                unknown ? self->priv->content_type : description);
    }

  g_object_get (self, "use-header-bar", &use_header, NULL); 
  if (use_header)
    {
      header = ctk_dialog_get_header_bar (CTK_DIALOG (self));
      ctk_header_bar_set_title (CTK_HEADER_BAR (header), title);
      ctk_header_bar_set_subtitle (CTK_HEADER_BAR (header), subtitle);
    }
  else
    {
      ctk_window_set_title (CTK_WINDOW (self), _("Select Application"));
    }

  if (self->priv->heading != NULL)
    {
      ctk_label_set_markup (CTK_LABEL (self->priv->label), self->priv->heading);
      ctk_widget_show (self->priv->label);
    }
  else
    {
      ctk_widget_hide (self->priv->label);
    }

  ctk_app_chooser_widget_set_default_text (CTK_APP_CHOOSER_WIDGET (self->priv->app_chooser_widget),
                                           string);

  g_free (title);
  g_free (subtitle);
  g_free (name);
  g_free (extension);
  g_free (description);
  g_free (string);
}

static void
show_more_button_clicked_cb (CtkButton *button G_GNUC_UNUSED,
                             gpointer   user_data)
{
  CtkAppChooserDialog *self = user_data;

  g_object_set (self->priv->app_chooser_widget,
                "show-recommended", TRUE,
                "show-fallback", TRUE,
                "show-other", TRUE,
                NULL);

  ctk_widget_hide (self->priv->show_more_button);
  self->priv->show_more_clicked = TRUE;
}

static void
widget_notify_for_button_cb (GObject    *source,
                             GParamSpec *pspec G_GNUC_UNUSED,
                             gpointer    user_data)
{
  CtkAppChooserDialog *self = user_data;
  CtkAppChooserWidget *widget = CTK_APP_CHOOSER_WIDGET (source);
  gboolean should_hide;

  should_hide = ctk_app_chooser_widget_get_show_other (widget) ||
    self->priv->show_more_clicked;

  if (should_hide)
    ctk_widget_hide (self->priv->show_more_button);
}

static void
forget_menu_item_activate_cb (CtkMenuItem *item G_GNUC_UNUSED,
                              gpointer     user_data)
{
  CtkAppChooserDialog *self = user_data;
  GAppInfo *info;

  info = ctk_app_chooser_get_app_info (CTK_APP_CHOOSER (self));

  if (info != NULL)
    {
      g_app_info_remove_supports_type (info, self->priv->content_type, NULL);

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));

      g_object_unref (info);
    }
}

static CtkWidget *
build_forget_menu_item (CtkAppChooserDialog *self)
{
  CtkWidget *retval;

  retval = ctk_menu_item_new_with_label (_("Forget association"));
  ctk_widget_show (retval);

  g_signal_connect (retval, "activate",
                    G_CALLBACK (forget_menu_item_activate_cb), self);

  return retval;
}

static void
widget_populate_popup_cb (CtkAppChooserWidget *widget G_GNUC_UNUSED,
                          CtkMenu             *menu,
                          GAppInfo            *info,
                          gpointer             user_data)
{
  CtkAppChooserDialog *self = user_data;
  CtkWidget *menu_item;

  if (g_app_info_can_remove_supports_type (info))
    {
      menu_item = build_forget_menu_item (self);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);
    }
}

static gboolean
key_press_event_cb (CtkWidget    *widget G_GNUC_UNUSED,
                    CdkEvent     *event,
                    CtkSearchBar *bar)
{
  return ctk_search_bar_handle_event (bar, event);
}

static void
construct_appchooser_widget (CtkAppChooserDialog *self)
{
  GAppInfo *info;

  /* Need to build the appchooser widget after, because of the content-type construct-only property */
  self->priv->app_chooser_widget = ctk_app_chooser_widget_new (self->priv->content_type);
  ctk_box_pack_start (CTK_BOX (self->priv->inner_box), self->priv->app_chooser_widget, TRUE, TRUE, 0);
  ctk_widget_show (self->priv->app_chooser_widget);

  g_signal_connect (self->priv->app_chooser_widget, "application-selected",
                    G_CALLBACK (widget_application_selected_cb), self);
  g_signal_connect (self->priv->app_chooser_widget, "application-activated",
                    G_CALLBACK (widget_application_activated_cb), self);
  g_signal_connect (self->priv->app_chooser_widget, "notify::show-other",
                    G_CALLBACK (widget_notify_for_button_cb), self);
  g_signal_connect (self->priv->app_chooser_widget, "populate-popup",
                    G_CALLBACK (widget_populate_popup_cb), self);

  /* Add the custom button to the new appchooser */
  ctk_box_pack_start (CTK_BOX (self->priv->inner_box),
		      self->priv->show_more_button, FALSE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (self->priv->inner_box),
		      self->priv->software_button, FALSE, FALSE, 0);

  info = ctk_app_chooser_get_app_info (CTK_APP_CHOOSER (self->priv->app_chooser_widget));
  ctk_dialog_set_response_sensitive (CTK_DIALOG (self), CTK_RESPONSE_OK, info != NULL);
  if (info)
    g_object_unref (info);

  _ctk_app_chooser_widget_set_search_entry (CTK_APP_CHOOSER_WIDGET (self->priv->app_chooser_widget),
                                            CTK_ENTRY (self->priv->search_entry));
  g_signal_connect (self, "key-press-event",
                    G_CALLBACK (key_press_event_cb), self->priv->search_bar);
}

static void
set_gfile_and_content_type (CtkAppChooserDialog *self,
                            GFile               *file)
{
  GFileInfo *info;

  if (file == NULL)
    return;

  self->priv->gfile = g_object_ref (file);

  info = g_file_query_info (self->priv->gfile,
                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                            0, NULL, NULL);
  self->priv->content_type = g_strdup (g_file_info_get_content_type (info));

  g_object_unref (info);
}

static GAppInfo *
ctk_app_chooser_dialog_get_app_info (CtkAppChooser *object)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (object);
  return ctk_app_chooser_get_app_info (CTK_APP_CHOOSER (self->priv->app_chooser_widget));
}

static void
ctk_app_chooser_dialog_refresh (CtkAppChooser *object)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (object);

  ctk_app_chooser_refresh (CTK_APP_CHOOSER (self->priv->app_chooser_widget));
}

static void
show_error_dialog (const gchar *primary,
                   const gchar *secondary,
                   CtkWindow   *parent)
{
  CtkWidget *message_dialog;

  message_dialog = ctk_message_dialog_new (parent, 0,
                                           CTK_MESSAGE_ERROR,
                                           CTK_BUTTONS_OK,
                                           NULL);
  g_object_set (message_dialog,
                "text", primary,
                "secondary-text", secondary,
                NULL);
  ctk_dialog_set_default_response (CTK_DIALOG (message_dialog), CTK_RESPONSE_OK);

  ctk_widget_show (message_dialog);

  g_signal_connect (message_dialog, "response",
                    G_CALLBACK (ctk_widget_destroy), NULL);
}

static void
software_button_clicked_cb (CtkButton           *button G_GNUC_UNUSED,
                            CtkAppChooserDialog *self)
{
  GSubprocess *process;
  GError *error = NULL;
  gchar *option;

  if (self->priv->content_type)
    option = g_strconcat ("--search=", self->priv->content_type, NULL);
  else
    option = g_strdup ("--mode=overview");

  process = g_subprocess_new (0, &error, "gnome-software", option, NULL);
  if (!process)
    {
      show_error_dialog (_("Failed to start GNOME Software"),
                         error->message, CTK_WINDOW (self));
      g_error_free (error);
    }
  else
    g_object_unref (process);

  g_free (option);
}

static void
ensure_software_button (CtkAppChooserDialog *self)
{
  gchar *path;

  path = g_find_program_in_path ("gnome-software");
  if (path != NULL)
    ctk_widget_show (self->priv->software_button);
  else
    ctk_widget_hide (self->priv->software_button);

  g_free (path);
}

static void
setup_search (CtkAppChooserDialog *self)
{
  gboolean use_header;

  g_object_get (self, "use-header-bar", &use_header, NULL);
  if (use_header)
    {
      CtkWidget *button;
      CtkWidget *image;
      CtkWidget *header;

      button = ctk_toggle_button_new ();
      ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
      image = ctk_image_new_from_icon_name ("edit-find-symbolic", CTK_ICON_SIZE_MENU);
      ctk_widget_show (image);
      ctk_container_add (CTK_CONTAINER (button), image);
      ctk_style_context_add_class (ctk_widget_get_style_context (button), "image-button");
      ctk_style_context_remove_class (ctk_widget_get_style_context (button), "text-button");
      ctk_widget_show (button);

      header = ctk_dialog_get_header_bar (CTK_DIALOG (self));
      ctk_header_bar_pack_end (CTK_HEADER_BAR (header), button);
      ctk_size_group_add_widget (self->priv->buttons, button);

      g_object_bind_property (button, "active",
                              self->priv->search_bar, "search-mode-enabled",
                              G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (self->priv->search_entry, "sensitive",
                              button, "sensitive",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
    }
}

static void
ctk_app_chooser_dialog_constructed (GObject *object)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (object);

  if (G_OBJECT_CLASS (ctk_app_chooser_dialog_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (ctk_app_chooser_dialog_parent_class)->constructed (object);

  construct_appchooser_widget (self);
  set_dialog_properties (self);
  ensure_software_button (self);
  setup_search (self);
}

/* This is necessary do deal with the fact that CtkDialog
 * exposes bits of its internal spacing as style properties,
 * and puts the action area inside the content area.
 * To achieve a flush-top search bar, we need the content
 * area border to be 0, and distribute the spacing to other
 * containers to compensate.
 */
static void
update_spacings (CtkAppChooserDialog *self)
{
  CtkWidget *widget;
  gint content_area_border;
  gint action_area_border;

  ctk_widget_style_get (CTK_WIDGET (self),
                        "content-area-border", &content_area_border,
                        "action-area-border", &action_area_border,
                        NULL);

  widget = ctk_dialog_get_content_area (CTK_DIALOG (self));
  ctk_container_set_border_width (CTK_CONTAINER (widget), 0);

  widget = ctk_dialog_get_action_area (CTK_DIALOG (self));

  ctk_container_set_border_width (CTK_CONTAINER (widget), 5 + content_area_border + action_area_border);

  widget = self->priv->inner_box;
  ctk_container_set_border_width (CTK_CONTAINER (widget), 10 + content_area_border);
}

static void
ctk_app_chooser_dialog_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_app_chooser_dialog_parent_class)->style_updated (widget);

  update_spacings (CTK_APP_CHOOSER_DIALOG (widget));
}

static void
ctk_app_chooser_dialog_dispose (GObject *object)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (object);
  
  g_clear_object (&self->priv->gfile);

  self->priv->dismissed = TRUE;

  G_OBJECT_CLASS (ctk_app_chooser_dialog_parent_class)->dispose (object);
}

static void
ctk_app_chooser_dialog_finalize (GObject *object)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (object);

  g_free (self->priv->content_type);
  g_free (self->priv->heading);

  G_OBJECT_CLASS (ctk_app_chooser_dialog_parent_class)->finalize (object);
}

static void
ctk_app_chooser_dialog_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (object);

  switch (property_id)
    {
    case PROP_GFILE:
      set_gfile_and_content_type (self, g_value_get_object (value));
      break;
    case PROP_CONTENT_TYPE:
      /* don't try to override a value previously set with the GFile */
      if (self->priv->content_type == NULL)
        self->priv->content_type = g_value_dup_string (value);
      break;
    case PROP_HEADING:
      ctk_app_chooser_dialog_set_heading (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_app_chooser_dialog_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CtkAppChooserDialog *self = CTK_APP_CHOOSER_DIALOG (object);

  switch (property_id)
    {
    case PROP_GFILE:
      if (self->priv->gfile != NULL)
        g_value_set_object (value, self->priv->gfile);
      break;
    case PROP_CONTENT_TYPE:
      g_value_set_string (value, self->priv->content_type);
      break;
    case PROP_HEADING:
      g_value_set_string (value, self->priv->heading);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_app_chooser_dialog_iface_init (CtkAppChooserIface *iface)
{
  iface->get_app_info = ctk_app_chooser_dialog_get_app_info;
  iface->refresh = ctk_app_chooser_dialog_refresh;
}

static void
ctk_app_chooser_dialog_class_init (CtkAppChooserDialogClass *klass)
{
  CtkWidgetClass *widget_class;
  GObjectClass *gobject_class;
  GParamSpec *pspec;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ctk_app_chooser_dialog_dispose;
  gobject_class->finalize = ctk_app_chooser_dialog_finalize;
  gobject_class->set_property = ctk_app_chooser_dialog_set_property;
  gobject_class->get_property = ctk_app_chooser_dialog_get_property;
  gobject_class->constructed = ctk_app_chooser_dialog_constructed;

  widget_class = CTK_WIDGET_CLASS (klass);
  widget_class->style_updated = ctk_app_chooser_dialog_style_updated;

  g_object_class_override_property (gobject_class, PROP_CONTENT_TYPE, "content-type");

  /**
   * CtkAppChooserDialog:gfile:
   *
   * The GFile used by the #CtkAppChooserDialog.
   * The dialog's #CtkAppChooserWidget content type will be guessed from the
   * file, if present.
   */
  pspec = g_param_spec_object ("gfile",
                               P_("GFile"),
                               P_("The GFile used by the app chooser dialog"),
                               G_TYPE_FILE,
                               G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (gobject_class, PROP_GFILE, pspec);

  /**
   * CtkAppChooserDialog:heading:
   *
   * The text to show at the top of the dialog.
   * The string may contain Pango markup.
   */
  pspec = g_param_spec_string ("heading",
                               P_("Heading"),
                               P_("The text to show at the top of the dialog"),
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                               G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_HEADING, pspec);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkappchooserdialog.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserDialog, label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserDialog, show_more_button);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserDialog, software_button);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserDialog, inner_box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserDialog, search_bar);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserDialog, search_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserDialog, buttons);
  ctk_widget_class_bind_template_callback (widget_class, show_more_button_clicked_cb);
  ctk_widget_class_bind_template_callback (widget_class, software_button_clicked_cb);
}

static void
ctk_app_chooser_dialog_init (CtkAppChooserDialog *self)
{
  self->priv = ctk_app_chooser_dialog_get_instance_private (self);

  ctk_widget_init_template (CTK_WIDGET (self));
  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (self));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_dialog_set_alternative_button_order (CTK_DIALOG (self),
                                           CTK_RESPONSE_OK,
                                           CTK_RESPONSE_CANCEL,
                                           -1);
G_GNUC_END_IGNORE_DEPRECATIONS

  /* we can't override the class signal handler here, as it's a RUN_LAST;
   * we want our signal handler instead to be executed before any user code.
   */
  g_signal_connect (self, "response",
                    G_CALLBACK (ctk_app_chooser_dialog_response), NULL);

  update_spacings (self);
}

static void
set_parent_and_flags (CtkWidget      *dialog,
                      CtkWindow      *parent,
                      CtkDialogFlags  flags)
{
  if (parent != NULL)
    ctk_window_set_transient_for (CTK_WINDOW (dialog), parent);

  if (flags & CTK_DIALOG_MODAL)
    ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);

  if (flags & CTK_DIALOG_DESTROY_WITH_PARENT)
    ctk_window_set_destroy_with_parent (CTK_WINDOW (dialog), TRUE);
}

/**
 * ctk_app_chooser_dialog_new:
 * @parent: (allow-none): a #CtkWindow, or %NULL
 * @flags: flags for this dialog
 * @file: a #GFile
 *
 * Creates a new #CtkAppChooserDialog for the provided #GFile,
 * to allow the user to select an application for it.
 *
 * Returns: a newly created #CtkAppChooserDialog
 *
 * Since: 3.0
 **/
CtkWidget *
ctk_app_chooser_dialog_new (CtkWindow      *parent,
                            CtkDialogFlags  flags,
                            GFile          *file)
{
  CtkWidget *retval;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  retval = g_object_new (CTK_TYPE_APP_CHOOSER_DIALOG,
                         "gfile", file,
                         NULL);

  set_parent_and_flags (retval, parent, flags);

  return retval;
}

/**
 * ctk_app_chooser_dialog_new_for_content_type:
 * @parent: (allow-none): a #CtkWindow, or %NULL
 * @flags: flags for this dialog
 * @content_type: a content type string
 *
 * Creates a new #CtkAppChooserDialog for the provided content type,
 * to allow the user to select an application for it.
 *
 * Returns: a newly created #CtkAppChooserDialog
 *
 * Since: 3.0
 **/
CtkWidget *
ctk_app_chooser_dialog_new_for_content_type (CtkWindow      *parent,
                                             CtkDialogFlags  flags,
                                             const gchar    *content_type)
{
  CtkWidget *retval;

  g_return_val_if_fail (content_type != NULL, NULL);

  retval = g_object_new (CTK_TYPE_APP_CHOOSER_DIALOG,
                         "content-type", content_type,
                         NULL);

  set_parent_and_flags (retval, parent, flags);

  return retval;
}

/**
 * ctk_app_chooser_dialog_get_widget:
 * @self: a #CtkAppChooserDialog
 *
 * Returns the #CtkAppChooserWidget of this dialog.
 *
 * Returns: (transfer none): the #CtkAppChooserWidget of @self
 *
 * Since: 3.0
 */
CtkWidget *
ctk_app_chooser_dialog_get_widget (CtkAppChooserDialog *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_DIALOG (self), NULL);

  return self->priv->app_chooser_widget;
}

/**
 * ctk_app_chooser_dialog_set_heading:
 * @self: a #CtkAppChooserDialog
 * @heading: a string containing Pango markup
 *
 * Sets the text to display at the top of the dialog.
 * If the heading is not set, the dialog displays a default text.
 */
void
ctk_app_chooser_dialog_set_heading (CtkAppChooserDialog *self,
                                    const gchar         *heading)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_DIALOG (self));

  g_free (self->priv->heading);
  self->priv->heading = g_strdup (heading);

  if (self->priv->label)
    {
      if (self->priv->heading)
        {
          ctk_label_set_markup (CTK_LABEL (self->priv->label), self->priv->heading);
          ctk_widget_show (self->priv->label);
        }
      else
        {
          ctk_widget_hide (self->priv->label);
        }
    }

  g_object_notify (G_OBJECT (self), "heading");
}

/**
 * ctk_app_chooser_dialog_get_heading:
 * @self: a #CtkAppChooserDialog
 *
 * Returns the text to display at the top of the dialog.
 *
 * Returns: (nullable): the text to display at the top of the dialog, or %NULL, in which
 *     case a default text is displayed
 */
const gchar *
ctk_app_chooser_dialog_get_heading (CtkAppChooserDialog *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_DIALOG (self), NULL);

  return self->priv->heading;
}
