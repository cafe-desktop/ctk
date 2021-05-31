/* CTK - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
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

#include "ctkcoloreditorprivate.h"

#include "ctkcolorchooserprivate.h"
#include "ctkcolorplaneprivate.h"
#include "ctkcolorscaleprivate.h"
#include "ctkcolorswatchprivate.h"
#include "ctkcolorutils.h"
#include "ctkcolorpickerprivate.h"
#include "ctkgrid.h"
#include "ctkbutton.h"
#include "ctkintl.h"
#include "ctkorientable.h"
#include "ctkentry.h"
#include "ctkoverlay.h"
#include "ctkadjustment.h"
#include "ctklabel.h"
#include "ctkrender.h"
#include "ctkspinbutton.h"
#include "ctkstylecontext.h"

#include <math.h>

struct _CtkColorEditorPrivate
{
  CtkWidget *overlay;
  CtkWidget *grid;
  CtkWidget *swatch;
  CtkWidget *entry;
  CtkWidget *h_slider;
  CtkWidget *h_popup;
  CtkWidget *h_entry;
  CtkWidget *a_slider;
  CtkWidget *a_popup;
  CtkWidget *a_entry;
  CtkWidget *sv_plane;
  CtkWidget *sv_popup;
  CtkWidget *s_entry;
  CtkWidget *v_entry;
  CtkWidget *current_popup;
  CtkWidget *popdown_focus;

  CtkAdjustment *h_adj;
  CtkAdjustment *s_adj;
  CtkAdjustment *v_adj;
  CtkAdjustment *a_adj;

  CtkWidget *picker_button;
  CtkColorPicker *picker;

  gint popup_position;

  guint text_changed : 1;
  guint use_alpha    : 1;
};

enum
{
  PROP_ZERO,
  PROP_RGBA,
  PROP_USE_ALPHA
};

static void ctk_color_editor_iface_init (CtkColorChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkColorEditor, ctk_color_editor, CTK_TYPE_BOX,
                         G_ADD_PRIVATE (CtkColorEditor)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_COLOR_CHOOSER,
                                                ctk_color_editor_iface_init))

static guint
scale_round (gdouble value, gdouble scale)
{
  value = floor (value * scale + 0.5);
  value = MAX (value, 0);
  value = MIN (value, scale);
  return (guint)value;
}

static void
entry_set_rgba (CtkColorEditor *editor,
                const GdkRGBA  *color)
{
  gchar *text;

  text = g_strdup_printf ("#%02X%02X%02X",
                          scale_round (color->red, 255),
                          scale_round (color->green, 255),
                          scale_round (color->blue, 255));
  ctk_entry_set_text (CTK_ENTRY (editor->priv->entry), text);
  editor->priv->text_changed = FALSE;
  g_free (text);
}

static void
entry_apply (CtkWidget      *entry,
             CtkColorEditor *editor)
{
  GdkRGBA color;
  gchar *text;

  if (!editor->priv->text_changed)
    return;

  text = ctk_editable_get_chars (CTK_EDITABLE (editor->priv->entry), 0, -1);
  if (gdk_rgba_parse (&color, text))
    {
      color.alpha = ctk_adjustment_get_value (editor->priv->a_adj);
      ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (editor), &color);
    }

  editor->priv->text_changed = FALSE;

  g_free (text);
}

static gboolean
entry_focus_out (CtkWidget      *entry,
                 GdkEventFocus  *event,
                 CtkColorEditor *editor)
{
  entry_apply (entry, editor);
  return FALSE;
}

static void
entry_text_changed (CtkWidget      *entry,
                    GParamSpec     *pspec,
                    CtkColorEditor *editor)
{
  editor->priv->text_changed = TRUE;
}

static void
hsv_changed (CtkColorEditor *editor)
{
  GdkRGBA color;
  gdouble h, s, v, a;

  h = ctk_adjustment_get_value (editor->priv->h_adj);
  s = ctk_adjustment_get_value (editor->priv->s_adj);
  v = ctk_adjustment_get_value (editor->priv->v_adj);
  a = ctk_adjustment_get_value (editor->priv->a_adj);

  ctk_hsv_to_rgb (h, s, v, &color.red, &color.green, &color.blue);
  color.alpha = a;

  ctk_color_swatch_set_rgba (CTK_COLOR_SWATCH (editor->priv->swatch), &color);
  ctk_color_scale_set_rgba (CTK_COLOR_SCALE (editor->priv->a_slider), &color);
  entry_set_rgba (editor, &color);

  g_object_notify (G_OBJECT (editor), "rgba");
}

static void
dismiss_current_popup (CtkColorEditor *editor)
{
  if (editor->priv->current_popup)
    {
      ctk_widget_hide (editor->priv->current_popup);
      editor->priv->current_popup = NULL;
      editor->priv->popup_position = 0;
      if (editor->priv->popdown_focus)
        {
          if (ctk_widget_is_visible (editor->priv->popdown_focus))
            ctk_widget_grab_focus (editor->priv->popdown_focus);
          g_clear_object (&editor->priv->popdown_focus);
        }
    }
}

static void
popup_edit (CtkWidget      *widget,
            CtkColorEditor *editor)
{
  CtkWidget *popup = NULL;
  CtkWidget *toplevel;
  CtkWidget *focus;
  gint position;
  gint s, e;

  if (widget == editor->priv->sv_plane)
    {
      popup = editor->priv->sv_popup;
      focus = editor->priv->s_entry;
      position = 0;
    }
  else if (widget == editor->priv->h_slider)
    {
      popup = editor->priv->h_popup;
      focus = editor->priv->h_entry;
      ctk_range_get_slider_range (CTK_RANGE (editor->priv->h_slider), &s, &e);
      position = (s + e) / 2;
    }
  else if (widget == editor->priv->a_slider)
    {
      popup = editor->priv->a_popup;
      focus = editor->priv->a_entry;
      ctk_range_get_slider_range (CTK_RANGE (editor->priv->a_slider), &s, &e);
      position = (s + e) / 2;
    }

  if (popup == editor->priv->current_popup)
    dismiss_current_popup (editor);
  else if (popup)
    {
      dismiss_current_popup (editor);
      toplevel = ctk_widget_get_toplevel (CTK_WIDGET (editor));
      g_set_object (&editor->priv->popdown_focus, ctk_window_get_focus (CTK_WINDOW (toplevel)));
      editor->priv->current_popup = popup;
      editor->priv->popup_position = position;
      ctk_widget_show (popup);
      ctk_widget_grab_focus (focus);
    }
}

static gboolean
popup_key_press (CtkWidget      *popup,
                 GdkEventKey    *event,
                 CtkColorEditor *editor)
{
  if (event->keyval == GDK_KEY_Escape)
    {
      dismiss_current_popup (editor);
      return TRUE;
    }

  return FALSE;
}

static gboolean
get_child_position (CtkOverlay     *overlay,
                    CtkWidget      *widget,
                    CtkAllocation  *allocation,
                    CtkColorEditor *editor)
{
  CtkRequisition req;
  CtkAllocation alloc;
  gint s, e;

  ctk_widget_get_preferred_size (widget, &req, NULL);

  allocation->x = 0;
  allocation->y = 0;
  allocation->width = req.width;
  allocation->height = req.height;

  if (widget == editor->priv->sv_popup)
    {
      ctk_widget_translate_coordinates (editor->priv->sv_plane,
                                        ctk_widget_get_parent (editor->priv->grid),
                                        0, -6,
                                        &allocation->x, &allocation->y);
      if (ctk_widget_get_direction (CTK_WIDGET (overlay)) == CTK_TEXT_DIR_RTL)
        allocation->x = 0;
      else
        allocation->x = ctk_widget_get_allocated_width (CTK_WIDGET (overlay)) - req.width;
    }
  else if (widget == editor->priv->h_popup)
    {
      ctk_widget_get_allocation (editor->priv->h_slider, &alloc);
      ctk_range_get_slider_range (CTK_RANGE (editor->priv->h_slider), &s, &e);

      if (ctk_widget_get_direction (CTK_WIDGET (overlay)) == CTK_TEXT_DIR_RTL)
        ctk_widget_translate_coordinates (editor->priv->h_slider,
                                          ctk_widget_get_parent (editor->priv->grid),
                                          - req.width - 6, editor->priv->popup_position - req.height / 2,
                                          &allocation->x, &allocation->y);
      else
        ctk_widget_translate_coordinates (editor->priv->h_slider,
                                          ctk_widget_get_parent (editor->priv->grid),
                                          alloc.width + 6, editor->priv->popup_position - req.height / 2,
                                          &allocation->x, &allocation->y);
    }
  else if (widget == editor->priv->a_popup)
    {
      ctk_widget_get_allocation (editor->priv->a_slider, &alloc);
      ctk_range_get_slider_range (CTK_RANGE (editor->priv->a_slider), &s, &e);

      ctk_widget_translate_coordinates (editor->priv->a_slider,
                                        ctk_widget_get_parent (editor->priv->grid),
                                        editor->priv->popup_position - req.width / 2, - req.height - 6,
                                        &allocation->x, &allocation->y);
    }
  else
    return FALSE;

  allocation->x = CLAMP (allocation->x, 0, ctk_widget_get_allocated_width (CTK_WIDGET (overlay)) - req.width);
  allocation->y = CLAMP (allocation->y, 0, ctk_widget_get_allocated_height (CTK_WIDGET (overlay)) - req.height);

  return TRUE;
}

static void
value_changed (CtkAdjustment *a,
               CtkAdjustment *as)
{
  gdouble scale;

  scale = ctk_adjustment_get_upper (as) / ctk_adjustment_get_upper (a);
  g_signal_handlers_block_by_func (as, value_changed, a);
  ctk_adjustment_set_value (as, ctk_adjustment_get_value (a) * scale);
  g_signal_handlers_unblock_by_func (as, value_changed, a);
}

static CtkAdjustment *
scaled_adjustment (CtkAdjustment *a,
                   gdouble        scale)
{
  CtkAdjustment *as;

  as = ctk_adjustment_new (ctk_adjustment_get_value (a) * scale,
                           ctk_adjustment_get_lower (a) * scale,
                           ctk_adjustment_get_upper (a) * scale,
                           ctk_adjustment_get_step_increment (a) * scale,
                           ctk_adjustment_get_page_increment (a) * scale,
                           ctk_adjustment_get_page_size (a) * scale);

  g_signal_connect (a, "value-changed", G_CALLBACK (value_changed), as);
  g_signal_connect (as, "value-changed", G_CALLBACK (value_changed), a);

  return as;
}

static gboolean
popup_draw (CtkWidget      *popup,
            cairo_t        *cr,
            CtkColorEditor *editor)
{
  CtkStyleContext *context;
  gint width, height;

  context = ctk_widget_get_style_context (popup);
  width = ctk_widget_get_allocated_width (popup);
  height = ctk_widget_get_allocated_height (popup);

  ctk_render_background (context, cr, 0, 0, width, height);
  ctk_render_frame (context, cr, 0, 0, width, height);

  return FALSE;
}

static void
color_picked (GObject      *source,
              GAsyncResult *res,
              gpointer      data)
{
  CtkColorPicker *picker = CTK_COLOR_PICKER (source);
  CtkColorEditor *editor = data;
  GError *error = NULL;
  GdkRGBA *color;

  color = ctk_color_picker_pick_finish (picker, res, &error);
  if (color == NULL)
    {
      g_error_free (error);
    }
  else
    {
      ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (editor), color);
      gdk_rgba_free (color);
    }
}

static void
pick_color (CtkButton      *button,
            CtkColorEditor *editor)
{
  ctk_color_picker_pick (editor->priv->picker, color_picked, editor);
}

static void
ctk_color_editor_init (CtkColorEditor *editor)
{
  editor->priv = ctk_color_editor_get_instance_private (editor);
  editor->priv->use_alpha = TRUE;

  g_type_ensure (CTK_TYPE_COLOR_SCALE);
  g_type_ensure (CTK_TYPE_COLOR_PLANE);
  g_type_ensure (CTK_TYPE_COLOR_SWATCH);
  ctk_widget_init_template (CTK_WIDGET (editor));

  /* Some post processing is needed in code to set this up */
  ctk_widget_set_events (editor->priv->swatch,
			 ctk_widget_get_events (editor->priv->swatch)
                                 & ~(GDK_BUTTON_PRESS_MASK
                                     | GDK_BUTTON_RELEASE_MASK
                                     | GDK_KEY_PRESS_MASK
                                     | GDK_KEY_RELEASE_MASK));

  if (ctk_widget_get_direction (editor->priv->h_slider) == CTK_TEXT_DIR_RTL)
    ctk_style_context_add_class (ctk_widget_get_style_context (editor->priv->h_slider),
                                 "marks-before");
  else
    ctk_style_context_add_class (ctk_widget_get_style_context (editor->priv->h_slider),
                                 "marks-after");

  /* Create the scaled popup adjustments manually here because connecting user data is not
   * supported by template CtkBuilder xml (it would be possible to set this up in the xml
   * but require 4 separate callbacks and would be rather ugly).
   */
  ctk_spin_button_set_adjustment (CTK_SPIN_BUTTON (editor->priv->h_entry), scaled_adjustment (editor->priv->h_adj, 100));
  ctk_spin_button_set_adjustment (CTK_SPIN_BUTTON (editor->priv->s_entry), scaled_adjustment (editor->priv->s_adj, 100));
  ctk_spin_button_set_adjustment (CTK_SPIN_BUTTON (editor->priv->v_entry), scaled_adjustment (editor->priv->v_adj, 100));
  ctk_spin_button_set_adjustment (CTK_SPIN_BUTTON (editor->priv->a_entry), scaled_adjustment (editor->priv->a_adj, 100));

  /* This can be setup in the .ui file, but requires work in Glade otherwise it cannot be edited there */
  ctk_overlay_add_overlay (CTK_OVERLAY (editor->priv->overlay), editor->priv->sv_popup);
  ctk_overlay_add_overlay (CTK_OVERLAY (editor->priv->overlay), editor->priv->h_popup);
  ctk_overlay_add_overlay (CTK_OVERLAY (editor->priv->overlay), editor->priv->a_popup);

  ctk_style_context_remove_class (ctk_widget_get_style_context (editor->priv->swatch), "activatable");

  editor->priv->picker = ctk_color_picker_new ();
  if (editor->priv->picker == NULL)
    ctk_widget_hide (editor->priv->picker_button);
}

static void
ctk_color_editor_dispose (GObject *object)
{
  CtkColorEditor *editor = CTK_COLOR_EDITOR (object);

  dismiss_current_popup (editor);
  g_clear_object (&editor->priv->picker);

  G_OBJECT_CLASS (ctk_color_editor_parent_class)->dispose (object);
}

static void
ctk_color_editor_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  CtkColorEditor *ce = CTK_COLOR_EDITOR (object);
  CtkColorChooser *cc = CTK_COLOR_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      {
        GdkRGBA color;
        ctk_color_chooser_get_rgba (cc, &color);
        g_value_set_boxed (value, &color);
      }
      break;
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, ctk_widget_get_visible (ce->priv->a_slider));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_color_editor_set_use_alpha (CtkColorEditor *editor,
                                gboolean        use_alpha)
{
  if (editor->priv->use_alpha != use_alpha)
    {
      editor->priv->use_alpha = use_alpha;
      ctk_widget_set_visible (editor->priv->a_slider, use_alpha);
      ctk_color_swatch_set_use_alpha (CTK_COLOR_SWATCH (editor->priv->swatch), use_alpha);
    }
}

static void
ctk_color_editor_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  CtkColorEditor *ce = CTK_COLOR_EDITOR (object);
  CtkColorChooser *cc = CTK_COLOR_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      ctk_color_chooser_set_rgba (cc, g_value_get_boxed (value));
      break;
    case PROP_USE_ALPHA:
      ctk_color_editor_set_use_alpha (ce, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_color_editor_class_init (CtkColorEditorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->dispose = ctk_color_editor_dispose;
  object_class->get_property = ctk_color_editor_get_property;
  object_class->set_property = ctk_color_editor_set_property;

  g_object_class_override_property (object_class, PROP_RGBA, "rgba");
  g_object_class_override_property (object_class, PROP_USE_ALPHA, "use-alpha");

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkcoloreditor.ui");

  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, overlay);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, grid);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, swatch);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, h_slider);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, h_popup);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, h_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, a_slider);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, a_popup);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, a_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, sv_plane);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, sv_popup);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, s_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, v_entry);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, h_adj);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, s_adj);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, v_adj);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, a_adj);
  ctk_widget_class_bind_template_child_private (widget_class, CtkColorEditor, picker_button);

  ctk_widget_class_bind_template_callback (widget_class, hsv_changed);
  ctk_widget_class_bind_template_callback (widget_class, popup_draw);
  ctk_widget_class_bind_template_callback (widget_class, popup_key_press);
  ctk_widget_class_bind_template_callback (widget_class, dismiss_current_popup);
  ctk_widget_class_bind_template_callback (widget_class, get_child_position);
  ctk_widget_class_bind_template_callback (widget_class, entry_text_changed);
  ctk_widget_class_bind_template_callback (widget_class, entry_apply);
  ctk_widget_class_bind_template_callback (widget_class, entry_focus_out);
  ctk_widget_class_bind_template_callback (widget_class, popup_edit);
  ctk_widget_class_bind_template_callback (widget_class, pick_color);
}

static void
ctk_color_editor_get_rgba (CtkColorChooser *chooser,
                           GdkRGBA         *color)
{
  CtkColorEditor *editor = CTK_COLOR_EDITOR (chooser);
  gdouble h, s, v;

  h = ctk_adjustment_get_value (editor->priv->h_adj);
  s = ctk_adjustment_get_value (editor->priv->s_adj);
  v = ctk_adjustment_get_value (editor->priv->v_adj);
  ctk_hsv_to_rgb (h, s, v, &color->red, &color->green, &color->blue);
  color->alpha = ctk_adjustment_get_value (editor->priv->a_adj);
}

static void
ctk_color_editor_set_rgba (CtkColorChooser *chooser,
                           const GdkRGBA   *color)
{
  CtkColorEditor *editor = CTK_COLOR_EDITOR (chooser);
  gdouble h, s, v;

  ctk_rgb_to_hsv (color->red, color->green, color->blue, &h, &s, &v);

  ctk_adjustment_set_value (editor->priv->h_adj, h);
  ctk_adjustment_set_value (editor->priv->s_adj, s);
  ctk_adjustment_set_value (editor->priv->v_adj, v);
  ctk_adjustment_set_value (editor->priv->a_adj, color->alpha);

  ctk_color_swatch_set_rgba (CTK_COLOR_SWATCH (editor->priv->swatch), color);
  ctk_color_scale_set_rgba (CTK_COLOR_SCALE (editor->priv->a_slider), color);
  entry_set_rgba (editor, color);

  g_object_notify (G_OBJECT (editor), "rgba");
}

static void
ctk_color_editor_iface_init (CtkColorChooserInterface *iface)
{
  iface->get_rgba = ctk_color_editor_get_rgba;
  iface->set_rgba = ctk_color_editor_set_rgba;
}

CtkWidget *
ctk_color_editor_new (void)
{
  return (CtkWidget *) g_object_new (CTK_TYPE_COLOR_EDITOR, NULL);
}
