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
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_EDITABLE_H__
#define __CTK_EDITABLE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>


G_BEGIN_DECLS

#define CTK_TYPE_EDITABLE             (ctk_editable_get_type ())
#define CTK_EDITABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_EDITABLE, CtkEditable))
#define CTK_IS_EDITABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_EDITABLE))
#define CTK_EDITABLE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CTK_TYPE_EDITABLE, CtkEditableInterface))

typedef struct _CtkEditable          CtkEditable;         /* Dummy typedef */
typedef struct _CtkEditableInterface CtkEditableInterface;

struct _CtkEditableInterface
{
  GTypeInterface		   base_iface;

  /* signals */
  void (* insert_text)              (CtkEditable    *editable,
				     const gchar    *new_text,
				     gint            new_text_length,
				     gint           *position);
  void (* delete_text)              (CtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);
  void (* changed)                  (CtkEditable    *editable);

  /* vtable */
  void (* do_insert_text)           (CtkEditable    *editable,
				     const gchar    *new_text,
				     gint            new_text_length,
				     gint           *position);
  void (* do_delete_text)           (CtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);

  gchar* (* get_chars)              (CtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);
  void (* set_selection_bounds)     (CtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);
  gboolean (* get_selection_bounds) (CtkEditable    *editable,
				     gint           *start_pos,
				     gint           *end_pos);
  void (* set_position)             (CtkEditable    *editable,
				     gint            position);
  gint (* get_position)             (CtkEditable    *editable);
};

GDK_AVAILABLE_IN_ALL
GType    ctk_editable_get_type             (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
void     ctk_editable_select_region        (CtkEditable *editable,
					    gint         start_pos,
					    gint         end_pos);
GDK_AVAILABLE_IN_ALL
gboolean ctk_editable_get_selection_bounds (CtkEditable *editable,
					    gint        *start_pos,
					    gint        *end_pos);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_insert_text          (CtkEditable *editable,
					    const gchar *new_text,
					    gint         new_text_length,
					    gint        *position);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_delete_text          (CtkEditable *editable,
					    gint         start_pos,
					    gint         end_pos);
GDK_AVAILABLE_IN_ALL
gchar*   ctk_editable_get_chars            (CtkEditable *editable,
					    gint         start_pos,
					    gint         end_pos);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_cut_clipboard        (CtkEditable *editable);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_copy_clipboard       (CtkEditable *editable);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_paste_clipboard      (CtkEditable *editable);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_delete_selection     (CtkEditable *editable);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_set_position         (CtkEditable *editable,
					    gint         position);
GDK_AVAILABLE_IN_ALL
gint     ctk_editable_get_position         (CtkEditable *editable);
GDK_AVAILABLE_IN_ALL
void     ctk_editable_set_editable         (CtkEditable *editable,
					    gboolean     is_editable);
GDK_AVAILABLE_IN_ALL
gboolean ctk_editable_get_editable         (CtkEditable *editable);

G_END_DECLS

#endif /* __CTK_EDITABLE_H__ */
