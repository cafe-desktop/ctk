/* GtkCustomPaperUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_CUSTOM_PAPER_UNIX_DIALOG_H__
#define __CTK_CUSTOM_PAPER_UNIX_DIALOG_H__

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG                  (ctk_custom_paper_unix_dialog_get_type ())
#define CTK_CUSTOM_PAPER_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG, GtkCustomPaperUnixDialog))
#define CTK_CUSTOM_PAPER_UNIX_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG, GtkCustomPaperUnixDialogClass))
#define CTK_IS_CUSTOM_PAPER_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG))
#define CTK_IS_CUSTOM_PAPER_UNIX_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG))
#define CTK_CUSTOM_PAPER_UNIX_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG, GtkCustomPaperUnixDialogClass))


typedef struct _GtkCustomPaperUnixDialog         GtkCustomPaperUnixDialog;
typedef struct _GtkCustomPaperUnixDialogClass    GtkCustomPaperUnixDialogClass;
typedef struct _GtkCustomPaperUnixDialogPrivate  GtkCustomPaperUnixDialogPrivate;

struct _GtkCustomPaperUnixDialog
{
  GtkDialog parent_instance;

  GtkCustomPaperUnixDialogPrivate *priv;
};

/**
 * GtkCustomPaperUnixDialogClass:
 * @parent_class: The parent class.
 */
struct _GtkCustomPaperUnixDialogClass
{
  GtkDialogClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType             ctk_custom_paper_unix_dialog_get_type           (void) G_GNUC_CONST;
GtkWidget *       _ctk_custom_paper_unix_dialog_new                (GtkWindow   *parent,
								   const gchar *title);
GtkUnit           _ctk_print_get_default_user_units                (void);
void              _ctk_print_load_custom_papers                    (GtkListStore *store);
void              _ctk_print_save_custom_papers                    (GtkListStore *store);
GList *           _ctk_load_custom_papers                          (void);


G_END_DECLS

#endif /* __CTK_CUSTOM_PAPER_UNIX_DIALOG_H__ */
