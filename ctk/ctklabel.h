/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_LABEL_H__
#define __CTK_LABEL_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/deprecated/ctkmisc.h>
#include <ctk/ctkwindow.h>
#include <ctk/ctkmenu.h>

G_BEGIN_DECLS

#define CTK_TYPE_LABEL		  (ctk_label_get_type ())
#define CTK_LABEL(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LABEL, GtkLabel))
#define CTK_LABEL_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LABEL, GtkLabelClass))
#define CTK_IS_LABEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LABEL))
#define CTK_IS_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LABEL))
#define CTK_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LABEL, GtkLabelClass))


typedef struct _GtkLabel              GtkLabel;
typedef struct _GtkLabelPrivate       GtkLabelPrivate;
typedef struct _GtkLabelClass         GtkLabelClass;

typedef struct _GtkLabelSelectionInfo GtkLabelSelectionInfo;

struct _GtkLabel
{
  GtkMisc misc;

  /*< private >*/
  GtkLabelPrivate *priv;
};

struct _GtkLabelClass
{
  GtkMiscClass parent_class;

  void (* move_cursor)     (GtkLabel       *label,
			    GtkMovementStep step,
			    gint            count,
			    gboolean        extend_selection);
  void (* copy_clipboard)  (GtkLabel       *label);

  /* Hook to customize right-click popup for selectable labels */
  void (* populate_popup)   (GtkLabel       *label,
                             GtkMenu        *menu);

  gboolean (*activate_link) (GtkLabel       *label,
                             const gchar    *uri);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};

GDK_AVAILABLE_IN_ALL
GType                 ctk_label_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget*            ctk_label_new               (const gchar   *str);
GDK_AVAILABLE_IN_ALL
GtkWidget*            ctk_label_new_with_mnemonic (const gchar   *str);
GDK_AVAILABLE_IN_ALL
void                  ctk_label_set_text          (GtkLabel      *label,
						   const gchar   *str);
GDK_AVAILABLE_IN_ALL
const gchar*          ctk_label_get_text          (GtkLabel      *label);
GDK_AVAILABLE_IN_ALL
void                  ctk_label_set_attributes    (GtkLabel      *label,
						   PangoAttrList *attrs);
GDK_AVAILABLE_IN_ALL
PangoAttrList        *ctk_label_get_attributes    (GtkLabel      *label);
GDK_AVAILABLE_IN_ALL
void                  ctk_label_set_label         (GtkLabel      *label,
						   const gchar   *str);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_label_get_label         (GtkLabel      *label);
GDK_AVAILABLE_IN_ALL
void                  ctk_label_set_markup        (GtkLabel      *label,
						   const gchar   *str);
GDK_AVAILABLE_IN_ALL
void                  ctk_label_set_use_markup    (GtkLabel      *label,
						   gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_label_get_use_markup    (GtkLabel      *label);
GDK_AVAILABLE_IN_ALL
void                  ctk_label_set_use_underline (GtkLabel      *label,
						   gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_label_get_use_underline (GtkLabel      *label);

GDK_AVAILABLE_IN_ALL
void     ctk_label_set_markup_with_mnemonic       (GtkLabel         *label,
						   const gchar      *str);
GDK_AVAILABLE_IN_ALL
guint    ctk_label_get_mnemonic_keyval            (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_mnemonic_widget            (GtkLabel         *label,
						   GtkWidget        *widget);
GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_label_get_mnemonic_widget          (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_text_with_mnemonic         (GtkLabel         *label,
						   const gchar      *str);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_justify                    (GtkLabel         *label,
						   GtkJustification  jtype);
GDK_AVAILABLE_IN_ALL
GtkJustification ctk_label_get_justify            (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_ellipsize		  (GtkLabel         *label,
						   PangoEllipsizeMode mode);
GDK_AVAILABLE_IN_ALL
PangoEllipsizeMode ctk_label_get_ellipsize        (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_width_chars		  (GtkLabel         *label,
						   gint              n_chars);
GDK_AVAILABLE_IN_ALL
gint     ctk_label_get_width_chars                (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_max_width_chars    	  (GtkLabel         *label,
					  	   gint              n_chars);
GDK_AVAILABLE_IN_ALL
gint     ctk_label_get_max_width_chars  	  (GtkLabel         *label);
GDK_AVAILABLE_IN_3_10
void     ctk_label_set_lines                      (GtkLabel         *label,
                                                   gint              lines);
GDK_AVAILABLE_IN_3_10
gint     ctk_label_get_lines                      (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_pattern                    (GtkLabel         *label,
						   const gchar      *pattern);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_line_wrap                  (GtkLabel         *label,
						   gboolean          wrap);
GDK_AVAILABLE_IN_ALL
gboolean ctk_label_get_line_wrap                  (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_line_wrap_mode             (GtkLabel         *label,
						   PangoWrapMode     wrap_mode);
GDK_AVAILABLE_IN_ALL
PangoWrapMode ctk_label_get_line_wrap_mode        (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_selectable                 (GtkLabel         *label,
						   gboolean          setting);
GDK_AVAILABLE_IN_ALL
gboolean ctk_label_get_selectable                 (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_set_angle                      (GtkLabel         *label,
						   gdouble           angle);
GDK_AVAILABLE_IN_ALL
gdouble  ctk_label_get_angle                      (GtkLabel         *label);
GDK_AVAILABLE_IN_ALL
void     ctk_label_select_region                  (GtkLabel         *label,
						   gint              start_offset,
						   gint              end_offset);
GDK_AVAILABLE_IN_ALL
gboolean ctk_label_get_selection_bounds           (GtkLabel         *label,
                                                   gint             *start,
                                                   gint             *end);

GDK_AVAILABLE_IN_ALL
PangoLayout *ctk_label_get_layout         (GtkLabel *label);
GDK_AVAILABLE_IN_ALL
void         ctk_label_get_layout_offsets (GtkLabel *label,
                                           gint     *x,
                                           gint     *y);

GDK_AVAILABLE_IN_ALL
void         ctk_label_set_single_line_mode  (GtkLabel *label,
                                              gboolean single_line_mode);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_label_get_single_line_mode  (GtkLabel *label);

GDK_AVAILABLE_IN_ALL
const gchar *ctk_label_get_current_uri (GtkLabel *label);
GDK_AVAILABLE_IN_ALL
void         ctk_label_set_track_visited_links  (GtkLabel *label,
                                                 gboolean  track_links);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_label_get_track_visited_links  (GtkLabel *label);

GDK_AVAILABLE_IN_3_16
void         ctk_label_set_xalign (GtkLabel *label,
                                   gfloat    xalign);

GDK_AVAILABLE_IN_3_16
gfloat       ctk_label_get_xalign (GtkLabel *label);

GDK_AVAILABLE_IN_3_16
void         ctk_label_set_yalign (GtkLabel *label,
                                   gfloat    yalign);

GDK_AVAILABLE_IN_3_16
gfloat       ctk_label_get_yalign (GtkLabel *label);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkLabel, g_object_unref)

G_END_DECLS

#endif /* __CTK_LABEL_H__ */
