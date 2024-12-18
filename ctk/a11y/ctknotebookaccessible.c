/* CTK+ - accessibility implementations
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

#include "config.h"

#include <string.h>
#include <ctk/ctk.h>
#include "ctknotebookaccessible.h"
#include "ctknotebookpageaccessible.h"

struct _CtkNotebookAccessiblePrivate
{
  /*
   * page_cache maintains a list of pre-ref'd Notebook Pages.
   * This cache is queried by ctk_notebook_accessible_ref_child().
   * If the page is found in the list then a new page does not
   * need to be created
   */
  GHashTable * pages;
  gint         selected_page;
};

static void atk_selection_interface_init (AtkSelectionIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkNotebookAccessible, ctk_notebook_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkNotebookAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION, atk_selection_interface_init))

static void
create_notebook_page_accessible (CtkNotebookAccessible *accessible,
                                 CtkNotebook           *notebook G_GNUC_UNUSED,
                                 CtkWidget             *child,
                                 gint                   page_num)
{
  AtkObject *obj;

  obj = ctk_notebook_page_accessible_new (accessible, child);
  g_hash_table_insert (accessible->priv->pages, child, obj);
  atk_object_set_parent (obj, ATK_OBJECT (accessible));
  g_signal_emit_by_name (accessible, "children-changed::add", page_num, obj, NULL);
}

static void
page_added_cb (CtkNotebook *notebook,
               CtkWidget   *child,
               guint        page_num,
               gpointer     data G_GNUC_UNUSED)
{
  AtkObject *atk_obj;
  CtkNotebookAccessible *accessible;

  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (notebook));
  accessible = CTK_NOTEBOOK_ACCESSIBLE (atk_obj);
  create_notebook_page_accessible (accessible, notebook, child, page_num);
}

static void
page_removed_cb (CtkNotebook *notebook,
                 CtkWidget   *widget,
                 guint        page_num,
                 gpointer     data G_GNUC_UNUSED)
{
  CtkNotebookAccessible *accessible;
  AtkObject *obj;

  accessible = CTK_NOTEBOOK_ACCESSIBLE (ctk_widget_get_accessible (CTK_WIDGET (notebook)));

  obj = g_hash_table_lookup (accessible->priv->pages, widget);
  g_return_if_fail (obj);
  g_signal_emit_by_name (accessible, "children-changed::remove",
                         page_num, obj, NULL);
  ctk_notebook_page_accessible_invalidate (CTK_NOTEBOOK_PAGE_ACCESSIBLE (obj));
  g_hash_table_remove (accessible->priv->pages, widget);
}


static void
ctk_notebook_accessible_initialize (AtkObject *obj,
                                    gpointer   data)
{
  CtkNotebookAccessible *accessible;
  CtkNotebook *notebook;
  gint i;

  ATK_OBJECT_CLASS (ctk_notebook_accessible_parent_class)->initialize (obj, data);

  accessible = CTK_NOTEBOOK_ACCESSIBLE (obj);
  notebook = CTK_NOTEBOOK (data);
  for (i = 0; i < ctk_notebook_get_n_pages (notebook); i++)
    {
      create_notebook_page_accessible (accessible,
                                       notebook,
                                       ctk_notebook_get_nth_page (notebook, i),
                                       i);
    }
  accessible->priv->selected_page = ctk_notebook_get_current_page (notebook);

  g_signal_connect (notebook, "page-added",
                    G_CALLBACK (page_added_cb), NULL);
  g_signal_connect (notebook, "page-removed",
                    G_CALLBACK (page_removed_cb), NULL);

  obj->role = ATK_ROLE_PAGE_TAB_LIST;
}

static void
ctk_notebook_accessible_finalize (GObject *object)
{
  CtkNotebookAccessible *accessible = CTK_NOTEBOOK_ACCESSIBLE (object);

  g_hash_table_destroy (accessible->priv->pages);

  G_OBJECT_CLASS (ctk_notebook_accessible_parent_class)->finalize (object);
}

static AtkObject *
ctk_notebook_accessible_ref_child (AtkObject *obj,
                                   gint       i)
{
  AtkObject *child;
  CtkNotebookAccessible *accessible;
  CtkNotebook *notebook;
  CtkWidget *widget;
 
  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  accessible = CTK_NOTEBOOK_ACCESSIBLE (obj);
  notebook = CTK_NOTEBOOK (widget);

  child = g_hash_table_lookup (accessible->priv->pages,
                               ctk_notebook_get_nth_page (notebook, i));
  /* can return NULL when i >= n_children */

  if (child)
    g_object_ref (child);

  return child;
}

static void
ctk_notebook_accessible_notify_ctk (GObject    *obj,
                                    GParamSpec *pspec)
{
  CtkWidget *widget;
  AtkObject* atk_obj;

  widget = CTK_WIDGET (obj);
  atk_obj = ctk_widget_get_accessible (widget);

  if (strcmp (pspec->name, "page") == 0)
    {
      gint page_num, old_page_num;
      CtkNotebookAccessible *accessible;
      CtkNotebook *notebook;

      accessible = CTK_NOTEBOOK_ACCESSIBLE (atk_obj);
      notebook = CTK_NOTEBOOK (widget);

      /* Notify SELECTED state change for old and new page */
      old_page_num = accessible->priv->selected_page;
      page_num = ctk_notebook_get_current_page (notebook);
      accessible->priv->selected_page = page_num;

      if (page_num != old_page_num)
        {
          AtkObject *child;

          if (old_page_num != -1)
            {
              child = ctk_notebook_accessible_ref_child (atk_obj, old_page_num);
              if (child)
                {
                  atk_object_notify_state_change (child, ATK_STATE_SELECTED, FALSE);
                  g_object_unref (child);
                }
            }
          child = ctk_notebook_accessible_ref_child (atk_obj, page_num);
          if (child)
            {
              atk_object_notify_state_change (child, ATK_STATE_SELECTED, TRUE);
              g_object_unref (child);
            }
          g_signal_emit_by_name (atk_obj, "selection-changed");
          g_signal_emit_by_name (atk_obj, "visible-data-changed");
        }
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_notebook_accessible_parent_class)->notify_ctk (obj, pspec);
}

/*
 * CtkNotebook only supports the selection of one page at a time.
 * Selecting a page unselects any previous selection, so this
 * changes the current selection instead of adding to it.
 */
static gboolean
ctk_notebook_accessible_add_selection (AtkSelection *selection,
                                       gint          i)
{
  CtkNotebook *notebook;
  CtkWidget *widget;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  notebook = CTK_NOTEBOOK (widget);
  ctk_notebook_set_current_page (notebook, i);
  return TRUE;
}

static void
ctk_notebook_accessible_class_init (CtkNotebookAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass  *class = ATK_OBJECT_CLASS (klass);
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;
  CtkContainerAccessibleClass *container_class = (CtkContainerAccessibleClass*)klass;

  gobject_class->finalize = ctk_notebook_accessible_finalize;

  class->ref_child = ctk_notebook_accessible_ref_child;
  class->initialize = ctk_notebook_accessible_initialize;

  widget_class->notify_ctk = ctk_notebook_accessible_notify_ctk;

  /* we listen to page-added/-removed, so we don't care about these */
  container_class->add_ctk = NULL;
  container_class->remove_ctk = NULL;
}

static void
ctk_notebook_accessible_init (CtkNotebookAccessible *notebook)
{
  notebook->priv = ctk_notebook_accessible_get_instance_private (notebook);
  notebook->priv->pages = g_hash_table_new_full (g_direct_hash,
                                                 g_direct_equal,
                                                 NULL,
                                                 g_object_unref);
  notebook->priv->selected_page = -1;
}

static AtkObject *
ctk_notebook_accessible_ref_selection (AtkSelection *selection,
                                       gint          i)
{
  AtkObject *accessible;
  CtkWidget *widget;
  CtkNotebook *notebook;
  gint pagenum;

  if (i != 0)
    return NULL;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return NULL;

  notebook = CTK_NOTEBOOK (widget);
  pagenum = ctk_notebook_get_current_page (notebook);
  if (pagenum == -1)
    return NULL;
  accessible = ctk_notebook_accessible_ref_child (ATK_OBJECT (selection), pagenum);

  return accessible;
}

/* Always return 1 because there can only be one page
 * selected at any time
 */
static gint
ctk_notebook_accessible_get_selection_count (AtkSelection *selection)
{
  CtkWidget *widget;
  CtkNotebook *notebook;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return 0;

  notebook = CTK_NOTEBOOK (widget);
  if (notebook == NULL || ctk_notebook_get_current_page (notebook) == -1)
    return 0;

  return 1;
}

static gboolean
ctk_notebook_accessible_is_child_selected (AtkSelection *selection,
                                           gint          i)
{
  CtkWidget *widget;
  CtkNotebook *notebook;
  gint pagenumber;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  notebook = CTK_NOTEBOOK (widget);
  pagenumber = ctk_notebook_get_current_page(notebook);

  if (pagenumber == i)
    return TRUE;

  return FALSE;
}

static void
atk_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = ctk_notebook_accessible_add_selection;
  iface->ref_selection = ctk_notebook_accessible_ref_selection;
  iface->get_selection_count = ctk_notebook_accessible_get_selection_count;
  iface->is_child_selected = ctk_notebook_accessible_is_child_selected;
}
