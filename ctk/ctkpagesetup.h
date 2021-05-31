/* GTK - The GIMP Toolkit
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

typedef struct _GtkPageSetup GtkPageSetup;

#define CTK_TYPE_PAGE_SETUP    (ctk_page_setup_get_type ())
#define CTK_PAGE_SETUP(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PAGE_SETUP, GtkPageSetup))
#define CTK_IS_PAGE_SETUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PAGE_SETUP))

GDK_AVAILABLE_IN_ALL
GType              ctk_page_setup_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkPageSetup *     ctk_page_setup_new               (void);
GDK_AVAILABLE_IN_ALL
GtkPageSetup *     ctk_page_setup_copy              (GtkPageSetup       *other);
GDK_AVAILABLE_IN_ALL
GtkPageOrientation ctk_page_setup_get_orientation   (GtkPageSetup       *setup);
GDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_orientation   (GtkPageSetup       *setup,
						     GtkPageOrientation  orientation);
GDK_AVAILABLE_IN_ALL
GtkPaperSize *     ctk_page_setup_get_paper_size    (GtkPageSetup       *setup);
GDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_paper_size    (GtkPageSetup       *setup,
						     GtkPaperSize       *size);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_top_margin    (GtkPageSetup       *setup,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_top_margin    (GtkPageSetup       *setup,
						     gdouble             margin,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_bottom_margin (GtkPageSetup       *setup,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_bottom_margin (GtkPageSetup       *setup,
						     gdouble             margin,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_left_margin   (GtkPageSetup       *setup,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_left_margin   (GtkPageSetup       *setup,
						     gdouble             margin,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_right_margin  (GtkPageSetup       *setup,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
void               ctk_page_setup_set_right_margin  (GtkPageSetup       *setup,
						     gdouble             margin,
						     GtkUnit             unit);

GDK_AVAILABLE_IN_ALL
void ctk_page_setup_set_paper_size_and_default_margins (GtkPageSetup    *setup,
							GtkPaperSize    *size);

/* These take orientation, but not margins into consideration */
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_paper_width   (GtkPageSetup       *setup,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_paper_height  (GtkPageSetup       *setup,
						     GtkUnit             unit);


/* These take orientation, and margins into consideration */
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_page_width    (GtkPageSetup       *setup,
						     GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_page_setup_get_page_height   (GtkPageSetup       *setup,
						     GtkUnit             unit);

/* Saving and restoring page setup */
GDK_AVAILABLE_IN_ALL
GtkPageSetup	  *ctk_page_setup_new_from_file	    (const gchar         *file_name,
						     GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean	   ctk_page_setup_load_file	    (GtkPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
GDK_AVAILABLE_IN_ALL
gboolean	   ctk_page_setup_to_file	    (GtkPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
GDK_AVAILABLE_IN_ALL
GtkPageSetup	  *ctk_page_setup_new_from_key_file (GKeyFile            *key_file,
						     const gchar         *group_name,
						     GError             **error);
GDK_AVAILABLE_IN_ALL
gboolean           ctk_page_setup_load_key_file     (GtkPageSetup        *setup,
				                     GKeyFile            *key_file,
				                     const gchar         *group_name,
				                     GError             **error);
GDK_AVAILABLE_IN_ALL
void		   ctk_page_setup_to_key_file	    (GtkPageSetup        *setup,
						     GKeyFile            *key_file,
						     const gchar         *group_name);

GDK_AVAILABLE_IN_3_22
GVariant          *ctk_page_setup_to_gvariant       (GtkPageSetup        *setup);
GDK_AVAILABLE_IN_3_22
GtkPageSetup      *ctk_page_setup_new_from_gvariant (GVariant            *variant);

G_END_DECLS

#endif /* __CTK_PAGE_SETUP_H__ */
