/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * CtkFontSelection widget for Ctk+, by Damon Chaplin, May 1998.
 * Based on the GnomeFontSelector widget, by Elliot Lee, but major changes.
 * The GnomeFontSelector was derived from app/text_tool.c in the GIMP.
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_FONTSEL_H__
#define __CTK_FONTSEL_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>
#include <ctk/ctkbox.h>


G_BEGIN_DECLS

#define CTK_TYPE_FONT_SELECTION              (ctk_font_selection_get_type ())
#define CTK_FONT_SELECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FONT_SELECTION, CtkFontSelection))
#define CTK_FONT_SELECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FONT_SELECTION, CtkFontSelectionClass))
#define CTK_IS_FONT_SELECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FONT_SELECTION))
#define CTK_IS_FONT_SELECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FONT_SELECTION))
#define CTK_FONT_SELECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FONT_SELECTION, CtkFontSelectionClass))


#define CTK_TYPE_FONT_SELECTION_DIALOG              (ctk_font_selection_dialog_get_type ())
#define CTK_FONT_SELECTION_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FONT_SELECTION_DIALOG, CtkFontSelectionDialog))
#define CTK_FONT_SELECTION_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FONT_SELECTION_DIALOG, CtkFontSelectionDialogClass))
#define CTK_IS_FONT_SELECTION_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FONT_SELECTION_DIALOG))
#define CTK_IS_FONT_SELECTION_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FONT_SELECTION_DIALOG))
#define CTK_FONT_SELECTION_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FONT_SELECTION_DIALOG, CtkFontSelectionDialogClass))


typedef struct _CtkFontSelection              CtkFontSelection;
typedef struct _CtkFontSelectionPrivate       CtkFontSelectionPrivate;
typedef struct _CtkFontSelectionClass         CtkFontSelectionClass;

typedef struct _CtkFontSelectionDialog              CtkFontSelectionDialog;
typedef struct _CtkFontSelectionDialogPrivate       CtkFontSelectionDialogPrivate;
typedef struct _CtkFontSelectionDialogClass         CtkFontSelectionDialogClass;

struct _CtkFontSelection
{
  CtkBox parent_instance;

  /*< private >*/
  CtkFontSelectionPrivate *priv;
};

struct _CtkFontSelectionClass
{
  CtkBoxClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


struct _CtkFontSelectionDialog
{
  CtkDialog parent_instance;

  /*< private >*/
  CtkFontSelectionDialogPrivate *priv;
};

struct _CtkFontSelectionDialogClass
{
  CtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_DEPRECATED_IN_3_2
GType        ctk_font_selection_get_type          (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *  ctk_font_selection_new               (void);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *  ctk_font_selection_get_family_list   (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *  ctk_font_selection_get_face_list     (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *  ctk_font_selection_get_size_entry    (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *  ctk_font_selection_get_size_list     (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *  ctk_font_selection_get_preview_entry (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
PangoFontFamily *
             ctk_font_selection_get_family        (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
PangoFontFace *
             ctk_font_selection_get_face          (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
gint         ctk_font_selection_get_size          (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
gchar*       ctk_font_selection_get_font_name     (CtkFontSelection *fontsel);

GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
gboolean     ctk_font_selection_set_font_name     (CtkFontSelection *fontsel,
                                                   const gchar      *fontname);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
const gchar* ctk_font_selection_get_preview_text  (CtkFontSelection *fontsel);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
void         ctk_font_selection_set_preview_text  (CtkFontSelection *fontsel,
                                                   const gchar      *text);

GDK_DEPRECATED_IN_3_2
GType      ctk_font_selection_dialog_get_type          (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *ctk_font_selection_dialog_new               (const gchar            *title);

GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *ctk_font_selection_dialog_get_ok_button     (CtkFontSelectionDialog *fsd);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *ctk_font_selection_dialog_get_cancel_button (CtkFontSelectionDialog *fsd);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
CtkWidget *ctk_font_selection_dialog_get_font_selection (CtkFontSelectionDialog *fsd);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
gchar*     ctk_font_selection_dialog_get_font_name     (CtkFontSelectionDialog *fsd);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
gboolean   ctk_font_selection_dialog_set_font_name     (CtkFontSelectionDialog *fsd,
                                                        const gchar            *fontname);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
const gchar*
          ctk_font_selection_dialog_get_preview_text   (CtkFontSelectionDialog *fsd);
GDK_DEPRECATED_IN_3_2_FOR(CtkFontChooser)
void      ctk_font_selection_dialog_set_preview_text   (CtkFontSelectionDialog *fsd,
                                                        const gchar            *text);

G_END_DECLS


#endif /* __CTK_FONTSEL_H__ */
