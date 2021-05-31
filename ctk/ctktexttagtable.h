/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TEXT_TAG_TABLE_H__
#define __CTK_TEXT_TAG_TABLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktexttag.h>

G_BEGIN_DECLS

/**
 * CtkTextTagTableForeach:
 * @tag: the #CtkTextTag
 * @data: (closure): data passed to ctk_text_tag_table_foreach()
 */
typedef void (* CtkTextTagTableForeach) (CtkTextTag *tag, gpointer data);

#define CTK_TYPE_TEXT_TAG_TABLE            (ctk_text_tag_table_get_type ())
#define CTK_TEXT_TAG_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TEXT_TAG_TABLE, CtkTextTagTable))
#define CTK_TEXT_TAG_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_TAG_TABLE, CtkTextTagTableClass))
#define CTK_IS_TEXT_TAG_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TEXT_TAG_TABLE))
#define CTK_IS_TEXT_TAG_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_TAG_TABLE))
#define CTK_TEXT_TAG_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_TAG_TABLE, CtkTextTagTableClass))

typedef struct _CtkTextTagTablePrivate       CtkTextTagTablePrivate;
typedef struct _CtkTextTagTableClass         CtkTextTagTableClass;

struct _CtkTextTagTable
{
  GObject parent_instance;

  CtkTextTagTablePrivate *priv;
};

struct _CtkTextTagTableClass
{
  GObjectClass parent_class;

  void (* tag_changed) (CtkTextTagTable *table, CtkTextTag *tag, gboolean size_changed);
  void (* tag_added) (CtkTextTagTable *table, CtkTextTag *tag);
  void (* tag_removed) (CtkTextTagTable *table, CtkTextTag *tag);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType          ctk_text_tag_table_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkTextTagTable *ctk_text_tag_table_new      (void);
GDK_AVAILABLE_IN_ALL
gboolean         ctk_text_tag_table_add      (CtkTextTagTable        *table,
                                              CtkTextTag             *tag);
GDK_AVAILABLE_IN_ALL
void             ctk_text_tag_table_remove   (CtkTextTagTable        *table,
                                              CtkTextTag             *tag);
GDK_AVAILABLE_IN_ALL
CtkTextTag      *ctk_text_tag_table_lookup   (CtkTextTagTable        *table,
                                              const gchar            *name);
GDK_AVAILABLE_IN_ALL
void             ctk_text_tag_table_foreach  (CtkTextTagTable        *table,
                                              CtkTextTagTableForeach  func,
                                              gpointer                data);
GDK_AVAILABLE_IN_ALL
gint             ctk_text_tag_table_get_size (CtkTextTagTable        *table);

G_END_DECLS

#endif

