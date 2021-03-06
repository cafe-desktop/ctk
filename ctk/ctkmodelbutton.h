/*
 * Copyright © 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

#ifndef __CTK_MODEL_BUTTON_H__
#define __CTK_MODEL_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_MODEL_BUTTON         (ctk_model_button_get_type ())
#define CTK_MODEL_BUTTON(inst)        (G_TYPE_CHECK_INSTANCE_CAST ((inst),                      \
                                      CTK_TYPE_MODEL_BUTTON, CtkModelButton))
#define CTK_IS_MODEL_BUTTON(inst)     (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                      \
                                      CTK_TYPE_MODEL_BUTTON))

typedef struct _CtkModelButton        CtkModelButton;

/**
 * CtkButtonRole:
 * @CTK_BUTTON_ROLE_NORMAL: A plain button
 * @CTK_BUTTON_ROLE_CHECK: A check button
 * @CTK_BUTTON_ROLE_RADIO: A radio button
 *
 * The role specifies the desired appearance of a #CtkModelButton.
 */
typedef enum {
  CTK_BUTTON_ROLE_NORMAL,
  CTK_BUTTON_ROLE_CHECK,
  CTK_BUTTON_ROLE_RADIO
} CtkButtonRole;

CDK_AVAILABLE_IN_3_16
GType       ctk_model_button_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_16
CtkWidget * ctk_model_button_new      (void);

G_END_DECLS

#endif /* __CTK_MODEL_BUTTON_H__ */
