/* ctkemojichooser.c: An Emoji chooser widget
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

#include "ctkemojichooser.h"

#include "ctkadjustmentprivate.h"
#include "ctkbox.h"
#include "ctkbutton.h"
#include "ctkcssprovider.h"
#include "ctkentry.h"
#include "ctkflowbox.h"
#include "ctkstack.h"
#include "ctklabel.h"
#include "ctkgesturelongpress.h"
#include "ctkgesturemultipress.h"
#include "ctkpopover.h"
#include "ctkscrolledwindow.h"
#include "ctkeventbox.h"
#include "ctkintl.h"
#include "ctkprivate.h"

#define BOX_SPACE 6

typedef struct {
  CtkWidget *box;
  CtkWidget *heading;
  CtkWidget *button;
  const char *first;
  gunichar label;
  gboolean empty;
} EmojiSection;

struct _CtkEmojiChooser
{
  CtkPopover parent_instance;

  CtkWidget *search_entry;
  CtkWidget *stack;
  CtkWidget *scrolled_window;

  int emoji_max_width;

  EmojiSection recent;
  EmojiSection people;
  EmojiSection body;
  EmojiSection nature;
  EmojiSection food;
  EmojiSection travel;
  EmojiSection activities;
  EmojiSection objects;
  EmojiSection symbols;
  EmojiSection flags;

  CtkGesture *recent_long_press;
  CtkGesture *recent_multi_press;
  CtkGesture *people_long_press;
  CtkGesture *people_multi_press;
  CtkGesture *body_long_press;
  CtkGesture *body_multi_press;

  GVariant *data;
  CtkWidget *box;
  GVariantIter *iter;
  guint populate_idle;

  GSettings *settings;
};

struct _CtkEmojiChooserClass {
  CtkPopoverClass parent_class;
};

enum {
  EMOJI_PICKED,
  LAST_SIGNAL
};

static int signals[LAST_SIGNAL];

G_DEFINE_TYPE (CtkEmojiChooser, ctk_emoji_chooser, CTK_TYPE_POPOVER)

static void
ctk_emoji_chooser_finalize (GObject *object)
{
  CtkEmojiChooser *chooser = CTK_EMOJI_CHOOSER (object);

  if (chooser->populate_idle)
    g_source_remove (chooser->populate_idle);

  g_variant_unref (chooser->data);
  g_object_unref (chooser->settings);

  g_clear_object (&chooser->recent_long_press);
  g_clear_object (&chooser->recent_multi_press);
  g_clear_object (&chooser->people_long_press);
  g_clear_object (&chooser->people_multi_press);
  g_clear_object (&chooser->body_long_press);
  g_clear_object (&chooser->body_multi_press);

  G_OBJECT_CLASS (ctk_emoji_chooser_parent_class)->finalize (object);
}

static void
scroll_to_section (CtkButton *button,
                   gpointer   data)
{
  EmojiSection *section = data;
  CtkEmojiChooser *chooser;
  CtkAdjustment *adj;
  CtkAllocation alloc = { 0, 0, 0, 0 };

  chooser = CTK_EMOJI_CHOOSER (ctk_widget_get_ancestor (CTK_WIDGET (button), CTK_TYPE_EMOJI_CHOOSER));

  adj = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (chooser->scrolled_window));

  if (section->heading)
    ctk_widget_get_allocation (section->heading, &alloc);

  ctk_adjustment_animate_to_value (adj, alloc.y - BOX_SPACE);
}

static void
add_emoji (CtkWidget    *box,
           gboolean      prepend,
           GVariant     *item,
           gunichar      modifier,
           CtkEmojiChooser *chooser);

#define MAX_RECENT (7*3)

static void
populate_recent_section (CtkEmojiChooser *chooser)
{
  GVariant *variant;
  GVariant *item;
  GVariantIter iter;
  gboolean empty = FALSE;

  variant = g_settings_get_value (chooser->settings, "recent-emoji");
  g_variant_iter_init (&iter, variant);
  while ((item = g_variant_iter_next_value (&iter)))
    {
      GVariant *emoji_data;
      gunichar modifier;

      emoji_data = g_variant_get_child_value (item, 0);
      g_variant_get_child (item, 1, "u", &modifier);
      add_emoji (chooser->recent.box, FALSE, emoji_data, modifier, chooser);
      g_variant_unref (emoji_data);
      g_variant_unref (item);
      empty = FALSE;
    }

  if (!empty)
    {
      ctk_widget_show (chooser->recent.box);
      ctk_widget_set_sensitive (chooser->recent.button, TRUE);
    }
  g_variant_unref (variant);
}

static void
add_recent_item (CtkEmojiChooser *chooser,
                 GVariant        *item,
                 gunichar         modifier)
{
  GList *children, *l;
  int i;
  GVariantBuilder builder;

  g_variant_ref (item);

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a((auss)u)"));
  g_variant_builder_add (&builder, "(@(auss)u)", item, modifier);

  children = ctk_container_get_children (CTK_CONTAINER (chooser->recent.box));
  for (l = children, i = 1; l; l = l->next, i++)
    {
      GVariant *item2 = g_object_get_data (G_OBJECT (l->data), "emoji-data");
      gunichar modifier2 = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (l->data), "modifier"));

      if (modifier == modifier2 && g_variant_equal (item, item2))
        {
          ctk_widget_destroy (CTK_WIDGET (l->data));
          i--;
          continue;
        }
      if (i >= MAX_RECENT)
        {
          ctk_widget_destroy (CTK_WIDGET (l->data));
          continue;
        }

      g_variant_builder_add (&builder, "(@(auss)u)", item2, modifier2);
    }
  g_list_free (children);

  add_emoji (chooser->recent.box, TRUE, item, modifier, chooser);

  /* Enable recent */
  ctk_widget_show (chooser->recent.box);
  ctk_widget_set_sensitive (chooser->recent.button, TRUE);

  g_settings_set_value (chooser->settings, "recent-emoji", g_variant_builder_end (&builder));

  g_variant_unref (item);
}

static void
emoji_activated (CtkFlowBox      *box,
                 CtkFlowBoxChild *child,
                 gpointer         data)
{
  CtkEmojiChooser *chooser = data;
  char *text;
  CtkWidget *ebox;
  CtkWidget *label;
  GVariant *item;
  gunichar modifier;

  ctk_popover_popdown (CTK_POPOVER (chooser));

  ebox = ctk_bin_get_child (CTK_BIN (child));
  label = ctk_bin_get_child (CTK_BIN (ebox));
  text = g_strdup (ctk_label_get_label (CTK_LABEL (label)));

  item = (GVariant*) g_object_get_data (G_OBJECT (child), "emoji-data");
  modifier = (gunichar) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (child), "modifier"));
  add_recent_item (chooser, item, modifier);

  g_signal_emit (data, signals[EMOJI_PICKED], 0, text);
  g_free (text);
}

static gboolean
has_variations (GVariant *emoji_data)
{
  GVariant *codes;
  int i;
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
show_variations (CtkEmojiChooser *chooser,
                 CtkWidget       *child)
{
  CtkWidget *popover;
  CtkWidget *view;
  CtkWidget *box;
  GVariant *emoji_data;
  CtkWidget *parent_popover;
  gunichar modifier;

  if (!child)
    return;

  emoji_data = (GVariant*) g_object_get_data (G_OBJECT (child), "emoji-data");
  if (!emoji_data)
    return;

  if (!has_variations (emoji_data))
    return;

  parent_popover = ctk_widget_get_ancestor (child, CTK_TYPE_POPOVER);
  popover = ctk_popover_new (child);
  view = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_style_context_add_class (ctk_widget_get_style_context (view), "view");
  box = ctk_flow_box_new ();
  ctk_flow_box_set_homogeneous (CTK_FLOW_BOX (box), TRUE);
  ctk_flow_box_set_min_children_per_line (CTK_FLOW_BOX (box), 6);
  ctk_flow_box_set_max_children_per_line (CTK_FLOW_BOX (box), 6);
  ctk_flow_box_set_activate_on_single_click (CTK_FLOW_BOX (box), TRUE);
  ctk_flow_box_set_selection_mode (CTK_FLOW_BOX (box), CTK_SELECTION_NONE);
  ctk_container_add (CTK_CONTAINER (popover), view);
  ctk_container_add (CTK_CONTAINER (view), box);

  g_signal_connect (box, "child-activated", G_CALLBACK (emoji_activated), parent_popover);

  add_emoji (box, FALSE, emoji_data, 0, chooser);
  for (modifier = 0x1f3fb; modifier <= 0x1f3ff; modifier++)
    add_emoji (box, FALSE, emoji_data, modifier, chooser);

  ctk_widget_show_all (view);
  ctk_popover_popup (CTK_POPOVER (popover));
}

static void
update_hover (CtkWidget *widget,
              CdkEvent  *event,
              gpointer   data)
{
  if (event->type == GDK_ENTER_NOTIFY)
    ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_PRELIGHT, FALSE);
  else
    ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_PRELIGHT);
}

static void
long_pressed_cb (CtkGesture *gesture,
                 double      x,
                 double      y,
                 gpointer    data)
{
  CtkEmojiChooser *chooser = data;
  CtkWidget *box;
  CtkWidget *child;

  box = ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture));
  child = CTK_WIDGET (ctk_flow_box_get_child_at_pos (CTK_FLOW_BOX (box), x, y));
  show_variations (chooser, child);
}

static void
pressed_cb (CtkGesture *gesture,
            int         n_press,
            double      x,
            double      y,
            gpointer    data)
{
  CtkEmojiChooser *chooser = data;
  CtkWidget *box;
  CtkWidget *child;

  box = ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture));
  child = CTK_WIDGET (ctk_flow_box_get_child_at_pos (CTK_FLOW_BOX (box), x, y));
  show_variations (chooser, child);
}

static gboolean
popup_menu (CtkWidget *widget,
            gpointer   data)
{
  CtkEmojiChooser *chooser = data;

  show_variations (chooser, widget);
  return TRUE;
}

static void
add_emoji (CtkWidget    *box,
           gboolean      prepend,
           GVariant     *item,
           gunichar      modifier,
           CtkEmojiChooser *chooser)
{
  CtkWidget *child;
  CtkWidget *ebox;
  CtkWidget *label;
  PangoAttrList *attrs;
  GVariant *codes;
  char text[64];
  char *p = text;
  int i;
  PangoLayout *layout;
  PangoRectangle rect;

  codes = g_variant_get_child_value (item, 0);
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

  label = ctk_label_new (text);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
  ctk_label_set_attributes (CTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

  layout = ctk_label_get_layout (CTK_LABEL (label));
  pango_layout_get_extents (layout, &rect, NULL);

  /* Check for fallback rendering that generates too wide items */
  if (pango_layout_get_unknown_glyphs_count (layout) > 0 ||
      rect.width >= 1.5 * chooser->emoji_max_width)
    {
      ctk_widget_destroy (label);
      return;
    }

  child = ctk_flow_box_child_new ();
  ctk_style_context_add_class (ctk_widget_get_style_context (child), "emoji");
  g_object_set_data_full (G_OBJECT (child), "emoji-data",
                          g_variant_ref (item),
                          (GDestroyNotify)g_variant_unref);
  if (modifier != 0)
    g_object_set_data (G_OBJECT (child), "modifier", GUINT_TO_POINTER (modifier));

  ebox = ctk_event_box_new ();
  ctk_widget_add_events (ebox, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  g_signal_connect (ebox, "enter-notify-event", G_CALLBACK (update_hover), FALSE);
  g_signal_connect (ebox, "leave-notify-event", G_CALLBACK (update_hover), FALSE);
  ctk_container_add (CTK_CONTAINER (child), ebox);
  ctk_container_add (CTK_CONTAINER (ebox), label);
  ctk_widget_show_all (child);

  if (chooser)
    g_signal_connect (child, "popup-menu", G_CALLBACK (popup_menu), chooser);

  ctk_flow_box_insert (CTK_FLOW_BOX (box), child, prepend ? 0 : -1);
}

static gboolean
populate_emoji_chooser (gpointer data)
{
  CtkEmojiChooser *chooser = data;
  GBytes *bytes = NULL;
  GVariant *item;
  guint64 start, now;

  start = g_get_monotonic_time ();

  if (!chooser->data)
    {
      bytes = g_resources_lookup_data ("/org/ctk/libctk/emoji/emoji.data", 0, NULL);
      chooser->data = g_variant_ref_sink (g_variant_new_from_bytes (G_VARIANT_TYPE ("a(auss)"), bytes, TRUE));
    }

  if (!chooser->iter)
    {
      chooser->iter = g_variant_iter_new (chooser->data);
      chooser->box = chooser->people.box;
    }
  while ((item = g_variant_iter_next_value (chooser->iter)))
    {
      const char *name;

      g_variant_get_child (item, 1, "&s", &name);

      if (strcmp (name, chooser->body.first) == 0)
        chooser->box = chooser->body.box;
      else if (strcmp (name, chooser->nature.first) == 0)
        chooser->box = chooser->nature.box;
      else if (strcmp (name, chooser->food.first) == 0)
        chooser->box = chooser->food.box;
      else if (strcmp (name, chooser->travel.first) == 0)
        chooser->box = chooser->travel.box;
      else if (strcmp (name, chooser->activities.first) == 0)
        chooser->box = chooser->activities.box;
      else if (strcmp (name, chooser->objects.first) == 0)
        chooser->box = chooser->objects.box;
      else if (strcmp (name, chooser->symbols.first) == 0)
        chooser->box = chooser->symbols.box;
      else if (strcmp (name, chooser->flags.first) == 0)
        chooser->box = chooser->flags.box;

      add_emoji (chooser->box, FALSE, item, 0, chooser);
      g_variant_unref (item);

      now = g_get_monotonic_time ();
      if (now > start + 8000)
        return G_SOURCE_CONTINUE;
    }

  /* We scroll to the top on show, so check the right button for the 1st time */
  ctk_widget_set_state_flags (chooser->recent.button, CTK_STATE_FLAG_CHECKED, FALSE);

  g_variant_iter_free (chooser->iter);
  chooser->iter = NULL;
  chooser->box = NULL;
  chooser->populate_idle = 0;

  return G_SOURCE_REMOVE;
}

static void
adj_value_changed (CtkAdjustment *adj,
                   gpointer       data)
{
  CtkEmojiChooser *chooser = data;
  double value = ctk_adjustment_get_value (adj);
  EmojiSection const *sections[] = {
    &chooser->recent,
    &chooser->people,
    &chooser->body,
    &chooser->nature,
    &chooser->food,
    &chooser->travel,
    &chooser->activities,
    &chooser->objects,
    &chooser->symbols,
    &chooser->flags,
  };
  EmojiSection const *select_section = sections[0];
  gsize i;

  /* Figure out which section the current scroll position is within */
  for (i = 0; i < G_N_ELEMENTS (sections); ++i)
    {
      EmojiSection const *section = sections[i];
      CtkAllocation alloc;

      if (section->heading)
        ctk_widget_get_allocation (section->heading, &alloc);
      else
        ctk_widget_get_allocation (section->box, &alloc);

      if (value < alloc.y - BOX_SPACE)
        break;

      select_section = section;
    }

  /* Un/Check the section buttons accordingly */
  for (i = 0; i < G_N_ELEMENTS (sections); ++i)
    {
      EmojiSection const *section = sections[i];

      if (section == select_section)
        ctk_widget_set_state_flags (section->button, CTK_STATE_FLAG_CHECKED, FALSE);
      else
        ctk_widget_unset_state_flags (section->button, CTK_STATE_FLAG_CHECKED);
    }
}

static gboolean
filter_func (CtkFlowBoxChild *child,
             gpointer         data)
{
  EmojiSection *section = data;
  CtkEmojiChooser *chooser;
  GVariant *emoji_data;
  const char *text;
  const char *name;
  gboolean res;

  res = TRUE;

  chooser = CTK_EMOJI_CHOOSER (ctk_widget_get_ancestor (CTK_WIDGET (child), CTK_TYPE_EMOJI_CHOOSER));
  text = ctk_entry_get_text (CTK_ENTRY (chooser->search_entry));
  emoji_data = (GVariant *) g_object_get_data (G_OBJECT (child), "emoji-data");

  if (text[0] == 0)
    goto out;

  if (!emoji_data)
    goto out;

  g_variant_get_child (emoji_data, 1, "&s", &name);
  res = g_str_match_string (text, name, TRUE);

out:
  if (res)
    section->empty = FALSE;

  return res;
}

static void
invalidate_section (EmojiSection *section)
{
  section->empty = TRUE;
  ctk_flow_box_invalidate_filter (CTK_FLOW_BOX (section->box));
}

static void
update_headings (CtkEmojiChooser *chooser)
{
  ctk_widget_set_visible (chooser->people.heading, !chooser->people.empty);
  ctk_widget_set_visible (chooser->people.box, !chooser->people.empty);
  ctk_widget_set_visible (chooser->body.heading, !chooser->body.empty);
  ctk_widget_set_visible (chooser->body.box, !chooser->body.empty);
  ctk_widget_set_visible (chooser->nature.heading, !chooser->nature.empty);
  ctk_widget_set_visible (chooser->nature.box, !chooser->nature.empty);
  ctk_widget_set_visible (chooser->food.heading, !chooser->food.empty);
  ctk_widget_set_visible (chooser->food.box, !chooser->food.empty);
  ctk_widget_set_visible (chooser->travel.heading, !chooser->travel.empty);
  ctk_widget_set_visible (chooser->travel.box, !chooser->travel.empty);
  ctk_widget_set_visible (chooser->activities.heading, !chooser->activities.empty);
  ctk_widget_set_visible (chooser->activities.box, !chooser->activities.empty);
  ctk_widget_set_visible (chooser->objects.heading, !chooser->objects.empty);
  ctk_widget_set_visible (chooser->objects.box, !chooser->objects.empty);
  ctk_widget_set_visible (chooser->symbols.heading, !chooser->symbols.empty);
  ctk_widget_set_visible (chooser->symbols.box, !chooser->symbols.empty);
  ctk_widget_set_visible (chooser->flags.heading, !chooser->flags.empty);
  ctk_widget_set_visible (chooser->flags.box, !chooser->flags.empty);

  if (chooser->recent.empty && chooser->people.empty &&
      chooser->body.empty && chooser->nature.empty &&
      chooser->food.empty && chooser->travel.empty &&
      chooser->activities.empty && chooser->objects.empty &&
      chooser->symbols.empty && chooser->flags.empty)
    ctk_stack_set_visible_child_name (CTK_STACK (chooser->stack), "empty");
  else
    ctk_stack_set_visible_child_name (CTK_STACK (chooser->stack), "list");
}

static void
search_changed (CtkEntry *entry,
                gpointer  data)
{
  CtkEmojiChooser *chooser = data;

  invalidate_section (&chooser->recent);
  invalidate_section (&chooser->people);
  invalidate_section (&chooser->body);
  invalidate_section (&chooser->nature);
  invalidate_section (&chooser->food);
  invalidate_section (&chooser->travel);
  invalidate_section (&chooser->activities);
  invalidate_section (&chooser->objects);
  invalidate_section (&chooser->symbols);
  invalidate_section (&chooser->flags);

  update_headings (chooser);
}

static void
setup_section (CtkEmojiChooser *chooser,
               EmojiSection   *section,
               const char     *first,
               const char     *icon)
{
  CtkAdjustment *adj;
  CtkWidget *image;

  section->first = first;

  image = ctk_bin_get_child (CTK_BIN (section->button));
  ctk_image_set_from_icon_name (CTK_IMAGE (image), icon, CTK_ICON_SIZE_BUTTON);

  adj = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (chooser->scrolled_window));

  ctk_container_set_focus_vadjustment (CTK_CONTAINER (section->box), adj);
  ctk_flow_box_set_filter_func (CTK_FLOW_BOX (section->box), filter_func, section, NULL);
  g_signal_connect (section->button, "clicked", G_CALLBACK (scroll_to_section), section);
}

static void
ctk_emoji_chooser_init (CtkEmojiChooser *chooser)
{
  CtkAdjustment *adj;

  chooser->settings = g_settings_new ("org.ctk.Settings.EmojiChooser");

  ctk_widget_init_template (CTK_WIDGET (chooser));

  /* Get a reasonable maximum width for an emoji. We do this to
   * skip overly wide fallback rendering for certain emojis the
   * font does not contain and therefore end up being rendered
   * as multiply glyphs.
   */
  {
    PangoLayout *layout = ctk_widget_create_pango_layout (CTK_WIDGET (chooser), "🙂");
    PangoAttrList *attrs;
    PangoRectangle rect;

    attrs = pango_attr_list_new ();
    pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
    pango_layout_set_attributes (layout, attrs);
    pango_attr_list_unref (attrs);

    pango_layout_get_extents (layout, &rect, NULL);
    chooser->emoji_max_width = rect.width;

    g_object_unref (layout);
  }

  chooser->recent_long_press = ctk_gesture_long_press_new (chooser->recent.box);
  g_signal_connect (chooser->recent_long_press, "pressed", G_CALLBACK (long_pressed_cb), chooser);
  chooser->recent_multi_press = ctk_gesture_multi_press_new (chooser->recent.box);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (chooser->recent_multi_press), GDK_BUTTON_SECONDARY);
  g_signal_connect (chooser->recent_multi_press, "pressed", G_CALLBACK (pressed_cb), chooser);

  chooser->people_long_press = ctk_gesture_long_press_new (chooser->people.box);
  g_signal_connect (chooser->people_long_press, "pressed", G_CALLBACK (long_pressed_cb), chooser);
  chooser->people_multi_press = ctk_gesture_multi_press_new (chooser->people.box);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (chooser->people_multi_press), GDK_BUTTON_SECONDARY);
  g_signal_connect (chooser->people_multi_press, "pressed", G_CALLBACK (pressed_cb), chooser);

  chooser->body_long_press = ctk_gesture_long_press_new (chooser->body.box);
  g_signal_connect (chooser->body_long_press, "pressed", G_CALLBACK (long_pressed_cb), chooser);
  chooser->body_multi_press = ctk_gesture_multi_press_new (chooser->body.box);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (chooser->body_multi_press), GDK_BUTTON_SECONDARY);
  g_signal_connect (chooser->body_multi_press, "pressed", G_CALLBACK (pressed_cb), chooser);

  adj = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (chooser->scrolled_window));
  g_signal_connect (adj, "value-changed", G_CALLBACK (adj_value_changed), chooser);

  setup_section (chooser, &chooser->recent, NULL, "emoji-recent-symbolic");
  setup_section (chooser, &chooser->people, "grinning face", "emoji-people-symbolic");
  setup_section (chooser, &chooser->body, "selfie", "emoji-body-symbolic");
  setup_section (chooser, &chooser->nature, "monkey face", "emoji-nature-symbolic");
  setup_section (chooser, &chooser->food, "grapes", "emoji-food-symbolic");
  setup_section (chooser, &chooser->travel, "globe showing Europe-Africa", "emoji-travel-symbolic");
  setup_section (chooser, &chooser->activities, "jack-o-lantern", "emoji-activities-symbolic");
  setup_section (chooser, &chooser->objects, "muted speaker", "emoji-objects-symbolic");
  setup_section (chooser, &chooser->symbols, "ATM sign", "emoji-symbols-symbolic");
  setup_section (chooser, &chooser->flags, "chequered flag", "emoji-flags-symbolic");

  populate_recent_section (chooser);

  chooser->populate_idle = g_idle_add (populate_emoji_chooser, chooser);
  g_source_set_name_by_id (chooser->populate_idle, "[ctk] populate_emoji_chooser");
}

static void
ctk_emoji_chooser_show (CtkWidget *widget)
{
  CtkEmojiChooser *chooser = CTK_EMOJI_CHOOSER (widget);
  CtkAdjustment *adj;

  CTK_WIDGET_CLASS (ctk_emoji_chooser_parent_class)->show (widget);

  adj = ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (chooser->scrolled_window));
  ctk_adjustment_set_value (adj, 0);

  ctk_entry_set_text (CTK_ENTRY (chooser->search_entry), "");
}

static void
ctk_emoji_chooser_class_init (CtkEmojiChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->finalize = ctk_emoji_chooser_finalize;
  widget_class->show = ctk_emoji_chooser_show;

  signals[EMOJI_PICKED] = g_signal_new ("emoji-picked",
                                        G_OBJECT_CLASS_TYPE (object_class),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_NONE, 1, G_TYPE_STRING|G_SIGNAL_TYPE_STATIC_SCOPE);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkemojichooser.ui");

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, search_entry);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, stack);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, scrolled_window);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, recent.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, recent.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, people.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, people.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, people.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, body.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, body.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, body.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, nature.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, nature.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, nature.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, food.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, food.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, food.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, travel.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, travel.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, travel.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, activities.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, activities.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, activities.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, objects.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, objects.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, objects.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, symbols.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, symbols.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, symbols.button);

  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, flags.box);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, flags.heading);
  ctk_widget_class_bind_template_child (widget_class, CtkEmojiChooser, flags.button);

  ctk_widget_class_bind_template_callback (widget_class, emoji_activated);
  ctk_widget_class_bind_template_callback (widget_class, search_changed);
}

CtkWidget *
ctk_emoji_chooser_new (void)
{
  return CTK_WIDGET (g_object_new (CTK_TYPE_EMOJI_CHOOSER, NULL));
}
