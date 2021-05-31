/* GtkPageSetupUnixDialog
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */


#include "config.h"
#include <string.h>
#include <locale.h>

#include "ctkintl.h"
#include "ctkprivate.h"

#include "ctkliststore.h"
#include "ctktreeviewcolumn.h"
#include "ctktreeselection.h"
#include "ctktreemodel.h"
#include "ctkbutton.h"
#include "ctkscrolledwindow.h"
#include "ctkcombobox.h"
#include "ctktogglebutton.h"
#include "ctkradiobutton.h"
#include "ctklabel.h"
#include "ctkgrid.h"
#include "ctkcelllayout.h"
#include "ctkcellrenderertext.h"

#include "ctkpagesetupunixdialog.h"
#include "ctkcustompaperunixdialog.h"
#include "ctkprintbackend.h"
#include "ctkpapersize.h"
#include "ctkprintutils.h"
#include "ctkdialogprivate.h"

/**
 * SECTION:ctkpagesetupunixdialog
 * @Short_description: A page setup dialog
 * @Title: GtkPageSetupUnixDialog
 * @Include: ctk/ctkunixprint.h
 *
 * #GtkPageSetupUnixDialog implements a page setup dialog for platforms
 * which don’t provide a native page setup dialog, like Unix. It can
 * be used very much like any other GTK+ dialog, at the cost of
 * the portability offered by the
 * [high-level printing API][ctk3-High-level-Printing-API]
 *
 * Printing support was added in GTK+ 2.10.
 */


struct _GtkPageSetupUnixDialogPrivate
{
  GtkListStore *printer_list;
  GtkListStore *page_setup_list;
  GtkListStore *custom_paper_list;

  GList *print_backends;

  GtkWidget *printer_combo;
  GtkWidget *paper_size_combo;
  GtkWidget *paper_size_label;
  GtkCellRenderer *paper_size_cell;

  GtkWidget *portrait_radio;
  GtkWidget *reverse_portrait_radio;
  GtkWidget *landscape_radio;
  GtkWidget *reverse_landscape_radio;

  gulong request_details_tag;
  GtkPrinter *request_details_printer;

  GtkPrintSettings *print_settings;

  /* Save last setup so we can re-set it after selecting manage custom sizes */
  GtkPageSetup *last_setup;

  gchar *waiting_for_printer;
};

/* Keep these in line with GtkListStores defined in ctkpagesetupunixprintdialog.ui */
enum {
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_PRINTER,
  PRINTER_LIST_N_COLS
};

enum {
  PAGE_SETUP_LIST_COL_PAGE_SETUP,
  PAGE_SETUP_LIST_COL_IS_SEPARATOR,
  PAGE_SETUP_LIST_N_COLS
};

G_DEFINE_TYPE_WITH_PRIVATE (GtkPageSetupUnixDialog, ctk_page_setup_unix_dialog, CTK_TYPE_DIALOG)

static void ctk_page_setup_unix_dialog_finalize  (GObject                *object);
static void fill_paper_sizes_from_printer        (GtkPageSetupUnixDialog *dialog,
                                                  GtkPrinter             *printer);
static void printer_added_cb                     (GtkPrintBackend        *backend,
                                                  GtkPrinter             *printer,
                                                  GtkPageSetupUnixDialog *dialog);
static void printer_removed_cb                   (GtkPrintBackend        *backend,
                                                  GtkPrinter             *printer,
                                                  GtkPageSetupUnixDialog *dialog);
static void printer_status_cb                    (GtkPrintBackend        *backend,
                                                  GtkPrinter             *printer,
                                                  GtkPageSetupUnixDialog *dialog);
static void printer_changed_callback             (GtkComboBox            *combo_box,
						  GtkPageSetupUnixDialog *dialog);
static void paper_size_changed                   (GtkComboBox            *combo_box,
						  GtkPageSetupUnixDialog *dialog);
static void page_name_func                       (GtkCellLayout          *cell_layout,
						  GtkCellRenderer        *cell,
						  GtkTreeModel           *tree_model,
						  GtkTreeIter            *iter,
						  gpointer                data);
static void load_print_backends                  (GtkPageSetupUnixDialog *dialog);
static gboolean paper_size_row_is_separator      (GtkTreeModel           *model,
						  GtkTreeIter            *iter,
						  gpointer                data);


static const gchar common_paper_sizes[][16] = {
  "na_letter",
  "na_legal",
  "iso_a4",
  "iso_a5",
  "roc_16k",
  "iso_b5",
  "jis_b5",
  "na_number-10",
  "iso_dl",
  "jpn_chou3",
  "na_ledger",
  "iso_a3"
};


static void
ctk_page_setup_unix_dialog_class_init (GtkPageSetupUnixDialogClass *class)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (class);
  widget_class = CTK_WIDGET_CLASS (class);

  object_class->finalize = ctk_page_setup_unix_dialog_finalize;

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkpagesetupunixdialog.ui");

  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, printer_list);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, page_setup_list);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, custom_paper_list);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, printer_combo);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, paper_size_combo);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, paper_size_label);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, paper_size_cell);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, portrait_radio);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, reverse_portrait_radio);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, landscape_radio);
  ctk_widget_class_bind_template_child_private (widget_class, GtkPageSetupUnixDialog, reverse_landscape_radio);

  ctk_widget_class_bind_template_callback (widget_class, printer_changed_callback);
  ctk_widget_class_bind_template_callback (widget_class, paper_size_changed);
}

static void
ctk_page_setup_unix_dialog_init (GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv;
  GtkTreeIter iter;
  gchar *tmp;

  priv = dialog->priv = ctk_page_setup_unix_dialog_get_instance_private (dialog);

  priv->print_backends = NULL;

  ctk_widget_init_template (CTK_WIDGET (dialog));
  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (dialog));
  ctk_dialog_add_buttons (CTK_DIALOG (dialog),
                          _("_Cancel"), CTK_RESPONSE_CANCEL,
                          _("_Apply"), CTK_RESPONSE_OK,
                          NULL);
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_dialog_set_alternative_button_order (CTK_DIALOG (dialog),
                                           CTK_RESPONSE_OK,
                                           CTK_RESPONSE_CANCEL,
                                           -1);
G_GNUC_END_IGNORE_DEPRECATIONS

  /* Do this in code, we want the translatable strings without the markup */
  ctk_list_store_append (priv->printer_list, &iter);
  tmp = g_strdup_printf ("<b>%s</b>\n%s", _("Any Printer"), _("For portable documents"));
  ctk_list_store_set (priv->printer_list, &iter,
                      PRINTER_LIST_COL_NAME, tmp,
                      PRINTER_LIST_COL_PRINTER, NULL,
                      -1);
  g_free (tmp);

  /* After adding the above row, set it active */
  ctk_combo_box_set_active (CTK_COMBO_BOX (priv->printer_combo), 0);

  /* Setup cell data func and separator func in code */
  ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (priv->paper_size_combo),
					paper_size_row_is_separator, NULL, NULL);
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (priv->paper_size_combo),
				      priv->paper_size_cell,
                                      page_name_func, NULL, NULL);

  /* Load data */
  _ctk_print_load_custom_papers (priv->custom_paper_list);
  load_print_backends (dialog);
}

static void
ctk_page_setup_unix_dialog_finalize (GObject *object)
{
  GtkPageSetupUnixDialog *dialog = CTK_PAGE_SETUP_UNIX_DIALOG (object);
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkPrintBackend *backend;
  GList *node;

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
                                   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }

  if (priv->printer_list)
    {
      g_object_unref (priv->printer_list);
      priv->printer_list = NULL;
    }

  if (priv->page_setup_list)
    {
      g_object_unref (priv->page_setup_list);
      priv->page_setup_list = NULL;
    }

  if (priv->custom_paper_list)
    {
      g_object_unref (priv->custom_paper_list);
      priv->custom_paper_list = NULL;
    }

  if (priv->print_settings)
    {
      g_object_unref (priv->print_settings);
      priv->print_settings = NULL;
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

  G_OBJECT_CLASS (ctk_page_setup_unix_dialog_parent_class)->finalize (object);
}

static void
printer_added_cb (GtkPrintBackend        *backend,
                  GtkPrinter             *printer,
                  GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkTreeIter iter;
  gchar *str;
  const gchar *location;

  if (ctk_printer_is_virtual (printer))
    return;

  location = ctk_printer_get_location (printer);
  if (location == NULL)
    location = "";
  str = g_strdup_printf ("<b>%s</b>\n%s",
                         ctk_printer_get_name (printer),
                         location);

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
printer_removed_cb (GtkPrintBackend        *backend,
                    GtkPrinter             *printer,
                    GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkTreeIter *iter;

  iter = g_object_get_data (G_OBJECT (printer), "ctk-print-tree-iter");
  ctk_list_store_remove (CTK_LIST_STORE (priv->printer_list), iter);
}


static void
printer_status_cb (GtkPrintBackend        *backend,
                   GtkPrinter             *printer,
                   GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkTreeIter *iter;
  gchar *str;
  const gchar *location;

  iter = g_object_get_data (G_OBJECT (printer), "ctk-print-tree-iter");

  location = ctk_printer_get_location (printer);
  if (location == NULL)
    location = "";
  str = g_strdup_printf ("<b>%s</b>\n%s",
                         ctk_printer_get_name (printer),
                         location);
  ctk_list_store_set (priv->printer_list, iter,
                      PRINTER_LIST_COL_NAME, str,
                      -1);
  g_free (str);
}

static void
printer_list_initialize (GtkPageSetupUnixDialog *dialog,
                         GtkPrintBackend        *print_backend)
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
load_print_backends (GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GList *node;

  if (g_module_supported ())
    priv->print_backends = ctk_print_backend_load_modules ();

  for (node = priv->print_backends; node != NULL; node = node->next)
    printer_list_initialize (dialog, CTK_PRINT_BACKEND (node->data));
}

static gboolean
paper_size_row_is_separator (GtkTreeModel *model,
                             GtkTreeIter  *iter,
                             gpointer      data)
{
  gboolean separator;

  ctk_tree_model_get (model, iter, PAGE_SETUP_LIST_COL_IS_SEPARATOR, &separator, -1);
  return separator;
}

static GtkPageSetup *
get_current_page_setup (GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkPageSetup *current_page_setup;
  GtkComboBox *combo_box;
  GtkTreeIter iter;

  current_page_setup = NULL;

  combo_box = CTK_COMBO_BOX (priv->paper_size_combo);
  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    ctk_tree_model_get (CTK_TREE_MODEL (priv->page_setup_list), &iter,
                        PAGE_SETUP_LIST_COL_PAGE_SETUP, &current_page_setup, -1);

  if (current_page_setup)
    return current_page_setup;

  /* No selected page size, return the default one.
   * This is used to set the first page setup when the dialog is created
   * as there is no selection on the first printer_changed.
   */
  return ctk_page_setup_new ();
}

static gboolean
page_setup_is_equal (GtkPageSetup *a,
                     GtkPageSetup *b)
{
  return
    ctk_paper_size_is_equal (ctk_page_setup_get_paper_size (a),
                             ctk_page_setup_get_paper_size (b)) &&
    ctk_page_setup_get_top_margin (a, CTK_UNIT_MM) == ctk_page_setup_get_top_margin (b, CTK_UNIT_MM) &&
    ctk_page_setup_get_bottom_margin (a, CTK_UNIT_MM) == ctk_page_setup_get_bottom_margin (b, CTK_UNIT_MM) &&
    ctk_page_setup_get_left_margin (a, CTK_UNIT_MM) == ctk_page_setup_get_left_margin (b, CTK_UNIT_MM) &&
    ctk_page_setup_get_right_margin (a, CTK_UNIT_MM) == ctk_page_setup_get_right_margin (b, CTK_UNIT_MM);
}

static gboolean
page_setup_is_same_size (GtkPageSetup *a,
                         GtkPageSetup *b)
{
  return ctk_paper_size_is_equal (ctk_page_setup_get_paper_size (a),
                                  ctk_page_setup_get_paper_size (b));
}

static gboolean
set_paper_size (GtkPageSetupUnixDialog *dialog,
                GtkPageSetup           *page_setup,
                gboolean                size_only,
                gboolean                add_item)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkPageSetup *list_page_setup;

  model = CTK_TREE_MODEL (priv->page_setup_list);

  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          ctk_tree_model_get (CTK_TREE_MODEL (priv->page_setup_list), &iter,
                              PAGE_SETUP_LIST_COL_PAGE_SETUP, &list_page_setup, -1);
          if (list_page_setup == NULL)
            continue;

          if ((size_only && page_setup_is_same_size (page_setup, list_page_setup)) ||
              (!size_only && page_setup_is_equal (page_setup, list_page_setup)))
            {
              ctk_combo_box_set_active_iter (CTK_COMBO_BOX (priv->paper_size_combo),
                                             &iter);
              g_object_unref (list_page_setup);
              return TRUE;
            }

          g_object_unref (list_page_setup);

        } while (ctk_tree_model_iter_next (model, &iter));
    }

  if (add_item)
    {
      ctk_list_store_append (priv->page_setup_list, &iter);
      ctk_list_store_set (priv->page_setup_list, &iter,
                          PAGE_SETUP_LIST_COL_IS_SEPARATOR, TRUE,
                          -1);
      ctk_list_store_append (priv->page_setup_list, &iter);
      ctk_list_store_set (priv->page_setup_list, &iter,
                          PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
                          -1);
      ctk_combo_box_set_active_iter (CTK_COMBO_BOX (priv->paper_size_combo),
                                     &iter);
      return TRUE;
    }

  return FALSE;
}

static void
fill_custom_paper_sizes (GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkTreeIter iter, paper_iter;
  GtkTreeModel *model;

  model = CTK_TREE_MODEL (priv->custom_paper_list);
  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      ctk_list_store_append (priv->page_setup_list, &paper_iter);
      ctk_list_store_set (priv->page_setup_list, &paper_iter,
                          PAGE_SETUP_LIST_COL_IS_SEPARATOR, TRUE,
                          -1);
      do
        {
          GtkPageSetup *page_setup;
          ctk_tree_model_get (model, &iter, 0, &page_setup, -1);

          ctk_list_store_append (priv->page_setup_list, &paper_iter);
          ctk_list_store_set (priv->page_setup_list, &paper_iter,
                              PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
                              -1);

          g_object_unref (page_setup);
        } while (ctk_tree_model_iter_next (model, &iter));
    }

  ctk_list_store_append (priv->page_setup_list, &paper_iter);
  ctk_list_store_set (priv->page_setup_list, &paper_iter,
                      PAGE_SETUP_LIST_COL_IS_SEPARATOR, TRUE,
                      -1);
  ctk_list_store_append (priv->page_setup_list, &paper_iter);
  ctk_list_store_set (priv->page_setup_list, &paper_iter,
                      PAGE_SETUP_LIST_COL_PAGE_SETUP, NULL,
                      -1);
}

static void
fill_paper_sizes_from_printer (GtkPageSetupUnixDialog *dialog,
                               GtkPrinter             *printer)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GList *list, *l;
  GtkPageSetup *current_page_setup, *page_setup;
  GtkPaperSize *paper_size;
  GtkTreeIter iter;
  gint i;

  ctk_list_store_clear (priv->page_setup_list);

  if (printer == NULL)
    {
      for (i = 0; i < G_N_ELEMENTS (common_paper_sizes); i++)
        {
          page_setup = ctk_page_setup_new ();
          paper_size = ctk_paper_size_new (common_paper_sizes[i]);
          ctk_page_setup_set_paper_size_and_default_margins (page_setup, paper_size);
          ctk_paper_size_free (paper_size);

          ctk_list_store_append (priv->page_setup_list, &iter);
          ctk_list_store_set (priv->page_setup_list, &iter,
                              PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
                              -1);
          g_object_unref (page_setup);
        }
    }
  else
    {
      list = ctk_printer_list_papers (printer);
      /* TODO: We should really sort this list so interesting size
         are at the top */
      for (l = list; l != NULL; l = l->next)
        {
          page_setup = l->data;
          ctk_list_store_append (priv->page_setup_list, &iter);
          ctk_list_store_set (priv->page_setup_list, &iter,
                              PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
                              -1);
          g_object_unref (page_setup);
        }
      g_list_free (list);
    }

  fill_custom_paper_sizes (dialog);

  current_page_setup = NULL;

  /* When selecting a different printer, select its default paper size */
  if (printer != NULL)
    current_page_setup = ctk_printer_get_default_page_size (printer);

  if (current_page_setup == NULL)
    current_page_setup = get_current_page_setup (dialog);

  if (!set_paper_size (dialog, current_page_setup, FALSE, FALSE))
    set_paper_size (dialog, current_page_setup, TRUE, TRUE);

  if (current_page_setup)
    g_object_unref (current_page_setup);
}

static void
printer_changed_finished_callback (GtkPrinter             *printer,
                                   gboolean                success,
                                   GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  g_signal_handler_disconnect (priv->request_details_printer,
                               priv->request_details_tag);
  g_object_unref (priv->request_details_printer);
  priv->request_details_tag = 0;
  priv->request_details_printer = NULL;

  if (success)
    fill_paper_sizes_from_printer (dialog, printer);

}

static void
printer_changed_callback (GtkComboBox            *combo_box,
                          GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkPrinter *printer;
  GtkTreeIter iter;

  /* If we're waiting for a specific printer but the user changed
   * to another printer, cancel that wait.
   */
  g_clear_pointer (&priv->waiting_for_printer, g_free);

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
                                   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }

  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    {
      ctk_tree_model_get (ctk_combo_box_get_model (combo_box), &iter,
                          PRINTER_LIST_COL_PRINTER, &printer, -1);

      if (printer == NULL || ctk_printer_has_details (printer))
        fill_paper_sizes_from_printer (dialog, printer);
      else
        {
          priv->request_details_printer = g_object_ref (printer);
          priv->request_details_tag =
            g_signal_connect (printer, "details-acquired",
                              G_CALLBACK (printer_changed_finished_callback), dialog);
          ctk_printer_request_details (printer);

        }

      if (printer)
        g_object_unref (printer);

      if (priv->print_settings)
        {
          const char *name = NULL;

          if (printer)
            name = ctk_printer_get_name (printer);

          ctk_print_settings_set (priv->print_settings,
                                  "format-for-printer", name);
        }
    }
}

/* We do this munging because we don't want to show zero digits
   after the decimal point, and not to many such digits if they
   are nonzero. I wish printf let you specify max precision for %f... */
static gchar *
double_to_string (gdouble d,
                  GtkUnit unit)
{
  gchar *val, *p;
  struct lconv *locale_data;
  const gchar *decimal_point;
  gint decimal_point_len;

  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);

  /* Max two decimal digits for inch, max one for mm */
  if (unit == CTK_UNIT_INCH)
    val = g_strdup_printf ("%.2f", d);
  else
    val = g_strdup_printf ("%.1f", d);

  if (strstr (val, decimal_point))
    {
      p = val + strlen (val) - 1;
      while (*p == '0')
        p--;
      if (p - val + 1 >= decimal_point_len &&
          strncmp (p - (decimal_point_len - 1), decimal_point, decimal_point_len) == 0)
        p -= decimal_point_len;
      p[1] = '\0';
    }

  return val;
}


static void
custom_paper_dialog_response_cb (GtkDialog *custom_paper_dialog,
                                 gint       response_id,
                                 gpointer   user_data)
{
  GtkPageSetupUnixDialog *page_setup_dialog = CTK_PAGE_SETUP_UNIX_DIALOG (user_data);
  GtkPageSetupUnixDialogPrivate *priv = page_setup_dialog->priv;

  _ctk_print_load_custom_papers (priv->custom_paper_list);

  /* Update printer page list */
  printer_changed_callback (CTK_COMBO_BOX (priv->printer_combo), page_setup_dialog);

  ctk_widget_destroy (CTK_WIDGET (custom_paper_dialog));
}

static void
paper_size_changed (GtkComboBox            *combo_box,
                    GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkTreeIter iter;
  GtkPageSetup *page_setup, *last_page_setup;
  GtkUnit unit;
  gchar *str, *w, *h;
  gchar *top, *bottom, *left, *right;
  GtkLabel *label;
  const gchar *unit_str;

  label = CTK_LABEL (priv->paper_size_label);

  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    {
      ctk_tree_model_get (ctk_combo_box_get_model (combo_box),
                          &iter, PAGE_SETUP_LIST_COL_PAGE_SETUP, &page_setup, -1);

      if (page_setup == NULL)
        {
          GtkWidget *custom_paper_dialog;

          /* Change from "manage" menu item to last value */
          if (priv->last_setup)
            last_page_setup = g_object_ref (priv->last_setup);
          else
            last_page_setup = ctk_page_setup_new (); /* "good" default */
          set_paper_size (dialog, last_page_setup, FALSE, TRUE);
          g_object_unref (last_page_setup);

          /* And show the custom paper dialog */
          custom_paper_dialog = _ctk_custom_paper_unix_dialog_new (CTK_WINDOW (dialog), NULL);
          g_signal_connect (custom_paper_dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), dialog);
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          ctk_window_present (CTK_WINDOW (custom_paper_dialog));
          G_GNUC_END_IGNORE_DEPRECATIONS

          return;
        }

      if (priv->last_setup)
        g_object_unref (priv->last_setup);

      priv->last_setup = g_object_ref (page_setup);

      unit = _ctk_print_get_default_user_units ();

      if (unit == CTK_UNIT_MM)
        unit_str = _("mm");
      else
        unit_str = _("inch");

      w = double_to_string (ctk_page_setup_get_paper_width (page_setup, unit),
                            unit);
      h = double_to_string (ctk_page_setup_get_paper_height (page_setup, unit),
                            unit);
      str = g_strdup_printf ("%s × %s %s", w, h, unit_str);
      g_free (w);
      g_free (h);

      ctk_label_set_text (label, str);
      g_free (str);

      top = double_to_string (ctk_page_setup_get_top_margin (page_setup, unit), unit);
      bottom = double_to_string (ctk_page_setup_get_bottom_margin (page_setup, unit), unit);
      left = double_to_string (ctk_page_setup_get_left_margin (page_setup, unit), unit);
      right = double_to_string (ctk_page_setup_get_right_margin (page_setup, unit), unit);

      str = g_strdup_printf (_("Margins:\n"
                               " Left: %s %s\n"
                               " Right: %s %s\n"
                               " Top: %s %s\n"
                               " Bottom: %s %s"
                               ),
                             left, unit_str,
                             right, unit_str,
                             top, unit_str,
                             bottom, unit_str);
      g_free (top);
      g_free (bottom);
      g_free (left);
      g_free (right);

      ctk_widget_set_tooltip_text (priv->paper_size_label, str);
      g_free (str);

      g_object_unref (page_setup);
    }
  else
    {
      ctk_label_set_text (label, "");
      ctk_widget_set_tooltip_text (priv->paper_size_label, NULL);
      if (priv->last_setup)
        g_object_unref (priv->last_setup);
      priv->last_setup = NULL;
    }
}

static void
page_name_func (GtkCellLayout   *cell_layout,
                GtkCellRenderer *cell,
                GtkTreeModel    *tree_model,
                GtkTreeIter     *iter,
                gpointer         data)
{
  GtkPageSetup *page_setup;
  GtkPaperSize *paper_size;

  ctk_tree_model_get (tree_model, iter,
                      PAGE_SETUP_LIST_COL_PAGE_SETUP, &page_setup, -1);
  if (page_setup)
    {
      paper_size = ctk_page_setup_get_paper_size (page_setup);
      g_object_set (cell, "text",  ctk_paper_size_get_display_name (paper_size), NULL);
      g_object_unref (page_setup);
    }
  else
    g_object_set (cell, "text",  _("Manage Custom Sizes…"), NULL);

}

/**
 * ctk_page_setup_unix_dialog_new:
 * @title: (allow-none): the title of the dialog, or %NULL
 * @parent: (allow-none): transient parent of the dialog, or %NULL
 *
 * Creates a new page setup dialog.
 *
 * Returns: the new #GtkPageSetupUnixDialog
 *
 * Since: 2.10
 */
GtkWidget *
ctk_page_setup_unix_dialog_new (const gchar *title,
                                GtkWindow   *parent)
{
  GtkWidget *result;

  if (title == NULL)
    title = _("Page Setup");

  result = g_object_new (CTK_TYPE_PAGE_SETUP_UNIX_DIALOG,
                         "title", title,
                         NULL);

  if (parent)
    ctk_window_set_transient_for (CTK_WINDOW (result), parent);

  return result;
}

static GtkPageOrientation
get_orientation (GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->portrait_radio)))
    return CTK_PAGE_ORIENTATION_PORTRAIT;
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->landscape_radio)))
    return CTK_PAGE_ORIENTATION_LANDSCAPE;
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->reverse_landscape_radio)))
    return CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE;
  return CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT;
}

static void
set_orientation (GtkPageSetupUnixDialog *dialog,
                 GtkPageOrientation      orientation)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  switch (orientation)
    {
    case CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->reverse_portrait_radio), TRUE);
      break;
    case CTK_PAGE_ORIENTATION_PORTRAIT:
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->portrait_radio), TRUE);
      break;
    case CTK_PAGE_ORIENTATION_LANDSCAPE:
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->landscape_radio), TRUE);
      break;
    case CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->reverse_landscape_radio), TRUE);
      break;
    }
}

/**
 * ctk_page_setup_unix_dialog_set_page_setup:
 * @dialog: a #GtkPageSetupUnixDialog
 * @page_setup: a #GtkPageSetup
 *
 * Sets the #GtkPageSetup from which the page setup
 * dialog takes its values.
 *
 * Since: 2.10
 **/
void
ctk_page_setup_unix_dialog_set_page_setup (GtkPageSetupUnixDialog *dialog,
                                           GtkPageSetup           *page_setup)
{
  if (page_setup)
    {
      set_paper_size (dialog, page_setup, FALSE, TRUE);
      set_orientation (dialog, ctk_page_setup_get_orientation (page_setup));
    }
}

/**
 * ctk_page_setup_unix_dialog_get_page_setup:
 * @dialog: a #GtkPageSetupUnixDialog
 *
 * Gets the currently selected page setup from the dialog.
 *
 * Returns: (transfer none): the current page setup
 *
 * Since: 2.10
 **/
GtkPageSetup *
ctk_page_setup_unix_dialog_get_page_setup (GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetup *page_setup;

  page_setup = get_current_page_setup (dialog);

  ctk_page_setup_set_orientation (page_setup, get_orientation (dialog));

  return page_setup;
}

static gboolean
set_active_printer (GtkPageSetupUnixDialog *dialog,
                    const gchar            *printer_name)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkPrinter *printer;

  model = CTK_TREE_MODEL (priv->printer_list);

  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          ctk_tree_model_get (CTK_TREE_MODEL (priv->printer_list), &iter,
                              PRINTER_LIST_COL_PRINTER, &printer, -1);
          if (printer == NULL)
            continue;

          if (strcmp (ctk_printer_get_name (printer), printer_name) == 0)
            {
              ctk_combo_box_set_active_iter (CTK_COMBO_BOX (priv->printer_combo),
                                             &iter);
              g_object_unref (printer);
              return TRUE;
            }

          g_object_unref (printer);

        } while (ctk_tree_model_iter_next (model, &iter));
    }

  return FALSE;
}

/**
 * ctk_page_setup_unix_dialog_set_print_settings:
 * @dialog: a #GtkPageSetupUnixDialog
 * @print_settings: a #GtkPrintSettings
 *
 * Sets the #GtkPrintSettings from which the page setup dialog
 * takes its values.
 *
 * Since: 2.10
 **/
void
ctk_page_setup_unix_dialog_set_print_settings (GtkPageSetupUnixDialog *dialog,
                                               GtkPrintSettings       *print_settings)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  const gchar *format_for_printer;

  if (priv->print_settings == print_settings) return;

  if (priv->print_settings)
    g_object_unref (priv->print_settings);

  priv->print_settings = print_settings;

  if (print_settings)
    {
      g_object_ref (print_settings);

      format_for_printer = ctk_print_settings_get (print_settings, "format-for-printer");

      /* Set printer if in list, otherwise set when
       * that printer is added
       */
      if (format_for_printer &&
          !set_active_printer (dialog, format_for_printer))
        priv->waiting_for_printer = g_strdup (format_for_printer);
    }
}

/**
 * ctk_page_setup_unix_dialog_get_print_settings:
 * @dialog: a #GtkPageSetupUnixDialog
 *
 * Gets the current print settings from the dialog.
 *
 * Returns: (transfer none): the current print settings
 *
 * Since: 2.10
 **/
GtkPrintSettings *
ctk_page_setup_unix_dialog_get_print_settings (GtkPageSetupUnixDialog *dialog)
{
  GtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  return priv->print_settings;
}
