/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * GtkToolbar copyright (C) Federico Mena
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TOOLBAR_H__
#define __CTK_TOOLBAR_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>
#include <ctk/ctktoolitem.h>

G_BEGIN_DECLS


#define CTK_TYPE_TOOLBAR            (ctk_toolbar_get_type ())
#define CTK_TOOLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TOOLBAR, GtkToolbar))
#define CTK_TOOLBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TOOLBAR, GtkToolbarClass))
#define CTK_IS_TOOLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TOOLBAR))
#define CTK_IS_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TOOLBAR))
#define CTK_TOOLBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TOOLBAR, GtkToolbarClass))

/**
 * GtkToolbarSpaceStyle:
 * @CTK_TOOLBAR_SPACE_EMPTY: Use blank spacers.
 * @CTK_TOOLBAR_SPACE_LINE: Use vertical lines for spacers.
 *
 * Whether spacers are vertical lines or just blank.
 *
 * Deprecated: 3.20
 */
typedef enum
{
  CTK_TOOLBAR_SPACE_EMPTY,
  CTK_TOOLBAR_SPACE_LINE
} GtkToolbarSpaceStyle;

typedef struct _GtkToolbar              GtkToolbar;
typedef struct _GtkToolbarPrivate       GtkToolbarPrivate;
typedef struct _GtkToolbarClass         GtkToolbarClass;

struct _GtkToolbar
{
  GtkContainer container;

  GtkToolbarPrivate *priv;
};

struct _GtkToolbarClass
{
  GtkContainerClass parent_class;

  /* signals */
  void     (* orientation_changed) (GtkToolbar       *toolbar,
				    GtkOrientation    orientation);
  void     (* style_changed)       (GtkToolbar       *toolbar,
				    GtkToolbarStyle   style);
  gboolean (* popup_context_menu)  (GtkToolbar       *toolbar,
				    gint              x,
				    gint              y,
				    gint              button_number);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType           ctk_toolbar_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget *     ctk_toolbar_new                     (void);

GDK_AVAILABLE_IN_ALL
void            ctk_toolbar_insert                  (GtkToolbar      *toolbar,
						     GtkToolItem     *item,
						     gint             pos);

GDK_AVAILABLE_IN_ALL
gint            ctk_toolbar_get_item_index          (GtkToolbar      *toolbar,
						     GtkToolItem     *item);
GDK_AVAILABLE_IN_ALL
gint            ctk_toolbar_get_n_items             (GtkToolbar      *toolbar);
GDK_AVAILABLE_IN_ALL
GtkToolItem *   ctk_toolbar_get_nth_item            (GtkToolbar      *toolbar,
						     gint             n);

GDK_AVAILABLE_IN_ALL
gboolean        ctk_toolbar_get_show_arrow          (GtkToolbar      *toolbar);
GDK_AVAILABLE_IN_ALL
void            ctk_toolbar_set_show_arrow          (GtkToolbar      *toolbar,
						     gboolean         show_arrow);

GDK_AVAILABLE_IN_ALL
GtkToolbarStyle ctk_toolbar_get_style               (GtkToolbar      *toolbar);
GDK_AVAILABLE_IN_ALL
void            ctk_toolbar_set_style               (GtkToolbar      *toolbar,
						     GtkToolbarStyle  style);
GDK_AVAILABLE_IN_ALL
void            ctk_toolbar_unset_style             (GtkToolbar      *toolbar);

GDK_AVAILABLE_IN_ALL
GtkIconSize     ctk_toolbar_get_icon_size           (GtkToolbar      *toolbar);
GDK_AVAILABLE_IN_ALL
void            ctk_toolbar_set_icon_size           (GtkToolbar      *toolbar,
                                                     GtkIconSize      icon_size);
GDK_AVAILABLE_IN_ALL
void            ctk_toolbar_unset_icon_size         (GtkToolbar      *toolbar);

GDK_AVAILABLE_IN_ALL
GtkReliefStyle  ctk_toolbar_get_relief_style        (GtkToolbar      *toolbar);
GDK_AVAILABLE_IN_ALL
gint            ctk_toolbar_get_drop_index          (GtkToolbar      *toolbar,
						     gint             x,
						     gint             y);
GDK_AVAILABLE_IN_ALL
void            ctk_toolbar_set_drop_highlight_item (GtkToolbar      *toolbar,
						     GtkToolItem     *tool_item,
						     gint             index_);


G_END_DECLS

#endif /* __CTK_TOOLBAR_H__ */
