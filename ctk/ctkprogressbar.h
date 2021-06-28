/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_PROGRESS_BAR_H__
#define __CTK_PROGRESS_BAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_PROGRESS_BAR            (ctk_progress_bar_get_type ())
#define CTK_PROGRESS_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PROGRESS_BAR, CtkProgressBar))
#define CTK_PROGRESS_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PROGRESS_BAR, CtkProgressBarClass))
#define CTK_IS_PROGRESS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PROGRESS_BAR))
#define CTK_IS_PROGRESS_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PROGRESS_BAR))
#define CTK_PROGRESS_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PROGRESS_BAR, CtkProgressBarClass))


typedef struct _CtkProgressBar              CtkProgressBar;
typedef struct _CtkProgressBarPrivate       CtkProgressBarPrivate;
typedef struct _CtkProgressBarClass         CtkProgressBarClass;

struct _CtkProgressBar
{
  CtkWidget parent;

  /*< private >*/
  CtkProgressBarPrivate *priv;
};

struct _CtkProgressBarClass
{
  CtkWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType      ctk_progress_bar_get_type             (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_progress_bar_new                  (void);

CDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_pulse                (CtkProgressBar *pbar);
CDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_text             (CtkProgressBar *pbar,
                                                  const gchar    *text);
CDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_fraction         (CtkProgressBar *pbar,
                                                  gdouble         fraction);

CDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_pulse_step       (CtkProgressBar *pbar,
                                                  gdouble         fraction);
CDK_AVAILABLE_IN_ALL
void       ctk_progress_bar_set_inverted         (CtkProgressBar *pbar,
                                                  gboolean        inverted);

CDK_AVAILABLE_IN_ALL
const gchar *      ctk_progress_bar_get_text       (CtkProgressBar *pbar);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_progress_bar_get_fraction   (CtkProgressBar *pbar);
CDK_AVAILABLE_IN_ALL
gdouble            ctk_progress_bar_get_pulse_step (CtkProgressBar *pbar);

CDK_AVAILABLE_IN_ALL
gboolean           ctk_progress_bar_get_inverted    (CtkProgressBar *pbar);
CDK_AVAILABLE_IN_ALL
void               ctk_progress_bar_set_ellipsize (CtkProgressBar     *pbar,
                                                   PangoEllipsizeMode  mode);
CDK_AVAILABLE_IN_ALL
PangoEllipsizeMode ctk_progress_bar_get_ellipsize (CtkProgressBar     *pbar);

CDK_AVAILABLE_IN_ALL
void               ctk_progress_bar_set_show_text (CtkProgressBar     *pbar,
                                                   gboolean            show_text);
CDK_AVAILABLE_IN_ALL
gboolean           ctk_progress_bar_get_show_text (CtkProgressBar     *pbar);

G_END_DECLS

#endif /* __CTK_PROGRESS_BAR_H__ */
