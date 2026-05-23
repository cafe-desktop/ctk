/* 
 * CTK - The GIMP Toolkit
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003  Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
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

#ifndef __CTK_ASSISTANT_H__
#define __CTK_ASSISTANT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_ASSISTANT         (ctk_assistant_get_type ())
#define CTK_ASSISTANT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_ASSISTANT, CtkAssistant))
#define CTK_ASSISTANT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_ASSISTANT, CtkAssistantClass))
#define CTK_IS_ASSISTANT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_ASSISTANT))
#define CTK_IS_ASSISTANT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_ASSISTANT))
#define CTK_ASSISTANT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_ASSISTANT, CtkAssistantClass))

/**
 * CtkAssistantPageType:
 * @CTK_ASSISTANT_PAGE_CONTENT: The page has regular contents. Both the
 *  Back and forward buttons will be shown.
 * @CTK_ASSISTANT_PAGE_INTRO: The page contains an introduction to the
 *  assistant task. Only the Forward button will be shown if there is a
 *   next page.
 * @CTK_ASSISTANT_PAGE_CONFIRM: The page lets the user confirm or deny the
 *  changes. The Back and Apply buttons will be shown.
 * @CTK_ASSISTANT_PAGE_SUMMARY: The page informs the user of the changes
 *  done. Only the Close button will be shown.
 * @CTK_ASSISTANT_PAGE_PROGRESS: Used for tasks that take a long time to
 *  complete, blocks the assistant until the page is marked as complete.
 *   Only the back button will be shown.
 * @CTK_ASSISTANT_PAGE_CUSTOM: Used for when other page types are not
 *  appropriate. No buttons will be shown, and the application must
 *  add its own buttons through ctk_assistant_add_action_widget().
 *
 * An enum for determining the page role inside the #CtkAssistant. It's
 * used to handle buttons sensitivity and visibility.
 *
 * Note that an assistant needs to end its page flow with a page of type
 * %CTK_ASSISTANT_PAGE_CONFIRM, %CTK_ASSISTANT_PAGE_SUMMARY or
 * %CTK_ASSISTANT_PAGE_PROGRESS to be correct.
 *
 * The Cancel button will only be shown if the page isn’t “committed”.
 * See ctk_assistant_commit() for details.
 */
typedef enum
{
  CTK_ASSISTANT_PAGE_CONTENT,
  CTK_ASSISTANT_PAGE_INTRO,
  CTK_ASSISTANT_PAGE_CONFIRM,
  CTK_ASSISTANT_PAGE_SUMMARY,
  CTK_ASSISTANT_PAGE_PROGRESS,
  CTK_ASSISTANT_PAGE_CUSTOM
} CtkAssistantPageType;

typedef struct _CtkAssistant        CtkAssistant;
typedef struct _CtkAssistantPrivate CtkAssistantPrivate;
typedef struct _CtkAssistantClass   CtkAssistantClass;

struct _CtkAssistant
{
  CtkWindow  parent;

  /*< private >*/
  CtkAssistantPrivate *priv;
};

/**
 * CtkAssistantClass:
 * @parent_class: The parent class.
 * @prepare: Signal emitted when a new page is set as the assistant’s current page, before making the new page visible.
 * @apply: Signal emitted when the apply button is clicked.
 * @close: Signal emitted either when the close button or last page apply button is clicked.
 * @cancel: Signal emitted when the cancel button is clicked.
 */
struct _CtkAssistantClass
{
  CtkWindowClass parent_class;

  /*< public >*/

  void (* prepare) (CtkAssistant *assistant, CtkWidget *page);
  void (* apply)   (CtkAssistant *assistant);
  void (* close)   (CtkAssistant *assistant);
  void (* cancel)  (CtkAssistant *assistant);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
};

/**
 * CtkAssistantPageFunc:
 * @current_page: The page number used to calculate the next page.
 * @data: (closure): user data.
 *
 * A function used by ctk_assistant_set_forward_page_func() to know which
 * is the next page given a current one. It’s called both for computing the
 * next page when the user presses the “forward” button and for handling
 * the behavior of the “last” button.
 *
 * Returns: The next page number.
 */
typedef gint (*CtkAssistantPageFunc) (gint current_page, gpointer data);

CDK_AVAILABLE_IN_ALL
GType                 ctk_assistant_get_type              (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget            *ctk_assistant_new                   (void);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_next_page             (CtkAssistant         *assistant);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_previous_page         (CtkAssistant         *assistant);
CDK_AVAILABLE_IN_ALL
gint                  ctk_assistant_get_current_page      (CtkAssistant         *assistant);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_set_current_page      (CtkAssistant         *assistant,
                                                           gint                  page_num);
CDK_AVAILABLE_IN_ALL
gint                  ctk_assistant_get_n_pages           (CtkAssistant         *assistant);
CDK_AVAILABLE_IN_ALL
CtkWidget            *ctk_assistant_get_nth_page          (CtkAssistant         *assistant,
                                                           gint                  page_num);
CDK_AVAILABLE_IN_ALL
gint                  ctk_assistant_prepend_page          (CtkAssistant         *assistant,
                                                           CtkWidget            *page);
CDK_AVAILABLE_IN_ALL
gint                  ctk_assistant_append_page           (CtkAssistant         *assistant,
                                                           CtkWidget            *page);
CDK_AVAILABLE_IN_ALL
gint                  ctk_assistant_insert_page           (CtkAssistant         *assistant,
                                                           CtkWidget            *page,
                                                           gint                  position);
CDK_AVAILABLE_IN_3_2
void                  ctk_assistant_remove_page           (CtkAssistant         *assistant,
                                                           gint                  page_num);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_set_forward_page_func (CtkAssistant         *assistant,
                                                           CtkAssistantPageFunc  page_func,
                                                           gpointer              data,
                                                           GDestroyNotify        destroy);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_set_page_type         (CtkAssistant         *assistant,
                                                           CtkWidget            *page,
                                                           CtkAssistantPageType  type);
CDK_AVAILABLE_IN_ALL
CtkAssistantPageType  ctk_assistant_get_page_type         (CtkAssistant         *assistant,
                                                           CtkWidget            *page);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_set_page_title        (CtkAssistant         *assistant,
                                                           CtkWidget            *page,
                                                           const gchar          *title);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_assistant_get_page_title        (CtkAssistant         *assistant,
                                                           CtkWidget            *page);

CDK_DEPRECATED_IN_3_2
void                  ctk_assistant_set_page_header_image (CtkAssistant         *assistant,
                                                           CtkWidget            *page,
                                                           CdkPixbuf            *pixbuf);
CDK_DEPRECATED_IN_3_2
CdkPixbuf            *ctk_assistant_get_page_header_image (CtkAssistant         *assistant,
                                                           CtkWidget            *page);
CDK_DEPRECATED_IN_3_2
void                  ctk_assistant_set_page_side_image   (CtkAssistant         *assistant,
                                                           CtkWidget            *page,
                                                           CdkPixbuf            *pixbuf);
CDK_DEPRECATED_IN_3_2
CdkPixbuf            *ctk_assistant_get_page_side_image   (CtkAssistant         *assistant,
                                                           CtkWidget            *page);

CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_set_page_complete     (CtkAssistant         *assistant,
                                                           CtkWidget            *page,
                                                           gboolean              complete);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_assistant_get_page_complete     (CtkAssistant         *assistant,
                                                           CtkWidget            *page);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_add_action_widget     (CtkAssistant         *assistant,
                                                           CtkWidget            *child);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_remove_action_widget  (CtkAssistant         *assistant,
                                                           CtkWidget            *child);

CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_update_buttons_state  (CtkAssistant *assistant);
CDK_AVAILABLE_IN_ALL
void                  ctk_assistant_commit                (CtkAssistant *assistant);

CDK_AVAILABLE_IN_3_18
void                  ctk_assistant_set_page_has_padding  (CtkAssistant *assistant,
                                                           CtkWidget    *page,
                                                           gboolean      has_padding);
CDK_AVAILABLE_IN_3_18
gboolean              ctk_assistant_get_page_has_padding  (CtkAssistant *assistant,
                                                           CtkWidget    *page);

G_END_DECLS

#endif /* __CTK_ASSISTANT_H__ */
