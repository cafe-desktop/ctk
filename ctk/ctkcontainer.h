/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_CONTAINER_H__
#define __CTK_CONTAINER_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_CONTAINER              (ctk_container_get_type ())
#define CTK_CONTAINER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CONTAINER, CtkContainer))
#define CTK_CONTAINER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CONTAINER, CtkContainerClass))
#define CTK_IS_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CONTAINER))
#define CTK_IS_CONTAINER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CONTAINER))
#define CTK_CONTAINER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CONTAINER, CtkContainerClass))


typedef struct _CtkContainer              CtkContainer;
typedef struct _CtkContainerPrivate       CtkContainerPrivate;
typedef struct _CtkContainerClass         CtkContainerClass;

struct _CtkContainer
{
  CtkWidget widget;

  /*< private >*/
  CtkContainerPrivate *priv;
};

/**
 * CtkContainerClass:
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
struct _CtkContainerClass
{
  CtkWidgetClass parent_class;

  /*< public >*/

  void    (*add)       		(CtkContainer	 *container,
				 CtkWidget	 *widget);
  void    (*remove)    		(CtkContainer	 *container,
				 CtkWidget	 *widget);
  void    (*check_resize)	(CtkContainer	 *container);
  void    (*forall)    		(CtkContainer	 *container,
				 gboolean	  include_internals,
				 CtkCallback	  callback,
				 gpointer	  callback_data);
  void    (*set_focus_child)	(CtkContainer	 *container,
				 CtkWidget	 *child);
  GType   (*child_type)		(CtkContainer	 *container);
  gchar*  (*composite_name)	(CtkContainer	 *container,
				 CtkWidget	 *child);
  void    (*set_child_property) (CtkContainer    *container,
				 CtkWidget       *child,
				 guint            property_id,
				 const GValue    *value,
				 GParamSpec      *pspec);
  void    (*get_child_property) (CtkContainer    *container,
                                 CtkWidget       *child,
				 guint            property_id,
				 GValue          *value,
				 GParamSpec      *pspec);
  CtkWidgetPath * (*get_path_for_child) (CtkContainer *container,
                                         CtkWidget    *child);


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
 * CtkResizeMode:
 * @CTK_RESIZE_PARENT: Pass resize request to the parent
 * @CTK_RESIZE_QUEUE: Queue resizes on this widget
 * @CTK_RESIZE_IMMEDIATE: Resize immediately. Deprecated.
 */
typedef enum
{
  CTK_RESIZE_PARENT,
  CTK_RESIZE_QUEUE,
  CTK_RESIZE_IMMEDIATE
} CtkResizeMode;


/* Application-level methods */

CDK_AVAILABLE_IN_ALL
GType   ctk_container_get_type		 (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
void    ctk_container_set_border_width	 (CtkContainer	   *container,
					  guint		    border_width);
CDK_AVAILABLE_IN_ALL
guint   ctk_container_get_border_width   (CtkContainer     *container);
CDK_AVAILABLE_IN_ALL
void    ctk_container_add		 (CtkContainer	   *container,
					  CtkWidget	   *widget);
CDK_AVAILABLE_IN_ALL
void    ctk_container_remove		 (CtkContainer	   *container,
					  CtkWidget	   *widget);

CDK_DEPRECATED_IN_3_12
void    ctk_container_set_resize_mode    (CtkContainer     *container,
					  CtkResizeMode     resize_mode);
CDK_DEPRECATED_IN_3_12
CtkResizeMode ctk_container_get_resize_mode (CtkContainer     *container);

CDK_AVAILABLE_IN_ALL
void    ctk_container_check_resize       (CtkContainer     *container);

CDK_AVAILABLE_IN_ALL
void     ctk_container_foreach      (CtkContainer       *container,
				     CtkCallback         callback,
				     gpointer            callback_data);
CDK_AVAILABLE_IN_ALL
GList*   ctk_container_get_children     (CtkContainer       *container);

CDK_AVAILABLE_IN_ALL
void     ctk_container_propagate_draw   (CtkContainer   *container,
					 CtkWidget      *child,
					 cairo_t        *cr);

CDK_DEPRECATED_IN_3_24
void     ctk_container_set_focus_chain  (CtkContainer   *container,
                                         GList          *focusable_widgets);
CDK_DEPRECATED_IN_3_24
gboolean ctk_container_get_focus_chain  (CtkContainer   *container,
					 GList         **focusable_widgets);
CDK_DEPRECATED_IN_3_24
void     ctk_container_unset_focus_chain (CtkContainer  *container);

#define CTK_IS_RESIZE_CONTAINER(widget) (CTK_IS_CONTAINER (widget) && \
                                        (ctk_container_get_resize_mode (CTK_CONTAINER (widget)) != CTK_RESIZE_PARENT))

/* Widget-level methods */

CDK_DEPRECATED_IN_3_14
void   ctk_container_set_reallocate_redraws (CtkContainer    *container,
					     gboolean         needs_redraws);
CDK_AVAILABLE_IN_ALL
void   ctk_container_set_focus_child	   (CtkContainer     *container,
					    CtkWidget	     *child);
CDK_AVAILABLE_IN_ALL
CtkWidget *
       ctk_container_get_focus_child	   (CtkContainer     *container);
CDK_AVAILABLE_IN_ALL
void   ctk_container_set_focus_vadjustment (CtkContainer     *container,
					    CtkAdjustment    *adjustment);
CDK_AVAILABLE_IN_ALL
CtkAdjustment *ctk_container_get_focus_vadjustment (CtkContainer *container);
CDK_AVAILABLE_IN_ALL
void   ctk_container_set_focus_hadjustment (CtkContainer     *container,
					    CtkAdjustment    *adjustment);
CDK_AVAILABLE_IN_ALL
CtkAdjustment *ctk_container_get_focus_hadjustment (CtkContainer *container);

CDK_DEPRECATED_IN_3_10
void    ctk_container_resize_children      (CtkContainer     *container);

CDK_AVAILABLE_IN_ALL
GType   ctk_container_child_type	   (CtkContainer     *container);


CDK_AVAILABLE_IN_ALL
void         ctk_container_class_install_child_property (CtkContainerClass *cclass,
							 guint		    property_id,
							 GParamSpec	   *pspec);
CDK_AVAILABLE_IN_3_18
void         ctk_container_class_install_child_properties (CtkContainerClass *cclass,
                                                           guint              n_pspecs,
                                                           GParamSpec       **pspecs);
CDK_AVAILABLE_IN_ALL
GParamSpec*  ctk_container_class_find_child_property	(GObjectClass	   *cclass,
							 const gchar	   *property_name);
CDK_AVAILABLE_IN_ALL
GParamSpec** ctk_container_class_list_child_properties	(GObjectClass	   *cclass,
							 guint		   *n_properties);
CDK_AVAILABLE_IN_ALL
void         ctk_container_add_with_properties		(CtkContainer	   *container,
							 CtkWidget	   *widget,
							 const gchar	   *first_prop_name,
							 ...) G_GNUC_NULL_TERMINATED;
CDK_AVAILABLE_IN_ALL
void         ctk_container_child_set			(CtkContainer	   *container,
							 CtkWidget	   *child,
							 const gchar	   *first_prop_name,
							 ...) G_GNUC_NULL_TERMINATED;
CDK_AVAILABLE_IN_ALL
void         ctk_container_child_get			(CtkContainer	   *container,
							 CtkWidget	   *child,
							 const gchar	   *first_prop_name,
							 ...) G_GNUC_NULL_TERMINATED;
CDK_AVAILABLE_IN_ALL
void         ctk_container_child_set_valist		(CtkContainer	   *container,
							 CtkWidget	   *child,
							 const gchar	   *first_property_name,
							 va_list	    var_args);
CDK_AVAILABLE_IN_ALL
void         ctk_container_child_get_valist		(CtkContainer	   *container,
							 CtkWidget	   *child,
							 const gchar	   *first_property_name,
							 va_list	    var_args);
CDK_AVAILABLE_IN_ALL
void	     ctk_container_child_set_property		(CtkContainer	   *container,
							 CtkWidget	   *child,
							 const gchar	   *property_name,
							 const GValue	   *value);
CDK_AVAILABLE_IN_ALL
void	     ctk_container_child_get_property		(CtkContainer	   *container,
							 CtkWidget	   *child,
							 const gchar	   *property_name,
	                                                 GValue		   *value);

CDK_AVAILABLE_IN_3_2
void ctk_container_child_notify (CtkContainer *container,
                                 CtkWidget    *child,
                                 const gchar  *child_property);

CDK_AVAILABLE_IN_3_18
void ctk_container_child_notify_by_pspec (CtkContainer *container,
                                          CtkWidget    *child,
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


CDK_AVAILABLE_IN_ALL
void    ctk_container_forall		     (CtkContainer *container,
					      CtkCallback   callback,
					      gpointer	    callback_data);

CDK_AVAILABLE_IN_ALL
void    ctk_container_class_handle_border_width (CtkContainerClass *klass);

CDK_AVAILABLE_IN_ALL
CtkWidgetPath * ctk_container_get_path_for_child (CtkContainer      *container,
                                                  CtkWidget         *child);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkContainer, g_object_unref)

G_END_DECLS

#endif /* __CTK_CONTAINER_H__ */
