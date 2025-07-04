/* CTK - The GIMP Toolkit
 * Copyright (C) 2017 Red Hat, Inc.
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

#include <string.h>
#include <wayland-client-protocol.h>

#include <ctk/ctk.h>
#include "ctk/ctkintl.h"
#include "ctk/ctkimmodule.h"

#include "cdk/wayland/cdkwayland.h"
#include "ctk-text-input-client-protocol.h"

typedef struct _CtkIMContextWaylandGlobal CtkIMContextWaylandGlobal;
typedef struct _CtkIMContextWayland CtkIMContextWayland;
typedef struct _CtkIMContextWaylandClass CtkIMContextWaylandClass;

struct _CtkIMContextWaylandGlobal
{
  struct wl_display *display;
  struct wl_registry *registry;
  uint32_t text_input_manager_wl_id;
  struct ctk_text_input_manager *text_input_manager;
  struct ctk_text_input *text_input;
  uint32_t enter_serial;

  CtkIMContext *current;
};

struct _CtkIMContextWaylandClass
{
  CtkIMContextSimpleClass parent_class;
};

struct _CtkIMContextWayland
{
  CtkIMContextSimple parent_instance;
  CdkWindow *window;
  CtkWidget *widget;

  CtkGesture *gesture;
  gdouble press_x;
  gdouble press_y;

  struct {
    gchar *text;
    gint cursor_idx;
  } surrounding;

  struct {
    gchar *text;
    gint cursor_idx;
  } preedit;

  cairo_rectangle_int_t cursor_rect;
  guint use_preedit : 1;
};

GType type_wayland = 0;
static GObjectClass *parent_class;
static CtkIMContextWaylandGlobal *global = NULL;

static const CtkIMContextInfo imwayland_info =
{
  "waylandctk",      /* ID */
  NC_("input method menu", "Waylandctk"),      /* Human readable name */
  GETTEXT_PACKAGE, /* Translation domain */
  CTK_LOCALEDIR,   /* Dir for bindtextdomain (not strictly needed for "ctk+") */
  "",              /* Languages for which this module is the default */
};

static const CtkIMContextInfo *info_list[] =
{
  &imwayland_info,
};

#define CTK_IM_CONTEXT_WAYLAND(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), type_wayland, CtkIMContextWayland))

#ifndef INCLUDE_IM_wayland
#define MODULE_ENTRY(type,function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _ctk_immodule_wayland_ ## function
#endif

static void
reset_preedit (CtkIMContextWayland *context)
{
  if (context->preedit.text == NULL)
    return;

  g_clear_pointer (&context->preedit.text, g_free);
  context->preedit.cursor_idx = 0;
  g_signal_emit_by_name (context, "preedit-changed");
}

static void
text_input_enter (void                  *data,
                  struct ctk_text_input *text_input G_GNUC_UNUSED,
                  uint32_t               serial,
                  struct wl_surface     *surface G_GNUC_UNUSED)
{
  CtkIMContextWaylandGlobal *global = data;

  global->enter_serial = serial;
}

static void
text_input_leave (void                  *data G_GNUC_UNUSED,
                  struct ctk_text_input *text_input G_GNUC_UNUSED,
                  uint32_t               serial G_GNUC_UNUSED,
                  struct wl_surface     *surface G_GNUC_UNUSED)
{
  CtkIMContextWayland *context;

  if (!global->current)
    return;

  context = CTK_IM_CONTEXT_WAYLAND (global->current);
  reset_preedit (context);
}

static void
text_input_preedit (void                  *data G_GNUC_UNUSED,
                    struct ctk_text_input *text_input G_GNUC_UNUSED,
                    const char            *text,
                    guint                  cursor)
{
  CtkIMContextWayland *context;
  gboolean state_change;

  if (!global->current)
    return;

  context = CTK_IM_CONTEXT_WAYLAND (global->current);
  if (!text && !context->preedit.text)
    return;

  state_change = ((text == NULL) != (context->preedit.text == NULL));

  if (state_change && !context->preedit.text)
    g_signal_emit_by_name (context, "preedit-start");

  g_free (context->preedit.text);
  context->preedit.text = g_strdup (text);
  context->preedit.cursor_idx = cursor;

  g_signal_emit_by_name (context, "preedit-changed");

  if (state_change && !context->preedit.text)
    g_signal_emit_by_name (context, "preedit-end");
}

static void
text_input_commit (void                  *data,
                   struct ctk_text_input *text_input G_GNUC_UNUSED,
                   const char            *text)
{
  CtkIMContextWaylandGlobal *global = data;

  if (global->current && text)
    g_signal_emit_by_name (global->current, "commit", text);
}

static void
text_input_delete_surrounding_text (void                  *data,
                                    struct ctk_text_input *text_input G_GNUC_UNUSED,
                                    uint32_t               offset,
                                    uint32_t               len)
{
  CtkIMContextWaylandGlobal *global = data;

  if (global->current)
    g_signal_emit_by_name (global->current, "delete-surrounding", offset, len);
}

static const struct ctk_text_input_listener text_input_listener = {
  text_input_enter,
  text_input_leave,
  text_input_preedit,
  text_input_commit,
  text_input_delete_surrounding_text
};

static void
registry_handle_global (void               *data,
                        struct wl_registry *registry G_GNUC_UNUSED,
                        uint32_t            id,
                        const char         *interface,
                        uint32_t            version G_GNUC_UNUSED)
{
  CdkSeat *seat = cdk_display_get_default_seat (cdk_display_get_default ());

  if (strcmp (interface, "ctk_text_input_manager") == 0)
    {
      CtkIMContextWaylandGlobal *global = data;

      global->text_input_manager_wl_id = id;
      global->text_input_manager =
        wl_registry_bind (global->registry, global->text_input_manager_wl_id,
                          &ctk_text_input_manager_interface, 1);
      global->text_input =
        ctk_text_input_manager_get_text_input (global->text_input_manager,
                                               cdk_wayland_seat_get_wl_seat (seat));
      ctk_text_input_add_listener (global->text_input,
                                   &text_input_listener, global);
    }
}

static void
registry_handle_global_remove (void               *data,
                               struct wl_registry *registry G_GNUC_UNUSED,
                               uint32_t            id)
{
  CtkIMContextWaylandGlobal *global = data;

  if (id != global->text_input_manager_wl_id)
    return;

  g_clear_pointer(&global->text_input, ctk_text_input_destroy);
  g_clear_pointer(&global->text_input_manager, ctk_text_input_manager_destroy);
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

static void
ctk_im_context_wayland_global_init (CdkDisplay *display)
{
  g_return_if_fail (global == NULL);

  global = g_new0 (CtkIMContextWaylandGlobal, 1);
  global->display = cdk_wayland_display_get_wl_display (display);
  global->registry = wl_display_get_registry (global->display);

  wl_registry_add_listener (global->registry, &registry_listener, global);
}

static void
notify_surrounding_text (CtkIMContextWayland *context)
{
  if (!global || !global->text_input)
    return;
  if (global->current != CTK_IM_CONTEXT (context))
    return;
  if (!context->surrounding.text)
    return;

  ctk_text_input_set_surrounding_text (global->text_input,
                                       context->surrounding.text,
                                       context->surrounding.cursor_idx,
                                       context->surrounding.cursor_idx);
}

static void
notify_cursor_location (CtkIMContextWayland *context)
{
  cairo_rectangle_int_t rect;

  if (!global || !global->text_input)
    return;
  if (global->current != CTK_IM_CONTEXT (context))
    return;
  if (!context->window)
    return;

  rect = context->cursor_rect;
  cdk_window_get_root_coords (context->window, rect.x, rect.y,
                              &rect.x, &rect.y);

  ctk_text_input_set_cursor_rectangle (global->text_input,
                                       rect.x, rect.y,
                                       rect.width, rect.height);
}

static uint32_t
translate_hints (CtkInputHints   input_hints,
                 CtkInputPurpose purpose)
{
  uint32_t hints = 0;

  if (input_hints & CTK_INPUT_HINT_SPELLCHECK)
    hints |= CTK_TEXT_INPUT_CONTENT_HINT_SPELLCHECK;
  if (input_hints & CTK_INPUT_HINT_WORD_COMPLETION)
    hints |= CTK_TEXT_INPUT_CONTENT_HINT_COMPLETION;
  if (input_hints & CTK_INPUT_HINT_LOWERCASE)
    hints |= CTK_TEXT_INPUT_CONTENT_HINT_LOWERCASE;
  if (input_hints & CTK_INPUT_HINT_UPPERCASE_CHARS)
    hints |= CTK_TEXT_INPUT_CONTENT_HINT_UPPERCASE;
  if (input_hints & CTK_INPUT_HINT_UPPERCASE_WORDS)
    hints |= CTK_TEXT_INPUT_CONTENT_HINT_TITLECASE;
  if (input_hints & CTK_INPUT_HINT_UPPERCASE_SENTENCES)
    hints |= CTK_TEXT_INPUT_CONTENT_HINT_AUTO_CAPITALIZATION;

  if (purpose == CTK_INPUT_PURPOSE_PIN ||
      purpose == CTK_INPUT_PURPOSE_PASSWORD)
    {
      hints |= (CTK_TEXT_INPUT_CONTENT_HINT_HIDDEN_TEXT |
                CTK_TEXT_INPUT_CONTENT_HINT_SENSITIVE_DATA);
    }

  return hints;
}

static uint32_t
translate_purpose (CtkInputPurpose purpose)
{
  switch (purpose)
    {
    case CTK_INPUT_PURPOSE_FREE_FORM:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;
    case CTK_INPUT_PURPOSE_ALPHA:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_ALPHA;
    case CTK_INPUT_PURPOSE_DIGITS:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_DIGITS;
    case CTK_INPUT_PURPOSE_NUMBER:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_NUMBER;
    case CTK_INPUT_PURPOSE_PHONE:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_PHONE;
    case CTK_INPUT_PURPOSE_URL:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_URL;
    case CTK_INPUT_PURPOSE_EMAIL:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_EMAIL;
    case CTK_INPUT_PURPOSE_NAME:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_NAME;
    case CTK_INPUT_PURPOSE_PASSWORD:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD;
    case CTK_INPUT_PURPOSE_PIN:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_PIN;
    case CTK_INPUT_PURPOSE_TERMINAL:
      return CTK_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;
    }

  return CTK_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;
}

static void
notify_content_type (CtkIMContextWayland *context)
{
  CtkInputHints hints;
  CtkInputPurpose purpose;

  if (global->current != CTK_IM_CONTEXT (context))
    return;

  g_object_get (context,
                "input-hints", &hints,
                "input-purpose", &purpose,
                NULL);

  ctk_text_input_set_content_type (global->text_input,
                                   translate_hints (hints, purpose),
                                   translate_purpose (purpose));
}

static void
commit_state (CtkIMContextWayland *context)
{
  if (global->current != CTK_IM_CONTEXT (context))
    return;
  ctk_text_input_commit (global->text_input);
}

static void
enable_text_input (CtkIMContextWayland *context,
                   gboolean             toggle_panel)
{
  guint flags = 0;

  if (context->use_preedit)
    flags |= CTK_TEXT_INPUT_ENABLE_FLAGS_CAN_SHOW_PREEDIT;
  if (toggle_panel)
    flags |= CTK_TEXT_INPUT_ENABLE_FLAGS_TOGGLE_INPUT_PANEL;

  ctk_text_input_enable (global->text_input,
                         global->enter_serial,
                         flags);
}

static void
ctk_im_context_wayland_finalize (GObject *object)
{
  CtkIMContextWayland *context = CTK_IM_CONTEXT_WAYLAND (object);

  g_clear_object (&context->window);
  g_clear_object (&context->gesture);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
pressed_cb (CtkGestureMultiPress *gesture G_GNUC_UNUSED,
            gint                  n_press,
            gdouble               x,
            gdouble               y,
            CtkIMContextWayland  *context)
{
  if (n_press == 1)
    {
      context->press_x = x;
      context->press_y = y;
    }
}

static void
released_cb (CtkGestureMultiPress *gesture G_GNUC_UNUSED,
             gint                  n_press,
             gdouble               x,
             gdouble               y,
             CtkIMContextWayland  *context)
{
  CtkInputHints hints;

  if (!global->current)
    return;

  g_object_get (context, "input-hints", &hints, NULL);

  if (n_press == 1 &&
      (hints & CTK_INPUT_HINT_INHIBIT_OSK) == 0 &&
      !ctk_drag_check_threshold (context->widget,
                                 context->press_x,
                                 context->press_y,
                                 x, y))
    {
      enable_text_input (CTK_IM_CONTEXT_WAYLAND (context), TRUE);
    }
}

static void
ctk_im_context_wayland_set_client_window (CtkIMContext *context,
                                          CdkWindow    *window)
{
  CtkIMContextWayland *context_wayland = CTK_IM_CONTEXT_WAYLAND (context);
  CtkWidget *widget = NULL;

  if (window == context_wayland->window)
    return;

  if (window)
    cdk_window_get_user_data (window, (gpointer*) &widget);

  if (context_wayland->widget && context_wayland->widget != widget)
    g_clear_object (&context_wayland->gesture);

  g_set_object (&context_wayland->window, window);

  if (context_wayland->widget != widget)
    {
      context_wayland->widget = widget;

      if (widget)
        {
          CtkGesture *gesture;

          gesture = ctk_gesture_multi_press_new (widget);
          ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                                      CTK_PHASE_CAPTURE);
          g_signal_connect (gesture, "pressed",
                            G_CALLBACK (pressed_cb), context);
          g_signal_connect (gesture, "released",
                            G_CALLBACK (released_cb), context);
          context_wayland->gesture = gesture;
        }
    }
}

static void
ctk_im_context_wayland_get_preedit_string (CtkIMContext   *context,
                                           gchar         **str,
                                           PangoAttrList **attrs,
                                           gint           *cursor_pos)
{
  CtkIMContextWayland *context_wayland = CTK_IM_CONTEXT_WAYLAND (context);
  gchar *preedit_str;

  if (attrs)
    *attrs = NULL;

  CTK_IM_CONTEXT_CLASS (parent_class)->get_preedit_string (context, str, attrs, cursor_pos);

  /* If the parent implementation returns a len>0 string, go with it */
  if (str && *str)
    {
      if (**str)
        return;

      g_free (*str);
    }

  preedit_str =
    context_wayland->preedit.text ? context_wayland->preedit.text : "";

  if (str)
    *str = g_strdup (preedit_str);
  if (cursor_pos)
    *cursor_pos = context_wayland->preedit.cursor_idx;

  if (attrs)
    {
      if (!*attrs)
        *attrs = pango_attr_list_new ();
      pango_attr_list_insert (*attrs,
                              pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));
    }
}

static gboolean
ctk_im_context_wayland_filter_keypress (CtkIMContext *context,
                                        CdkEventKey  *key)
{
  /* This is done by the compositor */
  return CTK_IM_CONTEXT_CLASS (parent_class)->filter_keypress (context, key);
}

static void
ctk_im_context_wayland_focus_in (CtkIMContext *context)
{
  CtkIMContextWayland *context_wayland = CTK_IM_CONTEXT_WAYLAND (context);

  if (global->current == context)
    return;
  if (!global->text_input)
    return;

  global->current = context;
  enable_text_input (context_wayland, FALSE);
  notify_content_type (context_wayland);
  notify_surrounding_text (context_wayland);
  notify_cursor_location (context_wayland);
  commit_state (context_wayland);
}

static void
ctk_im_context_wayland_focus_out (CtkIMContext *context)
{
  if (global->current != context)
    return;

  ctk_text_input_disable (global->text_input);
  global->current = NULL;
}

static void
ctk_im_context_wayland_reset (CtkIMContext *context)
{
  reset_preedit (CTK_IM_CONTEXT_WAYLAND (context));

  CTK_IM_CONTEXT_CLASS (parent_class)->reset (context);
}

static void
ctk_im_context_wayland_set_cursor_location (CtkIMContext *context,
                                            CdkRectangle *rect)
{
  CtkIMContextWayland *context_wayland;

  context_wayland = CTK_IM_CONTEXT_WAYLAND (context);

  context_wayland->cursor_rect = *rect;
  notify_cursor_location (context_wayland);
  commit_state (context_wayland);
}

static void
ctk_im_context_wayland_set_use_preedit (CtkIMContext *context,
                                        gboolean      use_preedit)
{
  CtkIMContextWayland *context_wayland = CTK_IM_CONTEXT_WAYLAND (context);

  context_wayland->use_preedit = !!use_preedit;
}

static void
ctk_im_context_wayland_set_surrounding (CtkIMContext *context,
                                        const gchar  *text,
                                        gint          len,
                                        gint          cursor_index)
{
  CtkIMContextWayland *context_wayland;

  context_wayland = CTK_IM_CONTEXT_WAYLAND (context);

  g_free (context_wayland->surrounding.text);
  context_wayland->surrounding.text = g_strndup (text, len);
  context_wayland->surrounding.cursor_idx = cursor_index;

  notify_surrounding_text (context_wayland);
  commit_state (context_wayland);
}

static gboolean
ctk_im_context_wayland_get_surrounding (CtkIMContext  *context,
                                        gchar        **text,
                                        gint          *cursor_index)
{
  CtkIMContextWayland *context_wayland;

  context_wayland = CTK_IM_CONTEXT_WAYLAND (context);

  if (!context_wayland->surrounding.text)
    return FALSE;

  *text = context_wayland->surrounding.text;
  *cursor_index = context_wayland->surrounding.cursor_idx;
  return TRUE;
}

static void
ctk_im_context_wayland_class_init (CtkIMContextWaylandClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkIMContextClass *im_context_class = CTK_IM_CONTEXT_CLASS (klass);

  object_class->finalize = ctk_im_context_wayland_finalize;

  im_context_class->set_client_window = ctk_im_context_wayland_set_client_window;
  im_context_class->get_preedit_string = ctk_im_context_wayland_get_preedit_string;
  im_context_class->filter_keypress = ctk_im_context_wayland_filter_keypress;
  im_context_class->focus_in = ctk_im_context_wayland_focus_in;
  im_context_class->focus_out = ctk_im_context_wayland_focus_out;
  im_context_class->reset = ctk_im_context_wayland_reset;
  im_context_class->set_cursor_location = ctk_im_context_wayland_set_cursor_location;
  im_context_class->set_use_preedit = ctk_im_context_wayland_set_use_preedit;
  im_context_class->set_surrounding = ctk_im_context_wayland_set_surrounding;
  im_context_class->get_surrounding = ctk_im_context_wayland_get_surrounding;

  parent_class = g_type_class_peek_parent (klass);
}

static void
on_content_type_changed (CtkIMContextWayland *context)
{
  notify_content_type (context);
  commit_state (context);
}

static void
ctk_im_context_wayland_init (CtkIMContextWayland *context)
{
  context->use_preedit = TRUE;
  g_signal_connect_swapped (context, "notify::input-purpose",
                            G_CALLBACK (on_content_type_changed), context);
  g_signal_connect_swapped (context, "notify::input-hints",
                            G_CALLBACK (on_content_type_changed), context);
}

static void
ctk_im_context_wayland_register_type (GTypeModule *module)
{
  const GTypeInfo object_info = {
    .class_size = sizeof (CtkIMContextWaylandClass),
    .class_init = (GClassInitFunc) ctk_im_context_wayland_class_init,
    .instance_size = sizeof (CtkIMContextWayland),
    .n_preallocs = 0,
    .instance_init = (GInstanceInitFunc) ctk_im_context_wayland_init,
  };

  type_wayland = g_type_module_register_type (module,
                                              CTK_TYPE_IM_CONTEXT_SIMPLE,
                                              "CtkIMContextWayland",
                                              &object_info, 0);
}

MODULE_ENTRY (void, init) (GTypeModule * module)
{
  ctk_im_context_wayland_register_type (module);
  ctk_im_context_wayland_global_init (cdk_display_get_default ());
}

MODULE_ENTRY (void, exit) (void)
{
}

MODULE_ENTRY (void, list) (const CtkIMContextInfo *** contexts, int *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (CtkIMContext *, create) (const gchar * context_id)
{
  if (strcmp (context_id, "waylandctk") == 0)
    return g_object_new (type_wayland, NULL);
  else
    return NULL;
}
