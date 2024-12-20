/*
 * Copyright (C) 2012 Alexander Larsson <alexl@redhat.com>
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

#include "ctkactionhelper.h"
#include "ctkadjustmentprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctklistbox.h"
#include "ctkwidget.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkcsscustomgadgetprivate.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "a11y/ctklistboxaccessibleprivate.h"
#include "a11y/ctklistboxrowaccessible.h"

/**
 * SECTION:ctklistbox
 * @Short_description: A list container
 * @Title: CtkListBox
 * @See_also: #CtkScrolledWindow
 *
 * A CtkListBox is a vertical container that contains CtkListBoxRow
 * children. These rows can by dynamically sorted and filtered, and
 * headers can be added dynamically depending on the row content.
 * It also allows keyboard and mouse navigation and selection like
 * a typical list.
 *
 * Using CtkListBox is often an alternative to #CtkTreeView, especially
 * when the list contents has a more complicated layout than what is allowed
 * by a #CtkCellRenderer, or when the contents is interactive (i.e. has a
 * button in it).
 *
 * Although a #CtkListBox must have only #CtkListBoxRow children you can
 * add any kind of widget to it via ctk_container_add(), and a #CtkListBoxRow
 * widget will automatically be inserted between the list and the widget.
 *
 * #CtkListBoxRows can be marked as activatable or selectable. If a row
 * is activatable, #CtkListBox::row-activated will be emitted for it when
 * the user tries to activate it. If it is selectable, the row will be marked
 * as selected when the user tries to select it.
 *
 * The CtkListBox widget was added in CTK+ 3.10.
 *
 * # CtkListBox as CtkBuildable
 *
 * The CtkListBox implementation of the #CtkBuildable interface supports
 * setting a child as the placeholder by specifying “placeholder” as the “type”
 * attribute of a <child> element. See ctk_list_box_set_placeholder() for info.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * list
 * ╰── row[.activatable]
 * ]|
 *
 * CtkListBox uses a single CSS node named list. Each CtkListBoxRow uses
 * a single CSS node named row. The row nodes get the .activatable
 * style class added when appropriate.
 */

typedef struct
{
  GSequence *children;
  GHashTable *header_hash;

  CtkWidget *placeholder;

  CtkCssGadget *gadget;

  CtkListBoxSortFunc sort_func;
  gpointer sort_func_target;
  GDestroyNotify sort_func_target_destroy_notify;

  CtkListBoxFilterFunc filter_func;
  gpointer filter_func_target;
  GDestroyNotify filter_func_target_destroy_notify;

  CtkListBoxUpdateHeaderFunc update_header_func;
  gpointer update_header_func_target;
  GDestroyNotify update_header_func_target_destroy_notify;

  CtkListBoxRow *selected_row;
  CtkListBoxRow *prelight_row;
  CtkListBoxRow *cursor_row;

  gboolean active_row_active;
  CtkListBoxRow *active_row;

  CtkSelectionMode selection_mode;

  CtkAdjustment *adjustment;
  gboolean activate_single_click;

  CtkGesture *multipress_gesture;

  /* DnD */
  CtkListBoxRow *drag_highlighted_row;

  int n_visible_rows;
  gboolean in_widget;

  GListModel *bound_model;
  CtkListBoxCreateWidgetFunc create_widget_func;
  gpointer create_widget_func_data;
  GDestroyNotify create_widget_func_data_destroy;
} CtkListBoxPrivate;

typedef struct
{
  GSequenceIter *iter;
  CtkWidget *header;
  CtkCssGadget *gadget;
  CtkActionHelper *action_helper;
  gint y;
  gint height;
  guint visible     :1;
  guint selected    :1;
  guint activatable :1;
  guint selectable  :1;
} CtkListBoxRowPrivate;

enum {
  ROW_SELECTED,
  ROW_ACTIVATED,
  ACTIVATE_CURSOR_ROW,
  TOGGLE_CURSOR_ROW,
  MOVE_CURSOR,
  SELECTED_ROWS_CHANGED,
  SELECT_ALL,
  UNSELECT_ALL,
  LAST_SIGNAL
};

enum {
  ROW__ACTIVATE,
  ROW__LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SELECTION_MODE,
  PROP_ACTIVATE_ON_SINGLE_CLICK,
  LAST_PROPERTY
};

enum {
  ROW_PROP_0,
  ROW_PROP_ACTIVATABLE,
  ROW_PROP_SELECTABLE,

  /* actionable properties */
  ROW_PROP_ACTION_NAME,
  ROW_PROP_ACTION_TARGET,

  LAST_ROW_PROPERTY = ROW_PROP_ACTION_NAME
};

#define BOX_PRIV(box) ((CtkListBoxPrivate*)ctk_list_box_get_instance_private ((CtkListBox*)(box)))
#define ROW_PRIV(row) ((CtkListBoxRowPrivate*)ctk_list_box_row_get_instance_private ((CtkListBoxRow*)(row)))

static void     ctk_list_box_buildable_interface_init   (CtkBuildableIface *iface);

static void     ctk_list_box_row_actionable_iface_init  (CtkActionableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkListBox, ctk_list_box, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkListBox)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_list_box_buildable_interface_init))
G_DEFINE_TYPE_WITH_CODE (CtkListBoxRow, ctk_list_box_row, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkListBoxRow)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIONABLE, ctk_list_box_row_actionable_iface_init))

static void                 ctk_list_box_apply_filter_all             (CtkListBox          *box);
static void                 ctk_list_box_update_header                (CtkListBox          *box,
                                                                       GSequenceIter       *iter);
static GSequenceIter *      ctk_list_box_get_next_visible             (CtkListBox          *box,
                                                                       GSequenceIter       *iter);
static void                 ctk_list_box_apply_filter                 (CtkListBox          *box,
                                                                       CtkListBoxRow       *row);
static void                 ctk_list_box_add_move_binding             (CtkBindingSet       *binding_set,
                                                                       guint                keyval,
                                                                       CdkModifierType      modmask,
                                                                       CtkMovementStep      step,
                                                                       gint                 count);
static void                 ctk_list_box_update_cursor                (CtkListBox          *box,
                                                                       CtkListBoxRow       *row,
                                                                       gboolean             grab_focus);
static void                 ctk_list_box_update_prelight              (CtkListBox          *box,
                                                                       CtkListBoxRow       *row);
static void                 ctk_list_box_update_active                (CtkListBox          *box,
                                                                       CtkListBoxRow       *row);
static gboolean             ctk_list_box_enter_notify_event           (CtkWidget           *widget,
                                                                       CdkEventCrossing    *event);
static gboolean             ctk_list_box_leave_notify_event           (CtkWidget           *widget,
                                                                       CdkEventCrossing    *event);
static gboolean             ctk_list_box_motion_notify_event          (CtkWidget           *widget,
                                                                       CdkEventMotion      *event);
static void                 ctk_list_box_show                         (CtkWidget           *widget);
static gboolean             ctk_list_box_focus                        (CtkWidget           *widget,
                                                                       CtkDirectionType     direction);
static GSequenceIter*       ctk_list_box_get_previous_visible         (CtkListBox          *box,
                                                                       GSequenceIter       *iter);
static CtkListBoxRow       *ctk_list_box_get_first_focusable          (CtkListBox          *box);
static CtkListBoxRow       *ctk_list_box_get_last_focusable           (CtkListBox          *box);
static gboolean             ctk_list_box_draw                         (CtkWidget           *widget,
                                                                       cairo_t             *cr);
static void                 ctk_list_box_realize                      (CtkWidget           *widget);
static void                 ctk_list_box_add                          (CtkContainer        *container,
                                                                       CtkWidget           *widget);
static void                 ctk_list_box_remove                       (CtkContainer        *container,
                                                                       CtkWidget           *widget);
static void                 ctk_list_box_forall                       (CtkContainer        *container,
                                                                       gboolean             include_internals,
                                                                       CtkCallback          callback,
                                                                       gpointer             callback_target);
static void                 ctk_list_box_compute_expand               (CtkWidget           *widget,
                                                                       gboolean            *hexpand,
                                                                       gboolean            *vexpand);
static GType                ctk_list_box_child_type                   (CtkContainer        *container);
static CtkSizeRequestMode   ctk_list_box_get_request_mode             (CtkWidget           *widget);
static void                 ctk_list_box_size_allocate                (CtkWidget           *widget,
                                                                       CtkAllocation       *allocation);
static void                 ctk_list_box_drag_leave                   (CtkWidget           *widget,
                                                                       CdkDragContext      *context,
                                                                       guint                time_);
static void                 ctk_list_box_activate_cursor_row          (CtkListBox          *box);
static void                 ctk_list_box_toggle_cursor_row            (CtkListBox          *box);
static void                 ctk_list_box_move_cursor                  (CtkListBox          *box,
                                                                       CtkMovementStep      step,
                                                                       gint                 count);
static void                 ctk_list_box_finalize                     (GObject             *obj);
static void                 ctk_list_box_parent_set                   (CtkWidget           *widget,
                                                                       CtkWidget           *prev_parent);

static void                 ctk_list_box_get_preferred_height           (CtkWidget           *widget,
                                                                         gint                *minimum_height,
                                                                         gint                *natural_height);
static void                 ctk_list_box_get_preferred_height_for_width (CtkWidget           *widget,
                                                                         gint                 width,
                                                                         gint                *minimum_height,
                                                                         gint                *natural_height);
static void                 ctk_list_box_get_preferred_width            (CtkWidget           *widget,
                                                                         gint                *minimum_width,
                                                                         gint                *natural_width);
static void                 ctk_list_box_get_preferred_width_for_height (CtkWidget           *widget,
                                                                         gint                 height,
                                                                         gint                *minimum_width,
                                                                         gint                *natural_width);

static void                 ctk_list_box_select_row_internal            (CtkListBox          *box,
                                                                         CtkListBoxRow       *row);
static void                 ctk_list_box_unselect_row_internal          (CtkListBox          *box,
                                                                         CtkListBoxRow       *row);
static void                 ctk_list_box_select_all_between             (CtkListBox          *box,
                                                                         CtkListBoxRow       *row1,
                                                                         CtkListBoxRow       *row2,
                                                                         gboolean             modify);
static gboolean             ctk_list_box_unselect_all_internal          (CtkListBox          *box);
static void                 ctk_list_box_selected_rows_changed          (CtkListBox          *box);

static void ctk_list_box_multipress_gesture_pressed  (CtkGestureMultiPress *gesture,
                                                      guint                 n_press,
                                                      gdouble               x,
                                                      gdouble               y,
                                                      CtkListBox           *box);
static void ctk_list_box_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                                      guint                 n_press,
                                                      gdouble               x,
                                                      gdouble               y,
                                                      CtkListBox           *box);

static void ctk_list_box_update_row_styles (CtkListBox    *box);
static void ctk_list_box_update_row_style  (CtkListBox    *box,
                                            CtkListBoxRow *row);

static void                 ctk_list_box_bound_model_changed            (GListModel          *list,
                                                                         guint                position,
                                                                         guint                removed,
                                                                         guint                added,
                                                                         gpointer             user_data);

static void                 ctk_list_box_check_model_compat             (CtkListBox          *box);

static void     ctk_list_box_measure    (CtkCssGadget        *gadget,
                                          CtkOrientation       orientation,
                                          gint                 for_size,
                                          gint                *minimum_size,
                                          gint                *natural_size,
                                          gint                *minimum_baseline,
                                          gint                *natural_baseline,
                                          gpointer             data);
static void     ctk_list_box_allocate    (CtkCssGadget        *gadget,
                                          const CtkAllocation *allocation,
                                          int                  baseline,
                                          CtkAllocation       *out_clip,
                                          gpointer             data);
static gboolean ctk_list_box_render      (CtkCssGadget        *gadget,
                                          cairo_t             *cr,
                                          int                  x,
                                          int                  y,
                                          int                  width,
                                          int                  height,
                                          gpointer             data);



static GParamSpec *properties[LAST_PROPERTY] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *row_properties[LAST_ROW_PROPERTY] = { NULL, };
static guint row_signals[ROW__LAST_SIGNAL] = { 0 };

/**
 * ctk_list_box_new:
 *
 * Creates a new #CtkListBox container.
 *
 * Returns: a new #CtkListBox
 *
 * Since: 3.10
 */
CtkWidget *
ctk_list_box_new (void)
{
  return g_object_new (CTK_TYPE_LIST_BOX, NULL);
}

static void
ctk_list_box_get_property (GObject    *obj,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CtkListBoxPrivate *priv = BOX_PRIV (obj);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, priv->selection_mode);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, priv->activate_single_click);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_list_box_set_property (GObject      *obj,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkListBox *box = CTK_LIST_BOX (obj);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      ctk_list_box_set_selection_mode (box, g_value_get_enum (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      ctk_list_box_set_activate_on_single_click (box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_list_box_finalize (GObject *obj)
{
  CtkListBoxPrivate *priv = BOX_PRIV (obj);

  if (priv->sort_func_target_destroy_notify != NULL)
    priv->sort_func_target_destroy_notify (priv->sort_func_target);
  if (priv->filter_func_target_destroy_notify != NULL)
    priv->filter_func_target_destroy_notify (priv->filter_func_target);
  if (priv->update_header_func_target_destroy_notify != NULL)
    priv->update_header_func_target_destroy_notify (priv->update_header_func_target);

  g_clear_object (&priv->adjustment);
  g_clear_object (&priv->drag_highlighted_row);
  g_clear_object (&priv->multipress_gesture);

  g_sequence_free (priv->children);
  g_hash_table_unref (priv->header_hash);

  if (priv->bound_model)
    {
      if (priv->create_widget_func_data_destroy)
        priv->create_widget_func_data_destroy (priv->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (priv->bound_model, ctk_list_box_bound_model_changed, obj);
      g_clear_object (&priv->bound_model);
    }

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_list_box_parent_class)->finalize (obj);
}

static void
ctk_list_box_dispose (GObject *object)
{
  CtkListBoxPrivate *priv = BOX_PRIV (object);

  if (priv->placeholder)
    {
      ctk_widget_unparent (priv->placeholder);
      priv->placeholder = NULL;
    }

  G_OBJECT_CLASS (ctk_list_box_parent_class)->dispose (object);
}

static void
ctk_list_box_class_init (CtkListBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);
  CtkBindingSet *binding_set;

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_LIST_BOX_ACCESSIBLE);

  object_class->get_property = ctk_list_box_get_property;
  object_class->set_property = ctk_list_box_set_property;
  object_class->finalize = ctk_list_box_finalize;
  object_class->dispose = ctk_list_box_dispose;
  widget_class->enter_notify_event = ctk_list_box_enter_notify_event;
  widget_class->leave_notify_event = ctk_list_box_leave_notify_event;
  widget_class->motion_notify_event = ctk_list_box_motion_notify_event;
  widget_class->show = ctk_list_box_show;
  widget_class->focus = ctk_list_box_focus;
  widget_class->draw = ctk_list_box_draw;
  widget_class->realize = ctk_list_box_realize;
  widget_class->compute_expand = ctk_list_box_compute_expand;
  widget_class->get_request_mode = ctk_list_box_get_request_mode;
  widget_class->get_preferred_height = ctk_list_box_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_list_box_get_preferred_height_for_width;
  widget_class->get_preferred_width = ctk_list_box_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_list_box_get_preferred_width_for_height;
  widget_class->size_allocate = ctk_list_box_size_allocate;
  widget_class->drag_leave = ctk_list_box_drag_leave;
  widget_class->parent_set = ctk_list_box_parent_set;
  container_class->add = ctk_list_box_add;
  container_class->remove = ctk_list_box_remove;
  container_class->forall = ctk_list_box_forall;
  container_class->child_type = ctk_list_box_child_type;
  klass->activate_cursor_row = ctk_list_box_activate_cursor_row;
  klass->toggle_cursor_row = ctk_list_box_toggle_cursor_row;
  klass->move_cursor = ctk_list_box_move_cursor;
  klass->select_all = ctk_list_box_select_all;
  klass->unselect_all = ctk_list_box_unselect_all;
  klass->selected_rows_changed = ctk_list_box_selected_rows_changed;

  properties[PROP_SELECTION_MODE] =
    g_param_spec_enum ("selection-mode",
                       P_("Selection mode"),
                       P_("The selection mode"),
                       CTK_TYPE_SELECTION_MODE,
                       CTK_SELECTION_SINGLE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_ACTIVATE_ON_SINGLE_CLICK] =
    g_param_spec_boolean ("activate-on-single-click",
                          P_("Activate on Single Click"),
                          P_("Activate row on a single click"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);

  /**
   * CtkListBox::row-selected:
   * @box: the #CtkListBox
   * @row: (nullable): the selected row
   *
   * The ::row-selected signal is emitted when a new row is selected, or
   * (with a %NULL @row) when the selection is cleared.
   *
   * When the @box is using #CTK_SELECTION_MULTIPLE, this signal will not
   * give you the full picture of selection changes, and you should use
   * the #CtkListBox::selected-rows-changed signal instead.
   *
   * Since: 3.10
   */
  signals[ROW_SELECTED] =
    g_signal_new (I_("row-selected"),
                  CTK_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkListBoxClass, row_selected),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_LIST_BOX_ROW);

  /**
   * CtkListBox::selected-rows-changed:
   * @box: the #CtkListBox on wich the signal is emitted
   *
   * The ::selected-rows-changed signal is emitted when the
   * set of selected rows changes.
   *
   * Since: 3.14
   */
  signals[SELECTED_ROWS_CHANGED] = g_signal_new (I_("selected-rows-changed"),
                                                 CTK_TYPE_LIST_BOX,
                                                 G_SIGNAL_RUN_FIRST,
                                                 G_STRUCT_OFFSET (CtkListBoxClass, selected_rows_changed),
                                                 NULL, NULL,
                                                 NULL,
                                                 G_TYPE_NONE, 0);

  /**
   * CtkListBox::select-all:
   * @box: the #CtkListBox on which the signal is emitted
   *
   * The ::select-all signal is a [keybinding signal][CtkBindingSignal]
   * which gets emitted to select all children of the box, if the selection
   * mode permits it.
   *
   * The default bindings for this signal is Ctrl-a.
   *
   * Since: 3.14
   */
  signals[SELECT_ALL] = g_signal_new (I_("select-all"),
                                      CTK_TYPE_LIST_BOX,
                                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                      G_STRUCT_OFFSET (CtkListBoxClass, select_all),
                                      NULL, NULL,
                                      NULL,
                                      G_TYPE_NONE, 0);

  /**
   * CtkListBox::unselect-all:
   * @box: the #CtkListBox on which the signal is emitted
   * 
   * The ::unselect-all signal is a [keybinding signal][CtkBindingSignal]
   * which gets emitted to unselect all children of the box, if the selection
   * mode permits it.
   *
   * The default bindings for this signal is Ctrl-Shift-a.
   *
   * Since: 3.14
   */
  signals[UNSELECT_ALL] = g_signal_new (I_("unselect-all"),
                                        CTK_TYPE_LIST_BOX,
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                        G_STRUCT_OFFSET (CtkListBoxClass, unselect_all),
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_NONE, 0);

  /**
   * CtkListBox::row-activated:
   * @box: the #CtkListBox
   * @row: the activated row
   *
   * The ::row-activated signal is emitted when a row has been activated by the user.
   *
   * Since: 3.10
   */
  signals[ROW_ACTIVATED] =
    g_signal_new (I_("row-activated"),
                  CTK_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkListBoxClass, row_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_LIST_BOX_ROW);
  signals[ACTIVATE_CURSOR_ROW] =
    g_signal_new (I_("activate-cursor-row"),
                  CTK_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkListBoxClass, activate_cursor_row),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  signals[TOGGLE_CURSOR_ROW] =
    g_signal_new (I_("toggle-cursor-row"),
                  CTK_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkListBoxClass, toggle_cursor_row),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
                  CTK_TYPE_LIST_BOX,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkListBoxClass, move_cursor),
                  NULL, NULL,
                  _ctk_marshal_VOID__ENUM_INT,
                  G_TYPE_NONE, 2,
                  CTK_TYPE_MOVEMENT_STEP, G_TYPE_INT);
  g_signal_set_va_marshaller (signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__ENUM_INTv);

  widget_class->activate_signal = signals[ACTIVATE_CURSOR_ROW];

  binding_set = ctk_binding_set_by_class (klass);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_Home, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, -1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_KP_Home, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, -1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_End, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, 1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_KP_End, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, 1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_Up, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, -1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_KP_Up, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, -1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_Down, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, 1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_KP_Down, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, 1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_Page_Up, 0,
                                 CTK_MOVEMENT_PAGES, -1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_KP_Page_Up, 0,
                                 CTK_MOVEMENT_PAGES, -1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_Page_Down, 0,
                                 CTK_MOVEMENT_PAGES, 1);
  ctk_list_box_add_move_binding (binding_set, CDK_KEY_KP_Page_Down, 0,
                                 CTK_MOVEMENT_PAGES, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_space, CDK_CONTROL_MASK,
                                "toggle-cursor-row", 0, NULL);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Space, CDK_CONTROL_MASK,
                                "toggle-cursor-row", 0, NULL);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_a, CDK_CONTROL_MASK,
                                "select-all", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_a, CDK_CONTROL_MASK | CDK_SHIFT_MASK,
                                "unselect-all", 0);

  ctk_widget_class_set_css_name (widget_class, "list");
}

static void
ctk_list_box_init (CtkListBox *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkWidget *widget = CTK_WIDGET (box);
  CtkCssNode *widget_node;

  ctk_widget_set_has_window (widget, TRUE);
  priv->selection_mode = CTK_SELECTION_SINGLE;
  priv->activate_single_click = TRUE;

  priv->children = g_sequence_new (NULL);
  priv->header_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

  priv->multipress_gesture = ctk_gesture_multi_press_new (widget);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->multipress_gesture),
                                              CTK_PHASE_BUBBLE);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->multipress_gesture),
                                     FALSE);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture),
                                 CDK_BUTTON_PRIMARY);
  g_signal_connect (priv->multipress_gesture, "pressed",
                    G_CALLBACK (ctk_list_box_multipress_gesture_pressed), box);
  g_signal_connect (priv->multipress_gesture, "released",
                    G_CALLBACK (ctk_list_box_multipress_gesture_released), box);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (box));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (box),
                                                     ctk_list_box_measure,
                                                     ctk_list_box_allocate,
                                                     ctk_list_box_render,
                                                     NULL,
                                                     NULL);

}

/**
 * ctk_list_box_get_selected_row:
 * @box: a #CtkListBox
 *
 * Gets the selected row.
 *
 * Note that the box may allow multiple selection, in which
 * case you should use ctk_list_box_selected_foreach() to
 * find all selected rows.
 *
 * Returns: (transfer none): the selected row
 *
 * Since: 3.10
 */
CtkListBoxRow *
ctk_list_box_get_selected_row (CtkListBox *box)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX (box), NULL);

  return BOX_PRIV (box)->selected_row;
}

/**
 * ctk_list_box_get_row_at_index:
 * @box: a #CtkListBox
 * @index_: the index of the row
 *
 * Gets the n-th child in the list (not counting headers).
 * If @_index is negative or larger than the number of items in the
 * list, %NULL is returned.
 *
 * Returns: (transfer none) (nullable): the child #CtkWidget or %NULL
 *
 * Since: 3.10
 */
CtkListBoxRow *
ctk_list_box_get_row_at_index (CtkListBox *box,
                               gint        index_)
{
  GSequenceIter *iter;

  g_return_val_if_fail (CTK_IS_LIST_BOX (box), NULL);

  iter = g_sequence_get_iter_at_pos (BOX_PRIV (box)->children, index_);
  if (!g_sequence_iter_is_end (iter))
    return g_sequence_get (iter);

  return NULL;
}

static int
row_y_cmp_func (gconstpointer a,
                gconstpointer b,
                gpointer      user_data G_GNUC_UNUSED)
{
  int y = GPOINTER_TO_INT (b);
  CtkListBoxRowPrivate *row_priv = ROW_PRIV (a);


  if (y < row_priv->y)
    return 1;
  else if (y >= row_priv->y + row_priv->height)
    return -1;

  return 0;
}

/**
 * ctk_list_box_get_row_at_y:
 * @box: a #CtkListBox
 * @y: position
 *
 * Gets the row at the @y position.
 *
 * Returns: (transfer none) (nullable): the row or %NULL
 *   in case no row exists for the given y coordinate.
 *
 * Since: 3.10
 */
CtkListBoxRow *
ctk_list_box_get_row_at_y (CtkListBox *box,
                           gint        y)
{
  GSequenceIter *iter;

  g_return_val_if_fail (CTK_IS_LIST_BOX (box), NULL);

  iter = g_sequence_lookup (BOX_PRIV (box)->children,
                            GINT_TO_POINTER (y),
                            row_y_cmp_func,
                            NULL);

  if (iter)
    return CTK_LIST_BOX_ROW (g_sequence_get (iter));

  return NULL;
}

/**
 * ctk_list_box_select_row:
 * @box: a #CtkListBox
 * @row: (allow-none): The row to select or %NULL
 *
 * Make @row the currently selected row.
 *
 * Since: 3.10
 */
void
ctk_list_box_select_row (CtkListBox    *box,
                         CtkListBoxRow *row)
{
  gboolean dirty = FALSE;

  g_return_if_fail (CTK_IS_LIST_BOX (box));
  g_return_if_fail (row == NULL || CTK_IS_LIST_BOX_ROW (row));

  if (row)
    ctk_list_box_select_row_internal (box, row);
  else
    dirty = ctk_list_box_unselect_all_internal (box);

  if (dirty)
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

/**   
 * ctk_list_box_unselect_row:
 * @box: a #CtkListBox
 * @row: the row to unselected
 *
 * Unselects a single row of @box, if the selection mode allows it.
 *
 * Since: 3.14
 */                       
void
ctk_list_box_unselect_row (CtkListBox    *box,
                           CtkListBoxRow *row)
{
  g_return_if_fail (CTK_IS_LIST_BOX (box));
  g_return_if_fail (CTK_IS_LIST_BOX_ROW (row));
  
  ctk_list_box_unselect_row_internal (box, row);
} 

/**
 * ctk_list_box_select_all:
 * @box: a #CtkListBox
 *
 * Select all children of @box, if the selection mode allows it.
 *
 * Since: 3.14
 */
void
ctk_list_box_select_all (CtkListBox *box)
{
  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (BOX_PRIV (box)->selection_mode != CTK_SELECTION_MULTIPLE)
    return;

  if (g_sequence_get_length (BOX_PRIV (box)->children) > 0)
    {
      ctk_list_box_select_all_between (box, NULL, NULL, FALSE);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

/**
 * ctk_list_box_unselect_all:
 * @box: a #CtkListBox
 *
 * Unselect all children of @box, if the selection mode allows it.
 *
 * Since: 3.14
 */
void
ctk_list_box_unselect_all (CtkListBox *box)
{
  gboolean dirty = FALSE;

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_BROWSE)
    return;

  dirty = ctk_list_box_unselect_all_internal (box);

  if (dirty)
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

static void
ctk_list_box_selected_rows_changed (CtkListBox *box)
{
  _ctk_list_box_accessible_selection_changed (box);
}

/**
 * CtkListBoxForeachFunc:
 * @box: a #CtkListBox
 * @row: a #CtkListBoxRow
 * @user_data: (closure): user data
 *
 * A function used by ctk_list_box_selected_foreach().
 * It will be called on every selected child of the @box.
 *
 * Since: 3.14
 */

/**
 * ctk_list_box_selected_foreach:
 * @box: a #CtkListBox
 * @func: (scope call): the function to call for each selected child
 * @data: user data to pass to the function
 *
 * Calls a function for each selected child.
 *
 * Note that the selection cannot be modified from within this function.
 *
 * Since: 3.14
 */
void
ctk_list_box_selected_foreach (CtkListBox            *box,
                               CtkListBoxForeachFunc  func,
                               gpointer               data)
{
  CtkListBoxRow *row;
  GSequenceIter *iter;

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (ctk_list_box_row_is_selected (row))
        (*func) (box, row, data);
    }
}

/**
 * ctk_list_box_get_selected_rows:
 * @box: a #CtkListBox
 *
 * Creates a list of all selected children.
 *
 * Returns: (element-type CtkListBoxRow) (transfer container):
 *     A #GList containing the #CtkWidget for each selected child.
 *     Free with g_list_free() when done.
 *
 * Since: 3.14
 */
GList *
ctk_list_box_get_selected_rows (CtkListBox *box)
{
  CtkListBoxRow *row;
  GSequenceIter *iter;
  GList *selected = NULL;

  g_return_val_if_fail (CTK_IS_LIST_BOX (box), NULL);

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (ctk_list_box_row_is_selected (row))
        selected = g_list_prepend (selected, row);
    }

  return g_list_reverse (selected);
}

/**
 * ctk_list_box_set_placeholder:
 * @box: a #CtkListBox
 * @placeholder: (allow-none): a #CtkWidget or %NULL
 *
 * Sets the placeholder widget that is shown in the list when
 * it doesn't display any visible children.
 *
 * Since: 3.10
 */
void
ctk_list_box_set_placeholder (CtkListBox *box,
                              CtkWidget  *placeholder)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (priv->placeholder)
    {
      ctk_widget_unparent (priv->placeholder);
      ctk_widget_queue_resize (CTK_WIDGET (box));
    }

  priv->placeholder = placeholder;

  if (placeholder)
    {
      ctk_widget_set_parent (CTK_WIDGET (placeholder), CTK_WIDGET (box));
      ctk_widget_set_child_visible (CTK_WIDGET (placeholder),
                                    priv->n_visible_rows == 0);
    }
}


/**
 * ctk_list_box_set_adjustment:
 * @box: a #CtkListBox
 * @adjustment: (allow-none): the adjustment, or %NULL
 *
 * Sets the adjustment (if any) that the widget uses to
 * for vertical scrolling. For instance, this is used
 * to get the page size for PageUp/Down key handling.
 *
 * In the normal case when the @box is packed inside
 * a #CtkScrolledWindow the adjustment from that will
 * be picked up automatically, so there is no need
 * to manually do that.
 *
 * Since: 3.10
 */
void
ctk_list_box_set_adjustment (CtkListBox    *box,
                             CtkAdjustment *adjustment)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));
  g_return_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref_sink (adjustment);
  if (priv->adjustment)
    g_object_unref (priv->adjustment);
  priv->adjustment = adjustment;
}

/**
 * ctk_list_box_get_adjustment:
 * @box: a #CtkListBox
 *
 * Gets the adjustment (if any) that the widget uses to
 * for vertical scrolling.
 *
 * Returns: (transfer none): the adjustment
 *
 * Since: 3.10
 */
CtkAdjustment *
ctk_list_box_get_adjustment (CtkListBox *box)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX (box), NULL);

  return BOX_PRIV (box)->adjustment;
}

static void
adjustment_changed (GObject    *object,
                    GParamSpec *pspec G_GNUC_UNUSED,
                    gpointer    data)
{
  CtkAdjustment *adjustment;

  adjustment = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (object));
  ctk_list_box_set_adjustment (CTK_LIST_BOX (data), adjustment);
}

static void
ctk_list_box_parent_set (CtkWidget *widget,
                         CtkWidget *prev_parent)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  if (prev_parent && CTK_IS_SCROLLABLE (prev_parent))
    g_signal_handlers_disconnect_by_func (prev_parent,
                                          G_CALLBACK (adjustment_changed), widget);

  if (parent && CTK_IS_SCROLLABLE (parent))
    {
      adjustment_changed (G_OBJECT (parent), NULL, widget);
      g_signal_connect (parent, "notify::vadjustment",
                        G_CALLBACK (adjustment_changed), widget);
    }
  else
    ctk_list_box_set_adjustment (CTK_LIST_BOX (widget), NULL);
}

/**
 * ctk_list_box_set_selection_mode:
 * @box: a #CtkListBox
 * @mode: The #CtkSelectionMode
 *
 * Sets how selection works in the listbox.
 * See #CtkSelectionMode for details.
 *
 * Since: 3.10
 */
void
ctk_list_box_set_selection_mode (CtkListBox       *box,
                                 CtkSelectionMode  mode)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  gboolean dirty = FALSE;

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (priv->selection_mode == mode)
    return;

  if (mode == CTK_SELECTION_NONE ||
      priv->selection_mode == CTK_SELECTION_MULTIPLE)
    dirty = ctk_list_box_unselect_all_internal (box);

  priv->selection_mode = mode;

  ctk_list_box_update_row_styles (box);

  g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_SELECTION_MODE]);

  if (dirty)
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

/**
 * ctk_list_box_get_selection_mode:
 * @box: a #CtkListBox
 *
 * Gets the selection mode of the listbox.
 *
 * Returns: a #CtkSelectionMode
 *
 * Since: 3.10
 */
CtkSelectionMode
ctk_list_box_get_selection_mode (CtkListBox *box)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX (box), CTK_SELECTION_NONE);

  return BOX_PRIV (box)->selection_mode;
}

/**
 * ctk_list_box_set_filter_func:
 * @box: a #CtkListBox
 * @filter_func: (closure user_data) (allow-none): callback that lets you filter which rows to show
 * @user_data: user data passed to @filter_func
 * @destroy: destroy notifier for @user_data
 *
 * By setting a filter function on the @box one can decide dynamically which
 * of the rows to show. For instance, to implement a search function on a list that
 * filters the original list to only show the matching rows.
 *
 * The @filter_func will be called for each row after the call, and it will
 * continue to be called each time a row changes (via ctk_list_box_row_changed()) or
 * when ctk_list_box_invalidate_filter() is called.
 *
 * Note that using a filter function is incompatible with using a model
 * (see ctk_list_box_bind_model()).
 *
 * Since: 3.10
 */
void
ctk_list_box_set_filter_func (CtkListBox           *box,
                              CtkListBoxFilterFunc  filter_func,
                              gpointer              user_data,
                              GDestroyNotify        destroy)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (priv->filter_func_target_destroy_notify != NULL)
    priv->filter_func_target_destroy_notify (priv->filter_func_target);

  priv->filter_func = filter_func;
  priv->filter_func_target = user_data;
  priv->filter_func_target_destroy_notify = destroy;

  ctk_list_box_check_model_compat (box);

  ctk_list_box_invalidate_filter (box);
}

/**
 * ctk_list_box_set_header_func:
 * @box: a #CtkListBox
 * @update_header: (closure user_data) (allow-none): callback that lets you add row headers
 * @user_data: user data passed to @update_header
 * @destroy: destroy notifier for @user_data
 *
 * By setting a header function on the @box one can dynamically add headers
 * in front of rows, depending on the contents of the row and its position in the list.
 * For instance, one could use it to add headers in front of the first item of a
 * new kind, in a list sorted by the kind.
 *
 * The @update_header can look at the current header widget using ctk_list_box_row_get_header()
 * and either update the state of the widget as needed, or set a new one using
 * ctk_list_box_row_set_header(). If no header is needed, set the header to %NULL.
 *
 * Note that you may get many calls @update_header to this for a particular row when e.g.
 * changing things that don’t affect the header. In this case it is important for performance
 * to not blindly replace an existing header with an identical one.
 *
 * The @update_header function will be called for each row after the call, and it will
 * continue to be called each time a row changes (via ctk_list_box_row_changed()) and when
 * the row before changes (either by ctk_list_box_row_changed() on the previous row, or when
 * the previous row becomes a different row). It is also called for all rows when
 * ctk_list_box_invalidate_headers() is called.
 *
 * Since: 3.10
 */
void
ctk_list_box_set_header_func (CtkListBox                 *box,
                              CtkListBoxUpdateHeaderFunc  update_header,
                              gpointer                    user_data,
                              GDestroyNotify              destroy)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (priv->update_header_func_target_destroy_notify != NULL)
    priv->update_header_func_target_destroy_notify (priv->update_header_func_target);

  priv->update_header_func = update_header;
  priv->update_header_func_target = user_data;
  priv->update_header_func_target_destroy_notify = destroy;
  ctk_list_box_invalidate_headers (box);
}

/**
 * ctk_list_box_invalidate_filter:
 * @box: a #CtkListBox
 *
 * Update the filtering for all rows. Call this when result
 * of the filter function on the @box is changed due
 * to an external factor. For instance, this would be used
 * if the filter function just looked for a specific search
 * string and the entry with the search string has changed.
 *
 * Since: 3.10
 */
void
ctk_list_box_invalidate_filter (CtkListBox *box)
{
  g_return_if_fail (CTK_IS_LIST_BOX (box));

  ctk_list_box_apply_filter_all (box);
  ctk_list_box_invalidate_headers (box);
  ctk_widget_queue_resize (CTK_WIDGET (box));
}

static gint
do_sort (CtkListBoxRow *a,
         CtkListBoxRow *b,
         CtkListBox    *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  return priv->sort_func (a, b, priv->sort_func_target);
}

static void
ctk_list_box_css_node_foreach (gpointer data,
                               gpointer user_data)
{
  CtkWidget **previous = user_data;
  CtkWidget *row = data;
  CtkCssNode *row_node;
  CtkCssNode *prev_node;

  if (*previous)
    {
      prev_node = ctk_widget_get_css_node (*previous);
      row_node = ctk_widget_get_css_node (row);
      ctk_css_node_insert_after (ctk_css_node_get_parent (row_node),
                                 row_node,
                                 prev_node);
    }

  *previous = row;
}

/**
 * ctk_list_box_invalidate_sort:
 * @box: a #CtkListBox
 *
 * Update the sorting for all rows. Call this when result
 * of the sort function on the @box is changed due
 * to an external factor.
 *
 * Since: 3.10
 */
void
ctk_list_box_invalidate_sort (CtkListBox *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkWidget *previous = NULL;

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (priv->sort_func == NULL)
    return;

  g_sequence_sort (priv->children, (GCompareDataFunc)do_sort, box);
  g_sequence_foreach (priv->children, ctk_list_box_css_node_foreach, &previous);

  ctk_list_box_invalidate_headers (box);
  ctk_widget_queue_resize (CTK_WIDGET (box));
}

static void
ctk_list_box_do_reseparate (CtkListBox *box)
{
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    ctk_list_box_update_header (box, iter);

  ctk_widget_queue_resize (CTK_WIDGET (box));
}


/**
 * ctk_list_box_invalidate_headers:
 * @box: a #CtkListBox
 *
 * Update the separators for all rows. Call this when result
 * of the header function on the @box is changed due
 * to an external factor.
 *
 * Since: 3.10
 */
void
ctk_list_box_invalidate_headers (CtkListBox *box)
{
  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (!ctk_widget_get_visible (CTK_WIDGET (box)))
    return;

  ctk_list_box_do_reseparate (box);
}

/**
 * ctk_list_box_set_sort_func:
 * @box: a #CtkListBox
 * @sort_func: (closure user_data) (allow-none): the sort function
 * @user_data: user data passed to @sort_func
 * @destroy: destroy notifier for @user_data
 *
 * By setting a sort function on the @box one can dynamically reorder the rows
 * of the list, based on the contents of the rows.
 *
 * The @sort_func will be called for each row after the call, and will continue to
 * be called each time a row changes (via ctk_list_box_row_changed()) and when
 * ctk_list_box_invalidate_sort() is called.
 *
 * Note that using a sort function is incompatible with using a model
 * (see ctk_list_box_bind_model()).
 *
 * Since: 3.10
 */
void
ctk_list_box_set_sort_func (CtkListBox         *box,
                            CtkListBoxSortFunc  sort_func,
                            gpointer            user_data,
                            GDestroyNotify      destroy)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (priv->sort_func_target_destroy_notify != NULL)
    priv->sort_func_target_destroy_notify (priv->sort_func_target);

  priv->sort_func = sort_func;
  priv->sort_func_target = user_data;
  priv->sort_func_target_destroy_notify = destroy;

  ctk_list_box_check_model_compat (box);

  ctk_list_box_invalidate_sort (box);
}

static void
ctk_list_box_got_row_changed (CtkListBox    *box,
                              CtkListBoxRow *row)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkListBoxRowPrivate *row_priv = ROW_PRIV (row);
  GSequenceIter *prev_next, *next;

  g_return_if_fail (CTK_IS_LIST_BOX (box));
  g_return_if_fail (CTK_IS_LIST_BOX_ROW (row));

  prev_next = ctk_list_box_get_next_visible (box, row_priv->iter);
  if (priv->sort_func != NULL)
    {
      g_sequence_sort_changed (row_priv->iter,
                               (GCompareDataFunc)do_sort,
                               box);
      ctk_widget_queue_resize (CTK_WIDGET (box));
    }
  ctk_list_box_apply_filter (box, row);
  if (ctk_widget_get_visible (CTK_WIDGET (box)))
    {
      next = ctk_list_box_get_next_visible (box, row_priv->iter);
      ctk_list_box_update_header (box, row_priv->iter);
      ctk_list_box_update_header (box, next);
      ctk_list_box_update_header (box, prev_next);
    }
}

/**
 * ctk_list_box_set_activate_on_single_click:
 * @box: a #CtkListBox
 * @single: a boolean
 *
 * If @single is %TRUE, rows will be activated when you click on them,
 * otherwise you need to double-click.
 *
 * Since: 3.10
 */
void
ctk_list_box_set_activate_on_single_click (CtkListBox *box,
                                           gboolean    single)
{
  g_return_if_fail (CTK_IS_LIST_BOX (box));

  single = single != FALSE;

  if (BOX_PRIV (box)->activate_single_click == single)
    return;

  BOX_PRIV (box)->activate_single_click = single;

  g_object_notify_by_pspec (G_OBJECT (box), properties[PROP_ACTIVATE_ON_SINGLE_CLICK]);
}

/**
 * ctk_list_box_get_activate_on_single_click:
 * @box: a #CtkListBox
 *
 * Returns whether rows activate on single clicks.
 *
 * Returns: %TRUE if rows are activated on single click, %FALSE otherwise
 *
 * Since: 3.10
 */
gboolean
ctk_list_box_get_activate_on_single_click (CtkListBox *box)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX (box), FALSE);

  return BOX_PRIV (box)->activate_single_click;
}


static void
ctk_list_box_add_move_binding (CtkBindingSet   *binding_set,
                               guint            keyval,
                               CdkModifierType  modmask,
                               CtkMovementStep  step,
                               gint             count)
{
  CdkDisplay *display;
  CdkModifierType extend_mod_mask = CDK_SHIFT_MASK;
  CdkModifierType modify_mod_mask = CDK_CONTROL_MASK;

  display = cdk_display_get_default ();
  if (display)
    {
      extend_mod_mask = cdk_keymap_get_modifier_mask (cdk_keymap_get_for_display (display),
                                                      CDK_MODIFIER_INTENT_EXTEND_SELECTION);
      modify_mod_mask = cdk_keymap_get_modifier_mask (cdk_keymap_get_for_display (display),
                                                      CDK_MODIFIER_INTENT_MODIFY_SELECTION);
    }

  ctk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | extend_mod_mask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | modify_mod_mask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | extend_mod_mask | modify_mod_mask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
}

static void
ensure_row_visible (CtkListBox    *box,
                    CtkListBoxRow *row)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkWidget *header;
  gint y, height;
  CtkAllocation allocation;

  if (!priv->adjustment)
    return;

  ctk_widget_get_allocation (CTK_WIDGET (row), &allocation);
  y = allocation.y;
  height = allocation.height;

  /* If the row has a header, we want to ensure that it is visible as well. */
  header = ROW_PRIV (row)->header;
  if (CTK_IS_WIDGET (header) && ctk_widget_is_drawable (header))
    {
      ctk_widget_get_allocation (header, &allocation);
      y = allocation.y;
      height += allocation.height;
    }

  ctk_adjustment_clamp_page (priv->adjustment, y, y + height);
}

static void
ctk_list_box_update_cursor (CtkListBox    *box,
                            CtkListBoxRow *row,
                            gboolean grab_focus)
{
  BOX_PRIV (box)->cursor_row = row;
  ensure_row_visible (box, row);
  if (grab_focus)
    ctk_widget_grab_focus (CTK_WIDGET (row));
  ctk_widget_queue_draw (CTK_WIDGET (row));
  _ctk_list_box_accessible_update_cursor (box, row);
}

static CtkListBox *
ctk_list_box_row_get_box (CtkListBoxRow *row)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (CTK_WIDGET (row));
  if (parent && CTK_IS_LIST_BOX (parent))
    return CTK_LIST_BOX (parent);

  return NULL;
}

static gboolean
row_is_visible (CtkListBoxRow *row)
{
  return ROW_PRIV (row)->visible;
}

static gboolean
ctk_list_box_row_set_selected (CtkListBoxRow *row,
                               gboolean       selected)
{
  if (!ROW_PRIV (row)->selectable)
    return FALSE;

  if (ROW_PRIV (row)->selected != selected)
    {
      ROW_PRIV (row)->selected = selected;
      if (selected)
        ctk_widget_set_state_flags (CTK_WIDGET (row),
                                    CTK_STATE_FLAG_SELECTED, FALSE);
      else
        ctk_widget_unset_state_flags (CTK_WIDGET (row),
                                      CTK_STATE_FLAG_SELECTED);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ctk_list_box_unselect_all_internal (CtkListBox *box)
{
  CtkListBoxRow *row;
  GSequenceIter *iter;
  gboolean dirty = FALSE;

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_NONE)
    return FALSE;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      dirty |= ctk_list_box_row_set_selected (row, FALSE);
    }

  BOX_PRIV (box)->selected_row = NULL;

  return dirty;
}

static void
ctk_list_box_unselect_row_internal (CtkListBox    *box,
                                    CtkListBoxRow *row)
{
  if (!ROW_PRIV (row)->selected)
    return;

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_NONE)
    return;
  else if (BOX_PRIV (box)->selection_mode != CTK_SELECTION_MULTIPLE)
    ctk_list_box_unselect_all_internal (box);
  else
    ctk_list_box_row_set_selected (row, FALSE);

  g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
  g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
}

static void
ctk_list_box_select_row_internal (CtkListBox    *box,
                                  CtkListBoxRow *row)
{
  if (!ROW_PRIV (row)->selectable)
    return;

  if (ROW_PRIV (row)->selected)
    return;

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_NONE)
    return;

  if (BOX_PRIV (box)->selection_mode != CTK_SELECTION_MULTIPLE)
    ctk_list_box_unselect_all_internal (box);

  ctk_list_box_row_set_selected (row, TRUE);
  BOX_PRIV (box)->selected_row = row;

  g_signal_emit (box, signals[ROW_SELECTED], 0, row);
  g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
}

static void
ctk_list_box_select_all_between (CtkListBox    *box,
                                 CtkListBoxRow *row1,
                                 CtkListBoxRow *row2,
                                 gboolean       modify)
{
  GSequenceIter *iter, *iter1, *iter2;

  if (row1)
    iter1 = ROW_PRIV (row1)->iter;
  else
    iter1 = g_sequence_get_begin_iter (BOX_PRIV (box)->children);

  if (row2)
    iter2 = ROW_PRIV (row2)->iter;
  else
    iter2 = g_sequence_get_end_iter (BOX_PRIV (box)->children);

  if (g_sequence_iter_compare (iter2, iter1) < 0)
    {
      iter = iter1;
      iter1 = iter2;
      iter2 = iter;
    }

  for (iter = iter1;
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkListBoxRow *row;

      row = CTK_LIST_BOX_ROW (g_sequence_get (iter));
      if (row_is_visible (row))
        {
          if (modify)
            ctk_list_box_row_set_selected (row, !ROW_PRIV (row)->selected);
          else
            ctk_list_box_row_set_selected (row, TRUE);
        }

      if (g_sequence_iter_compare (iter, iter2) == 0)
        break;
    }
}

#define ctk_list_box_update_selection(b,r,m,e) \
  ctk_list_box_update_selection_full((b), (r), (m), (e), TRUE)
static void
ctk_list_box_update_selection_full (CtkListBox    *box,
                                    CtkListBoxRow *row,
                                    gboolean       modify,
                                    gboolean       extend,
                                    gboolean       grab_cursor)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  ctk_list_box_update_cursor (box, row, grab_cursor);

  if (priv->selection_mode == CTK_SELECTION_NONE)
    return;

  if (!ROW_PRIV (row)->selectable)
    return;

  if (priv->selection_mode == CTK_SELECTION_BROWSE)
    {
      ctk_list_box_unselect_all_internal (box);
      ctk_list_box_row_set_selected (row, TRUE);
      priv->selected_row = row;
      g_signal_emit (box, signals[ROW_SELECTED], 0, row);
    }
  else if (priv->selection_mode == CTK_SELECTION_SINGLE)
    {
      gboolean was_selected;

      was_selected = ROW_PRIV (row)->selected;
      ctk_list_box_unselect_all_internal (box);
      ctk_list_box_row_set_selected (row, modify ? !was_selected : TRUE);
      priv->selected_row = ROW_PRIV (row)->selected ? row : NULL;
      g_signal_emit (box, signals[ROW_SELECTED], 0, priv->selected_row);
    }
  else /* CTK_SELECTION_MULTIPLE */
    {
      if (extend)
        {
          CtkListBoxRow *selected_row;

          selected_row = priv->selected_row;

          ctk_list_box_unselect_all_internal (box);

          if (selected_row == NULL)
            {
              ctk_list_box_row_set_selected (row, TRUE);
              priv->selected_row = row;
              g_signal_emit (box, signals[ROW_SELECTED], 0, row);
            }
          else
            ctk_list_box_select_all_between (box, selected_row, row, FALSE);
        }
      else
        {
          if (modify)
            {
              ctk_list_box_row_set_selected (row, !ROW_PRIV (row)->selected);
              g_signal_emit (box, signals[ROW_SELECTED], 0, ROW_PRIV (row)->selected ? row
                                                                                     : NULL);
            }
          else
            {
              ctk_list_box_unselect_all_internal (box);
              ctk_list_box_row_set_selected (row, !ROW_PRIV (row)->selected);
              priv->selected_row = row;
              g_signal_emit (box, signals[ROW_SELECTED], 0, row);
            }
        }
    }

  g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
}

static void
ctk_list_box_activate (CtkListBox    *box,
                       CtkListBoxRow *row)
{
  CtkListBoxRowPrivate *priv = ROW_PRIV (row);

  if (!ctk_list_box_row_get_activatable (row))
    return;

  if (priv->action_helper)
    ctk_action_helper_activate (priv->action_helper);
  else
    g_signal_emit (box, signals[ROW_ACTIVATED], 0, row);
}

#define ctk_list_box_select_and_activate(b,r) \
  ctk_list_box_select_and_activate_full ((b), (r), TRUE)
static void
ctk_list_box_select_and_activate_full (CtkListBox    *box,
                                       CtkListBoxRow *row,
                                       gboolean       grab_focus)
{
  if (row != NULL)
    {
      ctk_list_box_select_row_internal (box, row);
      ctk_list_box_update_cursor (box, row, grab_focus);
      ctk_list_box_activate (box, row);
    }
}

static void
ctk_list_box_update_prelight (CtkListBox    *box,
                              CtkListBoxRow *row)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  if (row != priv->prelight_row)
    {
      if (priv->prelight_row)
        ctk_widget_unset_state_flags (CTK_WIDGET (priv->prelight_row),
                                      CTK_STATE_FLAG_PRELIGHT);

      if (row != NULL && ctk_widget_is_sensitive (CTK_WIDGET (row)))
        {
          priv->prelight_row = row;
          ctk_widget_set_state_flags (CTK_WIDGET (priv->prelight_row),
                                      CTK_STATE_FLAG_PRELIGHT,
                                      FALSE);
        }
      else
        {
          priv->prelight_row = NULL;
        }
    }
}

static void
ctk_list_box_update_active (CtkListBox    *box,
                            CtkListBoxRow *row)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  gboolean val;

  val = priv->active_row == row;
  if (priv->active_row != NULL &&
      val != priv->active_row_active)
    {
      priv->active_row_active = val;
      if (priv->active_row_active)
        ctk_widget_set_state_flags (CTK_WIDGET (priv->active_row),
                                    CTK_STATE_FLAG_ACTIVE,
                                    FALSE);
      else
        ctk_widget_unset_state_flags (CTK_WIDGET (priv->active_row),
                                      CTK_STATE_FLAG_ACTIVE);
    }
}

static gboolean
ctk_list_box_enter_notify_event (CtkWidget        *widget,
                                 CdkEventCrossing *event)
{
  CtkListBox *box = CTK_LIST_BOX (widget);
  CtkListBoxRow *row;

  if (event->window != ctk_widget_get_window (widget))
    return FALSE;

  BOX_PRIV (box)->in_widget = TRUE;

  row = ctk_list_box_get_row_at_y (box, event->y);
  ctk_list_box_update_prelight (box, row);
  ctk_list_box_update_active (box, row);

  return FALSE;
}

static gboolean
ctk_list_box_leave_notify_event (CtkWidget        *widget,
                                 CdkEventCrossing *event)
{
  CtkListBox *box = CTK_LIST_BOX (widget);
  CtkListBoxRow *row = NULL;

  if (event->window != ctk_widget_get_window (widget))
    return FALSE;

  if (event->detail != CDK_NOTIFY_INFERIOR)
    {
      BOX_PRIV (box)->in_widget = FALSE;
      row = NULL;
    }
  else
    row = ctk_list_box_get_row_at_y (box, event->y);

  ctk_list_box_update_prelight (box, row);
  ctk_list_box_update_active (box, row);

  return FALSE;
}

static gboolean
ctk_list_box_motion_notify_event (CtkWidget      *widget,
                                  CdkEventMotion *event)
{
  CtkListBox *box = CTK_LIST_BOX (widget);
  CtkListBoxRow *row;
  CdkWindow *window, *event_window;
  gint relative_y;
  gdouble parent_y;

  if (!BOX_PRIV (box)->in_widget)
    return FALSE;

  window = ctk_widget_get_window (widget);
  event_window = event->window;
  relative_y = event->y;

  while ((event_window != NULL) && (event_window != window))
    {
      cdk_window_coords_to_parent (event_window, 0, relative_y, NULL, &parent_y);
      relative_y = parent_y;
      event_window = cdk_window_get_effective_parent (event_window);
    }

  row = ctk_list_box_get_row_at_y (box, relative_y);
  ctk_list_box_update_prelight (box, row);
  ctk_list_box_update_active (box, row);

  return FALSE;
}

static void
ctk_list_box_multipress_gesture_pressed (CtkGestureMultiPress *gesture G_GNUC_UNUSED,
                                         guint                 n_press,
                                         gdouble               x G_GNUC_UNUSED,
                                         gdouble               y,
                                         CtkListBox           *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkListBoxRow *row;

  priv->active_row = NULL;
  row = ctk_list_box_get_row_at_y (box, y);

  if (row != NULL && ctk_widget_is_sensitive (CTK_WIDGET (row)))
    {
      priv->active_row = row;
      priv->active_row_active = TRUE;
      ctk_widget_set_state_flags (CTK_WIDGET (priv->active_row),
                                  CTK_STATE_FLAG_ACTIVE,
                                  FALSE);

      if (n_press == 2 && !priv->activate_single_click)
        ctk_list_box_activate (box, row);
    }
}

static void
get_current_selection_modifiers (CtkWidget *widget,
                                 gboolean  *modify,
                                 gboolean  *extend)
{
  CdkModifierType state = 0;
  CdkModifierType mask;

  *modify = FALSE;
  *extend = FALSE;

  if (ctk_get_current_event_state (&state))
    {
      mask = ctk_widget_get_modifier_mask (widget, CDK_MODIFIER_INTENT_MODIFY_SELECTION);
      if ((state & mask) == mask)
        *modify = TRUE;
      mask = ctk_widget_get_modifier_mask (widget, CDK_MODIFIER_INTENT_EXTEND_SELECTION);
      if ((state & mask) == mask)
        *extend = TRUE;
    }
}

static void
ctk_list_box_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                          guint                 n_press,
                                          gdouble               x G_GNUC_UNUSED,
                                          gdouble               y G_GNUC_UNUSED,
                                          CtkListBox           *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  /* Take a ref to protect against reentrancy
   * (the activation may destroy the widget)
   */
  g_object_ref (box);

  if (priv->active_row != NULL && priv->active_row_active)
    {
      gboolean focus_on_click = ctk_widget_get_focus_on_click (CTK_WIDGET (priv->active_row));

      ctk_widget_unset_state_flags (CTK_WIDGET (priv->active_row),
                                    CTK_STATE_FLAG_ACTIVE);

      if (n_press == 1 && priv->activate_single_click)
        ctk_list_box_select_and_activate_full (box, priv->active_row, focus_on_click);
      else
        {
          CdkEventSequence *sequence;
          CdkInputSource source;
          const CdkEvent *event;
          gboolean modify;
          gboolean extend;

          get_current_selection_modifiers (CTK_WIDGET (box), &modify, &extend);
          /* With touch, we default to modifying the selection.
           * You can still clear the selection and start over
           * by holding Ctrl.
           */
          sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
          event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
          source = cdk_device_get_source (cdk_event_get_source_device (event));

          if (source == CDK_SOURCE_TOUCHSCREEN)
            modify = !modify;

          ctk_list_box_update_selection_full (box, priv->active_row, modify, extend, focus_on_click);
        }
    }

  priv->active_row = NULL;
  priv->active_row_active = FALSE;

  g_object_unref (box);
}

static void
ctk_list_box_show (CtkWidget *widget)
{
  ctk_list_box_do_reseparate (CTK_LIST_BOX (widget));

  CTK_WIDGET_CLASS (ctk_list_box_parent_class)->show (widget);
}

static gboolean
ctk_list_box_focus (CtkWidget        *widget,
                    CtkDirectionType  direction)
{
  CtkListBox *box = CTK_LIST_BOX (widget);
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkWidget *focus_child;
  CtkListBoxRow *next_focus_row;
  CtkWidget *row;
  CtkWidget *header;

  focus_child = ctk_container_get_focus_child ((CtkContainer *)box);

  next_focus_row = NULL;
  if (focus_child != NULL)
    {
      GSequenceIter *i;

      if (ctk_widget_child_focus (focus_child, direction))
        return TRUE;

      if (direction == CTK_DIR_UP || direction == CTK_DIR_TAB_BACKWARD)
        {
          if (CTK_IS_LIST_BOX_ROW (focus_child))
            {
              header = ROW_PRIV (CTK_LIST_BOX_ROW (focus_child))->header;
              if (header && ctk_widget_child_focus (header, direction))
                return TRUE;
            }

          if (CTK_IS_LIST_BOX_ROW (focus_child))
            row = focus_child;
          else
            row = g_hash_table_lookup (priv->header_hash, focus_child);

          if (CTK_IS_LIST_BOX_ROW (row))
            i = ctk_list_box_get_previous_visible (box, ROW_PRIV (CTK_LIST_BOX_ROW (row))->iter);
          else
            i = NULL;

          while (i != NULL)
            {
              if (ctk_widget_get_sensitive (g_sequence_get (i)))
                {
                  next_focus_row = g_sequence_get (i);
                  break;
                }

              i = ctk_list_box_get_previous_visible (box, i);
            }
        }
      else if (direction == CTK_DIR_DOWN || direction == CTK_DIR_TAB_FORWARD)
        {
          if (CTK_IS_LIST_BOX_ROW (focus_child))
            i = ctk_list_box_get_next_visible (box, ROW_PRIV (CTK_LIST_BOX_ROW (focus_child))->iter);
          else
            {
              row = g_hash_table_lookup (priv->header_hash, focus_child);
              if (CTK_IS_LIST_BOX_ROW (row))
                i = ROW_PRIV (CTK_LIST_BOX_ROW (row))->iter;
              else
                i = NULL;
            }

          while (!g_sequence_iter_is_end (i))
            {
              if (ctk_widget_get_sensitive (g_sequence_get (i)))
                {
                  next_focus_row = g_sequence_get (i);
                  break;
                }

              i = ctk_list_box_get_next_visible (box, i);
            }
        }
    }
  else
    {
      /* No current focus row */
      switch (direction)
        {
        case CTK_DIR_UP:
        case CTK_DIR_TAB_BACKWARD:
          next_focus_row = priv->selected_row;
          if (next_focus_row == NULL)
            next_focus_row = ctk_list_box_get_last_focusable (box);
          break;
        default:
          next_focus_row = priv->selected_row;
          if (next_focus_row == NULL)
            next_focus_row = ctk_list_box_get_first_focusable (box);
          break;
        }
    }

  if (next_focus_row == NULL)
    {
      if (direction == CTK_DIR_UP || direction == CTK_DIR_DOWN)
        {
          if (ctk_widget_keynav_failed (CTK_WIDGET (box), direction))
            return TRUE;
        }

      return FALSE;
    }

  if (direction == CTK_DIR_DOWN || direction == CTK_DIR_TAB_FORWARD)
    {
      header = ROW_PRIV (next_focus_row)->header;
      if (header && ctk_widget_child_focus (header, direction))
        return TRUE;
    }

  if (ctk_widget_child_focus (CTK_WIDGET (next_focus_row), direction))
    return TRUE;

  return FALSE;
}

static gboolean
ctk_list_box_draw (CtkWidget *widget,
                   cairo_t   *cr)
{
  ctk_css_gadget_draw (BOX_PRIV (widget)->gadget, cr);

  return FALSE;
}

static gboolean
ctk_list_box_render (CtkCssGadget *gadget,
                     cairo_t      *cr,
                     int           x G_GNUC_UNUSED,
                     int           y G_GNUC_UNUSED,
                     int           width G_GNUC_UNUSED,
                     int           height G_GNUC_UNUSED,
                     gpointer      data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);

  CTK_WIDGET_CLASS (ctk_list_box_parent_class)->draw (widget, cr);

  return FALSE;
}

static void
ctk_list_box_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CdkWindowAttr attributes = { 0, };
  CdkWindow *window;

  ctk_widget_get_allocation (widget, &allocation);
  ctk_widget_set_realized (widget, TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.event_mask = ctk_widget_get_events (widget) |
    CDK_ENTER_NOTIFY_MASK | CDK_LEAVE_NOTIFY_MASK | CDK_POINTER_MOTION_MASK |
    CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK;
  attributes.wclass = CDK_INPUT_OUTPUT;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, CDK_WA_X | CDK_WA_Y);
  cdk_window_set_user_data (window, (GObject*) widget);
  ctk_widget_set_window (widget, window); /* Passes ownership */
}

static void
list_box_add_visible_rows (CtkListBox *box,
                           gint        n)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  int was_zero;

  was_zero = priv->n_visible_rows == 0;
  priv->n_visible_rows += n;

  if (priv->placeholder &&
      (was_zero || priv->n_visible_rows == 0))
    ctk_widget_set_child_visible (CTK_WIDGET (priv->placeholder),
                                  priv->n_visible_rows == 0);
}

/* Children are visible if they are shown by the app (visible)
 * and not filtered out (child_visible) by the listbox
 */
static void
update_row_is_visible (CtkListBox    *box,
                       CtkListBoxRow *row)
{
  CtkListBoxRowPrivate *row_priv = ROW_PRIV (row);
  gboolean was_visible;

  was_visible = row_priv->visible;

  row_priv->visible =
    ctk_widget_get_visible (CTK_WIDGET (row)) &&
    ctk_widget_get_child_visible (CTK_WIDGET (row));

  if (was_visible && !row_priv->visible)
    list_box_add_visible_rows (box, -1);
  if (!was_visible && row_priv->visible)
    list_box_add_visible_rows (box, 1);
}

static void
ctk_list_box_apply_filter (CtkListBox    *box,
                           CtkListBoxRow *row)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  gboolean do_show;

  do_show = TRUE;
  if (priv->filter_func != NULL)
    do_show = priv->filter_func (row, priv->filter_func_target);

  ctk_widget_set_child_visible (CTK_WIDGET (row), do_show);

  update_row_is_visible (box, row);
}

static void
ctk_list_box_apply_filter_all (CtkListBox *box)
{
  CtkListBoxRow *row;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      ctk_list_box_apply_filter (box, row);
    }
}

static CtkListBoxRow *
ctk_list_box_get_first_focusable (CtkListBox *box)
{
  CtkListBoxRow *row;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
        row = g_sequence_get (iter);
        if (row_is_visible (row) && ctk_widget_is_sensitive (CTK_WIDGET (row)))
          return row;
    }

  return NULL;
}

static CtkListBoxRow *
ctk_list_box_get_last_focusable (CtkListBox *box)
{
  CtkListBoxRow *row;
  GSequenceIter *iter;

  iter = g_sequence_get_end_iter (BOX_PRIV (box)->children);
  while (!g_sequence_iter_is_begin (iter))
    {
      iter = g_sequence_iter_prev (iter);
      row = g_sequence_get (iter);
      if (row_is_visible (row) && ctk_widget_is_sensitive (CTK_WIDGET (row)))
        return row;
    }

  return NULL;
}

static GSequenceIter *
ctk_list_box_get_previous_visible (CtkListBox    *box G_GNUC_UNUSED,
                                   GSequenceIter *iter)
{
  CtkListBoxRow *row;

  if (g_sequence_iter_is_begin (iter))
    return NULL;

  do
    {
      iter = g_sequence_iter_prev (iter);
      row = g_sequence_get (iter);
      if (row_is_visible (row))
        return iter;
    }
  while (!g_sequence_iter_is_begin (iter));

  return NULL;
}

static GSequenceIter *
ctk_list_box_get_next_visible (CtkListBox    *box G_GNUC_UNUSED,
                               GSequenceIter *iter)
{
  CtkListBoxRow *row;

  if (g_sequence_iter_is_end (iter))
    return iter;

  do
    {
      iter = g_sequence_iter_next (iter);
      if (!g_sequence_iter_is_end (iter))
        {
        row = g_sequence_get (iter);
        if (row_is_visible (row))
          return iter;
        }
    }
  while (!g_sequence_iter_is_end (iter));

  return iter;
}

static GSequenceIter *
ctk_list_box_get_last_visible (CtkListBox    *box,
                               GSequenceIter *iter)
{
  GSequenceIter *next = NULL;

  if (g_sequence_iter_is_end (iter))
    return NULL;

  do
    {
      next = ctk_list_box_get_next_visible (box, iter);

      if (!g_sequence_iter_is_end (next))
        iter = next;
    }
  while (!g_sequence_iter_is_end (next));

  return iter;
}

static void
ctk_list_box_update_header (CtkListBox    *box,
                            GSequenceIter *iter)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkListBoxRow *row;
  GSequenceIter *before_iter;
  CtkListBoxRow *before_row;
  CtkWidget *old_header;

  if (iter == NULL || g_sequence_iter_is_end (iter))
    return;

  row = g_sequence_get (iter);
  g_object_ref (row);

  before_iter = ctk_list_box_get_previous_visible (box, iter);
  before_row = NULL;
  if (before_iter != NULL)
    {
      before_row = g_sequence_get (before_iter);
      if (before_row)
        g_object_ref (before_row);
    }

  if (priv->update_header_func != NULL &&
      row_is_visible (row))
    {
      old_header = ROW_PRIV (row)->header;
      if (old_header)
        g_object_ref (old_header);
      priv->update_header_func (row,
                                before_row,
                                priv->update_header_func_target);
      if (old_header != ROW_PRIV (row)->header)
        {
          if (old_header != NULL &&
              g_hash_table_lookup (priv->header_hash, old_header) == row)
            {
              /* Only unparent the @old_header if it hasn’t been re-used as the
               * header for a different row. */
              ctk_widget_unparent (old_header);
              g_hash_table_remove (priv->header_hash, old_header);
            }
          if (ROW_PRIV (row)->header != NULL)
            {
              g_hash_table_insert (priv->header_hash, ROW_PRIV (row)->header, row);
              ctk_widget_set_parent (ROW_PRIV (row)->header, CTK_WIDGET (box));
              ctk_widget_show (ROW_PRIV (row)->header);
            }
          ctk_widget_queue_resize (CTK_WIDGET (box));
        }
      if (old_header)
        g_object_unref (old_header);
    }
  else
    {
      if (ROW_PRIV (row)->header != NULL)
        {
          g_hash_table_remove (priv->header_hash, ROW_PRIV (row)->header);
          ctk_widget_unparent (ROW_PRIV (row)->header);
          ctk_list_box_row_set_header (row, NULL);
          ctk_widget_queue_resize (CTK_WIDGET (box));
        }
    }
  if (before_row)
    g_object_unref (before_row);
  g_object_unref (row);
}

static void
ctk_list_box_row_visibility_changed (CtkListBox    *box,
                                     CtkListBoxRow *row)
{
  update_row_is_visible (box, row);

  if (ctk_widget_get_visible (CTK_WIDGET (box)))
    {
      ctk_list_box_update_header (box, ROW_PRIV (row)->iter);
      ctk_list_box_update_header (box,
                                  ctk_list_box_get_next_visible (box, ROW_PRIV (row)->iter));
    }
}

static void
ctk_list_box_add (CtkContainer *container,
                  CtkWidget    *child)
{
  ctk_list_box_insert (CTK_LIST_BOX (container), child, -1);
}

static void
ctk_list_box_remove (CtkContainer *container,
                     CtkWidget    *child)
{
  CtkWidget *widget = CTK_WIDGET (container);
  CtkListBox *box = CTK_LIST_BOX (container);
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  gboolean was_visible;
  gboolean was_selected;
  CtkListBoxRow *row;
  GSequenceIter *next;

  was_visible = ctk_widget_get_visible (child);

  if (!CTK_IS_LIST_BOX_ROW (child))
    {
      row = g_hash_table_lookup (priv->header_hash, child);
      if (row != NULL)
        {
          g_hash_table_remove (priv->header_hash, child);
          g_clear_object (&ROW_PRIV (row)->header);
          ctk_widget_unparent (child);
          if (was_visible && ctk_widget_get_visible (widget))
            ctk_widget_queue_resize (widget);
        }
      else
        {
          g_warning ("Tried to remove non-child %p", child);
        }
      return;
    }

  row = CTK_LIST_BOX_ROW (child);
  if (g_sequence_iter_get_sequence (ROW_PRIV (row)->iter) != priv->children)
    {
      g_warning ("Tried to remove non-child %p", child);
      return;
    }

  was_selected = ROW_PRIV (row)->selected;

  if (ROW_PRIV (row)->visible)
    list_box_add_visible_rows (box, -1);

  if (ROW_PRIV (row)->header != NULL)
    {
      g_hash_table_remove (priv->header_hash, ROW_PRIV (row)->header);
      ctk_widget_unparent (ROW_PRIV (row)->header);
      g_clear_object (&ROW_PRIV (row)->header);
    }

  if (row == priv->selected_row)
    priv->selected_row = NULL;
  if (row == priv->prelight_row)
    {
      ctk_widget_unset_state_flags (CTK_WIDGET (row), CTK_STATE_FLAG_PRELIGHT);
      priv->prelight_row = NULL;
    }
  if (row == priv->cursor_row)
    priv->cursor_row = NULL;
  if (row == priv->active_row)
    {
      ctk_widget_unset_state_flags (CTK_WIDGET (row), CTK_STATE_FLAG_ACTIVE);
      priv->active_row = NULL;
    }

  if (row == priv->drag_highlighted_row)
    ctk_list_box_drag_unhighlight_row (box);

  next = ctk_list_box_get_next_visible (box, ROW_PRIV (row)->iter);
  ctk_widget_unparent (child);
  g_sequence_remove (ROW_PRIV (row)->iter);
  if (ctk_widget_get_visible (widget))
    ctk_list_box_update_header (box, next);

  if (was_visible && ctk_widget_get_visible (CTK_WIDGET (box)))
    ctk_widget_queue_resize (widget);

  if (was_selected && !ctk_widget_in_destruction (widget))
    {
      g_signal_emit (box, signals[ROW_SELECTED], 0, NULL);
      g_signal_emit (box, signals[SELECTED_ROWS_CHANGED], 0);
    }
}

static void
ctk_list_box_forall (CtkContainer *container,
                     gboolean      include_internals,
                     CtkCallback   callback,
                     gpointer      callback_target)
{
  CtkListBoxPrivate *priv = BOX_PRIV (container);
  GSequenceIter *iter;
  CtkListBoxRow *row;

  if (priv->placeholder != NULL && include_internals)
    callback (priv->placeholder, callback_target);

  iter = g_sequence_get_begin_iter (priv->children);
  while (!g_sequence_iter_is_end (iter))
    {
      row = g_sequence_get (iter);
      iter = g_sequence_iter_next (iter);
      if (ROW_PRIV (row)->header != NULL && include_internals)
        callback (ROW_PRIV (row)->header, callback_target);
      callback (CTK_WIDGET (row), callback_target);
    }
}

static void
ctk_list_box_compute_expand (CtkWidget *widget,
                             gboolean  *hexpand,
                             gboolean  *vexpand)
{
  CTK_WIDGET_CLASS (ctk_list_box_parent_class)->compute_expand (widget,
                                                                hexpand, vexpand);

  /* We don't expand vertically beyound the minimum size */
  if (vexpand)
    *vexpand = FALSE;
}

static GType
ctk_list_box_child_type (CtkContainer *container G_GNUC_UNUSED)
{
  /* We really support any type but we wrap it in a row. But that is
   * more like a C helper function, in an abstract sense we only support
   * row children, so that is what tools accessing this should use.
   */
  return CTK_TYPE_LIST_BOX_ROW;
}

static CtkSizeRequestMode
ctk_list_box_get_request_mode (CtkWidget *widget G_GNUC_UNUSED)
{
  return CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
ctk_list_box_get_preferred_height (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_get_preferred_height_for_width (CtkWidget *widget,
                                             gint       width,
                                             gint      *minimum,
                                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_get_preferred_width (CtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_get_preferred_width_for_height (CtkWidget *widget,
                                             gint       height,
                                             gint      *minimum,
                                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_measure (CtkCssGadget   *gadget,
                      CtkOrientation  orientation,
                      gint            for_size,
                      gint           *minimum,
                      gint           *natural,
                      gint           *minimum_baseline G_GNUC_UNUSED,
                      gint           *natural_baseline G_GNUC_UNUSED,
                      gpointer        unused G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkListBoxPrivate *priv = BOX_PRIV (widget);
  GSequenceIter *iter;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      *minimum = 0;
      *natural = 0;

      if (priv->placeholder && ctk_widget_get_child_visible (priv->placeholder))
        ctk_widget_get_preferred_width (priv->placeholder, minimum, natural);

      for (iter = g_sequence_get_begin_iter (priv->children);
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          CtkListBoxRow *row;
          gint row_min;
          gint row_nat;

          row = g_sequence_get (iter);

          /* We *do* take visible but filtered rows into account here so that
           * the list width doesn't change during filtering
           */
          if (!ctk_widget_get_visible (CTK_WIDGET (row)))
            continue;

          ctk_widget_get_preferred_width (CTK_WIDGET (row), &row_min, &row_nat);
          *minimum = MAX (*minimum, row_min);
          *natural = MAX (*natural, row_nat);

          if (ROW_PRIV (row)->header != NULL)
            {
              ctk_widget_get_preferred_width (ROW_PRIV (row)->header, &row_min, &row_nat);
              *minimum = MAX (*minimum, row_min);
              *natural = MAX (*natural, row_nat);
            }
        }
    }
  else
    {
      if (for_size < 0)
        ctk_css_gadget_get_preferred_size (priv->gadget,
                                           CTK_ORIENTATION_HORIZONTAL,
                                           -1,
                                           NULL, &for_size,
                                           NULL, NULL);

      *minimum = 0;

      if (priv->placeholder && ctk_widget_get_child_visible (priv->placeholder))
        ctk_widget_get_preferred_height_for_width (priv->placeholder, for_size,
                                                   minimum, NULL);

      for (iter = g_sequence_get_begin_iter (priv->children);
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          CtkListBoxRow *row;
          gint row_min = 0;

          row = g_sequence_get (iter);
          if (!row_is_visible (row))
            continue;

          if (ROW_PRIV (row)->header != NULL)
            {
              ctk_widget_get_preferred_height_for_width (ROW_PRIV (row)->header, for_size, &row_min, NULL);
              *minimum += row_min;
            }
          ctk_widget_get_preferred_height_for_width (CTK_WIDGET (row), for_size, &row_min, NULL);
          *minimum += row_min;
        }

      /* We always allocate the minimum height, since handling expanding rows
       * is way too costly, and unlikely to be used, as lists are generally put
       * inside a scrolling window anyway.
       */
      *natural = *minimum;
    }
}

static void
ctk_list_box_size_allocate (CtkWidget     *widget,
                            CtkAllocation *allocation)
{
  CtkAllocation clip;
  CdkWindow *window;
  CtkAllocation child_allocation;

  ctk_widget_set_allocation (widget, allocation);

  window = ctk_widget_get_window (widget);
  if (window != NULL)
    cdk_window_move_resize (window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = allocation->width;
  child_allocation.height = allocation->height;

  ctk_css_gadget_allocate (BOX_PRIV (widget)->gadget,
                           &child_allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  _ctk_widget_set_simple_clip (widget, &clip);
}


static void
ctk_list_box_allocate (CtkCssGadget        *gadget,
                       const CtkAllocation *allocation,
                       int                  baseline G_GNUC_UNUSED,
                       CtkAllocation       *out_clip,
                       gpointer             unused G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkListBoxPrivate *priv = BOX_PRIV (widget);
  CtkAllocation child_allocation;
  CtkAllocation header_allocation;
  CtkListBoxRow *row;
  GSequenceIter *iter;
  int child_min;

  child_allocation.x = allocation->x;
  child_allocation.y = allocation->y;
  child_allocation.width = allocation->width;
  child_allocation.height = 0;

  header_allocation.x = allocation->x;
  header_allocation.y = allocation->y;
  header_allocation.width = allocation->width;
  header_allocation.height = 0;

  if (priv->placeholder && ctk_widget_get_child_visible (priv->placeholder))
    {
      ctk_widget_get_preferred_height_for_width (priv->placeholder,
                                                 allocation->width, &child_min, NULL);
      header_allocation.height = allocation->height;
      header_allocation.y = child_allocation.y;
      ctk_widget_size_allocate (priv->placeholder, &header_allocation);
      child_allocation.y += child_min;
    }

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      if (!row_is_visible (row))
        {
          ROW_PRIV (row)->y = child_allocation.y;
          ROW_PRIV (row)->height = 0;
          continue;
        }

      if (ROW_PRIV (row)->header != NULL)
        {
          ctk_widget_get_preferred_height_for_width (ROW_PRIV (row)->header,
                                                     allocation->width, &child_min, NULL);
          header_allocation.height = child_min;
          header_allocation.y = child_allocation.y;
          ctk_widget_size_allocate (ROW_PRIV (row)->header, &header_allocation);
          child_allocation.y += child_min;
        }

      ROW_PRIV (row)->y = child_allocation.y;

      ctk_widget_get_preferred_height_for_width (CTK_WIDGET (row),
                                                 child_allocation.width, &child_min, NULL);
      child_allocation.height = child_min;

      ROW_PRIV (row)->height = child_allocation.height;
      ctk_widget_size_allocate (CTK_WIDGET (row), &child_allocation);
      child_allocation.y += child_min;
    }

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

/**
 * ctk_list_box_prepend:
 * @box: a #CtkListBox
 * @child: the #CtkWidget to add
 *
 * Prepend a widget to the list. If a sort function is set, the widget will
 * actually be inserted at the calculated position and this function has the
 * same effect of ctk_container_add().
 *
 * Since: 3.10
 */
void
ctk_list_box_prepend (CtkListBox *box,
                      CtkWidget  *child)
{
  ctk_list_box_insert (box, child, 0);
}

static void
ctk_list_box_insert_css_node (CtkListBox    *box,
                              CtkWidget     *child,
                              GSequenceIter *iter)
{
  GSequenceIter *prev_iter;
  CtkCssNode *sibling;

  prev_iter = g_sequence_iter_prev (iter);

  if (prev_iter != iter)
    sibling = ctk_widget_get_css_node (g_sequence_get (prev_iter));
  else
    sibling = NULL;

  ctk_css_node_insert_after (ctk_widget_get_css_node (CTK_WIDGET (box)),
                             ctk_widget_get_css_node (child),
                             sibling);
}

/**
 * ctk_list_box_insert:
 * @box: a #CtkListBox
 * @child: the #CtkWidget to add
 * @position: the position to insert @child in
 *
 * Insert the @child into the @box at @position. If a sort function is
 * set, the widget will actually be inserted at the calculated position and
 * this function has the same effect of ctk_container_add().
 *
 * If @position is -1, or larger than the total number of items in the
 * @box, then the @child will be appended to the end.
 *
 * Since: 3.10
 */
void
ctk_list_box_insert (CtkListBox *box,
                     CtkWidget  *child,
                     gint        position)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  CtkListBoxRow *row;
  GSequenceIter *iter = NULL;

  g_return_if_fail (CTK_IS_LIST_BOX (box));
  g_return_if_fail (CTK_IS_WIDGET (child));

  if (CTK_IS_LIST_BOX_ROW (child))
    row = CTK_LIST_BOX_ROW (child);
  else
    {
      row = CTK_LIST_BOX_ROW (ctk_list_box_row_new ());
      ctk_widget_show (CTK_WIDGET (row));
      ctk_container_add (CTK_CONTAINER (row), child);
    }

  if (priv->sort_func != NULL)
    iter = g_sequence_insert_sorted (priv->children, row,
                                     (GCompareDataFunc)do_sort, box);
  else if (position == 0)
    iter = g_sequence_prepend (priv->children, row);
  else if (position == -1)
    iter = g_sequence_append (priv->children, row);
  else
    {
      GSequenceIter *current_iter;

      current_iter = g_sequence_get_iter_at_pos (priv->children, position);
      iter = g_sequence_insert_before (current_iter, row);
    }

  ctk_list_box_insert_css_node (box, CTK_WIDGET (row), iter);

  ROW_PRIV (row)->iter = iter;
  ctk_widget_set_parent (CTK_WIDGET (row), CTK_WIDGET (box));
  ctk_widget_set_child_visible (CTK_WIDGET (row), TRUE);
  ROW_PRIV (row)->visible = ctk_widget_get_visible (CTK_WIDGET (row));
  if (ROW_PRIV (row)->visible)
    list_box_add_visible_rows (box, 1);
  ctk_list_box_apply_filter (box, row);
  ctk_list_box_update_row_style (box, row);
  if (ctk_widget_get_visible (CTK_WIDGET (box)))
    {
      ctk_list_box_update_header (box, ROW_PRIV (row)->iter);
      ctk_list_box_update_header (box,
                                  ctk_list_box_get_next_visible (box, ROW_PRIV (row)->iter));
    }
}

/**
 * ctk_list_box_drag_unhighlight_row:
 * @box: a #CtkListBox
 *
 * If a row has previously been highlighted via ctk_list_box_drag_highlight_row()
 * it will have the highlight removed.
 *
 * Since: 3.10
 */
void
ctk_list_box_drag_unhighlight_row (CtkListBox *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));

  if (priv->drag_highlighted_row == NULL)
    return;

  ctk_drag_unhighlight (CTK_WIDGET (priv->drag_highlighted_row));
  g_clear_object (&priv->drag_highlighted_row);
}

/**
 * ctk_list_box_drag_highlight_row:
 * @box: a #CtkListBox
 * @row: a #CtkListBoxRow
 *
 * This is a helper function for implementing DnD onto a #CtkListBox.
 * The passed in @row will be highlighted via ctk_drag_highlight(),
 * and any previously highlighted row will be unhighlighted.
 *
 * The row will also be unhighlighted when the widget gets
 * a drag leave event.
 *
 * Since: 3.10
 */
void
ctk_list_box_drag_highlight_row (CtkListBox    *box,
                                 CtkListBoxRow *row)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));
  g_return_if_fail (CTK_IS_LIST_BOX_ROW (row));

  if (priv->drag_highlighted_row == row)
    return;

  ctk_list_box_drag_unhighlight_row (box);
  ctk_drag_highlight (CTK_WIDGET (row));
  priv->drag_highlighted_row = g_object_ref (row);
}

static void
ctk_list_box_drag_leave (CtkWidget      *widget,
                         CdkDragContext *context G_GNUC_UNUSED,
                         guint           time_ G_GNUC_UNUSED)
{
  ctk_list_box_drag_unhighlight_row (CTK_LIST_BOX (widget));
}

static void
ctk_list_box_activate_cursor_row (CtkListBox *box)
{
  ctk_list_box_select_and_activate (box, BOX_PRIV (box)->cursor_row);
}

static void
ctk_list_box_toggle_cursor_row (CtkListBox *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  if (priv->cursor_row == NULL)
    return;

  if ((priv->selection_mode == CTK_SELECTION_SINGLE ||
       priv->selection_mode == CTK_SELECTION_MULTIPLE) &&
      ROW_PRIV (priv->cursor_row)->selected)
    ctk_list_box_unselect_row_internal (box, priv->cursor_row);
  else
    ctk_list_box_select_and_activate (box, priv->cursor_row);
}

static void
ctk_list_box_move_cursor (CtkListBox      *box,
                          CtkMovementStep  step,
                          gint             count)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);
  gboolean modify;
  gboolean extend;
  CtkListBoxRow *row;
  gint page_size;
  GSequenceIter *iter;
  gint start_y;
  gint end_y;
  int height;

  row = NULL;
  switch (step)
    {
    case CTK_MOVEMENT_BUFFER_ENDS:
      if (count < 0)
        row = ctk_list_box_get_first_focusable (box);
      else
        row = ctk_list_box_get_last_focusable (box);
      break;
    case CTK_MOVEMENT_DISPLAY_LINES:
      if (priv->cursor_row != NULL)
        {
          gint i = count;

          iter = ROW_PRIV (priv->cursor_row)->iter;

          while (i < 0  && iter != NULL)
            {
              iter = ctk_list_box_get_previous_visible (box, iter);
              i = i + 1;
            }
          while (i > 0  && iter != NULL)
            {
              iter = ctk_list_box_get_next_visible (box, iter);
              i = i - 1;
            }

          if (iter != NULL && !g_sequence_iter_is_end (iter))
            row = g_sequence_get (iter);
        }
      break;
    case CTK_MOVEMENT_PAGES:
      page_size = 100;
      if (priv->adjustment != NULL)
        page_size = ctk_adjustment_get_page_increment (priv->adjustment);

      if (priv->cursor_row != NULL)
        {
          start_y = ROW_PRIV (priv->cursor_row)->y;
          height = ctk_widget_get_allocated_height (CTK_WIDGET (box));
          end_y = CLAMP (start_y + page_size * count, 0, height - 1);
          row = ctk_list_box_get_row_at_y (box, end_y);

          if (!row)
            {
              GSequenceIter *cursor_iter;
              GSequenceIter *next_iter;

              if (count > 0)
                {
                  cursor_iter = ROW_PRIV (priv->cursor_row)->iter;
                  next_iter = ctk_list_box_get_last_visible (box, cursor_iter);

                  if (next_iter)
                    {
                      row = g_sequence_get (next_iter);
                      end_y = ROW_PRIV (row)->y;
                    }
                }
              else
                {
                  row = ctk_list_box_get_row_at_index (box, 0);
                  end_y = ROW_PRIV (row)->y;
                }
            }
          else if (row == priv->cursor_row)
            {
              iter = ROW_PRIV (row)->iter;

              /* Move at least one row. This is important when the cursor_row's height is
               * greater than page_size */
              if (count < 0)
                iter = g_sequence_iter_prev (iter);
              else
                iter = g_sequence_iter_next (iter);

              if (!g_sequence_iter_is_begin (iter) && !g_sequence_iter_is_end (iter))
                {
                  row = g_sequence_get (iter);
                  end_y = ROW_PRIV (row)->y;
                }
            }

          if (end_y != start_y && priv->adjustment != NULL)
            ctk_adjustment_animate_to_value (priv->adjustment, end_y);
        }
      break;
    default:
      return;
    }

  if (row == NULL || row == priv->cursor_row)
    {
      CtkDirectionType direction = count < 0 ? CTK_DIR_UP : CTK_DIR_DOWN;

      if (!ctk_widget_keynav_failed (CTK_WIDGET (box), direction))
        {
          CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (box));

          if (toplevel)
            ctk_widget_child_focus (toplevel,
                                    direction == CTK_DIR_UP ?
                                    CTK_DIR_TAB_BACKWARD :
                                    CTK_DIR_TAB_FORWARD);

        }

      return;
    }

  get_current_selection_modifiers (CTK_WIDGET (box), &modify, &extend);

  ctk_list_box_update_cursor (box, row, TRUE);
  if (!modify)
    ctk_list_box_update_selection (box, row, FALSE, extend);
}


/**
 * ctk_list_box_row_new:
 *
 * Creates a new #CtkListBoxRow, to be used as a child of a #CtkListBox.
 *
 * Returns: a new #CtkListBoxRow
 *
 * Since: 3.10
 */
CtkWidget *
ctk_list_box_row_new (void)
{
  return g_object_new (CTK_TYPE_LIST_BOX_ROW, NULL);
}

static void
ctk_list_box_row_set_focus (CtkListBoxRow *row)
{
  CtkListBox *box = ctk_list_box_row_get_box (row);
  gboolean modify;
  gboolean extend;

  if (!box)
    return;

  get_current_selection_modifiers (CTK_WIDGET (row), &modify, &extend);

  if (modify)
    ctk_list_box_update_cursor (box, row, TRUE);
  else
    ctk_list_box_update_selection (box, row, FALSE, FALSE);
}

static gboolean
ctk_list_box_row_focus (CtkWidget        *widget,
                        CtkDirectionType  direction)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (widget);
  gboolean had_focus = FALSE;
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (widget));

  g_object_get (widget, "has-focus", &had_focus, NULL);
  if (had_focus)
    {
      /* If on row, going right, enter into possible container */
      if (child &&
          (direction == CTK_DIR_RIGHT || direction == CTK_DIR_TAB_FORWARD))
        {
          if (ctk_widget_child_focus (CTK_WIDGET (child), direction))
            return TRUE;
        }

      return FALSE;
    }
  else if (ctk_container_get_focus_child (CTK_CONTAINER (row)) != NULL)
    {
      /* Child has focus, always navigate inside it first */
      if (ctk_widget_child_focus (child, direction))
        return TRUE;

      /* If exiting child container to the left, select row  */
      if (direction == CTK_DIR_LEFT || direction == CTK_DIR_TAB_BACKWARD)
        {
          ctk_list_box_row_set_focus (row);
          return TRUE;
        }

      return FALSE;
    }
  else
    {
      /* If coming from the left, enter into possible container */
      if (child &&
          (direction == CTK_DIR_LEFT || direction == CTK_DIR_TAB_BACKWARD))
        {
          if (ctk_widget_child_focus (child, direction))
            return TRUE;
        }

      ctk_list_box_row_set_focus (row);
      return TRUE;
    }
}

static void
ctk_list_box_row_activate (CtkListBoxRow *row)
{
  CtkListBox *box;

  box = ctk_list_box_row_get_box (row);
  if (box)
    ctk_list_box_select_and_activate (box, row);
}

static void
ctk_list_box_row_show (CtkWidget *widget)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (widget);
  CtkListBox *box;

  CTK_WIDGET_CLASS (ctk_list_box_row_parent_class)->show (widget);

  box = ctk_list_box_row_get_box (row);
  if (box)
    ctk_list_box_row_visibility_changed (box, row);
}

static void
ctk_list_box_row_hide (CtkWidget *widget)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (widget);
  CtkListBox *box;

  CTK_WIDGET_CLASS (ctk_list_box_row_parent_class)->hide (widget);

  box = ctk_list_box_row_get_box (row);
  if (box)
    ctk_list_box_row_visibility_changed (box, row);
}

static gboolean
ctk_list_box_row_draw (CtkWidget *widget,
                       cairo_t   *cr)
{
  ctk_css_gadget_draw (ROW_PRIV (CTK_LIST_BOX_ROW (widget))->gadget, cr);

  return CDK_EVENT_PROPAGATE;
}

static gboolean
ctk_list_box_row_render (CtkCssGadget *gadget,
                         cairo_t      *cr,
                         int           x G_GNUC_UNUSED,
                         int           y G_GNUC_UNUSED,
                         int           width G_GNUC_UNUSED,
                         int           height G_GNUC_UNUSED,
                         gpointer      data G_GNUC_UNUSED)
{
  CtkWidget *widget;

  widget = ctk_css_gadget_get_owner (gadget);

  CTK_WIDGET_CLASS (ctk_list_box_row_parent_class)->draw (widget, cr);

  return ctk_widget_has_visible_focus (widget);
}

static void
ctk_list_box_row_measure (CtkCssGadget   *gadget,
                          CtkOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline G_GNUC_UNUSED,
                          int            *natural_baseline G_GNUC_UNUSED,
                          gpointer        data G_GNUC_UNUSED)
{
  CtkWidget *widget;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);
  child = ctk_bin_get_child (CTK_BIN (widget));

  if (orientation == CTK_ORIENTATION_VERTICAL)
    {
      if (child && ctk_widget_get_visible (child))
        {
          if (for_size < 0)
            ctk_widget_get_preferred_height (child, minimum, natural);
          else
            ctk_widget_get_preferred_height_for_width (child, for_size, minimum, natural);
        }
      else
        *minimum = *natural = 0;
    }
  else
    {
      if (child && ctk_widget_get_visible (child))
        ctk_widget_get_preferred_width (child, minimum, natural);
      else
        *minimum = *natural = 0;
    }
}

static void
ctk_list_box_row_get_preferred_height (CtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  ctk_css_gadget_get_preferred_size (ROW_PRIV (CTK_LIST_BOX_ROW (widget))->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_row_get_preferred_height_for_width (CtkWidget *widget,
                                                 gint       width,
                                                 gint      *minimum,
                                                 gint      *natural)
{
  ctk_css_gadget_get_preferred_size (ROW_PRIV (CTK_LIST_BOX_ROW (widget))->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_row_get_preferred_width (CtkWidget *widget,
                                      gint      *minimum,
                                      gint      *natural)
{
  ctk_css_gadget_get_preferred_size (ROW_PRIV (CTK_LIST_BOX_ROW (widget))->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_row_get_preferred_width_for_height (CtkWidget *widget,
                                                 gint       height,
                                                 gint      *minimum,
                                                 gint      *natural)
{
  ctk_css_gadget_get_preferred_size (ROW_PRIV (CTK_LIST_BOX_ROW (widget))->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_list_box_row_size_allocate (CtkWidget     *widget,
                                CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (ROW_PRIV (CTK_LIST_BOX_ROW (widget))->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_list_box_row_allocate (CtkCssGadget        *gadget,
                           const CtkAllocation *allocation,
                           int                  baseline G_GNUC_UNUSED,
                           CtkAllocation       *out_clip,
                           gpointer             data G_GNUC_UNUSED)
{
  CtkWidget *widget;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    ctk_widget_size_allocate (child, (CtkAllocation *)allocation);

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

/**
 * ctk_list_box_row_changed:
 * @row: a #CtkListBoxRow
 *
 * Marks @row as changed, causing any state that depends on this
 * to be updated. This affects sorting, filtering and headers.
 *
 * Note that calls to this method must be in sync with the data
 * used for the row functions. For instance, if the list is
 * mirroring some external data set, and *two* rows changed in the
 * external data set then when you call ctk_list_box_row_changed()
 * on the first row the sort function must only read the new data
 * for the first of the two changed rows, otherwise the resorting
 * of the rows will be wrong.
 *
 * This generally means that if you don’t fully control the data
 * model you have to duplicate the data that affects the listbox
 * row functions into the row widgets themselves. Another alternative
 * is to call ctk_list_box_invalidate_sort() on any model change,
 * but that is more expensive.
 *
 * Since: 3.10
 */
void
ctk_list_box_row_changed (CtkListBoxRow *row)
{
  CtkListBox *box;

  g_return_if_fail (CTK_IS_LIST_BOX_ROW (row));

  box = ctk_list_box_row_get_box (row);
  if (box)
    ctk_list_box_got_row_changed (box, row);
}

/**
 * ctk_list_box_row_get_header:
 * @row: a #CtkListBoxRow
 *
 * Returns the current header of the @row. This can be used
 * in a #CtkListBoxUpdateHeaderFunc to see if there is a header
 * set already, and if so to update the state of it.
 *
 * Returns: (transfer none) (nullable): the current header, or %NULL if none
 *
 * Since: 3.10
 */
CtkWidget *
ctk_list_box_row_get_header (CtkListBoxRow *row)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX_ROW (row), NULL);

  return ROW_PRIV (row)->header;
}

/**
 * ctk_list_box_row_set_header:
 * @row: a #CtkListBoxRow
 * @header: (allow-none): the header, or %NULL
 *
 * Sets the current header of the @row. This is only allowed to be called
 * from a #CtkListBoxUpdateHeaderFunc. It will replace any existing
 * header in the row, and be shown in front of the row in the listbox.
 *
 * Since: 3.10
 */
void
ctk_list_box_row_set_header (CtkListBoxRow *row,
                             CtkWidget     *header)
{
  CtkListBoxRowPrivate *priv;

  g_return_if_fail (CTK_IS_LIST_BOX_ROW (row));
  g_return_if_fail (header == NULL || CTK_IS_WIDGET (header));

  priv = ROW_PRIV (row);

  if (priv->header)
    g_object_unref (priv->header);

  priv->header = header;

  if (header)
    g_object_ref_sink (header);
}

/**
 * ctk_list_box_row_get_index:
 * @row: a #CtkListBoxRow
 *
 * Gets the current index of the @row in its #CtkListBox container.
 *
 * Returns: the index of the @row, or -1 if the @row is not in a listbox
 *
 * Since: 3.10
 */
gint
ctk_list_box_row_get_index (CtkListBoxRow *row)
{
  CtkListBoxRowPrivate *priv;

  g_return_val_if_fail (CTK_IS_LIST_BOX_ROW (row), -1);

  priv = ROW_PRIV (row);

  if (priv->iter != NULL)
    return g_sequence_iter_get_position (priv->iter);

  return -1;
}

/**
 * ctk_list_box_row_is_selected:
 * @row: a #CtkListBoxRow
 *
 * Returns whether the child is currently selected in its
 * #CtkListBox container.
 *
 * Returns: %TRUE if @row is selected
 *
 * Since: 3.14
 */
gboolean
ctk_list_box_row_is_selected (CtkListBoxRow *row)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX_ROW (row), FALSE);

  return ROW_PRIV (row)->selected;
}

static void
ctk_list_box_update_row_style (CtkListBox    *box,
                               CtkListBoxRow *row)
{
  CtkStyleContext *context;
  gboolean can_select;

  if (box && BOX_PRIV (box)->selection_mode != CTK_SELECTION_NONE)
    can_select = TRUE;
  else
    can_select = FALSE;

  context = ctk_widget_get_style_context (CTK_WIDGET (row));
  if (ROW_PRIV (row)->activatable ||
      (ROW_PRIV (row)->selectable && can_select))
    ctk_style_context_add_class (context, "activatable");
  else
    ctk_style_context_remove_class (context, "activatable");
}

static void
ctk_list_box_update_row_styles (CtkListBox *box)
{
  GSequenceIter *iter;
  CtkListBoxRow *row;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      row = g_sequence_get (iter);
      ctk_list_box_update_row_style (box, row);
    }
}

/**
 * ctk_list_box_row_set_activatable:
 * @row: a #CtkListBoxRow
 * @activatable: %TRUE to mark the row as activatable
 *
 * Set the #CtkListBoxRow:activatable property for this row.
 *
 * Since: 3.14
 */
void
ctk_list_box_row_set_activatable (CtkListBoxRow *row,
                                  gboolean       activatable)
{
  g_return_if_fail (CTK_IS_LIST_BOX_ROW (row));

  activatable = activatable != FALSE;

  if (ROW_PRIV (row)->activatable != activatable)
    {
      ROW_PRIV (row)->activatable = activatable;

      ctk_list_box_update_row_style (ctk_list_box_row_get_box (row), row);
      g_object_notify_by_pspec (G_OBJECT (row), row_properties[ROW_PROP_ACTIVATABLE]);
    }
}

/**
 * ctk_list_box_row_get_activatable:
 * @row: a #CtkListBoxRow
 *
 * Gets the value of the #CtkListBoxRow:activatable property
 * for this row.
 *
 * Returns: %TRUE if the row is activatable
 *
 * Since: 3.14
 */
gboolean
ctk_list_box_row_get_activatable (CtkListBoxRow *row)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX_ROW (row), TRUE);

  return ROW_PRIV (row)->activatable;
}

/**
 * ctk_list_box_row_set_selectable:
 * @row: a #CtkListBoxRow
 * @selectable: %TRUE to mark the row as selectable
 *
 * Set the #CtkListBoxRow:selectable property for this row.
 *
 * Since: 3.14
 */
void
ctk_list_box_row_set_selectable (CtkListBoxRow *row,
                                 gboolean       selectable)
{
  g_return_if_fail (CTK_IS_LIST_BOX_ROW (row));

  selectable = selectable != FALSE;

  if (ROW_PRIV (row)->selectable != selectable)
    {
      if (!selectable)
        ctk_list_box_row_set_selected (row, FALSE);
 
      ROW_PRIV (row)->selectable = selectable;

      ctk_list_box_update_row_style (ctk_list_box_row_get_box (row), row);
      g_object_notify_by_pspec (G_OBJECT (row), row_properties[ROW_PROP_SELECTABLE]);
    }
}

/**
 * ctk_list_box_row_get_selectable:
 * @row: a #CtkListBoxRow
 *
 * Gets the value of the #CtkListBoxRow:selectable property
 * for this row.
 *
 * Returns: %TRUE if the row is selectable
 *
 * Since: 3.14
 */
gboolean
ctk_list_box_row_get_selectable (CtkListBoxRow *row)
{
  g_return_val_if_fail (CTK_IS_LIST_BOX_ROW (row), TRUE);

  return ROW_PRIV (row)->selectable;
}

static void
ctk_list_box_row_set_action_name (CtkActionable *actionable,
                                  const gchar   *action_name)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (actionable);
  CtkListBoxRowPrivate *priv = ROW_PRIV (row);

  if (!priv->action_helper)
    priv->action_helper = ctk_action_helper_new (actionable);

  ctk_action_helper_set_action_name (priv->action_helper, action_name);
}

static void
ctk_list_box_row_set_action_target_value (CtkActionable *actionable,
                                          GVariant      *action_target)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (actionable);
  CtkListBoxRowPrivate *priv = ROW_PRIV (row);

  if (!priv->action_helper)
    priv->action_helper = ctk_action_helper_new (actionable);

  ctk_action_helper_set_action_target_value (priv->action_helper, action_target);
}

static void
ctk_list_box_row_get_property (GObject    *obj,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (obj);

  switch (property_id)
    {
    case ROW_PROP_ACTIVATABLE:
      g_value_set_boolean (value, ctk_list_box_row_get_activatable (row));
      break;
    case ROW_PROP_SELECTABLE:
      g_value_set_boolean (value, ctk_list_box_row_get_selectable (row));
      break;
    case ROW_PROP_ACTION_NAME:
      g_value_set_string (value, ctk_action_helper_get_action_name (ROW_PRIV (row)->action_helper));
      break;
    case ROW_PROP_ACTION_TARGET:
      g_value_set_variant (value, ctk_action_helper_get_action_target_value (ROW_PRIV (row)->action_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_list_box_row_set_property (GObject      *obj,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (obj);

  switch (property_id)
    {
    case ROW_PROP_ACTIVATABLE:
      ctk_list_box_row_set_activatable (row, g_value_get_boolean (value));
      break;
    case ROW_PROP_SELECTABLE:
      ctk_list_box_row_set_selectable (row, g_value_get_boolean (value));
      break;
    case ROW_PROP_ACTION_NAME:
      ctk_list_box_row_set_action_name (CTK_ACTIONABLE (row), g_value_get_string (value));
      break;
    case ROW_PROP_ACTION_TARGET:
      ctk_list_box_row_set_action_target_value (CTK_ACTIONABLE (row), g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static const gchar *
ctk_list_box_row_get_action_name (CtkActionable *actionable)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (actionable);

  return ctk_action_helper_get_action_name (ROW_PRIV (row)->action_helper);
}

static GVariant *
ctk_list_box_row_get_action_target_value (CtkActionable *actionable)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (actionable);

  return ctk_action_helper_get_action_target_value (ROW_PRIV (row)->action_helper);
}

static void
ctk_list_box_row_actionable_iface_init (CtkActionableInterface *iface)
{
  iface->get_action_name = ctk_list_box_row_get_action_name;
  iface->set_action_name = ctk_list_box_row_set_action_name;
  iface->get_action_target_value = ctk_list_box_row_get_action_target_value;
  iface->set_action_target_value = ctk_list_box_row_set_action_target_value;
}

static void
ctk_list_box_row_finalize (GObject *obj)
{
  g_clear_object (&ROW_PRIV (CTK_LIST_BOX_ROW (obj))->header);
  g_clear_object (&ROW_PRIV (CTK_LIST_BOX_ROW (obj))->gadget);

  G_OBJECT_CLASS (ctk_list_box_row_parent_class)->finalize (obj);
}

static void
ctk_list_box_row_dispose (GObject *object)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (object);
  CtkListBoxRowPrivate *priv = ROW_PRIV (row);

  g_clear_object (&priv->action_helper);

  G_OBJECT_CLASS (ctk_list_box_row_parent_class)->dispose (object);
}

static void
ctk_list_box_row_grab_focus (CtkWidget *widget)
{
  CtkListBoxRow *row = CTK_LIST_BOX_ROW (widget);
  CtkListBox *box = ctk_list_box_row_get_box (row);

  g_return_if_fail (box != NULL);

  if (BOX_PRIV (box)->cursor_row != row)
    ctk_list_box_update_cursor (box, row, FALSE);

  CTK_WIDGET_CLASS (ctk_list_box_row_parent_class)->grab_focus (widget);
}

static void
ctk_list_box_row_class_init (CtkListBoxRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_LIST_BOX_ROW_ACCESSIBLE);

  object_class->get_property = ctk_list_box_row_get_property;
  object_class->set_property = ctk_list_box_row_set_property;
  object_class->finalize = ctk_list_box_row_finalize;
  object_class->dispose = ctk_list_box_row_dispose;

  widget_class->show = ctk_list_box_row_show;
  widget_class->hide = ctk_list_box_row_hide;
  widget_class->draw = ctk_list_box_row_draw;
  widget_class->get_preferred_height = ctk_list_box_row_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_list_box_row_get_preferred_height_for_width;
  widget_class->get_preferred_width = ctk_list_box_row_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_list_box_row_get_preferred_width_for_height;
  widget_class->size_allocate = ctk_list_box_row_size_allocate;
  widget_class->focus = ctk_list_box_row_focus;
  widget_class->grab_focus = ctk_list_box_row_grab_focus;

  klass->activate = ctk_list_box_row_activate;

  /**
   * CtkListBoxRow::activate:
   *
   * This is a keybinding signal, which will cause this row to be activated.
   *
   * If you want to be notified when the user activates a row (by key or not),
   * use the #CtkListBox::row-activated signal on the row’s parent #CtkListBox.
   *
   * Since: 3.10
   */
  row_signals[ROW__ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkListBoxRowClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  widget_class->activate_signal = row_signals[ROW__ACTIVATE];

  /**
   * CtkListBoxRow:activatable:
   *
   * The property determines whether the #CtkListBox::row-activated
   * signal will be emitted for this row.
   *
   * Since: 3.14
   */
  row_properties[ROW_PROP_ACTIVATABLE] =
    g_param_spec_boolean ("activatable",
                          P_("Activatable"),
                          P_("Whether this row can be activated"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkListBoxRow:selectable:
   *
   * The property determines whether this row can be selected.
   *
   * Since: 3.14
   */
  row_properties[ROW_PROP_SELECTABLE] =
    g_param_spec_boolean ("selectable",
                          P_("Selectable"),
                          P_("Whether this row can be selected"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_ROW_PROPERTY, row_properties);

  g_object_class_override_property (object_class, ROW_PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, ROW_PROP_ACTION_TARGET, "action-target");

  ctk_widget_class_set_css_name (widget_class, "row");
}

static void
ctk_list_box_row_init (CtkListBoxRow *row)
{
  ctk_widget_set_can_focus (CTK_WIDGET (row), TRUE);

  ROW_PRIV (row)->activatable = TRUE;
  ROW_PRIV (row)->selectable = TRUE;

  ROW_PRIV (row)->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (row)),
                                                     CTK_WIDGET (row),
                                                     ctk_list_box_row_measure,
                                                     ctk_list_box_row_allocate,
                                                     ctk_list_box_row_render,
                                                     NULL,
                                                     NULL);
  ctk_css_gadget_add_class (ROW_PRIV (row)->gadget, "activatable");
}

static void
ctk_list_box_buildable_add_child (CtkBuildable *buildable,
                                  CtkBuilder   *builder G_GNUC_UNUSED,
                                  GObject      *child,
                                  const gchar  *type)
{
  if (type && strcmp (type, "placeholder") == 0)
    ctk_list_box_set_placeholder (CTK_LIST_BOX (buildable), CTK_WIDGET (child));
  else if (!type)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
}

static void
ctk_list_box_buildable_interface_init (CtkBuildableIface *iface)
{
  iface->add_child = ctk_list_box_buildable_add_child;
}

static void
ctk_list_box_bound_model_changed (GListModel *list,
                                  guint       position,
                                  guint       removed,
                                  guint       added,
                                  gpointer    user_data)
{
  CtkListBox *box = user_data;
  CtkListBoxPrivate *priv = BOX_PRIV (user_data);
  guint i;

  while (removed--)
    {
      CtkListBoxRow *row;

      row = ctk_list_box_get_row_at_index (box, position);
      ctk_widget_destroy (CTK_WIDGET (row));
    }

  for (i = 0; i < added; i++)
    {
      GObject *item;
      CtkWidget *widget;

      item = g_list_model_get_item (list, position + i);
      widget = priv->create_widget_func (item, priv->create_widget_func_data);

      /* We allow the create_widget_func to either return a full
       * reference or a floating reference.  If we got the floating
       * reference, then turn it into a full reference now.  That means
       * that ctk_list_box_insert() will take another full reference.
       * Finally, we'll release this full reference below, leaving only
       * the one held by the box.
       */
      if (g_object_is_floating (widget))
        g_object_ref_sink (widget);

      ctk_widget_show (widget);
      ctk_list_box_insert (box, widget, position + i);

      g_object_unref (widget);
      g_object_unref (item);
    }
}

static void
ctk_list_box_check_model_compat (CtkListBox *box)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  if (priv->bound_model &&
      (priv->sort_func || priv->filter_func))
    g_warning ("CtkListBox with a model will ignore sort and filter functions");
}

/**
 * ctk_list_box_bind_model:
 * @box: a #CtkListBox
 * @model: (nullable): the #GListModel to be bound to @box
 * @create_widget_func: (nullable): a function that creates widgets for items
 *   or %NULL in case you also passed %NULL as @model
 * @user_data: user data passed to @create_widget_func
 * @user_data_free_func: function for freeing @user_data
 *
 * Binds @model to @box.
 *
 * If @box was already bound to a model, that previous binding is
 * destroyed.
 *
 * The contents of @box are cleared and then filled with widgets that
 * represent items from @model. @box is updated whenever @model changes.
 * If @model is %NULL, @box is left empty.
 *
 * It is undefined to add or remove widgets directly (for example, with
 * ctk_list_box_insert() or ctk_container_add()) while @box is bound to a
 * model.
 *
 * Note that using a model is incompatible with the filtering and sorting
 * functionality in CtkListBox. When using a model, filtering and sorting
 * should be implemented by the model.
 *
 * Since: 3.16
 */
void
ctk_list_box_bind_model (CtkListBox                 *box,
                         GListModel                 *model,
                         CtkListBoxCreateWidgetFunc  create_widget_func,
                         gpointer                    user_data,
                         GDestroyNotify              user_data_free_func)
{
  CtkListBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_LIST_BOX (box));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));
  g_return_if_fail (model == NULL || create_widget_func != NULL);

  if (priv->bound_model)
    {
      if (priv->create_widget_func_data_destroy)
        priv->create_widget_func_data_destroy (priv->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (priv->bound_model, ctk_list_box_bound_model_changed, box);
      g_clear_object (&priv->bound_model);
    }

  ctk_list_box_forall (CTK_CONTAINER (box), FALSE, (CtkCallback) ctk_widget_destroy, NULL);

  if (model == NULL)
    return;

  priv->bound_model = g_object_ref (model);
  priv->create_widget_func = create_widget_func;
  priv->create_widget_func_data = user_data;
  priv->create_widget_func_data_destroy = user_data_free_func;

  ctk_list_box_check_model_compat (box);

  g_signal_connect (priv->bound_model, "items-changed", G_CALLBACK (ctk_list_box_bound_model_changed), box);
  ctk_list_box_bound_model_changed (model, 0, 0, g_list_model_get_n_items (model), box);
}
