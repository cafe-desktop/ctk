/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2010 Javier Jardón
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_BUTTON_PRIVATE_H__
#define __CTK_BUTTON_PRIVATE_H__

#include "ctkactionhelper.h"
#include "ctkgesturesingle.h"
#include "deprecated/ctkaction.h"
#include "ctkcssgadgetprivate.h"

G_BEGIN_DECLS


struct _CtkButtonPrivate
{
  CtkAction             *action;
  CtkWidget             *image;
  CtkActionHelper       *action_helper;
  CtkCssGadget          *gadget;

  CdkDevice             *grab_keyboard;
  CdkWindow             *event_window;

  gchar                 *label_text;

  CtkGesture            *gesture;

  gfloat                 xalign;
  gfloat                 yalign;

  /* This is only used by checkbox and subclasses */
  gfloat                 baseline_align;

  guint                  activate_timeout;

  CtkPositionType        image_position;

  guint          align_set             : 1;
  guint          button_down           : 1;
  guint          constructed           : 1;
  guint          image_is_stock        : 1;
  guint          in_button             : 1;
  guint          use_action_appearance : 1;
  guint          use_stock             : 1;
  guint          use_underline         : 1;
  guint          always_show_image     : 1;
};


G_END_DECLS

#endif /* __CTK_BUTTON_PRIVATE_H__ */
