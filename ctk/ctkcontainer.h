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

#ifndef __CTK_CONTAINER_H__
#define __CTK_CONTAINER_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_CONTAINER              (ctk_container_get_type ())
#define CTK_CONTAINER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CONTAINER, GtkContainer))
#define CTK_CONTAINER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CONTAINER, GtkContainerClass))
#define CTK_IS_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CONTAINER))
#define CTK_IS_CONTAINER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CONTAINER))
#define CTK_CONTAINER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CONTAINER, GtkContainerClass))


typedef struct _GtkContainer              GtkContainer;
typedef struct _GtkContainerPrivate       GtkContainerPrivate;
typedef struct _GtkContainerClass         GtkContainerClass;

struct _GtkContainer
{
  GtkWidget widget;

  /*< private >*/
  GtkContainerPrivate *priv;
};

/**
 * GtkContainerClass:
 * @parent_class: The parent class.
 * @add: Signal emitted when a widget is added to container.
 * @remove: Signal emitted when a widget is removed from container.
 * @check_resize: Signal emitted when a size recalculation is needed.
 * @forall: Invokes callback on each child of container. The callback handler
 *    may remove the child.
 * @set_focus_child: Sets the focused child of container.
 * @child_type: Returns the type of the children supported by the container.
 * @composite_name: Gets a widget’s composite name. Deprecated: 3.10.
 * @set_child_property: Set a property on a child of container.
 * @get_child_property: Get a property from a child of container.
 * @get_path_for_child: Get path representing entire widget hierarchy
 *    from the toplevel down to and including @child.
 *
 * Base class for containers.
 */
struct _GtkContainerClass
{
  GtkWidgetClass parent_class;

  /*< public >*/

  void    (*add)       		(GtkContainer	 *container,
				 GtkWidget	 *widget);
  void    (*remove)    		(GtkContainer	 *container,
				 GtkWidget	 *widget);
  void    (*check_resize)	(GtkContainer	 *container);
  void    (*forall)    		(GtkContainer	 *container,
				 gboolean	  include_internals,
				 GtkCallback	  callback,
				 gpointer	  callback_data);
  void    (*set_focus_child)	(GtkContainer	 *container,
				 GtkWidget	 *child);
  GType   (*child_type)		(GtkContainer	 *container);
  gchar*  (*composite_name)	(GtkContainer	 *container,
				 GtkWidget	 *child);
  void    (*set_child_property) (GtkContainer    *container,
				 GtkWidget       *child,
				 guint            property_id,
				 const GValue    *value,
				 GParamSpec      *pspec);
  void    (*get_child_property) (GtkContainer    *container,
                                 GtkWidget       *child,
				 guint            property_id,
				 GValue          *value,
				 GParamSpec      *pspec);
  GtkWidgetPath * (*get_path_for_child) (GtkContainer *container,
                                         GtkWidget    *child);


  /*< private >*/

  unsigned int _handle_border_width : 1;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};


/**
 * GtkResizeMode:
 * @CTK_RESIZE_PARENT: Pass resize request to the parent
 * @CTK_RESIZE_QUEUE: Queue resizes on this widget
 * @CTK_RESIZE_IMMEDIATE: Resize immediately. Deprecated.
 */
typedef enum
{
  CTK_RESIZE_PARENT,
  CTK_RESIZE_QUEUE,
  CTK_RESIZE_IMMEDIATE
} GtkResizeMode;


/* Application-level methods */

GDK_AVAILABLE_IN_ALL
GType   ctk_container_get_type		 (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
void    ctk_container_set_border_width	 (GtkContainer	   *container,
					  guint		    border_width);
GDK_AVAILABLE_IN_ALL
guint   ctk_container_get_border_width   (GtkContainer     *container);
GDK_AVAILABLE_IN_ALL
void    ctk_container_add		 (GtkContainer	   *container,
					  GtkWidget	   *widget);
GDK_AVAILABLE_IN_ALL
void    ctk_container_remove		 (GtkContainer	   *container,
					  GtkWidget	   *widget);

GDK_DEPRECATED_IN_3_12
void    ctk_container_set_resize_mode    (GtkContainer     *container,
					  GtkResizeMode     resize_mode);
GDK_DEPRECATED_IN_3_12
GtkResizeMode ctk_container_get_resize_mode (GtkContainer     *container);

GDK_AVAILABLE_IN_ALL
void    ctk_container_check_resize       (GtkContainer     *container);

GDK_AVAILABLE_IN_ALL
void     ctk_container_foreach      (GtkContainer       *container,
				     GtkCallback         callback,
				     gpointer            callback_data);
GDK_AVAILABLE_IN_ALL
GList*   ctk_container_get_children     (GtkContainer       *container);

GDK_AVAILABLE_IN_ALL
void     ctk_container_propagate_draw   (GtkContainer   *container,
					 GtkWidget      *child,
					 cairo_t        *cr);

GDK_DEPRECATED_IN_3_24
void     ctk_container_set_focus_chain  (GtkContainer   *container,
                                         GList          *focusable_widgets);
GDK_DEPRECATED_IN_3_24
gboolean ctk_container_get_focus_chain  (GtkContainer   *container,
					 GList         **focusable_widgets);
GDK_DEPRECATED_IN_3_24
void     ctk_container_unset_focus_chain (GtkContainer  *container);

#define CTK_IS_RESIZE_CONTAINER(widget) (CTK_IS_CONTAINER (widget) && \
                                        (ctk_container_get_resize_mode (CTK_CONTAINER (widget)) != CTK_RESIZE_PARENT))

/* Widget-level methods */

GDK_DEPRECATED_IN_3_14
void   ctk_container_set_reallocate_redraws (GtkContainer    *container,
					     gboolean         needs_redraws);
GDK_AVAILABLE_IN_ALL
void   ctk_container_set_focus_child	   (GtkContainer     *container,
					    GtkWidget	     *child);
GDK_AVAILABLE_IN_ALL
GtkWidget *
       ctk_container_get_focus_child	   (GtkContainer     *container);
GDK_AVAILABLE_IN_ALL
void   ctk_container_set_focus_vadjustment (GtkContainer     *container,
					    GtkAdjustment    *adjustment);
GDK_AVAILABLE_IN_ALL
GtkAdjustment *ctk_container_get_focus_vadjustment (GtkContainer *container);
GDK_AVAILABLE_IN_ALL
void   ctk_container_set_focus_hadjustment (GtkContainer     *container,
					    GtkAdjustment    *adjustment);
GDK_AVAILABLE_IN_ALL
GtkAdjustment *ctk_container_get_focus_hadjustment (GtkContainer *container);

GDK_DEPRECATED_IN_3_10
void    ctk_container_resize_children      (GtkContainer     *container);

GDK_AVAILABLE_IN_ALL
GType   ctk_container_child_type	   (GtkContainer     *container);


GDK_AVAILABLE_IN_ALL
void         ctk_container_class_install_child_property (GtkContainerClass *cclass,
							 guint		    property_id,
							 GParamSpec	   *pspec);
GDK_AVAILABLE_IN_3_18
void         ctk_container_class_install_child_properties (GtkContainerClass *cclass,
                                                           guint              n_pspecs,
                                                           GParamSpec       **pspecs);
GDK_AVAILABLE_IN_ALL
GParamSpec*  ctk_container_class_find_child_property	(GObjectClass	   *cclass,
							 const gchar	   *property_name);
GDK_AVAILABLE_IN_ALL
GParamSpec** ctk_container_class_list_child_properties	(GObjectClass	   *cclass,
							 guint		   *n_properties);
GDK_AVAILABLE_IN_ALL
void         ctk_container_add_with_properties		(GtkContainer	   *container,
							 GtkWidget	   *widget,
							 const gchar	   *first_prop_name,
							 ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void         ctk_container_child_set			(GtkContainer	   *container,
							 GtkWidget	   *child,
							 const gchar	   *first_prop_name,
							 ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void         ctk_container_child_get			(GtkContainer	   *container,
							 GtkWidget	   *child,
							 const gchar	   *first_prop_name,
							 ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void         ctk_container_child_set_valist		(GtkContainer	   *container,
							 GtkWidget	   *child,
							 const gchar	   *first_property_name,
							 va_list	    var_args);
GDK_AVAILABLE_IN_ALL
void         ctk_container_child_get_valist		(GtkContainer	   *container,
							 GtkWidget	   *child,
							 const gchar	   *first_property_name,
							 va_list	    var_args);
GDK_AVAILABLE_IN_ALL
void	     ctk_container_child_set_property		(GtkContainer	   *container,
							 GtkWidget	   *child,
							 const gchar	   *property_name,
							 const GValue	   *value);
GDK_AVAILABLE_IN_ALL
void	     ctk_container_child_get_property		(GtkContainer	   *container,
							 GtkWidget	   *child,
							 const gchar	   *property_name,
	                                                 GValue		   *value);

GDK_AVAILABLE_IN_3_2
void ctk_container_child_notify (GtkContainer *container,
                                 GtkWidget    *child,
                                 const gchar  *child_property);

GDK_AVAILABLE_IN_3_18
void ctk_container_child_notify_by_pspec (GtkContainer *container,
                                          GtkWidget    *child,
                                          GParamSpec   *pspec);

/**
 * CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID:
 * @object: the #GObject on which set_child_property() or get_child_property()
 *  was called
 * @property_id: the numeric id of the property
 * @pspec: the #GParamSpec of the property
 *
 * This macro should be used to emit a standard warning about unexpected
 * properties in set_child_property() and get_child_property() implementations.
 */
#define CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID(object, property_id, pspec) \
    G_OBJECT_WARN_INVALID_PSPEC ((object), "child property", (property_id), (pspec))


GDK_AVAILABLE_IN_ALL
void    ctk_container_forall		     (GtkContainer *container,
					      GtkCallback   callback,
					      gpointer	    callback_data);

GDK_AVAILABLE_IN_ALL
void    ctk_container_class_handle_border_width (GtkContainerClass *klass);

GDK_AVAILABLE_IN_ALL
GtkWidgetPath * ctk_container_get_path_for_child (GtkContainer      *container,
                                                  GtkWidget         *child);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkContainer, g_object_unref)

G_END_DECLS

#endif /* __CTK_CONTAINER_H__ */
