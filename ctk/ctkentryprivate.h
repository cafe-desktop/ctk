/* ctkentryprivate.h
 * Copyright (C) 2003  Kristian Rietveld  <kris@gtk.org>
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

#ifndef __CTK_ENTRY_PRIVATE_H__
#define __CTK_ENTRY_PRIVATE_H__

#include <ctk/ctktreeviewcolumn.h>
#include <ctk/ctktreemodelfilter.h>
#include <ctk/ctkliststore.h>
#include <ctk/ctkentrycompletion.h>
#include <ctk/ctkentry.h>
#include <ctk/ctkcssgadgetprivate.h>
#include <ctk/ctkspinbutton.h>

G_BEGIN_DECLS

struct _CtkEntryCompletionPrivate
{
  CtkWidget *entry;

  CtkWidget *tree_view;
  CtkTreeViewColumn *column;
  CtkTreeModelFilter *filter_model;
  CtkListStore *actions;
  CtkCellArea *cell_area;

  CtkEntryCompletionMatchFunc match_func;
  gpointer match_data;
  GDestroyNotify match_notify;

  gint minimum_key_length;
  gint text_column;

  gchar *case_normalized_key;

  /* only used by CtkEntry when attached: */
  CtkWidget *popup_window;
  CtkWidget *vbox;
  CtkWidget *scrolled_window;
  CtkWidget *action_view;

  gulong completion_timeout;
  gulong changed_id;
  gulong insert_text_id;

  gint current_selected;

  guint first_sel_changed : 1;
  guint ignore_enter      : 1;
  guint has_completion    : 1;
  guint inline_completion : 1;
  guint popup_completion  : 1;
  guint popup_set_width   : 1;
  guint popup_single_match : 1;
  guint inline_selection   : 1;
  guint has_grab           : 1;

  gchar *completion_prefix;

  GSource *check_completion_idle;

  CdkDevice *device;
};

void     _ctk_entry_completion_resize_popup (CtkEntryCompletion *completion);
void     _ctk_entry_completion_popdown      (CtkEntryCompletion *completion);
void     _ctk_entry_completion_connect      (CtkEntryCompletion *completion,
                                             CtkEntry           *entry);
void     _ctk_entry_completion_disconnect   (CtkEntryCompletion *completion);

gchar*   _ctk_entry_get_display_text       (CtkEntry *entry,
                                            gint      start_pos,
                                            gint      end_pos);
void     _ctk_entry_get_borders            (CtkEntry  *entry,
                                            CtkBorder *borders);
CtkIMContext* _ctk_entry_get_im_context    (CtkEntry  *entry);
CtkCssGadget* ctk_entry_get_gadget         (CtkEntry  *entry);
void     _ctk_entry_grab_focus             (CtkEntry  *entry,
                                            gboolean   select_all);

/* in ctkspinbutton.c (because I'm too lazy to create ctkspinbuttonprivate.h) */
gint     ctk_spin_button_get_text_width    (CtkSpinButton *spin_button);

void     ctk_entry_enter_text              (CtkEntry   *entry,
                                            const char *text);
void     ctk_entry_set_positions           (CtkEntry   *entry,
                                            int         current_pos,
                                            int         selection_bound);

G_END_DECLS

#endif /* __CTK_ENTRY_PRIVATE_H__ */
