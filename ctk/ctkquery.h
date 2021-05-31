/*
 * Copyright (C) 2005 Novell, Inc.
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
 *
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 * Based on nautilus-query.h
 */

#ifndef __CTK_QUERY_H__
#define __CTK_QUERY_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_QUERY          (ctk_query_get_type ())
#define CTK_QUERY(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_QUERY, GtkQuery))
#define CTK_QUERY_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_QUERY, GtkQueryClass))
#define CTK_IS_QUERY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_QUERY))
#define CTK_IS_QUERY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_QUERY))
#define CTK_QUERY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_QUERY, GtkQueryClass))

typedef struct _GtkQuery GtkQuery;
typedef struct _GtkQueryClass GtkQueryClass;
typedef struct _GtkQueryPrivate GtkQueryPrivate;

struct _GtkQuery
{
  GObject parent;

  GtkQueryPrivate *priv;
};

struct _GtkQueryClass
{
  GObjectClass parent_class;
};

GType        ctk_query_get_type       (void);

GtkQuery    *ctk_query_new            (void);

const gchar *ctk_query_get_text       (GtkQuery    *query);
void         ctk_query_set_text       (GtkQuery    *query,
                                       const gchar *text);

GFile       *ctk_query_get_location   (GtkQuery    *query);
void         ctk_query_set_location   (GtkQuery    *query,
                                       GFile       *file);

gboolean     ctk_query_matches_string (GtkQuery    *query,
                                       const gchar *string);

G_END_DECLS

#endif /* __CTK_QUERY_H__ */
