/* CTK - The GIMP Toolkit
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

#ifndef __CTK_CLIPBOARD_H__
#define __CTK_CLIPBOARD_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkselection.h>

G_BEGIN_DECLS

#define CTK_TYPE_CLIPBOARD            (ctk_clipboard_get_type ())
#define CTK_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CLIPBOARD, CtkClipboard))
#define CTK_IS_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CLIPBOARD))

/**
 * CtkClipboardReceivedFunc:
 * @clipboard: the #CtkClipboard
 * @selection_data: a #CtkSelectionData containing the data was received.
 *   If retrieving the data failed, then then length field
 *   of @selection_data will be negative.
 * @data: (closure): the @user_data supplied to
 *   ctk_clipboard_request_contents().
 *
 * A function to be called when the results of ctk_clipboard_request_contents()
 * are received, or when the request fails.
 */
typedef void (* CtkClipboardReceivedFunc)         (CtkClipboard     *clipboard,
					           CtkSelectionData *selection_data,
					           gpointer          data);

/**
 * CtkClipboardTextReceivedFunc:
 * @clipboard: the #CtkClipboard
 * @text: (nullable): the text received, as a UTF-8 encoded string, or
 *   %NULL if retrieving the data failed.
 * @data: (closure): the @user_data supplied to
 *   ctk_clipboard_request_text().
 *
 * A function to be called when the results of ctk_clipboard_request_text()
 * are received, or when the request fails.
 */
typedef void (* CtkClipboardTextReceivedFunc)     (CtkClipboard     *clipboard,
					           const gchar      *text,
					           gpointer          data);

/**
 * CtkClipboardRichTextReceivedFunc:
 * @clipboard: the #CtkClipboard
 * @format: The format of the rich text
 * @text: (nullable) (type utf8): the rich text received, as
 *   a UTF-8 encoded string, or %NULL if retrieving the data failed.
 * @length: Length of the text.
 * @data: (closure): the @user_data supplied to
 *   ctk_clipboard_request_rich_text().
 *
 * A function to be called when the results of
 * ctk_clipboard_request_rich_text() are received, or when the request
 * fails.
 *
 * Since: 2.10
 */
typedef void (* CtkClipboardRichTextReceivedFunc) (CtkClipboard     *clipboard,
                                                   CdkAtom           format,
					           const guint8     *text,
                                                   gsize             length,
					           gpointer          data);

/**
 * CtkClipboardImageReceivedFunc:
 * @clipboard: the #CtkClipboard
 * @pixbuf: the received image
 * @data: (closure): the @user_data supplied to
 *   ctk_clipboard_request_image().
 *
 * A function to be called when the results of ctk_clipboard_request_image()
 * are received, or when the request fails.
 *
 * Since: 2.6
 */
typedef void (* CtkClipboardImageReceivedFunc)    (CtkClipboard     *clipboard,
						   CdkPixbuf        *pixbuf,
						   gpointer          data);

/**
 * CtkClipboardURIReceivedFunc:
 * @clipboard: the #CtkClipboard
 * @uris: (array zero-terminated=1): the received URIs
 * @data: (closure): the @user_data supplied to
 *   ctk_clipboard_request_uris().
 *
 * A function to be called when the results of
 * ctk_clipboard_request_uris() are received, or when the request
 * fails.
 *
 * Since: 2.14
 */
typedef void (* CtkClipboardURIReceivedFunc)      (CtkClipboard     *clipboard,
						   gchar           **uris,
						   gpointer          data);

/**
 * CtkClipboardTargetsReceivedFunc:
 * @clipboard: the #CtkClipboard
 * @atoms: (nullable) (array length=n_atoms): the supported targets,
 *   as array of #CdkAtom, or %NULL if retrieving the data failed.
 * @n_atoms: the length of the @atoms array.
 * @data: (closure): the @user_data supplied to
 *   ctk_clipboard_request_targets().
 *
 * A function to be called when the results of ctk_clipboard_request_targets()
 * are received, or when the request fails.
 *
 * Since: 2.4
 */
typedef void (* CtkClipboardTargetsReceivedFunc)  (CtkClipboard     *clipboard,
					           CdkAtom          *atoms,
						   gint              n_atoms,
					           gpointer          data);

/* Should these functions have CtkClipboard *clipboard as the first argument?
 * right now for ClearFunc, you may have trouble determining _which_ clipboard
 * was cleared, if you reuse your ClearFunc for multiple clipboards.
 */
/**
 * CtkClipboardGetFunc:
 * @clipboard: the #CtkClipboard
 * @selection_data: a #CtkSelectionData argument in which the requested
 *   data should be stored.
 * @info: the info field corresponding to the requested target from the
 *   #CtkTargetEntry array passed to ctk_clipboard_set_with_data() or
 *   ctk_clipboard_set_with_owner().
 * @user_data_or_owner: the @user_data argument passed to
 *   ctk_clipboard_set_with_data(), or the @owner argument passed to
 *   ctk_clipboard_set_with_owner()
 *
 * A function that will be called to provide the contents of the selection.
 * If multiple types of data were advertised, the requested type can
 * be determined from the @info parameter or by checking the target field
 * of @selection_data. If the data could successfully be converted into
 * then it should be stored into the @selection_data object by
 * calling ctk_selection_data_set() (or related functions such
 * as ctk_selection_data_set_text()). If no data is set, the requestor
 * will be informed that the attempt to get the data failed.
 */
typedef void (* CtkClipboardGetFunc)          (CtkClipboard     *clipboard,
					       CtkSelectionData *selection_data,
					       guint             info,
					       gpointer          user_data_or_owner);

/**
 * CtkClipboardClearFunc:
 * @clipboard: the #CtkClipboard
 * @user_data_or_owner: the @user_data argument passed to ctk_clipboard_set_with_data(),
 *   or the @owner argument passed to ctk_clipboard_set_with_owner()
 *
 * A function that will be called when the contents of the clipboard are changed
 * or cleared. Once this has called, the @user_data_or_owner argument
 * will not be used again.
 */
typedef void (* CtkClipboardClearFunc)        (CtkClipboard     *clipboard,
					       gpointer          user_data_or_owner);

GDK_AVAILABLE_IN_ALL
GType         ctk_clipboard_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkClipboard *ctk_clipboard_get_for_display (CdkDisplay   *display,
					     CdkAtom       selection);
GDK_AVAILABLE_IN_ALL
CtkClipboard *ctk_clipboard_get             (CdkAtom       selection);

GDK_AVAILABLE_IN_3_16
CtkClipboard *ctk_clipboard_get_default     (CdkDisplay    *display);

GDK_AVAILABLE_IN_ALL
CdkDisplay   *ctk_clipboard_get_display     (CtkClipboard *clipboard);


GDK_AVAILABLE_IN_ALL
gboolean ctk_clipboard_set_with_data  (CtkClipboard          *clipboard,
				       const CtkTargetEntry  *targets,
				       guint                  n_targets,
				       CtkClipboardGetFunc    get_func,
				       CtkClipboardClearFunc  clear_func,
				       gpointer               user_data);
GDK_AVAILABLE_IN_ALL
gboolean ctk_clipboard_set_with_owner (CtkClipboard          *clipboard,
				       const CtkTargetEntry  *targets,
				       guint                  n_targets,
				       CtkClipboardGetFunc    get_func,
				       CtkClipboardClearFunc  clear_func,
				       GObject               *owner);
GDK_AVAILABLE_IN_ALL
GObject *ctk_clipboard_get_owner      (CtkClipboard          *clipboard);
GDK_AVAILABLE_IN_ALL
void     ctk_clipboard_clear          (CtkClipboard          *clipboard);
GDK_AVAILABLE_IN_ALL
void     ctk_clipboard_set_text       (CtkClipboard          *clipboard,
				       const gchar           *text,
				       gint                   len);
GDK_AVAILABLE_IN_ALL
void     ctk_clipboard_set_image      (CtkClipboard          *clipboard,
				       CdkPixbuf             *pixbuf);

GDK_AVAILABLE_IN_ALL
void ctk_clipboard_request_contents  (CtkClipboard                     *clipboard,
                                      CdkAtom                           target,
                                      CtkClipboardReceivedFunc          callback,
                                      gpointer                          user_data);
GDK_AVAILABLE_IN_ALL
void ctk_clipboard_request_text      (CtkClipboard                     *clipboard,
                                      CtkClipboardTextReceivedFunc      callback,
                                      gpointer                          user_data);
GDK_AVAILABLE_IN_ALL
void ctk_clipboard_request_rich_text (CtkClipboard                     *clipboard,
                                      CtkTextBuffer                    *buffer,
                                      CtkClipboardRichTextReceivedFunc  callback,
                                      gpointer                          user_data);
GDK_AVAILABLE_IN_ALL
void ctk_clipboard_request_image     (CtkClipboard                     *clipboard,
                                      CtkClipboardImageReceivedFunc     callback,
                                      gpointer                          user_data);
GDK_AVAILABLE_IN_ALL
void ctk_clipboard_request_uris      (CtkClipboard                     *clipboard,
                                      CtkClipboardURIReceivedFunc       callback,
                                      gpointer                          user_data);
GDK_AVAILABLE_IN_ALL
void ctk_clipboard_request_targets   (CtkClipboard                     *clipboard,
                                      CtkClipboardTargetsReceivedFunc   callback,
                                      gpointer                          user_data);

GDK_AVAILABLE_IN_ALL
CtkSelectionData *ctk_clipboard_wait_for_contents  (CtkClipboard  *clipboard,
                                                    CdkAtom        target);
GDK_AVAILABLE_IN_ALL
gchar *           ctk_clipboard_wait_for_text      (CtkClipboard  *clipboard);
GDK_AVAILABLE_IN_ALL
guint8 *          ctk_clipboard_wait_for_rich_text (CtkClipboard  *clipboard,
                                                    CtkTextBuffer *buffer,
                                                    CdkAtom       *format,
                                                    gsize         *length);
GDK_AVAILABLE_IN_ALL
CdkPixbuf *       ctk_clipboard_wait_for_image     (CtkClipboard  *clipboard);
GDK_AVAILABLE_IN_ALL
gchar **          ctk_clipboard_wait_for_uris      (CtkClipboard  *clipboard);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_clipboard_wait_for_targets   (CtkClipboard  *clipboard,
                                                    CdkAtom      **targets,
                                                    gint          *n_targets);

GDK_AVAILABLE_IN_ALL
gboolean ctk_clipboard_wait_is_text_available      (CtkClipboard  *clipboard);
GDK_AVAILABLE_IN_ALL
gboolean ctk_clipboard_wait_is_rich_text_available (CtkClipboard  *clipboard,
                                                    CtkTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
gboolean ctk_clipboard_wait_is_image_available     (CtkClipboard  *clipboard);
GDK_AVAILABLE_IN_ALL
gboolean ctk_clipboard_wait_is_uris_available      (CtkClipboard  *clipboard);
GDK_AVAILABLE_IN_ALL
gboolean ctk_clipboard_wait_is_target_available    (CtkClipboard  *clipboard,
                                                    CdkAtom        target);


GDK_AVAILABLE_IN_ALL
void ctk_clipboard_set_can_store (CtkClipboard         *clipboard,
				  const CtkTargetEntry *targets,
				  gint                  n_targets);

GDK_AVAILABLE_IN_ALL
void ctk_clipboard_store         (CtkClipboard   *clipboard);

GDK_AVAILABLE_IN_3_22
CdkAtom ctk_clipboard_get_selection (CtkClipboard *clipboard);

G_END_DECLS

#endif /* __CTK_CLIPBOARD_H__ */
