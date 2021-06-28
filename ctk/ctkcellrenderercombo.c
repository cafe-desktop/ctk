/* CtkCellRendererCombo
 * Copyright (C) 2004 Lorenzo Gil Sanchez
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

#include "ctkintl.h"
#include "ctkbin.h"
#include "ctkentry.h"
#include "ctkcelllayout.h"
#include "ctkcellrenderercombo.h"
#include "ctkcellrenderertext.h"
#include "ctkcombobox.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"


/**
 * SECTION:ctkcellrenderercombo
 * @Short_description: Renders a combobox in a cell
 * @Title: CtkCellRendererCombo
 *
 * #CtkCellRendererCombo renders text in a cell like #CtkCellRendererText from
 * which it is derived. But while #CtkCellRendererText offers a simple entry to
 * edit the text, #CtkCellRendererCombo offers a #CtkComboBox
 * widget to edit the text. The values to display in the combo box are taken from
 * the tree model specified in the #CtkCellRendererCombo:model property.
 *
 * The combo cell renderer takes care of adding a text cell renderer to the combo
 * box and sets it to display the column specified by its
 * #CtkCellRendererCombo:text-column property. Further properties of the combo box
 * can be set in a handler for the #CtkCellRenderer::editing-started signal.
 *
 * The #CtkCellRendererCombo cell renderer was added in CTK+ 2.6.
 */


struct _CtkCellRendererComboPrivate
{
  CtkTreeModel *model;

  CtkWidget *combo;

  gboolean has_entry;

  gint text_column;

  gulong focus_out_id;
};


static void ctk_cell_renderer_combo_class_init (CtkCellRendererComboClass *klass);
static void ctk_cell_renderer_combo_init       (CtkCellRendererCombo      *self);
static void ctk_cell_renderer_combo_finalize     (GObject      *object);
static void ctk_cell_renderer_combo_get_property (GObject      *object,
						  guint         prop_id,
						  GValue       *value,
						  GParamSpec   *pspec);

static void ctk_cell_renderer_combo_set_property (GObject      *object,
						  guint         prop_id,
						  const GValue *value,
						  GParamSpec   *pspec);

static CtkCellEditable *ctk_cell_renderer_combo_start_editing (CtkCellRenderer     *cell,
                                                               CdkEvent            *event,
                                                               CtkWidget           *widget,
                                                               const gchar         *path,
                                                               const CdkRectangle  *background_area,
                                                               const CdkRectangle  *cell_area,
                                                               CtkCellRendererState flags);

enum {
  PROP_0,
  PROP_MODEL,
  PROP_TEXT_COLUMN,
  PROP_HAS_ENTRY
};

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint cell_renderer_combo_signals[LAST_SIGNAL] = { 0, };

#define CTK_CELL_RENDERER_COMBO_PATH "ctk-cell-renderer-combo-path"

G_DEFINE_TYPE_WITH_PRIVATE (CtkCellRendererCombo, ctk_cell_renderer_combo, CTK_TYPE_CELL_RENDERER_TEXT)

static void
ctk_cell_renderer_combo_class_init (CtkCellRendererComboClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkCellRendererClass *cell_class = CTK_CELL_RENDERER_CLASS (klass);

  object_class->finalize = ctk_cell_renderer_combo_finalize;
  object_class->get_property = ctk_cell_renderer_combo_get_property;
  object_class->set_property = ctk_cell_renderer_combo_set_property;

  cell_class->start_editing = ctk_cell_renderer_combo_start_editing;

  /**
   * CtkCellRendererCombo:model:
   *
   * Holds a tree model containing the possible values for the combo box. 
   * Use the text_column property to specify the column holding the values.
   * 
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
				   PROP_MODEL,
				   g_param_spec_object ("model",
							P_("Model"),
							P_("The model containing the possible values for the combo box"),
							CTK_TYPE_TREE_MODEL,
							CTK_PARAM_READWRITE));

  /**
   * CtkCellRendererCombo:text-column:
   *
   * Specifies the model column which holds the possible values for the 
   * combo box. 
   *
   * Note that this refers to the model specified in the model property, 
   * not the model backing the tree view to which 
   * this cell renderer is attached.
   * 
   * #CtkCellRendererCombo automatically adds a text cell renderer for 
   * this column to its combo box.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT_COLUMN,
                                   g_param_spec_int ("text-column",
                                                     P_("Text Column"),
                                                     P_("A column in the data source model to get the strings from"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /** 
   * CtkCellRendererCombo:has-entry:
   *
   * If %TRUE, the cell renderer will include an entry and allow to enter 
   * values other than the ones in the popup list. 
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_ENTRY,
                                   g_param_spec_boolean ("has-entry",
							 P_("Has Entry"),
							 P_("If FALSE, don't allow to enter strings other than the chosen ones"),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * CtkCellRendererCombo::changed:
   * @combo: the object on which the signal is emitted
   * @path_string: a string of the path identifying the edited cell
   *               (relative to the tree view model)
   * @new_iter: the new iter selected in the combo box
   *            (relative to the combo box model)
   *
   * This signal is emitted each time after the user selected an item in
   * the combo box, either by using the mouse or the arrow keys.  Contrary
   * to CtkComboBox, CtkCellRendererCombo::changed is not emitted for
   * changes made to a selected item in the entry.  The argument @new_iter
   * corresponds to the newly selected item in the combo box and it is relative
   * to the CtkTreeModel set via the model property on CtkCellRendererCombo.
   *
   * Note that as soon as you change the model displayed in the tree view,
   * the tree view will immediately cease the editing operating.  This
   * means that you most probably want to refrain from changing the model
   * until the combo cell renderer emits the edited or editing_canceled signal.
   *
   * Since: 2.14
   */
  cell_renderer_combo_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  0,
		  NULL, NULL,
		  _ctk_marshal_VOID__STRING_BOXED,
		  G_TYPE_NONE, 2,
		  G_TYPE_STRING,
		  CTK_TYPE_TREE_ITER);
  g_signal_set_va_marshaller (cell_renderer_combo_signals[CHANGED],
                              G_TYPE_FROM_CLASS (object_class),
                              _ctk_marshal_VOID__STRING_BOXEDv);
}

static void
ctk_cell_renderer_combo_init (CtkCellRendererCombo *self)
{
  CtkCellRendererComboPrivate *priv;

  self->priv = ctk_cell_renderer_combo_get_instance_private (self);
  priv = self->priv;

  priv->model = NULL;
  priv->text_column = -1;
  priv->has_entry = TRUE;
  priv->focus_out_id = 0;
}

/**
 * ctk_cell_renderer_combo_new: 
 * 
 * Creates a new #CtkCellRendererCombo. 
 * Adjust how text is drawn using object properties. 
 * Object properties can be set globally (with g_object_set()). 
 * Also, with #CtkTreeViewColumn, you can bind a property to a value 
 * in a #CtkTreeModel. For example, you can bind the “text” property 
 * on the cell renderer to a string value in the model, thus rendering 
 * a different string in each row of the #CtkTreeView.
 * 
 * Returns: the new cell renderer
 *
 * Since: 2.6
 */
CtkCellRenderer *
ctk_cell_renderer_combo_new (void)
{
  return g_object_new (CTK_TYPE_CELL_RENDERER_COMBO, NULL); 
}

static void
ctk_cell_renderer_combo_finalize (GObject *object)
{
  CtkCellRendererCombo *cell = CTK_CELL_RENDERER_COMBO (object);
  CtkCellRendererComboPrivate *priv = cell->priv;
  
  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }
  
  G_OBJECT_CLASS (ctk_cell_renderer_combo_parent_class)->finalize (object);
}

static void
ctk_cell_renderer_combo_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  CtkCellRendererCombo *cell = CTK_CELL_RENDERER_COMBO (object);
  CtkCellRendererComboPrivate *priv = cell->priv;

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break; 
    case PROP_TEXT_COLUMN:
      g_value_set_int (value, priv->text_column);
      break;
    case PROP_HAS_ENTRY:
      g_value_set_boolean (value, priv->has_entry);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_cell_renderer_combo_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  CtkCellRendererCombo *cell = CTK_CELL_RENDERER_COMBO (object);
  CtkCellRendererComboPrivate *priv = cell->priv;

  switch (prop_id)
    {
    case PROP_MODEL:
      {
        if (priv->model)
          g_object_unref (priv->model);
        priv->model = CTK_TREE_MODEL (g_value_get_object (value));
        if (priv->model)
          g_object_ref (priv->model);
        break;
      }
    case PROP_TEXT_COLUMN:
      if (priv->text_column != g_value_get_int (value))
        {
          priv->text_column = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HAS_ENTRY:
      if (priv->has_entry != g_value_get_boolean (value))
        {
          priv->has_entry = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_cell_renderer_combo_changed (CtkComboBox *combo,
				 gpointer     data)
{
  CtkTreeIter iter;
  CtkCellRendererCombo *cell;

  cell = CTK_CELL_RENDERER_COMBO (data);

  if (ctk_combo_box_get_active_iter (combo, &iter))
    {
      const char *path;

      path = g_object_get_data (G_OBJECT (combo), CTK_CELL_RENDERER_COMBO_PATH);
      g_signal_emit (cell, cell_renderer_combo_signals[CHANGED], 0,
		     path, &iter);
    }
}

static void
ctk_cell_renderer_combo_editing_done (CtkCellEditable *combo,
				      gpointer         data)
{
  const gchar *path;
  gchar *new_text = NULL;
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkCellRendererCombo *cell;
  CtkEntry *entry;
  gboolean canceled;
  CtkCellRendererComboPrivate *priv;

  cell = CTK_CELL_RENDERER_COMBO (data);
  priv = cell->priv;

  if (priv->focus_out_id > 0)
    {
      g_signal_handler_disconnect (combo, priv->focus_out_id);
      priv->focus_out_id = 0;
    }

  g_object_get (combo,
                "editing-canceled", &canceled,
                NULL);
  ctk_cell_renderer_stop_editing (CTK_CELL_RENDERER (data), canceled);
  if (canceled)
    {
      priv->combo = NULL;
      return;
    }

  if (ctk_combo_box_get_has_entry (CTK_COMBO_BOX (combo)))
    {
      entry = CTK_ENTRY (ctk_bin_get_child (CTK_BIN (combo)));
      new_text = g_strdup (ctk_entry_get_text (entry));
    }
  else 
    {
      model = ctk_combo_box_get_model (CTK_COMBO_BOX (combo));

      if (model
          && ctk_combo_box_get_active_iter (CTK_COMBO_BOX (combo), &iter))
        ctk_tree_model_get (model, &iter, priv->text_column, &new_text, -1);
    }

  path = g_object_get_data (G_OBJECT (combo), CTK_CELL_RENDERER_COMBO_PATH);
  g_signal_emit_by_name (cell, "edited", path, new_text);

  priv->combo = NULL;

  g_free (new_text);
}

static gboolean
ctk_cell_renderer_combo_focus_out_event (CtkWidget *widget,
					 CdkEvent  *event,
					 gpointer   data)
{
  
  ctk_cell_renderer_combo_editing_done (CTK_CELL_EDITABLE (widget), data);

  return FALSE;
}

typedef struct 
{
  CtkCellRendererCombo *cell;
  gboolean found;
  CtkTreeIter iter;
} SearchData;

static gboolean 
find_text (CtkTreeModel *model, 
	   CtkTreePath  *path, 
	   CtkTreeIter  *iter, 
	   gpointer      data)
{
  CtkCellRendererComboPrivate *priv;
  SearchData *search_data = (SearchData *)data;
  gchar *text, *cell_text;

  priv = search_data->cell->priv;
  
  ctk_tree_model_get (model, iter, priv->text_column, &text, -1);
  g_object_get (CTK_CELL_RENDERER_TEXT (search_data->cell),
                "text", &cell_text,
                NULL);
  if (text && cell_text && g_strcmp0 (text, cell_text) == 0)
    {
      search_data->iter = *iter;
      search_data->found = TRUE;
    }

  g_free (cell_text);
  g_free (text);
  
  return search_data->found;
}

static CtkCellEditable *
ctk_cell_renderer_combo_start_editing (CtkCellRenderer     *cell,
                                       CdkEvent            *event,
                                       CtkWidget           *widget,
                                       const gchar         *path,
                                       const CdkRectangle  *background_area,
                                       const CdkRectangle  *cell_area,
                                       CtkCellRendererState flags)
{
  CtkCellRendererCombo *cell_combo;
  CtkCellRendererText *cell_text;
  CtkWidget *combo;
  SearchData search_data;
  CtkCellRendererComboPrivate *priv;
  gboolean editable;
  gchar *text;

  cell_text = CTK_CELL_RENDERER_TEXT (cell);
  g_object_get (cell_text, "editable", &editable, NULL);
  if (editable == FALSE)
    return NULL;

  cell_combo = CTK_CELL_RENDERER_COMBO (cell);
  priv = cell_combo->priv;

  if (priv->text_column < 0)
    return NULL;

  if (priv->has_entry)
    {
      combo = g_object_new (CTK_TYPE_COMBO_BOX, "has-entry", TRUE, NULL);

      if (priv->model)
        ctk_combo_box_set_model (CTK_COMBO_BOX (combo), priv->model);
      ctk_combo_box_set_entry_text_column (CTK_COMBO_BOX (combo),
                                           priv->text_column);

      g_object_get (cell_text, "text", &text, NULL);
      if (text)
	ctk_entry_set_text (CTK_ENTRY (ctk_bin_get_child (CTK_BIN (combo))),
			    text);
      g_free (text);
    }
  else
    {
      cell = ctk_cell_renderer_text_new ();

      combo = ctk_combo_box_new ();
      if (priv->model)
        ctk_combo_box_set_model (CTK_COMBO_BOX (combo), priv->model);

      ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), cell, TRUE);
      ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo), 
				      cell, "text", priv->text_column,
				      NULL);

      /* determine the current value */
      if (priv->model)
        {
          search_data.cell = cell_combo;
          search_data.found = FALSE;
          ctk_tree_model_foreach (priv->model, find_text, &search_data);
          if (search_data.found)
            ctk_combo_box_set_active_iter (CTK_COMBO_BOX (combo),
                                           &(search_data.iter));
        }
    }

  g_object_set (combo, "has-frame", FALSE, NULL);
  g_object_set_data_full (G_OBJECT (combo),
			  I_(CTK_CELL_RENDERER_COMBO_PATH),
			  g_strdup (path), g_free);

  ctk_widget_show (combo);

  g_signal_connect (CTK_CELL_EDITABLE (combo), "editing-done",
		    G_CALLBACK (ctk_cell_renderer_combo_editing_done),
		    cell_combo);
  g_signal_connect (CTK_CELL_EDITABLE (combo), "changed",
		    G_CALLBACK (ctk_cell_renderer_combo_changed),
		    cell_combo);
  priv->focus_out_id = g_signal_connect (combo, "focus-out-event",
                                         G_CALLBACK (ctk_cell_renderer_combo_focus_out_event),
                                         cell_combo);

  priv->combo = combo;

  return CTK_CELL_EDITABLE (combo);
}
