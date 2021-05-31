/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_ICON_FACTORY_H__
#define __CTK_ICON_FACTORY_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctkenums.h>
#include <ctk/ctktypes.h>

G_BEGIN_DECLS


#define CTK_TYPE_ICON_FACTORY              (ctk_icon_factory_get_type ())
#define CTK_ICON_FACTORY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_ICON_FACTORY, CtkIconFactory))
#define CTK_ICON_FACTORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ICON_FACTORY, CtkIconFactoryClass))
#define CTK_IS_ICON_FACTORY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_ICON_FACTORY))
#define CTK_IS_ICON_FACTORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ICON_FACTORY))
#define CTK_ICON_FACTORY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ICON_FACTORY, CtkIconFactoryClass))
#define CTK_TYPE_ICON_SET                  (ctk_icon_set_get_type ())
#define CTK_TYPE_ICON_SOURCE               (ctk_icon_source_get_type ())

typedef struct _CtkIconFactory              CtkIconFactory;
typedef struct _CtkIconFactoryPrivate       CtkIconFactoryPrivate;
typedef struct _CtkIconFactoryClass         CtkIconFactoryClass;

struct _CtkIconFactory
{
  GObject parent_instance;

  /*< private >*/
  CtkIconFactoryPrivate *priv;
};

/**
 * CtkIconFactoryClass:
 * @parent_class: The parent class.
 */
struct _CtkIconFactoryClass
{
  GObjectClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_DEPRECATED_IN_3_10
GType           ctk_icon_factory_get_type (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_10
CtkIconFactory* ctk_icon_factory_new      (void);
GDK_DEPRECATED_IN_3_10
void            ctk_icon_factory_add      (CtkIconFactory *factory,
                                           const gchar    *stock_id,
                                           CtkIconSet     *icon_set);
GDK_DEPRECATED_IN_3_10
CtkIconSet*     ctk_icon_factory_lookup   (CtkIconFactory *factory,
                                           const gchar    *stock_id);

/* Manage the default icon factory stack */

GDK_DEPRECATED_IN_3_10
void        ctk_icon_factory_add_default     (CtkIconFactory  *factory);
GDK_DEPRECATED_IN_3_10
void        ctk_icon_factory_remove_default  (CtkIconFactory  *factory);
GDK_DEPRECATED_IN_3_10
CtkIconSet* ctk_icon_factory_lookup_default  (const gchar     *stock_id);

/* Get preferred real size from registered semantic size.  Note that
 * themes SHOULD use this size, but they aren’t required to; for size
 * requests and such, you should get the actual pixbuf from the icon
 * set and see what size was rendered.
 *
 * This function is intended for people who are scaling icons,
 * rather than for people who are displaying already-scaled icons.
 * That is, if you are displaying an icon, you should get the
 * size from the rendered pixbuf, not from here.
 */

#ifndef GDK_MULTIHEAD_SAFE
GDK_AVAILABLE_IN_ALL
gboolean ctk_icon_size_lookup              (CtkIconSize  size,
					    gint        *width,
					    gint        *height);
#endif /* GDK_MULTIHEAD_SAFE */
GDK_DEPRECATED_IN_3_10_FOR(ctk_icon_size_lookup)
gboolean ctk_icon_size_lookup_for_settings (CtkSettings *settings,
					    CtkIconSize  size,
					    gint        *width,
					    gint        *height);

GDK_DEPRECATED_IN_3_10
CtkIconSize           ctk_icon_size_register       (const gchar *name,
                                                    gint         width,
                                                    gint         height);
GDK_DEPRECATED_IN_3_10
void                  ctk_icon_size_register_alias (const gchar *alias,
                                                    CtkIconSize  target);
GDK_DEPRECATED_IN_3_10
CtkIconSize           ctk_icon_size_from_name      (const gchar *name);
GDK_DEPRECATED_IN_3_10
const gchar*          ctk_icon_size_get_name       (CtkIconSize  size);

/* Icon sets */

GDK_DEPRECATED_IN_3_10
GType       ctk_icon_set_get_type        (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_10
CtkIconSet* ctk_icon_set_new             (void);
GDK_DEPRECATED_IN_3_10
CtkIconSet* ctk_icon_set_new_from_pixbuf (GdkPixbuf       *pixbuf);

GDK_DEPRECATED_IN_3_10
CtkIconSet* ctk_icon_set_ref             (CtkIconSet      *icon_set);
GDK_DEPRECATED_IN_3_10
void        ctk_icon_set_unref           (CtkIconSet      *icon_set);
GDK_DEPRECATED_IN_3_10
CtkIconSet* ctk_icon_set_copy            (CtkIconSet      *icon_set);

GDK_DEPRECATED_IN_3_0_FOR(ctk_icon_set_render_icon_pixbuf)
GdkPixbuf*  ctk_icon_set_render_icon     (CtkIconSet      *icon_set,
                                          CtkStyle        *style,
                                          CtkTextDirection direction,
                                          CtkStateType     state,
                                          CtkIconSize      size,
                                          CtkWidget       *widget,
                                          const gchar     *detail);

GDK_DEPRECATED_IN_3_10
void           ctk_icon_set_add_source   (CtkIconSet          *icon_set,
                                          const CtkIconSource *source);

GDK_DEPRECATED_IN_3_10
void           ctk_icon_set_get_sizes    (CtkIconSet          *icon_set,
                                          CtkIconSize        **sizes,
                                          gint                *n_sizes);

GDK_DEPRECATED_IN_3_10
GType          ctk_icon_source_get_type                 (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_10
CtkIconSource* ctk_icon_source_new                      (void);
GDK_DEPRECATED_IN_3_10
CtkIconSource* ctk_icon_source_copy                     (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
void           ctk_icon_source_free                     (CtkIconSource       *source);

GDK_DEPRECATED_IN_3_10
void           ctk_icon_source_set_filename             (CtkIconSource       *source,
                                                         const gchar         *filename);
GDK_DEPRECATED_IN_3_10
void           ctk_icon_source_set_icon_name            (CtkIconSource       *source,
                                                         const gchar         *icon_name);
GDK_DEPRECATED_IN_3_10
void           ctk_icon_source_set_pixbuf               (CtkIconSource       *source,
                                                         GdkPixbuf           *pixbuf);

GDK_DEPRECATED_IN_3_10
const gchar *    ctk_icon_source_get_filename             (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
const gchar *    ctk_icon_source_get_icon_name            (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
GdkPixbuf*       ctk_icon_source_get_pixbuf               (const CtkIconSource *source);

GDK_DEPRECATED_IN_3_10
void             ctk_icon_source_set_direction_wildcarded (CtkIconSource       *source,
                                                           gboolean             setting);
GDK_DEPRECATED_IN_3_10
void             ctk_icon_source_set_state_wildcarded     (CtkIconSource       *source,
                                                           gboolean             setting);
GDK_DEPRECATED_IN_3_10
void             ctk_icon_source_set_size_wildcarded      (CtkIconSource       *source,
                                                           gboolean             setting);
GDK_DEPRECATED_IN_3_10
gboolean         ctk_icon_source_get_size_wildcarded      (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
gboolean         ctk_icon_source_get_state_wildcarded     (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
gboolean         ctk_icon_source_get_direction_wildcarded (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
void             ctk_icon_source_set_direction            (CtkIconSource       *source,
                                                           CtkTextDirection     direction);
GDK_DEPRECATED_IN_3_10
void             ctk_icon_source_set_state                (CtkIconSource       *source,
                                                           CtkStateType         state);
GDK_DEPRECATED_IN_3_10
void             ctk_icon_source_set_size                 (CtkIconSource       *source,
                                                           CtkIconSize          size);
GDK_DEPRECATED_IN_3_10
CtkTextDirection ctk_icon_source_get_direction            (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
CtkStateType     ctk_icon_source_get_state                (const CtkIconSource *source);
GDK_DEPRECATED_IN_3_10
CtkIconSize      ctk_icon_source_get_size                 (const CtkIconSource *source);

G_END_DECLS

#endif /* __CTK_ICON_FACTORY_H__ */
