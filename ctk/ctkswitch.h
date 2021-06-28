/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2010  Intel Corporation
 * Copyright (C) 2010  RedHat, Inc.
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
 *
 * Author:
 *      Emmanuele Bassi <ebassi@linux.intel.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *
 * Based on similar code from Mx.
 */

#ifndef __CTK_SWITCH_H__
#define __CTK_SWITCH_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_SWITCH                 (ctk_switch_get_type ())
#define CTK_SWITCH(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SWITCH, CtkSwitch))
#define CTK_IS_SWITCH(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SWITCH))
#define CTK_SWITCH_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SWITCH, CtkSwitchClass))
#define CTK_IS_SWITCH_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SWITCH))
#define CTK_SWITCH_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SWITCH, CtkSwitchClass))

typedef struct _CtkSwitch               CtkSwitch;
typedef struct _CtkSwitchPrivate        CtkSwitchPrivate;
typedef struct _CtkSwitchClass          CtkSwitchClass;

/**
 * CtkSwitch:
 *
 * The #CtkSwitch-struct contains private
 * data and it should only be accessed using the provided API.
 */
struct _CtkSwitch
{
  /*< private >*/
  CtkWidget parent_instance;

  CtkSwitchPrivate *priv;
};

/**
 * CtkSwitchClass:
 * @parent_class: The parent class.
 * @activate: An action signal and emitting it causes the switch to animate.
 * @state_set: Class handler for the ::state-set signal.
 */
struct _CtkSwitchClass
{
  CtkWidgetClass parent_class;

  /*< public >*/

  void (* activate) (CtkSwitch *sw);

  gboolean (* state_set) (CtkSwitch *sw, gboolean state);
  /*< private >*/

  void (* _switch_padding_1) (void);
  void (* _switch_padding_2) (void);
  void (* _switch_padding_3) (void);
  void (* _switch_padding_4) (void);
  void (* _switch_padding_5) (void);
};

CDK_AVAILABLE_IN_ALL
GType ctk_switch_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkWidget *     ctk_switch_new          (void);

CDK_AVAILABLE_IN_ALL
void            ctk_switch_set_active   (CtkSwitch *sw,
                                         gboolean   is_active);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_switch_get_active   (CtkSwitch *sw);

CDK_AVAILABLE_IN_3_14
void            ctk_switch_set_state   (CtkSwitch *sw,
                                        gboolean   state);
CDK_AVAILABLE_IN_3_14
gboolean        ctk_switch_get_state   (CtkSwitch *sw);

G_END_DECLS

#endif /* __CTK_SWITCH_H__ */
