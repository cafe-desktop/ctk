/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_SELECTION_H__
#define __CTK_SELECTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctktextiter.h>

G_BEGIN_DECLS

typedef struct _CtkTargetPair CtkTargetPair;

/**
 * CtkTargetPair:
 * @target: #CdkAtom representation of the target type
 * @flags: #CtkTargetFlags for DND
 * @info: an application-assigned integer ID which will
 *     get passed as a parameter to e.g the #CtkWidget::selection-get
 *     signal. It allows the application to identify the target
 *     type without extensive string compares.
 *
 * A #CtkTargetPair is used to represent the same
 * information as a table of #CtkTargetEntry, but in
 * an efficient form.
 */
struct _CtkTargetPair
{
  CdkAtom   target;
  guint     flags;
  guint     info;
};

/**
 * CtkTargetList:
 *
 * A #CtkTargetList-struct is a reference counted list
 * of #CtkTargetPair and should be treated as
 * opaque.
 */
typedef struct _CtkTargetList  CtkTargetList;
typedef struct _CtkTargetEntry CtkTargetEntry;

#define CTK_TYPE_SELECTION_DATA (ctk_selection_data_get_type ())
#define CTK_TYPE_TARGET_LIST    (ctk_target_list_get_type ())

/**
 * CtkTargetFlags:
 * @CTK_TARGET_SAME_APP: If this is set, the target will only be selected
 *   for drags within a single application.
 * @CTK_TARGET_SAME_WIDGET: If this is set, the target will only be selected
 *   for drags within a single widget.
 * @CTK_TARGET_OTHER_APP: If this is set, the target will not be selected
 *   for drags within a single application.
 * @CTK_TARGET_OTHER_WIDGET: If this is set, the target will not be selected
 *   for drags withing a single widget.
 *
 * The #CtkTargetFlags enumeration is used to specify
 * constraints on a #CtkTargetEntry.
 */
typedef enum {
  CTK_TARGET_SAME_APP = 1 << 0,    /*< nick=same-app >*/
  CTK_TARGET_SAME_WIDGET = 1 << 1, /*< nick=same-widget >*/
  CTK_TARGET_OTHER_APP = 1 << 2,   /*< nick=other-app >*/
  CTK_TARGET_OTHER_WIDGET = 1 << 3 /*< nick=other-widget >*/
} CtkTargetFlags;

/**
 * CtkTargetEntry:
 * @target: a string representation of the target type
 * @flags: #CtkTargetFlags for DND
 * @info: an application-assigned integer ID which will
 *     get passed as a parameter to e.g the #CtkWidget::selection-get
 *     signal. It allows the application to identify the target
 *     type without extensive string compares.
 *
 * A #CtkTargetEntry represents a single type of
 * data than can be supplied for by a widget for a selection
 * or for supplied or received during drag-and-drop.
 */
struct _CtkTargetEntry
{
  gchar *target;
  guint  flags;
  guint  info;
};

CDK_AVAILABLE_IN_ALL
GType          ctk_target_list_get_type  (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkTargetList *ctk_target_list_new       (const CtkTargetEntry *targets,
                                          guint                 ntargets);
CDK_AVAILABLE_IN_ALL
CtkTargetList *ctk_target_list_ref       (CtkTargetList  *list);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_unref     (CtkTargetList  *list);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_add       (CtkTargetList  *list,
                                          CdkAtom         target,
                                          guint           flags,
                                          guint           info);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_add_text_targets      (CtkTargetList  *list,
                                                      guint           info);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_add_rich_text_targets (CtkTargetList  *list,
                                                      guint           info,
                                                      gboolean        deserializable,
                                                      CtkTextBuffer  *buffer);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_add_image_targets     (CtkTargetList  *list,
                                                      guint           info,
                                                      gboolean        writable);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_add_uri_targets       (CtkTargetList  *list,
                                                      guint           info);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_add_table (CtkTargetList        *list,
                                          const CtkTargetEntry *targets,
                                          guint                 ntargets);
CDK_AVAILABLE_IN_ALL
void           ctk_target_list_remove    (CtkTargetList  *list,
                                          CdkAtom         target);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_target_list_find      (CtkTargetList  *list,
                                          CdkAtom         target,
                                          guint          *info);

CDK_AVAILABLE_IN_ALL
CtkTargetEntry * ctk_target_table_new_from_list (CtkTargetList  *list,
                                                 gint           *n_targets);
CDK_AVAILABLE_IN_ALL
void             ctk_target_table_free          (CtkTargetEntry *targets,
                                                 gint            n_targets);

CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_owner_set             (CtkWidget  *widget,
                                              CdkAtom     selection,
                                              guint32     time_);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_owner_set_for_display (CdkDisplay *display,
                                              CtkWidget  *widget,
                                              CdkAtom     selection,
                                              guint32     time_);

CDK_AVAILABLE_IN_ALL
void     ctk_selection_add_target    (CtkWidget            *widget,
                                      CdkAtom               selection,
                                      CdkAtom               target,
                                      guint                 info);
CDK_AVAILABLE_IN_ALL
void     ctk_selection_add_targets   (CtkWidget            *widget,
                                      CdkAtom               selection,
                                      const CtkTargetEntry *targets,
                                      guint                 ntargets);
CDK_AVAILABLE_IN_ALL
void     ctk_selection_clear_targets (CtkWidget            *widget,
                                      CdkAtom               selection);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_convert       (CtkWidget            *widget,
                                      CdkAtom               selection,
                                      CdkAtom               target,
                                      guint32               time_);
CDK_AVAILABLE_IN_ALL
void     ctk_selection_remove_all    (CtkWidget             *widget);

CDK_AVAILABLE_IN_ALL
CdkAtom       ctk_selection_data_get_selection (const CtkSelectionData *selection_data);
CDK_AVAILABLE_IN_ALL
CdkAtom       ctk_selection_data_get_target    (const CtkSelectionData *selection_data);
CDK_AVAILABLE_IN_ALL
CdkAtom       ctk_selection_data_get_data_type (const CtkSelectionData *selection_data);
CDK_AVAILABLE_IN_ALL
gint          ctk_selection_data_get_format    (const CtkSelectionData *selection_data);
CDK_AVAILABLE_IN_ALL
const guchar *ctk_selection_data_get_data      (const CtkSelectionData *selection_data);
CDK_AVAILABLE_IN_ALL
gint          ctk_selection_data_get_length    (const CtkSelectionData *selection_data);
CDK_AVAILABLE_IN_ALL
const guchar *ctk_selection_data_get_data_with_length
                                               (const CtkSelectionData *selection_data,
                                                gint                   *length);

CDK_AVAILABLE_IN_ALL
CdkDisplay   *ctk_selection_data_get_display   (const CtkSelectionData *selection_data);

CDK_AVAILABLE_IN_ALL
void     ctk_selection_data_set      (CtkSelectionData     *selection_data,
                                      CdkAtom               type,
                                      gint                  format,
                                      const guchar         *data,
                                      gint                  length);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_set_text (CtkSelectionData     *selection_data,
                                      const gchar          *str,
                                      gint                  len);
CDK_AVAILABLE_IN_ALL
guchar * ctk_selection_data_get_text (const CtkSelectionData     *selection_data);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_set_pixbuf   (CtkSelectionData  *selection_data,
                                          CdkPixbuf         *pixbuf);
CDK_AVAILABLE_IN_ALL
CdkPixbuf *ctk_selection_data_get_pixbuf (const CtkSelectionData  *selection_data);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_set_uris (CtkSelectionData     *selection_data,
                                      gchar               **uris);
CDK_AVAILABLE_IN_ALL
gchar  **ctk_selection_data_get_uris (const CtkSelectionData     *selection_data);

CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_get_targets          (const CtkSelectionData  *selection_data,
                                                  CdkAtom          **targets,
                                                  gint              *n_atoms);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_targets_include_text (const CtkSelectionData  *selection_data);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_targets_include_rich_text (const CtkSelectionData *selection_data,
                                                       CtkTextBuffer    *buffer);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_targets_include_image (const CtkSelectionData  *selection_data,
                                                   gboolean           writable);
CDK_AVAILABLE_IN_ALL
gboolean ctk_selection_data_targets_include_uri  (const CtkSelectionData  *selection_data);
CDK_AVAILABLE_IN_ALL
gboolean ctk_targets_include_text                (CdkAtom       *targets,
                                                  gint           n_targets);
CDK_AVAILABLE_IN_ALL
gboolean ctk_targets_include_rich_text           (CdkAtom       *targets,
                                                  gint           n_targets,
                                                  CtkTextBuffer *buffer);
CDK_AVAILABLE_IN_ALL
gboolean ctk_targets_include_image               (CdkAtom       *targets,
                                                  gint           n_targets,
                                                  gboolean       writable);
CDK_AVAILABLE_IN_ALL
gboolean ctk_targets_include_uri                 (CdkAtom       *targets,
                                                  gint           n_targets);


CDK_AVAILABLE_IN_ALL
GType             ctk_selection_data_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkSelectionData *ctk_selection_data_copy     (const CtkSelectionData *data);
CDK_AVAILABLE_IN_ALL
void              ctk_selection_data_free     (CtkSelectionData *data);

CDK_AVAILABLE_IN_ALL
GType             ctk_target_entry_get_type    (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkTargetEntry   *ctk_target_entry_new        (const gchar    *target,
                                               guint           flags,
                                               guint           info);
CDK_AVAILABLE_IN_ALL
CtkTargetEntry   *ctk_target_entry_copy       (CtkTargetEntry *data);
CDK_AVAILABLE_IN_ALL
void              ctk_target_entry_free       (CtkTargetEntry *data);

G_END_DECLS

#endif /* __CTK_SELECTION_H__ */
