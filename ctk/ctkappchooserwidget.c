/*
 * ctkappchooserwidget.c: an app-chooser widget
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

#include "config.h"

#include "ctkappchooserwidget.h"

#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkappchooserwidget.h"
#include "ctkappchooserprivate.h"
#include "ctkliststore.h"
#include "ctkcellrenderertext.h"
#include "ctkcellrendererpixbuf.h"
#include "ctktreeview.h"
#include "ctktreeselection.h"
#include "ctktreemodelsort.h"
#include "ctkorientable.h"
#include "ctkscrolledwindow.h"
#include "ctklabel.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

/**
 * SECTION:ctkappchooserwidget
 * @Title: CtkAppChooserWidget
 * @Short_description: Application chooser widget that can be embedded in other widgets
 *
 * #CtkAppChooserWidget is a widget for selecting applications.
 * It is the main building block for #CtkAppChooserDialog. Most
 * applications only need to use the latter; but you can use
 * this widget as part of a larger widget if you have special needs.
 *
 * #CtkAppChooserWidget offers detailed control over what applications
 * are shown, using the
 * #CtkAppChooserWidget:show-default,
 * #CtkAppChooserWidget:show-recommended,
 * #CtkAppChooserWidget:show-fallback,
 * #CtkAppChooserWidget:show-other and
 * #CtkAppChooserWidget:show-all
 * properties. See the #CtkAppChooser documentation for more information
 * about these groups of applications.
 *
 * To keep track of the selected application, use the
 * #CtkAppChooserWidget::application-selected and #CtkAppChooserWidget::application-activated signals.
 *
 * # CSS nodes
 *
 * CtkAppChooserWidget has a single CSS node with name appchooser.
 */

struct _CtkAppChooserWidgetPrivate {
  GAppInfo *selected_app_info;

  gchar *content_type;
  gchar *default_text;

  guint show_default     : 1;
  guint show_recommended : 1;
  guint show_fallback    : 1;
  guint show_other       : 1;
  guint show_all         : 1;

  CtkWidget *program_list;
  CtkListStore *program_list_store;
  CtkWidget *no_apps_label;
  CtkWidget *no_apps;

  CtkTreeViewColumn *column;
  CtkCellRenderer *padding_renderer;
  CtkCellRenderer *secondary_padding;

  GAppInfoMonitor *monitor;

  CtkWidget *popup_menu;
};

enum {
  COLUMN_APP_INFO,
  COLUMN_GICON,
  COLUMN_NAME,
  COLUMN_DESC,
  COLUMN_EXEC,
  COLUMN_DEFAULT,
  COLUMN_HEADING,
  COLUMN_HEADING_TEXT,
  COLUMN_RECOMMENDED,
  COLUMN_FALLBACK,
  NUM_COLUMNS
};


enum {
  PROP_CONTENT_TYPE = 1,
  PROP_GFILE,
  PROP_SHOW_DEFAULT,
  PROP_SHOW_RECOMMENDED,
  PROP_SHOW_FALLBACK,
  PROP_SHOW_OTHER,
  PROP_SHOW_ALL,
  PROP_DEFAULT_TEXT,
  N_PROPERTIES
};

enum {
  SIGNAL_APPLICATION_SELECTED,
  SIGNAL_APPLICATION_ACTIVATED,
  SIGNAL_POPULATE_POPUP,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

static void ctk_app_chooser_widget_iface_init (CtkAppChooserIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkAppChooserWidget, ctk_app_chooser_widget, CTK_TYPE_BOX,
                         G_ADD_PRIVATE (CtkAppChooserWidget)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_APP_CHOOSER,
                                                ctk_app_chooser_widget_iface_init));

static void
refresh_and_emit_app_selected (CtkAppChooserWidget *self,
                               CtkTreeSelection    *selection)
{
  CtkTreeModel *model;
  CtkTreeIter iter;
  GAppInfo *info = NULL;
  gboolean should_emit = FALSE;

  if (ctk_tree_selection_get_selected (selection, &model, &iter))
    ctk_tree_model_get (model, &iter, COLUMN_APP_INFO, &info, -1);

  if (info == NULL)
    return;

  if (self->priv->selected_app_info)
    {
      if (!g_app_info_equal (self->priv->selected_app_info, info))
        {
          should_emit = TRUE;
          g_set_object (&self->priv->selected_app_info, info);
        }
    }
  else
    {
      should_emit = TRUE;
      g_set_object (&self->priv->selected_app_info, info);
    }

  g_object_unref (info);

  if (should_emit)
    g_signal_emit (self, signals[SIGNAL_APPLICATION_SELECTED], 0,
                   self->priv->selected_app_info);
}

static GAppInfo *
get_app_info_for_event (CtkAppChooserWidget *self,
                        CdkEventButton      *event)
{
  CtkTreePath *path = NULL;
  CtkTreeIter iter;
  CtkTreeModel *model;
  GAppInfo *info;
  gboolean recommended;

  if (!ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (self->priv->program_list),
                                      event->x, event->y,
                                      &path,
                                      NULL, NULL, NULL))
    return NULL;

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (self->priv->program_list));

  if (!ctk_tree_model_get_iter (model, &iter, path))
    {
      ctk_tree_path_free (path);
      return NULL;
    }

  /* we only allow interaction with recommended applications */
  ctk_tree_model_get (model, &iter,
                      COLUMN_APP_INFO, &info,
                      COLUMN_RECOMMENDED, &recommended,
                      -1);

  if (!recommended)
    g_clear_object (&info);

  return info;
}

static void
popup_menu_detach (CtkWidget *attach_widget,
                   CtkMenu   *menu)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (attach_widget);

  self->priv->popup_menu = NULL;
}

static gboolean
widget_button_press_event_cb (CtkWidget      *widget,
                              CdkEventButton *event,
                              gpointer        user_data)
{
  CtkAppChooserWidget *self = user_data;

  if (event->button == CDK_BUTTON_SECONDARY && event->type == CDK_BUTTON_PRESS)
    {
      GAppInfo *info;
      CtkWidget *menu;
      GList *children;
      gint n_children;

      info = get_app_info_for_event (self, event);

      if (info == NULL)
        return FALSE;

      if (self->priv->popup_menu)
        ctk_widget_destroy (self->priv->popup_menu);

      self->priv->popup_menu = menu = ctk_menu_new ();
      ctk_menu_attach_to_widget (CTK_MENU (menu), CTK_WIDGET (self), popup_menu_detach);

      g_signal_emit (self, signals[SIGNAL_POPULATE_POPUP], 0, menu, info);

      g_object_unref (info);

      /* see if clients added menu items to this container */
      children = ctk_container_get_children (CTK_CONTAINER (menu));
      n_children = g_list_length (children);

      if (n_children > 0)
        /* actually popup the menu */
        ctk_menu_popup_at_pointer (CTK_MENU (menu), (CdkEvent *) event);

      g_list_free (children);
    }

  return FALSE;
}

static gboolean
path_is_heading (CtkTreeView *view,
                 CtkTreePath *path)
{
  CtkTreeIter iter;
  CtkTreeModel *model;
  gboolean res;

  model = ctk_tree_view_get_model (view);
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter,
                      COLUMN_HEADING, &res,
                      -1);

  return res;
}

static void
program_list_selection_activated (CtkTreeView       *view,
                                  CtkTreePath       *path,
                                  CtkTreeViewColumn *column,
                                  gpointer           user_data)
{
  CtkAppChooserWidget *self = user_data;
  CtkTreeSelection *selection;

  if (path_is_heading (view, path))
    return;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (self->priv->program_list));

  refresh_and_emit_app_selected (self, selection);

  g_signal_emit (self, signals[SIGNAL_APPLICATION_ACTIVATED], 0,
                 self->priv->selected_app_info);
}

static gboolean
ctk_app_chooser_search_equal_func (CtkTreeModel *model,
                                   gint          column,
                                   const gchar  *key,
                                   CtkTreeIter  *iter,
                                   gpointer      user_data)
{
  gchar *name;
  gchar *exec_name;
  gboolean ret;

  if (key != NULL)
    {
      ret = TRUE;

      ctk_tree_model_get (model, iter,
                          COLUMN_NAME, &name,
                          COLUMN_EXEC, &exec_name,
                          -1);

      if ((name != NULL && g_str_match_string (key, name, TRUE)) ||
          (exec_name != NULL && g_str_match_string (key, exec_name, FALSE)))
        ret = FALSE;

      g_free (name);
      g_free (exec_name);

      return ret;
    }
  else
    {
      return TRUE;
    }
}

static gint
ctk_app_chooser_sort_func (CtkTreeModel *model,
                           CtkTreeIter  *a,
                           CtkTreeIter  *b,
                           gpointer      user_data)
{
  gboolean a_recommended, b_recommended;
  gboolean a_fallback, b_fallback;
  gboolean a_heading, b_heading;
  gboolean a_default, b_default;
  gchar *a_name, *b_name, *a_casefold, *b_casefold;
  gint retval = 0;

  /* this returns:
   * - <0 if a should show before b
   * - =0 if a is the same as b
   * - >0 if a should show after b
   */

  ctk_tree_model_get (model, a,
                      COLUMN_NAME, &a_name,
                      COLUMN_RECOMMENDED, &a_recommended,
                      COLUMN_FALLBACK, &a_fallback,
                      COLUMN_HEADING, &a_heading,
                      COLUMN_DEFAULT, &a_default,
                      -1);

  ctk_tree_model_get (model, b,
                      COLUMN_NAME, &b_name,
                      COLUMN_RECOMMENDED, &b_recommended,
                      COLUMN_FALLBACK, &b_fallback,
                      COLUMN_HEADING, &b_heading,
                      COLUMN_DEFAULT, &b_default,
                      -1);

  /* the default one always wins */
  if (a_default && !b_default)
    {
      retval = -1;
      goto out;
    }

  if (b_default && !a_default)
    {
      retval = 1;
      goto out;
    }
  
  /* the recommended one always wins */
  if (a_recommended && !b_recommended)
    {
      retval = -1;
      goto out;
    }

  if (b_recommended && !a_recommended)
    {
      retval = 1;
      goto out;
    }

  /* the recommended one always wins */
  if (a_fallback && !b_fallback)
    {
      retval = -1;
      goto out;
    }

  if (b_fallback && !a_fallback)
    {
      retval = 1;
      goto out;
    }

  /* they're both recommended/fallback or not, so if one is a heading, wins */
  if (a_heading)
    {
      retval = -1;
      goto out;
    }

  if (b_heading)
    {
      retval = 1;
      goto out;
    }

  /* don't order by name recommended applications, but use GLib's ordering */
  if (!a_recommended)
    {
      a_casefold = a_name != NULL ?
        g_utf8_casefold (a_name, -1) : NULL;
      b_casefold = b_name != NULL ?
        g_utf8_casefold (b_name, -1) : NULL;

      retval = g_strcmp0 (a_casefold, b_casefold);

      g_free (a_casefold);
      g_free (b_casefold);
    }

 out:
  g_free (a_name);
  g_free (b_name);

  return retval;
}

static void
padding_cell_renderer_func (CtkTreeViewColumn *column,
                            CtkCellRenderer   *cell,
                            CtkTreeModel      *model,
                            CtkTreeIter       *iter,
                            gpointer           user_data)
{
  gboolean heading;

  ctk_tree_model_get (model, iter,
                      COLUMN_HEADING, &heading,
                      -1);
  if (heading)
    g_object_set (cell,
                  "visible", FALSE,
                  "xpad", 0,
                  "ypad", 0,
                  NULL);
  else
    g_object_set (cell,
                  "visible", TRUE,
                  "xpad", 3,
                  "ypad", 3,
                  NULL);
}

static gboolean
ctk_app_chooser_selection_func (CtkTreeSelection *selection,
                                CtkTreeModel     *model,
                                CtkTreePath      *path,
                                gboolean          path_currently_selected,
                                gpointer          user_data)
{
  CtkTreeIter iter;
  gboolean heading;

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter,
                      COLUMN_HEADING, &heading,
                      -1);

  return !heading;
}

static gint
compare_apps_func (gconstpointer a,
                   gconstpointer b)
{
  return !g_app_info_equal (G_APP_INFO (a), G_APP_INFO (b));
}

static gboolean
ctk_app_chooser_widget_add_section (CtkAppChooserWidget *self,
                                    const gchar         *heading_title,
                                    gboolean             show_headings,
                                    gboolean             recommended,
                                    gboolean             fallback,
                                    GList               *applications,
                                    GList               *exclude_apps)
{
  gboolean heading_added, unref_icon;
  CtkTreeIter iter;
  GAppInfo *app;
  gchar *app_string, *bold_string;
  GIcon *icon;
  GList *l;
  gboolean retval;

  retval = FALSE;
  heading_added = FALSE;
  bold_string = g_strdup_printf ("<b>%s</b>", heading_title);
  
  for (l = applications; l != NULL; l = l->next)
    {
      app = l->data;

      if (self->priv->content_type != NULL &&
          !g_app_info_supports_uris (app) &&
          !g_app_info_supports_files (app))
        continue;

      if (g_list_find_custom (exclude_apps, app,
                              (GCompareFunc) compare_apps_func))
        continue;

      if (!heading_added && show_headings)
        {
          ctk_list_store_append (self->priv->program_list_store, &iter);
          ctk_list_store_set (self->priv->program_list_store, &iter,
                              COLUMN_HEADING_TEXT, bold_string,
                              COLUMN_HEADING, TRUE,
                              COLUMN_RECOMMENDED, recommended,
                              COLUMN_FALLBACK, fallback,
                              -1);

          heading_added = TRUE;
        }

      app_string = g_markup_printf_escaped ("%s",
                                            g_app_info_get_name (app) != NULL ?
                                            g_app_info_get_name (app) : "");

      icon = g_app_info_get_icon (app);
      unref_icon = FALSE;
      if (icon == NULL)
        {
          icon = g_themed_icon_new ("application-x-executable");
          unref_icon = TRUE;
        }

      ctk_list_store_append (self->priv->program_list_store, &iter);
      ctk_list_store_set (self->priv->program_list_store, &iter,
                          COLUMN_APP_INFO, app,
                          COLUMN_GICON, icon,
                          COLUMN_NAME, g_app_info_get_name (app),
                          COLUMN_DESC, app_string,
                          COLUMN_EXEC, g_app_info_get_executable (app),
                          COLUMN_HEADING, FALSE,
                          COLUMN_RECOMMENDED, recommended,
                          COLUMN_FALLBACK, fallback,
                          -1);

      retval = TRUE;

      g_free (app_string);
      if (unref_icon)
        g_object_unref (icon);
    }

  g_free (bold_string);

  return retval;
}


static void
ctk_app_chooser_add_default (CtkAppChooserWidget *self,
                             GAppInfo            *app)
{
  CtkTreeIter iter;
  GIcon *icon;
  gchar *string;
  gboolean unref_icon;

  unref_icon = FALSE;
  string = g_strdup_printf ("<b>%s</b>", _("Default Application"));

  ctk_list_store_append (self->priv->program_list_store, &iter);
  ctk_list_store_set (self->priv->program_list_store, &iter,
                      COLUMN_HEADING_TEXT, string,
                      COLUMN_HEADING, TRUE,
                      COLUMN_DEFAULT, TRUE,
                      -1);

  g_free (string);

  string = g_markup_printf_escaped ("%s",
                                    g_app_info_get_name (app) != NULL ?
                                    g_app_info_get_name (app) : "");

  icon = g_app_info_get_icon (app);
  if (icon == NULL)
    {
      icon = g_themed_icon_new ("application-x-executable");
      unref_icon = TRUE;
    }

  ctk_list_store_append (self->priv->program_list_store, &iter);
  ctk_list_store_set (self->priv->program_list_store, &iter,
                      COLUMN_APP_INFO, app,
                      COLUMN_GICON, icon,
                      COLUMN_NAME, g_app_info_get_name (app),
                      COLUMN_DESC, string,
                      COLUMN_EXEC, g_app_info_get_executable (app),
                      COLUMN_HEADING, FALSE,
                      COLUMN_DEFAULT, TRUE,
                      -1);

  g_free (string);

  if (unref_icon)
    g_object_unref (icon);
}

static void
update_no_applications_label (CtkAppChooserWidget *self)
{
  gchar *text = NULL, *desc = NULL;
  const gchar *string;

  if (self->priv->default_text == NULL)
    {
      if (self->priv->content_type)
	desc = g_content_type_get_description (self->priv->content_type);

      string = text = g_strdup_printf (_("No applications found for “%s”."), desc);
      g_free (desc);
    }
  else
    {
      string = self->priv->default_text;
    }

  ctk_label_set_text (CTK_LABEL (self->priv->no_apps_label), string);

  g_free (text);
}

static void
ctk_app_chooser_widget_select_first (CtkAppChooserWidget *self)
{
  CtkTreeIter iter;
  GAppInfo *info = NULL;
  CtkTreeModel *model;

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (self->priv->program_list));
  if (!ctk_tree_model_get_iter_first (model, &iter))
    return;

  while (info == NULL)
    {
      ctk_tree_model_get (model, &iter,
                          COLUMN_APP_INFO, &info,
                          -1);

      if (info != NULL)
        break;

      if (!ctk_tree_model_iter_next (model, &iter))
        break;
    }

  if (info != NULL)
    {
      CtkTreeSelection *selection;

      selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (self->priv->program_list));
      ctk_tree_selection_select_iter (selection, &iter);

      g_object_unref (info);
    }
}

static void
ctk_app_chooser_widget_real_add_items (CtkAppChooserWidget *self)
{
  GList *all_applications = NULL;
  GList *recommended_apps = NULL;
  GList *fallback_apps = NULL;
  GList *exclude_apps = NULL;
  GAppInfo *default_app = NULL;
  gboolean show_headings;
  gboolean apps_added;

  show_headings = TRUE;
  apps_added = FALSE;

  if (self->priv->show_all)
    show_headings = FALSE;

  if (self->priv->show_default && self->priv->content_type)
    {
      default_app = g_app_info_get_default_for_type (self->priv->content_type, FALSE);

      if (default_app != NULL)
        {
          ctk_app_chooser_add_default (self, default_app);
          apps_added = TRUE;
          exclude_apps = g_list_prepend (exclude_apps, default_app);
        }
    }

#ifndef G_OS_WIN32
  if ((self->priv->content_type && self->priv->show_recommended) || self->priv->show_all)
    {
      if (self->priv->content_type)
	recommended_apps = g_app_info_get_recommended_for_type (self->priv->content_type);

      apps_added |= ctk_app_chooser_widget_add_section (self, _("Recommended Applications"),
                                                        show_headings,
                                                        !self->priv->show_all, /* mark as recommended */
                                                        FALSE, /* mark as fallback */
                                                        recommended_apps, exclude_apps);

      exclude_apps = g_list_concat (exclude_apps,
                                    g_list_copy (recommended_apps));
    }

  if ((self->priv->content_type && self->priv->show_fallback) || self->priv->show_all)
    {
      if (self->priv->content_type)
	fallback_apps = g_app_info_get_fallback_for_type (self->priv->content_type);

      apps_added |= ctk_app_chooser_widget_add_section (self, _("Related Applications"),
                                                        show_headings,
                                                        FALSE, /* mark as recommended */
                                                        !self->priv->show_all, /* mark as fallback */
                                                        fallback_apps, exclude_apps);
      exclude_apps = g_list_concat (exclude_apps,
                                    g_list_copy (fallback_apps));
    }
#endif

  if (self->priv->show_other || self->priv->show_all)
    {
      all_applications = g_app_info_get_all ();

      apps_added |= ctk_app_chooser_widget_add_section (self, _("Other Applications"),
                                                        show_headings,
                                                        FALSE,
                                                        FALSE,
                                                        all_applications, exclude_apps);
    }

  if (!apps_added)
    update_no_applications_label (self);

  ctk_widget_set_visible (self->priv->no_apps, !apps_added);

  ctk_app_chooser_widget_select_first (self);

  if (default_app != NULL)
    g_object_unref (default_app);

  g_list_free_full (all_applications, g_object_unref);
  g_list_free_full (recommended_apps, g_object_unref);
  g_list_free_full (fallback_apps, g_object_unref);
  g_list_free (exclude_apps);
}

static void
ctk_app_chooser_widget_initialize_items (CtkAppChooserWidget *self)
{
  /* initial padding */
  g_object_set (self->priv->padding_renderer,
                "xpad", self->priv->show_all ? 0 : 6,
                NULL);

  /* populate the widget */
  ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
}

static void
app_info_changed (GAppInfoMonitor     *monitor,
                  CtkAppChooserWidget *self)
{
  ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
}

static void
ctk_app_chooser_widget_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (object);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      self->priv->content_type = g_value_dup_string (value);
      break;
    case PROP_SHOW_DEFAULT:
      ctk_app_chooser_widget_set_show_default (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_RECOMMENDED:
      ctk_app_chooser_widget_set_show_recommended (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_FALLBACK:
      ctk_app_chooser_widget_set_show_fallback (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_OTHER:
      ctk_app_chooser_widget_set_show_other (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_ALL:
      ctk_app_chooser_widget_set_show_all (self, g_value_get_boolean (value));
      break;
    case PROP_DEFAULT_TEXT:
      ctk_app_chooser_widget_set_default_text (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_app_chooser_widget_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (object);

  switch (property_id)
    {
    case PROP_CONTENT_TYPE:
      g_value_set_string (value, self->priv->content_type);
      break;
    case PROP_SHOW_DEFAULT:
      g_value_set_boolean (value, self->priv->show_default);
      break;
    case PROP_SHOW_RECOMMENDED:
      g_value_set_boolean (value, self->priv->show_recommended);
      break;
    case PROP_SHOW_FALLBACK:
      g_value_set_boolean (value, self->priv->show_fallback);
      break;
    case PROP_SHOW_OTHER:
      g_value_set_boolean (value, self->priv->show_other);
      break;
    case PROP_SHOW_ALL:
      g_value_set_boolean (value, self->priv->show_all);
      break;
    case PROP_DEFAULT_TEXT:
      g_value_set_string (value, self->priv->default_text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_app_chooser_widget_constructed (GObject *object)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (object);

  if (G_OBJECT_CLASS (ctk_app_chooser_widget_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (ctk_app_chooser_widget_parent_class)->constructed (object);

  ctk_app_chooser_widget_initialize_items (self);
}

static void
ctk_app_chooser_widget_finalize (GObject *object)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (object);

  g_free (self->priv->content_type);
  g_free (self->priv->default_text);
  g_signal_handlers_disconnect_by_func (self->priv->monitor, app_info_changed, self);
  g_object_unref (self->priv->monitor);

  G_OBJECT_CLASS (ctk_app_chooser_widget_parent_class)->finalize (object);
}

static void
ctk_app_chooser_widget_dispose (GObject *object)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (object);

  g_clear_object (&self->priv->selected_app_info);

  G_OBJECT_CLASS (ctk_app_chooser_widget_parent_class)->dispose (object);
}

static void
ctk_app_chooser_widget_class_init (CtkAppChooserWidgetClass *klass)
{
  CtkWidgetClass *widget_class;
  GObjectClass *gobject_class;
  GParamSpec *pspec;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = ctk_app_chooser_widget_dispose;
  gobject_class->finalize = ctk_app_chooser_widget_finalize;
  gobject_class->set_property = ctk_app_chooser_widget_set_property;
  gobject_class->get_property = ctk_app_chooser_widget_get_property;
  gobject_class->constructed = ctk_app_chooser_widget_constructed;

  g_object_class_override_property (gobject_class, PROP_CONTENT_TYPE, "content-type");

  /**
   * CtkAppChooserWidget:show-default:
   *
   * The ::show-default property determines whether the app chooser
   * should show the default handler for the content type in a
   * separate section. If %FALSE, the default handler is listed
   * among the recommended applications.
   */
  pspec = g_param_spec_boolean ("show-default",
                                P_("Show default app"),
                                P_("Whether the widget should show the default application"),
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_DEFAULT, pspec);

  /**
   * CtkAppChooserWidget:show-recommended:
   *
   * The #CtkAppChooserWidget:show-recommended property determines
   * whether the app chooser should show a section for recommended
   * applications. If %FALSE, the recommended applications are listed
   * among the other applications.
   */
  pspec = g_param_spec_boolean ("show-recommended",
                                P_("Show recommended apps"),
                                P_("Whether the widget should show recommended applications"),
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_RECOMMENDED, pspec);

  /**
   * CtkAppChooserWidget:show-fallback:
   *
   * The #CtkAppChooserWidget:show-fallback property determines whether
   * the app chooser should show a section for fallback applications.
   * If %FALSE, the fallback applications are listed among the other
   * applications.
   */
  pspec = g_param_spec_boolean ("show-fallback",
                                P_("Show fallback apps"),
                                P_("Whether the widget should show fallback applications"),
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_FALLBACK, pspec);

  /**
   * CtkAppChooserWidget:show-other:
   *
   * The #CtkAppChooserWidget:show-other property determines whether
   * the app chooser should show a section for other applications.
   */
  pspec = g_param_spec_boolean ("show-other",
                                P_("Show other apps"),
                                P_("Whether the widget should show other applications"),
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_OTHER, pspec);

  /**
   * CtkAppChooserWidget:show-all:
   *
   * If the #CtkAppChooserWidget:show-all property is %TRUE, the app
   * chooser presents all applications in a single list, without
   * subsections for default, recommended or related applications.
   */
  pspec = g_param_spec_boolean ("show-all",
                                P_("Show all apps"),
                                P_("Whether the widget should show all applications"),
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_SHOW_ALL, pspec);

  /**
   * CtkAppChooserWidget:default-text:
   *
   * The #CtkAppChooserWidget:default-text property determines the text
   * that appears in the widget when there are no applications for the
   * given content type.
   * See also ctk_app_chooser_widget_set_default_text().
   */
  pspec = g_param_spec_string ("default-text",
                               P_("Widget's default text"),
                               P_("The default text appearing when there are no applications"),
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (gobject_class, PROP_DEFAULT_TEXT, pspec);

  /**
   * CtkAppChooserWidget::application-selected:
   * @self: the object which received the signal
   * @application: the selected #GAppInfo
   *
   * Emitted when an application item is selected from the widget's list.
   */
  signals[SIGNAL_APPLICATION_SELECTED] =
    g_signal_new (I_("application-selected"),
                  CTK_TYPE_APP_CHOOSER_WIDGET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkAppChooserWidgetClass, application_selected),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_APP_INFO);

  /**
   * CtkAppChooserWidget::application-activated:
   * @self: the object which received the signal
   * @application: the activated #GAppInfo
   *
   * Emitted when an application item is activated from the widget's list.
   *
   * This usually happens when the user double clicks an item, or an item
   * is selected and the user presses one of the keys Space, Shift+Space,
   * Return or Enter.
   */
  signals[SIGNAL_APPLICATION_ACTIVATED] =
    g_signal_new (I_("application-activated"),
                  CTK_TYPE_APP_CHOOSER_WIDGET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkAppChooserWidgetClass, application_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_APP_INFO);

  /**
   * CtkAppChooserWidget::populate-popup:
   * @self: the object which received the signal
   * @menu: the #CtkMenu to populate
   * @application: the current #GAppInfo
   *
   * Emitted when a context menu is about to popup over an application item.
   * Clients can insert menu items into the provided #CtkMenu object in the
   * callback of this signal; the context menu will be shown over the item
   * if at least one item has been added to the menu.
   */
  signals[SIGNAL_POPULATE_POPUP] =
    g_signal_new (I_("populate-popup"),
                  CTK_TYPE_APP_CHOOSER_WIDGET,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkAppChooserWidgetClass, populate_popup),
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_OBJECT,
                  G_TYPE_NONE,
                  2, CTK_TYPE_MENU, G_TYPE_APP_INFO);

  /* Bind class to template
   */
  widget_class = CTK_WIDGET_CLASS (klass);
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkappchooserwidget.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserWidget, program_list);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserWidget, program_list_store);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserWidget, column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserWidget, padding_renderer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserWidget, secondary_padding);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserWidget, no_apps_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkAppChooserWidget, no_apps);
  ctk_widget_class_bind_template_callback (widget_class, refresh_and_emit_app_selected);
  ctk_widget_class_bind_template_callback (widget_class, program_list_selection_activated);
  ctk_widget_class_bind_template_callback (widget_class, widget_button_press_event_cb);

  ctk_widget_class_set_css_name (widget_class, "appchooser");
}

static void
ctk_app_chooser_widget_init (CtkAppChooserWidget *self)
{
  CtkTreeSelection *selection;
  CtkTreeModel *sort;

  self->priv = ctk_app_chooser_widget_get_instance_private (self);

  ctk_widget_init_template (CTK_WIDGET (self));

  /* Various parts of the CtkTreeView code need custom code to setup, mostly
   * because we lack signals to connect to, or properties to set.
   */
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (self->priv->program_list));
  ctk_tree_selection_set_select_function (selection, ctk_app_chooser_selection_func,
                                          self, NULL);

  sort = ctk_tree_view_get_model (CTK_TREE_VIEW (self->priv->program_list));
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (sort),
                                        COLUMN_NAME,
                                        CTK_SORT_ASCENDING);
  ctk_tree_sortable_set_sort_func (CTK_TREE_SORTABLE (sort),
                                   COLUMN_NAME,
                                   ctk_app_chooser_sort_func,
                                   self, NULL);

  ctk_tree_view_set_search_column (CTK_TREE_VIEW (self->priv->program_list), COLUMN_NAME);
  ctk_tree_view_set_search_equal_func (CTK_TREE_VIEW (self->priv->program_list),
                                       ctk_app_chooser_search_equal_func,
                                       NULL, NULL);

  ctk_tree_view_column_set_cell_data_func (self->priv->column,
					   self->priv->secondary_padding,
                                           padding_cell_renderer_func,
                                           NULL, NULL);

  self->priv->monitor = g_app_info_monitor_get ();
  g_signal_connect (self->priv->monitor, "changed",
		    G_CALLBACK (app_info_changed), self);
}

static GAppInfo *
ctk_app_chooser_widget_get_app_info (CtkAppChooser *object)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (object);

  if (self->priv->selected_app_info == NULL)
    return NULL;

  return g_object_ref (self->priv->selected_app_info);
}

static void
ctk_app_chooser_widget_refresh (CtkAppChooser *object)
{
  CtkAppChooserWidget *self = CTK_APP_CHOOSER_WIDGET (object);

  if (self->priv->program_list_store != NULL)
    {
      ctk_list_store_clear (self->priv->program_list_store);

      /* don't add additional xpad if we don't have headings */
      g_object_set (self->priv->padding_renderer,
                    "visible", !self->priv->show_all,
                    NULL);

      ctk_app_chooser_widget_real_add_items (self);
    }
}

static void
ctk_app_chooser_widget_iface_init (CtkAppChooserIface *iface)
{
  iface->get_app_info = ctk_app_chooser_widget_get_app_info;
  iface->refresh = ctk_app_chooser_widget_refresh;
}

/**
 * ctk_app_chooser_widget_new:
 * @content_type: the content type to show applications for
 *
 * Creates a new #CtkAppChooserWidget for applications
 * that can handle content of the given type.
 *
 * Returns: a newly created #CtkAppChooserWidget
 *
 * Since: 3.0
 */
CtkWidget *
ctk_app_chooser_widget_new (const gchar *content_type)
{
  return g_object_new (CTK_TYPE_APP_CHOOSER_WIDGET,
                       "content-type", content_type,
                       NULL);
}

/**
 * ctk_app_chooser_widget_set_show_default:
 * @self: a #CtkAppChooserWidget
 * @setting: the new value for #CtkAppChooserWidget:show-default
 *
 * Sets whether the app chooser should show the default handler
 * for the content type in a separate section.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_widget_set_show_default (CtkAppChooserWidget *self,
                                         gboolean             setting)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self));

  if (self->priv->show_default != setting)
    {
      self->priv->show_default = setting;

      g_object_notify (G_OBJECT (self), "show-default");

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_widget_get_show_default:
 * @self: a #CtkAppChooserWidget
 *
 * Returns the current value of the #CtkAppChooserWidget:show-default
 * property.
 *
 * Returns: the value of #CtkAppChooserWidget:show-default
 *
 * Since: 3.0
 */
gboolean
ctk_app_chooser_widget_get_show_default (CtkAppChooserWidget *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->priv->show_default;
}

/**
 * ctk_app_chooser_widget_set_show_recommended:
 * @self: a #CtkAppChooserWidget
 * @setting: the new value for #CtkAppChooserWidget:show-recommended
 *
 * Sets whether the app chooser should show recommended applications
 * for the content type in a separate section.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_widget_set_show_recommended (CtkAppChooserWidget *self,
                                             gboolean             setting)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self));

  if (self->priv->show_recommended != setting)
    {
      self->priv->show_recommended = setting;

      g_object_notify (G_OBJECT (self), "show-recommended");

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_widget_get_show_recommended:
 * @self: a #CtkAppChooserWidget
 *
 * Returns the current value of the #CtkAppChooserWidget:show-recommended
 * property.
 *
 * Returns: the value of #CtkAppChooserWidget:show-recommended
 *
 * Since: 3.0
 */
gboolean
ctk_app_chooser_widget_get_show_recommended (CtkAppChooserWidget *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->priv->show_recommended;
}

/**
 * ctk_app_chooser_widget_set_show_fallback:
 * @self: a #CtkAppChooserWidget
 * @setting: the new value for #CtkAppChooserWidget:show-fallback
 *
 * Sets whether the app chooser should show related applications
 * for the content type in a separate section.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_widget_set_show_fallback (CtkAppChooserWidget *self,
                                          gboolean             setting)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self));

  if (self->priv->show_fallback != setting)
    {
      self->priv->show_fallback = setting;

      g_object_notify (G_OBJECT (self), "show-fallback");

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_widget_get_show_fallback:
 * @self: a #CtkAppChooserWidget
 *
 * Returns the current value of the #CtkAppChooserWidget:show-fallback
 * property.
 *
 * Returns: the value of #CtkAppChooserWidget:show-fallback
 *
 * Since: 3.0
 */
gboolean
ctk_app_chooser_widget_get_show_fallback (CtkAppChooserWidget *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->priv->show_fallback;
}

/**
 * ctk_app_chooser_widget_set_show_other:
 * @self: a #CtkAppChooserWidget
 * @setting: the new value for #CtkAppChooserWidget:show-other
 *
 * Sets whether the app chooser should show applications
 * which are unrelated to the content type.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_widget_set_show_other (CtkAppChooserWidget *self,
                                       gboolean             setting)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self));

  if (self->priv->show_other != setting)
    {
      self->priv->show_other = setting;

      g_object_notify (G_OBJECT (self), "show-other");

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_widget_get_show_other:
 * @self: a #CtkAppChooserWidget
 *
 * Returns the current value of the #CtkAppChooserWidget:show-other
 * property.
 *
 * Returns: the value of #CtkAppChooserWidget:show-other
 *
 * Since: 3.0
 */
gboolean
ctk_app_chooser_widget_get_show_other (CtkAppChooserWidget *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->priv->show_other;
}

/**
 * ctk_app_chooser_widget_set_show_all:
 * @self: a #CtkAppChooserWidget
 * @setting: the new value for #CtkAppChooserWidget:show-all
 *
 * Sets whether the app chooser should show all applications
 * in a flat list.
 *
 * Since: 3.0
 */
void
ctk_app_chooser_widget_set_show_all (CtkAppChooserWidget *self,
                                     gboolean             setting)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self));

  if (self->priv->show_all != setting)
    {
      self->priv->show_all = setting;

      g_object_notify (G_OBJECT (self), "show-all");

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_widget_get_show_all:
 * @self: a #CtkAppChooserWidget
 *
 * Returns the current value of the #CtkAppChooserWidget:show-all
 * property.
 *
 * Returns: the value of #CtkAppChooserWidget:show-all
 *
 * Since: 3.0
 */
gboolean
ctk_app_chooser_widget_get_show_all (CtkAppChooserWidget *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self), FALSE);

  return self->priv->show_all;
}

/**
 * ctk_app_chooser_widget_set_default_text:
 * @self: a #CtkAppChooserWidget
 * @text: the new value for #CtkAppChooserWidget:default-text
 *
 * Sets the text that is shown if there are not applications
 * that can handle the content type.
 */
void
ctk_app_chooser_widget_set_default_text (CtkAppChooserWidget *self,
                                         const gchar         *text)
{
  g_return_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self));

  if (g_strcmp0 (text, self->priv->default_text) != 0)
    {
      g_free (self->priv->default_text);
      self->priv->default_text = g_strdup (text);

      g_object_notify (G_OBJECT (self), "default-text");

      ctk_app_chooser_refresh (CTK_APP_CHOOSER (self));
    }
}

/**
 * ctk_app_chooser_widget_get_default_text:
 * @self: a #CtkAppChooserWidget
 *
 * Returns the text that is shown if there are not applications
 * that can handle the content type.
 *
 * Returns: the value of #CtkAppChooserWidget:default-text
 *
 * Since: 3.0
 */
const gchar *
ctk_app_chooser_widget_get_default_text (CtkAppChooserWidget *self)
{
  g_return_val_if_fail (CTK_IS_APP_CHOOSER_WIDGET (self), NULL);

  return self->priv->default_text;
}

void
_ctk_app_chooser_widget_set_search_entry (CtkAppChooserWidget *self,
                                          CtkEntry            *entry)
{
  ctk_tree_view_set_search_entry (CTK_TREE_VIEW (self->priv->program_list), entry);

  g_object_bind_property (self->priv->no_apps, "visible",
                          entry, "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
}
