/* ctkplacesview.h
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CTK_PLACES_VIEW_H
#define CTK_PLACES_VIEW_H

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbox.h>
#include <ctk/ctkplacessidebar.h>

G_BEGIN_DECLS

#define CTK_TYPE_PLACES_VIEW        (ctk_places_view_get_type ())
#define CTK_PLACES_VIEW(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PLACES_VIEW, CtkPlacesView))
#define CTK_PLACES_VIEW_CLASS(klass)(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PLACES_VIEW, CtkPlacesViewClass))
#define CTK_IS_PLACES_VIEW(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PLACES_VIEW))
#define CTK_IS_PLACES_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PLACES_VIEW))
#define CTK_PLACES_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PLACES_VIEW, CtkPlacesViewClass))

typedef struct _CtkPlacesView CtkPlacesView;
typedef struct _CtkPlacesViewClass CtkPlacesViewClass;
typedef struct _CtkPlacesViewPrivate CtkPlacesViewPrivate;

struct _CtkPlacesViewClass
{
  CtkBoxClass parent_class;

  void     (* open_location)        (CtkPlacesView          *view,
                                     GFile                  *location,
                                     CtkPlacesOpenFlags  open_flags);

  void    (* show_error_message)     (CtkPlacesSidebar      *sidebar,
                                      const gchar           *primary,
                                      const gchar           *secondary);

  /*< private >*/

  /* Padding for future expansion */
  gpointer reserved[10];
};

struct _CtkPlacesView
{
  CtkBox parent_instance;
};

GType              ctk_places_view_get_type                      (void) G_GNUC_CONST;

CtkPlacesOpenFlags ctk_places_view_get_open_flags                (CtkPlacesView      *view);
void               ctk_places_view_set_open_flags                (CtkPlacesView      *view,
                                                                  CtkPlacesOpenFlags  flags);

const gchar*       ctk_places_view_get_search_query              (CtkPlacesView      *view);
void               ctk_places_view_set_search_query              (CtkPlacesView      *view,
                                                                  const gchar        *query_text);

gboolean           ctk_places_view_get_local_only                (CtkPlacesView         *view);

void               ctk_places_view_set_local_only                (CtkPlacesView         *view,
                                                                  gboolean               local_only);

gboolean           ctk_places_view_get_loading                   (CtkPlacesView         *view);

CtkWidget *        ctk_places_view_new                           (void);

G_END_DECLS

#endif /* CTK_PLACES_VIEW_H */
