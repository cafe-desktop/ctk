/* Ctk+ property notify tests
 * Copyright (C) 2014 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <string.h>
#include <ctk/ctk.h>
#include <ctk/ctkunixprint.h>
#ifdef CDK_WINDOWING_X11
#include <ctk/ctkx.h>
#endif
#ifdef CDK_WINDOWING_WAYLAND
#include "cdk/wayland/cdkwayland.h"
#endif

typedef struct
{
  const gchar *name;
  gint count;
} NotifyData;

static void
count_notify (GObject *obj, GParamSpec *pspec, NotifyData *data)
{
  if (g_strcmp0 (data->name, pspec->name) == 0)
    data->count++;
}

/* Check that we get notifications when properties change.
 * Also check that we don't emit redundant notifications for
 * enum, flags, booleans, ints. We allow redundant notifications
 * for strings, and floats
 */
static void
check_property (GObject *instance, GParamSpec *pspec)
{
  if (G_TYPE_IS_ENUM (pspec->value_type))
    {
      GEnumClass *class;
      gint i;
      NotifyData data;
      gulong id;
      gint first;
      gint value;
      gint current_count;

      class = g_type_class_ref (pspec->value_type);

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      g_object_get (instance, pspec->name, &value, NULL);
      g_object_set (instance, pspec->name, value, NULL);

      g_assert_cmpint (data.count, ==, 0);

      if (class->values[0].value == value)
        first = 1;
      else
        first = 0;
  
      for (i = first; i < class->n_values; i++)
        {
          current_count = data.count + 1;
          g_object_set (instance, pspec->name, class->values[i].value, NULL);
          g_assert_cmpint (data.count, ==, current_count);  

          if (current_count == 10) /* just test a few */
            break;
        }

      g_signal_handler_disconnect (instance, id);
      g_type_class_unref (class);
    }
  else if (G_TYPE_IS_FLAGS (pspec->value_type))
    {
      GFlagsClass *class;
      gint i;
      NotifyData data;
      gulong id;
      guint value;
      gint current_count;

      class = g_type_class_ref (pspec->value_type);

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      g_object_get (instance, pspec->name, &value, NULL);
      g_object_set (instance, pspec->name, value, NULL);

      g_assert_cmpint (data.count, ==, 0);

      for (i = 0; i < class->n_values; i++)
        {
          /* some flags have a 'none' member, skip it */
          if (class->values[i].value == 0)
            continue;

          if ((value & class->values[i].value) != 0)
            continue;

          value |= class->values[i].value;
          current_count = data.count + 1;
          g_object_set (instance, pspec->name, value, NULL);
          g_assert_cmpint (data.count, ==, current_count);  

          if (current_count == 10) /* just test a few */
            break;
        }

      g_signal_handler_disconnect (instance, id);
      g_type_class_unref (class);
    }
  else if (pspec->value_type == G_TYPE_BOOLEAN)
    {
      NotifyData data;
      gboolean value;
      gulong id;

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      g_object_get (instance, pspec->name, &value, NULL);
      g_object_set (instance, pspec->name, value, NULL);

      g_assert_cmpint (data.count, ==, 0);

      g_object_set (instance, pspec->name, 1 - value, NULL);

      g_assert_cmpint (data.count, ==, 1);

      g_signal_handler_disconnect (instance, id);
    } 
  else if (pspec->value_type == G_TYPE_INT)
    {
      GParamSpecInt *p = G_PARAM_SPEC_INT (pspec);
      gint i;
      NotifyData data;
      gulong id;
      gint value;
      gint current_count;

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      g_object_get (instance, pspec->name, &value, NULL);
      g_object_set (instance, pspec->name, value, NULL);

      g_assert_cmpint (data.count, ==, 0);

      for (i = p->minimum; i <= p->maximum; i++)
        {
          g_object_get (instance, pspec->name, &value, NULL);
          if (value == i)
            continue;

          current_count = data.count + 1;
          g_object_set (instance, pspec->name, i, NULL);
          g_assert_cmpint (data.count, ==, current_count);  

          if (current_count == 10) /* just test a few */
            break;
        }

      g_signal_handler_disconnect (instance, id);
    }
  else if (pspec->value_type == G_TYPE_UINT)
    {
      guint i;
      NotifyData data;
      gulong id;
      guint value;
      gint current_count;
      guint minimum, maximum;

      if (G_IS_PARAM_SPEC_UINT (pspec))
        {
          minimum = G_PARAM_SPEC_UINT (pspec)->minimum;
          maximum = G_PARAM_SPEC_UINT (pspec)->maximum;
        }
      else /* random */
        {
          minimum = 0;
          maximum = 1000;
        }

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      g_object_get (instance, pspec->name, &value, NULL);
      g_object_set (instance, pspec->name, value, NULL);

      g_assert_cmpint (data.count, ==, 0);

      for (i = minimum; i <= maximum; i++)
        {
          g_object_get (instance, pspec->name, &value, NULL);
          if (value == i)
            continue;

          current_count = data.count + 1;
          g_object_set (instance, pspec->name, i, NULL);
          g_assert_cmpint (data.count, ==, current_count);  

          if (current_count == 10) /* just test a few */
            break;
        }

      g_signal_handler_disconnect (instance, id);
    }
  else if (pspec->value_type == G_TYPE_STRING)
    {
      NotifyData data;
      gulong id;
      gchar *value;
      gchar *new_value;

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      g_object_get (instance, pspec->name, &value, NULL);
      /* don't check redundant notifications */

      new_value = g_strconcat ("(", value, ".", value, ")", NULL);

      g_object_set (instance, pspec->name, new_value, NULL);

      g_assert_cmpint (data.count, ==, 1);

      g_free (value);
      g_free (new_value);

      g_signal_handler_disconnect (instance, id);
    }
  else if (pspec->value_type == G_TYPE_DOUBLE)
    {
      GParamSpecDouble *p = G_PARAM_SPEC_DOUBLE (pspec);
      guint i;
      NotifyData data;
      gulong id;
      gdouble value;
      gdouble new_value;
      gint current_count;
      gdouble delta;

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      /* don't check redundant notifications */
      g_object_get (instance, pspec->name, &value, NULL);
      
      if (p->maximum > 100 || p->minimum < -100)
        delta = M_PI;
      else
        delta = (p->maximum - p->minimum) / 10.0;

      new_value = p->minimum;
      for (i = 0; i < 10; i++)
        {
          new_value += delta;

          if (fabs (value - new_value) < p->epsilon)
            continue;

          if (new_value > p->maximum)
            break;

          current_count = data.count + 1;
          g_object_set (instance, pspec->name, new_value, NULL);
          g_assert_cmpint (data.count, ==, current_count);  
        }

      g_signal_handler_disconnect (instance, id);
    }
  else if (pspec->value_type == G_TYPE_FLOAT)
    {
      GParamSpecFloat *p = G_PARAM_SPEC_FLOAT (pspec);
      guint i;
      NotifyData data;
      gulong id;
      gfloat value;
      gfloat new_value;
      gint current_count;

      data.name = pspec->name;
      data.count = 0;
      id = g_signal_connect (instance, "notify", G_CALLBACK (count_notify), &data);

      /* don't check redundant notifications */
      g_object_get (instance, pspec->name, &value, NULL);
      
      new_value = p->minimum;
      for (i = 0; i < 10; i++)
        {
          if (fabs (value - new_value) < p->epsilon)
            continue;

          current_count = data.count + 1;
          new_value += (p->maximum - p->minimum) / 10.0;

          if (new_value > p->maximum)
            break;

          g_object_set (instance, pspec->name, new_value, NULL);
          g_assert_cmpint (data.count, ==, current_count);  
        }

      g_signal_handler_disconnect (instance, id);
    }
  else
    {
      if (g_test_verbose ())
        g_print ("Skipping property %s.%s of type %s\n", g_type_name (pspec->owner_type), pspec->name, g_type_name (pspec->value_type));
    }
}

static void
test_type (gconstpointer data)
{
  GObjectClass *klass;
  GObject *instance;
  GParamSpec **pspecs;
  guint n_pspecs, i;
  GType type;
  CdkDisplay *display;

  type = * (GType *) data;

  display = cdk_display_get_default ();

  if (!G_TYPE_IS_CLASSED (type))
    return;

  if (G_TYPE_IS_ABSTRACT (type))
    return;

  if (!g_type_is_a (type, G_TYPE_OBJECT))
    return;

  /* non-CTK+ */
  if (g_str_equal (g_type_name (type), "AtkObject") ||
      g_str_equal (g_type_name (type), "GdkPixbufSimpleAnim"))
    return;

  /* Deprecated, not getting fixed */
  if (g_str_equal (g_type_name (type), "CtkColorSelection") ||
      g_str_equal (g_type_name (type), "CtkHandleBox") ||
      g_str_equal (g_type_name (type), "CtkHPaned") ||
      g_str_equal (g_type_name (type), "CtkVPaned") ||
      g_str_equal (g_type_name (type), "CtkHScale") ||
      g_str_equal (g_type_name (type), "CtkVScale") ||
      g_str_equal (g_type_name (type), "CtkHScrollbar") ||
      g_str_equal (g_type_name (type), "CtkVScrollbar") ||
      g_str_equal (g_type_name (type), "CtkHSeparator") ||
      g_str_equal (g_type_name (type), "CtkVSeparator") ||
      g_str_equal (g_type_name (type), "CtkHBox") ||
      g_str_equal (g_type_name (type), "CtkVBox") ||
      g_str_equal (g_type_name (type), "CtkArrow") ||
      g_str_equal (g_type_name (type), "CtkNumerableIcon") ||
      g_str_equal (g_type_name (type), "CtkRadioAction") ||
      g_str_equal (g_type_name (type), "CtkToggleAction") ||
      g_str_equal (g_type_name (type), "CtkTable") ||
      g_str_equal (g_type_name (type), "CtkUIManager") ||
      g_str_equal (g_type_name (type), "CtkImageMenuItem"))
    return;

  /* These can't be freely constructed/destroyed */
  if (g_type_is_a (type, CTK_TYPE_APPLICATION) ||
      g_type_is_a (type, CDK_TYPE_PIXBUF_LOADER) ||
      g_type_is_a (type, CDK_TYPE_DRAWING_CONTEXT) ||
#ifdef G_OS_UNIX
      g_type_is_a (type, CTK_TYPE_PRINT_JOB) ||
#endif
      g_type_is_a (type, cdk_pixbuf_simple_anim_iter_get_type ()) ||
      g_str_equal (g_type_name (type), "CdkX11DeviceManagerXI2") ||
      g_str_equal (g_type_name (type), "CdkX11DeviceManagerCore") ||
      g_str_equal (g_type_name (type), "CdkX11Display") ||
      g_str_equal (g_type_name (type), "CdkX11DisplayManager") ||
      g_str_equal (g_type_name (type), "CdkX11Screen") ||
      g_str_equal (g_type_name (type), "CdkX11GLContext"))
    return;

  /* This throws a critical when the connection is dropped */
  if (g_type_is_a (type, CTK_TYPE_APP_CHOOSER_DIALOG))
    return;

  /* These leak their GDBusConnections */
  if (g_type_is_a (type, CTK_TYPE_FILE_CHOOSER_BUTTON) ||
      g_type_is_a (type, CTK_TYPE_FILE_CHOOSER_DIALOG) ||
      g_type_is_a (type, CTK_TYPE_FILE_CHOOSER_WIDGET) ||
      g_type_is_a (type, CTK_TYPE_FILE_CHOOSER_NATIVE) ||
      g_type_is_a (type, CTK_TYPE_PLACES_SIDEBAR))
    return;

  /* These rely on a d-bus session bus */
  if (g_type_is_a (type, CTK_TYPE_MOUNT_OPERATION))
    return;

  /* Backend-specific */
#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (cdk_display_get_default ())) ;
  else if (g_type_is_a (type, CTK_TYPE_PLUG) ||
           g_type_is_a (type, CTK_TYPE_SOCKET))
    return;
#endif

  if (g_type_is_a (type, CTK_TYPE_STATUS_ICON))
    return;

  klass = g_type_class_ref (type);

  if (g_type_is_a (type, CTK_TYPE_SETTINGS))
    instance = G_OBJECT (g_object_ref (ctk_settings_get_default ()));
  else if (g_type_is_a (type, CDK_TYPE_WINDOW))
    {
      CdkWindowAttr attributes;
      attributes.wclass = CDK_INPUT_OUTPUT;
      attributes.window_type = CDK_WINDOW_TEMP;
      attributes.event_mask = 0;
      attributes.width = 100;
      attributes.height = 100;
      instance = G_OBJECT (g_object_ref (cdk_window_new (NULL, &attributes, 0)));
    }
  else if (g_str_equal (g_type_name (type), "CdkX11Cursor"))
    instance = g_object_new (type, "display", display, NULL);
  else
    instance = g_object_new (type, NULL);

  if (g_type_is_a (type, G_TYPE_INITIALLY_UNOWNED))
    g_object_ref_sink (instance);

  pspecs = g_object_class_list_properties (klass, &n_pspecs);
  for (i = 0; i < n_pspecs; ++i)
    {
      GParamSpec *pspec = pspecs[i];

      if ((pspec->flags & G_PARAM_READABLE) == 0)
	continue;

      if ((pspec->flags & G_PARAM_WRITABLE) == 0)
        continue;

      if ((pspec->flags & G_PARAM_CONSTRUCT_ONLY) != 0)
        continue;

      /* non-CTK+ */
      if (g_str_equal (g_type_name (pspec->owner_type), "AtkObject") ||
          g_str_equal (g_type_name (pspec->owner_type), "GdkPixbufSimpleAnim") || 
          g_str_equal (g_type_name (pspec->owner_type), "GMountOperation")) 
        continue;

      /* set properties are best skipped */
      if (pspec->value_type == G_TYPE_BOOLEAN &&
          g_str_has_suffix (pspec->name, "-set"))
        continue;

      /* These are special */
      if (g_type_is_a (pspec->owner_type, CTK_TYPE_WIDGET) &&
	  (g_str_equal (pspec->name, "has-focus") ||
	   g_str_equal (pspec->name, "has-default") ||
           g_str_equal (pspec->name, "is-focus") ||
           g_str_equal (pspec->name, "margin") ||
           g_str_equal (pspec->name, "hexpand") ||
           g_str_equal (pspec->name, "vexpand") ||
           g_str_equal (pspec->name, "expand")
            ))
	continue;

      if (pspec->owner_type == CTK_TYPE_ENTRY &&
          g_str_equal (pspec->name, "im-module"))
        continue;

      if (type == CTK_TYPE_SETTINGS)
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_ENTRY_COMPLETION) &&
          g_str_equal (pspec->name, "text-column"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_MENU_ITEM) &&
	  g_str_equal (pspec->name, "accel-path"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_MENU) &&
	  (g_str_equal (pspec->name, "accel-path") ||
	   g_str_equal (pspec->name, "active")))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_CHECK_MENU_ITEM) &&
	  g_str_equal (pspec->name, "active"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_COLOR_CHOOSER) &&
	  g_str_equal (pspec->name, "show-editor"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_NOTEBOOK) &&
	  g_str_equal (pspec->name, "page"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_TOGGLE_BUTTON) &&
	  g_str_equal (pspec->name, "draw-indicator"))
        continue;

      /* Not supported in subclass */
      if (g_str_equal (g_type_name (type), "CtkRecentAction") &&
	  g_str_equal (pspec->name, "select-multiple"))
        continue;

      if (g_str_equal (g_type_name (type), "CtkRecentChooserMenu") &&
	  g_str_equal (pspec->name, "select-multiple"))
        continue;

      /* Really a bug in the way CtkButton and its subclasses interact:
       * setting label etc on a subclass destroys the content, breaking
       * e.g. CtkColorButton pretty badly
       */
      if (type == CTK_TYPE_COLOR_BUTTON && pspec->owner_type == CTK_TYPE_BUTTON)
        continue;

      /* CdkOffscreenWindow is missing many implementations */
      if (type == CTK_TYPE_OFFSCREEN_WINDOW)
        continue;

      /* Too many special cases involving -set properties */
      if (g_str_equal (g_type_name (pspec->owner_type), "CtkCellRendererText") ||
          g_str_equal (g_type_name (pspec->owner_type), "CtkTextTag"))
        continue;

      /* Most things assume a model is set */
      if (g_str_equal (g_type_name (pspec->owner_type), "CtkComboBox"))
        continue;

      /* Deprecated, not getting fixed */
      if (g_str_equal (g_type_name (pspec->owner_type), "CtkActivatable") ||
          g_str_equal (g_type_name (pspec->owner_type), "CtkActionGroup") ||
          g_str_equal (g_type_name (pspec->owner_type), "CtkAction"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_CONTAINER) &&
	  g_str_equal (pspec->name, "resize-mode"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_COLOR_BUTTON) &&
	  g_str_equal (pspec->name, "alpha"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_CELL_RENDERER_PIXBUF) &&
	  (g_str_equal (pspec->name, "follow-state") ||
	   g_str_equal (pspec->name, "stock-id") ||
           g_str_equal (pspec->name, "stock-size") ||
           g_str_equal (pspec->name, "stock-detail")))
        continue;

       if (g_str_equal (g_type_name (pspec->owner_type), "CtkArrow") ||
          g_str_equal (g_type_name (pspec->owner_type), "CtkAlignment") ||
          g_str_equal (g_type_name (pspec->owner_type), "CtkMisc"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_MENU) &&
	  g_str_equal (pspec->name, "tearoff-state"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_WIDGET) &&
	  g_str_equal (pspec->name, "double-buffered"))
        continue;

      if (g_type_is_a (pspec->owner_type, CTK_TYPE_WINDOW) &&
	  g_str_equal (pspec->name, "has-resize-grip"))
        continue;

      /* Can only be set on window widgets */
      if (pspec->owner_type == CTK_TYPE_WIDGET &&
          g_str_equal (pspec->name, "events"))
        continue;

      /* Can only be set on unmapped windows */
      if (pspec->owner_type == CTK_TYPE_WINDOW &&
          g_str_equal (pspec->name, "type-hint"))
        continue;

      /* Special restrictions on allowed values */
      if (pspec->owner_type == CTK_TYPE_COMBO_BOX &&
          (g_str_equal (pspec->name, "row-span-column") ||
           g_str_equal (pspec->name, "column-span-column") ||
           g_str_equal (pspec->name, "id-column") ||
           g_str_equal (pspec->name, "active-id") ||
           g_str_equal (pspec->name, "entry-text-column")))
        continue;

      if (pspec->owner_type == CTK_TYPE_ENTRY_COMPLETION &&
          g_str_equal (pspec->name, "text-column"))
        continue;

      if (pspec->owner_type == CTK_TYPE_PRINT_OPERATION &&
          (g_str_equal (pspec->name, "current-page") ||
           g_str_equal (pspec->name, "n-pages")))
        continue;

      if (pspec->owner_type == CTK_TYPE_RANGE &&
          g_str_equal (pspec->name, "fill-level"))
        continue;

      if (pspec->owner_type == CTK_TYPE_SPIN_BUTTON &&
          g_str_equal (pspec->name, "value"))
        continue;

      if (pspec->owner_type == CTK_TYPE_STACK &&
          g_str_equal (pspec->name, "visible-child-name"))
        continue;

      if (pspec->owner_type == CTK_TYPE_POPOVER_MENU &&
          g_str_equal (pspec->name, "visible-submenu"))
        continue;

      if (pspec->owner_type == CTK_TYPE_TEXT_VIEW &&
          g_str_equal (pspec->name, "im-module"))
        continue;

      if (pspec->owner_type == CTK_TYPE_TOOLBAR &&
          g_str_equal (pspec->name, "icon-size"))
        continue;

      if (pspec->owner_type == CTK_TYPE_TREE_SELECTION &&
          g_str_equal (pspec->name, "mode")) /* requires a treeview */
        continue;

      if (pspec->owner_type == CTK_TYPE_TREE_VIEW &&
          g_str_equal (pspec->name, "headers-clickable")) /* requires columns */
        continue;

      /* This one has a special-purpose default value */
      if (g_type_is_a (type, CTK_TYPE_DIALOG) &&
	  g_str_equal (pspec->name, "use-header-bar"))
	continue;

      if (g_type_is_a (type, CTK_TYPE_ASSISTANT) &&
	  g_str_equal (pspec->name, "use-header-bar"))
	continue;

      if (type == CTK_TYPE_MODEL_BUTTON &&
          pspec->owner_type == CTK_TYPE_BUTTON)
        continue;

      if (g_type_is_a (type, CTK_TYPE_SHORTCUTS_SHORTCUT) &&
	  g_str_equal (pspec->name, "accelerator"))
	continue;

      if (g_type_is_a (type, CTK_TYPE_SHORTCUT_LABEL) &&
	  g_str_equal (pspec->name, "accelerator"))
	continue;

      if (g_type_is_a (type, CTK_TYPE_FONT_CHOOSER) &&
	  g_str_equal (pspec->name, "font"))
	continue;

      if (g_type_is_a (type, CTK_TYPE_FONT_BUTTON) &&
	  g_str_equal (pspec->name, "font-name"))
	continue;

      /* these depend on the min-content- properties in a way that breaks our test */
      if (g_type_is_a (type, CTK_TYPE_SCROLLED_WINDOW) &&
	  (g_str_equal (pspec->name, "max-content-width") ||
	   g_str_equal (pspec->name, "max-content-height")))
	continue;

      if (g_test_verbose ())
        g_print ("Property %s.%s\n", g_type_name (pspec->owner_type), pspec->name);

      check_property (instance, pspec);
    }
  g_free (pspecs);

  if (g_type_is_a (type, CDK_TYPE_WINDOW))
    cdk_window_destroy (CDK_WINDOW (instance));
  else
    g_object_unref (instance);

  g_type_class_unref (klass);
}

int
main (int argc, char **argv)
{
  const GType *otypes;
  guint i;
  gchar *schema_dir;
  gint result;

  ctk_test_init (&argc, &argv);
  ctk_test_register_all_types();

  /* g_test_build_filename must be called after ctk_test_init */
  schema_dir = g_test_build_filename (G_TEST_BUILT, "", NULL);
  if (g_getenv ("CTK_TEST_MESON") == NULL)
    g_setenv ("GSETTINGS_SCHEMA_DIR", schema_dir, TRUE);

  otypes = ctk_test_list_all_types (NULL);
  for (i = 0; otypes[i]; i++)
    {
      gchar *testname;

      testname = g_strdup_printf ("/Notification/%s", g_type_name (otypes[i]));
      g_test_add_data_func (testname, &otypes[i], test_type);
      g_free (testname);
    }

  result = g_test_run ();

  g_free (schema_dir);

  return result;
}
