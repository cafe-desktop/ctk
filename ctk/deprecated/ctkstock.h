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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_STOCK_H__
#define __CTK_STOCK_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

/**
 * CtkTranslateFunc:
 * @path: The id of the message. In #CtkActionGroup this will be a label
 *   or tooltip from a #CtkActionEntry.
 * @func_data: (closure): user data passed in when registering the
 *   function
 *
 * The function used to translate messages in e.g. #CtkIconFactory
 * and #CtkActionGroup.
 *
 * Returns: the translated message
 *
 * Deprecated: 3.10
 */
typedef gchar * (*CtkTranslateFunc) (const gchar  *path,
                                     gpointer      func_data);

typedef struct _CtkStockItem CtkStockItem;

/**
 * CtkStockItem:
 * @stock_id: Identifier.
 * @label: User visible label.
 * @modifier: Modifier type for keyboard accelerator
 * @keyval: Keyboard accelerator
 * @translation_domain: Translation domain of the menu or toolbar item
 *
 * Deprecated: 3.10
 */
struct _CtkStockItem
{
  gchar *stock_id;
  gchar *label;
  GdkModifierType modifier;
  guint keyval;
  gchar *translation_domain;
};

GDK_DEPRECATED_IN_3_10
void     ctk_stock_add        (const CtkStockItem  *items,
                               guint                n_items);
GDK_DEPRECATED_IN_3_10
void     ctk_stock_add_static (const CtkStockItem  *items,
                               guint                n_items);
GDK_DEPRECATED_IN_3_10
gboolean ctk_stock_lookup     (const gchar         *stock_id,
                               CtkStockItem        *item);

/* Should free the list (and free each string in it also).
 * This function is only useful for GUI builders and such.
 */
GDK_DEPRECATED_IN_3_10
GSList*  ctk_stock_list_ids  (void);

GDK_DEPRECATED_IN_3_10
CtkStockItem *ctk_stock_item_copy (const CtkStockItem *item);
GDK_DEPRECATED_IN_3_10
void          ctk_stock_item_free (CtkStockItem       *item);

GDK_DEPRECATED_IN_3_10
void          ctk_stock_set_translate_func (const gchar      *domain,
					    CtkTranslateFunc  func,
					    gpointer          data,
					    GDestroyNotify    notify);

/* the following type exists just so we can get deprecation warnings */
#ifndef GDK_DISABLE_DEPRECATION_WARNINGS
#if GDK_VERSION_MIN_REQUIRED >= GDK_VERSION_3_10
G_DEPRECATED
#endif
#endif
typedef char * CtkStock;

#ifndef CTK_DISABLE_DEPRECATED

/* Stock IDs (not all are stock items; some are images only) */
/**
 * CTK_STOCK_ABOUT:
 *
 * The “About” item.
 * ![](help-about.png)
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;help-about&quot; or the label &quot;_About&quot;.
 */
#define CTK_STOCK_ABOUT            ((CtkStock)"ctk-about")

/**
 * CTK_STOCK_ADD:
 *
 * The “Add” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;list-add&quot; or the label &quot;_Add&quot;.
 */
#define CTK_STOCK_ADD              ((CtkStock)"ctk-add")

/**
 * CTK_STOCK_APPLY:
 *
 * The “Apply” item and icon.
 *
 * Deprecated: 3.10: Do not use an icon. Use label &quot;_Apply&quot;.
 */
#define CTK_STOCK_APPLY            ((CtkStock)"ctk-apply")

/**
 * CTK_STOCK_BOLD:
 *
 * The “Bold” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-text-bold&quot;.
 */
#define CTK_STOCK_BOLD             ((CtkStock)"ctk-bold")

/**
 * CTK_STOCK_CANCEL:
 *
 * The “Cancel” item and icon.
 *
 * Deprecated: 3.10: Do not use an icon. Use label &quot;_Cancel&quot;.
 */
#define CTK_STOCK_CANCEL           ((CtkStock)"ctk-cancel")

/**
 * CTK_STOCK_CAPS_LOCK_WARNING:
 *
 * The “Caps Lock Warning” icon.
 *
 * Since: 2.16
 *
 * Deprecated: 3.10: Use named icon &quot;dialog-warning-symbolic&quot;.
 */
#define CTK_STOCK_CAPS_LOCK_WARNING ((CtkStock)"ctk-caps-lock-warning")

/**
 * CTK_STOCK_CDROM:
 *
 * The “CD-Rom” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;media-optical&quot;.
 */
#define CTK_STOCK_CDROM            ((CtkStock)"ctk-cdrom")

/**
 * CTK_STOCK_CLEAR:
 *
 * The “Clear” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;edit-clear&quot;.
 */
#define CTK_STOCK_CLEAR            ((CtkStock)"ctk-clear")

/**
 * CTK_STOCK_CLOSE:
 *
 * The “Close” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;window-close&quot; or the label &quot;_Close&quot;.
 */
#define CTK_STOCK_CLOSE            ((CtkStock)"ctk-close")

/**
 * CTK_STOCK_COLOR_PICKER:
 *
 * The “Color Picker” item and icon.
 *
 * Since: 2.2
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_COLOR_PICKER     ((CtkStock)"ctk-color-picker")

/**
 * CTK_STOCK_CONNECT:
 *
 * The “Connect” icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_CONNECT          ((CtkStock)"ctk-connect")

/**
 * CTK_STOCK_CONVERT:
 *
 * The “Convert” item and icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_CONVERT          ((CtkStock)"ctk-convert")

/**
 * CTK_STOCK_COPY:
 *
 * The “Copy” item and icon.
 *
 * Deprecated: 3.10: Use the named icon &quot;edit-copy&quot; or the label &quot;_Copy&quot;.
 */
#define CTK_STOCK_COPY             ((CtkStock)"ctk-copy")

/**
 * CTK_STOCK_CUT:
 *
 * The “Cut” item and icon.
 *
 * Deprecated: 3.10: Use the named icon &quot;edit-cut&quot; or the label &quot;Cu_t&quot;.
 */
#define CTK_STOCK_CUT              ((CtkStock)"ctk-cut")

/**
 * CTK_STOCK_DELETE:
 *
 * The “Delete” item and icon.
 *
 * Deprecated: 3.10: Use the named icon &quot;edit-delete&quot; or the label &quot;_Delete&quot;.
 */
#define CTK_STOCK_DELETE           ((CtkStock)"ctk-delete")

/**
 * CTK_STOCK_DIALOG_AUTHENTICATION:
 *
 * The “Authentication” item and icon.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10: Use named icon &quot;dialog-password&quot;.
 */
#define CTK_STOCK_DIALOG_AUTHENTICATION ((CtkStock)"ctk-dialog-authentication")

/**
 * CTK_STOCK_DIALOG_INFO:
 *
 * The “Information” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;dialog-information&quot;.
 */
#define CTK_STOCK_DIALOG_INFO      ((CtkStock)"ctk-dialog-info")

/**
 * CTK_STOCK_DIALOG_WARNING:
 *
 * The “Warning” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;dialog-warning&quot;.
 */
#define CTK_STOCK_DIALOG_WARNING   ((CtkStock)"ctk-dialog-warning")

/**
 * CTK_STOCK_DIALOG_ERROR:
 *
 * The “Error” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;dialog-error&quot;.
 */
#define CTK_STOCK_DIALOG_ERROR     ((CtkStock)"ctk-dialog-error")

/**
 * CTK_STOCK_DIALOG_QUESTION:
 *
 * The “Question” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;dialog-question&quot;.
 */
#define CTK_STOCK_DIALOG_QUESTION  ((CtkStock)"ctk-dialog-question")

/**
 * CTK_STOCK_DIRECTORY:
 *
 * The “Directory” icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;folder&quot;.
 */
#define CTK_STOCK_DIRECTORY        ((CtkStock)"ctk-directory")

/**
 * CTK_STOCK_DISCARD:
 *
 * The “Discard” item.
 *
 * Since: 2.12
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_DISCARD          ((CtkStock)"ctk-discard")

/**
 * CTK_STOCK_DISCONNECT:
 *
 * The “Disconnect” icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_DISCONNECT       ((CtkStock)"ctk-disconnect")

/**
 * CTK_STOCK_DND:
 *
 * The “Drag-And-Drop” icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_DND              ((CtkStock)"ctk-dnd")

/**
 * CTK_STOCK_DND_MULTIPLE:
 *
 * The “Drag-And-Drop multiple” icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_DND_MULTIPLE     ((CtkStock)"ctk-dnd-multiple")

/**
 * CTK_STOCK_EDIT:
 *
 * The “Edit” item and icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_EDIT             ((CtkStock)"ctk-edit")

/**
 * CTK_STOCK_EXECUTE:
 *
 * The “Execute” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;system-run&quot;.
 */
#define CTK_STOCK_EXECUTE          ((CtkStock)"ctk-execute")

/**
 * CTK_STOCK_FILE:
 *
 * The “File” item and icon.
 *
 * Since 3.0, this item has a label, before it only had an icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;text-x-generic&quot;.
 */
#define CTK_STOCK_FILE             ((CtkStock)"ctk-file")

/**
 * CTK_STOCK_FIND:
 *
 * The “Find” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;edit-find&quot;.
 */
#define CTK_STOCK_FIND             ((CtkStock)"ctk-find")

/**
 * CTK_STOCK_FIND_AND_REPLACE:
 *
 * The “Find and Replace” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;edit-find-replace&quot;.
 */
#define CTK_STOCK_FIND_AND_REPLACE ((CtkStock)"ctk-find-and-replace")

/**
 * CTK_STOCK_FLOPPY:
 *
 * The “Floppy” item and icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_FLOPPY           ((CtkStock)"ctk-floppy")

/**
 * CTK_STOCK_FULLSCREEN:
 *
 * The “Fullscreen” item and icon.
 *
 * Since: 2.8
 *
 * Deprecated: 3.10: Use named icon &quot;view-fullscreen&quot;.
 */
#define CTK_STOCK_FULLSCREEN       ((CtkStock)"ctk-fullscreen")

/**
 * CTK_STOCK_GOTO_BOTTOM:
 *
 * The “Bottom” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;go-bottom&quot;.
 */
#define CTK_STOCK_GOTO_BOTTOM      ((CtkStock)"ctk-goto-bottom")

/**
 * CTK_STOCK_GOTO_FIRST:
 *
 * The “First” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;go-first&quot;.
 */
#define CTK_STOCK_GOTO_FIRST       ((CtkStock)"ctk-goto-first")

/**
 * CTK_STOCK_GOTO_LAST:
 *
 * The “Last” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;go-last&quot;.
 */
#define CTK_STOCK_GOTO_LAST        ((CtkStock)"ctk-goto-last")

/**
 * CTK_STOCK_GOTO_TOP:
 *
 * The “Top” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;go-top&quot;.
 */
#define CTK_STOCK_GOTO_TOP         ((CtkStock)"ctk-goto-top")

/**
 * CTK_STOCK_GO_BACK:
 *
 * The “Back” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;go-previous&quot;.
*/
#define CTK_STOCK_GO_BACK          ((CtkStock)"ctk-go-back")

/**
 * CTK_STOCK_GO_DOWN:
 *
 * The “Down” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;go-down&quot;.
 */
#define CTK_STOCK_GO_DOWN          ((CtkStock)"ctk-go-down")

/**
 * CTK_STOCK_GO_FORWARD:
 *
 * The “Forward” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;go-next&quot;.
 */
#define CTK_STOCK_GO_FORWARD       ((CtkStock)"ctk-go-forward")

/**
 * CTK_STOCK_GO_UP:
 *
 * The “Up” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;go-up&quot;.
 */
#define CTK_STOCK_GO_UP            ((CtkStock)"ctk-go-up")

/**
 * CTK_STOCK_HARDDISK:
 *
 * The “Harddisk” item and icon.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10: Use named icon &quot;drive-harddisk&quot;.
 */
#define CTK_STOCK_HARDDISK         ((CtkStock)"ctk-harddisk")

/**
 * CTK_STOCK_HELP:
 *
 * The “Help” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;help-browser&quot;.
 */
#define CTK_STOCK_HELP             ((CtkStock)"ctk-help")

/**
 * CTK_STOCK_HOME:
 *
 * The “Home” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;go-home&quot;.
 */
#define CTK_STOCK_HOME             ((CtkStock)"ctk-home")

/**
 * CTK_STOCK_INDEX:
 *
 * The “Index” item and icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_INDEX            ((CtkStock)"ctk-index")

/**
 * CTK_STOCK_INDENT:
 *
 * The “Indent” item and icon. The icon has an RTL variant.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10: Use named icon &quot;format-indent-more&quot;.
 */
#define CTK_STOCK_INDENT           ((CtkStock)"ctk-indent")

/**
 * CTK_STOCK_INFO:
 *
 * The “Info” item and icon.
 *
 * Since: 2.8
 *
 * Deprecated: 3.10: Use named icon &quot;dialog-information&quot;.
 */
#define CTK_STOCK_INFO             ((CtkStock)"ctk-info")

/**
 * CTK_STOCK_ITALIC:
 *
 * The “Italic” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-text-italic&quot;.
 */
#define CTK_STOCK_ITALIC           ((CtkStock)"ctk-italic")

/**
 * CTK_STOCK_JUMP_TO:
 *
 * The “Jump to” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;go-jump&quot;.
 */
#define CTK_STOCK_JUMP_TO          ((CtkStock)"ctk-jump-to")

/**
 * CTK_STOCK_JUSTIFY_CENTER:
 *
 * The “Center” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-justify-center&quot;.
 */
#define CTK_STOCK_JUSTIFY_CENTER   ((CtkStock)"ctk-justify-center")

/**
 * CTK_STOCK_JUSTIFY_FILL:
 *
 * The “Fill” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-justify-fill&quot;.
 */
#define CTK_STOCK_JUSTIFY_FILL     ((CtkStock)"ctk-justify-fill")

/**
 * CTK_STOCK_JUSTIFY_LEFT:
 *
 * The “Left” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-justify-left&quot;.
 */
#define CTK_STOCK_JUSTIFY_LEFT     ((CtkStock)"ctk-justify-left")

/**
 * CTK_STOCK_JUSTIFY_RIGHT:
 *
 * The “Right” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-justify-right&quot;.
 */
#define CTK_STOCK_JUSTIFY_RIGHT    ((CtkStock)"ctk-justify-right")

/**
 * CTK_STOCK_LEAVE_FULLSCREEN:
 *
 * The “Leave Fullscreen” item and icon.
 *
 * Since: 2.8
 *
 * Deprecated: 3.10: Use named icon &quot;view-restore&quot;.
 */
#define CTK_STOCK_LEAVE_FULLSCREEN ((CtkStock)"ctk-leave-fullscreen")

/**
 * CTK_STOCK_MISSING_IMAGE:
 *
 * The “Missing image” icon.
 *
 * Deprecated: 3.10: Use named icon &quot;image-missing&quot;.
 */
#define CTK_STOCK_MISSING_IMAGE    ((CtkStock)"ctk-missing-image")

/**
 * CTK_STOCK_MEDIA_FORWARD:
 *
 * The “Media Forward” item and icon. The icon has an RTL variant.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-seek-forward&quot; or the label &quot;_Forward&quot;.
 */
#define CTK_STOCK_MEDIA_FORWARD    ((CtkStock)"ctk-media-forward")

/**
 * CTK_STOCK_MEDIA_NEXT:
 *
 * The “Media Next” item and icon. The icon has an RTL variant.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-skip-forward&quot; or the label &quot;_Next&quot;.
 */
#define CTK_STOCK_MEDIA_NEXT       ((CtkStock)"ctk-media-next")

/**
 * CTK_STOCK_MEDIA_PAUSE:
 *
 * The “Media Pause” item and icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-playback-pause&quot; or the label &quot;P_ause&quot;.
 */
#define CTK_STOCK_MEDIA_PAUSE      ((CtkStock)"ctk-media-pause")

/**
 * CTK_STOCK_MEDIA_PLAY:
 *
 * The “Media Play” item and icon. The icon has an RTL variant.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-playback-start&quot; or the label &quot;_Play&quot;.
 */
#define CTK_STOCK_MEDIA_PLAY       ((CtkStock)"ctk-media-play")

/**
 * CTK_STOCK_MEDIA_PREVIOUS:
 *
 * The “Media Previous” item and icon. The icon has an RTL variant.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-skip-backward&quot; or the label &quot;Pre_vious&quot;.
 */
#define CTK_STOCK_MEDIA_PREVIOUS   ((CtkStock)"ctk-media-previous")

/**
 * CTK_STOCK_MEDIA_RECORD:
 *
 * The “Media Record” item and icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-record&quot; or the label &quot;_Record&quot;.
 */
#define CTK_STOCK_MEDIA_RECORD     ((CtkStock)"ctk-media-record")

/**
 * CTK_STOCK_MEDIA_REWIND:
 *
 * The “Media Rewind” item and icon. The icon has an RTL variant.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-seek-backward&quot; or the label &quot;R_ewind&quot;.
 */
#define CTK_STOCK_MEDIA_REWIND     ((CtkStock)"ctk-media-rewind")

/**
 * CTK_STOCK_MEDIA_STOP:
 *
 * The “Media Stop” item and icon.
 *
 * Since: 2.6
 *
 * Deprecated: 3.10: Use named icon &quot;media-playback-stop&quot; or the label &quot;_Stop&quot;.
 */
#define CTK_STOCK_MEDIA_STOP       ((CtkStock)"ctk-media-stop")

/**
 * CTK_STOCK_NETWORK:
 *
 * The “Network” item and icon.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10: Use named icon &quot;network-workgroup&quot;.
 */
#define CTK_STOCK_NETWORK          ((CtkStock)"ctk-network")

/**
 * CTK_STOCK_NEW:
 *
 * The “New” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;document-new&quot; or the label &quot;_New&quot;.
 */
#define CTK_STOCK_NEW              ((CtkStock)"ctk-new")

/**
 * CTK_STOCK_NO:
 *
 * The “No” item and icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_NO               ((CtkStock)"ctk-no")

/**
 * CTK_STOCK_OK:
 *
 * The “OK” item and icon.
 *
 * Deprecated: 3.10: Do not use an icon. Use label &quot;_OK&quot;.
 */
#define CTK_STOCK_OK               ((CtkStock)"ctk-ok")

/**
 * CTK_STOCK_OPEN:
 *
 * The “Open” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;document-open&quot; or the label &quot;_Open&quot;.
 */
#define CTK_STOCK_OPEN             ((CtkStock)"ctk-open")

/**
 * CTK_STOCK_ORIENTATION_PORTRAIT:
 *
 * The “Portrait Orientation” item and icon.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_ORIENTATION_PORTRAIT ((CtkStock)"ctk-orientation-portrait")

/**
 * CTK_STOCK_ORIENTATION_LANDSCAPE:
 *
 * The “Landscape Orientation” item and icon.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_ORIENTATION_LANDSCAPE ((CtkStock)"ctk-orientation-landscape")

/**
 * CTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE:
 *
 * The “Reverse Landscape Orientation” item and icon.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE ((CtkStock)"ctk-orientation-reverse-landscape")

/**
 * CTK_STOCK_ORIENTATION_REVERSE_PORTRAIT:
 *
 * The “Reverse Portrait Orientation” item and icon.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_ORIENTATION_REVERSE_PORTRAIT ((CtkStock)"ctk-orientation-reverse-portrait")

/**
 * CTK_STOCK_PAGE_SETUP:
 *
 * The “Page Setup” item and icon.
 *
 * Since: 2.14
 *
 * Deprecated: 3.10: Use named icon &quot;document-page-setup&quot; or the label &quot;Page Set_up&quot;.
 */
#define CTK_STOCK_PAGE_SETUP       ((CtkStock)"ctk-page-setup")

/**
 * CTK_STOCK_PASTE:
 *
 * The “Paste” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;edit-paste&quot; or the label &quot;_Paste&quot;.
 */
#define CTK_STOCK_PASTE            ((CtkStock)"ctk-paste")

/**
 * CTK_STOCK_PREFERENCES:
 *
 * The “Preferences” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;preferences-system&quot; or the label &quot;_Preferences&quot;.
 */
#define CTK_STOCK_PREFERENCES      ((CtkStock)"ctk-preferences")

/**
 * CTK_STOCK_PRINT:
 *
 * The “Print” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;document-print&quot; or the label &quot;_Print&quot;.
 */
#define CTK_STOCK_PRINT            ((CtkStock)"ctk-print")

/**
 * CTK_STOCK_PRINT_ERROR:
 *
 * The “Print Error” icon.
 *
 * Since: 2.14
 *
 * Deprecated: 3.10: Use named icon &quot;printer-error&quot;.
 */
#define CTK_STOCK_PRINT_ERROR      ((CtkStock)"ctk-print-error")

/**
 * CTK_STOCK_PRINT_PAUSED:
 *
 * The “Print Paused” icon.
 *
 * Since: 2.14
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_PRINT_PAUSED     ((CtkStock)"ctk-print-paused")

/**
 * CTK_STOCK_PRINT_PREVIEW:
 *
 * The “Print Preview” item and icon.
 *
 * Deprecated: 3.10: Use label &quot;Pre_view&quot;.
 */
#define CTK_STOCK_PRINT_PREVIEW    ((CtkStock)"ctk-print-preview")

/**
 * CTK_STOCK_PRINT_REPORT:
 *
 * The “Print Report” icon.
 *
 * Since: 2.14
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_PRINT_REPORT     ((CtkStock)"ctk-print-report")


/**
 * CTK_STOCK_PRINT_WARNING:
 *
 * The “Print Warning” icon.
 *
 * Since: 2.14
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_PRINT_WARNING    ((CtkStock)"ctk-print-warning")

/**
 * CTK_STOCK_PROPERTIES:
 *
 * The “Properties” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;document-properties&quot; or the label &quot;_Properties&quot;.
 */
#define CTK_STOCK_PROPERTIES       ((CtkStock)"ctk-properties")

/**
 * CTK_STOCK_QUIT:
 *
 * The “Quit” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;application-exit&quot; or the label &quot;_Quit&quot;.
 */
#define CTK_STOCK_QUIT             ((CtkStock)"ctk-quit")

/**
 * CTK_STOCK_REDO:
 *
 * The “Redo” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;edit-redo&quot; or the label &quot;_Redo&quot;.
 */
#define CTK_STOCK_REDO             ((CtkStock)"ctk-redo")

/**
 * CTK_STOCK_REFRESH:
 *
 * The “Refresh” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;view-refresh&quot; or the label &quot;_Refresh&quot;.
 */
#define CTK_STOCK_REFRESH          ((CtkStock)"ctk-refresh")

/**
 * CTK_STOCK_REMOVE:
 *
 * The “Remove” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;list-remove&quot; or the label &quot;_Remove&quot;.
 */
#define CTK_STOCK_REMOVE           ((CtkStock)"ctk-remove")

/**
 * CTK_STOCK_REVERT_TO_SAVED:
 *
 * The “Revert” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;document-revert&quot; or the label &quot;_Revert&quot;.
 */
#define CTK_STOCK_REVERT_TO_SAVED  ((CtkStock)"ctk-revert-to-saved")

/**
 * CTK_STOCK_SAVE:
 *
 * The “Save” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;document-save&quot; or the label &quot;_Save&quot;.
 */
#define CTK_STOCK_SAVE             ((CtkStock)"ctk-save")

/**
 * CTK_STOCK_SAVE_AS:
 *
 * The “Save As” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;document-save-as&quot; or the label &quot;Save _As&quot;.
 */
#define CTK_STOCK_SAVE_AS          ((CtkStock)"ctk-save-as")

/**
 * CTK_STOCK_SELECT_ALL:
 *
 * The “Select All” item and icon.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10: Use named icon &quot;edit-select-all&quot; or the label &quot;Select _All&quot;.
 */
#define CTK_STOCK_SELECT_ALL       ((CtkStock)"ctk-select-all")

/**
 * CTK_STOCK_SELECT_COLOR:
 *
 * The “Color” item and icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_SELECT_COLOR     ((CtkStock)"ctk-select-color")

/**
 * CTK_STOCK_SELECT_FONT:
 *
 * The “Font” item and icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_SELECT_FONT      ((CtkStock)"ctk-select-font")

/**
 * CTK_STOCK_SORT_ASCENDING:
 *
 * The “Ascending” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;view-sort-ascending&quot;.
 */
#define CTK_STOCK_SORT_ASCENDING   ((CtkStock)"ctk-sort-ascending")

/**
 * CTK_STOCK_SORT_DESCENDING:
 *
 * The “Descending” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;view-sort-descending&quot;.
 */
#define CTK_STOCK_SORT_DESCENDING  ((CtkStock)"ctk-sort-descending")

/**
 * CTK_STOCK_SPELL_CHECK:
 *
 * The “Spell Check” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;tools-check-spelling&quot;.
 */
#define CTK_STOCK_SPELL_CHECK      ((CtkStock)"ctk-spell-check")

/**
 * CTK_STOCK_STOP:
 *
 * The “Stop” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;process-stop&quot; or the label &quot;_Stop&quot;.
 */
#define CTK_STOCK_STOP             ((CtkStock)"ctk-stop")

/**
 * CTK_STOCK_STRIKETHROUGH:
 *
 * The “Strikethrough” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-text-strikethrough&quot; or the label &quot;_Strikethrough&quot;.
 */
#define CTK_STOCK_STRIKETHROUGH    ((CtkStock)"ctk-strikethrough")

/**
 * CTK_STOCK_UNDELETE:
 *
 * The “Undelete” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_UNDELETE         ((CtkStock)"ctk-undelete")

/**
 * CTK_STOCK_UNDERLINE:
 *
 * The “Underline” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;format-text-underline&quot; or the label &quot;_Underline&quot;.
 */
#define CTK_STOCK_UNDERLINE        ((CtkStock)"ctk-underline")

/**
 * CTK_STOCK_UNDO:
 *
 * The “Undo” item and icon. The icon has an RTL variant.
 *
 * Deprecated: 3.10: Use named icon &quot;edit-undo&quot; or the label &quot;_Undo&quot;.
 */
#define CTK_STOCK_UNDO             ((CtkStock)"ctk-undo")

/**
 * CTK_STOCK_UNINDENT:
 *
 * The “Unindent” item and icon. The icon has an RTL variant.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10: Use named icon &quot;format-indent-less&quot;.
 */
#define CTK_STOCK_UNINDENT         ((CtkStock)"ctk-unindent")

/**
 * CTK_STOCK_YES:
 *
 * The “Yes” item and icon.
 *
 * Deprecated: 3.10
 */
#define CTK_STOCK_YES              ((CtkStock)"ctk-yes")

/**
 * CTK_STOCK_ZOOM_100:
 *
 * The “Zoom 100%” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;zoom-original&quot; or the label &quot;_Normal Size&quot;.
 */
#define CTK_STOCK_ZOOM_100         ((CtkStock)"ctk-zoom-100")

/**
 * CTK_STOCK_ZOOM_FIT:
 *
 * The “Zoom to Fit” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;zoom-fit-best&quot; or the label &quot;Best _Fit&quot;.
 */
#define CTK_STOCK_ZOOM_FIT         ((CtkStock)"ctk-zoom-fit")

/**
 * CTK_STOCK_ZOOM_IN:
 *
 * The “Zoom In” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;zoom-in&quot; or the label &quot;Zoom _In&quot;.
 */
#define CTK_STOCK_ZOOM_IN          ((CtkStock)"ctk-zoom-in")

/**
 * CTK_STOCK_ZOOM_OUT:
 *
 * The “Zoom Out” item and icon.
 *
 * Deprecated: 3.10: Use named icon &quot;zoom-out&quot; or the label &quot;Zoom _Out&quot;.
 */
#define CTK_STOCK_ZOOM_OUT         ((CtkStock)"ctk-zoom-out")

#endif

G_END_DECLS

#endif /* __CTK_STOCK_H__ */
