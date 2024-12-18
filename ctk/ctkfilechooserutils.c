/* CTK - The GIMP Toolkit
 * ctkfilechooserutils.c: Private utility functions useful for
 *                        implementing a CtkFileChooser interface
 * Copyright (C) 2003, Red Hat, Inc.
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
#include "ctkfilechooserutils.h"
#include "ctkfilechooser.h"
#include "ctkfilesystem.h"
#include "ctktypebuiltins.h"
#include "ctkintl.h"


static gboolean       delegate_set_current_folder     (CtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static GFile *        delegate_get_current_folder     (CtkFileChooser    *chooser);
static void           delegate_set_current_name       (CtkFileChooser    *chooser,
						       const gchar       *name);
static gchar *        delegate_get_current_name       (CtkFileChooser    *chooser);
static gboolean       delegate_select_file            (CtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static void           delegate_unselect_file          (CtkFileChooser    *chooser,
						       GFile             *file);
static void           delegate_select_all             (CtkFileChooser    *chooser);
static void           delegate_unselect_all           (CtkFileChooser    *chooser);
static GSList *       delegate_get_files              (CtkFileChooser    *chooser);
static GFile *        delegate_get_preview_file       (CtkFileChooser    *chooser);
static CtkFileSystem *delegate_get_file_system        (CtkFileChooser    *chooser);
static void           delegate_add_filter             (CtkFileChooser    *chooser,
						       CtkFileFilter     *filter);
static void           delegate_remove_filter          (CtkFileChooser    *chooser,
						       CtkFileFilter     *filter);
static GSList *       delegate_list_filters           (CtkFileChooser    *chooser);
static gboolean       delegate_add_shortcut_folder    (CtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static gboolean       delegate_remove_shortcut_folder (CtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static GSList *       delegate_list_shortcut_folders  (CtkFileChooser    *chooser);
static void           delegate_notify                 (GObject           *object,
						       GParamSpec        *pspec,
						       gpointer           data);
static void           delegate_current_folder_changed (CtkFileChooser    *chooser,
						       gpointer           data);
static void           delegate_selection_changed      (CtkFileChooser    *chooser,
						       gpointer           data);
static void           delegate_update_preview         (CtkFileChooser    *chooser,
						       gpointer           data);
static void           delegate_file_activated         (CtkFileChooser    *chooser,
						       gpointer           data);

static CtkFileChooserConfirmation delegate_confirm_overwrite (CtkFileChooser    *chooser,
							      gpointer           data);
static void           delegate_add_choice             (CtkFileChooser  *chooser,
                                                       const char      *id,
                                                       const char      *label,
                                                       const char     **options,
                                                       const char     **option_labels);
static void           delegate_remove_choice          (CtkFileChooser  *chooser,
                                                       const char      *id);
static void           delegate_set_choice             (CtkFileChooser  *chooser,
                                                       const char      *id,
                                                       const char      *option);
static const char *   delegate_get_choice             (CtkFileChooser  *chooser,
                                                       const char      *id);


/**
 * _ctk_file_chooser_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #CtkFileChooser. A #CtkParamSpecOverride property is installed
 * for each property, using the values from the #CtkFileChooserProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don’t collide with some other property values they
 * are using.
 **/
void
_ctk_file_chooser_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_ACTION,
				    "action");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_EXTRA_WIDGET,
				    "extra-widget");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_FILTER,
				    "filter");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_LOCAL_ONLY,
				    "local-only");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET,
				    "preview-widget");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE,
				    "preview-widget-active");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL,
				    "use-preview-label");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE,
				    "select-multiple");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_SHOW_HIDDEN,
				    "show-hidden");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION,
				    "do-overwrite-confirmation");
  g_object_class_override_property (klass,
				    CTK_FILE_CHOOSER_PROP_CREATE_FOLDERS,
				    "create-folders");
}

/**
 * _ctk_file_chooser_delegate_iface_init:
 * @iface: a #CtkFileChoserIface structure
 *
 * An interface-initialization function for use in cases where
 * an object is simply delegating the methods, signals of
 * the #CtkFileChooser interface to another object.
 * _ctk_file_chooser_set_delegate() must be called on each
 * instance of the object so that the delegate object can
 * be found.
 **/
void
_ctk_file_chooser_delegate_iface_init (CtkFileChooserIface *iface)
{
  iface->set_current_folder = delegate_set_current_folder;
  iface->get_current_folder = delegate_get_current_folder;
  iface->set_current_name = delegate_set_current_name;
  iface->get_current_name = delegate_get_current_name;
  iface->select_file = delegate_select_file;
  iface->unselect_file = delegate_unselect_file;
  iface->select_all = delegate_select_all;
  iface->unselect_all = delegate_unselect_all;
  iface->get_files = delegate_get_files;
  iface->get_preview_file = delegate_get_preview_file;
  iface->get_file_system = delegate_get_file_system;
  iface->add_filter = delegate_add_filter;
  iface->remove_filter = delegate_remove_filter;
  iface->list_filters = delegate_list_filters;
  iface->add_shortcut_folder = delegate_add_shortcut_folder;
  iface->remove_shortcut_folder = delegate_remove_shortcut_folder;
  iface->list_shortcut_folders = delegate_list_shortcut_folders;
  iface->add_choice = delegate_add_choice;
  iface->remove_choice = delegate_remove_choice;
  iface->set_choice = delegate_set_choice;
  iface->get_choice = delegate_get_choice;
}

/**
 * _ctk_file_chooser_set_delegate:
 * @receiver: a #GObject implementing #CtkFileChooser
 * @delegate: another #GObject implementing #CtkFileChooser
 *
 * Establishes that calls on @receiver for #CtkFileChooser
 * methods should be delegated to @delegate, and that
 * #CtkFileChooser signals emitted on @delegate should be
 * forwarded to @receiver. Must be used in conjunction with
 * _ctk_file_chooser_delegate_iface_init().
 **/
void
_ctk_file_chooser_set_delegate (CtkFileChooser *receiver,
				CtkFileChooser *delegate)
{
  g_return_if_fail (CTK_IS_FILE_CHOOSER (receiver));
  g_return_if_fail (CTK_IS_FILE_CHOOSER (delegate));

  g_object_set_data (G_OBJECT (receiver), I_("ctk-file-chooser-delegate"), delegate);
  g_signal_connect (delegate, "notify",
		    G_CALLBACK (delegate_notify), receiver);
  g_signal_connect (delegate, "current-folder-changed",
		    G_CALLBACK (delegate_current_folder_changed), receiver);
  g_signal_connect (delegate, "selection-changed",
		    G_CALLBACK (delegate_selection_changed), receiver);
  g_signal_connect (delegate, "update-preview",
		    G_CALLBACK (delegate_update_preview), receiver);
  g_signal_connect (delegate, "file-activated",
		    G_CALLBACK (delegate_file_activated), receiver);
  g_signal_connect (delegate, "confirm-overwrite",
		    G_CALLBACK (delegate_confirm_overwrite), receiver);
}

GQuark
_ctk_file_chooser_delegate_get_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("ctk-file-chooser-delegate");
  
  return quark;
}

static CtkFileChooser *
get_delegate (CtkFileChooser *receiver)
{
  return g_object_get_qdata (G_OBJECT (receiver),
			     CTK_FILE_CHOOSER_DELEGATE_QUARK);
}

static gboolean
delegate_select_file (CtkFileChooser    *chooser,
		      GFile             *file,
		      GError           **error)
{
  return ctk_file_chooser_select_file (get_delegate (chooser), file, error);
}

static void
delegate_unselect_file (CtkFileChooser *chooser,
			GFile          *file)
{
  ctk_file_chooser_unselect_file (get_delegate (chooser), file);
}

static void
delegate_select_all (CtkFileChooser *chooser)
{
  ctk_file_chooser_select_all (get_delegate (chooser));
}

static void
delegate_unselect_all (CtkFileChooser *chooser)
{
  ctk_file_chooser_unselect_all (get_delegate (chooser));
}

static GSList *
delegate_get_files (CtkFileChooser *chooser)
{
  return ctk_file_chooser_get_files (get_delegate (chooser));
}

static GFile *
delegate_get_preview_file (CtkFileChooser *chooser)
{
  return ctk_file_chooser_get_preview_file (get_delegate (chooser));
}

static CtkFileSystem *
delegate_get_file_system (CtkFileChooser *chooser)
{
  return _ctk_file_chooser_get_file_system (get_delegate (chooser));
}

static void
delegate_add_filter (CtkFileChooser *chooser,
		     CtkFileFilter  *filter)
{
  ctk_file_chooser_add_filter (get_delegate (chooser), filter);
}

static void
delegate_remove_filter (CtkFileChooser *chooser,
			CtkFileFilter  *filter)
{
  ctk_file_chooser_remove_filter (get_delegate (chooser), filter);
}

static GSList *
delegate_list_filters (CtkFileChooser *chooser)
{
  return ctk_file_chooser_list_filters (get_delegate (chooser));
}

static gboolean
delegate_add_shortcut_folder (CtkFileChooser  *chooser,
			      GFile           *file,
			      GError         **error)
{
  return _ctk_file_chooser_add_shortcut_folder (get_delegate (chooser), file, error);
}

static gboolean
delegate_remove_shortcut_folder (CtkFileChooser  *chooser,
				 GFile           *file,
				 GError         **error)
{
  return _ctk_file_chooser_remove_shortcut_folder (get_delegate (chooser), file, error);
}

static GSList *
delegate_list_shortcut_folders (CtkFileChooser *chooser)
{
  return _ctk_file_chooser_list_shortcut_folder_files (get_delegate (chooser));
}

static gboolean
delegate_set_current_folder (CtkFileChooser  *chooser,
			     GFile           *file,
			     GError         **error)
{
  return ctk_file_chooser_set_current_folder_file (get_delegate (chooser), file, error);
}

static GFile *
delegate_get_current_folder (CtkFileChooser *chooser)
{
  return ctk_file_chooser_get_current_folder_file (get_delegate (chooser));
}

static void
delegate_set_current_name (CtkFileChooser *chooser,
			   const gchar    *name)
{
  ctk_file_chooser_set_current_name (get_delegate (chooser), name);
}

static gchar *
delegate_get_current_name (CtkFileChooser *chooser)
{
  return ctk_file_chooser_get_current_name (get_delegate (chooser));
}

static void
delegate_notify (GObject    *object,
		 GParamSpec *pspec,
		 gpointer    data)
{
  gpointer iface;

  iface = g_type_interface_peek (g_type_class_peek (G_OBJECT_TYPE (object)),
				 ctk_file_chooser_get_type ());
  if (g_object_interface_find_property (iface, pspec->name))
    g_object_notify (data, pspec->name);
}

static void
delegate_selection_changed (CtkFileChooser *chooser G_GNUC_UNUSED,
			    gpointer        data)
{
  g_signal_emit_by_name (data, "selection-changed");
}

static void
delegate_current_folder_changed (CtkFileChooser *chooser G_GNUC_UNUSED,
				 gpointer        data)
{
  g_signal_emit_by_name (data, "current-folder-changed");
}

static void
delegate_update_preview (CtkFileChooser    *chooser G_GNUC_UNUSED,
			 gpointer           data)
{
  g_signal_emit_by_name (data, "update-preview");
}

static void
delegate_file_activated (CtkFileChooser *chooser G_GNUC_UNUSED,
			 gpointer        data)
{
  g_signal_emit_by_name (data, "file-activated");
}

static CtkFileChooserConfirmation
delegate_confirm_overwrite (CtkFileChooser *chooser G_GNUC_UNUSED,
			    gpointer        data)
{
  CtkFileChooserConfirmation conf;

  g_signal_emit_by_name (data, "confirm-overwrite", &conf);
  return conf;
}

static GFile *
get_parent_for_uri (const char *uri)
{
  GFile *file;
  GFile *parent;

  file = g_file_new_for_uri (uri);
  parent = g_file_get_parent (file);

  g_object_unref (file);
  return parent;
	
}

/* Extracts the parent folders out of the supplied list of CtkRecentInfo* items, and returns
 * a list of GFile* for those unique parents.
 */
GList *
_ctk_file_chooser_extract_recent_folders (GList *infos)
{
  GList *l;
  GList *result;
  GHashTable *folders;

  result = NULL;

  folders = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);

  for (l = infos; l; l = l->next)
    {
      CtkRecentInfo *info = l->data;
      const char *uri;
      GFile *parent;

      uri = ctk_recent_info_get_uri (info);
      parent = get_parent_for_uri (uri);

      if (parent)
	{
	  if (!g_hash_table_lookup (folders, parent))
	    {
	      g_hash_table_insert (folders, parent, (gpointer) 1);
	      result = g_list_prepend (result, g_object_ref (parent));
	    }

	  g_object_unref (parent);
	}
    }

  result = g_list_reverse (result);

  g_hash_table_destroy (folders);

  return result;
}

GSettings *
_ctk_file_chooser_get_settings_for_widget (CtkWidget *widget)
{
  static GQuark file_chooser_settings_quark = 0;
  CtkSettings *ctksettings;
  GSettings *settings;

  if (G_UNLIKELY (file_chooser_settings_quark == 0))
    file_chooser_settings_quark = g_quark_from_static_string ("-ctk-file-chooser-settings");

  ctksettings = ctk_widget_get_settings (widget);
  settings = g_object_get_qdata (G_OBJECT (ctksettings), file_chooser_settings_quark);

  if (G_UNLIKELY (settings == NULL))
    {
      settings = g_settings_new ("org.ctk.Settings.FileChooser");
      g_settings_delay (settings);

      g_object_set_qdata_full (G_OBJECT (ctksettings),
                               file_chooser_settings_quark,
                               settings,
                               g_object_unref);
    }

  return settings;
}

gchar *
_ctk_file_chooser_label_for_file (GFile *file)
{
  const gchar *path, *start, *end, *p;
  gchar *uri, *host, *label;

  uri = g_file_get_uri (file);

  start = strstr (uri, "://");
  if (start)
    {
      start += 3;
      path = strchr (start, '/');
      if (path)
        end = path;
      else
        {
          end = uri + strlen (uri);
          path = "/";
        }

      /* strip username */
      p = strchr (start, '@');
      if (p && p < end)
        start = p + 1;

      p = strchr (start, ':');
      if (p && p < end)
        end = p;

      host = g_strndup (start, end - start);
      /* Translators: the first string is a path and the second string 
       * is a hostname. Nautilus and the panel contain the same string 
       * to translate. 
       */
      label = g_strdup_printf (_("%1$s on %2$s"), path, host);

      g_free (host);
    }
  else
    {
      label = g_strdup (uri);
    }

  g_free (uri);

  return label;
}

static void
delegate_add_choice (CtkFileChooser *chooser,
                     const char      *id,
                     const char      *label,
                     const char     **options,
                     const char     **option_labels)
{
  ctk_file_chooser_add_choice (get_delegate (chooser),
                               id, label, options, option_labels);
}
static void
delegate_remove_choice (CtkFileChooser  *chooser,
                        const char      *id)
{
  ctk_file_chooser_remove_choice (get_delegate (chooser), id);
}

static void
delegate_set_choice (CtkFileChooser  *chooser,
                     const char      *id,
                     const char      *option)
{
  ctk_file_chooser_set_choice (get_delegate (chooser), id, option);
}


static const char *
delegate_get_choice (CtkFileChooser  *chooser,
                     const char      *id)
{
  return ctk_file_chooser_get_choice (get_delegate (chooser), id);
}
