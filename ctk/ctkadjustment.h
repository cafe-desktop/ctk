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

#ifndef __CTK_ADJUSTMENT_H__
#define __CTK_ADJUSTMENT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctktypes.h>

G_BEGIN_DECLS

#define CTK_TYPE_ADJUSTMENT                  (ctk_adjustment_get_type ())
#define CTK_ADJUSTMENT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ADJUSTMENT, CtkAdjustment))
#define CTK_ADJUSTMENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ADJUSTMENT, CtkAdjustmentClass))
#define CTK_IS_ADJUSTMENT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ADJUSTMENT))
#define CTK_IS_ADJUSTMENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ADJUSTMENT))
#define CTK_ADJUSTMENT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ADJUSTMENT, CtkAdjustmentClass))


typedef struct _CtkAdjustmentPrivate  CtkAdjustmentPrivate;
typedef struct _CtkAdjustmentClass    CtkAdjustmentClass;

/**
 * CtkAdjustment:
 *
 * The #CtkAdjustment-struct contains only private fields and
 * should not be directly accessed.
 */
struct _CtkAdjustment
{
  GInitiallyUnowned parent_instance;

  CtkAdjustmentPrivate *priv;
};

struct _CtkAdjustmentClass
{
  GInitiallyUnownedClass parent_class;

  void (* changed)       (CtkAdjustment *adjustment);
  void (* value_changed) (CtkAdjustment *adjustment);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      ctk_adjustment_get_type              (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkAdjustment*   ctk_adjustment_new             (gdouble          value,
                                                 gdouble          lower,
                                                 gdouble          upper,
                                                 gdouble          step_increment,
                                                 gdouble          page_increment,
                                                 gdouble          page_size);

GDK_DEPRECATED_IN_3_18
void       ctk_adjustment_changed               (CtkAdjustment   *adjustment);
GDK_DEPRECATED_IN_3_18
void       ctk_adjustment_value_changed         (CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_clamp_page            (CtkAdjustment   *adjustment,
                                                 gdouble          lower,
                                                 gdouble          upper);

GDK_AVAILABLE_IN_ALL
gdouble    ctk_adjustment_get_value             (CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_set_value             (CtkAdjustment   *adjustment,
                                                 gdouble          value);
GDK_AVAILABLE_IN_ALL
gdouble    ctk_adjustment_get_lower             (CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_set_lower             (CtkAdjustment   *adjustment,
                                                 gdouble          lower);
GDK_AVAILABLE_IN_ALL
gdouble    ctk_adjustment_get_upper             (CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_set_upper             (CtkAdjustment   *adjustment,
                                                 gdouble          upper);
GDK_AVAILABLE_IN_ALL
gdouble    ctk_adjustment_get_step_increment    (CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_set_step_increment    (CtkAdjustment   *adjustment,
                                                 gdouble          step_increment);
GDK_AVAILABLE_IN_ALL
gdouble    ctk_adjustment_get_page_increment    (CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_set_page_increment    (CtkAdjustment   *adjustment,
                                                 gdouble          page_increment);
GDK_AVAILABLE_IN_ALL
gdouble    ctk_adjustment_get_page_size         (CtkAdjustment   *adjustment);
GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_set_page_size         (CtkAdjustment   *adjustment,
                                                 gdouble          page_size);

GDK_AVAILABLE_IN_ALL
void       ctk_adjustment_configure             (CtkAdjustment   *adjustment,
                                                 gdouble          value,
                                                 gdouble          lower,
                                                 gdouble          upper,
                                                 gdouble          step_increment,
                                                 gdouble          page_increment,
                                                 gdouble          page_size);
GDK_AVAILABLE_IN_3_2
gdouble    ctk_adjustment_get_minimum_increment (CtkAdjustment   *adjustment);

G_END_DECLS

#endif /* __CTK_ADJUSTMENT_H__ */
