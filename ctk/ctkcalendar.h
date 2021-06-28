/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * CTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel and Shawn T. Amundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_CALENDAR_H__
#define __CTK_CALENDAR_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>


G_BEGIN_DECLS

#define CTK_TYPE_CALENDAR                  (ctk_calendar_get_type ())
#define CTK_CALENDAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CALENDAR, CtkCalendar))
#define CTK_CALENDAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CALENDAR, CtkCalendarClass))
#define CTK_IS_CALENDAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CALENDAR))
#define CTK_IS_CALENDAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CALENDAR))
#define CTK_CALENDAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CALENDAR, CtkCalendarClass))


typedef struct _CtkCalendar	       CtkCalendar;
typedef struct _CtkCalendarClass       CtkCalendarClass;

typedef struct _CtkCalendarPrivate     CtkCalendarPrivate;

/**
 * CtkCalendarDisplayOptions:
 * @CTK_CALENDAR_SHOW_HEADING: Specifies that the month and year should be displayed.
 * @CTK_CALENDAR_SHOW_DAY_NAMES: Specifies that three letter day descriptions should be present.
 * @CTK_CALENDAR_NO_MONTH_CHANGE: Prevents the user from switching months with the calendar.
 * @CTK_CALENDAR_SHOW_WEEK_NUMBERS: Displays each week numbers of the current year, down the
 * left side of the calendar.
 * @CTK_CALENDAR_SHOW_DETAILS: Just show an indicator, not the full details
 * text when details are provided. See ctk_calendar_set_detail_func().
 *
 * These options can be used to influence the display and behaviour of a #CtkCalendar.
 */
typedef enum
{
  CTK_CALENDAR_SHOW_HEADING		= 1 << 0,
  CTK_CALENDAR_SHOW_DAY_NAMES		= 1 << 1,
  CTK_CALENDAR_NO_MONTH_CHANGE		= 1 << 2,
  CTK_CALENDAR_SHOW_WEEK_NUMBERS	= 1 << 3,
  CTK_CALENDAR_SHOW_DETAILS		= 1 << 5
} CtkCalendarDisplayOptions;

/**
 * CtkCalendarDetailFunc:
 * @calendar: a #CtkCalendar.
 * @year: the year for which details are needed.
 * @month: the month for which details are needed.
 * @day: the day of @month for which details are needed.
 * @user_data: the data passed with ctk_calendar_set_detail_func().
 *
 * This kind of functions provide Pango markup with detail information for the
 * specified day. Examples for such details are holidays or appointments. The
 * function returns %NULL when no information is available.
 *
 * Since: 2.14
 *
 * Returns: (nullable) (transfer full): Newly allocated string with Pango markup
 *     with details for the specified day or %NULL.
 */
typedef gchar* (*CtkCalendarDetailFunc) (CtkCalendar *calendar,
                                         guint        year,
                                         guint        month,
                                         guint        day,
                                         gpointer     user_data);

struct _CtkCalendar
{
  CtkWidget widget;

  CtkCalendarPrivate *priv;
};

struct _CtkCalendarClass
{
  CtkWidgetClass parent_class;
  
  /* Signal handlers */
  void (* month_changed)		(CtkCalendar *calendar);
  void (* day_selected)			(CtkCalendar *calendar);
  void (* day_selected_double_click)	(CtkCalendar *calendar);
  void (* prev_month)			(CtkCalendar *calendar);
  void (* next_month)			(CtkCalendar *calendar);
  void (* prev_year)			(CtkCalendar *calendar);
  void (* next_year)			(CtkCalendar *calendar);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType	   ctk_calendar_get_type	(void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_calendar_new		(void);

CDK_AVAILABLE_IN_ALL
void       ctk_calendar_select_month	(CtkCalendar *calendar,
					 guint	      month,
					 guint	      year);
CDK_AVAILABLE_IN_ALL
void	   ctk_calendar_select_day	(CtkCalendar *calendar,
					 guint	      day);

CDK_AVAILABLE_IN_ALL
void       ctk_calendar_mark_day	(CtkCalendar *calendar,
					 guint	      day);
CDK_AVAILABLE_IN_ALL
void       ctk_calendar_unmark_day	(CtkCalendar *calendar,
					 guint	      day);
CDK_AVAILABLE_IN_ALL
void	   ctk_calendar_clear_marks	(CtkCalendar *calendar);


CDK_AVAILABLE_IN_ALL
void	   ctk_calendar_set_display_options (CtkCalendar    	      *calendar,
					     CtkCalendarDisplayOptions flags);
CDK_AVAILABLE_IN_ALL
CtkCalendarDisplayOptions
           ctk_calendar_get_display_options (CtkCalendar   	      *calendar);
CDK_AVAILABLE_IN_ALL
void	   ctk_calendar_get_date	(CtkCalendar *calendar, 
					 guint	     *year,
					 guint	     *month,
					 guint	     *day);

CDK_AVAILABLE_IN_ALL
void       ctk_calendar_set_detail_func (CtkCalendar           *calendar,
                                         CtkCalendarDetailFunc  func,
                                         gpointer               data,
                                         GDestroyNotify         destroy);

CDK_AVAILABLE_IN_ALL
void       ctk_calendar_set_detail_width_chars (CtkCalendar    *calendar,
                                                gint            chars);
CDK_AVAILABLE_IN_ALL
void       ctk_calendar_set_detail_height_rows (CtkCalendar    *calendar,
                                                gint            rows);

CDK_AVAILABLE_IN_ALL
gint       ctk_calendar_get_detail_width_chars (CtkCalendar    *calendar);
CDK_AVAILABLE_IN_ALL
gint       ctk_calendar_get_detail_height_rows (CtkCalendar    *calendar);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_calendar_get_day_is_marked      (CtkCalendar    *calendar,
                                                guint           day);

G_END_DECLS

#endif /* __CTK_CALENDAR_H__ */
