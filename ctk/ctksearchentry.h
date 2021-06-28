/* CTK - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
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
 * Modified by the CTK+ Team and others 2012.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_SEARCH_ENTRY_H__
#define __CTK_SEARCH_ENTRY_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkentry.h>

G_BEGIN_DECLS

#define CTK_TYPE_SEARCH_ENTRY                 (ctk_search_entry_get_type ())
#define CTK_SEARCH_ENTRY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEARCH_ENTRY, CtkSearchEntry))
#define CTK_SEARCH_ENTRY_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEARCH_ENTRY, CtkSearchEntryClass))
#define CTK_IS_SEARCH_ENTRY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEARCH_ENTRY))
#define CTK_IS_SEARCH_ENTRY_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEARCH_ENTRY))
#define CTK_SEARCH_ENTRY_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SEARCH_ENTRY, CtkSearchEntryClass))

typedef struct _CtkSearchEntry       CtkSearchEntry;
typedef struct _CtkSearchEntryClass  CtkSearchEntryClass;

struct _CtkSearchEntry
{
  CtkEntry parent;
};

struct _CtkSearchEntryClass
{
  CtkEntryClass parent_class;

  void (*search_changed) (CtkSearchEntry *entry);
  void (*next_match)     (CtkSearchEntry *entry);
  void (*previous_match) (CtkSearchEntry *entry);
  void (*stop_search)    (CtkSearchEntry *entry);
};

GDK_AVAILABLE_IN_3_6
GType           ctk_search_entry_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_6
CtkWidget*      ctk_search_entry_new            (void);

GDK_AVAILABLE_IN_3_16
gboolean        ctk_search_entry_handle_event   (CtkSearchEntry *entry,
                                                 CdkEvent       *event);

G_END_DECLS

#endif /* __CTK_SEARCH_ENTRY_H__ */
