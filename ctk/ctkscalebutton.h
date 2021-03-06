/* CTK - The GIMP Toolkit
 * Copyright (C) 2005 Ronald S. Bultje
 * Copyright (C) 2006, 2007 Christian Persch
 * Copyright (C) 2006 Jan Arne Petersen
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * Authors:
 * - Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * - Bastien Nocera <bnocera@redhat.com>
 * - Jan Arne Petersen <jpetersen@jpetersen.org>
 * - Christian Persch <chpe@svn.gnome.org>
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
 * Modified by the CTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_SCALE_BUTTON_H__
#define __CTK_SCALE_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbutton.h>

G_BEGIN_DECLS

#define CTK_TYPE_SCALE_BUTTON                 (ctk_scale_button_get_type ())
#define CTK_SCALE_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SCALE_BUTTON, CtkScaleButton))
#define CTK_SCALE_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SCALE_BUTTON, CtkScaleButtonClass))
#define CTK_IS_SCALE_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SCALE_BUTTON))
#define CTK_IS_SCALE_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SCALE_BUTTON))
#define CTK_SCALE_BUTTON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SCALE_BUTTON, CtkScaleButtonClass))

typedef struct _CtkScaleButton        CtkScaleButton;
typedef struct _CtkScaleButtonClass   CtkScaleButtonClass;
typedef struct _CtkScaleButtonPrivate CtkScaleButtonPrivate;

struct _CtkScaleButton
{
  CtkButton parent;

  /*< private >*/
  CtkScaleButtonPrivate *priv;
};

struct _CtkScaleButtonClass
{
  CtkButtonClass parent_class;

  /* signals */
  void	(* value_changed) (CtkScaleButton *button,
                           gdouble         value);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType            ctk_scale_button_get_type         (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget *      ctk_scale_button_new              (CtkIconSize      size,
                                                    gdouble          min,
                                                    gdouble          max,
                                                    gdouble          step,
                                                    const gchar    **icons);
CDK_AVAILABLE_IN_ALL
void             ctk_scale_button_set_icons        (CtkScaleButton  *button,
                                                    const gchar    **icons);
CDK_AVAILABLE_IN_ALL
gdouble          ctk_scale_button_get_value        (CtkScaleButton  *button);
CDK_AVAILABLE_IN_ALL
void             ctk_scale_button_set_value        (CtkScaleButton  *button,
                                                    gdouble          value);
CDK_AVAILABLE_IN_ALL
CtkAdjustment *  ctk_scale_button_get_adjustment   (CtkScaleButton  *button);
CDK_AVAILABLE_IN_ALL
void             ctk_scale_button_set_adjustment   (CtkScaleButton  *button,
                                                    CtkAdjustment   *adjustment);
CDK_AVAILABLE_IN_ALL
CtkWidget *      ctk_scale_button_get_plus_button  (CtkScaleButton  *button);
CDK_AVAILABLE_IN_ALL
CtkWidget *      ctk_scale_button_get_minus_button (CtkScaleButton  *button);
CDK_AVAILABLE_IN_ALL
CtkWidget *      ctk_scale_button_get_popup        (CtkScaleButton  *button);

G_END_DECLS

#endif /* __CTK_SCALE_BUTTON_H__ */
