/* widget-factory: a collection of widgets, for easy theme testing
 *
 * Copyright (C) 2011 Canonical Ltd
 *
 * This  library is free  software; you can  redistribute it and/or
 * modify it  under  the terms  of the  GNU Lesser  General  Public
 * License  as published  by the Free  Software  Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed  in the hope that it will be useful,
 * but  WITHOUT ANY WARRANTY; without even  the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Andrea Cimitan <andrea.cimitan@canonical.com>
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static void
change_theme_state (GSimpleAction *action,
                    GVariant      *state,
                    gpointer       user_data)
{
  GtkSettings *settings = ctk_settings_get_default ();

  g_object_set (G_OBJECT (settings),
                "gtk-application-prefer-dark-theme",
                g_variant_get_boolean (state),
                NULL);

  g_simple_action_set_state (action, state);
}

static GtkWidget *page_stack;

static void
change_transition_state (GSimpleAction *action,
                         GVariant      *state,
                         gpointer       user_data)
{
  GtkStackTransitionType transition;

  if (g_variant_get_boolean (state))
    transition = CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT;
  else
    transition = CTK_STACK_TRANSITION_TYPE_NONE;

  ctk_stack_set_transition_type (CTK_STACK (page_stack), transition);

  g_simple_action_set_state (action, state);
}

static gboolean
get_idle (gpointer data)
{
  GtkWidget *window = data;
  GtkApplication *app = ctk_window_get_application (CTK_WINDOW (window));

  ctk_widget_set_sensitive (window, TRUE);
  gdk_window_set_cursor (ctk_widget_get_window (window), NULL);
  g_application_unmark_busy (G_APPLICATION (app));

  return G_SOURCE_REMOVE;
}

static void
get_busy (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  GtkWidget *window = user_data;
  GdkCursor *cursor;
  GtkApplication *app = ctk_window_get_application (CTK_WINDOW (window));

  g_application_mark_busy (G_APPLICATION (app));
  cursor = gdk_cursor_new_from_name (ctk_widget_get_display (window), "wait");
  gdk_window_set_cursor (ctk_widget_get_window (window), cursor);
  g_object_unref (cursor);
  g_timeout_add (5000, get_idle, window);

  ctk_widget_set_sensitive (window, FALSE);
}

static gint current_page = 0;
static gboolean
on_page (gint i)
{
  return current_page == i;
}

static void
activate_search (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GtkWidget *window = user_data;
  GtkWidget *searchbar;

  if (!on_page (2))
    return;

  searchbar = CTK_WIDGET (g_object_get_data (G_OBJECT (window), "searchbar"));
  ctk_search_bar_set_search_mode (CTK_SEARCH_BAR (searchbar), TRUE);
}

static void
activate_delete (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GtkWidget *window = user_data;
  GtkWidget *infobar;

  if (!on_page (2))
    return;

  infobar = CTK_WIDGET (g_object_get_data (G_OBJECT (window), "infobar"));
  ctk_widget_show (infobar);
}

static void populate_flowbox (GtkWidget *flowbox);

static void
activate_background (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  GtkWidget *window = user_data;
  GtkWidget *dialog;
  GtkWidget *flowbox;

  if (!on_page (2))
    return;

  dialog = CTK_WIDGET (g_object_get_data (G_OBJECT (window), "selection_dialog"));
  flowbox = CTK_WIDGET (g_object_get_data (G_OBJECT (window), "selection_flowbox"));

  ctk_widget_show (dialog);
  populate_flowbox (flowbox);
}

static void
activate_open (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GtkWidget *window = user_data;
  GtkWidget *button;

  if (!on_page (3))
    return;

  button = CTK_WIDGET (g_object_get_data (G_OBJECT (window), "open_menubutton"));
  ctk_button_clicked (CTK_BUTTON (button));
}

static void
activate_record (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GtkWidget *window = user_data;
  GtkWidget *button;

  if (!on_page (3))
    return;

  button = CTK_WIDGET (g_object_get_data (G_OBJECT (window), "record_button"));
  ctk_button_clicked (CTK_BUTTON (button));
}

static void
activate_lock (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GtkWidget *window = user_data;
  GtkWidget *button;

  if (!on_page (3))
    return;

  button = CTK_WIDGET (g_object_get_data (G_OBJECT (window), "lockbutton"));
  ctk_button_clicked (CTK_BUTTON (button));
}

static void
activate_about (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GtkApplication *app = user_data;
  const gchar *authors[] = {
    "Andrea Cimitan",
    "Cosimo Cecchi",
    NULL
  };
  gchar *version;

  version = g_strdup_printf ("%s\nRunning against GTK+ %d.%d.%d",
                             PACKAGE_VERSION,
                             ctk_get_major_version (),
                             ctk_get_minor_version (),
                             ctk_get_micro_version ());

  ctk_show_about_dialog (CTK_WINDOW (ctk_application_get_active_window (app)),
                         "program-name", "GTK Widget Factory",
                         "version", version,
                         "copyright", "© 1997—2019 The GTK Team",
                         "license-type", CTK_LICENSE_LGPL_2_1,
                         "website", "http://www.gtk.org",
                         "comments", "Program to demonstrate GTK themes and widgets",
                         "authors", authors,
                         "logo-icon-name", "gtk3-widget-factory",
                         "title", "About GTK Widget Factory",
                         NULL);

  g_free (version);
}

static void
activate_quit (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GtkApplication *app = user_data;
  GtkWidget *win;
  GList *list, *next;

  list = ctk_application_get_windows (app);
  while (list)
    {
      win = list->data;
      next = list->next;

      ctk_widget_destroy (CTK_WIDGET (win));

      list = next;
    }
}

static void
activate_inspector (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  ctk_window_set_interactive_debugging (TRUE);
}

static void
spin_value_changed (GtkAdjustment *adjustment, GtkWidget *label)
{
  GtkWidget *w;
  gint v;
  gchar *text;

  v = (int)ctk_adjustment_get_value (adjustment);

  if ((v % 3) == 0)
    {
      text = g_strdup_printf ("%d is a multiple of 3", v);
      ctk_label_set_label (CTK_LABEL (label), text);
      g_free (text);
    }

  w = ctk_widget_get_ancestor (label, CTK_TYPE_REVEALER);
  ctk_revealer_set_reveal_child (CTK_REVEALER (w), (v % 3) == 0);
}

static void
dismiss (GtkWidget *button)
{
  GtkWidget *w;

  w = ctk_widget_get_ancestor (button, CTK_TYPE_REVEALER);
  ctk_revealer_set_reveal_child (CTK_REVEALER (w), FALSE);
}

static void
spin_value_reset (GtkWidget *button, GtkAdjustment *adjustment)
{
  ctk_adjustment_set_value (adjustment, 50.0);
  dismiss (button);
}

static gint pulse_time = 250;
static gint pulse_entry_mode = 0;

static void
remove_pulse (gpointer pulse_id)
{
  g_source_remove (GPOINTER_TO_UINT (pulse_id));
}

static gboolean
pulse_it (GtkWidget *widget)
{
  guint pulse_id;

  if (CTK_IS_ENTRY (widget))
    ctk_entry_progress_pulse (CTK_ENTRY (widget));
  else
    ctk_progress_bar_pulse (CTK_PROGRESS_BAR (widget));

  pulse_id = g_timeout_add (pulse_time, (GSourceFunc)pulse_it, widget);
  g_object_set_data_full (G_OBJECT (widget), "pulse_id", GUINT_TO_POINTER (pulse_id), remove_pulse);

  return G_SOURCE_REMOVE;
}

static void
update_pulse_time (GtkAdjustment *adjustment, GtkWidget *widget)
{
  gdouble value;
  guint pulse_id;

  value = ctk_adjustment_get_value (adjustment);

  pulse_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), "pulse_id"));

  /* vary between 50 and 450 */
  pulse_time = 50 + 4 * value;

  if (value == 100)
    {
      g_object_set_data (G_OBJECT (widget), "pulse_id", NULL);
    }
  else if (value < 100)
    {
      if (pulse_id == 0 && (CTK_IS_PROGRESS_BAR (widget) || pulse_entry_mode % 3 == 2))
        {
          pulse_id = g_timeout_add (pulse_time, (GSourceFunc)pulse_it, widget);
          g_object_set_data_full (G_OBJECT (widget), "pulse_id", GUINT_TO_POINTER (pulse_id), remove_pulse);
        }
    }
}

static void
on_entry_icon_release (GtkEntry            *entry,
                       GtkEntryIconPosition icon_pos,
                       GdkEvent            *event,
                       gpointer             user_data)
{
  if (icon_pos != CTK_ENTRY_ICON_SECONDARY)
    return;

  pulse_entry_mode++;

  if (pulse_entry_mode % 3 == 0)
    {
      g_object_set_data (G_OBJECT (entry), "pulse_id", NULL);
      ctk_entry_set_progress_fraction (entry, 0);
    }
  else if (pulse_entry_mode % 3 == 1)
    ctk_entry_set_progress_fraction (entry, 0.25);
  else if (pulse_entry_mode % 3 == 2)
    {
      if (pulse_time - 50 < 400)
        {
          ctk_entry_set_progress_pulse_step (entry, 0.1);
          pulse_it (CTK_WIDGET (entry));
        }
    }
}

#define EPSILON (1e-10)

static gboolean
on_scale_button_query_tooltip (GtkWidget  *button,
                               gint        x,
                               gint        y,
                               gboolean    keyboard_mode,
                               GtkTooltip *tooltip,
                               gpointer    user_data)
{
  GtkScaleButton *scale_button = CTK_SCALE_BUTTON (button);
  GtkAdjustment *adjustment;
  gdouble val;
  gchar *str;
  AtkImage *image;

  image = ATK_IMAGE (ctk_widget_get_accessible (button));

  adjustment = ctk_scale_button_get_adjustment (scale_button);
  val = ctk_scale_button_get_value (scale_button);

  if (val < (ctk_adjustment_get_lower (adjustment) + EPSILON))
    {
      str = g_strdup (_("Muted"));
    }
  else if (val >= (ctk_adjustment_get_upper (adjustment) - EPSILON))
    {
      str = g_strdup (_("Full Volume"));
    }
  else
    {
      gint percent;

      percent = (gint) (100. * val / (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_lower (adjustment)) + .5);

      str = g_strdup_printf (C_("volume percentage", "%d %%"), percent);
    }

  ctk_tooltip_set_text (tooltip, str);
  atk_image_set_image_description (image, str);
  g_free (str);

  return TRUE;
}

static void
on_scale_button_value_changed (GtkScaleButton *button,
                               gdouble         value,
                               gpointer        user_data)
{
  ctk_widget_trigger_tooltip_query (CTK_WIDGET (button));
}

static void
on_record_button_toggled (GtkToggleButton *button,
                          gpointer         user_data)
{
  GtkStyleContext *context;

  context = ctk_widget_get_style_context (CTK_WIDGET (button));
  if (ctk_toggle_button_get_active (button))
    ctk_style_context_remove_class (context, "destructive-action");
  else
    ctk_style_context_add_class (context, "destructive-action");
}

static void
on_page_combo_changed (GtkComboBox *combo,
                       gpointer     user_data)
{
  GtkWidget *from;
  GtkWidget *to;
  GtkWidget *print;

  from = CTK_WIDGET (g_object_get_data (G_OBJECT (combo), "range_from_spin"));
  to = CTK_WIDGET (g_object_get_data (G_OBJECT (combo), "range_to_spin"));
  print = CTK_WIDGET (g_object_get_data (G_OBJECT (combo), "print_button"));

  switch (ctk_combo_box_get_active (combo))
    {
    case 0: /* Range */
      ctk_widget_set_sensitive (from, TRUE);
      ctk_widget_set_sensitive (to, TRUE);
      ctk_widget_set_sensitive (print, TRUE);
      break;
    case 1: /* All */
      ctk_widget_set_sensitive (from, FALSE);
      ctk_widget_set_sensitive (to, FALSE);
      ctk_spin_button_set_value (CTK_SPIN_BUTTON (from), 1);
      ctk_spin_button_set_value (CTK_SPIN_BUTTON (to), 99);
      ctk_widget_set_sensitive (print, TRUE);
      break;
    case 2: /* Current */
      ctk_widget_set_sensitive (from, FALSE);
      ctk_widget_set_sensitive (to, FALSE);
      ctk_spin_button_set_value (CTK_SPIN_BUTTON (from), 7);
      ctk_spin_button_set_value (CTK_SPIN_BUTTON (to), 7);
      ctk_widget_set_sensitive (print, TRUE);
      break;
    case 4:
      ctk_widget_set_sensitive (from, FALSE);
      ctk_widget_set_sensitive (to, FALSE);
      ctk_widget_set_sensitive (print, FALSE);
      break;
    default:;
    }
}

static void
on_range_from_changed (GtkSpinButton *from)
{
  GtkSpinButton *to;
  gint v1, v2;

  to = CTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (from), "range_to_spin"));

  v1 = ctk_spin_button_get_value_as_int (from);
  v2 = ctk_spin_button_get_value_as_int (to);

  if (v1 > v2)
    ctk_spin_button_set_value (to, v1);
}

static void
on_range_to_changed (GtkSpinButton *to)
{
  GtkSpinButton *from;
  gint v1, v2;

  from = CTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (to), "range_from_spin"));

  v1 = ctk_spin_button_get_value_as_int (from);
  v2 = ctk_spin_button_get_value_as_int (to);

  if (v1 > v2)
    ctk_spin_button_set_value (from, v2);
}

static void
update_header (GtkListBoxRow *row,
               GtkListBoxRow *before,
               gpointer       data)
{
  if (before != NULL &&
      ctk_list_box_row_get_header (row) == NULL)
    {
      GtkWidget *separator;

      separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_widget_show (separator);
      ctk_list_box_row_set_header (row, separator);
    }
}

static void
info_bar_response (GtkWidget *infobar, gint response_id)
{
  if (response_id == CTK_RESPONSE_CLOSE)
    ctk_widget_hide (infobar);
}

static void
show_dialog (GtkWidget *button, GtkWidget *dialog)
{
  ctk_widget_show (dialog);
}

static void
close_dialog (GtkWidget *dialog)
{
  ctk_widget_hide (dialog);
}

static void
set_needs_attention (GtkWidget *page, gboolean needs_attention)
{
  GtkWidget *stack;

  stack = ctk_widget_get_parent (page);
  ctk_container_child_set (CTK_CONTAINER (stack), page,
                           "needs-attention", needs_attention,
                           NULL);
}

static gboolean
demand_attention (gpointer stack)
{
  GtkWidget *page;

  page = ctk_stack_get_child_by_name (CTK_STACK (stack), "page3");
  set_needs_attention (page, TRUE);

  return G_SOURCE_REMOVE;
}

static void
action_dialog_button_clicked (GtkButton *button, GtkWidget *page)
{
  g_timeout_add (1000, demand_attention, page);
}

static void
page_changed_cb (GtkWidget *stack, GParamSpec *pspec, gpointer data)
{
  const gchar *name;
  GtkWidget *window;
  GtkWidget *page;

  if (ctk_widget_in_destruction (stack))
    return;

  name = ctk_stack_get_visible_child_name (CTK_STACK (stack));

  window = ctk_widget_get_ancestor (stack, CTK_TYPE_APPLICATION_WINDOW);
  g_object_set (ctk_application_window_get_help_overlay (CTK_APPLICATION_WINDOW (window)),
                "view-name", name,
                NULL);

  if (g_str_equal (name, "page1"))
    current_page = 1;
  else if (g_str_equal (name, "page2"))
    current_page = 2;
  if (g_str_equal (name, "page3"))
    {
      current_page = 3;
      page = ctk_stack_get_visible_child (CTK_STACK (stack));
      set_needs_attention (CTK_WIDGET (page), FALSE);
    }
}

static void
populate_model (GtkTreeStore *store)
{
  GtkTreeIter iter, parent0, parent1, parent2, parent3;

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter,
                      0, "Charlemagne",
                      1, "742",
                      2, "814",
                      -1);
  parent0 = iter;
  ctk_tree_store_append (store, &iter, &parent0);
  ctk_tree_store_set (store, &iter,
                      0, "Pepin the Short",
                      1, "714",
                      2, "768",
                      -1);
  parent1 = iter;
  ctk_tree_store_append (store, &iter, &parent1);
  ctk_tree_store_set (store, &iter,
                      0, "Charles Martel",
                      1, "688",
                      2, "741",
                      -1);
  parent2 = iter;
  ctk_tree_store_append (store, &iter, &parent2);
  ctk_tree_store_set (store, &iter,
                      0, "Pepin of Herstal",
                      1, "635",
                      2, "714",
                      -1);
  parent3 = iter;
  ctk_tree_store_append (store, &iter, &parent3);
  ctk_tree_store_set (store, &iter,
                      0, "Ansegisel",
                      1, "602 or 610",
                      2, "murdered before 679",
                      -1);
  ctk_tree_store_append (store, &iter, &parent3);
  ctk_tree_store_set (store, &iter,
                      0, "Begga",
                      1, "615",
                      2, "693",
                      -1);
  ctk_tree_store_append (store, &iter, &parent2);
  ctk_tree_store_set (store, &iter,
                      0, "Alpaida",
                      -1);
  ctk_tree_store_append (store, &iter, &parent1);
  ctk_tree_store_set (store, &iter,
                      0, "Rotrude",
                      -1);
  parent2 = iter;
  ctk_tree_store_append (store, &iter, &parent2);
  ctk_tree_store_set (store, &iter,
                      0, "Liévin de Trèves",
                      -1);
  parent3 = iter;
  ctk_tree_store_append (store, &iter, &parent3);
  ctk_tree_store_set (store, &iter,
                      0, "Guérin",
                      -1);
  ctk_tree_store_append (store, &iter, &parent3);
  ctk_tree_store_set (store, &iter,
                      0, "Gunza",
                      -1);
  ctk_tree_store_append (store, &iter, &parent2);
  ctk_tree_store_set (store, &iter,
                      0, "Willigarde de Bavière",
                      -1);
  ctk_tree_store_append (store, &iter, &parent0);
  ctk_tree_store_set (store, &iter,
                      0, "Bertrada of Laon",
                      1, "710",
                      2, "783",
                      -1);
  parent1 = iter;
  ctk_tree_store_append (store, &iter, &parent1);
  ctk_tree_store_set (store, &iter,
                      0, "Caribert of Laon",
                      2, "before 762",
                      -1);
  parent2 = iter;
  ctk_tree_store_append (store, &iter, &parent2);
  ctk_tree_store_set (store, &iter,
                      0, "Unknown",
                      -1);
  ctk_tree_store_append (store, &iter, &parent2);
  ctk_tree_store_set (store, &iter,
                      0, "Bertrada of Prüm",
                      1, "ca. 670",
                      2, "after 721",
                      -1);
  ctk_tree_store_append (store, &iter, &parent1);
  ctk_tree_store_set (store, &iter,
                      0, "Gisele of Aquitaine",
                      -1);
  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 3, TRUE, -1);
  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter,
                      0, "Attila the Hun",
                      1, "ca. 390",
                      2, "453",
                      -1);
}

static gboolean
row_separator_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gboolean is_sep;

  ctk_tree_model_get (model, iter, 3, &is_sep, -1);

  return is_sep;
}

static void
update_title_header (GtkListBoxRow *row,
                     GtkListBoxRow *before,
                     gpointer       data)
{
  GtkWidget *header;
  gchar *title;

  header = ctk_list_box_row_get_header (row);
  title = (gchar *)g_object_get_data (G_OBJECT (row), "title");
  if (!header && title)
    {
      title = g_strdup_printf ("<b>%s</b>", title);

      header = ctk_label_new (title);
      ctk_label_set_use_markup (CTK_LABEL (header), TRUE);
      ctk_widget_set_halign (header, CTK_ALIGN_START);
      ctk_widget_set_margin_top (header, 12);
      ctk_widget_set_margin_start (header, 6);
      ctk_widget_set_margin_end (header, 6);
      ctk_widget_set_margin_bottom (header, 6);
      ctk_widget_show (header);

      ctk_list_box_row_set_header (row, header);

      g_free (title);
    }
}

static void
overshot (GtkScrolledWindow *sw, GtkPositionType pos, GtkWidget *widget)
{
  GtkWidget *box, *row, *label, *swatch;
  GdkRGBA rgba;
  const gchar *color;
  gchar *text;
  GtkWidget *silver;
  GtkWidget *gold;

  silver = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "Silver"));
  gold = CTK_WIDGET (g_object_get_data (G_OBJECT (widget), "Gold"));

  if (pos == CTK_POS_TOP)
    {
      if (silver)
        {
          ctk_container_remove (CTK_CONTAINER (widget), silver);
          g_object_set_data (G_OBJECT (widget), "Silver", NULL);
        }
      if (gold)
        {
          ctk_container_remove (CTK_CONTAINER (widget), gold);
          g_object_set_data (G_OBJECT (widget), "Gold", NULL);
        }

      return;
    }


  if (gold)
    return;
  else if (silver)
    color = "Gold";
  else
    color = "Silver";

  row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 20);
  text = g_strconcat ("<b>", color, "</b>", NULL);
  label = ctk_label_new (text);
  g_free (text);
  g_object_set (label,
                "use-markup", TRUE,
                "halign", CTK_ALIGN_START,
                "valign", CTK_ALIGN_CENTER,
                "margin", 6,
                "xalign", 0.0,
                NULL);
  ctk_box_pack_start (CTK_BOX (row), label, TRUE, TRUE, 0);
  gdk_rgba_parse (&rgba, color);
  swatch = g_object_new (g_type_from_name ("GtkColorSwatch"),
                         "rgba", &rgba,
                         "selectable", FALSE,
                         "halign", CTK_ALIGN_END,
                         "valign", CTK_ALIGN_CENTER,
                         "margin", 6,
                         "height-request", 24,
                         NULL);
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (box), swatch);
  ctk_box_pack_start (CTK_BOX (row), box, FALSE, FALSE, 0);
  ctk_widget_show_all (row);
  ctk_list_box_insert (CTK_LIST_BOX (widget), row, -1);
  row = ctk_widget_get_parent (row);
  ctk_list_box_row_set_activatable (CTK_LIST_BOX_ROW (row), FALSE);
  g_object_set_data (G_OBJECT (widget), color, row);
  g_object_set_data (G_OBJECT (row), "color", (gpointer)color);
}

static void
rgba_changed (GtkColorChooser *chooser, GParamSpec *pspec, GtkListBox *box)
{
  ctk_list_box_select_row (box, NULL);
}

static void
set_color (GtkListBox *box, GtkListBoxRow *row, GtkColorChooser *chooser)
{
  const char *color;
  GdkRGBA rgba;

  if (!row)
    return;

  color = (const char *)g_object_get_data (G_OBJECT (row), "color");

  if (!color)
    return;

  if (gdk_rgba_parse (&rgba, color))
    {
      g_signal_handlers_block_by_func (chooser, rgba_changed, box);
      ctk_color_chooser_set_rgba (chooser, &rgba);
      g_signal_handlers_unblock_by_func (chooser, rgba_changed, box);
    }
}

static void
populate_colors (GtkWidget *widget, GtkWidget *chooser)
{
  struct { const gchar *name; const gchar *color; const gchar *title; } colors[] = {
    { "2.5", "#C8828C", "Red" },
    { "5", "#C98286", NULL },
    { "7.5", "#C9827F", NULL },
    { "10", "#C98376", NULL },
    { "2.5", "#C8856D", "Red/Yellow" },
    { "5", "#C58764", NULL },
    { "7.5", "#C1895E", NULL },
    { "10", "#BB8C56", NULL },
    { "2.5", "#B58F4F", "Yellow" },
    { "5", "#AD924B", NULL },
    { "7.5", "#A79548", NULL },
    { "10", "#A09749", NULL },
    { "2.5", "#979A4E", "Yellow/Green" },
    { "5", "#8D9C55", NULL },
    { "7.5", "#7F9F62", NULL },
    { "10", "#73A06E", NULL },
    { "2.5", "#65A27C", "Green" },
    { "5", "#5CA386", NULL },
    { "7.5", "#57A38D", NULL },
    { "10", "#52A394", NULL },
    { "2.5", "#4EA39A", "Green/Blue" },
    { "5", "#49A3A2", NULL },
    { "7.5", "#46A2AA", NULL },
    { "10", "#46A1B1", NULL },
    { "2.5", "#49A0B8", "Blue" },
    { "5", "#529EBD", NULL },
    { "7.5", "#5D9CC1", NULL },
    { "10", "#689AC3", NULL },
    { "2.5", "#7597C5", "Blue/Purple" },
    { "5", "#8095C6", NULL },
    { "7.5", "#8D91C6", NULL },
    { "10", "#988EC4", NULL },
    { "2.5", "#A08CC1", "Purple" },
    { "5", "#A88ABD", NULL },
    { "7.5", "#B187B6", NULL },
    { "10", "#B786B0", NULL },
    { "2.5", "#BC84A9", "Purple/Red" },
    { "5", "#C183A0", NULL },
    { "7.5", "#C48299", NULL },
    { "10", "#C68292", NULL }
  };
  gint i;
  GtkWidget *row, *box, *label, *swatch;
  GtkWidget *sw;
  GdkRGBA rgba;

  ctk_list_box_set_header_func (CTK_LIST_BOX (widget), update_title_header, NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (colors); i++)
    {
      row = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 20);
      label = ctk_label_new (colors[i].name);
      g_object_set (label,
                    "halign", CTK_ALIGN_START,
                    "valign", CTK_ALIGN_CENTER,
                    "margin", 6,
                    "xalign", 0.0,
                    NULL);
      ctk_box_pack_start (CTK_BOX (row), label, TRUE, TRUE, 0);
      gdk_rgba_parse (&rgba, colors[i].color);
      swatch = g_object_new (g_type_from_name ("GtkColorSwatch"),
                             "rgba", &rgba,
                             "selectable", FALSE,
                             "halign", CTK_ALIGN_END,
                             "valign", CTK_ALIGN_CENTER,
                             "margin", 6,
                             "height-request", 24,
                             NULL);
      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_add (CTK_CONTAINER (box), swatch);
      ctk_box_pack_start (CTK_BOX (row), box, FALSE, FALSE, 0);
      ctk_widget_show_all (row);
      ctk_list_box_insert (CTK_LIST_BOX (widget), row, -1);
      row = ctk_widget_get_parent (row);
      ctk_list_box_row_set_activatable (CTK_LIST_BOX_ROW (row), FALSE);
      g_object_set_data (G_OBJECT (row), "color", (gpointer)colors[i].color);
      if (colors[i].title)
        g_object_set_data (G_OBJECT (row), "title", (gpointer)colors[i].title);
    }

  g_signal_connect (widget, "row-selected", G_CALLBACK (set_color), chooser);

  ctk_list_box_invalidate_headers (CTK_LIST_BOX (widget));

  sw = ctk_widget_get_ancestor (widget, CTK_TYPE_SCROLLED_WINDOW);
  g_signal_connect (sw, "edge-overshot", G_CALLBACK (overshot), widget);
}

typedef struct {
  GtkWidget *flowbox;
  gchar *filename;
} BackgroundData;

static void
background_loaded_cb (GObject      *source,
                      GAsyncResult *res,
                      gpointer      data)
{
  BackgroundData *bd = data;
  GtkWidget *child;
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, &error);
  if (error)
    {
      g_warning ("Error loading '%s': %s", bd->filename, error->message);
      g_error_free (error);
      return;
    }

  child = ctk_image_new_from_pixbuf (pixbuf);
  ctk_widget_show (child);
  ctk_flow_box_insert (CTK_FLOW_BOX (bd->flowbox), child, -1);
  child = ctk_widget_get_parent (child);
  g_object_set_data_full (G_OBJECT (child), "filename", bd->filename, g_free);
  g_free (bd);
}

static void
populate_flowbox (GtkWidget *flowbox)
{
  const gchar *location;
  GDir *dir;
  GError *error = NULL;
  const gchar *name;
  gchar *filename;
  GFile *file;
  GInputStream *stream;
  BackgroundData *bd;
  GdkPixbuf *pixbuf;
  GtkWidget *child;

  if (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (flowbox), "populated")))
    return;

  g_object_set_data (G_OBJECT (flowbox), "populated", GUINT_TO_POINTER (1));

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 110, 70);
  gdk_pixbuf_fill (pixbuf, 0xffffffff);
  child = ctk_image_new_from_pixbuf (pixbuf);
  ctk_widget_show (child);
  ctk_flow_box_insert (CTK_FLOW_BOX (flowbox), child, -1);

  location = "/usr/share/backgrounds/gnome";
  dir = g_dir_open (location, 0, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return;
    }

  while ((name = g_dir_read_name (dir)) != NULL)
    {
      filename = g_build_filename (location, name, NULL);
      file = g_file_new_for_path (filename);
      stream = G_INPUT_STREAM (g_file_read (file, NULL, &error));
      if (error)
        {
          g_warning ("%s", error->message);
          g_clear_error (&error);
          g_free (filename); 
        }
      else
        {
          bd = g_new (BackgroundData, 1);
          bd->flowbox = flowbox;
          bd->filename = filename;
          gdk_pixbuf_new_from_stream_at_scale_async (stream, 110, 110, TRUE, NULL, 
                                                     background_loaded_cb, bd);
        }

      g_object_unref (file);
      g_object_unref (stream);
    }

  g_dir_close (dir);
}

static void
row_activated (GtkListBox *box, GtkListBoxRow *row)
{
  GtkWidget *image;
  GtkWidget *dialog;

  image = (GtkWidget *)g_object_get_data (G_OBJECT (row), "image");
  dialog = (GtkWidget *)g_object_get_data (G_OBJECT (row), "dialog");

  if (image)
    {
      if (ctk_widget_get_opacity (image) > 0)
        ctk_widget_set_opacity (image, 0);
      else
        ctk_widget_set_opacity (image, 1);
    }
  else if (dialog)
    {
      ctk_window_present (CTK_WINDOW (dialog));
    }
}

static void
set_accel (GtkApplication *app, GtkWidget *widget)
{
  GtkWidget *accel_label;
  const gchar *action;
  gchar **accels;
  guint key;
  GdkModifierType mods;

  accel_label = ctk_bin_get_child (CTK_BIN (widget));
  g_assert (CTK_IS_ACCEL_LABEL (accel_label));

  action = ctk_actionable_get_action_name (CTK_ACTIONABLE (widget));
  accels = ctk_application_get_accels_for_action (app, action);

  ctk_accelerator_parse (accels[0], &key, &mods);
  ctk_accel_label_set_accel (CTK_ACCEL_LABEL (accel_label), key, mods);

  g_strfreev (accels);
}

typedef struct
{
  GtkTextView tv;
  cairo_surface_t *surface;
} MyTextView;

typedef GtkTextViewClass MyTextViewClass;

G_DEFINE_TYPE (MyTextView, my_text_view, CTK_TYPE_TEXT_VIEW)

static void
my_text_view_init (MyTextView *tv)
{
}

static void
my_tv_draw_layer (GtkTextView      *widget,
                  GtkTextViewLayer  layer,
                  cairo_t          *cr)
{
  MyTextView *tv = (MyTextView *)widget;

  if (layer == CTK_TEXT_VIEW_LAYER_BELOW_TEXT && tv->surface)
    {
      cairo_save (cr);
      cairo_set_source_surface (cr, tv->surface, 0.0, 0.0);
      cairo_paint_with_alpha (cr, 0.333);
      cairo_restore (cr);
    }
}

static void
my_tv_finalize (GObject *object)
{
  MyTextView *tv = (MyTextView *)object;

  if (tv->surface)
    cairo_surface_destroy (tv->surface);

  G_OBJECT_CLASS (my_text_view_parent_class)->finalize (object);
}

static void
my_text_view_class_init (MyTextViewClass *class)
{
  GtkTextViewClass *tv_class = CTK_TEXT_VIEW_CLASS (class);
  GObjectClass *o_class = G_OBJECT_CLASS (class);

  o_class->finalize = my_tv_finalize;
  tv_class->draw_layer = my_tv_draw_layer;
}

static void
my_text_view_set_background (MyTextView *tv, const gchar *filename)
{
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  if (tv->surface)
    cairo_surface_destroy (tv->surface);

  tv->surface = NULL;

  if (filename == NULL)
    return;

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return;
    }

  tv->surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, 1, NULL);

  g_object_unref (pixbuf);

  ctk_widget_queue_draw (CTK_WIDGET (tv));
}

static void
close_selection_dialog (GtkWidget *dialog, gint response, GtkWidget *tv)
{
  GtkWidget *box;
  GtkWidget *child;
  GList *children;
  const gchar *filename;

  ctk_widget_hide (dialog);

  if (response == CTK_RESPONSE_CANCEL)
    return;

  box = ctk_dialog_get_content_area (CTK_DIALOG (dialog));
  children = ctk_container_get_children (CTK_CONTAINER (box));
  box = children->data;
  g_list_free (children);
  g_assert (CTK_IS_FLOW_BOX (box));
  children = ctk_flow_box_get_selected_children (CTK_FLOW_BOX (box));

  if (!children)
    return;

  child = children->data;
  filename = (const gchar *)g_object_get_data (G_OBJECT (child), "filename");

  g_list_free (children);

  my_text_view_set_background ((MyTextView *)tv, filename);
}

static void
toggle_selection_mode (GtkSwitch  *sw,
                       GParamSpec *pspec,
                       GtkListBox *listbox)
{
  if (ctk_switch_get_active (sw))
    ctk_list_box_set_selection_mode (listbox, CTK_SELECTION_SINGLE);
  else
    ctk_list_box_set_selection_mode (listbox, CTK_SELECTION_NONE);

  ctk_list_box_set_activate_on_single_click (listbox, !ctk_switch_get_active (sw));
}

static void
handle_insert (GtkWidget *button, GtkWidget *textview)
{
  GtkTextBuffer *buffer;
  const gchar *id;
  const gchar *text;

  id = ctk_buildable_get_name (CTK_BUILDABLE (button));

  if (strcmp (id, "toolbutton1") == 0)
    text = "⌘";
  else if (strcmp (id, "toolbutton2") == 0)
    text = "⚽";
  else if (strcmp (id, "toolbutton3") == 0)
    text = "⤢";
  else if (strcmp (id, "toolbutton4") == 0)
    text = "☆";
  else
    text = "";

  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (textview));
  ctk_text_buffer_insert_at_cursor (buffer, text, -1);
}

static void
handle_cutcopypaste (GtkWidget *button, GtkWidget *textview)
{
  GtkTextBuffer *buffer;
  GtkClipboard *clipboard;
  const gchar *id;

  clipboard = ctk_widget_get_clipboard (textview, GDK_SELECTION_CLIPBOARD);
  buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (textview));
  id = ctk_buildable_get_name (CTK_BUILDABLE (button));

  if (strcmp (id, "cutbutton") == 0)
    ctk_text_buffer_cut_clipboard (buffer, clipboard, TRUE);
  else if (strcmp (id, "copybutton") == 0)
    ctk_text_buffer_copy_clipboard (buffer, clipboard);
  else if (strcmp (id, "pastebutton") == 0)
    ctk_text_buffer_paste_clipboard (buffer, clipboard, NULL, TRUE);
  else if (strcmp (id, "deletebutton") == 0)
    ctk_text_buffer_delete_selection (buffer, TRUE, TRUE);
}

static void
clipboard_owner_change (GtkClipboard *clipboard, GdkEvent *event, GtkWidget *button)
{
  const gchar *id;
  gboolean has_text;

  id = ctk_buildable_get_name (CTK_BUILDABLE (button));
  has_text = ctk_clipboard_wait_is_text_available (clipboard);

  if (strcmp (id, "pastebutton") == 0)
    ctk_widget_set_sensitive (button, has_text);
}

static void
textbuffer_notify_selection (GObject *object, GParamSpec *pspec, GtkWidget *button)
{
  const gchar *id;
  gboolean has_selection;

  id = ctk_buildable_get_name (CTK_BUILDABLE (button));
  has_selection = ctk_text_buffer_get_has_selection (CTK_TEXT_BUFFER (object));

  if (strcmp (id, "cutbutton") == 0 ||
      strcmp (id, "copybutton") == 0 ||
      strcmp (id, "deletebutton") == 0)
    ctk_widget_set_sensitive (button, has_selection);
}

static gboolean
osd_frame_button_press (GtkWidget *frame, GdkEventButton *event, gpointer data)
{
  GtkWidget *osd;
  gboolean visible;

  osd = g_object_get_data (G_OBJECT (frame), "osd");
  visible = ctk_widget_get_visible (osd);
  ctk_widget_set_visible (osd, !visible);

  return GDK_EVENT_STOP;
}

static gboolean
page_combo_separator_func (GtkTreeModel *model,
                           GtkTreeIter  *iter,
                           gpointer      data)
{
  gchar *text;
  gboolean res;

  ctk_tree_model_get (model, iter, 0, &text, -1);
  res = g_strcmp0 (text, "-") == 0;
  g_free (text);

  return res;
}

static void
activate_item (GtkWidget *item, GtkTextView *tv)
{
  const gchar *tag;
  GtkTextIter start, end;
  gboolean active;

  g_object_get (item, "active", &active, NULL);
  tag = (const gchar *)g_object_get_data (G_OBJECT (item), "tag");
  ctk_text_buffer_get_selection_bounds (ctk_text_view_get_buffer (tv), &start, &end);
  if (active)
    ctk_text_buffer_apply_tag_by_name (ctk_text_view_get_buffer (tv), tag, &start, &end);
  else
    ctk_text_buffer_remove_tag_by_name (ctk_text_view_get_buffer (tv), tag, &start, &end);
}

static void
add_item (GtkTextView *tv,
          GtkWidget   *popup,
          const gchar *text,
          const gchar *tag,
          gboolean     set)
{
  GtkWidget *item, *label;

  if (CTK_IS_MENU (popup))
    {
      item = ctk_check_menu_item_new ();
      ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (item), set);
      g_signal_connect (item, "toggled", G_CALLBACK (activate_item), tv);
    }
  else
    {
      item = ctk_check_button_new ();
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (item), set);
      ctk_widget_set_focus_on_click (item, FALSE);
      g_signal_connect (item, "clicked", G_CALLBACK (activate_item), tv);
    }

  label = ctk_label_new ("");
  ctk_label_set_xalign (CTK_LABEL (label), 0);
  ctk_label_set_markup (CTK_LABEL (label), text);
  ctk_widget_show (label);
  ctk_container_add (CTK_CONTAINER (item), label);
  g_object_set_data (G_OBJECT (item), "tag", (gpointer)tag);
  ctk_widget_show (item);
  ctk_container_add (CTK_CONTAINER (popup), item);
}

static void
populate_popup (GtkTextView *tv,
                GtkWidget   *popup)
{
  gboolean has_selection;
  GtkWidget *item;
  GtkTextIter start, end, iter;
  GtkTextTagTable *tags;
  GtkTextTag *bold, *italic, *underline;
  gboolean all_bold, all_italic, all_underline;

  has_selection = ctk_text_buffer_get_selection_bounds (ctk_text_view_get_buffer (tv), &start, &end);

  if (!has_selection)
    return;

  tags = ctk_text_buffer_get_tag_table (ctk_text_view_get_buffer (tv));
  bold = ctk_text_tag_table_lookup (tags, "bold");
  italic = ctk_text_tag_table_lookup (tags, "italic");
  underline = ctk_text_tag_table_lookup (tags, "underline");
  all_bold = TRUE;
  all_italic = TRUE;
  all_underline = TRUE;
  ctk_text_iter_assign (&iter, &start);
  while (!ctk_text_iter_equal (&iter, &end))
    {
      all_bold &= ctk_text_iter_has_tag (&iter, bold);
      all_italic &= ctk_text_iter_has_tag (&iter, italic);
      all_underline &= ctk_text_iter_has_tag (&iter, underline);
      ctk_text_iter_forward_char (&iter);
    }

  if (CTK_IS_MENU (popup))
    {
      item = ctk_separator_menu_item_new ();
      ctk_widget_show (item);
      ctk_container_add (CTK_CONTAINER (popup), item);
    }

  add_item (tv, popup, "<b>Bold</b>", "bold", all_bold);
  add_item (tv, popup, "<i>Italics</i>", "italic", all_italic);
  add_item (tv, popup, "<u>Underline</u>", "underline", all_underline);
}

static void
open_popover_text_changed (GtkEntry *entry, GParamSpec *pspec, GtkWidget *button)
{
  const gchar *text;

  text = ctk_entry_get_text (entry);
  ctk_widget_set_sensitive (button, strlen (text) > 0);
}

static gboolean
show_page_again (gpointer data)
{
  ctk_widget_show (CTK_WIDGET (data));
  return G_SOURCE_REMOVE;
}

static void
tab_close_cb (GtkWidget *page)
{
  ctk_widget_hide (page);
  g_timeout_add (2500, show_page_again, page);
}

typedef struct _GTestPermission GTestPermission;
typedef struct _GTestPermissionClass GTestPermissionClass;

struct _GTestPermission
{
  GPermission parent;
};

struct _GTestPermissionClass
{
  GPermissionClass parent_class;
};

G_DEFINE_TYPE (GTestPermission, g_test_permission, G_TYPE_PERMISSION)

static void
g_test_permission_init (GTestPermission *test)
{
  g_permission_impl_update (G_PERMISSION (test), TRUE, TRUE, TRUE);
}

static gboolean
update_allowed (GPermission *permission,
                gboolean     allowed)
{
  g_permission_impl_update (permission, allowed, TRUE, TRUE);

  return TRUE;
}

static gboolean
acquire (GPermission   *permission,
         GCancellable  *cancellable,
         GError       **error)
{
  return update_allowed (permission, TRUE);
}

static void
acquire_async (GPermission         *permission,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
  GTask *task;

  task = g_task_new ((GObject*)permission, NULL, callback, user_data);
  g_task_return_boolean (task, update_allowed (permission, TRUE));
  g_object_unref (task);
}

gboolean
acquire_finish (GPermission   *permission,
                GAsyncResult  *res,
                GError       **error)
{
  return g_task_propagate_boolean (G_TASK (res), error);
}

static gboolean
release (GPermission   *permission,
         GCancellable  *cancellable,
         GError       **error)
{
  return update_allowed (permission, FALSE);
}

static void
release_async (GPermission         *permission,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
  GTask *task;

  task = g_task_new ((GObject*)permission, NULL, callback, user_data);
  g_task_return_boolean (task, update_allowed (permission, FALSE));
  g_object_unref (task);
}

gboolean
release_finish (GPermission   *permission,
                GAsyncResult  *result,
                GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
g_test_permission_class_init (GTestPermissionClass *class)
{
  GPermissionClass *permission_class = G_PERMISSION_CLASS (class);

  permission_class->acquire = acquire;
  permission_class->acquire_async = acquire_async;
  permission_class->acquire_finish = acquire_finish;

  permission_class->release = release;
  permission_class->release_async = release_async;
  permission_class->release_finish = release_finish;
}

static int icon_sizes[5];

static void
register_icon_sizes (void)
{
  static gboolean registered;

  if (registered)
    return;

  registered = TRUE;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  icon_sizes[0] = ctk_icon_size_register ("a", 16, 16);
  icon_sizes[1] = ctk_icon_size_register ("b", 24, 24);
  icon_sizes[2] = ctk_icon_size_register ("c", 32, 32);
  icon_sizes[3] = ctk_icon_size_register ("d", 48, 48);
  icon_sizes[4] = ctk_icon_size_register ("e", 64, 64);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static int
find_icon_size (GtkIconSize size)
{
  gint w, h, w2, h2;
  gint i;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_icon_size_lookup (size, &w, &h);
  for (i = 0; i < G_N_ELEMENTS (icon_sizes); i++)
    {
      ctk_icon_size_lookup (icon_sizes[i], &w2, &h2);
      if (w == w2)
        return i;
    }
G_GNUC_END_IGNORE_DEPRECATIONS

  return 2;
}

static void
update_buttons (GtkWidget *iv, int pos)
{
  GtkWidget *button;

  button = CTK_WIDGET (g_object_get_data (G_OBJECT (iv), "increase_button"));
  ctk_widget_set_sensitive (button, pos + 1 < G_N_ELEMENTS (icon_sizes));
  button = CTK_WIDGET (g_object_get_data (G_OBJECT (iv), "decrease_button"));
  ctk_widget_set_sensitive (button, pos > 0);
}

static void
increase_icon_size (GtkWidget *iv)
{
  GList *cells;
  GtkCellRendererPixbuf *cell;
  GtkIconSize size;
  int i;

  cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (iv));
  cell = cells->data;
  g_list_free (cells);

  g_object_get (cell, "stock-size", &size, NULL);

  i = find_icon_size (size);
  i = CLAMP (i + 1, 0, G_N_ELEMENTS (icon_sizes) - 1);
  size = icon_sizes[i];

  g_object_set (cell, "stock-size", size, NULL);

  update_buttons (iv, i);

  ctk_widget_queue_resize (iv);
}

static void
decrease_icon_size (GtkWidget *iv)
{
  GList *cells;
  GtkCellRendererPixbuf *cell;
  GtkIconSize size;
  int i;

  cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (iv));
  cell = cells->data;
  g_list_free (cells);

  g_object_get (cell, "stock-size", &size, NULL);

  i = find_icon_size (size);
  i = CLAMP (i - 1, 0, G_N_ELEMENTS (icon_sizes) - 1);
  size = icon_sizes[i];

  g_object_set (cell, "stock-size", size, NULL);

  update_buttons (iv, i);

  ctk_widget_queue_resize (iv);
}

static void
reset_icon_size (GtkWidget *iv)
{
  GList *cells;
  GtkCellRendererPixbuf *cell;

  cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (iv));
  cell = cells->data;
  g_list_free (cells);

  g_object_set (cell, "stock-size", icon_sizes[2], NULL);

  update_buttons (iv, 2);

  ctk_widget_queue_resize (iv);
}

static gchar *
scale_format_value_blank (GtkScale *scale, gdouble value)
{
  return g_strdup (" ");
}

static gchar *
scale_format_value (GtkScale *scale, gdouble value)
{
  return g_strdup_printf ("%0.*f", 1, value);
}

static void
adjustment3_value_changed (GtkAdjustment *adj, GtkProgressBar *pbar)
{
  double fraction;

  fraction = ctk_adjustment_get_value (adj) / (ctk_adjustment_get_upper (adj) - ctk_adjustment_get_lower (adj));

  ctk_progress_bar_set_fraction (pbar, fraction);
}

static void
validate_more_details (GtkEntry   *entry,
                       GParamSpec *pspec,
                       GtkEntry   *details)
{
  if (strlen (ctk_entry_get_text (entry)) > 0 &&
      strlen (ctk_entry_get_text (details)) == 0)
    {
      ctk_widget_set_tooltip_text (CTK_WIDGET (entry), "Must have details first");
      ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (entry)), "error");
    }
  else
    {
      ctk_widget_set_tooltip_text (CTK_WIDGET (entry), "");
      ctk_style_context_remove_class (ctk_widget_get_style_context (CTK_WIDGET (entry)), "error");
    }
}

static gboolean
mode_switch_state_set (GtkSwitch *sw, gboolean state)
{
  GtkWidget *dialog = ctk_widget_get_ancestor (CTK_WIDGET (sw), CTK_TYPE_DIALOG);
  GtkWidget *scale = CTK_WIDGET (g_object_get_data (G_OBJECT (dialog), "level_scale"));
  GtkWidget *label = CTK_WIDGET (g_object_get_data (G_OBJECT (dialog), "error_label"));

  if (!state ||
      (ctk_range_get_value (CTK_RANGE (scale)) > 50))
    {
      ctk_widget_hide (label);
      ctk_switch_set_state (sw, state);
    }
  else
    {
      ctk_widget_show (label);
    }

  return TRUE;
}

static void
level_scale_value_changed (GtkRange *range)
{
  GtkWidget *dialog = ctk_widget_get_ancestor (CTK_WIDGET (range), CTK_TYPE_DIALOG);
  GtkWidget *sw = CTK_WIDGET (g_object_get_data (G_OBJECT (dialog), "mode_switch"));
  GtkWidget *label = CTK_WIDGET (g_object_get_data (G_OBJECT (dialog), "error_label"));

  if (ctk_switch_get_active (CTK_SWITCH (sw)) &&
      !ctk_switch_get_state (CTK_SWITCH (sw)) &&
      (ctk_range_get_value (range) > 50))
    {
      ctk_widget_hide (label);
      ctk_switch_set_state (CTK_SWITCH (sw), TRUE);
    }
  else if (ctk_switch_get_state (CTK_SWITCH (sw)) &&
          (ctk_range_get_value (range) <= 50))
    {
      ctk_switch_set_state (CTK_SWITCH (sw), FALSE);
    }
}

static void
activate (GApplication *app)
{
  GtkBuilder *builder;
  GtkWindow *window;
  GtkWidget *widget;
  GtkWidget *widget2;
  GtkWidget *widget3;
  GtkWidget *widget4;
  GtkWidget *stack;
  GtkWidget *dialog;
  GtkAdjustment *adj;
  GtkCssProvider *provider;
  static GActionEntry win_entries[] = {
    { "dark", NULL, NULL, "false", change_theme_state },
    { "transition", NULL, NULL, "false", change_transition_state },
    { "search", activate_search, NULL, NULL, NULL },
    { "delete", activate_delete, NULL, NULL, NULL },
    { "busy", get_busy, NULL, NULL, NULL },
    { "background", activate_background, NULL, NULL, NULL },
    { "open", activate_open, NULL, NULL, NULL },
    { "record", activate_record, NULL, NULL, NULL },
    { "lock", activate_lock, NULL, NULL, NULL },
  };
  struct {
    const gchar *action_and_target;
    const gchar *accelerators[2];
  } accels[] = {
    { "app.about", { "F1", NULL } },
    { "app.quit", { "<Primary>q", NULL } },
    { "win.dark", { "<Primary>d", NULL } },
    { "win.search", { "<Primary>s", NULL } },
    { "win.delete", { "Delete", NULL } },
    { "win.background", { "<Primary>b", NULL } },
    { "win.open", { "<Primary>o", NULL } },
    { "win.record", { "<Primary>r", NULL } },
    { "win.lock", { "<Primary>l", NULL } },
  };
  gint i;
  GPermission *permission;
  GAction *action;

  g_type_ensure (my_text_view_get_type ());
  register_icon_sizes ();

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_resource (provider, "/org/gtk/WidgetFactory/widget-factory.css");
  ctk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             CTK_STYLE_PROVIDER (provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  builder = ctk_builder_new_from_resource ("/org/gtk/WidgetFactory/widget-factory.ui");
  ctk_builder_add_callback_symbol (builder, "on_entry_icon_release", (GCallback)on_entry_icon_release);
  ctk_builder_add_callback_symbol (builder, "on_scale_button_value_changed", (GCallback)on_scale_button_value_changed);
  ctk_builder_add_callback_symbol (builder, "on_scale_button_query_tooltip", (GCallback)on_scale_button_query_tooltip);
  ctk_builder_add_callback_symbol (builder, "on_record_button_toggled", (GCallback)on_record_button_toggled);
  ctk_builder_add_callback_symbol (builder, "on_page_combo_changed", (GCallback)on_page_combo_changed);
  ctk_builder_add_callback_symbol (builder, "on_range_from_changed", (GCallback)on_range_from_changed);
  ctk_builder_add_callback_symbol (builder, "on_range_to_changed", (GCallback)on_range_to_changed);
  ctk_builder_add_callback_symbol (builder, "osd_frame_button_press", (GCallback)osd_frame_button_press);
  ctk_builder_add_callback_symbol (builder, "tab_close_cb", (GCallback)tab_close_cb);
  ctk_builder_add_callback_symbol (builder, "increase_icon_size", (GCallback)increase_icon_size);
  ctk_builder_add_callback_symbol (builder, "decrease_icon_size", (GCallback)decrease_icon_size);
  ctk_builder_add_callback_symbol (builder, "reset_icon_size", (GCallback)reset_icon_size);
  ctk_builder_add_callback_symbol (builder, "scale_format_value", (GCallback)scale_format_value);
  ctk_builder_add_callback_symbol (builder, "scale_format_value_blank", (GCallback)scale_format_value_blank);
  ctk_builder_add_callback_symbol (builder, "validate_more_details", (GCallback)validate_more_details);
  ctk_builder_add_callback_symbol (builder, "mode_switch_state_set", (GCallback)mode_switch_state_set);
  ctk_builder_add_callback_symbol (builder, "level_scale_value_changed", (GCallback)level_scale_value_changed);

  ctk_builder_connect_signals (builder, NULL);

  window = (GtkWindow *)ctk_builder_get_object (builder, "window");
  ctk_application_add_window (CTK_APPLICATION (app), window);
  g_action_map_add_action_entries (G_ACTION_MAP (window),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   window);

  for (i = 0; i < G_N_ELEMENTS (accels); i++)
    ctk_application_set_accels_for_action (CTK_APPLICATION (app), accels[i].action_and_target, accels[i].accelerators);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "statusbar");
  ctk_statusbar_push (CTK_STATUSBAR (widget), 0, "All systems are operating normally.");
  action = G_ACTION (g_property_action_new ("statusbar", widget, "visible"));
  g_action_map_add_action (G_ACTION_MAP (window), action);
  g_object_unref (G_OBJECT (action));

  widget = (GtkWidget *)ctk_builder_get_object (builder, "toolbar");
  action = G_ACTION (g_property_action_new ("toolbar", widget, "visible"));
  g_action_map_add_action (G_ACTION_MAP (window), action);
  g_object_unref (G_OBJECT (action));

  adj = (GtkAdjustment *)ctk_builder_get_object (builder, "adjustment1");

  widget = (GtkWidget *)ctk_builder_get_object (builder, "progressbar3");
  g_signal_connect (adj, "value-changed", G_CALLBACK (update_pulse_time), widget);
  update_pulse_time (adj, widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "entry1");
  g_signal_connect (adj, "value-changed", G_CALLBACK (update_pulse_time), widget);
  update_pulse_time (adj, widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "page2reset");
  adj = (GtkAdjustment *) ctk_builder_get_object (builder, "adjustment2");
  g_signal_connect (widget, "clicked", G_CALLBACK (spin_value_reset), adj);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "page2dismiss");
  g_signal_connect (widget, "clicked", G_CALLBACK (dismiss), NULL);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "page2note");
  adj = (GtkAdjustment *) ctk_builder_get_object (builder, "adjustment2");
  g_signal_connect (adj, "value-changed", G_CALLBACK (spin_value_changed), widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "listbox");
  ctk_list_box_set_header_func (CTK_LIST_BOX (widget), update_header, NULL, NULL);
  g_signal_connect (widget, "row-activated", G_CALLBACK (row_activated), NULL);

  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "listboxrow1switch");
  g_signal_connect (widget2, "notify::active", G_CALLBACK (toggle_selection_mode), widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "listboxrow3");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "listboxrow3image");
  g_object_set_data (G_OBJECT (widget), "image", widget2);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "listboxrow4");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "info_dialog");
  g_object_set_data (G_OBJECT (widget), "dialog", widget2);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "listboxrow5button");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "action_dialog");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (ctk_window_present), widget2);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "toolbar");
  g_object_set_data (G_OBJECT (window), "toolbar", widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "searchbar");
  g_object_set_data (G_OBJECT (window), "searchbar", widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "infobar");
  g_signal_connect (widget, "response", G_CALLBACK (info_bar_response), NULL); 
  g_object_set_data (G_OBJECT (window), "infobar", widget);

  dialog = (GtkWidget *)ctk_builder_get_object (builder, "info_dialog");
  g_signal_connect (dialog, "response", G_CALLBACK (close_dialog), NULL);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "info_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  dialog = (GtkWidget *)ctk_builder_get_object (builder, "action_dialog");
  g_signal_connect (dialog, "response", G_CALLBACK (close_dialog), NULL);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "action_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "act_action_dialog");
  stack = (GtkWidget *)ctk_builder_get_object (builder, "toplevel_stack");
  g_signal_connect (widget, "clicked", G_CALLBACK (action_dialog_button_clicked), stack);
  g_signal_connect (stack, "notify::visible-child-name", G_CALLBACK (page_changed_cb), NULL);
  page_changed_cb (stack, NULL, NULL);

  page_stack = stack;

  dialog = (GtkWidget *)ctk_builder_get_object (builder, "preference_dialog");
  g_signal_connect (dialog, "response", G_CALLBACK (close_dialog), NULL);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "preference_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "circular_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "level_scale");
  g_object_set_data (G_OBJECT (dialog), "level_scale", widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "mode_switch");
  g_object_set_data (G_OBJECT (dialog), "mode_switch", widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "error_label");
  g_object_set_data (G_OBJECT (dialog), "error_label", widget);

  dialog = (GtkWidget *)ctk_builder_get_object (builder, "selection_dialog");
  g_object_set_data (G_OBJECT (window), "selection_dialog", dialog);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "text3");
  g_signal_connect (dialog, "response", G_CALLBACK (close_selection_dialog), widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "selection_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "selection_flowbox");
  g_object_set_data (G_OBJECT (window), "selection_flowbox", widget2);
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (populate_flowbox), widget2);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "charletree");
  populate_model ((GtkTreeStore *)ctk_tree_view_get_model (CTK_TREE_VIEW (widget)));
  ctk_tree_view_set_row_separator_func (CTK_TREE_VIEW (widget), row_separator_func, NULL, NULL);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (widget));

  widget = CTK_WIDGET (ctk_builder_get_object (builder, "munsell"));
  widget2 = CTK_WIDGET (ctk_builder_get_object (builder, "cchooser"));

  populate_colors (widget, widget2);
  g_signal_connect (widget2, "notify::rgba", G_CALLBACK (rgba_changed), widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "page_combo");
  ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (widget), page_combo_separator_func, NULL, NULL);
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "range_from_spin");
  widget3 = (GtkWidget *)ctk_builder_get_object (builder, "range_to_spin");
  widget4 = (GtkWidget *)ctk_builder_get_object (builder, "print_button");
  g_object_set_data (G_OBJECT (widget), "range_from_spin", widget2);
  g_object_set_data (G_OBJECT (widget3), "range_from_spin", widget2);
  g_object_set_data (G_OBJECT (widget), "range_to_spin", widget3);
  g_object_set_data (G_OBJECT (widget2), "range_to_spin", widget3);
  g_object_set_data (G_OBJECT (widget), "print_button", widget4);

  set_accel (CTK_APPLICATION (app), CTK_WIDGET (ctk_builder_get_object (builder, "quitmenuitem")));
  set_accel (CTK_APPLICATION (app), CTK_WIDGET (ctk_builder_get_object (builder, "deletemenuitem")));
  set_accel (CTK_APPLICATION (app), CTK_WIDGET (ctk_builder_get_object (builder, "searchmenuitem")));
  set_accel (CTK_APPLICATION (app), CTK_WIDGET (ctk_builder_get_object (builder, "darkmenuitem")));
  set_accel (CTK_APPLICATION (app), CTK_WIDGET (ctk_builder_get_object (builder, "aboutmenuitem")));
  set_accel (CTK_APPLICATION (app), CTK_WIDGET (ctk_builder_get_object (builder, "bgmenuitem")));

  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "tooltextview");

  widget = (GtkWidget *)ctk_builder_get_object (builder, "toolbutton1");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "toolbutton2");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "toolbutton3");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "toolbutton4");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "cutbutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect (ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget2)), "notify::has-selection",
                    G_CALLBACK (textbuffer_notify_selection), widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "copybutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect (ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget2)), "notify::has-selection",
                    G_CALLBACK (textbuffer_notify_selection), widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "deletebutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect (ctk_text_view_get_buffer (CTK_TEXT_VIEW (widget2)), "notify::has-selection",
                    G_CALLBACK (textbuffer_notify_selection), widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "pastebutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect_object (ctk_widget_get_clipboard (widget2, GDK_SELECTION_CLIPBOARD), "owner-change",
                           G_CALLBACK (clipboard_owner_change), widget, 0);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "osd_frame");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "totem_like_osd");
  g_object_set_data (G_OBJECT (widget), "osd", widget2);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "textview1");
  g_signal_connect (widget, "populate-popup",
                    G_CALLBACK (populate_popup), NULL);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "open_popover");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "open_popover_entry");
  widget3 = (GtkWidget *)ctk_builder_get_object (builder, "open_popover_button");
  ctk_popover_set_default_widget (CTK_POPOVER (widget), widget3);
  g_signal_connect (widget2, "notify::text", G_CALLBACK (open_popover_text_changed), widget3);
  g_signal_connect_swapped (widget3, "clicked", G_CALLBACK (ctk_widget_hide), widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "open_menubutton");
  g_object_set_data (G_OBJECT (window), "open_menubutton", widget);
  widget = (GtkWidget *)ctk_builder_get_object (builder, "record_button");
  g_object_set_data (G_OBJECT (window), "record_button", widget);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "lockbox");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "lockbutton");
  g_object_set_data (G_OBJECT (window), "lockbutton", widget2);
  permission = g_object_new (g_test_permission_get_type (), NULL);
  g_object_bind_property (permission, "allowed",
                          widget, "sensitive",
                          G_BINDING_SYNC_CREATE);
  action = g_action_map_lookup_action (G_ACTION_MAP (window), "open");
  g_object_bind_property (permission, "allowed",
                          action, "enabled",
                          G_BINDING_SYNC_CREATE);
  action = g_action_map_lookup_action (G_ACTION_MAP (window), "record");
  g_object_bind_property (permission, "allowed",
                          action, "enabled",
                          G_BINDING_SYNC_CREATE);
  ctk_lock_button_set_permission (CTK_LOCK_BUTTON (widget2), permission);
  g_object_unref (permission);

  widget = (GtkWidget *)ctk_builder_get_object (builder, "iconview1");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "increase_button");
  g_object_set_data (G_OBJECT (widget), "increase_button", widget2);
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "decrease_button");
  g_object_set_data (G_OBJECT (widget), "decrease_button", widget2);

  adj = (GtkAdjustment *)ctk_builder_get_object (builder, "adjustment3");
  widget = (GtkWidget *)ctk_builder_get_object (builder, "progressbar1");
  widget2 = (GtkWidget *)ctk_builder_get_object (builder, "progressbar2");
  g_signal_connect (adj, "value-changed", G_CALLBACK (adjustment3_value_changed), widget);
  g_signal_connect (adj, "value-changed", G_CALLBACK (adjustment3_value_changed), widget2);

  ctk_widget_show_all (CTK_WIDGET (window));

  g_object_unref (builder);
}

static void
print_version (void)
{
  g_print ("gtk3-widget-factory %d.%d.%d\n",
           ctk_get_major_version (),
           ctk_get_minor_version (),
           ctk_get_micro_version ());
}

static int
local_options (GApplication *app,
               GVariantDict *options,
               gpointer      data)
{
  gboolean version = FALSE;

  g_variant_dict_lookup (options, "version", "b", &version);

  if (version)
    {
      print_version ();
      return 0;
    }

  return -1;
}

int
main (int argc, char *argv[])
{
  GtkApplication *app;
  GAction *action;
  static GActionEntry app_entries[] = {
    { "about", activate_about, NULL, NULL, NULL },
    { "quit", activate_quit, NULL, NULL, NULL },
    { "inspector", activate_inspector, NULL, NULL, NULL },
    { "main", NULL, "s", "'steak'", NULL },
    { "wine", NULL, NULL, "false", NULL },
    { "beer", NULL, NULL, "false", NULL },
    { "water", NULL, NULL, "true", NULL },
    { "dessert", NULL, "s", "'bars'", NULL },
    { "pay", NULL, "s", NULL, NULL }
  };
  gint status;

  app = ctk_application_new ("org.gtk.WidgetFactory", G_APPLICATION_NON_UNIQUE);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
  action = g_action_map_lookup_action (G_ACTION_MAP (app), "wine");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  g_application_add_main_option (G_APPLICATION (app), "version", 0, 0, G_OPTION_ARG_NONE, "Show program version", NULL);

  g_signal_connect (app, "handle-local-options", G_CALLBACK (local_options), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
