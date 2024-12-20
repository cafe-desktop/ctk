/* ctkentrycompletion.c
 * Copyright (C) 2003  Kristian Rietveld  <kris@gtk.org>
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

/**
 * SECTION:ctkentrycompletion
 * @Short_description: Completion functionality for CtkEntry
 * @Title: CtkEntryCompletion
 *
 * #CtkEntryCompletion is an auxiliary object to be used in conjunction with
 * #CtkEntry to provide the completion functionality. It implements the
 * #CtkCellLayout interface, to allow the user to add extra cells to the
 * #CtkTreeView with completion matches.
 *
 * “Completion functionality” means that when the user modifies the text
 * in the entry, #CtkEntryCompletion checks which rows in the model match
 * the current content of the entry, and displays a list of matches.
 * By default, the matching is done by comparing the entry text
 * case-insensitively against the text column of the model (see
 * ctk_entry_completion_set_text_column()), but this can be overridden
 * with a custom match function (see ctk_entry_completion_set_match_func()).
 *
 * When the user selects a completion, the content of the entry is
 * updated. By default, the content of the entry is replaced by the
 * text column of the model, but this can be overridden by connecting
 * to the #CtkEntryCompletion::match-selected signal and updating the
 * entry in the signal handler. Note that you should return %TRUE from
 * the signal handler to suppress the default behaviour.
 *
 * To add completion functionality to an entry, use ctk_entry_set_completion().
 *
 * In addition to regular completion matches, which will be inserted into the
 * entry when they are selected, #CtkEntryCompletion also allows to display
 * “actions” in the popup window. Their appearance is similar to menuitems,
 * to differentiate them clearly from completion strings. When an action is
 * selected, the #CtkEntryCompletion::action-activated signal is emitted.
 *
 * CtkEntryCompletion uses a #CtkTreeModelFilter model to represent the
 * subset of the entire model that is currently matching. While the
 * CtkEntryCompletion signals #CtkEntryCompletion::match-selected and
 * #CtkEntryCompletion::cursor-on-match take the original model and an
 * iter pointing to that model as arguments, other callbacks and signals
 * (such as #CtkCellLayoutDataFuncs or #CtkCellArea::apply-attributes)
 * will generally take the filter model as argument. As long as you are
 * only calling ctk_tree_model_get(), this will make no difference to
 * you. If for some reason, you need the original model, use
 * ctk_tree_model_filter_get_model(). Don’t forget to use
 * ctk_tree_model_filter_convert_iter_to_child_iter() to obtain a
 * matching iter.
 */

#include "config.h"

#include "ctkentrycompletion.h"

#include "ctkentryprivate.h"
#include "ctkcelllayout.h"
#include "ctkcellareabox.h"

#include "ctkintl.h"
#include "ctkcellrenderertext.h"
#include "ctkframe.h"
#include "ctktreeselection.h"
#include "ctktreeview.h"
#include "ctkscrolledwindow.h"
#include "ctksizerequest.h"
#include "ctkbox.h"
#include "ctkwindow.h"
#include "ctkwindowgroup.h"
#include "ctkentry.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"

#include "ctkprivate.h"
#include "ctkwindowprivate.h"

#include <string.h>

#define PAGE_STEP 14
#define COMPLETION_TIMEOUT 100

/* signals */
enum
{
  INSERT_PREFIX,
  MATCH_SELECTED,
  ACTION_ACTIVATED,
  CURSOR_ON_MATCH,
  NO_MATCHES,
  LAST_SIGNAL
};

/* properties */
enum
{
  PROP_0,
  PROP_MODEL,
  PROP_MINIMUM_KEY_LENGTH,
  PROP_TEXT_COLUMN,
  PROP_INLINE_COMPLETION,
  PROP_POPUP_COMPLETION,
  PROP_POPUP_SET_WIDTH,
  PROP_POPUP_SINGLE_MATCH,
  PROP_INLINE_SELECTION,
  PROP_CELL_AREA,
  NUM_PROPERTIES
};


static void     ctk_entry_completion_cell_layout_init    (CtkCellLayoutIface      *iface);
static CtkCellArea* ctk_entry_completion_get_area        (CtkCellLayout           *cell_layout);

static void     ctk_entry_completion_constructed         (GObject      *object);
static void     ctk_entry_completion_set_property        (GObject      *object,
                                                          guint         prop_id,
                                                          const GValue *value,
                                                          GParamSpec   *pspec);
static void     ctk_entry_completion_get_property        (GObject      *object,
                                                          guint         prop_id,
                                                          GValue       *value,
                                                          GParamSpec   *pspec);
static void     ctk_entry_completion_finalize            (GObject      *object);
static void     ctk_entry_completion_dispose             (GObject      *object);

static gboolean ctk_entry_completion_visible_func        (CtkTreeModel       *model,
                                                          CtkTreeIter        *iter,
                                                          gpointer            data);
static gboolean ctk_entry_completion_popup_key_event     (CtkWidget          *widget,
                                                          CdkEventKey        *event,
                                                          gpointer            user_data);
static gboolean ctk_entry_completion_popup_button_press  (CtkWidget          *widget,
                                                          CdkEventButton     *event,
                                                          gpointer            user_data);
static gboolean ctk_entry_completion_list_button_press   (CtkWidget          *widget,
                                                          CdkEventButton     *event,
                                                          gpointer            user_data);
static gboolean ctk_entry_completion_action_button_press (CtkWidget          *widget,
                                                          CdkEventButton     *event,
                                                          gpointer            user_data);
static void     ctk_entry_completion_selection_changed   (CtkTreeSelection   *selection,
                                                          gpointer            data);
static gboolean ctk_entry_completion_list_enter_notify   (CtkWidget          *widget,
                                                          CdkEventCrossing   *event,
                                                          gpointer            data);
static gboolean ctk_entry_completion_list_motion_notify  (CtkWidget          *widget,
                                                          CdkEventMotion     *event,
                                                          gpointer            data);
static void     ctk_entry_completion_insert_action       (CtkEntryCompletion *completion,
                                                          gint                index,
                                                          const gchar        *string,
                                                          gboolean            markup);
static void     ctk_entry_completion_action_data_func    (CtkTreeViewColumn  *tree_column,
                                                          CtkCellRenderer    *cell,
                                                          CtkTreeModel       *model,
                                                          CtkTreeIter        *iter,
                                                          gpointer            data);

static gboolean ctk_entry_completion_match_selected      (CtkEntryCompletion *completion,
                                                          CtkTreeModel       *model,
                                                          CtkTreeIter        *iter);
static gboolean ctk_entry_completion_real_insert_prefix  (CtkEntryCompletion *completion,
                                                          const gchar        *prefix);
static gboolean ctk_entry_completion_cursor_on_match     (CtkEntryCompletion *completion,
                                                          CtkTreeModel       *model,
                                                          CtkTreeIter        *iter);
static gboolean ctk_entry_completion_insert_completion   (CtkEntryCompletion *completion,
                                                          CtkTreeModel       *model,
                                                          CtkTreeIter        *iter);
static void     ctk_entry_completion_insert_completion_text (CtkEntryCompletion *completion,
                                                             const gchar *text);
static void     connect_completion_signals                  (CtkEntryCompletion *completion);
static void     disconnect_completion_signals               (CtkEntryCompletion *completion);


static GParamSpec *entry_completion_props[NUM_PROPERTIES] = { NULL, };

static guint entry_completion_signals[LAST_SIGNAL] = { 0 };

/* CtkBuildable */
static void     ctk_entry_completion_buildable_init      (CtkBuildableIface  *iface);

G_DEFINE_TYPE_WITH_CODE (CtkEntryCompletion, ctk_entry_completion, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkEntryCompletion)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_LAYOUT,
                                                ctk_entry_completion_cell_layout_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_entry_completion_buildable_init))


static void
ctk_entry_completion_class_init (CtkEntryCompletionClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *)klass;

  object_class->constructed = ctk_entry_completion_constructed;
  object_class->set_property = ctk_entry_completion_set_property;
  object_class->get_property = ctk_entry_completion_get_property;
  object_class->dispose = ctk_entry_completion_dispose;
  object_class->finalize = ctk_entry_completion_finalize;

  klass->match_selected = ctk_entry_completion_match_selected;
  klass->insert_prefix = ctk_entry_completion_real_insert_prefix;
  klass->cursor_on_match = ctk_entry_completion_cursor_on_match;
  klass->no_matches = NULL;

  /**
   * CtkEntryCompletion::insert-prefix:
   * @widget: the object which received the signal
   * @prefix: the common prefix of all possible completions
   *
   * Gets emitted when the inline autocompletion is triggered.
   * The default behaviour is to make the entry display the
   * whole prefix and select the newly inserted part.
   *
   * Applications may connect to this signal in order to insert only a
   * smaller part of the @prefix into the entry - e.g. the entry used in
   * the #CtkFileChooser inserts only the part of the prefix up to the
   * next '/'.
   *
   * Returns: %TRUE if the signal has been handled
   *
   * Since: 2.6
   */
  entry_completion_signals[INSERT_PREFIX] =
    g_signal_new (I_("insert-prefix"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkEntryCompletionClass, insert_prefix),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__STRING,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_STRING);

  /**
   * CtkEntryCompletion::match-selected:
   * @widget: the object which received the signal
   * @model: the #CtkTreeModel containing the matches
   * @iter: a #CtkTreeIter positioned at the selected match
   *
   * Gets emitted when a match from the list is selected.
   * The default behaviour is to replace the contents of the
   * entry with the contents of the text column in the row
   * pointed to by @iter.
   *
   * Note that @model is the model that was passed to
   * ctk_entry_completion_set_model().
   *
   * Returns: %TRUE if the signal has been handled
   *
   * Since: 2.4
   */
  entry_completion_signals[MATCH_SELECTED] =
    g_signal_new (I_("match-selected"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkEntryCompletionClass, match_selected),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__OBJECT_BOXED,
                  G_TYPE_BOOLEAN, 2,
                  CTK_TYPE_TREE_MODEL,
                  CTK_TYPE_TREE_ITER);

  /**
   * CtkEntryCompletion::cursor-on-match:
   * @widget: the object which received the signal
   * @model: the #CtkTreeModel containing the matches
   * @iter: a #CtkTreeIter positioned at the selected match
   *
   * Gets emitted when a match from the cursor is on a match
   * of the list. The default behaviour is to replace the contents
   * of the entry with the contents of the text column in the row
   * pointed to by @iter.
   *
   * Note that @model is the model that was passed to
   * ctk_entry_completion_set_model().
   *
   * Returns: %TRUE if the signal has been handled
   *
   * Since: 2.12
   */
  entry_completion_signals[CURSOR_ON_MATCH] =
    g_signal_new (I_("cursor-on-match"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkEntryCompletionClass, cursor_on_match),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__OBJECT_BOXED,
                  G_TYPE_BOOLEAN, 2,
                  CTK_TYPE_TREE_MODEL,
                  CTK_TYPE_TREE_ITER);

  /**
   * CtkEntryCompletion::no-matches:
   * @widget: the object which received the signal
   *
   * Gets emitted when the filter model has zero
   * number of rows in completion_complete method.
   * (In other words when CtkEntryCompletion is out of
   *  suggestions)
   *
   * Since: 3.14
   */
  entry_completion_signals[NO_MATCHES] =
    g_signal_new (I_("no-matches"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkEntryCompletionClass, no_matches),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkEntryCompletion::action-activated:
   * @widget: the object which received the signal
   * @index: the index of the activated action
   *
   * Gets emitted when an action is activated.
   *
   * Since: 2.4
   */
  entry_completion_signals[ACTION_ACTIVATED] =
    g_signal_new (I_("action-activated"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkEntryCompletionClass, action_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  entry_completion_props[PROP_MODEL] =
      g_param_spec_object ("model",
                           P_("Completion Model"),
                           P_("The model to find matches in"),
                           CTK_TYPE_TREE_MODEL,
                           CTK_PARAM_READWRITE);

  entry_completion_props[PROP_MINIMUM_KEY_LENGTH] =
      g_param_spec_int ("minimum-key-length",
                        P_("Minimum Key Length"),
                        P_("Minimum length of the search key in order to look up matches"),
                        0, G_MAXINT, 1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntryCompletion:text-column:
   *
   * The column of the model containing the strings.
   * Note that the strings must be UTF-8.
   *
   * Since: 2.6
   */
  entry_completion_props[PROP_TEXT_COLUMN] =
    g_param_spec_int ("text-column",
                      P_("Text column"),
                      P_("The column of the model containing the strings."),
                      -1, G_MAXINT, -1,
                      CTK_PARAM_READWRITE);

  /**
   * CtkEntryCompletion:inline-completion:
   *
   * Determines whether the common prefix of the possible completions
   * should be inserted automatically in the entry. Note that this
   * requires text-column to be set, even if you are using a custom
   * match function.
   *
   * Since: 2.6
   **/
  entry_completion_props[PROP_INLINE_COMPLETION] =
      g_param_spec_boolean ("inline-completion",
                            P_("Inline completion"),
                            P_("Whether the common prefix should be inserted automatically"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntryCompletion:popup-completion:
   *
   * Determines whether the possible completions should be
   * shown in a popup window.
   *
   * Since: 2.6
   **/
  entry_completion_props[PROP_POPUP_COMPLETION] =
      g_param_spec_boolean ("popup-completion",
                            P_("Popup completion"),
                            P_("Whether the completions should be shown in a popup window"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntryCompletion:popup-set-width:
   *
   * Determines whether the completions popup window will be
   * resized to the width of the entry.
   *
   * Since: 2.8
   */
  entry_completion_props[PROP_POPUP_SET_WIDTH] =
      g_param_spec_boolean ("popup-set-width",
                            P_("Popup set width"),
                            P_("If TRUE, the popup window will have the same size as the entry"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntryCompletion:popup-single-match:
   *
   * Determines whether the completions popup window will shown
   * for a single possible completion. You probably want to set
   * this to %FALSE if you are using
   * [inline completion][CtkEntryCompletion--inline-completion].
   *
   * Since: 2.8
   */
  entry_completion_props[PROP_POPUP_SINGLE_MATCH] =
      g_param_spec_boolean ("popup-single-match",
                            P_("Popup single match"),
                            P_("If TRUE, the popup window will appear for a single match."),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntryCompletion:inline-selection:
   *
   * Determines whether the possible completions on the popup
   * will appear in the entry as you navigate through them.
   *
   * Since: 2.12
   */
  entry_completion_props[PROP_INLINE_SELECTION] =
      g_param_spec_boolean ("inline-selection",
                            P_("Inline selection"),
                            P_("Your description here"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntryCompletion:cell-area:
   *
   * The #CtkCellArea used to layout cell renderers in the treeview column.
   *
   * If no area is specified when creating the entry completion with
   * ctk_entry_completion_new_with_area() a horizontally oriented
   * #CtkCellAreaBox will be used.
   *
   * Since: 3.0
   */
  entry_completion_props[PROP_CELL_AREA] =
      g_param_spec_object ("cell-area",
                           P_("Cell Area"),
                           P_("The CtkCellArea used to layout cells"),
                           CTK_TYPE_CELL_AREA,
                           CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, entry_completion_props);
}


static void
ctk_entry_completion_buildable_custom_tag_end (CtkBuildable *buildable,
                                                CtkBuilder   *builder,
                                                GObject      *child,
                                                const gchar  *tagname,
                                                gpointer     *data)
{
  /* Just ignore the boolean return from here */
  _ctk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname, data);
}

static void
ctk_entry_completion_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = _ctk_cell_layout_buildable_add_child;
  iface->custom_tag_start = _ctk_cell_layout_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_entry_completion_buildable_custom_tag_end;
}

static void
ctk_entry_completion_cell_layout_init (CtkCellLayoutIface *iface)
{
  iface->get_area = ctk_entry_completion_get_area;
}

static void
ctk_entry_completion_init (CtkEntryCompletion *completion)
{
  CtkEntryCompletionPrivate *priv;

  /* yes, also priv, need to keep the code readable */
  completion->priv = ctk_entry_completion_get_instance_private (completion);
  priv = completion->priv;

  priv->minimum_key_length = 1;
  priv->text_column = -1;
  priv->has_completion = FALSE;
  priv->inline_completion = FALSE;
  priv->popup_completion = TRUE;
  priv->popup_set_width = TRUE;
  priv->popup_single_match = TRUE;
  priv->inline_selection = FALSE;

  priv->filter_model = NULL;
}

static void
ctk_entry_completion_constructed (GObject *object)
{
  CtkEntryCompletion        *completion = CTK_ENTRY_COMPLETION (object);
  CtkEntryCompletionPrivate *priv = completion->priv;
  CtkCellRenderer           *cell;
  CtkTreeSelection          *sel;
  CtkWidget                 *popup_frame;

  G_OBJECT_CLASS (ctk_entry_completion_parent_class)->constructed (object);

  if (!priv->cell_area)
    {
      priv->cell_area = ctk_cell_area_box_new ();
      g_object_ref_sink (priv->cell_area);
    }

  /* completions */
  priv->tree_view = ctk_tree_view_new ();
  g_signal_connect (priv->tree_view, "button-press-event",
                    G_CALLBACK (ctk_entry_completion_list_button_press),
                    completion);
  g_signal_connect (priv->tree_view, "enter-notify-event",
                    G_CALLBACK (ctk_entry_completion_list_enter_notify),
                    completion);
  g_signal_connect (priv->tree_view, "motion-notify-event",
                    G_CALLBACK (ctk_entry_completion_list_motion_notify),
                    completion);

  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->tree_view), FALSE);
  ctk_tree_view_set_hover_selection (CTK_TREE_VIEW (priv->tree_view), TRUE);

  sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
  ctk_tree_selection_set_mode (sel, CTK_SELECTION_SINGLE);
  ctk_tree_selection_unselect_all (sel);
  g_signal_connect (sel, "changed",
                    G_CALLBACK (ctk_entry_completion_selection_changed),
                    completion);
  priv->first_sel_changed = TRUE;

  priv->column = ctk_tree_view_column_new_with_area (priv->cell_area);
  ctk_tree_view_append_column (CTK_TREE_VIEW (priv->tree_view), priv->column);

  priv->scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (priv->scrolled_window),
                                  CTK_POLICY_NEVER,
                                  CTK_POLICY_AUTOMATIC);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (priv->scrolled_window),
                                       CTK_SHADOW_NONE);

  /* a nasty hack to get the completions treeview to size nicely */
  ctk_widget_set_size_request (ctk_scrolled_window_get_vscrollbar (CTK_SCROLLED_WINDOW (priv->scrolled_window)),
                               -1, 0);

  /* actions */
  priv->actions = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

  priv->action_view =
    ctk_tree_view_new_with_model (CTK_TREE_MODEL (priv->actions));
  g_object_ref_sink (priv->action_view);
  g_signal_connect (priv->action_view, "button-press-event",
                    G_CALLBACK (ctk_entry_completion_action_button_press),
                    completion);
  g_signal_connect (priv->action_view, "enter-notify-event",
                    G_CALLBACK (ctk_entry_completion_list_enter_notify),
                    completion);
  g_signal_connect (priv->action_view, "motion-notify-event",
                    G_CALLBACK (ctk_entry_completion_list_motion_notify),
                    completion);
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->action_view), FALSE);
  ctk_tree_view_set_hover_selection (CTK_TREE_VIEW (priv->action_view), TRUE);

  sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->action_view));
  ctk_tree_selection_set_mode (sel, CTK_SELECTION_SINGLE);
  ctk_tree_selection_unselect_all (sel);

  cell = ctk_cell_renderer_text_new ();
  ctk_tree_view_insert_column_with_data_func (CTK_TREE_VIEW (priv->action_view),
                                              0, "",
                                              cell,
                                              ctk_entry_completion_action_data_func,
                                              NULL,
                                              NULL);

  /* pack it all */
  priv->popup_window = ctk_window_new (CTK_WINDOW_POPUP);
  ctk_window_set_use_subsurface (CTK_WINDOW (priv->popup_window), TRUE);
  ctk_window_set_resizable (CTK_WINDOW (priv->popup_window), FALSE);
  ctk_window_set_type_hint (CTK_WINDOW(priv->popup_window),
                            CDK_WINDOW_TYPE_HINT_COMBO);

  g_signal_connect (priv->popup_window, "key-press-event",
                    G_CALLBACK (ctk_entry_completion_popup_key_event),
                    completion);
  g_signal_connect (priv->popup_window, "key-release-event",
                    G_CALLBACK (ctk_entry_completion_popup_key_event),
                    completion);
  g_signal_connect (priv->popup_window, "button-press-event",
                    G_CALLBACK (ctk_entry_completion_popup_button_press),
                    completion);

  popup_frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (popup_frame),
                             CTK_SHADOW_ETCHED_IN);
  ctk_widget_show (popup_frame);
  ctk_container_add (CTK_CONTAINER (priv->popup_window), popup_frame);

  priv->vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (popup_frame), priv->vbox);

  ctk_container_add (CTK_CONTAINER (priv->scrolled_window), priv->tree_view);
  ctk_box_pack_start (CTK_BOX (priv->vbox), priv->scrolled_window,
                      TRUE, TRUE, 0);

  /* we don't want to see the action treeview when no actions have
   * been inserted, so we pack the action treeview after the first
   * action has been added
   */
}


static void
ctk_entry_completion_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (object);
  CtkEntryCompletionPrivate *priv = completion->priv;
  CtkCellArea *area;

  switch (prop_id)
    {
      case PROP_MODEL:
        ctk_entry_completion_set_model (completion,
                                        g_value_get_object (value));
        break;

      case PROP_MINIMUM_KEY_LENGTH:
        ctk_entry_completion_set_minimum_key_length (completion,
                                                     g_value_get_int (value));
        break;

      case PROP_TEXT_COLUMN:
        priv->text_column = g_value_get_int (value);
        break;

      case PROP_INLINE_COMPLETION:
	ctk_entry_completion_set_inline_completion (completion,
						    g_value_get_boolean (value));
        break;

      case PROP_POPUP_COMPLETION:
	ctk_entry_completion_set_popup_completion (completion,
						   g_value_get_boolean (value));
        break;

      case PROP_POPUP_SET_WIDTH:
	ctk_entry_completion_set_popup_set_width (completion,
						  g_value_get_boolean (value));
        break;

      case PROP_POPUP_SINGLE_MATCH:
	ctk_entry_completion_set_popup_single_match (completion,
						     g_value_get_boolean (value));
        break;

      case PROP_INLINE_SELECTION:
	ctk_entry_completion_set_inline_selection (completion,
						   g_value_get_boolean (value));
        break;

      case PROP_CELL_AREA:
        /* Construct-only, can only be assigned once */
        area = g_value_get_object (value);
        if (area)
          {
            if (priv->cell_area != NULL)
              {
                g_warning ("cell-area has already been set, ignoring construct property");
                g_object_ref_sink (area);
                g_object_unref (area);
              }
            else
              priv->cell_area = g_object_ref_sink (area);
          }
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ctk_entry_completion_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (object);

  switch (prop_id)
    {
      case PROP_MODEL:
        g_value_set_object (value,
                            ctk_entry_completion_get_model (completion));
        break;

      case PROP_MINIMUM_KEY_LENGTH:
        g_value_set_int (value, ctk_entry_completion_get_minimum_key_length (completion));
        break;

      case PROP_TEXT_COLUMN:
        g_value_set_int (value, ctk_entry_completion_get_text_column (completion));
        break;

      case PROP_INLINE_COMPLETION:
        g_value_set_boolean (value, ctk_entry_completion_get_inline_completion (completion));
        break;

      case PROP_POPUP_COMPLETION:
        g_value_set_boolean (value, ctk_entry_completion_get_popup_completion (completion));
        break;

      case PROP_POPUP_SET_WIDTH:
        g_value_set_boolean (value, ctk_entry_completion_get_popup_set_width (completion));
        break;

      case PROP_POPUP_SINGLE_MATCH:
        g_value_set_boolean (value, ctk_entry_completion_get_popup_single_match (completion));
        break;

      case PROP_INLINE_SELECTION:
        g_value_set_boolean (value, ctk_entry_completion_get_inline_selection (completion));
        break;

      case PROP_CELL_AREA:
        g_value_set_object (value, completion->priv->cell_area);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ctk_entry_completion_finalize (GObject *object)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (object);
  CtkEntryCompletionPrivate *priv = completion->priv;

  g_free (priv->case_normalized_key);
  g_free (priv->completion_prefix);

  if (priv->match_notify)
    (* priv->match_notify) (priv->match_data);

  G_OBJECT_CLASS (ctk_entry_completion_parent_class)->finalize (object);
}

static void
ctk_entry_completion_dispose (GObject *object)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (object);
  CtkEntryCompletionPrivate *priv = completion->priv;

  if (priv->tree_view)
    {
      ctk_widget_destroy (priv->tree_view);
      priv->tree_view = NULL;
    }

  if (priv->entry)
    ctk_entry_set_completion (CTK_ENTRY (priv->entry), NULL);

  if (priv->actions)
    {
      g_object_unref (priv->actions);
      priv->actions = NULL;
    }

  if (priv->action_view)
    {
      g_object_unref (priv->action_view);
      priv->action_view = NULL;
    }

  if (priv->popup_window)
    {
      ctk_widget_destroy (priv->popup_window);
      priv->popup_window = NULL;
    }

  if (priv->cell_area)
    {
      g_object_unref (priv->cell_area);
      priv->cell_area = NULL;
    }

  G_OBJECT_CLASS (ctk_entry_completion_parent_class)->dispose (object);
}

/* implement cell layout interface (only need to return the underlying cell area) */
static CtkCellArea*
ctk_entry_completion_get_area (CtkCellLayout *cell_layout)
{
  CtkEntryCompletionPrivate *priv;

  priv = CTK_ENTRY_COMPLETION (cell_layout)->priv;

  if (G_UNLIKELY (!priv->cell_area))
    {
      priv->cell_area = ctk_cell_area_box_new ();
      g_object_ref_sink (priv->cell_area);
    }

  return priv->cell_area;
}

/* all those callbacks */
static gboolean
ctk_entry_completion_default_completion_func (CtkEntryCompletion *completion,
                                              const gchar        *key,
                                              CtkTreeIter        *iter,
                                              gpointer            user_data G_GNUC_UNUSED)
{
  gchar *item = NULL;
  gchar *normalized_string;
  gchar *case_normalized_string;

  gboolean ret = FALSE;

  CtkTreeModel *model;

  model = ctk_tree_model_filter_get_model (completion->priv->filter_model);

  g_return_val_if_fail (ctk_tree_model_get_column_type (model, completion->priv->text_column) == G_TYPE_STRING,
                        FALSE);

  ctk_tree_model_get (model, iter,
                      completion->priv->text_column, &item,
                      -1);

  if (item != NULL)
    {
      normalized_string = g_utf8_normalize (item, -1, G_NORMALIZE_ALL);

      if (normalized_string != NULL)
        {
          case_normalized_string = g_utf8_casefold (normalized_string, -1);

          if (!strncmp (key, case_normalized_string, strlen (key)))
            ret = TRUE;

          g_free (case_normalized_string);
        }
      g_free (normalized_string);
    }
  g_free (item);

  return ret;
}

static gboolean
ctk_entry_completion_visible_func (CtkTreeModel *model G_GNUC_UNUSED,
                                   CtkTreeIter  *iter,
                                   gpointer      data)
{
  gboolean ret = FALSE;

  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (data);

  if (!completion->priv->case_normalized_key)
    return ret;

  if (completion->priv->match_func)
    ret = (* completion->priv->match_func) (completion,
                                            completion->priv->case_normalized_key,
                                            iter,
                                            completion->priv->match_data);
  else if (completion->priv->text_column >= 0)
    ret = ctk_entry_completion_default_completion_func (completion,
                                                        completion->priv->case_normalized_key,
                                                        iter,
                                                        NULL);

  return ret;
}

static gboolean
ctk_entry_completion_popup_key_event (CtkWidget   *widget G_GNUC_UNUSED,
                                      CdkEventKey *event,
                                      gpointer     user_data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (user_data);

  if (!ctk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  /* propagate event to the entry */
  ctk_widget_event (completion->priv->entry, (CdkEvent *)event);

  return TRUE;
}

static gboolean
ctk_entry_completion_popup_button_press (CtkWidget      *widget G_GNUC_UNUSED,
                                         CdkEventButton *event G_GNUC_UNUSED,
                                         gpointer        user_data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (user_data);

  if (!ctk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  /* if we come here, it's usually time to popdown */
  _ctk_entry_completion_popdown (completion);

  return TRUE;
}

static gboolean
ctk_entry_completion_list_button_press (CtkWidget      *widget,
                                        CdkEventButton *event,
                                        gpointer        user_data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (user_data);
  CtkTreePath *path = NULL;

  if (!ctk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  if (ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (widget),
                                     event->x, event->y,
                                     &path, NULL, NULL, NULL))
    {
      CtkTreeIter iter;
      gboolean entry_set;
      CtkTreeModel *model;
      CtkTreeIter child_iter;

      ctk_tree_model_get_iter (CTK_TREE_MODEL (completion->priv->filter_model),
                               &iter, path);
      ctk_tree_path_free (path);
      ctk_tree_model_filter_convert_iter_to_child_iter (completion->priv->filter_model,
                                                        &child_iter,
                                                        &iter);
      model = ctk_tree_model_filter_get_model (completion->priv->filter_model);

      g_signal_handler_block (completion->priv->entry,
                              completion->priv->changed_id);
      g_signal_emit (completion, entry_completion_signals[MATCH_SELECTED],
                     0, model, &child_iter, &entry_set);
      g_signal_handler_unblock (completion->priv->entry,
                                completion->priv->changed_id);

      _ctk_entry_completion_popdown (completion);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ctk_entry_completion_action_button_press (CtkWidget      *widget,
                                          CdkEventButton *event,
                                          gpointer        user_data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (user_data);
  CtkTreePath *path = NULL;

  if (!ctk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  ctk_entry_reset_im_context (CTK_ENTRY (completion->priv->entry));

  if (ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (widget),
                                     event->x, event->y,
                                     &path, NULL, NULL, NULL))
    {
      g_signal_emit (completion, entry_completion_signals[ACTION_ACTIVATED],
                     0, ctk_tree_path_get_indices (path)[0]);
      ctk_tree_path_free (path);

      _ctk_entry_completion_popdown (completion);
      return TRUE;
    }

  return FALSE;
}

static void
ctk_entry_completion_action_data_func (CtkTreeViewColumn *tree_column G_GNUC_UNUSED,
                                       CtkCellRenderer   *cell,
                                       CtkTreeModel      *model,
                                       CtkTreeIter       *iter,
                                       gpointer           data G_GNUC_UNUSED)
{
  gchar *string = NULL;
  gboolean markup;

  ctk_tree_model_get (model, iter,
                      0, &string,
                      1, &markup,
                      -1);

  if (!string)
    return;

  if (markup)
    g_object_set (cell,
                  "text", NULL,
                  "markup", string,
                  NULL);
  else
    g_object_set (cell,
                  "markup", NULL,
                  "text", string,
                  NULL);

  g_free (string);
}

static void
ctk_entry_completion_selection_changed (CtkTreeSelection *selection,
                                        gpointer          data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (data);

  if (completion->priv->first_sel_changed)
    {
      completion->priv->first_sel_changed = FALSE;
      if (ctk_widget_is_focus (completion->priv->tree_view))
        ctk_tree_selection_unselect_all (selection);
    }
}

static void
prepare_popup_func (CdkSeat   *seat G_GNUC_UNUSED,
                    CdkWindow *window G_GNUC_UNUSED,
                    gpointer   user_data)
{
  CtkEntryCompletion *completion = user_data;

  /* prevent the first row being focused */
  ctk_widget_grab_focus (completion->priv->tree_view);

  ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->tree_view)));
  ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->action_view)));

  ctk_widget_show (completion->priv->popup_window);
}

static void
ctk_entry_completion_popup (CtkEntryCompletion *completion)
{
  CtkWidget *toplevel;

  if (ctk_widget_get_mapped (completion->priv->popup_window))
    return;

  if (!ctk_widget_get_mapped (completion->priv->entry))
    return;

  if (!ctk_widget_has_focus (completion->priv->entry))
    return;

  if (completion->priv->has_grab)
    return;

  completion->priv->ignore_enter = TRUE;

  ctk_widget_show_all (completion->priv->vbox);

  /* default on no match */
  completion->priv->current_selected = -1;

  toplevel = ctk_widget_get_toplevel (completion->priv->entry);
  if (CTK_IS_WINDOW (toplevel))
    {
      ctk_window_set_transient_for (CTK_WINDOW (completion->priv->popup_window),
                                    CTK_WINDOW (toplevel));
      ctk_window_group_add_window (ctk_window_get_group (CTK_WINDOW (toplevel)),
                                   CTK_WINDOW (completion->priv->popup_window));
    }

  ctk_window_set_screen (CTK_WINDOW (completion->priv->popup_window),
                         ctk_widget_get_screen (completion->priv->entry));

  _ctk_entry_completion_resize_popup (completion);

  if (completion->priv->device)
    {
      ctk_grab_add (completion->priv->popup_window);
      cdk_seat_grab (cdk_device_get_seat (completion->priv->device),
                     ctk_widget_get_window (completion->priv->popup_window),
                     CDK_SEAT_CAPABILITY_POINTER | CDK_SEAT_CAPABILITY_TOUCH,
                     TRUE, NULL, NULL,
                     prepare_popup_func, completion);

      completion->priv->has_grab = TRUE;
    }
}

void
_ctk_entry_completion_popdown (CtkEntryCompletion *completion)
{
  if (!ctk_widget_get_mapped (completion->priv->popup_window))
    return;

  completion->priv->ignore_enter = FALSE;

  if (completion->priv->has_grab)
    {
      cdk_seat_ungrab (cdk_device_get_seat (completion->priv->device));
      ctk_grab_remove (completion->priv->popup_window);
      completion->priv->has_grab = FALSE;
    }

  ctk_widget_hide (completion->priv->popup_window);
}

/* public API */

/**
 * ctk_entry_completion_new:
 *
 * Creates a new #CtkEntryCompletion object.
 *
 * Returns: A newly created #CtkEntryCompletion object
 *
 * Since: 2.4
 */
CtkEntryCompletion *
ctk_entry_completion_new (void)
{
  CtkEntryCompletion *completion;

  completion = g_object_new (CTK_TYPE_ENTRY_COMPLETION, NULL);

  return completion;
}

/**
 * ctk_entry_completion_new_with_area:
 * @area: the #CtkCellArea used to layout cells
 *
 * Creates a new #CtkEntryCompletion object using the
 * specified @area to layout cells in the underlying
 * #CtkTreeViewColumn for the drop-down menu.
 *
 * Returns: A newly created #CtkEntryCompletion object
 *
 * Since: 3.0
 */
CtkEntryCompletion *
ctk_entry_completion_new_with_area (CtkCellArea *area)
{
  CtkEntryCompletion *completion;

  completion = g_object_new (CTK_TYPE_ENTRY_COMPLETION, "cell-area", area, NULL);

  return completion;
}

/**
 * ctk_entry_completion_get_entry:
 * @completion: a #CtkEntryCompletion
 *
 * Gets the entry @completion has been attached to.
 *
 * Returns: (transfer none): The entry @completion has been attached to
 *
 * Since: 2.4
 */
CtkWidget *
ctk_entry_completion_get_entry (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), NULL);

  return completion->priv->entry;
}

/**
 * ctk_entry_completion_set_model:
 * @completion: a #CtkEntryCompletion
 * @model: (allow-none): the #CtkTreeModel
 *
 * Sets the model for a #CtkEntryCompletion. If @completion already has
 * a model set, it will remove it before setting the new model.
 * If model is %NULL, then it will unset the model.
 *
 * Since: 2.4
 */
void
ctk_entry_completion_set_model (CtkEntryCompletion *completion,
                                CtkTreeModel       *model)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (model == NULL || CTK_IS_TREE_MODEL (model));

  if (!model)
    {
      ctk_tree_view_set_model (CTK_TREE_VIEW (completion->priv->tree_view),
                               NULL);
      _ctk_entry_completion_popdown (completion);
      completion->priv->filter_model = NULL;
      return;
    }

  /* code will unref the old filter model (if any) */
  completion->priv->filter_model =
    CTK_TREE_MODEL_FILTER (ctk_tree_model_filter_new (model, NULL));
  ctk_tree_model_filter_set_visible_func (completion->priv->filter_model,
                                          ctk_entry_completion_visible_func,
                                          completion,
                                          NULL);

  ctk_tree_view_set_model (CTK_TREE_VIEW (completion->priv->tree_view),
                           CTK_TREE_MODEL (completion->priv->filter_model));
  g_object_unref (completion->priv->filter_model);

  g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_MODEL]);

  if (ctk_widget_get_visible (completion->priv->popup_window))
    _ctk_entry_completion_resize_popup (completion);
}

/**
 * ctk_entry_completion_get_model:
 * @completion: a #CtkEntryCompletion
 *
 * Returns the model the #CtkEntryCompletion is using as data source.
 * Returns %NULL if the model is unset.
 *
 * Returns: (nullable) (transfer none): A #CtkTreeModel, or %NULL if none
 *     is currently being used
 *
 * Since: 2.4
 */
CtkTreeModel *
ctk_entry_completion_get_model (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), NULL);

  if (!completion->priv->filter_model)
    return NULL;

  return ctk_tree_model_filter_get_model (completion->priv->filter_model);
}

/**
 * ctk_entry_completion_set_match_func:
 * @completion: a #CtkEntryCompletion
 * @func: the #CtkEntryCompletionMatchFunc to use
 * @func_data: user data for @func
 * @func_notify: destroy notify for @func_data.
 *
 * Sets the match function for @completion to be @func. The match function
 * is used to determine if a row should or should not be in the completion
 * list.
 *
 * Since: 2.4
 */
void
ctk_entry_completion_set_match_func (CtkEntryCompletion          *completion,
                                     CtkEntryCompletionMatchFunc  func,
                                     gpointer                     func_data,
                                     GDestroyNotify               func_notify)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));

  if (completion->priv->match_notify)
    (* completion->priv->match_notify) (completion->priv->match_data);

  completion->priv->match_func = func;
  completion->priv->match_data = func_data;
  completion->priv->match_notify = func_notify;
}

/**
 * ctk_entry_completion_set_minimum_key_length:
 * @completion: a #CtkEntryCompletion
 * @length: the minimum length of the key in order to start completing
 *
 * Requires the length of the search key for @completion to be at least
 * @length. This is useful for long lists, where completing using a small
 * key takes a lot of time and will come up with meaningless results anyway
 * (ie, a too large dataset).
 *
 * Since: 2.4
 */
void
ctk_entry_completion_set_minimum_key_length (CtkEntryCompletion *completion,
                                             gint                length)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (length >= 0);

  if (completion->priv->minimum_key_length != length)
    {
      completion->priv->minimum_key_length = length;

      g_object_notify_by_pspec (G_OBJECT (completion),
                                entry_completion_props[PROP_MINIMUM_KEY_LENGTH]);
    }
}

/**
 * ctk_entry_completion_get_minimum_key_length:
 * @completion: a #CtkEntryCompletion
 *
 * Returns the minimum key length as set for @completion.
 *
 * Returns: The currently used minimum key length
 *
 * Since: 2.4
 */
gint
ctk_entry_completion_get_minimum_key_length (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), 0);

  return completion->priv->minimum_key_length;
}

/**
 * ctk_entry_completion_complete:
 * @completion: a #CtkEntryCompletion
 *
 * Requests a completion operation, or in other words a refiltering of the
 * current list with completions, using the current key. The completion list
 * view will be updated accordingly.
 *
 * Since: 2.4
 */
void
ctk_entry_completion_complete (CtkEntryCompletion *completion)
{
  gchar *tmp;
  CtkTreeIter iter;

  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (CTK_IS_ENTRY (completion->priv->entry));

  if (completion->priv->filter_model)
    {
      gint matches;
      gint actions;
      gboolean popup_single;

      g_free (completion->priv->case_normalized_key);

      tmp = g_utf8_normalize (ctk_entry_get_text (CTK_ENTRY (completion->priv->entry)),
                              -1, G_NORMALIZE_ALL);
      completion->priv->case_normalized_key = g_utf8_casefold (tmp, -1);
      g_free (tmp);

      ctk_tree_model_filter_refilter (completion->priv->filter_model);

      if (!ctk_tree_model_get_iter_first (CTK_TREE_MODEL (completion->priv->filter_model), &iter))
        g_signal_emit (completion, entry_completion_signals[NO_MATCHES], 0);

      if (ctk_widget_get_visible (completion->priv->popup_window))
        _ctk_entry_completion_resize_popup (completion);

      matches = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (completion->priv->filter_model), NULL);
      actions = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (completion->priv->actions), NULL);

      g_object_get (completion, "popup-single-match", &popup_single, NULL);
      if ((matches > (popup_single ? 0: 1)) || actions > 0)
        {
          if (ctk_widget_get_visible (completion->priv->popup_window))
            _ctk_entry_completion_resize_popup (completion);
          else
            ctk_entry_completion_popup (completion);
        }
      else
        _ctk_entry_completion_popdown (completion);
    }
  else if (ctk_widget_get_visible (completion->priv->popup_window))
    _ctk_entry_completion_popdown (completion);

}

static void
ctk_entry_completion_insert_action (CtkEntryCompletion *completion,
                                    gint                index,
                                    const gchar        *string,
                                    gboolean            markup)
{
  CtkTreeIter iter;

  ctk_list_store_insert (completion->priv->actions, &iter, index);
  ctk_list_store_set (completion->priv->actions, &iter,
                      0, string,
                      1, markup,
                      -1);

  if (!ctk_widget_get_parent (completion->priv->action_view))
    {
      CtkTreePath *path = ctk_tree_path_new_from_indices (0, -1);

      ctk_tree_view_set_cursor (CTK_TREE_VIEW (completion->priv->action_view),
                                path, NULL, FALSE);
      ctk_tree_path_free (path);

      ctk_box_pack_start (CTK_BOX (completion->priv->vbox),
                          completion->priv->action_view, FALSE, FALSE, 0);
      ctk_widget_show (completion->priv->action_view);
    }
}

/**
 * ctk_entry_completion_insert_action_text:
 * @completion: a #CtkEntryCompletion
 * @index_: the index of the item to insert
 * @text: text of the item to insert
 *
 * Inserts an action in @completion’s action item list at position @index_
 * with text @text. If you want the action item to have markup, use
 * ctk_entry_completion_insert_action_markup().
 *
 * Note that @index_ is a relative position in the list of actions and
 * the position of an action can change when deleting a different action.
 *
 * Since: 2.4
 */
void
ctk_entry_completion_insert_action_text (CtkEntryCompletion *completion,
                                         gint                index_,
                                         const gchar        *text)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (text != NULL);

  ctk_entry_completion_insert_action (completion, index_, text, FALSE);
}

/**
 * ctk_entry_completion_insert_action_markup:
 * @completion: a #CtkEntryCompletion
 * @index_: the index of the item to insert
 * @markup: markup of the item to insert
 *
 * Inserts an action in @completion’s action item list at position @index_
 * with markup @markup.
 *
 * Since: 2.4
 */
void
ctk_entry_completion_insert_action_markup (CtkEntryCompletion *completion,
                                           gint                index_,
                                           const gchar        *markup)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (markup != NULL);

  ctk_entry_completion_insert_action (completion, index_, markup, TRUE);
}

/**
 * ctk_entry_completion_delete_action:
 * @completion: a #CtkEntryCompletion
 * @index_: the index of the item to delete
 *
 * Deletes the action at @index_ from @completion’s action list.
 *
 * Note that @index_ is a relative position and the position of an
 * action may have changed since it was inserted.
 *
 * Since: 2.4
 */
void
ctk_entry_completion_delete_action (CtkEntryCompletion *completion,
                                    gint                index_)
{
  CtkTreeIter iter;

  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (index_ >= 0);

  ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (completion->priv->actions),
                                 &iter, NULL, index_);
  ctk_list_store_remove (completion->priv->actions, &iter);
}

/**
 * ctk_entry_completion_set_text_column:
 * @completion: a #CtkEntryCompletion
 * @column: the column in the model of @completion to get strings from
 *
 * Convenience function for setting up the most used case of this code: a
 * completion list with just strings. This function will set up @completion
 * to have a list displaying all (and just) strings in the completion list,
 * and to get those strings from @column in the model of @completion.
 *
 * This functions creates and adds a #CtkCellRendererText for the selected
 * column. If you need to set the text column, but don't want the cell
 * renderer, use g_object_set() to set the #CtkEntryCompletion:text-column
 * property directly.
 *
 * Since: 2.4
 */
void
ctk_entry_completion_set_text_column (CtkEntryCompletion *completion,
                                      gint                column)
{
  CtkCellRenderer *cell;

  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (column >= 0);

  if (completion->priv->text_column == column)
    return;

  completion->priv->text_column = column;

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (completion),
                              cell, TRUE);
  ctk_cell_layout_add_attribute (CTK_CELL_LAYOUT (completion),
                                 cell,
                                 "text", column);

  g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_TEXT_COLUMN]);
}

/**
 * ctk_entry_completion_get_text_column:
 * @completion: a #CtkEntryCompletion
 *
 * Returns the column in the model of @completion to get strings from.
 *
 * Returns: the column containing the strings
 *
 * Since: 2.6
 */
gint
ctk_entry_completion_get_text_column (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), -1);

  return completion->priv->text_column;
}

/* private */

static gboolean
ctk_entry_completion_list_enter_notify (CtkWidget        *widget G_GNUC_UNUSED,
                                        CdkEventCrossing *event G_GNUC_UNUSED,
                                        gpointer          data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (data);

  return completion->priv->ignore_enter;
}

static gboolean
ctk_entry_completion_list_motion_notify (CtkWidget      *widget G_GNUC_UNUSED,
                                         CdkEventMotion *event G_GNUC_UNUSED,
                                         gpointer        data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (data);

  completion->priv->ignore_enter = FALSE;

  return FALSE;
}


/* some nasty size requisition */
void
_ctk_entry_completion_resize_popup (CtkEntryCompletion *completion)
{
  CtkAllocation allocation;
  gint x, y;
  gint matches, actions, items, height;
  CdkDisplay *display;
  CdkMonitor *monitor;
  gint vertical_separator;
  CdkRectangle area;
  CdkWindow *window;
  CtkRequisition popup_req;
  CtkRequisition entry_req;
  CtkRequisition tree_req;
  CtkTreePath *path;
  gboolean above;
  gint width;
  CtkTreeViewColumn *action_column;
  gint action_height;

  window = ctk_widget_get_window (completion->priv->entry);

  if (!window)
    return;

  if (!completion->priv->filter_model)
    return;

  ctk_widget_get_allocation (completion->priv->entry, &allocation);
  ctk_widget_get_preferred_size (completion->priv->entry,
                                 &entry_req, NULL);

  cdk_window_get_origin (window, &x, &y);
  x += allocation.x;
  y += allocation.y + (allocation.height - entry_req.height) / 2;

  matches = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (completion->priv->filter_model), NULL);
  actions = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (completion->priv->actions), NULL);
  action_column  = ctk_tree_view_get_column (CTK_TREE_VIEW (completion->priv->action_view), 0);

  /* Call get preferred size on the on the tree view to force it to validate its
   * cells before calling into the cell size functions.
   */
  ctk_widget_get_preferred_size (completion->priv->tree_view,
                                 &tree_req, NULL);
  ctk_tree_view_column_cell_get_size (completion->priv->column, NULL,
                                      NULL, NULL, NULL, &height);
  ctk_tree_view_column_cell_get_size (action_column, NULL,
                                      NULL, NULL, NULL, &action_height);

  ctk_widget_style_get (CTK_WIDGET (completion->priv->tree_view),
                        "vertical-separator", &vertical_separator,
                        NULL);

  height += vertical_separator;

  ctk_widget_realize (completion->priv->tree_view);

  display = ctk_widget_get_display (CTK_WIDGET (completion->priv->entry));
  monitor = cdk_display_get_monitor_at_window (display, window);
  cdk_monitor_get_workarea (monitor, &area);

  if (height == 0)
    items = 0;
  else if (y > area.height / 2)
    items = MIN (matches, (((area.y + y) - (actions * action_height)) / height) - 1);
  else
    items = MIN (matches, (((area.height - y) - (actions * action_height)) / height) - 1);

  if (items <= 0)
    ctk_widget_hide (completion->priv->scrolled_window);
  else
    ctk_widget_show (completion->priv->scrolled_window);

  if (completion->priv->popup_set_width)
    width = MIN (allocation.width, area.width);
  else
    width = -1;

  ctk_tree_view_columns_autosize (CTK_TREE_VIEW (completion->priv->tree_view));
  ctk_scrolled_window_set_min_content_width (CTK_SCROLLED_WINDOW (completion->priv->scrolled_window), width);
  ctk_widget_set_size_request (completion->priv->popup_window, width, -1);
  ctk_scrolled_window_set_min_content_height (CTK_SCROLLED_WINDOW (completion->priv->scrolled_window), items * height);

  if (actions)
    ctk_widget_show (completion->priv->action_view);
  else
    ctk_widget_hide (completion->priv->action_view);

  ctk_widget_get_preferred_size (completion->priv->popup_window,
                                 &popup_req, NULL);

  if (x < area.x)
    x = area.x;
  else if (x + popup_req.width > area.x + area.width)
    x = area.x + area.width - popup_req.width;

  if (y + entry_req.height + popup_req.height <= area.y + area.height ||
      y - area.y < (area.y + area.height) - (y + entry_req.height))
    {
      y += entry_req.height;
      above = FALSE;
    }
  else
    {
      y -= popup_req.height;
      above = TRUE;
    }

  if (matches > 0)
    {
      path = ctk_tree_path_new_from_indices (above ? matches - 1 : 0, -1);
      ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (completion->priv->tree_view), path,
                                    NULL, FALSE, 0.0, 0.0);
      ctk_tree_path_free (path);
    }

  ctk_window_move (CTK_WINDOW (completion->priv->popup_window), x, y);
}

static gboolean
ctk_entry_completion_match_selected (CtkEntryCompletion *completion,
                                     CtkTreeModel       *model,
                                     CtkTreeIter        *iter)
{
  gchar *str = NULL;

  ctk_tree_model_get (model, iter, completion->priv->text_column, &str, -1);
  ctk_entry_set_text (CTK_ENTRY (completion->priv->entry), str ? str : "");

  /* move cursor to the end */
  ctk_editable_set_position (CTK_EDITABLE (completion->priv->entry), -1);

  g_free (str);

  return TRUE;
}

static gboolean
ctk_entry_completion_cursor_on_match (CtkEntryCompletion *completion,
                                      CtkTreeModel       *model,
                                      CtkTreeIter        *iter)
{
  ctk_entry_completion_insert_completion (completion, model, iter);

  return TRUE;
}

/**
 * ctk_entry_completion_compute_prefix:
 * @completion: the entry completion
 * @key: The text to complete for
 *
 * Computes the common prefix that is shared by all rows in @completion
 * that start with @key. If no row matches @key, %NULL will be returned.
 * Note that a text column must have been set for this function to work,
 * see ctk_entry_completion_set_text_column() for details. 
 *
 * Returns: (nullable) (transfer full): The common prefix all rows starting with
 *   @key or %NULL if no row matches @key.
 *
 * Since: 3.4
 **/
gchar *
ctk_entry_completion_compute_prefix (CtkEntryCompletion *completion,
                                     const char         *key)
{
  CtkTreeIter iter;
  gchar *prefix = NULL;
  gboolean valid;

  if (completion->priv->text_column < 0)
    return NULL;

  valid = ctk_tree_model_get_iter_first (CTK_TREE_MODEL (completion->priv->filter_model),
                                         &iter);

  while (valid)
    {
      gchar *text;

      ctk_tree_model_get (CTK_TREE_MODEL (completion->priv->filter_model),
                          &iter, completion->priv->text_column, &text,
                          -1);

      if (text && g_str_has_prefix (text, key))
        {
          if (!prefix)
            prefix = g_strdup (text);
          else
            {
              gchar *p = prefix;
              gchar *q = text;

              while (*p && *p == *q)
                {
                  p++;
                  q++;
                }

              *p = '\0';

              if (p > prefix)
                {
                  /* strip a partial multibyte character */
                  q = g_utf8_find_prev_char (prefix, p);
                  switch (g_utf8_get_char_validated (q, p - q))
                    {
                    case (gunichar)-2:
                    case (gunichar)-1:
                      *q = 0;
                    default: ;
                    }
                }
            }
        }

      g_free (text);
      valid = ctk_tree_model_iter_next (CTK_TREE_MODEL (completion->priv->filter_model),
                                        &iter);
    }

  return prefix;
}


static gboolean
ctk_entry_completion_real_insert_prefix (CtkEntryCompletion *completion,
                                         const gchar        *prefix)
{
  if (prefix)
    {
      gint key_len;
      gint prefix_len;
      const gchar *key;

      prefix_len = g_utf8_strlen (prefix, -1);

      key = ctk_entry_get_text (CTK_ENTRY (completion->priv->entry));
      key_len = g_utf8_strlen (key, -1);

      if (prefix_len > key_len)
        {
          gint pos = prefix_len;

          ctk_editable_insert_text (CTK_EDITABLE (completion->priv->entry),
                                    prefix + strlen (key), -1, &pos);
          ctk_editable_select_region (CTK_EDITABLE (completion->priv->entry),
                                      key_len, prefix_len);

          completion->priv->has_completion = TRUE;
        }
    }

  return TRUE;
}

/**
 * ctk_entry_completion_get_completion_prefix:
 * @completion: a #CtkEntryCompletion
 *
 * Get the original text entered by the user that triggered
 * the completion or %NULL if there’s no completion ongoing.
 *
 * Returns: the prefix for the current completion
 *
 * Since: 2.12
 */
const gchar*
ctk_entry_completion_get_completion_prefix (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), NULL);

  return completion->priv->completion_prefix;
}

static void
ctk_entry_completion_insert_completion_text (CtkEntryCompletion *completion,
                                             const gchar *text)
{
  CtkEntryCompletionPrivate *priv = completion->priv;
  gint len;

  priv = completion->priv;

  if (priv->changed_id > 0)
    g_signal_handler_block (priv->entry, priv->changed_id);

  if (priv->insert_text_id > 0)
    g_signal_handler_block (priv->entry, priv->insert_text_id);

  ctk_entry_set_text (CTK_ENTRY (priv->entry), text);

  len = strlen (priv->completion_prefix);
  ctk_editable_select_region (CTK_EDITABLE (priv->entry), len, -1);

  if (priv->changed_id > 0)
    g_signal_handler_unblock (priv->entry, priv->changed_id);

  if (priv->insert_text_id > 0)
    g_signal_handler_unblock (priv->entry, priv->insert_text_id);
}

static gboolean
ctk_entry_completion_insert_completion (CtkEntryCompletion *completion,
                                        CtkTreeModel       *model,
                                        CtkTreeIter        *iter)
{
  gchar *str = NULL;

  if (completion->priv->text_column < 0)
    return FALSE;

  ctk_tree_model_get (model, iter,
                      completion->priv->text_column, &str,
                      -1);

  ctk_entry_completion_insert_completion_text (completion, str);

  g_free (str);

  return TRUE;
}

/**
 * ctk_entry_completion_insert_prefix:
 * @completion: a #CtkEntryCompletion
 *
 * Requests a prefix insertion.
 *
 * Since: 2.6
 */
void
ctk_entry_completion_insert_prefix (CtkEntryCompletion *completion)
{
  gboolean done;
  gchar *prefix;

  if (completion->priv->insert_text_id > 0)
    g_signal_handler_block (completion->priv->entry,
                            completion->priv->insert_text_id);

  prefix = ctk_entry_completion_compute_prefix (completion,
                                                ctk_entry_get_text (CTK_ENTRY (completion->priv->entry)));

  if (prefix)
    {
      g_signal_emit (completion, entry_completion_signals[INSERT_PREFIX],
                     0, prefix, &done);
      g_free (prefix);
    }

  if (completion->priv->insert_text_id > 0)
    g_signal_handler_unblock (completion->priv->entry,
                              completion->priv->insert_text_id);
}

/**
 * ctk_entry_completion_set_inline_completion:
 * @completion: a #CtkEntryCompletion
 * @inline_completion: %TRUE to do inline completion
 *
 * Sets whether the common prefix of the possible completions should
 * be automatically inserted in the entry.
 *
 * Since: 2.6
 */
void
ctk_entry_completion_set_inline_completion (CtkEntryCompletion *completion,
                                            gboolean            inline_completion)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));

  inline_completion = inline_completion != FALSE;

  if (completion->priv->inline_completion != inline_completion)
    {
      completion->priv->inline_completion = inline_completion;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_INLINE_COMPLETION]);
    }
}

/**
 * ctk_entry_completion_get_inline_completion:
 * @completion: a #CtkEntryCompletion
 *
 * Returns whether the common prefix of the possible completions should
 * be automatically inserted in the entry.
 *
 * Returns: %TRUE if inline completion is turned on
 *
 * Since: 2.6
 */
gboolean
ctk_entry_completion_get_inline_completion (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), FALSE);

  return completion->priv->inline_completion;
}

/**
 * ctk_entry_completion_set_popup_completion:
 * @completion: a #CtkEntryCompletion
 * @popup_completion: %TRUE to do popup completion
 *
 * Sets whether the completions should be presented in a popup window.
 *
 * Since: 2.6
 */
void
ctk_entry_completion_set_popup_completion (CtkEntryCompletion *completion,
                                           gboolean            popup_completion)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));

  popup_completion = popup_completion != FALSE;

  if (completion->priv->popup_completion != popup_completion)
    {
      completion->priv->popup_completion = popup_completion;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_POPUP_COMPLETION]);
    }
}


/**
 * ctk_entry_completion_get_popup_completion:
 * @completion: a #CtkEntryCompletion
 *
 * Returns whether the completions should be presented in a popup window.
 *
 * Returns: %TRUE if popup completion is turned on
 *
 * Since: 2.6
 */
gboolean
ctk_entry_completion_get_popup_completion (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), TRUE);

  return completion->priv->popup_completion;
}

/**
 * ctk_entry_completion_set_popup_set_width:
 * @completion: a #CtkEntryCompletion
 * @popup_set_width: %TRUE to make the width of the popup the same as the entry
 *
 * Sets whether the completion popup window will be resized to be the same
 * width as the entry.
 *
 * Since: 2.8
 */
void
ctk_entry_completion_set_popup_set_width (CtkEntryCompletion *completion,
                                          gboolean            popup_set_width)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));

  popup_set_width = popup_set_width != FALSE;

  if (completion->priv->popup_set_width != popup_set_width)
    {
      completion->priv->popup_set_width = popup_set_width;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_POPUP_SET_WIDTH]);
    }
}

/**
 * ctk_entry_completion_get_popup_set_width:
 * @completion: a #CtkEntryCompletion
 *
 * Returns whether the  completion popup window will be resized to the
 * width of the entry.
 *
 * Returns: %TRUE if the popup window will be resized to the width of
 *   the entry
 *
 * Since: 2.8
 */
gboolean
ctk_entry_completion_get_popup_set_width (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), TRUE);

  return completion->priv->popup_set_width;
}


/**
 * ctk_entry_completion_set_popup_single_match:
 * @completion: a #CtkEntryCompletion
 * @popup_single_match: %TRUE if the popup should appear even for a single
 *     match
 *
 * Sets whether the completion popup window will appear even if there is
 * only a single match. You may want to set this to %FALSE if you
 * are using [inline completion][CtkEntryCompletion--inline-completion].
 *
 * Since: 2.8
 */
void
ctk_entry_completion_set_popup_single_match (CtkEntryCompletion *completion,
                                             gboolean            popup_single_match)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));

  popup_single_match = popup_single_match != FALSE;

  if (completion->priv->popup_single_match != popup_single_match)
    {
      completion->priv->popup_single_match = popup_single_match;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_POPUP_SINGLE_MATCH]);
    }
}

/**
 * ctk_entry_completion_get_popup_single_match:
 * @completion: a #CtkEntryCompletion
 *
 * Returns whether the completion popup window will appear even if there is
 * only a single match.
 *
 * Returns: %TRUE if the popup window will appear regardless of the
 *    number of matches
 *
 * Since: 2.8
 */
gboolean
ctk_entry_completion_get_popup_single_match (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), TRUE);

  return completion->priv->popup_single_match;
}

/**
 * ctk_entry_completion_set_inline_selection:
 * @completion: a #CtkEntryCompletion
 * @inline_selection: %TRUE to do inline selection
 *
 * Sets whether it is possible to cycle through the possible completions
 * inside the entry.
 *
 * Since: 2.12
 */
void
ctk_entry_completion_set_inline_selection (CtkEntryCompletion *completion,
                                           gboolean inline_selection)
{
  g_return_if_fail (CTK_IS_ENTRY_COMPLETION (completion));

  inline_selection = inline_selection != FALSE;

  if (completion->priv->inline_selection != inline_selection)
    {
      completion->priv->inline_selection = inline_selection;

      g_object_notify_by_pspec (G_OBJECT (completion), entry_completion_props[PROP_INLINE_SELECTION]);
    }
}

/**
 * ctk_entry_completion_get_inline_selection:
 * @completion: a #CtkEntryCompletion
 *
 * Returns %TRUE if inline-selection mode is turned on.
 *
 * Returns: %TRUE if inline-selection mode is on
 *
 * Since: 2.12
 */
gboolean
ctk_entry_completion_get_inline_selection (CtkEntryCompletion *completion)
{
  g_return_val_if_fail (CTK_IS_ENTRY_COMPLETION (completion), FALSE);

  return completion->priv->inline_selection;
}


static gint
ctk_entry_completion_timeout (gpointer data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (data);

  completion->priv->completion_timeout = 0;

  if (completion->priv->filter_model &&
      g_utf8_strlen (ctk_entry_get_text (CTK_ENTRY (completion->priv->entry)), -1)
      >= completion->priv->minimum_key_length)
    {
      ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->tree_view)));
      ctk_entry_completion_complete (completion);
    }
  else if (ctk_widget_get_visible (completion->priv->popup_window))
    _ctk_entry_completion_popdown (completion);

  return FALSE;
}

static inline gboolean
keyval_is_cursor_move (guint keyval)
{
  if (keyval == CDK_KEY_Up || keyval == CDK_KEY_KP_Up)
    return TRUE;

  if (keyval == CDK_KEY_Down || keyval == CDK_KEY_KP_Down)
    return TRUE;

  if (keyval == CDK_KEY_Page_Up)
    return TRUE;

  if (keyval == CDK_KEY_Page_Down)
    return TRUE;

  return FALSE;
}

static gboolean
ctk_entry_completion_key_press (CtkWidget   *widget,
                                CdkEventKey *event,
                                gpointer     user_data)
{
  gint matches, actions = 0;
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (user_data);

  if (!completion->priv->popup_completion)
    return FALSE;

  if (event->keyval == CDK_KEY_Return ||
      event->keyval == CDK_KEY_KP_Enter ||
      event->keyval == CDK_KEY_ISO_Enter ||
      event->keyval == CDK_KEY_Escape)
    {
      if (completion->priv->completion_timeout)
        {
          g_source_remove (completion->priv->completion_timeout);
          completion->priv->completion_timeout = 0;
        }
    }

  if (!ctk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  matches = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (completion->priv->filter_model), NULL);

  if (completion->priv->actions)
    actions = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (completion->priv->actions), NULL);

  if (keyval_is_cursor_move (event->keyval))
    {
      CtkTreePath *path = NULL;

      if (event->keyval == CDK_KEY_Up || event->keyval == CDK_KEY_KP_Up)
        {
          if (completion->priv->current_selected < 0)
            completion->priv->current_selected = matches + actions - 1;
          else
            completion->priv->current_selected--;
        }
      else if (event->keyval == CDK_KEY_Down || event->keyval == CDK_KEY_KP_Down)
        {
          if (completion->priv->current_selected < matches + actions - 1)
            completion->priv->current_selected++;
          else
            completion->priv->current_selected = -1;
        }
      else if (event->keyval == CDK_KEY_Page_Up)
        {
          if (completion->priv->current_selected < 0)
            completion->priv->current_selected = matches + actions - 1;
          else if (completion->priv->current_selected == 0)
            completion->priv->current_selected = -1;
          else if (completion->priv->current_selected < matches)
            {
              completion->priv->current_selected -= PAGE_STEP;
              if (completion->priv->current_selected < 0)
                completion->priv->current_selected = 0;
            }
          else
            {
              completion->priv->current_selected -= PAGE_STEP;
              if (completion->priv->current_selected < matches - 1)
                completion->priv->current_selected = matches - 1;
            }
        }
      else if (event->keyval == CDK_KEY_Page_Down)
        {
          if (completion->priv->current_selected < 0)
            completion->priv->current_selected = 0;
          else if (completion->priv->current_selected < matches - 1)
            {
              completion->priv->current_selected += PAGE_STEP;
              if (completion->priv->current_selected > matches - 1)
                completion->priv->current_selected = matches - 1;
            }
          else if (completion->priv->current_selected == matches + actions - 1)
            {
              completion->priv->current_selected = -1;
            }
          else
            {
              completion->priv->current_selected += PAGE_STEP;
              if (completion->priv->current_selected > matches + actions - 1)
                completion->priv->current_selected = matches + actions - 1;
            }
        }

      if (completion->priv->current_selected < 0)
        {
          ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->tree_view)));
          ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->action_view)));

          if (completion->priv->inline_selection &&
              completion->priv->completion_prefix)
            {
              ctk_entry_set_text (CTK_ENTRY (completion->priv->entry),
                                  completion->priv->completion_prefix);
              ctk_editable_set_position (CTK_EDITABLE (widget), -1);
            }
        }
      else if (completion->priv->current_selected < matches)
        {
          ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->action_view)));

          path = ctk_tree_path_new_from_indices (completion->priv->current_selected, -1);
          ctk_tree_view_set_cursor (CTK_TREE_VIEW (completion->priv->tree_view),
                                    path, NULL, FALSE);

          if (completion->priv->inline_selection)
            {

              CtkTreeIter iter;
              CtkTreeIter child_iter;
              CtkTreeModel *model = NULL;
              CtkTreeSelection *sel;
              gboolean entry_set;

              sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->tree_view));
              if (!ctk_tree_selection_get_selected (sel, &model, &iter))
                return FALSE;
             ctk_tree_model_filter_convert_iter_to_child_iter (CTK_TREE_MODEL_FILTER (model), &child_iter, &iter);
              model = ctk_tree_model_filter_get_model (CTK_TREE_MODEL_FILTER (model));

              if (completion->priv->completion_prefix == NULL)
                completion->priv->completion_prefix = g_strdup (ctk_entry_get_text (CTK_ENTRY (completion->priv->entry)));

              g_signal_emit_by_name (completion, "cursor-on-match", model,
                                     &child_iter, &entry_set);
            }
        }
      else if (completion->priv->current_selected - matches >= 0)
        {
          ctk_tree_selection_unselect_all (ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->tree_view)));

          path = ctk_tree_path_new_from_indices (completion->priv->current_selected - matches, -1);
          ctk_tree_view_set_cursor (CTK_TREE_VIEW (completion->priv->action_view),
                                    path, NULL, FALSE);

          if (completion->priv->inline_selection &&
              completion->priv->completion_prefix)
            {
              ctk_entry_set_text (CTK_ENTRY (completion->priv->entry),
                                  completion->priv->completion_prefix);
              ctk_editable_set_position (CTK_EDITABLE (widget), -1);
            }
        }

      ctk_tree_path_free (path);

      return TRUE;
    }
  else if (event->keyval == CDK_KEY_Escape ||
           event->keyval == CDK_KEY_Left ||
           event->keyval == CDK_KEY_KP_Left ||
           event->keyval == CDK_KEY_Right ||
           event->keyval == CDK_KEY_KP_Right)
    {
      gboolean retval = TRUE;

      ctk_entry_reset_im_context (CTK_ENTRY (widget));
      _ctk_entry_completion_popdown (completion);

      if (completion->priv->current_selected < 0)
        {
          retval = FALSE;
          goto keypress_completion_out;
        }
      else if (completion->priv->inline_selection)
        {
          /* Escape rejects the tentative completion */
          if (event->keyval == CDK_KEY_Escape)
            {
              if (completion->priv->completion_prefix)
                ctk_entry_set_text (CTK_ENTRY (completion->priv->entry),
                                    completion->priv->completion_prefix);
              else
                ctk_entry_set_text (CTK_ENTRY (completion->priv->entry), "");
            }

          /* Move the cursor to the end for Right/Esc */
          if (event->keyval == CDK_KEY_Right ||
              event->keyval == CDK_KEY_KP_Right ||
              event->keyval == CDK_KEY_Escape)
            ctk_editable_set_position (CTK_EDITABLE (widget), -1);
          /* Let the default keybindings run for Left, i.e. either move to the
 *            * previous character or select word if a modifier is used */
          else
            retval = FALSE;
        }

keypress_completion_out:
      if (completion->priv->inline_selection)
        {
          g_free (completion->priv->completion_prefix);
          completion->priv->completion_prefix = NULL;
        }

      return retval;
    }
  else if (event->keyval == CDK_KEY_Tab ||
           event->keyval == CDK_KEY_KP_Tab ||
           event->keyval == CDK_KEY_ISO_Left_Tab)
    {
      ctk_entry_reset_im_context (CTK_ENTRY (widget));
      _ctk_entry_completion_popdown (completion);

      g_free (completion->priv->completion_prefix);
      completion->priv->completion_prefix = NULL;

      return FALSE;
    }
  else if (event->keyval == CDK_KEY_ISO_Enter ||
           event->keyval == CDK_KEY_KP_Enter ||
           event->keyval == CDK_KEY_Return)
    {
      CtkTreeIter iter;
      CtkTreeModel *model = NULL;
      CtkTreeModel *child_model;
      CtkTreeIter child_iter;
      CtkTreeSelection *sel;
      gboolean retval = TRUE;

      ctk_entry_reset_im_context (CTK_ENTRY (widget));
      _ctk_entry_completion_popdown (completion);

      if (completion->priv->current_selected < matches)
        {
          gboolean entry_set;

          sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->tree_view));
          if (ctk_tree_selection_get_selected (sel, &model, &iter))
            {
              ctk_tree_model_filter_convert_iter_to_child_iter (CTK_TREE_MODEL_FILTER (model), &child_iter, &iter);
              child_model = ctk_tree_model_filter_get_model (CTK_TREE_MODEL_FILTER (model));
              g_signal_handler_block (widget, completion->priv->changed_id);
              g_signal_emit_by_name (completion, "match-selected",
                                     child_model, &child_iter, &entry_set);
              g_signal_handler_unblock (widget, completion->priv->changed_id);

              if (!entry_set)
                {
                  gchar *str = NULL;

                  ctk_tree_model_get (model, &iter,
                                      completion->priv->text_column, &str,
                                      -1);

                  ctk_entry_set_text (CTK_ENTRY (widget), str);

                  /* move the cursor to the end */
                  ctk_editable_set_position (CTK_EDITABLE (widget), -1);
                  g_free (str);
                }
            }
          else
            retval = FALSE;
        }
      else if (completion->priv->current_selected - matches >= 0)
        {
          sel = ctk_tree_view_get_selection (CTK_TREE_VIEW (completion->priv->action_view));
          if (ctk_tree_selection_get_selected (sel, &model, &iter))
            {
              CtkTreePath *path;

              path = ctk_tree_path_new_from_indices (completion->priv->current_selected - matches, -1);
              g_signal_emit_by_name (completion, "action-activated",
                                     ctk_tree_path_get_indices (path)[0]);
              ctk_tree_path_free (path);
            }
          else
            retval = FALSE;
        }

      g_free (completion->priv->completion_prefix);
      completion->priv->completion_prefix = NULL;

      return retval;
    }

  return FALSE;
}

static void
ctk_entry_completion_changed (CtkWidget *widget,
                              gpointer   user_data)
{
  CtkEntryCompletion *completion = CTK_ENTRY_COMPLETION (user_data);
  CtkEntry *entry = CTK_ENTRY (widget);
  CdkDevice *device;

  if (!completion->priv->popup_completion)
    return;

  /* (re)install completion timeout */
  if (completion->priv->completion_timeout)
    {
      g_source_remove (completion->priv->completion_timeout);
      completion->priv->completion_timeout = 0;
    }

  if (!ctk_entry_get_text (entry))
    return;

  /* no need to normalize for this test */
  if (completion->priv->minimum_key_length > 0 &&
      strcmp ("", ctk_entry_get_text (entry)) == 0)
    {
      if (ctk_widget_get_visible (completion->priv->popup_window))
        _ctk_entry_completion_popdown (completion);
      return;
    }

  device = ctk_get_current_event_device ();

  if (device && cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    device = cdk_device_get_associated_device (device);

  if (device)
    completion->priv->device = device;

  completion->priv->completion_timeout =
    cdk_threads_add_timeout (COMPLETION_TIMEOUT,
                   ctk_entry_completion_timeout,
                   completion);
  g_source_set_name_by_id (completion->priv->completion_timeout, "[ctk+] ctk_entry_completion_timeout");
}

static gboolean
check_completion_callback (CtkEntryCompletion *completion)
{
  completion->priv->check_completion_idle = NULL;
  
  ctk_entry_completion_complete (completion);
  ctk_entry_completion_insert_prefix (completion);

  return FALSE;
}

static void
clear_completion_callback (CtkEntry   *entry,
                           GParamSpec *pspec)
{
  CtkEntryCompletion *completion = ctk_entry_get_completion (entry);
      
  if (!completion->priv->inline_completion)
    return;

  if (pspec->name == I_("cursor-position") ||
      pspec->name == I_("selection-bound"))
    completion->priv->has_completion = FALSE;
}

static gboolean
accept_completion_callback (CtkEntry *entry)
{
  CtkEntryCompletion *completion = ctk_entry_get_completion (entry);

  if (!completion->priv->inline_completion)
    return FALSE;

  if (completion->priv->has_completion)
    ctk_editable_set_position (CTK_EDITABLE (entry),
                               ctk_entry_buffer_get_length (ctk_entry_get_buffer (entry)));

  return FALSE;
}

static void
completion_insert_text_callback (CtkEntry           *entry G_GNUC_UNUSED,
                                 const gchar        *text G_GNUC_UNUSED,
                                 gint                length G_GNUC_UNUSED,
                                 gint                position G_GNUC_UNUSED,
                                 CtkEntryCompletion *completion)
{
  if (!completion->priv->inline_completion)
    return;

  /* idle to update the selection based on the file list */
  if (completion->priv->check_completion_idle == NULL)
    {
      completion->priv->check_completion_idle = g_idle_source_new ();
      g_source_set_priority (completion->priv->check_completion_idle, G_PRIORITY_HIGH);
      g_source_set_closure (completion->priv->check_completion_idle,
                            g_cclosure_new_object (G_CALLBACK (check_completion_callback),
                                                   G_OBJECT (completion)));
      g_source_attach (completion->priv->check_completion_idle, NULL);
    }
}

static void
connect_completion_signals (CtkEntryCompletion *completion)
{
  completion->priv->changed_id =
    g_signal_connect (completion->priv->entry, "changed",
                      G_CALLBACK (ctk_entry_completion_changed), completion);
  g_signal_connect (completion->priv->entry, "key-press-event",
                    G_CALLBACK (ctk_entry_completion_key_press), completion);

    completion->priv->insert_text_id =
      g_signal_connect (completion->priv->entry, "insert-text",
                        G_CALLBACK (completion_insert_text_callback), completion);
    g_signal_connect (completion->priv->entry, "notify",
                      G_CALLBACK (clear_completion_callback), completion);
    g_signal_connect (completion->priv->entry, "activate",
                      G_CALLBACK (accept_completion_callback), completion);
    g_signal_connect (completion->priv->entry, "focus-out-event",
                      G_CALLBACK (accept_completion_callback), completion);
}

static void
set_accessible_relation (CtkWidget *window,
                         CtkWidget *entry)
{
  AtkObject *window_accessible;
  AtkObject *entry_accessible;

  window_accessible = ctk_widget_get_accessible (window);
  entry_accessible = ctk_widget_get_accessible (entry);

  atk_object_add_relationship (window_accessible,
                               ATK_RELATION_POPUP_FOR,
                               entry_accessible);
}

static void
unset_accessible_relation (CtkWidget *window,
                           CtkWidget *entry)
{
  AtkObject *window_accessible;
  AtkObject *entry_accessible;

  window_accessible = ctk_widget_get_accessible (window);
  entry_accessible = ctk_widget_get_accessible (entry);

  atk_object_remove_relationship (window_accessible,
                                  ATK_RELATION_POPUP_FOR,
                                  entry_accessible);
}

static void
disconnect_completion_signals (CtkEntryCompletion *completion)
{
  if (completion->priv->changed_id > 0 &&
      g_signal_handler_is_connected (completion->priv->entry,
                                     completion->priv->changed_id))
    {
      g_signal_handler_disconnect (completion->priv->entry,
                                   completion->priv->changed_id);
      completion->priv->changed_id = 0;
    }
  g_signal_handlers_disconnect_by_func (completion->priv->entry,
                                        G_CALLBACK (ctk_entry_completion_key_press), completion);
  if (completion->priv->insert_text_id > 0 &&
      g_signal_handler_is_connected (completion->priv->entry,
                                     completion->priv->insert_text_id))
    {
      g_signal_handler_disconnect (completion->priv->entry,
                                   completion->priv->insert_text_id);
      completion->priv->insert_text_id = 0;
    }
  g_signal_handlers_disconnect_by_func (completion->priv->entry,
                                        G_CALLBACK (completion_insert_text_callback), completion);
  g_signal_handlers_disconnect_by_func (completion->priv->entry,
                                        G_CALLBACK (clear_completion_callback), completion);
  g_signal_handlers_disconnect_by_func (completion->priv->entry,
                                        G_CALLBACK (accept_completion_callback), completion);
}

void
_ctk_entry_completion_disconnect (CtkEntryCompletion *completion)
{
  if (completion->priv->completion_timeout)
    {
      g_source_remove (completion->priv->completion_timeout);
      completion->priv->completion_timeout = 0;
    }
  if (completion->priv->check_completion_idle)
    {
      g_source_destroy (completion->priv->check_completion_idle);
      completion->priv->check_completion_idle = NULL;
    }

  if (ctk_widget_get_mapped (completion->priv->popup_window))
    _ctk_entry_completion_popdown (completion);

  disconnect_completion_signals (completion);

  unset_accessible_relation (completion->priv->popup_window,
                             completion->priv->entry);
  ctk_window_set_attached_to (CTK_WINDOW (completion->priv->popup_window),
                              NULL);

  ctk_window_set_transient_for (CTK_WINDOW (completion->priv->popup_window), NULL);

  completion->priv->entry = NULL;
}

void
_ctk_entry_completion_connect (CtkEntryCompletion *completion,
                               CtkEntry           *entry)
{
  completion->priv->entry = CTK_WIDGET (entry);

  set_accessible_relation (completion->priv->popup_window,
                           completion->priv->entry);
  ctk_window_set_attached_to (CTK_WINDOW (completion->priv->popup_window),
                              completion->priv->entry);

  connect_completion_signals (completion);
}
