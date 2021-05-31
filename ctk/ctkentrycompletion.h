/* ctkentrycompletion.h
 * Copyright (C) 2003  Kristian Rietveld  <kris@ctk.org>
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

#ifndef __CTK_ENTRY_COMPLETION_H__
#define __CTK_ENTRY_COMPLETION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctkliststore.h>
#include <ctk/ctkcellarea.h>
#include <ctk/ctktreeviewcolumn.h>
#include <ctk/ctktreemodelfilter.h>

G_BEGIN_DECLS

#define CTK_TYPE_ENTRY_COMPLETION            (ctk_entry_completion_get_type ())
#define CTK_ENTRY_COMPLETION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ENTRY_COMPLETION, CtkEntryCompletion))
#define CTK_ENTRY_COMPLETION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ENTRY_COMPLETION, CtkEntryCompletionClass))
#define CTK_IS_ENTRY_COMPLETION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ENTRY_COMPLETION))
#define CTK_IS_ENTRY_COMPLETION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ENTRY_COMPLETION))
#define CTK_ENTRY_COMPLETION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ENTRY_COMPLETION, CtkEntryCompletionClass))

typedef struct _CtkEntryCompletion            CtkEntryCompletion;
typedef struct _CtkEntryCompletionClass       CtkEntryCompletionClass;
typedef struct _CtkEntryCompletionPrivate     CtkEntryCompletionPrivate;

/**
 * CtkEntryCompletionMatchFunc:
 * @completion: the #CtkEntryCompletion
 * @key: the string to match, normalized and case-folded
 * @iter: a #CtkTreeIter indicating the row to match
 * @user_data: user data given to ctk_entry_completion_set_match_func()
 *
 * A function which decides whether the row indicated by @iter matches
 * a given @key, and should be displayed as a possible completion for @key.
 * Note that @key is normalized and case-folded (see g_utf8_normalize()
 * and g_utf8_casefold()). If this is not appropriate, match functions
 * have access to the unmodified key via
 * `ctk_entry_get_text (CTK_ENTRY (ctk_entry_completion_get_entry ()))`.
 *
 * Returns: %TRUE if @iter should be displayed as a possible completion
 *     for @key
 */
typedef gboolean (* CtkEntryCompletionMatchFunc) (CtkEntryCompletion *completion,
                                                  const gchar        *key,
                                                  CtkTreeIter        *iter,
                                                  gpointer            user_data);


struct _CtkEntryCompletion
{
  GObject parent_instance;

  /*< private >*/
  CtkEntryCompletionPrivate *priv;
};

struct _CtkEntryCompletionClass
{
  GObjectClass parent_class;

  gboolean (* match_selected)   (CtkEntryCompletion *completion,
                                 CtkTreeModel       *model,
                                 CtkTreeIter        *iter);
  void     (* action_activated) (CtkEntryCompletion *completion,
                                 gint                index_);
  gboolean (* insert_prefix)    (CtkEntryCompletion *completion,
                                 const gchar        *prefix);
  gboolean (* cursor_on_match)  (CtkEntryCompletion *completion,
                                 CtkTreeModel       *model,
                                 CtkTreeIter        *iter);
  void     (* no_matches)       (CtkEntryCompletion *completion);

  /* Padding for future expansion */
  void (*_ctk_reserved0) (void);
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
};

/* core */
GDK_AVAILABLE_IN_ALL
GType               ctk_entry_completion_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkEntryCompletion *ctk_entry_completion_new                    (void);
GDK_AVAILABLE_IN_ALL
CtkEntryCompletion *ctk_entry_completion_new_with_area          (CtkCellArea                 *area);

GDK_AVAILABLE_IN_ALL
CtkWidget          *ctk_entry_completion_get_entry              (CtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_model              (CtkEntryCompletion          *completion,
                                                                 CtkTreeModel                *model);
GDK_AVAILABLE_IN_ALL
CtkTreeModel       *ctk_entry_completion_get_model              (CtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_match_func         (CtkEntryCompletion          *completion,
                                                                 CtkEntryCompletionMatchFunc  func,
                                                                 gpointer                     func_data,
                                                                 GDestroyNotify               func_notify);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_minimum_key_length (CtkEntryCompletion          *completion,
                                                                 gint                         length);
GDK_AVAILABLE_IN_ALL
gint                ctk_entry_completion_get_minimum_key_length (CtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_3_4
gchar *             ctk_entry_completion_compute_prefix         (CtkEntryCompletion          *completion,
                                                                 const char                  *key);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_complete               (CtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_insert_prefix          (CtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_insert_action_text     (CtkEntryCompletion          *completion,
                                                                 gint                         index_,
                                                                 const gchar                 *text);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_insert_action_markup   (CtkEntryCompletion          *completion,
                                                                 gint                         index_,
                                                                 const gchar                 *markup);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_delete_action          (CtkEntryCompletion          *completion,
                                                                 gint                         index_);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_inline_completion  (CtkEntryCompletion          *completion,
                                                                 gboolean                     inline_completion);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_inline_completion  (CtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_inline_selection  (CtkEntryCompletion          *completion,
                                                                 gboolean                     inline_selection);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_inline_selection  (CtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_popup_completion   (CtkEntryCompletion          *completion,
                                                                 gboolean                     popup_completion);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_popup_completion   (CtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_popup_set_width    (CtkEntryCompletion          *completion,
                                                                 gboolean                     popup_set_width);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_popup_set_width    (CtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_popup_single_match (CtkEntryCompletion          *completion,
                                                                 gboolean                     popup_single_match);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_popup_single_match (CtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
const gchar         *ctk_entry_completion_get_completion_prefix (CtkEntryCompletion *completion);
/* convenience */
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_text_column        (CtkEntryCompletion          *completion,
                                                                 gint                         column);
GDK_AVAILABLE_IN_ALL
gint                ctk_entry_completion_get_text_column        (CtkEntryCompletion          *completion);

G_END_DECLS

#endif /* __CTK_ENTRY_COMPLETION_H__ */
