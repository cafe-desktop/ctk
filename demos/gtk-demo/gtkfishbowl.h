/* GTK - The GIMP Toolkit
 * Copyright (C) 2017 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_FISHBOWL_H__
#define __CTK_FISHBOWL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CTK_TYPE_FISHBOWL                  (ctk_fishbowl_get_type ())
#define CTK_FISHBOWL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FISHBOWL, GtkFishbowl))
#define CTK_FISHBOWL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FISHBOWL, GtkFishbowlClass))
#define CTK_IS_FISHBOWL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FISHBOWL))
#define CTK_IS_FISHBOWL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FISHBOWL))
#define CTK_FISHBOWL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FISHBOWL, GtkFishbowlClass))

typedef struct _GtkFishbowl              GtkFishbowl;
typedef struct _GtkFishbowlClass         GtkFishbowlClass;

typedef GtkWidget * (* GtkFishCreationFunc) (void);

struct _GtkFishbowl
{
  GtkContainer parent;
};

struct _GtkFishbowlClass
{
  GtkContainerClass parent_class;
};

GType      ctk_fishbowl_get_type          (void) G_GNUC_CONST;

GtkWidget* ctk_fishbowl_new               (void);

guint      ctk_fishbowl_get_count         (GtkFishbowl       *fishbowl);
void       ctk_fishbowl_set_count         (GtkFishbowl       *fishbowl,
                                           guint              count);
gboolean   ctk_fishbowl_get_animating     (GtkFishbowl       *fishbowl);
void       ctk_fishbowl_set_animating     (GtkFishbowl       *fishbowl,
                                           gboolean           animating);
gboolean   ctk_fishbowl_get_benchmark     (GtkFishbowl       *fishbowl);
void       ctk_fishbowl_set_benchmark     (GtkFishbowl       *fishbowl,
                                           gboolean           animating);
double     ctk_fishbowl_get_framerate     (GtkFishbowl       *fishbowl);
gint64     ctk_fishbowl_get_update_delay  (GtkFishbowl       *fishbowl);
void       ctk_fishbowl_set_update_delay  (GtkFishbowl       *fishbowl,
                                           gint64             update_delay);
void       ctk_fishbowl_set_creation_func (GtkFishbowl       *fishbowl,
                                           GtkFishCreationFunc creation_func);

G_END_DECLS

#endif /* __CTK_FISHBOWL_H__ */
