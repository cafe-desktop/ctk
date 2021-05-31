/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Copyright (C) 2004-2006 Christian Hammond
 * Copyright (C) 2008 Cody Russell
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef __CTK_ENTRY_H__
#define __CTK_ENTRY_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkeditable.h>
#include <ctk/ctkimcontext.h>
#include <ctk/ctkmenu.h>
#include <ctk/ctkentrybuffer.h>
#include <ctk/ctkentrycompletion.h>
#include <ctk/ctkimage.h>
#include <ctk/ctkselection.h>


G_BEGIN_DECLS

#define CTK_TYPE_ENTRY                  (ctk_entry_get_type ())
#define CTK_ENTRY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ENTRY, CtkEntry))
#define CTK_ENTRY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ENTRY, CtkEntryClass))
#define CTK_IS_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ENTRY))
#define CTK_IS_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ENTRY))
#define CTK_ENTRY_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ENTRY, CtkEntryClass))

/**
 * CtkEntryIconPosition:
 * @CTK_ENTRY_ICON_PRIMARY: At the beginning of the entry (depending on the text direction).
 * @CTK_ENTRY_ICON_SECONDARY: At the end of the entry (depending on the text direction).
 *
 * Specifies the side of the entry at which an icon is placed.
 *
 * Since: 2.16
 */
typedef enum
{
  CTK_ENTRY_ICON_PRIMARY,
  CTK_ENTRY_ICON_SECONDARY
} CtkEntryIconPosition;

typedef struct _CtkEntry              CtkEntry;
typedef struct _CtkEntryPrivate       CtkEntryPrivate;
typedef struct _CtkEntryClass         CtkEntryClass;

struct _CtkEntry
{
  /*< private >*/
  CtkWidget  parent_instance;

  CtkEntryPrivate *priv;
};

/**
 * CtkEntryClass:
 * @parent_class: The parent class.
 * @populate_popup: Class handler for the #CtkEntry::populate-popup signal. If
 *   non-%NULL, this will be called to add additional entries to the context
 *   menu when it is displayed.
 * @activate: Class handler for the #CtkEntry::activate signal. The default
 *   implementation calls ctk_window_activate_default() on the entry’s top-level
 *   window.
 * @move_cursor: Class handler for the #CtkEntry::move-cursor signal. The
 *   default implementation specifies the standard #CtkEntry cursor movement
 *   behavior.
 * @insert_at_cursor: Class handler for the #CtkEntry::insert-at-cursor signal.
 *   The default implementation inserts text at the cursor.
 * @delete_from_cursor: Class handler for the #CtkEntry::delete-from-cursor
 *   signal. The default implementation deletes the selection or the specified
 *   number of characters or words.
 * @backspace: Class handler for the #CtkEntry::backspace signal. The default
 *   implementation deletes the selection or a single character or word.
 * @cut_clipboard: Class handler for the #CtkEntry::cut-clipboard signal. The
 *   default implementation cuts the selection, if one exists.
 * @copy_clipboard: Class handler for the #CtkEntry::copy-clipboard signal. The
 *   default implementation copies the selection, if one exists.
 * @paste_clipboard: Class handler for the #CtkEntry::paste-clipboard signal.
 *   The default implementation pastes at the current cursor position or over
 *   the current selection if one exists.
 * @toggle_overwrite: Class handler for the #CtkEntry::toggle-overwrite signal.
 *   The default implementation toggles overwrite mode and blinks the cursor.
 * @get_text_area_size: Calculate the size of the text area, which is its
 *   allocated width and requested height, minus space for margins and borders.
 *   This virtual function must be non-%NULL.
 * @get_frame_size: Calculate the size of the text area frame, which is its
 *   allocated width and requested height, minus space for margins and borders,
 *   and taking baseline and text height into account. This virtual function
 *   must be non-%NULL.
 *
 * Class structure for #CtkEntry. All virtual functions have a default
 * implementation. Derived classes may set the virtual function pointers for the
 * signal handlers to %NULL, but must keep @get_text_area_size and
 * @get_frame_size non-%NULL; either use the default implementation, or provide
 * a custom one.
 */
struct _CtkEntryClass
{
  CtkWidgetClass parent_class;

  /* Hook to customize right-click popup */
  void (* populate_popup)   (CtkEntry       *entry,
                             CtkWidget      *popup);

  /* Action signals
   */
  void (* activate)           (CtkEntry             *entry);
  void (* move_cursor)        (CtkEntry             *entry,
			       CtkMovementStep       step,
			       gint                  count,
			       gboolean              extend_selection);
  void (* insert_at_cursor)   (CtkEntry             *entry,
			       const gchar          *str);
  void (* delete_from_cursor) (CtkEntry             *entry,
			       CtkDeleteType         type,
			       gint                  count);
  void (* backspace)          (CtkEntry             *entry);
  void (* cut_clipboard)      (CtkEntry             *entry);
  void (* copy_clipboard)     (CtkEntry             *entry);
  void (* paste_clipboard)    (CtkEntry             *entry);
  void (* toggle_overwrite)   (CtkEntry             *entry);

  /* hooks to add other objects beside the entry (like in CtkSpinButton) */
  void (* get_text_area_size) (CtkEntry       *entry,
			       gint           *x,
			       gint           *y,
			       gint           *width,
			       gint           *height);
  void (* get_frame_size)     (CtkEntry       *entry,
                               gint           *x,
                               gint           *y,
			       gint           *width,
			       gint           *height);
  void (* insert_emoji)       (CtkEntry             *entry);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1)      (void);
  void (*_ctk_reserved2)      (void);
  void (*_ctk_reserved3)      (void);
  void (*_ctk_reserved4)      (void);
  void (*_ctk_reserved5)      (void);
  void (*_ctk_reserved6)      (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_entry_get_type       		(void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_entry_new            		(void);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_entry_new_with_buffer            (CtkEntryBuffer *buffer);

GDK_AVAILABLE_IN_ALL
CtkEntryBuffer* ctk_entry_get_buffer            (CtkEntry       *entry);
GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_buffer                 (CtkEntry       *entry,
                                                 CtkEntryBuffer *buffer);

GDK_AVAILABLE_IN_ALL
void       ctk_entry_get_text_area              (CtkEntry       *entry,
                                                 GdkRectangle   *text_area);

GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_visibility 		(CtkEntry      *entry,
						 gboolean       visible);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_entry_get_visibility             (CtkEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_invisible_char         (CtkEntry      *entry,
                                                 gunichar       ch);
GDK_AVAILABLE_IN_ALL
gunichar   ctk_entry_get_invisible_char         (CtkEntry      *entry);
GDK_AVAILABLE_IN_ALL
void       ctk_entry_unset_invisible_char       (CtkEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_has_frame              (CtkEntry      *entry,
                                                 gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_entry_get_has_frame              (CtkEntry      *entry);

GDK_DEPRECATED_IN_3_4
void             ctk_entry_set_inner_border     (CtkEntry        *entry,
                                                 const CtkBorder *border);
GDK_DEPRECATED_IN_3_4
const CtkBorder* ctk_entry_get_inner_border     (CtkEntry        *entry);

GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_overwrite_mode         (CtkEntry      *entry,
                                                 gboolean       overwrite);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_entry_get_overwrite_mode         (CtkEntry      *entry);

/* text is truncated if needed */
GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_max_length 		(CtkEntry      *entry,
						 gint           max);
GDK_AVAILABLE_IN_ALL
gint       ctk_entry_get_max_length             (CtkEntry      *entry);
GDK_AVAILABLE_IN_ALL
guint16    ctk_entry_get_text_length            (CtkEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_activates_default      (CtkEntry      *entry,
                                                 gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_entry_get_activates_default      (CtkEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_width_chars            (CtkEntry      *entry,
                                                 gint           n_chars);
GDK_AVAILABLE_IN_ALL
gint       ctk_entry_get_width_chars            (CtkEntry      *entry);

GDK_AVAILABLE_IN_3_12
void       ctk_entry_set_max_width_chars        (CtkEntry      *entry,
                                                 gint           n_chars);
GDK_AVAILABLE_IN_3_12
gint       ctk_entry_get_max_width_chars        (CtkEntry      *entry);

/* Somewhat more convenient than the CtkEditable generic functions
 */
GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_text                   (CtkEntry      *entry,
                                                 const gchar   *text);
/* returns a reference to the text */
GDK_AVAILABLE_IN_ALL
const gchar* ctk_entry_get_text        (CtkEntry      *entry);

GDK_AVAILABLE_IN_ALL
PangoLayout* ctk_entry_get_layout               (CtkEntry      *entry);
GDK_AVAILABLE_IN_ALL
void         ctk_entry_get_layout_offsets       (CtkEntry      *entry,
                                                 gint          *x,
                                                 gint          *y);
GDK_AVAILABLE_IN_ALL
void       ctk_entry_set_alignment              (CtkEntry      *entry,
                                                 gfloat         xalign);
GDK_AVAILABLE_IN_ALL
gfloat     ctk_entry_get_alignment              (CtkEntry      *entry);

GDK_AVAILABLE_IN_ALL
void                ctk_entry_set_completion (CtkEntry           *entry,
                                              CtkEntryCompletion *completion);
GDK_AVAILABLE_IN_ALL
CtkEntryCompletion *ctk_entry_get_completion (CtkEntry           *entry);

GDK_AVAILABLE_IN_ALL
gint       ctk_entry_layout_index_to_text_index (CtkEntry      *entry,
                                                 gint           layout_index);
GDK_AVAILABLE_IN_ALL
gint       ctk_entry_text_index_to_layout_index (CtkEntry      *entry,
                                                 gint           text_index);

/* For scrolling cursor appropriately
 */
GDK_AVAILABLE_IN_ALL
void           ctk_entry_set_cursor_hadjustment (CtkEntry      *entry,
                                                 CtkAdjustment *adjustment);
GDK_AVAILABLE_IN_ALL
CtkAdjustment* ctk_entry_get_cursor_hadjustment (CtkEntry      *entry);

/* Progress API
 */
GDK_AVAILABLE_IN_ALL
void           ctk_entry_set_progress_fraction   (CtkEntry     *entry,
                                                  gdouble       fraction);
GDK_AVAILABLE_IN_ALL
gdouble        ctk_entry_get_progress_fraction   (CtkEntry     *entry);

GDK_AVAILABLE_IN_ALL
void           ctk_entry_set_progress_pulse_step (CtkEntry     *entry,
                                                  gdouble       fraction);
GDK_AVAILABLE_IN_ALL
gdouble        ctk_entry_get_progress_pulse_step (CtkEntry     *entry);

GDK_AVAILABLE_IN_ALL
void           ctk_entry_progress_pulse          (CtkEntry     *entry);
GDK_AVAILABLE_IN_3_2
const gchar*   ctk_entry_get_placeholder_text    (CtkEntry             *entry);
GDK_AVAILABLE_IN_3_2
void           ctk_entry_set_placeholder_text    (CtkEntry             *entry,
                                                  const gchar          *text);
/* Setting and managing icons
 */
GDK_AVAILABLE_IN_ALL
void           ctk_entry_set_icon_from_pixbuf            (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  GdkPixbuf            *pixbuf);
GDK_DEPRECATED_IN_3_10_FOR(ctk_entry_set_icon_from_icon_name)
void           ctk_entry_set_icon_from_stock             (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  const gchar          *stock_id);
GDK_AVAILABLE_IN_ALL
void           ctk_entry_set_icon_from_icon_name         (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  const gchar          *icon_name);
GDK_AVAILABLE_IN_ALL
void           ctk_entry_set_icon_from_gicon             (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  GIcon                *icon);
GDK_AVAILABLE_IN_ALL
CtkImageType ctk_entry_get_icon_storage_type             (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
GdkPixbuf*   ctk_entry_get_icon_pixbuf                   (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos);
GDK_DEPRECATED_IN_3_10_FOR(ctk_entry_get_icon_name)
const gchar* ctk_entry_get_icon_stock                    (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
const gchar* ctk_entry_get_icon_name                     (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
GIcon*       ctk_entry_get_icon_gicon                    (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         ctk_entry_set_icon_activatable              (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  gboolean              activatable);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_entry_get_icon_activatable              (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         ctk_entry_set_icon_sensitive                (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  gboolean              sensitive);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_entry_get_icon_sensitive                (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
gint         ctk_entry_get_icon_at_pos                   (CtkEntry             *entry,
							  gint                  x,
							  gint                  y);
GDK_AVAILABLE_IN_ALL
void         ctk_entry_set_icon_tooltip_text             (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  const gchar          *tooltip);
GDK_AVAILABLE_IN_ALL
gchar *      ctk_entry_get_icon_tooltip_text             (CtkEntry             *entry,
                                                          CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         ctk_entry_set_icon_tooltip_markup           (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  const gchar          *tooltip);
GDK_AVAILABLE_IN_ALL
gchar *      ctk_entry_get_icon_tooltip_markup           (CtkEntry             *entry,
                                                          CtkEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         ctk_entry_set_icon_drag_source              (CtkEntry             *entry,
							  CtkEntryIconPosition  icon_pos,
							  CtkTargetList        *target_list,
							  GdkDragAction         actions);
GDK_AVAILABLE_IN_ALL
gint         ctk_entry_get_current_icon_drag_source      (CtkEntry             *entry);
GDK_AVAILABLE_IN_ALL
void         ctk_entry_get_icon_area                     (CtkEntry             *entry,
                                                          CtkEntryIconPosition  icon_pos,
                                                          GdkRectangle         *icon_area);

GDK_AVAILABLE_IN_ALL
gboolean    ctk_entry_im_context_filter_keypress         (CtkEntry             *entry,
                                                          GdkEventKey          *event);
GDK_AVAILABLE_IN_ALL
void        ctk_entry_reset_im_context                   (CtkEntry             *entry);

GDK_AVAILABLE_IN_3_6
void            ctk_entry_set_input_purpose                  (CtkEntry             *entry,
                                                              CtkInputPurpose       purpose);
GDK_AVAILABLE_IN_3_6
CtkInputPurpose ctk_entry_get_input_purpose                  (CtkEntry             *entry);

GDK_AVAILABLE_IN_3_6
void            ctk_entry_set_input_hints                    (CtkEntry             *entry,
                                                              CtkInputHints         hints);
GDK_AVAILABLE_IN_3_6
CtkInputHints   ctk_entry_get_input_hints                    (CtkEntry             *entry);

GDK_AVAILABLE_IN_3_6
void            ctk_entry_set_attributes                     (CtkEntry             *entry,
                                                              PangoAttrList        *attrs);
GDK_AVAILABLE_IN_3_6
PangoAttrList  *ctk_entry_get_attributes                     (CtkEntry             *entry);

GDK_AVAILABLE_IN_3_10
void            ctk_entry_set_tabs                           (CtkEntry             *entry,
                                                              PangoTabArray        *tabs);

GDK_AVAILABLE_IN_3_10
PangoTabArray  *ctk_entry_get_tabs                           (CtkEntry             *entry);

GDK_AVAILABLE_IN_3_16
void           ctk_entry_grab_focus_without_selecting        (CtkEntry             *entry);

G_END_DECLS

#endif /* __CTK_ENTRY_H__ */
