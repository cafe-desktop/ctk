/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * CTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel, Shawn T. Amundson and Mattias Groenlund
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

/**
 * SECTION:ctkcalendar
 * @Short_description: Displays a calendar and allows the user to select a date
 * @Title: CtkCalendar
 *
 * #CtkCalendar is a widget that displays a Gregorian calendar, one month
 * at a time. It can be created with ctk_calendar_new().
 *
 * The month and year currently displayed can be altered with
 * ctk_calendar_select_month(). The exact day can be selected from the
 * displayed month using ctk_calendar_select_day().
 *
 * To place a visual marker on a particular day, use ctk_calendar_mark_day()
 * and to remove the marker, ctk_calendar_unmark_day(). Alternative, all
 * marks can be cleared with ctk_calendar_clear_marks().
 *
 * The way in which the calendar itself is displayed can be altered using
 * ctk_calendar_set_display_options().
 *
 * The selected date can be retrieved from a #CtkCalendar using
 * ctk_calendar_get_date().
 *
 * Users should be aware that, although the Gregorian calendar is the
 * legal calendar in most countries, it was adopted progressively
 * between 1582 and 1929. Display before these dates is likely to be
 * historically incorrect.
 */

#include "config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
#include <langinfo.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "ctkcalendar.h"
#include "ctkdnd.h"
#include "ctkdragdest.h"
#include "ctkintl.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctktooltip.h"
#include "ctkprivate.h"
#include "ctkrender.h"

#define TIMEOUT_INITIAL  500
#define TIMEOUT_REPEAT    50

static const guint month_length[2][13] =
{
  { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static gboolean
leap (guint year)
{
  return ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));
}

static guint
day_of_week (guint year, guint mm, guint dd)
{
  GDateTime *dt;
  guint days;

  dt = g_date_time_new_local (year, mm, dd, 1, 1, 1);
  if (dt == NULL)
    return 0;

  days = g_date_time_get_day_of_week (dt);
  g_date_time_unref (dt);

  return days;
}

static guint
week_of_year (guint year, guint mm, guint dd)
{
  GDateTime *dt;
  guint week;

  dt = g_date_time_new_local (year, mm, dd, 1, 1, 1);
  if (dt == NULL)
    return 1;

  week = g_date_time_get_week_of_year (dt);
  g_date_time_unref (dt);

  return week;
}

/* Spacing around day/week headers and main area, inside those windows */
#define CALENDAR_MARGIN          0

#define DAY_XSEP                 0 /* not really good for small calendar */
#define DAY_YSEP                 0 /* not really good for small calendar */

#define SCROLL_DELAY_FACTOR      5

enum {
  ARROW_YEAR_LEFT,
  ARROW_YEAR_RIGHT,
  ARROW_MONTH_LEFT,
  ARROW_MONTH_RIGHT
};

enum {
  MONTH_PREV,
  MONTH_CURRENT,
  MONTH_NEXT
};

enum {
  MONTH_CHANGED_SIGNAL,
  DAY_SELECTED_SIGNAL,
  DAY_SELECTED_DOUBLE_CLICK_SIGNAL,
  PREV_MONTH_SIGNAL,
  NEXT_MONTH_SIGNAL,
  PREV_YEAR_SIGNAL,
  NEXT_YEAR_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_YEAR,
  PROP_MONTH,
  PROP_DAY,
  PROP_SHOW_HEADING,
  PROP_SHOW_DAY_NAMES,
  PROP_NO_MONTH_CHANGE,
  PROP_SHOW_WEEK_NUMBERS,
  PROP_SHOW_DETAILS,
  PROP_DETAIL_WIDTH_CHARS,
  PROP_DETAIL_HEIGHT_ROWS
};

static guint ctk_calendar_signals[LAST_SIGNAL] = { 0 };

struct _CtkCalendarPrivate
{
  CtkCalendarDisplayOptions display_flags;

  CdkWindow *main_win;
  CdkWindow *arrow_win[4];

  gchar grow_space [32];

  gint  month;
  gint  year;
  gint  selected_day;

  gint  day_month[6][7];
  gint  day[6][7];

  gint  num_marked_dates;
  gint  marked_date[31];

  gint  focus_row;
  gint  focus_col;

  guint header_h;
  guint day_name_h;
  guint main_h;

  guint arrow_prelight : 4;
  guint arrow_width;
  guint max_month_width;
  guint max_year_width;

  guint day_width;
  guint week_width;

  guint min_day_width;
  guint max_day_char_width;
  guint max_day_char_ascent;
  guint max_day_char_descent;
  guint max_label_char_ascent;
  guint max_label_char_descent;
  guint max_week_char_width;

  /* flags */
  guint year_before : 1;

  guint need_timer  : 1;

  guint in_drag : 1;
  guint drag_highlight : 1;

  guint32 timer;
  gint click_child;

  gint week_start;

  gint drag_start_x;
  gint drag_start_y;

  /* Optional callback, used to display extra information for each day. */
  CtkCalendarDetailFunc detail_func;
  gpointer              detail_func_user_data;
  GDestroyNotify        detail_func_destroy;

  /* Size requistion for details provided by the hook. */
  gint detail_height_rows;
  gint detail_width_chars;
  gint detail_overflow[6];
};

static void ctk_calendar_finalize     (GObject      *calendar);
static void ctk_calendar_destroy      (CtkWidget    *widget);
static void ctk_calendar_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void ctk_calendar_get_property (GObject      *object,
                                       guint         prop_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);

static void     ctk_calendar_realize        (CtkWidget        *widget);
static void     ctk_calendar_unrealize      (CtkWidget        *widget);
static void     ctk_calendar_map            (CtkWidget        *widget);
static void     ctk_calendar_unmap          (CtkWidget        *widget);
static void     ctk_calendar_get_preferred_width  (CtkWidget   *widget,
                                                   gint        *minimum,
                                                   gint        *natural);
static void     ctk_calendar_get_preferred_height (CtkWidget   *widget,
                                                   gint        *minimum,
                                                   gint        *natural);
static void     ctk_calendar_size_allocate  (CtkWidget        *widget,
                                             CtkAllocation    *allocation);
static gboolean ctk_calendar_draw           (CtkWidget        *widget,
                                             cairo_t          *cr);
static gboolean ctk_calendar_button_press   (CtkWidget        *widget,
                                             CdkEventButton   *event);
static gboolean ctk_calendar_button_release (CtkWidget        *widget,
                                             CdkEventButton   *event);
static gboolean ctk_calendar_motion_notify  (CtkWidget        *widget,
                                             CdkEventMotion   *event);
static gboolean ctk_calendar_enter_notify   (CtkWidget        *widget,
                                             CdkEventCrossing *event);
static gboolean ctk_calendar_leave_notify   (CtkWidget        *widget,
                                             CdkEventCrossing *event);
static gboolean ctk_calendar_scroll         (CtkWidget        *widget,
                                             CdkEventScroll   *event);
static gboolean ctk_calendar_key_press      (CtkWidget        *widget,
                                             CdkEventKey      *event);
static gboolean ctk_calendar_focus_out      (CtkWidget        *widget,
                                             CdkEventFocus    *event);
static void     ctk_calendar_grab_notify    (CtkWidget        *widget,
                                             gboolean          was_grabbed);
static void     ctk_calendar_state_flags_changed  (CtkWidget     *widget,
                                                   CtkStateFlags  previous_state);
static gboolean ctk_calendar_query_tooltip  (CtkWidget        *widget,
                                             gint              x,
                                             gint              y,
                                             gboolean          keyboard_mode,
                                             CtkTooltip       *tooltip);

static void     ctk_calendar_drag_data_get      (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 CtkSelectionData *selection_data,
                                                 guint             info,
                                                 guint             time);
static void     ctk_calendar_drag_data_received (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 gint              x,
                                                 gint              y,
                                                 CtkSelectionData *selection_data,
                                                 guint             info,
                                                 guint             time);
static gboolean ctk_calendar_drag_motion        (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 gint              x,
                                                 gint              y,
                                                 guint             time);
static void     ctk_calendar_drag_leave         (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 guint             time);
static gboolean ctk_calendar_drag_drop          (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 gint              x,
                                                 gint              y,
                                                 guint             time);


static void calendar_start_spinning (CtkCalendar *calendar,
                                     gint         click_child);
static void calendar_stop_spinning  (CtkCalendar *calendar);

static void calendar_invalidate_day     (CtkCalendar *widget,
                                         gint       row,
                                         gint       col);
static void calendar_invalidate_day_num (CtkCalendar *widget,
                                         gint       day);
static void calendar_invalidate_arrow   (CtkCalendar *widget,
                                         guint      arrow);

static void calendar_compute_days      (CtkCalendar *calendar);
static gint calendar_get_xsep          (CtkCalendar *calendar);
static gint calendar_get_ysep          (CtkCalendar *calendar);
static gint calendar_get_inner_border  (CtkCalendar *calendar);

static char    *default_abbreviated_dayname[7];
static char    *default_monthname[12];

G_DEFINE_TYPE_WITH_PRIVATE (CtkCalendar, ctk_calendar, CTK_TYPE_WIDGET)

static void
ctk_calendar_class_init (CtkCalendarClass *class)
{
  GObjectClass   *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = (GObjectClass*)  class;
  widget_class = (CtkWidgetClass*) class;

  gobject_class->set_property = ctk_calendar_set_property;
  gobject_class->get_property = ctk_calendar_get_property;
  gobject_class->finalize = ctk_calendar_finalize;

  widget_class->destroy = ctk_calendar_destroy;
  widget_class->realize = ctk_calendar_realize;
  widget_class->unrealize = ctk_calendar_unrealize;
  widget_class->map = ctk_calendar_map;
  widget_class->unmap = ctk_calendar_unmap;
  widget_class->draw = ctk_calendar_draw;
  widget_class->get_preferred_width = ctk_calendar_get_preferred_width;
  widget_class->get_preferred_height = ctk_calendar_get_preferred_height;
  widget_class->size_allocate = ctk_calendar_size_allocate;
  widget_class->button_press_event = ctk_calendar_button_press;
  widget_class->button_release_event = ctk_calendar_button_release;
  widget_class->motion_notify_event = ctk_calendar_motion_notify;
  widget_class->enter_notify_event = ctk_calendar_enter_notify;
  widget_class->leave_notify_event = ctk_calendar_leave_notify;
  widget_class->key_press_event = ctk_calendar_key_press;
  widget_class->scroll_event = ctk_calendar_scroll;
  widget_class->state_flags_changed = ctk_calendar_state_flags_changed;
  widget_class->grab_notify = ctk_calendar_grab_notify;
  widget_class->focus_out_event = ctk_calendar_focus_out;
  widget_class->query_tooltip = ctk_calendar_query_tooltip;

  widget_class->drag_data_get = ctk_calendar_drag_data_get;
  widget_class->drag_motion = ctk_calendar_drag_motion;
  widget_class->drag_leave = ctk_calendar_drag_leave;
  widget_class->drag_drop = ctk_calendar_drag_drop;
  widget_class->drag_data_received = ctk_calendar_drag_data_received;

  /**
   * CtkCalendar:year:
   *
   * The selected year.
   * This property gets initially set to the current year.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_YEAR,
                                   g_param_spec_int ("year",
                                                     P_("Year"),
                                                     P_("The selected year"),
                                                     0, G_MAXINT >> 9, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkCalendar:month:
   *
   * The selected month (as a number between 0 and 11).
   * This property gets initially set to the current month.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MONTH,
                                   g_param_spec_int ("month",
                                                     P_("Month"),
                                                     P_("The selected month (as a number between 0 and 11)"),
                                                     0, 11, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkCalendar:day:
   *
   * The selected day (as a number between 1 and 31, or 0
   * to unselect the currently selected day).
   * This property gets initially set to the current day.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_DAY,
                                   g_param_spec_int ("day",
                                                     P_("Day"),
                                                     P_("The selected day (as a number between 1 and 31, or 0 to unselect the currently selected day)"),
                                                     0, 31, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

/**
 * CtkCalendar:show-heading:
 *
 * Determines whether a heading is displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_HEADING,
                                   g_param_spec_boolean ("show-heading",
                                                         P_("Show Heading"),
                                                         P_("If TRUE, a heading is displayed"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

/**
 * CtkCalendar:show-day-names:
 *
 * Determines whether day names are displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_DAY_NAMES,
                                   g_param_spec_boolean ("show-day-names",
                                                         P_("Show Day Names"),
                                                         P_("If TRUE, day names are displayed"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
/**
 * CtkCalendar:no-month-change:
 *
 * Determines whether the selected month can be changed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_NO_MONTH_CHANGE,
                                   g_param_spec_boolean ("no-month-change",
                                                         P_("No Month Change"),
                                                         P_("If TRUE, the selected month cannot be changed"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

/**
 * CtkCalendar:show-week-numbers:
 *
 * Determines whether week numbers are displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_WEEK_NUMBERS,
                                   g_param_spec_boolean ("show-week-numbers",
                                                         P_("Show Week Numbers"),
                                                         P_("If TRUE, week numbers are displayed"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

/**
 * CtkCalendar:detail-width-chars:
 *
 * Width of a detail cell, in characters.
 * A value of 0 allows any width. See ctk_calendar_set_detail_func().
 *
 * Since: 2.14
 */
  g_object_class_install_property (gobject_class,
                                   PROP_DETAIL_WIDTH_CHARS,
                                   g_param_spec_int ("detail-width-chars",
                                                     P_("Details Width"),
                                                     P_("Details width in characters"),
                                                     0, 127, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

/**
 * CtkCalendar:detail-height-rows:
 *
 * Height of a detail cell, in rows.
 * A value of 0 allows any width. See ctk_calendar_set_detail_func().
 *
 * Since: 2.14
 */
  g_object_class_install_property (gobject_class,
                                   PROP_DETAIL_HEIGHT_ROWS,
                                   g_param_spec_int ("detail-height-rows",
                                                     P_("Details Height"),
                                                     P_("Details height in rows"),
                                                     0, 127, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

/**
 * CtkCalendar:show-details:
 *
 * Determines whether details are shown directly in the widget, or if they are
 * available only as tooltip. When this property is set days with details are
 * marked.
 *
 * Since: 2.14
 */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_DETAILS,
                                   g_param_spec_boolean ("show-details",
                                                         P_("Show Details"),
                                                         P_("If TRUE, details are shown"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * CtkCalendar:inner-border:
   *
   * The spacing around the day/week headers and main area.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("inner-border",
                                                             P_("Inner border"),
                                                             P_("Inner border space"),
                                                             0, G_MAXINT, 4,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkCalndar:vertical-separation:
   *
   * Separation between day headers and main area.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("vertical-separation",
                                                             P_("Vertical separation"),
                                                             P_("Space between day headers and main area"),
                                                             0, G_MAXINT, 4,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkCalendar:horizontal-separation:
   *
   * Separation between week headers and main area.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("horizontal-separation",
                                                             P_("Horizontal separation"),
                                                             P_("Space between week headers and main area"),
                                                             0, G_MAXINT, 4,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkCalendar::month-changed:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user clicks a button to change the selected month on a
   * calendar.
   */
  ctk_calendar_signals[MONTH_CHANGED_SIGNAL] =
    g_signal_new (I_("month-changed"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCalendarClass, month_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkCalendar::day-selected:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user selects a day.
   */
  ctk_calendar_signals[DAY_SELECTED_SIGNAL] =
    g_signal_new (I_("day-selected"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCalendarClass, day_selected),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkCalendar::day-selected-double-click:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user double-clicks a day.
   */
  ctk_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL] =
    g_signal_new (I_("day-selected-double-click"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCalendarClass, day_selected_double_click),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkCalendar::prev-month:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user switched to the previous month.
   */
  ctk_calendar_signals[PREV_MONTH_SIGNAL] =
    g_signal_new (I_("prev-month"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCalendarClass, prev_month),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkCalendar::next-month:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user switched to the next month.
   */
  ctk_calendar_signals[NEXT_MONTH_SIGNAL] =
    g_signal_new (I_("next-month"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCalendarClass, next_month),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkCalendar::prev-year:
   * @calendar: the object which received the signal.
   *
   * Emitted when user switched to the previous year.
   */
  ctk_calendar_signals[PREV_YEAR_SIGNAL] =
    g_signal_new (I_("prev-year"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCalendarClass, prev_year),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkCalendar::next-year:
   * @calendar: the object which received the signal.
   *
   * Emitted when user switched to the next year.
   */
  ctk_calendar_signals[NEXT_YEAR_SIGNAL] =
    g_signal_new (I_("next-year"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCalendarClass, next_year),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_CALENDAR);
  ctk_widget_class_set_css_name (widget_class, "calendar");
}

static void
ctk_calendar_init (CtkCalendar *calendar)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  time_t secs;
  struct tm *tm;
  gint i;
#ifdef G_OS_WIN32
  wchar_t wbuffer[100];
#else
  static const char *month_format = NULL;
  char buffer[255];
  time_t tmp_time;
#endif
  CtkCalendarPrivate *priv;
  gchar *year_before;
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  union { unsigned int word; char *string; } langinfo;
  gint week_1stday = 0;
  gint first_weekday = 1;
  guint week_origin;
#else
  gchar *week_start;
#endif

  priv = calendar->priv = ctk_calendar_get_instance_private (calendar);

  ctk_widget_set_can_focus (widget, TRUE);
  ctk_widget_set_has_window (widget, FALSE);

  if (!default_abbreviated_dayname[0])
    for (i=0; i<7; i++)
      {
#ifndef G_OS_WIN32
        tmp_time= (i+3)*86400;
        strftime (buffer, sizeof (buffer), "%a", gmtime (&tmp_time));
        default_abbreviated_dayname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
#else
        if (!GetLocaleInfoW (GetThreadLocale (), LOCALE_SABBREVDAYNAME1 + (i+6)%7,
                             wbuffer, G_N_ELEMENTS (wbuffer)))
          default_abbreviated_dayname[i] = g_strdup_printf ("(%d)", i);
        else
          default_abbreviated_dayname[i] = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);
#endif
      }

  if (!default_monthname[0])
    for (i=0; i<12; i++)
      {
#ifndef G_OS_WIN32
        tmp_time=i*2764800;
        if (G_UNLIKELY (month_format == NULL))
          {
            buffer[0] = '\0';
            month_format = "%OB";
            strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));
            /* "%OB" is not supported in Linux with glibc < 2.27  */
            if (!strcmp (buffer, "%OB") || !strcmp (buffer, "OB") || !strcmp (buffer, ""))
              {
                month_format = "%B";
                strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));
              }
          }
        else
          strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));

        default_monthname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
#else
        if (!GetLocaleInfoW (GetThreadLocale (), LOCALE_SMONTHNAME1 + i,
                             wbuffer, G_N_ELEMENTS (wbuffer)))
          default_monthname[i] = g_strdup_printf ("(%d)", i);
        else
          default_monthname[i] = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);
#endif
      }

  /* Set defaults */
  secs = time (NULL);
  tm = localtime (&secs);
  priv->month = tm->tm_mon;
  priv->year  = 1900 + tm->tm_year;

  for (i=0;i<31;i++)
    priv->marked_date[i] = FALSE;
  priv->num_marked_dates = 0;
  priv->selected_day = tm->tm_mday;

  priv->display_flags = (CTK_CALENDAR_SHOW_HEADING |
                             CTK_CALENDAR_SHOW_DAY_NAMES |
                             CTK_CALENDAR_SHOW_DETAILS);

  priv->focus_row = -1;
  priv->focus_col = -1;

  priv->max_year_width = 0;
  priv->max_month_width = 0;
  priv->max_day_char_width = 0;
  priv->max_week_char_width = 0;

  priv->max_day_char_ascent = 0;
  priv->max_day_char_descent = 0;
  priv->max_label_char_ascent = 0;
  priv->max_label_char_descent = 0;

  priv->arrow_width = 10;

  priv->need_timer = 0;
  priv->timer = 0;
  priv->click_child = -1;

  priv->in_drag = 0;
  priv->drag_highlight = 0;

  ctk_drag_dest_set (widget, 0, NULL, 0, CDK_ACTION_COPY);
  ctk_drag_dest_add_text_targets (widget);

  priv->year_before = 0;

  /* Translate to calendar:YM if you want years to be displayed
   * before months; otherwise translate to calendar:MY.
   * Do *not* translate it to anything else, if it
   * it isn't calendar:YM or calendar:MY it will not work.
   *
   * Note that the ordering described here is logical order, which is
   * further influenced by BIDI ordering. Thus, if you have a default
   * text direction of RTL and specify "calendar:YM", then the year
   * will appear to the right of the month.
   */
  year_before = _("calendar:MY");
  if (strcmp (year_before, "calendar:YM") == 0)
    priv->year_before = 1;
  else if (strcmp (year_before, "calendar:MY") != 0)
    g_warning ("Whoever translated calendar:MY did so wrongly.");

#ifdef G_OS_WIN32
  priv->week_start = 0;
  week_start = NULL;

  if (GetLocaleInfoW (GetThreadLocale (), LOCALE_IFIRSTDAYOFWEEK,
                      wbuffer, G_N_ELEMENTS (wbuffer)))
    week_start = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);

  if (week_start != NULL)
    {
      priv->week_start = (week_start[0] - '0' + 1) % 7;
      g_free(week_start);
    }
#else
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  langinfo.string = nl_langinfo (_NL_TIME_FIRST_WEEKDAY);
  first_weekday = langinfo.string[0];
  langinfo.string = nl_langinfo (_NL_TIME_WEEK_1STDAY);
  week_origin = langinfo.word;
  if (week_origin == 19971130) /* Sunday */
    week_1stday = 0;
  else if (week_origin == 19971201) /* Monday */
    week_1stday = 1;
  else
    g_warning ("Unknown value of _NL_TIME_WEEK_1STDAY.");

  priv->week_start = (week_1stday + first_weekday - 1) % 7;
#else
  /* Translate to calendar:week_start:0 if you want Sunday to be the
   * first day of the week to calendar:week_start:1 if you want Monday
   * to be the first day of the week, and so on.
   */
  week_start = _("calendar:week_start:0");

  if (strncmp (week_start, "calendar:week_start:", 20) == 0)
    priv->week_start = *(week_start + 20) - '0';
  else
    priv->week_start = -1;

  if (priv->week_start < 0 || priv->week_start > 6)
    {
      g_warning ("Whoever translated calendar:week_start:0 did so wrongly.");
      priv->week_start = 0;
    }
#endif
#endif

  calendar_compute_days (calendar);
}


/****************************************
 *          Utility Functions           *
 ****************************************/

static void
calendar_queue_refresh (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;

  if (!(priv->detail_func) ||
      !(priv->display_flags & CTK_CALENDAR_SHOW_DETAILS) ||
       (priv->detail_width_chars && priv->detail_height_rows))
    ctk_widget_queue_draw (CTK_WIDGET (calendar));
  else
    ctk_widget_queue_resize (CTK_WIDGET (calendar));
}

static void
calendar_set_month_next (CtkCalendar *calendar)
{
  gint month_len;
  CtkCalendarPrivate *priv = calendar->priv;

  if (priv->display_flags & CTK_CALENDAR_NO_MONTH_CHANGE)
    return;

  if (priv->month == 11)
    {
      priv->month = 0;
      priv->year++;
    }
  else
    priv->month++;

  calendar_compute_days (calendar);
  g_signal_emit (calendar,
                 ctk_calendar_signals[NEXT_MONTH_SIGNAL],
                 0);
  g_signal_emit (calendar,
                 ctk_calendar_signals[MONTH_CHANGED_SIGNAL],
                 0);

  month_len = month_length[leap (priv->year)][priv->month + 1];

  if (month_len < priv->selected_day)
    {
      priv->selected_day = 0;
      ctk_calendar_select_day (calendar, month_len);
    }
  else
    ctk_calendar_select_day (calendar, priv->selected_day);

  calendar_queue_refresh (calendar);
}

static void
calendar_set_year_prev (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint month_len;

  priv->year--;
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
                 ctk_calendar_signals[PREV_YEAR_SIGNAL],
                 0);
  g_signal_emit (calendar,
                 ctk_calendar_signals[MONTH_CHANGED_SIGNAL],
                 0);

  month_len = month_length[leap (priv->year)][priv->month + 1];

  if (month_len < priv->selected_day)
    {
      priv->selected_day = 0;
      ctk_calendar_select_day (calendar, month_len);
    }
  else
    ctk_calendar_select_day (calendar, priv->selected_day);

  calendar_queue_refresh (calendar);
}

static void
calendar_set_year_next (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint month_len;

  priv->year++;
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
                 ctk_calendar_signals[NEXT_YEAR_SIGNAL],
                 0);
  g_signal_emit (calendar,
                 ctk_calendar_signals[MONTH_CHANGED_SIGNAL],
                 0);

  month_len = month_length[leap (priv->year)][priv->month + 1];

  if (month_len < priv->selected_day)
    {
      priv->selected_day = 0;
      ctk_calendar_select_day (calendar, month_len);
    }
  else
    ctk_calendar_select_day (calendar, priv->selected_day);

  calendar_queue_refresh (calendar);
}

static void
calendar_compute_days (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint month;
  gint year;
  gint ndays_in_month;
  gint ndays_in_prev_month;
  gint first_day;
  gint row;
  gint col;
  gint day;

  year = priv->year;
  month = priv->month + 1;

  ndays_in_month = month_length[leap (year)][month];

  first_day = day_of_week (year, month, 1);
  first_day = (first_day + 7 - priv->week_start) % 7;
  if (first_day == 0)
    first_day = 7;

  /* Compute days of previous month */
  if (month > 1)
    ndays_in_prev_month = month_length[leap (year)][month - 1];
  else
    ndays_in_prev_month = month_length[leap (year - 1)][12];
  day = ndays_in_prev_month - first_day+ 1;

  for (col = 0; col < first_day; col++)
    {
      priv->day[0][col] = day;
      priv->day_month[0][col] = MONTH_PREV;
      day++;
    }

  /* Compute days of current month */
  row = first_day / 7;
  col = first_day % 7;
  for (day = 1; day <= ndays_in_month; day++)
    {
      priv->day[row][col] = day;
      priv->day_month[row][col] = MONTH_CURRENT;

      col++;
      if (col == 7)
        {
          row++;
          col = 0;
        }
    }

  /* Compute days of next month */
  day = 1;
  for (; row <= 5; row++)
    {
      for (; col <= 6; col++)
        {
          priv->day[row][col] = day;
          priv->day_month[row][col] = MONTH_NEXT;
          day++;
        }
      col = 0;
    }
}

static void
calendar_select_and_focus_day (CtkCalendar *calendar,
                               guint        day)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint old_focus_row = priv->focus_row;
  gint old_focus_col = priv->focus_col;
  gint row;
  gint col;

  for (row = 0; row < 6; row ++)
    for (col = 0; col < 7; col++)
      {
        if (priv->day_month[row][col] == MONTH_CURRENT
            && priv->day[row][col] == day)
          {
            priv->focus_row = row;
            priv->focus_col = col;
          }
      }

  if (old_focus_row != -1 && old_focus_col != -1)
    calendar_invalidate_day (calendar, old_focus_row, old_focus_col);

  ctk_calendar_select_day (calendar, day);
}


/****************************************
 *     Layout computation utilities     *
 ****************************************/

static gint
calendar_row_height (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;

  return (priv->main_h - CALENDAR_MARGIN
          - ((priv->display_flags & CTK_CALENDAR_SHOW_DAY_NAMES)
             ? calendar_get_ysep (calendar) : CALENDAR_MARGIN)) / 6;
}

static void
get_component_paddings (CtkCalendar *calendar,
                        CtkBorder   *padding,
                        CtkBorder   *day_padding,
                        CtkBorder   *day_name_padding,
                        CtkBorder   *week_padding)
{
  CtkStyleContext * context;
  CtkStateFlags state;
  CtkWidget *widget;

  widget = CTK_WIDGET (calendar);
  context = ctk_widget_get_style_context (widget);
  state = ctk_style_context_get_state (context);

  if (padding)
    ctk_style_context_get_padding (context, state, padding);

  if (day_padding)
    {
      ctk_style_context_save (context);
      ctk_style_context_add_class (context, "day-number");
      ctk_style_context_get_padding (context, state, day_padding);
      ctk_style_context_restore (context);
    }

  if (day_name_padding)
    {
      ctk_style_context_save (context);
      ctk_style_context_add_class (context, "day-name");
      ctk_style_context_get_padding (context, state, day_name_padding);
      ctk_style_context_restore (context);
    }

  if (week_padding)
    {
      ctk_style_context_save (context);
      ctk_style_context_add_class (context, "week-number");
      ctk_style_context_get_padding (context, state, week_padding);
      ctk_style_context_restore (context);
    }
}

/* calendar_left_x_for_column: returns the x coordinate
 * for the left of the column */
static gint
calendar_left_x_for_column (CtkCalendar *calendar,
                            gint         column)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint width;
  gint x_left;
  gint week_width;
  gint calendar_xsep = calendar_get_xsep (calendar);
  gint inner_border = calendar_get_inner_border (calendar);
  CtkBorder padding;

  get_component_paddings (calendar, &padding, NULL, NULL, NULL);

  week_width = priv->week_width + padding.left + inner_border;

  if (ctk_widget_get_direction (CTK_WIDGET (calendar)) == CTK_TEXT_DIR_RTL)
    {
      column = 6 - column;
      week_width = 0;
    }

  width = calendar->priv->day_width;
  if (priv->display_flags & CTK_CALENDAR_SHOW_WEEK_NUMBERS)
    x_left = week_width + calendar_xsep + (width + DAY_XSEP) * column;
  else
    x_left = week_width + CALENDAR_MARGIN + (width + DAY_XSEP) * column;

  return x_left;
}

/* column_from_x: returns the column 0-6 that the
 * x pixel of the xwindow is in */
static gint
calendar_column_from_x (CtkCalendar *calendar,
                        gint         event_x)
{
  gint c, column;
  gint x_left, x_right;

  column = -1;

  for (c = 0; c < 7; c++)
    {
      x_left = calendar_left_x_for_column (calendar, c);
      x_right = x_left + calendar->priv->day_width;

      if (event_x >= x_left && event_x < x_right)
        {
          column = c;
          break;
        }
    }

  return column;
}

/* calendar_top_y_for_row: returns the y coordinate
 * for the top of the row */
static gint
calendar_top_y_for_row (CtkCalendar *calendar,
                        gint         row)
{
  CtkCalendarPrivate *priv = calendar->priv;
  CtkBorder padding;
  gint inner_border = calendar_get_inner_border (calendar);

  get_component_paddings (calendar, &padding, NULL, NULL, NULL);

  return priv->header_h + priv->day_name_h + padding.top + inner_border
         + row * calendar_row_height (calendar);
}

/* row_from_y: returns the row 0-5 that the
 * y pixel of the xwindow is in */
static gint
calendar_row_from_y (CtkCalendar *calendar,
                     gint         event_y)
{
  gint r, row;
  gint height;
  gint y_top, y_bottom;

  height = calendar_row_height (calendar);
  row = -1;

  for (r = 0; r < 6; r++)
    {
      y_top = calendar_top_y_for_row (calendar, r);
      y_bottom = y_top + height;

      if (event_y >= y_top && event_y < y_bottom)
        {
          row = r;
          break;
        }
    }

  return row;
}

static void
calendar_arrow_rectangle (CtkCalendar  *calendar,
                          guint         arrow,
                          CdkRectangle *rect)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkAllocation allocation;
  CtkBorder padding;
  gboolean year_left;

  get_component_paddings (calendar, &padding, NULL, NULL, NULL);
  ctk_widget_get_allocation (widget, &allocation);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR)
    year_left = priv->year_before;
  else
    year_left = !priv->year_before;

  rect->y = 3;
  rect->width = priv->arrow_width;
  rect->height = priv->header_h - 7;

  switch (arrow)
    {
    case ARROW_MONTH_LEFT:
      if (year_left)
        rect->x = (allocation.width - padding.left - padding.right
                   - (3 + 2 * priv->arrow_width + priv->max_month_width));
      else
        rect->x = 3;
      break;
    case ARROW_MONTH_RIGHT:
      if (year_left)
        rect->x = (allocation.width - padding.left - padding.right
                   - 3 - priv->arrow_width);
      else
        rect->x = (priv->arrow_width + priv->max_month_width);
      break;
    case ARROW_YEAR_LEFT:
      if (year_left)
        rect->x = 3;
      else
        rect->x = (allocation.width - padding.left - padding.right
                   - (3 + 2 * priv->arrow_width + priv->max_year_width));
      break;
    case ARROW_YEAR_RIGHT:
      if (year_left)
        rect->x = (priv->arrow_width + priv->max_year_width);
      else
        rect->x = (allocation.width - padding.left - padding.right
                   - 3 - priv->arrow_width);
      break;

    default:
      g_assert_not_reached ();
    }

  rect->x += padding.left;
  rect->y += padding.top;
}

static void
calendar_day_rectangle (CtkCalendar  *calendar,
                        gint          row,
                        gint          col,
                        CdkRectangle *rect)
{
  CtkCalendarPrivate *priv = calendar->priv;

  rect->x = calendar_left_x_for_column (calendar, col);
  rect->y = calendar_top_y_for_row (calendar, row);
  rect->height = calendar_row_height (calendar);
  rect->width = priv->day_width;
}

static void
calendar_set_month_prev (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint month_len;

  if (priv->display_flags & CTK_CALENDAR_NO_MONTH_CHANGE)
    return;

  if (priv->month == 0)
    {
      priv->month = 11;
      priv->year--;
    }
  else
    priv->month--;

  month_len = month_length[leap (priv->year)][priv->month + 1];

  calendar_compute_days (calendar);

  g_signal_emit (calendar,
                 ctk_calendar_signals[PREV_MONTH_SIGNAL],
                 0);
  g_signal_emit (calendar,
                 ctk_calendar_signals[MONTH_CHANGED_SIGNAL],
                 0);

  if (month_len < priv->selected_day)
    {
      priv->selected_day = 0;
      ctk_calendar_select_day (calendar, month_len);
    }
  else
    {
      if (priv->selected_day < 0)
        priv->selected_day = priv->selected_day + 1 + month_length[leap (priv->year)][priv->month + 1];
      ctk_calendar_select_day (calendar, priv->selected_day);
    }

  calendar_queue_refresh (calendar);
}


/****************************************
 *           Basic object methods       *
 ****************************************/

static void
ctk_calendar_finalize (GObject *object)
{
  G_OBJECT_CLASS (ctk_calendar_parent_class)->finalize (object);
}

static void
ctk_calendar_destroy (CtkWidget *widget)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;

  calendar_stop_spinning (CTK_CALENDAR (widget));

  /* Call the destroy function for the extra display callback: */
  if (priv->detail_func_destroy && priv->detail_func_user_data)
    {
      priv->detail_func_destroy (priv->detail_func_user_data);
      priv->detail_func_user_data = NULL;
      priv->detail_func_destroy = NULL;
    }

  CTK_WIDGET_CLASS (ctk_calendar_parent_class)->destroy (widget);
}

static gboolean
calendar_set_display_option (CtkCalendar              *calendar,
                             CtkCalendarDisplayOptions flag,
                             gboolean                  setting)
{
  CtkCalendarPrivate *priv = calendar->priv;
  CtkCalendarDisplayOptions flags;
  gboolean old_setting;

  old_setting = (priv->display_flags & flag) != 0;
  if (old_setting == setting)
    return FALSE;

  if (setting)
    flags = priv->display_flags | flag;
  else
    flags = priv->display_flags & ~flag;

  ctk_calendar_set_display_options (calendar, flags);

  return TRUE;
}

static gboolean
calendar_get_display_option (CtkCalendar              *calendar,
                             CtkCalendarDisplayOptions flag)
{
  CtkCalendarPrivate *priv = calendar->priv;

  return (priv->display_flags & flag) != 0;
}

static void
ctk_calendar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkCalendar *calendar = CTK_CALENDAR (object);
  CtkCalendarPrivate *priv = calendar->priv;

  switch (prop_id)
    {
    case PROP_YEAR:
      ctk_calendar_select_month (calendar,
                                 priv->month,
                                 g_value_get_int (value));
      break;
    case PROP_MONTH:
      ctk_calendar_select_month (calendar,
                                 g_value_get_int (value),
                                 priv->year);
      break;
    case PROP_DAY:
      ctk_calendar_select_day (calendar,
                               g_value_get_int (value));
      break;
    case PROP_SHOW_HEADING:
      if (calendar_set_display_option (calendar,
                                       CTK_CALENDAR_SHOW_HEADING,
                                       g_value_get_boolean (value)))
        g_object_notify (object, "show-heading");
      break;
    case PROP_SHOW_DAY_NAMES:
      if (calendar_set_display_option (calendar,
                                       CTK_CALENDAR_SHOW_DAY_NAMES,
                                       g_value_get_boolean (value)))
        g_object_notify (object, "show-day-names");
      break;
    case PROP_NO_MONTH_CHANGE:
      if (calendar_set_display_option (calendar,
                                       CTK_CALENDAR_NO_MONTH_CHANGE,
                                       g_value_get_boolean (value)))
        g_object_notify (object, "no-month-change");
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      if (calendar_set_display_option (calendar,
                                       CTK_CALENDAR_SHOW_WEEK_NUMBERS,
                                       g_value_get_boolean (value)))
        g_object_notify (object, "show-week-numbers");
      break;
    case PROP_SHOW_DETAILS:
      if (calendar_set_display_option (calendar,
                                       CTK_CALENDAR_SHOW_DETAILS,
                                       g_value_get_boolean (value)))
        g_object_notify (object, "show-details");
      break;
    case PROP_DETAIL_WIDTH_CHARS:
      ctk_calendar_set_detail_width_chars (calendar,
                                           g_value_get_int (value));
      break;
    case PROP_DETAIL_HEIGHT_ROWS:
      ctk_calendar_set_detail_height_rows (calendar,
                                           g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_calendar_get_property (GObject      *object,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  CtkCalendar *calendar = CTK_CALENDAR (object);
  CtkCalendarPrivate *priv = calendar->priv;

  switch (prop_id)
    {
    case PROP_YEAR:
      g_value_set_int (value, priv->year);
      break;
    case PROP_MONTH:
      g_value_set_int (value, priv->month);
      break;
    case PROP_DAY:
      g_value_set_int (value, priv->selected_day);
      break;
    case PROP_SHOW_HEADING:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
                                                               CTK_CALENDAR_SHOW_HEADING));
      break;
    case PROP_SHOW_DAY_NAMES:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
                                                               CTK_CALENDAR_SHOW_DAY_NAMES));
      break;
    case PROP_NO_MONTH_CHANGE:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
                                                               CTK_CALENDAR_NO_MONTH_CHANGE));
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
                                                               CTK_CALENDAR_SHOW_WEEK_NUMBERS));
      break;
    case PROP_SHOW_DETAILS:
      g_value_set_boolean (value, calendar_get_display_option (calendar,
                                                               CTK_CALENDAR_SHOW_DETAILS));
      break;
    case PROP_DETAIL_WIDTH_CHARS:
      g_value_set_int (value, priv->detail_width_chars);
      break;
    case PROP_DETAIL_HEIGHT_ROWS:
      g_value_set_int (value, priv->detail_height_rows);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/****************************************
 *             Realization              *
 ****************************************/

static void
calendar_realize_arrows (CtkCalendar *calendar)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  CdkWindowAttr attributes;
  gint attributes_mask;
  gint i;
  CtkAllocation allocation;

  if (! (priv->display_flags & CTK_CALENDAR_NO_MONTH_CHANGE)
      && (priv->display_flags & CTK_CALENDAR_SHOW_HEADING))
    {
      ctk_widget_get_allocation (widget, &allocation);

      attributes.wclass = CDK_INPUT_ONLY;
      attributes.window_type = CDK_WINDOW_CHILD;
      attributes.event_mask = (ctk_widget_get_events (widget)
                               | CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK
                               | CDK_ENTER_NOTIFY_MASK | CDK_LEAVE_NOTIFY_MASK);
      attributes_mask = CDK_WA_X | CDK_WA_Y;
      for (i = 0; i < 4; i++)
        {
          CdkRectangle rect;
          calendar_arrow_rectangle (calendar, i, &rect);

          attributes.x = allocation.x + rect.x;
          attributes.y = allocation.y + rect.y;
          attributes.width = rect.width;
          attributes.height = rect.height;
          priv->arrow_win[i] = cdk_window_new (ctk_widget_get_window (widget),
                                               &attributes,
                                               attributes_mask);

          ctk_widget_register_window (widget, priv->arrow_win[i]);
        }
      priv->arrow_prelight = 0x0;
    }
  else
    {
      for (i = 0; i < 4; i++)
        priv->arrow_win[i] = NULL;
    }
}

static void
calendar_unrealize_arrows (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint i;

  for (i = 0; i < 4; i++)
    {
      if (priv->arrow_win[i])
        {
          ctk_widget_unregister_window (CTK_WIDGET (calendar), priv->arrow_win[i]);
          cdk_window_destroy (priv->arrow_win[i]);
          priv->arrow_win[i] = NULL;
        }
    }

}

static gint
calendar_get_inner_border (CtkCalendar *calendar)
{
  gint inner_border;

  ctk_widget_style_get (CTK_WIDGET (calendar),
                        "inner-border", &inner_border,
                        NULL);

  return inner_border;
}

static gint
calendar_get_xsep (CtkCalendar *calendar)
{
  gint xsep;

  ctk_widget_style_get (CTK_WIDGET (calendar),
                        "horizontal-separation", &xsep,
                        NULL);

  return xsep;
}

static gint
calendar_get_ysep (CtkCalendar *calendar)
{
  gint ysep;

  ctk_widget_style_get (CTK_WIDGET (calendar),
                        "vertical-separation", &ysep,
                        NULL);

  return ysep;
}

static void
ctk_calendar_realize (CtkWidget *widget)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;
  CdkWindowAttr attributes;
  gint attributes_mask;
  gint inner_border = calendar_get_inner_border (CTK_CALENDAR (widget));
  CtkAllocation allocation;
  CtkBorder padding;

  get_component_paddings (CTK_CALENDAR (widget), &padding, NULL, NULL, NULL);
  ctk_widget_get_allocation (widget, &allocation);

  CTK_WIDGET_CLASS (ctk_calendar_parent_class)->realize (widget);

  attributes.wclass = CDK_INPUT_ONLY;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.event_mask = (ctk_widget_get_events (widget)
                           | CDK_SCROLL_MASK
                           | CDK_BUTTON_PRESS_MASK
                           | CDK_BUTTON_RELEASE_MASK
                           | CDK_POINTER_MOTION_MASK
                           | CDK_LEAVE_NOTIFY_MASK);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR)
    attributes.x = priv->week_width + padding.left + inner_border;
  else
    attributes.x = padding.left + inner_border;

  attributes.y = priv->header_h + priv->day_name_h + padding.top + inner_border;
  attributes.width = allocation.width - attributes.x - (padding.right + inner_border);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    attributes.width -= priv->week_width;

  attributes.height = priv->main_h;
  attributes_mask = CDK_WA_X | CDK_WA_Y;

  attributes.x += allocation.x;
  attributes.y += allocation.y;

  priv->main_win = cdk_window_new (ctk_widget_get_window (widget),
                                   &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->main_win);

  calendar_realize_arrows (CTK_CALENDAR (widget));
}

static void
ctk_calendar_unrealize (CtkWidget *widget)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;

  calendar_unrealize_arrows (CTK_CALENDAR (widget));

  if (priv->main_win)
    {
      ctk_widget_unregister_window (widget, priv->main_win);
      cdk_window_destroy (priv->main_win);
      priv->main_win = NULL;
    }

  CTK_WIDGET_CLASS (ctk_calendar_parent_class)->unrealize (widget);
}

static void
calendar_map_arrows (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint i;

  for (i = 0; i < 4; i++)
    {
      if (priv->arrow_win[i])
        cdk_window_show (priv->arrow_win[i]);
    }
}

static void
calendar_unmap_arrows (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint i;

  for (i = 0; i < 4; i++)
    {
      if (priv->arrow_win[i])
        cdk_window_hide (priv->arrow_win[i]);
    }
}

static void
ctk_calendar_map (CtkWidget *widget)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;

  CTK_WIDGET_CLASS (ctk_calendar_parent_class)->map (widget);

  cdk_window_show (priv->main_win);

  calendar_map_arrows (CTK_CALENDAR (widget));
}

static void
ctk_calendar_unmap (CtkWidget *widget)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;

  calendar_unmap_arrows (CTK_CALENDAR (widget));

  cdk_window_hide (priv->main_win);

  CTK_WIDGET_CLASS (ctk_calendar_parent_class)->unmap (widget);
}

static gchar*
ctk_calendar_get_detail (CtkCalendar *calendar,
                         gint         row,
                         gint         column)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (calendar)->priv;
  gint year, month;

  if (priv->detail_func == NULL)
    return NULL;

  year = priv->year;
  month = priv->month + priv->day_month[row][column] - MONTH_CURRENT;

  if (month < 0)
    {
      month += 12;
      year -= 1;
    }
  else if (month > 11)
    {
      month -= 12;
      year += 1;
    }

  return priv->detail_func (calendar,
                            year, month,
                            priv->day[row][column],
                            priv->detail_func_user_data);
}

static gboolean
ctk_calendar_query_tooltip (CtkWidget  *widget,
                            gint        x,
                            gint        y,
                            gboolean    keyboard_mode,
                            CtkTooltip *tooltip)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  gchar *detail = NULL;
  CdkRectangle day_rect;
  gint row, col;

  col = calendar_column_from_x (calendar, x);
  row = calendar_row_from_y (calendar, y);

  if (col != -1 && row != -1 &&
      (0 != (priv->detail_overflow[row] & (1 << col)) ||
      0 == (priv->display_flags & CTK_CALENDAR_SHOW_DETAILS)))
    {
      detail = ctk_calendar_get_detail (calendar, row, col);
      calendar_day_rectangle (calendar, row, col, &day_rect);
    }

  if (detail)
    {
      ctk_tooltip_set_tip_area (tooltip, &day_rect);
      ctk_tooltip_set_markup (tooltip, detail);

      g_free (detail);

      return TRUE;
    }

  if (CTK_WIDGET_CLASS (ctk_calendar_parent_class)->query_tooltip)
    return CTK_WIDGET_CLASS (ctk_calendar_parent_class)->query_tooltip (widget, x, y, keyboard_mode, tooltip);

  return FALSE;
}

/****************************************
 *       Size Request and Allocate      *
 ****************************************/

static void
ctk_calendar_size_request (CtkWidget      *widget,
                           CtkRequisition *requisition)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkBorder padding, day_padding, day_name_padding, week_padding;
  PangoLayout *layout;
  PangoRectangle logical_rect;

  gint height;
  gint i, r, c;
  gint calendar_margin = CALENDAR_MARGIN;
  gint header_width, main_width;
  gint max_header_height = 0;
  gint max_detail_height;
  gint inner_border = calendar_get_inner_border (calendar);
  gint calendar_ysep = calendar_get_ysep (calendar);
  gint calendar_xsep = calendar_get_xsep (calendar);

  gboolean show_week_numbers = (priv->display_flags & CTK_CALENDAR_SHOW_WEEK_NUMBERS);

  layout = ctk_widget_create_pango_layout (widget, NULL);

  /*
   * Calculate the requisition  width for the widget.
   */

  /* Header width */

  if (priv->display_flags & CTK_CALENDAR_SHOW_HEADING)
    {
      priv->max_month_width = 0;
      for (i = 0; i < 12; i++)
        {
          pango_layout_set_text (layout, default_monthname[i], -1);
          pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
          priv->max_month_width = MAX (priv->max_month_width,
                                               logical_rect.width + 8);
          max_header_height = MAX (max_header_height, logical_rect.height);
        }

      priv->max_year_width = 0;
      /* Translators:  This is a text measurement template.
       * Translate it to the widest year text
       *
       * If you don't understand this, leave it as "2000"
       */
      pango_layout_set_text (layout, C_("year measurement template", "2000"), -1);
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      priv->max_year_width = MAX (priv->max_year_width,
                                  logical_rect.width + 8);
      max_header_height = MAX (max_header_height, logical_rect.height);
    }
  else
    {
      priv->max_month_width = 0;
      priv->max_year_width = 0;
    }

  if (priv->display_flags & CTK_CALENDAR_NO_MONTH_CHANGE)
    header_width = (priv->max_month_width
                    + priv->max_year_width
                    + 3 * 3);
  else
    header_width = (priv->max_month_width
                    + priv->max_year_width
                    + 4 * priv->arrow_width + 3 * 3);

  /* Mainwindow labels width */

  priv->max_day_char_width = 0;
  priv->max_day_char_ascent = 0;
  priv->max_day_char_descent = 0;
  priv->min_day_width = 0;

  for (i = 0; i < 9; i++)
    {
      gchar buffer[32];
      g_snprintf (buffer, sizeof (buffer), C_("calendar:day:digits", "%d"), i * 11);
      pango_layout_set_text (layout, buffer, -1);
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      priv->min_day_width = MAX (priv->min_day_width,
                                         logical_rect.width);

      priv->max_day_char_ascent = MAX (priv->max_day_char_ascent,
                                               PANGO_ASCENT (logical_rect));
      priv->max_day_char_descent = MAX (priv->max_day_char_descent,
                                                PANGO_DESCENT (logical_rect));
    }

  priv->max_label_char_ascent = 0;
  priv->max_label_char_descent = 0;
  if (priv->display_flags & CTK_CALENDAR_SHOW_DAY_NAMES)
    for (i = 0; i < 7; i++)
      {
        pango_layout_set_text (layout, default_abbreviated_dayname[i], -1);
        pango_layout_line_get_pixel_extents (pango_layout_get_lines_readonly (layout)->data, NULL, &logical_rect);

        priv->min_day_width = MAX (priv->min_day_width, logical_rect.width);
        priv->max_label_char_ascent = MAX (priv->max_label_char_ascent,
                                                   PANGO_ASCENT (logical_rect));
        priv->max_label_char_descent = MAX (priv->max_label_char_descent,
                                                    PANGO_DESCENT (logical_rect));
      }

  priv->max_week_char_width = 0;
  if (show_week_numbers)
    for (i = 0; i < 9; i++)
      {
        gchar buffer[32];
        g_snprintf (buffer, sizeof (buffer), C_("calendar:week:digits", "%d"), i * 11);
        pango_layout_set_text (layout, buffer, -1);
        pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
        priv->max_week_char_width = MAX (priv->max_week_char_width,
                                           logical_rect.width / 2);
      }

  /* Calculate detail extents. Do this as late as possible since
   * pango_layout_set_markup is called which alters font settings. */
  max_detail_height = 0;

  if (priv->detail_func && (priv->display_flags & CTK_CALENDAR_SHOW_DETAILS))
    {
      gchar *markup, *tail;

      if (priv->detail_width_chars || priv->detail_height_rows)
        {
          gint rows = MAX (1, priv->detail_height_rows) - 1;
          gsize len = priv->detail_width_chars + rows + 16;

          markup = tail = g_alloca (len);

          memcpy (tail,     "<small>", 7);
          tail += 7;

          memset (tail, 'm', priv->detail_width_chars);
          tail += priv->detail_width_chars;

          memset (tail, '\n', rows);
          tail += rows;

          memcpy (tail,     "</small>", 9);
          tail += 9;

          g_assert (len == (tail - markup));

          pango_layout_set_markup (layout, markup, -1);
          pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

          if (priv->detail_width_chars)
            priv->min_day_width = MAX (priv->min_day_width, logical_rect.width);
          if (priv->detail_height_rows)
            max_detail_height = MAX (max_detail_height, logical_rect.height);
        }

      if (!priv->detail_width_chars || !priv->detail_height_rows)
        for (r = 0; r < 6; r++)
          for (c = 0; c < 7; c++)
            {
              gchar *detail = ctk_calendar_get_detail (calendar, r, c);

              if (detail)
                {
                  markup = g_strconcat ("<small>", detail, "</small>", NULL);
                  pango_layout_set_markup (layout, markup, -1);

                  if (priv->detail_width_chars)
                    {
                      pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
                      pango_layout_set_width (layout, PANGO_SCALE * priv->min_day_width);
                    }

                  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

                  if (!priv->detail_width_chars)
                    priv->min_day_width = MAX (priv->min_day_width, logical_rect.width);
                  if (!priv->detail_height_rows)
                    max_detail_height = MAX (max_detail_height, logical_rect.height);

                  g_free (markup);
                  g_free (detail);
                }
            }
    }

  get_component_paddings (calendar, &padding, &day_padding, &day_name_padding, &week_padding);

  priv->min_day_width += day_padding.left + day_padding.right;
  if (show_week_numbers)
    priv->max_week_char_width += week_padding.left + week_padding.right;

  /* We add one to max_day_char_width to be able to make the marked day "bold" */
  priv->max_day_char_width = priv->min_day_width / 2 + 1;

  main_width = (7 * (priv->min_day_width) + (DAY_XSEP * 6) + CALENDAR_MARGIN * 2
                + (show_week_numbers
                   ? priv->max_week_char_width * 2 + calendar_xsep * 2
                   : 0));

  requisition->width = MAX (header_width, main_width + inner_border * 2) + padding.left + padding.right;

  /*
   * Calculate the requisition height for the widget.
   */

  if (priv->display_flags & CTK_CALENDAR_SHOW_HEADING)
    {
      priv->header_h = (max_header_height + calendar_ysep * 2);
    }
  else
    {
      priv->header_h = 0;
    }

  if (priv->display_flags & CTK_CALENDAR_SHOW_DAY_NAMES)
    {
      priv->day_name_h = (priv->max_label_char_ascent
                          + priv->max_label_char_descent
                          + day_name_padding.top + day_name_padding.bottom
                          + calendar_margin);
      calendar_margin = calendar_ysep;
    }
  else
    {
      priv->day_name_h = 0;
    }

  priv->main_h = (CALENDAR_MARGIN + calendar_margin
                          + 6 * (priv->max_day_char_ascent
                                 + priv->max_day_char_descent
                                 + max_detail_height
                                 + day_padding.top + day_padding.bottom)
                          + DAY_YSEP * 5);

  height = priv->header_h + priv->day_name_h + priv->main_h;

  requisition->height = height + padding.top + padding.bottom + (inner_border * 2);

  g_object_unref (layout);
}

static void
ctk_calendar_get_preferred_width (CtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  CtkRequisition requisition;

  ctk_calendar_size_request (widget, &requisition);

  *minimum = *natural = requisition.width;
}

static void
ctk_calendar_get_preferred_height (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  CtkRequisition requisition;

  ctk_calendar_size_request (widget, &requisition);

  *minimum = *natural = requisition.height;
}

static void
ctk_calendar_size_allocate (CtkWidget     *widget,
                            CtkAllocation *allocation)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkBorder padding;
  guint i;
  gint inner_border = calendar_get_inner_border (calendar);
  gint calendar_xsep = calendar_get_xsep (calendar);

  get_component_paddings (calendar, &padding, NULL, NULL, NULL);
  ctk_widget_set_allocation (widget, allocation);

  if (priv->display_flags & CTK_CALENDAR_SHOW_WEEK_NUMBERS)
    {
      priv->day_width = (priv->min_day_width
                         * ((allocation->width - (inner_border * 2) - padding.left - padding.right
                             - (CALENDAR_MARGIN * 2) -  (DAY_XSEP * 6) - calendar_xsep * 2))
                         / (7 * priv->min_day_width + priv->max_week_char_width * 2));
      priv->week_width = ((allocation->width - (inner_border * 2) - padding.left - padding.right
                           - (CALENDAR_MARGIN * 2) - (DAY_XSEP * 6) - calendar_xsep * 2 )
                          - priv->day_width * 7 + CALENDAR_MARGIN + calendar_xsep);
    }
  else
    {
      priv->day_width = (allocation->width
                         - (inner_border * 2)
                         - padding.left - padding.right
                         - (CALENDAR_MARGIN * 2)
                         - (DAY_XSEP * 6))/7;
      priv->week_width = 0;
    }

  if (ctk_widget_get_realized (widget))
    {
      if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR)
        cdk_window_move_resize (priv->main_win,
                                allocation->x
                                 + priv->week_width + padding.left + inner_border,
                                allocation->y
                                 + priv->header_h + priv->day_name_h
                                 + (padding.top + inner_border),
                                allocation->width - priv->week_width
                                - (inner_border * 2) - padding.left - padding.right,
                                priv->main_h);
      else
        cdk_window_move_resize (priv->main_win,
                                allocation->x
                                 + padding.left + inner_border,
                                allocation->y
                                 + priv->header_h + priv->day_name_h
                                 + padding.top + inner_border,
                                allocation->width - priv->week_width
                                - (inner_border * 2) - padding.left - padding.right,
                                priv->main_h);

      for (i = 0 ; i < 4 ; i++)
        {
          if (priv->arrow_win[i])
            {
              CdkRectangle rect;
              calendar_arrow_rectangle (calendar, i, &rect);

              cdk_window_move_resize (priv->arrow_win[i],
                                      allocation->x + rect.x,
                                      allocation->y + rect.y,
                                      rect.width, rect.height);
            }
        }
    }
}


/****************************************
 *              Repainting              *
 ****************************************/

static void
calendar_paint_header (CtkCalendar *calendar, cairo_t *cr)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkAllocation allocation;
  CtkStyleContext *context;
  CtkStateFlags state;
  CtkBorder padding;
  char buffer[255];
  gint x, y;
  gint header_width;
  gint max_month_width;
  gint max_year_width;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gboolean year_left;
  time_t tmp_time;
  struct tm *tm;
  gchar *str;

  get_component_paddings (calendar, &padding, NULL, NULL, NULL);
  context = ctk_widget_get_style_context (widget);

  cairo_save (cr);
  cairo_translate (cr, padding.left, padding.top);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR)
    year_left = priv->year_before;
  else
    year_left = !priv->year_before;

  ctk_widget_get_allocation (widget, &allocation);

  header_width = allocation.width - padding.left - padding.right;

  max_month_width = priv->max_month_width;
  max_year_width = priv->max_year_width;

  state = ctk_style_context_get_state (context);
  state &= ~CTK_STATE_FLAG_DROP_ACTIVE;

  ctk_style_context_save (context);

  ctk_style_context_set_state (context, state);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_HEADER);

  ctk_render_background (context, cr, 0, 0, header_width, priv->header_h);
  ctk_render_frame (context, cr, 0, 0, header_width, priv->header_h);

  tmp_time = 1;  /* Jan 1 1970, 00:00:01 UTC */
  tm = gmtime (&tmp_time);
  tm->tm_year = priv->year - 1900;

  /* Translators: This dictates how the year is displayed in
   * ctkcalendar widget.  See strftime() manual for the format.
   * Use only ASCII in the translation.
   *
   * Also look for the msgid "2000".
   * Translate that entry to a year with the widest output of this
   * msgid.
   *
   * "%Y" is appropriate for most locales.
   */
  strftime (buffer, sizeof (buffer), C_("calendar year format", "%Y"), tm);
  str = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
  layout = ctk_widget_create_pango_layout (widget, str);
  g_free (str);

  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  /* Draw title */
  y = (priv->header_h - logical_rect.height) / 2;

  /* Draw year and its arrows */

  if (priv->display_flags & CTK_CALENDAR_NO_MONTH_CHANGE)
    if (year_left)
      x = 3 + (max_year_width - logical_rect.width)/2;
    else
      x = header_width - (3 + max_year_width
                          - (max_year_width - logical_rect.width)/2);
  else
    if (year_left)
      x = 3 + priv->arrow_width + (max_year_width - logical_rect.width)/2;
    else
      x = header_width - (3 + priv->arrow_width + max_year_width
                          - (max_year_width - logical_rect.width)/2);

  ctk_render_layout (context, cr, x, y, layout);

  /* Draw month */
  g_snprintf (buffer, sizeof (buffer), "%s", default_monthname[priv->month]);
  pango_layout_set_text (layout, buffer, -1);
  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  if (priv->display_flags & CTK_CALENDAR_NO_MONTH_CHANGE)
    if (year_left)
      x = header_width - (3 + max_month_width
                          - (max_month_width - logical_rect.width)/2);
    else
      x = 3 + (max_month_width - logical_rect.width) / 2;
  else
    if (year_left)
      x = header_width - (3 + priv->arrow_width + max_month_width
                          - (max_month_width - logical_rect.width)/2);
    else
      x = 3 + priv->arrow_width + (max_month_width - logical_rect.width)/2;

  ctk_render_layout (context, cr, x, y, layout);
  g_object_unref (layout);

  ctk_style_context_restore (context);
  cairo_restore (cr);
}

static void
calendar_paint_day_names (CtkCalendar *calendar,
                          cairo_t     *cr)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkStyleContext *context;
  CtkStateFlags state;
  CtkBorder padding, day_name_padding;
  CtkAllocation allocation;
  char buffer[255];
  int day,i;
  int day_width, cal_width;
  int day_wid_sep;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gint calendar_ysep = calendar_get_ysep (calendar);
  gint calendar_xsep = calendar_get_xsep (calendar);
  gint inner_border = calendar_get_inner_border (calendar);

  get_component_paddings (calendar, &padding, NULL, &day_name_padding, NULL);
  context = ctk_widget_get_style_context (widget);

  cairo_save (cr);

  cairo_translate (cr,
                   padding.left + inner_border,
                   priv->header_h + padding.top + inner_border);

  ctk_widget_get_allocation (widget, &allocation);

  day_width = priv->day_width;
  cal_width = allocation.width - (inner_border * 2) - padding.left - padding.right;
  day_wid_sep = day_width + DAY_XSEP;

  /*
   * Draw rectangles as inverted background for the labels.
   */

  state = ctk_style_context_get_state (context);
  state &= ~CTK_STATE_FLAG_DROP_ACTIVE;

  ctk_style_context_save (context);

  ctk_style_context_set_state (context, state);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_HIGHLIGHT);

  ctk_render_background (context, cr,
                         CALENDAR_MARGIN, CALENDAR_MARGIN,
                         cal_width - CALENDAR_MARGIN * 2,
                         priv->day_name_h - CALENDAR_MARGIN);

  if (priv->display_flags & CTK_CALENDAR_SHOW_WEEK_NUMBERS)
    ctk_render_background (context, cr,
                           CALENDAR_MARGIN,
                           priv->day_name_h - calendar_ysep,
                           priv->week_width - calendar_ysep - CALENDAR_MARGIN,
                           calendar_ysep);

  /*
   * Write the labels
   */
  layout = ctk_widget_create_pango_layout (widget, NULL);

  for (i = 0; i < 7; i++)
    {
      if (ctk_widget_get_direction (CTK_WIDGET (calendar)) == CTK_TEXT_DIR_RTL)
        day = 6 - i;
      else
        day = i;
      day = (day + priv->week_start) % 7;
      g_snprintf (buffer, sizeof (buffer), "%s", default_abbreviated_dayname[day]);

      pango_layout_set_text (layout, buffer, -1);
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      ctk_render_layout (context, cr,
                         (CALENDAR_MARGIN +
                          + (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR ?
                             (priv->week_width + (priv->week_width ? calendar_xsep : 0))
                             : 0)
                          + day_wid_sep * i
                          + (day_width - logical_rect.width)/2),
                         CALENDAR_MARGIN + day_name_padding.top + logical_rect.y,
                         layout);
    }

  g_object_unref (layout);

  ctk_style_context_restore (context);
  cairo_restore (cr);
}

static void
calendar_paint_week_numbers (CtkCalendar *calendar,
                             cairo_t     *cr)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkStyleContext *context;
  CtkStateFlags state;
  CtkBorder padding, week_padding;
  gint row, x_loc, y_loc;
  gint day_height;
  char buffer[32];
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gint calendar_xsep = calendar_get_xsep (calendar);
  gint inner_border = calendar_get_inner_border (calendar);
  gint x, y;

  get_component_paddings (calendar, &padding, NULL, NULL, &week_padding);
  context = ctk_widget_get_style_context (widget);

  cairo_save (cr);

  y = priv->header_h + priv->day_name_h + (padding.top + inner_border);
  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR)
    x = padding.left + inner_border;
  else
    x = ctk_widget_get_allocated_width (widget) - priv->week_width - (padding.right + inner_border);

  state = ctk_style_context_get_state (context);
  state &= ~CTK_STATE_FLAG_DROP_ACTIVE;

  ctk_style_context_save (context);

  ctk_style_context_set_state (context, state);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_HIGHLIGHT);

  if (priv->display_flags & CTK_CALENDAR_SHOW_DAY_NAMES)
    ctk_render_background (context, cr,
                           x + CALENDAR_MARGIN, y,
                           priv->week_width - CALENDAR_MARGIN,
                           priv->main_h - CALENDAR_MARGIN);
  else
    ctk_render_background (context, cr,
                           x + CALENDAR_MARGIN,
                           y + CALENDAR_MARGIN,
                           priv->week_width - CALENDAR_MARGIN,
                           priv->main_h - 2 * CALENDAR_MARGIN);

  /*
   * Write the labels
   */

  layout = ctk_widget_create_pango_layout (widget, NULL);
  day_height = calendar_row_height (calendar);

  for (row = 0; row < 6; row++)
    {
      gint year, month, week;

      year = priv->year;
      month = priv->month + priv->day_month[row][6] - MONTH_CURRENT;

      if (month < 0)
        {
          month += 12;
          year -= 1;
        }
      else if (month > 11)
        {
          month -= 12;
          year += 1;
        }

      month += 1;

      week = week_of_year (year, month, priv->day[row][6]);

      /* Translators: this defines whether the week numbers should use
       * localized digits or the ones used in English (0123...).
       *
       * Translate to "%Id" if you want to use localized digits, or
       * translate to "%d" otherwise.
       *
       * Note that translating this doesn't guarantee that you get localized
       * digits. That needs support from your system and locale definition
       * too.
       */
      g_snprintf (buffer, sizeof (buffer), C_("calendar:week:digits", "%d"), week);
      pango_layout_set_text (layout, buffer, -1);
      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      y_loc = calendar_top_y_for_row (calendar, row) + (day_height - logical_rect.height) / 2;

      x_loc = x + (priv->week_width
                   - logical_rect.width
                   - calendar_xsep - week_padding.right);

      ctk_render_layout (context, cr, x_loc, y_loc, layout);
    }

  g_object_unref (layout);

  ctk_style_context_restore (context);
  cairo_restore (cr);
}

static void
calendar_invalidate_day_num (CtkCalendar *calendar,
                             gint         day)
{
  CtkCalendarPrivate *priv = calendar->priv;
  gint r, c, row, col;

  row = -1;
  col = -1;
  for (r = 0; r < 6; r++)
    for (c = 0; c < 7; c++)
      if (priv->day_month[r][c] == MONTH_CURRENT &&
          priv->day[r][c] == day)
        {
          row = r;
          col = c;
        }

  g_return_if_fail (row != -1);
  g_return_if_fail (col != -1);

  calendar_invalidate_day (calendar, row, col);
}

static void
calendar_invalidate_day (CtkCalendar *calendar,
                         gint         row,
                         gint         col)
{
  CdkRectangle day_rect;
  CtkAllocation allocation;

  ctk_widget_get_allocation (CTK_WIDGET (calendar), &allocation);
  calendar_day_rectangle (calendar, row, col, &day_rect);
  ctk_widget_queue_draw_area (CTK_WIDGET (calendar),
                              allocation.x + day_rect.x,
                              allocation.y + day_rect.y,
                              day_rect.width, day_rect.height);
}

static gboolean
is_color_attribute (PangoAttribute *attribute,
                    gpointer        data G_GNUC_UNUSED)
{
  return (attribute->klass->type == PANGO_ATTR_FOREGROUND ||
          attribute->klass->type == PANGO_ATTR_BACKGROUND);
}

static void
calendar_paint_day (CtkCalendar *calendar,
                    cairo_t     *cr,
                    gint         row,
                    gint         col)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkStyleContext *context;
  CtkStateFlags state = 0;
  gchar *detail;
  gchar buffer[32];
  gint day;
  gint x_loc, y_loc;
  CdkRectangle day_rect;

  PangoLayout *layout;
  PangoRectangle logical_rect;
  gboolean overflow = FALSE;
  gboolean show_details;

  g_return_if_fail (row < 6);
  g_return_if_fail (col < 7);

  context = ctk_widget_get_style_context (widget);
  state = ctk_widget_get_state_flags (widget);

  day = priv->day[row][col];
  show_details = (priv->display_flags & CTK_CALENDAR_SHOW_DETAILS);

  calendar_day_rectangle (calendar, row, col, &day_rect);

  ctk_style_context_save (context);

  state &= ~(CTK_STATE_FLAG_INCONSISTENT | CTK_STATE_FLAG_ACTIVE | CTK_STATE_FLAG_SELECTED | CTK_STATE_FLAG_DROP_ACTIVE);

  if (priv->day_month[row][col] == MONTH_PREV ||
      priv->day_month[row][col] == MONTH_NEXT)
    state |= CTK_STATE_FLAG_INCONSISTENT;
  else
    {
      if (priv->marked_date[day-1])
        state |= CTK_STATE_FLAG_ACTIVE;

      if (priv->selected_day == day)
        {
          state |= CTK_STATE_FLAG_SELECTED;

          ctk_style_context_set_state (context, state);
          ctk_render_background (context, cr,
                                 day_rect.x, day_rect.y,
                                 day_rect.width, day_rect.height);
        }
    }

  ctk_style_context_set_state (context, state);

  /* Translators: this defines whether the day numbers should use
   * localized digits or the ones used in English (0123...).
   *
   * Translate to "%Id" if you want to use localized digits, or
   * translate to "%d" otherwise.
   *
   * Note that translating this doesn't guarantee that you get localized
   * digits. That needs support from your system and locale definition
   * too.
   */
  g_snprintf (buffer, sizeof (buffer), C_("calendar:day:digits", "%d"), day);

  /* Get extra information to show, if any: */

  detail = ctk_calendar_get_detail (calendar, row, col);

  layout = ctk_widget_create_pango_layout (widget, buffer);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  x_loc = day_rect.x + (day_rect.width - logical_rect.width) / 2;
  y_loc = day_rect.y;

  ctk_render_layout (context, cr, x_loc, y_loc, layout);

  if (priv->day_month[row][col] == MONTH_CURRENT &&
     (priv->marked_date[day-1] || (detail && !show_details)))
    ctk_render_layout (context, cr, x_loc - 1, y_loc, layout);

  y_loc += priv->max_day_char_descent;

  if (priv->detail_func && show_details)
    {
      CdkRGBA color;

      cairo_save (cr);

      ctk_style_context_get_color (context, state, &color);
      cdk_cairo_set_source_rgba (cr, &color);

      cairo_set_line_width (cr, 1);
      cairo_move_to (cr, day_rect.x + 2, y_loc + 0.5);
      cairo_line_to (cr, day_rect.x + day_rect.width - 2, y_loc + 0.5);
      cairo_stroke (cr);

      cairo_restore (cr);

      y_loc += 2;
    }

  if (detail && show_details)
    {
      gchar *markup = g_strconcat ("<small>", detail, "</small>", NULL);
      pango_layout_set_markup (layout, markup, -1);
      g_free (markup);

      if (day == priv->selected_day)
        {
          /* Stripping colors as they conflict with selection marking. */

          PangoAttrList *attrs = pango_layout_get_attributes (layout);
          PangoAttrList *colors = NULL;

          if (attrs)
            colors = pango_attr_list_filter (attrs, is_color_attribute, NULL);
          if (colors)
            pango_attr_list_unref (colors);
        }

      pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
      pango_layout_set_width (layout, PANGO_SCALE * day_rect.width);

      if (priv->detail_height_rows)
        {
          gint dy = day_rect.height - (y_loc - day_rect.y);
          pango_layout_set_height (layout, PANGO_SCALE * dy);
          pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
        }

      cairo_move_to (cr, day_rect.x, y_loc);
      pango_cairo_show_layout (cr, layout);
    }

  if (ctk_widget_has_visible_focus (widget) &&
      priv->focus_row == row && priv->focus_col == col)
    ctk_render_focus (context, cr,
                      day_rect.x, day_rect.y,
                      day_rect.width, day_rect.height);

  if (overflow)
    priv->detail_overflow[row] |= (1 << col);
  else
    priv->detail_overflow[row] &= ~(1 << col);

  ctk_style_context_restore (context);
  g_object_unref (layout);
  g_free (detail);
}

static void
calendar_paint_main (CtkCalendar *calendar,
                     cairo_t     *cr)
{
  gint row, col;

  cairo_save (cr);

  for (col = 0; col < 7; col++)
    for (row = 0; row < 6; row++)
      calendar_paint_day (calendar, cr, row, col);

  cairo_restore (cr);
}

static void
calendar_invalidate_arrow (CtkCalendar *calendar,
                           guint        arrow)
{
  CtkCalendarPrivate *priv = calendar->priv;

  if (priv->display_flags & CTK_CALENDAR_SHOW_HEADING &&
      priv->arrow_win[arrow])
    {
      CdkRectangle rect;
      CtkAllocation allocation;

      calendar_arrow_rectangle (calendar, arrow, &rect);
      ctk_widget_get_allocation (CTK_WIDGET (calendar), &allocation);
      ctk_widget_queue_draw_area (CTK_WIDGET (calendar),
                                  allocation.x + rect.x,
                                  allocation.y + rect.y,
                                  rect.width, rect.height);
    }
}

static void
calendar_paint_arrow (CtkCalendar *calendar,
                      cairo_t     *cr,
                      guint        arrow)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  CtkStyleContext *context;
  CtkStateFlags state;
  CdkRectangle rect;
  gdouble angle;

  if (!priv->arrow_win[arrow])
    return;

  calendar_arrow_rectangle (calendar, arrow, &rect);

  cairo_save (cr);

  context = ctk_widget_get_style_context (widget);
  state = ctk_widget_get_state_flags (widget);

  if (priv->arrow_prelight & (1 << arrow))
    state |= CTK_STATE_FLAG_PRELIGHT;
  else
    state &= ~(CTK_STATE_FLAG_PRELIGHT);

  ctk_style_context_save (context);
  ctk_style_context_set_state (context, state);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_BUTTON);

  ctk_render_background (context, cr,
                         rect.x, rect.y,
                         rect.width, rect.height);

  if (arrow == ARROW_MONTH_LEFT || arrow == ARROW_YEAR_LEFT)
    angle = 3 * (G_PI / 2);
  else
    angle = G_PI / 2;

  ctk_render_arrow (context, cr, angle,
                    rect.x + (rect.width - 8) / 2,
                    rect.y + (rect.height - 8) / 2,
                    8);

  ctk_style_context_restore (context);
  cairo_restore (cr);
}

static gboolean
ctk_calendar_draw (CtkWidget *widget,
                   cairo_t   *cr)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  int i;

  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)))
    {
      CtkStyleContext *context;

      context = ctk_widget_get_style_context (widget);

      ctk_style_context_save (context);
      ctk_style_context_add_class (context, CTK_STYLE_CLASS_VIEW);

      ctk_render_background (context, cr, 0, 0,
                             ctk_widget_get_allocated_width (widget),
                             ctk_widget_get_allocated_height (widget));
      ctk_render_frame (context, cr, 0, 0,
                        ctk_widget_get_allocated_width (widget),
                        ctk_widget_get_allocated_height (widget));

      ctk_style_context_restore (context);
    }

  calendar_paint_main (calendar, cr);

  if (priv->display_flags & CTK_CALENDAR_SHOW_HEADING)
    {
      calendar_paint_header (calendar, cr);
      for (i = 0; i < 4; i++)
        calendar_paint_arrow (calendar, cr, i);
    }

  if (priv->display_flags & CTK_CALENDAR_SHOW_DAY_NAMES)
    calendar_paint_day_names (calendar, cr);

  if (priv->display_flags & CTK_CALENDAR_SHOW_WEEK_NUMBERS)
    calendar_paint_week_numbers (calendar, cr);

  return FALSE;
}


/****************************************
 *           Mouse handling             *
 ****************************************/

static void
calendar_arrow_action (CtkCalendar *calendar,
                       guint        arrow)
{
  switch (arrow)
    {
    case ARROW_YEAR_LEFT:
      calendar_set_year_prev (calendar);
      break;
    case ARROW_YEAR_RIGHT:
      calendar_set_year_next (calendar);
      break;
    case ARROW_MONTH_LEFT:
      calendar_set_month_prev (calendar);
      break;
    case ARROW_MONTH_RIGHT:
      calendar_set_month_next (calendar);
      break;
    default:;
      /* do nothing */
    }
}

static gboolean
calendar_timer (gpointer data)
{
  CtkCalendar *calendar = data;
  CtkCalendarPrivate *priv = calendar->priv;
  gboolean retval = FALSE;

  if (priv->timer)
    {
      calendar_arrow_action (calendar, priv->click_child);

      if (priv->need_timer)
        {
          priv->need_timer = FALSE;
          priv->timer = cdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
                                            TIMEOUT_REPEAT * SCROLL_DELAY_FACTOR,
                                            (GSourceFunc) calendar_timer,
                                            (gpointer) calendar, NULL);
          g_source_set_name_by_id (priv->timer, "[ctk+] calendar_timer");
        }
      else
        retval = TRUE;
    }

  return retval;
}

static void
calendar_start_spinning (CtkCalendar *calendar,
                         gint         click_child)
{
  CtkCalendarPrivate *priv = calendar->priv;

  priv->click_child = click_child;

  if (!priv->timer)
    {
      priv->need_timer = TRUE;
      priv->timer = cdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
                                        TIMEOUT_INITIAL,
                                        (GSourceFunc) calendar_timer,
                                        (gpointer) calendar, NULL);
      g_source_set_name_by_id (priv->timer, "[ctk+] calendar_timer");
    }
}

static void
calendar_stop_spinning (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv = calendar->priv;

  if (priv->timer)
    {
      g_source_remove (priv->timer);
      priv->timer = 0;
      priv->need_timer = FALSE;
    }
}

static void
calendar_main_button_press (CtkCalendar    *calendar,
                            CdkEventButton *event)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  gint x, y;
  gint win_x, win_y;
  gint row, col;
  gint day_month;
  gint day;
  CtkAllocation allocation;

  x = (gint) (event->x);
  y = (gint) (event->y);

  cdk_window_get_position (priv->main_win, &win_x, &win_y);
  ctk_widget_get_allocation (widget, &allocation);

  row = calendar_row_from_y (calendar, y + win_y - allocation.y);
  col = calendar_column_from_x (calendar, x + win_x - allocation.x);

  /* If row or column isn't found, just return. */
  if (row == -1 || col == -1)
    return;

  day_month = priv->day_month[row][col];

  if (event->type == CDK_BUTTON_PRESS)
    {
      day = priv->day[row][col];

      if (day_month == MONTH_PREV)
        calendar_set_month_prev (calendar);
      else if (day_month == MONTH_NEXT)
        calendar_set_month_next (calendar);

      if (!ctk_widget_has_focus (widget))
        ctk_widget_grab_focus (widget);

      if (event->button == CDK_BUTTON_PRIMARY)
        {
          priv->in_drag = 1;
          priv->drag_start_x = x;
          priv->drag_start_y = y;
        }

      calendar_select_and_focus_day (calendar, day);
    }
  else if (event->type == CDK_2BUTTON_PRESS)
    {
      priv->in_drag = 0;
      if (day_month == MONTH_CURRENT)
        g_signal_emit (calendar,
                       ctk_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL],
                       0);
    }
}

static gboolean
ctk_calendar_button_press (CtkWidget      *widget,
                           CdkEventButton *event)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  gint arrow = -1;

  if (!ctk_widget_has_focus (widget))
    ctk_widget_grab_focus (widget);

  if (event->window == priv->main_win)
    calendar_main_button_press (calendar, event);

  for (arrow = ARROW_YEAR_LEFT; arrow <= ARROW_MONTH_RIGHT; arrow++)
    {
      if (event->window == priv->arrow_win[arrow])
        {

          /* only call the action on single click, not double */
          if (event->type == CDK_BUTTON_PRESS)
            {
              if (event->button == CDK_BUTTON_PRIMARY)
                calendar_start_spinning (calendar, arrow);

              calendar_arrow_action (calendar, arrow);
            }

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
ctk_calendar_button_release (CtkWidget    *widget,
                             CdkEventButton *event)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;

  if (event->button == CDK_BUTTON_PRIMARY)
    {
      calendar_stop_spinning (calendar);

      if (priv->in_drag)
        priv->in_drag = 0;
    }

  return TRUE;
}

static gboolean
ctk_calendar_motion_notify (CtkWidget      *widget,
                            CdkEventMotion *event)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;

  if (priv->in_drag)
    {
      if (ctk_drag_check_threshold (widget,
                                    priv->drag_start_x, priv->drag_start_y,
                                    event->x, event->y))
        {
          CdkDragContext *context;
          CtkTargetList *target_list = ctk_target_list_new (NULL, 0);
          ctk_target_list_add_text_targets (target_list, 0);
          context = ctk_drag_begin_with_coordinates (widget, target_list, CDK_ACTION_COPY,
                                                     1, (CdkEvent *)event,
                                                     priv->drag_start_x, priv->drag_start_y);

          priv->in_drag = 0;
          ctk_target_list_unref (target_list);
          ctk_drag_set_icon_default (context);
        }
    }

  return TRUE;
}

static gboolean
ctk_calendar_enter_notify (CtkWidget        *widget,
                           CdkEventCrossing *event)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;

  if (event->window == priv->arrow_win[ARROW_MONTH_LEFT])
    {
      priv->arrow_prelight |= (1 << ARROW_MONTH_LEFT);
      calendar_invalidate_arrow (calendar, ARROW_MONTH_LEFT);
    }

  if (event->window == priv->arrow_win[ARROW_MONTH_RIGHT])
    {
      priv->arrow_prelight |= (1 << ARROW_MONTH_RIGHT);
      calendar_invalidate_arrow (calendar, ARROW_MONTH_RIGHT);
    }

  if (event->window == priv->arrow_win[ARROW_YEAR_LEFT])
    {
      priv->arrow_prelight |= (1 << ARROW_YEAR_LEFT);
      calendar_invalidate_arrow (calendar, ARROW_YEAR_LEFT);
    }

  if (event->window == priv->arrow_win[ARROW_YEAR_RIGHT])
    {
      priv->arrow_prelight |= (1 << ARROW_YEAR_RIGHT);
      calendar_invalidate_arrow (calendar, ARROW_YEAR_RIGHT);
    }

  return TRUE;
}

static gboolean
ctk_calendar_leave_notify (CtkWidget        *widget,
                           CdkEventCrossing *event)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;

  if (event->window == priv->arrow_win[ARROW_MONTH_LEFT])
    {
      priv->arrow_prelight &= ~(1 << ARROW_MONTH_LEFT);
      calendar_invalidate_arrow (calendar, ARROW_MONTH_LEFT);
    }

  if (event->window == priv->arrow_win[ARROW_MONTH_RIGHT])
    {
      priv->arrow_prelight &= ~(1 << ARROW_MONTH_RIGHT);
      calendar_invalidate_arrow (calendar, ARROW_MONTH_RIGHT);
    }

  if (event->window == priv->arrow_win[ARROW_YEAR_LEFT])
    {
      priv->arrow_prelight &= ~(1 << ARROW_YEAR_LEFT);
      calendar_invalidate_arrow (calendar, ARROW_YEAR_LEFT);
    }

  if (event->window == priv->arrow_win[ARROW_YEAR_RIGHT])
    {
      priv->arrow_prelight &= ~(1 << ARROW_YEAR_RIGHT);
      calendar_invalidate_arrow (calendar, ARROW_YEAR_RIGHT);
    }

  return TRUE;
}

static gboolean
ctk_calendar_scroll (CtkWidget      *widget,
                     CdkEventScroll *event)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);

  if (event->direction == CDK_SCROLL_UP)
    {
      if (!ctk_widget_has_focus (widget))
        ctk_widget_grab_focus (widget);
      calendar_set_month_prev (calendar);
    }
  else if (event->direction == CDK_SCROLL_DOWN)
    {
      if (!ctk_widget_has_focus (widget))
        ctk_widget_grab_focus (widget);
      calendar_set_month_next (calendar);
    }
  else
    return FALSE;

  return TRUE;
}


/****************************************
 *             Key handling              *
 ****************************************/

static void
move_focus (CtkCalendar *calendar,
            gint         direction)
{
  CtkCalendarPrivate *priv = calendar->priv;
  CtkTextDirection text_dir = ctk_widget_get_direction (CTK_WIDGET (calendar));

  if ((text_dir == CTK_TEXT_DIR_LTR && direction == -1) ||
      (text_dir == CTK_TEXT_DIR_RTL && direction == 1))
    {
      if (priv->focus_col > 0)
          priv->focus_col--;
      else if (priv->focus_row > 0)
        {
          priv->focus_col = 6;
          priv->focus_row--;
        }

      if (priv->focus_col < 0)
        priv->focus_col = 6;
      if (priv->focus_row < 0)
        priv->focus_row = 5;
    }
  else
    {
      if (priv->focus_col < 6)
        priv->focus_col++;
      else if (priv->focus_row < 5)
        {
          priv->focus_col = 0;
          priv->focus_row++;
        }

      if (priv->focus_col < 0)
        priv->focus_col = 0;
      if (priv->focus_row < 0)
        priv->focus_row = 0;
    }
}

static gboolean
ctk_calendar_key_press (CtkWidget   *widget,
                        CdkEventKey *event)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  gint return_val;
  gint old_focus_row;
  gint old_focus_col;
  gint row, col, day;

  return_val = FALSE;

  old_focus_row = priv->focus_row;
  old_focus_col = priv->focus_col;

  switch (event->keyval)
    {
    case CDK_KEY_KP_Left:
    case CDK_KEY_Left:
      return_val = TRUE;
      if (event->state & CDK_CONTROL_MASK)
        calendar_set_month_prev (calendar);
      else
        {
          move_focus (calendar, -1);
          calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
          calendar_invalidate_day (calendar, priv->focus_row,
                                   priv->focus_col);
        }
      break;
    case CDK_KEY_KP_Right:
    case CDK_KEY_Right:
      return_val = TRUE;
      if (event->state & CDK_CONTROL_MASK)
        calendar_set_month_next (calendar);
      else
        {
          move_focus (calendar, 1);
          calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
          calendar_invalidate_day (calendar, priv->focus_row,
                                   priv->focus_col);
        }
      break;
    case CDK_KEY_KP_Up:
    case CDK_KEY_Up:
      return_val = TRUE;
      if (event->state & CDK_CONTROL_MASK)
        calendar_set_year_prev (calendar);
      else
        {
          if (priv->focus_row > 0)
            priv->focus_row--;
          if (priv->focus_row < 0)
            priv->focus_row = 5;
          if (priv->focus_col < 0)
            priv->focus_col = 6;
          calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
          calendar_invalidate_day (calendar, priv->focus_row,
                                   priv->focus_col);
        }
      break;
    case CDK_KEY_KP_Down:
    case CDK_KEY_Down:
      return_val = TRUE;
      if (event->state & CDK_CONTROL_MASK)
        calendar_set_year_next (calendar);
      else
        {
          if (priv->focus_row < 5)
            priv->focus_row++;
          if (priv->focus_col < 0)
            priv->focus_col = 0;
          calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
          calendar_invalidate_day (calendar, priv->focus_row,
                                   priv->focus_col);
        }
      break;
    case CDK_KEY_KP_Space:
    case CDK_KEY_space:
      row = priv->focus_row;
      col = priv->focus_col;

      if (row > -1 && col > -1)
        {
          return_val = TRUE;

          day = priv->day[row][col];
          if (priv->day_month[row][col] == MONTH_PREV)
            calendar_set_month_prev (calendar);
          else if (priv->day_month[row][col] == MONTH_NEXT)
            calendar_set_month_next (calendar);

          calendar_select_and_focus_day (calendar, day);
        }
    }

  return return_val;
}


/****************************************
 *           Misc widget methods        *
 ****************************************/

static void
ctk_calendar_state_flags_changed (CtkWidget     *widget,
                                  CtkStateFlags  previous_state G_GNUC_UNUSED)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;

  if (!ctk_widget_is_sensitive (widget))
    {
      priv->in_drag = 0;
      calendar_stop_spinning (calendar);
    }
}

static void
ctk_calendar_grab_notify (CtkWidget *widget,
                          gboolean   was_grabbed)
{
  if (!was_grabbed)
    calendar_stop_spinning (CTK_CALENDAR (widget));
}

static gboolean
ctk_calendar_focus_out (CtkWidget     *widget,
                        CdkEventFocus *event G_GNUC_UNUSED)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);

  calendar_queue_refresh (calendar);
  calendar_stop_spinning (calendar);

  calendar->priv->in_drag = 0;

  return FALSE;
}


/****************************************
 *          Drag and Drop               *
 ****************************************/

static void
ctk_calendar_drag_data_get (CtkWidget        *widget,
                            CdkDragContext   *context G_GNUC_UNUSED,
                            CtkSelectionData *selection_data,
                            guint             info G_GNUC_UNUSED,
                            guint             time G_GNUC_UNUSED)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  GDate *date;
  gchar str[128];
  gsize len;

  date = g_date_new_dmy (priv->selected_day, priv->month + 1, priv->year);
  len = g_date_strftime (str, 127, "%x", date);
  ctk_selection_data_set_text (selection_data, str, len);

  g_free (date);
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn’t result from a drop.
 */
static void
set_status_pending (CdkDragContext *context,
                    CdkDragAction   suggested_action)
{
  g_object_set_data (G_OBJECT (context),
                     I_("ctk-calendar-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static CdkDragAction
get_status_pending (CdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context),
                                             "ctk-calendar-status-pending"));
}

static void
ctk_calendar_drag_leave (CtkWidget      *widget,
                         CdkDragContext *context G_GNUC_UNUSED,
                         guint           time G_GNUC_UNUSED)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;

  priv->drag_highlight = 0;
  ctk_drag_unhighlight (widget);

}

static gboolean
ctk_calendar_drag_motion (CtkWidget      *widget,
                          CdkDragContext *context,
                          gint            x G_GNUC_UNUSED,
                          gint            y G_GNUC_UNUSED,
                          guint           time)
{
  CtkCalendarPrivate *priv = CTK_CALENDAR (widget)->priv;
  CdkAtom target;

  if (!priv->drag_highlight)
    {
      priv->drag_highlight = 1;
      ctk_drag_highlight (widget);
    }

  target = ctk_drag_dest_find_target (widget, context, NULL);
  if (target == CDK_NONE || cdk_drag_context_get_suggested_action (context) == 0)
    cdk_drag_status (context, 0, time);
  else
    {
      set_status_pending (context, cdk_drag_context_get_suggested_action (context));
      ctk_drag_get_data (widget, context, target, time);
    }

  return TRUE;
}

static gboolean
ctk_calendar_drag_drop (CtkWidget      *widget,
                        CdkDragContext *context,
                        gint            x G_GNUC_UNUSED,
                        gint            y G_GNUC_UNUSED,
                        guint           time)
{
  CdkAtom target;

  target = ctk_drag_dest_find_target (widget, context, NULL);
  if (target != CDK_NONE)
    {
      ctk_drag_get_data (widget, context,
                         target,
                         time);
      return TRUE;
    }

  return FALSE;
}

static void
ctk_calendar_drag_data_received (CtkWidget        *widget,
                                 CdkDragContext   *context,
                                 gint              x G_GNUC_UNUSED,
                                 gint              y G_GNUC_UNUSED,
                                 CtkSelectionData *selection_data,
                                 guint             info G_GNUC_UNUSED,
                                 guint             time)
{
  CtkCalendar *calendar = CTK_CALENDAR (widget);
  CtkCalendarPrivate *priv = calendar->priv;
  guint day, month, year;
  gchar *str;
  GDate *date;
  CdkDragAction suggested_action;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      set_status_pending (context, 0);

      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      str = (gchar*) ctk_selection_data_get_text (selection_data);

      if (str)
        {
          date = g_date_new ();
          g_date_set_parse (date, str);
          if (!g_date_valid (date))
              suggested_action = 0;
          g_date_free (date);
          g_free (str);
        }
      else
        suggested_action = 0;

      cdk_drag_status (context, suggested_action, time);

      return;
    }

  date = g_date_new ();
  str = (gchar*) ctk_selection_data_get_text (selection_data);
  if (str)
    {
      g_date_set_parse (date, str);
      g_free (str);
    }

  if (!g_date_valid (date))
    {
      g_warning ("Received invalid date data");
      g_date_free (date);
      ctk_drag_finish (context, FALSE, FALSE, time);
      return;
    }

  day = g_date_get_day (date);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  g_date_free (date);

  ctk_drag_finish (context, TRUE, FALSE, time);


  g_object_freeze_notify (G_OBJECT (calendar));
  if (!(priv->display_flags & CTK_CALENDAR_NO_MONTH_CHANGE)
      && (priv->display_flags & CTK_CALENDAR_SHOW_HEADING))
    ctk_calendar_select_month (calendar, month - 1, year);
  ctk_calendar_select_day (calendar, day);
  g_object_thaw_notify (G_OBJECT (calendar));
}


/****************************************
 *              Public API              *
 ****************************************/

/**
 * ctk_calendar_new:
 *
 * Creates a new calendar, with the current date being selected.
 *
 * Returns: a newly #CtkCalendar widget
 **/
CtkWidget*
ctk_calendar_new (void)
{
  return g_object_new (CTK_TYPE_CALENDAR, NULL);
}

/**
 * ctk_calendar_get_display_options:
 * @calendar: a #CtkCalendar
 *
 * Returns the current display options of @calendar.
 *
 * Returns: the display options.
 *
 * Since: 2.4
 **/
CtkCalendarDisplayOptions
ctk_calendar_get_display_options (CtkCalendar         *calendar)
{
  g_return_val_if_fail (CTK_IS_CALENDAR (calendar), 0);

  return calendar->priv->display_flags;
}

/**
 * ctk_calendar_set_display_options:
 * @calendar: a #CtkCalendar
 * @flags: the display options to set
 *
 * Sets display options (whether to display the heading and the month
 * headings).
 *
 * Since: 2.4
 **/
void
ctk_calendar_set_display_options (CtkCalendar          *calendar,
                                  CtkCalendarDisplayOptions flags)
{
  CtkWidget *widget = CTK_WIDGET (calendar);
  CtkCalendarPrivate *priv = calendar->priv;
  gint resize = 0;
  CtkCalendarDisplayOptions old_flags;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  old_flags = priv->display_flags;

  if (ctk_widget_get_realized (widget))
    {
      if ((flags ^ priv->display_flags) & CTK_CALENDAR_NO_MONTH_CHANGE)
        {
          resize ++;
          if (! (flags & CTK_CALENDAR_NO_MONTH_CHANGE)
              && (priv->display_flags & CTK_CALENDAR_SHOW_HEADING))
            {
              priv->display_flags &= ~CTK_CALENDAR_NO_MONTH_CHANGE;
              calendar_realize_arrows (calendar);
              if (ctk_widget_get_mapped (widget))
                calendar_map_arrows (calendar);
            }
          else
            {
              calendar_unrealize_arrows (calendar);
            }
        }

      if ((flags ^ priv->display_flags) & CTK_CALENDAR_SHOW_HEADING)
        {
          resize++;

          if (flags & CTK_CALENDAR_SHOW_HEADING)
            {
              priv->display_flags |= CTK_CALENDAR_SHOW_HEADING;
              calendar_realize_arrows (calendar);
              if (ctk_widget_get_mapped (widget))
                calendar_map_arrows (calendar);
            }
          else
            {
              calendar_unrealize_arrows (calendar);
            }
        }

      if ((flags ^ priv->display_flags) & CTK_CALENDAR_SHOW_DAY_NAMES)
        {
          resize++;

          if (flags & CTK_CALENDAR_SHOW_DAY_NAMES)
            priv->display_flags |= CTK_CALENDAR_SHOW_DAY_NAMES;
        }

      if ((flags ^ priv->display_flags) & CTK_CALENDAR_SHOW_WEEK_NUMBERS)
        {
          resize++;

          if (flags & CTK_CALENDAR_SHOW_WEEK_NUMBERS)
            priv->display_flags |= CTK_CALENDAR_SHOW_WEEK_NUMBERS;
        }

      if ((flags ^ priv->display_flags) & CTK_CALENDAR_SHOW_DETAILS)
        resize++;

      priv->display_flags = flags;
      if (resize)
        ctk_widget_queue_resize (CTK_WIDGET (calendar));
    }
  else
    priv->display_flags = flags;

  g_object_freeze_notify (G_OBJECT (calendar));
  if ((old_flags ^ priv->display_flags) & CTK_CALENDAR_SHOW_HEADING)
    g_object_notify (G_OBJECT (calendar), "show-heading");
  if ((old_flags ^ priv->display_flags) & CTK_CALENDAR_SHOW_DAY_NAMES)
    g_object_notify (G_OBJECT (calendar), "show-day-names");
  if ((old_flags ^ priv->display_flags) & CTK_CALENDAR_NO_MONTH_CHANGE)
    g_object_notify (G_OBJECT (calendar), "no-month-change");
  if ((old_flags ^ priv->display_flags) & CTK_CALENDAR_SHOW_WEEK_NUMBERS)
    g_object_notify (G_OBJECT (calendar), "show-week-numbers");
  g_object_thaw_notify (G_OBJECT (calendar));
}

/**
 * ctk_calendar_select_month:
 * @calendar: a #CtkCalendar
 * @month: a month number between 0 and 11.
 * @year: the year the month is in.
 *
 * Shifts the calendar to a different month.
 **/
void
ctk_calendar_select_month (CtkCalendar *calendar,
                           guint        month,
                           guint        year)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));
  g_return_if_fail (month <= 11);

  priv = calendar->priv;

  g_object_freeze_notify (G_OBJECT (calendar));

  if (priv->month != month)
    {
      priv->month = month;
      g_object_notify (G_OBJECT (calendar), "month");
    }
  if (priv->year != year)
    {
      priv->year  = year;
      g_object_notify (G_OBJECT (calendar), "year");
    }

  calendar_compute_days (calendar);
  calendar_queue_refresh (calendar);

  g_object_thaw_notify (G_OBJECT (calendar));

  g_signal_emit (calendar, ctk_calendar_signals[MONTH_CHANGED_SIGNAL], 0);
}

/**
 * ctk_calendar_select_day:
 * @calendar: a #CtkCalendar.
 * @day: the day number between 1 and 31, or 0 to unselect
 *   the currently selected day.
 *
 * Selects a day from the current month.
 **/
void
ctk_calendar_select_day (CtkCalendar *calendar,
                         guint        day)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));
  g_return_if_fail (day <= 31);

  priv = calendar->priv;

  if (priv->selected_day != day)
    {
      /* Deselect the old day */
      if (priv->selected_day > 0)
        {
          if (ctk_widget_is_drawable (CTK_WIDGET (calendar)))
            calendar_invalidate_day_num (calendar, priv->selected_day);
          priv->selected_day = 0;
        }

      priv->selected_day = day;

      /* Select the new day */
      if (priv->selected_day > 0)
        {
          if (ctk_widget_is_drawable (CTK_WIDGET (calendar)))
            calendar_invalidate_day_num (calendar, priv->selected_day);
        }

      g_object_notify (G_OBJECT (calendar), "day");
    }

  g_signal_emit (calendar, ctk_calendar_signals[DAY_SELECTED_SIGNAL], 0);
}

/**
 * ctk_calendar_clear_marks:
 * @calendar: a #CtkCalendar
 *
 * Remove all visual markers.
 **/
void
ctk_calendar_clear_marks (CtkCalendar *calendar)
{
  CtkCalendarPrivate *priv;
  guint day;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  priv = calendar->priv;

  for (day = 0; day < 31; day++)
    {
      priv->marked_date[day] = FALSE;
    }

  priv->num_marked_dates = 0;
  calendar_queue_refresh (calendar);
}

/**
 * ctk_calendar_mark_day:
 * @calendar: a #CtkCalendar
 * @day: the day number to mark between 1 and 31.
 *
 * Places a visual marker on a particular day.
 */
void
ctk_calendar_mark_day (CtkCalendar *calendar,
                       guint        day)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  priv = calendar->priv;

  if (day >= 1 && day <= 31 && !priv->marked_date[day-1])
    {
      priv->marked_date[day - 1] = TRUE;
      priv->num_marked_dates++;
      calendar_invalidate_day_num (calendar, day);
    }
}

/**
 * ctk_calendar_get_day_is_marked:
 * @calendar: a #CtkCalendar
 * @day: the day number between 1 and 31.
 *
 * Returns if the @day of the @calendar is already marked.
 *
 * Returns: whether the day is marked.
 *
 * Since: 3.0
 */
gboolean
ctk_calendar_get_day_is_marked (CtkCalendar *calendar,
                                guint        day)
{
  CtkCalendarPrivate *priv;

  g_return_val_if_fail (CTK_IS_CALENDAR (calendar), FALSE);

  priv = calendar->priv;

  if (day >= 1 && day <= 31)
    return priv->marked_date[day - 1];

  return FALSE;
}

/**
 * ctk_calendar_unmark_day:
 * @calendar: a #CtkCalendar.
 * @day: the day number to unmark between 1 and 31.
 *
 * Removes the visual marker from a particular day.
 */
void
ctk_calendar_unmark_day (CtkCalendar *calendar,
                         guint        day)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  priv = calendar->priv;

  if (day >= 1 && day <= 31 && priv->marked_date[day-1])
    {
      priv->marked_date[day - 1] = FALSE;
      priv->num_marked_dates--;
      calendar_invalidate_day_num (calendar, day);
    }
}

/**
 * ctk_calendar_get_date:
 * @calendar: a #CtkCalendar
 * @year: (out) (allow-none): location to store the year as a decimal
 *     number (e.g. 2011), or %NULL
 * @month: (out) (allow-none): location to store the month number
 *     (between 0 and 11), or %NULL
 * @day: (out) (allow-none): location to store the day number (between
 *     1 and 31), or %NULL
 *
 * Obtains the selected date from a #CtkCalendar.
 */
void
ctk_calendar_get_date (CtkCalendar *calendar,
                       guint       *year,
                       guint       *month,
                       guint       *day)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  priv = calendar->priv;

  if (year)
    *year = priv->year;

  if (month)
    *month = priv->month;

  if (day)
    *day = priv->selected_day;
}

/**
 * ctk_calendar_set_detail_func:
 * @calendar: a #CtkCalendar.
 * @func: a function providing details for each day.
 * @data: data to pass to @func invokations.
 * @destroy: a function for releasing @data.
 *
 * Installs a function which provides Pango markup with detail information
 * for each day. Examples for such details are holidays or appointments. That
 * information is shown below each day when #CtkCalendar:show-details is set.
 * A tooltip containing with full detail information is provided, if the entire
 * text should not fit into the details area, or if #CtkCalendar:show-details
 * is not set.
 *
 * The size of the details area can be restricted by setting the
 * #CtkCalendar:detail-width-chars and #CtkCalendar:detail-height-rows
 * properties.
 *
 * Since: 2.14
 */
void
ctk_calendar_set_detail_func (CtkCalendar           *calendar,
                              CtkCalendarDetailFunc  func,
                              gpointer               data,
                              GDestroyNotify         destroy)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  priv = calendar->priv;

  if (priv->detail_func_destroy)
    priv->detail_func_destroy (priv->detail_func_user_data);

  priv->detail_func = func;
  priv->detail_func_user_data = data;
  priv->detail_func_destroy = destroy;

  ctk_widget_set_has_tooltip (CTK_WIDGET (calendar),
                              NULL != priv->detail_func);
  ctk_widget_queue_resize (CTK_WIDGET (calendar));
}

/**
 * ctk_calendar_set_detail_width_chars:
 * @calendar: a #CtkCalendar.
 * @chars: detail width in characters.
 *
 * Updates the width of detail cells.
 * See #CtkCalendar:detail-width-chars.
 *
 * Since: 2.14
 */
void
ctk_calendar_set_detail_width_chars (CtkCalendar *calendar,
                                     gint         chars)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  priv = calendar->priv;

  if (chars != priv->detail_width_chars)
    {
      priv->detail_width_chars = chars;
      g_object_notify (G_OBJECT (calendar), "detail-width-chars");
      ctk_widget_queue_resize_no_redraw (CTK_WIDGET (calendar));
    }
}

/**
 * ctk_calendar_set_detail_height_rows:
 * @calendar: a #CtkCalendar.
 * @rows: detail height in rows.
 *
 * Updates the height of detail cells.
 * See #CtkCalendar:detail-height-rows.
 *
 * Since: 2.14
 */
void
ctk_calendar_set_detail_height_rows (CtkCalendar *calendar,
                                     gint         rows)
{
  CtkCalendarPrivate *priv;

  g_return_if_fail (CTK_IS_CALENDAR (calendar));

  priv = calendar->priv;

  if (rows != priv->detail_height_rows)
    {
      priv->detail_height_rows = rows;
      g_object_notify (G_OBJECT (calendar), "detail-height-rows");
      ctk_widget_queue_resize_no_redraw (CTK_WIDGET (calendar));
    }
}

/**
 * ctk_calendar_get_detail_width_chars:
 * @calendar: a #CtkCalendar.
 *
 * Queries the width of detail cells, in characters.
 * See #CtkCalendar:detail-width-chars.
 *
 * Since: 2.14
 *
 * Returns: The width of detail cells, in characters.
 */
gint
ctk_calendar_get_detail_width_chars (CtkCalendar *calendar)
{
  g_return_val_if_fail (CTK_IS_CALENDAR (calendar), 0);

  return calendar->priv->detail_width_chars;
}

/**
 * ctk_calendar_get_detail_height_rows:
 * @calendar: a #CtkCalendar.
 *
 * Queries the height of detail cells, in rows.
 * See #CtkCalendar:detail-width-chars.
 *
 * Since: 2.14
 *
 * Returns: The height of detail cells, in rows.
 */
gint
ctk_calendar_get_detail_height_rows (CtkCalendar *calendar)
{
  g_return_val_if_fail (CTK_IS_CALENDAR (calendar), 0);

  return calendar->priv->detail_height_rows;
}
