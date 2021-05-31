/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 */

#ifndef __CTK_EXPANDER_H__
#define __CTK_EXPANDER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkbin.h>

G_BEGIN_DECLS

#define CTK_TYPE_EXPANDER            (ctk_expander_get_type ())
#define CTK_EXPANDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_EXPANDER, GtkExpander))
#define CTK_EXPANDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_EXPANDER, GtkExpanderClass))
#define CTK_IS_EXPANDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_EXPANDER))
#define CTK_IS_EXPANDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_EXPANDER))
#define CTK_EXPANDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_EXPANDER, GtkExpanderClass))

typedef struct _GtkExpander        GtkExpander;
typedef struct _GtkExpanderClass   GtkExpanderClass;
typedef struct _GtkExpanderPrivate GtkExpanderPrivate;

struct _GtkExpander
{
  GtkBin              bin;

  GtkExpanderPrivate *priv;
};

/**
 * GtkExpanderClass:
 * @parent_class: The parent class.
 * @activate: Keybinding signal is emitted when the user hits the Enter key.
 */
struct _GtkExpanderClass
{
  GtkBinClass    parent_class;

  /*< public >*/

  /* Key binding signal; to get notification on the expansion
   * state connect to notify:expanded.
   */
  void        (* activate) (GtkExpander *expander);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType                 ctk_expander_get_type            (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkWidget            *ctk_expander_new                 (const gchar *label);
GDK_AVAILABLE_IN_ALL
GtkWidget            *ctk_expander_new_with_mnemonic   (const gchar *label);

GDK_AVAILABLE_IN_ALL
void                  ctk_expander_set_expanded        (GtkExpander *expander,
                                                        gboolean     expanded);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_expander_get_expanded        (GtkExpander *expander);

/* Spacing between the expander/label and the child */
GDK_AVAILABLE_IN_ALL
void                  ctk_expander_set_spacing         (GtkExpander *expander,
                                                        gint         spacing);
GDK_AVAILABLE_IN_ALL
gint                  ctk_expander_get_spacing         (GtkExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  ctk_expander_set_label           (GtkExpander *expander,
                                                        const gchar *label);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_expander_get_label           (GtkExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  ctk_expander_set_use_underline   (GtkExpander *expander,
                                                        gboolean     use_underline);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_expander_get_use_underline   (GtkExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  ctk_expander_set_use_markup      (GtkExpander *expander,
                                                        gboolean    use_markup);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_expander_get_use_markup      (GtkExpander *expander);

GDK_AVAILABLE_IN_ALL
void                  ctk_expander_set_label_widget    (GtkExpander *expander,
						        GtkWidget   *label_widget);
GDK_AVAILABLE_IN_ALL
GtkWidget            *ctk_expander_get_label_widget    (GtkExpander *expander);
GDK_AVAILABLE_IN_ALL
void                  ctk_expander_set_label_fill      (GtkExpander *expander,
						        gboolean     label_fill);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_expander_get_label_fill      (GtkExpander *expander);
GDK_AVAILABLE_IN_3_2
void                  ctk_expander_set_resize_toplevel (GtkExpander *expander,
                                                        gboolean     resize_toplevel);
GDK_AVAILABLE_IN_3_2
gboolean              ctk_expander_get_resize_toplevel (GtkExpander *expander);

G_END_DECLS

#endif /* __CTK_EXPANDER_H__ */
