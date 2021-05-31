/* GTK - The GIMP Toolkit
 * Recent chooser action for GtkUIManager
 *
 * Copyright (C) 2007, Emmanuele Bassi
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

#ifndef __CTK_RECENT_ACTION_H__
#define __CTK_RECENT_ACTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/deprecated/gtkaction.h>
#include <gtk/gtkrecentmanager.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_ACTION                  (ctk_recent_action_get_type ())
#define CTK_RECENT_ACTION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_ACTION, GtkRecentAction))
#define CTK_IS_RECENT_ACTION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_ACTION))
#define CTK_RECENT_ACTION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RECENT_ACTION, GtkRecentActionClass))
#define CTK_IS_RECENT_ACTION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RECENT_ACTION))
#define CTK_RECENT_ACTION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RECENT_ACTION, GtkRecentActionClass))

typedef struct _GtkRecentAction         GtkRecentAction;
typedef struct _GtkRecentActionPrivate  GtkRecentActionPrivate;
typedef struct _GtkRecentActionClass    GtkRecentActionClass;

struct _GtkRecentAction
{
  GtkAction parent_instance;

  /*< private >*/
  GtkRecentActionPrivate *priv;
};

struct _GtkRecentActionClass
{
  GtkActionClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_DEPRECATED_IN_3_10
GType      ctk_recent_action_get_type         (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_10
GtkAction *ctk_recent_action_new              (const gchar      *name,
                                               const gchar      *label,
                                               const gchar      *tooltip,
                                               const gchar      *stock_id);
GDK_DEPRECATED_IN_3_10
GtkAction *ctk_recent_action_new_for_manager  (const gchar      *name,
                                               const gchar      *label,
                                               const gchar      *tooltip,
                                               const gchar      *stock_id,
                                               GtkRecentManager *manager);
GDK_DEPRECATED_IN_3_10
gboolean   ctk_recent_action_get_show_numbers (GtkRecentAction  *action);
GDK_DEPRECATED_IN_3_10
void       ctk_recent_action_set_show_numbers (GtkRecentAction  *action,
                                               gboolean          show_numbers);

G_END_DECLS

#endif /* __CTK_RECENT_ACTION_H__ */
