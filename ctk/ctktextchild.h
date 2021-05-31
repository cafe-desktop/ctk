/* GTK - The GIMP Toolkit
 * ctktextchild.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_TEXT_CHILD_H__
#define __CTK_TEXT_CHILD_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <glib-object.h>

G_BEGIN_DECLS


/**
 * GtkTextChildAnchor:
 *
 * A #GtkTextChildAnchor is a spot in the buffer where child widgets can
 * be “anchored” (inserted inline, as if they were characters). The anchor
 * can have multiple widgets anchored, to allow for multiple views.
 */
typedef struct _GtkTextChildAnchor      GtkTextChildAnchor;
typedef struct _GtkTextChildAnchorClass GtkTextChildAnchorClass;

#define CTK_TYPE_TEXT_CHILD_ANCHOR              (ctk_text_child_anchor_get_type ())
#define CTK_TEXT_CHILD_ANCHOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_TEXT_CHILD_ANCHOR, GtkTextChildAnchor))
#define CTK_TEXT_CHILD_ANCHOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_CHILD_ANCHOR, GtkTextChildAnchorClass))
#define CTK_IS_TEXT_CHILD_ANCHOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_TEXT_CHILD_ANCHOR))
#define CTK_IS_TEXT_CHILD_ANCHOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_CHILD_ANCHOR))
#define CTK_TEXT_CHILD_ANCHOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_CHILD_ANCHOR, GtkTextChildAnchorClass))

struct _GtkTextChildAnchor
{
  GObject parent_instance;

  /*< private >*/
  gpointer segment;
};

struct _GtkTextChildAnchorClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType               ctk_text_child_anchor_get_type    (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkTextChildAnchor* ctk_text_child_anchor_new         (void);

GDK_AVAILABLE_IN_ALL
GList*              ctk_text_child_anchor_get_widgets (GtkTextChildAnchor *anchor);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_text_child_anchor_get_deleted (GtkTextChildAnchor *anchor);

G_END_DECLS

#endif
