/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_RANGE_H__
#define __CTK_RANGE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>


G_BEGIN_DECLS


#define CTK_TYPE_RANGE            (ctk_range_get_type ())
#define CTK_RANGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RANGE, CtkRange))
#define CTK_RANGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RANGE, CtkRangeClass))
#define CTK_IS_RANGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RANGE))
#define CTK_IS_RANGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RANGE))
#define CTK_RANGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RANGE, CtkRangeClass))

typedef struct _CtkRange              CtkRange;
typedef struct _CtkRangePrivate       CtkRangePrivate;
typedef struct _CtkRangeClass         CtkRangeClass;

struct _CtkRange
{
  CtkWidget widget;

  CtkRangePrivate *priv;
};

struct _CtkRangeClass
{
  CtkWidgetClass parent_class;

  /* what detail to pass to CTK drawing functions */
  G_GNUC_DEPRECATED gchar *slider_detail;
  G_GNUC_DEPRECATED gchar *stepper_detail;

  void (* value_changed)    (CtkRange     *range);
  void (* adjust_bounds)    (CtkRange     *range,
                             gdouble	   new_value);

  /* action signals for keybindings */
  void (* move_slider)      (CtkRange     *range,
                             CtkScrollType scroll);

  /* Virtual functions */
  void (* get_range_border) (CtkRange     *range,
                             CtkBorder    *border_);

  gboolean (* change_value) (CtkRange     *range,
                             CtkScrollType scroll,
                             gdouble       new_value);

   void (* get_range_size_request) (CtkRange       *range,
                                    CtkOrientation  orientation,
                                    gint           *minimum,
                                    gint           *natural);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
};


GDK_AVAILABLE_IN_ALL
GType              ctk_range_get_type                      (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void               ctk_range_set_adjustment                (CtkRange      *range,
                                                            CtkAdjustment *adjustment);
GDK_AVAILABLE_IN_ALL
CtkAdjustment*     ctk_range_get_adjustment                (CtkRange      *range);

GDK_AVAILABLE_IN_ALL
void               ctk_range_set_inverted                  (CtkRange      *range,
                                                            gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean           ctk_range_get_inverted                  (CtkRange      *range);

GDK_AVAILABLE_IN_ALL
void               ctk_range_set_flippable                 (CtkRange      *range,
                                                            gboolean       flippable);
GDK_AVAILABLE_IN_ALL
gboolean           ctk_range_get_flippable                 (CtkRange      *range);

GDK_AVAILABLE_IN_ALL
void               ctk_range_set_slider_size_fixed         (CtkRange      *range,
                                                            gboolean       size_fixed);
GDK_AVAILABLE_IN_ALL
gboolean           ctk_range_get_slider_size_fixed         (CtkRange      *range);

GDK_DEPRECATED_IN_3_20
void               ctk_range_set_min_slider_size           (CtkRange      *range,
                                                            gint           min_size);
GDK_DEPRECATED_IN_3_20
gint               ctk_range_get_min_slider_size           (CtkRange      *range);

GDK_AVAILABLE_IN_ALL
void               ctk_range_get_range_rect                (CtkRange      *range,
                                                            GdkRectangle  *range_rect);
GDK_AVAILABLE_IN_ALL
void               ctk_range_get_slider_range              (CtkRange      *range,
                                                            gint          *slider_start,
                                                            gint          *slider_end);

GDK_AVAILABLE_IN_ALL
void               ctk_range_set_lower_stepper_sensitivity (CtkRange      *range,
                                                            CtkSensitivityType sensitivity);
GDK_AVAILABLE_IN_ALL
CtkSensitivityType ctk_range_get_lower_stepper_sensitivity (CtkRange      *range);
GDK_AVAILABLE_IN_ALL
void               ctk_range_set_upper_stepper_sensitivity (CtkRange      *range,
                                                            CtkSensitivityType sensitivity);
GDK_AVAILABLE_IN_ALL
CtkSensitivityType ctk_range_get_upper_stepper_sensitivity (CtkRange      *range);

GDK_AVAILABLE_IN_ALL
void               ctk_range_set_increments                (CtkRange      *range,
                                                            gdouble        step,
                                                            gdouble        page);
GDK_AVAILABLE_IN_ALL
void               ctk_range_set_range                     (CtkRange      *range,
                                                            gdouble        min,
                                                            gdouble        max);
GDK_AVAILABLE_IN_ALL
void               ctk_range_set_value                     (CtkRange      *range,
                                                            gdouble        value);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_range_get_value                     (CtkRange      *range);

GDK_AVAILABLE_IN_ALL
void               ctk_range_set_show_fill_level           (CtkRange      *range,
                                                            gboolean       show_fill_level);
GDK_AVAILABLE_IN_ALL
gboolean           ctk_range_get_show_fill_level           (CtkRange      *range);
GDK_AVAILABLE_IN_ALL
void               ctk_range_set_restrict_to_fill_level    (CtkRange      *range,
                                                            gboolean       restrict_to_fill_level);
GDK_AVAILABLE_IN_ALL
gboolean           ctk_range_get_restrict_to_fill_level    (CtkRange      *range);
GDK_AVAILABLE_IN_ALL
void               ctk_range_set_fill_level                (CtkRange      *range,
                                                            gdouble        fill_level);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_range_get_fill_level                (CtkRange      *range);
GDK_AVAILABLE_IN_ALL
void               ctk_range_set_round_digits              (CtkRange      *range,
                                                            gint           round_digits);
GDK_AVAILABLE_IN_ALL
gint                ctk_range_get_round_digits              (CtkRange      *range);


G_END_DECLS


#endif /* __CTK_RANGE_H__ */
