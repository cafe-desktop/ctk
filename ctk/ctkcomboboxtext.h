/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2010 Christian Dywan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_COMBO_BOX_TEXT_H__
#define __CTK_COMBO_BOX_TEXT_H__

#if defined(CTK_DISABLE_SINGLE_INCLUDES) && !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcombobox.h>

G_BEGIN_DECLS

#define CTK_TYPE_COMBO_BOX_TEXT                 (ctk_combo_box_text_get_type ())
#define CTK_COMBO_BOX_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COMBO_BOX_TEXT, GtkComboBoxText))
#define CTK_COMBO_BOX_TEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COMBO_BOX_TEXT, GtkComboBoxTextClass))
#define CTK_IS_COMBO_BOX_TEXT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COMBO_BOX_TEXT))
#define CTK_IS_COMBO_BOX_TEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COMBO_BOX_TEXT))
#define CTK_COMBO_BOX_TEXT_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COMBO_BOX_TEXT, GtkComboBoxTextClass))

typedef struct _GtkComboBoxText             GtkComboBoxText;
typedef struct _GtkComboBoxTextPrivate      GtkComboBoxTextPrivate;
typedef struct _GtkComboBoxTextClass        GtkComboBoxTextClass;

struct _GtkComboBoxText
{
  /*< private >*/
  GtkComboBox parent_instance;

  GtkComboBoxTextPrivate *priv;
};

struct _GtkComboBoxTextClass
{
  GtkComboBoxClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType         ctk_combo_box_text_get_type        (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget*    ctk_combo_box_text_new             (void);
GDK_AVAILABLE_IN_ALL
GtkWidget*    ctk_combo_box_text_new_with_entry  (void);
GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_append_text     (GtkComboBoxText     *combo_box,
                                                  const gchar         *text);
GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_insert_text     (GtkComboBoxText     *combo_box,
                                                  gint                 position,
                                                  const gchar         *text);
GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_prepend_text    (GtkComboBoxText     *combo_box,
                                                  const gchar         *text);
GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_remove          (GtkComboBoxText     *combo_box,
                                                  gint                 position);
GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_remove_all      (GtkComboBoxText     *combo_box);
GDK_AVAILABLE_IN_ALL
gchar        *ctk_combo_box_text_get_active_text (GtkComboBoxText     *combo_box);

GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_insert          (GtkComboBoxText     *combo_box,
                                                  gint                 position,
                                                  const gchar         *id,
                                                  const gchar         *text);
GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_append          (GtkComboBoxText     *combo_box,
                                                  const gchar         *id,
                                                  const gchar         *text);
GDK_AVAILABLE_IN_ALL
void          ctk_combo_box_text_prepend         (GtkComboBoxText     *combo_box,
                                                  const gchar         *id,
                                                  const gchar         *text);

G_END_DECLS

#endif /* __CTK_COMBO_BOX_TEXT_H__ */
