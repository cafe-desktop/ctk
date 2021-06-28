/* CTK - The GIMP Toolkit
 * ctkpagesetup.h: Page Setup
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

#ifndef __CTK_PAGE_SETUP_H__
#define __CTK_PAGE_SETUP_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkpapersize.h>


G_BEGIN_DECLS

typedef struct _CtkPageSetup CtkPageSetup;

#define CTK_TYPE_PAGE_SETUP    (ctk_page_setup_get_type ())
#define CTK_PAGE_SETUP(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PAGE_SETUP, CtkPageSetup))
#define CTK_IS_PAGE_SETUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PAGE_SETUP))

CDK_AVAILABLE_IN_ALL
GType              ctk_page_setup_get_type          (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkPageSetup *     ctk_page_setup_new               (void);
CDK_AVAILABLE_IN_ALL
CtkPageSetup *     ctk_page_setup_copy              (CtkPageSetup       *other);
CDK_AVAILABLE_IN_ALL
CtkPageOrientation ctk_page_setup_get_orientation   (CtkPageSetup       *setup);
CDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_orientation   (CtkPageSetup       *setup,
						     CtkPageOrientation  orientation);
CDK_AVAILABLE_IN_ALL
CtkPaperSize *     ctk_page_setup_get_paper_size    (CtkPageSetup       *setup);
CDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_paper_size    (CtkPageSetup       *setup,
						     CtkPaperSize       *size);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_top_margin    (CtkPageSetup       *setup,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_top_margin    (CtkPageSetup       *setup,
						     gdouble             margin,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_bottom_margin (CtkPageSetup       *setup,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_bottom_margin (CtkPageSetup       *setup,
						     gdouble             margin,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_left_margin   (CtkPageSetup       *setup,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_left_margin   (CtkPageSetup       *setup,
						     gdouble             margin,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_right_margin  (CtkPageSetup       *setup,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_right_margin  (CtkPageSetup       *setup,
						     gdouble             margin,
						     CtkUnit             unit);

CDK_AVAILABLE_IN_ALL
void ctk_page_setup_set_paper_size_and_default_margins (CtkPageSetup    *setup,
							CtkPaperSize    *size);

/* These take orientation, but not margins into consideration */
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_paper_width   (CtkPageSetup       *setup,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_paper_height  (CtkPageSetup       *setup,
						     CtkUnit             unit);


/* These take orientation, and margins into consideration */
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_page_width    (CtkPageSetup       *setup,
						     CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_page_height   (CtkPageSetup       *setup,
						     CtkUnit             unit);

/* Saving and restoring page setup */
CDK_AVAILABLE_IN_ALL
CtkPageSetup	  *ctk_page_setup_new_from_file	    (const gchar         *file_name,
						     GError              **error);
CDK_AVAILABLE_IN_ALL
gboolean	   ctk_page_setup_load_file	    (CtkPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
CDK_AVAILABLE_IN_ALL
gboolean	   ctk_page_setup_to_file	    (CtkPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
CDK_AVAILABLE_IN_ALL
CtkPageSetup	  *ctk_page_setup_new_from_key_file (GKeyFile            *key_file,
						     const gchar         *group_name,
						     GError             **error);
CDK_AVAILABLE_IN_ALL
gboolean           ctk_page_setup_load_key_file     (CtkPageSetup        *setup,
				                     GKeyFile            *key_file,
				                     const gchar         *group_name,
				                     GError             **error);
CDK_AVAILABLE_IN_ALL
void		   ctk_page_setup_to_key_file	    (CtkPageSetup        *setup,
						     GKeyFile            *key_file,
						     const gchar         *group_name);

CDK_AVAILABLE_IN_3_22
GVariant          *ctk_page_setup_to_gvariant       (CtkPageSetup        *setup);
CDK_AVAILABLE_IN_3_22
CtkPageSetup      *ctk_page_setup_new_from_gvariant (GVariant            *variant);

G_END_DECLS

#endif /* __CTK_PAGE_SETUP_H__ */
