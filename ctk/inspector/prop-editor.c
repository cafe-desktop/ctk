/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "config.h"
#include <glib/gi18n-lib.h>

#include "prop-editor.h"
#include "strv-editor.h"
#include "object-tree.h"

#include "ctkactionable.h"
#include "ctkadjustment.h"
#include "ctkapplicationwindow.h"
#include "ctkcelllayout.h"
#include "ctkcellrenderertext.h"
#include "ctkcolorbutton.h"
#include "ctkcolorchooser.h"
#include "ctkcolorchooserwidget.h"
#include "ctkcombobox.h"
#include "ctkfontchooser.h"
#include "ctkfontchooserwidget.h"
#include "ctkiconview.h"
#include "ctklabel.h"
#include "ctkpopover.h"
#include "ctkradiobutton.h"
#include "ctkscrolledwindow.h"
#include "ctkspinbutton.h"
#include "ctksettingsprivate.h"
#include "ctktogglebutton.h"
#include "ctkwidgetprivate.h"
#include "ctkcssnodeprivate.h"

struct _CtkInspectorPropEditorPrivate
{
  GObject *object;
  gchar *name;
  gboolean is_child_property;
  CtkWidget *editor;
};

enum
{
  PROP_0,
  PROP_OBJECT,
  PROP_NAME,
  PROP_IS_CHILD_PROPERTY
};

enum
{
  SHOW_OBJECT,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorPropEditor, ctk_inspector_prop_editor, CTK_TYPE_BOX);

static gboolean
is_child_property (GParamSpec *pspec)
{
  return g_param_spec_get_qdata (pspec, g_quark_from_string ("is-child-prop")) != NULL;
}

static GParamSpec *
mark_child_property (GParamSpec *pspec)
{
  if (pspec)
    g_param_spec_set_qdata (pspec, g_quark_from_string ("is-child-prop"), GINT_TO_POINTER (TRUE));
  return pspec;
}

static GParamSpec *
find_property (CtkInspectorPropEditor *editor)
{
  if (editor->priv->is_child_property)
    {
      CtkWidget *widget = CTK_WIDGET (editor->priv->object);
      CtkWidget *parent = ctk_widget_get_parent (widget);

      return mark_child_property (ctk_container_class_find_child_property (G_OBJECT_GET_CLASS (parent), editor->priv->name));
    }

  return g_object_class_find_property (G_OBJECT_GET_CLASS (editor->priv->object), editor->priv->name);
}

typedef struct
{
  gpointer instance;
  GObject *alive_object;
  gulong id;
} DisconnectData;

static void
disconnect_func (gpointer data)
{
  DisconnectData *dd = data;

  g_signal_handler_disconnect (dd->instance, dd->id);
}

static void
signal_removed (gpointer  data,
                GClosure *closure G_GNUC_UNUSED)
{
  DisconnectData *dd = data;

  g_object_steal_data (dd->alive_object, "alive-object-data");
  g_free (dd);
}

static void
g_object_connect_property (GObject    *object,
                           GParamSpec *spec,
                           GCallback   func,
                           gpointer    data,
                           GObject    *alive_object)
{
  GClosure *closure;
  gchar *with_detail;
  DisconnectData *dd;

  if (is_child_property (spec))
    with_detail = g_strconcat ("child-notify::", spec->name, NULL);
  else
    with_detail = g_strconcat ("notify::", spec->name, NULL);

  dd = g_new (DisconnectData, 1);

  closure = g_cclosure_new (func, data, NULL);
  g_closure_add_invalidate_notifier (closure, dd, signal_removed);
  dd->id = g_signal_connect_closure (object, with_detail, closure, FALSE);
  dd->instance = object;
  dd->alive_object = alive_object;

  g_object_set_data_full (G_OBJECT (alive_object), "alive-object-data",
                          dd, disconnect_func);

  g_free (with_detail);
}

static void
block_notify (GObject *editor)
{
  DisconnectData *dd = (DisconnectData *)g_object_get_data (editor, "alive-object-data");

  if (dd)
    g_signal_handler_block (dd->instance, dd->id);
}

static void
unblock_notify (GObject *editor)
{
  DisconnectData *dd = (DisconnectData *)g_object_get_data (editor, "alive-object-data");

  if (dd)
    g_signal_handler_unblock (dd->instance, dd->id);
}

typedef struct
{
  GObject *obj;
  GParamSpec *spec;
  gulong modified_id;
} ObjectProperty;

static void
free_object_property (ObjectProperty *p)
{
  g_free (p);
}

static void
connect_controller (GObject     *controller,
                    const gchar *signal,
                    GObject     *model,
                    GParamSpec  *spec,
                    GCallback    func)
{
  ObjectProperty *p;

  p = g_new (ObjectProperty, 1);
  p->obj = model;
  p->spec = spec;

  p->modified_id = g_signal_connect_data (controller, signal, func, p,
                                          (GClosureNotify)free_object_property, 0);
  g_object_set_data (controller, "object-property", p);
}

static void
block_controller (GObject *controller)
{
  ObjectProperty *p = g_object_get_data (controller, "object-property");

  if (p)
    g_signal_handler_block (controller, p->modified_id);
}

static void
unblock_controller (GObject *controller)
{
  ObjectProperty *p = g_object_get_data (controller, "object-property");

  if (p)
    g_signal_handler_unblock (controller, p->modified_id);
}

static void
get_property_value (GObject *object, GParamSpec *pspec, GValue *value)
{
  if (is_child_property (pspec))
    {
      CtkWidget *widget = CTK_WIDGET (object);
      CtkWidget *parent = ctk_widget_get_parent (widget);

      ctk_container_child_get_property (CTK_CONTAINER (parent),
                                        widget, pspec->name, value);
    }
  else
    g_object_get_property (object, pspec->name, value);
}

static void
set_property_value (GObject *object, GParamSpec *pspec, GValue *value)
{
  if (is_child_property (pspec))
    {
      CtkWidget *widget = CTK_WIDGET (object);
      CtkWidget *parent = ctk_widget_get_parent (widget);

      ctk_container_child_set_property (CTK_CONTAINER (parent),
                                        widget, pspec->name, value);
    }
  else
    g_object_set_property (object, pspec->name, value);
}

static void
notify_property (GObject *object, GParamSpec *pspec)
{
  if (is_child_property (pspec))
    {
      CtkWidget *widget = CTK_WIDGET (object);
      CtkWidget *parent = ctk_widget_get_parent (widget);

      ctk_container_child_notify (CTK_CONTAINER (parent), widget, pspec->name);
    }
  else
    g_object_notify (object, pspec->name);
}

static void
int_modified (CtkAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  
  g_value_init (&val, G_TYPE_INT);
  g_value_set_int (&val, (int) ctk_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
int_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkAdjustment *adj = CTK_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_INT);
  get_property_value (object, pspec, &val);

  if (g_value_get_int (&val) != (int)ctk_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      ctk_adjustment_set_value (adj, g_value_get_int (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}
static void
uint_modified (CtkAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  
  g_value_init (&val, G_TYPE_UINT);
  g_value_set_uint (&val, (guint) ctk_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
uint_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkAdjustment *adj = CTK_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_UINT);
  get_property_value (object, pspec, &val);

  if (g_value_get_uint (&val) != (guint)ctk_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      ctk_adjustment_set_value (adj, g_value_get_uint (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}

static void
float_modified (CtkAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  
  g_value_init (&val, G_TYPE_FLOAT);
  g_value_set_float (&val, (float) ctk_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
float_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkAdjustment *adj = CTK_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_FLOAT);
  get_property_value (object, pspec, &val);

  if (g_value_get_float (&val) != (float) ctk_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      ctk_adjustment_set_value (adj, g_value_get_float (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}

static void
double_modified (CtkAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  
  g_value_init (&val, G_TYPE_DOUBLE);
  g_value_set_double (&val, ctk_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
double_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkAdjustment *adj = CTK_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_DOUBLE);
  get_property_value (object, pspec, &val);

  if (g_value_get_double (&val) != ctk_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      ctk_adjustment_set_value (adj, g_value_get_double (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}

static void
string_modified (CtkEntry *entry, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  
  g_value_init (&val, G_TYPE_STRING);
  g_value_set_static_string (&val, ctk_entry_get_text (entry));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
intern_string_modified (CtkEntry *entry, ObjectProperty *p)
{
  const gchar *s;

  s = g_intern_string (ctk_entry_get_text (entry));
  if (g_str_equal (p->spec->name, "id"))
    ctk_css_node_set_id (CTK_CSS_NODE (p->obj), s);
  else if (g_str_equal (p->spec->name, "name"))
    ctk_css_node_set_name (CTK_CSS_NODE (p->obj), s);
}

static void
string_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkEntry *entry = CTK_ENTRY (data);
  GValue val = G_VALUE_INIT;
  const gchar *str;
  const gchar *text;

  g_value_init (&val, G_TYPE_STRING);
  get_property_value (object, pspec, &val);

  str = g_value_get_string (&val);
  if (str == NULL)
    str = "";
  text = ctk_entry_get_text (entry);
  if (g_strcmp0 (str, text) != 0)
    {
      block_controller (G_OBJECT (entry));
      ctk_entry_set_text (entry, str);
      unblock_controller (G_OBJECT (entry));
    }

  g_value_unset (&val);
}

static void
strv_modified (CtkInspectorStrvEditor *editor, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  gchar **strv;

  g_value_init (&val, G_TYPE_STRV);
  strv = ctk_inspector_strv_editor_get_strv (editor);
  g_value_take_boxed (&val, strv);
  block_notify (G_OBJECT (editor));
  set_property_value (p->obj, p->spec, &val);
  unblock_notify (G_OBJECT (editor));
  g_value_unset (&val);
}

static void
strv_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkInspectorStrvEditor *editor = data;
  GValue val = G_VALUE_INIT;
  gchar **strv;

  g_value_init (&val, G_TYPE_STRV);
  get_property_value (object, pspec, &val);

  strv = g_value_get_boxed (&val);
  block_controller (G_OBJECT (editor));
  ctk_inspector_strv_editor_set_strv (editor, strv);
  unblock_controller (G_OBJECT (editor));

  g_value_unset (&val);
}
static void
bool_modified (CtkToggleButton *tb, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean (&val, ctk_toggle_button_get_active (tb));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
bool_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkToggleButton *tb = CTK_TOGGLE_BUTTON (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_BOOLEAN);
  get_property_value (object, pspec, &val);

  if (g_value_get_boolean (&val) != ctk_toggle_button_get_active (tb))
    {
      block_controller (G_OBJECT (tb));
      ctk_toggle_button_set_active (tb, g_value_get_boolean (&val));
      unblock_controller (G_OBJECT (tb));
    }

  ctk_button_set_label (CTK_BUTTON (tb),
                        g_value_get_boolean (&val) ? "TRUE" : "FALSE");

  g_value_unset (&val);
}

static void
enum_modified (CtkToggleButton *button, ObjectProperty *p)
{
  gint i;
  GEnumClass *eclass;
  GValue val = G_VALUE_INIT;

  if (!ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button)))
    return;

  eclass = G_ENUM_CLASS (g_type_class_peek (p->spec->value_type));
  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "index"));

  g_value_init (&val, p->spec->value_type);
  g_value_set_enum (&val, eclass->values[i].value);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
enum_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkWidget *viewport;
  CtkWidget *box;
  GList *children, *c;
  GValue val = G_VALUE_INIT;
  GEnumClass *eclass;
  gint i, j;

  eclass = G_ENUM_CLASS (g_type_class_peek (pspec->value_type));

  g_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);

  i = 0;
  while (i < eclass->n_values)
    {
      if (eclass->values[i].value == g_value_get_enum (&val))
        break;
      ++i;
    }
  g_value_unset (&val);

  viewport = ctk_bin_get_child (CTK_BIN (data));
  box = ctk_bin_get_child (CTK_BIN (viewport));
  children = ctk_container_get_children (CTK_CONTAINER (box));

  for (c = children; c; c = c->next)
    block_controller (G_OBJECT (c->data));

  for (c = children, j = 0; c; c = c->next, j++)
    {
      if (j == i)
        ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (c->data), TRUE);
    }

  for (c = children; c; c = c->next)
    unblock_controller (G_OBJECT (c->data));
}

static void
flags_modified (CtkCheckButton *button, ObjectProperty *p)
{
  gboolean active;
  GFlagsClass *fclass;
  guint flags;
  gint i;
  GValue val = G_VALUE_INIT;

  active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button));
  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "index"));
  fclass = G_FLAGS_CLASS (g_type_class_peek (p->spec->value_type));

  g_value_init (&val, p->spec->value_type);
  get_property_value (p->obj, p->spec, &val);
  flags = g_value_get_flags (&val);
  if (active)
    flags |= fclass->values[i].value;
  else
    flags &= ~fclass->values[i].value;
  g_value_set_flags (&val, flags);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
flags_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  GList *children, *c;
  GValue val = G_VALUE_INIT;
  GFlagsClass *fclass;
  guint flags;
  gint i;
  CtkWidget *viewport;
  CtkWidget *box;

  fclass = G_FLAGS_CLASS (g_type_class_peek (pspec->value_type));

  g_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);
  flags = g_value_get_flags (&val);
  g_value_unset (&val);

  viewport = ctk_bin_get_child (CTK_BIN (data));
  box = ctk_bin_get_child (CTK_BIN (viewport));
  children = ctk_container_get_children (CTK_CONTAINER (box));

  for (c = children; c; c = c->next)
    block_controller (G_OBJECT (c->data));

  for (c = children, i = 0; c; c = c->next, i++)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (c->data),
                                  (fclass->values[i].value & flags) != 0);

  for (c = children; c; c = c->next)
    unblock_controller (G_OBJECT (c->data));

  g_list_free (children);
}

static gunichar
unichar_get_value (CtkEntry *entry)
{
  const gchar *text = ctk_entry_get_text (entry);

  if (text[0])
    return g_utf8_get_char (text);
  else
    return 0;
}

static void
unichar_modified (CtkEntry *entry, ObjectProperty *p)
{
  gunichar u = unichar_get_value (entry);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, p->spec->value_type);
  g_value_set_uint (&val, u);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}
static void
unichar_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkEntry *entry = CTK_ENTRY (data);
  gunichar new_val;
  gunichar old_val = unichar_get_value (entry);
  GValue val = G_VALUE_INIT;
  gchar buf[7];
  gint len;

  g_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);
  new_val = (gunichar)g_value_get_uint (&val);

  if (new_val != old_val)
    {
      if (!new_val)
        len = 0;
      else
        len = g_unichar_to_utf8 (new_val, buf);

      buf[len] = '\0';

      block_controller (G_OBJECT (entry));
      ctk_entry_set_text (entry, buf);
      unblock_controller (G_OBJECT (entry));
    }
}

static void
pointer_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkLabel *label = CTK_LABEL (data);
  gchar *str;
  gpointer ptr;

  g_object_get (object, pspec->name, &ptr, NULL);

  str = g_strdup_printf (_("Pointer: %p"), ptr);
  ctk_label_set_text (label, str);
  g_free (str);
}

static gchar *
object_label (GObject *obj, GParamSpec *pspec)
{
  const gchar *name;

  if (obj)
    name = g_type_name (G_TYPE_FROM_INSTANCE (obj));
  else if (pspec)
    name = g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec));
  else
    name = C_("type name", "Unknown");
  return g_strdup_printf (_("Object: %p (%s)"), obj, name);
}

static void
object_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkWidget *label, *button;
  gchar *str;
  GObject *obj;

  GList *children = ctk_container_get_children (CTK_CONTAINER (data));
  label = CTK_WIDGET (children->data);
  button = CTK_WIDGET (children->next->data);
  g_object_get (object, pspec->name, &obj, NULL);
  g_list_free (children);

  str = object_label (obj, pspec);

  ctk_label_set_text (CTK_LABEL (label), str);
  ctk_widget_set_sensitive (button, G_IS_OBJECT (obj));

  if (obj)
    g_object_unref (obj);

  g_free (str);
}

static void
object_properties (CtkInspectorPropEditor *editor)
{
  GObject *obj;

  g_object_get (editor->priv->object, editor->priv->name, &obj, NULL);
  if (G_IS_OBJECT (obj))
    g_signal_emit (editor, signals[SHOW_OBJECT], 0, obj, editor->priv->name, "properties");
}

static void
rgba_modified (CtkColorButton *cb,
               GParamSpec     *ignored G_GNUC_UNUSED,
               ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, p->spec->value_type);
  g_object_get_property (G_OBJECT (cb), "rgba", &val);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
rgba_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkColorChooser *cb = CTK_COLOR_CHOOSER (data);
  GValue val = G_VALUE_INIT;
  CdkRGBA *color;
  CdkRGBA cb_color;

  g_value_init (&val, CDK_TYPE_RGBA);
  get_property_value (object, pspec, &val);

  color = g_value_get_boxed (&val);
  ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (cb), &cb_color);

  if (color != NULL && !cdk_rgba_equal (color, &cb_color))
    {
      block_controller (G_OBJECT (cb));
      ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (cb), color);
      unblock_controller (G_OBJECT (cb));
    }
 g_value_unset (&val);
}

static void
color_modified (CtkColorButton *cb,
                GParamSpec     *ignored G_GNUC_UNUSED,
                ObjectProperty *p)
{
  CdkRGBA rgba;
  CdkColor color;
  GValue val = G_VALUE_INIT;

  ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (cb), &rgba);
  color.red = 65535 * rgba.red;
  color.green = 65535 * rgba.green;
  color.blue = 65535 * rgba.blue;

  g_value_init (&val, p->spec->value_type);
  g_value_set_boxed (&val, &color);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
color_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkColorChooser *cb = CTK_COLOR_CHOOSER (data);
  GValue val = G_VALUE_INIT;
  CdkColor *color;
  CdkRGBA rgba;

  g_value_init (&val, CDK_TYPE_COLOR);
  get_property_value (object, pspec, &val);
  color = g_value_get_boxed (&val);
  rgba.red = color->red / 65535.0;
  rgba.green = color->green / 65535.0;
  rgba.blue = color->blue / 65535.0;
  rgba.alpha = 1.0;

  if (g_value_get_boxed (&val))
    {
      block_controller (G_OBJECT (cb));
      ctk_color_chooser_set_rgba (cb, &rgba);
      unblock_controller (G_OBJECT (cb));
    }

  g_value_unset (&val);
}

static void
font_modified (CtkFontChooser *fb,
               GParamSpec     *pspec G_GNUC_UNUSED,
               ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, PANGO_TYPE_FONT_DESCRIPTION);
  g_object_get_property (G_OBJECT (fb), "font-desc", &val);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
font_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  CtkFontChooser *fb = CTK_FONT_CHOOSER (data);
  GValue val = G_VALUE_INIT;
  const PangoFontDescription *font_desc;
  PangoFontDescription *fb_font_desc;

  g_value_init (&val, PANGO_TYPE_FONT_DESCRIPTION);
  get_property_value (object, pspec, &val);

  font_desc = g_value_get_boxed (&val);
  fb_font_desc = ctk_font_chooser_get_font_desc (fb);

  if (font_desc == NULL ||
      (fb_font_desc != NULL &&
       !pango_font_description_equal (fb_font_desc, font_desc)))
    {
      block_controller (G_OBJECT (fb));
      ctk_font_chooser_set_font_desc (fb, font_desc);
      unblock_controller (G_OBJECT (fb));
    }

  g_value_unset (&val);
  pango_font_description_free (fb_font_desc);
}

static CtkWidget *
property_editor (GObject                *object,
                 GParamSpec             *spec,
                 CtkInspectorPropEditor *editor)
{
  CtkWidget *prop_edit;
  CtkAdjustment *adj;
  GType type = G_PARAM_SPEC_TYPE (spec);

  if (type == G_TYPE_PARAM_INT)
    {
      adj = ctk_adjustment_new (G_PARAM_SPEC_INT (spec)->default_value,
                                G_PARAM_SPEC_INT (spec)->minimum,
                                G_PARAM_SPEC_INT (spec)->maximum,
                                1,
                                MAX ((G_PARAM_SPEC_INT (spec)->maximum - G_PARAM_SPEC_INT (spec)->minimum) / 10, 1),
                                0.0);

      prop_edit = ctk_spin_button_new (adj, 1.0, 0);

      g_object_connect_property (object, spec, G_CALLBACK (int_changed), adj, G_OBJECT (adj)); 

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (int_modified));
    }
  else if (type == G_TYPE_PARAM_UINT)
    {
      adj = ctk_adjustment_new (G_PARAM_SPEC_UINT (spec)->default_value,
                                G_PARAM_SPEC_UINT (spec)->minimum,
                                G_PARAM_SPEC_UINT (spec)->maximum,
                                1,
                                MAX ((G_PARAM_SPEC_UINT (spec)->maximum - G_PARAM_SPEC_UINT (spec)->minimum) / 10, 1),
                                0.0);

      prop_edit = ctk_spin_button_new (adj, 1.0, 0);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (uint_changed),
                                 adj, G_OBJECT (adj));

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (uint_modified));
    }
  else if (type == G_TYPE_PARAM_FLOAT)
    {
      adj = ctk_adjustment_new (G_PARAM_SPEC_FLOAT (spec)->default_value,
                                G_PARAM_SPEC_FLOAT (spec)->minimum,
                                G_PARAM_SPEC_FLOAT (spec)->maximum,
                                0.1,
                                MAX ((G_PARAM_SPEC_FLOAT (spec)->maximum - G_PARAM_SPEC_FLOAT (spec)->minimum) / 10, 0.1),
                                0.0);

      prop_edit = ctk_spin_button_new (adj, 0.1, 2);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (float_changed),
                                 adj, G_OBJECT (adj));

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (float_modified));
    }
  else if (type == G_TYPE_PARAM_DOUBLE)
    {
      adj = ctk_adjustment_new (G_PARAM_SPEC_DOUBLE (spec)->default_value,
                                G_PARAM_SPEC_DOUBLE (spec)->minimum,
                                G_PARAM_SPEC_DOUBLE (spec)->maximum,
                                0.1,
                                1.0,
                                0.0);

      prop_edit = ctk_spin_button_new (adj, 0.1, 2);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (double_changed),
                                 adj, G_OBJECT (adj));

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (double_modified));
    }
  else if (type == G_TYPE_PARAM_STRING)
    {
      prop_edit = ctk_entry_new ();

      g_object_connect_property (object, spec,
                                 G_CALLBACK (string_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      if (CTK_IS_CSS_NODE (object))
        connect_controller (G_OBJECT (prop_edit), "changed",
                            object, spec, G_CALLBACK (intern_string_modified));
      else
        connect_controller (G_OBJECT (prop_edit), "changed",
                            object, spec, G_CALLBACK (string_modified));
    }
  else if (type == G_TYPE_PARAM_BOOLEAN)
    {
      prop_edit = ctk_toggle_button_new_with_label ("");

      g_object_connect_property (object, spec,
                                 G_CALLBACK (bool_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "toggled",
                          object, spec, G_CALLBACK (bool_modified));
    }
  else if (type == G_TYPE_PARAM_ENUM)
    {
      {
        CtkWidget *box;
        GEnumClass *eclass;
        CtkWidget *first;
        gint j;

        prop_edit = ctk_scrolled_window_new (NULL, NULL);
        g_object_set (prop_edit,
                      "expand", TRUE,
                      "hscrollbar-policy", CTK_POLICY_NEVER,
                      "vscrollbar-policy", CTK_POLICY_NEVER,
                      NULL);
        box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_widget_show (box);
        ctk_container_add (CTK_CONTAINER (prop_edit), box);

        eclass = G_ENUM_CLASS (g_type_class_ref (spec->value_type));

        j = 0;
        first = NULL;
        while (j < eclass->n_values)
          {
            CtkWidget *b;

            b = ctk_radio_button_new_with_label_from_widget ((CtkRadioButton*)first, eclass->values[j].value_name);
            if (first == NULL)
              first = b;
            g_object_set_data (G_OBJECT (b), "index", GINT_TO_POINTER (j));
            ctk_widget_show (b);
            ctk_box_pack_start (CTK_BOX (box), b, FALSE, FALSE, 0);
            connect_controller (G_OBJECT (b), "toggled",
                                object, spec, G_CALLBACK (enum_modified));
            ++j;
          }

        if (j >= 10)
          g_object_set (prop_edit, "vscrollbar-policy", CTK_POLICY_AUTOMATIC, NULL);

        g_type_class_unref (eclass);

        g_object_connect_property (object, spec,
                                   G_CALLBACK (enum_changed),
                                   prop_edit, G_OBJECT (prop_edit));
      }
    }
  else if (type == G_TYPE_PARAM_FLAGS)
    {
      {
        CtkWidget *box;
        GFlagsClass *fclass;
        gint j;

        prop_edit = ctk_scrolled_window_new (NULL, NULL);
        g_object_set (prop_edit,
                      "expand", TRUE,
                      "hscrollbar-policy", CTK_POLICY_NEVER,
                      "vscrollbar-policy", CTK_POLICY_NEVER,
                      NULL);
        box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
        ctk_widget_show (box);
        ctk_container_add (CTK_CONTAINER (prop_edit), box);

        fclass = G_FLAGS_CLASS (g_type_class_ref (spec->value_type));

        for (j = 0; j < fclass->n_values; j++)
          {
            CtkWidget *b;

            b = ctk_check_button_new_with_label (fclass->values[j].value_name);
            g_object_set_data (G_OBJECT (b), "index", GINT_TO_POINTER (j));
            ctk_widget_show (b);
            ctk_box_pack_start (CTK_BOX (box), b, FALSE, FALSE, 0);
            connect_controller (G_OBJECT (b), "toggled",
                                object, spec, G_CALLBACK (flags_modified));
          }

        if (j >= 10)
          g_object_set (prop_edit, "vscrollbar-policy", CTK_POLICY_AUTOMATIC, NULL);

        g_type_class_unref (fclass);

        g_object_connect_property (object, spec,
                                   G_CALLBACK (flags_changed),
                                   prop_edit, G_OBJECT (prop_edit));
      }
    }
  else if (type == G_TYPE_PARAM_UNICHAR)
    {
      prop_edit = ctk_entry_new ();
      ctk_entry_set_max_length (CTK_ENTRY (prop_edit), 1);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (unichar_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "changed",
                          object, spec, G_CALLBACK (unichar_modified));
    }
  else if (type == G_TYPE_PARAM_POINTER)
    {
      prop_edit = ctk_label_new ("");

      g_object_connect_property (object, spec,
                                 G_CALLBACK (pointer_changed),
                                 prop_edit, G_OBJECT (prop_edit));
    }
  else if (type == G_TYPE_PARAM_OBJECT)
    {
      CtkWidget *label, *button;

      prop_edit = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);

      label = ctk_label_new ("");
      button = ctk_button_new_with_label (_("Properties"));
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (object_properties),
                                editor);
      ctk_container_add (CTK_CONTAINER (prop_edit), label);
      ctk_container_add (CTK_CONTAINER (prop_edit), button);
      ctk_widget_show (label);
      ctk_widget_show (button);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (object_changed),
                                 prop_edit, G_OBJECT (label));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == CDK_TYPE_RGBA)
    {
      prop_edit = ctk_color_chooser_widget_new ();
      ctk_color_chooser_set_use_alpha (CTK_COLOR_CHOOSER (prop_edit), TRUE);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (rgba_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "notify::rgba",
                          object, spec, G_CALLBACK (rgba_modified));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == g_type_from_name ("CdkColor"))
    {
      prop_edit = ctk_color_chooser_widget_new ();
      ctk_color_chooser_set_use_alpha (CTK_COLOR_CHOOSER (prop_edit), FALSE);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (color_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "notify::rgba",
                          object, spec, G_CALLBACK (color_modified));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == PANGO_TYPE_FONT_DESCRIPTION)
    {
      prop_edit = ctk_font_chooser_widget_new ();

      g_object_connect_property (object, spec,
                                 G_CALLBACK (font_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "notify::font-desc",
                          object, spec, G_CALLBACK (font_modified));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == G_TYPE_STRV)
    {
      prop_edit = g_object_new (ctk_inspector_strv_editor_get_type (),
                                "visible", TRUE,
                                NULL);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (strv_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "changed",
                          object, spec, G_CALLBACK (strv_modified));

      ctk_widget_set_halign (prop_edit, CTK_ALIGN_START);
      ctk_widget_set_valign (prop_edit, CTK_ALIGN_CENTER);
    }
  else
    {
      gchar *msg;

      msg = g_strdup_printf (_("Uneditable property type: %s"),
                             g_type_name (G_PARAM_SPEC_TYPE (spec)));
      prop_edit = ctk_label_new (msg);
      g_free (msg);
      ctk_widget_set_halign (prop_edit, CTK_ALIGN_START);
      ctk_widget_set_valign (prop_edit, CTK_ALIGN_CENTER);
    }

  if (g_param_spec_get_blurb (spec))
    ctk_widget_set_tooltip_text (prop_edit, g_param_spec_get_blurb (spec));

  notify_property (object, spec);

  return prop_edit;
}

static void
ctk_inspector_prop_editor_init (CtkInspectorPropEditor *editor)
{
  editor->priv = ctk_inspector_prop_editor_get_instance_private (editor);
  g_object_set (editor,
                "orientation", CTK_ORIENTATION_VERTICAL,
                "spacing", 10,
                "margin", 10,
                NULL);
}

static CtkTreeModel *
ctk_cell_layout_get_model (CtkCellLayout *layout)
{
  if (CTK_IS_TREE_VIEW_COLUMN (layout))
    return ctk_tree_view_get_model (CTK_TREE_VIEW (ctk_tree_view_column_get_tree_view (CTK_TREE_VIEW_COLUMN (layout))));
  else if (CTK_IS_ICON_VIEW (layout))
    return ctk_icon_view_get_model (CTK_ICON_VIEW (layout));
  else if (CTK_IS_COMBO_BOX (layout)) 
    return ctk_combo_box_get_model (CTK_COMBO_BOX (layout));
  else
    return NULL;
}

static CtkWidget *
ctk_cell_layout_get_widget (CtkCellLayout *layout)
{
  if (CTK_IS_TREE_VIEW_COLUMN (layout))
    return ctk_tree_view_column_get_tree_view (CTK_TREE_VIEW_COLUMN (layout));
  else if (CTK_IS_WIDGET (layout))
    return CTK_WIDGET (layout);
  else
    return NULL;
}

static void
model_properties (CtkButton              *button,
                  CtkInspectorPropEditor *editor)
{
  GObject *model;

  model = g_object_get_data (G_OBJECT (button), "model");
  g_signal_emit (editor, signals[SHOW_OBJECT], 0, model, "model", "data");
}

static void
attribute_mapping_changed (CtkComboBox            *combo,
                           CtkInspectorPropEditor *editor)
{
  gint col;
  gpointer layout;

  col = ctk_combo_box_get_active (combo) - 1;
  layout = g_object_get_data (editor->priv->object, "ctk-inspector-cell-layout");
  if (CTK_IS_CELL_LAYOUT (layout))
    {
      CtkCellRenderer *cell;
      CtkCellArea *area;

      cell = CTK_CELL_RENDERER (editor->priv->object);
      area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (layout));
      ctk_cell_area_attribute_disconnect (area, cell, editor->priv->name);
      if (col != -1)
        ctk_cell_area_attribute_connect (area, cell, editor->priv->name, col);
      ctk_widget_set_sensitive (editor->priv->editor, col == -1);
      notify_property (editor->priv->object, find_property (editor));
      ctk_widget_queue_draw (ctk_cell_layout_get_widget (CTK_CELL_LAYOUT (layout)));
    }
}

static CtkWidget *
attribute_editor (GObject                *object,
                  GParamSpec             *spec,
                  CtkInspectorPropEditor *editor)
{
  gpointer layout;
  CtkTreeModel *model = NULL;
  gint col = -1;
  CtkWidget *label;
  CtkWidget *button;
  CtkWidget *vbox;
  CtkWidget *box;
  CtkWidget *combo;
  gchar *text;
  gint i;
  gboolean sensitive;
  CtkCellRenderer *renderer;
  CtkListStore *store;
  CtkTreeIter iter;

  layout = g_object_get_data (object, "ctk-inspector-cell-layout");
  if (CTK_IS_CELL_LAYOUT (layout))
    {
      CtkCellArea *area;

      area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (layout));
      col = ctk_cell_area_attribute_get_column (area,
                                                CTK_CELL_RENDERER (object), 
                                                editor->priv->name);
      model = ctk_cell_layout_get_model (CTK_CELL_LAYOUT (layout));
    }

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  label = ctk_label_new (_("Attribute mapping"));
  ctk_widget_set_margin_top (label, 10);
  ctk_container_add (CTK_CONTAINER (vbox), label);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new (_("Model:")));
  text = g_strdup_printf (_("%p (%s)"), model, model ? g_type_name (G_TYPE_FROM_INSTANCE (model)) : "null" );
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new (text));
  g_free (text);
  button = ctk_button_new_with_label (_("Properties"));
  g_object_set_data (G_OBJECT (button), "model", model);
  g_signal_connect (button, "clicked", G_CALLBACK (model_properties), editor);
  ctk_container_add (CTK_CONTAINER (box), button);
  ctk_container_add (CTK_CONTAINER (vbox), box);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (box), ctk_label_new (_("Column:")));
  store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  combo = ctk_combo_box_new_with_model (CTK_TREE_MODEL (store));
  renderer = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), renderer, FALSE);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo), renderer,
                                  "text", 0,
                                  "sensitive", 1,
                                  NULL);
  ctk_list_store_append (store, &iter);
  ctk_list_store_set (store, &iter, 0, C_("property name", "None"), 1, TRUE, -1);
  for (i = 0; i < ctk_tree_model_get_n_columns (model); i++)
    {
      text = g_strdup_printf ("%d", i);
      sensitive = g_value_type_transformable (ctk_tree_model_get_column_type (model, i),
                                              spec->value_type);
      ctk_list_store_append (store, &iter);
      ctk_list_store_set (store, &iter, 0, text, 1, sensitive, -1);
      g_free (text);
    }
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), col + 1);
  attribute_mapping_changed (CTK_COMBO_BOX (combo), editor);
  g_signal_connect (combo, "changed",
                    G_CALLBACK (attribute_mapping_changed), editor);
  ctk_container_add (CTK_CONTAINER (box), combo);
  ctk_container_add (CTK_CONTAINER (vbox), box);
  ctk_widget_show_all (vbox);

  return vbox;
}

static CtkWidget *
action_ancestor (CtkWidget *widget)
{
  if (CTK_IS_MENU (widget))
    return ctk_menu_get_attach_widget (CTK_MENU (widget));
  else if (CTK_IS_POPOVER (widget))
    return ctk_popover_get_relative_to (CTK_POPOVER (widget));
  else
    return ctk_widget_get_parent (widget);
}

static GObject *
find_action_owner (CtkActionable *actionable)
{
  CtkWidget *widget = CTK_WIDGET (actionable);
  const gchar *full_name;
  const gchar *dot;
  const gchar *name;
  gchar *prefix;
  CtkWidget *win;

  full_name = ctk_actionable_get_action_name (actionable);
  if (!full_name)
    return NULL;

  dot = strchr (full_name, '.');
  prefix = g_strndup (full_name, dot - full_name);
  name = dot + 1;

  win = ctk_widget_get_ancestor (widget, CTK_TYPE_APPLICATION_WINDOW);
  if (g_strcmp0 (prefix, "win") == 0)
    {
      if (G_IS_OBJECT (win))
        return (GObject *)win;
    }
  else if (g_strcmp0 (prefix, "app") == 0)
    {  
      if (CTK_IS_WINDOW (win))
        return (GObject *)ctk_window_get_application (CTK_WINDOW (win));
    }

  while (widget != NULL)
    {
      GActionGroup *group;

      group = ctk_widget_get_action_group (widget, prefix);
      if (group && g_action_group_has_action (group, name))
        return (GObject *)widget;
      widget = action_ancestor (widget);
    }

  return NULL;  
}

static void
show_action_owner (CtkButton              *button,
                   CtkInspectorPropEditor *editor)
{
  GObject *owner;

  owner = g_object_get_data (G_OBJECT (button), "owner");
  g_signal_emit (editor, signals[SHOW_OBJECT], 0, owner, NULL, "actions");
}

static CtkWidget *
action_editor (GObject                *object,
               CtkInspectorPropEditor *editor)
{
  CtkWidget *vbox;
  GObject *owner;

  owner = find_action_owner (CTK_ACTIONABLE (object));

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  if (owner)
    {
      CtkWidget *label;
      CtkWidget *box;
      CtkWidget *button;
      gchar *text;

      label = ctk_label_new (_("Action"));
      ctk_widget_set_margin_top (label, 10);
      ctk_container_add (CTK_CONTAINER (vbox), label);
      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      text = g_strdup_printf (_("Defined at: %p (%s)"),
                              owner, g_type_name_from_instance ((GTypeInstance *)owner));
      ctk_container_add (CTK_CONTAINER (box), ctk_label_new (text));
      g_free (text);
      button = ctk_button_new_with_label (_("Properties"));
      g_object_set_data (G_OBJECT (button), "owner", owner);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (show_action_owner), editor);
      ctk_container_add (CTK_CONTAINER (box), button);
      ctk_container_add (CTK_CONTAINER (vbox), box);
      ctk_widget_show_all (vbox);
    }

  return vbox;
}

static void
binding_object_properties (CtkButton *button, CtkInspectorPropEditor *editor)
{
  GObject *obj;

  obj = (GObject *)g_object_get_data (G_OBJECT (button), "object");
  if (G_IS_OBJECT (obj))
    g_signal_emit (editor, signals[SHOW_OBJECT], 0, obj, NULL, "properties");
}

static void
add_binding_info (CtkInspectorPropEditor *editor)
{
  GObject *object;
  const gchar *name;
  GHashTable *bindings;
  GHashTableIter iter;
  GBinding *binding;
  CtkWidget *row;
  CtkWidget *button;
  gchar *str;
  GObject *other;
  const gchar *property;
  const gchar *direction;
  const gchar *tip;
  CtkWidget *label;

  object = editor->priv->object;
  name = editor->priv->name;

  /* Note: this is accessing private GBinding details, so keep it
   * in sync with the implementation in GObject
   */
  bindings = (GHashTable *)g_object_get_data (G_OBJECT (object), "g-binding");
  if (!bindings)
    return;

  g_hash_table_iter_init (&iter, bindings);
  while (g_hash_table_iter_next (&iter, (gpointer*)&binding, NULL))
    {
      if (g_binding_dup_source (binding) == object &&
          g_str_equal (g_binding_get_source_property (binding), name))
        {
          other = g_binding_dup_target (binding);
          property = g_binding_get_target_property (binding);
          if (g_binding_get_flags (binding) & G_BINDING_INVERT_BOOLEAN)
            {
              direction = "↛";
              tip = _("inverted");
            }
          else
            {
              direction = "→";
              tip = NULL;
            }
        }
      else if (g_binding_dup_target (binding) == object &&
               g_str_equal (g_binding_get_target_property (binding), name))
        {
          other = g_binding_dup_source (binding);
          property = g_binding_get_source_property (binding);
          if (g_binding_get_flags (binding) & G_BINDING_INVERT_BOOLEAN)
            {
              direction = "↚";
              tip = _("inverted");
            }
          else
            {
              direction = "←";
              tip = NULL;
            }
        }
      else
        continue;
     
      if (g_binding_get_flags (binding) & G_BINDING_BIDIRECTIONAL)
        {
          if (g_binding_get_flags (binding) & G_BINDING_INVERT_BOOLEAN)
            {
              direction = "↮";
              tip = _("bidirectional, inverted");
            }
          else
            {
              direction = "↔";
              tip = _("bidirectional");
            }
        }

      row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_container_add (CTK_CONTAINER (row), ctk_label_new (_("Binding:")));
      label = ctk_label_new (direction);
      if (tip)
        ctk_widget_set_tooltip_text (label, tip);
      ctk_container_add (CTK_CONTAINER (row), label);
      str = g_strdup_printf ("%p :: %s", other, property);
      label = ctk_label_new (str);
      ctk_container_add (CTK_CONTAINER (row), label);
      g_free (str);
      button = ctk_button_new_with_label (_("Properties"));
      g_object_set_data (G_OBJECT (button), "object", other);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (binding_object_properties), editor);
      ctk_container_add (CTK_CONTAINER (row), button);

       ctk_widget_show_all (row);
       ctk_container_add (CTK_CONTAINER (editor), row);
    }
}

/* Note: Slightly nasty that we have to poke at the
 * GSettingsSchemaKey internals here. Keep this in
 * sync with the implementation in GIO!
 */
struct _GSettingsSchemaKey
{
  GSettingsSchema *schema;
  const gchar *name;

  guint is_flags : 1;
  guint is_enum  : 1;

  const guint32 *strinfo;
  gsize strinfo_length;

  const gchar *unparsed;
  gchar lc_char;

  const GVariantType *type;
  GVariant *minimum, *maximum;
  GVariant *default_value;

  gint ref_count;
};

typedef struct
{
  GSettingsSchemaKey key;
  GSettings *settings;
  GObject *object;

  GSettingsBindGetMapping get_mapping;
  GSettingsBindSetMapping set_mapping;
  gpointer user_data;
  GDestroyNotify destroy;

  guint writable_handler_id;
  guint property_handler_id;
  const GParamSpec *property;
  guint key_handler_id;

  /* prevent recursion */
  gboolean running;
} GSettingsBinding;

static void
add_attribute_info (CtkInspectorPropEditor *editor,
                    GParamSpec             *spec)
{
  if (CTK_IS_CELL_RENDERER (editor->priv->object))
    ctk_container_add (CTK_CONTAINER (editor),
                       attribute_editor (editor->priv->object, spec, editor));
}

static void
add_actionable_info (CtkInspectorPropEditor *editor)
{
  if (CTK_IS_ACTIONABLE (editor->priv->object) &&
      g_strcmp0 (editor->priv->name, "action-name") == 0)
    ctk_container_add (CTK_CONTAINER (editor),
                       action_editor (editor->priv->object, editor));
}

static void
add_settings_info (CtkInspectorPropEditor *editor)
{
  gchar *key;
  GSettingsBinding *binding;
  GObject *object;
  const gchar *name;
  const gchar *direction;
  const gchar *tip;
  CtkWidget *row;
  CtkWidget *label;
  gchar *str;

  object = editor->priv->object;
  name = editor->priv->name;

  key = g_strconcat ("gsettingsbinding-", name, NULL);
  binding = (GSettingsBinding *)g_object_get_data (object, key);
  g_free (key);

  if (!binding)
    return;

  if (binding->key_handler_id && binding->property_handler_id)
    {
      direction = "↔";
      tip = _("bidirectional");
    }
  else if (binding->key_handler_id)
    {
      direction = "←";
      tip = NULL;
    }
  else if (binding->property_handler_id)
    {
      direction = "→";
      tip = NULL;
    }
  else
    {
      direction = "?";
      tip = NULL;
    }

  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (row), ctk_label_new (_("Setting:")));
  label = ctk_label_new (direction);
  if (tip)
    ctk_widget_set_tooltip_text (label, tip);
  ctk_container_add (CTK_CONTAINER (row), label);
  
  str = g_strdup_printf ("%s %s",
                         g_settings_schema_get_id (binding->key.schema),
                         binding->key.name);
  label = ctk_label_new (str);
  ctk_container_add (CTK_CONTAINER (row), label);
  g_free (str);

  ctk_widget_show_all (row);
  ctk_container_add (CTK_CONTAINER (editor), row);
}

static void
reset_setting (CtkInspectorPropEditor *editor)
{
  ctk_settings_reset_property (CTK_SETTINGS (editor->priv->object),
                               editor->priv->name);
}

static void
add_ctk_settings_info (CtkInspectorPropEditor *editor)
{
  GObject *object;
  const gchar *name;
  CtkWidget *row;
  const gchar *source;
  CtkWidget *button;

  object = editor->priv->object;
  name = editor->priv->name;

  if (!CTK_IS_SETTINGS (object))
    return;

  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_container_add (CTK_CONTAINER (row), ctk_label_new (_("Source:")));

  button = ctk_button_new_with_label (_("Reset"));
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (reset_setting), editor);

  ctk_widget_set_halign (button, CTK_ALIGN_END);
  ctk_widget_show (button);
  ctk_widget_set_sensitive (button, FALSE);
  ctk_box_pack_end (CTK_BOX (row), button, FALSE, FALSE, 0);

  switch (_ctk_settings_get_setting_source (CTK_SETTINGS (object), name))
    {
    case CTK_SETTINGS_SOURCE_DEFAULT:
      source = _("Default");
      break;
    case CTK_SETTINGS_SOURCE_THEME:
      source = _("Theme");
      break;
    case CTK_SETTINGS_SOURCE_XSETTING:
      source = _("XSettings");
      break;
    case CTK_SETTINGS_SOURCE_APPLICATION:
      ctk_widget_set_sensitive (button, TRUE);
      source = _("Application");
      break;
    default:
      source = _("Unknown");
      break;
    }
  ctk_container_add (CTK_CONTAINER (row), ctk_label_new (source));

  ctk_widget_show_all (row);
  ctk_container_add (CTK_CONTAINER (editor), row);
}

static void
constructed (GObject *object)
{
  CtkInspectorPropEditor *editor = CTK_INSPECTOR_PROP_EDITOR (object);
  GParamSpec *spec;
  CtkWidget *label;
  gboolean can_modify;

  spec = find_property (editor);

  label = ctk_label_new (g_param_spec_get_nick (spec));
  ctk_widget_show (label);
  ctk_container_add (CTK_CONTAINER (editor), label);

  can_modify = ((spec->flags & G_PARAM_WRITABLE) != 0 &&
                (spec->flags & G_PARAM_CONSTRUCT_ONLY) == 0);

  if ((spec->flags & G_PARAM_CONSTRUCT_ONLY) != 0)
    label = ctk_label_new ("(construct-only)");
  else if ((spec->flags & G_PARAM_WRITABLE) == 0)
    label = ctk_label_new ("(not writable)");
  else
    label = NULL;

  if (label)
    {
      ctk_widget_show (label);
      ctk_style_context_add_class (ctk_widget_get_style_context (label), CTK_STYLE_CLASS_DIM_LABEL);
      ctk_container_add (CTK_CONTAINER (editor), label);
    }

  /* By reaching this, we already know the property is readable.
   * Since all we can do for a GObject is dive down into it's properties
   * and inspect bindings and such, pretend to be mutable.
   */
  if (g_type_is_a (spec->value_type, G_TYPE_OBJECT))
    can_modify = TRUE;

  if (!can_modify)
    return;

  editor->priv->editor = property_editor (editor->priv->object, spec, editor);
  ctk_widget_show (editor->priv->editor);
  ctk_container_add (CTK_CONTAINER (editor), editor->priv->editor);

  add_attribute_info (editor, spec);
  add_actionable_info (editor);
  add_binding_info (editor);
  add_settings_info (editor);
  add_ctk_settings_info (editor);
}

static void
finalize (GObject *object)
{
  CtkInspectorPropEditor *editor = CTK_INSPECTOR_PROP_EDITOR (object);

  g_free (editor->priv->name);

  G_OBJECT_CLASS (ctk_inspector_prop_editor_parent_class)->finalize (object);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  CtkInspectorPropEditor *r = CTK_INSPECTOR_PROP_EDITOR (object);

  switch (param_id)
    {
    case PROP_OBJECT:
      g_value_set_object (value, r->priv->object);
      break;

    case PROP_NAME:
      g_value_set_string (value, r->priv->name);
      break;

    case PROP_IS_CHILD_PROPERTY:
      g_value_set_boolean (value, r->priv->is_child_property);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  CtkInspectorPropEditor *r = CTK_INSPECTOR_PROP_EDITOR (object);

  switch (param_id)
    {
    case PROP_OBJECT:
      r->priv->object = g_value_get_object (value);
      break;

    case PROP_NAME:
      g_free (r->priv->name);
      r->priv->name = g_value_dup_string (value);
      break;

    case PROP_IS_CHILD_PROPERTY:
      r->priv->is_child_property = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    } 
}

static void
ctk_inspector_prop_editor_class_init (CtkInspectorPropEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = constructed;
  object_class->finalize = finalize;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  signals[SHOW_OBJECT] =
    g_signal_new ("show-object",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkInspectorPropEditorClass, show_object),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 3, G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_STRING);

  g_object_class_install_property (object_class, PROP_OBJECT,
      g_param_spec_object ("object", "Object", "The object owning the property",
                           G_TYPE_OBJECT, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", "Name", "The property name",
                           NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_IS_CHILD_PROPERTY,
      g_param_spec_boolean ("is-child-property", "Child property", "Whether this is a child property",
                            FALSE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
}

CtkWidget *
ctk_inspector_prop_editor_new (GObject     *object,
                               const gchar *name,
                               gboolean     is_child_property)
{
  return g_object_new (CTK_TYPE_INSPECTOR_PROP_EDITOR,
                       "object", object,
                       "name", name,
                       "is-child-property", is_child_property,
                       NULL);
}

gboolean
ctk_inspector_prop_editor_should_expand (CtkInspectorPropEditor *editor)
{
  if (CTK_IS_SCROLLED_WINDOW (editor->priv->editor))
    {
      CtkPolicyType policy;

      g_object_get (editor->priv->editor, "vscrollbar-policy", &policy, NULL);
      if (policy != CTK_POLICY_NEVER)
        return TRUE;
    }

  return FALSE;
}
