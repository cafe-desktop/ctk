/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_PROGRESS_BAR_H__
#define __CTK_PROGRESS_BAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_PROGRESS_BAR            (ctk_progress_bar_get_type ())
#define CTK_PROGRESS_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PROGRESS_BAR, GtkProgressBar))
#define CTK_PROGRESS_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PROGRESS_BAR, GtkProgressBarClass))
#define CTK_IS_PROGRESS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PROGRESS_BAR))
#define CTK_IS_PROGRESS_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PROGRESS_BAR))
#define CTK_PROGRESS_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PROGRESS_BAR, GtkProgressBarClass))


typedef struct _GtkProgressBar              GtkProgressBar;
typedef struct _GtkProgressBarPrivate       GtkProgressBarPrivate;
typedef struct _GtkProgressBarClass         GtkProgressBarClass;

struct _GtkProgressBar
{
  GtkWidget parent;

  /*< private >*/
  GtkProgressBarPrivate *priv;
};

struct _GtkProgressBarClass
{
  GtkWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      ctk_progress_bar_get_type             (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget* ctk_progress_bar_new                  (void);

GDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_pulse                (GtkProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_text             (GtkProgressBar *pbar,
                                                  const gchar    *text);
GDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_fraction         (GtkProgressBar *pbar,
                                                  gdouble         fraction);

GDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_pulse_step       (GtkProgressBar *pbar,
                                                  gdouble         fraction);
GDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_inverted         (GtkProgressBar *pbar,
                                                  gboolean        inverted);

GDK_AVAILABLE_IN_ALL
const gchar *      ctk_progress_bar_get_text       (GtkProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_progress_bar_get_fraction   (GtkProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
gdouble            ctk_progress_bar_get_pulse_step (GtkProgressBar *pbar);

GDK_AVAILABLE_IN_ALL
gboolean           ctk_progress_bar_get_inverted    (GtkProgressBar *pbar);
GDK_AVAILABLE_IN_ALL
void               ctk_progress_bar_set_ellipsize (GtkProgressBar     *pbar,
                                                   PangoEllipsizeMode  mode);
GDK_AVAILABLE_IN_ALL
PangoEllipsizeMode ctk_progress_bar_get_ellipsize (GtkProgressBar     *pbar);

GDK_AVAILABLE_IN_ALL
void               ctk_progress_bar_set_show_text (GtkProgressBar     *pbar,
                                                   gboolean            show_text);
GDK_AVAILABLE_IN_ALL
gboolean           ctk_progress_bar_get_show_text (GtkProgressBar     *pbar);

G_END_DECLS

#endif /* __CTK_PROGRESS_BAR_H__ */
