/* ctktreemenu.c
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *
 * Based on some GtkComboBox menu code by Kristian Rietveld <kris@ctk.org>
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

/*
 * SECTION:ctktreemenu
 * @Short_Description: A GtkMenu automatically created from a #GtkTreeModel
 * @Title: GtkTreeMenu
 *
 * The #GtkTreeMenu is used to display a drop-down menu allowing selection
 * of every row in the model and is used by the #GtkComboBox for its drop-down
 * menu.
 */

#include "config.h"
#include "ctkintl.h"
#include "ctktreemenu.h"
#include "ctkmarshalers.h"
#include "ctkmenuitem.h"
#include "ctkseparatormenuitem.h"
#include "ctkcellareabox.h"
#include "ctkcellareacontext.h"
#include "ctkcelllayout.h"
#include "ctkcellview.h"
#include "ctkmenushellprivate.h"
#include "ctkprivate.h"

#undef GDK_DEPRECATED
#undef GDK_DEPRECATED_FOR
#define GDK_DEPRECATED
#define GDK_DEPRECATED_FOR(f)

#include "deprecated/ctktearoffmenuitem.h"

/* GObjectClass */
static void      ctk_tree_menu_constructed                    (GObject            *object);
static void      ctk_tree_menu_dispose                        (GObject            *object);
static void      ctk_tree_menu_finalize                       (GObject            *object);
static void      ctk_tree_menu_set_property                   (GObject            *object,
                                                               guint               prop_id,
                                                               const GValue       *value,
                                                               GParamSpec         *pspec);
static void      ctk_tree_menu_get_property                   (GObject            *object,
                                                               guint               prop_id,
                                                               GValue             *value,
                                                               GParamSpec         *pspec);

/* GtkWidgetClass */
static void      ctk_tree_menu_get_preferred_width            (GtkWidget           *widget,
                                                               gint                *minimum_size,
                                                               gint                *natural_size);
static void      ctk_tree_menu_get_preferred_height           (GtkWidget           *widget,
                                                               gint                *minimum_size,
                                                               gint                *natural_size);
static void      ctk_tree_menu_get_preferred_width_for_height (GtkWidget           *widget,
                                                               gint                 for_height,
                                                               gint                *minimum_size,
                                                               gint                *natural_size);
static void      ctk_tree_menu_get_preferred_height_for_width (GtkWidget           *widget,
                                                               gint                 for_width,
                                                               gint                *minimum_size,
                                                               gint                *natural_size);

/* GtkCellLayoutIface */
static void      ctk_tree_menu_cell_layout_init               (GtkCellLayoutIface  *iface);
static GtkCellArea *ctk_tree_menu_cell_layout_get_area        (GtkCellLayout        *layout);


/* TreeModel/DrawingArea callbacks and building menus/submenus */
static inline void rebuild_menu                               (GtkTreeMenu          *menu);
static gboolean   menu_occupied                               (GtkTreeMenu          *menu,
                                                               guint                 left_attach,
                                                               guint                 right_attach,
                                                               guint                 top_attach,
                                                               guint                 bottom_attach);
static void       relayout_item                               (GtkTreeMenu          *menu,
                                                               GtkWidget            *item,
                                                               GtkTreeIter          *iter,
                                                               GtkWidget            *prev);
static void       ctk_tree_menu_populate                      (GtkTreeMenu          *menu);
static GtkWidget *ctk_tree_menu_create_item                   (GtkTreeMenu          *menu,
                                                               GtkTreeIter          *iter,
                                                               gboolean              header_item);
static void       ctk_tree_menu_create_submenu                (GtkTreeMenu          *menu,
                                                               GtkWidget            *item,
                                                               GtkTreePath          *path);
static void       ctk_tree_menu_set_area                      (GtkTreeMenu          *menu,
                                                               GtkCellArea          *area);
static GtkWidget *ctk_tree_menu_get_path_item                 (GtkTreeMenu          *menu,
                                                               GtkTreePath          *path);
static gboolean   ctk_tree_menu_path_in_menu                  (GtkTreeMenu          *menu,
                                                               GtkTreePath          *path,
                                                               gboolean             *header_item);
static void       row_inserted_cb                             (GtkTreeModel         *model,
                                                               GtkTreePath          *path,
                                                               GtkTreeIter          *iter,
                                                               GtkTreeMenu          *menu);
static void       row_deleted_cb                              (GtkTreeModel         *model,
                                                               GtkTreePath          *path,
                                                               GtkTreeMenu          *menu);
static void       row_reordered_cb                            (GtkTreeModel         *model,
                                                               GtkTreePath          *path,
                                                               GtkTreeIter          *iter,
                                                               gint                 *new_order,
                                                               GtkTreeMenu          *menu);
static void       row_changed_cb                              (GtkTreeModel         *model,
                                                               GtkTreePath          *path,
                                                               GtkTreeIter          *iter,
                                                               GtkTreeMenu          *menu);
static void       context_size_changed_cb                     (GtkCellAreaContext   *context,
                                                               GParamSpec           *pspec,
                                                               GtkWidget            *menu);
static void       area_apply_attributes_cb                    (GtkCellArea          *area,
                                                               GtkTreeModel         *tree_model,
                                                               GtkTreeIter          *iter,
                                                               gboolean              is_expander,
                                                               gboolean              is_expanded,
                                                               GtkTreeMenu          *menu);
static void       item_activated_cb                           (GtkMenuItem          *item,
                                                               GtkTreeMenu          *menu);
static void       submenu_activated_cb                        (GtkTreeMenu          *submenu,
                                                               const gchar          *path,
                                                               GtkTreeMenu          *menu);
static void       ctk_tree_menu_set_model_internal            (GtkTreeMenu          *menu,
                                                               GtkTreeModel         *model);



struct _GtkTreeMenuPrivate
{
  /* TreeModel and parent for this menu */
  GtkTreeModel        *model;
  GtkTreeRowReference *root;

  /* CellArea and context for this menu */
  GtkCellArea         *area;
  GtkCellAreaContext  *context;

  /* Signals */
  gulong               size_changed_id;
  gulong               apply_attributes_id;
  gulong               row_inserted_id;
  gulong               row_deleted_id;
  gulong               row_reordered_id;
  gulong               row_changed_id;

  /* Grid menu mode */
  gint                 wrap_width;
  gint                 row_span_col;
  gint                 col_span_col;

  /* Flags */
  guint32              menu_with_header : 1;
  guint32              tearoff     : 1;

  /* Row separators */
  GtkTreeViewRowSeparatorFunc row_separator_func;
  gpointer                    row_separator_data;
  GDestroyNotify              row_separator_destroy;
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_ROOT,
  PROP_CELL_AREA,
  PROP_TEAROFF,
  PROP_WRAP_WIDTH,
  PROP_ROW_SPAN_COL,
  PROP_COL_SPAN_COL
};

enum {
  SIGNAL_MENU_ACTIVATE,
  N_SIGNALS
};

static guint   tree_menu_signals[N_SIGNALS] = { 0 };
static GQuark  tree_menu_path_quark = 0;

G_DEFINE_TYPE_WITH_CODE (GtkTreeMenu, _ctk_tree_menu, CTK_TYPE_MENU,
                         G_ADD_PRIVATE (GtkTreeMenu)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_LAYOUT,
                                                ctk_tree_menu_cell_layout_init));

static void
_ctk_tree_menu_init (GtkTreeMenu *menu)
{
  menu->priv = _ctk_tree_menu_get_instance_private (menu);
  menu->priv->row_span_col = -1;
  menu->priv->col_span_col = -1;

  ctk_menu_set_reserve_toggle_size (CTK_MENU (menu), FALSE);
}

static void
_ctk_tree_menu_class_init (GtkTreeMenuClass *class)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  tree_menu_path_quark = g_quark_from_static_string ("ctk-tree-menu-path");

  object_class->constructed  = ctk_tree_menu_constructed;
  object_class->dispose      = ctk_tree_menu_dispose;
  object_class->finalize     = ctk_tree_menu_finalize;
  object_class->set_property = ctk_tree_menu_set_property;
  object_class->get_property = ctk_tree_menu_get_property;

  widget_class->get_preferred_width  = ctk_tree_menu_get_preferred_width;
  widget_class->get_preferred_height = ctk_tree_menu_get_preferred_height;
  widget_class->get_preferred_width_for_height  = ctk_tree_menu_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_tree_menu_get_preferred_height_for_width;

  /*
   * GtkTreeMenu::menu-activate:
   * @menu: a #GtkTreeMenu
   * @path: the #GtkTreePath string for the item which was activated
   * @user_data: the user data
   *
   * This signal is emitted to notify that a menu item in the #GtkTreeMenu
   * was activated and provides the path string from the #GtkTreeModel
   * to specify which row was selected.
   *
   * Since: 3.0
   */
  tree_menu_signals[SIGNAL_MENU_ACTIVATE] =
    g_signal_new (I_("menu-activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  0, /* No class closure here */
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  /*
   * GtkTreeMenu:model:
   *
   * The #GtkTreeModel from which the menu is constructed.
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        P_("TreeMenu model"),
                                                        P_("The model for the tree menu"),
                                                        CTK_TYPE_TREE_MODEL,
                                                        CTK_PARAM_READWRITE));

  /*
   * GtkTreeMenu:root:
   *
   * The #GtkTreePath that is the root for this menu, or %NULL.
   *
   * The #GtkTreeMenu recursively creates submenus for #GtkTreeModel
   * rows that have children and the "root" for each menu is provided
   * by the parent menu.
   *
   * If you dont provide a root for the #GtkTreeMenu then the whole
   * model will be added to the menu. Specifying a root allows you
   * to build a menu for a given #GtkTreePath and its children.
   * 
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_ROOT,
                                   g_param_spec_boxed ("root",
                                                       P_("TreeMenu root row"),
                                                       P_("The TreeMenu will display children of the "
                                                          "specified root"),
                                                       CTK_TYPE_TREE_PATH,
                                                       CTK_PARAM_READWRITE));

  /*
   * GtkTreeMenu:cell-area:
   *
   * The #GtkCellArea used to render cells in the menu items.
   *
   * You can provide a different cell area at object construction
   * time, otherwise the #GtkTreeMenu will use a #GtkCellAreaBox.
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_CELL_AREA,
                                   g_param_spec_object ("cell-area",
                                                        P_("Cell Area"),
                                                        P_("The GtkCellArea used to layout cells"),
                                                        CTK_TYPE_CELL_AREA,
                                                        CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /*
   * GtkTreeMenu:tearoff:
   *
   * Specifies whether this menu comes with a leading tearoff menu item
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_TEAROFF,
                                   g_param_spec_boolean ("tearoff",
                                                         P_("Tearoff"),
                                                         P_("Whether the menu has a tearoff item"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

  /*
   * GtkTreeMenu:wrap-width:
   *
   * If wrap-width is set to a positive value, items in the popup will be laid
   * out along multiple columns, starting a new row on reaching the wrap width.
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_WRAP_WIDTH,
                                   g_param_spec_int ("wrap-width",
                                                     P_("Wrap Width"),
                                                     P_("Wrap width for laying out items in a grid"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  /*
   * GtkTreeMenu:row-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column
   * of type %G_TYPE_INT in the model. The value in that column for each item
   * will determine how many rows that item will span in the popup. Therefore,
   * values in this column must be greater than zero.
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_ROW_SPAN_COL,
                                   g_param_spec_int ("row-span-column",
                                                     P_("Row span column"),
                                                     P_("TreeModel column containing the row span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     CTK_PARAM_READWRITE));

  /*
   * GtkTreeMenu:column-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column
   * of type %G_TYPE_INT in the model. The value in that column for each item
   * will determine how many columns that item will span in the popup.
   * Therefore, values in this column must be greater than zero, and the sum of
   * an item’s column position + span should not exceed #GtkTreeMenu:wrap-width.
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_COL_SPAN_COL,
                                   g_param_spec_int ("column-span-column",
                                                     P_("Column span column"),
                                                     P_("TreeModel column containing the column span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     CTK_PARAM_READWRITE));
}

/****************************************************************
 *                         GObjectClass                         *
 ****************************************************************/
static void
ctk_tree_menu_constructed (GObject *object)
{
  GtkTreeMenu *menu = CTK_TREE_MENU (object);
  GtkTreeMenuPrivate *priv = menu->priv;

  G_OBJECT_CLASS (_ctk_tree_menu_parent_class)->constructed (object);

  if (!priv->area)
    {
      GtkCellArea *area = ctk_cell_area_box_new ();

      ctk_tree_menu_set_area (menu, area);
    }

  priv->context = ctk_cell_area_create_context (priv->area);

  priv->size_changed_id =
    g_signal_connect (priv->context, "notify",
                      G_CALLBACK (context_size_changed_cb), menu);
}

static void
ctk_tree_menu_dispose (GObject *object)
{
  GtkTreeMenu        *menu;
  GtkTreeMenuPrivate *priv;

  menu = CTK_TREE_MENU (object);
  priv = menu->priv;

  _ctk_tree_menu_set_model (menu, NULL);
  ctk_tree_menu_set_area (menu, NULL);

  if (priv->context)
    {
      /* Disconnect signals */
      g_signal_handler_disconnect (priv->context, priv->size_changed_id);

      g_object_unref (priv->context);
      priv->context = NULL;
      priv->size_changed_id = 0;
    }

  G_OBJECT_CLASS (_ctk_tree_menu_parent_class)->dispose (object);
}

static void
ctk_tree_menu_finalize (GObject *object)
{
  GtkTreeMenu        *menu;
  GtkTreeMenuPrivate *priv;

  menu = CTK_TREE_MENU (object);
  priv = menu->priv;

  _ctk_tree_menu_set_row_separator_func (menu, NULL, NULL, NULL);

  if (priv->root)
    ctk_tree_row_reference_free (priv->root);

  G_OBJECT_CLASS (_ctk_tree_menu_parent_class)->finalize (object);
}

static void
ctk_tree_menu_set_property (GObject            *object,
                            guint               prop_id,
                            const GValue       *value,
                            GParamSpec         *pspec)
{
  GtkTreeMenu *menu = CTK_TREE_MENU (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      _ctk_tree_menu_set_model (menu, g_value_get_object (value));
      break;

    case PROP_ROOT:
      _ctk_tree_menu_set_root (menu, g_value_get_boxed (value));
      break;

    case PROP_CELL_AREA:
      /* Construct-only, can only be assigned once */
      ctk_tree_menu_set_area (menu, (GtkCellArea *)g_value_get_object (value));
      break;

    case PROP_TEAROFF:
      _ctk_tree_menu_set_tearoff (menu, g_value_get_boolean (value));
      break;

    case PROP_WRAP_WIDTH:
      _ctk_tree_menu_set_wrap_width (menu, g_value_get_int (value));
      break;

     case PROP_ROW_SPAN_COL:
      _ctk_tree_menu_set_row_span_column (menu, g_value_get_int (value));
      break;

     case PROP_COL_SPAN_COL:
      _ctk_tree_menu_set_column_span_column (menu, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_tree_menu_get_property (GObject            *object,
                            guint               prop_id,
                            GValue             *value,
                            GParamSpec         *pspec)
{
  GtkTreeMenu        *menu = CTK_TREE_MENU (object);
  GtkTreeMenuPrivate *priv = menu->priv;

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    case PROP_ROOT:
      g_value_set_boxed (value, priv->root);
      break;

    case PROP_CELL_AREA:
      g_value_set_object (value, priv->area);
      break;

    case PROP_TEAROFF:
      g_value_set_boolean (value, priv->tearoff);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/****************************************************************
 *                         GtkWidgetClass                       *
 ****************************************************************/

/* We tell all the menu items to reserve space for the submenu
 * indicator if there is at least one submenu, this way we ensure
 * that every internal cell area gets allocated the
 * same width (and requested height for the same appropriate width).
 */
static void
sync_reserve_submenu_size (GtkTreeMenu *menu)
{
  GList              *children, *l;
  gboolean            has_submenu = FALSE;

  children = ctk_container_get_children (CTK_CONTAINER (menu));
  for (l = children; l; l = l->next)
    {
      GtkMenuItem *item = l->data;

      if (ctk_menu_item_get_submenu (item) != NULL)
        {
          has_submenu = TRUE;
          break;
        }
    }

  for (l = children; l; l = l->next)
    {
      GtkMenuItem *item = l->data;

      ctk_menu_item_set_reserve_indicator (item, has_submenu);
    }

  g_list_free (children);
}

static void
ctk_tree_menu_get_preferred_width (GtkWidget           *widget,
                                   gint                *minimum_size,
                                   gint                *natural_size)
{
  GtkTreeMenu        *menu = CTK_TREE_MENU (widget);
  GtkTreeMenuPrivate *priv = menu->priv;

  /* We leave the requesting work up to the cellviews which operate in the same
   * context, reserving space for the submenu indicator if any of the items have
   * submenus ensures that every cellview will receive the same allocated width.
   *
   * Since GtkMenu does hieght-for-width correctly, we know that the width of
   * every cell will be requested before the height-for-widths are requested.
   */
  g_signal_handler_block (priv->context, priv->size_changed_id);

  sync_reserve_submenu_size (menu);

  CTK_WIDGET_CLASS (_ctk_tree_menu_parent_class)->get_preferred_width (widget, minimum_size, natural_size);

  g_signal_handler_unblock (priv->context, priv->size_changed_id);
}

static void
ctk_tree_menu_get_preferred_height (GtkWidget           *widget,
                                    gint                *minimum_size,
                                    gint                *natural_size)
{
  GtkTreeMenu        *menu = CTK_TREE_MENU (widget);
  GtkTreeMenuPrivate *priv = menu->priv;

  g_signal_handler_block (priv->context, priv->size_changed_id);

  sync_reserve_submenu_size (menu);

  CTK_WIDGET_CLASS (_ctk_tree_menu_parent_class)->get_preferred_height (widget, minimum_size, natural_size);

  g_signal_handler_unblock (priv->context, priv->size_changed_id);
}

static void
ctk_tree_menu_get_preferred_width_for_height (GtkWidget           *widget,
                                              gint                 for_height,
                                              gint                *minimum_size,
                                              gint                *natural_size)
{
  GtkTreeMenu        *menu = CTK_TREE_MENU (widget);
  GtkTreeMenuPrivate *priv = menu->priv;

  /* We leave the requesting work up to the cellviews which operate in the same
   * context, reserving space for the submenu indicator if any of the items have
   * submenus ensures that every cellview will receive the same allocated width.
   *
   * Since GtkMenu does hieght-for-width correctly, we know that the width of
   * every cell will be requested before the height-for-widths are requested.
   */
  g_signal_handler_block (priv->context, priv->size_changed_id);

  sync_reserve_submenu_size (menu);

  CTK_WIDGET_CLASS (_ctk_tree_menu_parent_class)->get_preferred_width_for_height (widget, for_height, minimum_size, natural_size);

  g_signal_handler_unblock (priv->context, priv->size_changed_id);
}

static void
ctk_tree_menu_get_preferred_height_for_width (GtkWidget           *widget,
                                              gint                 for_width,
                                              gint                *minimum_size,
                                              gint                *natural_size)
{
  GtkTreeMenu        *menu = CTK_TREE_MENU (widget);
  GtkTreeMenuPrivate *priv = menu->priv;

  g_signal_handler_block (priv->context, priv->size_changed_id);

  sync_reserve_submenu_size (menu);

  CTK_WIDGET_CLASS (_ctk_tree_menu_parent_class)->get_preferred_height_for_width (widget, for_width, minimum_size, natural_size);

  g_signal_handler_unblock (priv->context, priv->size_changed_id);
}

/****************************************************************
 *                      GtkCellLayoutIface                      *
 ****************************************************************/
static void
ctk_tree_menu_cell_layout_init (GtkCellLayoutIface  *iface)
{
  iface->get_area = ctk_tree_menu_cell_layout_get_area;
}

static GtkCellArea *
ctk_tree_menu_cell_layout_get_area (GtkCellLayout *layout)
{
  GtkTreeMenu        *menu = CTK_TREE_MENU (layout);
  GtkTreeMenuPrivate *priv = menu->priv;

  return priv->area;
}


/****************************************************************
 *             TreeModel callbacks/populating menus             *
 ****************************************************************/
static GtkWidget *
ctk_tree_menu_get_path_item (GtkTreeMenu          *menu,
                             GtkTreePath          *search)
{
  GtkWidget *item = NULL;
  GList     *children, *l;

  children = ctk_container_get_children (CTK_CONTAINER (menu));

  for (l = children; item == NULL && l != NULL; l = l->next)
    {
      GtkWidget   *child = l->data;
      GtkTreePath *path  = NULL;

      if (CTK_IS_SEPARATOR_MENU_ITEM (child))
        {
          GtkTreeRowReference *row =
            g_object_get_qdata (G_OBJECT (child), tree_menu_path_quark);

          if (row)
            {
              path = ctk_tree_row_reference_get_path (row);

              if (!path)
                /* Return any first child where its row-reference became invalid,
                 * this is because row-references get null paths before we recieve
                 * the "row-deleted" signal.
                 */
                item = child;
            }
        }
      else if (!CTK_IS_TEAROFF_MENU_ITEM (child))
        {
          GtkWidget *view = ctk_bin_get_child (CTK_BIN (child));

          /* It's always a cellview */
          if (CTK_IS_CELL_VIEW (view))
            path = ctk_cell_view_get_displayed_row (CTK_CELL_VIEW (view));

          if (!path)
            /* Return any first child where its row-reference became invalid,
             * this is because row-references get null paths before we recieve
             * the "row-deleted" signal.
             */
            item = child;
        }

      if (path)
        {
          if (ctk_tree_path_compare (search, path) == 0)
            item = child;

          ctk_tree_path_free (path);
        }
    }

  g_list_free (children);

  return item;
}

static gboolean
ctk_tree_menu_path_in_menu (GtkTreeMenu  *menu,
                            GtkTreePath  *path,
                            gboolean     *header_item)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  gboolean            in_menu = FALSE;
  gboolean            is_header = FALSE;

  /* Check if the is in root of the model */
  if (ctk_tree_path_get_depth (path) == 1 && !priv->root)
    in_menu = TRUE;
  /* If we are a submenu, compare the parent path */
  else if (priv->root)
    {
      GtkTreePath *root_path   = ctk_tree_row_reference_get_path (priv->root);
      GtkTreePath *search_path = ctk_tree_path_copy (path);

      if (root_path)
        {
          if (priv->menu_with_header &&
              ctk_tree_path_compare (root_path, search_path) == 0)
            {
              in_menu   = TRUE;
              is_header = TRUE;
            }
          else if (ctk_tree_path_get_depth (search_path) > 1)
            {
              ctk_tree_path_up (search_path);

              if (ctk_tree_path_compare (root_path, search_path) == 0)
                in_menu = TRUE;
            }
        }
      ctk_tree_path_free (root_path);
      ctk_tree_path_free (search_path);
    }

  if (header_item)
    *header_item = is_header;

  return in_menu;
}

static GtkWidget *
ctk_tree_menu_path_needs_submenu (GtkTreeMenu *menu,
                                  GtkTreePath *search)
{
  GtkWidget   *item = NULL;
  GList       *children, *l;
  GtkTreePath *parent_path;

  if (ctk_tree_path_get_depth (search) <= 1)
    return NULL;

  parent_path = ctk_tree_path_copy (search);
  ctk_tree_path_up (parent_path);

  children    = ctk_container_get_children (CTK_CONTAINER (menu));

  for (l = children; item == NULL && l != NULL; l = l->next)
    {
      GtkWidget   *child = l->data;
      GtkTreePath *path  = NULL;

      /* Separators dont get submenus, if it already has a submenu then let
       * the submenu handle inserted rows */
      if (!CTK_IS_SEPARATOR_MENU_ITEM (child) &&
          !ctk_menu_item_get_submenu (CTK_MENU_ITEM (child)))
        {
          GtkWidget *view = ctk_bin_get_child (CTK_BIN (child));

          /* It's always a cellview */
          if (CTK_IS_CELL_VIEW (view))
            path = ctk_cell_view_get_displayed_row (CTK_CELL_VIEW (view));
        }

      if (path)
        {
          if (ctk_tree_path_compare (parent_path, path) == 0)
            item = child;

          ctk_tree_path_free (path);
        }
    }

  g_list_free (children);
  ctk_tree_path_free (parent_path);

  return item;
}

static GtkWidget *
find_empty_submenu (GtkTreeMenu  *menu)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  GList              *children, *l;
  GtkWidget          *submenu = NULL;

  children = ctk_container_get_children (CTK_CONTAINER (menu));

  for (l = children; submenu == NULL && l != NULL; l = l->next)
    {
      GtkWidget   *child = l->data;
      GtkTreePath *path  = NULL;
      GtkTreeIter  iter;

      /* Separators dont get submenus, if it already has a submenu then let
       * the submenu handle inserted rows */
      if (!CTK_IS_SEPARATOR_MENU_ITEM (child) && !CTK_IS_TEAROFF_MENU_ITEM (child))
        {
          GtkWidget *view = ctk_bin_get_child (CTK_BIN (child));

          /* It's always a cellview */
          if (CTK_IS_CELL_VIEW (view))
            path = ctk_cell_view_get_displayed_row (CTK_CELL_VIEW (view));
        }

      if (path)
        {
          if (ctk_tree_model_get_iter (priv->model, &iter, path) &&
              !ctk_tree_model_iter_has_child (priv->model, &iter))
            submenu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (child));

          ctk_tree_path_free (path);
        }
    }

  g_list_free (children);

  return submenu;
}

static void
row_inserted_cb (GtkTreeModel     *model,
                 GtkTreePath      *path,
                 GtkTreeIter      *iter,
                 GtkTreeMenu      *menu)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  gint               *indices, index, depth;
  GtkWidget          *item;

  /* If the iter should be in this menu then go ahead and insert it */
  if (ctk_tree_menu_path_in_menu (menu, path, NULL))
    {
      if (priv->wrap_width > 0)
        rebuild_menu (menu);
      else
        {
          /* Get the index of the path for this depth */
          indices = ctk_tree_path_get_indices (path);
          depth   = ctk_tree_path_get_depth (path);
          index   = indices[depth -1];

          /* Menus with a header include a menuitem for its root node
           * and a separator menu item */
          if (priv->menu_with_header)
            index += 2;

          /* Index after the tearoff item for the root menu if
           * there is a tearoff item
           */
          if (priv->root == NULL && priv->tearoff)
            index += 1;

          item = ctk_tree_menu_create_item (menu, iter, FALSE);
          ctk_menu_shell_insert (CTK_MENU_SHELL (menu), item, index);

          /* Resize everything */
          ctk_cell_area_context_reset (menu->priv->context);
        }
    }
  else
    {
      /* Create submenus for iters if we need to */
      item = ctk_tree_menu_path_needs_submenu (menu, path);
      if (item)
        {
          GtkTreePath *item_path = ctk_tree_path_copy (path);

          ctk_tree_path_up (item_path);
          ctk_tree_menu_create_submenu (menu, item, item_path);
          ctk_tree_path_free (item_path);
        }
    }
}

static void
row_deleted_cb (GtkTreeModel     *model,
                GtkTreePath      *path,
                GtkTreeMenu      *menu)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  GtkWidget          *item;

  /* If it's the header item we leave it to the parent menu
   * to remove us from its menu
   */
  item = ctk_tree_menu_get_path_item (menu, path);

  if (item)
    {
      if (priv->wrap_width > 0)
        rebuild_menu (menu);
      else
        {
          /* Get rid of the deleted item */
          ctk_widget_destroy (item);

          /* Resize everything */
          ctk_cell_area_context_reset (menu->priv->context);
        }
    }
  else
    {
      /* It's up to the parent menu to destroy a child menu that becomes empty
       * since the topmost menu belongs to the user and is allowed to have no contents */
      GtkWidget *submenu = find_empty_submenu (menu);
      if (submenu)
        ctk_widget_destroy (submenu);
    }
}

static void
row_reordered_cb (GtkTreeModel    *model,
                  GtkTreePath     *path,
                  GtkTreeIter     *iter,
                  gint            *new_order,
                  GtkTreeMenu     *menu)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  gboolean            this_menu = FALSE;

  if (ctk_tree_path_get_depth (path) == 0 && !priv->root)
    this_menu = TRUE;
  else if (priv->root)
    {
      GtkTreePath *root_path =
        ctk_tree_row_reference_get_path (priv->root);

      if (ctk_tree_path_compare (root_path, path) == 0)
        this_menu = TRUE;

      ctk_tree_path_free (root_path);
    }

  if (this_menu)
    rebuild_menu (menu);
}

static gint
menu_item_position (GtkTreeMenu *menu,
                    GtkWidget   *item)
{
  GList *children;
  gint   position;

  children = ctk_container_get_children (CTK_CONTAINER (menu));
  position = g_list_index (children, item);
  g_list_free (children);

  return position;
}

static void
row_changed_cb (GtkTreeModel         *model,
                GtkTreePath          *path,
                GtkTreeIter          *iter,
                GtkTreeMenu          *menu)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  gboolean            is_separator = FALSE;
  GtkWidget          *item;

  item = ctk_tree_menu_get_path_item (menu, path);

  if (priv->root)
    {
      GtkTreePath *root_path =
        ctk_tree_row_reference_get_path (priv->root);

      if (root_path && ctk_tree_path_compare (root_path, path) == 0)
        {
          if (item)
            {
              /* Destroy the header item and then the following separator */
              ctk_widget_destroy (item);
              ctk_widget_destroy (CTK_MENU_SHELL (menu)->priv->children->data);

              priv->menu_with_header = FALSE;
            }

          ctk_tree_path_free (root_path);
        }
    }

  if (item)
    {
      if (priv->wrap_width > 0)
        /* Ugly, we need to rebuild the menu here if
         * the row-span/row-column values change
         */
        rebuild_menu (menu);
      else
        {
          if (priv->row_separator_func)
            is_separator =
              priv->row_separator_func (model, iter,
                                        priv->row_separator_data);


          if (is_separator != CTK_IS_SEPARATOR_MENU_ITEM (item))
            {
              gint position = menu_item_position (menu, item);

              ctk_widget_destroy (item);
              item = ctk_tree_menu_create_item (menu, iter, FALSE);
              ctk_menu_shell_insert (CTK_MENU_SHELL (menu), item, position);
            }
        }
    }
}

static void
context_size_changed_cb (GtkCellAreaContext  *context,
                         GParamSpec          *pspec,
                         GtkWidget           *menu)
{
  if (!strcmp (pspec->name, "minimum-width") ||
      !strcmp (pspec->name, "natural-width") ||
      !strcmp (pspec->name, "minimum-height") ||
      !strcmp (pspec->name, "natural-height"))
    ctk_widget_queue_resize (menu);
}

static gboolean
area_is_sensitive (GtkCellArea *area)
{
  GList    *cells, *list;
  gboolean  sensitive = FALSE;

  cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (area));

  for (list = cells; list; list = list->next)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);

      if (sensitive)
        break;
    }
  g_list_free (cells);

  return sensitive;
}

static void
area_apply_attributes_cb (GtkCellArea          *area,
                          GtkTreeModel         *tree_model,
                          GtkTreeIter          *iter,
                          gboolean              is_expander,
                          gboolean              is_expanded,
                          GtkTreeMenu          *menu)
{
  /* If the menu for this iter has a submenu */
  GtkTreeMenuPrivate *priv = menu->priv;
  GtkTreePath        *path;
  GtkWidget          *item;
  gboolean            is_header;
  gboolean            sensitive;

  path = ctk_tree_model_get_path (tree_model, iter);

  if (ctk_tree_menu_path_in_menu (menu, path, &is_header))
    {
      item = ctk_tree_menu_get_path_item (menu, path);

      /* If there is no submenu, go ahead and update item sensitivity,
       * items with submenus are always sensitive */
      if (item && !ctk_menu_item_get_submenu (CTK_MENU_ITEM (item)))
        {
          sensitive = area_is_sensitive (priv->area);

          ctk_widget_set_sensitive (item, sensitive);

          if (is_header)
            {
              /* For header items we need to set the sensitivity
               * of the following separator item
               */
              if (CTK_MENU_SHELL (menu)->priv->children &&
                  CTK_MENU_SHELL (menu)->priv->children->next)
                {
                  GtkWidget *separator =
                    CTK_MENU_SHELL (menu)->priv->children->next->data;

                  ctk_widget_set_sensitive (separator, sensitive);
                }
            }
        }
    }

  ctk_tree_path_free (path);
}

static void
ctk_tree_menu_set_area (GtkTreeMenu *menu,
                        GtkCellArea *area)
{
  GtkTreeMenuPrivate *priv = menu->priv;

  if (priv->area)
    {
      g_signal_handler_disconnect (priv->area,
                                   priv->apply_attributes_id);
      priv->apply_attributes_id = 0;

      g_object_unref (priv->area);
    }

  priv->area = area;

  if (priv->area)
    {
      g_object_ref_sink (priv->area);

      priv->apply_attributes_id =
        g_signal_connect (priv->area, "apply-attributes",
                          G_CALLBACK (area_apply_attributes_cb), menu);
    }
}

static gboolean
menu_occupied (GtkTreeMenu *menu,
               guint        left_attach,
               guint        right_attach,
               guint        top_attach,
               guint        bottom_attach)
{
  GList *i;

  for (i = CTK_MENU_SHELL (menu)->priv->children; i; i = i->next)
    {
      guint l, r, b, t;

      ctk_container_child_get (CTK_CONTAINER (menu),
                               i->data,
                               "left-attach", &l,
                               "right-attach", &r,
                               "bottom-attach", &b,
                               "top-attach", &t,
                               NULL);

      /* look if this item intersects with the given coordinates */
      if (right_attach > l && left_attach < r && bottom_attach > t && top_attach < b)
        return TRUE;
    }

  return FALSE;
}

static void
relayout_item (GtkTreeMenu *menu,
               GtkWidget   *item,
               GtkTreeIter *iter,
               GtkWidget   *prev)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  gint                current_col = 0, current_row = 0;
  gint                rows = 1, cols = 1;

  if (priv->col_span_col == -1 &&
      priv->row_span_col == -1 &&
      prev)
    {
      ctk_container_child_get (CTK_CONTAINER (menu), prev,
                               "right-attach", &current_col,
                               "top-attach", &current_row,
                               NULL);
      if (current_col + cols > priv->wrap_width)
        {
          current_col = 0;
          current_row++;
        }
    }
  else
    {
      if (priv->col_span_col != -1)
        ctk_tree_model_get (priv->model, iter,
                            priv->col_span_col, &cols,
                            -1);
      if (priv->row_span_col != -1)
        ctk_tree_model_get (priv->model, iter,
                            priv->row_span_col, &rows,
                            -1);

      while (1)
        {
          if (current_col + cols > priv->wrap_width)
            {
              current_col = 0;
              current_row++;
            }

          if (!menu_occupied (menu,
                              current_col, current_col + cols,
                              current_row, current_row + rows))
            break;

          current_col++;
        }
    }

  /* set attach props */
  ctk_menu_attach (CTK_MENU (menu), item,
                   current_col, current_col + cols,
                   current_row, current_row + rows);
}

static void
ctk_tree_menu_create_submenu (GtkTreeMenu *menu,
                              GtkWidget   *item,
                              GtkTreePath *path)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  GtkWidget          *view;
  GtkWidget          *submenu;

  view = ctk_bin_get_child (CTK_BIN (item));
  ctk_cell_view_set_draw_sensitive (CTK_CELL_VIEW (view), TRUE);

  submenu = _ctk_tree_menu_new_with_area (priv->area);

  _ctk_tree_menu_set_row_separator_func (CTK_TREE_MENU (submenu),
                                         priv->row_separator_func,
                                         priv->row_separator_data,
                                         priv->row_separator_destroy);

  _ctk_tree_menu_set_wrap_width (CTK_TREE_MENU (submenu), priv->wrap_width);
  _ctk_tree_menu_set_row_span_column (CTK_TREE_MENU (submenu), priv->row_span_col);
  _ctk_tree_menu_set_column_span_column (CTK_TREE_MENU (submenu), priv->col_span_col);

  ctk_tree_menu_set_model_internal (CTK_TREE_MENU (submenu), priv->model);
  _ctk_tree_menu_set_root (CTK_TREE_MENU (submenu), path);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (item), submenu);

  g_signal_connect (submenu, "menu-activate",
                    G_CALLBACK (submenu_activated_cb), menu);
}

static GtkWidget *
ctk_tree_menu_create_item (GtkTreeMenu *menu,
                           GtkTreeIter *iter,
                           gboolean     header_item)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  GtkWidget          *item, *view;
  GtkTreePath        *path;
  gboolean            is_separator = FALSE;

  path = ctk_tree_model_get_path (priv->model, iter);

  if (priv->row_separator_func)
    is_separator =
      priv->row_separator_func (priv->model, iter,
                                priv->row_separator_data);

  if (is_separator)
    {
      item = ctk_separator_menu_item_new ();
      ctk_widget_show (item);

      g_object_set_qdata_full (G_OBJECT (item),
                               tree_menu_path_quark,
                               ctk_tree_row_reference_new (priv->model, path),
                               (GDestroyNotify)ctk_tree_row_reference_free);
    }
  else
    {
      view = ctk_cell_view_new_with_context (priv->area, priv->context);
      item = ctk_menu_item_new ();
      ctk_widget_show (view);
      ctk_widget_show (item);

      ctk_cell_view_set_model (CTK_CELL_VIEW (view), priv->model);
      ctk_cell_view_set_displayed_row (CTK_CELL_VIEW (view), path);

      ctk_widget_show (view);
      ctk_container_add (CTK_CONTAINER (item), view);

      g_signal_connect (item, "activate", G_CALLBACK (item_activated_cb), menu);

      /* Add a GtkTreeMenu submenu to render the children of this row */
      if (header_item == FALSE &&
          ctk_tree_model_iter_has_child (priv->model, iter))
        ctk_tree_menu_create_submenu (menu, item, path);
    }

  ctk_tree_path_free (path);

  return item;
}

static inline void
rebuild_menu (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv = menu->priv;

  /* Destroy all the menu items */
  ctk_container_foreach (CTK_CONTAINER (menu),
                         (GtkCallback) ctk_widget_destroy, NULL);

  /* Populate */
  if (priv->model)
    ctk_tree_menu_populate (menu);
}


static void
ctk_tree_menu_populate (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv = menu->priv;
  GtkTreePath        *path = NULL;
  GtkTreeIter         parent;
  GtkTreeIter         iter;
  gboolean            valid = FALSE;
  GtkWidget          *menu_item, *prev = NULL;

  if (!priv->model)
    return;

  if (priv->root)
    path = ctk_tree_row_reference_get_path (priv->root);

  if (path)
    {
      if (ctk_tree_model_get_iter (priv->model, &parent, path))
        valid = ctk_tree_model_iter_children (priv->model, &iter, &parent);

      ctk_tree_path_free (path);
    }
  else
    {
      /* Tearoff menu items only go in the root menu */
      if (priv->tearoff)
        {
          menu_item = ctk_tearoff_menu_item_new ();
          ctk_widget_show (menu_item);

          if (priv->wrap_width > 0)
            ctk_menu_attach (CTK_MENU (menu), menu_item, 0, priv->wrap_width, 0, 1);
          else
            ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);

          prev = menu_item;
        }

      valid = ctk_tree_model_iter_children (priv->model, &iter, NULL);
    }

  /* Create a menu item for every row at the current depth, add a GtkTreeMenu
   * submenu for iters/items that have children */
  while (valid)
    {
      menu_item = ctk_tree_menu_create_item (menu, &iter, FALSE);

      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);

      if (priv->wrap_width > 0)
        relayout_item (menu, menu_item, &iter, prev);

      prev  = menu_item;
      valid = ctk_tree_model_iter_next (priv->model, &iter);
    }
}

static void
item_activated_cb (GtkMenuItem          *item,
                   GtkTreeMenu          *menu)
{
  GtkCellView *view;
  GtkTreePath *path;
  gchar       *path_str;

  /* Only activate leafs, not parents */
  if (!ctk_menu_item_get_submenu (item))
    {
      view     = CTK_CELL_VIEW (ctk_bin_get_child (CTK_BIN (item)));
      path     = ctk_cell_view_get_displayed_row (view);
      path_str = ctk_tree_path_to_string (path);

      g_signal_emit (menu, tree_menu_signals[SIGNAL_MENU_ACTIVATE], 0, path_str);

      g_free (path_str);
      ctk_tree_path_free (path);
    }
}

static void
submenu_activated_cb (GtkTreeMenu          *submenu,
                      const gchar          *path,
                      GtkTreeMenu          *menu)
{
  g_signal_emit (menu, tree_menu_signals[SIGNAL_MENU_ACTIVATE], 0, path);
}

/* Sets the model without rebuilding the menu, prevents
 * infinite recursion while building submenus (we wait
 * until the root is set and then build the menu) */
static void
ctk_tree_menu_set_model_internal (GtkTreeMenu  *menu,
                                  GtkTreeModel *model)
{
  GtkTreeMenuPrivate *priv;

  priv = menu->priv;

  if (priv->model != model)
    {
      if (priv->model)
        {
          /* Disconnect signals */
          g_signal_handler_disconnect (priv->model,
                                       priv->row_inserted_id);
          g_signal_handler_disconnect (priv->model,
                                       priv->row_deleted_id);
          g_signal_handler_disconnect (priv->model,
                                       priv->row_reordered_id);
          g_signal_handler_disconnect (priv->model,
                                       priv->row_changed_id);
          priv->row_inserted_id  = 0;
          priv->row_deleted_id   = 0;
          priv->row_reordered_id = 0;
          priv->row_changed_id = 0;

          g_object_unref (priv->model);
        }

      priv->model = model;

      if (priv->model)
        {
          g_object_ref (priv->model);

          /* Connect signals */
          priv->row_inserted_id  = g_signal_connect (priv->model, "row-inserted",
                                                     G_CALLBACK (row_inserted_cb), menu);
          priv->row_deleted_id   = g_signal_connect (priv->model, "row-deleted",
                                                     G_CALLBACK (row_deleted_cb), menu);
          priv->row_reordered_id = g_signal_connect (priv->model, "rows-reordered",
                                                     G_CALLBACK (row_reordered_cb), menu);
          priv->row_changed_id   = g_signal_connect (priv->model, "row-changed",
                                                     G_CALLBACK (row_changed_cb), menu);
        }
    }
}

/****************************************************************
 *                            API                               *
 ****************************************************************/

/**
 * _ctk_tree_menu_new:
 *
 * Creates a new #GtkTreeMenu.
 *
 * Returns: A newly created #GtkTreeMenu with no model or root.
 *
 * Since: 3.0
 */
GtkWidget *
_ctk_tree_menu_new (void)
{
  return (GtkWidget *)g_object_new (CTK_TYPE_TREE_MENU, NULL);
}

/*
 * _ctk_tree_menu_new_with_area:
 * @area: the #GtkCellArea to use to render cells in the menu
 *
 * Creates a new #GtkTreeMenu using @area to render its cells.
 *
 * Returns: A newly created #GtkTreeMenu with no model or root.
 *
 * Since: 3.0
 */
GtkWidget *
_ctk_tree_menu_new_with_area (GtkCellArea    *area)
{
  return (GtkWidget *)g_object_new (CTK_TYPE_TREE_MENU,
                                    "cell-area", area,
                                    NULL);
}

/*
 * _ctk_tree_menu_new_full:
 * @area: (allow-none): the #GtkCellArea to use to render cells in the menu, or %NULL.
 * @model: (allow-none): the #GtkTreeModel to build the menu heirarchy from, or %NULL.
 * @root: (allow-none): the #GtkTreePath indicating the root row for this menu, or %NULL.
 *
 * Creates a new #GtkTreeMenu hierarchy from the provided @model and @root using @area to render its cells.
 *
 * Returns: A newly created #GtkTreeMenu.
 *
 * Since: 3.0
 */
GtkWidget *
_ctk_tree_menu_new_full (GtkCellArea         *area,
                         GtkTreeModel        *model,
                         GtkTreePath         *root)
{
  return (GtkWidget *)g_object_new (CTK_TYPE_TREE_MENU,
                                    "cell-area", area,
                                    "model", model,
                                    "root", root,
                                    NULL);
}

/*
 * _ctk_tree_menu_set_model:
 * @menu: a #GtkTreeMenu
 * @model: (allow-none): the #GtkTreeModel to build the menu hierarchy from, or %NULL.
 *
 * Sets @model to be used to build the menu heirarhcy.
 *
 * Since: 3.0
 */
void
_ctk_tree_menu_set_model (GtkTreeMenu  *menu,
                          GtkTreeModel *model)
{
  g_return_if_fail (CTK_IS_TREE_MENU (menu));
  g_return_if_fail (model == NULL || CTK_IS_TREE_MODEL (model));

  ctk_tree_menu_set_model_internal (menu, model);

  rebuild_menu (menu);
}

/*
 * _ctk_tree_menu_get_model:
 * @menu: a #GtkTreeMenu
 *
 * Gets the @model currently used for the menu heirarhcy.
 *
 * Returns: (transfer none): the #GtkTreeModel which is used
 * for @menu’s hierarchy.
 *
 * Since: 3.0
 */
GtkTreeModel *
_ctk_tree_menu_get_model (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_MENU (menu), NULL);

  priv = menu->priv;

  return priv->model;
}

/*
 * _ctk_tree_menu_set_root:
 * @menu: a #GtkTreeMenu
 * @path: (allow-none): the #GtkTreePath which is the root of @menu, or %NULL.
 *
 * Sets the root of a @menu’s hierarchy to be @path. @menu must already
 * have a model set and @path must point to a valid path inside the model.
 *
 * Since: 3.0
 */
void
_ctk_tree_menu_set_root (GtkTreeMenu *menu,
                         GtkTreePath *path)
{
  GtkTreeMenuPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_MENU (menu));
  g_return_if_fail (menu->priv->model != NULL || path == NULL);

  priv = menu->priv;

  if (priv->root)
    ctk_tree_row_reference_free (priv->root);

  if (path)
    priv->root = ctk_tree_row_reference_new (priv->model, path);
  else
    priv->root = NULL;

  rebuild_menu (menu);
}

/*
 * _ctk_tree_menu_get_root:
 * @menu: a #GtkTreeMenu
 *
 * Gets the @root path for @menu’s hierarchy, or returns %NULL if @menu
 * has no model or is building a heirarchy for the entire model. *
 *
 * Returns: (transfer full) (allow-none): A newly created #GtkTreePath
 * pointing to the root of @menu which must be freed with ctk_tree_path_free().
 *
 * Since: 3.0
 */
GtkTreePath *
_ctk_tree_menu_get_root (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_MENU (menu), NULL);

  priv = menu->priv;

  if (priv->root)
    return ctk_tree_row_reference_get_path (priv->root);

  return NULL;
}

/*
 * _ctk_tree_menu_get_tearoff:
 * @menu: a #GtkTreeMenu
 *
 * Gets whether this menu is build with a leading tearoff menu item.
 *
 * Returns: %TRUE if the menu has a tearoff item.
 *
 * Since: 3.0
 */
gboolean
_ctk_tree_menu_get_tearoff (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_MENU (menu), FALSE);

  priv = menu->priv;

  return priv->tearoff;
}

/*
 * _ctk_tree_menu_set_tearoff:
 * @menu: a #GtkTreeMenu
 * @tearoff: whether the menu should have a leading tearoff menu item.
 *
 * Sets whether this menu has a leading tearoff menu item.
 *
 * Since: 3.0
 */
void
_ctk_tree_menu_set_tearoff (GtkTreeMenu *menu,
                            gboolean     tearoff)
{
  GtkTreeMenuPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_MENU (menu));

  priv = menu->priv;

  if (priv->tearoff != tearoff)
    {
      priv->tearoff = tearoff;

      rebuild_menu (menu);

      g_object_notify (G_OBJECT (menu), "tearoff");
    }
}

/*
 * _ctk_tree_menu_get_wrap_width:
 * @menu: a #GtkTreeMenu
 *
 * Gets the wrap width which is used to determine the number of columns
 * for @menu. If the wrap width is larger than 1, @menu is in table mode.
 *
 * Returns: the wrap width.
 *
 * Since: 3.0
 */
gint
_ctk_tree_menu_get_wrap_width (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_MENU (menu), FALSE);

  priv = menu->priv;

  return priv->wrap_width;
}

/*
 * _ctk_tree_menu_set_wrap_width:
 * @menu: a #GtkTreeMenu
 * @width: the wrap width
 *
 * Sets the wrap width which is used to determine the number of columns
 * for @menu. If the wrap width is larger than 1, @menu is in table mode.
 *
 * Since: 3.0
 */
void
_ctk_tree_menu_set_wrap_width (GtkTreeMenu *menu,
                               gint         width)
{
  GtkTreeMenuPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_MENU (menu));
  g_return_if_fail (width >= 0);

  priv = menu->priv;

  if (priv->wrap_width != width)
    {
      priv->wrap_width = width;

      rebuild_menu (menu);

      g_object_notify (G_OBJECT (menu), "wrap-width");
    }
}

/*
 * _ctk_tree_menu_get_row_span_column:
 * @menu: a #GtkTreeMenu
 *
 * Gets the column with row span information for @menu.
 * The row span column contains integers which indicate how many rows
 * a menu item should span.
 *
 * Returns: the column in @menu’s model containing row span information, or -1.
 *
 * Since: 3.0
 */
gint
_ctk_tree_menu_get_row_span_column (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_MENU (menu), FALSE);

  priv = menu->priv;

  return priv->row_span_col;
}

/*
 * _ctk_tree_menu_set_row_span_column:
 * @menu: a #GtkTreeMenu
 * @row_span: the column in the model to fetch the row span for a given menu item.
 *
 * Sets the column with row span information for @menu to be @row_span.
 * The row span column contains integers which indicate how many rows
 * a menu item should span.
 *
 * Since: 3.0
 */
void
_ctk_tree_menu_set_row_span_column (GtkTreeMenu *menu,
                                    gint         row_span)
{
  GtkTreeMenuPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_MENU (menu));

  priv = menu->priv;

  if (priv->row_span_col != row_span)
    {
      priv->row_span_col = row_span;

      if (priv->wrap_width > 0)
        rebuild_menu (menu);

      g_object_notify (G_OBJECT (menu), "row-span-column");
    }
}

/*
 * _ctk_tree_menu_get_column_span_column:
 * @menu: a #GtkTreeMenu
 *
 * Gets the column with column span information for @menu.
 * The column span column contains integers which indicate how many columns
 * a menu item should span.
 *
 * Returns: the column in @menu’s model containing column span information, or -1.
 *
 * Since: 3.0
 */
gint
_ctk_tree_menu_get_column_span_column (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_MENU (menu), FALSE);

  priv = menu->priv;

  return priv->col_span_col;
}

/*
 * _ctk_tree_menu_set_column_span_column:
 * @menu: a #GtkTreeMenu
 * @column_span: the column in the model to fetch the column span for a given menu item.
 *
 * Sets the column with column span information for @menu to be @column_span.
 * The column span column contains integers which indicate how many columns
 * a menu item should span.
 *
 * Since: 3.0
 */
void
_ctk_tree_menu_set_column_span_column (GtkTreeMenu *menu,
                                       gint         column_span)
{
  GtkTreeMenuPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_MENU (menu));

  priv = menu->priv;

  if (priv->col_span_col != column_span)
    {
      priv->col_span_col = column_span;

      if (priv->wrap_width > 0)
        rebuild_menu (menu);

      g_object_notify (G_OBJECT (menu), "column-span-column");
    }
}

/*
 * _ctk_tree_menu_get_row_separator_func:
 * @menu: a #GtkTreeMenu
 *
 * Gets the current #GtkTreeViewRowSeparatorFunc separator function.
 *
 * Returns: the current row separator function.
 *
 * Since: 3.0
 */
GtkTreeViewRowSeparatorFunc
_ctk_tree_menu_get_row_separator_func (GtkTreeMenu *menu)
{
  GtkTreeMenuPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_MENU (menu), NULL);

  priv = menu->priv;

  return priv->row_separator_func;
}

/*
 * _ctk_tree_menu_set_row_separator_func:
 * @menu: a #GtkTreeMenu
 * @func: (allow-none): a #GtkTreeViewRowSeparatorFunc, or %NULL to unset the separator function.
 * @data: (allow-none): user data to pass to @func, or %NULL
 * @destroy: (allow-none): destroy notifier for @data, or %NULL
 *
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Since: 3.0
 */
void
_ctk_tree_menu_set_row_separator_func (GtkTreeMenu          *menu,
                                       GtkTreeViewRowSeparatorFunc func,
                                       gpointer              data,
                                       GDestroyNotify        destroy)
{
  GtkTreeMenuPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_MENU (menu));

  priv = menu->priv;

  if (priv->row_separator_destroy)
    priv->row_separator_destroy (priv->row_separator_data);

  priv->row_separator_func    = func;
  priv->row_separator_data    = data;
  priv->row_separator_destroy = destroy;

  rebuild_menu (menu);
}
