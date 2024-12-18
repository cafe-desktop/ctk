/* ctkemojicompletion.c: An Emoji picker widget
 * Copyright 2017, Red Hat, Inc.
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

#include "ctkemojicompletion.h"

#include "ctkentryprivate.h"
#include "ctkbox.h"
#include "ctkcssprovider.h"
#include "ctklistbox.h"
#include "ctklabel.h"
#include "ctkpopover.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkgesturelongpress.h"
#include "ctkflowbox.h"
#include "ctkstack.h"

struct _CtkEmojiCompletion
{
  CtkPopover parent_instance;

  CtkEntry *entry;
  char *text;
  guint length;
  guint offset;
  gulong changed_id;
  guint n_matches;

  CtkWidget *list;
  CtkWidget *active;
  CtkWidget *active_variation;

  GVariant *data;

  CtkGesture *long_press;
};

struct _CtkEmojiCompletionClass {
  CtkPopoverClass parent_class;
};

static void connect_signals    (CtkEmojiCompletion *completion,
                                CtkEntry           *entry);
static void disconnect_signals (CtkEmojiCompletion *completion);
static int populate_completion (CtkEmojiCompletion *completion,
                                const char          *text,
                                guint                offset);

#define MAX_ROWS 5

G_DEFINE_TYPE (CtkEmojiCompletion, ctk_emoji_completion, CTK_TYPE_POPOVER)

static void
ctk_emoji_completion_finalize (GObject *object)
{
  CtkEmojiCompletion *completion = CTK_EMOJI_COMPLETION (object);

  disconnect_signals (completion);

  g_free (completion->text);
  g_variant_unref (completion->data);

  g_clear_object (&completion->long_press);

  G_OBJECT_CLASS (ctk_emoji_completion_parent_class)->finalize (object);
}

static void
update_completion (CtkEmojiCompletion *completion)
{
  const char *text;
  guint length;
  guint n_matches;

  n_matches = 0;

  text = ctk_entry_get_text (CTK_ENTRY (completion->entry));
  length = strlen (text);

  if (length > 0)
    {
      gboolean found_candidate = FALSE;
      const char *p;

      p = text + length;
      do
        {
next:
          p = g_utf8_prev_char (p);
          if (*p == ':')
            {
              if (p + 1 == text + length)
                goto next;

              if (p == text || !g_unichar_isalnum (g_utf8_get_char (p - 1)))
                {
                  found_candidate = TRUE;
                }

              break;
            }
        }
      while (g_unichar_isalnum (g_utf8_get_char (p)) || *p == '_');

      if (found_candidate)
        n_matches = populate_completion (completion, p, 0);
    }

  if (n_matches > 0)
    ctk_popover_popup (CTK_POPOVER (completion));
  else
    ctk_popover_popdown (CTK_POPOVER (completion));
}

static void
entry_changed (CtkEntry           *entry G_GNUC_UNUSED,
               CtkEmojiCompletion *completion)
{
  update_completion (completion);
}

static void
emoji_activated (CtkWidget          *row,
                 CtkEmojiCompletion *completion)
{
  const char *emoji;
  guint length;

  ctk_popover_popdown (CTK_POPOVER (completion));

  emoji = (const char *)g_object_get_data (G_OBJECT (row), "text");

  g_signal_handler_block (completion->entry, completion->changed_id);

  length = g_utf8_strlen (ctk_entry_get_text (completion->entry), -1);
  ctk_entry_set_positions (completion->entry, length - completion->length, length);
  ctk_entry_enter_text (completion->entry, emoji);

  g_signal_handler_unblock (completion->entry, completion->changed_id);
}

static void
row_activated (CtkListBox    *list G_GNUC_UNUSED,
               CtkListBoxRow *row,
               gpointer       data)
{
  CtkEmojiCompletion *completion = data;

  emoji_activated (CTK_WIDGET (row), completion);
}

static void
child_activated (CtkFlowBox      *box G_GNUC_UNUSED,
                 CtkFlowBoxChild *child,
                 gpointer         data)
{
  CtkEmojiCompletion *completion = data;

  emoji_activated (CTK_WIDGET (child), completion);
}

static void
move_active_row (CtkEmojiCompletion *completion,
                 int                 direction)
{
  CtkWidget *child;
  CtkWidget *base;
  GList *children, *l, *active, *last;

  active = NULL;
  last = NULL;
  children = ctk_container_get_children (CTK_CONTAINER (completion->list));
  for (l = children; l; l = l->next)
    {
      child = l->data;

      if (completion->active == child)
        active = l;

      if (l->next == NULL)
        last = l;

      ctk_widget_unset_state_flags (child, CTK_STATE_FLAG_PRELIGHT);
      base = CTK_WIDGET (g_object_get_data (G_OBJECT (child), "base"));
      ctk_widget_unset_state_flags (base, CTK_STATE_FLAG_PRELIGHT);
    }

  if (completion->active != NULL)
    {
      if (direction == 1)
        completion->active = (active && active->next) ? active->next->data : NULL;
      else
        completion->active = (active && active->prev) ? active->prev->data : NULL;
    }

  if (completion->active == NULL)
    {
      if (direction == 1)
        completion->active = children->data;
      else
        completion->active = last->data;
    }

  if (completion->active != NULL)
    ctk_widget_set_state_flags (completion->active, CTK_STATE_FLAG_PRELIGHT, FALSE);

  if (completion->active_variation)
    {
      ctk_widget_unset_state_flags (completion->active_variation, CTK_STATE_FLAG_PRELIGHT);
      completion->active_variation = NULL;
    }

  g_list_free (children);
}

static void
activate_active_row (CtkEmojiCompletion *completion)
{
  if (CTK_IS_FLOW_BOX_CHILD (completion->active_variation))
    emoji_activated (completion->active_variation, completion);
  else if (completion->active != NULL)
    emoji_activated (completion->active, completion);
}

static void
show_variations (CtkEmojiCompletion *completion,
                 CtkWidget          *row,
                 gboolean            visible)
{
  CtkWidget *stack;
  CtkWidget *box;
  gboolean is_visible;

  if (!row)
    return;

  stack = CTK_WIDGET (g_object_get_data (G_OBJECT (row), "stack"));
  box = ctk_stack_get_child_by_name (CTK_STACK (stack), "variations");
  if (!box)
    return;

  is_visible = ctk_stack_get_visible_child (CTK_STACK (stack)) == box;
  if (is_visible == visible)
    return;

  if (visible)
    ctk_widget_unset_state_flags (row, CTK_STATE_FLAG_PRELIGHT);
  else
    ctk_widget_set_state_flags (row, CTK_STATE_FLAG_PRELIGHT, FALSE);

  ctk_stack_set_visible_child_name (CTK_STACK (stack), visible ? "variations" : "text");
  if (completion->active_variation)
    {
      ctk_widget_unset_state_flags (completion->active_variation, CTK_STATE_FLAG_PRELIGHT);
      completion->active_variation = NULL;
    }
}

static gboolean
move_active_variation (CtkEmojiCompletion *completion,
                       int                 direction)
{
  CtkWidget *base;
  CtkWidget *stack;
  CtkWidget *box;
  CtkWidget *next;
  GList *children, *l, *active;

  if (!completion->active)
    return FALSE;

  base = CTK_WIDGET (g_object_get_data (G_OBJECT (completion->active), "base"));
  stack = CTK_WIDGET (g_object_get_data (G_OBJECT (completion->active), "stack"));
  box = ctk_stack_get_child_by_name (CTK_STACK (stack), "variations");

  if (ctk_stack_get_visible_child (CTK_STACK (stack)) != box)
    return FALSE;

  next = NULL;

  active = NULL;
  children = ctk_container_get_children (CTK_CONTAINER (box));
  for (l = children; l; l = l->next)
    {
      if (l->data == completion->active_variation)
        active = l;
    }

  if (!completion->active_variation)
    next = base;
  else if (completion->active_variation == base && direction == 1)
    next = children->data;
  else if (completion->active_variation == children->data && direction == -1)
    next = base;
  else if (direction == 1)
    next = (active && active->next) ? active->next->data : NULL;
  else if (direction == -1)
    next = (active && active->prev) ? active->prev->data : NULL;

  if (next)
    {
      if (completion->active_variation)
        ctk_widget_unset_state_flags (completion->active_variation, CTK_STATE_FLAG_PRELIGHT);
      completion->active_variation = next;
      ctk_widget_set_state_flags (completion->active_variation, CTK_STATE_FLAG_PRELIGHT, FALSE);
    }

  g_list_free (children);

  return next != NULL;
}

static gboolean
entry_key_press (CtkEntry           *entry G_GNUC_UNUSED,
                 CdkEventKey        *event,
                 CtkEmojiCompletion *completion)
{
  guint keyval;

  if (!ctk_widget_get_visible (CTK_WIDGET (completion)))
    return FALSE;

  cdk_event_get_keyval ((CdkEvent*)event, &keyval);

  if (keyval == CDK_KEY_Escape)
    {
      ctk_popover_popdown (CTK_POPOVER (completion));
      return TRUE;
    }

  if (keyval == CDK_KEY_Tab)
    {
      guint offset;
      show_variations (completion, completion->active, FALSE);

      offset = completion->offset + MAX_ROWS;
      if (offset >= completion->n_matches)
        offset = 0;
      populate_completion (completion, completion->text, offset);
      return TRUE;
    }

  if (keyval == CDK_KEY_Up)
    {
      show_variations (completion, completion->active, FALSE);

      move_active_row (completion, -1);
      return TRUE;
    }

  if (keyval == CDK_KEY_Down)
    {
      show_variations (completion, completion->active, FALSE);

      move_active_row (completion, 1);
      return TRUE;
    }

  if (keyval == CDK_KEY_Return ||
      keyval == CDK_KEY_KP_Enter ||
      keyval == CDK_KEY_ISO_Enter)
    {
      activate_active_row (completion);
      return TRUE;
    }

  if (keyval == CDK_KEY_Right)
    {
      show_variations (completion, completion->active, TRUE);
      move_active_variation (completion, 1);
      return TRUE;
    }

  if (keyval == CDK_KEY_Left)
    {
      if (!move_active_variation (completion, -1))
        show_variations (completion, completion->active, FALSE);
      return TRUE;
    }

  return FALSE;
}

static gboolean
entry_focus_out (CtkWidget          *entry,
                 GParamSpec         *pspec G_GNUC_UNUSED,
                 CtkEmojiCompletion *completion)
{
  if (!ctk_widget_has_focus (entry))
    ctk_popover_popdown (CTK_POPOVER (completion));
  return FALSE;
}

static void
connect_signals (CtkEmojiCompletion *completion,
                 CtkEntry           *entry)
{
  completion->entry = entry;

  completion->changed_id = g_signal_connect (entry, "changed", G_CALLBACK (entry_changed), completion);
  g_signal_connect (entry, "key-press-event", G_CALLBACK (entry_key_press), completion);
  g_signal_connect (entry, "notify::has-focus", G_CALLBACK (entry_focus_out), completion);
}

static void
disconnect_signals (CtkEmojiCompletion *completion)
{
  g_signal_handlers_disconnect_by_func (completion->entry, entry_changed, completion);
  g_signal_handlers_disconnect_by_func (completion->entry, entry_key_press, completion);
  g_signal_handlers_disconnect_by_func (completion->entry, entry_focus_out, completion);

  completion->entry = NULL;
}

static gboolean
has_variations (GVariant *emoji_data)
{
  GVariant *codes;
  gsize i;
  gboolean has_variations;

  has_variations = FALSE;
  codes = g_variant_get_child_value (emoji_data, 0);
  for (i = 0; i < g_variant_n_children (codes); i++)
    {
      gunichar code;
      g_variant_get_child (codes, i, "u", &code);
      if (code == 0)
        {
          has_variations = TRUE;
          break;
        }
    }
  g_variant_unref (codes);

  return has_variations;
}

static void
get_text (GVariant *emoji_data,
          gunichar  modifier,
          char     *text,
          gsize     length G_GNUC_UNUSED)
{
  GVariant *codes;
  int i;
  char *p;

  p = text;
  codes = g_variant_get_child_value (emoji_data, 0);
  for (i = 0; i < g_variant_n_children (codes); i++)
    {
      gunichar code;

      g_variant_get_child (codes, i, "u", &code);
      if (code == 0)
        code = modifier;
      if (code != 0)
        p += g_unichar_to_utf8 (code, p);
    }
  g_variant_unref (codes);
  p += g_unichar_to_utf8 (0xFE0F, p); /* U+FE0F is the Emoji variation selector */
  p[0] = 0;
}

static void
add_emoji_variation (CtkWidget *box,
                     GVariant  *emoji_data,
                     gunichar   modifier)
{
  CtkWidget *child;
  CtkWidget *label;
  PangoAttrList *attrs;
  char text[64];

  get_text (emoji_data, modifier, text, 64);

  label = ctk_label_new (text);
  ctk_widget_show (label);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
  ctk_label_set_attributes (CTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

  child = ctk_flow_box_child_new ();
  ctk_widget_show (child);
  ctk_style_context_add_class (ctk_widget_get_style_context (child), "emoji");
  g_object_set_data_full (G_OBJECT (child), "text", g_strdup (text), g_free);
  g_object_set_data_full (G_OBJECT (child), "emoji-data",
                          g_variant_ref (emoji_data),
                          (GDestroyNotify)g_variant_unref);
  if (modifier != 0)
    g_object_set_data (G_OBJECT (child), "modifier", GUINT_TO_POINTER (modifier));

  ctk_container_add (CTK_CONTAINER (child), label);
  ctk_flow_box_insert (CTK_FLOW_BOX (box), child, -1);
}

static void
add_emoji (CtkWidget          *list,
           GVariant           *emoji_data,
           CtkEmojiCompletion *completion)
{
  CtkWidget *child;
  CtkWidget *label;
  CtkWidget *box;
  PangoAttrList *attrs;
  char text[64];
  const char *shortname;
  CtkWidget *stack;
  gunichar modifier;

  get_text (emoji_data, 0, text, 64);

  label = ctk_label_new (text);
  ctk_widget_show (label);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
  ctk_label_set_attributes (CTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);
  ctk_style_context_add_class (ctk_widget_get_style_context (label), "emoji");

  child = ctk_list_box_row_new ();
  ctk_widget_show (child);
  ctk_widget_set_focus_on_click (child, FALSE);
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_widget_show (box);
  ctk_container_add (CTK_CONTAINER (child), box);
  ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (child), "base", label);

  stack = ctk_stack_new ();
  ctk_widget_show (stack);
  ctk_stack_set_homogeneous (CTK_STACK (stack), TRUE);
  ctk_stack_set_transition_type (CTK_STACK (stack), CTK_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT);
  ctk_box_pack_start (CTK_BOX (box), stack, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (child), "stack", stack);

  g_variant_get_child (emoji_data, 2, "&s", &shortname);
  label = ctk_label_new (shortname);
  ctk_widget_show (label);
  ctk_label_set_xalign (CTK_LABEL (label), 0);

  ctk_stack_add_named (CTK_STACK (stack), label, "text");

  if (has_variations (emoji_data))
    {
      box = ctk_flow_box_new ();
      ctk_widget_show (box);
      ctk_flow_box_set_homogeneous (CTK_FLOW_BOX (box), TRUE);
      ctk_flow_box_set_min_children_per_line (CTK_FLOW_BOX (box), 5);
      ctk_flow_box_set_max_children_per_line (CTK_FLOW_BOX (box), 5);
      ctk_flow_box_set_activate_on_single_click (CTK_FLOW_BOX (box), TRUE);
      ctk_flow_box_set_selection_mode (CTK_FLOW_BOX (box), CTK_SELECTION_NONE);
      g_signal_connect (box, "child-activated", G_CALLBACK (child_activated), completion);
      for (modifier = 0x1f3fb; modifier <= 0x1f3ff; modifier++)
        add_emoji_variation (box, emoji_data, modifier);

      ctk_stack_add_named (CTK_STACK (stack), box, "variations");
    }

  g_object_set_data_full (G_OBJECT (child), "text", g_strdup (text), g_free);
  g_object_set_data_full (G_OBJECT (child), "emoji-data",
                          g_variant_ref (emoji_data), (GDestroyNotify)g_variant_unref);
  ctk_style_context_add_class (ctk_widget_get_style_context (child), "emoji-completion-row");

  ctk_list_box_insert (CTK_LIST_BOX (list), child, -1);
}

static int
populate_completion (CtkEmojiCompletion *completion,
                     const char          *text,
                     guint                offset)
{
  GList *children, *l;
  guint n_matches;
  guint n_added;
  GVariantIter iter;
  GVariant *item;

  g_free (completion->text);
  completion->text = g_strdup (text);
  completion->length = g_utf8_strlen (text, -1);
  completion->offset = offset;

  children = ctk_container_get_children (CTK_CONTAINER (completion->list));
  for (l = children; l; l = l->next)
    ctk_widget_destroy (CTK_WIDGET (l->data));
  g_list_free (children);

  completion->active = NULL;

  n_matches = 0;
  n_added = 0;
  g_variant_iter_init (&iter, completion->data);
  while ((item = g_variant_iter_next_value (&iter)))
    {
      const char *shortname;

      g_variant_get_child (item, 2, "&s", &shortname);
      if (g_str_has_prefix (shortname, text))
        {
          n_matches++;

          if (n_matches > offset && n_added < MAX_ROWS)
            {
              add_emoji (completion->list, item, completion);
              n_added++;
            }
        }
    }

  completion->n_matches = n_matches;

  if (n_added > 0)
    {
      GList *children;

      children = ctk_container_get_children (CTK_CONTAINER (completion->list));
      completion->active = children->data;
      g_list_free (children);
      ctk_widget_set_state_flags (completion->active, CTK_STATE_FLAG_PRELIGHT, FALSE);
    }

  return n_added;
}

static void
long_pressed_cb (CtkGesture *gesture G_GNUC_UNUSED,
                 double      x G_GNUC_UNUSED,
                 double      y,
                 gpointer    data)
{
  CtkEmojiCompletion *completion = data;
  CtkWidget *row;

  row = CTK_WIDGET (ctk_list_box_get_row_at_y (CTK_LIST_BOX (completion->list), y));
  if (!row)
    return;

  show_variations (completion, row, TRUE);
}

static void
ctk_emoji_completion_init (CtkEmojiCompletion *completion)
{
  GBytes *bytes = NULL;

  ctk_widget_init_template (CTK_WIDGET (completion));

  bytes = g_resources_lookup_data ("/org/ctk/libctk/emoji/emoji.data", 0, NULL);
  completion->data = g_variant_ref_sink (g_variant_new_from_bytes (G_VARIANT_TYPE ("a(auss)"), bytes, TRUE));
  g_bytes_unref (bytes);

  completion->long_press = ctk_gesture_long_press_new (completion->list);
  g_signal_connect (completion->long_press, "pressed", G_CALLBACK (long_pressed_cb), completion);
}

static void
ctk_emoji_completion_class_init (CtkEmojiCompletionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->finalize = ctk_emoji_completion_finalize;

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkemojicompletion.ui");

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiCompletion, list);

  ctk_widget_class_bind_template_callback (widget_class, row_activated);
}

CtkWidget *
ctk_emoji_completion_new (CtkEntry *entry)
{
  CtkEmojiCompletion *completion;

  completion = CTK_EMOJI_COMPLETION (g_object_new (CTK_TYPE_EMOJI_COMPLETION,
                                                   "relative-to", entry,
                                                   NULL));

  connect_signals (completion, entry);

  return CTK_WIDGET (completion);
}
