/* ctkcellarea.h
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
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
 */

#ifndef __CTK_CELL_AREA_H__
#define __CTK_CELL_AREA_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellrenderer.h>
#include <ctk/ctkwidget.h>
#include <ctk/ctktreemodel.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_AREA                (ctk_cell_area_get_type ())
#define CTK_CELL_AREA(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_AREA, CtkCellArea))
#define CTK_CELL_AREA_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_AREA, CtkCellAreaClass))
#define CTK_IS_CELL_AREA(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_AREA))
#define CTK_IS_CELL_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_AREA))
#define CTK_CELL_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_AREA, CtkCellAreaClass))

typedef struct _CtkCellArea              CtkCellArea;
typedef struct _CtkCellAreaClass         CtkCellAreaClass;
typedef struct _CtkCellAreaPrivate       CtkCellAreaPrivate;
typedef struct _CtkCellAreaContext       CtkCellAreaContext;

/**
 * CTK_CELL_AREA_WARN_INVALID_CELL_PROPERTY_ID:
 * @object: the #GObject on which set_cell_property() or get_cell_property()
 *     was called
 * @property_id: the numeric id of the property
 * @pspec: the #GParamSpec of the property
 *
 * This macro should be used to emit a standard warning about unexpected
 * properties in set_cell_property() and get_cell_property() implementations.
 */
#define CTK_CELL_AREA_WARN_INVALID_CELL_PROPERTY_ID(object, property_id, pspec) \
  G_OBJECT_WARN_INVALID_PSPEC ((object), "cell property id", (property_id), (pspec))

/**
 * CtkCellCallback:
 * @renderer: the cell renderer to operate on
 * @data: (closure): user-supplied data
 *
 * The type of the callback functions used for iterating over
 * the cell renderers of a #CtkCellArea, see ctk_cell_area_foreach().
 *
 * Returns: %TRUE to stop iterating over cells.
 */
typedef gboolean    (*CtkCellCallback) (CtkCellRenderer  *renderer,
                                        gpointer          data);

/**
 * CtkCellAllocCallback:
 * @renderer: the cell renderer to operate on
 * @cell_area: the area allocated to @renderer inside the rectangle
 *     provided to ctk_cell_area_foreach_alloc().
 * @cell_background: the background area for @renderer inside the
 *     background area provided to ctk_cell_area_foreach_alloc().
 * @data: (closure): user-supplied data
 *
 * The type of the callback functions used for iterating over the
 * cell renderers and their allocated areas inside a #CtkCellArea,
 * see ctk_cell_area_foreach_alloc().
 *
 * Returns: %TRUE to stop iterating over cells.
 */
typedef gboolean    (*CtkCellAllocCallback) (CtkCellRenderer    *renderer,
                                             const CdkRectangle *cell_area,
                                             const CdkRectangle *cell_background,
                                             gpointer            data);


struct _CtkCellArea
{
  /*< private >*/
  GInitiallyUnowned parent_instance;

  CtkCellAreaPrivate *priv;
};


/**
 * CtkCellAreaClass:
 * @add: adds a #CtkCellRenderer to the area.
 * @remove: removes a #CtkCellRenderer from the area.
 * @foreach: calls the #CtkCellCallback function on every #CtkCellRenderer in
 *     the area with the provided user data until the callback returns %TRUE.
 * @foreach_alloc: Calls the #CtkCellAllocCallback function on every
 *     #CtkCellRenderer in the area with the allocated area for the cell
 *     and the provided user data until the callback returns %TRUE.
 * @event: Handle an event in the area, this is generally used to activate
 *     a cell at the event location for button events but can also be used
 *     to generically pass events to #CtkWidgets drawn onto the area.
 * @render: Actually render the area’s cells to the specified rectangle,
 *     @background_area should be correctly distributed to the cells
 *     corresponding background areas.
 * @apply_attributes: Apply the cell attributes to the cells. This is
 *     implemented as a signal and generally #CtkCellArea subclasses don't
 *     need to implement it since it is handled by the base class.
 * @create_context: Creates and returns a class specific #CtkCellAreaContext
 *     to store cell alignment and allocation details for a said #CtkCellArea
 *     class.
 * @copy_context: Creates a new #CtkCellAreaContext in the same state as
 *     the passed @context with any cell alignment data and allocations intact.
 * @get_request_mode: This allows an area to tell its layouting widget whether
 *     it prefers to be allocated in %CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH or
 *     %CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT mode.
 * @get_preferred_width: Calculates the minimum and natural width of the
 *     areas cells with the current attributes applied while considering
 *     the particular layouting details of the said #CtkCellArea. While
 *     requests are performed over a series of rows, alignments and overall
 *     minimum and natural sizes should be stored in the corresponding
 *     #CtkCellAreaContext.
 * @get_preferred_height_for_width: Calculates the minimum and natural height
 *     for the area if the passed @context would be allocated the given width.
 *     When implementing this virtual method it is safe to assume that @context
 *     has already stored the aligned cell widths for every #CtkTreeModel row
 *     that @context will be allocated for since this information was stored
 *     at #CtkCellAreaClass.get_preferred_width() time. This virtual method
 *     should also store any necessary alignments of cell heights for the
 *     case that the context is allocated a height.
 * @get_preferred_height: Calculates the minimum and natural height of the
 *     areas cells with the current attributes applied. Essentially this is
 *     the same as #CtkCellAreaClass.get_preferred_width() only for areas
 *     that are being requested as %CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT.
 * @get_preferred_width_for_height: Calculates the minimum and natural width
 *     for the area if the passed @context would be allocated the given
 *     height. The same as #CtkCellAreaClass.get_preferred_height_for_width()
 *     only for handling requests in the %CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT
 *     mode.
 * @set_cell_property: This should be implemented to handle changes in child
 *     cell properties for a given #CtkCellRenderer that were previously
 *     installed on the #CtkCellAreaClass with ctk_cell_area_class_install_cell_property().
 * @get_cell_property: This should be implemented to report the values of
 *     child cell properties for a given child #CtkCellRenderer.
 * @focus: This virtual method should be implemented to navigate focus from
 *     cell to cell inside the #CtkCellArea. The #CtkCellArea should move
 *     focus from cell to cell inside the area and return %FALSE if focus
 *     logically leaves the area with the following exceptions: When the
 *     area contains no activatable cells, the entire area recieves focus.
 *     Focus should not be given to cells that are actually “focus siblings”
 *     of other sibling cells (see ctk_cell_area_get_focus_from_sibling()).
 *     Focus is set by calling ctk_cell_area_set_focus_cell().
 * @is_activatable: Returns whether the #CtkCellArea can respond to
 *     #CtkCellAreaClass.activate(), usually this does not need to be
 *     implemented since the base class takes care of this however it can
 *     be enhanced if the #CtkCellArea subclass can handle activation in
 *     other ways than activating its #CtkCellRenderers.
 * @activate: This is called when the layouting widget rendering the
 *     #CtkCellArea activates the focus cell (see ctk_cell_area_get_focus_cell()).
 */
struct _CtkCellAreaClass
{
  /*< private >*/
  GInitiallyUnownedClass parent_class;

  /*< public >*/

  /* Basic methods */
  void               (* add)                             (CtkCellArea             *area,
                                                          CtkCellRenderer         *renderer);
  void               (* remove)                          (CtkCellArea             *area,
                                                          CtkCellRenderer         *renderer);
  void               (* foreach)                         (CtkCellArea             *area,
                                                          CtkCellCallback          callback,
                                                          gpointer                 callback_data);
  void               (* foreach_alloc)                   (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          const CdkRectangle      *cell_area,
                                                          const CdkRectangle      *background_area,
                                                          CtkCellAllocCallback     callback,
                                                          gpointer                 callback_data);
  gint               (* event)                           (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          CdkEvent                *event,
                                                          const CdkRectangle      *cell_area,
                                                          CtkCellRendererState     flags);
  void               (* render)                          (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          cairo_t                 *cr,
                                                          const CdkRectangle      *background_area,
                                                          const CdkRectangle      *cell_area,
                                                          CtkCellRendererState     flags,
                                                          gboolean                 paint_focus);
  void               (* apply_attributes)                (CtkCellArea             *area,
                                                          CtkTreeModel            *tree_model,
                                                          CtkTreeIter             *iter,
                                                          gboolean                 is_expander,
                                                          gboolean                 is_expanded);

  /* Geometry */
  CtkCellAreaContext *(* create_context)                 (CtkCellArea             *area);
  CtkCellAreaContext *(* copy_context)                   (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context);
  CtkSizeRequestMode (* get_request_mode)                (CtkCellArea             *area);
  void               (* get_preferred_width)             (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          gint                    *minimum_width,
                                                          gint                    *natural_width);
  void               (* get_preferred_height_for_width)  (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          gint                     width,
                                                          gint                    *minimum_height,
                                                          gint                    *natural_height);
  void               (* get_preferred_height)            (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          gint                    *minimum_height,
                                                          gint                    *natural_height);
  void               (* get_preferred_width_for_height)  (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          gint                     height,
                                                          gint                    *minimum_width,
                                                          gint                    *natural_width);

  /* Cell Properties */
  void               (* set_cell_property)               (CtkCellArea             *area,
                                                          CtkCellRenderer         *renderer,
                                                          guint                    property_id,
                                                          const GValue            *value,
                                                          GParamSpec              *pspec);
  void               (* get_cell_property)               (CtkCellArea             *area,
                                                          CtkCellRenderer         *renderer,
                                                          guint                    property_id,
                                                          GValue                  *value,
                                                          GParamSpec              *pspec);

  /* Focus */
  gboolean           (* focus)                           (CtkCellArea             *area,
                                                          CtkDirectionType         direction);
  gboolean           (* is_activatable)                  (CtkCellArea             *area);
  gboolean           (* activate)                        (CtkCellArea             *area,
                                                          CtkCellAreaContext      *context,
                                                          CtkWidget               *widget,
                                                          const CdkRectangle      *cell_area,
                                                          CtkCellRendererState     flags,
                                                          gboolean                 edit_only);

  /*< private >*/

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

GDK_AVAILABLE_IN_ALL
GType                 ctk_cell_area_get_type                       (void) G_GNUC_CONST;

/* Basic methods */
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_add                            (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_remove                         (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_cell_area_has_renderer                   (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_foreach                        (CtkCellArea          *area,
                                                                    CtkCellCallback       callback,
                                                                    gpointer              callback_data);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_foreach_alloc                  (CtkCellArea          *area,
                                                                    CtkCellAreaContext   *context,
                                                                    CtkWidget            *widget,
                                                                    const CdkRectangle   *cell_area,
                                                                    const CdkRectangle   *background_area,
                                                                    CtkCellAllocCallback  callback,
                                                                    gpointer              callback_data);
GDK_AVAILABLE_IN_ALL
gint                  ctk_cell_area_event                          (CtkCellArea          *area,
                                                                    CtkCellAreaContext   *context,
                                                                    CtkWidget            *widget,
                                                                    CdkEvent             *event,
                                                                    const CdkRectangle   *cell_area,
                                                                    CtkCellRendererState  flags);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_render                         (CtkCellArea          *area,
                                                                    CtkCellAreaContext   *context,
                                                                    CtkWidget            *widget,
                                                                    cairo_t              *cr,
                                                                    const CdkRectangle   *background_area,
                                                                    const CdkRectangle   *cell_area,
                                                                    CtkCellRendererState  flags,
                                                                    gboolean              paint_focus);

GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_get_cell_allocation            (CtkCellArea          *area,
                                                                    CtkCellAreaContext   *context,
                                                                    CtkWidget            *widget,
                                                                    CtkCellRenderer      *renderer,
                                                                    const CdkRectangle   *cell_area,
                                                                    CdkRectangle         *allocation);
GDK_AVAILABLE_IN_ALL
CtkCellRenderer      *ctk_cell_area_get_cell_at_position           (CtkCellArea          *area,
                                                                    CtkCellAreaContext   *context,
                                                                    CtkWidget            *widget,
                                                                    const CdkRectangle   *cell_area,
                                                                    gint                  x,
                                                                    gint                  y,
                                                                    CdkRectangle         *alloc_area);

/* Geometry */
GDK_AVAILABLE_IN_ALL
CtkCellAreaContext   *ctk_cell_area_create_context                 (CtkCellArea        *area);
GDK_AVAILABLE_IN_ALL
CtkCellAreaContext   *ctk_cell_area_copy_context                   (CtkCellArea        *area,
                                                                    CtkCellAreaContext *context);
GDK_AVAILABLE_IN_ALL
CtkSizeRequestMode    ctk_cell_area_get_request_mode               (CtkCellArea        *area);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_get_preferred_width            (CtkCellArea        *area,
                                                                    CtkCellAreaContext *context,
                                                                    CtkWidget          *widget,
                                                                    gint               *minimum_width,
                                                                    gint               *natural_width);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_get_preferred_height_for_width (CtkCellArea        *area,
                                                                    CtkCellAreaContext *context,
                                                                    CtkWidget          *widget,
                                                                    gint                width,
                                                                    gint               *minimum_height,
                                                                    gint               *natural_height);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_get_preferred_height           (CtkCellArea        *area,
                                                                    CtkCellAreaContext *context,
                                                                    CtkWidget          *widget,
                                                                    gint               *minimum_height,
                                                                    gint               *natural_height);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_get_preferred_width_for_height (CtkCellArea        *area,
                                                                    CtkCellAreaContext *context,
                                                                    CtkWidget          *widget,
                                                                    gint                height,
                                                                    gint               *minimum_width,
                                                                    gint               *natural_width);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_cell_area_get_current_path_string        (CtkCellArea        *area);


/* Attributes */
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_apply_attributes               (CtkCellArea        *area,
                                                                    CtkTreeModel       *tree_model,
                                                                    CtkTreeIter        *iter,
                                                                    gboolean            is_expander,
                                                                    gboolean            is_expanded);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_attribute_connect              (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *attribute,
                                                                    gint                column);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_attribute_disconnect           (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *attribute);
GDK_AVAILABLE_IN_3_14
gint                  ctk_cell_area_attribute_get_column           (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *attribute);


/* Cell Properties */
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_class_install_cell_property    (CtkCellAreaClass   *aclass,
                                                                    guint               property_id,
                                                                    GParamSpec         *pspec);
GDK_AVAILABLE_IN_ALL
GParamSpec*           ctk_cell_area_class_find_cell_property       (CtkCellAreaClass   *aclass,
                                                                    const gchar        *property_name);
GDK_AVAILABLE_IN_ALL
GParamSpec**          ctk_cell_area_class_list_cell_properties     (CtkCellAreaClass   *aclass,
                                                                    guint                   *n_properties);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_add_with_properties            (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar     *first_prop_name,
                                                                    ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_cell_set                       (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *first_prop_name,
                                                                    ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_cell_get                       (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *first_prop_name,
                                                                    ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_cell_set_valist                (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *first_property_name,
                                                                    va_list             var_args);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_cell_get_valist                (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *first_property_name,
                                                                    va_list             var_args);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_cell_set_property              (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *property_name,
                                                                    const GValue       *value);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_cell_get_property              (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    const gchar        *property_name,
                                                                    GValue             *value);

/* Focus */
GDK_AVAILABLE_IN_ALL
gboolean              ctk_cell_area_is_activatable                 (CtkCellArea         *area);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_cell_area_activate                       (CtkCellArea         *area,
                                                                    CtkCellAreaContext  *context,
                                                                    CtkWidget           *widget,
                                                                    const CdkRectangle  *cell_area,
                                                                    CtkCellRendererState flags,
                                                                    gboolean             edit_only);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_cell_area_focus                          (CtkCellArea         *area,
                                                                    CtkDirectionType     direction);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_set_focus_cell                 (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer);
GDK_AVAILABLE_IN_ALL
CtkCellRenderer      *ctk_cell_area_get_focus_cell                 (CtkCellArea          *area);


/* Focus siblings */
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_add_focus_sibling              (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer,
                                                                    CtkCellRenderer      *sibling);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_remove_focus_sibling           (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer,
                                                                    CtkCellRenderer      *sibling);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_cell_area_is_focus_sibling               (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer,
                                                                    CtkCellRenderer      *sibling);
GDK_AVAILABLE_IN_ALL
const GList *         ctk_cell_area_get_focus_siblings             (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer);
GDK_AVAILABLE_IN_ALL
CtkCellRenderer      *ctk_cell_area_get_focus_from_sibling         (CtkCellArea          *area,
                                                                    CtkCellRenderer      *renderer);

/* Cell Activation/Editing */
GDK_AVAILABLE_IN_ALL
CtkCellRenderer      *ctk_cell_area_get_edited_cell                (CtkCellArea          *area);
GDK_AVAILABLE_IN_ALL
CtkCellEditable      *ctk_cell_area_get_edit_widget                (CtkCellArea          *area);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_cell_area_activate_cell                  (CtkCellArea          *area,
                                                                    CtkWidget            *widget,
                                                                    CtkCellRenderer      *renderer,
                                                                    CdkEvent             *event,
                                                                    const CdkRectangle   *cell_area,
                                                                    CtkCellRendererState  flags);
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_stop_editing                   (CtkCellArea          *area,
                                                                    gboolean              canceled);

/* Functions for area implementations */

/* Distinguish the inner cell area from the whole requested area including margins */
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_inner_cell_area                (CtkCellArea        *area,
                                                                    CtkWidget          *widget,
                                                                    const CdkRectangle *cell_area,
                                                                    CdkRectangle       *inner_area);

/* Request the size of a cell while respecting the cell margins (requests are margin inclusive) */
GDK_AVAILABLE_IN_ALL
void                  ctk_cell_area_request_renderer               (CtkCellArea        *area,
                                                                    CtkCellRenderer    *renderer,
                                                                    CtkOrientation      orientation,
                                                                    CtkWidget          *widget,
                                                                    gint                for_size,
                                                                    gint               *minimum_size,
                                                                    gint               *natural_size);

/* For api stability, this is called from ctkcelllayout.c in order to ensure the correct
 * object is passed to the user function in ctk_cell_layout_set_cell_data_func.
 *
 * This private api takes gpointer & GFunc arguments to circumvent circular header file
 * dependancies.
 */
void                 _ctk_cell_area_set_cell_data_func_with_proxy  (CtkCellArea           *area,
								    CtkCellRenderer       *cell,
								    GFunc                  func,
								    gpointer               func_data,
								    GDestroyNotify         destroy,
								    gpointer               proxy);

G_END_DECLS

#endif /* __CTK_CELL_AREA_H__ */
