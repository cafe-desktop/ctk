/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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
 *
 * Global clipboard abstraction.
 */

#ifndef __CTK_CLIPBOARD_PRIVATE_H__
#define __CTK_CLIPBOARD_PRIVATE_H__

#include <ctk/ctkclipboard.h>

G_BEGIN_DECLS

#define CTK_CLIPBOARD_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CLIPBOARD, CtkClipboardClass))
#define CTK_IS_CLIPBOARD_CLASS(klass)	        (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CLIPBOARD))
#define CTK_CLIPBOARD_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CLIPBOARD, CtkClipboardClass))

typedef struct _CtkClipboardClass CtkClipboardClass;

struct _CtkClipboard 
{
  GObject parent_instance;

  GdkAtom selection;

  CtkClipboardGetFunc get_func;
  CtkClipboardClearFunc clear_func;
  gpointer user_data;
  gboolean have_owner;

  guint32 timestamp;

  gboolean have_selection;
  GdkDisplay *display;

  GdkAtom *cached_targets;
  gint     n_cached_targets;

  gulong     notify_signal_id;
  gboolean   storing_selection;
  GMainLoop *store_loop;
  guint      store_timeout;
  gint       n_storable_targets;
  GdkAtom   *storable_targets;
};

struct _CtkClipboardClass
{
  GObjectClass parent_class;

  /* vfuncs */
  gboolean      (* set_contents)                (CtkClipboard                   *clipboard,
                                                 const CtkTargetEntry           *targets,
                                                 guint                           n_targets,
                                                 CtkClipboardGetFunc             get_func,
                                                 CtkClipboardClearFunc           clear_func,
                                                 gpointer                        user_data,
                                                 gboolean                        have_owner);
  void          (* clear)                       (CtkClipboard                   *clipboard);
  void          (* request_contents)            (CtkClipboard                   *clipboard,
                                                 GdkAtom                         target,
                                                 CtkClipboardReceivedFunc        callback,
                                                 gpointer                        user_data);
  void          (* set_can_store)               (CtkClipboard                   *clipboard,
                                                 const CtkTargetEntry           *targets,
                                                 gint                            n_targets);
  void          (* store)                       (CtkClipboard                   *clipboard);

  /* signals */
  void          (* owner_change)                (CtkClipboard                   *clipboard,
                                                 GdkEventOwnerChange            *event);
};
void     _ctk_clipboard_handle_event    (GdkEventOwnerChange *event);

void     _ctk_clipboard_store_all       (void);


G_END_DECLS

#endif /* __CTK_CLIPBOARD_PRIVATE_H__ */
