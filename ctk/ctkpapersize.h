/* CTK - The GIMP Toolkit
 * ctkpapersize.h: Paper Size
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

#ifndef __CTK_PAPER_SIZE_H__
#define __CTK_PAPER_SIZE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctkenums.h>


G_BEGIN_DECLS

typedef struct _CtkPaperSize CtkPaperSize;

#define CTK_TYPE_PAPER_SIZE    (ctk_paper_size_get_type ())

/* Common names, from PWG 5101.1-2002 PWG: Standard for Media Standardized Names */
/**
 * CTK_PAPER_NAME_A3:
 *
 * Name for the A3 paper size.
 */
#define CTK_PAPER_NAME_A3 "iso_a3"

/**
 * CTK_PAPER_NAME_A4:
 *
 * Name for the A4 paper size.
 */
#define CTK_PAPER_NAME_A4 "iso_a4"

/**
 * CTK_PAPER_NAME_A5:
 *
 * Name for the A5 paper size.
 */
#define CTK_PAPER_NAME_A5 "iso_a5"

/**
 * CTK_PAPER_NAME_B5:
 *
 * Name for the B5 paper size.
 */
#define CTK_PAPER_NAME_B5 "iso_b5"

/**
 * CTK_PAPER_NAME_LETTER:
 *
 * Name for the Letter paper size.
 */
#define CTK_PAPER_NAME_LETTER "na_letter"

/**
 * CTK_PAPER_NAME_EXECUTIVE:
 *
 * Name for the Executive paper size.
 */
#define CTK_PAPER_NAME_EXECUTIVE "na_executive"

/**
 * CTK_PAPER_NAME_LEGAL:
 *
 * Name for the Legal paper size.
 */
#define CTK_PAPER_NAME_LEGAL "na_legal"

GDK_AVAILABLE_IN_ALL
GType ctk_paper_size_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkPaperSize *ctk_paper_size_new          (const gchar  *name);
GDK_AVAILABLE_IN_ALL
CtkPaperSize *ctk_paper_size_new_from_ppd (const gchar  *ppd_name,
					   const gchar  *ppd_display_name,
					   gdouble       width,
					   gdouble       height);
GDK_AVAILABLE_IN_ALL
CtkPaperSize *ctk_paper_size_new_from_ipp (const gchar  *ipp_name,
					   gdouble       width,
					   gdouble       height);
GDK_AVAILABLE_IN_ALL
CtkPaperSize *ctk_paper_size_new_custom   (const gchar  *name,
					   const gchar  *display_name,
					   gdouble       width,
					   gdouble       height,
					   CtkUnit       unit);
GDK_AVAILABLE_IN_ALL
CtkPaperSize *ctk_paper_size_copy         (CtkPaperSize *other);
GDK_AVAILABLE_IN_ALL
void          ctk_paper_size_free         (CtkPaperSize *size);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_paper_size_is_equal     (CtkPaperSize *size1,
					   CtkPaperSize *size2);

GDK_AVAILABLE_IN_ALL
GList        *ctk_paper_size_get_paper_sizes (gboolean include_custom);

/* The width is always the shortest side, measure in mm */
GDK_AVAILABLE_IN_ALL
const gchar *ctk_paper_size_get_name         (CtkPaperSize *size);
GDK_AVAILABLE_IN_ALL
const gchar *ctk_paper_size_get_display_name (CtkPaperSize *size);
GDK_AVAILABLE_IN_ALL
const gchar *ctk_paper_size_get_ppd_name     (CtkPaperSize *size);

GDK_AVAILABLE_IN_ALL
gdouble  ctk_paper_size_get_width        (CtkPaperSize *size, CtkUnit unit);
GDK_AVAILABLE_IN_ALL
gdouble  ctk_paper_size_get_height       (CtkPaperSize *size, CtkUnit unit);
GDK_AVAILABLE_IN_ALL
gboolean ctk_paper_size_is_custom        (CtkPaperSize *size);
GDK_AVAILABLE_IN_ALL
gboolean ctk_paper_size_is_ipp           (CtkPaperSize *size);

/* Only for custom sizes: */
GDK_AVAILABLE_IN_ALL
void    ctk_paper_size_set_size                  (CtkPaperSize *size, 
                                                  gdouble       width, 
                                                  gdouble       height, 
                                                  CtkUnit       unit);

GDK_AVAILABLE_IN_ALL
gdouble ctk_paper_size_get_default_top_margin    (CtkPaperSize *size,
						  CtkUnit       unit);
GDK_AVAILABLE_IN_ALL
gdouble ctk_paper_size_get_default_bottom_margin (CtkPaperSize *size,
						  CtkUnit       unit);
GDK_AVAILABLE_IN_ALL
gdouble ctk_paper_size_get_default_left_margin   (CtkPaperSize *size,
						  CtkUnit       unit);
GDK_AVAILABLE_IN_ALL
gdouble ctk_paper_size_get_default_right_margin  (CtkPaperSize *size,
						  CtkUnit       unit);

GDK_AVAILABLE_IN_ALL
const gchar *ctk_paper_size_get_default (void);

GDK_AVAILABLE_IN_ALL
CtkPaperSize *ctk_paper_size_new_from_key_file (GKeyFile    *key_file,
					        const gchar *group_name,
					        GError     **error);
GDK_AVAILABLE_IN_ALL
void     ctk_paper_size_to_key_file            (CtkPaperSize *size,
					        GKeyFile     *key_file,
					        const gchar  *group_name);

GDK_AVAILABLE_IN_3_22
CtkPaperSize *ctk_paper_size_new_from_gvariant (GVariant     *variant);
GDK_AVAILABLE_IN_3_22
GVariant     *ctk_paper_size_to_gvariant       (CtkPaperSize *paper_size);

G_END_DECLS

#endif /* __CTK_PAPER_SIZE_H__ */
