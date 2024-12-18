/* ctkappchooserbutton.c: an app-chooser combobox
 *
 * Copyright (C) 2010 Red Hat, Inc.
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
 * Authors: Cosimo Cecchi <ccecchi@redhat.com>
 */

/**
 * SECTION:ctkappchooserbutton
 * @Title: CtkAppChooserButton
 * @Short_description: A button to launch an application chooser dialog
 *
 * The #CtkAppChooserButton is a widget that lets the user select
 * an application. It implements the #CtkAppChooser interface.
 *
 * Initially, a #CtkAppChooserButton selects the first application
 * in its list, which will either be the most-recently used application
 * or, if #CtkAppChooserButton:show-default-item is %TRUE, the
 * default application.
 *
 * The list of applications shown in a #CtkAppChooserButton includes
 * the recommended applications for the given content type. When
 * #CtkAppChooserButton:show-default-item is set, the default application
 * is also included. To let the user chooser other applications,
 * you can set the #CtkAppChooserButton:show-dialog-item property,
 * which allows to open a full #CtkAppChooserDialog.
 *
 * It is possible to add custom items to the list, using
 * ctk_app_chooser_button_append_custom_item(). These items cause
 * the #CtkAppChooserButton::custom-item-activated signal to be
 * emitted when they are selected.
 *
 * To track changes in the selected application, use the
 * #CtkComboBox::changed signal.
 */
#include "config.h"

#include "ctkappchooserbutton.h"

#include "ctkappchooser.h"
#include "ctkappchooserdialog.h"
#include "ctkappchooserprivate.h"
#include "ctkcelllayout.h"
#include "ctkcellrendererpixbuf.h"
#include "ctkcellrenderertext.h"
#include "ctkcombobox.h"
#include "ctkdialog.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"

enum {
  PROP_SHOW_DIALOG_ITEM = 1,
  PROP_SHOW_DEFAULT_ITEM,
  PROP_HEADING,
  NUM_PROPERTIES,

  PROP_CONTENT_TYPE = NUM_PROPERTIES
};

enum {
  SIGNAL_CUSTOM_ITEM_ACTIVATED,
  NUM_SIGNALS
};

enum {
  COLUMN_APP_INFO,
  COLUMN_NAME,
  COLUMN_LABEL,
  COLUMN_ICON,
  COLUMN_CUSTOM,
  COLUMN_SEPARATOR,
  NUM_COLUMNS,
};

#define CUSTOM_ITEM_OTHER_APP "ctk-internal-item-other-app"

static void app_chooser_iface_init  (CtkAppChooserIface *iface);

static void real_insert_custom_item (CtkAppChooserButton *self,
                                     const gchar         *name,
                                     const gchar         *label,
                                     GIcon               *icon,
                                     gboolean             custom,
                                     CtkTreeIter         *iter);

static void real_insert_separator   (CtkAppChooserButton *self,
                                     gboolean             custom,
                                     CtkTreeIter         *iter);

static guint signals[NUM_SIGNALS] = { 0, };
static GParamSpec *properties[NUM_PROPERTIES];

struct _CtkAppChooserButtonPrivate {
  CtkListStore *store;

  gchar *content_type;
  gchar *heading;
  gint last_active;
  gboolean show_dialog_item;
  gboolean show_default_item;

  GHashTable *custom_item_names;
};

G_DEFINE_TYPE_WITH_CODE (CtkAppChooserButton, ctk_app_chooser_button, CTK_TYPE_COMBO_BOX,
                         G_ADD_PRIVATE (CtkAppChooserButton)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_APP_CHOOSER,
                                                app_chooser_iface_init));

static gboolean
row_separator_func (CtkTreeModel *model,
                    CtkTreeIter  *iter,
                    gpointer      user_data G_GNUC_UNUSED)
{
  gboolean separator;

  ctk_tree_model_get (model, iter,
                      COLUMN_SEPARATOR, &separator,
                      -1);

  return separator;
}

static void
get_first_iter (CtkListStore *store,
                CtkTreeIter  *iter)
{
  CtkTreeIter iter2;

  if (!ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), iter))
    {
      /* the model is empty, append */
      ctk_list_store_append (store, iter);
    }
  else
    {
      ctk_list_store_insert_before (store, &iter2, iter);
      *iter = iter2;
    }
}

typedef struct {
  CtkAppChooserButton *self;
  GAppInfo *info;
  gint active_index;
} SelectAppData;

static void
select_app_data_free (SelectAppData *data)
{
  g_clear_object (&data->self);
  g_clear_object (&data->info);

  g_slice_free (SelectAppData, data);
}

static gboolean
select_application_func_cb (CtkTreeModel *model,
                            CtkTreePath  *path G_GNUC_UNUSED,
                            CtkTreeIter  *iter,
                            gpointer      user_data)
{
  SelectAppData *data = user_data;
  GAppInfo *app_to_match = data->info;
  GAppInfo *app = NULL;
  gboolean custom;
  gboolean result;

  ctk_tree_model_get (model, iter,
                      COLUMN_APP_INFO, &app,
                      COLUMN_CUSTOM, &custom,
                      -1);

  /* custom items are always after GAppInfos, so iterating further here
   * is just useless.
   */
  if (custom)
    result = TRUE;
  else if (g_app_info_equal (app, app_to_match))
    {
      ctk_combo_box_set_active_iter (CTK_COMBO_BOX (data->self), iter);
      result = TRUE;
    }
  else
    result = FALSE;

  g_object_unref (app);

  return result;
}

static void
ctk_app_chooser_button_select_application (CtkAppChooserButton *self,
                                           GAppInfo            *info)
{
  SelectAppData *data;

  data = g_slice_new0 (SelectAppData);
  data->self = g_object_ref (self);
  data->info = g_object_ref (info);

  ctk_tree_model_foreach (CTK_TREE_MODEL (self->priv->store),
                          select_application_func_cb, data);

  select_app_data_free (data);
}

static void
other_application_dialog_response_cb (CtkDialog *dialog,
                                      gint       response_id,
                                      gpointer   user_data)
{
  CtkAppChooserButton *self = user_data;
  GAppInfo *info;

  if (response_id != CTK_RESPONSE_OK)
    {
      /* reset the active item, otherwise we are stuck on
       * 'Other application…'
       */
      ctk_combo_box_set_active (CTK_COMBO_BOX (self), self->priv->last_active);
      ctk_widget_destroy (CTK_WIDGET (dialog));
      return;
    }

  info = ctk_app_chooser_get_app_info (CTK_APP_CHOOSER (dialog));

  ctk_widget_destroy (CTK_WIDGET (dialog));

  /* refresh the combobox to get the new application */
  ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
  ctk_app_chooser_button_select_application (self, info);

  g_object_unref (info);
}

static void
other_application_item_activated_cb (CtkAppChooserButton *self)
{
  CtkWidget *dialog, *widget;
  CtkWindow *toplevel;

  toplevel = CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (self)));
  dialog = ctk_app_chooser_dialog_new_for_content_type (toplevel,
                                                        CTK_DIALOG_DESTROY_WITH_PARENT,
                                                        self->priv->content_type);

  ctk_window_set_modal (CTK_WINDOW (dialog), ctk_window_get_modal (toplevel));
  ctk_app_chooser_dialog_set_heading (CTK_APP_CHOOSER_DIALOG (dialog),
                                      self->priv->heading);

  widget = ctk_app_chooser_dialog_get_widget (CTK_APP_CHOOSER_DIALOG (dialog));
  g_object_set (widget,
                "show-fallback", TRUE,
                "show-other", TRUE,
                NULL);
  ctk_widget_show (dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (other_application_dialog_response_cb), self);
}

static void
ctk_app_chooser_button_ensure_dialog_item (CtkAppChooserButton *self,
                                           CtkTreeIter         *prev_iter)
{
  CtkTreeIter iter, iter2;

  if (!self->priv->show_dialog_item || !self->priv->content_type)
    return;

  if (prev_iter == NULL)
    ctk_list_store_append (self->priv->store, &iter);
  else
    ctk_list_store_insert_after (self->priv->store, &iter, prev_iter);

  real_insert_separator (self, FALSE, &iter);
  iter2 = iter;

  ctk_list_store_insert_after (self->priv->store, &iter, &iter2);
  real_insert_custom_item (self, CUSTOM_ITEM_OTHER_APP,
                           _("Other application…"), NULL,
                           FALSE, &iter);
}

static void
insert_one_application (CtkAppChooserButton *self,
                        GAppInfo            *app,
                        CtkTreeIter         *iter)
{
  GIcon *icon;

  icon = g_app_info_get_icon (app);

  if (icon == NULL)
    icon = g_themed_icon_new ("application-x-executable");
  else
    g_object_ref (icon);

  ctk_list_store_set (self->priv->store, iter,
                      COLUMN_APP_INFO, app,
                      COLUMN_LABEL, g_app_info_get_name (app),
                      COLUMN_ICON, icon,
                      COLUMN_CUSTOM, FALSE,
                      -1);

  g_object_unref (icon);
}

static void
ctk_app_chooser_button_populate (CtkAppChooserButton *self)
{
  GList *recommended_apps = NULL, *l;
  GAppInfo *app, *default_app = NULL;
  CtkTreeIter iter, iter2;
  gboolean cycled_recommended;

#ifndef G_OS_WIN32
  if (self->priv->content_type)
    recommended_apps = g_app_info_get_recommended_for_type (self->priv->content_type);
#endif
  cycled_recommended = FALSE;

  if (self->priv->show_default_item)
    {
      if (self->priv->content_type)
        default_app = g_app_info_get_default_for_type (self->priv->content_type, FALSE);

      if (default_app != NULL)
        {
          get_first_iter (self->priv->store, &iter);
          cycled_recommended = TRUE;

          insert_one_application (self, default_app, &iter);

          g_object_unref (default_app);
        }
    }

  for (l = recommended_apps; l != NULL; l = l->next)
    {
      app = l->data;

      if (default_app != NULL && g_app_info_equal (app, default_app))
        continue;

      if (cycled_recommended)
        {
          ctk_list_store_insert_after (self->priv->store, &iter2, &iter);
          iter = iter2;
        }
      else
        {
          get_first_iter (self->priv->store, &iter);
          cycled_recommended = TRUE;
        }

      insert_one_application (self, app, &iter);
    }

  if (recommended_apps != NULL)
    g_list_free_full (recommended_apps, g_object_unref);

  if (!cycled_recommended)
    ctk_app_chooser_button_ensure_dialog_item (self, NULL);
  else
    ctk_app_chooser_button_ensure_dialog_item (self, &iter);

  ctk_combo_box_set_active (CTK_COMBO_BOX (self), 0);
}

static void
ctk_app_chooser_button_build_ui (CtkAppChooserButton *self)
{
  CtkCellRenderer *cell;
  CtkCellArea *area;

  ctk_combo_box_set_model (CTK_COMBO_BOX (self),
                           CTK_TREE_MODEL (self->priv->store));

  area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (self));

  ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (self),
                                        row_separator_func, NULL, NULL);

  cell = ctk_cell_renderer_pixbuf_new ();
  ctk_cell_area_add_with_properties (area, cell,
                                     "align", FALSE,
                                     "expand", FALSE,
                                     "fixed-size", FALSE,
                                     NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (self), cell,
                                  "gicon", COLUMN_ICON,
                                  NULL);

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_area_add_with_properties (area, cell,
                                     "align", FALSE,
                                     "expand", TRUE,
                                     NULL);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (self), cell,
                                  "text", COLUMN_LABEL,
                                  NULL);

  ctk_app_chooser_button_populate (self);
}

static void
ctk_app_chooser_button_remove_non_custom (CtkAppChooserButton *self)
{
  CtkTreeModel *model;
  CtkTreeIter iter;
  gboolean custom, res;

  model = CTK_TREE_MODEL (self->priv->store);

  if (!ctk_tree_model_get_iter_first (model, &iter))
    return;

  do {
    ctk_tree_model_get (model, &iter,
                        COLUMN_CUSTOM, &custom,
                        -1);
    if (custom)
      res = ctk_tree_model_iter_next (model, &iter);
    else
      res = ctk_list_store_remove (CTK_LIST_STORE (model), &iter);
  } while (res);
}

static void
ctk_app_chooser_button_changed (CtkComboBox *object)
{
  CtkAppChooserButton *self = CTK_APP_CHOOSER_BUTTON (object);
  CtkTreeIter iter;
  gchar *name = NULL;
  gboolean custom;
  GQuark name_quark;

  if (!ctk_combo_box_get_active_iter (object, &iter))
    return;

  ctk_tree_model_get (CTK_TREE_MODEL (self->priv->store), &iter,
                      COLUMN_NAME, &name,
                      COLUMN_CUSTOM, &custom,
                      -1);

  if (name != NULL)
    {
      if (custom)
        {
          name_quark = g_quark_from_string (name);
          g_signal_emit (self, signals[SIGNAL_CUSTOM_ITEM_ACTIVATED], name_quark, name);
          self->priv->last_active = ctk_combo_box_get_active (object);
        }
      else
        {
          /* trigger the dialog internally */
          other_application_item_activated_cb (self);
        }

      g_free (name);
    }
  else
    self->priv->last_active = ctk_combo_box_get_active (object);
}

static void
ctk_app_chooser_button_refresh (CtkAppChooser *object)
{
  CtkAppChooserButton *self = CTK_APP_CHOOSER_BUTTON (object);

  ctk_app_chooser_button_remove_non_custom (self);
  ctk_app_chooser_button_populate (self);
}

static GAppInfo *
ctk_app_chooser_button_get_app_info (CtkAppChooser *object)
{
  CtkAppChooserButton *self = CTK_APP_CHOOSER_BUTTON (object);
  CtkTreeIter iter;
  GAppInfo *info;

  if (!ctk_combo_box_get_active_iter (CTK_COMBO_BOX (self), &iter))
    return NULL;

  ctk_tree_model_get (CTK_TREE_MODEL (self->priv->store), &iter,
                      COLUMN_APP_INFO, &info,
                      -1);

  return info;
}

static void
ctk_app_chooser_button_constructed (GObject *obj)
{
  CtkAppChooserButton *self = CTK_APP_CHOOSER_BUTTON (obj);

  if (G_OBJECT_CLASS (ctk_app_chooser_button_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (ctk_app_chooser_button_parent_class)->constructed (obj);

  ctk_app_chooser_button_build_ui (self);
}

static void
ctk_app_chooser_button_set_property (GObject      *obj,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkAppChooserButton *self = CTK_APP_CHOOSER_BUTTON (obj);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      self->priv->content_type = g_value_dup_string (value);
      break;
    case PROP_SHOW_DIALOG_ITEM:
      ctk_app_chooser_button_set_show_dialog_item (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_DEFAULT_ITEM:
      ctk_app_chooser_button_set_show_default_item (self, g_value_get_boolean (value));
      break;
    case PROP_HEADING:
      ctk_app_chooser_button_set_heading (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_app_chooser_button_get_property (GObject    *obj,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CtkAppChooserButton *self = CTK_APP_CHOOSER_BUTTON (obj);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      g_value_set_string (value, self->priv->content_type);
      break;
    case PROP_SHOW_DIALOG_ITEM:
      g_value_set_boolean (value, self->priv->show_dialog_item);
      break;
    case PROP_SHOW_DEFAULT_ITEM:
      g_value_set_boolean (value, self->priv->show_default_item);
      break;
    case PROP_HEADING:
      g_value_set_string (value, self->priv->heading);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_app_chooser_button_finalize (GObject *obj)
{
  CtkAppChooserButton *self = CTK_APP_CHOOSER_BUTTON (obj);

  g_hash_table_destroy (self->priv->custom_item_names);
  g_free (self->priv->content_type);
  g_free (self->priv->heading);
  g_object_unref (self->priv->store);

  G_OBJECT_CLASS (ctk_app_chooser_button_parent_class)->finalize (obj);
}

static void
app_chooser_iface_init (CtkAppChooserIface *iface)
{
  iface->get_app_info = ctk_app_chooser_button_get_app_info;
  iface->refresh = ctk_app_chooser_button_refresh;
}

static void
ctk_app_chooser_button_class_init (CtkAppChooserButtonClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  CtkComboBoxClass *combo_class = CTK_COMBO_BOX_CLASS (klass);

  oclass->set_property = ctk_app_chooser_button_set_property;
  oclass->get_property = ctk_app_chooser_button_get_property;
  oclass->finalize = ctk_app_chooser_button_finalize;
  oclass->constructed = ctk_app_chooser_button_constructed;

  combo_class->changed = ctk_app_chooser_button_changed;

  g_object_class_override_property (oclass, PROP_CONTENT_TYPE, "content-type");

  /**
   * CtkAppChooserButton:show-dialog-item:
   *
   * The #CtkAppChooserButton:show-dialog-item property determines
   * whether the dropdown menu should show an item that triggers
   * a #CtkAppChooserDialog when clicked.
   */
  properties[PROP_SHOW_DIALOG_ITEM] =
    g_param_spec_boolean ("show-dialog-item",
                          P_("Include an 'Other…' item"),
                          P_("Whether the combobox should include an item that triggers a CtkAppChooserDialog"),
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkAppChooserButton:show-default-item:
   *
   * The #CtkAppChooserButton:show-default-item property determines
   * whether the dropdown menu should show the default application
   * on top for the provided content type.
   *
   * Since: 3.2
   */
  properties[PROP_SHOW_DEFAULT_ITEM] =
    g_param_spec_boolean ("show-default-item",
                          P_("Show default item"),
                          P_("Whether the combobox should show the default application on top"),
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkAppChooserButton:heading:
   *
   * The text to show at the top of the dialog that can be
   * opened from the button. The string may contain Pango markup.
   */
  properties[PROP_HEADING] =
    g_param_spec_string ("heading",
                         P_("Heading"),
                         P_("The text to show at the top of the dialog"),
                         NULL,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);

  /**
   * CtkAppChooserButton::custom-item-activated:
   * @self: the object which received the signal
   * @item_name: the name of the activated item
   *
   * Emitted when a custom item, previously added with
   * ctk_app_chooser_button_append_custom_item(), is activated from the
   * dropdown menu.
   */
  signals[SIGNAL_CUSTOM_ITEM_ACTIVATED] =
    g_signal_new (I_("custom-item-activated"),
                  CTK_TYPE_APP_CHOOSER_BUTTON,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (CtkAppChooserButtonClass, custom_item_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_STRING);
}

static void
ctk_app_chooser_button_init (CtkAppChooserButton *self)
{
  self->priv = ctk_app_chooser_button_get_instance_private (self);
  self->priv->custom_item_names =
    g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->priv->store = ctk_list_store_new (NUM_COLUMNS,
                                          G_TYPE_APP_INFO,
                                          G_TYPE_STRING, /* name */
                                          G_TYPE_STRING, /* label */
                                          G_TYPE_ICON,
                                          G_TYPE_BOOLEAN, /* separator */
                                          G_TYPE_BOOLEAN); /* custom */
}

static gboolean
app_chooser_button_iter_from_custom_name (CtkAppChooserButton *self,
                                          const gchar         *name,
                                          CtkTreeIter         *set_me)
{
  CtkTreeIter iter;
  gchar *custom_name = NULL;

  if (!ctk_tree_model_get_iter_first (CTK_TREE_MODEL (self->priv->store), &iter))
    return FALSE;

  do {
    ctk_tree_model_get (CTK_TREE_MODEL (self->priv->store), &iter,
                        COLUMN_NAME, &custom_name,
                        -1);

    if (g_strcmp0 (custom_name, name) == 0)
      {
        g_free (custom_name);
        *set_me = iter;

        return TRUE;
      }

    g_free (custom_name);
  } while (ctk_tree_model_iter_next (CTK_TREE_MODEL (self->priv->store), &iter));

  return FALSE;
}

static void
real_insert_custom_item (CtkAppChooserButton *self,
                         const gchar         *name,
                         const gchar         *label,
                         GIcon               *icon,
                         gboolean             custom,
                         CtkTreeIter         *iter)
{
  if (custom)
    {
      if (g_hash_table_lookup (self->priv->custom_item_names, name) != NULL)
        {
          g_warning ("Attempting to add custom item %s to CtkAppChooserButton, "
                     "when there's already an item with the same name", name);
          return;
        }

      g_hash_table_insert (self->priv->custom_item_names,
                           g_strdup (name), GINT_TO_POINTER (1));
    }

  ctk_list_store_set (self->priv->store, iter,
                      COLUMN_NAME, name,
                      COLUMN_LABEL, label,
                      COLUMN_ICON, icon,
                      COLUMN_CUSTOM, custom,
                      COLUMN_SEPARATOR, FALSE,
                      -1);
}

static void
real_insert_separator (CtkAppChooserButton *self,
                       gboolean             custom,
                       CtkTreeIter         *iter)
{
  ctk_list_store_set (self->priv->store, iter,
                      COLUMN_CUSTOM, custom,
                      COLUMN_SEPARATOR, TRUE,
                      -1);
}

/**
 * ctk_app_chooser_button_new:
 * @content_type: the content type to show applications for
 *
 * Creates a new #CtkAppChooserButton for applications
 * that can handle content of the given type.
 *
 * Returns: a newly created #CtkAppChooserButton
 *
 * Since: 3.0
 */
CtkWidget *
ctk_app_chooser_button_new (const gchar *content_type)
{
  g_return_val_if_fail (content_type != NULL, NULL);

  return g_object_new (CTK_TYPE_APP_CHOOSER_BUTTON,
                       "content-type", content_type,
                       NULL);
}

/**
 * ctk_app_chooser_button_append_separator:
 * @self: a #CtkAppChooserButton
 *
 * Appends a separator to the list of applications that is shown
 * in the popup.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_button_append_separator (CtkAppChooserButton *self)
{
  CtkTreeIter iter;

  g_return_if_fail (CTK_IS_APP_CHOOSER_BUTTON (self));

  ctk_list_store_append (self->priv->store, &iter);
  real_insert_separator (self, TRUE, &iter);
}

/**
 * ctk_app_chooser_button_append_custom_item:
 * @self: a #CtkAppChooserButton
 * @name: the name of the custom item
 * @label: the label for the custom item
 * @icon: the icon for the custom item
 *
 * Appends a custom item to the list of applications that is shown
 * in the popup; the item name must be unique per-widget.
 * Clients can use the provided name as a detail for the
 * #CtkAppChooserButton::custom-item-activated signal, to add a
 * callback for the activation of a particular custom item in the list.
 * See also ctk_app_chooser_button_append_separator().
 *
 * Since: 3.0
 */
void
ctk_app_chooser_button_append_custom_item (CtkAppChooserButton *self,
                                           const gchar         *name,
                                           const gchar         *label,
                                           GIcon               *icon)
{
  CtkTreeIter iter;

  g_return_if_fail (CTK_IS_APP_CHOOSER_BUTTON (self));
  g_return_if_fail (name != NULL);

  ctk_list_store_append (self->priv->store, &iter);
  real_insert_custom_item (self, name, label, icon, TRUE, &iter);
}

/**
 * ctk_app_chooser_button_set_active_custom_item:
 * @self: a #CtkAppChooserButton
 * @name: the name of the custom item
 *
 * Selects a custom item previously added with
 * ctk_app_chooser_button_append_custom_item().
 *
 * Use ctk_app_chooser_refresh() to bring the selection
 * to its initial state.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_button_set_active_custom_item (CtkAppChooserButton *self,
                                               const gchar         *name)
{
  CtkTreeIter iter;

  g_return_if_fail (CTK_IS_APP_CHOOSER_BUTTON (self));
  g_return_if_fail (name != NULL);

  if (!g_hash_table_contains (self->priv->custom_item_names, name) ||
      !app_chooser_button_iter_from_custom_name (self, name, &iter))
    {
      g_warning ("Can't find the item named %s in the app chooser.", name);
      return;
    }

  ctk_combo_box_set_active_iter (CTK_COMBO_BOX (self), &iter);
}

/**
 * ctk_app_chooser_button_get_show_dialog_item:
 * @self: a #CtkAppChooserButton
 *
 * Returns the current value of the #CtkAppChooserButton:show-dialog-item
 * property.
 *
 * Returns: the value of #CtkAppChooserButton:show-dialog-item
 *
 * Since: 3.0
 */
gboolean
ctk_app_chooser_button_get_show_dialog_item (CtkAppChooserButton *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_BUTTON (self), FALSE);

  return self->priv->show_dialog_item;
}

/**
 * ctk_app_chooser_button_set_show_dialog_item:
 * @self: a #CtkAppChooserButton
 * @setting: the new value for #CtkAppChooserButton:show-dialog-item
 *
 * Sets whether the dropdown menu of this button should show an
 * entry to trigger a #CtkAppChooserDialog.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_button_set_show_dialog_item (CtkAppChooserButton *self,
                                             gboolean             setting)
{
  if (self->priv->show_dialog_item != setting)
    {
      self->priv->show_dialog_item = setting;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_DIALOG_ITEM]);

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_button_get_show_default_item:
 * @self: a #CtkAppChooserButton
 *
 * Returns the current value of the #CtkAppChooserButton:show-default-item
 * property.
 *
 * Returns: the value of #CtkAppChooserButton:show-default-item
 *
 * Since: 3.2
 */
gboolean
ctk_app_chooser_button_get_show_default_item (CtkAppChooserButton *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_BUTTON (self), FALSE);

  return self->priv->show_default_item;
}

/**
 * ctk_app_chooser_button_set_show_default_item:
 * @self: a #CtkAppChooserButton
 * @setting: the new value for #CtkAppChooserButton:show-default-item
 *
 * Sets whether the dropdown menu of this button should show the
 * default application for the given content type at top.
 *
 * Since: 3.2
 */
void
ctk_app_chooser_button_set_show_default_item (CtkAppChooserButton *self,
                                              gboolean             setting)
{
  if (self->priv->show_default_item != setting)
    {
      self->priv->show_default_item = setting;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_DEFAULT_ITEM]);

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_button_set_heading:
 * @self: a #CtkAppChooserButton
 * @heading: a string containing Pango markup
 *
 * Sets the text to display at the top of the dialog.
 * If the heading is not set, the dialog displays a default text.
 */
void
ctk_app_chooser_button_set_heading (CtkAppChooserButton *self,
                                    const gchar         *heading)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_BUTTON (self));

  g_free (self->priv->heading);
  self->priv->heading = g_strdup (heading);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HEADING]);
}

/**
 * ctk_app_chooser_button_get_heading:
 * @self: a #CtkAppChooserButton
 *
 * Returns the text to display at the top of the dialog.
 *
 * Returns: (nullable): the text to display at the top of the dialog,
 *     or %NULL, in which case a default text is displayed
 */
const gchar *
ctk_app_chooser_button_get_heading (CtkAppChooserButton *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_BUTTON (self), NULL);

  return self->priv->heading;
}
