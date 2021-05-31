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
#define CTK_ENTRY_COMPLETION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ENTRY_COMPLETION, GtkEntryCompletion))
#define CTK_ENTRY_COMPLETION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ENTRY_COMPLETION, GtkEntryCompletionClass))
#define CTK_IS_ENTRY_COMPLETION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ENTRY_COMPLETION))
#define CTK_IS_ENTRY_COMPLETION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ENTRY_COMPLETION))
#define CTK_ENTRY_COMPLETION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ENTRY_COMPLETION, GtkEntryCompletionClass))

typedef struct _GtkEntryCompletion            GtkEntryCompletion;
typedef struct _GtkEntryCompletionClass       GtkEntryCompletionClass;
typedef struct _GtkEntryCompletionPrivate     GtkEntryCompletionPrivate;

/**
 * GtkEntryCompletionMatchFunc:
 * @completion: the #GtkEntryCompletion
 * @key: the string to match, normalized and case-folded
 * @iter: a #GtkTreeIter indicating the row to match
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
typedef gboolean (* GtkEntryCompletionMatchFunc) (GtkEntryCompletion *completion,
                                                  const gchar        *key,
                                                  GtkTreeIter        *iter,
                                                  gpointer            user_data);


struct _GtkEntryCompletion
{
  GObject parent_instance;

  /*< private >*/
  GtkEntryCompletionPrivate *priv;
};

struct _GtkEntryCompletionClass
{
  GObjectClass parent_class;

  gboolean (* match_selected)   (GtkEntryCompletion *completion,
                                 GtkTreeModel       *model,
                                 GtkTreeIter        *iter);
  void     (* action_activated) (GtkEntryCompletion *completion,
                                 gint                index_);
  gboolean (* insert_prefix)    (GtkEntryCompletion *completion,
                                 const gchar        *prefix);
  gboolean (* cursor_on_match)  (GtkEntryCompletion *completion,
                                 GtkTreeModel       *model,
                                 GtkTreeIter        *iter);
  void     (* no_matches)       (GtkEntryCompletion *completion);

  /* Padding for future expansion */
  void (*_ctk_reserved0) (void);
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
};

/* core */
GDK_AVAILABLE_IN_ALL
GType               ctk_entry_completion_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkEntryCompletion *ctk_entry_completion_new                    (void);
GDK_AVAILABLE_IN_ALL
GtkEntryCompletion *ctk_entry_completion_new_with_area          (GtkCellArea                 *area);

GDK_AVAILABLE_IN_ALL
GtkWidget          *ctk_entry_completion_get_entry              (GtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_model              (GtkEntryCompletion          *completion,
                                                                 GtkTreeModel                *model);
GDK_AVAILABLE_IN_ALL
GtkTreeModel       *ctk_entry_completion_get_model              (GtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_match_func         (GtkEntryCompletion          *completion,
                                                                 GtkEntryCompletionMatchFunc  func,
                                                                 gpointer                     func_data,
                                                                 GDestroyNotify               func_notify);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_minimum_key_length (GtkEntryCompletion          *completion,
                                                                 gint                         length);
GDK_AVAILABLE_IN_ALL
gint                ctk_entry_completion_get_minimum_key_length (GtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_3_4
gchar *             ctk_entry_completion_compute_prefix         (GtkEntryCompletion          *completion,
                                                                 const char                  *key);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_complete               (GtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_insert_prefix          (GtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_insert_action_text     (GtkEntryCompletion          *completion,
                                                                 gint                         index_,
                                                                 const gchar                 *text);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_insert_action_markup   (GtkEntryCompletion          *completion,
                                                                 gint                         index_,
                                                                 const gchar                 *markup);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_delete_action          (GtkEntryCompletion          *completion,
                                                                 gint                         index_);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_inline_completion  (GtkEntryCompletion          *completion,
                                                                 gboolean                     inline_completion);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_inline_completion  (GtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_inline_selection  (GtkEntryCompletion          *completion,
                                                                 gboolean                     inline_selection);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_inline_selection  (GtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_popup_completion   (GtkEntryCompletion          *completion,
                                                                 gboolean                     popup_completion);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_popup_completion   (GtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_popup_set_width    (GtkEntryCompletion          *completion,
                                                                 gboolean                     popup_set_width);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_popup_set_width    (GtkEntryCompletion          *completion);
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_popup_single_match (GtkEntryCompletion          *completion,
                                                                 gboolean                     popup_single_match);
GDK_AVAILABLE_IN_ALL
gboolean            ctk_entry_completion_get_popup_single_match (GtkEntryCompletion          *completion);

GDK_AVAILABLE_IN_ALL
const gchar         *ctk_entry_completion_get_completion_prefix (GtkEntryCompletion *completion);
/* convenience */
GDK_AVAILABLE_IN_ALL
void                ctk_entry_completion_set_text_column        (GtkEntryCompletion          *completion,
                                                                 gint                         column);
GDK_AVAILABLE_IN_ALL
gint                ctk_entry_completion_get_text_column        (GtkEntryCompletion          *completion);

G_END_DECLS

#endif /* __CTK_ENTRY_COMPLETION_H__ */
