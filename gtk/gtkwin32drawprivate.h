/* GTK - The GIMP Toolkit
 * Copyright (C) 2016 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_WIN32_DRAW_H__
#define __CTK_WIN32_DRAW_H__

#include <gdk/gdk.h>
#include <cairo.h>

#include <gtk/gtkborder.h>

G_BEGIN_DECLS

enum {
  CTK_WIN32_SYS_COLOR_SCROLLBAR,
  CTK_WIN32_SYS_COLOR_BACKGROUND,
  CTK_WIN32_SYS_COLOR_ACTIVECAPTION,
  CTK_WIN32_SYS_COLOR_INACTIVECAPTION,
  CTK_WIN32_SYS_COLOR_MENU,
  CTK_WIN32_SYS_COLOR_WINDOW,
  CTK_WIN32_SYS_COLOR_WINDOWFRAME,
  CTK_WIN32_SYS_COLOR_MENUTEXT,
  CTK_WIN32_SYS_COLOR_WINDOWTEXT,
  CTK_WIN32_SYS_COLOR_CAPTIONTEXT,
  CTK_WIN32_SYS_COLOR_ACTIVEBORDER,
  CTK_WIN32_SYS_COLOR_INACTIVEBORDER,
  CTK_WIN32_SYS_COLOR_APPWORKSPACE,
  CTK_WIN32_SYS_COLOR_HIGHLIGHT,
  CTK_WIN32_SYS_COLOR_HIGHLIGHTTEXT,
  CTK_WIN32_SYS_COLOR_BTNFACE,
  CTK_WIN32_SYS_COLOR_BTNSHADOW,
  CTK_WIN32_SYS_COLOR_GRAYTEXT,
  CTK_WIN32_SYS_COLOR_BTNTEXT,
  CTK_WIN32_SYS_COLOR_INACTIVECAPTIONTEXT,
  CTK_WIN32_SYS_COLOR_BTNHIGHLIGHT,
  CTK_WIN32_SYS_COLOR_3DDKSHADOW,
  CTK_WIN32_SYS_COLOR_3DLIGHT,
  CTK_WIN32_SYS_COLOR_INFOTEXT,
  CTK_WIN32_SYS_COLOR_INFOBK,
  CTK_WIN32_SYS_COLOR_ALTERNATEBTNFACE,
  CTK_WIN32_SYS_COLOR_HOTLIGHT,
  CTK_WIN32_SYS_COLOR_GRADIENTACTIVECAPTION,
  CTK_WIN32_SYS_COLOR_GRADIENTINACTIVECAPTION,
  CTK_WIN32_SYS_COLOR_MENUHILIGHT,
  CTK_WIN32_SYS_COLOR_MENUBAR
};

enum {
  CTK_WIN32_SYS_METRIC_CXSCREEN = 0,
  CTK_WIN32_SYS_METRIC_CYSCREEN = 1,
  CTK_WIN32_SYS_METRIC_CXVSCROLL = 2,
  CTK_WIN32_SYS_METRIC_CYHSCROLL = 3,
  CTK_WIN32_SYS_METRIC_CYCAPTION = 4,
  CTK_WIN32_SYS_METRIC_CXBORDER = 5,
  CTK_WIN32_SYS_METRIC_CYBORDER = 6,
  CTK_WIN32_SYS_METRIC_CXDLGFRAME = 7,
  CTK_WIN32_SYS_METRIC_CYDLGFRAME = 8,
  CTK_WIN32_SYS_METRIC_CYVTHUMB = 9,
  CTK_WIN32_SYS_METRIC_CXHTHUMB = 10,
  CTK_WIN32_SYS_METRIC_CXICON = 11,
  CTK_WIN32_SYS_METRIC_CYICON = 12,
  CTK_WIN32_SYS_METRIC_CXCURSOR = 13,
  CTK_WIN32_SYS_METRIC_CYCURSOR = 14,
  CTK_WIN32_SYS_METRIC_CYMENU = 15,
  CTK_WIN32_SYS_METRIC_CXFULLSCREEN = 16,
  CTK_WIN32_SYS_METRIC_CYFULLSCREEN = 17,
  CTK_WIN32_SYS_METRIC_CYKANJIWINDOW = 18,
  CTK_WIN32_SYS_METRIC_MOUSEPRESENT = 19,
  CTK_WIN32_SYS_METRIC_CYVSCROLL = 20,
  CTK_WIN32_SYS_METRIC_CXHSCROLL = 21,
  CTK_WIN32_SYS_METRIC_DEBUG = 22,
  CTK_WIN32_SYS_METRIC_SWAPBUTTON = 23,
  CTK_WIN32_SYS_METRIC_RESERVED1 = 24,
  CTK_WIN32_SYS_METRIC_RESERVED2 = 25,
  CTK_WIN32_SYS_METRIC_RESERVED3 = 26,
  CTK_WIN32_SYS_METRIC_RESERVED4 = 27,
  CTK_WIN32_SYS_METRIC_CXMIN = 28,
  CTK_WIN32_SYS_METRIC_CYMIN = 29,
  CTK_WIN32_SYS_METRIC_CXSIZE = 30,
  CTK_WIN32_SYS_METRIC_CYSIZE = 31,
  CTK_WIN32_SYS_METRIC_CXFRAME = 32,
  CTK_WIN32_SYS_METRIC_CYFRAME = 33,
  CTK_WIN32_SYS_METRIC_CXMINTRACK = 34,
  CTK_WIN32_SYS_METRIC_CYMINTRACK = 35,
  CTK_WIN32_SYS_METRIC_CXDOUBLECLK = 36,
  CTK_WIN32_SYS_METRIC_CYDOUBLECLK = 37,
  CTK_WIN32_SYS_METRIC_CXICONSPACING = 38,
  CTK_WIN32_SYS_METRIC_CYICONSPACING = 39,
  CTK_WIN32_SYS_METRIC_MENUDROPALIGNMENT = 40,
  CTK_WIN32_SYS_METRIC_PENWINDOWS = 41,
  CTK_WIN32_SYS_METRIC_DBCSENABLED = 42,
  CTK_WIN32_SYS_METRIC_CMOUSEBUTTONS = 43,
  CTK_WIN32_SYS_METRIC_SECURE = 44,
  CTK_WIN32_SYS_METRIC_CXEDGE = 45,
  CTK_WIN32_SYS_METRIC_CYEDGE = 46,
  CTK_WIN32_SYS_METRIC_CXMINSPACING = 47,
  CTK_WIN32_SYS_METRIC_CYMINSPACING = 48,
  CTK_WIN32_SYS_METRIC_CXSMICON = 49,
  CTK_WIN32_SYS_METRIC_CYSMICON = 50,
  CTK_WIN32_SYS_METRIC_CYSMCAPTION = 51,
  CTK_WIN32_SYS_METRIC_CXSMSIZE = 52,
  CTK_WIN32_SYS_METRIC_CYSMSIZE = 53,
  CTK_WIN32_SYS_METRIC_CXMENUSIZE = 54,
  CTK_WIN32_SYS_METRIC_CYMENUSIZE = 55,
  CTK_WIN32_SYS_METRIC_ARRANGE = 56,
  CTK_WIN32_SYS_METRIC_CXMINIMIZED = 57,
  CTK_WIN32_SYS_METRIC_CYMINIMIZED = 58,
  CTK_WIN32_SYS_METRIC_CXMAXTRACK = 59,
  CTK_WIN32_SYS_METRIC_CYMAXTRACK = 60,
  CTK_WIN32_SYS_METRIC_CXMAXIMIZED = 61,
  CTK_WIN32_SYS_METRIC_CYMAXIMIZED = 62,
  CTK_WIN32_SYS_METRIC_NETWORK = 63,
  CTK_WIN32_SYS_METRIC_CLEANBOOT = 67,
  CTK_WIN32_SYS_METRIC_CXDRAG = 68,
  CTK_WIN32_SYS_METRIC_CYDRAG = 69,
  CTK_WIN32_SYS_METRIC_SHOWSOUNDS = 70,
  CTK_WIN32_SYS_METRIC_CXMENUCHECK = 71,
  CTK_WIN32_SYS_METRIC_CYMENUCHECK = 72,
  CTK_WIN32_SYS_METRIC_SLOWMACHINE = 73,
  CTK_WIN32_SYS_METRIC_MIDEASTENABLED = 74,
  CTK_WIN32_SYS_METRIC_MOUSEWHEELPRESENT = 75,
  CTK_WIN32_SYS_METRIC_XVIRTUALSCREEN = 76,
  CTK_WIN32_SYS_METRIC_YVIRTUALSCREEN = 77,
  CTK_WIN32_SYS_METRIC_CXVIRTUALSCREEN = 78,
  CTK_WIN32_SYS_METRIC_CYVIRTUALSCREEN = 79,
  CTK_WIN32_SYS_METRIC_CMONITORS = 80,
  CTK_WIN32_SYS_METRIC_SAMEDISPLAYFORMAT = 81,
  CTK_WIN32_SYS_METRIC_IMMENABLED = 82,
  CTK_WIN32_SYS_METRIC_CXFOCUSBORDER = 83,
  CTK_WIN32_SYS_METRIC_CYFOCUSBORDER = 84,
  CTK_WIN32_SYS_METRIC_TABLETPC = 86,
  CTK_WIN32_SYS_METRIC_MEDIACENTER = 87,
  CTK_WIN32_SYS_METRIC_STARTER = 88,
  CTK_WIN32_SYS_METRIC_SERVERR2 = 89,
  CTK_WIN32_SYS_METRIC_CMETRICS = 90,
  CTK_WIN32_SYS_METRIC_MOUSEHORIZONTALWHEELPRESENT = 91,
  CTK_WIN32_SYS_METRIC_CXPADDEDBORDER = 92
};

void                    ctk_win32_draw_theme_background         (cairo_t        *cr,
                                                                 const char     *class_name,
                                                                 int             part,
                                                                 int             state,
                                                                 int             width,
                                                                 int             height);
void                    ctk_win32_get_theme_part_size           (const char     *class_name,
                                                                 int             part,
                                                                 int             state,
                                                                 int            *width,
                                                                 int            *height);
void                    ctk_win32_get_theme_margins             (const char     *class_name,
                                                                 int             part,
                                                                 int             state,
                                                                 GtkBorder      *out_margins);

const char *            ctk_win32_get_sys_metric_name_for_id    (gint            id);
int                     ctk_win32_get_sys_metric_id_for_name    (const char     *name);
int                     ctk_win32_get_sys_metric                (gint            id);

const char *            ctk_win32_get_sys_color_name_for_id     (gint            id);
int                     ctk_win32_get_sys_color_id_for_name     (const char     *name);
void                    ctk_win32_get_sys_color                 (gint            id,
                                                                 GdkRGBA        *color);

G_END_DECLS

#endif /* __CTK_WIN32_DRAW_H__ */
