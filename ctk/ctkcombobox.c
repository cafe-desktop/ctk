/* ctkcombobox.c
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@gtk.org>
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

#include "ctkcombobox.h"

#include "ctkadjustment.h"
#include "ctkcellareabox.h"
#include "ctktreemenu.h"
#include "ctkbindings.h"
#include "ctkcelllayout.h"
#include "ctkcellrenderertext.h"
#include "ctkcellview.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkeventbox.h"
#include "ctkframe.h"
#include "ctkiconprivate.h"
#include "ctkbox.h"
#include "ctkliststore.h"
#include "ctkmain.h"
#include "ctkmenuprivate.h"
#include "ctkmenushellprivate.h"
#include "ctkscrolledwindow.h"
#include "ctktearoffmenuitem.h"
#include "ctktogglebutton.h"
#include "ctktreeselection.h"
#include "ctkwidgetpath.h"
#include "ctkwidgetprivate.h"
#include "ctkwindow.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctktooltipprivate.h"
#include "ctkcomboboxprivate.h"

#include <gobject/gvaluecollector.h>

#include <string.h>
#include <stdarg.h>

#include "ctkmarshalers.h"
#include "ctkintl.h"

#include "ctkentryprivate.h"
#include "ctktreeprivate.h"
#include "a11y/ctkcomboboxaccessible.h"


/**
 * SECTION:ctkcombobox
 * @Short_description: A widget used to choose from a list of items
 * @Title: CtkComboBox
 * @See_also: #CtkComboBoxText, #CtkTreeModel, #CtkCellRenderer
 *
 * A CtkComboBox is a widget that allows the user to choose from a list of
 * valid choices. The CtkComboBox displays the selected choice. When
 * activated, the CtkComboBox displays a popup which allows the user to
 * make a new choice. The style in which the selected value is displayed,
 * and the style of the popup is determined by the current theme. It may
 * be similar to a Windows-style combo box.
 *
 * The CtkComboBox uses the model-view pattern; the list of valid choices
 * is specified in the form of a tree model, and the display of the choices
 * can be adapted to the data in the model by using cell renderers, as you
 * would in a tree view. This is possible since CtkComboBox implements the
 * #CtkCellLayout interface. The tree model holding the valid choices is
 * not restricted to a flat list, it can be a real tree, and the popup will
 * reflect the tree structure.
 *
 * To allow the user to enter values not in the model, the “has-entry”
 * property allows the CtkComboBox to contain a #CtkEntry. This entry
 * can be accessed by calling ctk_bin_get_child() on the combo box.
 *
 * For a simple list of textual choices, the model-view API of CtkComboBox
 * can be a bit overwhelming. In this case, #CtkComboBoxText offers a
 * simple alternative. Both CtkComboBox and #CtkComboBoxText can contain
 * an entry.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * combobox
 * ├── box.linked
 * │   ╰── button.combo
 * │       ╰── box
 * │           ├── cellview
 * │           ╰── arrow
 * ╰── window.popup
 * ]|
 *
 * A normal combobox contains a box with the .linked class, a button
 * with the .combo class and inside those buttons, there are a cellview and
 * an arrow.
 *
 * |[<!-- language="plain" -->
 * combobox
 * ├── box.linked
 * │   ├── entry.combo
 * │   ╰── button.combo
 * │       ╰── box
 * │           ╰── arrow
 * ╰── window.popup
 * ]|
 *
 * A CtkComboBox with an entry has a single CSS node with name combobox. It
 * contains a box with the .linked class. That box contains an entry and a
 * button, both with the .combo class added.
 * The button also contains another node with name arrow.
 */


/* WELCOME, to THE house of evil code */
struct _CtkComboBoxPrivate
{
  CtkTreeModel *model;

  CtkCellArea *area;

  gint col_column;
  gint row_column;

  gint wrap_width;

  gint active; /* Only temporary */
  CtkTreeRowReference *active_row;

  CtkWidget *tree_view;

  CtkWidget *cell_view;

  CtkWidget *box;
  CtkWidget *button;
  CtkWidget *arrow;

  CtkWidget *popup_widget;
  CtkWidget *popup_window;
  CtkWidget *scrolled_window;

  CtkCssGadget *gadget;

  guint popup_idle_id;
  CdkEvent *trigger_event;
  guint scroll_timer;
  guint resize_idle_id;

  /* For "has-entry" specific behavior we track
   * an automated cell renderer and text column
   */
  gint  text_column;
  CtkCellRenderer *text_renderer;

  gint id_column;

  guint popup_in_progress : 1;
  guint popup_shown : 1;
  guint add_tearoffs : 1;
  guint has_frame : 1;
  guint is_cell_renderer : 1;
  guint editing_canceled : 1;
  guint auto_scroll : 1;
  guint button_sensitivity : 2;
  guint has_entry : 1;
  guint popup_fixed_width : 1;

  CtkTreeViewRowSeparatorFunc row_separator_func;
  gpointer                    row_separator_data;
  GDestroyNotify              row_separator_destroy;

  CdkDevice *grab_pointer;

  gchar *tearoff_title;
};

/* While debugging this evil code, I have learned that
 * there are actually 4 modes to this widget, which can
 * be characterized as follows
 *
 * 1) menu mode, no child added
 *
 * tree_view -> NULL
 * cell_view -> CtkCellView, regular child
 * button -> CtkToggleButton set_parent to combo
 * arrow -> CtkArrow set_parent to button
 * popup_widget -> CtkMenu
 * popup_window -> NULL
 * scrolled_window -> NULL
 *
 * 2) menu mode, child added
 *
 * tree_view -> NULL
 * cell_view -> NULL
 * button -> CtkToggleButton set_parent to combo
 * arrow -> CtkArrow, child of button
 * popup_widget -> CtkMenu
 * popup_window -> NULL
 * scrolled_window -> NULL
 *
 * 3) list mode, no child added
 *
 * tree_view -> CtkTreeView, child of scrolled_window
 * cell_view -> CtkCellView, regular child
 * button -> CtkToggleButton, set_parent to combo
 * arrow -> CtkArrow, child of button
 * popup_widget -> tree_view
 * popup_window -> CtkWindow
 * scrolled_window -> CtkScrolledWindow, child of popup_window
 *
 * 4) list mode, child added
 *
 * tree_view -> CtkTreeView, child of scrolled_window
 * cell_view -> NULL
 * button -> CtkToggleButton, set_parent to combo
 * arrow -> CtkArrow, child of button
 * popup_widget -> tree_view
 * popup_window -> CtkWindow
 * scrolled_window -> CtkScrolledWindow, child of popup_window
 *
 */

enum {
  CHANGED,
  MOVE_ACTIVE,
  POPUP,
  POPDOWN,
  FORMAT_ENTRY_TEXT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_WRAP_WIDTH,
  PROP_ROW_SPAN_COLUMN,
  PROP_COLUMN_SPAN_COLUMN,
  PROP_ACTIVE,
  PROP_ADD_TEAROFFS,
  PROP_TEAROFF_TITLE,
  PROP_HAS_FRAME,
  PROP_POPUP_SHOWN,
  PROP_BUTTON_SENSITIVITY,
  PROP_EDITING_CANCELED,
  PROP_HAS_ENTRY,
  PROP_ENTRY_TEXT_COLUMN,
  PROP_POPUP_FIXED_WIDTH,
  PROP_ID_COLUMN,
  PROP_ACTIVE_ID,
  PROP_CELL_AREA
};

static guint combo_box_signals[LAST_SIGNAL] = {0,};

#define SCROLL_TIME  100

/* common */

static void     ctk_combo_box_cell_layout_init     (CtkCellLayoutIface *iface);
static void     ctk_combo_box_cell_editable_init   (CtkCellEditableIface *iface);
static void     ctk_combo_box_constructed          (GObject          *object);
static void     ctk_combo_box_dispose              (GObject          *object);
static void     ctk_combo_box_finalize             (GObject          *object);
static void     ctk_combo_box_unmap                (CtkWidget        *widget);
static void     ctk_combo_box_destroy              (CtkWidget        *widget);

static void     ctk_combo_box_set_property         (GObject         *object,
                                                    guint            prop_id,
                                                    const GValue    *value,
                                                    GParamSpec      *spec);
static void     ctk_combo_box_get_property         (GObject         *object,
                                                    guint            prop_id,
                                                    GValue          *value,
                                                    GParamSpec      *spec);

static void     ctk_combo_box_grab_focus           (CtkWidget       *widget);
static void     ctk_combo_box_style_updated        (CtkWidget       *widget);
static void     ctk_combo_box_button_toggled       (CtkWidget       *widget,
                                                    gpointer         data);
static void     ctk_combo_box_button_state_flags_changed (CtkWidget     *widget,
                                                          CtkStateFlags  previous,
                                                          gpointer       data);
static void     ctk_combo_box_add                  (CtkContainer    *container,
                                                    CtkWidget       *widget);
static void     ctk_combo_box_remove               (CtkContainer    *container,
                                                    CtkWidget       *widget);

static void     ctk_combo_box_menu_show            (CtkWidget        *menu,
                                                    gpointer          user_data);
static void     ctk_combo_box_menu_hide            (CtkWidget        *menu,
                                                    gpointer          user_data);

static void     ctk_combo_box_set_popup_widget     (CtkComboBox      *combo_box,
                                                    CtkWidget        *popup);

static void     ctk_combo_box_unset_model          (CtkComboBox      *combo_box);

static void     ctk_combo_box_forall               (CtkContainer     *container,
                                                    gboolean          include_internals,
                                                    CtkCallback       callback,
                                                    gpointer          callback_data);
static gboolean ctk_combo_box_scroll_event         (CtkWidget        *widget,
                                                    CdkEventScroll   *event);
static void     ctk_combo_box_set_active_internal  (CtkComboBox      *combo_box,
                                                    CtkTreePath      *path);

static void     ctk_combo_box_check_appearance     (CtkComboBox      *combo_box);
static void     ctk_combo_box_real_move_active     (CtkComboBox      *combo_box,
                                                    CtkScrollType     scroll);
static void     ctk_combo_box_real_popup           (CtkComboBox      *combo_box);
static gboolean ctk_combo_box_real_popdown         (CtkComboBox      *combo_box);

/* listening to the model */
static void     ctk_combo_box_model_row_inserted   (CtkTreeModel     *model,
                                                    CtkTreePath      *path,
                                                    CtkTreeIter      *iter,
                                                    gpointer          user_data);
static void     ctk_combo_box_model_row_deleted    (CtkTreeModel     *model,
                                                    CtkTreePath      *path,
                                                    gpointer          user_data);
static void     ctk_combo_box_model_rows_reordered (CtkTreeModel     *model,
                                                    CtkTreePath      *path,
                                                    CtkTreeIter      *iter,
                                                    gint             *new_order,
                                                    gpointer          user_data);
static void     ctk_combo_box_model_row_changed    (CtkTreeModel     *model,
                                                    CtkTreePath      *path,
                                                    CtkTreeIter      *iter,
                                                    gpointer          data);
static void     ctk_combo_box_model_row_expanded   (CtkTreeModel     *model,
                                                    CtkTreePath      *path,
                                                    CtkTreeIter      *iter,
                                                    gpointer          data);

/* list */
static void     ctk_combo_box_list_position        (CtkComboBox      *combo_box,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *width,
                                                    gint             *height);
static void     ctk_combo_box_list_setup           (CtkComboBox      *combo_box);
static void     ctk_combo_box_list_destroy         (CtkComboBox      *combo_box);

static gboolean ctk_combo_box_list_button_released (CtkWidget        *widget,
                                                    CdkEventButton   *event,
                                                    gpointer          data);
static gboolean ctk_combo_box_list_key_press       (CtkWidget        *widget,
                                                    CdkEventKey      *event,
                                                    gpointer          data);
static gboolean ctk_combo_box_list_enter_notify    (CtkWidget        *widget,
                                                    CdkEventCrossing *event,
                                                    gpointer          data);
static void     ctk_combo_box_list_auto_scroll     (CtkComboBox   *combo,
                                                    gint           x,
                                                    gint           y);
static gboolean ctk_combo_box_list_scroll_timeout  (CtkComboBox   *combo);
static gboolean ctk_combo_box_list_button_pressed  (CtkWidget        *widget,
                                                    CdkEventButton   *event,
                                                    gpointer          data);

static gboolean ctk_combo_box_list_select_func     (CtkTreeSelection *selection,
                                                    CtkTreeModel     *model,
                                                    CtkTreePath      *path,
                                                    gboolean          path_currently_selected,
                                                    gpointer          data);

static void     ctk_combo_box_list_row_changed     (CtkTreeModel     *model,
                                                    CtkTreePath      *path,
                                                    CtkTreeIter      *iter,
                                                    gpointer          data);
static void     ctk_combo_box_list_popup_resize    (CtkComboBox      *combo_box);

/* menu */
static void     ctk_combo_box_menu_setup           (CtkComboBox      *combo_box);
static void     ctk_combo_box_update_title         (CtkComboBox      *combo_box);
static void     ctk_combo_box_menu_destroy         (CtkComboBox      *combo_box);


static gboolean ctk_combo_box_menu_button_press    (CtkWidget        *widget,
                                                    CdkEventButton   *event,
                                                    gpointer          user_data);
static void     ctk_combo_box_menu_activate        (CtkWidget        *menu,
                                                    const gchar      *path,
                                                    CtkComboBox      *combo_box);
static void     ctk_combo_box_update_sensitivity   (CtkComboBox      *combo_box);
static gboolean ctk_combo_box_menu_key_press       (CtkWidget        *widget,
                                                    CdkEventKey      *event,
                                                    gpointer          data);
static void     ctk_combo_box_menu_popup           (CtkComboBox      *combo_box,
                                                    const CdkEvent   *trigger_event);

/* cell layout */
static CtkCellArea *ctk_combo_box_cell_layout_get_area       (CtkCellLayout    *cell_layout);

static gboolean ctk_combo_box_mnemonic_activate              (CtkWidget    *widget,
                                                              gboolean      group_cycling);

static void     ctk_combo_box_child_show                     (CtkWidget       *widget,
                                                              CtkComboBox     *combo_box);
static void     ctk_combo_box_child_hide                     (CtkWidget       *widget,
                                                              CtkComboBox     *combo_box);

/* CtkComboBox:has-entry callbacks */
static void     ctk_combo_box_entry_contents_changed         (CtkEntry        *entry,
                                                              gpointer         user_data);
static void     ctk_combo_box_entry_active_changed           (CtkComboBox     *combo_box,
                                                              gpointer         user_data);
static gchar   *ctk_combo_box_format_entry_text              (CtkComboBox     *combo_box,
                                                              const gchar     *path);

/* CtkBuildable method implementation */
static CtkBuildableIface *parent_buildable_iface;

static void     ctk_combo_box_buildable_init                 (CtkBuildableIface *iface);
static void     ctk_combo_box_buildable_add_child            (CtkBuildable  *buildable,
                                                              CtkBuilder    *builder,
                                                              GObject       *child,
                                                              const gchar   *type);
static gboolean ctk_combo_box_buildable_custom_tag_start     (CtkBuildable  *buildable,
                                                              CtkBuilder    *builder,
                                                              GObject       *child,
                                                              const gchar   *tagname,
                                                              GMarkupParser *parser,
                                                              gpointer      *data);
static void     ctk_combo_box_buildable_custom_tag_end       (CtkBuildable  *buildable,
                                                              CtkBuilder    *builder,
                                                              GObject       *child,
                                                              const gchar   *tagname,
                                                              gpointer      *data);
static GObject *ctk_combo_box_buildable_get_internal_child   (CtkBuildable *buildable,
                                                              CtkBuilder   *builder,
                                                              const gchar  *childname);


/* CtkCellEditable method implementations */
static void     ctk_combo_box_start_editing                  (CtkCellEditable *cell_editable,
                                                              CdkEvent        *event);

G_DEFINE_TYPE_WITH_CODE (CtkComboBox, ctk_combo_box, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkComboBox)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_LAYOUT,
                                                ctk_combo_box_cell_layout_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_EDITABLE,
                                                ctk_combo_box_cell_editable_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_combo_box_buildable_init))


/* common */
static void
ctk_combo_box_measure (CtkCssGadget   *gadget,
                       CtkOrientation  orientation,
                       int             size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline,
                       gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);
  CtkComboBoxPrivate *priv = combo_box->priv;

  _ctk_widget_get_preferred_size_for_size (priv->box,
                                           orientation,
                                           size,
                                           minimum, natural,
                                           minimum_baseline, natural_baseline);
}

static void
ctk_combo_box_allocate (CtkCssGadget        *gadget,
                        const CtkAllocation *allocation,
                        int                  baseline,
                        CtkAllocation       *out_clip,
                        gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);
  CtkComboBoxPrivate *priv = combo_box->priv;

  ctk_widget_size_allocate_with_baseline (priv->box, (CtkAllocation *) allocation, baseline);
  ctk_widget_get_clip (priv->box, out_clip);

  if (!priv->tree_view)
    {
      if (ctk_widget_get_visible (priv->popup_widget))
        {
          gint menu_width;

          if (priv->wrap_width == 0)
            {
              ctk_widget_set_size_request (priv->popup_widget, -1, -1);

              if (priv->popup_fixed_width)
                ctk_widget_get_preferred_width (priv->popup_widget, &menu_width, NULL);
              else
                ctk_widget_get_preferred_width (priv->popup_widget, NULL, &menu_width);

              ctk_widget_set_size_request (priv->popup_widget,
                                           MAX (allocation->width, menu_width), -1);
            }

          /* reposition the menu after giving it a new width */
          ctk_menu_reposition (CTK_MENU (priv->popup_widget));
        }
    }
  else
    {
      if (ctk_widget_get_visible (priv->popup_window))
        {
          gint x, y, width, height;
          ctk_combo_box_list_position (combo_box, &x, &y, &width, &height);
          ctk_window_move (CTK_WINDOW (priv->popup_window), x, y);
          ctk_widget_set_size_request (priv->popup_window, width, height);
        }
    }
}

static gboolean
ctk_combo_box_render (CtkCssGadget *gadget,
                      cairo_t      *cr,
                      int           x,
                      int           y,
                      int           width,
                      int           height,
                      gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);
  CtkComboBoxPrivate *priv = combo_box->priv;

  ctk_container_propagate_draw (CTK_CONTAINER (widget),
                                priv->box, cr);

  return FALSE;
}

static void
ctk_combo_box_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum_size,
                                   gint      *natural_size)
{
  gint dummy;

  /* https://bugzilla.gnome.org/show_bug.cgi?id=729496 */
  if (natural_size == NULL)
    natural_size = &dummy;

  ctk_css_gadget_get_preferred_size (CTK_COMBO_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_combo_box_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum_size,
                                    gint      *natural_size)
{
  gint min_width;

  /* Combo box is height-for-width only
   * (so we always just reserve enough height for the minimum width) */
  ctk_css_gadget_get_preferred_size (CTK_COMBO_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     &min_width, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (CTK_COMBO_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     min_width,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_combo_box_get_preferred_width_for_height (CtkWidget *widget,
                                              gint       avail_size,
                                              gint      *minimum_size,
                                              gint      *natural_size)
{
  /* Combo box is height-for-width only
   * (so we assume we always reserved enough height for the minimum width) */
  ctk_css_gadget_get_preferred_size (CTK_COMBO_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     avail_size,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_combo_box_get_preferred_height_for_width (CtkWidget *widget,
                                              gint       avail_size,
                                              gint      *minimum_size,
                                              gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_COMBO_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     avail_size,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_combo_box_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CTK_COMBO_BOX (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static gboolean
ctk_combo_box_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_COMBO_BOX (widget)->priv->gadget, cr);
  return FALSE;
}

static void
ctk_combo_box_compute_expand (CtkWidget *widget,
                              gboolean  *hexpand,
                              gboolean  *vexpand)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (combo_box));
  if (child && child != priv->cell_view)
    {
      *hexpand = ctk_widget_compute_expand (child, CTK_ORIENTATION_HORIZONTAL);
      *vexpand = ctk_widget_compute_expand (child, CTK_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static void
ctk_combo_box_class_init (CtkComboBoxClass *klass)
{
  GObjectClass *object_class;
  CtkContainerClass *container_class;
  CtkWidgetClass *widget_class;
  CtkBindingSet *binding_set;

  container_class = (CtkContainerClass *)klass;
  container_class->forall = ctk_combo_box_forall;
  container_class->add = ctk_combo_box_add;
  container_class->remove = ctk_combo_box_remove;

  ctk_container_class_handle_border_width (container_class);

  widget_class = (CtkWidgetClass *)klass;
  widget_class->size_allocate = ctk_combo_box_size_allocate;
  widget_class->draw = ctk_combo_box_draw;
  widget_class->scroll_event = ctk_combo_box_scroll_event;
  widget_class->mnemonic_activate = ctk_combo_box_mnemonic_activate;
  widget_class->grab_focus = ctk_combo_box_grab_focus;
  widget_class->style_updated = ctk_combo_box_style_updated;
  widget_class->get_preferred_width = ctk_combo_box_get_preferred_width;
  widget_class->get_preferred_height = ctk_combo_box_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_combo_box_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = ctk_combo_box_get_preferred_width_for_height;
  widget_class->unmap = ctk_combo_box_unmap;
  widget_class->destroy = ctk_combo_box_destroy;
  widget_class->compute_expand = ctk_combo_box_compute_expand;

  object_class = (GObjectClass *)klass;
  object_class->constructed = ctk_combo_box_constructed;
  object_class->dispose = ctk_combo_box_dispose;
  object_class->finalize = ctk_combo_box_finalize;
  object_class->set_property = ctk_combo_box_set_property;
  object_class->get_property = ctk_combo_box_get_property;

  klass->format_entry_text = ctk_combo_box_format_entry_text;

  /* signals */
  /**
   * CtkComboBox::changed:
   * @widget: the object which received the signal
   *
   * The changed signal is emitted when the active
   * item is changed. The can be due to the user selecting
   * a different item from the list, or due to a
   * call to ctk_combo_box_set_active_iter().
   * It will also be emitted while typing into the entry of a combo box
   * with an entry.
   *
   * Since: 2.4
   */
  combo_box_signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkComboBoxClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  /**
   * CtkComboBox::move-active:
   * @widget: the object that received the signal
   * @scroll_type: a #CtkScrollType
   *
   * The ::move-active signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to move the active selection.
   *
   * Since: 2.12
   */
  combo_box_signals[MOVE_ACTIVE] =
    g_signal_new_class_handler (I_("move-active"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_combo_box_real_move_active),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 1,
                                CTK_TYPE_SCROLL_TYPE);

  /**
   * CtkComboBox::popup:
   * @widget: the object that received the signal
   *
   * The ::popup signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to popup the combo box list.
   *
   * The default binding for this signal is Alt+Down.
   *
   * Since: 2.12
   */
  combo_box_signals[POPUP] =
    g_signal_new_class_handler (I_("popup"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_combo_box_real_popup),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);
  /**
   * CtkComboBox::popdown:
   * @button: the object which received the signal
   *
   * The ::popdown signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to popdown the combo box list.
   *
   * The default bindings for this signal are Alt+Up and Escape.
   *
   * Since: 2.12
   */
  combo_box_signals[POPDOWN] =
    g_signal_new_class_handler (I_("popdown"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_combo_box_real_popdown),
                                NULL, NULL,
                                _ctk_marshal_BOOLEAN__VOID,
                                G_TYPE_BOOLEAN, 0);

  /**
   * CtkComboBox::format-entry-text:
   * @combo: the object which received the signal
   * @path: the CtkTreePath string from the combo box's current model to format text for
   *
   * For combo boxes that are created with an entry (See CtkComboBox:has-entry).
   *
   * A signal which allows you to change how the text displayed in a combo box's
   * entry is displayed.
   *
   * Connect a signal handler which returns an allocated string representing
   * @path. That string will then be used to set the text in the combo box's entry.
   * The default signal handler uses the text from the CtkComboBox::entry-text-column
   * model column.
   *
   * Here's an example signal handler which fetches data from the model and
   * displays it in the entry.
   * |[<!-- language="C" -->
   * static gchar*
   * format_entry_text_callback (CtkComboBox *combo,
   *                             const gchar *path,
   *                             gpointer     user_data)
   * {
   *   CtkTreeIter iter;
   *   CtkTreeModel model;
   *   gdouble      value;
   *
   *   model = ctk_combo_box_get_model (combo);
   *
   *   ctk_tree_model_get_iter_from_string (model, &iter, path);
   *   ctk_tree_model_get (model, &iter,
   *                       THE_DOUBLE_VALUE_COLUMN, &value,
   *                       -1);
   *
   *   return g_strdup_printf ("%g", value);
   * }
   * ]|
   *
   * Returns: (transfer full): a newly allocated string representing @path
   * for the current CtkComboBox model.
   *
   * Since: 3.4
   */
  combo_box_signals[FORMAT_ENTRY_TEXT] =
    g_signal_new (I_("format-entry-text"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkComboBoxClass, format_entry_text),
                  _ctk_single_string_accumulator, NULL,
                  _ctk_marshal_STRING__STRING,
                  G_TYPE_STRING, 1, G_TYPE_STRING);

  /* key bindings */
  binding_set = ctk_binding_set_by_class (widget_class);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Down, CDK_MOD1_MASK,
                                "popup", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Down, CDK_MOD1_MASK,
                                "popup", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Up, CDK_MOD1_MASK,
                                "popdown", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Up, CDK_MOD1_MASK,
                                "popdown", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Escape, 0,
                                "popdown", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Up, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_STEP_UP);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Up, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_STEP_UP);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Page_Up, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_UP);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Page_Up, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_UP);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Home, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_START);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Home, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_START);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Down, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_STEP_DOWN);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Down, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_STEP_DOWN);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Page_Down, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_DOWN);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Page_Down, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_DOWN);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_End, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_END);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_End, 0,
                                "move-active", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_END);

  /* properties */
  g_object_class_override_property (object_class,
                                    PROP_EDITING_CANCELED,
                                    "editing-canceled");

  /**
   * CtkComboBox:model:
   *
   * The model from which the combo box takes the values shown
   * in the list.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        P_("ComboBox model"),
                                                        P_("The model for the combo box"),
                                                        CTK_TYPE_TREE_MODEL,
                                                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkComboBox:wrap-width:
   *
   * If wrap-width is set to a positive value, items in the popup will be laid
   * out along multiple columns, starting a new row on reaching the wrap width.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_WRAP_WIDTH,
                                   g_param_spec_int ("wrap-width",
                                                     P_("Wrap width"),
                                                     P_("Wrap width for laying out the items in a grid"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * CtkComboBox:row-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column
   * of type %G_TYPE_INT in the model. The value in that column for each item
   * will determine how many rows that item will span in the popup. Therefore,
   * values in this column must be greater than zero.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ROW_SPAN_COLUMN,
                                   g_param_spec_int ("row-span-column",
                                                     P_("Row span column"),
                                                     P_("TreeModel column containing the row span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * CtkComboBox:column-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column
   * of type %G_TYPE_INT in the model. The value in that column for each item
   * will determine how many columns that item will span in the popup.
   * Therefore, values in this column must be greater than zero, and the sum of
   * an item’s column position + span should not exceed #CtkComboBox:wrap-width.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_COLUMN_SPAN_COLUMN,
                                   g_param_spec_int ("column-span-column",
                                                     P_("Column span column"),
                                                     P_("TreeModel column containing the column span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * CtkComboBox:active:
   *
   * The item which is currently active. If the model is a non-flat treemodel,
   * and the active item is not an immediate child of the root of the tree,
   * this property has the value
   * `ctk_tree_path_get_indices (path)[0]`,
   * where `path` is the #CtkTreePath of the active item.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_int ("active",
                                                     P_("Active item"),
                                                     P_("The item which is currently active"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkComboBox:add-tearoffs:
   *
   * The add-tearoffs property controls whether generated menus
   * have tearoff menu items.
   *
   * Note that this only affects menu style combo boxes.
   *
   * Since: 2.6
   *
   * Deprecated: 3.10
   */
  g_object_class_install_property (object_class,
                                   PROP_ADD_TEAROFFS,
                                   g_param_spec_boolean ("add-tearoffs",
                                                         P_("Add tearoffs to menus"),
                                                         P_("Whether dropdowns should have a tearoff menu item"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED));

  /**
   * CtkComboBox:has-frame:
   *
   * The has-frame property controls whether a frame
   * is drawn around the entry.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_FRAME,
                                   g_param_spec_boolean ("has-frame",
                                                         P_("Has Frame"),
                                                         P_("Whether the combo box draws a frame around the child"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkComboBox:tearoff-title:
   *
   * A title that may be displayed by the window manager
   * when the popup is torn-off.
   *
   * Since: 2.10
   *
   * Deprecated: 3.10
   */
  g_object_class_install_property (object_class,
                                   PROP_TEAROFF_TITLE,
                                   g_param_spec_string ("tearoff-title",
                                                        P_("Tearoff Title"),
                                                        P_("A title that may be displayed by the window manager when the popup is torn-off"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED));


  /**
   * CtkComboBox:popup-shown:
   *
   * Whether the combo boxes dropdown is popped up.
   * Note that this property is mainly useful, because
   * it allows you to connect to notify::popup-shown.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_POPUP_SHOWN,
                                   g_param_spec_boolean ("popup-shown",
                                                         P_("Popup shown"),
                                                         P_("Whether the combo's dropdown is shown"),
                                                         FALSE,
                                                         CTK_PARAM_READABLE));


   /**
    * CtkComboBox:button-sensitivity:
    *
    * Whether the dropdown button is sensitive when
    * the model is empty.
    *
    * Since: 2.14
    */
   g_object_class_install_property (object_class,
                                    PROP_BUTTON_SENSITIVITY,
                                    g_param_spec_enum ("button-sensitivity",
                                                       P_("Button Sensitivity"),
                                                       P_("Whether the dropdown button is sensitive when the model is empty"),
                                                       CTK_TYPE_SENSITIVITY_TYPE,
                                                       CTK_SENSITIVITY_AUTO,
                                                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * CtkComboBox:has-entry:
    *
    * Whether the combo box has an entry.
    *
    * Since: 2.24
    */
   g_object_class_install_property (object_class,
                                    PROP_HAS_ENTRY,
                                    g_param_spec_boolean ("has-entry",
                                                          P_("Has Entry"),
                                                          P_("Whether combo box has an entry"),
                                                          FALSE,
                                                          CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

   /**
    * CtkComboBox:entry-text-column:
    *
    * The column in the combo box's model to associate with strings from the entry
    * if the combo was created with #CtkComboBox:has-entry = %TRUE.
    *
    * Since: 2.24
    */
   g_object_class_install_property (object_class,
                                    PROP_ENTRY_TEXT_COLUMN,
                                    g_param_spec_int ("entry-text-column",
                                                      P_("Entry Text Column"),
                                                      P_("The column in the combo box's model to associate "
                                                         "with strings from the entry if the combo was "
                                                         "created with #CtkComboBox:has-entry = %TRUE"),
                                                      -1, G_MAXINT, -1,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * CtkComboBox:id-column:
    *
    * The column in the combo box's model that provides string
    * IDs for the values in the model, if != -1.
    *
    * Since: 3.0
    */
   g_object_class_install_property (object_class,
                                    PROP_ID_COLUMN,
                                    g_param_spec_int ("id-column",
                                                      P_("ID Column"),
                                                      P_("The column in the combo box's model that provides "
                                                      "string IDs for the values in the model"),
                                                      -1, G_MAXINT, -1,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * CtkComboBox:active-id:
    *
    * The value of the ID column of the active row.
    *
    * Since: 3.0
    */
   g_object_class_install_property (object_class,
                                    PROP_ACTIVE_ID,
                                    g_param_spec_string ("active-id",
                                                         P_("Active id"),
                                                         P_("The value of the id column "
                                                         "for the active row"),
                                                         NULL,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * CtkComboBox:popup-fixed-width:
    *
    * Whether the popup's width should be a fixed width matching the
    * allocated width of the combo box.
    *
    * Since: 3.0
    */
   g_object_class_install_property (object_class,
                                    PROP_POPUP_FIXED_WIDTH,
                                    g_param_spec_boolean ("popup-fixed-width",
                                                          P_("Popup Fixed Width"),
                                                          P_("Whether the popup's width should be a "
                                                             "fixed width matching the allocated width "
                                                             "of the combo box"),
                                                          TRUE,
                                                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * CtkComboBox:cell-area:
    *
    * The #CtkCellArea used to layout cell renderers for this combo box.
    *
    * If no area is specified when creating the combo box with ctk_combo_box_new_with_area()
    * a horizontally oriented #CtkCellAreaBox will be used.
    *
    * Since: 3.0
    */
   g_object_class_install_property (object_class,
                                    PROP_CELL_AREA,
                                    g_param_spec_object ("cell-area",
                                                         P_("Cell Area"),
                                                         P_("The CtkCellArea used to layout cells"),
                                                         CTK_TYPE_CELL_AREA,
                                                         CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("appears-as-list",
                                                                 P_("Appears as list"),
                                                                 P_("Whether dropdowns should look like lists rather than menus"),
                                                                 FALSE,
                                                                 CTK_PARAM_READABLE));

  /**
   * CtkComboBox:arrow-size:
   *
   * Sets the minimum size of the arrow in the combo box.  Note
   * that the arrow size is coupled to the font size, so in case
   * a larger font is used, the arrow will be larger than set
   * by arrow size.
   *
   * Since: 2.12
   *
   * Deprecated: 3.20: use the standard min-width/min-height CSS properties on
   *   the arrow node; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("arrow-size",
                                                             P_("Arrow Size"),
                                                             P_("The minimum size of the arrow in the combo box"),
                                                             0,
                                                             G_MAXINT,
                                                             15,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkComboBox:arrow-scaling:
   *
   * Sets the amount of space used up by the combobox arrow,
   * proportional to the font size.
   *
   * Deprecated: 3.20: use the standard min-width/min-height CSS properties on
   *   the arrow node; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
                                                               P_("Arrow Scaling"),
                                                               P_("The amount of space used by the arrow"),
                                                             0,
                                                             2.0,
                                                             1.0,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkComboBox:shadow-type:
   *
   * Which kind of shadow to draw around the combo box.
   *
   * Since: 2.12
   *
   * Deprecated: 3.20: use CSS styling to change the appearance of the combobox
   *   frame; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("shadow-type",
                                                              P_("Shadow type"),
                                                              P_("Which kind of shadow to draw around the combo box"),
                                                              CTK_TYPE_SHADOW_TYPE,
                                                              CTK_SHADOW_NONE,
                                                              CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkcombobox.ui");
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkComboBox, box);
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkComboBox, button);
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkComboBox, arrow);
  ctk_widget_class_bind_template_callback (widget_class, ctk_combo_box_button_toggled);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_COMBO_BOX_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "combobox");
}

static void
ctk_combo_box_buildable_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = ctk_combo_box_buildable_add_child;
  iface->custom_tag_start = ctk_combo_box_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_combo_box_buildable_custom_tag_end;
  iface->get_internal_child = ctk_combo_box_buildable_get_internal_child;
}

static void
ctk_combo_box_cell_layout_init (CtkCellLayoutIface *iface)
{
  iface->get_area = ctk_combo_box_cell_layout_get_area;
}

static void
ctk_combo_box_cell_editable_init (CtkCellEditableIface *iface)
{
  iface->start_editing = ctk_combo_box_start_editing;
}

static void
ctk_combo_box_init (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv;
  CtkCssNode *widget_node;
  CtkStyleContext *context;

  combo_box->priv = ctk_combo_box_get_instance_private (combo_box);
  priv = combo_box->priv;

  priv->wrap_width = 0;

  priv->active = -1;
  priv->active_row = NULL;
  priv->col_column = -1;
  priv->row_column = -1;

  priv->popup_shown = FALSE;
  priv->add_tearoffs = FALSE;
  priv->has_frame = TRUE;
  priv->is_cell_renderer = FALSE;
  priv->editing_canceled = FALSE;
  priv->auto_scroll = FALSE;
  priv->button_sensitivity = CTK_SENSITIVITY_AUTO;
  priv->has_entry = FALSE;
  priv->popup_fixed_width = TRUE;

  priv->text_column = -1;
  priv->text_renderer = NULL;
  priv->id_column = -1;

  g_type_ensure (CTK_TYPE_ICON);
  ctk_widget_init_template (CTK_WIDGET (combo_box));

  ctk_widget_add_events (priv->button, CDK_SCROLL_MASK);

  context = ctk_widget_get_style_context (priv->button);
  ctk_style_context_remove_class (context, "toggle");
  ctk_style_context_add_class (context, "combo");

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (combo_box));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (combo_box),
                                                     ctk_combo_box_measure,
                                                     ctk_combo_box_allocate,
                                                     ctk_combo_box_render,
                                                     NULL, NULL);
}

static void
ctk_combo_box_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (object);
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkCellArea *area;

  switch (prop_id)
    {
    case PROP_MODEL:
      ctk_combo_box_set_model (combo_box, g_value_get_object (value));
      break;

    case PROP_WRAP_WIDTH:
      ctk_combo_box_set_wrap_width (combo_box, g_value_get_int (value));
      break;

    case PROP_ROW_SPAN_COLUMN:
      ctk_combo_box_set_row_span_column (combo_box, g_value_get_int (value));
      break;

    case PROP_COLUMN_SPAN_COLUMN:
      ctk_combo_box_set_column_span_column (combo_box, g_value_get_int (value));
      break;

    case PROP_ACTIVE:
      ctk_combo_box_set_active (combo_box, g_value_get_int (value));
      break;

    case PROP_ADD_TEAROFFS:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_combo_box_set_add_tearoffs (combo_box, g_value_get_boolean (value));
G_GNUC_END_IGNORE_DEPRECATIONS;
      break;

    case PROP_HAS_FRAME:
      if (priv->has_frame != g_value_get_boolean (value))
        {
          priv->has_frame = g_value_get_boolean (value);
          if (priv->has_entry)
            ctk_entry_set_has_frame (CTK_ENTRY (ctk_bin_get_child (CTK_BIN (combo_box))),
                                     priv->has_frame);
          g_object_notify (object, "has-frame");
        }
      break;

    case PROP_TEAROFF_TITLE:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_combo_box_set_title (combo_box, g_value_get_string (value));
G_GNUC_END_IGNORE_DEPRECATIONS;
      break;

    case PROP_POPUP_SHOWN:
      if (g_value_get_boolean (value))
        ctk_combo_box_popup (combo_box);
      else
        ctk_combo_box_popdown (combo_box);
      break;

    case PROP_BUTTON_SENSITIVITY:
      ctk_combo_box_set_button_sensitivity (combo_box,
                                            g_value_get_enum (value));
      break;

    case PROP_POPUP_FIXED_WIDTH:
      ctk_combo_box_set_popup_fixed_width (combo_box,
                                           g_value_get_boolean (value));
      break;

    case PROP_EDITING_CANCELED:
      if (priv->editing_canceled != g_value_get_boolean (value))
        {
          priv->editing_canceled = g_value_get_boolean (value);
          g_object_notify (object, "editing-canceled");
        }
      break;

    case PROP_HAS_ENTRY:
      priv->has_entry = g_value_get_boolean (value);
      break;

    case PROP_ENTRY_TEXT_COLUMN:
      ctk_combo_box_set_entry_text_column (combo_box, g_value_get_int (value));
      break;

    case PROP_ID_COLUMN:
      ctk_combo_box_set_id_column (combo_box, g_value_get_int (value));
      break;

    case PROP_ACTIVE_ID:
      ctk_combo_box_set_active_id (combo_box, g_value_get_string (value));
      break;

    case PROP_CELL_AREA:
      /* Construct-only, can only be assigned once */
      area = g_value_get_object (value);
      if (area)
        {
          if (priv->area != NULL)
            {
              g_warning ("cell-area has already been set, ignoring construct property");
              g_object_ref_sink (area);
              g_object_unref (area);
            }
          else
            priv->area = g_object_ref_sink (area);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_combo_box_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (object);
  CtkComboBoxPrivate *priv = combo_box->priv;

  switch (prop_id)
    {
      case PROP_MODEL:
        g_value_set_object (value, priv->model);
        break;

      case PROP_WRAP_WIDTH:
        g_value_set_int (value, priv->wrap_width);
        break;

      case PROP_ROW_SPAN_COLUMN:
        g_value_set_int (value, priv->row_column);
        break;

      case PROP_COLUMN_SPAN_COLUMN:
        g_value_set_int (value, priv->col_column);
        break;

      case PROP_ACTIVE:
        g_value_set_int (value, ctk_combo_box_get_active (combo_box));
        break;

      case PROP_ADD_TEAROFFS:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        g_value_set_boolean (value, ctk_combo_box_get_add_tearoffs (combo_box));
G_GNUC_END_IGNORE_DEPRECATIONS;
        break;

      case PROP_HAS_FRAME:
        g_value_set_boolean (value, priv->has_frame);
        break;

      case PROP_TEAROFF_TITLE:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        g_value_set_string (value, ctk_combo_box_get_title (combo_box));
G_GNUC_END_IGNORE_DEPRECATIONS;
        break;

      case PROP_POPUP_SHOWN:
        g_value_set_boolean (value, priv->popup_shown);
        break;

      case PROP_BUTTON_SENSITIVITY:
        g_value_set_enum (value, priv->button_sensitivity);
        break;

      case PROP_POPUP_FIXED_WIDTH:
        g_value_set_boolean (value, priv->popup_fixed_width);
        break;

      case PROP_EDITING_CANCELED:
        g_value_set_boolean (value, priv->editing_canceled);
        break;

      case PROP_HAS_ENTRY:
        g_value_set_boolean (value, priv->has_entry);
        break;

      case PROP_ENTRY_TEXT_COLUMN:
        g_value_set_int (value, priv->text_column);
        break;

      case PROP_ID_COLUMN:
        g_value_set_int (value, priv->id_column);
        break;

      case PROP_ACTIVE_ID:
        g_value_set_string (value, ctk_combo_box_get_active_id (combo_box));
        break;

      case PROP_CELL_AREA:
        g_value_set_object (value, priv->area);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ctk_combo_box_button_state_flags_changed (CtkWidget     *widget,
                                          CtkStateFlags  previous,
                                          gpointer       data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (ctk_widget_get_realized (widget))
    {
      if (!priv->tree_view && priv->cell_view)
        ctk_widget_set_state_flags (priv->cell_view,
                                    ctk_widget_get_state_flags (widget),
                                    TRUE);
    }
}

static void
ctk_combo_box_check_appearance (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  gboolean appears_as_list;

  /* if wrap_width > 0, then we are in grid-mode and forced to use
   * unix style
   */
  if (priv->wrap_width)
    appears_as_list = FALSE;
  else
    ctk_widget_style_get (CTK_WIDGET (combo_box),
                          "appears-as-list", &appears_as_list,
                          NULL);
  
  if (appears_as_list)
    {
      /* Destroy all the menu mode widgets, if they exist. */
      if (CTK_IS_MENU (priv->popup_widget))
        ctk_combo_box_menu_destroy (combo_box);

      /* Create the list mode widgets, if they don't already exist. */
      if (!CTK_IS_TREE_VIEW (priv->tree_view))
        ctk_combo_box_list_setup (combo_box);
    }
  else
    {
      /* Destroy all the list mode widgets, if they exist. */
      if (CTK_IS_TREE_VIEW (priv->tree_view))
        ctk_combo_box_list_destroy (combo_box);

      /* Create the menu mode widgets, if they don't already exist. */
      if (!CTK_IS_MENU (priv->popup_widget))
        ctk_combo_box_menu_setup (combo_box);
    }
}

static void
ctk_combo_box_style_updated (CtkWidget *widget)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);

  CTK_WIDGET_CLASS (ctk_combo_box_parent_class)->style_updated (widget);

  ctk_combo_box_check_appearance (combo_box);
}

static void
ctk_combo_box_button_toggled (CtkWidget *widget,
                              gpointer   data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget)))
    {
      if (!combo_box->priv->popup_in_progress)
        ctk_combo_box_popup (combo_box);
    }
  else
    ctk_combo_box_popdown (combo_box);
}

static void
ctk_combo_box_create_child (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkWidget *child;

  if (priv->has_entry)
    {
      CtkWidget *entry;
      CtkStyleContext *context;

      entry = ctk_entry_new ();
      ctk_widget_show (entry);
      ctk_container_add (CTK_CONTAINER (combo_box), entry);

      context = ctk_widget_get_style_context (CTK_WIDGET (entry));
      ctk_style_context_add_class (context, "combo");

      g_signal_connect (combo_box, "changed",
                        G_CALLBACK (ctk_combo_box_entry_active_changed), NULL);
    }
  else
    {
      child = ctk_cell_view_new_with_context (priv->area, NULL);
      priv->cell_view = child;
      ctk_widget_set_hexpand (child, TRUE);
      ctk_cell_view_set_fit_model (CTK_CELL_VIEW (priv->cell_view), TRUE);
      ctk_cell_view_set_model (CTK_CELL_VIEW (priv->cell_view), priv->model);
      ctk_container_add (CTK_CONTAINER (ctk_widget_get_parent (priv->arrow)),
                         priv->cell_view);
      _ctk_bin_set_child (CTK_BIN (combo_box), priv->cell_view);
      ctk_widget_show (priv->cell_view);
    }
}

static void
ctk_combo_box_add (CtkContainer *container,
                   CtkWidget    *widget)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (container);
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (priv->box == NULL)
    {
      ctk_widget_set_parent (widget, CTK_WIDGET (container));
      return;
    }

  if (priv->has_entry && !CTK_IS_ENTRY (widget))
    {
      g_warning ("Attempting to add a widget with type %s to a CtkComboBox that needs an entry "
                 "(need an instance of CtkEntry or of a subclass)",
                 G_OBJECT_TYPE_NAME (widget));
      return;
    }

  if (priv->cell_view)
    {
      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (priv->cell_view)),
                            priv->cell_view);
      _ctk_bin_set_child (CTK_BIN (container), NULL);
      priv->cell_view = NULL;
    }
  
  ctk_box_pack_start (CTK_BOX (priv->box), widget, TRUE, TRUE, 0);
  _ctk_bin_set_child (CTK_BIN (container), widget);

  if (priv->has_entry)
    {
      g_signal_connect (widget, "changed",
                        G_CALLBACK (ctk_combo_box_entry_contents_changed),
                        combo_box);

      ctk_entry_set_has_frame (CTK_ENTRY (widget), priv->has_frame);
    }
}

static void
ctk_combo_box_remove (CtkContainer *container,
                      CtkWidget    *widget)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (container);
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreePath *path;
  gboolean appears_as_list;

  if (priv->has_entry)
    {
      CtkWidget *child_widget;

      child_widget = ctk_bin_get_child (CTK_BIN (container));
      if (widget && widget == child_widget)
        {
          g_signal_handlers_disconnect_by_func (widget,
                                                ctk_combo_box_entry_contents_changed,
                                                container);
        }
    }

  ctk_container_remove (CTK_CONTAINER (priv->box), widget);
  _ctk_bin_set_child (CTK_BIN (container), NULL);

  if (ctk_widget_in_destruction (CTK_WIDGET (combo_box)))
    return;

  ctk_widget_queue_resize (CTK_WIDGET (container));

  if (!priv->tree_view)
    appears_as_list = FALSE;
  else
    appears_as_list = TRUE;
  
  if (appears_as_list)
    ctk_combo_box_list_destroy (combo_box);
  else if (CTK_IS_MENU (priv->popup_widget))
    {
      ctk_combo_box_menu_destroy (combo_box);
      ctk_menu_detach (CTK_MENU (priv->popup_widget));
      priv->popup_widget = NULL;
    }

  ctk_combo_box_create_child (combo_box);

  if (appears_as_list)
    ctk_combo_box_list_setup (combo_box);
  else
    ctk_combo_box_menu_setup (combo_box);

  if (ctk_tree_row_reference_valid (priv->active_row))
    {
      path = ctk_tree_row_reference_get_path (priv->active_row);
      ctk_combo_box_set_active_internal (combo_box, path);
      ctk_tree_path_free (path);
    }
  else
    ctk_combo_box_set_active_internal (combo_box, NULL);
}

static void
ctk_combo_box_menu_show (CtkWidget *menu,
                         gpointer   user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);
  CtkComboBoxPrivate *priv = combo_box->priv;

  ctk_combo_box_child_show (menu, user_data);

  priv->popup_in_progress = TRUE;
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->button),
                                TRUE);
  priv->popup_in_progress = FALSE;
}

static void
ctk_combo_box_menu_hide (CtkWidget *menu,
                         gpointer   user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);

  ctk_combo_box_child_hide (menu,user_data);

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (combo_box->priv->button),
                                FALSE);
}

static void
ctk_combo_box_detacher (CtkWidget *widget,
                        CtkMenu          *menu)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);
  CtkComboBoxPrivate *priv = combo_box->priv;

  g_return_if_fail (priv->popup_widget == (CtkWidget *) menu);

  if (menu->priv->toplevel)
    {
      g_signal_handlers_disconnect_by_func (menu->priv->toplevel,
                                            ctk_combo_box_menu_show,
                                            combo_box);
      g_signal_handlers_disconnect_by_func (menu->priv->toplevel,
                                            ctk_combo_box_menu_hide,
                                            combo_box);
    }

  priv->popup_widget = NULL;
}

static gboolean
ctk_combo_box_grab_broken_event (CtkWidget          *widget,
                                 CdkEventGrabBroken *event,
                                 gpointer            user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);

  if (event->grab_window == NULL)
    ctk_combo_box_popdown (combo_box);

  return TRUE;
}

static void
ctk_combo_box_set_popup_widget (CtkComboBox *combo_box,
                                CtkWidget   *popup)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (CTK_IS_MENU (priv->popup_widget))
    {
      ctk_menu_detach (CTK_MENU (priv->popup_widget));
      priv->popup_widget = NULL;
    }
  else if (priv->popup_widget)
    {
      ctk_container_remove (CTK_CONTAINER (priv->scrolled_window),
                            priv->popup_widget);
      g_object_unref (priv->popup_widget);
      priv->popup_widget = NULL;
    }

  if (CTK_IS_MENU (popup))
    {
      if (priv->popup_window)
        {
          ctk_widget_destroy (priv->popup_window);
          priv->popup_window = NULL;
        }

      priv->popup_widget = popup;

      /*
       * Note that we connect to show/hide on the toplevel, not the
       * menu itself, since the menu is not shown/hidden when it is
       * popped up while torn-off.
       */
      g_signal_connect (CTK_MENU (popup)->priv->toplevel, "show",
                        G_CALLBACK (ctk_combo_box_menu_show), combo_box);
      g_signal_connect (CTK_MENU (popup)->priv->toplevel, "hide",
                        G_CALLBACK (ctk_combo_box_menu_hide), combo_box);

      ctk_menu_attach_to_widget (CTK_MENU (popup),
                                 CTK_WIDGET (combo_box),
                                 ctk_combo_box_detacher);
    }
  else
    {
      if (!priv->popup_window)
        {
          priv->popup_window = ctk_window_new (CTK_WINDOW_POPUP);
          ctk_widget_set_name (priv->popup_window, "ctk-combobox-popup-window");

          ctk_window_set_type_hint (CTK_WINDOW (priv->popup_window),
                                    CDK_WINDOW_TYPE_HINT_COMBO);
          ctk_window_set_modal (CTK_WINDOW (priv->popup_window), TRUE);

          g_signal_connect (priv->popup_window, "show",
                            G_CALLBACK (ctk_combo_box_child_show),
                            combo_box);
          g_signal_connect (priv->popup_window, "hide",
                            G_CALLBACK (ctk_combo_box_child_hide),
                            combo_box);
          g_signal_connect (priv->popup_window, "grab-broken-event",
                            G_CALLBACK (ctk_combo_box_grab_broken_event),
                            combo_box);

          ctk_window_set_resizable (CTK_WINDOW (priv->popup_window), FALSE);

          priv->scrolled_window = ctk_scrolled_window_new (NULL, NULL);

          ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (priv->scrolled_window),
                                          CTK_POLICY_NEVER,
                                          CTK_POLICY_NEVER);
          ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (priv->scrolled_window),
                                               CTK_SHADOW_IN);

          ctk_widget_show (priv->scrolled_window);

          ctk_container_add (CTK_CONTAINER (priv->popup_window),
                             priv->scrolled_window);
        }

      ctk_container_add (CTK_CONTAINER (priv->scrolled_window),
                         popup);

      ctk_widget_show (popup);
      g_object_ref (popup);
      priv->popup_widget = popup;
    }
}

static void
ctk_combo_box_list_position (CtkComboBox *combo_box,
                             gint        *x,
                             gint        *y,
                             gint        *width,
                             gint        *height)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkAllocation content_allocation;
  CdkDisplay *display;
  CdkMonitor *monitor;
  CdkRectangle area;
  CtkRequisition popup_req;
  CtkPolicyType hpolicy, vpolicy;
  CdkWindow *window;

  /* under windows, the drop down list is as wide as the combo box itself.
     see bug #340204 */
  CtkWidget *widget = CTK_WIDGET (combo_box);

  ctk_css_gadget_get_content_allocation (priv->gadget, &content_allocation, NULL);

  *x = content_allocation.x;
  *y = content_allocation.y;
  *width = content_allocation.width;

  window = ctk_widget_get_window (CTK_WIDGET (combo_box));
  cdk_window_get_root_coords (window, *x, *y, x, y);

  hpolicy = vpolicy = CTK_POLICY_NEVER;
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (priv->scrolled_window),
                                  hpolicy, vpolicy);

  if (priv->popup_fixed_width)
    {
      ctk_widget_get_preferred_size (priv->scrolled_window, &popup_req, NULL);

      if (popup_req.width > *width)
        {
          hpolicy = CTK_POLICY_ALWAYS;
          ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (priv->scrolled_window),
                                          hpolicy, vpolicy);
        }
    }
  else
    {
      /* XXX This code depends on treeviews properly reporting their natural width
       * list-mode menus won't fill up to their natural width until then */
      ctk_widget_get_preferred_size (priv->scrolled_window, NULL, &popup_req);

      if (popup_req.width > *width)
        *width = popup_req.width;
    }

  *height = popup_req.height;

  display = ctk_widget_get_display (widget);
  monitor = cdk_display_get_monitor_at_window (display, window);
  cdk_monitor_get_workarea (monitor, &area);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    *x = *x + content_allocation.width - *width;

  if (*x < area.x)
    *x = area.x;
  else if (*x + *width > area.x + area.width)
    *x = area.x + area.width - *width;

  if (*y + content_allocation.height + *height <= area.y + area.height)
    *y += content_allocation.height;
  else if (*y - *height >= area.y)
    *y -= *height;
  else if (area.y + area.height - (*y + content_allocation.height) > *y - area.y)
    {
      *y += content_allocation.height;
      *height = area.y + area.height - *y;
    }
  else
    {
      *height = *y - area.y;
      *y = area.y;
    }

  if (popup_req.height > *height)
    {
      vpolicy = CTK_POLICY_ALWAYS;

      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (priv->scrolled_window),
                                      hpolicy, vpolicy);
    }
}

static gboolean
cell_layout_is_sensitive (CtkCellLayout *layout)
{
  GList *cells, *list;
  gboolean sensitive;

  cells = ctk_cell_layout_get_cells (layout);

  sensitive = FALSE;
  for (list = cells; list; list = list->next)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);

      if (sensitive)
        break;
    }
  g_list_free (cells);

  return sensitive;
}

static gboolean
cell_is_sensitive (CtkCellRenderer *cell,
                   gpointer         data)
{
  gboolean *sensitive = data;

  g_object_get (cell, "sensitive", sensitive, NULL);

  return *sensitive;
}

static gboolean
tree_column_row_is_sensitive (CtkComboBox *combo_box,
                              CtkTreeIter *iter)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (priv->row_separator_func)
    {
      if (priv->row_separator_func (priv->model, iter,
                                    priv->row_separator_data))
        return FALSE;
    }

  if (priv->area)
    {
      gboolean sensitive;

      ctk_cell_area_apply_attributes (priv->area, priv->model, iter, FALSE, FALSE);

      sensitive = FALSE;

      ctk_cell_area_foreach (priv->area, cell_is_sensitive, &sensitive);

      return sensitive;
    }

  return TRUE;
}

static void
update_menu_sensitivity (CtkComboBox *combo_box,
                         CtkWidget   *menu)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  GList *children, *child;
  CtkWidget *item, *submenu;
  CtkWidget *cell_view;
  gboolean sensitive;

  if (!priv->model)
    return;

  children = ctk_container_get_children (CTK_CONTAINER (menu));

  for (child = children; child; child = child->next)
    {
      item = CTK_WIDGET (child->data);
      cell_view = ctk_bin_get_child (CTK_BIN (item));

      if (!CTK_IS_CELL_VIEW (cell_view))
        continue;

      submenu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (item));
      if (submenu != NULL)
        {
          ctk_widget_set_sensitive (item, TRUE);
          update_menu_sensitivity (combo_box, submenu);
        }
      else
        {
          sensitive = cell_layout_is_sensitive (CTK_CELL_LAYOUT (cell_view));
          ctk_widget_set_sensitive (item, sensitive);
        }
    }

  g_list_free (children);
}

static void
ctk_combo_box_menu_popup (CtkComboBox    *combo_box,
                          const CdkEvent *trigger_event)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  gint active_item;
  CtkAllocation border_allocation;
  CtkAllocation content_allocation;
  CtkWidget *active;

  update_menu_sensitivity (combo_box, priv->popup_widget);

  active_item = -1;
  if (ctk_tree_row_reference_valid (priv->active_row))
    {
      CtkTreePath *path;

      path = ctk_tree_row_reference_get_path (priv->active_row);
      active_item = ctk_tree_path_get_indices (path)[0];
      ctk_tree_path_free (path);

      if (priv->add_tearoffs)
        active_item++;
    }

  /* FIXME handle nested menus better */
  ctk_menu_set_active (CTK_MENU (priv->popup_widget), active_item);

  if (priv->wrap_width == 0)
    {
      gint width, min_width, nat_width;

      ctk_css_gadget_get_content_allocation (priv->gadget, &content_allocation, NULL);
      width = content_allocation.width;
      ctk_widget_set_size_request (priv->popup_widget, -1, -1);
      ctk_widget_get_preferred_width (priv->popup_widget, &min_width, &nat_width);

      if (priv->popup_fixed_width)
        width = MAX (width, min_width);
      else
        width = MAX (width, nat_width);

      ctk_widget_set_size_request (priv->popup_widget, width, -1);
    }

  g_signal_handlers_disconnect_by_func (priv->popup_widget,
                                        ctk_menu_update_scroll_offset,
                                        NULL);

  g_object_set (priv->popup_widget, "menu-type-hint", CDK_WINDOW_TYPE_HINT_COMBO, NULL);

  if (priv->wrap_width > 0 || priv->cell_view == NULL)
    {
      ctk_css_gadget_get_border_allocation (priv->gadget, &border_allocation, NULL);
      ctk_css_gadget_get_content_allocation (priv->gadget, &content_allocation, NULL);

      g_object_set (priv->popup_widget,
                    "anchor-hints", (CDK_ANCHOR_FLIP_Y |
                                     CDK_ANCHOR_SLIDE |
                                     CDK_ANCHOR_RESIZE),
                    "rect-anchor-dx", border_allocation.x - content_allocation.x,
                    NULL);

      ctk_menu_popup_at_widget (CTK_MENU (priv->popup_widget),
                                ctk_bin_get_child (CTK_BIN (combo_box)),
                                CDK_GRAVITY_SOUTH_WEST,
                                CDK_GRAVITY_NORTH_WEST,
                                trigger_event);
    }
  else
    {
      /* FIXME handle nested menus better */
      gint rect_anchor_dy = -2;
      GList *i;
      CtkWidget *child;

      active = ctk_menu_get_active (CTK_MENU (priv->popup_widget));

      if (!(active && ctk_widget_get_visible (active)))
        {
          for (i = CTK_MENU_SHELL (priv->popup_widget)->priv->children; i && !active; i = i->next)
            {
              child = i->data;

              if (child && ctk_widget_get_visible (child))
                active = child;
            }
        }

      if (active)
        {
          gint child_height;

          for (i = CTK_MENU_SHELL (priv->popup_widget)->priv->children; i && i->data != active; i = i->next)
            {
              child = i->data;

              if (child && ctk_widget_get_visible (child))
                {
                  ctk_widget_get_preferred_height (child, &child_height, NULL);
                  rect_anchor_dy -= child_height;
                }
            }

          ctk_widget_get_preferred_height (active, &child_height, NULL);
          rect_anchor_dy -= child_height / 2;
        }

      g_object_set (priv->popup_widget,
                    "anchor-hints", (CDK_ANCHOR_SLIDE |
                                     CDK_ANCHOR_RESIZE),
                    "rect-anchor-dy", rect_anchor_dy,
                    NULL);

      g_signal_connect (priv->popup_widget,
                        "popped-up",
                        G_CALLBACK (ctk_menu_update_scroll_offset),
                        NULL);

      ctk_menu_popup_at_widget (CTK_MENU (priv->popup_widget),
                                CTK_WIDGET (combo_box),
                                CDK_GRAVITY_WEST,
                                CDK_GRAVITY_NORTH_WEST,
                                trigger_event);
    }

    /* Re-get the active item before selecting it, as a popped-up handler – like
     * that of FileChooserButton in folder mode – can refilter the model, making
     * the original active item pointer invalid. This seems ugly and makes some
     * of the above code pointless in such cases, so hopefully we can FIXME. */
    active = ctk_menu_get_active (CTK_MENU (priv->popup_widget));
    if (active && ctk_widget_get_visible (active))
      ctk_menu_shell_select_item (CTK_MENU_SHELL (priv->popup_widget), active);
}

static gboolean
popup_grab_on_window (CdkWindow *window,
                      CdkDevice *pointer)
{
  CdkGrabStatus status;
  CdkSeat *seat;

  seat = cdk_device_get_seat (pointer);
  status = cdk_seat_grab (seat, window,
                          CDK_SEAT_CAPABILITY_ALL, TRUE,
                          NULL, NULL, NULL, NULL);

  return status == CDK_GRAB_SUCCESS;
}

/**
 * ctk_combo_box_popup:
 * @combo_box: a #CtkComboBox
 *
 * Pops up the menu or dropdown list of @combo_box.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Before calling this, @combo_box must be mapped, or nothing will happen.
 *
 * Since: 2.4
 */
void
ctk_combo_box_popup (CtkComboBox *combo_box)
{
  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  if (ctk_widget_get_mapped (CTK_WIDGET (combo_box)))
    g_signal_emit (combo_box, combo_box_signals[POPUP], 0);
}

/**
 * ctk_combo_box_popup_for_device:
 * @combo_box: a #CtkComboBox
 * @device: a #CdkDevice
 *
 * Pops up the menu or dropdown list of @combo_box, the popup window
 * will be grabbed so only @device and its associated pointer/keyboard
 * are the only #CdkDevices able to send events to it.
 *
 * Since: 3.0
 **/
void
ctk_combo_box_popup_for_device (CtkComboBox *combo_box,
                                CdkDevice   *device)
{
  CtkComboBoxPrivate *priv;
  gint x, y, width, height;
  CtkTreePath *path = NULL, *ppath;
  CtkWidget *toplevel;
  CdkDevice *pointer;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (CDK_IS_DEVICE (device));

  priv = combo_box->priv;

  if (!ctk_widget_get_realized (CTK_WIDGET (combo_box)))
    return;

  if (ctk_widget_get_mapped (priv->popup_widget))
    return;

  if (priv->grab_pointer)
    return;

  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    pointer = cdk_device_get_associated_device (device);
  else
    pointer = device;

  if (CTK_IS_MENU (priv->popup_widget))
    {
      ctk_combo_box_menu_popup (combo_box, priv->trigger_event);
      return;
    }

  _ctk_tooltip_hide (CTK_WIDGET (combo_box));
  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (combo_box));
  if (CTK_IS_WINDOW (toplevel))
    {
      ctk_window_group_add_window (ctk_window_get_group (CTK_WINDOW (toplevel)),
                                   CTK_WINDOW (priv->popup_window));
      ctk_window_set_transient_for (CTK_WINDOW (priv->popup_window),
                                    CTK_WINDOW (toplevel));
    }

  ctk_combo_box_list_position (combo_box, &x, &y, &width, &height);

  ctk_widget_set_size_request (priv->popup_window, width, height);
  ctk_window_move (CTK_WINDOW (priv->popup_window), x, y);

  if (ctk_tree_row_reference_valid (priv->active_row))
    {
      path = ctk_tree_row_reference_get_path (priv->active_row);
      ppath = ctk_tree_path_copy (path);
      if (ctk_tree_path_up (ppath))
        ctk_tree_view_expand_to_path (CTK_TREE_VIEW (priv->tree_view),
                                      ppath);
      ctk_tree_path_free (ppath);
    }
  ctk_tree_view_set_hover_expand (CTK_TREE_VIEW (priv->tree_view),
                                  TRUE);

  /* popup */
  ctk_window_set_screen (CTK_WINDOW (priv->popup_window),
                         ctk_widget_get_screen (CTK_WIDGET (combo_box)));
  ctk_widget_show (priv->popup_window);

  if (path)
    {
      ctk_tree_view_set_cursor (CTK_TREE_VIEW (priv->tree_view),
                                path, NULL, FALSE);
      ctk_tree_path_free (path);
    }

  ctk_widget_grab_focus (priv->popup_window);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->button),
                                TRUE);

  if (!ctk_widget_has_focus (priv->tree_view))
    ctk_widget_grab_focus (priv->tree_view);

  if (!popup_grab_on_window (ctk_widget_get_window (priv->popup_window), pointer))
    {
      ctk_widget_hide (priv->popup_window);
      return;
    }

  priv->grab_pointer = pointer;
}

static void
ctk_combo_box_real_popup (CtkComboBox *combo_box)
{
  CdkDevice *device;

  device = ctk_get_current_event_device ();

  if (!device)
    {
      CdkDisplay *display;

      /* No device was set, pick the first master device */
      display = ctk_widget_get_display (CTK_WIDGET (combo_box));
      device = cdk_seat_get_pointer (cdk_display_get_default_seat (display));
    }

  ctk_combo_box_popup_for_device (combo_box, device);
}

static gboolean
ctk_combo_box_real_popdown (CtkComboBox *combo_box)
{
  if (combo_box->priv->popup_shown)
    {
      ctk_combo_box_popdown (combo_box);
      return TRUE;
    }

  return FALSE;
}

/**
 * ctk_combo_box_popdown:
 * @combo_box: a #CtkComboBox
 *
 * Hides the menu or dropdown list of @combo_box.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Since: 2.4
 */
void
ctk_combo_box_popdown (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  if (CTK_IS_MENU (priv->popup_widget))
    {
      ctk_menu_popdown (CTK_MENU (priv->popup_widget));
      return;
    }

  if (!ctk_widget_get_realized (CTK_WIDGET (combo_box)))
    return;

  if (!priv->popup_window)
    return;

  if (!ctk_widget_is_drawable (priv->popup_window))
    return;

  if (priv->grab_pointer)
    cdk_seat_ungrab (cdk_device_get_seat (priv->grab_pointer));

  ctk_widget_hide (priv->popup_window);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->button),
                                FALSE);

  if (priv->scroll_timer)
    {
      g_source_remove (priv->scroll_timer);
      priv->scroll_timer = 0;
    }

  priv->grab_pointer = NULL;
}

static void
ctk_combo_box_unset_model (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (priv->model)
    {
      g_signal_handlers_disconnect_by_func (priv->model,
                                            ctk_combo_box_model_row_inserted,
                                            combo_box);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            ctk_combo_box_model_row_deleted,
                                            combo_box);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            ctk_combo_box_model_rows_reordered,
                                            combo_box);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            ctk_combo_box_model_row_changed,
                                            combo_box);

      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->active_row)
    {
      ctk_tree_row_reference_free (priv->active_row);
      priv->active_row = NULL;
    }

  if (priv->cell_view)
    ctk_cell_view_set_model (CTK_CELL_VIEW (priv->cell_view), NULL);
}

static void
ctk_combo_box_forall (CtkContainer *container,
                      gboolean      include_internals,
                      CtkCallback   callback,
                      gpointer      callback_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (container);
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkWidget *child;

  if (include_internals)
    {
      if (priv->box)
        (* callback) (priv->box, callback_data);
    }

  child = ctk_bin_get_child (CTK_BIN (container));
  if (child && child != priv->cell_view)
    (* callback) (child, callback_data);
}

static void
ctk_combo_box_child_show (CtkWidget *widget,
                          CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  priv->popup_shown = TRUE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

static void
ctk_combo_box_child_hide (CtkWidget *widget,
                          CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  priv->popup_shown = FALSE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

typedef struct {
  CtkComboBox *combo;
  CtkTreePath *path;
  CtkTreeIter iter;
  gboolean found;
  gboolean set;
} SearchData;

static gboolean
tree_next_func (CtkTreeModel *model,
                CtkTreePath  *path,
                CtkTreeIter  *iter,
                gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (search_data->found)
    {
      if (!tree_column_row_is_sensitive (search_data->combo, iter))
        return FALSE;

      search_data->set = TRUE;
      search_data->iter = *iter;

      return TRUE;
    }

  if (ctk_tree_path_compare (path, search_data->path) == 0)
    search_data->found = TRUE;

  return FALSE;
}

static gboolean
tree_next (CtkComboBox  *combo,
           CtkTreeModel *model,
           CtkTreeIter  *iter,
           CtkTreeIter  *next)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = ctk_tree_model_get_path (model, iter);
  search_data.found = FALSE;
  search_data.set = FALSE;

  ctk_tree_model_foreach (model, tree_next_func, &search_data);

  *next = search_data.iter;

  ctk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_prev_func (CtkTreeModel *model,
                CtkTreePath  *path,
                CtkTreeIter  *iter,
                gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (ctk_tree_path_compare (path, search_data->path) == 0)
    {
      search_data->found = TRUE;
      return TRUE;
    }

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;

  search_data->set = TRUE;
  search_data->iter = *iter;

  return FALSE;
}

static gboolean
tree_prev (CtkComboBox  *combo,
           CtkTreeModel *model,
           CtkTreeIter  *iter,
           CtkTreeIter  *prev)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = ctk_tree_model_get_path (model, iter);
  search_data.found = FALSE;
  search_data.set = FALSE;

  ctk_tree_model_foreach (model, tree_prev_func, &search_data);

  *prev = search_data.iter;

  ctk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_last_func (CtkTreeModel *model,
                CtkTreePath  *path,
                CtkTreeIter  *iter,
                gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;

  search_data->set = TRUE;
  search_data->iter = *iter;

  return FALSE;
}

static gboolean
tree_last (CtkComboBox  *combo,
           CtkTreeModel *model,
           CtkTreeIter  *last)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.set = FALSE;

  ctk_tree_model_foreach (model, tree_last_func, &search_data);

  *last = search_data.iter;

  return search_data.set;
}


static gboolean
tree_first_func (CtkTreeModel *model,
                 CtkTreePath  *path,
                 CtkTreeIter  *iter,
                 gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;

  search_data->set = TRUE;
  search_data->iter = *iter;

  return TRUE;
}

static gboolean
tree_first (CtkComboBox  *combo,
            CtkTreeModel *model,
            CtkTreeIter  *first)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.set = FALSE;

  ctk_tree_model_foreach (model, tree_first_func, &search_data);

  *first = search_data.iter;

  return search_data.set;
}

static gboolean
ctk_combo_box_scroll_event (CtkWidget          *widget,
                            CdkEventScroll     *event)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);
  CtkComboBoxPrivate *priv = combo_box->priv;
  gboolean found;
  CtkTreeIter iter;
  CtkTreeIter new_iter;

  if (!ctk_combo_box_get_active_iter (combo_box, &iter))
    return TRUE;

  if (event->direction == CDK_SCROLL_UP)
    found = tree_prev (combo_box, priv->model,
                       &iter, &new_iter);
  else
    found = tree_next (combo_box, priv->model,
                       &iter, &new_iter);

  if (found)
    ctk_combo_box_set_active_iter (combo_box, &new_iter);

  return TRUE;
}

/*
 * menu style
 */
static gboolean
ctk_combo_box_row_separator_func (CtkTreeModel      *model,
                                  CtkTreeIter       *iter,
                                  CtkComboBox       *combo)
{
  CtkComboBoxPrivate *priv = combo->priv;

  if (priv->row_separator_func)
    return priv->row_separator_func (model, iter, priv->row_separator_data);

  return FALSE;
}

static void
ctk_combo_box_menu_setup (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkWidget *menu;

  g_signal_connect (priv->button, "button-press-event",
                    G_CALLBACK (ctk_combo_box_menu_button_press),
                    combo_box);
  g_signal_connect (priv->button, "state-flags-changed",
                    G_CALLBACK (ctk_combo_box_button_state_flags_changed),
                    combo_box);

  /* create our funky menu */
  menu = _ctk_tree_menu_new_with_area (priv->area);
  ctk_widget_set_name (menu, "ctk-combobox-popup-menu");

  _ctk_tree_menu_set_model (CTK_TREE_MENU (menu), priv->model);

  _ctk_tree_menu_set_wrap_width (CTK_TREE_MENU (menu), priv->wrap_width);
  _ctk_tree_menu_set_row_span_column (CTK_TREE_MENU (menu), priv->row_column);
  _ctk_tree_menu_set_column_span_column (CTK_TREE_MENU (menu), priv->col_column);
  _ctk_tree_menu_set_tearoff (CTK_TREE_MENU (menu), priv->add_tearoffs);

  g_signal_connect (menu, "menu-activate",
                    G_CALLBACK (ctk_combo_box_menu_activate), combo_box);

  /* Chain our row_separator_func through */
  _ctk_tree_menu_set_row_separator_func (CTK_TREE_MENU (menu),
                                         (CtkTreeViewRowSeparatorFunc)ctk_combo_box_row_separator_func,
                                         combo_box, NULL);

  g_signal_connect (menu, "key-press-event",
                    G_CALLBACK (ctk_combo_box_menu_key_press), combo_box);
  ctk_combo_box_set_popup_widget (combo_box, menu);

  ctk_combo_box_update_title (combo_box);
}

static void
ctk_combo_box_menu_destroy (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  g_signal_handlers_disconnect_by_func (priv->button,
                                        ctk_combo_box_menu_button_press,
                                        combo_box);
  g_signal_handlers_disconnect_by_func (priv->button,
                                        ctk_combo_box_button_state_flags_changed,
                                        combo_box);
  g_signal_handlers_disconnect_by_data (priv->popup_widget, combo_box);

  /* changing the popup window will unref the menu and the children */
}

/* callbacks */
static gboolean
ctk_combo_box_menu_button_press (CtkWidget      *widget,
                                 CdkEventButton *event,
                                 gpointer        user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (CTK_IS_MENU (priv->popup_widget) &&
      event->type == CDK_BUTTON_PRESS && event->button == CDK_BUTTON_PRIMARY)
    {
      if (ctk_widget_get_focus_on_click (CTK_WIDGET (combo_box)) &&
          !ctk_widget_has_focus (priv->button))
        ctk_widget_grab_focus (priv->button);

      ctk_combo_box_menu_popup (combo_box, (const CdkEvent *) event);

      return TRUE;
    }

  return FALSE;
}

static void
ctk_combo_box_menu_activate (CtkWidget   *menu,
                             const gchar *path,
                             CtkComboBox *combo_box)
{
  CtkTreeIter iter;

  if (ctk_tree_model_get_iter_from_string (combo_box->priv->model, &iter, path))
    ctk_combo_box_set_active_iter (combo_box, &iter);

  g_object_set (combo_box,
                "editing-canceled", FALSE,
                NULL);
}


static void
ctk_combo_box_update_sensitivity (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreeIter iter;
  gboolean sensitive = TRUE; /* fool code checkers */

  if (!priv->button)
    return;

  switch (priv->button_sensitivity)
    {
      case CTK_SENSITIVITY_ON:
        sensitive = TRUE;
        break;
      case CTK_SENSITIVITY_OFF:
        sensitive = FALSE;
        break;
      case CTK_SENSITIVITY_AUTO:
        sensitive = priv->model &&
                    ctk_tree_model_get_iter_first (priv->model, &iter);
        break;
      default:
        g_assert_not_reached ();
        break;
    }

  ctk_widget_set_sensitive (priv->button, sensitive);
}

static void
ctk_combo_box_model_row_inserted (CtkTreeModel     *model,
                                  CtkTreePath      *path,
                                  CtkTreeIter      *iter,
                                  gpointer          user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);

  if (combo_box->priv->tree_view)
    ctk_combo_box_list_popup_resize (combo_box);

  ctk_combo_box_update_sensitivity (combo_box);
}

static void
ctk_combo_box_model_row_deleted (CtkTreeModel     *model,
                                 CtkTreePath      *path,
                                 gpointer          user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (!ctk_tree_row_reference_valid (priv->active_row))
    {
      if (priv->cell_view)
        ctk_cell_view_set_displayed_row (CTK_CELL_VIEW (priv->cell_view), NULL);
      g_signal_emit (combo_box, combo_box_signals[CHANGED], 0);
    }
  
  if (priv->tree_view)
    ctk_combo_box_list_popup_resize (combo_box);

  ctk_combo_box_update_sensitivity (combo_box);
}

static void
ctk_combo_box_model_rows_reordered (CtkTreeModel    *model,
                                    CtkTreePath     *path,
                                    CtkTreeIter     *iter,
                                    gint            *new_order,
                                    gpointer         user_data)
{
  ctk_tree_row_reference_reordered (G_OBJECT (user_data), path, iter, new_order);
}

static void
ctk_combo_box_model_row_changed (CtkTreeModel     *model,
                                 CtkTreePath      *path,
                                 CtkTreeIter      *iter,
                                 gpointer          user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreePath *active_path;

  /* FIXME this belongs to CtkCellView */
  if (ctk_tree_row_reference_valid (priv->active_row))
    {
      active_path = ctk_tree_row_reference_get_path (priv->active_row);
      if (ctk_tree_path_compare (path, active_path) == 0 &&
          priv->cell_view)
        ctk_widget_queue_resize (CTK_WIDGET (priv->cell_view));
      ctk_tree_path_free (active_path);
    }

  if (priv->tree_view)
    ctk_combo_box_list_row_changed (model, path, iter, user_data);
}

static gboolean
list_popup_resize_idle (gpointer user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);
  CtkComboBoxPrivate *priv = combo_box->priv;
  gint x, y, width, height;

  if (priv->tree_view && ctk_widget_get_mapped (priv->popup_window))
    {
      ctk_combo_box_list_position (combo_box, &x, &y, &width, &height);

      ctk_widget_set_size_request (priv->popup_window, width, height);
      ctk_window_move (CTK_WINDOW (priv->popup_window), x, y);
    }

  priv->resize_idle_id = 0;

  return FALSE;
}

static void
ctk_combo_box_list_popup_resize (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (!priv->resize_idle_id)
    {
      priv->resize_idle_id =
        cdk_threads_add_idle (list_popup_resize_idle, combo_box);
      g_source_set_name_by_id (priv->resize_idle_id, "[ctk+] list_popup_resize_idle");
    }
}

static void
ctk_combo_box_model_row_expanded (CtkTreeModel     *model,
                                  CtkTreePath      *path,
                                  CtkTreeIter      *iter,
                                  gpointer          user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);

  ctk_combo_box_list_popup_resize (combo_box);
}


/*
 * list style
 */

static void
ctk_combo_box_list_setup (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreeSelection *sel;

  g_signal_connect (priv->button, "button-press-event",
                    G_CALLBACK (ctk_combo_box_list_button_pressed), combo_box);

  priv->tree_view = ctk_tree_view_new ();
  sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
  ctk_tree_selection_set_mode (sel, CTK_SELECTION_BROWSE);
  ctk_tree_selection_set_select_function (sel,
                                          ctk_combo_box_list_select_func,
                                          NULL, NULL);
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->tree_view),
                                     FALSE);
  ctk_tree_view_set_hover_selection (CTK_TREE_VIEW (priv->tree_view),
                                     TRUE);

  ctk_tree_view_set_row_separator_func (CTK_TREE_VIEW (priv->tree_view),
                                        (CtkTreeViewRowSeparatorFunc)ctk_combo_box_row_separator_func,
                                        combo_box, NULL);

  if (priv->model)
    ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), priv->model);

  ctk_tree_view_append_column (CTK_TREE_VIEW (priv->tree_view),
                               ctk_tree_view_column_new_with_area (priv->area));

  if (ctk_tree_row_reference_valid (priv->active_row))
    {
      CtkTreePath *path;

      path = ctk_tree_row_reference_get_path (priv->active_row);
      ctk_tree_view_set_cursor (CTK_TREE_VIEW (priv->tree_view),
                                path, NULL, FALSE);
      ctk_tree_path_free (path);
    }

  /* set sample/popup widgets */
  ctk_combo_box_set_popup_widget (combo_box, priv->tree_view);

  g_signal_connect (priv->tree_view, "key-press-event",
                    G_CALLBACK (ctk_combo_box_list_key_press),
                    combo_box);
  g_signal_connect (priv->tree_view, "enter-notify-event",
                    G_CALLBACK (ctk_combo_box_list_enter_notify),
                    combo_box);
  g_signal_connect (priv->tree_view, "row-expanded",
                    G_CALLBACK (ctk_combo_box_model_row_expanded),
                    combo_box);
  g_signal_connect (priv->tree_view, "row-collapsed",
                    G_CALLBACK (ctk_combo_box_model_row_expanded),
                    combo_box);
  g_signal_connect (priv->popup_window, "button-press-event",
                    G_CALLBACK (ctk_combo_box_list_button_pressed),
                    combo_box);
  g_signal_connect (priv->popup_window, "button-release-event",
                    G_CALLBACK (ctk_combo_box_list_button_released),
                    combo_box);

  ctk_widget_show (priv->tree_view);

  ctk_combo_box_update_sensitivity (combo_box);
}

static void
ctk_combo_box_list_destroy (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  /* disconnect signals */
  g_signal_handlers_disconnect_by_func (priv->button,
                                        ctk_combo_box_list_button_pressed,
                                        combo_box);
  g_signal_handlers_disconnect_by_data (priv->tree_view, combo_box);
  g_signal_handlers_disconnect_by_data (priv->popup_window, combo_box);

  if (priv->cell_view)
    {
      g_object_set (priv->cell_view,
                    "background-set", FALSE,
                    NULL);
    }

  if (priv->scroll_timer)
    {
      g_source_remove (priv->scroll_timer);
      priv->scroll_timer = 0;
    }

  if (priv->resize_idle_id)
    {
      g_source_remove (priv->resize_idle_id);
      priv->resize_idle_id = 0;
    }

  ctk_widget_destroy (priv->tree_view);
  priv->tree_view = NULL;

  if (priv->popup_widget)
    {
      g_object_unref (priv->popup_widget);
      priv->popup_widget = NULL;
    }
}

/* callbacks */

static gboolean
ctk_combo_box_list_button_pressed (CtkWidget      *widget,
                                   CdkEventButton *event,
                                   gpointer        data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);
  CtkComboBoxPrivate *priv = combo_box->priv;

  CtkWidget *ewidget = ctk_get_event_widget ((CdkEvent *)event);

  if (ewidget == priv->popup_window)
    return TRUE;

  if (ewidget != priv->button ||
      ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->button)))
    return FALSE;

  if (ctk_widget_get_focus_on_click (CTK_WIDGET (combo_box)) &&
      !ctk_widget_has_focus (priv->button))
    ctk_widget_grab_focus (priv->button);

  ctk_combo_box_popup_for_device (combo_box, event->device);

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->button), TRUE);

  priv->auto_scroll = FALSE;
  if (priv->scroll_timer == 0) {
    priv->scroll_timer = cdk_threads_add_timeout (SCROLL_TIME,
                                                  (GSourceFunc) ctk_combo_box_list_scroll_timeout,
                                                   combo_box);
    g_source_set_name_by_id (priv->scroll_timer, "[ctk+] ctk_combo_box_list_scroll_timeout");
  }

  priv->popup_in_progress = TRUE;

  return TRUE;
}

static gboolean
ctk_combo_box_list_button_released (CtkWidget      *widget,
                                    CdkEventButton *event,
                                    gpointer        data)
{
  gboolean ret;
  CtkTreePath *path = NULL;
  CtkTreeIter iter;
  CtkTreeViewColumn *column;
  gint x;
  CdkRectangle cell_area;

  CtkComboBox *combo_box = CTK_COMBO_BOX (data);
  CtkComboBoxPrivate *priv = combo_box->priv;

  gboolean popup_in_progress = FALSE;

  CtkWidget *ewidget = ctk_get_event_widget ((CdkEvent *)event);

  if (priv->popup_in_progress)
    {
      popup_in_progress = TRUE;
      priv->popup_in_progress = FALSE;
    }

  ctk_tree_view_set_hover_expand (CTK_TREE_VIEW (priv->tree_view),
                                  FALSE);
  if (priv->scroll_timer)
    {
      g_source_remove (priv->scroll_timer);
      priv->scroll_timer = 0;
    }

  if (ewidget != priv->tree_view)
    {
      CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (priv->scrolled_window);

      if (ewidget == priv->button &&
          !popup_in_progress &&
          ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->button)))
        {
          ctk_combo_box_popdown (combo_box);

          return TRUE;
        }

      /* If released outside treeview, pop down, unless finishing a scroll */
      if (ewidget != priv->button &&
          ewidget != ctk_scrolled_window_get_hscrollbar (scrolled_window) &&
          ewidget != ctk_scrolled_window_get_vscrollbar (scrolled_window))
        {
          ctk_combo_box_popdown (combo_box);

          return TRUE;
        }

      return FALSE;
    }

  /* Determine which row was clicked and which column therein */
  ret = ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (priv->tree_view),
                                       event->x, event->y,
                                       &path, &column,
                                       &x, NULL);

  if (!ret)
    return TRUE; /* clicked outside window? */

  /* Don’t select/close after clicking row’s expander. cell_area excludes that */
  ctk_tree_view_get_cell_area (CTK_TREE_VIEW (priv->tree_view),
                               path, column, &cell_area);
  if (x >= cell_area.x && x < cell_area.x + cell_area.width)
    {
      ctk_tree_model_get_iter (priv->model, &iter, path);

      /* Use iter before popdown, as mis-users like CtkFileChooserButton alter the
       * model during notify::popped-up, which means the iterator becomes invalid.
       */
      if (tree_column_row_is_sensitive (combo_box, &iter))
        ctk_combo_box_set_active_internal (combo_box, path);

      ctk_combo_box_popdown (combo_box);
    }

  ctk_tree_path_free (path);

  return TRUE;
}

static gboolean
ctk_combo_box_menu_key_press (CtkWidget   *widget,
                              CdkEventKey *event,
                              gpointer     data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);

  if (!ctk_bindings_activate_event (G_OBJECT (widget), event))
    {
      /* The menu hasn't managed the
       * event, forward it to the combobox
       */
      ctk_bindings_activate_event (G_OBJECT (combo_box), event);
    }

  return TRUE;
}

static gboolean
ctk_combo_box_list_key_press (CtkWidget   *widget,
                              CdkEventKey *event,
                              gpointer     data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreeIter iter;

  if (event->keyval == CDK_KEY_Return || event->keyval == CDK_KEY_ISO_Enter || event->keyval == CDK_KEY_KP_Enter ||
      event->keyval == CDK_KEY_space || event->keyval == CDK_KEY_KP_Space)
  {
    CtkTreeModel *model = NULL;

    ctk_combo_box_popdown (combo_box);

    if (priv->model)
      {
        CtkTreeSelection *sel;

        sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));

        if (ctk_tree_selection_get_selected (sel, &model, &iter))
          ctk_combo_box_set_active_iter (combo_box, &iter);
      }

    return TRUE;
  }

  if (!ctk_bindings_activate_event (G_OBJECT (widget), event))
    {
      /* The list hasn't managed the
       * event, forward it to the combobox
       */
      ctk_bindings_activate_event (G_OBJECT (combo_box), event);
    }

  return TRUE;
}

static void
ctk_combo_box_list_auto_scroll (CtkComboBox *combo_box,
                                gint         x,
                                gint         y)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkAdjustment *adj;
  CtkAllocation allocation;
  gdouble value;

  ctk_widget_get_allocation (priv->tree_view, &allocation);

  adj = ctk_scrolled_window_get_hadjustment (CTK_SCROLLED_WINDOW (priv->scrolled_window));
  if (adj && ctk_adjustment_get_upper (adj) - ctk_adjustment_get_lower (adj) > ctk_adjustment_get_page_size (adj))
    {
      if (x <= allocation.x &&
          ctk_adjustment_get_lower (adj) < ctk_adjustment_get_value (adj))
        {
          value = ctk_adjustment_get_value (adj) - (allocation.x - x + 1);
          ctk_adjustment_set_value (adj, value);
        }
      else if (x >= allocation.x + allocation.width &&
               ctk_adjustment_get_upper (adj) - ctk_adjustment_get_page_size (adj) > ctk_adjustment_get_value (adj))
        {
          value = ctk_adjustment_get_value (adj) + (x - allocation.x - allocation.width + 1);
          ctk_adjustment_set_value (adj, MAX (value, 0.0));
        }
    }

  adj = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (priv->scrolled_window));
  if (adj && ctk_adjustment_get_upper (adj) - ctk_adjustment_get_lower (adj) > ctk_adjustment_get_page_size (adj))
    {
      if (y <= allocation.y &&
          ctk_adjustment_get_lower (adj) < ctk_adjustment_get_value (adj))
        {
          value = ctk_adjustment_get_value (adj) - (allocation.y - y + 1);
          ctk_adjustment_set_value (adj, value);
        }
      else if (y >= allocation.height &&
               ctk_adjustment_get_upper (adj) - ctk_adjustment_get_page_size (adj) > ctk_adjustment_get_value (adj))
        {
          value = ctk_adjustment_get_value (adj) + (y - allocation.height + 1);
          ctk_adjustment_set_value (adj, MAX (value, 0.0));
        }
    }
}

static gboolean
ctk_combo_box_list_scroll_timeout (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  gint x, y;

  if (priv->auto_scroll)
    {
      cdk_window_get_device_position (ctk_widget_get_window (priv->tree_view),
                                      priv->grab_pointer,
                                      &x, &y, NULL);
      ctk_combo_box_list_auto_scroll (combo_box, x, y);
    }

  return TRUE;
}

static gboolean
ctk_combo_box_list_enter_notify (CtkWidget        *widget,
                                 CdkEventCrossing *event,
                                 gpointer          data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);

  combo_box->priv->auto_scroll = TRUE;

  return TRUE;
}

static gboolean
ctk_combo_box_list_select_func (CtkTreeSelection *selection,
                                CtkTreeModel     *model,
                                CtkTreePath      *path,
                                gboolean          path_currently_selected,
                                gpointer          data)
{
  GList *list, *columns;
  gboolean sensitive = FALSE;

  columns = ctk_tree_view_get_columns (ctk_tree_selection_get_tree_view (selection));

  for (list = columns; list && !sensitive; list = list->next)
    {
      GList *cells, *cell;
      gboolean cell_sensitive, cell_visible;
      CtkTreeIter iter;
      CtkTreeViewColumn *column = CTK_TREE_VIEW_COLUMN (list->data);

      if (!ctk_tree_view_column_get_visible (column))
        continue;

      ctk_tree_model_get_iter (model, &iter, path);
      ctk_tree_view_column_cell_set_cell_data (column, model, &iter,
                                               FALSE, FALSE);

      cell = cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (column));
      while (cell)
        {
          g_object_get (cell->data,
                        "sensitive", &cell_sensitive,
                        "visible", &cell_visible,
                        NULL);

          if (cell_visible && cell_sensitive)
            {
              sensitive = TRUE;
              break;
            }

          cell = cell->next;
        }

      g_list_free (cells);
    }

  g_list_free (columns);

  return sensitive;
}

static void
ctk_combo_box_list_row_changed (CtkTreeModel *model,
                                CtkTreePath  *path,
                                CtkTreeIter  *iter,
                                gpointer      data)
{
  /* XXX Do nothing ? */
}

/*
 * CtkCellLayout implementation
 */
static CtkCellArea *
ctk_combo_box_cell_layout_get_area (CtkCellLayout *cell_layout)
{
  CtkComboBox *combo = CTK_COMBO_BOX (cell_layout);
  CtkComboBoxPrivate *priv = combo->priv;

  if (G_UNLIKELY (!priv->area))
    {
      priv->area = ctk_cell_area_box_new ();
      g_object_ref_sink (priv->area);
    }

  return priv->area;
}

/*
 * public API
 */

/**
 * ctk_combo_box_new:
 *
 * Creates a new empty #CtkComboBox.
 *
 * Returns: A new #CtkComboBox.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_combo_box_new (void)
{
  return g_object_new (CTK_TYPE_COMBO_BOX, NULL);
}

/**
 * ctk_combo_box_new_with_area:
 * @area: the #CtkCellArea to use to layout cell renderers
 *
 * Creates a new empty #CtkComboBox using @area to layout cells.
 *
 * Returns: A new #CtkComboBox.
 */
CtkWidget *
ctk_combo_box_new_with_area (CtkCellArea  *area)
{
  return g_object_new (CTK_TYPE_COMBO_BOX, "cell-area", area, NULL);
}

/**
 * ctk_combo_box_new_with_area_and_entry:
 * @area: the #CtkCellArea to use to layout cell renderers
 *
 * Creates a new empty #CtkComboBox with an entry.
 *
 * The new combo box will use @area to layout cells.
 *
 * Returns: A new #CtkComboBox.
 */
CtkWidget *
ctk_combo_box_new_with_area_and_entry (CtkCellArea *area)
{
  return g_object_new (CTK_TYPE_COMBO_BOX,
                       "has-entry", TRUE,
                       "cell-area", area,
                       NULL);
}


/**
 * ctk_combo_box_new_with_entry:
 *
 * Creates a new empty #CtkComboBox with an entry.
 *
 * Returns: A new #CtkComboBox.
 *
 * Since: 2.24
 */
CtkWidget *
ctk_combo_box_new_with_entry (void)
{
  return g_object_new (CTK_TYPE_COMBO_BOX, "has-entry", TRUE, NULL);
}

/**
 * ctk_combo_box_new_with_model:
 * @model: A #CtkTreeModel.
 *
 * Creates a new #CtkComboBox with the model initialized to @model.
 *
 * Returns: A new #CtkComboBox.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_combo_box_new_with_model (CtkTreeModel *model)
{
  CtkComboBox *combo_box;

  g_return_val_if_fail (CTK_IS_TREE_MODEL (model), NULL);

  combo_box = g_object_new (CTK_TYPE_COMBO_BOX, "model", model, NULL);

  return CTK_WIDGET (combo_box);
}

/**
 * ctk_combo_box_new_with_model_and_entry:
 * @model: A #CtkTreeModel
 *
 * Creates a new empty #CtkComboBox with an entry
 * and with the model initialized to @model.
 *
 * Returns: A new #CtkComboBox
 *
 * Since: 2.24
 */
CtkWidget *
ctk_combo_box_new_with_model_and_entry (CtkTreeModel *model)
{
  return g_object_new (CTK_TYPE_COMBO_BOX,
                       "has-entry", TRUE,
                       "model", model,
                       NULL);
}

/**
 * ctk_combo_box_get_wrap_width:
 * @combo_box: A #CtkComboBox
 *
 * Returns the wrap width which is used to determine the number of columns
 * for the popup menu. If the wrap width is larger than 1, the combo box
 * is in table mode.
 *
 * Returns: the wrap width.
 *
 * Since: 2.6
 */
gint
ctk_combo_box_get_wrap_width (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->wrap_width;
}

/**
 * ctk_combo_box_set_wrap_width:
 * @combo_box: A #CtkComboBox
 * @width: Preferred number of columns
 *
 * Sets the wrap width of @combo_box to be @width. The wrap width is basically
 * the preferred number of columns when you want the popup to be layed out
 * in a table.
 *
 * Since: 2.4
 */
void
ctk_combo_box_set_wrap_width (CtkComboBox *combo_box,
                              gint         width)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (width >= 0);

  priv = combo_box->priv;

  if (width != priv->wrap_width)
    {
      priv->wrap_width = width;

      ctk_combo_box_check_appearance (combo_box);

      if (CTK_IS_TREE_MENU (priv->popup_widget))
        _ctk_tree_menu_set_wrap_width (CTK_TREE_MENU (priv->popup_widget), priv->wrap_width);

      g_object_notify (G_OBJECT (combo_box), "wrap-width");
    }
}

/**
 * ctk_combo_box_get_row_span_column:
 * @combo_box: A #CtkComboBox
 *
 * Returns the column with row span information for @combo_box.
 *
 * Returns: the row span column.
 *
 * Since: 2.6
 */
gint
ctk_combo_box_get_row_span_column (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->row_column;
}

/**
 * ctk_combo_box_set_row_span_column:
 * @combo_box: A #CtkComboBox.
 * @row_span: A column in the model passed during construction.
 *
 * Sets the column with row span information for @combo_box to be @row_span.
 * The row span column contains integers which indicate how many rows
 * an item should span.
 *
 * Since: 2.4
 */
void
ctk_combo_box_set_row_span_column (CtkComboBox *combo_box,
                                   gint         row_span)
{
  CtkComboBoxPrivate *priv;
  gint col;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  col = ctk_tree_model_get_n_columns (priv->model);
  g_return_if_fail (row_span >= -1 && row_span < col);

  if (row_span != priv->row_column)
    {
      priv->row_column = row_span;

      if (CTK_IS_TREE_MENU (priv->popup_widget))
        _ctk_tree_menu_set_row_span_column (CTK_TREE_MENU (priv->popup_widget), priv->row_column);

      g_object_notify (G_OBJECT (combo_box), "row-span-column");
    }
}

/**
 * ctk_combo_box_get_column_span_column:
 * @combo_box: A #CtkComboBox
 *
 * Returns the column with column span information for @combo_box.
 *
 * Returns: the column span column.
 *
 * Since: 2.6
 */
gint
ctk_combo_box_get_column_span_column (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->col_column;
}

/**
 * ctk_combo_box_set_column_span_column:
 * @combo_box: A #CtkComboBox
 * @column_span: A column in the model passed during construction
 *
 * Sets the column with column span information for @combo_box to be
 * @column_span. The column span column contains integers which indicate
 * how many columns an item should span.
 *
 * Since: 2.4
 */
void
ctk_combo_box_set_column_span_column (CtkComboBox *combo_box,
                                      gint         column_span)
{
  CtkComboBoxPrivate *priv;
  gint col;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  col = ctk_tree_model_get_n_columns (priv->model);
  g_return_if_fail (column_span >= -1 && column_span < col);

  if (column_span != priv->col_column)
    {
      priv->col_column = column_span;

      if (CTK_IS_TREE_MENU (priv->popup_widget))
        _ctk_tree_menu_set_column_span_column (CTK_TREE_MENU (priv->popup_widget), priv->col_column);

      g_object_notify (G_OBJECT (combo_box), "column-span-column");
    }
}

/**
 * ctk_combo_box_get_active:
 * @combo_box: A #CtkComboBox
 *
 * Returns the index of the currently active item, or -1 if there’s no
 * active item. If the model is a non-flat treemodel, and the active item
 * is not an immediate child of the root of the tree, this function returns
 * `ctk_tree_path_get_indices (path)[0]`, where
 * `path` is the #CtkTreePath of the active item.
 *
 * Returns: An integer which is the index of the currently active item,
 *     or -1 if there’s no active item.
 *
 * Since: 2.4
 */
gint
ctk_combo_box_get_active (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv;
  gint result;

  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), 0);

  priv = combo_box->priv;

  if (ctk_tree_row_reference_valid (priv->active_row))
    {
      CtkTreePath *path;

      path = ctk_tree_row_reference_get_path (priv->active_row);
      result = ctk_tree_path_get_indices (path)[0];
      ctk_tree_path_free (path);
    }
  else
    result = -1;

  return result;
}

/**
 * ctk_combo_box_set_active:
 * @combo_box: A #CtkComboBox
 * @index_: An index in the model passed during construction, or -1 to have
 * no active item
 *
 * Sets the active item of @combo_box to be the item at @index.
 *
 * Since: 2.4
 */
void
ctk_combo_box_set_active (CtkComboBox *combo_box,
                          gint         index_)
{
  CtkComboBoxPrivate *priv;
  CtkTreePath *path = NULL;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (index_ >= -1);

  priv = combo_box->priv;

  if (priv->model == NULL)
    {
      /* Save index, in case the model is set after the index */
      priv->active = index_;
      if (index_ != -1)
        return;
    }

  if (index_ != -1)
    path = ctk_tree_path_new_from_indices (index_, -1);

  ctk_combo_box_set_active_internal (combo_box, path);

  if (path)
    ctk_tree_path_free (path);
}

static void
ctk_combo_box_set_active_internal (CtkComboBox *combo_box,
                                   CtkTreePath *path)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreePath *active_path;
  gint path_cmp;

  /* Remember whether the initially active row is valid. */
  gboolean is_valid_row_reference = ctk_tree_row_reference_valid (priv->active_row);

  if (path && is_valid_row_reference)
    {
      active_path = ctk_tree_row_reference_get_path (priv->active_row);
      path_cmp = ctk_tree_path_compare (path, active_path);
      ctk_tree_path_free (active_path);
      if (path_cmp == 0)
        return;
    }

  if (priv->active_row)
    {
      ctk_tree_row_reference_free (priv->active_row);
      priv->active_row = NULL;
    }

  if (!path)
    {
      if (priv->tree_view)
        ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view)));
      else
        {
          CtkMenu *menu = CTK_MENU (priv->popup_widget);

          if (CTK_IS_MENU (menu))
            ctk_menu_set_active (menu, -1);
        }

      if (priv->cell_view)
        ctk_cell_view_set_displayed_row (CTK_CELL_VIEW (priv->cell_view), NULL);

      /*
       *  Do not emit a "changed" signal when an already invalid selection was
       *  now set to invalid.
       */
      if (!is_valid_row_reference)
        return;
    }
  else
    {
      priv->active_row =
        ctk_tree_row_reference_new (priv->model, path);

      if (priv->tree_view)
        {
          ctk_tree_view_set_cursor (CTK_TREE_VIEW (priv->tree_view),
                                    path, NULL, FALSE);
        }
      else if (CTK_IS_MENU (priv->popup_widget))
        {
          /* FIXME handle nested menus better */
          ctk_menu_set_active (CTK_MENU (priv->popup_widget),
                               ctk_tree_path_get_indices (path)[0]);
        }

      if (priv->cell_view)
        ctk_cell_view_set_displayed_row (CTK_CELL_VIEW (priv->cell_view),
                                         path);
    }

  g_signal_emit (combo_box, combo_box_signals[CHANGED], 0);
  g_object_notify (G_OBJECT (combo_box), "active");
  if (priv->id_column >= 0)
    g_object_notify (G_OBJECT (combo_box), "active-id");
}


/**
 * ctk_combo_box_get_active_iter:
 * @combo_box: A #CtkComboBox
 * @iter: (out): A #CtkTreeIter
 *
 * Sets @iter to point to the currently active item, if any item is active.
 * Otherwise, @iter is left unchanged.
 *
 * Returns: %TRUE if @iter was set, %FALSE otherwise
 *
 * Since: 2.4
 */
gboolean
ctk_combo_box_get_active_iter (CtkComboBox     *combo_box,
                               CtkTreeIter     *iter)
{
  CtkComboBoxPrivate *priv;
  CtkTreePath *path;
  gboolean result;

  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), FALSE);

  priv = combo_box->priv;

  if (!ctk_tree_row_reference_valid (priv->active_row))
    return FALSE;

  path = ctk_tree_row_reference_get_path (priv->active_row);
  result = ctk_tree_model_get_iter (priv->model, iter, path);
  ctk_tree_path_free (path);

  return result;
}

/**
 * ctk_combo_box_set_active_iter:
 * @combo_box: A #CtkComboBox
 * @iter: (allow-none): The #CtkTreeIter, or %NULL
 *
 * Sets the current active item to be the one referenced by @iter, or
 * unsets the active item if @iter is %NULL.
 *
 * Since: 2.4
 */
void
ctk_combo_box_set_active_iter (CtkComboBox     *combo_box,
                               CtkTreeIter     *iter)
{
  CtkTreePath *path = NULL;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  if (iter)
    path = ctk_tree_model_get_path (ctk_combo_box_get_model (combo_box), iter);

  ctk_combo_box_set_active_internal (combo_box, path);
  ctk_tree_path_free (path);
}

/**
 * ctk_combo_box_set_model:
 * @combo_box: A #CtkComboBox
 * @model: (allow-none): A #CtkTreeModel
 *
 * Sets the model used by @combo_box to be @model. Will unset a previously set
 * model (if applicable). If model is %NULL, then it will unset the model.
 *
 * Note that this function does not clear the cell renderers, you have to
 * call ctk_cell_layout_clear() yourself if you need to set up different
 * cell renderers for the new model.
 *
 * Since: 2.4
 */
void
ctk_combo_box_set_model (CtkComboBox  *combo_box,
                         CtkTreeModel *model)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (model == NULL || CTK_IS_TREE_MODEL (model));

  priv = combo_box->priv;

  if (model == priv->model)
    return;

  ctk_combo_box_unset_model (combo_box);

  if (model == NULL)
    goto out;

  priv->model = model;
  g_object_ref (priv->model);

  g_signal_connect (priv->model, "row-inserted",
                    G_CALLBACK (ctk_combo_box_model_row_inserted),
                    combo_box);
  g_signal_connect (priv->model, "row-deleted",
                    G_CALLBACK (ctk_combo_box_model_row_deleted),
                    combo_box);
  g_signal_connect (priv->model, "rows-reordered",
                    G_CALLBACK (ctk_combo_box_model_rows_reordered),
                    combo_box);
  g_signal_connect (priv->model, "row-changed",
                    G_CALLBACK (ctk_combo_box_model_row_changed),
                    combo_box);

  if (priv->tree_view)
    {
      /* list mode */
      ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view),
                               priv->model);
      ctk_combo_box_list_popup_resize (combo_box);
    }

  if (CTK_IS_TREE_MENU (priv->popup_widget))
    {
      /* menu mode */
      _ctk_tree_menu_set_model (CTK_TREE_MENU (priv->popup_widget),
                                priv->model);
    }

  if (priv->cell_view)
    ctk_cell_view_set_model (CTK_CELL_VIEW (priv->cell_view),
                             priv->model);

  if (priv->active != -1)
    {
      /* If an index was set in advance, apply it now */
      ctk_combo_box_set_active (combo_box, priv->active);
      priv->active = -1;
    }

out:
  ctk_combo_box_update_sensitivity (combo_box);

  g_object_notify (G_OBJECT (combo_box), "model");
}

/**
 * ctk_combo_box_get_model:
 * @combo_box: A #CtkComboBox
 *
 * Returns the #CtkTreeModel which is acting as data source for @combo_box.
 *
 * Returns: (transfer none): A #CtkTreeModel which was passed
 *     during construction.
 *
 * Since: 2.4
 */
CtkTreeModel *
ctk_combo_box_get_model (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), NULL);

  return combo_box->priv->model;
}

static void
ctk_combo_box_real_move_active (CtkComboBox   *combo_box,
                                CtkScrollType  scroll)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreeIter iter;
  CtkTreeIter new_iter;
  gboolean    active_iter;
  gboolean    found;

  if (!priv->model)
    {
      ctk_widget_error_bell (CTK_WIDGET (combo_box));
      return;
    }

  active_iter = ctk_combo_box_get_active_iter (combo_box, &iter);

  switch (scroll)
    {
    case CTK_SCROLL_STEP_BACKWARD:
    case CTK_SCROLL_STEP_UP:
    case CTK_SCROLL_STEP_LEFT:
      if (active_iter)
        {
          found = tree_prev (combo_box, priv->model,
                             &iter, &new_iter);
          break;
        }
      /* else fall through */

    case CTK_SCROLL_PAGE_FORWARD:
    case CTK_SCROLL_PAGE_DOWN:
    case CTK_SCROLL_PAGE_RIGHT:
    case CTK_SCROLL_END:
      found = tree_last (combo_box, priv->model, &new_iter);
      break;

    case CTK_SCROLL_STEP_FORWARD:
    case CTK_SCROLL_STEP_DOWN:
    case CTK_SCROLL_STEP_RIGHT:
      if (active_iter)
        {
          found = tree_next (combo_box, priv->model,
                             &iter, &new_iter);
          break;
        }
      /* else fall through */

    case CTK_SCROLL_PAGE_BACKWARD:
    case CTK_SCROLL_PAGE_UP:
    case CTK_SCROLL_PAGE_LEFT:
    case CTK_SCROLL_START:
      found = tree_first (combo_box, priv->model, &new_iter);
      break;

    default:
      return;
    }

  if (found && active_iter)
    {
      CtkTreePath *old_path;
      CtkTreePath *new_path;

      old_path = ctk_tree_model_get_path (priv->model, &iter);
      new_path = ctk_tree_model_get_path (priv->model, &new_iter);

      if (ctk_tree_path_compare (old_path, new_path) == 0)
        found = FALSE;

      ctk_tree_path_free (old_path);
      ctk_tree_path_free (new_path);
    }

  if (found)
    {
      ctk_combo_box_set_active_iter (combo_box, &new_iter);
    }
  else
    {
      ctk_widget_error_bell (CTK_WIDGET (combo_box));
    }
}

static gboolean
ctk_combo_box_mnemonic_activate (CtkWidget *widget,
                                 gboolean   group_cycling)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);

  if (combo_box->priv->has_entry)
    {
      CtkWidget* child;

      child = ctk_bin_get_child (CTK_BIN (combo_box));
      if (child)
        ctk_widget_grab_focus (child);
    }
  else
    ctk_widget_grab_focus (combo_box->priv->button);

  return TRUE;
}

static void
ctk_combo_box_grab_focus (CtkWidget *widget)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);

  if (combo_box->priv->has_entry)
    {
      CtkWidget *child;

      child = ctk_bin_get_child (CTK_BIN (combo_box));
      if (child)
        ctk_widget_grab_focus (child);
    }
  else
    ctk_widget_grab_focus (combo_box->priv->button);
}

static void
ctk_combo_box_unmap (CtkWidget *widget)
{
  ctk_combo_box_popdown (CTK_COMBO_BOX (widget));

  CTK_WIDGET_CLASS (ctk_combo_box_parent_class)->unmap (widget);
}

static void
ctk_combo_box_destroy (CtkWidget *widget)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (widget);
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (priv->popup_idle_id > 0)
    {
      g_source_remove (priv->popup_idle_id);
      priv->popup_idle_id = 0;
    }

  g_clear_pointer (&priv->trigger_event, cdk_event_free);

  if (priv->box)
    {
      /* destroy things (unparent will kill the latest ref from us)
       * last unref on button will destroy the arrow
       */
      ctk_widget_unparent (priv->box);
      priv->box = NULL;
      priv->button = NULL;
      priv->arrow = NULL;
      priv->cell_view = NULL;
      _ctk_bin_set_child (CTK_BIN (combo_box), NULL);
    }

  if (priv->row_separator_destroy)
    priv->row_separator_destroy (priv->row_separator_data);

  priv->row_separator_func = NULL;
  priv->row_separator_data = NULL;
  priv->row_separator_destroy = NULL;

  CTK_WIDGET_CLASS (ctk_combo_box_parent_class)->destroy (widget);
  priv->cell_view = NULL;
}

static void
ctk_combo_box_entry_contents_changed (CtkEntry *entry,
                                      gpointer  user_data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (user_data);

  /*
   *  Fixes regression reported in bug #574059. The old functionality relied on
   *  bug #572478.  As a bugfix, we now emit the "changed" signal ourselves
   *  when the selection was already set to -1.
   */
  if (ctk_combo_box_get_active(combo_box) == -1)
    g_signal_emit_by_name (combo_box, "changed");
  else
    ctk_combo_box_set_active (combo_box, -1);
}

static void
ctk_combo_box_entry_active_changed (CtkComboBox *combo_box,
                                    gpointer     user_data)
{
  CtkTreeModel *model;
  CtkTreeIter iter;

  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    {
      CtkEntry *entry = CTK_ENTRY (ctk_bin_get_child (CTK_BIN (combo_box)));

      if (entry)
        {
          CtkTreePath *path;
          gchar       *path_str;
          gchar       *text = NULL;

          model    = ctk_combo_box_get_model (combo_box);
          path     = ctk_tree_model_get_path (model, &iter);
          path_str = ctk_tree_path_to_string (path);

          g_signal_handlers_block_by_func (entry,
                                           ctk_combo_box_entry_contents_changed,
                                           combo_box);


          g_signal_emit (combo_box, combo_box_signals[FORMAT_ENTRY_TEXT], 0,
                         path_str, &text);

          ctk_entry_set_text (entry, text);

          g_signal_handlers_unblock_by_func (entry,
                                             ctk_combo_box_entry_contents_changed,
                                             combo_box);

          ctk_tree_path_free (path);
          g_free (text);
          g_free (path_str);
        }
    }
}

static gchar *
ctk_combo_box_format_entry_text (CtkComboBox     *combo_box,
                                 const gchar     *path)
{
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkTreeModel       *model;
  CtkTreeIter         iter;
  gchar              *text = NULL;

  if (priv->text_column >= 0)
    {
      model = ctk_combo_box_get_model (combo_box);
      ctk_tree_model_get_iter_from_string (model, &iter, path);

      ctk_tree_model_get (model, &iter,
                          priv->text_column, &text,
                          -1);
    }

  return text;
}

static void
ctk_combo_box_constructed (GObject *object)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (object);
  CtkComboBoxPrivate *priv = combo_box->priv;

  G_OBJECT_CLASS (ctk_combo_box_parent_class)->constructed (object);

  if (!priv->area)
    {
      priv->area = ctk_cell_area_box_new ();
      g_object_ref_sink (priv->area);
    }

  ctk_combo_box_create_child (combo_box);

  ctk_combo_box_check_appearance (combo_box);

  if (priv->has_entry)
    {
      priv->text_renderer = ctk_cell_renderer_text_new ();
      ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo_box),
                                  priv->text_renderer, TRUE);

      ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), -1);
    }
}


static void
ctk_combo_box_dispose(GObject* object)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (object);
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (CTK_IS_MENU (priv->popup_widget))
    {
      ctk_combo_box_menu_destroy (combo_box);
      ctk_menu_detach (CTK_MENU (priv->popup_widget));
      priv->popup_widget = NULL;
    }

  if (priv->area)
    {
      g_object_unref (priv->area);
      priv->area = NULL;
    }

  if (CTK_IS_TREE_VIEW (priv->tree_view))
    ctk_combo_box_list_destroy (combo_box);

  if (priv->popup_window)
    {
      ctk_widget_destroy (priv->popup_window);
      priv->popup_window = NULL;
    }

  ctk_combo_box_unset_model (combo_box);

  G_OBJECT_CLASS (ctk_combo_box_parent_class)->dispose (object);
}

static void
ctk_combo_box_finalize (GObject *object)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (object);

  g_free (combo_box->priv->tearoff_title);
  g_clear_object (&combo_box->priv->gadget);

  G_OBJECT_CLASS (ctk_combo_box_parent_class)->finalize (object);
}

static gboolean
ctk_cell_editable_key_press (CtkWidget   *widget,
                             CdkEventKey *event,
                             gpointer     data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);

  if (event->keyval == CDK_KEY_Escape)
    {
      g_object_set (combo_box,
                    "editing-canceled", TRUE,
                    NULL);
      ctk_cell_editable_editing_done (CTK_CELL_EDITABLE (combo_box));
      ctk_cell_editable_remove_widget (CTK_CELL_EDITABLE (combo_box));

      return TRUE;
    }
  else if (event->keyval == CDK_KEY_Return ||
           event->keyval == CDK_KEY_ISO_Enter ||
           event->keyval == CDK_KEY_KP_Enter)
    {
      ctk_cell_editable_editing_done (CTK_CELL_EDITABLE (combo_box));
      ctk_cell_editable_remove_widget (CTK_CELL_EDITABLE (combo_box));

      return TRUE;
    }

  return FALSE;
}

static gboolean
popdown_idle (gpointer data)
{
  CtkComboBox *combo_box;

  combo_box = CTK_COMBO_BOX (data);

  ctk_cell_editable_editing_done (CTK_CELL_EDITABLE (combo_box));
  ctk_cell_editable_remove_widget (CTK_CELL_EDITABLE (combo_box));

  g_object_unref (combo_box);

  return FALSE;
}

static void
popdown_handler (CtkWidget *widget,
                 gpointer   data)
{
  guint id;
  id = cdk_threads_add_idle (popdown_idle, g_object_ref (data));
  g_source_set_name_by_id (id, "[ctk+] popdown_idle");
}

static gboolean
popup_idle (gpointer data)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (data);
  CtkComboBoxPrivate *priv = combo_box->priv;

  if (CTK_IS_MENU (priv->popup_widget) &&
      priv->cell_view)
    g_signal_connect_object (priv->popup_widget,
                             "unmap", G_CALLBACK (popdown_handler),
                             combo_box, 0);

  /* we unset this if a menu item is activated */
  g_object_set (combo_box,
                "editing-canceled", TRUE,
                NULL);
  ctk_combo_box_popup (combo_box);

  g_clear_pointer (&priv->trigger_event, cdk_event_free);

  priv->popup_idle_id = 0;

  return FALSE;
}

static void
ctk_combo_box_start_editing (CtkCellEditable *cell_editable,
                             CdkEvent        *event)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (cell_editable);
  CtkComboBoxPrivate *priv = combo_box->priv;
  CtkWidget *child;

  priv->is_cell_renderer = TRUE;

  if (priv->cell_view)
    {
      g_signal_connect_object (priv->button, "key-press-event",
                               G_CALLBACK (ctk_cell_editable_key_press),
                               cell_editable, 0);

      ctk_widget_grab_focus (priv->button);
    }
  else
    {
      child = ctk_bin_get_child (CTK_BIN (combo_box));

      g_signal_connect_object (child, "key-press-event",
                               G_CALLBACK (ctk_cell_editable_key_press),
                               cell_editable, 0);

      ctk_widget_grab_focus (child);
      ctk_widget_set_can_focus (priv->button, FALSE);
    }

  /* we do the immediate popup only for the optionmenu-like
   * appearance
   */
  if (priv->is_cell_renderer &&
      priv->cell_view && !priv->tree_view)
    {
      g_clear_pointer (&priv->trigger_event, cdk_event_free);

      if (event)
        priv->trigger_event = cdk_event_copy (event);
      else
        priv->trigger_event = ctk_get_current_event ();

      priv->popup_idle_id =
          cdk_threads_add_idle (popup_idle, combo_box);
      g_source_set_name_by_id (priv->popup_idle_id, "[ctk+] popup_idle");
    }
}


/**
 * ctk_combo_box_get_add_tearoffs:
 * @combo_box: a #CtkComboBox
 *
 * Gets the current value of the :add-tearoffs property.
 *
 * Returns: the current value of the :add-tearoffs property.
 *
 * Deprecated: 3.10
 */
gboolean
ctk_combo_box_get_add_tearoffs (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->add_tearoffs;
}

/**
 * ctk_combo_box_set_add_tearoffs:
 * @combo_box: a #CtkComboBox
 * @add_tearoffs: %TRUE to add tearoff menu items
 *
 * Sets whether the popup menu should have a tearoff
 * menu item.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10
 */
void
ctk_combo_box_set_add_tearoffs (CtkComboBox *combo_box,
                                gboolean     add_tearoffs)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;
  add_tearoffs = add_tearoffs != FALSE;

  if (priv->add_tearoffs != add_tearoffs)
    {
      priv->add_tearoffs = add_tearoffs;
      ctk_combo_box_check_appearance (combo_box);

      if (CTK_IS_TREE_MENU (priv->popup_widget))
        _ctk_tree_menu_set_tearoff (CTK_TREE_MENU (priv->popup_widget),
                                    priv->add_tearoffs);

      g_object_notify (G_OBJECT (combo_box), "add-tearoffs");
    }
}

/**
 * ctk_combo_box_get_title:
 * @combo_box: a #CtkComboBox
 *
 * Gets the current title of the menu in tearoff mode. See
 * ctk_combo_box_set_add_tearoffs().
 *
 * Returns: the menu’s title in tearoff mode. This is an internal copy of the
 * string which must not be freed.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10
 */
const gchar*
ctk_combo_box_get_title (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), NULL);

  return combo_box->priv->tearoff_title;
}

static void
ctk_combo_box_update_title (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv = combo_box->priv;

  ctk_combo_box_check_appearance (combo_box);

  if (priv->popup_widget && CTK_IS_MENU (priv->popup_widget))
    ctk_menu_set_title (CTK_MENU (priv->popup_widget), priv->tearoff_title);
}

/**
 * ctk_combo_box_set_title:
 * @combo_box: a #CtkComboBox
 * @title: a title for the menu in tearoff mode
 *
 * Sets the menu’s title in tearoff mode.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10
 */
void
ctk_combo_box_set_title (CtkComboBox *combo_box,
                         const gchar *title)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  if (strcmp (title ? title : "",
              priv->tearoff_title ? priv->tearoff_title : "") != 0)
    {
      g_free (priv->tearoff_title);
      priv->tearoff_title = g_strdup (title);

      ctk_combo_box_update_title (combo_box);

      g_object_notify (G_OBJECT (combo_box), "tearoff-title");
    }
}


/**
 * ctk_combo_box_set_popup_fixed_width:
 * @combo_box: a #CtkComboBox
 * @fixed: whether to use a fixed popup width
 *
 * Specifies whether the popup’s width should be a fixed width
 * matching the allocated width of the combo box.
 *
 * Since: 3.0
 **/
void
ctk_combo_box_set_popup_fixed_width (CtkComboBox *combo_box,
                                     gboolean     fixed)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  if (priv->popup_fixed_width != fixed)
    {
      priv->popup_fixed_width = fixed;

      g_object_notify (G_OBJECT (combo_box), "popup-fixed-width");
    }
}

/**
 * ctk_combo_box_get_popup_fixed_width:
 * @combo_box: a #CtkComboBox
 *
 * Gets whether the popup uses a fixed width matching
 * the allocated width of the combo box.
 *
 * Returns: %TRUE if the popup uses a fixed width
 *
 * Since: 3.0
 **/
gboolean
ctk_combo_box_get_popup_fixed_width (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->popup_fixed_width;
}


/**
 * ctk_combo_box_get_popup_accessible:
 * @combo_box: a #CtkComboBox
 *
 * Gets the accessible object corresponding to the combo box’s popup.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Returns: (transfer none): the accessible object corresponding
 *     to the combo box’s popup.
 *
 * Since: 2.6
 */
AtkObject *
ctk_combo_box_get_popup_accessible (CtkComboBox *combo_box)
{
  CtkComboBoxPrivate *priv;

  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), NULL);

  priv = combo_box->priv;

  if (priv->popup_widget)
    return ctk_widget_get_accessible (priv->popup_widget);

  return NULL;
}

/**
 * ctk_combo_box_get_row_separator_func: (skip)
 * @combo_box: a #CtkComboBox
 *
 * Returns the current row separator function.
 *
 * Returns: the current row separator function.
 *
 * Since: 2.6
 */
CtkTreeViewRowSeparatorFunc
ctk_combo_box_get_row_separator_func (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), NULL);

  return combo_box->priv->row_separator_func;
}

/**
 * ctk_combo_box_set_row_separator_func:
 * @combo_box: a #CtkComboBox
 * @func: a #CtkTreeViewRowSeparatorFunc
 * @data: (allow-none): user data to pass to @func, or %NULL
 * @destroy: (allow-none): destroy notifier for @data, or %NULL
 *
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Since: 2.6
 */
void
ctk_combo_box_set_row_separator_func (CtkComboBox                 *combo_box,
                                      CtkTreeViewRowSeparatorFunc  func,
                                      gpointer                     data,
                                      GDestroyNotify               destroy)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  if (priv->row_separator_destroy)
    priv->row_separator_destroy (priv->row_separator_data);

  priv->row_separator_func = func;
  priv->row_separator_data = data;
  priv->row_separator_destroy = destroy;

  /* Provoke the underlying treeview/menu to rebuild themselves with the new separator func */
  if (priv->tree_view)
    {
      ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), NULL);
      ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), priv->model);
    }

  if (CTK_IS_TREE_MENU (priv->popup_widget))
    {
      _ctk_tree_menu_set_model (CTK_TREE_MENU (priv->popup_widget), NULL);
      _ctk_tree_menu_set_model (CTK_TREE_MENU (priv->popup_widget), priv->model);
    }

  ctk_widget_queue_draw (CTK_WIDGET (combo_box));
}

/**
 * ctk_combo_box_set_button_sensitivity:
 * @combo_box: a #CtkComboBox
 * @sensitivity: specify the sensitivity of the dropdown button
 *
 * Sets whether the dropdown button of the combo box should be
 * always sensitive (%CTK_SENSITIVITY_ON), never sensitive (%CTK_SENSITIVITY_OFF)
 * or only if there is at least one item to display (%CTK_SENSITIVITY_AUTO).
 *
 * Since: 2.14
 **/
void
ctk_combo_box_set_button_sensitivity (CtkComboBox        *combo_box,
                                      CtkSensitivityType  sensitivity)
{
  CtkComboBoxPrivate *priv;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  if (priv->button_sensitivity != sensitivity)
    {
      priv->button_sensitivity = sensitivity;
      ctk_combo_box_update_sensitivity (combo_box);

      g_object_notify (G_OBJECT (combo_box), "button-sensitivity");
    }
}

/**
 * ctk_combo_box_get_button_sensitivity:
 * @combo_box: a #CtkComboBox
 *
 * Returns whether the combo box sets the dropdown button
 * sensitive or not when there are no items in the model.
 *
 * Returns: %CTK_SENSITIVITY_ON if the dropdown button
 *    is sensitive when the model is empty, %CTK_SENSITIVITY_OFF
 *    if the button is always insensitive or
 *    %CTK_SENSITIVITY_AUTO if it is only sensitive as long as
 *    the model has one item to be selected.
 *
 * Since: 2.14
 **/
CtkSensitivityType
ctk_combo_box_get_button_sensitivity (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->button_sensitivity;
}


/**
 * ctk_combo_box_get_has_entry:
 * @combo_box: a #CtkComboBox
 *
 * Returns whether the combo box has an entry.
 *
 * Returns: whether there is an entry in @combo_box.
 *
 * Since: 2.24
 **/
gboolean
ctk_combo_box_get_has_entry (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->has_entry;
}

/**
 * ctk_combo_box_set_entry_text_column:
 * @combo_box: A #CtkComboBox
 * @text_column: A column in @model to get the strings from for
 *     the internal entry
 *
 * Sets the model column which @combo_box should use to get strings from
 * to be @text_column. The column @text_column in the model of @combo_box
 * must be of type %G_TYPE_STRING.
 *
 * This is only relevant if @combo_box has been created with
 * #CtkComboBox:has-entry as %TRUE.
 *
 * Since: 2.24
 */
void
ctk_combo_box_set_entry_text_column (CtkComboBox *combo_box,
                                     gint         text_column)
{
  CtkComboBoxPrivate *priv;
  CtkTreeModel *model;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;
  model = ctk_combo_box_get_model (combo_box);

  g_return_if_fail (text_column >= 0);
  g_return_if_fail (model == NULL || text_column < ctk_tree_model_get_n_columns (model));

  if (priv->text_column != text_column)
    {
      priv->text_column = text_column;

      if (priv->text_renderer != NULL)
        ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo_box),
                                        priv->text_renderer,
                                        "text", text_column,
                                        NULL);

      g_object_notify (G_OBJECT (combo_box), "entry-text-column");
    }
}

/**
 * ctk_combo_box_get_entry_text_column:
 * @combo_box: A #CtkComboBox.
 *
 * Returns the column which @combo_box is using to get the strings
 * from to display in the internal entry.
 *
 * Returns: A column in the data source model of @combo_box.
 *
 * Since: 2.24
 */
gint
ctk_combo_box_get_entry_text_column (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), 0);

  return combo_box->priv->text_column;
}

/**
 * ctk_combo_box_set_focus_on_click:
 * @combo: a #CtkComboBox
 * @focus_on_click: whether the combo box grabs focus when clicked
 *    with the mouse
 *
 * Sets whether the combo box will grab focus when it is clicked with
 * the mouse. Making mouse clicks not grab focus is useful in places
 * like toolbars where you don’t want the keyboard focus removed from
 * the main area of the application.
 *
 * Since: 2.6
 *
 * Deprecated: 3.20: Use ctk_widget_set_focus_on_click() instead
 */
void
ctk_combo_box_set_focus_on_click (CtkComboBox *combo_box,
                                  gboolean     focus_on_click)
{
  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  ctk_widget_set_focus_on_click (CTK_WIDGET (combo_box), focus_on_click);
}

/**
 * ctk_combo_box_get_focus_on_click:
 * @combo: a #CtkComboBox
 *
 * Returns whether the combo box grabs focus when it is clicked
 * with the mouse. See ctk_combo_box_set_focus_on_click().
 *
 * Returns: %TRUE if the combo box grabs focus when it is
 *     clicked with the mouse.
 *
 * Since: 2.6
 *
 * Deprecated: 3.20: Use ctk_widget_get_focus_on_click() instead
 */
gboolean
ctk_combo_box_get_focus_on_click (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), FALSE);
  
  return ctk_widget_get_focus_on_click (CTK_WIDGET (combo_box));
}

static void
ctk_combo_box_buildable_add_child (CtkBuildable *buildable,
                                   CtkBuilder   *builder,
                                   GObject      *child,
                                   const gchar  *type)
{
  if (CTK_IS_WIDGET (child))
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
      return;
    }

  _ctk_cell_layout_buildable_add_child (buildable, builder, child, type);
}

static gboolean
ctk_combo_box_buildable_custom_tag_start (CtkBuildable  *buildable,
                                          CtkBuilder    *builder,
                                          GObject       *child,
                                          const gchar   *tagname,
                                          GMarkupParser *parser,
                                          gpointer      *data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, data))
    return TRUE;

  return _ctk_cell_layout_buildable_custom_tag_start (buildable, builder, child,
                                                      tagname, parser, data);
}

static void
ctk_combo_box_buildable_custom_tag_end (CtkBuildable *buildable,
                                        CtkBuilder   *builder,
                                        GObject      *child,
                                        const gchar  *tagname,
                                        gpointer     *data)
{
  if (!_ctk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname, data))
    parent_buildable_iface->custom_tag_end (buildable, builder, child, tagname, data);
}

static GObject *
ctk_combo_box_buildable_get_internal_child (CtkBuildable *buildable,
                                            CtkBuilder   *builder,
                                            const gchar  *childname)
{
  CtkComboBox *combo_box = CTK_COMBO_BOX (buildable);

  if (combo_box->priv->has_entry && strcmp (childname, "entry") == 0)
    return G_OBJECT (ctk_bin_get_child (CTK_BIN (buildable)));

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

/**
 * ctk_combo_box_set_id_column:
 * @combo_box: A #CtkComboBox
 * @id_column: A column in @model to get string IDs for values from
 *
 * Sets the model column which @combo_box should use to get string IDs
 * for values from. The column @id_column in the model of @combo_box
 * must be of type %G_TYPE_STRING.
 *
 * Since: 3.0
 */
void
ctk_combo_box_set_id_column (CtkComboBox *combo_box,
                             gint         id_column)
{
  CtkComboBoxPrivate *priv;
  CtkTreeModel *model;

  g_return_if_fail (CTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  if (id_column != priv->id_column)
    {
      model = ctk_combo_box_get_model (combo_box);

      g_return_if_fail (id_column >= 0);
      g_return_if_fail (model == NULL ||
                        id_column < ctk_tree_model_get_n_columns (model));

      priv->id_column = id_column;

      g_object_notify (G_OBJECT (combo_box), "id-column");
      g_object_notify (G_OBJECT (combo_box), "active-id");
    }
}

/**
 * ctk_combo_box_get_id_column:
 * @combo_box: A #CtkComboBox
 *
 * Returns the column which @combo_box is using to get string IDs
 * for values from.
 *
 * Returns: A column in the data source model of @combo_box.
 *
 * Since: 3.0
 */
gint
ctk_combo_box_get_id_column (CtkComboBox *combo_box)
{
  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), 0);

  return combo_box->priv->id_column;
}

/**
 * ctk_combo_box_get_active_id:
 * @combo_box: a #CtkComboBox
 *
 * Returns the ID of the active row of @combo_box.  This value is taken
 * from the active row and the column specified by the #CtkComboBox:id-column
 * property of @combo_box (see ctk_combo_box_set_id_column()).
 *
 * The returned value is an interned string which means that you can
 * compare the pointer by value to other interned strings and that you
 * must not free it.
 *
 * If the #CtkComboBox:id-column property of @combo_box is not set, or if
 * no row is active, or if the active row has a %NULL ID value, then %NULL
 * is returned.
 *
 * Returns: (nullable): the ID of the active row, or %NULL
 *
 * Since: 3.0
 **/
const gchar *
ctk_combo_box_get_active_id (CtkComboBox *combo_box)
{
  CtkTreeModel *model;
  CtkTreeIter iter;
  gint column;

  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), NULL);

  column = combo_box->priv->id_column;

  if (column < 0)
    return NULL;

  model = ctk_combo_box_get_model (combo_box);
  g_return_val_if_fail (ctk_tree_model_get_column_type (model, column) ==
                        G_TYPE_STRING, NULL);

  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    {
      const gchar *interned;
      gchar *id;

      ctk_tree_model_get (model, &iter, column, &id, -1);
      interned = g_intern_string (id);
      g_free (id);

      return interned;
    }

  return NULL;
}

/**
 * ctk_combo_box_set_active_id:
 * @combo_box: a #CtkComboBox
 * @active_id: (allow-none): the ID of the row to select, or %NULL
 *
 * Changes the active row of @combo_box to the one that has an ID equal to
 * @active_id, or unsets the active row if @active_id is %NULL.  Rows having
 * a %NULL ID string cannot be made active by this function.
 *
 * If the #CtkComboBox:id-column property of @combo_box is unset or if no
 * row has the given ID then the function does nothing and returns %FALSE.
 *
 * Returns: %TRUE if a row with a matching ID was found.  If a %NULL
 *          @active_id was given to unset the active row, the function
 *          always returns %TRUE.
 *
 * Since: 3.0
 **/
gboolean
ctk_combo_box_set_active_id (CtkComboBox *combo_box,
                             const gchar *active_id)
{
  CtkTreeModel *model;
  CtkTreeIter iter;
  gboolean match = FALSE;
  gint column;

  g_return_val_if_fail (CTK_IS_COMBO_BOX (combo_box), FALSE);

  if (active_id == NULL)
    {
      ctk_combo_box_set_active (combo_box, -1);
      return TRUE;  /* active row was successfully unset */
    }

  column = combo_box->priv->id_column;

  if (column < 0)
    return FALSE;

  model = ctk_combo_box_get_model (combo_box);
  g_return_val_if_fail (ctk_tree_model_get_column_type (model, column) ==
                        G_TYPE_STRING, FALSE);

  if (ctk_tree_model_get_iter_first (model, &iter))
    do {
      gchar *id;

      ctk_tree_model_get (model, &iter, column, &id, -1);
      if (id != NULL)
        match = strcmp (id, active_id) == 0;
      g_free (id);

      if (match)
        {
          ctk_combo_box_set_active_iter (combo_box, &iter);
          break;
        }
    } while (ctk_tree_model_iter_next (model, &iter));

  g_object_notify (G_OBJECT (combo_box), "active-id");

  return match;
}

CtkWidget *
ctk_combo_box_get_popup (CtkComboBox *combo)
{
  if (combo->priv->popup_window)
    return combo->priv->popup_window;
  else
    return combo->priv->popup_widget;
}
