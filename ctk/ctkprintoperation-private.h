/* GTK - The GIMP Toolkit
 * ctkprintoperation.h: Print Operation
 * Copyright (C) 2006, Red Hat, Inc.
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

#ifndef __CTK_PRINT_OPERATION_PRIVATE_H__
#define __CTK_PRINT_OPERATION_PRIVATE_H__

#include "ctkprintoperation.h"

G_BEGIN_DECLS

/* Page drawing states */
typedef enum
{
  CTK_PAGE_DRAWING_STATE_READY,
  CTK_PAGE_DRAWING_STATE_DRAWING,
  CTK_PAGE_DRAWING_STATE_DEFERRED_DRAWING
} CtkPageDrawingState;

struct _CtkPrintOperationPrivate
{
  CtkPrintOperationAction action;
  CtkPrintStatus status;
  GError *error;
  gchar *status_string;
  CtkPageSetup *default_page_setup;
  CtkPrintSettings *print_settings;
  gchar *job_name;
  gint nr_of_pages;
  gint nr_of_pages_to_print;
  gint page_position;
  gint current_page;
  CtkUnit unit;
  gchar *export_filename;
  guint use_full_page      : 1;
  guint track_print_status : 1;
  guint show_progress      : 1;
  guint cancelled          : 1;
  guint allow_async        : 1;
  guint is_sync            : 1;
  guint support_selection  : 1;
  guint has_selection      : 1;
  guint embed_page_setup   : 1;

  CtkPageDrawingState      page_drawing_state;

  guint print_pages_idle_id;
  guint show_progress_timeout_id;

  CtkPrintContext *print_context;
  
  CtkPrintPages print_pages;
  CtkPageRange *page_ranges;
  gint num_page_ranges;
  
  gint manual_num_copies;
  guint manual_collation   : 1;
  guint manual_reverse     : 1;
  guint manual_orientation : 1;
  double manual_scale;
  CtkPageSet manual_page_set;
  guint manual_number_up;
  CtkNumberUpLayout manual_number_up_layout;

  CtkWidget *custom_widget;
  gchar *custom_tab_label;
  
  gpointer platform_data;
  GDestroyNotify free_platform_data;

  GMainLoop *rloop; /* recursive mainloop */

  void (*start_page) (CtkPrintOperation *operation,
		      CtkPrintContext   *print_context,
		      CtkPageSetup      *page_setup);
  void (*end_page)   (CtkPrintOperation *operation,
		      CtkPrintContext   *print_context);
  void (*end_run)    (CtkPrintOperation *operation,
		      gboolean           wait,
		      gboolean           cancelled);
};


typedef void (* CtkPrintOperationPrintFunc) (CtkPrintOperation      *op,
					     CtkWindow              *parent,
					     gboolean                do_print,
					     CtkPrintOperationResult result);

CtkPrintOperationResult _ctk_print_operation_platform_backend_run_dialog             (CtkPrintOperation           *operation,
										      gboolean                     show_dialog,
										      CtkWindow                   *parent,
										      gboolean                    *do_print);
void                    _ctk_print_operation_platform_backend_run_dialog_async       (CtkPrintOperation           *op,
										      gboolean                     show_dialog,
										      CtkWindow                   *parent,
										      CtkPrintOperationPrintFunc   print_cb);
void                    _ctk_print_operation_platform_backend_launch_preview         (CtkPrintOperation           *op,
										      cairo_surface_t             *surface,
										      CtkWindow                   *parent,
										      const char                  *filename);
cairo_surface_t *       _ctk_print_operation_platform_backend_create_preview_surface (CtkPrintOperation           *op,
										      CtkPageSetup                *page_setup,
										      gdouble                     *dpi_x,
										      gdouble                     *dpi_y,
										      gchar                       **target);
void                    _ctk_print_operation_platform_backend_resize_preview_surface (CtkPrintOperation           *op,
										      CtkPageSetup                *page_setup,
										      cairo_surface_t             *surface);
void                    _ctk_print_operation_platform_backend_preview_start_page     (CtkPrintOperation *op,
										      cairo_surface_t *surface,
										      cairo_t *cr);
void                    _ctk_print_operation_platform_backend_preview_end_page       (CtkPrintOperation *op,
										      cairo_surface_t *surface,
										      cairo_t *cr);

void _ctk_print_operation_set_status (CtkPrintOperation *op,
				      CtkPrintStatus     status,
				      const gchar       *string);

/* CtkPrintContext private functions: */

CtkPrintContext *_ctk_print_context_new                             (CtkPrintOperation *op);
void             _ctk_print_context_set_page_setup                  (CtkPrintContext   *context,
								     CtkPageSetup      *page_setup);
void             _ctk_print_context_translate_into_margin           (CtkPrintContext   *context);
void             _ctk_print_context_rotate_according_to_orientation (CtkPrintContext   *context);
void             _ctk_print_context_reverse_according_to_orientation (CtkPrintContext *context);
void             _ctk_print_context_set_hard_margins                (CtkPrintContext   *context,
								     gdouble            top,
								     gdouble            bottom,
								     gdouble            left,
								     gdouble            right);

G_END_DECLS

#endif /* __CTK_PRINT_OPERATION_PRIVATE_H__ */
