/* CtkPrinterOptionWidget
 * Copyright (C) 2006 Alexander Larsson  <alexl@redhat.com>
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "ctkintl.h"
#include "ctkcheckbutton.h"
#include "ctkcelllayout.h"
#include "ctkcellrenderertext.h"
#include "ctkcombobox.h"
#include "ctkfilechooserdialog.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctkliststore.h"
#include "ctkradiobutton.h"
#include "ctkgrid.h"
#include "ctktogglebutton.h"
#include "ctkorientable.h"
#include "ctkprivate.h"

#include "ctkprinteroptionwidget.h"

/* This defines the max file length that the file chooser
 * button should display. The total length will be
 * FILENAME_LENGTH_MAX+3 because the truncated name is prefixed
 * with “...”.
 */
#define FILENAME_LENGTH_MAX 27

static void ctk_printer_option_widget_finalize (GObject *object);

static void deconstruct_widgets (CtkPrinterOptionWidget *widget);
static void construct_widgets   (CtkPrinterOptionWidget *widget);
static void update_widgets      (CtkPrinterOptionWidget *widget);

static gchar *trim_long_filename (const gchar *filename);

struct CtkPrinterOptionWidgetPrivate
{
  CtkPrinterOption *source;
  gulong source_changed_handler;

  CtkWidget *check;
  CtkWidget *combo;
  CtkWidget *entry;
  CtkWidget *image;
  CtkWidget *label;
  CtkWidget *info_label;
  CtkWidget *box;
  CtkWidget *button;

  /* the last location for save to file, that the user selected */
  gchar *last_location;
};

enum {
  CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SOURCE
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkPrinterOptionWidget, ctk_printer_option_widget, CTK_TYPE_BOX)

static void ctk_printer_option_widget_set_property (GObject      *object,
						    guint         prop_id,
						    const GValue *value,
						    GParamSpec   *pspec);
static void ctk_printer_option_widget_get_property (GObject      *object,
						    guint         prop_id,
						    GValue       *value,
						    GParamSpec   *pspec);
static gboolean ctk_printer_option_widget_mnemonic_activate (CtkWidget *widget,
							     gboolean   group_cycling);

static void
ctk_printer_option_widget_class_init (CtkPrinterOptionWidgetClass *class)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;

  object_class = (GObjectClass *) class;
  widget_class = (CtkWidgetClass *) class;

  object_class->finalize = ctk_printer_option_widget_finalize;
  object_class->set_property = ctk_printer_option_widget_set_property;
  object_class->get_property = ctk_printer_option_widget_get_property;

  widget_class->mnemonic_activate = ctk_printer_option_widget_mnemonic_activate;

  signals[CHANGED] =
    g_signal_new (I_("changed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrinterOptionWidgetClass, changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   PROP_SOURCE,
                                   g_param_spec_object ("source",
							P_("Source option"),
							P_("The PrinterOption backing this widget"),
							CTK_TYPE_PRINTER_OPTION,
							CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

}

static void
ctk_printer_option_widget_init (CtkPrinterOptionWidget *widget)
{
  widget->priv = ctk_printer_option_widget_get_instance_private (widget);

  ctk_box_set_spacing (CTK_BOX (widget), 12);
}

static void
ctk_printer_option_widget_finalize (GObject *object)
{
  CtkPrinterOptionWidget *widget = CTK_PRINTER_OPTION_WIDGET (object);
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  
  if (priv->source)
    {
      g_signal_handler_disconnect (priv->source,
				   priv->source_changed_handler);
      g_object_unref (priv->source);
      priv->source = NULL;
    }
  
  G_OBJECT_CLASS (ctk_printer_option_widget_parent_class)->finalize (object);
}

static void
ctk_printer_option_widget_set_property (GObject         *object,
					guint            prop_id,
					const GValue    *value,
					GParamSpec      *pspec)
{
  CtkPrinterOptionWidget *widget;
  
  widget = CTK_PRINTER_OPTION_WIDGET (object);

  switch (prop_id)
    {
    case PROP_SOURCE:
      ctk_printer_option_widget_set_source (widget, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_printer_option_widget_get_property (GObject    *object,
					guint       prop_id,
					GValue     *value,
					GParamSpec *pspec)
{
  CtkPrinterOptionWidget *widget = CTK_PRINTER_OPTION_WIDGET (object);
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;

  switch (prop_id)
    {
    case PROP_SOURCE:
      g_value_set_object (value, priv->source);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
ctk_printer_option_widget_mnemonic_activate (CtkWidget *widget,
					     gboolean   group_cycling)
{
  CtkPrinterOptionWidget *powidget = CTK_PRINTER_OPTION_WIDGET (widget);
  CtkPrinterOptionWidgetPrivate *priv = powidget->priv;

  if (priv->check)
    return ctk_widget_mnemonic_activate (priv->check, group_cycling);
  if (priv->combo)
    return ctk_widget_mnemonic_activate (priv->combo, group_cycling);
  if (priv->entry)
    return ctk_widget_mnemonic_activate (priv->entry, group_cycling);
  if (priv->button)
    return ctk_widget_mnemonic_activate (priv->button, group_cycling);

  return FALSE;
}

static void
emit_changed (CtkPrinterOptionWidget *widget)
{
  g_signal_emit (widget, signals[CHANGED], 0);
}

CtkWidget *
ctk_printer_option_widget_new (CtkPrinterOption *source)
{
  return g_object_new (CTK_TYPE_PRINTER_OPTION_WIDGET, "source", source, NULL);
}

static void
source_changed_cb (CtkPrinterOption *source,
		   CtkPrinterOptionWidget  *widget)
{
  update_widgets (widget);
  emit_changed (widget);
}

void
ctk_printer_option_widget_set_source (CtkPrinterOptionWidget *widget,
				      CtkPrinterOption       *source)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;

  if (source)
    g_object_ref (source);
  
  if (priv->source)
    {
      g_signal_handler_disconnect (priv->source,
				   priv->source_changed_handler);
      g_object_unref (priv->source);
    }

  priv->source = source;

  if (source)
    priv->source_changed_handler =
      g_signal_connect (source, "changed", G_CALLBACK (source_changed_cb), widget);

  construct_widgets (widget);
  update_widgets (widget);

  g_object_notify (G_OBJECT (widget), "source");
}

enum {
  NAME_COLUMN,
  VALUE_COLUMN,
  N_COLUMNS
};

static void
combo_box_set_model (CtkWidget *combo_box)
{
  CtkListStore *store;

  store = ctk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
  ctk_combo_box_set_model (CTK_COMBO_BOX (combo_box), CTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
combo_box_set_view (CtkWidget *combo_box)
{
  CtkCellRenderer *cell;

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo_box), cell, TRUE);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo_box), cell,
                                  "text", NAME_COLUMN,
                                   NULL);
}

static CtkWidget *
combo_box_entry_new (void)
{
  CtkWidget *combo_box;
  combo_box = g_object_new (CTK_TYPE_COMBO_BOX, "has-entry", TRUE, NULL);

  combo_box_set_model (combo_box);

  ctk_combo_box_set_entry_text_column (CTK_COMBO_BOX (combo_box), NAME_COLUMN);

  return combo_box;
}

static CtkWidget *
combo_box_new (void)
{
  CtkWidget *combo_box;
  combo_box = ctk_combo_box_new ();

  combo_box_set_model (combo_box);
  combo_box_set_view (combo_box);

  return combo_box;
}
  
static void
combo_box_append (CtkWidget   *combo,
		  const gchar *display_text,
		  const gchar *value)
{
  CtkTreeModel *model;
  CtkListStore *store;
  CtkTreeIter iter;
  
  model = ctk_combo_box_get_model (CTK_COMBO_BOX (combo));
  store = CTK_LIST_STORE (model);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter,
		      NAME_COLUMN, display_text,
		      VALUE_COLUMN, value,
		      -1);
}

struct ComboSet {
  CtkComboBox *combo;
  const gchar *value;
};

static gboolean
set_cb (CtkTreeModel *model, 
	CtkTreePath  *path, 
	CtkTreeIter  *iter, 
	gpointer      data)
{
  struct ComboSet *set_data = data;
  gboolean found;
  char *value;
  
  ctk_tree_model_get (model, iter, VALUE_COLUMN, &value, -1);
  found = (strcmp (value, set_data->value) == 0);
  g_free (value);
  
  if (found)
    ctk_combo_box_set_active_iter (set_data->combo, iter);

  return found;
}

static void
combo_box_set (CtkWidget   *combo,
	       const gchar *value)
{
  CtkTreeModel *model;
  struct ComboSet set_data;
  
  model = ctk_combo_box_get_model (CTK_COMBO_BOX (combo));

  set_data.combo = CTK_COMBO_BOX (combo);
  set_data.value = value;
  ctk_tree_model_foreach (model, set_cb, &set_data);
}

static gchar *
combo_box_get (CtkWidget *combo, gboolean *custom)
{
  CtkTreeModel *model;
  gchar *value;
  CtkTreeIter iter;

  model = ctk_combo_box_get_model (CTK_COMBO_BOX (combo));

  value = NULL;
  if (ctk_combo_box_get_active_iter (CTK_COMBO_BOX (combo), &iter))
    {
      ctk_tree_model_get (model, &iter, VALUE_COLUMN, &value, -1);
      *custom = FALSE;
    }
  else
    {
      if (ctk_combo_box_get_has_entry (CTK_COMBO_BOX (combo)))
        {
          value = g_strdup (ctk_entry_get_text (CTK_ENTRY (ctk_bin_get_child (CTK_BIN (combo)))));
          *custom = TRUE;
        }

      if (!value || !ctk_tree_model_get_iter_first (model, &iter))
        return value;

      /* If the user entered an item from the dropdown list manually, return
       * the non-custom option instead. */
      do
        {
          gchar *val, *name;
          ctk_tree_model_get (model, &iter, VALUE_COLUMN, &val,
                                            NAME_COLUMN, &name, -1);
          if (g_str_equal (value, name))
            {
              *custom = FALSE;
              g_free (name);
              g_free (value);
              return val;
            }

          g_free (val);
          g_free (name);
        }
      while (ctk_tree_model_iter_next (model, &iter));
    }

  return value;
}


static void
deconstruct_widgets (CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;

  if (priv->check)
    {
      ctk_widget_destroy (priv->check);
      priv->check = NULL;
    }
  
  if (priv->combo)
    {
      ctk_widget_destroy (priv->combo);
      priv->combo = NULL;
    }
  
  if (priv->entry)
    {
      ctk_widget_destroy (priv->entry);
      priv->entry = NULL;
    }

  if (priv->image)
    {
      ctk_widget_destroy (priv->image);
      priv->image = NULL;
    }

  if (priv->label)
    {
      ctk_widget_destroy (priv->label);
      priv->label = NULL;
    }
  if (priv->info_label)
    {
      ctk_widget_destroy (priv->info_label);
      priv->info_label = NULL;
    }
}

static void
check_toggled_cb (CtkToggleButton        *toggle_button,
		  CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;

  g_signal_handler_block (priv->source, priv->source_changed_handler);
  ctk_printer_option_set_boolean (priv->source,
				  ctk_toggle_button_get_active (toggle_button));
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
dialog_response_callback (CtkDialog              *dialog,
                          gint                    response_id,
                          CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  gchar *uri = NULL;
  gchar *new_location = NULL;

  if (response_id == CTK_RESPONSE_ACCEPT)
    {
      gchar *filename;
      gchar *filename_utf8;
      gchar *filename_short;

      new_location = ctk_file_chooser_get_uri (CTK_FILE_CHOOSER (dialog));

      filename = ctk_file_chooser_get_filename (CTK_FILE_CHOOSER (dialog));
      filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
      filename_short = trim_long_filename (filename_utf8);
      ctk_button_set_label (CTK_BUTTON (priv->button), filename_short);
      g_free (filename_short);
      g_free (filename_utf8);
      g_free (filename);
    }

  ctk_widget_destroy (CTK_WIDGET (dialog));

  if (new_location)
    uri = new_location;
  else
    uri = priv->last_location;

  if (uri)
    {
      ctk_printer_option_set (priv->source, uri);
      emit_changed (widget);
    }

  g_free (new_location);
  g_free (priv->last_location);
  priv->last_location = NULL;

  /* unblock the handler which was blocked in the filesave_choose_cb function */
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
}

static void
filesave_choose_cb (CtkWidget              *button,
                    CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  gchar *last_location = NULL;
  CtkWidget *dialog;
  CtkWindow *toplevel;

  /* this will be unblocked in the dialog_response_callback function */
  g_signal_handler_block (priv->source, priv->source_changed_handler);

  toplevel = CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (widget)));
  dialog = ctk_file_chooser_dialog_new (_("Select a filename"),
                                        toplevel,
                                        CTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Cancel"), CTK_RESPONSE_CANCEL,
                                        _("_Select"), CTK_RESPONSE_ACCEPT,
                                        NULL);

  /* The confirmation dialog will appear, when the user clicks print */
  ctk_file_chooser_set_do_overwrite_confirmation (CTK_FILE_CHOOSER (dialog), FALSE);

  /* select the current filename in the dialog */
  if (priv->source != NULL)
    {
      priv->last_location = last_location = g_strdup (priv->source->value);
      if (last_location)
        {
          GFile *file;
          gchar *basename;
          gchar *basename_utf8;

          ctk_file_chooser_select_uri (CTK_FILE_CHOOSER (dialog), last_location);
          file = g_file_new_for_uri (last_location);
          basename = g_file_get_basename (file);
          basename_utf8 = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
          ctk_file_chooser_set_current_name (CTK_FILE_CHOOSER (dialog), basename_utf8);
          g_free (basename_utf8);
          g_free (basename);
          g_object_unref (file);
        }
    }

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response_callback), widget);
  ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_window_present (CTK_WINDOW (dialog));
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static gchar *
filter_numeric (const gchar *val,
                gboolean     allow_neg,
		gboolean     allow_dec,
                gboolean    *changed_out)
{
  gchar *filtered_val;
  int i, j;
  int len = strlen (val);
  gboolean dec_set = FALSE;

  filtered_val = g_malloc (len + 1);

  for (i = 0, j = 0; i < len; i++)
    {
      if (isdigit (val[i]))
        {
          filtered_val[j] = val[i];
	  j++;
	}
      else if (allow_dec && !dec_set && 
               (val[i] == '.' || val[i] == ','))
        {
	  /* allow one period or comma
	   * we should be checking locals
	   * but this is good enough for now
	   */
          filtered_val[j] = val[i];
	  dec_set = TRUE;
	  j++;
	}
      else if (allow_neg && i == 0 && val[0] == '-')
        {
          filtered_val[0] = val[0];
	  j++;
	}
    }

  filtered_val[j] = '\0';
  *changed_out = !(i == j);

  return filtered_val;
}

static void
combo_changed_cb (CtkWidget              *combo,
		  CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  gchar *value;
  gchar *filtered_val = NULL;
  gboolean changed;
  gboolean custom = TRUE;

  g_signal_handler_block (priv->source, priv->source_changed_handler);
  
  value = combo_box_get (combo, &custom);

  /* Handle constraints if the user entered a custom value. */
  if (custom)
    {
      switch (priv->source->type)
        {
        case CTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
          filtered_val = filter_numeric (value, FALSE, FALSE, &changed);
          break;
        case CTK_PRINTER_OPTION_TYPE_PICKONE_INT:
          filtered_val = filter_numeric (value, TRUE, FALSE, &changed);
          break;
        case CTK_PRINTER_OPTION_TYPE_PICKONE_REAL:
          filtered_val = filter_numeric (value, TRUE, TRUE, &changed);
          break;
        default:
          break;
        }
    }

  if (filtered_val)
    {
      g_free (value);

      if (changed)
        {
          CtkEntry *entry;
	  
	  entry = CTK_ENTRY (ctk_bin_get_child (CTK_BIN (combo)));

          ctk_entry_set_text (entry, filtered_val);
	}
      value = filtered_val;
    }

  if (value)
    ctk_printer_option_set (priv->source, value);
  g_free (value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
entry_changed_cb (CtkWidget              *entry,
		  CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  const gchar *value;
  
  g_signal_handler_block (priv->source, priv->source_changed_handler);
  value = ctk_entry_get_text (CTK_ENTRY (entry));
  if (value)
    ctk_printer_option_set (priv->source, value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}


static void
radio_changed_cb (CtkWidget              *button,
		  CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  gchar *value;
  
  g_signal_handler_block (priv->source, priv->source_changed_handler);
  value = g_object_get_data (G_OBJECT (button), "value");
  if (value)
    ctk_printer_option_set (priv->source, value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
select_maybe (CtkWidget   *widget, 
	      const gchar *value)
{
  gchar *v = g_object_get_data (G_OBJECT (widget), "value");
      
  if (strcmp (value, v) == 0)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
}

static void
alternative_set (CtkWidget   *box,
		 const gchar *value)
{
  ctk_container_foreach (CTK_CONTAINER (box), 
			 (CtkCallback) select_maybe,
			 (gpointer) value);
}

static GSList *
alternative_append (CtkWidget              *box,
		    const gchar            *label,
                    const gchar            *value,
		    CtkPrinterOptionWidget *widget,
		    GSList                 *group)
{
  CtkWidget *button;

  button = ctk_radio_button_new_with_label (group, label);
  ctk_widget_show (button);
  ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);
  ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (button), "value", (gpointer)value);
  g_signal_connect (button, "toggled", 
		    G_CALLBACK (radio_changed_cb), widget);

  return ctk_radio_button_get_group (CTK_RADIO_BUTTON (button));
}

static void
construct_widgets (CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  CtkPrinterOption *source;
  char *text;
  int i;
  GSList *group;

  source = priv->source;
  
  deconstruct_widgets (widget);
  
  ctk_widget_set_sensitive (CTK_WIDGET (widget), TRUE);

  if (source == NULL)
    {
      priv->combo = combo_box_new ();
      combo_box_append (priv->combo,_("Not available"), "None");
      ctk_combo_box_set_active (CTK_COMBO_BOX (priv->combo), 0);
      ctk_widget_set_sensitive (CTK_WIDGET (widget), FALSE);
      ctk_widget_show (priv->combo);
      ctk_box_pack_start (CTK_BOX (widget), priv->combo, TRUE, TRUE, 0);
    }
  else switch (source->type)
    {
    case CTK_PRINTER_OPTION_TYPE_BOOLEAN:
      priv->check = ctk_check_button_new_with_mnemonic (source->display_text);
      g_signal_connect (priv->check, "toggled", G_CALLBACK (check_toggled_cb), widget);
      ctk_widget_show (priv->check);
      ctk_box_pack_start (CTK_BOX (widget), priv->check, TRUE, TRUE, 0);
      break;
    case CTK_PRINTER_OPTION_TYPE_PICKONE:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_REAL:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_INT:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_STRING:
      if (source->type == CTK_PRINTER_OPTION_TYPE_PICKONE)
        {
          priv->combo = combo_box_new ();
	}
      else
        {
          priv->combo = combo_box_entry_new ();

          if (source->type == CTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD ||
	      source->type == CTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE)
	    {
              CtkEntry *entry;

	      entry = CTK_ENTRY (ctk_bin_get_child (CTK_BIN (priv->combo)));

              ctk_entry_set_visibility (entry, FALSE); 
	    }
        }

      for (i = 0; i < source->num_choices; i++)
        combo_box_append (priv->combo,
                          source->choices_display[i],
                          source->choices[i]);
      ctk_widget_show (priv->combo);
      ctk_box_pack_start (CTK_BOX (widget), priv->combo, TRUE, TRUE, 0);
      g_signal_connect (priv->combo, "changed", G_CALLBACK (combo_changed_cb), widget);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = ctk_label_new_with_mnemonic (text);
      g_free (text);
      ctk_widget_show (priv->label);
      break;

    case CTK_PRINTER_OPTION_TYPE_ALTERNATIVE:
      group = NULL;
      priv->box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
      ctk_widget_set_valign (priv->box, CTK_ALIGN_BASELINE);
      ctk_widget_show (priv->box);
      ctk_box_pack_start (CTK_BOX (widget), priv->box, TRUE, TRUE, 0);
      for (i = 0; i < source->num_choices; i++)
        {
	  group = alternative_append (priv->box,
                                      source->choices_display[i],
                                      source->choices[i],
                                      widget,
                                      group);
          /* for mnemonic activation */
          if (i == 0)
            priv->button = group->data;
        }

      if (source->display_text)
	{
	  text = g_strdup_printf ("%s:", source->display_text);
	  priv->label = ctk_label_new_with_mnemonic (text);
          ctk_widget_set_valign (priv->label, CTK_ALIGN_BASELINE);
	  g_free (text);
	  ctk_widget_show (priv->label);
	}
      break;

    case CTK_PRINTER_OPTION_TYPE_STRING:
      priv->entry = ctk_entry_new ();
      ctk_entry_set_activates_default (CTK_ENTRY (priv->entry),
                                       ctk_printer_option_get_activates_default (source));
      ctk_widget_show (priv->entry);
      ctk_box_pack_start (CTK_BOX (widget), priv->entry, TRUE, TRUE, 0);
      g_signal_connect (priv->entry, "changed", G_CALLBACK (entry_changed_cb), widget);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = ctk_label_new_with_mnemonic (text);
      g_free (text);
      ctk_widget_show (priv->label);

      break;

    case CTK_PRINTER_OPTION_TYPE_FILESAVE:
      priv->button = ctk_button_new ();
      ctk_widget_show (priv->button);
      ctk_box_pack_start (CTK_BOX (widget), priv->button, TRUE, TRUE, 0);
      g_signal_connect (priv->button, "clicked", G_CALLBACK (filesave_choose_cb), widget);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = ctk_label_new_with_mnemonic (text);
      g_free (text);
      ctk_widget_show (priv->label);

      break;

    case CTK_PRINTER_OPTION_TYPE_INFO:
      priv->info_label = ctk_label_new (NULL);
      ctk_label_set_selectable (CTK_LABEL (priv->info_label), TRUE);
      ctk_widget_show (priv->info_label);
      ctk_box_pack_start (CTK_BOX (widget), priv->info_label, FALSE, TRUE, 0);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = ctk_label_new_with_mnemonic (text);
      g_free (text);
      ctk_widget_show (priv->label);

      break;

    default:
      break;
    }

  priv->image = ctk_image_new_from_icon_name ("dialog-warning", CTK_ICON_SIZE_MENU);
  ctk_box_pack_start (CTK_BOX (widget), priv->image, FALSE, FALSE, 0);
}

/*
 * If the filename exceeds FILENAME_LENGTH_MAX, then trim it and replace
 * the first three letters with three dots.
 */
static gchar *
trim_long_filename (const gchar *filename)
{
  const gchar *home;
  gint len, offset;
  gchar *result;

  home = g_get_home_dir ();
  if (g_str_has_prefix (filename, home))
    {
      gchar *homeless_filename;

      offset = g_utf8_strlen (home, -1);
      len = g_utf8_strlen (filename, -1);
      homeless_filename = g_utf8_substring (filename, offset, len);
      result = g_strconcat ("~", homeless_filename, NULL);
      g_free (homeless_filename);
    }
  else
    result = g_strdup (filename);

  len = g_utf8_strlen (result, -1);
  if (len > FILENAME_LENGTH_MAX)
    {
      gchar *suffix;

      suffix = g_utf8_substring (result, len - FILENAME_LENGTH_MAX, len);
      g_free (result);
      result = g_strconcat ("...", suffix, NULL);
      g_free (suffix);
    }

  return result;
}

static void
update_widgets (CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;
  CtkPrinterOption *source;

  source = priv->source;
  
  if (source == NULL)
    {
      ctk_widget_hide (priv->image);
      return;
    }

  switch (source->type)
    {
    case CTK_PRINTER_OPTION_TYPE_BOOLEAN:
      if (g_ascii_strcasecmp (source->value, "True") == 0)
	ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->check), TRUE);
      else
	ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->check), FALSE);
      break;
    case CTK_PRINTER_OPTION_TYPE_PICKONE:
      combo_box_set (priv->combo, source->value);
      break;
    case CTK_PRINTER_OPTION_TYPE_ALTERNATIVE:
      alternative_set (priv->box, source->value);
      break;
    case CTK_PRINTER_OPTION_TYPE_STRING:
      ctk_entry_set_text (CTK_ENTRY (priv->entry), source->value);
      break;
    case CTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_REAL:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_INT:
    case CTK_PRINTER_OPTION_TYPE_PICKONE_STRING:
      {
        CtkEntry *entry;

        entry = CTK_ENTRY (ctk_bin_get_child (CTK_BIN (priv->combo)));
        if (ctk_printer_option_has_choice (source, source->value))
          combo_box_set (priv->combo, source->value);
        else
          ctk_entry_set_text (entry, source->value);

        break;
      }
    case CTK_PRINTER_OPTION_TYPE_FILESAVE:
      {
        gchar *text;
        gchar *filename;

        filename = g_filename_from_uri (source->value, NULL, NULL);
        if (filename != NULL)
          {
            text = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
            if (text != NULL)
              {
                gchar *short_filename;

                short_filename = trim_long_filename (text);
                ctk_button_set_label (CTK_BUTTON (priv->button), short_filename);
                g_free (short_filename);
              }

            g_free (text);
            g_free (filename);
          }
        else
          ctk_button_set_label (CTK_BUTTON (priv->button), source->value);
        break;
      }
    case CTK_PRINTER_OPTION_TYPE_INFO:
      ctk_label_set_text (CTK_LABEL (priv->info_label), source->value);
      break;
    default:
      break;
    }

  if (source->has_conflict)
    ctk_widget_show (priv->image);
  else
    ctk_widget_hide (priv->image);
}

gboolean
ctk_printer_option_widget_has_external_label (CtkPrinterOptionWidget *widget)
{
  return widget->priv->label != NULL;
}

CtkWidget *
ctk_printer_option_widget_get_external_label (CtkPrinterOptionWidget  *widget)
{
  return widget->priv->label;
}

const gchar *
ctk_printer_option_widget_get_value (CtkPrinterOptionWidget *widget)
{
  CtkPrinterOptionWidgetPrivate *priv = widget->priv;

  if (priv->source)
    return priv->source->value;

  return "";
}
