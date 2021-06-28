/* HSV color selector for CTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for CTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for CTK+)
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

#ifndef __CTK_HSV_H__
#define __CTK_HSV_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_HSV            (ctk_hsv_get_type ())
#define CTK_HSV(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_HSV, CtkHSV))
#define CTK_HSV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_HSV, CtkHSVClass))
#define CTK_IS_HSV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_HSV))
#define CTK_IS_HSV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_HSV))
#define CTK_HSV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_HSV, CtkHSVClass))


typedef struct _CtkHSV              CtkHSV;
typedef struct _CtkHSVPrivate       CtkHSVPrivate;
typedef struct _CtkHSVClass         CtkHSVClass;

struct _CtkHSV
{
  CtkWidget parent_instance;

  /*< private >*/
  CtkHSVPrivate *priv;
};

struct _CtkHSVClass
{
  CtkWidgetClass parent_class;

  /* Notification signals */
  void (* changed) (CtkHSV          *hsv);

  /* Keybindings */
  void (* move)    (CtkHSV          *hsv,
                    CtkDirectionType type);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_DEPRECATED_IN_3_4
GType      ctk_hsv_get_type     (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_4
CtkWidget* ctk_hsv_new          (void);
CDK_DEPRECATED_IN_3_4
void       ctk_hsv_set_color    (CtkHSV    *hsv,
				 double     h,
				 double     s,
				 double     v);
CDK_DEPRECATED_IN_3_4
void       ctk_hsv_get_color    (CtkHSV    *hsv,
				 gdouble   *h,
				 gdouble   *s,
				 gdouble   *v);
CDK_DEPRECATED_IN_3_4
void       ctk_hsv_set_metrics  (CtkHSV    *hsv,
				 gint       size,
				 gint       ring_width);
CDK_DEPRECATED_IN_3_4
void       ctk_hsv_get_metrics  (CtkHSV    *hsv,
				 gint      *size,
				 gint      *ring_width);
CDK_DEPRECATED_IN_3_4
gboolean   ctk_hsv_is_adjusting (CtkHSV    *hsv);

G_END_DECLS

#endif /* __CTK_HSV_H__ */
