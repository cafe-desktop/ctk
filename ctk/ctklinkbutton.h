/* GTK - The GIMP Toolkit
 * gtklinkbutton.h - an hyperlink-enabled button
 *
 * Copyright (C) 2005 Emmanuele Bassi <ebassi@gmail.com>
 * All rights reserved.
 *
 * Based on gnome-href code by:
 * 	James Henstridge <james@daa.com.au>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_LINK_BUTTON_H__
#define __CTK_LINK_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkbutton.h>

G_BEGIN_DECLS

#define CTK_TYPE_LINK_BUTTON		(ctk_link_button_get_type ())
#define CTK_LINK_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LINK_BUTTON, GtkLinkButton))
#define CTK_IS_LINK_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LINK_BUTTON))
#define CTK_LINK_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LINK_BUTTON, GtkLinkButtonClass))
#define CTK_IS_LINK_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LINK_BUTTON))
#define CTK_LINK_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LINK_BUTTON, GtkLinkButtonClass))

typedef struct _GtkLinkButton		GtkLinkButton;
typedef struct _GtkLinkButtonClass	GtkLinkButtonClass;
typedef struct _GtkLinkButtonPrivate	GtkLinkButtonPrivate;

/**
 * GtkLinkButton:
 *
 * The #GtkLinkButton-struct contains only
 * private data and should be accessed using the provided API.
 */
struct _GtkLinkButton
{
  /*< private >*/
  GtkButton parent_instance;

  GtkLinkButtonPrivate *priv;
};

/**
 * GtkLinkButtonClass:
 * @activate_link: class handler for the #GtkLinkButton::activate-link signal
 *
 * The #GtkLinkButtonClass contains only
 * private data.
 */
struct _GtkLinkButtonClass
{
  /*< private >*/
  GtkButtonClass parent_class;

  /*< public >*/
  gboolean (* activate_link) (GtkLinkButton *button);

  /*< private >*/
  /* Padding for future expansion */
  void (*_ctk_padding1) (void);
  void (*_ctk_padding2) (void);
  void (*_ctk_padding3) (void);
  void (*_ctk_padding4) (void);
};

GDK_AVAILABLE_IN_ALL
GType                 ctk_link_button_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkWidget *           ctk_link_button_new               (const gchar   *uri);
GDK_AVAILABLE_IN_ALL
GtkWidget *           ctk_link_button_new_with_label    (const gchar   *uri,
						         const gchar   *label);

GDK_AVAILABLE_IN_ALL
const gchar *         ctk_link_button_get_uri           (GtkLinkButton *link_button);
GDK_AVAILABLE_IN_ALL
void                  ctk_link_button_set_uri           (GtkLinkButton *link_button,
						         const gchar   *uri);

GDK_AVAILABLE_IN_ALL
gboolean              ctk_link_button_get_visited       (GtkLinkButton *link_button);
GDK_AVAILABLE_IN_ALL
void                  ctk_link_button_set_visited       (GtkLinkButton *link_button,
                                                         gboolean       visited);


G_END_DECLS

#endif /* __CTK_LINK_BUTTON_H__ */
