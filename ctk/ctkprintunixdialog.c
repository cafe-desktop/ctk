/* CtkPrintUnixDialog
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 * Copyright © 2006, 2007 Christian Persch
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
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#include "ctkprintunixdialog.h"

#include "ctkcustompaperunixdialog.h"
#include "ctkprintbackend.h"
#include "ctkprinter-private.h"
#include "ctkprinteroptionwidget.h"
#include "ctkprintutils.h"

#include "ctkspinbutton.h"
#include "ctkcellrendererpixbuf.h"
#include "ctkcellrenderertext.h"
#include "ctkimage.h"
#include "ctktreeselection.h"
#include "ctknotebook.h"
#include "ctkscrolledwindow.h"
#include "ctkcombobox.h"
#include "ctktogglebutton.h"
#include "ctkradiobutton.h"
#include "ctkdrawingarea.h"
#include "ctkbox.h"
#include "ctkgrid.h"
#include "ctkframe.h"
#include "ctklabel.h"
#include "ctkeventbox.h"
#include "ctkbuildable.h"
#include "ctkmessagedialog.h"
#include "ctkbutton.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkdialogprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidgetprivate.h"


/**
 * SECTION:ctkprintunixdialog
 * @Short_description: A print dialog
 * @Title: CtkPrintUnixDialog
 * @Include: ctk/ctkunixprint.h
 * @See_also: #CtkPageSetupUnixDialog, #CtkPrinter, #CtkPrintJob
 *
 * CtkPrintUnixDialog implements a print dialog for platforms
 * which don’t provide a native print dialog, like Unix. It can
 * be used very much like any other CTK+ dialog, at the cost of
 * the portability offered by the
 * [high-level printing API][ctk3-High-level-Printing-API]
 *
 * In order to print something with #CtkPrintUnixDialog, you need
 * to use ctk_print_unix_dialog_get_selected_printer() to obtain
 * a #CtkPrinter object and use it to construct a #CtkPrintJob using
 * ctk_print_job_new().
 *
 * #CtkPrintUnixDialog uses the following response values:
 * - %CTK_RESPONSE_OK: for the “Print” button
 * - %CTK_RESPONSE_APPLY: for the “Preview” button
 * - %CTK_RESPONSE_CANCEL: for the “Cancel” button
 *
 * Printing support was added in CTK+ 2.10.
 *
 * # CtkPrintUnixDialog as CtkBuildable
 *
 * The CtkPrintUnixDialog implementation of the CtkBuildable interface exposes its
 * @notebook internal children with the name “notebook”.
 *
 * An example of a #CtkPrintUnixDialog UI definition fragment:
 * |[
 * <object class="CtkPrintUnixDialog" id="dialog1">
 *   <child internal-child="notebook">
 *     <object class="CtkNotebook" id="notebook">
 *       <child>
 *         <object class="CtkLabel" id="tabcontent">
 *         <property name="label">Content on notebook tab</property>
 *         </object>
 *       </child>
 *       <child type="tab">
 *         <object class="CtkLabel" id="tablabel">
 *           <property name="label">Tab label</property>
 *         </object>
 *         <packing>
 *           <property name="tab_expand">False</property>
 *           <property name="tab_fill">False</property>
 *         </packing>
 *       </child>
 *     </object>
 *   </child>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * CtkPrintUnixDialog has a single CSS node with name printdialog.
 */


#define EXAMPLE_PAGE_AREA_SIZE 110
#define RULER_DISTANCE 7.5
#define RULER_RADIUS 2


static void     ctk_print_unix_dialog_constructed  (GObject            *object);
static void     ctk_print_unix_dialog_destroy      (CtkWidget          *widget);
static void     ctk_print_unix_dialog_finalize     (GObject            *object);
static void     ctk_print_unix_dialog_set_property (GObject            *object,
                                                    guint               prop_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void     ctk_print_unix_dialog_get_property (GObject            *object,
                                                    guint               prop_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void     ctk_print_unix_dialog_style_updated (CtkWidget          *widget);
static void     unschedule_idle_mark_conflicts     (CtkPrintUnixDialog *dialog);
static void     selected_printer_changed           (CtkTreeSelection   *selection,
                                                    CtkPrintUnixDialog *dialog);
static void     clear_per_printer_ui               (CtkPrintUnixDialog *dialog);
static void     printer_added_cb                   (CtkPrintBackend    *backend,
                                                    CtkPrinter         *printer,
                                                    CtkPrintUnixDialog *dialog);
static void     printer_removed_cb                 (CtkPrintBackend    *backend,
                                                    CtkPrinter         *printer,
                                                    CtkPrintUnixDialog *dialog);
static void     printer_status_cb                  (CtkPrintBackend    *backend,
                                                    CtkPrinter         *printer,
                                                    CtkPrintUnixDialog *dialog);
static void     update_collate_icon                (CtkToggleButton    *toggle_button,
                                                    CtkPrintUnixDialog *dialog);
static gboolean error_dialogs                      (CtkPrintUnixDialog *print_dialog,
						    gint                print_dialog_response_id,
						    gpointer            data);
static void     emit_ok_response                   (CtkTreeView        *tree_view,
						    CtkTreePath        *path,
						    CtkTreeViewColumn  *column,
						    gpointer           *user_data);
static void     update_page_range_entry_sensitivity(CtkWidget          *button,
						    CtkPrintUnixDialog *dialog);
static void     update_print_at_entry_sensitivity  (CtkWidget          *button,
						    CtkPrintUnixDialog *dialog);
static void     update_print_at_option             (CtkPrintUnixDialog *dialog);
static void     update_dialog_from_capabilities    (CtkPrintUnixDialog *dialog);
static gboolean draw_collate_cb                    (CtkWidget          *widget,
						    cairo_t            *cr,
						    CtkPrintUnixDialog *dialog);
static gboolean is_printer_active                  (CtkTreeModel        *model,
						    CtkTreeIter         *iter,
						    CtkPrintUnixDialog  *dialog);
static gint     default_printer_list_sort_func     (CtkTreeModel        *model,
						    CtkTreeIter         *a,
						    CtkTreeIter         *b,
						    gpointer             user_data);
static gboolean paper_size_row_is_separator        (CtkTreeModel        *model,
						    CtkTreeIter         *iter,
						    gpointer             data);
static void     page_name_func                     (CtkCellLayout       *cell_layout,
						    CtkCellRenderer     *cell,
						    CtkTreeModel        *tree_model,
						    CtkTreeIter         *iter,
						    gpointer             data);
static void     update_number_up_layout            (CtkPrintUnixDialog  *dialog);
static gboolean draw_page_cb                       (CtkWidget           *widget,
						    cairo_t             *cr,
						    CtkPrintUnixDialog  *dialog);


static gboolean dialog_get_collate                 (CtkPrintUnixDialog *dialog);
static gboolean dialog_get_reverse                 (CtkPrintUnixDialog *dialog);
static gint     dialog_get_n_copies                (CtkPrintUnixDialog *dialog);

static void     set_cell_sensitivity_func          (CtkTreeViewColumn *tree_column,
                                                    CtkCellRenderer   *cell,
                                                    CtkTreeModel      *model,
                                                    CtkTreeIter       *iter,
                                                    gpointer           data);
static gboolean set_active_printer                 (CtkPrintUnixDialog *dialog,
                                                    const gchar        *printer_name);
static void redraw_page_layout_preview             (CtkPrintUnixDialog *dialog);
static void load_print_backends                    (CtkPrintUnixDialog *dialog);
static gboolean printer_compare                    (CtkTreeModel       *model,
                                                    gint                column,
                                                    const gchar        *key,
                                                    CtkTreeIter        *iter,
                                                    gpointer            search_data);

/* CtkBuildable */
static void ctk_print_unix_dialog_buildable_init                    (CtkBuildableIface *iface);
static GObject *ctk_print_unix_dialog_buildable_get_internal_child  (CtkBuildable *buildable,
                                                                     CtkBuilder   *builder,
                                                                     const gchar  *childname);

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
  "iso_a3",
};

/* Keep in line with liststore defined in ctkprintunixdialog.ui */
enum {
  PAGE_SETUP_LIST_COL_PAGE_SETUP,
  PAGE_SETUP_LIST_COL_IS_SEPARATOR,
  PAGE_SETUP_LIST_N_COLS
};

/* Keep in line with liststore defined in ctkprintunixdialog.ui */
enum {
  PRINTER_LIST_COL_ICON,
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_STATE,
  PRINTER_LIST_COL_JOBS,
  PRINTER_LIST_COL_LOCATION,
  PRINTER_LIST_COL_PRINTER_OBJ,
  PRINTER_LIST_N_COLS
};

enum {
  PROP_0,
  PROP_PAGE_SETUP,
  PROP_CURRENT_PAGE,
  PROP_PRINT_SETTINGS,
  PROP_SELECTED_PRINTER,
  PROP_MANUAL_CAPABILITIES,
  PROP_SUPPORT_SELECTION,
  PROP_HAS_SELECTION,
  PROP_EMBED_PAGE_SETUP
};

struct CtkPrintUnixDialogPrivate
{
  CtkWidget *notebook;

  CtkWidget *printer_treeview;
  CtkTreeViewColumn *printer_icon_column;
  CtkTreeViewColumn *printer_name_column;
  CtkTreeViewColumn *printer_location_column;
  CtkTreeViewColumn *printer_status_column;
  CtkCellRenderer *printer_icon_renderer;
  CtkCellRenderer *printer_name_renderer;
  CtkCellRenderer *printer_location_renderer;
  CtkCellRenderer *printer_status_renderer;

  CtkPrintCapabilities manual_capabilities;
  CtkPrintCapabilities printer_capabilities;

  CtkTreeModel *printer_list;
  CtkTreeModelFilter *printer_list_filter;

  CtkPageSetup *page_setup;
  gboolean page_setup_set;
  gboolean embed_page_setup;
  CtkListStore *page_setup_list;
  CtkListStore *custom_paper_list;

  gboolean support_selection;
  gboolean has_selection;

  CtkWidget *all_pages_radio;
  CtkWidget *current_page_radio;
  CtkWidget *selection_radio;
  CtkWidget *range_table;
  CtkWidget *page_range_radio;
  CtkWidget *page_range_entry;

  CtkWidget *copies_spin;
  CtkWidget *collate_check;
  CtkWidget *reverse_check;
  CtkWidget *collate_image;
  CtkWidget *page_layout_preview;
  CtkWidget *scale_spin;
  CtkWidget *page_set_combo;
  CtkWidget *print_now_radio;
  CtkWidget *print_at_radio;
  CtkWidget *print_at_entry;
  CtkWidget *print_hold_radio;
  CtkWidget *paper_size_combo;
  CtkWidget *paper_size_combo_label;
  CtkCellRenderer *paper_size_renderer;
  CtkWidget *orientation_combo;
  CtkWidget *orientation_combo_label;
  gboolean internal_page_setup_change;
  gboolean updating_print_at;
  CtkPrinterOptionWidget *pages_per_sheet;
  CtkPrinterOptionWidget *duplex;
  CtkPrinterOptionWidget *paper_type;
  CtkPrinterOptionWidget *paper_source;
  CtkPrinterOptionWidget *output_tray;
  CtkPrinterOptionWidget *job_prio;
  CtkPrinterOptionWidget *billing_info;
  CtkPrinterOptionWidget *cover_before;
  CtkPrinterOptionWidget *cover_after;
  CtkPrinterOptionWidget *number_up_layout;

  CtkWidget *conflicts_widget;

  CtkWidget *job_page;
  CtkWidget *finishing_table;
  CtkWidget *finishing_page;
  CtkWidget *image_quality_table;
  CtkWidget *image_quality_page;
  CtkWidget *color_table;
  CtkWidget *color_page;

  CtkWidget *advanced_vbox;
  CtkWidget *advanced_page;

  CtkWidget *extension_point;

  /* These are set initially on selected printer (either default printer,
   * printer taken from set settings, or user-selected), but when any
   * setting is changed by the user it is cleared.
   */
  CtkPrintSettings *initial_settings;

  CtkPrinterOption *number_up_layout_n_option;
  CtkPrinterOption *number_up_layout_2_option;

  /* This is the initial printer set by set_settings. We look for it in
   * the added printers. We clear this whenever the user manually changes
   * to another printer, when the user changes a setting or when we find
   * this printer.
   */
  char *waiting_for_printer;
  gboolean internal_printer_change;

  GList *print_backends;

  CtkPrinter *current_printer;
  CtkPrinter *request_details_printer;
  gulong request_details_tag;
  CtkPrinterOptionSet *options;
  gulong options_changed_handler;
  gulong mark_conflicts_id;

  gchar *format_for_printer;

  gint current_page;
};

G_DEFINE_TYPE_WITH_CODE (CtkPrintUnixDialog, ctk_print_unix_dialog, CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkPrintUnixDialog)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_print_unix_dialog_buildable_init))

static CtkBuildableIface *parent_buildable_iface;

static gboolean
is_default_printer (CtkPrintUnixDialog *dialog,
                    CtkPrinter         *printer)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->format_for_printer)
    return strcmp (priv->format_for_printer,
                   ctk_printer_get_name (printer)) == 0;
 else
   return ctk_printer_is_default (printer);
}

static void
ctk_print_unix_dialog_class_init (CtkPrintUnixDialogClass *class)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;

  object_class = (GObjectClass *) class;
  widget_class = (CtkWidgetClass *) class;

  object_class->constructed = ctk_print_unix_dialog_constructed;
  object_class->finalize = ctk_print_unix_dialog_finalize;
  object_class->set_property = ctk_print_unix_dialog_set_property;
  object_class->get_property = ctk_print_unix_dialog_get_property;

  widget_class->style_updated = ctk_print_unix_dialog_style_updated;
  widget_class->destroy = ctk_print_unix_dialog_destroy;

  g_object_class_install_property (object_class,
                                   PROP_PAGE_SETUP,
                                   g_param_spec_object ("page-setup",
                                                        P_("Page Setup"),
                                                        P_("The CtkPageSetup to use"),
                                                        CTK_TYPE_PAGE_SETUP,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_CURRENT_PAGE,
                                   g_param_spec_int ("current-page",
                                                     P_("Current Page"),
                                                     P_("The current page in the document"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_PRINT_SETTINGS,
                                   g_param_spec_object ("print-settings",
                                                        P_("Print Settings"),
                                                        P_("The CtkPrintSettings used for initializing the dialog"),
                                                        CTK_TYPE_PRINT_SETTINGS,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SELECTED_PRINTER,
                                   g_param_spec_object ("selected-printer",
                                                        P_("Selected Printer"),
                                                        P_("The CtkPrinter which is selected"),
                                                        CTK_TYPE_PRINTER,
                                                        CTK_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_MANUAL_CAPABILITIES,
                                   g_param_spec_flags ("manual-capabilities",
                                                       P_("Manual Capabilities"),
                                                       P_("Capabilities the application can handle"),
                                                       CTK_TYPE_PRINT_CAPABILITIES,
                                                       0,
                                                       CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SUPPORT_SELECTION,
                                   g_param_spec_boolean ("support-selection",
                                                         P_("Support Selection"),
                                                         P_("Whether the dialog supports selection"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_HAS_SELECTION,
                                   g_param_spec_boolean ("has-selection",
                                                         P_("Has Selection"),
                                                         P_("Whether the application has a selection"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

   g_object_class_install_property (object_class,
                                   PROP_EMBED_PAGE_SETUP,
                                   g_param_spec_boolean ("embed-page-setup",
                                                         P_("Embed Page Setup"),
                                                         P_("TRUE if page setup combos are embedded in CtkPrintUnixDialog"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkprintunixdialog.ui");

  /* CtkTreeView / CtkTreeModel */
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_treeview);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_list);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_list_filter);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, page_setup_list);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, custom_paper_list);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_icon_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_name_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_location_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_status_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_icon_renderer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_name_renderer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_location_renderer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, printer_status_renderer);

  /* General Widgetry */
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, notebook);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, all_pages_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, all_pages_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, current_page_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, selection_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, range_table);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, page_range_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, page_range_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, copies_spin);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, collate_check);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, reverse_check);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, collate_image);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, page_layout_preview);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, scale_spin);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, page_set_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, print_now_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, print_at_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, print_at_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, print_hold_radio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, paper_size_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, paper_size_combo_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, paper_size_renderer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, orientation_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, orientation_combo_label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, conflicts_widget);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, job_page);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, finishing_table);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, finishing_page);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, image_quality_table);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, image_quality_page);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, color_table);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, color_page);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, advanced_vbox);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, advanced_page);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, extension_point);

  /* CtkPrinterOptionWidgets... */
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, pages_per_sheet);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, duplex);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, paper_type);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, paper_source);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, output_tray);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, job_prio);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, billing_info);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, cover_before);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, cover_after);
  ctk_widget_class_bind_template_child_private (widget_class, CtkPrintUnixDialog, number_up_layout);

  /* Callbacks handled in the UI */
  ctk_widget_class_bind_template_callback (widget_class, redraw_page_layout_preview);
  ctk_widget_class_bind_template_callback (widget_class, error_dialogs);
  ctk_widget_class_bind_template_callback (widget_class, emit_ok_response);
  ctk_widget_class_bind_template_callback (widget_class, selected_printer_changed);
  ctk_widget_class_bind_template_callback (widget_class, update_page_range_entry_sensitivity);
  ctk_widget_class_bind_template_callback (widget_class, update_print_at_entry_sensitivity);
  ctk_widget_class_bind_template_callback (widget_class, update_print_at_option);
  ctk_widget_class_bind_template_callback (widget_class, update_dialog_from_capabilities);
  ctk_widget_class_bind_template_callback (widget_class, update_collate_icon);
  ctk_widget_class_bind_template_callback (widget_class, draw_collate_cb);
  ctk_widget_class_bind_template_callback (widget_class, redraw_page_layout_preview);
  ctk_widget_class_bind_template_callback (widget_class, update_number_up_layout);
  ctk_widget_class_bind_template_callback (widget_class, redraw_page_layout_preview);
  ctk_widget_class_bind_template_callback (widget_class, draw_page_cb);

  ctk_widget_class_set_css_name (widget_class, "printdialog");
}

/* Returns a toplevel CtkWindow, or NULL if none */
static CtkWindow *
get_toplevel (CtkWidget *widget)
{
  CtkWidget *toplevel = NULL;

  toplevel = ctk_widget_get_toplevel (widget);
  if (!ctk_widget_is_toplevel (toplevel))
    return NULL;
  else
    return CTK_WINDOW (toplevel);
}

static void
set_busy_cursor (CtkPrintUnixDialog *dialog,
                 gboolean            busy)
{
  CtkWidget *widget;
  CtkWindow *toplevel;
  GdkDisplay *display;
  GdkCursor *cursor;

  toplevel = get_toplevel (CTK_WIDGET (dialog));
  widget = CTK_WIDGET (toplevel);

  if (!toplevel || !ctk_widget_get_realized (widget))
    return;

  display = ctk_widget_get_display (widget);

  if (busy)
    cursor = cdk_cursor_new_from_name (display, "progress");
  else
    cursor = NULL;

  cdk_window_set_cursor (ctk_widget_get_window (widget), cursor);
  cdk_display_flush (display);

  if (cursor)
    g_object_unref (cursor);
}

/* This function handles error messages before printing.
 */
static gboolean
error_dialogs (CtkPrintUnixDialog *print_dialog,
               gint                print_dialog_response_id,
               gpointer            data)
{
  CtkPrintUnixDialogPrivate *priv = print_dialog->priv;
  CtkPrinterOption          *option = NULL;
  CtkPrinter                *printer = NULL;
  CtkWindow                 *toplevel = NULL;
  CtkWidget                 *dialog = NULL;
  GFile                     *file = NULL;
  gchar                     *basename = NULL;
  gchar                     *dirname = NULL;
  int                        response;

  if (print_dialog != NULL && print_dialog_response_id == CTK_RESPONSE_OK)
    {
      printer = ctk_print_unix_dialog_get_selected_printer (print_dialog);

      if (printer != NULL)
        {
          if (priv->request_details_tag || !ctk_printer_is_accepting_jobs (printer))
            {
              g_signal_stop_emission_by_name (print_dialog, "response");
              return TRUE;
            }

          /* Shows overwrite confirmation dialog in the case of printing
           * to file which already exists.
           */
          if (ctk_printer_is_virtual (printer))
            {
              option = ctk_printer_option_set_lookup (priv->options,
                                                      "ctk-main-page-custom-input");

              if (option != NULL &&
                  option->type == CTK_PRINTER_OPTION_TYPE_FILESAVE)
                {
                  file = g_file_new_for_uri (option->value);

                  if (g_file_query_exists (file, NULL))
                    {
                      GFile *parent;

                      toplevel = get_toplevel (CTK_WIDGET (print_dialog));

                      basename = g_file_get_basename (file);
                      parent = g_file_get_parent (file);
                      dirname = g_file_get_parse_name (parent);
                      g_object_unref (parent);

                      dialog = ctk_message_dialog_new (toplevel,
                                                       CTK_DIALOG_MODAL |
                                                       CTK_DIALOG_DESTROY_WITH_PARENT,
                                                       CTK_MESSAGE_QUESTION,
                                                       CTK_BUTTONS_NONE,
                                                       _("A file named “%s” already exists.  Do you want to replace it?"),
                                                       basename);

                      ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                                                _("The file already exists in “%s”.  Replacing it will "
                                                                "overwrite its contents."),
                                                                dirname);

                      ctk_dialog_add_button (CTK_DIALOG (dialog),
                                             _("_Cancel"),
                                             CTK_RESPONSE_CANCEL);
                      ctk_dialog_add_button (CTK_DIALOG (dialog),
                                             _("_Replace"),
                                             CTK_RESPONSE_ACCEPT);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                      ctk_dialog_set_alternative_button_order (CTK_DIALOG (dialog),
                                                               CTK_RESPONSE_ACCEPT,
                                                               CTK_RESPONSE_CANCEL,
                                                               -1);
G_GNUC_END_IGNORE_DEPRECATIONS
                      ctk_dialog_set_default_response (CTK_DIALOG (dialog),
                                                       CTK_RESPONSE_ACCEPT);

                      if (ctk_window_has_group (toplevel))
                        ctk_window_group_add_window (ctk_window_get_group (toplevel),
                                                     CTK_WINDOW (dialog));

                      response = ctk_dialog_run (CTK_DIALOG (dialog));

                      ctk_widget_destroy (dialog);

                      g_free (dirname);
                      g_free (basename);

                      if (response != CTK_RESPONSE_ACCEPT)
                        {
                          g_signal_stop_emission_by_name (print_dialog, "response");
                          g_object_unref (file);
                          return TRUE;
                        }
                    }

                  g_object_unref (file);
                }
            }
        }
    }
  return FALSE;
}

static void
ctk_print_unix_dialog_init (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv;
  CtkTreeSortable *sort;
  CtkWidget *widget;

  dialog->priv = ctk_print_unix_dialog_get_instance_private (dialog);
  priv = dialog->priv;

  priv->print_backends = NULL;
  priv->current_page = -1;
  priv->number_up_layout_n_option = NULL;
  priv->number_up_layout_2_option = NULL;

  priv->page_setup = ctk_page_setup_new ();
  priv->page_setup_set = FALSE;
  priv->embed_page_setup = FALSE;
  priv->internal_page_setup_change = FALSE;

  priv->support_selection = FALSE;
  priv->has_selection = FALSE;

  g_type_ensure (CTK_TYPE_PRINTER);
  g_type_ensure (CTK_TYPE_PRINTER_OPTION);
  g_type_ensure (CTK_TYPE_PRINTER_OPTION_SET);
  g_type_ensure (CTK_TYPE_PRINTER_OPTION_WIDGET);

  ctk_widget_init_template (CTK_WIDGET (dialog));
  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (dialog));
  ctk_dialog_add_buttons (CTK_DIALOG (dialog),
                          _("Pre_view"), CTK_RESPONSE_APPLY,
                          _("_Cancel"), CTK_RESPONSE_CANCEL,
                          _("_Print"), CTK_RESPONSE_OK,
                          NULL);
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
  widget = ctk_dialog_get_widget_for_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
  ctk_widget_set_sensitive (widget, FALSE);

  /* Treeview auxiliary functions need to be setup here */
  ctk_tree_model_filter_set_visible_func (priv->printer_list_filter,
                                          (CtkTreeModelFilterVisibleFunc) is_printer_active,
                                          dialog,
                                          NULL);

  sort = CTK_TREE_SORTABLE (priv->printer_list);
  ctk_tree_sortable_set_default_sort_func (sort,
                                           default_printer_list_sort_func,
                                           NULL,
                                           NULL);

  ctk_tree_sortable_set_sort_column_id (sort,
                                        CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                        CTK_SORT_ASCENDING);

  ctk_tree_view_set_search_equal_func (CTK_TREE_VIEW (priv->printer_treeview),
                                       printer_compare, NULL, NULL);

  ctk_tree_view_column_set_cell_data_func (priv->printer_icon_column,
					   priv->printer_icon_renderer,
					   set_cell_sensitivity_func, NULL, NULL);

  ctk_tree_view_column_set_cell_data_func (priv->printer_name_column,
					   priv->printer_name_renderer,
					   set_cell_sensitivity_func, NULL, NULL);

  ctk_tree_view_column_set_cell_data_func (priv->printer_location_column,
					   priv->printer_location_renderer,
					   set_cell_sensitivity_func, NULL, NULL);

  ctk_tree_view_column_set_cell_data_func (priv->printer_status_column,
					   priv->printer_status_renderer,
					   set_cell_sensitivity_func, NULL, NULL);


  /* Paper size combo auxilary funcs */
  ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (priv->paper_size_combo),
                                        paper_size_row_is_separator, NULL, NULL);
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (priv->paper_size_combo),
				      priv->paper_size_renderer,
                                      page_name_func, NULL, NULL);

  /* Preview drawing area has no window */
  ctk_widget_set_has_window (priv->page_layout_preview, FALSE);

  /* Load backends */
  load_print_backends (dialog);

  /* Load custom papers */
  _ctk_print_load_custom_papers (priv->custom_paper_list);

  ctk_css_node_set_name (ctk_widget_get_css_node (priv->collate_image), I_("paper"));
  ctk_css_node_set_name (ctk_widget_get_css_node (priv->page_layout_preview), I_("paper"));
}

static void
ctk_print_unix_dialog_constructed (GObject *object)
{
  gboolean use_header;

  G_OBJECT_CLASS (ctk_print_unix_dialog_parent_class)->constructed (object);

  g_object_get (object, "use-header-bar", &use_header, NULL);
  if (use_header)
    {
       /* Reorder the preview button */
       CtkWidget *button, *parent;
       button = ctk_dialog_get_widget_for_response (CTK_DIALOG (object), CTK_RESPONSE_APPLY);
       g_object_ref (button);
       parent = ctk_widget_get_parent (button);
       ctk_container_remove (CTK_CONTAINER (parent), button); 
       ctk_header_bar_pack_end (CTK_HEADER_BAR (parent), button);
       g_object_unref (button);
    }

  update_dialog_from_capabilities (CTK_PRINT_UNIX_DIALOG (object));
}

static void
ctk_print_unix_dialog_destroy (CtkWidget *widget)
{
  CtkPrintUnixDialog *dialog = CTK_PRINT_UNIX_DIALOG (widget);

  /* Make sure we don't destroy custom widgets owned by the backends */
  clear_per_printer_ui (dialog);

  CTK_WIDGET_CLASS (ctk_print_unix_dialog_parent_class)->destroy (widget);
}

static void
disconnect_printer_details_request (CtkPrintUnixDialog *dialog,
                                    gboolean            details_failed)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
                                   priv->request_details_tag);
      priv->request_details_tag = 0;
      set_busy_cursor (dialog, FALSE);
      if (details_failed)
        ctk_list_store_set (CTK_LIST_STORE (priv->printer_list),
                            g_object_get_data (G_OBJECT (priv->request_details_printer),
                                               "ctk-print-tree-iter"),
                            PRINTER_LIST_COL_STATE,
                             _("Getting printer information failed"),
                            -1);
      else
        ctk_list_store_set (CTK_LIST_STORE (priv->printer_list),
                            g_object_get_data (G_OBJECT (priv->request_details_printer),
                                               "ctk-print-tree-iter"),
                            PRINTER_LIST_COL_STATE,
                            ctk_printer_get_state_message (priv->request_details_printer),
                            -1);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
    }
}

static void
ctk_print_unix_dialog_finalize (GObject *object)
{
  CtkPrintUnixDialog *dialog = CTK_PRINT_UNIX_DIALOG (object);
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrintBackend *backend;
  GList *node;

  unschedule_idle_mark_conflicts (dialog);
  disconnect_printer_details_request (dialog, FALSE);

  g_clear_object (&priv->current_printer);
  g_clear_object (&priv->options);

  if (priv->number_up_layout_2_option)
    {
      priv->number_up_layout_2_option->choices[0] = NULL;
      priv->number_up_layout_2_option->choices[1] = NULL;
      g_free (priv->number_up_layout_2_option->choices_display[0]);
      g_free (priv->number_up_layout_2_option->choices_display[1]);
      priv->number_up_layout_2_option->choices_display[0] = NULL;
      priv->number_up_layout_2_option->choices_display[1] = NULL;
      g_object_unref (priv->number_up_layout_2_option);
      priv->number_up_layout_2_option = NULL;
    }

  g_clear_object (&priv->number_up_layout_n_option);
  g_clear_object (&priv->page_setup);
  g_clear_object (&priv->initial_settings);
  g_clear_pointer (&priv->waiting_for_printer, (GDestroyNotify)g_free);
  g_clear_pointer (&priv->format_for_printer, (GDestroyNotify)g_free);

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

  g_clear_object (&priv->page_setup_list);

  G_OBJECT_CLASS (ctk_print_unix_dialog_parent_class)->finalize (object);
}

static void
printer_removed_cb (CtkPrintBackend    *backend,
                    CtkPrinter         *printer,
                    CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter *iter;

  iter = g_object_get_data (G_OBJECT (printer), "ctk-print-tree-iter");
  ctk_list_store_remove (CTK_LIST_STORE (priv->printer_list), iter);
}

static void
ctk_print_unix_dialog_buildable_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->get_internal_child = ctk_print_unix_dialog_buildable_get_internal_child;
}

static GObject *
ctk_print_unix_dialog_buildable_get_internal_child (CtkBuildable *buildable,
                                                    CtkBuilder   *builder,
                                                    const gchar  *childname)
{
  if (strcmp (childname, "notebook") == 0)
    return G_OBJECT (CTK_PRINT_UNIX_DIALOG (buildable)->priv->notebook);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

/* This function controls "sensitive" property of CtkCellRenderer
 * based on pause state of printers.
 */
void set_cell_sensitivity_func (CtkTreeViewColumn *tree_column,
                                CtkCellRenderer   *cell,
                                CtkTreeModel      *tree_model,
                                CtkTreeIter       *iter,
                                gpointer           data)
{
  CtkPrinter *printer;

  ctk_tree_model_get (tree_model, iter,
                      PRINTER_LIST_COL_PRINTER_OBJ, &printer,
                      -1);

  if (printer != NULL && !ctk_printer_is_accepting_jobs (printer))
    g_object_set (cell, "sensitive", FALSE, NULL);
  else
    g_object_set (cell, "sensitive", TRUE, NULL);

  g_clear_object (&printer);
}

static void
printer_status_cb (CtkPrintBackend    *backend,
                   CtkPrinter         *printer,
                   CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter *iter;
  CtkTreeSelection *selection;
  GIcon *icon;

  iter = g_object_get_data (G_OBJECT (printer), "ctk-print-tree-iter");

  icon = g_themed_icon_new ("printer");
  g_themed_icon_prepend_name (G_THEMED_ICON (icon), ctk_printer_get_icon_name (printer));
  ctk_list_store_set (CTK_LIST_STORE (priv->printer_list), iter,
                      PRINTER_LIST_COL_ICON, icon,
                      PRINTER_LIST_COL_STATE, ctk_printer_get_state_message (printer),
                      PRINTER_LIST_COL_JOBS, ctk_printer_get_job_count (printer),
                      PRINTER_LIST_COL_LOCATION, ctk_printer_get_location (printer),
                      -1);
  g_object_unref (icon);

  /* When the pause state change then we need to update sensitive property
   * of CTK_RESPONSE_OK button inside of selected_printer_changed function.
   */
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->printer_treeview));
  priv->internal_printer_change = TRUE;
  selected_printer_changed (selection, dialog);
  priv->internal_printer_change = FALSE;

  if (ctk_print_backend_printer_list_is_done (backend) &&
      ctk_printer_is_default (printer) &&
      (ctk_tree_selection_count_selected_rows (selection) == 0))
    set_active_printer (dialog, ctk_printer_get_name (printer));
}

static void
printer_added_cb (CtkPrintBackend    *backend,
                  CtkPrinter         *printer,
                  CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter iter, filter_iter;
  CtkTreeSelection *selection;
  CtkTreePath *path;
  GIcon *icon;

  ctk_list_store_append (CTK_LIST_STORE (priv->printer_list), &iter);

  g_object_set_data_full (G_OBJECT (printer),
                         "ctk-print-tree-iter",
                          ctk_tree_iter_copy (&iter),
                          (GDestroyNotify) ctk_tree_iter_free);

  icon = g_themed_icon_new ("printer");
  g_themed_icon_prepend_name (G_THEMED_ICON (icon), ctk_printer_get_icon_name (printer));
  ctk_list_store_set (CTK_LIST_STORE (priv->printer_list), &iter,
                      PRINTER_LIST_COL_ICON, icon,
                      PRINTER_LIST_COL_NAME, ctk_printer_get_name (printer),
                      PRINTER_LIST_COL_STATE, ctk_printer_get_state_message (printer),
                      PRINTER_LIST_COL_JOBS, ctk_printer_get_job_count (printer),
                      PRINTER_LIST_COL_LOCATION, ctk_printer_get_location (printer),
                      PRINTER_LIST_COL_PRINTER_OBJ, printer,
                      -1);
  g_object_unref (icon);

  ctk_tree_model_filter_convert_child_iter_to_iter (priv->printer_list_filter,
                                                    &filter_iter, &iter);
  path = ctk_tree_model_get_path (CTK_TREE_MODEL (priv->printer_list_filter), &filter_iter);

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->printer_treeview));

  if (priv->waiting_for_printer != NULL &&
      strcmp (ctk_printer_get_name (printer), priv->waiting_for_printer) == 0)
    {
      priv->internal_printer_change = TRUE;
      ctk_tree_selection_select_iter (selection, &filter_iter);
      ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (priv->printer_treeview),
                                    path, NULL, TRUE, 0.5, 0.0);
      priv->internal_printer_change = FALSE;
      g_free (priv->waiting_for_printer);
      priv->waiting_for_printer = NULL;
    }
  else if (is_default_printer (dialog, printer) &&
           ctk_tree_selection_count_selected_rows (selection) == 0)
    {
      priv->internal_printer_change = TRUE;
      ctk_tree_selection_select_iter (selection, &filter_iter);
      ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (priv->printer_treeview),
                                    path, NULL, TRUE, 0.5, 0.0);
      priv->internal_printer_change = FALSE;
    }

  ctk_tree_path_free (path);
}

static void
printer_list_initialize (CtkPrintUnixDialog *dialog,
                         CtkPrintBackend    *print_backend)
{
  GList *list;
  GList *node;

  g_return_if_fail (print_backend != NULL);

  g_signal_connect_object (print_backend, "printer-added",
                           (GCallback) printer_added_cb, G_OBJECT (dialog), 0);

  g_signal_connect_object (print_backend, "printer-removed",
                           (GCallback) printer_removed_cb, G_OBJECT (dialog), 0);

  g_signal_connect_object (print_backend, "printer-status-changed",
                           (GCallback) printer_status_cb, G_OBJECT (dialog), 0);

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
load_print_backends (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  GList *node;

  if (g_module_supported ())
    priv->print_backends = ctk_print_backend_load_modules ();

  for (node = priv->print_backends; node != NULL; node = node->next)
    {
      CtkPrintBackend *backend = node->data;
      printer_list_initialize (dialog, backend);
    }
}

static void
ctk_print_unix_dialog_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)

{
  CtkPrintUnixDialog *dialog = CTK_PRINT_UNIX_DIALOG (object);

  switch (prop_id)
    {
    case PROP_PAGE_SETUP:
      ctk_print_unix_dialog_set_page_setup (dialog, g_value_get_object (value));
      break;
    case PROP_CURRENT_PAGE:
      ctk_print_unix_dialog_set_current_page (dialog, g_value_get_int (value));
      break;
    case PROP_PRINT_SETTINGS:
      ctk_print_unix_dialog_set_settings (dialog, g_value_get_object (value));
      break;
    case PROP_MANUAL_CAPABILITIES:
      ctk_print_unix_dialog_set_manual_capabilities (dialog, g_value_get_flags (value));
      break;
    case PROP_SUPPORT_SELECTION:
      ctk_print_unix_dialog_set_support_selection (dialog, g_value_get_boolean (value));
      break;
    case PROP_HAS_SELECTION:
      ctk_print_unix_dialog_set_has_selection (dialog, g_value_get_boolean (value));
      break;
    case PROP_EMBED_PAGE_SETUP:
      ctk_print_unix_dialog_set_embed_page_setup (dialog, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_print_unix_dialog_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  CtkPrintUnixDialog *dialog = CTK_PRINT_UNIX_DIALOG (object);
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  switch (prop_id)
    {
    case PROP_PAGE_SETUP:
      g_value_set_object (value, priv->page_setup);
      break;
    case PROP_CURRENT_PAGE:
      g_value_set_int (value, priv->current_page);
      break;
    case PROP_PRINT_SETTINGS:
      g_value_take_object (value, ctk_print_unix_dialog_get_settings (dialog));
      break;
    case PROP_SELECTED_PRINTER:
      g_value_set_object (value, priv->current_printer);
      break;
    case PROP_MANUAL_CAPABILITIES:
      g_value_set_flags (value, priv->manual_capabilities);
      break;
    case PROP_SUPPORT_SELECTION:
      g_value_set_boolean (value, priv->support_selection);
      break;
    case PROP_HAS_SELECTION:
      g_value_set_boolean (value, priv->has_selection);
      break;
    case PROP_EMBED_PAGE_SETUP:
      g_value_set_boolean (value, priv->embed_page_setup);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
is_printer_active (CtkTreeModel       *model,
                   CtkTreeIter        *iter,
                   CtkPrintUnixDialog *dialog)
{
  gboolean result;
  CtkPrinter *printer;
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  ctk_tree_model_get (model, iter,
                      PRINTER_LIST_COL_PRINTER_OBJ, &printer,
                      -1);

  if (printer == NULL)
    return FALSE;

  result = ctk_printer_is_active (printer);

  if (result &&
      priv->manual_capabilities & (CTK_PRINT_CAPABILITY_GENERATE_PDF |
                                   CTK_PRINT_CAPABILITY_GENERATE_PS))
    {
       /* Check that the printer can handle at least one of the data
        * formats that the application supports.
        */
       result = ((priv->manual_capabilities & CTK_PRINT_CAPABILITY_GENERATE_PDF) &&
                 ctk_printer_accepts_pdf (printer)) ||
                ((priv->manual_capabilities & CTK_PRINT_CAPABILITY_GENERATE_PS) &&
                 ctk_printer_accepts_ps (printer));
    }

  g_object_unref (printer);

  return result;
}

static gint
default_printer_list_sort_func (CtkTreeModel *model,
                                CtkTreeIter  *a,
                                CtkTreeIter  *b,
                                gpointer      user_data)
{
  gchar *a_name;
  gchar *b_name;
  CtkPrinter *a_printer;
  CtkPrinter *b_printer;
  gint result;

  ctk_tree_model_get (model, a,
                      PRINTER_LIST_COL_NAME, &a_name,
                      PRINTER_LIST_COL_PRINTER_OBJ, &a_printer,
                      -1);
  ctk_tree_model_get (model, b,
                      PRINTER_LIST_COL_NAME, &b_name,
                      PRINTER_LIST_COL_PRINTER_OBJ, &b_printer,
                      -1);

  if (a_printer == NULL && b_printer == NULL)
    result = 0;
  else if (a_printer == NULL)
   result = G_MAXINT;
  else if (b_printer == NULL)
   result = G_MININT;
  else if (ctk_printer_is_virtual (a_printer) && ctk_printer_is_virtual (b_printer))
    result = 0;
  else if (ctk_printer_is_virtual (a_printer) && !ctk_printer_is_virtual (b_printer))
    result = G_MININT;
  else if (!ctk_printer_is_virtual (a_printer) && ctk_printer_is_virtual (b_printer))
    result = G_MAXINT;
  else if (a_name == NULL && b_name == NULL)
    result = 0;
  else if (a_name == NULL && b_name != NULL)
    result = 1;
  else if (a_name != NULL && b_name == NULL)
    result = -1;
  else
    result = g_ascii_strcasecmp (a_name, b_name);

  g_free (a_name);
  g_free (b_name);
  if (a_printer)
    g_object_unref (a_printer);
  if (b_printer)
    g_object_unref (b_printer);

  return result;
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

static gboolean
setup_option (CtkPrintUnixDialog     *dialog,
              const gchar            *option_name,
              CtkPrinterOptionWidget *widget)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrinterOption *option;

  option = ctk_printer_option_set_lookup (priv->options, option_name);
  ctk_printer_option_widget_set_source (widget, option);

  return option != NULL;
}

static void
add_option_to_extension_point (CtkPrinterOption *option,
                               gpointer          data)
{
  CtkWidget *extension_point = data;
  CtkWidget *widget;

  widget = ctk_printer_option_widget_new (option);
  ctk_widget_show (widget);

  if (ctk_printer_option_widget_has_external_label (CTK_PRINTER_OPTION_WIDGET (widget)))
    {
      CtkWidget *label, *hbox;

      ctk_widget_set_valign (widget, CTK_ALIGN_BASELINE);

      label = ctk_printer_option_widget_get_external_label (CTK_PRINTER_OPTION_WIDGET (widget));
      ctk_widget_show (label);
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);
      ctk_label_set_mnemonic_widget (CTK_LABEL (label), widget);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
      ctk_widget_set_valign (hbox, CTK_ALIGN_BASELINE);
      ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
      ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 0);
      ctk_widget_show (hbox);

      ctk_box_pack_start (CTK_BOX (extension_point), hbox, TRUE, TRUE, 0);
    }
  else
    ctk_box_pack_start (CTK_BOX (extension_point), widget, TRUE, TRUE, 0);
}

static gint
grid_rows (CtkGrid *table)
{
  gint t0, t1, t, h;
  GList *children, *c;

  children = ctk_container_get_children (CTK_CONTAINER (table));
  t0 = t1 = 0;
  for (c = children; c; c = c->next)
    {
      ctk_container_child_get (CTK_CONTAINER (table), c->data,
                               "top-attach", &t,
                               "height", &h,
                               NULL);
      if (c == children)
        {
          t0 = t;
          t1 = t + h;
        }
      else
        {
          if (t < t0)
            t0 = t;
          if (t + h > t1)
            t1 = t + h;
        }
    }
  g_list_free (children);

  return t1 - t0;
}

static void
add_option_to_table (CtkPrinterOption *option,
                     gpointer          user_data)
{
  CtkGrid *table;
  CtkWidget *label, *widget;
  guint row;

  table = CTK_GRID (user_data);

  if (g_str_has_prefix (option->name, "ctk-"))
    return;

  row = grid_rows (table);

  widget = ctk_printer_option_widget_new (option);
  ctk_widget_show (widget);

  if (ctk_printer_option_widget_has_external_label (CTK_PRINTER_OPTION_WIDGET (widget)))
    {
      label = ctk_printer_option_widget_get_external_label (CTK_PRINTER_OPTION_WIDGET (widget));
      ctk_widget_show (label);

      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_label_set_mnemonic_widget (CTK_LABEL (label), widget);

      ctk_grid_attach (table, label, 0, row - 1, 1, 1);
      ctk_grid_attach (table, widget, 1, row - 1, 1, 1);
    }
  else
    ctk_grid_attach (table, widget, 0, row - 1, 2, 1);
}

static void
setup_page_table (CtkPrinterOptionSet *options,
                  const gchar         *group,
                  CtkWidget           *table,
                  CtkWidget           *page)
{
  gint nrows;

  ctk_printer_option_set_foreach_in_group (options, group,
                                           add_option_to_table,
                                           table);

  nrows = grid_rows (CTK_GRID (table));
  if (nrows == 0)
    ctk_widget_hide (page);
  else
    ctk_widget_show (page);
}

static void
update_print_at_option (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrinterOption *option;

  option = ctk_printer_option_set_lookup (priv->options, "ctk-print-time");

  if (option == NULL)
    return;

  if (priv->updating_print_at)
    return;

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->print_at_radio)))
    ctk_printer_option_set (option, "at");
  else if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->print_hold_radio)))
    ctk_printer_option_set (option, "on-hold");
  else
    ctk_printer_option_set (option, "now");

  option = ctk_printer_option_set_lookup (priv->options, "ctk-print-time-text");
  if (option != NULL)
    {
      const gchar *text;

      text = ctk_entry_get_text (CTK_ENTRY (priv->print_at_entry));
      ctk_printer_option_set (option, text);
    }
}


static gboolean
setup_print_at (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrinterOption *option;

  option = ctk_printer_option_set_lookup (priv->options, "ctk-print-time");

  if (option == NULL)
    {
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->print_now_radio),
                                    TRUE);
      ctk_widget_set_sensitive (priv->print_at_radio, FALSE);
      ctk_widget_set_sensitive (priv->print_at_entry, FALSE);
      ctk_widget_set_sensitive (priv->print_hold_radio, FALSE);
      ctk_entry_set_text (CTK_ENTRY (priv->print_at_entry), "");
      return FALSE;
    }

  priv->updating_print_at = TRUE;

  ctk_widget_set_sensitive (priv->print_at_entry, FALSE);
  ctk_widget_set_sensitive (priv->print_at_radio,
                            ctk_printer_option_has_choice (option, "at"));

  ctk_widget_set_sensitive (priv->print_hold_radio,
                            ctk_printer_option_has_choice (option, "on-hold"));

  update_print_at_option (dialog);

  if (strcmp (option->value, "at") == 0)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->print_at_radio),
                                  TRUE);
  else if (strcmp (option->value, "on-hold") == 0)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->print_hold_radio),
                                  TRUE);
  else
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->print_now_radio),
                                  TRUE);

  option = ctk_printer_option_set_lookup (priv->options, "ctk-print-time-text");
  if (option != NULL)
    ctk_entry_set_text (CTK_ENTRY (priv->print_at_entry), option->value);

  priv->updating_print_at = FALSE;

  return TRUE;
}

static void
update_dialog_from_settings (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  GList *groups, *l;
  gchar *group;
  CtkWidget *table, *frame;
  gboolean has_advanced, has_job;
  guint nrows;
  GList *children;

  if (priv->current_printer == NULL)
    {
       clear_per_printer_ui (dialog);
       ctk_widget_hide (priv->job_page);
       ctk_widget_hide (priv->advanced_page);
       ctk_widget_hide (priv->image_quality_page);
       ctk_widget_hide (priv->finishing_page);
       ctk_widget_hide (priv->color_page);
       ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), CTK_RESPONSE_OK, FALSE);

       return;
    }

  setup_option (dialog, "ctk-n-up", priv->pages_per_sheet);
  setup_option (dialog, "ctk-n-up-layout", priv->number_up_layout);
  setup_option (dialog, "ctk-duplex", priv->duplex);
  setup_option (dialog, "ctk-paper-type", priv->paper_type);
  setup_option (dialog, "ctk-paper-source", priv->paper_source);
  setup_option (dialog, "ctk-output-tray", priv->output_tray);

  has_job = FALSE;
  has_job |= setup_option (dialog, "ctk-job-prio", priv->job_prio);
  has_job |= setup_option (dialog, "ctk-billing-info", priv->billing_info);
  has_job |= setup_option (dialog, "ctk-cover-before", priv->cover_before);
  has_job |= setup_option (dialog, "ctk-cover-after", priv->cover_after);
  has_job |= setup_print_at (dialog);

  if (has_job)
    ctk_widget_show (priv->job_page);
  else
    ctk_widget_hide (priv->job_page);

  setup_page_table (priv->options,
                    "ImageQualityPage",
                    priv->image_quality_table,
                    priv->image_quality_page);

  setup_page_table (priv->options,
                    "FinishingPage",
                    priv->finishing_table,
                    priv->finishing_page);

  setup_page_table (priv->options,
                    "ColorPage",
                    priv->color_table,
                    priv->color_page);

  ctk_printer_option_set_foreach_in_group (priv->options,
                                           "CtkPrintDialogExtension",
                                           add_option_to_extension_point,
                                           priv->extension_point);

  /* A bit of a hack, keep the last option flush right.
   * This keeps the file format radios from moving as the
   * filename changes.
   */
  children = ctk_container_get_children (CTK_CONTAINER (priv->extension_point));
  l = g_list_last (children);
  if (l && l != children)
    ctk_widget_set_halign (CTK_WIDGET (l->data), CTK_ALIGN_END);
  g_list_free (children);

  /* Put the rest of the groups in the advanced page */
  groups = ctk_printer_option_set_get_groups (priv->options);

  has_advanced = FALSE;
  for (l = groups; l != NULL; l = l->next)
    {
      group = l->data;

      if (group == NULL)
        continue;

      if (strcmp (group, "ImageQualityPage") == 0 ||
          strcmp (group, "ColorPage") == 0 ||
          strcmp (group, "FinishingPage") == 0 ||
          strcmp (group, "CtkPrintDialogExtension") == 0)
        continue;

      table = ctk_grid_new ();
      ctk_grid_set_row_spacing (CTK_GRID (table), 6);
      ctk_grid_set_column_spacing (CTK_GRID (table), 12);

      ctk_printer_option_set_foreach_in_group (priv->options,
                                               group,
                                               add_option_to_table,
                                               table);

      nrows = grid_rows (CTK_GRID (table));
      if (nrows == 0)
        ctk_widget_destroy (table);
      else
        {
          has_advanced = TRUE;
          frame = wrap_in_frame (group, table);
          ctk_widget_show (table);
          ctk_widget_show (frame);

          ctk_box_pack_start (CTK_BOX (priv->advanced_vbox),
                              frame, FALSE, FALSE, 0);
        }
    }

  if (has_advanced)
    ctk_widget_show (priv->advanced_page);
  else
    ctk_widget_hide (priv->advanced_page);

  g_list_free_full (groups, g_free);
}

static void
update_dialog_from_capabilities (CtkPrintUnixDialog *dialog)
{
  CtkPrintCapabilities caps;
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  gboolean can_collate;
  const gchar *copies;
  CtkWidget *button;

  copies = ctk_entry_get_text (CTK_ENTRY (priv->copies_spin));
  can_collate = (*copies != '\0' && atoi (copies) > 1);

  caps = priv->manual_capabilities | priv->printer_capabilities;

  ctk_widget_set_sensitive (priv->page_set_combo,
                            caps & CTK_PRINT_CAPABILITY_PAGE_SET);
  ctk_widget_set_sensitive (priv->copies_spin,
                            caps & CTK_PRINT_CAPABILITY_COPIES);
  ctk_widget_set_sensitive (priv->collate_check,
                            can_collate &&
                            (caps & CTK_PRINT_CAPABILITY_COLLATE));
  ctk_widget_set_sensitive (priv->reverse_check,
                            caps & CTK_PRINT_CAPABILITY_REVERSE);
  ctk_widget_set_sensitive (priv->scale_spin,
                            caps & CTK_PRINT_CAPABILITY_SCALE);
  ctk_widget_set_sensitive (CTK_WIDGET (priv->pages_per_sheet),
                            caps & CTK_PRINT_CAPABILITY_NUMBER_UP);

  button = ctk_dialog_get_widget_for_response (CTK_DIALOG (dialog), CTK_RESPONSE_APPLY);
  ctk_widget_set_visible (button, (caps & CTK_PRINT_CAPABILITY_PREVIEW) != 0);

  update_collate_icon (NULL, dialog);

  ctk_tree_model_filter_refilter (priv->printer_list_filter);
}

static gboolean
page_setup_is_equal (CtkPageSetup *a,
                     CtkPageSetup *b)
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
page_setup_is_same_size (CtkPageSetup *a,
                         CtkPageSetup *b)
{
  return ctk_paper_size_is_equal (ctk_page_setup_get_paper_size (a),
                                  ctk_page_setup_get_paper_size (b));
}

static gboolean
set_paper_size (CtkPrintUnixDialog *dialog,
                CtkPageSetup       *page_setup,
                gboolean            size_only,
                gboolean            add_item)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkPageSetup *list_page_setup;

  if (!priv->internal_page_setup_change)
    return TRUE;

  if (page_setup == NULL)
    return FALSE;

  model = CTK_TREE_MODEL (priv->page_setup_list);

  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          ctk_tree_model_get (CTK_TREE_MODEL (priv->page_setup_list), &iter,
                              PAGE_SETUP_LIST_COL_PAGE_SETUP, &list_page_setup,
                              -1);
          if (list_page_setup == NULL)
            continue;

          if ((size_only && page_setup_is_same_size (page_setup, list_page_setup)) ||
              (!size_only && page_setup_is_equal (page_setup, list_page_setup)))
            {
              ctk_combo_box_set_active_iter (CTK_COMBO_BOX (priv->paper_size_combo),
                                             &iter);
              ctk_combo_box_set_active (CTK_COMBO_BOX (priv->orientation_combo),
                                        ctk_page_setup_get_orientation (page_setup));
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
      ctk_combo_box_set_active (CTK_COMBO_BOX (priv->orientation_combo),
                                ctk_page_setup_get_orientation (page_setup));
      return TRUE;
    }

  return FALSE;
}

static void
fill_custom_paper_sizes (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter iter, paper_iter;
  CtkTreeModel *model;

  model = CTK_TREE_MODEL (priv->custom_paper_list);
  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      ctk_list_store_append (priv->page_setup_list, &paper_iter);
      ctk_list_store_set (priv->page_setup_list, &paper_iter,
                          PAGE_SETUP_LIST_COL_IS_SEPARATOR, TRUE,
                          -1);
      do
        {
          CtkPageSetup *page_setup;
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
fill_paper_sizes (CtkPrintUnixDialog *dialog,
                  CtkPrinter         *printer)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  GList *list, *l;
  CtkPageSetup *page_setup;
  CtkPaperSize *paper_size;
  CtkTreeIter iter;
  gint i;

  ctk_list_store_clear (priv->page_setup_list);

  if (printer == NULL || (list = ctk_printer_list_papers (printer)) == NULL)
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
}

static void
update_paper_sizes (CtkPrintUnixDialog *dialog)
{
  CtkPageSetup *current_page_setup = NULL;
  CtkPrinter   *printer;

  printer = ctk_print_unix_dialog_get_selected_printer (dialog);

  fill_paper_sizes (dialog, printer);

  current_page_setup = ctk_page_setup_copy (ctk_print_unix_dialog_get_page_setup (dialog));

  if (current_page_setup)
    {
      if (!set_paper_size (dialog, current_page_setup, FALSE, FALSE))
        set_paper_size (dialog, current_page_setup, TRUE, TRUE);

      g_object_unref (current_page_setup);
    }
}

static void
mark_conflicts (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrinter *printer;
  gboolean have_conflict;

  have_conflict = FALSE;

  printer = priv->current_printer;

  if (printer)
    {

      g_signal_handler_block (priv->options,
                              priv->options_changed_handler);

      ctk_printer_option_set_clear_conflicts (priv->options);

      have_conflict = _ctk_printer_mark_conflicts (printer,
                                                   priv->options);

      g_signal_handler_unblock (priv->options,
                                priv->options_changed_handler);
    }

  if (have_conflict)
    ctk_widget_show (priv->conflicts_widget);
  else
    ctk_widget_hide (priv->conflicts_widget);
}

static gboolean
mark_conflicts_callback (gpointer data)
{
  CtkPrintUnixDialog *dialog = data;
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  priv->mark_conflicts_id = 0;

  mark_conflicts (dialog);

  return FALSE;
}

static void
unschedule_idle_mark_conflicts (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->mark_conflicts_id != 0)
    {
      g_source_remove (priv->mark_conflicts_id);
      priv->mark_conflicts_id = 0;
    }
}

static void
schedule_idle_mark_conflicts (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->mark_conflicts_id != 0)
    return;

  priv->mark_conflicts_id = cdk_threads_add_idle (mark_conflicts_callback,
                                        dialog);
  g_source_set_name_by_id (priv->mark_conflicts_id, "[ctk+] mark_conflicts_callback");
}

static void
options_changed_cb (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  schedule_idle_mark_conflicts (dialog);

  g_free (priv->waiting_for_printer);
  priv->waiting_for_printer = NULL;
}

static void
remove_custom_widget (CtkWidget    *widget,
                      CtkContainer *container)
{
  ctk_container_remove (container, widget);
}

static void
extension_point_clear_children (CtkContainer *container)
{
  ctk_container_foreach (container,
                         (CtkCallback)remove_custom_widget,
                         container);
}

static void
clear_per_printer_ui (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->finishing_table == NULL)
    return;

  ctk_container_foreach (CTK_CONTAINER (priv->finishing_table),
                         (CtkCallback)ctk_widget_destroy, NULL);
  ctk_container_foreach (CTK_CONTAINER (priv->image_quality_table),
                         (CtkCallback)ctk_widget_destroy, NULL);
  ctk_container_foreach (CTK_CONTAINER (priv->color_table),
                         (CtkCallback)ctk_widget_destroy, NULL);
  ctk_container_foreach (CTK_CONTAINER (priv->advanced_vbox),
                         (CtkCallback)ctk_widget_destroy, NULL);
  extension_point_clear_children (CTK_CONTAINER (priv->extension_point));
}

static void
printer_details_acquired (CtkPrinter         *printer,
                          gboolean            success,
                          CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  disconnect_printer_details_request (dialog, !success);

  if (success)
    {
      CtkTreeSelection *selection;
      selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->printer_treeview));

      selected_printer_changed (selection, dialog);
    }
}

static void
selected_printer_changed (CtkTreeSelection   *selection,
                          CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrinter *printer;
  CtkTreeIter iter, filter_iter;

  /* Whenever the user selects a printer we stop looking for
   * the printer specified in the initial settings
   */
  if (priv->waiting_for_printer &&
      !priv->internal_printer_change)
    {
      g_free (priv->waiting_for_printer);
      priv->waiting_for_printer = NULL;
    }

  disconnect_printer_details_request (dialog, FALSE);

  printer = NULL;
  if (ctk_tree_selection_get_selected (selection, NULL, &filter_iter))
    {
      ctk_tree_model_filter_convert_iter_to_child_iter (priv->printer_list_filter,
                                                        &iter,
                                                        &filter_iter);

      ctk_tree_model_get (priv->printer_list, &iter,
                          PRINTER_LIST_COL_PRINTER_OBJ, &printer,
                          -1);
    }

  /* sets CTK_RESPONSE_OK button sensitivity depending on whether the printer
   * accepts/rejects jobs
   */
  if (printer != NULL)
    {
      if (!ctk_printer_is_accepting_jobs (printer))
        ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), CTK_RESPONSE_OK, FALSE);
      else if (priv->current_printer == printer && ctk_printer_has_details (printer))

        ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), CTK_RESPONSE_OK, TRUE);
    }

  if (printer != NULL && !ctk_printer_has_details (printer))
    {
      ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), CTK_RESPONSE_OK, FALSE);
      priv->request_details_tag =
        g_signal_connect (printer, "details-acquired",
                          G_CALLBACK (printer_details_acquired), dialog);
      /* take the reference */
      priv->request_details_printer = printer;
      set_busy_cursor (dialog, TRUE);
      ctk_list_store_set (CTK_LIST_STORE (priv->printer_list),
                          g_object_get_data (G_OBJECT (printer), "ctk-print-tree-iter"),
                          PRINTER_LIST_COL_STATE, _("Getting printer information…"),
                          -1);
      ctk_printer_request_details (printer);
      return;
    }

  if (printer == priv->current_printer)
    {
      if (printer)
        g_object_unref (printer);
      return;
    }

  if (priv->options)
    {
      g_clear_object (&priv->options);
      clear_per_printer_ui (dialog);
    }

  g_clear_object (&priv->current_printer);
  priv->printer_capabilities = 0;

  if (printer != NULL && ctk_printer_is_accepting_jobs (printer))
    ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), CTK_RESPONSE_OK, TRUE);
  priv->current_printer = printer;

  if (printer != NULL)
    {
      if (!priv->page_setup_set)
        {
          /* if no explicit page setup has been set, use the printer default */
          CtkPageSetup *page_setup;

          page_setup = ctk_printer_get_default_page_size (printer);

          if (!page_setup)
            page_setup = ctk_page_setup_new ();

          if (page_setup && priv->page_setup)
            ctk_page_setup_set_orientation (page_setup, ctk_page_setup_get_orientation (priv->page_setup));

          g_clear_object (&priv->page_setup);
          priv->page_setup = page_setup; /* transfer ownership */
        }

      priv->printer_capabilities = ctk_printer_get_capabilities (printer);
      priv->options = _ctk_printer_get_options (printer,
                                                priv->initial_settings,
                                                priv->page_setup,
                                                priv->manual_capabilities);

      priv->options_changed_handler =
        g_signal_connect_swapped (priv->options, "changed", G_CALLBACK (options_changed_cb), dialog);
      schedule_idle_mark_conflicts (dialog);
    }

  update_dialog_from_settings (dialog);
  update_dialog_from_capabilities (dialog);

  priv->internal_page_setup_change = TRUE;
  update_paper_sizes (dialog);
  priv->internal_page_setup_change = FALSE;

  g_object_notify ( G_OBJECT(dialog), "selected-printer");
}

static gboolean
printer_compare (CtkTreeModel *model,
                 gint          column,
                 const gchar  *key,
                 CtkTreeIter  *iter,
                 gpointer      search_data)
{
  gboolean matches = FALSE;

  if (key != NULL)
    {
      gchar  *name = NULL;
      gchar  *location = NULL;
      gchar  *casefold_key = NULL;
      gchar  *casefold_name = NULL;
      gchar  *casefold_location = NULL;
      gchar **keys;
      gchar  *tmp1, *tmp2;
      gint    i;

      ctk_tree_model_get (model, iter,
                          PRINTER_LIST_COL_NAME, &name,
                          PRINTER_LIST_COL_LOCATION, &location,
                          -1);

      casefold_key = g_utf8_casefold (key, -1);

      if (name != NULL)
        {
          casefold_name = g_utf8_casefold (name, -1);
          g_free (name);
        }

      if (location != NULL)
        {
          casefold_location = g_utf8_casefold (location, -1);
          g_free (location);
        }

      if (casefold_name != NULL ||
          casefold_location != NULL)
        {
          keys = g_strsplit_set (casefold_key, " \t", 0);
          if (keys != NULL)
            {
              matches = TRUE;

              for (i = 0; keys[i] != NULL; i++)
                {
                  if (keys[i][0] != '\0')
                    {
                      tmp1 = tmp2 = NULL;

                      if (casefold_name != NULL)
                        tmp1 = g_strstr_len (casefold_name, -1, keys[i]);

                      if (casefold_location != NULL)
                        tmp2 = g_strstr_len (casefold_location, -1, keys[i]);

                      if (tmp1 == NULL && tmp2 == NULL)
                        {
                          matches = FALSE;
                          break;
                        }
                    }
                }

              g_strfreev (keys);
            }
        }

      g_free (casefold_location);
      g_free (casefold_name);
      g_free (casefold_key);
    }

  return !matches;
}

static void
update_collate_icon (CtkToggleButton    *toggle_button,
                     CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  ctk_widget_queue_draw (priv->collate_image);
}

static void
paint_page (CtkWidget *widget,
            cairo_t   *cr,
            gint       x,
            gint       y,
            gchar     *text,
            gint       text_x)
{
  CtkStyleContext *context;
  gint width, height;
  gint text_y;

  width = 20;
  height = 26;
  text_y = 21;

  context = ctk_widget_get_style_context (widget);

  ctk_render_background (context, cr, x, y, width, height);
  ctk_render_frame (context, cr, x, y, width, height);

  cairo_select_font_face (cr, "Sans",
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 9);
  cairo_move_to (cr, x + text_x, y + text_y);
  cairo_show_text (cr, text);
}

static gboolean
draw_collate_cb (CtkWidget          *widget,
                 cairo_t            *cr,
                 CtkPrintUnixDialog *dialog)
{
  gboolean collate, reverse, rtl;
  gint copies;
  gint text_x;
  gint x, y, x1, x2, p1, p2;

  collate = dialog_get_collate (dialog);
  reverse = dialog_get_reverse (dialog);
  copies = dialog_get_n_copies (dialog);

  rtl = (ctk_widget_get_direction (CTK_WIDGET (widget)) == CTK_TEXT_DIR_RTL);

  x = (ctk_widget_get_allocated_width (widget) - 30) / 2;
  y = (ctk_widget_get_allocated_height (widget) - 36) / 2;
  if (rtl)
    {
      x1 = x;
      x2 = x - 36;
      p1 = 0;
      p2 = 10;
      text_x = 4;
    }
  else
    {
      x1 = x;
      x2 = x + 36;
      p1 = 10;
      p2 = 0;
      text_x = 11;
    }

  if (copies == 1)
    {
      paint_page (widget, cr, x1 + p1, y, reverse ? "1" : "2", text_x);
      paint_page (widget, cr, x1 + p2, y + 10, reverse ? "2" : "1", text_x);
    }
  else
    {
      paint_page (widget, cr, x1 + p1, y, collate == reverse ? "1" : "2", text_x);
      paint_page (widget, cr, x1 + p2, y + 10, reverse ? "2" : "1", text_x);

      paint_page (widget, cr, x2 + p1, y, reverse ? "1" : "2", text_x);
      paint_page (widget, cr, x2 + p2, y + 10, collate == reverse ? "2" : "1", text_x);
    }

  return TRUE;
}

static void
ctk_print_unix_dialog_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_print_unix_dialog_parent_class)->style_updated (widget);

  if (ctk_widget_has_screen (widget))
    {
      CtkPrintUnixDialog *dialog = (CtkPrintUnixDialog *)widget;
      CtkPrintUnixDialogPrivate *priv = dialog->priv;
      gint size;
      gfloat scale;

      ctk_icon_size_lookup (CTK_ICON_SIZE_DIALOG, &size, NULL);
      scale = size / 48.0;

      ctk_widget_set_size_request (priv->collate_image,
                                   (50 + 20) * scale,
                                   (15 + 26) * scale);
    }
}

static void
update_page_range_entry_sensitivity (CtkWidget *button,
				     CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  gboolean active;

  active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));

  ctk_widget_set_sensitive (priv->page_range_entry, active);

  if (active)
    ctk_widget_grab_focus (priv->page_range_entry);
}

static void
update_print_at_entry_sensitivity (CtkWidget *button,
				   CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  gboolean active;

  active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));

  ctk_widget_set_sensitive (priv->print_at_entry, active);

  if (active)
    ctk_widget_grab_focus (priv->print_at_entry);
}

static void
emit_ok_response (CtkTreeView       *tree_view,
                  CtkTreePath       *path,
                  CtkTreeViewColumn *column,
                  gpointer          *user_data)
{
  CtkPrintUnixDialog *print_dialog;

  print_dialog = (CtkPrintUnixDialog *) user_data;

  ctk_dialog_response (CTK_DIALOG (print_dialog), CTK_RESPONSE_OK);
}

static gboolean
is_range_separator (gchar c)
{
  return (c == ',' || c == ';' || c == ':');
}

static CtkPageRange *
dialog_get_page_ranges (CtkPrintUnixDialog *dialog,
                        gint               *n_ranges_out)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  gint i, n_ranges;
  const gchar *text, *p;
  gchar *next;
  CtkPageRange *ranges;
  gint start, end;

  text = ctk_entry_get_text (CTK_ENTRY (priv->page_range_entry));

  if (*text == 0)
    {
      *n_ranges_out = 0;
      return NULL;
    }

  n_ranges = 1;
  p = text;
  while (*p)
    {
      if (is_range_separator (*p))
        n_ranges++;
      p++;
    }

  ranges = g_new0 (CtkPageRange, n_ranges);

  i = 0;
  p = text;
  while (*p)
    {
      while (isspace (*p)) p++;

      if (*p == '-')
        {
          /* a half-open range like -2 */
          start = 1;
        }
      else
        {
          start = (int)strtol (p, &next, 10);
          if (start < 1)
            start = 1;
          p = next;
        }

      end = start;

      while (isspace (*p)) p++;

      if (*p == '-')
        {
          p++;
          end = (int)strtol (p, &next, 10);
          if (next == p) /* a half-open range like 2- */
            end = 0;
          else if (end < start)
            end = start;
        }

      ranges[i].start = start - 1;
      ranges[i].end = end - 1;
      i++;

      /* Skip until end or separator */
      while (*p && !is_range_separator (*p))
        p++;

      /* if not at end, skip separator */
      if (*p)
        p++;
    }

  *n_ranges_out = i;

  return ranges;
}

static void
dialog_set_page_ranges (CtkPrintUnixDialog *dialog,
                        CtkPageRange       *ranges,
                        gint                n_ranges)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  gint i;
  GString *s = g_string_new (NULL);

  for (i = 0; i < n_ranges; i++)
    {
      g_string_append_printf (s, "%d", ranges[i].start + 1);
      if (ranges[i].end > ranges[i].start)
        g_string_append_printf (s, "-%d", ranges[i].end + 1);
      else if (ranges[i].end == -1)
        g_string_append (s, "-");

      if (i != n_ranges - 1)
        g_string_append (s, ",");
    }

  ctk_entry_set_text (CTK_ENTRY (priv->page_range_entry), s->str);

  g_string_free (s, TRUE);
}

static CtkPrintPages
dialog_get_print_pages (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->all_pages_radio)))
    return CTK_PRINT_PAGES_ALL;
  else if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->current_page_radio)))
    return CTK_PRINT_PAGES_CURRENT;
  else if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->selection_radio)))
    return CTK_PRINT_PAGES_SELECTION;
  else
    return CTK_PRINT_PAGES_RANGES;
}

static void
dialog_set_print_pages (CtkPrintUnixDialog *dialog,
                        CtkPrintPages       pages)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (pages == CTK_PRINT_PAGES_RANGES)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->page_range_radio), TRUE);
  else if (pages == CTK_PRINT_PAGES_CURRENT)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->current_page_radio), TRUE);
  else if (pages == CTK_PRINT_PAGES_SELECTION)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->selection_radio), TRUE);
  else
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->all_pages_radio), TRUE);
}

static gdouble
dialog_get_scale (CtkPrintUnixDialog *dialog)
{
  if (ctk_widget_is_sensitive (dialog->priv->scale_spin))
    return ctk_spin_button_get_value (CTK_SPIN_BUTTON (dialog->priv->scale_spin));
  else
    return 100.0;
}

static void
dialog_set_scale (CtkPrintUnixDialog *dialog,
                  gdouble             val)
{
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (dialog->priv->scale_spin), val);
}

static CtkPageSet
dialog_get_page_set (CtkPrintUnixDialog *dialog)
{
  if (ctk_widget_is_sensitive (dialog->priv->page_set_combo))
    return (CtkPageSet)ctk_combo_box_get_active (CTK_COMBO_BOX (dialog->priv->page_set_combo));
  else
    return CTK_PAGE_SET_ALL;
}

static void
dialog_set_page_set (CtkPrintUnixDialog *dialog,
                     CtkPageSet          val)
{
  ctk_combo_box_set_active (CTK_COMBO_BOX (dialog->priv->page_set_combo),
                            (int)val);
}

static gint
dialog_get_n_copies (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkAdjustment *adjustment;
  const gchar *text;
  gchar *endptr = NULL;
  gint n_copies;

  adjustment = ctk_spin_button_get_adjustment (CTK_SPIN_BUTTON (priv->copies_spin));

  text = ctk_entry_get_text (CTK_ENTRY (priv->copies_spin));
  n_copies = g_ascii_strtoull (text, &endptr, 0);

  if (ctk_widget_is_sensitive (dialog->priv->copies_spin))
    {
      if (n_copies != 0 && endptr != text && (endptr != NULL && endptr[0] == '\0') &&
          n_copies >= ctk_adjustment_get_lower (adjustment) &&
          n_copies <= ctk_adjustment_get_upper (adjustment))
        {
          return n_copies;
        }

      return ctk_spin_button_get_value_as_int (CTK_SPIN_BUTTON (priv->copies_spin));
    }

  return 1;
}

static void
dialog_set_n_copies (CtkPrintUnixDialog *dialog,
                     gint                n_copies)
{
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (dialog->priv->copies_spin),
                             n_copies);
}

static gboolean
dialog_get_collate (CtkPrintUnixDialog *dialog)
{
  if (ctk_widget_is_sensitive (dialog->priv->collate_check))
    return ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (dialog->priv->collate_check));
  return FALSE;
}

static void
dialog_set_collate (CtkPrintUnixDialog *dialog,
                    gboolean            collate)
{
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (dialog->priv->collate_check),
                                collate);
}

static gboolean
dialog_get_reverse (CtkPrintUnixDialog *dialog)
{
  if (ctk_widget_is_sensitive (dialog->priv->reverse_check))
    return ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (dialog->priv->reverse_check));
  return FALSE;
}

static void
dialog_set_reverse (CtkPrintUnixDialog *dialog,
                    gboolean            reverse)
{
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (dialog->priv->reverse_check),
                                reverse);
}

static gint
dialog_get_pages_per_sheet (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  const gchar *val;
  gint num;

  val = ctk_printer_option_widget_get_value (priv->pages_per_sheet);

  num = 1;

  if (val)
    {
      num = atoi(val);
      if (num < 1)
        num = 1;
    }

  return num;
}

static CtkNumberUpLayout
dialog_get_number_up_layout (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrintCapabilities       caps;
  CtkNumberUpLayout          layout;
  const gchar               *val;
  GEnumClass                *enum_class;
  GEnumValue                *enum_value;

  val = ctk_printer_option_widget_get_value (priv->number_up_layout);

  caps = priv->manual_capabilities | priv->printer_capabilities;

  if ((caps & CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT) == 0)
    return CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;

  if (ctk_widget_get_direction (CTK_WIDGET (dialog)) == CTK_TEXT_DIR_LTR)
    layout = CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
  else
    layout = CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM;

  if (val == NULL)
    return layout;

  if (val[0] == '\0' && priv->options)
    {
      CtkPrinterOption *option = ctk_printer_option_set_lookup (priv->options, "ctk-n-up-layout");
      if (option)
        val = option->value;
    }

  enum_class = g_type_class_ref (CTK_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value_by_nick (enum_class, val);
  if (enum_value)
    layout = enum_value->value;
  g_type_class_unref (enum_class);

  return layout;
}

static gboolean
draw_page_cb (CtkWidget          *widget,
              cairo_t            *cr,
              CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkStyleContext *context;
  gdouble ratio;
  gint w, h, tmp, shadow_offset;
  gint pages_x, pages_y, i, x, y, layout_w, layout_h;
  gdouble page_width, page_height;
  CtkPageOrientation orientation;
  gboolean landscape;
  PangoLayout *layout;
  PangoFontDescription *font;
  gchar *text;
  GdkRGBA color;
  CtkNumberUpLayout number_up_layout;
  gint start_x, end_x, start_y, end_y;
  gint dx, dy;
  gint width, height;
  gboolean horizontal;
  CtkPageSetup *page_setup;
  gdouble paper_width, paper_height;
  gdouble pos_x, pos_y;
  gint pages_per_sheet;
  gboolean ltr = TRUE;

  orientation = ctk_page_setup_get_orientation (priv->page_setup);
  landscape =
    (orientation == CTK_PAGE_ORIENTATION_LANDSCAPE) ||
    (orientation == CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE);

  number_up_layout = dialog_get_number_up_layout (dialog);
  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);

  cairo_save (cr);

  page_setup = ctk_print_unix_dialog_get_page_setup (dialog);

  if (page_setup != NULL)
    {
      if (!landscape)
        {
          paper_width = ctk_page_setup_get_paper_width (page_setup, CTK_UNIT_MM);
          paper_height = ctk_page_setup_get_paper_height (page_setup, CTK_UNIT_MM);
        }
      else
        {
          paper_width = ctk_page_setup_get_paper_height (page_setup, CTK_UNIT_MM);
          paper_height = ctk_page_setup_get_paper_width (page_setup, CTK_UNIT_MM);
        }

      if (paper_width < paper_height)
        {
          h = EXAMPLE_PAGE_AREA_SIZE - 3;
          w = (paper_height != 0) ? h * paper_width / paper_height : 0;
        }
      else
        {
          w = EXAMPLE_PAGE_AREA_SIZE - 3;
          h = (paper_width != 0) ? w * paper_height / paper_width : 0;
        }

      if (paper_width == 0)
        w = 0;

      if (paper_height == 0)
        h = 0;
    }
  else
    {
      ratio = G_SQRT2;
      w = (EXAMPLE_PAGE_AREA_SIZE - 3) / ratio;
      h = EXAMPLE_PAGE_AREA_SIZE - 3;
    }

  pages_per_sheet = dialog_get_pages_per_sheet (dialog);
  switch (pages_per_sheet)
    {
    default:
    case 1:
      pages_x = 1; pages_y = 1;
      break;
    case 2:
      landscape = !landscape;
      pages_x = 1; pages_y = 2;
      break;
    case 4:
      pages_x = 2; pages_y = 2;
      break;
    case 6:
      landscape = !landscape;
      pages_x = 2; pages_y = 3;
      break;
    case 9:
      pages_x = 3; pages_y = 3;
      break;
    case 16:
      pages_x = 4; pages_y = 4;
      break;
    }

  if (landscape)
    {
      tmp = w;
      w = h;
      h = tmp;

      tmp = pages_x;
      pages_x = pages_y;
      pages_y = tmp;
    }

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_get_color (context, ctk_style_context_get_state (context), &color);

  pos_x = (width - w) / 2;
  pos_y = (height - h) / 2 - 10;
  cairo_translate (cr, pos_x, pos_y);

  shadow_offset = 3;

  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.5);
  cairo_rectangle (cr, shadow_offset + 1, shadow_offset + 1, w, h);
  cairo_fill (cr);

  ctk_render_background (context, cr, 1, 1, w, h);

  cairo_set_line_width (cr, 1.0);
  cairo_rectangle (cr, 0.5, 0.5, w + 1, h + 1);
  cdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  i = 1;

  page_width = (gdouble)w / pages_x;
  page_height = (gdouble)h / pages_y;

  layout  = pango_cairo_create_layout (cr);

  font = pango_font_description_new ();
  pango_font_description_set_family (font, "sans");

  if (page_height > 0)
    pango_font_description_set_absolute_size (font, page_height * 0.4 * PANGO_SCALE);
  else
    pango_font_description_set_absolute_size (font, 1);

  pango_layout_set_font_description (layout, font);
  pango_font_description_free (font);

  pango_layout_set_width (layout, page_width * PANGO_SCALE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

  switch (number_up_layout)
    {
      default:
      case CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = 0;
        end_y = pages_y - 1;
        dx = 1;
        dy = 1;
        horizontal = TRUE;
        break;
      case CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = pages_y - 1;
        end_y = 0;
        dx = 1;
        dy = - 1;
        horizontal = TRUE;
        break;
      case CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = 0;
        end_y = pages_y - 1;
        dx = - 1;
        dy = 1;
        horizontal = TRUE;
        break;
      case CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = pages_y - 1;
        end_y = 0;
        dx = - 1;
        dy = - 1;
        horizontal = TRUE;
        break;
      case CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = 0;
        end_y = pages_y - 1;
        dx = 1;
        dy = 1;
        horizontal = FALSE;
        break;
      case CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = 0;
        end_y = pages_y - 1;
        dx = - 1;
        dy = 1;
        horizontal = FALSE;
        break;
      case CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = pages_y - 1;
        end_y = 0;
        dx = 1;
        dy = - 1;
        horizontal = FALSE;
        break;
      case CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = pages_y - 1;
        end_y = 0;
        dx = - 1;
        dy = - 1;
        horizontal = FALSE;
        break;
    }

  if (horizontal)
    for (y = start_y; y != end_y + dy; y += dy)
      {
        for (x = start_x; x != end_x + dx; x += dx)
          {
            text = g_strdup_printf ("%d", i++);
            pango_layout_set_text (layout, text, -1);
            g_free (text);
            pango_layout_get_size (layout, &layout_w, &layout_h);
            cairo_save (cr);
            cairo_translate (cr,
                             x * page_width,
                             y * page_height + (page_height - layout_h / 1024.0) / 2);

            pango_cairo_show_layout (cr, layout);
            cairo_restore (cr);
          }
      }
  else
    for (x = start_x; x != end_x + dx; x += dx)
      {
        for (y = start_y; y != end_y + dy; y += dy)
          {
            text = g_strdup_printf ("%d", i++);
            pango_layout_set_text (layout, text, -1);
            g_free (text);
            pango_layout_get_size (layout, &layout_w, &layout_h);
            cairo_save (cr);
            cairo_translate (cr,
                             x * page_width,
                             y * page_height + (page_height - layout_h / 1024.0) / 2);

            pango_cairo_show_layout (cr, layout);
            cairo_restore (cr);
          }
      }

  g_object_unref (layout);

  if (page_setup != NULL)
    {
      PangoContext *pango_c = NULL;
      PangoFontDescription *pango_f = NULL;
      gint font_size = 12 * PANGO_SCALE;

      pos_x += 1;
      pos_y += 1;

      if (pages_per_sheet == 2 || pages_per_sheet == 6)
        {
          paper_width = ctk_page_setup_get_paper_height (page_setup, _ctk_print_get_default_user_units ());
          paper_height = ctk_page_setup_get_paper_width (page_setup, _ctk_print_get_default_user_units ());
        }
      else
        {
          paper_width = ctk_page_setup_get_paper_width (page_setup, _ctk_print_get_default_user_units ());
          paper_height = ctk_page_setup_get_paper_height (page_setup, _ctk_print_get_default_user_units ());
        }

      cairo_restore (cr);
      cairo_save (cr);

      layout = pango_cairo_create_layout (cr);

      font = pango_font_description_new ();
      pango_font_description_set_family (font, "sans");

      pango_c = ctk_widget_get_pango_context (widget);
      if (pango_c != NULL)
        {
          pango_f = pango_context_get_font_description (pango_c);
          if (pango_f != NULL)
            font_size = pango_font_description_get_size (pango_f);
        }

      pango_font_description_set_size (font, font_size);
      pango_layout_set_font_description (layout, font);
      pango_font_description_free (font);

      pango_layout_set_width (layout, -1);
      pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

      if (_ctk_print_get_default_user_units () == CTK_UNIT_MM)
        text = g_strdup_printf ("%.1f mm", paper_height);
      else
        text = g_strdup_printf ("%.2f inch", paper_height);

      pango_layout_set_text (layout, text, -1);
      g_free (text);
      pango_layout_get_size (layout, &layout_w, &layout_h);

      ltr = ctk_widget_get_direction (CTK_WIDGET (dialog)) == CTK_TEXT_DIR_LTR;

      if (ltr)
        cairo_translate (cr, pos_x - layout_w / PANGO_SCALE - 2 * RULER_DISTANCE,
                             (height - layout_h / PANGO_SCALE) / 2);
      else
        cairo_translate (cr, pos_x + w + shadow_offset + 2 * RULER_DISTANCE,
                             (height - layout_h / PANGO_SCALE) / 2);

      cdk_cairo_set_source_rgba (cr, &color);
      pango_cairo_show_layout (cr, layout);

      cairo_restore (cr);
      cairo_save (cr);

      if (_ctk_print_get_default_user_units () == CTK_UNIT_MM)
        text = g_strdup_printf ("%.1f mm", paper_width);
      else
        text = g_strdup_printf ("%.2f inch", paper_width);

      pango_layout_set_text (layout, text, -1);
      g_free (text);
      pango_layout_get_size (layout, &layout_w, &layout_h);

      cairo_translate (cr, (width - layout_w / PANGO_SCALE) / 2,
                           pos_y + h + shadow_offset + 2 * RULER_DISTANCE);

      cdk_cairo_set_source_rgba (cr, &color);
      pango_cairo_show_layout (cr, layout);

      g_object_unref (layout);

      cairo_restore (cr);

      cairo_set_line_width (cr, 1);

      cdk_cairo_set_source_rgba (cr, &color);

      if (ltr)
        {
          cairo_move_to (cr, pos_x - RULER_DISTANCE, pos_y);
          cairo_line_to (cr, pos_x - RULER_DISTANCE, pos_y + h);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x - RULER_DISTANCE - RULER_RADIUS, pos_y - 0.5);
          cairo_line_to (cr, pos_x - RULER_DISTANCE + RULER_RADIUS, pos_y - 0.5);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x - RULER_DISTANCE - RULER_RADIUS, pos_y + h + 0.5);
          cairo_line_to (cr, pos_x - RULER_DISTANCE + RULER_RADIUS, pos_y + h + 0.5);
          cairo_stroke (cr);
        }
      else
        {
          cairo_move_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE, pos_y);
          cairo_line_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE, pos_y + h);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE - RULER_RADIUS, pos_y - 0.5);
          cairo_line_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE + RULER_RADIUS, pos_y - 0.5);
          cairo_stroke (cr);

          cairo_move_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE - RULER_RADIUS, pos_y + h + 0.5);
          cairo_line_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE + RULER_RADIUS, pos_y + h + 0.5);
          cairo_stroke (cr);
        }

      cairo_move_to (cr, pos_x, pos_y + h + shadow_offset + RULER_DISTANCE);
      cairo_line_to (cr, pos_x + w, pos_y + h + shadow_offset + RULER_DISTANCE);
      cairo_stroke (cr);

      cairo_move_to (cr, pos_x - 0.5, pos_y + h + shadow_offset + RULER_DISTANCE - RULER_RADIUS);
      cairo_line_to (cr, pos_x - 0.5, pos_y + h + shadow_offset + RULER_DISTANCE + RULER_RADIUS);
      cairo_stroke (cr);

      cairo_move_to (cr, pos_x + w + 0.5, pos_y + h + shadow_offset + RULER_DISTANCE - RULER_RADIUS);
      cairo_line_to (cr, pos_x + w + 0.5, pos_y + h + shadow_offset + RULER_DISTANCE + RULER_RADIUS);
      cairo_stroke (cr);
    }

  return TRUE;
}

static void
redraw_page_layout_preview (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->page_layout_preview)
    ctk_widget_queue_draw (priv->page_layout_preview);
}

static void
update_number_up_layout (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPrintCapabilities       caps;
  CtkPrinterOptionSet       *set;
  CtkNumberUpLayout          layout;
  CtkPrinterOption          *option;
  CtkPrinterOption          *old_option;
  CtkPageOrientation         page_orientation;

  set = priv->options;

  caps = priv->manual_capabilities | priv->printer_capabilities;

  if (caps & CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT)
    {
      if (priv->number_up_layout_n_option == NULL)
        {
          priv->number_up_layout_n_option = ctk_printer_option_set_lookup (set, "ctk-n-up-layout");
          if (priv->number_up_layout_n_option == NULL)
            {
              char *n_up_layout[] = { "lrtb", "lrbt", "rltb", "rlbt", "tblr", "tbrl", "btlr", "btrl" };
               /* Translators: These strings name the possible arrangements of
                * multiple pages on a sheet when printing (same as in ctkprintbackendcups.c)
                */
              char *n_up_layout_display[] = { N_("Left to right, top to bottom"), N_("Left to right, bottom to top"),
                                              N_("Right to left, top to bottom"), N_("Right to left, bottom to top"),
                                              N_("Top to bottom, left to right"), N_("Top to bottom, right to left"),
                                              N_("Bottom to top, left to right"), N_("Bottom to top, right to left") };
              int i;

              priv->number_up_layout_n_option = ctk_printer_option_new ("ctk-n-up-layout",
                                                                        _("Page Ordering"),
                                                                        CTK_PRINTER_OPTION_TYPE_PICKONE);
              ctk_printer_option_allocate_choices (priv->number_up_layout_n_option, 8);

              for (i = 0; i < G_N_ELEMENTS (n_up_layout_display); i++)
                {
                  priv->number_up_layout_n_option->choices[i] = g_strdup (n_up_layout[i]);
                  priv->number_up_layout_n_option->choices_display[i] = g_strdup (_(n_up_layout_display[i]));
                }
            }
          g_object_ref (priv->number_up_layout_n_option);

          priv->number_up_layout_2_option = ctk_printer_option_new ("ctk-n-up-layout",
                                                                    _("Page Ordering"),
                                                                    CTK_PRINTER_OPTION_TYPE_PICKONE);
          ctk_printer_option_allocate_choices (priv->number_up_layout_2_option, 2);
        }

      page_orientation = ctk_page_setup_get_orientation (priv->page_setup);
      if (page_orientation == CTK_PAGE_ORIENTATION_PORTRAIT ||
          page_orientation == CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT)
        {
          if (! (priv->number_up_layout_2_option->choices[0] == priv->number_up_layout_n_option->choices[0] &&
                 priv->number_up_layout_2_option->choices[1] == priv->number_up_layout_n_option->choices[2]))
            {
              g_free (priv->number_up_layout_2_option->choices_display[0]);
              g_free (priv->number_up_layout_2_option->choices_display[1]);
              priv->number_up_layout_2_option->choices[0] = priv->number_up_layout_n_option->choices[0];
              priv->number_up_layout_2_option->choices[1] = priv->number_up_layout_n_option->choices[2];
              priv->number_up_layout_2_option->choices_display[0] = g_strdup ( _("Left to right"));
              priv->number_up_layout_2_option->choices_display[1] = g_strdup ( _("Right to left"));
            }
        }
      else
        {
          if (! (priv->number_up_layout_2_option->choices[0] == priv->number_up_layout_n_option->choices[0] &&
                 priv->number_up_layout_2_option->choices[1] == priv->number_up_layout_n_option->choices[1]))
            {
              g_free (priv->number_up_layout_2_option->choices_display[0]);
              g_free (priv->number_up_layout_2_option->choices_display[1]);
              priv->number_up_layout_2_option->choices[0] = priv->number_up_layout_n_option->choices[0];
              priv->number_up_layout_2_option->choices[1] = priv->number_up_layout_n_option->choices[1];
              priv->number_up_layout_2_option->choices_display[0] = g_strdup ( _("Top to bottom"));
              priv->number_up_layout_2_option->choices_display[1] = g_strdup ( _("Bottom to top"));
            }
        }

      layout = dialog_get_number_up_layout (dialog);

      old_option = ctk_printer_option_set_lookup (set, "ctk-n-up-layout");
      if (old_option != NULL)
        ctk_printer_option_set_remove (set, old_option);

      if (dialog_get_pages_per_sheet (dialog) != 1)
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;
          enum_class = g_type_class_ref (CTK_TYPE_NUMBER_UP_LAYOUT);

          if (dialog_get_pages_per_sheet (dialog) == 2)
            {
              option = priv->number_up_layout_2_option;

              switch (layout)
                {
                case CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM:
                case CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT:
                  enum_value = g_enum_get_value (enum_class, CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);
                  break;

                case CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP:
                case CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT:
                  enum_value = g_enum_get_value (enum_class, CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP);
                  break;

                case CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM:
                case CTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT:
                  enum_value = g_enum_get_value (enum_class, CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM);
                  break;

                case CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP:
                case CTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT:
                  enum_value = g_enum_get_value (enum_class, CTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP);
                  break;

                default:
                  g_assert_not_reached();
                  enum_value = NULL;
                }
            }
          else
            {
              option = priv->number_up_layout_n_option;

              enum_value = g_enum_get_value (enum_class, layout);
            }

          g_assert (enum_value != NULL);
          ctk_printer_option_set (option, enum_value->value_nick);
          g_type_class_unref (enum_class);

          ctk_printer_option_set_add (set, option);
        }
    }

  setup_option (dialog, "ctk-n-up-layout", priv->number_up_layout);

  if (priv->number_up_layout != NULL)
    ctk_widget_set_sensitive (CTK_WIDGET (priv->number_up_layout),
                              (caps & CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT) &&
                              (dialog_get_pages_per_sheet (dialog) > 1));
}

static void
custom_paper_dialog_response_cb (CtkDialog *custom_paper_dialog,
                                 gint       response_id,
                                 gpointer   user_data)
{
  CtkPrintUnixDialog        *print_dialog = CTK_PRINT_UNIX_DIALOG (user_data);
  CtkPrintUnixDialogPrivate *priv = print_dialog->priv;
  CtkTreeModel              *model;
  CtkTreeIter                iter;

  _ctk_print_load_custom_papers (priv->custom_paper_list);

  priv->internal_page_setup_change = TRUE;
  update_paper_sizes (print_dialog);
  priv->internal_page_setup_change = FALSE;

  if (priv->page_setup_set)
    {
      model = CTK_TREE_MODEL (priv->custom_paper_list);
      if (ctk_tree_model_get_iter_first (model, &iter))
        {
          do
            {
              CtkPageSetup *page_setup;
              ctk_tree_model_get (model, &iter, 0, &page_setup, -1);

              if (page_setup &&
                  g_strcmp0 (ctk_paper_size_get_display_name (ctk_page_setup_get_paper_size (page_setup)),
                             ctk_paper_size_get_display_name (ctk_page_setup_get_paper_size (priv->page_setup))) == 0)
                ctk_print_unix_dialog_set_page_setup (print_dialog, page_setup);

              g_clear_object (&page_setup);
            } while (ctk_tree_model_iter_next (model, &iter));
        }
    }

  ctk_widget_destroy (CTK_WIDGET (custom_paper_dialog));
}

static void
orientation_changed (CtkComboBox        *combo_box,
                     CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkPageOrientation         orientation;
  CtkPageSetup              *page_setup;

  if (priv->internal_page_setup_change)
    return;

  orientation = (CtkPageOrientation) ctk_combo_box_get_active (CTK_COMBO_BOX (priv->orientation_combo));

  if (priv->page_setup)
    {
      page_setup = ctk_page_setup_copy (priv->page_setup);
      if (page_setup)
        ctk_page_setup_set_orientation (page_setup, orientation);

      ctk_print_unix_dialog_set_page_setup (dialog, page_setup);
    }

  redraw_page_layout_preview (dialog);
}

static void
paper_size_changed (CtkComboBox        *combo_box,
                    CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkTreeIter iter;
  CtkPageSetup *page_setup, *last_page_setup;
  CtkPageOrientation orientation;

  if (priv->internal_page_setup_change)
    return;

  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    {
      ctk_tree_model_get (ctk_combo_box_get_model (combo_box),
                          &iter, PAGE_SETUP_LIST_COL_PAGE_SETUP, &page_setup,
                          -1);

      if (page_setup == NULL)
        {
          CtkWidget *custom_paper_dialog;

          /* Change from "manage" menu item to last value */
          if (priv->page_setup)
            last_page_setup = g_object_ref (priv->page_setup);
          else
            last_page_setup = ctk_page_setup_new (); /* "good" default */

          if (!set_paper_size (dialog, last_page_setup, FALSE, FALSE))
            set_paper_size (dialog, last_page_setup, TRUE, TRUE);
          g_object_unref (last_page_setup);

          /* And show the custom paper dialog */
          custom_paper_dialog = _ctk_custom_paper_unix_dialog_new (CTK_WINDOW (dialog), _("Manage Custom Sizes"));
          g_signal_connect (custom_paper_dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), dialog);
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          ctk_window_present (CTK_WINDOW (custom_paper_dialog));
          G_GNUC_END_IGNORE_DEPRECATIONS

          return;
        }

      if (priv->page_setup)
        orientation = ctk_page_setup_get_orientation (priv->page_setup);
      else
        orientation = CTK_PAGE_ORIENTATION_PORTRAIT;

      ctk_page_setup_set_orientation (page_setup, orientation);
      ctk_print_unix_dialog_set_page_setup (dialog, page_setup);

      g_object_unref (page_setup);
    }

  redraw_page_layout_preview (dialog);
}

static gboolean
paper_size_row_is_separator (CtkTreeModel *model,
                             CtkTreeIter  *iter,
                             gpointer      data)
{
  gboolean separator;

  ctk_tree_model_get (model, iter,
                      PAGE_SETUP_LIST_COL_IS_SEPARATOR, &separator,
                      -1);
  return separator;
}

static void
page_name_func (CtkCellLayout   *cell_layout,
                CtkCellRenderer *cell,
                CtkTreeModel    *tree_model,
                CtkTreeIter     *iter,
                gpointer         data)
{
  CtkPageSetup *page_setup;
  CtkPaperSize *paper_size;

  ctk_tree_model_get (tree_model, iter,
                      PAGE_SETUP_LIST_COL_PAGE_SETUP, &page_setup,
                      -1);
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
 * ctk_print_unix_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 *
 * Creates a new #CtkPrintUnixDialog.
 *
 * Returns: a new #CtkPrintUnixDialog
 *
 * Since: 2.10
 */
CtkWidget *
ctk_print_unix_dialog_new (const gchar *title,
                           CtkWindow   *parent)
{
  CtkWidget *result;

  result = g_object_new (CTK_TYPE_PRINT_UNIX_DIALOG,
                         "transient-for", parent,
                         "title", title ? title : _("Print"),
                         NULL);

  return result;
}

/**
 * ctk_print_unix_dialog_get_selected_printer:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the currently selected printer.
 *
 * Returns: (transfer none): the currently selected printer
 *
 * Since: 2.10
 */
CtkPrinter *
ctk_print_unix_dialog_get_selected_printer (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  return dialog->priv->current_printer;
}

/**
 * ctk_print_unix_dialog_set_page_setup:
 * @dialog: a #CtkPrintUnixDialog
 * @page_setup: a #CtkPageSetup
 *
 * Sets the page setup of the #CtkPrintUnixDialog.
 *
 * Since: 2.10
 */
void
ctk_print_unix_dialog_set_page_setup (CtkPrintUnixDialog *dialog,
                                      CtkPageSetup       *page_setup)
{
  CtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog));
  g_return_if_fail (CTK_IS_PAGE_SETUP (page_setup));

  priv = dialog->priv;

  if (priv->page_setup != page_setup)
    {
      g_clear_object (&priv->page_setup);
      priv->page_setup = g_object_ref (page_setup);

      priv->page_setup_set = TRUE;

      g_object_notify (G_OBJECT (dialog), "page-setup");
    }
}

/**
 * ctk_print_unix_dialog_get_page_setup:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the page setup that is used by the #CtkPrintUnixDialog.
 *
 * Returns: (transfer none): the page setup of @dialog.
 *
 * Since: 2.10
 */
CtkPageSetup *
ctk_print_unix_dialog_get_page_setup (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  return dialog->priv->page_setup;
}

/**
 * ctk_print_unix_dialog_get_page_setup_set:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the page setup that is used by the #CtkPrintUnixDialog.
 *
 * Returns: whether a page setup was set by user.
 *
 * Since: 2.18
 */
gboolean
ctk_print_unix_dialog_get_page_setup_set (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->page_setup_set;
}

/**
 * ctk_print_unix_dialog_set_current_page:
 * @dialog: a #CtkPrintUnixDialog
 * @current_page: the current page number.
 *
 * Sets the current page number. If @current_page is not -1, this enables
 * the current page choice for the range of pages to print.
 *
 * Since: 2.10
 */
void
ctk_print_unix_dialog_set_current_page (CtkPrintUnixDialog *dialog,
                                        gint                current_page)
{
  CtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  if (priv->current_page != current_page)
    {
      priv->current_page = current_page;

      if (priv->current_page_radio)
        ctk_widget_set_sensitive (priv->current_page_radio, current_page != -1);

      g_object_notify (G_OBJECT (dialog), "current-page");
    }
}

/**
 * ctk_print_unix_dialog_get_current_page:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the current page of the #CtkPrintUnixDialog.
 *
 * Returns: the current page of @dialog
 *
 * Since: 2.10
 */
gint
ctk_print_unix_dialog_get_current_page (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), -1);

  return dialog->priv->current_page;
}

static gboolean
set_active_printer (CtkPrintUnixDialog *dialog,
                    const gchar        *printer_name)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;
  CtkTreeModel *model;
  CtkTreeIter iter, filter_iter;
  CtkTreeSelection *selection;
  CtkPrinter *printer;

  model = CTK_TREE_MODEL (priv->printer_list);

  if (ctk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          ctk_tree_model_get (CTK_TREE_MODEL (priv->printer_list), &iter,
                              PRINTER_LIST_COL_PRINTER_OBJ, &printer,
                              -1);
          if (printer == NULL)
            continue;

          if (strcmp (ctk_printer_get_name (printer), printer_name) == 0)
            {
              ctk_tree_model_filter_convert_child_iter_to_iter (priv->printer_list_filter,
                                                                &filter_iter, &iter);

              selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->printer_treeview));
              priv->internal_printer_change = TRUE;
              ctk_tree_selection_select_iter (selection, &filter_iter);
              priv->internal_printer_change = FALSE;
              g_free (priv->waiting_for_printer);
              priv->waiting_for_printer = NULL;

              g_object_unref (printer);
              return TRUE;
            }

          g_object_unref (printer);

        } while (ctk_tree_model_iter_next (model, &iter));
    }

  return FALSE;
}

/**
 * ctk_print_unix_dialog_set_settings:
 * @dialog: a #CtkPrintUnixDialog
 * @settings: (allow-none): a #CtkPrintSettings, or %NULL
 *
 * Sets the #CtkPrintSettings for the #CtkPrintUnixDialog. Typically,
 * this is used to restore saved print settings from a previous print
 * operation before the print dialog is shown.
 *
 * Since: 2.10
 **/
void
ctk_print_unix_dialog_set_settings (CtkPrintUnixDialog *dialog,
                                    CtkPrintSettings   *settings)
{
  CtkPrintUnixDialogPrivate *priv;
  const gchar *printer;
  CtkPageRange *ranges;
  gint num_ranges;

  g_return_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog));
  g_return_if_fail (settings == NULL || CTK_IS_PRINT_SETTINGS (settings));

  priv = dialog->priv;

  if (settings != NULL)
    {
      dialog_set_collate (dialog, ctk_print_settings_get_collate (settings));
      dialog_set_reverse (dialog, ctk_print_settings_get_reverse (settings));
      dialog_set_n_copies (dialog, ctk_print_settings_get_n_copies (settings));
      dialog_set_scale (dialog, ctk_print_settings_get_scale (settings));
      dialog_set_page_set (dialog, ctk_print_settings_get_page_set (settings));
      dialog_set_print_pages (dialog, ctk_print_settings_get_print_pages (settings));
      ranges = ctk_print_settings_get_page_ranges (settings, &num_ranges);
      if (ranges)
        {
          dialog_set_page_ranges (dialog, ranges, num_ranges);
          g_free (ranges);
        }

      priv->format_for_printer =
        g_strdup (ctk_print_settings_get (settings, "format-for-printer"));
    }

  if (priv->initial_settings)
    g_object_unref (priv->initial_settings);

  priv->initial_settings = settings;

  g_free (priv->waiting_for_printer);
  priv->waiting_for_printer = NULL;

  if (settings)
    {
      g_object_ref (settings);

      printer = ctk_print_settings_get_printer (settings);

      if (printer && !set_active_printer (dialog, printer))
        priv->waiting_for_printer = g_strdup (printer);
    }

  g_object_notify (G_OBJECT (dialog), "print-settings");
}

/**
 * ctk_print_unix_dialog_get_settings:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets a new #CtkPrintSettings object that represents the
 * current values in the print dialog. Note that this creates a
 * new object, and you need to unref it
 * if don’t want to keep it.
 *
 * Returns: a new #CtkPrintSettings object with the values from @dialog
 *
 * Since: 2.10
 */
CtkPrintSettings *
ctk_print_unix_dialog_get_settings (CtkPrintUnixDialog *dialog)
{
  CtkPrintUnixDialogPrivate *priv;
  CtkPrintSettings *settings;
  CtkPrintPages print_pages;
  CtkPageRange *ranges;
  gint n_ranges;

  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  priv = dialog->priv;
  settings = ctk_print_settings_new ();

  if (priv->current_printer)
    ctk_print_settings_set_printer (settings,
                                    ctk_printer_get_name (priv->current_printer));
  else
    ctk_print_settings_set_printer (settings, "default");

  ctk_print_settings_set (settings, "format-for-printer",
                          priv->format_for_printer);

  ctk_print_settings_set_collate (settings,
                                  dialog_get_collate (dialog));

  ctk_print_settings_set_reverse (settings,
                                  dialog_get_reverse (dialog));

  ctk_print_settings_set_n_copies (settings,
                                   dialog_get_n_copies (dialog));

  ctk_print_settings_set_scale (settings,
                                dialog_get_scale (dialog));

  ctk_print_settings_set_page_set (settings,
                                   dialog_get_page_set (dialog));

  print_pages = dialog_get_print_pages (dialog);
  ctk_print_settings_set_print_pages (settings, print_pages);

  ranges = dialog_get_page_ranges (dialog, &n_ranges);
  if (ranges)
    {
      ctk_print_settings_set_page_ranges (settings, ranges, n_ranges);
      g_free (ranges);
    }

  /* TODO: print when. How to handle? */

  if (priv->current_printer)
    _ctk_printer_get_settings_from_options (priv->current_printer,
                                            priv->options,
                                            settings);

  return settings;
}

/**
 * ctk_print_unix_dialog_add_custom_tab:
 * @dialog: a #CtkPrintUnixDialog
 * @child: the widget to put in the custom tab
 * @tab_label: the widget to use as tab label
 *
 * Adds a custom tab to the print dialog.
 *
 * Since: 2.10
 */
void
ctk_print_unix_dialog_add_custom_tab (CtkPrintUnixDialog *dialog,
                                      CtkWidget          *child,
                                      CtkWidget          *tab_label)
{
  ctk_notebook_insert_page (CTK_NOTEBOOK (dialog->priv->notebook),
                            child, tab_label, 2);
  ctk_widget_show (child);
  ctk_widget_show (tab_label);
}

/**
 * ctk_print_unix_dialog_set_manual_capabilities:
 * @dialog: a #CtkPrintUnixDialog
 * @capabilities: the printing capabilities of your application
 *
 * This lets you specify the printing capabilities your application
 * supports. For instance, if you can handle scaling the output then
 * you pass #CTK_PRINT_CAPABILITY_SCALE. If you don’t pass that, then
 * the dialog will only let you select the scale if the printing
 * system automatically handles scaling.
 *
 * Since: 2.10
 */
void
ctk_print_unix_dialog_set_manual_capabilities (CtkPrintUnixDialog   *dialog,
                                               CtkPrintCapabilities  capabilities)
{
  CtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->manual_capabilities != capabilities)
    {
      priv->manual_capabilities = capabilities;
      update_dialog_from_capabilities (dialog);

      if (priv->current_printer)
        {
          CtkTreeSelection *selection;

          selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->printer_treeview));
          g_clear_object (&priv->current_printer);
          priv->internal_printer_change = TRUE;
          selected_printer_changed (selection, dialog);
          priv->internal_printer_change = FALSE;
       }

      g_object_notify (G_OBJECT (dialog), "manual-capabilities");
    }
}

/**
 * ctk_print_unix_dialog_get_manual_capabilities:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the value of #CtkPrintUnixDialog:manual-capabilities property.
 *
 * Returns: the printing capabilities
 *
 * Since: 2.18
 */
CtkPrintCapabilities
ctk_print_unix_dialog_get_manual_capabilities (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->manual_capabilities;
}

/**
 * ctk_print_unix_dialog_set_support_selection:
 * @dialog: a #CtkPrintUnixDialog
 * @support_selection: %TRUE to allow print selection
 *
 * Sets whether the print dialog allows user to print a selection.
 *
 * Since: 2.18
 */
void
ctk_print_unix_dialog_set_support_selection (CtkPrintUnixDialog *dialog,
                                             gboolean            support_selection)
{
  CtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  support_selection = support_selection != FALSE;
  if (priv->support_selection != support_selection)
    {
      priv->support_selection = support_selection;

      if (priv->selection_radio)
        {
          if (support_selection)
            {
              ctk_widget_set_sensitive (priv->selection_radio, priv->has_selection);
              ctk_widget_show (priv->selection_radio);
            }
          else
            {
              ctk_widget_set_sensitive (priv->selection_radio, FALSE);
              ctk_widget_hide (priv->selection_radio);
            }
        }

      g_object_notify (G_OBJECT (dialog), "support-selection");
    }
}

/**
 * ctk_print_unix_dialog_get_support_selection:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the value of #CtkPrintUnixDialog:support-selection property.
 *
 * Returns: whether the application supports print of selection
 *
 * Since: 2.18
 */
gboolean
ctk_print_unix_dialog_get_support_selection (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->support_selection;
}

/**
 * ctk_print_unix_dialog_set_has_selection:
 * @dialog: a #CtkPrintUnixDialog
 * @has_selection: %TRUE indicates that a selection exists
 *
 * Sets whether a selection exists.
 *
 * Since: 2.18
 */
void
ctk_print_unix_dialog_set_has_selection (CtkPrintUnixDialog *dialog,
                                         gboolean            has_selection)
{
  CtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  has_selection = has_selection != FALSE;
  if (priv->has_selection != has_selection)
    {
      priv->has_selection = has_selection;

      if (priv->selection_radio)
        {
          if (priv->support_selection)
            ctk_widget_set_sensitive (priv->selection_radio, has_selection);
          else
            ctk_widget_set_sensitive (priv->selection_radio, FALSE);
        }

      g_object_notify (G_OBJECT (dialog), "has-selection");
    }
}

/**
 * ctk_print_unix_dialog_get_has_selection:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the value of #CtkPrintUnixDialog:has-selection property.
 *
 * Returns: whether there is a selection
 *
 * Since: 2.18
 */
gboolean
ctk_print_unix_dialog_get_has_selection (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->has_selection;
}

/**
 * ctk_print_unix_dialog_set_embed_page_setup
 * @dialog: a #CtkPrintUnixDialog
 * @embed: embed page setup selection
 *
 * Embed page size combo box and orientation combo box into page setup page.
 *
 * Since: 2.18
 */
void
ctk_print_unix_dialog_set_embed_page_setup (CtkPrintUnixDialog *dialog,
                                            gboolean            embed)
{
  CtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  embed = embed != FALSE;
  if (priv->embed_page_setup != embed)
    {
      priv->embed_page_setup = embed;

      ctk_widget_set_sensitive (priv->paper_size_combo, priv->embed_page_setup);
      ctk_widget_set_sensitive (priv->orientation_combo, priv->embed_page_setup);

      if (priv->embed_page_setup)
        {
          if (priv->paper_size_combo != NULL)
            g_signal_connect (priv->paper_size_combo, "changed", G_CALLBACK (paper_size_changed), dialog);

          if (priv->orientation_combo)
            g_signal_connect (priv->orientation_combo, "changed", G_CALLBACK (orientation_changed), dialog);
        }
      else
        {
          if (priv->paper_size_combo != NULL)
            g_signal_handlers_disconnect_by_func (priv->paper_size_combo, G_CALLBACK (paper_size_changed), dialog);

          if (priv->orientation_combo)
            g_signal_handlers_disconnect_by_func (priv->orientation_combo, G_CALLBACK (orientation_changed), dialog);
        }

      priv->internal_page_setup_change = TRUE;
      update_paper_sizes (dialog);
      priv->internal_page_setup_change = FALSE;
    }
}

/**
 * ctk_print_unix_dialog_get_embed_page_setup:
 * @dialog: a #CtkPrintUnixDialog
 *
 * Gets the value of #CtkPrintUnixDialog:embed-page-setup property.
 *
 * Returns: whether there is a selection
 *
 * Since: 2.18
 */
gboolean
ctk_print_unix_dialog_get_embed_page_setup (CtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->embed_page_setup;
}
