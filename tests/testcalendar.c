/* example-start calendar calendar.c */
/*
 * Copyright (C) 1998 Cesar Miquel, Shawn T. Amundson, Mattias Grönlund
 * Copyright (C) 2000 Tony Gale
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#define DEF_PAD 12
#define DEF_PAD_SMALL 6

#define TM_YEAR_BASE 1900

typedef struct _CalendarData
{
  GtkWidget *calendar_widget;
  GtkWidget *flag_checkboxes[6];
  gboolean  settings[6];
  GtkWidget *font_dialog;
  GtkWidget *window;
  GtkWidget *prev2_sig;
  GtkWidget *prev_sig;
  GtkWidget *last_sig;
  GtkWidget *month;

  GHashTable    *details_table;
  GtkTextBuffer *details_buffer;
  gulong         details_changed;
} CalendarData;

enum
{
  calendar_show_header,
  calendar_show_days,
  calendar_month_change, 
  calendar_show_week,
  calendar_monday_first
};

/*
 * GtkCalendar
 */

static void
calendar_date_to_string (CalendarData *data,
			      char         *buffer,
			      gint          buff_len)
{
  GDate *date;
  guint year, month, day;

  ctk_calendar_get_date (CTK_CALENDAR(data->window),
			 &year, &month, &day);
  if (g_date_valid_dmy (day, month + 1, year))
    {
      date = g_date_new_dmy (day, month + 1, year);
      g_date_strftime (buffer, buff_len-1, "%x", date);
      g_date_free (date);
    }
  else
    {
      g_snprintf (buffer, buff_len - 1, "%d/%d/%d (invalid)", month + 1, day, year);
    }
}

static void
calendar_set_detail (CalendarData *data,
                     guint         year,
                     guint         month,
                     guint         day,
                     gchar        *detail)
{
  gchar *key = g_strdup_printf ("%04d-%02d-%02d", year, month + 1, day);
  g_hash_table_replace (data->details_table, key, detail);
}

static gchar*
calendar_get_detail (CalendarData *data,
                     guint         year,
                     guint         month,
                     guint         day)
{
  const gchar *detail;
  gchar *key;

  key = g_strdup_printf ("%04d-%02d-%02d", year, month + 1, day);
  detail = g_hash_table_lookup (data->details_table, key);
  g_free (key);

  return (detail ? g_strdup (detail) : NULL);
}

static void
calendar_update_details (CalendarData *data)
{
  guint year, month, day;
  gchar *detail;

  ctk_calendar_get_date (CTK_CALENDAR (data->calendar_widget), &year, &month, &day);
  detail = calendar_get_detail (data, year, month, day);

  g_signal_handler_block (data->details_buffer, data->details_changed);
  ctk_text_buffer_set_text (data->details_buffer, detail ? detail : "", -1);
  g_signal_handler_unblock (data->details_buffer, data->details_changed);

  g_free (detail);
}

static void
calendar_set_signal_strings (char         *sig_str,
				  CalendarData *data)
{
  const gchar *prev_sig;

  prev_sig = ctk_label_get_text (CTK_LABEL (data->prev_sig));
  ctk_label_set_text (CTK_LABEL (data->prev2_sig), prev_sig);

  prev_sig = ctk_label_get_text (CTK_LABEL (data->last_sig));
  ctk_label_set_text (CTK_LABEL (data->prev_sig), prev_sig);
  ctk_label_set_text (CTK_LABEL (data->last_sig), sig_str);
}

static void
calendar_month_changed (GtkWidget    *widget,
                             CalendarData *data)
{
  char buffer[256] = "month_changed: ";

  calendar_date_to_string (data, buffer+15, 256-15);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_day_selected (GtkWidget    *widget,
                            CalendarData *data)
{
  char buffer[256] = "day_selected: ";

  calendar_date_to_string (data, buffer+14, 256-14);
  calendar_set_signal_strings (buffer, data);

  calendar_update_details (data);
}

static void
calendar_day_selected_double_click (GtkWidget    *widget,
                                         CalendarData *data)
{
  char buffer[256] = "day_selected_double_click: ";
  guint day;

  calendar_date_to_string (data, buffer+27, 256-27);
  calendar_set_signal_strings (buffer, data);
  ctk_calendar_get_date (CTK_CALENDAR (data->window),
                         NULL, NULL, &day);

  if (ctk_calendar_get_day_is_marked (CTK_CALENDAR (data->window), day))
    ctk_calendar_unmark_day (CTK_CALENDAR (data->window), day);
  else
    ctk_calendar_mark_day (CTK_CALENDAR (data->window), day);
}

static void
calendar_prev_month (GtkWidget    *widget,
                          CalendarData *data)
{
  char buffer[256] = "prev_month: ";

  calendar_date_to_string (data, buffer+12, 256-12);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_next_month (GtkWidget    *widget,
                     CalendarData *data)
{
  char buffer[256] = "next_month: ";

  calendar_date_to_string (data, buffer+12, 256-12);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_prev_year (GtkWidget    *widget,
                    CalendarData *data)
{
  char buffer[256] = "prev_year: ";

  calendar_date_to_string (data, buffer+11, 256-11);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_next_year (GtkWidget    *widget,
                    CalendarData *data)
{
  char buffer[256] = "next_year: ";

  calendar_date_to_string (data, buffer+11, 256-11);
  calendar_set_signal_strings (buffer, data);
}


static void
calendar_set_flags (CalendarData *calendar)
{
  gint options = 0, i;

  for (i = 0; i < G_N_ELEMENTS (calendar->settings); i++)
    if (calendar->settings[i])
      options=options + (1 << i);

  if (calendar->window)
    ctk_calendar_set_display_options (CTK_CALENDAR (calendar->window), options);
}

static void
calendar_toggle_flag (GtkWidget    *toggle,
                      CalendarData *calendar)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (calendar->flag_checkboxes); i++)
    if (calendar->flag_checkboxes[i] == toggle)
      calendar->settings[i] = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (toggle));

  calendar_set_flags(calendar);
  
}

void calendar_select_font (GtkWidget    *button,
                                 CalendarData *calendar)
{
  const char *font = NULL;
  GtkCssProvider *provider;
  gchar *data;

  if (calendar->window)
    {
      provider = g_object_get_data (G_OBJECT (calendar->window), "css-provider");
      if (!provider)
        {
          provider = ctk_css_provider_new ();
          ctk_style_context_add_provider (ctk_widget_get_style_context (calendar->window), CTK_STYLE_PROVIDER (provider), CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
          g_object_set_data_full (G_OBJECT (calendar->window), "css-provider", provider, g_object_unref);
        }
      font = ctk_font_chooser_get_font (CTK_FONT_CHOOSER (button));
      data = g_strdup_printf ("GtkCalendar { font: %s; }", font);
      ctk_css_provider_load_from_data (provider, data, -1, NULL);
      g_free (data);
    }
}

static gchar*
calendar_detail_cb (GtkCalendar *calendar,
                    guint        year,
                    guint        month,
                    guint        day,
                    gpointer     data)
{
  return calendar_get_detail (data, year, month, day);
}

static void
calendar_details_changed (GtkTextBuffer *buffer,
                          CalendarData  *data)
{
  GtkTextIter start, end;
  guint year, month, day;
  gchar *detail;

  ctk_text_buffer_get_start_iter(buffer, &start);
  ctk_text_buffer_get_end_iter(buffer, &end);

  ctk_calendar_get_date (CTK_CALENDAR (data->calendar_widget), &year, &month, &day);
  detail = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);

  if (!detail[0])
    {
      g_free (detail);
      detail = NULL;
    }

  calendar_set_detail (data, year, month, day, detail);
  ctk_widget_queue_resize (data->calendar_widget);
}

static void
demonstrate_details (CalendarData *data)
{
  static char *rainbow[] = { "#900", "#980", "#390", "#095", "#059", "#309", "#908" };
  GtkCalendar *calendar = CTK_CALENDAR (data->calendar_widget);
  guint year, month, day;
  gchar *detail;

  ctk_calendar_get_date (calendar,
                         &year, &month, &day);

  for (day = 0; day < 29; ++day)
    {
      detail = g_strdup_printf ("<span color='%s'>yadda\n"
                                "(%04d-%02d-%02d)</span>",
                                rainbow[(day - 1) % 7], year, month, day);
      calendar_set_detail (data, year, month, day, detail);
   }

  ctk_widget_queue_resize (data->calendar_widget);
  calendar_update_details (data);
}

static void
reset_details (CalendarData *data)
{
  g_hash_table_remove_all (data->details_table);
  ctk_widget_queue_resize (data->calendar_widget);
  calendar_update_details (data);
}

static void
calendar_toggle_details (GtkWidget    *widget,
                         CalendarData *data)
{
  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget)))
    ctk_calendar_set_detail_func (CTK_CALENDAR (data->calendar_widget),
                                  calendar_detail_cb, data, NULL);
  else
    ctk_calendar_set_detail_func (CTK_CALENDAR (data->calendar_widget),
                                  NULL, NULL, NULL);
}

static GtkWidget*
create_expander (const char *caption,
                 GtkWidget  *child,
                 GtkAlign    halign,
                 GtkAlign    valign)
{
  GtkWidget *expander = ctk_expander_new ("");
  GtkWidget *label = ctk_expander_get_label_widget (CTK_EXPANDER (expander));

  g_object_set (child,
                "margin-top", 6,
                "margin-bottom", 0,
                "margin-start", 18,
                "margin-end", 0,
                "halign", halign,
                "valign", valign,
                NULL);
  ctk_label_set_markup (CTK_LABEL (label), caption);

  ctk_container_add (CTK_CONTAINER (expander), child);

  return expander;
}

static GtkWidget*
create_frame (const char *caption,
              GtkWidget  *child,
              GtkAlign    halign,
              GtkAlign    valign)
{
  GtkWidget *frame = ctk_frame_new ("");
  GtkWidget *label = ctk_frame_get_label_widget (CTK_FRAME (frame));

  g_object_set (child,
                "margin-top", 6,
                "margin-bottom", 0,
                "margin-start", 18,
                "margin-end", 0,
                "halign", halign,
                "valign", valign,
                NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_NONE);
  ctk_label_set_markup (CTK_LABEL (label), caption);

  ctk_container_add (CTK_CONTAINER (frame), child);

  return frame;
}

static void
detail_width_changed (GtkSpinButton *button,
                      CalendarData  *data)
{
  gint value = (gint) ctk_spin_button_get_value (button);
  ctk_calendar_set_detail_width_chars (CTK_CALENDAR (data->calendar_widget), value);
}

static void
detail_height_changed (GtkSpinButton *button,
                      CalendarData  *data)
{
  gint value = (gint) ctk_spin_button_get_value (button);
  ctk_calendar_set_detail_height_rows (CTK_CALENDAR (data->calendar_widget), value);
}

static void
create_calendar(void)
{
  static CalendarData calendar_data;

  GtkWidget *window, *hpaned, *vbox, *rpane, *hbox;
  GtkWidget *calendar, *toggle, *scroller, *button;
  GtkWidget *frame, *label, *bbox, *details;

  GtkSizeGroup *size;
  GtkStyleContext *context;
  PangoFontDescription *font_desc;
  gchar *font;
  gint i;
  
  struct {
    gboolean init;
    char *label;
  } flags[] =
    {
      { TRUE,  "Show _Heading" },
      { TRUE,  "Show Day _Names" },
      { FALSE, "No Month _Change" },
      { TRUE,  "Show _Week Numbers" },
      { FALSE, "Week Start _Monday" },
      { TRUE,  "Show De_tails" },
    };

  calendar_data.window = NULL;
  calendar_data.font_dialog = NULL;
  calendar_data.details_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  for (i = 0; i < G_N_ELEMENTS (calendar_data.settings); i++)
    calendar_data.settings[i] = 0;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "GtkCalendar Example");
  ctk_container_set_border_width (CTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (ctk_main_quit),
		    NULL);
  g_signal_connect (window, "delete-event",
		    G_CALLBACK (ctk_false),
		    NULL);

  hpaned = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);

  /* Calendar widget */

  calendar = ctk_calendar_new ();
  calendar_data.calendar_widget = calendar;
  frame = create_frame ("<b>Calendar</b>", calendar, CTK_ALIGN_CENTER, CTK_ALIGN_CENTER);
  ctk_paned_pack1 (CTK_PANED (hpaned), frame, TRUE, FALSE);

  calendar_data.window = calendar;
  calendar_set_flags(&calendar_data);
  ctk_calendar_mark_day (CTK_CALENDAR (calendar), 19);	

  g_signal_connect (calendar, "month_changed", 
		    G_CALLBACK (calendar_month_changed),
		    &calendar_data);
  g_signal_connect (calendar, "day_selected", 
		    G_CALLBACK (calendar_day_selected),
		    &calendar_data);
  g_signal_connect (calendar, "day_selected_double_click", 
		    G_CALLBACK (calendar_day_selected_double_click),
		    &calendar_data);
  g_signal_connect (calendar, "prev_month", 
		    G_CALLBACK (calendar_prev_month),
		    &calendar_data);
  g_signal_connect (calendar, "next_month", 
		    G_CALLBACK (calendar_next_month),
		    &calendar_data);
  g_signal_connect (calendar, "prev_year", 
		    G_CALLBACK (calendar_prev_year),
		    &calendar_data);
  g_signal_connect (calendar, "next_year", 
		    G_CALLBACK (calendar_next_year),
		    &calendar_data);

  rpane = ctk_box_new (CTK_ORIENTATION_VERTICAL, DEF_PAD_SMALL);
  ctk_paned_pack2 (CTK_PANED (hpaned), rpane, FALSE, FALSE);

  /* Build the right font-button */

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, DEF_PAD_SMALL);
  frame = create_frame ("<b>Options</b>", vbox, CTK_ALIGN_FILL, CTK_ALIGN_CENTER);
  ctk_box_pack_start (CTK_BOX (rpane), frame, FALSE, TRUE, 0);
  size = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);

  context = ctk_widget_get_style_context (calendar);
  ctk_style_context_get (context, CTK_STATE_FLAG_NORMAL,
			 CTK_STYLE_PROPERTY_FONT, &font_desc, NULL);
  font = pango_font_description_to_string (font_desc);
  button = ctk_font_button_new_with_font (font);
  g_free (font);
  pango_font_description_free (font_desc);

  g_signal_connect (button, "font-set",
                    G_CALLBACK(calendar_select_font),
                    &calendar_data);

  label = ctk_label_new_with_mnemonic ("_Font:");
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), button);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_size_group_add_widget (size, label);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, DEF_PAD_SMALL);
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  /* Build the width entry */

  button = ctk_spin_button_new_with_range (0, 127, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (button),
                             ctk_calendar_get_detail_width_chars (CTK_CALENDAR (calendar)));

  g_signal_connect (button, "value-changed",
                    G_CALLBACK (detail_width_changed),
                    &calendar_data);

  label = ctk_label_new_with_mnemonic ("Details W_idth:");
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), button);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_size_group_add_widget (size, label);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, DEF_PAD_SMALL);
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  /* Build the height entry */

  button = ctk_spin_button_new_with_range (0, 127, 1);
  ctk_spin_button_set_value (CTK_SPIN_BUTTON (button),
                             ctk_calendar_get_detail_height_rows (CTK_CALENDAR (calendar)));

  g_signal_connect (button, "value-changed",
                    G_CALLBACK (detail_height_changed),
                    &calendar_data);

  label = ctk_label_new_with_mnemonic ("Details H_eight:");
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), button);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_size_group_add_widget (size, label);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, DEF_PAD_SMALL);
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  /* Build the right details frame */

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, DEF_PAD_SMALL);
  frame = create_frame ("<b>Details</b>", vbox, CTK_ALIGN_FILL, CTK_ALIGN_FILL);
  ctk_box_pack_start (CTK_BOX (rpane), frame, FALSE, TRUE, 0);

  details = ctk_text_view_new();
  calendar_data.details_buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (details));

  calendar_data.details_changed = g_signal_connect (calendar_data.details_buffer, "changed",
                                                    G_CALLBACK (calendar_details_changed),
                                                    &calendar_data);

  scroller = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (scroller), details);

  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scroller),
                                       CTK_SHADOW_IN);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scroller),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);

  ctk_box_pack_start (CTK_BOX (vbox), scroller, FALSE, TRUE, 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, DEF_PAD_SMALL);
  ctk_widget_set_halign (hbox, CTK_ALIGN_START);
  ctk_widget_set_valign (hbox, CTK_ALIGN_CENTER);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  button = ctk_button_new_with_mnemonic ("Demonstrate _Details");

  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (demonstrate_details),
                            &calendar_data);

  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = ctk_button_new_with_mnemonic ("_Reset Details");

  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (reset_details),
                            &calendar_data);

  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, TRUE, 0);

  toggle = ctk_check_button_new_with_mnemonic ("_Use Details");
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK(calendar_toggle_details),
                    &calendar_data);
  ctk_box_pack_start (CTK_BOX (vbox), toggle, FALSE, TRUE, 0);
  
  /* Build the Right frame with the flags in */ 

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  frame = create_expander ("<b>Flags</b>", vbox, CTK_ALIGN_FILL, CTK_ALIGN_CENTER);
  ctk_box_pack_start (CTK_BOX (rpane), frame, TRUE, TRUE, 0);

  for (i = 0; i < G_N_ELEMENTS (calendar_data.settings); i++)
    {
      toggle = ctk_check_button_new_with_mnemonic(flags[i].label);
      ctk_box_pack_start (CTK_BOX (vbox), toggle, FALSE, TRUE, 0);
      calendar_data.flag_checkboxes[i] = toggle;

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (calendar_toggle_flag),
			&calendar_data);

      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (toggle), flags[i].init);
    }

  /*
   *  Build the Signal-event part.
   */

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, DEF_PAD_SMALL);
  ctk_box_set_homogeneous (CTK_BOX (vbox), TRUE);
  frame = create_frame ("<b>Signal Events</b>", vbox, CTK_ALIGN_FILL, CTK_ALIGN_CENTER);
  
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 3);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  label = ctk_label_new ("Signal:");
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.last_sig = ctk_label_new ("");
  ctk_box_pack_start (CTK_BOX (hbox), calendar_data.last_sig, FALSE, TRUE, 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 3);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  label = ctk_label_new ("Previous signal:");
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.prev_sig = ctk_label_new ("");
  ctk_box_pack_start (CTK_BOX (hbox), calendar_data.prev_sig, FALSE, TRUE, 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 3);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  label = ctk_label_new ("Second previous signal:");
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.prev2_sig = ctk_label_new ("");
  ctk_box_pack_start (CTK_BOX (hbox), calendar_data.prev2_sig, FALSE, TRUE, 0);

  /*
   *  Glue everything together
   */

  bbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_button_box_set_layout(CTK_BUTTON_BOX(bbox), CTK_BUTTONBOX_END);

  button = ctk_button_new_with_label ("Close");
  g_signal_connect (button, "clicked", G_CALLBACK (ctk_main_quit), NULL);
  ctk_container_add (CTK_CONTAINER (bbox), button);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, DEF_PAD_SMALL);

  ctk_box_pack_start (CTK_BOX (vbox), hpaned,
                      TRUE,  TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL),
                      FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), frame,
                      FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL),
                      FALSE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), bbox,
                      FALSE, TRUE, 0);

  ctk_container_add (CTK_CONTAINER (window), vbox);

  ctk_widget_set_can_default (button, TRUE);
  ctk_widget_grab_default (button);

  ctk_window_set_default_size (CTK_WINDOW (window), 600, 0);
  ctk_widget_show_all (window);
}


int main(int   argc,
         char *argv[] )
{
  ctk_init (&argc, &argv);

  if (g_getenv ("CTK_RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  create_calendar();

  ctk_main();

  return(0);
}
/* example-end */
