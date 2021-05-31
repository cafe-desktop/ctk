/*
 * Copyright © 2011 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __CTK_APPLICATION_WINDOW_H__
#define __CTK_APPLICATION_WINDOW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>
#include <ctk/ctkshortcutswindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_APPLICATION_WINDOW            (ctk_application_window_get_type ())
#define CTK_APPLICATION_WINDOW(inst)           (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                                                CTK_TYPE_APPLICATION_WINDOW, GtkApplicationWindow))
#define CTK_APPLICATION_WINDOW_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class),   \
                                                CTK_TYPE_APPLICATION_WINDOW, GtkApplicationWindowClass))
#define CTK_IS_APPLICATION_WINDOW(inst)        (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                                                CTK_TYPE_APPLICATION_WINDOW))
#define CTK_IS_APPLICATION_WINDOW_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class),   \
                                                CTK_TYPE_APPLICATION_WINDOW))
#define CTK_APPLICATION_WINDOW_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS ((inst),  \
                                                CTK_TYPE_APPLICATION_WINDOW, GtkApplicationWindowClass))

typedef struct _GtkApplicationWindowPrivate GtkApplicationWindowPrivate;
typedef struct _GtkApplicationWindowClass   GtkApplicationWindowClass;
typedef struct _GtkApplicationWindow        GtkApplicationWindow;

struct _GtkApplicationWindow
{
  GtkWindow parent_instance;

  /*< private >*/
  GtkApplicationWindowPrivate *priv;
};

/**
 * GtkApplicationWindowClass:
 * @parent_class: The parent class.
 */
struct _GtkApplicationWindowClass
{
  GtkWindowClass parent_class;

  /*< private >*/
  gpointer padding[14];
};

GDK_AVAILABLE_IN_3_4
GType       ctk_application_window_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_4
GtkWidget * ctk_application_window_new               (GtkApplication      *application);

GDK_AVAILABLE_IN_3_4
void        ctk_application_window_set_show_menubar (GtkApplicationWindow *window,
                                                     gboolean              show_menubar);
GDK_AVAILABLE_IN_3_4
gboolean    ctk_application_window_get_show_menubar (GtkApplicationWindow *window);

GDK_AVAILABLE_IN_3_6
guint       ctk_application_window_get_id           (GtkApplicationWindow *window);

GDK_AVAILABLE_IN_3_20
void        ctk_application_window_set_help_overlay (GtkApplicationWindow *window,
                                                     GtkShortcutsWindow   *help_overlay);
GDK_AVAILABLE_IN_3_20
GtkShortcutsWindow *
            ctk_application_window_get_help_overlay (GtkApplicationWindow *window);

G_END_DECLS

#endif /* __CTK_APPLICATION_WINDOW_H__ */
