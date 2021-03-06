/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TEXT_CHILD_H__
#define __CTK_TEXT_CHILD_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <glib-object.h>

G_BEGIN_DECLS


/**
 * CtkTextChildAnchor:
 *
 * A #CtkTextChildAnchor is a spot in the buffer where child widgets can
 * be “anchored” (inserted inline, as if they were characters). The anchor
 * can have multiple widgets anchored, to allow for multiple views.
 */
typedef struct _CtkTextChildAnchor      CtkTextChildAnchor;
typedef struct _CtkTextChildAnchorClass CtkTextChildAnchorClass;

#define CTK_TYPE_TEXT_CHILD_ANCHOR              (ctk_text_child_anchor_get_type ())
#define CTK_TEXT_CHILD_ANCHOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_TEXT_CHILD_ANCHOR, CtkTextChildAnchor))
#define CTK_TEXT_CHILD_ANCHOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_CHILD_ANCHOR, CtkTextChildAnchorClass))
#define CTK_IS_TEXT_CHILD_ANCHOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_TEXT_CHILD_ANCHOR))
#define CTK_IS_TEXT_CHILD_ANCHOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_CHILD_ANCHOR))
#define CTK_TEXT_CHILD_ANCHOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_CHILD_ANCHOR, CtkTextChildAnchorClass))

struct _CtkTextChildAnchor
{
  GObject parent_instance;

  /*< private >*/
  gpointer segment;
};

struct _CtkTextChildAnchorClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType               ctk_text_child_anchor_get_type    (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkTextChildAnchor* ctk_text_child_anchor_new         (void);

CDK_AVAILABLE_IN_ALL
GList*              ctk_text_child_anchor_get_widgets (CtkTextChildAnchor *anchor);
CDK_AVAILABLE_IN_ALL
gboolean            ctk_text_child_anchor_get_deleted (CtkTextChildAnchor *anchor);

G_END_DECLS

#endif
