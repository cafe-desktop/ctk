/* CTK - The GIMP Toolkit

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>
   Copyright (C) 2003, 2004 Matthias Clasen <mclasen@redhat.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library. If not, see <http://www.gnu.org/licenses/>.

   Author: Anders Carlsson <andersca@codefactory.se>
*/

#ifndef __CTK_ABOUT_DIALOG_H__
#define __CTK_ABOUT_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>

G_BEGIN_DECLS

#define CTK_TYPE_ABOUT_DIALOG            (ctk_about_dialog_get_type ())
#define CTK_ABOUT_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_ABOUT_DIALOG, CtkAboutDialog))
#define CTK_ABOUT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ABOUT_DIALOG, CtkAboutDialogClass))
#define CTK_IS_ABOUT_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_ABOUT_DIALOG))
#define CTK_IS_ABOUT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ABOUT_DIALOG))
#define CTK_ABOUT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ABOUT_DIALOG, CtkAboutDialogClass))

typedef struct _CtkAboutDialog        CtkAboutDialog;
typedef struct _CtkAboutDialogClass   CtkAboutDialogClass;
typedef struct _CtkAboutDialogPrivate CtkAboutDialogPrivate;

/**
 * CtkLicense:
 * @CTK_LICENSE_UNKNOWN: No license specified
 * @CTK_LICENSE_CUSTOM: A license text is going to be specified by the
 *   developer
 * @CTK_LICENSE_GPL_2_0: The GNU General Public License, version 2.0 or later
 * @CTK_LICENSE_GPL_3_0: The GNU General Public License, version 3.0 or later
 * @CTK_LICENSE_LGPL_2_1: The GNU Lesser General Public License, version 2.1 or later
 * @CTK_LICENSE_LGPL_3_0: The GNU Lesser General Public License, version 3.0 or later
 * @CTK_LICENSE_BSD: The BSD standard license
 * @CTK_LICENSE_MIT_X11: The MIT/X11 standard license
 * @CTK_LICENSE_ARTISTIC: The Artistic License, version 2.0
 * @CTK_LICENSE_GPL_2_0_ONLY: The GNU General Public License, version 2.0 only. Since 3.12.
 * @CTK_LICENSE_GPL_3_0_ONLY: The GNU General Public License, version 3.0 only. Since 3.12.
 * @CTK_LICENSE_LGPL_2_1_ONLY: The GNU Lesser General Public License, version 2.1 only. Since 3.12.
 * @CTK_LICENSE_LGPL_3_0_ONLY: The GNU Lesser General Public License, version 3.0 only. Since 3.12.
 * @CTK_LICENSE_AGPL_3_0: The GNU Affero General Public License, version 3.0 or later. Since: 3.22.
 * @CTK_LICENSE_AGPL_3_0_ONLY: The GNU Affero General Public License, version 3.0 only. Since: 3.22.27.
 * @CTK_LICENSE_BSD_3: The 3-clause BSD licence. Since: 3.24.20.
 * @CTK_LICENSE_APACHE_2_0: The Apache License, version 2.0. Since: 3.24.20.
 * @CTK_LICENSE_MPL_2_0: The Mozilla Public License, version 2.0. Since: 3.24.20.
 *
 * The type of license for an application.
 *
 * This enumeration can be expanded at later date.
 *
 * Since: 3.0
 */
typedef enum {
  CTK_LICENSE_UNKNOWN,
  CTK_LICENSE_CUSTOM,

  CTK_LICENSE_GPL_2_0,
  CTK_LICENSE_GPL_3_0,

  CTK_LICENSE_LGPL_2_1,
  CTK_LICENSE_LGPL_3_0,

  CTK_LICENSE_BSD,
  CTK_LICENSE_MIT_X11,

  CTK_LICENSE_ARTISTIC,

  CTK_LICENSE_GPL_2_0_ONLY,
  CTK_LICENSE_GPL_3_0_ONLY,
  CTK_LICENSE_LGPL_2_1_ONLY,
  CTK_LICENSE_LGPL_3_0_ONLY,

  CTK_LICENSE_AGPL_3_0,
  CTK_LICENSE_AGPL_3_0_ONLY,

  CTK_LICENSE_BSD_3,
  CTK_LICENSE_APACHE_2_0,
  CTK_LICENSE_MPL_2_0
} CtkLicense;

/**
 * CtkAboutDialog:
 *
 * The #CtkAboutDialog-struct contains
 * only private fields and should not be directly accessed.
 */
struct _CtkAboutDialog
{
  CtkDialog parent_instance;

  /*< private >*/
  CtkAboutDialogPrivate *priv;
};

struct _CtkAboutDialogClass
{
  CtkDialogClass parent_class;

  gboolean (*activate_link) (CtkAboutDialog *dialog,
                             const gchar    *uri);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType                  ctk_about_dialog_get_type               (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget             *ctk_about_dialog_new                    (void);
CDK_AVAILABLE_IN_ALL
void                   ctk_show_about_dialog                   (CtkWindow       *parent,
                                                                const gchar     *first_property_name,
                                                                ...) G_GNUC_NULL_TERMINATED;
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_program_name       (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_program_name       (CtkAboutDialog  *about,
                                                                const gchar     *name);
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_version            (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_version            (CtkAboutDialog  *about,
                                                                const gchar     *version);
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_copyright          (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_copyright          (CtkAboutDialog  *about,
                                                                const gchar     *copyright);
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_comments           (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_comments           (CtkAboutDialog  *about,
                                                                const gchar     *comments);
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_license            (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_license            (CtkAboutDialog  *about,
                                                                const gchar     *license);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_license_type       (CtkAboutDialog  *about,
                                                                CtkLicense       license_type);
CDK_AVAILABLE_IN_ALL
CtkLicense             ctk_about_dialog_get_license_type       (CtkAboutDialog  *about);

CDK_AVAILABLE_IN_ALL
gboolean               ctk_about_dialog_get_wrap_license       (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_wrap_license       (CtkAboutDialog  *about,
                                                                gboolean         wrap_license);

CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_website            (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_website            (CtkAboutDialog  *about,
                                                                const gchar     *website);
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_website_label      (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_website_label      (CtkAboutDialog  *about,
                                                                const gchar     *website_label);
CDK_AVAILABLE_IN_ALL
const gchar* const *   ctk_about_dialog_get_authors            (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_authors            (CtkAboutDialog  *about,
                                                                const gchar    **authors);
CDK_AVAILABLE_IN_ALL
const gchar* const *   ctk_about_dialog_get_documenters        (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_documenters        (CtkAboutDialog  *about,
                                                                const gchar    **documenters);
CDK_AVAILABLE_IN_ALL
const gchar* const *   ctk_about_dialog_get_artists            (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_artists            (CtkAboutDialog  *about,
                                                                const gchar    **artists);
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_translator_credits (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_translator_credits (CtkAboutDialog  *about,
                                                                const gchar     *translator_credits);
CDK_AVAILABLE_IN_ALL
GdkPixbuf             *ctk_about_dialog_get_logo               (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_logo               (CtkAboutDialog  *about,
                                                                GdkPixbuf       *logo);
CDK_AVAILABLE_IN_ALL
const gchar *          ctk_about_dialog_get_logo_icon_name     (CtkAboutDialog  *about);
CDK_AVAILABLE_IN_ALL
void                   ctk_about_dialog_set_logo_icon_name     (CtkAboutDialog  *about,
                                                                const gchar     *icon_name);
CDK_AVAILABLE_IN_3_4
void                  ctk_about_dialog_add_credit_section      (CtkAboutDialog  *about,
                                                                const gchar     *section_name,
                                                                const gchar    **people);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkAboutDialog, g_object_unref)

G_END_DECLS

#endif /* __CTK_ABOUT_DIALOG_H__ */


