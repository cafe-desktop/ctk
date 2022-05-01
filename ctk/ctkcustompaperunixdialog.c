/* CtkCustomPaperUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 * Copyright © 2006, 2007, 2008 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */


#include "config.h"
#include <string.h>
#include <locale.h>

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
#include <langinfo.h>
#endif

#include "ctkintl.h"
#include "ctkprivate.h"

#include "ctkliststore.h"

#include "ctktreeviewcolumn.h"
#include "ctklabel.h"
#include "ctkspinbutton.h"

#include "ctkcustompaperunixdialog.h"
#include "ctkprintbackend.h"
#include "ctkprintutils.h"
#include "ctkdialogprivate.h"

#define LEGACY_CUSTOM_PAPER_FILENAME ".ctk-custom-papers"
#define CUSTOM_PAPER_FILENAME "custom-papers"


typedef struct
{
  CtkUnit    display_unit;
  CtkWidget *spin_button;
} UnitWidget;

struct _CtkCustomPaperUnixDialogPrivate
{

  CtkWidget *treeview;
  CtkWidget *values_box;
  CtkWidget *printer_combo;
  CtkWidget *width_widget;
  CtkWidget *height_widget;
  CtkWidget *top_widget;
  CtkWidget *bottom_widget;
  CtkWidget *left_widget;
  CtkWidget *right_widget;

  CtkTreeViewColumn *text_column;

  gulong printer_inserted_tag;
  gulong printer_removed_tag;

  guint request_details_tag;
  CtkPrinter *request_details_printer;

  guint non_user_change : 1;

  CtkListStore *custom_paper_list;
  CtkListStore *printer_list;

  GList *print_backends;

  gchar *waiting_for_printer;
};

enum {
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_PRINTER,
  PRINTER_LIST_N_COLS
};


G_DEFINE_TYPE_WITH_PRIVATE (CtkCustomPaperUnixDialog, ctk_custom_paper_unix_dialog, CTK_TYPE_DIALOG)


static void ctk_custom_paper_unix_dialog_constructed (GObject *object);
static void ctk_custom_paper_unix_dialog_finalize  (GObject                *object);
static void populate_dialog                        (CtkCustomPaperUnixDialog *dialog);
static void printer_added_cb                       (CtkPrintBackend        *backend,
						    CtkPrinter             *printer,
						    CtkCustomPaperUnixDialog *dialog);
static void printer_removed_cb                     (CtkPrintBackend        *backend,
						    CtkPrinter             *printer,
						    CtkCustomPaperUnixDialog *dialog);
static void printer_status_cb                      (CtkPrintBackend        *backend,
						    CtkPrinter             *printer,
						    CtkCustomPaperUnixDialog *dialog);



CtkUnit
_ctk_print_get_default_user_units (void)
{
  /* Translate to the default units to use for presenting
   * lengths to the user. Translate to default:inch if you
   * want inches, otherwise translate to default:mm.
   * Do *not* translate it to "predefinito:mm", if it
   * it isn't default:mm or default:inch it will not work
   */
  gchar *e = _("default:mm");

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
  gchar *imperial = NULL;

  imperial = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
  if (imperial && imperial[0] == 2 )
    return CTK_UNIT_INCH;  /* imperial */
  if (imperial && imperial[0] == 1 )
    return CTK_UNIT_MM;  /* metric */
#endif

  if (strcmp (e, "default:inch")==0)
    return CTK_UNIT_INCH;
  else if (strcmp (e, "default:mm"))
    g_warning ("Whoever translated default:mm did so wrongly.");
  return CTK_UNIT_MM;
}

static char *
custom_paper_get_legacy_filename (void)
{
  gchar *filename;

  filename = g_build_filename (g_get_home_dir (),
			       LEGACY_CUSTOM_PAPER_FILENAME, NULL);
  g_assert (filename != NULL);
  return filename;
}

static char *
custom_paper_get_filename (void)
{
  gchar *filename;

  filename = g_build_filename (g_get_user_config_dir (),
                               "ctk-3.0",
			       CUSTOM_PAPER_FILENAME, NULL);
  g_assert (filename != NULL);
  return filename;
}

GList *
_ctk_load_custom_papers (void)
{
  GKeyFile *keyfile;
  gchar *filename;
  gchar **groups;
  gsize n_groups, i;
  gboolean load_ok;
  GList *result = NULL;

  filename = custom_paper_get_filename ();

  keyfile = g_key_file_new ();
  load_ok = g_key_file_load_from_file (keyfile, filename, 0, NULL);
  g_free (filename);
  if (!load_ok)
    {
      /* try legacy file */
      filename = custom_paper_get_legacy_filename ();
      load_ok = g_key_file_load_from_file (keyfile, filename, 0, NULL);
      g_free (filename);
    }
  if (!load_ok)
    {
      g_key_file_free (keyfile);
      return NULL;
    }

  groups = g_key_file_get_groups (keyfile, &n_groups);
  for (i = 0; i < n_groups; ++i)
    {
      CtkPageSetup *page_setup;

      page_setup = ctk_page_setup_new_from_key_file (keyfile, groups[i], NULL);
      if (!page_setup)
        continue;

      result = g_list_prepend (result, page_setup);
    }

  g_strfreev (groups);
  g_key_file_free (keyfile);

  return g_list_reverse (result);
}

void
_ctk_print_load_custom_papers (CtkListStore *store)
{
  CtkTreeIter iter;
  GList *papers, *p;
  CtkPageSetup *page_setup;

  ctk_list_store_clear (store);

  papers = _ctk_load_custom_papers ();
  for (p = papers; p; p = p->next)
    {
      page_setup = p->data;
      ctk_list_store_append (store, &iter);
      ctk_list_store_set (store, &iter,
			  0, page_setup,
			  -1);
      g_object_unref (page_setup);
    }

  g_list_free (papers);
}

void
_ctk_print_save_custom_papers (CtkListStore *store)
{
  CtkTreeModel *model = CTK_TREE_MODEL (store);
  CtkTreeIter iter;
  GKeyFile *keyfile;
  gchar *filename, *data, *parentdir;
  gsize len;
  gint i = 0;

  keyfile = g_key_file_new ();

  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      do
	{
	  CtkPageSetup *page_setup;
	  gchar group[32];

	  g_snprintf (group, sizeof (group), "Paper%u", i);

	  ctk_tree_model_get (model, &iter, 0, &page_setup, -1);

	  ctk_page_setup_to_key_file (page_setup, keyfile, group);

	  ++i;
	} while (ctk_tree_model_iter_next (model, &iter));
    }

  filename = custom_paper_get_filename ();
  parentdir = g_build_filename (g_get_user_config_dir (),
                                "ctk-3.0",
                                NULL);
  if (g_mkdir_with_parents (parentdir, 0700) == 0)
    {
      data = g_key_file_to_data (keyfile, &len, NULL);
      g_file_set_contents (filename, data, len, NULL);
      g_free (data);
    }
  g_free (parentdir);
  g_free (filename);
}

static void
ctk_custom_paper_unix_dialog_class_init (CtkCustomPaperUnixDialogClass *class)
{
  G_OBJECT_CLASS (class)->constructed = ctk_custom_paper_unix_dialog_constructed;
  G_OBJECT_CLASS (class)->finalize = ctk_custom_paper_unix_dialog_finalize;
}

static void
custom_paper_dialog_response_cb (CtkDialog *dialog,
				 gint       response,
				 gpointer   user_data)
{
  CtkCustomPaperUnixDialogPrivate *priv = CTK_CUSTOM_PAPER_UNIX_DIALOG (dialog)->priv;

  _ctk_print_save_custom_papers (priv->custom_paper_list);
}

static void
ctk_custom_paper_unix_dialog_init (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv;
  CtkTreeIter iter;

  dialog->priv = ctk_custom_paper_unix_dialog_get_instance_private (dialog);
  priv = dialog->priv;

  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (dialog));

  priv->print_backends = NULL;

  priv->request_details_printer = NULL;
  priv->request_details_tag = 0;

  priv->printer_list = ctk_list_store_new (PRINTER_LIST_N_COLS,
					   G_TYPE_STRING,
					   G_TYPE_OBJECT);

  ctk_list_store_append (priv->printer_list, &iter);

  priv->custom_paper_list = ctk_list_store_new (1, G_TYPE_OBJECT);
  _ctk_print_load_custom_papers (priv->custom_paper_list);

  populate_dialog (dialog);

  g_signal_connect (dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), NULL);
}

static void
ctk_custom_paper_unix_dialog_constructed (GObject *object)
{
  gboolean use_header;

  G_OBJECT_CLASS (ctk_custom_paper_unix_dialog_parent_class)->constructed (object);

  g_object_get (object, "use-header-bar", &use_header, NULL);
  if (!use_header)
    {
      ctk_dialog_add_buttons (CTK_DIALOG (object),
                              _("_Close"), CTK_RESPONSE_CLOSE,
                              NULL);
      ctk_dialog_set_default_response (CTK_DIALOG (object), CTK_RESPONSE_CLOSE);
    }
}

static void
ctk_custom_paper_unix_dialog_finalize (GObject *object)
{
  CtkCustomPaperUnixDialog *dialog = CTK_CUSTOM_PAPER_UNIX_DIALOG (object);
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkPrintBackend *backend;
  GList *node;

  if (priv->printer_list)
    {
      g_signal_handler_disconnect (priv->printer_list, priv->printer_inserted_tag);
      g_signal_handler_disconnect (priv->printer_list, priv->printer_removed_tag);
      g_object_unref (priv->printer_list);
      priv->printer_list = NULL;
    }

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
				   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }

  if (priv->custom_paper_list)
    {
      g_object_unref (priv->custom_paper_list);
      priv->custom_paper_list = NULL;
    }

  g_free (priv->waiting_for_printer);
  priv->waiting_for_printer = NULL;

  for (node = priv->print_backends; node != NULL; node = node->next)
    {
      backend = CTK_PRINT_BACKEND (node->data);

      g_signal_handlers_disconnect_by_func (backend, printer_added_cb, dialog);
      g_signal_handlers_disconnect_by_func (backend, printer_removed_cb, dialog);
      g_signal_handlers_disconnect_by_func (backend, printer_status_cb, dialog);

      ctk_print_backend_destroy (backend);
      g_object_unref (backend);
    }

  g_list_free (priv->print_backends);
  priv->print_backends = NULL;

  G_OBJECT_CLASS (ctk_custom_paper_unix_dialog_parent_class)->finalize (object);
}

/**
 * ctk_custom_paper_unix_dialog_new:
 * @title: (allow-none): the title of the dialog, or %NULL
 * @parent: (allow-none): transient parent of the dialog, or %NULL
 *
 * Creates a new custom paper dialog.
 *
 * Returns: the new #CtkCustomPaperUnixDialog
 *
 * Since: 2.18
 */
CtkWidget *
_ctk_custom_paper_unix_dialog_new (CtkWindow   *parent,
                                   const gchar *title)
{
  CtkWidget *result;

  if (title == NULL)
    title = _("Manage Custom Sizes");

  result = g_object_new (CTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG,
                         "title", title,
                         "transient-for", parent,
                         "modal", parent != NULL,
                         "destroy-with-parent", TRUE,
                         "resizable", FALSE,
                         NULL);

  return result;
}

static void
printer_added_cb (CtkPrintBackend          *backend,
		  CtkPrinter               *printer,
		  CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter iter;
  gchar *str;

  if (ctk_printer_is_virtual (printer))
    return;

  str = g_strdup_printf ("<b>%s</b>",
			 ctk_printer_get_name (printer));

  ctk_list_store_append (priv->printer_list, &iter);
  ctk_list_store_set (priv->printer_list, &iter,
                      PRINTER_LIST_COL_NAME, str,
                      PRINTER_LIST_COL_PRINTER, printer,
                      -1);

  g_object_set_data_full (G_OBJECT (printer),
			  "ctk-print-tree-iter",
                          ctk_tree_iter_copy (&iter),
                          (GDestroyNotify) ctk_tree_iter_free);

  g_free (str);

  if (priv->waiting_for_printer != NULL &&
      strcmp (priv->waiting_for_printer,
	      ctk_printer_get_name (printer)) == 0)
    {
      ctk_combo_box_set_active_iter (CTK_COMBO_BOX (priv->printer_combo),
				     &iter);
      priv->waiting_for_printer = NULL;
    }
}

static void
printer_removed_cb (CtkPrintBackend        *backend,
		    CtkPrinter             *printer,
		    CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter *iter;

  iter = g_object_get_data (G_OBJECT (printer), "ctk-print-tree-iter");
  ctk_list_store_remove (CTK_LIST_STORE (priv->printer_list), iter);
}


static void
printer_status_cb (CtkPrintBackend        *backend,
                   CtkPrinter             *printer,
		   CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter *iter;
  gchar *str;

  iter = g_object_get_data (G_OBJECT (printer), "ctk-print-tree-iter");

  str = g_strdup_printf ("<b>%s</b>",
			 ctk_printer_get_name (printer));
  ctk_list_store_set (priv->printer_list, iter,
                      PRINTER_LIST_COL_NAME, str,
                      -1);
  g_free (str);
}

static void
printer_list_initialize (CtkCustomPaperUnixDialog *dialog,
			 CtkPrintBackend        *print_backend)
{
  GList *list, *node;

  g_return_if_fail (print_backend != NULL);

  g_signal_connect_object (print_backend,
			   "printer-added",
			   (GCallback) printer_added_cb,
			   G_OBJECT (dialog), 0);

  g_signal_connect_object (print_backend,
			   "printer-removed",
			   (GCallback) printer_removed_cb,
			   G_OBJECT (dialog), 0);

  g_signal_connect_object (print_backend,
			   "printer-status-changed",
			   (GCallback) printer_status_cb,
			   G_OBJECT (dialog), 0);

  list = ctk_print_backend_get_printer_list (print_backend);

  node = list;
  while (node != NULL)
    {
      printer_added_cb (print_backend, node->data, dialog);
      node = node->next;
    }

  g_list_free (list);
}

static void
load_print_backends (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  GList *node;

  if (g_module_supported ())
    priv->print_backends = ctk_print_backend_load_modules ();

  for (node = priv->print_backends; node != NULL; node = node->next)
    printer_list_initialize (dialog, CTK_PRINT_BACKEND (node->data));
}

static void unit_widget_changed (CtkCustomPaperUnixDialog *dialog);

static CtkWidget *
new_unit_widget (CtkCustomPaperUnixDialog *dialog,
		 CtkUnit                   unit,
		 CtkWidget                *mnemonic_label)
{
  CtkWidget *hbox, *button, *label;
  UnitWidget *data;

  data = g_new0 (UnitWidget, 1);
  data->display_unit = unit;

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);

  button = ctk_spin_button_new_with_range (0.0, 9999.0, 1);
  ctk_widget_set_valign (button, CTK_ALIGN_BASELINE);
  if (unit == CTK_UNIT_INCH)
    ctk_spin_button_set_digits (CTK_SPIN_BUTTON (button), 2);
  else
    ctk_spin_button_set_digits (CTK_SPIN_BUTTON (button), 1);

  ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 0);
  ctk_widget_show (button);

  data->spin_button = button;

  g_signal_connect_swapped (button, "value-changed",
			    G_CALLBACK (unit_widget_changed), dialog);

  if (unit == CTK_UNIT_INCH)
    label = ctk_label_new (_("inch"));
  else
    label = ctk_label_new (_("mm"));
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);

  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
  ctk_widget_show (label);
  ctk_label_set_mnemonic_widget (CTK_LABEL (mnemonic_label), button);

  g_object_set_data_full (G_OBJECT (hbox), "unit-data", data, g_free);

  return hbox;
}

static double
unit_widget_get (CtkWidget *unit_widget)
{
  UnitWidget *data = g_object_get_data (G_OBJECT (unit_widget), "unit-data");
  return _ctk_print_convert_to_mm (ctk_spin_button_get_value (CTK_SPIN_BUTTON (data->spin_button)),
				   data->display_unit);
}

static void
unit_widget_set (CtkWidget *unit_widget,
		 gdouble    value)
{
  UnitWidget *data;

  data = g_object_get_data (G_OBJECT (unit_widget), "unit-data");
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (data->spin_button),
			     _ctk_print_convert_from_mm (value, data->display_unit));
}

static void
custom_paper_printer_data_func (CtkCellLayout   *cell_layout,
				CtkCellRenderer *cell,
				CtkTreeModel    *tree_model,
				CtkTreeIter     *iter,
				gpointer         data)
{
  CtkPrinter *printer;

  ctk_tree_model_get (tree_model, iter,
		      PRINTER_LIST_COL_PRINTER, &printer, -1);

  if (printer)
    g_object_set (cell, "text",  ctk_printer_get_name (printer), NULL);
  else
    g_object_set (cell, "text",  _("Margins from Printer…"), NULL);

  if (printer)
    g_object_unref (printer);
}

static void
update_combo_sensitivity_from_printers (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter iter;
  gboolean sensitive;
  CtkTreeSelection *selection;
  CtkTreeModel *model;

  sensitive = FALSE;
  model = CTK_TREE_MODEL (priv->printer_list);
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->treeview));
  if (ctk_tree_model_get_iter_first (model, &iter) &&
      ctk_tree_model_iter_next (model, &iter) &&
      ctk_tree_selection_get_selected (selection, NULL, &iter))
    sensitive = TRUE;

  ctk_widget_set_sensitive (priv->printer_combo, sensitive);
}

static void
update_custom_widgets_from_list (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeSelection *selection;
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkPageSetup *page_setup;

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->treeview));
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->treeview));

  priv->non_user_change = TRUE;
  if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
      ctk_tree_model_get (model, &iter, 0, &page_setup, -1);

      unit_widget_set (priv->width_widget,
		       ctk_page_setup_get_paper_width (page_setup, CTK_UNIT_MM));
      unit_widget_set (priv->height_widget,
		       ctk_page_setup_get_paper_height (page_setup, CTK_UNIT_MM));
      unit_widget_set (priv->top_widget,
		       ctk_page_setup_get_top_margin (page_setup, CTK_UNIT_MM));
      unit_widget_set (priv->bottom_widget,
		       ctk_page_setup_get_bottom_margin (page_setup, CTK_UNIT_MM));
      unit_widget_set (priv->left_widget,
		       ctk_page_setup_get_left_margin (page_setup, CTK_UNIT_MM));
      unit_widget_set (priv->right_widget,
		       ctk_page_setup_get_right_margin (page_setup, CTK_UNIT_MM));

      ctk_widget_set_sensitive (priv->values_box, TRUE);
    }
  else
    {
      ctk_widget_set_sensitive (priv->values_box, FALSE);
    }

  if (priv->printer_list)
    update_combo_sensitivity_from_printers (dialog);
  priv->non_user_change = FALSE;
}

static void
selected_custom_paper_changed (CtkTreeSelection         *selection,
			       CtkCustomPaperUnixDialog *dialog)
{
  update_custom_widgets_from_list (dialog);
}

static void
unit_widget_changed (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  gdouble w, h, top, bottom, left, right;
  CtkTreeSelection *selection;
  CtkTreeIter iter;
  CtkPageSetup *page_setup;
  CtkPaperSize *paper_size;

  if (priv->non_user_change)
    return;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->treeview));

  if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
      ctk_tree_model_get (CTK_TREE_MODEL (priv->custom_paper_list), &iter, 0, &page_setup, -1);

      w = unit_widget_get (priv->width_widget);
      h = unit_widget_get (priv->height_widget);

      paper_size = ctk_page_setup_get_paper_size (page_setup);
      ctk_paper_size_set_size (paper_size, w, h, CTK_UNIT_MM);

      top = unit_widget_get (priv->top_widget);
      bottom = unit_widget_get (priv->bottom_widget);
      left = unit_widget_get (priv->left_widget);
      right = unit_widget_get (priv->right_widget);

      ctk_page_setup_set_top_margin (page_setup, top, CTK_UNIT_MM);
      ctk_page_setup_set_bottom_margin (page_setup, bottom, CTK_UNIT_MM);
      ctk_page_setup_set_left_margin (page_setup, left, CTK_UNIT_MM);
      ctk_page_setup_set_right_margin (page_setup, right, CTK_UNIT_MM);

      g_object_unref (page_setup);
    }
}

static gboolean
custom_paper_name_used (CtkCustomPaperUnixDialog *dialog,
			const gchar              *name)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkPageSetup *page_setup;
  CtkPaperSize *paper_size;

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->treeview));

  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      do
	{
	  ctk_tree_model_get (model, &iter, 0, &page_setup, -1);
	  paper_size = ctk_page_setup_get_paper_size (page_setup);
	  if (strcmp (name,
		      ctk_paper_size_get_name (paper_size)) == 0)
	    {
	      g_object_unref (page_setup);
	      return TRUE;
	    }
	  g_object_unref (page_setup);
	} while (ctk_tree_model_iter_next (model, &iter));
    }

  return FALSE;
}

static void
add_custom_paper (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkListStore *store;
  CtkPageSetup *page_setup;
  CtkPaperSize *paper_size;
  CtkTreeSelection *selection;
  CtkTreePath *path;
  CtkTreeIter iter;
  gchar *name;
  gint i;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->treeview));
  store = priv->custom_paper_list;

  i = 1;
  name = NULL;
  do
    {
      g_free (name);
      name = g_strdup_printf (_("Custom Size %d"), i);
      i++;
    } while (custom_paper_name_used (dialog, name));

  page_setup = ctk_page_setup_new ();
  paper_size = ctk_paper_size_new_custom (name, name,
					  ctk_page_setup_get_paper_width (page_setup, CTK_UNIT_MM),
					  ctk_page_setup_get_paper_height (page_setup, CTK_UNIT_MM),
					  CTK_UNIT_MM);
  ctk_page_setup_set_paper_size (page_setup, paper_size);
  ctk_paper_size_free (paper_size);

  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, page_setup, -1);
  g_object_unref (page_setup);

  ctk_tree_selection_select_iter (selection, &iter);
  path = ctk_tree_model_get_path (CTK_TREE_MODEL (store), &iter);
  ctk_widget_grab_focus (priv->treeview);
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (priv->treeview), path,
			    priv->text_column, TRUE);
  ctk_tree_path_free (path);
  g_free (name);
}

static void
remove_custom_paper (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeSelection *selection;
  CtkTreeIter iter;
  CtkListStore *store;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->treeview));
  store = priv->custom_paper_list;

  if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
      CtkTreePath *path = ctk_tree_model_get_path (CTK_TREE_MODEL (store), &iter);
      ctk_list_store_remove (store, &iter);

      if (ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, path))
	ctk_tree_selection_select_iter (selection, &iter);
      else if (ctk_tree_path_prev (path) && ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, path))
	ctk_tree_selection_select_iter (selection, &iter);

      ctk_tree_path_free (path);
    }
}

static void
set_margins_from_printer (CtkCustomPaperUnixDialog *dialog,
			  CtkPrinter               *printer)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  gdouble top, bottom, left, right;

  top = bottom = left = right = 0;
  if (!ctk_printer_get_hard_margins (printer, &top, &bottom, &left, &right))
    return;

  priv->non_user_change = TRUE;
  unit_widget_set (priv->top_widget, _ctk_print_convert_to_mm (top, CTK_UNIT_POINTS));
  unit_widget_set (priv->bottom_widget, _ctk_print_convert_to_mm (bottom, CTK_UNIT_POINTS));
  unit_widget_set (priv->left_widget, _ctk_print_convert_to_mm (left, CTK_UNIT_POINTS));
  unit_widget_set (priv->right_widget, _ctk_print_convert_to_mm (right, CTK_UNIT_POINTS));
  priv->non_user_change = FALSE;

  /* Only send one change */
  unit_widget_changed (dialog);
}

static void
get_margins_finished_callback (CtkPrinter               *printer,
			       gboolean                  success,
			       CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;

  g_signal_handler_disconnect (priv->request_details_printer,
			       priv->request_details_tag);
  g_object_unref (priv->request_details_printer);
  priv->request_details_tag = 0;
  priv->request_details_printer = NULL;

  if (success)
    set_margins_from_printer (dialog, printer);

  ctk_combo_box_set_active (CTK_COMBO_BOX (priv->printer_combo), 0);
}

static void
margins_from_printer_changed (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter iter;
  CtkComboBox *combo;
  CtkPrinter *printer;

  combo = CTK_COMBO_BOX (priv->printer_combo);

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
				   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }

  if (ctk_combo_box_get_active_iter (combo, &iter))
    {
      ctk_tree_model_get (ctk_combo_box_get_model (combo), &iter,
			  PRINTER_LIST_COL_PRINTER, &printer, -1);

      if (printer)
	{
	  if (ctk_printer_has_details (printer))
	    {
	      set_margins_from_printer (dialog, printer);
	      ctk_combo_box_set_active (combo, 0);
	    }
	  else
	    {
	      priv->request_details_printer = g_object_ref (printer);
	      priv->request_details_tag =
		g_signal_connect (printer, "details-acquired",
				  G_CALLBACK (get_margins_finished_callback), dialog);
	      ctk_printer_request_details (printer);
	    }

	  g_object_unref (printer);
	}
    }
}

static void
custom_size_name_edited (CtkCellRenderer          *cell,
			 gchar                    *path_string,
			 gchar                    *new_text,
			 CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkListStore *store;
  CtkPageSetup *page_setup;
  CtkPaperSize *paper_size;

  store = priv->custom_paper_list;
  path = ctk_tree_path_new_from_string (path_string);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, path);
  ctk_tree_model_get (CTK_TREE_MODEL (store), &iter, 0, &page_setup, -1);
  ctk_tree_path_free (path);

  paper_size = ctk_paper_size_new_custom (new_text, new_text,
					  ctk_page_setup_get_paper_width (page_setup, CTK_UNIT_MM),
					  ctk_page_setup_get_paper_height (page_setup, CTK_UNIT_MM),
					  CTK_UNIT_MM);
  ctk_page_setup_set_paper_size (page_setup, paper_size);
  ctk_paper_size_free (paper_size);

  g_object_unref (page_setup);
}

static void
custom_name_func (CtkTreeViewColumn *tree_column,
		  CtkCellRenderer   *cell,
		  CtkTreeModel      *tree_model,
		  CtkTreeIter       *iter,
		  gpointer           data)
{
  CtkPageSetup *page_setup;
  CtkPaperSize *paper_size;

  ctk_tree_model_get (tree_model, iter, 0, &page_setup, -1);
  if (page_setup)
    {
      paper_size = ctk_page_setup_get_paper_size (page_setup);
      g_object_set (cell, "text",  ctk_paper_size_get_display_name (paper_size), NULL);
      g_object_unref (page_setup);
    }
}

static CtkWidget *
wrap_in_frame (const gchar *label,
               CtkWidget   *child)
{
  CtkWidget *frame, *label_widget;
  gchar *bold_text;

  label_widget = ctk_label_new (NULL);
  ctk_widget_set_halign (label_widget, CTK_ALIGN_START);
  ctk_widget_set_valign (label_widget, CTK_ALIGN_CENTER);
  ctk_widget_show (label_widget);

  bold_text = g_markup_printf_escaped ("<b>%s</b>", label);
  ctk_label_set_markup (CTK_LABEL (label_widget), bold_text);
  g_free (bold_text);

  frame = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_box_pack_start (CTK_BOX (frame), label_widget, FALSE, FALSE, 0);

  ctk_widget_set_margin_start (child, 12);
  ctk_widget_set_halign (child, CTK_ALIGN_FILL);
  ctk_widget_set_valign (child, CTK_ALIGN_FILL);

  ctk_box_pack_start (CTK_BOX (frame), child, FALSE, FALSE, 0);

  ctk_widget_show (frame);

  return frame;
}

static CtkWidget *
toolbutton_new (CtkCustomPaperUnixDialog *dialog,
                GIcon                    *icon,
                gboolean                  sensitive,
                gboolean                  show,
                GCallback                 callback)
{
  CtkToolItem *item;
  CtkWidget *image;

  item = ctk_tool_button_new (NULL, NULL);
  image = ctk_image_new_from_gicon (icon, CTK_ICON_SIZE_SMALL_TOOLBAR);
  ctk_widget_show (image);
  ctk_tool_button_set_icon_widget (CTK_TOOL_BUTTON (item), image);

  ctk_widget_set_sensitive (CTK_WIDGET (item), sensitive);
  g_signal_connect_swapped (item, "clicked", callback, dialog);

  if (show)
    ctk_widget_show (CTK_WIDGET (item));

  return CTK_WIDGET (item);
}

static void
populate_dialog (CtkCustomPaperUnixDialog *dialog)
{
  CtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  CtkDialog *cpu_dialog = CTK_DIALOG (dialog);
  CtkWidget *action_area, *content_area;
  CtkWidget *grid, *label, *widget, *frame, *combo;
  CtkWidget *hbox, *vbox, *treeview, *scrolled, *toolbar, *button;
  CtkCellRenderer *cell;
  CtkTreeViewColumn *column;
  CtkTreeIter iter;
  CtkTreeSelection *selection;
  CtkUnit user_units;
  GIcon *icon;
  CtkStyleContext *context;

  content_area = ctk_dialog_get_content_area (cpu_dialog);
  action_area = ctk_dialog_get_action_area (cpu_dialog);

  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
  ctk_box_set_spacing (CTK_BOX (content_area), 2); /* 2 * 5 + 2 = 12 */
  ctk_container_set_border_width (CTK_CONTAINER (action_area), 5);
  ctk_box_set_spacing (CTK_BOX (action_area), 6);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 18);
  ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
  ctk_box_pack_start (CTK_BOX (content_area), hbox, TRUE, TRUE, 0);
  ctk_widget_show (hbox);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_box_pack_start (CTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  ctk_widget_show (vbox);

  scrolled = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled),
                                  CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled),
                                       CTK_SHADOW_IN);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  ctk_widget_show (scrolled);

  context = ctk_widget_get_style_context (scrolled);
  ctk_style_context_set_junction_sides (context, CTK_JUNCTION_BOTTOM);

  treeview = ctk_tree_view_new_with_model (CTK_TREE_MODEL (priv->custom_paper_list));
  priv->treeview = treeview;
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (treeview), FALSE);
  ctk_widget_set_size_request (treeview, 140, -1);

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview));
  ctk_tree_selection_set_mode (selection, CTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed", G_CALLBACK (selected_custom_paper_changed), dialog);

  cell = ctk_cell_renderer_text_new ();
  g_object_set (cell, "editable", TRUE, NULL);
  g_signal_connect (cell, "edited",
                    G_CALLBACK (custom_size_name_edited), dialog);
  priv->text_column = column =
    ctk_tree_view_column_new_with_attributes ("paper", cell, NULL);
  ctk_tree_view_column_set_cell_data_func  (column, cell, custom_name_func, NULL, NULL);

  ctk_tree_view_append_column (CTK_TREE_VIEW (treeview), column);

  ctk_container_add (CTK_CONTAINER (scrolled), treeview);
  ctk_widget_show (treeview);

  toolbar = ctk_toolbar_new ();
  ctk_toolbar_set_icon_size (CTK_TOOLBAR (toolbar), CTK_ICON_SIZE_MENU);

  context = ctk_widget_get_style_context (toolbar);
  ctk_style_context_set_junction_sides (context, CTK_JUNCTION_TOP);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_INLINE_TOOLBAR);

  ctk_box_pack_start (CTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  ctk_widget_show (toolbar);

  icon = g_themed_icon_new_with_default_fallbacks ("list-add-symbolic");
  button = toolbutton_new (dialog, icon, TRUE, TRUE, G_CALLBACK (add_custom_paper));
  g_object_unref (icon);

  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), CTK_TOOL_ITEM (button), 0);

  icon = g_themed_icon_new_with_default_fallbacks ("list-remove-symbolic");
  button = toolbutton_new (dialog, icon, TRUE, TRUE, G_CALLBACK (remove_custom_paper));
  g_object_unref (icon);

  ctk_toolbar_insert (CTK_TOOLBAR (toolbar), CTK_TOOL_ITEM (button), 1);

  user_units = _ctk_print_get_default_user_units ();

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
  priv->values_box = vbox;
  ctk_box_pack_start (CTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  ctk_widget_show (vbox);

  grid = ctk_grid_new ();

  ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 12);

  label = ctk_label_new_with_mnemonic (_("_Width:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_widget_show (label);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);

  widget = new_unit_widget (dialog, user_units, label);
  priv->width_widget = widget;
  ctk_grid_attach (CTK_GRID (grid), widget, 1, 0, 1, 1);
  ctk_widget_show (widget);

  label = ctk_label_new_with_mnemonic (_("_Height:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_widget_show (label);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);

  widget = new_unit_widget (dialog, user_units, label);
  priv->height_widget = widget;
  ctk_grid_attach (CTK_GRID (grid), widget, 1, 1, 1, 1);
  ctk_widget_show (widget);

  frame = wrap_in_frame (_("Paper Size"), grid);
  ctk_widget_show (grid);
  ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);
  ctk_widget_show (frame);

  grid = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 12);

  label = ctk_label_new_with_mnemonic (_("_Top:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
  ctk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->top_widget = widget;
  ctk_grid_attach (CTK_GRID (grid), widget, 1, 0, 1, 1);
  ctk_widget_show (widget);

  label = ctk_label_new_with_mnemonic (_("_Bottom:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);
  ctk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->bottom_widget = widget;
  ctk_grid_attach (CTK_GRID (grid), widget, 1, 1, 1, 1);
  ctk_widget_show (widget);

  label = ctk_label_new_with_mnemonic (_("_Left:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 2, 1, 1);
  ctk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->left_widget = widget;
  ctk_grid_attach (CTK_GRID (grid), widget, 1, 2, 1, 1);
  ctk_widget_show (widget);

  label = ctk_label_new_with_mnemonic (_("_Right:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 3, 1, 1);
  ctk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->right_widget = widget;
  ctk_grid_attach (CTK_GRID (grid), widget, 1, 3, 1, 1);
  ctk_widget_show (widget);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_grid_attach (CTK_GRID (grid), hbox, 0, 4, 2, 1);
  ctk_widget_show (hbox);

  combo = ctk_combo_box_new_with_model (CTK_TREE_MODEL (priv->printer_list));
  priv->printer_combo = combo;

  priv->printer_inserted_tag =
    g_signal_connect_swapped (priv->printer_list, "row-inserted",
			      G_CALLBACK (update_combo_sensitivity_from_printers), dialog);
  priv->printer_removed_tag =
    g_signal_connect_swapped (priv->printer_list, "row-deleted",
			      G_CALLBACK (update_combo_sensitivity_from_printers), dialog);
  update_combo_sensitivity_from_printers (dialog);

  cell = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), cell, TRUE);
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (combo), cell,
				      custom_paper_printer_data_func,
				      NULL, NULL);

  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);
  ctk_box_pack_start (CTK_BOX (hbox), combo, FALSE, FALSE, 0);
  ctk_widget_show (combo);

  g_signal_connect_swapped (combo, "changed",
			    G_CALLBACK (margins_from_printer_changed), dialog);

  frame = wrap_in_frame (_("Paper Margins"), grid);
  ctk_widget_show (grid);
  ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);
  ctk_widget_show (frame);

  update_custom_widgets_from_list (dialog);

  /* If no custom sizes, add one */
  if (!ctk_tree_model_get_iter_first (CTK_TREE_MODEL (priv->custom_paper_list),
				      &iter))
    {
      /* Need to realize treeview so we can start the rename */
      ctk_widget_realize (treeview);
      add_custom_paper (dialog);
    }

  load_print_backends (dialog);
}
