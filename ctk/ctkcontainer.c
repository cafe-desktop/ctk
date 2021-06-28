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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#include "config.h"

#define GDK_DISABLE_DEPRECATION_WARNINGS
#include "ctkcontainer.h"
#include "ctkcontainerprivate.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <gobject/gobjectnotifyqueue.c>
#include <gobject/gvaluecollector.h>

#include "ctkadjustment.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctksizerequest.h"
#include "ctksizerequestcacheprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkwindow.h"
#include "ctkassistant.h"
#include "ctkintl.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidgetpath.h"
#include "a11y/ctkcontaineraccessible.h"
#include "a11y/ctkcontaineraccessibleprivate.h"
#include "ctkpopovermenu.h"
#include "ctkshortcutswindow.h"

/* A handful of containers inside CTK+ are cheating and widgets
 * inside internal structure as direct children for the purpose
 * of forall().
 */
#define SPECIAL_CONTAINER(x) (CTK_IS_ASSISTANT (x) || \
                              CTK_IS_ACTION_BAR (x) || \
                              CTK_IS_POPOVER_MENU (x) || \
                              CTK_IS_SHORTCUTS_SECTION (x) || \
                              CTK_IS_SHORTCUTS_WINDOW (x))

/**
 * SECTION:ctkcontainer
 * @Short_description: Base class for widgets which contain other widgets
 * @Title: CtkContainer
 *
 * A CTK+ user interface is constructed by nesting widgets inside widgets.
 * Container widgets are the inner nodes in the resulting tree of widgets:
 * they contain other widgets. So, for example, you might have a #CtkWindow
 * containing a #CtkFrame containing a #CtkLabel. If you wanted an image instead
 * of a textual label inside the frame, you might replace the #CtkLabel widget
 * with a #CtkImage widget.
 *
 * There are two major kinds of container widgets in CTK+. Both are subclasses
 * of the abstract CtkContainer base class.
 *
 * The first type of container widget has a single child widget and derives
 * from #CtkBin. These containers are decorators, which
 * add some kind of functionality to the child. For example, a #CtkButton makes
 * its child into a clickable button; a #CtkFrame draws a frame around its child
 * and a #CtkWindow places its child widget inside a top-level window.
 *
 * The second type of container can have more than one child; its purpose is to
 * manage layout. This means that these containers assign
 * sizes and positions to their children. For example, a #CtkHBox arranges its
 * children in a horizontal row, and a #CtkGrid arranges the widgets it contains
 * in a two-dimensional grid.
 *
 * For implementations of #CtkContainer the virtual method #CtkContainerClass.forall()
 * is always required, since it's used for drawing and other internal operations
 * on the children.
 * If the #CtkContainer implementation expect to have non internal children
 * it's needed to implement both #CtkContainerClass.add() and #CtkContainerClass.remove().
 * If the CtkContainer implementation has internal children, they should be added
 * with ctk_widget_set_parent() on init() and removed with ctk_widget_unparent()
 * in the #CtkWidgetClass.destroy() implementation.
 * See more about implementing custom widgets at https://wiki.gnome.org/HowDoI/CustomWidgets
 *
 * # Height for width geometry management
 *
 * CTK+ uses a height-for-width (and width-for-height) geometry management system.
 * Height-for-width means that a widget can change how much vertical space it needs,
 * depending on the amount of horizontal space that it is given (and similar for
 * width-for-height).
 *
 * There are some things to keep in mind when implementing container widgets
 * that make use of CTK+’s height for width geometry management system. First,
 * it’s important to note that a container must prioritize one of its
 * dimensions, that is to say that a widget or container can only have a
 * #CtkSizeRequestMode that is %CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH or
 * %CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT. However, every widget and container
 * must be able to respond to the APIs for both dimensions, i.e. even if a
 * widget has a request mode that is height-for-width, it is possible that
 * its parent will request its sizes using the width-for-height APIs.
 *
 * To ensure that everything works properly, here are some guidelines to follow
 * when implementing height-for-width (or width-for-height) containers.
 *
 * Each request mode involves 2 virtual methods. Height-for-width apis run
 * through ctk_widget_get_preferred_width() and then through ctk_widget_get_preferred_height_for_width().
 * When handling requests in the opposite #CtkSizeRequestMode it is important that
 * every widget request at least enough space to display all of its content at all times.
 *
 * When ctk_widget_get_preferred_height() is called on a container that is height-for-width,
 * the container must return the height for its minimum width. This is easily achieved by
 * simply calling the reverse apis implemented for itself as follows:
 *
 * |[<!-- language="C" -->
 * static void
 * foo_container_get_preferred_height (CtkWidget *widget,
 *                                     gint *min_height,
 *                                     gint *nat_height)
 * {
 *    if (i_am_in_height_for_width_mode)
 *      {
 *        gint min_width;
 *
 *        CTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget,
 *                                                            &min_width,
 *                                                            NULL);
 *        CTK_WIDGET_GET_CLASS (widget)->get_preferred_height_for_width
 *                                                           (widget,
 *                                                            min_width,
 *                                                            min_height,
 *                                                            nat_height);
 *      }
 *    else
 *      {
 *        ... many containers support both request modes, execute the
 *        real width-for-height request here by returning the
 *        collective heights of all widgets that are stacked
 *        vertically (or whatever is appropriate for this container)
 *        ...
 *      }
 * }
 * ]|
 *
 * Similarly, when ctk_widget_get_preferred_width_for_height() is called for a container or widget
 * that is height-for-width, it then only needs to return the base minimum width like so:
 *
 * |[<!-- language="C" -->
 * static void
 * foo_container_get_preferred_width_for_height (CtkWidget *widget,
 *                                               gint for_height,
 *                                               gint *min_width,
 *                                               gint *nat_width)
 * {
 *    if (i_am_in_height_for_width_mode)
 *      {
 *        CTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget,
 *                                                            min_width,
 *                                                            nat_width);
 *      }
 *    else
 *      {
 *        ... execute the real width-for-height request here based on
 *        the required width of the children collectively if the
 *        container were to be allocated the said height ...
 *      }
 * }
 * ]|
 *
 * Height for width requests are generally implemented in terms of a virtual allocation
 * of widgets in the input orientation. Assuming an height-for-width request mode, a container
 * would implement the get_preferred_height_for_width() virtual function by first calling
 * ctk_widget_get_preferred_width() for each of its children.
 *
 * For each potential group of children that are lined up horizontally, the values returned by
 * ctk_widget_get_preferred_width() should be collected in an array of #CtkRequestedSize structures.
 * Any child spacing should be removed from the input @for_width and then the collective size should be
 * allocated using the ctk_distribute_natural_allocation() convenience function.
 *
 * The container will then move on to request the preferred height for each child by using
 * ctk_widget_get_preferred_height_for_width() and using the sizes stored in the #CtkRequestedSize array.
 *
 * To allocate a height-for-width container, it’s again important
 * to consider that a container must prioritize one dimension over the other. So if
 * a container is a height-for-width container it must first allocate all widgets horizontally
 * using a #CtkRequestedSize array and ctk_distribute_natural_allocation() and then add any
 * extra space (if and where appropriate) for the widget to expand.
 *
 * After adding all the expand space, the container assumes it was allocated sufficient
 * height to fit all of its content. At this time, the container must use the total horizontal sizes
 * of each widget to request the height-for-width of each of its children and store the requests in a
 * #CtkRequestedSize array for any widgets that stack vertically (for tabular containers this can
 * be generalized into the heights and widths of rows and columns).
 * The vertical space must then again be distributed using ctk_distribute_natural_allocation()
 * while this time considering the allocated height of the widget minus any vertical spacing
 * that the container adds. Then vertical expand space should be added where appropriate and available
 * and the container should go on to actually allocating the child widgets.
 *
 * See [CtkWidget’s geometry management section][geometry-management]
 * to learn more about implementing height-for-width geometry management for widgets.
 *
 * # Child properties
 *
 * CtkContainer introduces child properties.
 * These are object properties that are not specific
 * to either the container or the contained widget, but rather to their relation.
 * Typical examples of child properties are the position or pack-type of a widget
 * which is contained in a #CtkBox.
 *
 * Use ctk_container_class_install_child_property() to install child properties
 * for a container class and ctk_container_class_find_child_property() or
 * ctk_container_class_list_child_properties() to get information about existing
 * child properties.
 *
 * To set the value of a child property, use ctk_container_child_set_property(),
 * ctk_container_child_set() or ctk_container_child_set_valist().
 * To obtain the value of a child property, use
 * ctk_container_child_get_property(), ctk_container_child_get() or
 * ctk_container_child_get_valist(). To emit notification about child property
 * changes, use ctk_widget_child_notify().
 *
 * # CtkContainer as CtkBuildable
 *
 * The CtkContainer implementation of the CtkBuildable interface supports
 * a <packing> element for children, which can contain multiple <property>
 * elements that specify child properties for the child.
 * 
 * Since 2.16, child properties can also be marked as translatable using
 * the same “translatable”, “comments” and “context” attributes that are used
 * for regular properties.
 *
 * Since 3.16, containers can have a <focus-chain> element containing multiple
 * <widget> elements, one for each child that should be added to the focus
 * chain. The ”name” attribute gives the id of the widget.
 *
 * An example of these properties in UI definitions:
 * |[
 * <object class="CtkBox">
 *   <child>
 *     <object class="CtkEntry" id="entry1"/>
 *     <packing>
 *       <property name="pack-type">start</property>
 *     </packing>
 *   </child>
 *   <child>
 *     <object class="CtkEntry" id="entry2"/>
 *   </child>
 *   <focus-chain>
 *     <widget name="entry1"/>
 *     <widget name="entry2"/>
 *   </focus-chain>
 * </object>
 * ]|
 *
 */


struct _CtkContainerPrivate
{
  CtkWidget *focus_child;

  GdkFrameClock *resize_clock;
  guint resize_handler;

  guint border_width : 16;
  guint border_width_set   : 1;

  guint has_focus_chain    : 1;
  guint reallocate_redraws : 1;
  guint restyle_pending    : 1;
  guint resize_mode        : 2;
  guint resize_mode_set    : 1;
  guint request_mode       : 2;
};

enum {
  ADD,
  REMOVE,
  CHECK_RESIZE,
  SET_FOCUS_CHILD,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_BORDER_WIDTH,
  PROP_RESIZE_MODE,
  PROP_CHILD,
  LAST_PROP
};

static GParamSpec *container_props[LAST_PROP];

#define PARAM_SPEC_PARAM_ID(pspec)              ((pspec)->param_id)
#define PARAM_SPEC_SET_PARAM_ID(pspec, id)      ((pspec)->param_id = (id))


/* --- prototypes --- */
static void     ctk_container_base_class_init      (CtkContainerClass *klass);
static void     ctk_container_base_class_finalize  (CtkContainerClass *klass);
static void     ctk_container_class_init           (CtkContainerClass *klass);
static void     ctk_container_init                 (CtkContainer      *container);
static void     ctk_container_destroy              (CtkWidget         *widget);
static void     ctk_container_set_property         (GObject         *object,
                                                    guint            prop_id,
                                                    const GValue    *value,
                                                    GParamSpec      *pspec);
static void     ctk_container_get_property         (GObject         *object,
                                                    guint            prop_id,
                                                    GValue          *value,
                                                    GParamSpec      *pspec);
static void     ctk_container_add_unimplemented    (CtkContainer      *container,
                                                    CtkWidget         *widget);
static void     ctk_container_remove_unimplemented (CtkContainer      *container,
                                                    CtkWidget         *widget);
static void     ctk_container_real_check_resize    (CtkContainer      *container);
static void     ctk_container_compute_expand       (CtkWidget         *widget,
                                                    gboolean          *hexpand_p,
                                                    gboolean          *vexpand_p);
static gboolean ctk_container_focus                (CtkWidget         *widget,
                                                    CtkDirectionType   direction);
static void     ctk_container_real_set_focus_child (CtkContainer      *container,
                                                    CtkWidget         *widget);

static gboolean ctk_container_focus_move           (CtkContainer      *container,
                                                    GList             *children,
                                                    CtkDirectionType   direction);
static void     ctk_container_children_callback    (CtkWidget         *widget,
                                                    gpointer           client_data);
static void     ctk_container_show_all             (CtkWidget         *widget);
static gint     ctk_container_draw                 (CtkWidget         *widget,
                                                    cairo_t           *cr);
static void     ctk_container_map                  (CtkWidget         *widget);
static void     ctk_container_unmap                (CtkWidget         *widget);
static void     ctk_container_adjust_size_request  (CtkWidget         *widget,
                                                    CtkOrientation     orientation,
                                                    gint              *minimum_size,
                                                    gint              *natural_size);
static void     ctk_container_adjust_baseline_request (CtkWidget      *widget,
						       gint           *minimum_baseline,
						       gint           *natural_baseline);
static void     ctk_container_adjust_size_allocation (CtkWidget       *widget,
                                                      CtkOrientation   orientation,
                                                      gint            *minimum_size,
                                                      gint            *natural_size,
                                                      gint            *allocated_pos,
                                                      gint            *allocated_size);
static void     ctk_container_adjust_baseline_allocation (CtkWidget      *widget,
							  gint           *baseline);
static CtkSizeRequestMode ctk_container_get_request_mode (CtkWidget   *widget);

static gchar* ctk_container_child_default_composite_name (CtkContainer *container,
                                                          CtkWidget    *child);

static CtkWidgetPath * ctk_container_real_get_path_for_child (CtkContainer *container,
                                                              CtkWidget    *child);

/* CtkBuildable */
static void ctk_container_buildable_init           (CtkBuildableIface *iface);
static void ctk_container_buildable_add_child      (CtkBuildable *buildable,
                                                    CtkBuilder   *builder,
                                                    GObject      *child,
                                                    const gchar  *type);
static gboolean ctk_container_buildable_custom_tag_start (CtkBuildable  *buildable,
                                                          CtkBuilder    *builder,
                                                          GObject       *child,
                                                          const gchar   *tagname,
                                                          GMarkupParser *parser,
                                                          gpointer      *data);
static void    ctk_container_buildable_custom_tag_end (CtkBuildable *buildable,
                                                       CtkBuilder   *builder,
                                                       GObject      *child,
                                                       const gchar  *tagname,
                                                       gpointer     *data);
static void    ctk_container_buildable_custom_finished (CtkBuildable *buildable,
                                                        CtkBuilder   *builder,
                                                        GObject      *child,
                                                        const gchar  *tagname,
                                                        gpointer      data);

static gboolean ctk_container_should_propagate_draw (CtkContainer   *container,
                                                     CtkWidget      *child,
                                                     cairo_t        *cr);

/* --- variables --- */
static GQuark                vadjustment_key_id;
static GQuark                hadjustment_key_id;
static GQuark                quark_focus_chain;
static guint                 container_signals[LAST_SIGNAL] = { 0 };
static gint                  CtkContainer_private_offset;
static CtkWidgetClass       *parent_class = NULL;
extern GParamSpecPool       *_ctk_widget_child_property_pool;
extern GObjectNotifyContext *_ctk_widget_child_property_notify_context;
static CtkBuildableIface    *parent_buildable_iface;


/* --- functions --- */
static inline gpointer
ctk_container_get_instance_private (CtkContainer *self)
{
  return G_STRUCT_MEMBER_P (self, CtkContainer_private_offset);
}

GType
ctk_container_get_type (void)
{
  static GType container_type = 0;

  if (!container_type)
    {
      const GTypeInfo container_info =
      {
        sizeof (CtkContainerClass),
        (GBaseInitFunc) ctk_container_base_class_init,
        (GBaseFinalizeFunc) ctk_container_base_class_finalize,
        (GClassInitFunc) ctk_container_class_init,
        NULL        /* class_finalize */,
        NULL        /* class_data */,
        sizeof (CtkContainer),
        0           /* n_preallocs */,
        (GInstanceInitFunc) ctk_container_init,
        NULL,       /* value_table */
      };

      const GInterfaceInfo buildable_info =
      {
        (GInterfaceInitFunc) ctk_container_buildable_init,
        NULL,
        NULL
      };

      container_type =
        g_type_register_static (CTK_TYPE_WIDGET, I_("CtkContainer"),
                                &container_info, G_TYPE_FLAG_ABSTRACT);

      CtkContainer_private_offset =
        g_type_add_instance_private (container_type, sizeof (CtkContainerPrivate));

      g_type_add_interface_static (container_type,
                                   CTK_TYPE_BUILDABLE,
                                   &buildable_info);

    }

  return container_type;
}

static void
ctk_container_base_class_init (CtkContainerClass *class)
{
  /* reset instance specifc class fields that don't get inherited */
  class->set_child_property = NULL;
  class->get_child_property = NULL;
}

static void
ctk_container_base_class_finalize (CtkContainerClass *class)
{
  GList *list, *node;

  list = g_param_spec_pool_list_owned (_ctk_widget_child_property_pool, G_OBJECT_CLASS_TYPE (class));
  for (node = list; node; node = node->next)
    {
      GParamSpec *pspec = node->data;

      g_param_spec_pool_remove (_ctk_widget_child_property_pool, pspec);
      PARAM_SPEC_SET_PARAM_ID (pspec, 0);
      g_param_spec_unref (pspec);
    }
  g_list_free (list);
}

static void
ctk_container_class_init (CtkContainerClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  vadjustment_key_id = g_quark_from_static_string ("ctk-vadjustment");
  hadjustment_key_id = g_quark_from_static_string ("ctk-hadjustment");
  quark_focus_chain = g_quark_from_static_string ("ctk-container-focus-chain");

  gobject_class->set_property = ctk_container_set_property;
  gobject_class->get_property = ctk_container_get_property;

  widget_class->destroy = ctk_container_destroy;
  widget_class->compute_expand = ctk_container_compute_expand;
  widget_class->show_all = ctk_container_show_all;
  widget_class->draw = ctk_container_draw;
  widget_class->map = ctk_container_map;
  widget_class->unmap = ctk_container_unmap;
  widget_class->focus = ctk_container_focus;

  widget_class->adjust_size_request = ctk_container_adjust_size_request;
  widget_class->adjust_baseline_request = ctk_container_adjust_baseline_request;
  widget_class->adjust_size_allocation = ctk_container_adjust_size_allocation;
  widget_class->adjust_baseline_allocation = ctk_container_adjust_baseline_allocation;
  widget_class->get_request_mode = ctk_container_get_request_mode;

  class->add = ctk_container_add_unimplemented;
  class->remove = ctk_container_remove_unimplemented;
  class->check_resize = ctk_container_real_check_resize;
  class->forall = NULL;
  class->set_focus_child = ctk_container_real_set_focus_child;
  class->child_type = NULL;
  class->composite_name = ctk_container_child_default_composite_name;
  class->get_path_for_child = ctk_container_real_get_path_for_child;

  container_props[PROP_RESIZE_MODE] =
      g_param_spec_enum ("resize-mode",
                         P_("Resize mode"),
                         P_("Specify how resize events are handled"),
                         CTK_TYPE_RESIZE_MODE,
                         CTK_RESIZE_PARENT,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  container_props[PROP_BORDER_WIDTH] =
      g_param_spec_uint ("border-width",
                         P_("Border width"),
                         P_("The width of the empty border outside the containers children"),
                         0, 65535,
                         0,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  container_props[PROP_CHILD] =
      g_param_spec_object ("child",
                           P_("Child"),
                           P_("Can be used to add a new child to the container"),
                           CTK_TYPE_WIDGET,
                           CTK_PARAM_WRITABLE|G_PARAM_DEPRECATED);

  g_object_class_install_properties (gobject_class, LAST_PROP, container_props);

  container_signals[ADD] =
    g_signal_new (I_("add"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkContainerClass, add),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_WIDGET);
  container_signals[REMOVE] =
    g_signal_new (I_("remove"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkContainerClass, remove),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_WIDGET);
  container_signals[CHECK_RESIZE] =
    g_signal_new (I_("check-resize"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkContainerClass, check_resize),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  container_signals[SET_FOCUS_CHILD] =
    g_signal_new (I_("set-focus-child"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkContainerClass, set_focus_child),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_WIDGET);

  if (CtkContainer_private_offset != 0)
    g_type_class_adjust_private_offset (class, &CtkContainer_private_offset);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_CONTAINER_ACCESSIBLE);
}

static void
ctk_container_buildable_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = ctk_container_buildable_add_child;
  iface->custom_tag_start = ctk_container_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_container_buildable_custom_tag_end;
  iface->custom_finished = ctk_container_buildable_custom_finished;
}

static void
ctk_container_buildable_add_child (CtkBuildable  *buildable,
                                   CtkBuilder    *builder,
                                   GObject       *child,
                                   const gchar   *type)
{
  if (type)
    {
      CTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
    }
  else if (CTK_IS_WIDGET (child) &&
           _ctk_widget_get_parent (CTK_WIDGET (child)) == NULL)
    {
      ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
    }
  else
    g_warning ("Cannot add an object of type %s to a container of type %s",
               g_type_name (G_OBJECT_TYPE (child)), g_type_name (G_OBJECT_TYPE (buildable)));
}

static inline void
container_set_child_property (CtkContainer       *container,
                              CtkWidget          *child,
                              GParamSpec         *pspec,
                              const GValue       *value,
                              GObjectNotifyQueue *nqueue)
{
  GValue tmp_value = G_VALUE_INIT;
  CtkContainerClass *class = g_type_class_peek (pspec->owner_type);

  /* provide a copy to work from, convert (if necessary) and validate */
  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (!g_value_transform (value, &tmp_value))
    g_warning ("unable to set child property '%s' of type '%s' from value of type '%s'",
               pspec->name,
               g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
               G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
    {
      gchar *contents = g_strdup_value_contents (value);

      g_warning ("value \"%s\" of type '%s' is invalid for property '%s' of type '%s'",
                 contents,
                 G_VALUE_TYPE_NAME (value),
                 pspec->name,
                 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      g_free (contents);
    }
  else
    {
      class->set_child_property (container, child, PARAM_SPEC_PARAM_ID (pspec), &tmp_value, pspec);
      g_object_notify_queue_add (G_OBJECT (child), nqueue, pspec);
    }
  g_value_unset (&tmp_value);
}

static void
ctk_container_buildable_set_child_property (CtkContainer *container,
                                            CtkBuilder   *builder,
                                            CtkWidget    *child,
                                            gchar        *name,
                                            const gchar  *value)
{
  GParamSpec *pspec;
  GValue gvalue = G_VALUE_INIT;
  GError *error = NULL;
  GObjectNotifyQueue *nqueue;

  if (_ctk_widget_get_parent (child) != (CtkWidget *)container &&
      !SPECIAL_CONTAINER (container))
    {
      /* This can happen with internal children of complex widgets.
       * Silently ignore the child properties in this case. We explicitly
       * allow it for CtkAssistant, since that is how it works.
       */
      return;
    }

  pspec = ctk_container_class_find_child_property (G_OBJECT_GET_CLASS (container), name);
  if (!pspec)
    {
      g_warning ("%s does not have a child property called %s",
                 G_OBJECT_TYPE_NAME (container), name);
      return;
    }
  else if (!(pspec->flags & G_PARAM_WRITABLE))
    {
      g_warning ("Child property '%s' of container class '%s' is not writable",
                 name, G_OBJECT_TYPE_NAME (container));
      return;
    }

  if (!ctk_builder_value_from_string (builder, pspec, value, &gvalue, &error))
    {
      g_warning ("Could not read property %s:%s with value %s of type %s: %s",
                 g_type_name (G_OBJECT_TYPE (container)),
                 name,
                 value,
                 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                 error->message);
      g_error_free (error);
      return;
    }

  g_object_ref (container);
  g_object_ref (child);
  nqueue = g_object_notify_queue_freeze (G_OBJECT (child), _ctk_widget_child_property_notify_context);
  container_set_child_property (container, child, pspec, &gvalue, nqueue);
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);
  g_object_unref (container);
  g_object_unref (child);
  g_value_unset (&gvalue);
}

typedef struct {
  CtkBuilder   *builder;
  CtkContainer *container;
  CtkWidget    *child;
  GString      *string;
  gchar        *child_prop_name;
  gchar        *context;
  gboolean      translatable;
} PackingData;

static void
packing_start_element (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **names,
                       const gchar         **values,
                       gpointer              user_data,
                       GError              **error)
{
  PackingData *data = (PackingData*)user_data;

  if (strcmp (element_name, "property") == 0)
    {
      const gchar *name;
      gboolean translatable = FALSE;
      const gchar *ctx = NULL;

      if (!_ctk_builder_check_parent (data->builder, context, "packing", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &ctx,
                                        G_MARKUP_COLLECT_INVALID))
       {
         _ctk_builder_prefix_error (data->builder, context, error);
         return;
       }

     data->child_prop_name = g_strdup (name);
     data->translatable = translatable;
     data->context = g_strdup (ctx);
    }
  else if (strcmp (element_name, "packing") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "child", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkContainer", element_name,
                                        error);
    }
}

static void
packing_text_element (GMarkupParseContext  *context,
                      const gchar          *text,
                      gsize                 text_len,
                      gpointer              user_data,
                      GError              **error)
{
  PackingData *data = (PackingData*)user_data;

  if (data->child_prop_name)
    g_string_append_len (data->string, text, text_len);
}

static void
packing_end_element (GMarkupParseContext  *context,
                     const gchar          *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  PackingData *data = (PackingData*)user_data;

  /* translate the string */
  if (data->string->len && data->translatable)
    {
      const gchar *translated;
      const gchar *domain;

      domain = ctk_builder_get_translation_domain (data->builder);

      translated = _ctk_builder_parser_translate (domain,
                                                  data->context,
                                                  data->string->str);
      g_string_assign (data->string, translated);
    }

  if (data->child_prop_name)
    ctk_container_buildable_set_child_property (data->container,
                                                data->builder,
                                                data->child,
                                                data->child_prop_name,
                                                data->string->str);

  g_string_set_size (data->string, 0);
  g_clear_pointer (&data->child_prop_name, g_free);
  g_clear_pointer (&data->context, g_free);
  data->translatable = FALSE;
}

static const GMarkupParser packing_parser =
  {
    packing_start_element,
    packing_end_element,
    packing_text_element,
  };

typedef struct
  {
    gchar *name;
    gint line;
    gint col;
  } FocusChainWidget;

static void
focus_chain_widget_free (gpointer data)
{
  FocusChainWidget *fcw = data;

  g_free (fcw->name);
  g_free (fcw);
}

typedef struct
  {
    GSList *items;
    GObject *object;
    CtkBuilder *builder;
    gint line;
    gint col;
  } FocusChainData;

static void
focus_chain_start_element (GMarkupParseContext  *context,
                           const gchar          *element_name,
                           const gchar         **names,
                           const gchar         **values,
                           gpointer              user_data,
                           GError              **error)
{
  FocusChainData *data = (FocusChainData*)user_data;

  if (strcmp (element_name, "widget") == 0)
    {
      const gchar *name;
      FocusChainWidget *fcw;

      if (!_ctk_builder_check_parent (data->builder, context, "focus-chain", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      fcw = g_new (FocusChainWidget, 1);
      fcw->name = g_strdup (name);
      g_markup_parse_context_get_position (context, &fcw->line, &fcw->col);
      data->items = g_slist_prepend (data->items, fcw);
    }
  else if (strcmp (element_name, "focus-chain") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, "", NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkContainer", element_name,
                                        error);
    }
}

static const GMarkupParser focus_chain_parser =
  {
    focus_chain_start_element
  };

static gboolean
ctk_container_buildable_custom_tag_start (CtkBuildable  *buildable,
                                          CtkBuilder    *builder,
                                          GObject       *child,
                                          const gchar   *tagname,
                                          GMarkupParser *parser,
                                          gpointer      *parser_data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, parser_data))
    return TRUE;

  if (child && strcmp (tagname, "packing") == 0)
    {
      PackingData *data;

      data = g_slice_new0 (PackingData);
      data->string = g_string_new ("");
      data->builder = builder;
      data->container = CTK_CONTAINER (buildable);
      data->child = CTK_WIDGET (child);
      data->child_prop_name = NULL;

      *parser = packing_parser;
      *parser_data = data;

      return TRUE;
    }
  else if (!child && strcmp (tagname, "focus-chain") == 0)
    {
      FocusChainData *data;

      data = g_slice_new0 (FocusChainData);
      data->items = NULL;
      data->object = G_OBJECT (buildable);
      data->builder = builder;

      *parser = focus_chain_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

static void
ctk_container_buildable_custom_tag_end (CtkBuildable *buildable,
                                        CtkBuilder   *builder,
                                        GObject      *child,
                                        const gchar  *tagname,
                                        gpointer     *parser_data)
{
  if (strcmp (tagname, "packing") == 0)
    {
      PackingData *data = (PackingData*)parser_data;

      g_string_free (data->string, TRUE);
      g_slice_free (PackingData, data);

      return;
    }

  if (parent_buildable_iface->custom_tag_end)
    parent_buildable_iface->custom_tag_end (buildable, builder,
                                            child, tagname, parser_data);
}

static void
ctk_container_buildable_custom_finished (CtkBuildable *buildable,
                                         CtkBuilder   *builder,
                                         GObject      *child,
                                         const gchar  *tagname,
                                         gpointer      parser_data)
{
   if (strcmp (tagname, "focus-chain") == 0)
    {
      FocusChainData *data = (FocusChainData*)parser_data;
      FocusChainWidget *fcw;
      GSList *l;
      GList *chain;
      GObject *object;

      chain = NULL;
      for (l = data->items; l; l = l->next)
        {
          fcw = l->data;
          object = _ctk_builder_lookup_object (builder, fcw->name, fcw->line, fcw->col);
          if (!object)
            continue;
          chain = g_list_prepend (chain, object);
        }

      ctk_container_set_focus_chain (CTK_CONTAINER (data->object), chain);
      g_list_free (chain);

      g_slist_free_full (data->items, focus_chain_widget_free);
      g_slice_free (FocusChainData, data);

      return;
    }

  if (parent_buildable_iface->custom_finished)
    parent_buildable_iface->custom_finished (buildable, builder,
                                             child, tagname, parser_data);
}

/**
 * ctk_container_child_type:
 * @container: a #CtkContainer
 *
 * Returns the type of the children supported by the container.
 *
 * Note that this may return %G_TYPE_NONE to indicate that no more
 * children can be added, e.g. for a #CtkPaned which already has two
 * children.
 *
 * Returns: a #GType.
 **/
GType
ctk_container_child_type (CtkContainer *container)
{
  GType slot;
  CtkContainerClass *class;

  g_return_val_if_fail (CTK_IS_CONTAINER (container), 0);

  class = CTK_CONTAINER_GET_CLASS (container);
  if (class->child_type)
    slot = class->child_type (container);
  else
    slot = G_TYPE_NONE;

  return slot;
}

/* --- CtkContainer child property mechanism --- */

/**
 * ctk_container_child_notify:
 * @container: the #CtkContainer
 * @child: the child widget
 * @child_property: the name of a child property installed on
 *     the class of @container
 *
 * Emits a #CtkWidget::child-notify signal for the
 * [child property][child-properties]
 * @child_property on the child.
 *
 * This is an analogue of g_object_notify() for child properties.
 *
 * Also see ctk_widget_child_notify().
 *
 * Since: 3.2
 */
void
ctk_container_child_notify (CtkContainer *container,
                            CtkWidget    *child,
                            const gchar  *child_property)
{
  GObject *obj;
  GParamSpec *pspec;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (child_property != NULL);

  obj = G_OBJECT (child);

  if (obj->ref_count == 0)
    return;

  g_object_ref (obj);

  pspec = g_param_spec_pool_lookup (_ctk_widget_child_property_pool,
                                    child_property,
                                    G_OBJECT_TYPE (container),
                                    TRUE);

  if (pspec == NULL)
    {
      g_warning ("%s: container class '%s' has no child property named '%s'",
                 G_STRLOC,
                 G_OBJECT_TYPE_NAME (container),
                 child_property);
    }
  else
    {
      GObjectNotifyQueue *nqueue;

      nqueue = g_object_notify_queue_freeze (obj, _ctk_widget_child_property_notify_context);

      g_object_notify_queue_add (obj, nqueue, pspec);
      g_object_notify_queue_thaw (obj, nqueue);
    }

  g_object_unref (obj);
}

/**
 * ctk_container_child_notify_by_pspec:
 * @container: the #CtkContainer
 * @child: the child widget
 * @pspec: the #GParamSpec of a child property instealled on
 *     the class of @container
 *
 * Emits a #CtkWidget::child-notify signal for the
 * [child property][child-properties] specified by
 * @pspec on the child.
 *
 * This is an analogue of g_object_notify_by_pspec() for child properties.
 *
 * Since: 3.18
 */
void
ctk_container_child_notify_by_pspec (CtkContainer *container,
                                     CtkWidget    *child,
                                     GParamSpec   *pspec)
{
  GObject *obj = G_OBJECT (child);
  GObjectNotifyQueue *nqueue;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (obj->ref_count == 0)
    return;

  g_object_ref (obj);

  nqueue = g_object_notify_queue_freeze (obj, _ctk_widget_child_property_notify_context);

  g_object_notify_queue_add (obj, nqueue, pspec);
  g_object_notify_queue_thaw (obj, nqueue);

  g_object_unref (obj);
}

static inline void
container_get_child_property (CtkContainer *container,
                              CtkWidget    *child,
                              GParamSpec   *pspec,
                              GValue       *value)
{
  CtkContainerClass *class = g_type_class_peek (pspec->owner_type);

  class->get_child_property (container, child, PARAM_SPEC_PARAM_ID (pspec), value, pspec);
}

/**
 * ctk_container_child_get_valist:
 * @container: a #CtkContainer
 * @child: a widget which is a child of @container
 * @first_property_name: the name of the first property to get
 * @var_args: return location for the first property, followed
 *     optionally by more name/return location pairs, followed by %NULL
 *
 * Gets the values of one or more child properties for @child and @container.
 **/
void
ctk_container_child_get_valist (CtkContainer *container,
                                CtkWidget    *child,
                                const gchar  *first_property_name,
                                va_list       var_args)
{
  const gchar *name;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (child));

  g_object_ref (container);
  g_object_ref (child);

  name = first_property_name;
  while (name)
    {
      GValue value = G_VALUE_INIT;
      GParamSpec *pspec;
      gchar *error;

      pspec = g_param_spec_pool_lookup (_ctk_widget_child_property_pool,
                                        name,
                                        G_OBJECT_TYPE (container),
                                        TRUE);
      if (!pspec)
        {
          g_warning ("%s: container class '%s' has no child property named '%s'",
                     G_STRLOC,
                     G_OBJECT_TYPE_NAME (container),
                     name);
          break;
        }
      if (!(pspec->flags & G_PARAM_READABLE))
        {
          g_warning ("%s: child property '%s' of container class '%s' is not readable",
                     G_STRLOC,
                     pspec->name,
                     G_OBJECT_TYPE_NAME (container));
          break;
        }
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      container_get_child_property (container, child, pspec, &value);
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);
          g_value_unset (&value);
          break;
        }
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }

  g_object_unref (child);
  g_object_unref (container);
}

/**
 * ctk_container_child_get_property:
 * @container: a #CtkContainer
 * @child: a widget which is a child of @container
 * @property_name: the name of the property to get
 * @value: a location to return the value
 *
 * Gets the value of a child property for @child and @container.
 **/
void
ctk_container_child_get_property (CtkContainer *container,
                                  CtkWidget    *child,
                                  const gchar  *property_name,
                                  GValue       *value)
{
  GParamSpec *pspec;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  g_object_ref (container);
  g_object_ref (child);
  pspec = g_param_spec_pool_lookup (_ctk_widget_child_property_pool, property_name,
                                    G_OBJECT_TYPE (container), TRUE);
  if (!pspec)
    g_warning ("%s: container class '%s' has no child property named '%s'",
               G_STRLOC,
               G_OBJECT_TYPE_NAME (container),
               property_name);
  else if (!(pspec->flags & G_PARAM_READABLE))
    g_warning ("%s: child property '%s' of container class '%s' is not readable",
               G_STRLOC,
               pspec->name,
               G_OBJECT_TYPE_NAME (container));
  else
    {
      GValue *prop_value, tmp_value = G_VALUE_INIT;

      /* auto-conversion of the callers value type
       */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
        {
          g_value_reset (value);
          prop_value = value;
        }
      else if (!g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
        {
          g_warning ("can't retrieve child property '%s' of type '%s' as value of type '%s'",
                     pspec->name,
                     g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                     G_VALUE_TYPE_NAME (value));
          g_object_unref (child);
          g_object_unref (container);
          return;
        }
      else
        {
          g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          prop_value = &tmp_value;
        }
      container_get_child_property (container, child, pspec, prop_value);
      if (prop_value != value)
        {
          g_value_transform (prop_value, value);
          g_value_unset (&tmp_value);
        }
    }
  g_object_unref (child);
  g_object_unref (container);
}

/**
 * ctk_container_child_set_valist:
 * @container: a #CtkContainer
 * @child: a widget which is a child of @container
 * @first_property_name: the name of the first property to set
 * @var_args: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 *
 * Sets one or more child properties for @child and @container.
 **/
void
ctk_container_child_set_valist (CtkContainer *container,
                                CtkWidget    *child,
                                const gchar  *first_property_name,
                                va_list       var_args)
{
  GObjectNotifyQueue *nqueue;
  const gchar *name;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (child));

  g_object_ref (container);
  g_object_ref (child);

  nqueue = g_object_notify_queue_freeze (G_OBJECT (child), _ctk_widget_child_property_notify_context);
  name = first_property_name;
  while (name)
    {
      GValue value = G_VALUE_INIT;
      gchar *error = NULL;
      GParamSpec *pspec = g_param_spec_pool_lookup (_ctk_widget_child_property_pool,
                                                    name,
                                                    G_OBJECT_TYPE (container),
                                                    TRUE);
      if (!pspec)
        {
          g_warning ("%s: container class '%s' has no child property named '%s'",
                     G_STRLOC,
                     G_OBJECT_TYPE_NAME (container),
                     name);
          break;
        }
      if (!(pspec->flags & G_PARAM_WRITABLE))
        {
          g_warning ("%s: child property '%s' of container class '%s' is not writable",
                     G_STRLOC,
                     pspec->name,
                     G_OBJECT_TYPE_NAME (container));
          break;
        }

      G_VALUE_COLLECT_INIT (&value, G_PARAM_SPEC_VALUE_TYPE (pspec),
                            var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);

          /* we purposely leak the value here, it might not be
           * in a sane state if an error condition occoured
           */
          break;
        }
      container_set_child_property (container, child, pspec, &value, nqueue);
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);

  g_object_unref (container);
  g_object_unref (child);
}

/**
 * ctk_container_child_set_property:
 * @container: a #CtkContainer
 * @child: a widget which is a child of @container
 * @property_name: the name of the property to set
 * @value: the value to set the property to
 *
 * Sets a child property for @child and @container.
 **/
void
ctk_container_child_set_property (CtkContainer *container,
                                  CtkWidget    *child,
                                  const gchar  *property_name,
                                  const GValue *value)
{
  GObjectNotifyQueue *nqueue;
  GParamSpec *pspec;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  g_object_ref (container);
  g_object_ref (child);

  nqueue = g_object_notify_queue_freeze (G_OBJECT (child), _ctk_widget_child_property_notify_context);
  pspec = g_param_spec_pool_lookup (_ctk_widget_child_property_pool, property_name,
                                    G_OBJECT_TYPE (container), TRUE);
  if (!pspec)
    g_warning ("%s: container class '%s' has no child property named '%s'",
               G_STRLOC,
               G_OBJECT_TYPE_NAME (container),
               property_name);
  else if (!(pspec->flags & G_PARAM_WRITABLE))
    g_warning ("%s: child property '%s' of container class '%s' is not writable",
               G_STRLOC,
               pspec->name,
               G_OBJECT_TYPE_NAME (container));
  else
    {
      container_set_child_property (container, child, pspec, value, nqueue);
    }
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);
  g_object_unref (container);
  g_object_unref (child);
}

/**
 * ctk_container_add_with_properties:
 * @container: a #CtkContainer
 * @widget: a widget to be placed inside @container
 * @first_prop_name: the name of the first child property to set
 * @...: a %NULL-terminated list of property names and values, starting
 *     with @first_prop_name
 *
 * Adds @widget to @container, setting child properties at the same time.
 * See ctk_container_add() and ctk_container_child_set() for more details.
 */
void
ctk_container_add_with_properties (CtkContainer *container,
                                   CtkWidget    *widget,
                                   const gchar  *first_prop_name,
                                   ...)
{
  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (_ctk_widget_get_parent (widget) == NULL);

  g_object_ref (container);
  g_object_ref (widget);
  ctk_widget_freeze_child_notify (widget);

  g_signal_emit (container, container_signals[ADD], 0, widget);
  if (_ctk_widget_get_parent (widget))
    {
      va_list var_args;

      va_start (var_args, first_prop_name);
      ctk_container_child_set_valist (container, widget, first_prop_name, var_args);
      va_end (var_args);
    }

  ctk_widget_thaw_child_notify (widget);
  g_object_unref (widget);
  g_object_unref (container);
}

/**
 * ctk_container_child_set:
 * @container: a #CtkContainer
 * @child: a widget which is a child of @container
 * @first_prop_name: the name of the first property to set
 * @...: a %NULL-terminated list of property names and values, starting
 *     with @first_prop_name
 *
 * Sets one or more child properties for @child and @container.
 */
void
ctk_container_child_set (CtkContainer      *container,
                         CtkWidget         *child,
                         const gchar       *first_prop_name,
                         ...)
{
  va_list var_args;

  va_start (var_args, first_prop_name);
  ctk_container_child_set_valist (container, child, first_prop_name, var_args);
  va_end (var_args);
}

/**
 * ctk_container_child_get:
 * @container: a #CtkContainer
 * @child: a widget which is a child of @container
 * @first_prop_name: the name of the first property to get
 * @...: return location for the first property, followed
 *     optionally by more name/return location pairs, followed by %NULL
 *
 * Gets the values of one or more child properties for @child and @container.
 */
void
ctk_container_child_get (CtkContainer      *container,
                         CtkWidget         *child,
                         const gchar       *first_prop_name,
                         ...)
{
  va_list var_args;

  va_start (var_args, first_prop_name);
  ctk_container_child_get_valist (container, child, first_prop_name, var_args);
  va_end (var_args);
}

static inline void
install_child_property_internal (GType       g_type,
                                 guint       property_id,
                                 GParamSpec *pspec)
{
  if (g_param_spec_pool_lookup (_ctk_widget_child_property_pool, pspec->name, g_type, FALSE))
    {
      g_warning ("Class '%s' already contains a child property named '%s'",
                 g_type_name (g_type),
                 pspec->name);
      return;
    }
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  PARAM_SPEC_SET_PARAM_ID (pspec, property_id);
  g_param_spec_pool_insert (_ctk_widget_child_property_pool, pspec, g_type);
}

/**
 * ctk_container_class_install_child_property:
 * @cclass: a #CtkContainerClass
 * @property_id: the id for the property
 * @pspec: the #GParamSpec for the property
 *
 * Installs a child property on a container class.
 **/
void
ctk_container_class_install_child_property (CtkContainerClass *cclass,
                                            guint              property_id,
                                            GParamSpec        *pspec)
{
  g_return_if_fail (CTK_IS_CONTAINER_CLASS (cclass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (pspec->flags & G_PARAM_WRITABLE)
    g_return_if_fail (cclass->set_child_property != NULL);
  if (pspec->flags & G_PARAM_READABLE)
    g_return_if_fail (cclass->get_child_property != NULL);
  g_return_if_fail (property_id > 0);
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);  /* paranoid */
  if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    g_return_if_fail ((pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)) == 0);

  install_child_property_internal (G_OBJECT_CLASS_TYPE (cclass), property_id, pspec);
}

/**
 * ctk_container_class_install_child_properties:
 * @cclass: a #CtkContainerClass
 * @n_pspecs: the length of the #GParamSpec array
 * @pspecs: (array length=n_pspecs): the #GParamSpec array defining the new
 *     child properties
 *
 * Installs child properties on a container class.
 *
 * Since: 3.18
 */
void
ctk_container_class_install_child_properties (CtkContainerClass  *cclass,
                                              guint               n_pspecs,
                                              GParamSpec        **pspecs)
{
  gint i;

  g_return_if_fail (CTK_IS_CONTAINER_CLASS (cclass));
  g_return_if_fail (n_pspecs > 1);
  g_return_if_fail (pspecs[0] == NULL);

  /* we skip the first element of the array as it would have a 0 prop_id */
  for (i = 1; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      g_return_if_fail (G_IS_PARAM_SPEC (pspec));
      if (pspec->flags & G_PARAM_WRITABLE)
        g_return_if_fail (cclass->set_child_property != NULL);
      if (pspec->flags & G_PARAM_READABLE)
        g_return_if_fail (cclass->get_child_property != NULL);
      g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);  /* paranoid */
      if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
        g_return_if_fail ((pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)) == 0);

      install_child_property_internal (G_OBJECT_CLASS_TYPE (cclass), i, pspec);
    }
}

/**
 * ctk_container_class_find_child_property:
 * @cclass: (type CtkContainerClass): a #CtkContainerClass
 * @property_name: the name of the child property to find
 *
 * Finds a child property of a container class by name.
 *
 * Returns: (nullable) (transfer none): the #GParamSpec of the child
 *     property or %NULL if @class has no child property with that
 *     name.
 */
GParamSpec*
ctk_container_class_find_child_property (GObjectClass *cclass,
                                         const gchar  *property_name)
{
  g_return_val_if_fail (CTK_IS_CONTAINER_CLASS (cclass), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (_ctk_widget_child_property_pool,
                                   property_name,
                                   G_OBJECT_CLASS_TYPE (cclass),
                                   TRUE);
}

/**
 * ctk_container_class_list_child_properties:
 * @cclass: (type CtkContainerClass): a #CtkContainerClass
 * @n_properties: location to return the number of child properties found
 *
 * Returns all child properties of a container class.
 *
 * Returns: (array length=n_properties) (transfer container):
 *     a newly allocated %NULL-terminated array of #GParamSpec*.
 *     The array must be freed with g_free().
 */
GParamSpec**
ctk_container_class_list_child_properties (GObjectClass *cclass,
                                           guint        *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (CTK_IS_CONTAINER_CLASS (cclass), NULL);

  pspecs = g_param_spec_pool_list (_ctk_widget_child_property_pool,
                                   G_OBJECT_CLASS_TYPE (cclass),
                                   &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}

static void
ctk_container_add_unimplemented (CtkContainer     *container,
                                 CtkWidget        *widget)
{
  g_warning ("CtkContainerClass::add not implemented for '%s'", g_type_name (G_TYPE_FROM_INSTANCE (container)));
}

static void
ctk_container_remove_unimplemented (CtkContainer     *container,
                                    CtkWidget        *widget)
{
  g_warning ("CtkContainerClass::remove not implemented for '%s'", g_type_name (G_TYPE_FROM_INSTANCE (container)));
}

static void
ctk_container_init (CtkContainer *container)
{
  CtkContainerPrivate *priv;

  container->priv = ctk_container_get_instance_private (container);
  priv = container->priv;

  priv->focus_child = NULL;
  priv->border_width = 0;
  priv->resize_mode = CTK_RESIZE_PARENT;
  priv->reallocate_redraws = FALSE;
  priv->border_width_set = FALSE;
}

static void
ctk_container_destroy (CtkWidget *widget)
{
  CtkContainer *container = CTK_CONTAINER (widget);
  CtkContainerPrivate *priv = container->priv;

  if (priv->restyle_pending)
    priv->restyle_pending = FALSE;

  g_clear_object (&priv->focus_child);

  /* do this before walking child widgets, to avoid
   * removing children from focus chain one by one.
   */
  if (priv->has_focus_chain)
    ctk_container_unset_focus_chain (container);

  ctk_container_foreach (container, (CtkCallback) ctk_widget_destroy, NULL);

  CTK_WIDGET_CLASS (parent_class)->destroy (widget);
}

static void
ctk_container_set_property (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec)
{
  CtkContainer *container = CTK_CONTAINER (object);

  switch (prop_id)
    {
    case PROP_BORDER_WIDTH:
      ctk_container_set_border_width (container, g_value_get_uint (value));
      break;
    case PROP_RESIZE_MODE:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_container_set_resize_mode (container, g_value_get_enum (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_CHILD:
      ctk_container_add (container, CTK_WIDGET (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_container_get_property (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec)
{
  CtkContainer *container = CTK_CONTAINER (object);
  CtkContainerPrivate *priv = container->priv;

  switch (prop_id)
    {
    case PROP_BORDER_WIDTH:
      g_value_set_uint (value, priv->border_width);
      break;
    case PROP_RESIZE_MODE:
      g_value_set_enum (value, priv->resize_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

gboolean
_ctk_container_get_border_width_set (CtkContainer *container)
{
  CtkContainerPrivate *priv;

  g_return_val_if_fail (CTK_IS_CONTAINER (container), FALSE);

  priv = container->priv;

  return priv->border_width_set;
}

void
_ctk_container_set_border_width_set (CtkContainer *container,
                                     gboolean      border_width_set)
{
  CtkContainerPrivate *priv;

  g_return_if_fail (CTK_IS_CONTAINER (container));

  priv = container->priv;

  priv->border_width_set = border_width_set ? TRUE : FALSE;
}

/**
 * ctk_container_set_border_width:
 * @container: a #CtkContainer
 * @border_width: amount of blank space to leave outside
 *   the container. Valid values are in the range 0-65535 pixels.
 *
 * Sets the border width of the container.
 *
 * The border width of a container is the amount of space to leave
 * around the outside of the container. The only exception to this is
 * #CtkWindow; because toplevel windows can’t leave space outside,
 * they leave the space inside. The border is added on all sides of
 * the container. To add space to only one side, use a specific
 * #CtkWidget:margin property on the child widget, for example
 * #CtkWidget:margin-top.
 **/
void
ctk_container_set_border_width (CtkContainer *container,
                                guint         border_width)
{
  CtkContainerPrivate *priv;

  g_return_if_fail (CTK_IS_CONTAINER (container));

  priv = container->priv;

  if (priv->border_width != border_width)
    {
      priv->border_width = border_width;
      _ctk_container_set_border_width_set (container, TRUE);

      g_object_notify_by_pspec (G_OBJECT (container), container_props[PROP_BORDER_WIDTH]);

      if (_ctk_widget_get_realized (CTK_WIDGET (container)))
        ctk_widget_queue_resize (CTK_WIDGET (container));
    }
}

/**
 * ctk_container_get_border_width:
 * @container: a #CtkContainer
 *
 * Retrieves the border width of the container. See
 * ctk_container_set_border_width().
 *
 * Returns: the current border width
 **/
guint
ctk_container_get_border_width (CtkContainer *container)
{
  g_return_val_if_fail (CTK_IS_CONTAINER (container), 0);

  return container->priv->border_width;
}

/**
 * ctk_container_add:
 * @container: a #CtkContainer
 * @widget: a widget to be placed inside @container
 *
 * Adds @widget to @container. Typically used for simple containers
 * such as #CtkWindow, #CtkFrame, or #CtkButton; for more complicated
 * layout containers such as #CtkBox or #CtkGrid, this function will
 * pick default packing parameters that may not be correct.  So
 * consider functions such as ctk_box_pack_start() and
 * ctk_grid_attach() as an alternative to ctk_container_add() in
 * those cases. A widget may be added to only one container at a time;
 * you can’t place the same widget inside two different containers.
 *
 * Note that some containers, such as #CtkScrolledWindow or #CtkListBox,
 * may add intermediate children between the added widget and the
 * container.
 */
void
ctk_container_add (CtkContainer *container,
                   CtkWidget    *widget)
{
  CtkWidget *parent;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  parent = _ctk_widget_get_parent (widget);

  if (parent != NULL)
    {
      g_warning ("Attempting to add a widget with type %s to a container of "
                 "type %s, but the widget is already inside a container of type %s, "
                 "please remove the widget from its existing container first." ,
                 g_type_name (G_OBJECT_TYPE (widget)),
                 g_type_name (G_OBJECT_TYPE (container)),
                 g_type_name (G_OBJECT_TYPE (parent)));
      return;
    }

  g_signal_emit (container, container_signals[ADD], 0, widget);

  _ctk_container_accessible_add (CTK_WIDGET (container), widget);
}

/**
 * ctk_container_remove:
 * @container: a #CtkContainer
 * @widget: a current child of @container
 *
 * Removes @widget from @container. @widget must be inside @container.
 * Note that @container will own a reference to @widget, and that this
 * may be the last reference held; so removing a widget from its
 * container can destroy that widget. If you want to use @widget
 * again, you need to add a reference to it before removing it from
 * a container, using g_object_ref(). If you don’t want to use @widget
 * again it’s usually more efficient to simply destroy it directly
 * using ctk_widget_destroy() since this will remove it from the
 * container and help break any circular reference count cycles.
 **/
void
ctk_container_remove (CtkContainer *container,
                      CtkWidget    *widget)
{
  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  g_object_ref (container);
  g_object_ref (widget);

  g_signal_emit (container, container_signals[REMOVE], 0, widget);

  _ctk_container_accessible_remove (CTK_WIDGET (container), widget);

  g_object_unref (widget);
  g_object_unref (container);
}

static void
ctk_container_real_set_resize_mode (CtkContainer  *container,
                                    CtkResizeMode  resize_mode)
{
  CtkWidget *widget = CTK_WIDGET (container);
  CtkContainerPrivate *priv = container->priv;

  if (_ctk_widget_is_toplevel (widget) &&
      resize_mode == CTK_RESIZE_PARENT)
    {
      resize_mode = CTK_RESIZE_QUEUE;
    }

  if (priv->resize_mode != resize_mode)
    {
      priv->resize_mode = resize_mode;

      ctk_widget_queue_resize (widget);
      g_object_notify_by_pspec (G_OBJECT (container), container_props[PROP_RESIZE_MODE]);
    }
}

/**
 * ctk_container_set_resize_mode:
 * @container: a #CtkContainer
 * @resize_mode: the new resize mode
 *
 * Sets the resize mode for the container.
 *
 * The resize mode of a container determines whether a resize request
 * will be passed to the container’s parent, queued for later execution
 * or executed immediately.
 *
 * Deprecated: 3.12: Resize modes are deprecated. They aren’t necessary
 *     anymore since frame clocks and might introduce obscure bugs if
 *     used.
 **/
void
ctk_container_set_resize_mode (CtkContainer  *container,
                               CtkResizeMode  resize_mode)
{
  CtkContainerPrivate *priv;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (resize_mode <= CTK_RESIZE_IMMEDIATE);

  priv = container->priv;
  priv->resize_mode_set = TRUE;

  ctk_container_real_set_resize_mode (container, resize_mode);
}

void
ctk_container_set_default_resize_mode (CtkContainer *container,
                                       CtkResizeMode resize_mode)
{
  CtkContainerPrivate *priv = container->priv;

  if (priv->resize_mode_set)
    return;

  ctk_container_real_set_resize_mode (container, resize_mode);
}

/**
 * ctk_container_get_resize_mode:
 * @container: a #CtkContainer
 *
 * Returns the resize mode for the container. See
 * ctk_container_set_resize_mode ().
 *
 * Returns: the current resize mode
 *
 * Deprecated: 3.12: Resize modes are deprecated. They aren’t necessary
 *     anymore since frame clocks and might introduce obscure bugs if
 *     used.
 **/
CtkResizeMode
ctk_container_get_resize_mode (CtkContainer *container)
{
  g_return_val_if_fail (CTK_IS_CONTAINER (container), CTK_RESIZE_PARENT);

  return container->priv->resize_mode;
}

/**
 * ctk_container_set_reallocate_redraws:
 * @container: a #CtkContainer
 * @needs_redraws: the new value for the container’s @reallocate_redraws flag
 *
 * Sets the @reallocate_redraws flag of the container to the given value.
 *
 * Containers requesting reallocation redraws get automatically
 * redrawn if any of their children changed allocation.
 *
 * Deprecated: 3.14: Call ctk_widget_queue_draw() in your size_allocate handler.
 **/
void
ctk_container_set_reallocate_redraws (CtkContainer *container,
                                      gboolean      needs_redraws)
{
  g_return_if_fail (CTK_IS_CONTAINER (container));

  container->priv->reallocate_redraws = needs_redraws ? TRUE : FALSE;
}

static gboolean
ctk_container_needs_idle_sizer (CtkContainer *container)
{
  CtkContainerPrivate *priv = container->priv;

  if (priv->resize_mode == CTK_RESIZE_PARENT)
    return FALSE;

  if (container->priv->restyle_pending)
    return TRUE;

  if (priv->resize_mode == CTK_RESIZE_IMMEDIATE)
    return FALSE;

  return ctk_widget_needs_allocate (CTK_WIDGET (container));
}

static void
ctk_container_idle_sizer (GdkFrameClock *clock,
			  CtkContainer  *container)
{
  /* We validate the style contexts in a single loop before even trying
   * to handle resizes instead of doing validations inline.
   * This is mostly necessary for compatibility reasons with old code,
   * because both style_updated and size_allocate functions often change
   * styles and so could cause infinite loops in this function.
   *
   * It's important to note that even an invalid style context returns
   * sane values. So the result of an invalid style context will never be
   * a program crash, but only a wrong layout or rendering.
   */
  if (container->priv->restyle_pending)
    {
      container->priv->restyle_pending = FALSE;
      ctk_css_node_validate (ctk_widget_get_css_node (CTK_WIDGET (container)));
    }

  /* we may be invoked with a container_resize_queue of NULL, because
   * queue_resize could have been adding an extra idle function while
   * the queue still got processed. we better just ignore such case
   * than trying to explicitly work around them with some extra flags,
   * since it doesn't cause any actual harm.
   */
  if (ctk_widget_needs_allocate (CTK_WIDGET (container)))
    {
      ctk_container_check_resize (container);
    }

  if (!ctk_container_needs_idle_sizer (container))
    {
      _ctk_container_stop_idle_sizer (container);
    }
  else
    {
      cdk_frame_clock_request_phase (clock,
                                     GDK_FRAME_CLOCK_PHASE_LAYOUT);
    }
}

static void
ctk_container_start_idle_sizer (CtkContainer *container)
{
  GdkFrameClock *clock;

  if (container->priv->resize_handler != 0)
    return;

  clock = ctk_widget_get_frame_clock (CTK_WIDGET (container));
  if (clock == NULL)
    return;

  if (!CTK_WIDGET (container)->priv->frameclock_connected)
    return;

  container->priv->resize_clock = clock;
  container->priv->resize_handler = g_signal_connect (clock, "layout",
						      G_CALLBACK (ctk_container_idle_sizer), container);
  cdk_frame_clock_request_phase (clock,
                                 GDK_FRAME_CLOCK_PHASE_LAYOUT);
}

void
_ctk_container_stop_idle_sizer (CtkContainer *container)
{
  if (container->priv->resize_handler == 0)
    return;

  g_signal_handler_disconnect (container->priv->resize_clock,
                               container->priv->resize_handler);
  container->priv->resize_handler = 0;
  container->priv->resize_clock = NULL;
}

void
ctk_container_queue_resize_handler (CtkContainer *container)
{
  CtkWidget *widget;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  g_return_if_fail (CTK_IS_RESIZE_CONTAINER (container));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  widget = CTK_WIDGET (container);

  if (_ctk_widget_get_visible (widget) &&
      (_ctk_widget_is_toplevel (widget) ||
       _ctk_widget_get_realized (widget)))
    {
      switch (container->priv->resize_mode)
        {
        case CTK_RESIZE_QUEUE:
          if (ctk_widget_needs_allocate (widget))
            ctk_container_start_idle_sizer (container);
          break;

        case CTK_RESIZE_IMMEDIATE:
          ctk_container_check_resize (container);
          break;

        case CTK_RESIZE_PARENT:
        default:
          g_assert_not_reached ();
          break;
        }
    }
}

void
_ctk_container_queue_restyle (CtkContainer *container)
{
  CtkContainerPrivate *priv;

  g_return_if_fail (CTK_CONTAINER (container));

  priv = container->priv;

  if (priv->restyle_pending)
    return;

  ctk_container_start_idle_sizer (container);
  priv->restyle_pending = TRUE;
}

void
_ctk_container_maybe_start_idle_sizer (CtkContainer *container)
{
  if (ctk_container_needs_idle_sizer (container))
    ctk_container_start_idle_sizer (container);
}

void
ctk_container_check_resize (CtkContainer *container)
{
  g_return_if_fail (CTK_IS_CONTAINER (container));

  g_signal_emit (container, container_signals[CHECK_RESIZE], 0);
}

static void
ctk_container_real_check_resize (CtkContainer *container)
{
  CtkWidget *widget = CTK_WIDGET (container);
  CtkAllocation allocation;
  CtkRequisition requisition;
  int baseline;

  if (_ctk_widget_get_alloc_needed (widget))
    {
      ctk_widget_get_preferred_size (widget, &requisition, NULL);
      ctk_widget_get_allocated_size (widget, &allocation, &baseline);

      if (requisition.width > allocation.width ||
          requisition.height > allocation.height)
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
          if (CTK_IS_RESIZE_CONTAINER (container))
            {
              ctk_widget_size_allocate (widget, &allocation);
            }
          else
            ctk_widget_queue_resize (widget);
          G_GNUC_END_IGNORE_DEPRECATIONS;
        }
      else
        {
          ctk_widget_size_allocate_with_baseline (widget, &allocation, baseline);
        }
    }
  else
    {
      ctk_widget_ensure_allocate (widget);
    }
}

/* The container hasn't changed size but one of its children
 *  queued a resize request. Which means that the allocation
 *  is not sufficient for the requisition of some child.
 *  We’ve already performed a size request at this point,
 *  so we simply need to reallocate and let the allocation
 *  trickle down via CTK_WIDGET_ALLOC_NEEDED flags.
 */
/**
 * ctk_container_resize_children:
 * @container: a #CtkContainer
 *
 * Deprecated: 3.10
 **/
void
ctk_container_resize_children (CtkContainer *container)
{
  CtkAllocation allocation;
  CtkWidget *widget;
  gint baseline;

  /* resizing invariants:
   * toplevels have *always* resize_mode != CTK_RESIZE_PARENT set.
   * containers that have an idle sizer pending must be flagged with
   * RESIZE_PENDING.
   */
  g_return_if_fail (CTK_IS_CONTAINER (container));

  widget = CTK_WIDGET (container);
  ctk_widget_get_allocated_size (widget, &allocation, &baseline);

  ctk_widget_size_allocate_with_baseline (widget, &allocation, baseline);
}

static void
ctk_container_adjust_size_request (CtkWidget         *widget,
                                   CtkOrientation     orientation,
                                   gint              *minimum_size,
                                   gint              *natural_size)
{
  CtkContainer *container;

  container = CTK_CONTAINER (widget);

  if (CTK_CONTAINER_GET_CLASS (widget)->_handle_border_width)
    {
      int border_width;

      border_width = container->priv->border_width;

      *minimum_size += border_width * 2;
      *natural_size += border_width * 2;
    }

  /* chain up last so ctk_widget_set_size_request() values
   * will have a chance to overwrite our border width.
   */
  parent_class->adjust_size_request (widget, orientation,
                                     minimum_size, natural_size);
}

static void
ctk_container_adjust_baseline_request (CtkWidget         *widget,
				       gint              *minimum_baseline,
				       gint              *natural_baseline)
{
  CtkContainer *container;

  container = CTK_CONTAINER (widget);

  if (CTK_CONTAINER_GET_CLASS (widget)->_handle_border_width)
    {
      int border_width;

      border_width = container->priv->border_width;

      *minimum_baseline += border_width;
      *natural_baseline += border_width;
    }

  parent_class->adjust_baseline_request (widget, minimum_baseline, natural_baseline);
}

static void
ctk_container_adjust_size_allocation (CtkWidget         *widget,
                                      CtkOrientation     orientation,
                                      gint              *minimum_size,
                                      gint              *natural_size,
                                      gint              *allocated_pos,
                                      gint              *allocated_size)
{
  CtkContainer *container;
  int border_width;

  container = CTK_CONTAINER (widget);

  if (CTK_CONTAINER_GET_CLASS (widget)->_handle_border_width)
    {
      border_width = container->priv->border_width;

      *allocated_size -= border_width * 2;
      *allocated_pos += border_width;
      *minimum_size -= border_width * 2;
      *natural_size -= border_width * 2;
    }

  /* Chain up to CtkWidgetClass *after* removing our border width from
   * the proposed allocation size. This is because it's possible that the
   * widget was allocated more space than it needs in a said orientation,
   * if CtkWidgetClass does any alignments and thus limits the size to the
   * natural size... then we need that to be done *after* removing any margins
   * and padding values.
   */
  parent_class->adjust_size_allocation (widget, orientation,
                                        minimum_size, natural_size, allocated_pos,
                                        allocated_size);
}

static void
ctk_container_adjust_baseline_allocation (CtkWidget         *widget,
					  gint              *baseline)
{
  CtkContainer *container;
  int border_width;

  container = CTK_CONTAINER (widget);

  if (CTK_CONTAINER_GET_CLASS (widget)->_handle_border_width)
    {
      border_width = container->priv->border_width;

      if (*baseline >= 0)
	*baseline -= border_width;
    }

  parent_class->adjust_baseline_allocation (widget, baseline);
}


typedef struct {
  gint hfw;
  gint wfh;
} RequestModeCount;

static void
count_request_modes (CtkWidget        *widget,
		     RequestModeCount *count)
{
  CtkSizeRequestMode mode = ctk_widget_get_request_mode (widget);

  switch (mode)
    {
    case CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
      count->hfw++;
      break;
    case CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
      count->wfh++;
      break;
    case CTK_SIZE_REQUEST_CONSTANT_SIZE:
    default:
      break;
    }
}

static CtkSizeRequestMode 
ctk_container_get_request_mode (CtkWidget *widget)
{
  CtkContainer *container = CTK_CONTAINER (widget);
  RequestModeCount count = { 0, 0 };

  ctk_container_forall (container, (CtkCallback)count_request_modes, &count);

  if (!count.hfw && !count.wfh)
    return CTK_SIZE_REQUEST_CONSTANT_SIZE;
  else
    return count.wfh > count.hfw ? 
        CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT :
	CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

/**
 * ctk_container_class_handle_border_width:
 * @klass: the class struct of a #CtkContainer subclass
 *
 * Modifies a subclass of #CtkContainerClass to automatically add and
 * remove the border-width setting on CtkContainer.  This allows the
 * subclass to ignore the border width in its size request and
 * allocate methods. The intent is for a subclass to invoke this
 * in its class_init function.
 *
 * ctk_container_class_handle_border_width() is necessary because it
 * would break API too badly to make this behavior the default. So
 * subclasses must “opt in” to the parent class handling border_width
 * for them.
 */
void
ctk_container_class_handle_border_width (CtkContainerClass *klass)
{
  g_return_if_fail (CTK_IS_CONTAINER_CLASS (klass));

  klass->_handle_border_width = TRUE;
}

/**
 * ctk_container_forall: (virtual forall)
 * @container: a #CtkContainer
 * @callback: (scope call) (closure callback_data): a callback
 * @callback_data: callback user data
 *
 * Invokes @callback on each direct child of @container, including
 * children that are considered “internal” (implementation details
 * of the container). “Internal” children generally weren’t added
 * by the user of the container, but were added by the container
 * implementation itself.
 *
 * Most applications should use ctk_container_foreach(), rather
 * than ctk_container_forall().
 **/
void
ctk_container_forall (CtkContainer *container,
                      CtkCallback   callback,
                      gpointer      callback_data)
{
  CtkContainerClass *class;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (callback != NULL);

  class = CTK_CONTAINER_GET_CLASS (container);

  if (class->forall)
    class->forall (container, TRUE, callback, callback_data);
}

/**
 * ctk_container_foreach:
 * @container: a #CtkContainer
 * @callback: (scope call):  a callback
 * @callback_data: callback user data
 *
 * Invokes @callback on each non-internal child of @container.
 * See ctk_container_forall() for details on what constitutes
 * an “internal” child. For all practical purposes, this function
 * should iterate over precisely those child widgets that were
 * added to the container by the application with explicit add()
 * calls.
 *
 * It is permissible to remove the child from the @callback handler.
 *
 * Most applications should use ctk_container_foreach(),
 * rather than ctk_container_forall().
 **/
void
ctk_container_foreach (CtkContainer *container,
                       CtkCallback   callback,
                       gpointer      callback_data)
{
  CtkContainerClass *class;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (callback != NULL);

  class = CTK_CONTAINER_GET_CLASS (container);

  if (class->forall)
    class->forall (container, FALSE, callback, callback_data);
}

/**
 * ctk_container_set_focus_child:
 * @container: a #CtkContainer
 * @child: (allow-none): a #CtkWidget, or %NULL
 *
 * Sets, or unsets if @child is %NULL, the focused child of @container.
 *
 * This function emits the CtkContainer::set_focus_child signal of
 * @container. Implementations of #CtkContainer can override the
 * default behaviour by overriding the class closure of this signal.
 *
 * This is function is mostly meant to be used by widgets. Applications can use
 * ctk_widget_grab_focus() to manually set the focus to a specific widget.
 */
void
ctk_container_set_focus_child (CtkContainer *container,
                               CtkWidget    *child)
{
  g_return_if_fail (CTK_IS_CONTAINER (container));
  if (child)
    g_return_if_fail (CTK_IS_WIDGET (child));

  g_signal_emit (container, container_signals[SET_FOCUS_CHILD], 0, child);
}

/**
 * ctk_container_get_focus_child:
 * @container: a #CtkContainer
 *
 * Returns the current focus child widget inside @container. This is not the
 * currently focused widget. That can be obtained by calling
 * ctk_window_get_focus().
 *
 * Returns: (nullable) (transfer none): The child widget which will receive the
 *          focus inside @container when the @container is focused,
 *          or %NULL if none is set.
 *
 * Since: 2.14
 **/
CtkWidget *
ctk_container_get_focus_child (CtkContainer *container)
{
  g_return_val_if_fail (CTK_IS_CONTAINER (container), NULL);

  return container->priv->focus_child;
}

/**
 * ctk_container_get_children:
 * @container: a #CtkContainer
 *
 * Returns the container’s non-internal children. See
 * ctk_container_forall() for details on what constitutes an "internal" child.
 *
 * Returns: (element-type CtkWidget) (transfer container): a newly-allocated list of the container’s non-internal children.
 **/
GList*
ctk_container_get_children (CtkContainer *container)
{
  GList *children = NULL;

  ctk_container_foreach (container,
                         ctk_container_children_callback,
                         &children);

  return g_list_reverse (children);
}

static void
ctk_container_child_position_callback (CtkWidget *widget,
                                       gpointer   client_data)
{
  struct {
    CtkWidget *child;
    guint i;
    guint index;
  } *data = client_data;

  data->i++;
  if (data->child == widget)
    data->index = data->i;
}

static gchar*
ctk_container_child_default_composite_name (CtkContainer *container,
                                            CtkWidget    *child)
{
  struct {
    CtkWidget *child;
    guint i;
    guint index;
  } data;
  gchar *name;

  /* fallback implementation */
  data.child = child;
  data.i = 0;
  data.index = 0;
  ctk_container_forall (container,
                        ctk_container_child_position_callback,
                        &data);

  name = g_strdup_printf ("%s-%u",
                          g_type_name (G_TYPE_FROM_INSTANCE (child)),
                          data.index);

  return name;
}

gchar*
_ctk_container_child_composite_name (CtkContainer *container,
                                    CtkWidget    *child)
{
  gboolean composite_child;

  g_return_val_if_fail (CTK_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (_ctk_widget_get_parent (child) == CTK_WIDGET (container), NULL);

  g_object_get (child, "composite-child", &composite_child, NULL);
  if (composite_child)
    {
      static GQuark quark_composite_name = 0;
      gchar *name;

      if (!quark_composite_name)
        quark_composite_name = g_quark_from_static_string ("ctk-composite-name");

      name = g_object_get_qdata (G_OBJECT (child), quark_composite_name);
      if (!name)
        {
          CtkContainerClass *class;

          class = CTK_CONTAINER_GET_CLASS (container);
          if (class->composite_name)
            name = class->composite_name (container, child);
        }
      else
        name = g_strdup (name);

      return name;
    }

  return NULL;
}

typedef struct {
  gboolean hexpand;
  gboolean vexpand;
} ComputeExpandData;

static void
ctk_container_compute_expand_callback (CtkWidget *widget,
                                       gpointer   client_data)
{
  ComputeExpandData *data = client_data;

  /* note that we don't get_expand on the child if we already know we
   * have to expand, so we only recurse into children until we find
   * one that expands and then we basically don't do any more
   * work. This means that we can leave some children in a
   * need_compute_expand state, which is fine, as long as CtkWidget
   * doesn't rely on an invariant that "if a child has
   * need_compute_expand, its parents also do"
   *
   * ctk_widget_compute_expand() always returns FALSE if the
   * child is !visible so that's taken care of.
   */
  data->hexpand = data->hexpand ||
    ctk_widget_compute_expand (widget, CTK_ORIENTATION_HORIZONTAL);

  data->vexpand = data->vexpand ||
    ctk_widget_compute_expand (widget, CTK_ORIENTATION_VERTICAL);
}

static void
ctk_container_compute_expand (CtkWidget         *widget,
                              gboolean          *hexpand_p,
                              gboolean          *vexpand_p)
{
  ComputeExpandData data;

  data.hexpand = FALSE;
  data.vexpand = FALSE;

  ctk_container_forall (CTK_CONTAINER (widget),
                        ctk_container_compute_expand_callback,
                        &data);

  *hexpand_p = data.hexpand;
  *vexpand_p = data.vexpand;
}

static void
ctk_container_real_set_focus_child (CtkContainer     *container,
                                    CtkWidget        *child)
{
  CtkContainerPrivate *priv;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (child == NULL || CTK_IS_WIDGET (child));

  priv = container->priv;

  if (child != priv->focus_child)
    {
      if (priv->focus_child)
        g_object_unref (priv->focus_child);

      priv->focus_child = child;

      if (priv->focus_child)
        g_object_ref (priv->focus_child);
    }

  /* Check for h/v adjustments and scroll to show the focus child if possible */
  if (priv->focus_child)
    {
      CtkAdjustment *hadj;
      CtkAdjustment *vadj;
      CtkAllocation allocation;
      CtkWidget *focus_child;
      gint x, y;

      hadj = g_object_get_qdata (G_OBJECT (container), hadjustment_key_id);
      vadj = g_object_get_qdata (G_OBJECT (container), vadjustment_key_id);
      if (hadj || vadj)
        {
          focus_child = priv->focus_child;
          while (CTK_IS_CONTAINER (focus_child) && ctk_container_get_focus_child (CTK_CONTAINER (focus_child)))
            {
              focus_child = ctk_container_get_focus_child (CTK_CONTAINER (focus_child));
            }

          if (!ctk_widget_translate_coordinates (focus_child, priv->focus_child,
                                                 0, 0, &x, &y))
            return;

          _ctk_widget_get_allocation (priv->focus_child, &allocation);
          x += allocation.x;
          y += allocation.y;

          _ctk_widget_get_allocation (focus_child, &allocation);

          if (vadj)
            ctk_adjustment_clamp_page (vadj, y, y + allocation.height);

          if (hadj)
            ctk_adjustment_clamp_page (hadj, x, x + allocation.width);
        }
    }
}

static GList*
get_focus_chain (CtkContainer *container)
{
  return g_object_get_qdata (G_OBJECT (container), quark_focus_chain);
}

/* same as ctk_container_get_children, except it includes internals
 */
GList *
ctk_container_get_all_children (CtkContainer *container)
{
  GList *children = NULL;

  ctk_container_forall (container,
                         ctk_container_children_callback,
                         &children);

  return children;
}

static CtkWidgetPath *
ctk_container_real_get_path_for_child (CtkContainer *container,
                                       CtkWidget    *child)
{
  CtkWidgetPath *path;
  CtkWidget *widget = CTK_WIDGET (container);

  path = _ctk_widget_create_path (widget);

  ctk_widget_path_append_for_widget (path, child);

  return path;
}

static gboolean
ctk_container_focus (CtkWidget        *widget,
                     CtkDirectionType  direction)
{
  GList *children;
  GList *sorted_children;
  gint return_val;
  CtkContainer *container;
  CtkContainerPrivate *priv;

  g_return_val_if_fail (CTK_IS_CONTAINER (widget), FALSE);

  container = CTK_CONTAINER (widget);
  priv = container->priv;

  return_val = FALSE;

  if (ctk_widget_get_can_focus (widget))
    {
      if (!ctk_widget_has_focus (widget))
        {
          ctk_widget_grab_focus (widget);
          return_val = TRUE;
        }
    }
  else
    {
      /* Get a list of the containers children, allowing focus
       * chain to override.
       */
      if (priv->has_focus_chain)
        children = g_list_copy (get_focus_chain (container));
      else
        children = ctk_container_get_all_children (container);

      if (priv->has_focus_chain &&
          (direction == CTK_DIR_TAB_FORWARD ||
           direction == CTK_DIR_TAB_BACKWARD))
        {
          sorted_children = g_list_copy (children);

          if (direction == CTK_DIR_TAB_BACKWARD)
            sorted_children = g_list_reverse (sorted_children);
        }
      else
        sorted_children = _ctk_container_focus_sort (container, children, direction, NULL);

      return_val = ctk_container_focus_move (container, sorted_children, direction);

      g_list_free (sorted_children);
      g_list_free (children);
    }

  return return_val;
}

static gint
tab_compare (gconstpointer a,
             gconstpointer b,
             gpointer      data)
{
  CtkAllocation child1_allocation, child2_allocation;
  const CtkWidget *child1 = a;
  const CtkWidget *child2 = b;
  CtkTextDirection text_direction = GPOINTER_TO_INT (data);
  gint y1, y2;

  _ctk_widget_get_allocation ((CtkWidget *) child1, &child1_allocation);
  _ctk_widget_get_allocation ((CtkWidget *) child2, &child2_allocation);

  y1 = child1_allocation.y + child1_allocation.height / 2;
  y2 = child2_allocation.y + child2_allocation.height / 2;

  if (y1 == y2)
    {
      gint x1 = child1_allocation.x + child1_allocation.width / 2;
      gint x2 = child2_allocation.x + child2_allocation.width / 2;

      if (text_direction == CTK_TEXT_DIR_RTL)
        return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
      else
        return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (y1 < y2) ? -1 : 1;
}

static GList *
ctk_container_focus_sort_tab (CtkContainer     *container,
                              GList            *children,
                              CtkDirectionType  direction,
                              CtkWidget        *old_focus)
{
  CtkTextDirection text_direction = _ctk_widget_get_direction (CTK_WIDGET (container));
  children = g_list_sort_with_data (children, tab_compare, GINT_TO_POINTER (text_direction));

  /* if we are going backwards then reverse the order
   *  of the children.
   */
  if (direction == CTK_DIR_TAB_BACKWARD)
    children = g_list_reverse (children);

  return children;
}

/* Get coordinates of @widget's allocation with respect to
 * allocation of @container.
 */
static gboolean
get_allocation_coords (CtkContainer  *container,
                       CtkWidget     *widget,
                       GdkRectangle  *allocation)
{
  ctk_widget_get_allocation (widget, allocation);

  return ctk_widget_translate_coordinates (widget, CTK_WIDGET (container),
                                           0, 0, &allocation->x, &allocation->y);
}

/* Look for a child in @children that is intermediate between
 * the focus widget and container. This widget, if it exists,
 * acts as the starting widget for focus navigation.
 */
static CtkWidget *
find_old_focus (CtkContainer *container,
                GList        *children)
{
  GList *tmp_list = children;
  while (tmp_list)
    {
      CtkWidget *child = tmp_list->data;
      CtkWidget *widget = child;

      while (widget && widget != (CtkWidget *)container)
        {
          CtkWidget *parent;

          parent = _ctk_widget_get_parent (widget);

          if (parent && (ctk_container_get_focus_child (CTK_CONTAINER (parent)) != widget))
            goto next;

          widget = parent;
        }

      return child;

    next:
      tmp_list = tmp_list->next;
    }

  return NULL;
}

static gboolean
old_focus_coords (CtkContainer *container,
                  GdkRectangle *old_focus_rect)
{
  CtkWidget *widget = CTK_WIDGET (container);
  CtkWidget *toplevel = _ctk_widget_get_toplevel (widget);
  CtkWidget *old_focus;

  if (CTK_IS_WINDOW (toplevel))
    {
      old_focus = ctk_window_get_focus (CTK_WINDOW (toplevel));
      if (old_focus)
        return get_allocation_coords (container, old_focus, old_focus_rect);
    }

  return FALSE;
}

typedef struct _CompareInfo CompareInfo;

struct _CompareInfo
{
  CtkContainer *container;
  gint x;
  gint y;
  gboolean reverse;
};

static gint
up_down_compare (gconstpointer a,
                 gconstpointer b,
                 gpointer      data)
{
  GdkRectangle allocation1;
  GdkRectangle allocation2;
  CompareInfo *compare = data;
  gint y1, y2;

  get_allocation_coords (compare->container, (CtkWidget *)a, &allocation1);
  get_allocation_coords (compare->container, (CtkWidget *)b, &allocation2);

  y1 = allocation1.y + allocation1.height / 2;
  y2 = allocation2.y + allocation2.height / 2;

  if (y1 == y2)
    {
      gint x1 = abs (allocation1.x + allocation1.width / 2 - compare->x);
      gint x2 = abs (allocation2.x + allocation2.width / 2 - compare->x);

      if (compare->reverse)
        return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
      else
        return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (y1 < y2) ? -1 : 1;
}

static GList *
ctk_container_focus_sort_up_down (CtkContainer     *container,
                                  GList            *children,
                                  CtkDirectionType  direction,
                                  CtkWidget        *old_focus)
{
  CompareInfo compare;
  GList *tmp_list;
  GdkRectangle old_allocation;

  compare.container = container;
  compare.reverse = (direction == CTK_DIR_UP);

  if (!old_focus)
      old_focus = find_old_focus (container, children);

  if (old_focus && get_allocation_coords (container, old_focus, &old_allocation))
    {
      gint compare_x1;
      gint compare_x2;
      gint compare_y;

      /* Delete widgets from list that don't match minimum criteria */

      compare_x1 = old_allocation.x;
      compare_x2 = old_allocation.x + old_allocation.width;

      if (direction == CTK_DIR_UP)
        compare_y = old_allocation.y;
      else
        compare_y = old_allocation.y + old_allocation.height;

      tmp_list = children;
      while (tmp_list)
        {
          CtkWidget *child = tmp_list->data;
          GList *next = tmp_list->next;
          gint child_x1, child_x2;
          GdkRectangle child_allocation;

          if (child != old_focus)
            {
              if (get_allocation_coords (container, child, &child_allocation))
                {
                  child_x1 = child_allocation.x;
                  child_x2 = child_allocation.x + child_allocation.width;

                  if ((child_x2 <= compare_x1 || child_x1 >= compare_x2) /* No horizontal overlap */ ||
                      (direction == CTK_DIR_DOWN && child_allocation.y + child_allocation.height < compare_y) || /* Not below */
                      (direction == CTK_DIR_UP && child_allocation.y > compare_y)) /* Not above */
                    {
                      children = g_list_delete_link (children, tmp_list);
                    }
                }
              else
                children = g_list_delete_link (children, tmp_list);
            }

          tmp_list = next;
        }

      compare.x = (compare_x1 + compare_x2) / 2;
      compare.y = old_allocation.y + old_allocation.height / 2;
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      CtkAllocation allocation;
      CtkWidget *widget = CTK_WIDGET (container);
      GdkRectangle old_focus_rect;

      _ctk_widget_get_allocation (widget, &allocation);

      if (old_focus_coords (container, &old_focus_rect))
        {
          compare.x = old_focus_rect.x + old_focus_rect.width / 2;
        }
      else
        {
          if (!_ctk_widget_get_has_window (widget))
            compare.x = allocation.x + allocation.width / 2;
          else
            compare.x = allocation.width / 2;
        }

      if (!_ctk_widget_get_has_window (widget))
        compare.y = (direction == CTK_DIR_DOWN) ? allocation.y : allocation.y + allocation.height;
      else
        compare.y = (direction == CTK_DIR_DOWN) ? 0 : + allocation.height;
    }

  children = g_list_sort_with_data (children, up_down_compare, &compare);

  if (compare.reverse)
    children = g_list_reverse (children);

  return children;
}

static gint
left_right_compare (gconstpointer a,
                    gconstpointer b,
                    gpointer      data)
{
  GdkRectangle allocation1;
  GdkRectangle allocation2;
  CompareInfo *compare = data;
  gint x1, x2;

  get_allocation_coords (compare->container, (CtkWidget *)a, &allocation1);
  get_allocation_coords (compare->container, (CtkWidget *)b, &allocation2);

  x1 = allocation1.x + allocation1.width / 2;
  x2 = allocation2.x + allocation2.width / 2;

  if (x1 == x2)
    {
      gint y1 = abs (allocation1.y + allocation1.height / 2 - compare->y);
      gint y2 = abs (allocation2.y + allocation2.height / 2 - compare->y);

      if (compare->reverse)
        return (y1 < y2) ? 1 : ((y1 == y2) ? 0 : -1);
      else
        return (y1 < y2) ? -1 : ((y1 == y2) ? 0 : 1);
    }
  else
    return (x1 < x2) ? -1 : 1;
}

static GList *
ctk_container_focus_sort_left_right (CtkContainer     *container,
                                     GList            *children,
                                     CtkDirectionType  direction,
                                     CtkWidget        *old_focus)
{
  CompareInfo compare;
  GList *tmp_list;
  GdkRectangle old_allocation;

  compare.container = container;
  compare.reverse = (direction == CTK_DIR_LEFT);

  if (!old_focus)
    old_focus = find_old_focus (container, children);

  if (old_focus && get_allocation_coords (container, old_focus, &old_allocation))
    {
      gint compare_y1;
      gint compare_y2;
      gint compare_x;

      /* Delete widgets from list that don't match minimum criteria */

      compare_y1 = old_allocation.y;
      compare_y2 = old_allocation.y + old_allocation.height;

      if (direction == CTK_DIR_LEFT)
        compare_x = old_allocation.x;
      else
        compare_x = old_allocation.x + old_allocation.width;

      tmp_list = children;
      while (tmp_list)
        {
          CtkWidget *child = tmp_list->data;
          GList *next = tmp_list->next;
          gint child_y1, child_y2;
          GdkRectangle child_allocation;

          if (child != old_focus)
            {
              if (get_allocation_coords (container, child, &child_allocation))
                {
                  child_y1 = child_allocation.y;
                  child_y2 = child_allocation.y + child_allocation.height;

                  if ((child_y2 <= compare_y1 || child_y1 >= compare_y2) /* No vertical overlap */ ||
                      (direction == CTK_DIR_RIGHT && child_allocation.x + child_allocation.width < compare_x) || /* Not to left */
                      (direction == CTK_DIR_LEFT && child_allocation.x > compare_x)) /* Not to right */
                    {
                      children = g_list_delete_link (children, tmp_list);
                    }
                }
              else
                children = g_list_delete_link (children, tmp_list);
            }

          tmp_list = next;
        }

      compare.y = (compare_y1 + compare_y2) / 2;
      compare.x = old_allocation.x + old_allocation.width / 2;
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      CtkAllocation allocation;
      CtkWidget *widget = CTK_WIDGET (container);
      GdkRectangle old_focus_rect;

      _ctk_widget_get_allocation (widget, &allocation);

      if (old_focus_coords (container, &old_focus_rect))
        {
          compare.y = old_focus_rect.y + old_focus_rect.height / 2;
        }
      else
        {
          if (!_ctk_widget_get_has_window (widget))
            compare.y = allocation.y + allocation.height / 2;
          else
            compare.y = allocation.height / 2;
        }

      if (!_ctk_widget_get_has_window (widget))
        compare.x = (direction == CTK_DIR_RIGHT) ? allocation.x : allocation.x + allocation.width;
      else
        compare.x = (direction == CTK_DIR_RIGHT) ? 0 : allocation.width;
    }

  children = g_list_sort_with_data (children, left_right_compare, &compare);

  if (compare.reverse)
    children = g_list_reverse (children);

  return children;
}

/**
 * ctk_container_focus_sort:
 * @container: a #CtkContainer
 * @children:  a list of descendents of @container (they don't
 *             have to be direct children)
 * @direction: focus direction
 * @old_focus: (allow-none): widget to use for the starting position, or %NULL
 *             to determine this automatically.
 *             (Note, this argument isn’t used for CTK_DIR_TAB_*,
 *              which is the only @direction we use currently,
 *              so perhaps this argument should be removed)
 *
 * Sorts @children in the correct order for focusing with
 * direction type @direction.
 *
 * Returns: a copy of @children, sorted in correct focusing order,
 *   with children that aren’t suitable for focusing in this direction
 *   removed.
 **/
GList *
_ctk_container_focus_sort (CtkContainer     *container,
                           GList            *children,
                           CtkDirectionType  direction,
                           CtkWidget        *old_focus)
{
  GList *visible_children = NULL;

  while (children)
    {
      if (_ctk_widget_get_realized (children->data))
        visible_children = g_list_prepend (visible_children, children->data);
      children = children->next;
    }

  switch (direction)
    {
    case CTK_DIR_TAB_FORWARD:
    case CTK_DIR_TAB_BACKWARD:
      return ctk_container_focus_sort_tab (container, visible_children, direction, old_focus);
    case CTK_DIR_UP:
    case CTK_DIR_DOWN:
      return ctk_container_focus_sort_up_down (container, visible_children, direction, old_focus);
    case CTK_DIR_LEFT:
    case CTK_DIR_RIGHT:
      return ctk_container_focus_sort_left_right (container, visible_children, direction, old_focus);
    }

  g_assert_not_reached ();

  return NULL;
}

static gboolean
ctk_container_focus_move (CtkContainer     *container,
                          GList            *children,
                          CtkDirectionType  direction)
{
  CtkContainerPrivate *priv = container->priv;
  CtkWidget *focus_child;
  CtkWidget *child;

  focus_child = priv->focus_child;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (!child)
        continue;

      if (focus_child)
        {
          if (focus_child == child)
            {
              focus_child = NULL;

                if (ctk_widget_child_focus (child, direction))
                  return TRUE;
            }
        }
      else if (_ctk_widget_is_drawable (child) &&
               ctk_widget_is_ancestor (child, CTK_WIDGET (container)))
        {
          if (ctk_widget_child_focus (child, direction))
            return TRUE;
        }
    }

  return FALSE;
}


static void
ctk_container_children_callback (CtkWidget *widget,
                                 gpointer   client_data)
{
  GList **children;

  children = (GList**) client_data;
  *children = g_list_prepend (*children, widget);
}

static void
chain_widget_destroyed (CtkWidget *widget,
                        gpointer   user_data)
{
  CtkContainer *container;
  GList *chain;

  container = CTK_CONTAINER (user_data);

  chain = g_object_get_qdata (G_OBJECT (container), quark_focus_chain);

  chain = g_list_remove (chain, widget);

  g_signal_handlers_disconnect_by_func (widget,
                                        chain_widget_destroyed,
                                        user_data);

  g_object_set_qdata (G_OBJECT (container), quark_focus_chain, chain);
}

/**
 * ctk_container_set_focus_chain:
 * @container: a #CtkContainer
 * @focusable_widgets: (transfer none) (element-type CtkWidget):
 *     the new focus chain
 *
 * Sets a focus chain, overriding the one computed automatically by CTK+.
 *
 * In principle each widget in the chain should be a descendant of the
 * container, but this is not enforced by this method, since it’s allowed
 * to set the focus chain before you pack the widgets, or have a widget
 * in the chain that isn’t always packed. The necessary checks are done
 * when the focus chain is actually traversed.
 *
 * Deprecated: 3.24: For overriding focus behavior, use the
 *     CtkWidgetClass::focus signal.
 **/
void
ctk_container_set_focus_chain (CtkContainer *container,
                               GList        *focusable_widgets)
{
  GList *chain;
  GList *tmp_list;
  CtkContainerPrivate *priv;

  g_return_if_fail (CTK_IS_CONTAINER (container));

  priv = container->priv;

  if (priv->has_focus_chain)
    ctk_container_unset_focus_chain (container);

  priv->has_focus_chain = TRUE;

  chain = NULL;
  tmp_list = focusable_widgets;
  while (tmp_list != NULL)
    {
      g_return_if_fail (CTK_IS_WIDGET (tmp_list->data));

      /* In principle each widget in the chain should be a descendant
       * of the container, but we don't want to check that here. It's
       * expensive and also it's allowed to set the focus chain before
       * you pack the widgets, or have a widget in the chain that isn't
       * always packed. So we check for ancestor during actual traversal.
       */

      chain = g_list_prepend (chain, tmp_list->data);

      g_signal_connect (tmp_list->data,
                        "destroy",
                        G_CALLBACK (chain_widget_destroyed),
                        container);

      tmp_list = tmp_list->next;
    }

  chain = g_list_reverse (chain);

  g_object_set_qdata (G_OBJECT (container), quark_focus_chain, chain);
}

/**
 * ctk_container_get_focus_chain:
 * @container:         a #CtkContainer
 * @focusable_widgets: (element-type CtkWidget) (out) (transfer container): location
 *                     to store the focus chain of the
 *                     container, or %NULL. You should free this list
 *                     using g_list_free() when you are done with it, however
 *                     no additional reference count is added to the
 *                     individual widgets in the focus chain.
 *
 * Retrieves the focus chain of the container, if one has been
 * set explicitly. If no focus chain has been explicitly
 * set, CTK+ computes the focus chain based on the positions
 * of the children. In that case, CTK+ stores %NULL in
 * @focusable_widgets and returns %FALSE.
 *
 * Returns: %TRUE if the focus chain of the container
 * has been set explicitly.
 *
 * Deprecated: 3.24: For overriding focus behavior, use the
 *     CtkWidgetClass::focus signal.
 **/
gboolean
ctk_container_get_focus_chain (CtkContainer *container,
                               GList       **focus_chain)
{
  CtkContainerPrivate *priv;

  g_return_val_if_fail (CTK_IS_CONTAINER (container), FALSE);

  priv = container->priv;

  if (focus_chain)
    {
      if (priv->has_focus_chain)
        *focus_chain = g_list_copy (get_focus_chain (container));
      else
        *focus_chain = NULL;
    }

  return priv->has_focus_chain;
}

/**
 * ctk_container_unset_focus_chain:
 * @container: a #CtkContainer
 *
 * Removes a focus chain explicitly set with ctk_container_set_focus_chain().
 *
 * Deprecated: 3.24: For overriding focus behavior, use the
 *     CtkWidgetClass::focus signal.
 **/
void
ctk_container_unset_focus_chain (CtkContainer  *container)
{
  CtkContainerPrivate *priv;

  g_return_if_fail (CTK_IS_CONTAINER (container));

  priv = container->priv;

  if (priv->has_focus_chain)
    {
      GList *chain;
      GList *tmp_list;

      chain = get_focus_chain (container);

      priv->has_focus_chain = FALSE;

      g_object_set_qdata (G_OBJECT (container), quark_focus_chain, NULL);

      tmp_list = chain;
      while (tmp_list != NULL)
        {
          g_signal_handlers_disconnect_by_func (tmp_list->data,
                                                chain_widget_destroyed,
                                                container);

          tmp_list = tmp_list->next;
        }

      g_list_free (chain);
    }
}

/**
 * ctk_container_set_focus_vadjustment:
 * @container: a #CtkContainer
 * @adjustment: an adjustment which should be adjusted when the focus
 *   is moved among the descendents of @container
 *
 * Hooks up an adjustment to focus handling in a container, so when a
 * child of the container is focused, the adjustment is scrolled to
 * show that widget. This function sets the vertical alignment. See
 * ctk_scrolled_window_get_vadjustment() for a typical way of obtaining
 * the adjustment and ctk_container_set_focus_hadjustment() for setting
 * the horizontal adjustment.
 *
 * The adjustments have to be in pixel units and in the same coordinate
 * system as the allocation for immediate children of the container.
 */
void
ctk_container_set_focus_vadjustment (CtkContainer  *container,
                                     CtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_CONTAINER (container));
  if (adjustment)
    g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref (adjustment);

  g_object_set_qdata_full (G_OBJECT (container),
                           vadjustment_key_id,
                           adjustment,
                           g_object_unref);
}

/**
 * ctk_container_get_focus_vadjustment:
 * @container: a #CtkContainer
 *
 * Retrieves the vertical focus adjustment for the container. See
 * ctk_container_set_focus_vadjustment().
 *
 * Returns: (nullable) (transfer none): the vertical focus adjustment, or
 *   %NULL if none has been set.
 **/
CtkAdjustment *
ctk_container_get_focus_vadjustment (CtkContainer *container)
{
  CtkAdjustment *vadjustment;

  g_return_val_if_fail (CTK_IS_CONTAINER (container), NULL);

  vadjustment = g_object_get_qdata (G_OBJECT (container), vadjustment_key_id);

  return vadjustment;
}

/**
 * ctk_container_set_focus_hadjustment:
 * @container: a #CtkContainer
 * @adjustment: an adjustment which should be adjusted when the focus is
 *   moved among the descendents of @container
 *
 * Hooks up an adjustment to focus handling in a container, so when a child
 * of the container is focused, the adjustment is scrolled to show that
 * widget. This function sets the horizontal alignment.
 * See ctk_scrolled_window_get_hadjustment() for a typical way of obtaining
 * the adjustment and ctk_container_set_focus_vadjustment() for setting
 * the vertical adjustment.
 *
 * The adjustments have to be in pixel units and in the same coordinate
 * system as the allocation for immediate children of the container.
 */
void
ctk_container_set_focus_hadjustment (CtkContainer  *container,
                                     CtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_CONTAINER (container));
  if (adjustment)
    g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref (adjustment);

  g_object_set_qdata_full (G_OBJECT (container),
                           hadjustment_key_id,
                           adjustment,
                           g_object_unref);
}

/**
 * ctk_container_get_focus_hadjustment:
 * @container: a #CtkContainer
 *
 * Retrieves the horizontal focus adjustment for the container. See
 * ctk_container_set_focus_hadjustment ().
 *
 * Returns: (nullable) (transfer none): the horizontal focus adjustment, or %NULL if
 *   none has been set.
 **/
CtkAdjustment *
ctk_container_get_focus_hadjustment (CtkContainer *container)
{
  CtkAdjustment *hadjustment;

  g_return_val_if_fail (CTK_IS_CONTAINER (container), NULL);

  hadjustment = g_object_get_qdata (G_OBJECT (container), hadjustment_key_id);

  return hadjustment;
}


static void
ctk_container_show_all (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_CONTAINER (widget));

  ctk_container_foreach (CTK_CONTAINER (widget),
                         (CtkCallback) ctk_widget_show_all,
                         NULL);
  ctk_widget_show (widget);
}

typedef struct {
  CtkWidget *child;
  int window_depth;
} ChildOrderInfo;

static void
ctk_container_draw_forall (CtkWidget *widget,
                           gpointer   client_data)
{
  struct {
    CtkContainer *container;
    GArray *child_infos;
    cairo_t *cr;
  } *data = client_data;
  ChildOrderInfo info;
  GList *siblings;
  GdkWindow *window;

  if (ctk_container_should_propagate_draw (data->container, widget, data->cr))
    {
      info.child = widget;
      info.window_depth = G_MAXINT;
      window = _ctk_widget_get_window (widget);
      if (window != ctk_widget_get_window (CTK_WIDGET (data->container)))
        {
          siblings = cdk_window_peek_children (cdk_window_get_parent (window));
          info.window_depth = g_list_index (siblings, window);
        }
      g_array_append_val (data->child_infos, info);
    }
}

static gint
compare_children_for_draw (gconstpointer  _a,
                           gconstpointer  _b)
{
  const ChildOrderInfo *a = _a;
  const ChildOrderInfo *b = _b;

  return b->window_depth - a->window_depth;
}

static gint
ctk_container_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  CtkContainer *container = CTK_CONTAINER (widget);
  GArray *child_infos;
  int i;
  ChildOrderInfo *child_info;
  struct {
    CtkContainer *container;
    GArray *child_infos;
    cairo_t *cr;
  } data;

  child_infos = g_array_new (FALSE, TRUE, sizeof (ChildOrderInfo));

  data.container = container;
  data.cr = cr;
  data.child_infos = child_infos;
  ctk_container_forall (container,
                        ctk_container_draw_forall,
                        &data);

  g_array_sort (child_infos, compare_children_for_draw);

  for (i = 0; i < child_infos->len; i++)
    {
      child_info = &g_array_index (child_infos, ChildOrderInfo, i);
      ctk_container_propagate_draw (container, child_info->child, cr);
    }

  g_array_free (child_infos, TRUE);

  return FALSE;
}

static void
ctk_container_map_child (CtkWidget *child,
                         gpointer   client_data)
{
  if (_ctk_widget_get_visible (child) &&
      _ctk_widget_get_child_visible (child) &&
      !_ctk_widget_get_mapped (child))
    ctk_widget_map (child);
}

static void
ctk_container_map (CtkWidget *widget)
{
  ctk_widget_set_mapped (widget, TRUE);

  ctk_container_forall (CTK_CONTAINER (widget),
                        ctk_container_map_child,
                        NULL);

  if (_ctk_widget_get_has_window (widget))
    cdk_window_show (_ctk_widget_get_window (widget));
}

static void
ctk_container_unmap (CtkWidget *widget)
{
  ctk_widget_set_mapped (widget, FALSE);

  /* hide our window first so user doesn't see all the child windows
   * vanishing one by one.  (only matters these days if one of the
   * children has an actual native window instead of client-side
   * window, e.g. a CtkSocket would)
   */
  if (_ctk_widget_get_has_window (widget))
    cdk_window_hide (_ctk_widget_get_window (widget));

  ctk_container_forall (CTK_CONTAINER (widget),
                        (CtkCallback)ctk_widget_unmap,
                        NULL);
}

static gboolean
ctk_container_should_propagate_draw (CtkContainer   *container,
                                     CtkWidget      *child,
                                     cairo_t        *cr)
{
  GdkWindow *child_in_window;

  if (!_ctk_widget_is_drawable (child))
    return FALSE;

  /* Never propagate to a child window when exposing a window
   * that is not the one the child widget is in.
   */
  if (_ctk_widget_get_has_window (child))
    child_in_window = cdk_window_get_parent (_ctk_widget_get_window (child));
  else
    child_in_window = _ctk_widget_get_window (child);
  if (!ctk_cairo_should_draw_window (cr, child_in_window))
    return FALSE;

  return TRUE;
}

static void
union_with_clip (CtkWidget *widget,
                 gpointer   data)
{
  GdkRectangle *clip = data;
  CtkAllocation widget_clip;

  if (!ctk_widget_is_visible (widget) ||
      !_ctk_widget_get_child_visible (widget))
    return;

  ctk_widget_get_clip (widget, &widget_clip);

  if (clip->width == 0 || clip->height == 0)
    *clip = widget_clip;
  else
    cdk_rectangle_union (&widget_clip, clip, clip);
}

void
ctk_container_get_children_clip (CtkContainer  *container,
                                 CtkAllocation *out_clip)
{
  memset (out_clip, 0, sizeof (CtkAllocation));

  ctk_container_forall (container, union_with_clip, out_clip);
}

/**
 * ctk_container_propagate_draw:
 * @container: a #CtkContainer
 * @child: a child of @container
 * @cr: Cairo context as passed to the container. If you want to use @cr
 *   in container’s draw function, consider using cairo_save() and
 *   cairo_restore() before calling this function.
 *
 * When a container receives a call to the draw function, it must send
 * synthetic #CtkWidget::draw calls to all children that don’t have their
 * own #GdkWindows. This function provides a convenient way of doing this.
 * A container, when it receives a call to its #CtkWidget::draw function,
 * calls ctk_container_propagate_draw() once for each child, passing in
 * the @cr the container received.
 *
 * ctk_container_propagate_draw() takes care of translating the origin of @cr,
 * and deciding whether the draw needs to be sent to the child. It is a
 * convenient and optimized way of getting the same effect as calling
 * ctk_widget_draw() on the child directly.
 *
 * In most cases, a container can simply either inherit the
 * #CtkWidget::draw implementation from #CtkContainer, or do some drawing
 * and then chain to the ::draw implementation from #CtkContainer.
 **/
void
ctk_container_propagate_draw (CtkContainer *container,
                              CtkWidget    *child,
                              cairo_t      *cr)
{
  CtkAllocation allocation;
  GdkWindow *window, *w;
  int x, y;

  g_return_if_fail (CTK_IS_CONTAINER (container));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (_ctk_widget_get_parent (child) == CTK_WIDGET (container));

  if (!ctk_container_should_propagate_draw (container, child, cr))
    return;

  /* translate coordinates. Ugly business, that. */
  if (!_ctk_widget_get_has_window (CTK_WIDGET (container)))
    {
      _ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
      x = -allocation.x;
      y = -allocation.y;
    }
  else
    {
      x = 0;
      y = 0;
    }

  window = _ctk_widget_get_window (CTK_WIDGET (container));

  for (w = _ctk_widget_get_window (child); w && w != window; w = cdk_window_get_parent (w))
    {
      int wx, wy;
      cdk_window_get_position (w, &wx, &wy);
      x += wx;
      y += wy;
    }

  if (w == NULL)
    {
      x = 0;
      y = 0;
    }

  if (!_ctk_widget_get_has_window (child))
    {
      _ctk_widget_get_allocation (child, &allocation);
      x += allocation.x;
      y += allocation.y;
    }

  cairo_save (cr);
  cairo_translate (cr, x, y);

  ctk_widget_draw_internal (child, cr, TRUE);

  cairo_restore (cr);
}

gboolean
_ctk_container_get_reallocate_redraws (CtkContainer *container)
{
  return container->priv->reallocate_redraws;
}

/**
 * ctk_container_get_path_for_child:
 * @container: a #CtkContainer
 * @child: a child of @container
 *
 * Returns a newly created widget path representing all the widget hierarchy
 * from the toplevel down to and including @child.
 *
 * Returns: A newly created #CtkWidgetPath
 **/
CtkWidgetPath *
ctk_container_get_path_for_child (CtkContainer *container,
                                  CtkWidget    *child)
{
  CtkWidgetPath *path;

  g_return_val_if_fail (CTK_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (container == (CtkContainer *) _ctk_widget_get_parent (child), NULL);

  path = CTK_CONTAINER_GET_CLASS (container)->get_path_for_child (container, child);
  if (ctk_widget_path_get_object_type (path) != G_OBJECT_TYPE (child))
    {
      g_critical ("%s %p returned a widget path for type %s, but child is %s",
                  G_OBJECT_TYPE_NAME (container),
                  container,
                  g_type_name (ctk_widget_path_get_object_type (path)),
                  G_OBJECT_TYPE_NAME (child));
    }

  return path;
}
