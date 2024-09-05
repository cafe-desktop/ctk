/*
 * Copyright (c) 2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "ctkstackcombo.h"
#include "ctkbox.h"
#include "ctkstack.h"
#include "ctkcomboboxtext.h"
#include "ctkprivate.h"
#include "ctkintl.h"

struct _CtkStackCombo
{
  CtkBox box;

  CtkComboBox *combo;
  CtkStack *stack;
  GBinding *binding;
};

struct _CtkStackComboClass {
  CtkBoxClass parent_class;
};

enum {
  PROP_0,
  PROP_STACK
};

G_DEFINE_TYPE (CtkStackCombo, ctk_stack_combo, CTK_TYPE_BOX)

static void
ctk_stack_combo_init (CtkStackCombo *self)
{
  self->stack = NULL;
  self->combo = CTK_COMBO_BOX (ctk_combo_box_text_new ());
  ctk_widget_show (CTK_WIDGET (self->combo));
  ctk_box_pack_start (CTK_BOX (self), CTK_WIDGET (self->combo), FALSE, FALSE, 0);
}

static void ctk_stack_combo_set_stack (CtkStackCombo *self,
                                       CtkStack      *stack);

static void
rebuild_combo (CtkStackCombo *self)
{
  ctk_stack_combo_set_stack (self, self->stack);
}

static void
on_child_visible_changed (CtkStackCombo *self)
{
  rebuild_combo (self);
}

static void
add_child (CtkWidget     *widget,
           CtkStackCombo *self)
{
  g_signal_handlers_disconnect_by_func (widget, G_CALLBACK (on_child_visible_changed), self);
  g_signal_connect_swapped (widget, "notify::visible", G_CALLBACK (on_child_visible_changed), self);

  if (ctk_widget_get_visible (widget))
    {
      char *name, *title;

      ctk_container_child_get (CTK_CONTAINER (self->stack), widget,
                               "name", &name,
                              "title", &title,
                               NULL);

      ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (self->combo), name, title);

      g_free (name);
      g_free (title);
    }
}

static void
populate_combo (CtkStackCombo *self)
{
  ctk_container_foreach (CTK_CONTAINER (self->stack), (CtkCallback)add_child, self);
}

static void
clear_combo (CtkStackCombo *self)
{
  ctk_combo_box_text_remove_all (CTK_COMBO_BOX_TEXT (self->combo));
}

static void
on_stack_child_added (CtkContainer  *container G_GNUC_UNUSED,
                      CtkWidget     *widget G_GNUC_UNUSED,
                      CtkStackCombo *self)
{
  rebuild_combo (self);
}

static void
on_stack_child_removed (CtkContainer  *container G_GNUC_UNUSED,
                        CtkWidget     *widget,
                        CtkStackCombo *self)
{
  g_signal_handlers_disconnect_by_func (widget, G_CALLBACK (on_child_visible_changed), self);
  rebuild_combo (self);
}

static void
disconnect_stack_signals (CtkStackCombo *self)
{
  g_binding_unbind (self->binding);
  self->binding = NULL;
  g_signal_handlers_disconnect_by_func (self->stack, on_stack_child_added, self);
  g_signal_handlers_disconnect_by_func (self->stack, on_stack_child_removed, self);
  g_signal_handlers_disconnect_by_func (self->stack, disconnect_stack_signals, self);
}

static void
connect_stack_signals (CtkStackCombo *self)
{
  g_signal_connect_after (self->stack, "add", G_CALLBACK (on_stack_child_added), self);
  g_signal_connect_after (self->stack, "remove", G_CALLBACK (on_stack_child_removed), self);
  g_signal_connect_swapped (self->stack, "destroy", G_CALLBACK (disconnect_stack_signals), self);
  self->binding = g_object_bind_property (self->stack, "visible-child-name", self->combo, "active-id", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
ctk_stack_combo_set_stack (CtkStackCombo *self,
                           CtkStack      *stack)
{
  if (stack)
    g_object_ref (stack);

  if (self->stack)
    {
      disconnect_stack_signals (self);
      clear_combo (self);
      g_clear_object (&self->stack);
    }

  if (stack)
    {
      self->stack = stack;
      populate_combo (self);
      connect_stack_signals (self);
    }
}

static void
ctk_stack_combo_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkStackCombo *self = CTK_STACK_COMBO (object);

  switch (prop_id)
    {
    case PROP_STACK:
      g_value_set_object (value, self->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_stack_combo_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkStackCombo *self = CTK_STACK_COMBO (object);

  switch (prop_id)
    {
    case PROP_STACK:
      if (self->stack != g_value_get_object (value))
        {
          ctk_stack_combo_set_stack (self, g_value_get_object (value));
          g_object_notify (G_OBJECT (self), "stack");
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_stack_combo_dispose (GObject *object)
{
  CtkStackCombo *self = CTK_STACK_COMBO (object);

  ctk_stack_combo_set_stack (self, NULL);

  G_OBJECT_CLASS (ctk_stack_combo_parent_class)->dispose (object);
}

static void
ctk_stack_combo_class_init (CtkStackComboClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->get_property = ctk_stack_combo_get_property;
  object_class->set_property = ctk_stack_combo_set_property;
  object_class->dispose = ctk_stack_combo_dispose;

  g_object_class_install_property (object_class,
                                   PROP_STACK,
                                   g_param_spec_object ("stack",
                                                        P_("Stack"),
                                                        P_("Stack"),
                                                        CTK_TYPE_STACK,
                                                        CTK_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  ctk_widget_class_set_css_name (widget_class, "stackcombo");
}
