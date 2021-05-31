/* CTK - The GIMP Toolkit
 * ctkprintoperationpreview.c: Abstract print preview interface
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

#include "config.h"

#include "ctkprintoperationpreview.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"


static void ctk_print_operation_preview_base_init (gpointer g_iface);

GType
ctk_print_operation_preview_get_type (void)
{
  static GType print_operation_preview_type = 0;

  if (!print_operation_preview_type)
    {
      const GTypeInfo print_operation_preview_info =
      {
        sizeof (CtkPrintOperationPreviewIface), /* class_size */
	ctk_print_operation_preview_base_init,   /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };

      print_operation_preview_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("CtkPrintOperationPreview"),
				&print_operation_preview_info, 0);

      g_type_interface_add_prerequisite (print_operation_preview_type, G_TYPE_OBJECT);
    }

  return print_operation_preview_type;
}

static void
ctk_print_operation_preview_base_init (gpointer g_iface)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      /**
       * CtkPrintOperationPreview::ready:
       * @preview: the object on which the signal is emitted
       * @context: the current #CtkPrintContext
       *
       * The ::ready signal gets emitted once per preview operation,
       * before the first page is rendered.
       * 
       * A handler for this signal can be used for setup tasks.
       */
      g_signal_new (I_("ready"),
		    CTK_TYPE_PRINT_OPERATION_PREVIEW,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (CtkPrintOperationPreviewIface, ready),
		    NULL, NULL,
		    NULL,
		    G_TYPE_NONE, 1,
		    CTK_TYPE_PRINT_CONTEXT);

      /**
       * CtkPrintOperationPreview::got-page-size:
       * @preview: the object on which the signal is emitted
       * @context: the current #CtkPrintContext
       * @page_setup: the #CtkPageSetup for the current page
       *
       * The ::got-page-size signal is emitted once for each page
       * that gets rendered to the preview. 
       *
       * A handler for this signal should update the @context
       * according to @page_setup and set up a suitable cairo
       * context, using ctk_print_context_set_cairo_context().
       */
      g_signal_new (I_("got-page-size"),
		    CTK_TYPE_PRINT_OPERATION_PREVIEW,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (CtkPrintOperationPreviewIface, got_page_size),
		    NULL, NULL,
		    _ctk_marshal_VOID__OBJECT_OBJECT,
		    G_TYPE_NONE, 2,
		    CTK_TYPE_PRINT_CONTEXT,
		    CTK_TYPE_PAGE_SETUP);

      initialized = TRUE;
    }
}

/**
 * ctk_print_operation_preview_render_page:
 * @preview: a #CtkPrintOperationPreview
 * @page_nr: the page to render
 *
 * Renders a page to the preview, using the print context that
 * was passed to the #CtkPrintOperation::preview handler together
 * with @preview.
 *
 * A custom iprint preview should use this function in its ::expose
 * handler to render the currently selected page.
 * 
 * Note that this function requires a suitable cairo context to 
 * be associated with the print context. 
 *
 * Since: 2.10 
 */
void    
ctk_print_operation_preview_render_page (CtkPrintOperationPreview *preview,
					 gint			   page_nr)
{
  g_return_if_fail (CTK_IS_PRINT_OPERATION_PREVIEW (preview));

  CTK_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->render_page (preview,
								page_nr);
}

/**
 * ctk_print_operation_preview_end_preview:
 * @preview: a #CtkPrintOperationPreview
 *
 * Ends a preview. 
 *
 * This function must be called to finish a custom print preview.
 *
 * Since: 2.10
 */
void
ctk_print_operation_preview_end_preview (CtkPrintOperationPreview *preview)
{
  g_return_if_fail (CTK_IS_PRINT_OPERATION_PREVIEW (preview));

  CTK_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->end_preview (preview);
}

/**
 * ctk_print_operation_preview_is_selected:
 * @preview: a #CtkPrintOperationPreview
 * @page_nr: a page number
 *
 * Returns whether the given page is included in the set of pages that
 * have been selected for printing.
 * 
 * Returns: %TRUE if the page has been selected for printing
 *
 * Since: 2.10
 */
gboolean
ctk_print_operation_preview_is_selected (CtkPrintOperationPreview *preview,
					 gint                      page_nr)
{
  g_return_val_if_fail (CTK_IS_PRINT_OPERATION_PREVIEW (preview), FALSE);

  return CTK_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->is_selected (preview, page_nr);
}
