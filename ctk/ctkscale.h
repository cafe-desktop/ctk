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

#ifndef __CTK_SCALE_H__
#define __CTK_SCALE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkrange.h>


G_BEGIN_DECLS

#define CTK_TYPE_SCALE            (ctk_scale_get_type ())
#define CTK_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SCALE, CtkScale))
#define CTK_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SCALE, CtkScaleClass))
#define CTK_IS_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SCALE))
#define CTK_IS_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SCALE))
#define CTK_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SCALE, CtkScaleClass))


typedef struct _CtkScale              CtkScale;
typedef struct _CtkScalePrivate       CtkScalePrivate;
typedef struct _CtkScaleClass         CtkScaleClass;

struct _CtkScale
{
  CtkRange range;

  /*< private >*/
  CtkScalePrivate *priv;
};

struct _CtkScaleClass
{
  CtkRangeClass parent_class;

  gchar* (* format_value) (CtkScale *scale,
                           gdouble   value);

  void (* draw_value) (CtkScale *scale);

  void (* get_layout_offsets) (CtkScale *scale,
                               gint     *x,
                               gint     *y);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType             ctk_scale_get_type           (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget       * ctk_scale_new                (CtkOrientation   orientation,
                                                CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
CtkWidget       * ctk_scale_new_with_range     (CtkOrientation   orientation,
                                                gdouble          min,
                                                gdouble          max,
                                                gdouble          step);
GDK_AVAILABLE_IN_ALL
void              ctk_scale_set_digits         (CtkScale        *scale,
                                                gint             digits);
GDK_AVAILABLE_IN_ALL
gint              ctk_scale_get_digits         (CtkScale        *scale);
GDK_AVAILABLE_IN_ALL
void              ctk_scale_set_draw_value     (CtkScale        *scale,
                                                gboolean         draw_value);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_scale_get_draw_value     (CtkScale        *scale);
GDK_AVAILABLE_IN_3_4
void              ctk_scale_set_has_origin     (CtkScale        *scale,
                                                gboolean         has_origin);
GDK_AVAILABLE_IN_3_4
gboolean          ctk_scale_get_has_origin     (CtkScale        *scale);
GDK_AVAILABLE_IN_ALL
void              ctk_scale_set_value_pos      (CtkScale        *scale,
                                                CtkPositionType  pos);
GDK_AVAILABLE_IN_ALL
CtkPositionType   ctk_scale_get_value_pos      (CtkScale        *scale);

GDK_AVAILABLE_IN_ALL
PangoLayout     * ctk_scale_get_layout         (CtkScale        *scale);
GDK_AVAILABLE_IN_ALL
void              ctk_scale_get_layout_offsets (CtkScale        *scale,
                                                gint            *x,
                                                gint            *y);

GDK_AVAILABLE_IN_ALL
void              ctk_scale_add_mark           (CtkScale        *scale,
                                                gdouble          value,
                                                CtkPositionType  position,
                                                const gchar     *markup);
GDK_AVAILABLE_IN_ALL
void              ctk_scale_clear_marks        (CtkScale        *scale);


G_END_DECLS

#endif /* __CTK_SCALE_H__ */
