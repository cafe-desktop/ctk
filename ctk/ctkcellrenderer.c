/* ctkcellrenderer.c
 * Copyright (C) 2000  Red Hat, Inc. Jonathan Blandford
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

#include "config.h"
#include "ctkcellrenderer.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctktreeprivate.h"
#include "a11y/ctkrenderercellaccessible.h"


/**
 * SECTION:ctkcellrenderer
 * @Short_description: An object for rendering a single cell
 * @Title: CtkCellRenderer
 * @See_also: #CtkCellEditable
 *
 * The #CtkCellRenderer is a base class of a set of objects used for
 * rendering a cell to a #cairo_t.  These objects are used primarily by
 * the #CtkTreeView widget, though they aren’t tied to them in any
 * specific way.  It is worth noting that #CtkCellRenderer is not a
 * #CtkWidget and cannot be treated as such.
 *
 * The primary use of a #CtkCellRenderer is for drawing a certain graphical
 * elements on a #cairo_t. Typically, one cell renderer is used to
 * draw many cells on the screen.  To this extent, it isn’t expected that a
 * CellRenderer keep any permanent state around.  Instead, any state is set
 * just prior to use using #GObjects property system.  Then, the
 * cell is measured using ctk_cell_renderer_get_size(). Finally, the cell
 * is rendered in the correct location using ctk_cell_renderer_render().
 *
 * There are a number of rules that must be followed when writing a new
 * #CtkCellRenderer.  First and foremost, it’s important that a certain set
 * of properties will always yield a cell renderer of the same size,
 * barring a #CtkStyle change.  The #CtkCellRenderer also has a number of
 * generic properties that are expected to be honored by all children.
 *
 * Beyond merely rendering a cell, cell renderers can optionally
 * provide active user interface elements. A cell renderer can be
 * “activatable” like #CtkCellRendererToggle,
 * which toggles when it gets activated by a mouse click, or it can be
 * “editable” like #CtkCellRendererText, which
 * allows the user to edit the text using a widget implementing the
 * #CtkCellEditable interface, e.g. #CtkEntry.
 * To make a cell renderer activatable or editable, you have to
 * implement the #CtkCellRendererClass.activate or
 * #CtkCellRendererClass.start_editing virtual functions, respectively.
 *
 * Many properties of #CtkCellRenderer and its subclasses have a
 * corresponding “set” property, e.g. “cell-background-set” corresponds
 * to “cell-background”. These “set” properties reflect whether a property
 * has been set or not. You should not set them independently.
 */


#define DEBUG_CELL_SIZE_REQUEST 0

static void ctk_cell_renderer_init          (CtkCellRenderer      *cell);
static void ctk_cell_renderer_class_init    (CtkCellRendererClass *class);
static void ctk_cell_renderer_get_property  (GObject              *object,
					     guint                 param_id,
					     GValue               *value,
					     GParamSpec           *pspec);
static void ctk_cell_renderer_set_property  (GObject              *object,
					     guint                 param_id,
					     const GValue         *value,
					     GParamSpec           *pspec);
static void set_cell_bg_color               (CtkCellRenderer      *cell,
					     CdkRGBA              *rgba);

/* Fallback CtkCellRenderer    implementation to use remaining ->get_size() implementations */
static CtkSizeRequestMode ctk_cell_renderer_real_get_request_mode(CtkCellRenderer         *cell);
static void ctk_cell_renderer_real_get_preferred_width           (CtkCellRenderer         *cell,
                                                                  CtkWidget               *widget,
                                                                  gint                    *minimum_size,
                                                                  gint                    *natural_size);
static void ctk_cell_renderer_real_get_preferred_height          (CtkCellRenderer         *cell,
                                                                  CtkWidget               *widget,
                                                                  gint                    *minimum_size,
                                                                  gint                    *natural_size);
static void ctk_cell_renderer_real_get_preferred_height_for_width(CtkCellRenderer         *cell,
                                                                  CtkWidget               *widget,
                                                                  gint                     width,
                                                                  gint                    *minimum_height,
                                                                  gint                    *natural_height);
static void ctk_cell_renderer_real_get_preferred_width_for_height(CtkCellRenderer         *cell,
                                                                  CtkWidget               *widget,
                                                                  gint                     height,
                                                                  gint                    *minimum_width,
                                                                  gint                    *natural_width);
static void ctk_cell_renderer_real_get_aligned_area              (CtkCellRenderer         *cell,
								  CtkWidget               *widget,
								  CtkCellRendererState     flags,
								  const CdkRectangle      *cell_area,
								  CdkRectangle            *aligned_area);


struct _CtkCellRendererPrivate
{
  gfloat xalign;
  gfloat yalign;

  gint width;
  gint height;

  guint16 xpad;
  guint16 ypad;

  guint mode                : 2;
  guint visible             : 1;
  guint is_expander         : 1;
  guint is_expanded         : 1;
  guint cell_background_set : 1;
  guint sensitive           : 1;
  guint editing             : 1;

  CdkRGBA cell_background;
};

struct _CtkCellRendererClassPrivate
{
  GType accessible_type;
};

enum {
  PROP_0,
  PROP_MODE,
  PROP_VISIBLE,
  PROP_SENSITIVE,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_XPAD,
  PROP_YPAD,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_IS_EXPANDER,
  PROP_IS_EXPANDED,
  PROP_CELL_BACKGROUND,
  PROP_CELL_BACKGROUND_CDK,
  PROP_CELL_BACKGROUND_RGBA,
  PROP_CELL_BACKGROUND_SET,
  PROP_EDITING
};

/* Signal IDs */
enum {
  EDITING_CANCELED,
  EDITING_STARTED,
  LAST_SIGNAL
};

static gint CtkCellRenderer_private_offset;
static guint  cell_renderer_signals[LAST_SIGNAL] = { 0 };

static inline gpointer
ctk_cell_renderer_get_instance_private (CtkCellRenderer *self)
{
  return (G_STRUCT_MEMBER_P (self, CtkCellRenderer_private_offset));
}

static void
ctk_cell_renderer_init (CtkCellRenderer *cell)
{
  CtkCellRendererPrivate *priv;

  cell->priv = ctk_cell_renderer_get_instance_private (cell);
  priv = cell->priv;

  priv->mode = CTK_CELL_RENDERER_MODE_INERT;
  priv->visible = TRUE;
  priv->width = -1;
  priv->height = -1;
  priv->xalign = 0.5;
  priv->yalign = 0.5;
  priv->xpad = 0;
  priv->ypad = 0;
  priv->sensitive = TRUE;
  priv->is_expander = FALSE;
  priv->is_expanded = FALSE;
  priv->editing = FALSE;
}

static void
ctk_cell_renderer_class_init (CtkCellRendererClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = ctk_cell_renderer_get_property;
  object_class->set_property = ctk_cell_renderer_set_property;

  class->render = NULL;
  class->get_size = NULL;
  class->get_request_mode               = ctk_cell_renderer_real_get_request_mode;
  class->get_preferred_width            = ctk_cell_renderer_real_get_preferred_width;
  class->get_preferred_height           = ctk_cell_renderer_real_get_preferred_height;
  class->get_preferred_width_for_height = ctk_cell_renderer_real_get_preferred_width_for_height;
  class->get_preferred_height_for_width = ctk_cell_renderer_real_get_preferred_height_for_width;
  class->get_aligned_area               = ctk_cell_renderer_real_get_aligned_area;

  /**
   * CtkCellRenderer::editing-canceled:
   * @renderer: the object which received the signal
   *
   * This signal gets emitted when the user cancels the process of editing a
   * cell.  For example, an editable cell renderer could be written to cancel
   * editing when the user presses Escape. 
   *
   * See also: ctk_cell_renderer_stop_editing().
   *
   * Since: 2.4
   */
  cell_renderer_signals[EDITING_CANCELED] =
    g_signal_new (I_("editing-canceled"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkCellRendererClass, editing_canceled),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkCellRenderer::editing-started:
   * @renderer: the object which received the signal
   * @editable: the #CtkCellEditable
   * @path: the path identifying the edited cell
   *
   * This signal gets emitted when a cell starts to be edited.
   * The intended use of this signal is to do special setup
   * on @editable, e.g. adding a #CtkEntryCompletion or setting
   * up additional columns in a #CtkComboBox.
   *
   * See ctk_cell_editable_start_editing() for information on the lifecycle of
   * the @editable and a way to do setup that doesn’t depend on the @renderer.
   *
   * Note that CTK+ doesn't guarantee that cell renderers will
   * continue to use the same kind of widget for editing in future
   * releases, therefore you should check the type of @editable
   * before doing any specific setup, as in the following example:
   * |[<!-- language="C" -->
   * static void
   * text_editing_started (CtkCellRenderer *cell,
   *                       CtkCellEditable *editable,
   *                       const gchar     *path,
   *                       gpointer         data)
   * {
   *   if (CTK_IS_ENTRY (editable)) 
   *     {
   *       CtkEntry *entry = CTK_ENTRY (editable);
   *       
   *       // ... create a CtkEntryCompletion
   *       
   *       ctk_entry_set_completion (entry, completion);
   *     }
   * }
   * ]|
   *
   * Since: 2.6
   */
  cell_renderer_signals[EDITING_STARTED] =
    g_signal_new (I_("editing-started"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkCellRendererClass, editing_started),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_STRING,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_CELL_EDITABLE,
		  G_TYPE_STRING);
  g_signal_set_va_marshaller (cell_renderer_signals[EDITING_STARTED],
                              G_TYPE_FROM_CLASS (object_class),
                              _ctk_marshal_VOID__OBJECT_STRINGv);

  g_object_class_install_property (object_class,
				   PROP_MODE,
				   g_param_spec_enum ("mode",
						      P_("mode"),
						      P_("Editable mode of the CellRenderer"),
						      CTK_TYPE_CELL_RENDERER_MODE,
						      CTK_CELL_RENDERER_MODE_INERT,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
							 P_("visible"),
							 P_("Display the cell"),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive",
							 P_("Sensitive"),
							 P_("Display the cell sensitive"),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_XALIGN,
				   g_param_spec_float ("xalign",
						       P_("xalign"),
						       P_("The x-align"),
						       0.0,
						       1.0,
						       0.5,
						       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_YALIGN,
				   g_param_spec_float ("yalign",
						       P_("yalign"),
						       P_("The y-align"),
						       0.0,
						       1.0,
						       0.5,
						       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_XPAD,
				   g_param_spec_uint ("xpad",
						      P_("xpad"),
						      P_("The xpad"),
						      0,
						      G_MAXUINT,
						      0,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_YPAD,
				   g_param_spec_uint ("ypad",
						      P_("ypad"),
						      P_("The ypad"),
						      0,
						      G_MAXUINT,
						      0,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_WIDTH,
				   g_param_spec_int ("width",
						     P_("width"),
						     P_("The fixed width"),
						     -1,
						     G_MAXINT,
						     -1,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_HEIGHT,
				   g_param_spec_int ("height",
						     P_("height"),
						     P_("The fixed height"),
						     -1,
						     G_MAXINT,
						     -1,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_IS_EXPANDER,
				   g_param_spec_boolean ("is-expander",
							 P_("Is Expander"),
							 P_("Row has children"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  g_object_class_install_property (object_class,
				   PROP_IS_EXPANDED,
				   g_param_spec_boolean ("is-expanded",
							 P_("Is Expanded"),
							 P_("Row is an expander row, and is expanded"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_CELL_BACKGROUND,
				   g_param_spec_string ("cell-background",
							P_("Cell background color name"),
							P_("Cell background color as a string"),
							NULL,
							CTK_PARAM_WRITABLE));

  /**
   * CtkCellRenderer:cell-background-cdk:
   *
   * Cell background as a #CdkColor
   */
  g_object_class_install_property (object_class,
				   PROP_CELL_BACKGROUND_CDK,
				   g_param_spec_boxed ("cell-background-cdk",
						       P_("Cell background color"),
						       P_("Cell background color as a CdkColor"),
						       CDK_TYPE_COLOR,
						       CTK_PARAM_READWRITE));

  /**
   * CtkCellRenderer:cell-background-rgba:
   *
   * Cell background as a #CdkRGBA
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
				   PROP_CELL_BACKGROUND_RGBA,
				   g_param_spec_boxed ("cell-background-rgba",
						       P_("Cell background RGBA color"),
						       P_("Cell background color as a CdkRGBA"),
						       CDK_TYPE_RGBA,
						       CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_EDITING,
				   g_param_spec_boolean ("editing",
							 P_("Editing"),
							 P_("Whether the cell renderer is currently in editing mode"),
							 FALSE,
							 CTK_PARAM_READABLE));


#define ADD_SET_PROP(propname, propval, nick, blurb) g_object_class_install_property (object_class, propval, g_param_spec_boolean (propname, nick, blurb, FALSE, CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY))

  ADD_SET_PROP ("cell-background-set", PROP_CELL_BACKGROUND_SET,
                P_("Cell background set"),
                P_("Whether the cell background color is set"));

  if (CtkCellRenderer_private_offset != 0)
    g_type_class_adjust_private_offset (class, &CtkCellRenderer_private_offset);

  ctk_cell_renderer_class_set_accessible_type (class, CTK_TYPE_RENDERER_CELL_ACCESSIBLE);
}

static void
ctk_cell_renderer_base_class_init (gpointer g_class)
{
  CtkCellRendererClass *klass = g_class;

  klass->priv = G_TYPE_CLASS_GET_PRIVATE (g_class, CTK_TYPE_CELL_RENDERER, CtkCellRendererClassPrivate);
}

GType
ctk_cell_renderer_get_type (void)
{
  static GType cell_renderer_type = 0;

  if (G_UNLIKELY (cell_renderer_type == 0))
    {
      const GTypeInfo cell_renderer_info =
      {
	sizeof (CtkCellRendererClass),
	ctk_cell_renderer_base_class_init,
        NULL,
	(GClassInitFunc) ctk_cell_renderer_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_init */
	sizeof (CtkWidget),
	0,		/* n_preallocs */
	(GInstanceInitFunc) ctk_cell_renderer_init,
	NULL,		/* value_table */
      };
      cell_renderer_type = g_type_register_static (G_TYPE_INITIALLY_UNOWNED, "CtkCellRenderer",
                                                   &cell_renderer_info, G_TYPE_FLAG_ABSTRACT);

      CtkCellRenderer_private_offset =
        g_type_add_instance_private (cell_renderer_type, sizeof (CtkCellRendererPrivate));
      g_type_add_class_private (cell_renderer_type, sizeof (CtkCellRendererClassPrivate));
    }

  return cell_renderer_type;
}

static void
ctk_cell_renderer_get_property (GObject     *object,
				guint        param_id,
				GValue      *value,
				GParamSpec  *pspec)
{
  CtkCellRenderer *cell = CTK_CELL_RENDERER (object);
  CtkCellRendererPrivate *priv = cell->priv;

  switch (param_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, priv->visible);
      break;
    case PROP_SENSITIVE:
      g_value_set_boolean (value, priv->sensitive);
      break;
    case PROP_EDITING:
      g_value_set_boolean (value, priv->editing);
      break;
    case PROP_XALIGN:
      g_value_set_float (value, priv->xalign);
      break;
    case PROP_YALIGN:
      g_value_set_float (value, priv->yalign);
      break;
    case PROP_XPAD:
      g_value_set_uint (value, priv->xpad);
      break;
    case PROP_YPAD:
      g_value_set_uint (value, priv->ypad);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, priv->height);
      break;
    case PROP_IS_EXPANDER:
      g_value_set_boolean (value, priv->is_expander);
      break;
    case PROP_IS_EXPANDED:
      g_value_set_boolean (value, priv->is_expanded);
      break;
    case PROP_CELL_BACKGROUND_CDK:
      {
	CdkColor color;

	color.red = (guint16) (priv->cell_background.red * 65535);
	color.green = (guint16) (priv->cell_background.green * 65535);
	color.blue = (guint16) (priv->cell_background.blue * 65535);

	g_value_set_boxed (value, &color);
      }
      break;
    case PROP_CELL_BACKGROUND_RGBA:
      g_value_set_boxed (value, &priv->cell_background);
      break;
    case PROP_CELL_BACKGROUND_SET:
      g_value_set_boolean (value, priv->cell_background_set);
      break;
    case PROP_CELL_BACKGROUND:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

}

static void
ctk_cell_renderer_set_property (GObject      *object,
				guint         param_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  CtkCellRenderer *cell = CTK_CELL_RENDERER (object);
  CtkCellRendererPrivate *priv = cell->priv;

  switch (param_id)
    {
    case PROP_MODE:
      if (priv->mode != g_value_get_enum (value))
        {
          priv->mode = g_value_get_enum (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VISIBLE:
      if (priv->visible != g_value_get_boolean (value))
        {
          priv->visible = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SENSITIVE:
      if (priv->sensitive != g_value_get_boolean (value))
        {
          priv->sensitive = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_XALIGN:
      if (priv->xalign != g_value_get_float (value))
        {
          priv->xalign = g_value_get_float (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_YALIGN:
      if (priv->yalign != g_value_get_float (value))
        {
          priv->yalign = g_value_get_float (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_XPAD:
      if (priv->xpad != g_value_get_uint (value))
        {
          priv->xpad = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_YPAD:
      if (priv->ypad != g_value_get_uint (value))
        {
          priv->ypad = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_WIDTH:
      if (priv->width != g_value_get_int (value))
        {
          priv->width = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HEIGHT:
      if (priv->height != g_value_get_int (value))
        {
          priv->height = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_IS_EXPANDER:
      if (priv->is_expander != g_value_get_boolean (value))
        {
          priv->is_expander = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_IS_EXPANDED:
      if (priv->is_expanded != g_value_get_boolean (value))
        {
          priv->is_expanded = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_CELL_BACKGROUND:
      {
        CdkRGBA rgba;

        if (!g_value_get_string (value))
          set_cell_bg_color (cell, NULL);
        else if (cdk_rgba_parse (&rgba, g_value_get_string (value)))
          set_cell_bg_color (cell, &rgba);
        else
          g_warning ("Don't know color '%s'", g_value_get_string (value));

        g_object_notify (object, "cell-background");
      }
      break;
    case PROP_CELL_BACKGROUND_CDK:
      {
        CdkColor *color;

        color = g_value_get_boxed (value);
        if (color)
          {
            CdkRGBA rgba;

            rgba.red = color->red / 65535.;
            rgba.green = color->green / 65535.;
            rgba.blue = color->blue / 65535.;
            rgba.alpha = 1;

            set_cell_bg_color (cell, &rgba);
          }
        else
          {
            set_cell_bg_color (cell, NULL);
          }
        g_object_notify (object, "cell-background-cdk");
      }
      break;
    case PROP_CELL_BACKGROUND_RGBA:
      set_cell_bg_color (cell, g_value_get_boxed (value));
      break;
    case PROP_CELL_BACKGROUND_SET:
      if (priv->cell_background_set != g_value_get_boolean (value))
        {
          priv->cell_background_set = g_value_get_boolean (value);
          g_object_notify (object, "cell-background-set");
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_cell_bg_color (CtkCellRenderer *cell,
                   CdkRGBA         *rgba)
{
  CtkCellRendererPrivate *priv = cell->priv;

  if (rgba)
    {
      if (!priv->cell_background_set)
        {
          priv->cell_background_set = TRUE;
          g_object_notify (G_OBJECT (cell), "cell-background-set");
        }

      priv->cell_background = *rgba;
    }
  else
    {
      if (priv->cell_background_set)
        {
	  priv->cell_background_set = FALSE;
	  g_object_notify (G_OBJECT (cell), "cell-background-set");
	}
    }
  g_object_notify (G_OBJECT (cell), "cell-background-rgba");
}

/**
 * ctk_cell_renderer_get_size:
 * @cell: a #CtkCellRenderer
 * @widget: the widget the renderer is rendering to
 * @cell_area: (allow-none): The area a cell will be allocated, or %NULL
 * @x_offset: (out) (allow-none): location to return x offset of cell relative to @cell_area, or %NULL
 * @y_offset: (out) (allow-none): location to return y offset of cell relative to @cell_area, or %NULL
 * @width: (out) (allow-none): location to return width needed to render a cell, or %NULL
 * @height: (out) (allow-none): location to return height needed to render a cell, or %NULL
 *
 * Obtains the width and height needed to render the cell. Used by view 
 * widgets to determine the appropriate size for the cell_area passed to
 * ctk_cell_renderer_render().  If @cell_area is not %NULL, fills in the
 * x and y offsets (if set) of the cell relative to this location. 
 *
 * Please note that the values set in @width and @height, as well as those 
 * in @x_offset and @y_offset are inclusive of the xpad and ypad properties.
 *
 *
 * Deprecated: 3.0: Use ctk_cell_renderer_get_preferred_size() instead.
 **/
void
ctk_cell_renderer_get_size (CtkCellRenderer    *cell,
			    CtkWidget          *widget,
			    const CdkRectangle *cell_area,
			    gint               *x_offset,
			    gint               *y_offset,
			    gint               *width,
			    gint               *height)
{
  CtkRequisition request;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  ctk_cell_renderer_get_preferred_size (cell, widget, &request, NULL);

  if (width)
    *width = request.width;
  
  if (height)
    *height = request.height;

  if (cell_area)
    _ctk_cell_renderer_calc_offset (cell, cell_area, ctk_widget_get_direction (widget),
                                    request.width, request.height, x_offset, y_offset);
}

/**
 * ctk_cell_renderer_render:
 * @cell: a #CtkCellRenderer
 * @cr: a cairo context to draw to
 * @widget: the widget owning @window
 * @background_area: entire cell area (including tree expanders and maybe 
 *    padding on the sides)
 * @cell_area: area normally rendered by a cell renderer
 * @flags: flags that affect rendering
 *
 * Invokes the virtual render function of the #CtkCellRenderer. The three
 * passed-in rectangles are areas in @cr. Most renderers will draw within
 * @cell_area; the xalign, yalign, xpad, and ypad fields of the #CtkCellRenderer
 * should be honored with respect to @cell_area. @background_area includes the
 * blank space around the cell, and also the area containing the tree expander;
 * so the @background_area rectangles for all cells tile to cover the entire
 * @window.
 **/
void
ctk_cell_renderer_render (CtkCellRenderer      *cell,
                          cairo_t              *cr,
                          CtkWidget            *widget,
                          const CdkRectangle   *background_area,
                          const CdkRectangle   *cell_area,
                          CtkCellRendererState  flags)
{
  gboolean selected = FALSE;
  CtkCellRendererPrivate *priv = cell->priv;
  CtkStyleContext *context;
  CtkStateFlags state;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (CTK_CELL_RENDERER_GET_CLASS (cell)->render != NULL);
  g_return_if_fail (cr != NULL);

  selected = (flags & CTK_CELL_RENDERER_SELECTED) == CTK_CELL_RENDERER_SELECTED;

  cairo_save (cr);

  if (priv->cell_background_set && !selected)
    {
      cdk_cairo_rectangle (cr, background_area);
      cdk_cairo_set_source_rgba (cr, &priv->cell_background);
      cairo_fill (cr);
    }

  cdk_cairo_rectangle (cr, background_area);
  cairo_clip (cr);

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save (context);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_CELL);

  state = ctk_cell_renderer_get_state (cell, widget, flags);
  ctk_style_context_set_state (context, state);

  CTK_CELL_RENDERER_GET_CLASS (cell)->render (cell,
                                              cr,
					      widget,
					      background_area,
					      cell_area,
					      flags);
  ctk_style_context_restore (context);
  cairo_restore (cr);
}

/**
 * ctk_cell_renderer_activate:
 * @cell: a #CtkCellRenderer
 * @event: a #CdkEvent
 * @widget: widget that received the event
 * @path: widget-dependent string representation of the event location; 
 *    e.g. for #CtkTreeView, a string representation of #CtkTreePath
 * @background_area: background area as passed to ctk_cell_renderer_render()
 * @cell_area: cell area as passed to ctk_cell_renderer_render()
 * @flags: render flags
 *
 * Passes an activate event to the cell renderer for possible processing.  
 * Some cell renderers may use events; for example, #CtkCellRendererToggle 
 * toggles when it gets a mouse click.
 *
 * Returns: %TRUE if the event was consumed/handled
 **/
gboolean
ctk_cell_renderer_activate (CtkCellRenderer      *cell,
			    CdkEvent             *event,
			    CtkWidget            *widget,
			    const gchar          *path,
			    const CdkRectangle   *background_area,
			    const CdkRectangle   *cell_area,
			    CtkCellRendererState  flags)
{
  CtkCellRendererPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_RENDERER (cell), FALSE);

  priv = cell->priv;

  if (priv->mode != CTK_CELL_RENDERER_MODE_ACTIVATABLE)
    return FALSE;

  if (CTK_CELL_RENDERER_GET_CLASS (cell)->activate == NULL)
    return FALSE;

  return CTK_CELL_RENDERER_GET_CLASS (cell)->activate (cell,
						       event,
						       widget,
						       path,
						       (CdkRectangle *) background_area,
						       (CdkRectangle *) cell_area,
						       flags);
}

/**
 * ctk_cell_renderer_start_editing:
 * @cell: a #CtkCellRenderer
 * @event: (nullable): a #CdkEvent
 * @widget: widget that received the event
 * @path: widget-dependent string representation of the event location;
 *    e.g. for #CtkTreeView, a string representation of #CtkTreePath
 * @background_area: background area as passed to ctk_cell_renderer_render()
 * @cell_area: cell area as passed to ctk_cell_renderer_render()
 * @flags: render flags
 *
 * Starts editing the contents of this @cell, through a new #CtkCellEditable
 * widget created by the #CtkCellRendererClass.start_editing virtual function.
 *
 * Returns: (nullable) (transfer none): A new #CtkCellEditable for editing this
 *   @cell, or %NULL if editing is not possible
 **/
CtkCellEditable *
ctk_cell_renderer_start_editing (CtkCellRenderer      *cell,
				 CdkEvent             *event,
				 CtkWidget            *widget,
				 const gchar          *path,
				 const CdkRectangle   *background_area,
				 const CdkRectangle   *cell_area,
				 CtkCellRendererState  flags)

{
  CtkCellRendererPrivate *priv;
  CtkCellEditable *editable;

  g_return_val_if_fail (CTK_IS_CELL_RENDERER (cell), NULL);

  priv = cell->priv;

  if (priv->mode != CTK_CELL_RENDERER_MODE_EDITABLE)
    return NULL;

  if (CTK_CELL_RENDERER_GET_CLASS (cell)->start_editing == NULL)
    return NULL;

  editable = CTK_CELL_RENDERER_GET_CLASS (cell)->start_editing (cell,
								event,
								widget,
								path,
								(CdkRectangle *) background_area,
								(CdkRectangle *) cell_area,
								flags);
  if (editable == NULL)
    return NULL;

  ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (editable)),
                               CTK_STYLE_CLASS_CELL);

  g_signal_emit (cell, 
		 cell_renderer_signals[EDITING_STARTED], 0,
		 editable, path);

  priv->editing = TRUE;

  return editable;
}

/**
 * ctk_cell_renderer_set_fixed_size:
 * @cell: A #CtkCellRenderer
 * @width: the width of the cell renderer, or -1
 * @height: the height of the cell renderer, or -1
 *
 * Sets the renderer size to be explicit, independent of the properties set.
 **/
void
ctk_cell_renderer_set_fixed_size (CtkCellRenderer *cell,
				  gint             width,
				  gint             height)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (width >= -1 && height >= -1);

  priv = cell->priv;

  if ((width != priv->width) || (height != priv->height))
    {
      g_object_freeze_notify (G_OBJECT (cell));

      if (width != priv->width)
        {
          priv->width = width;
          g_object_notify (G_OBJECT (cell), "width");
        }

      if (height != priv->height)
        {
          priv->height = height;
          g_object_notify (G_OBJECT (cell), "height");
        }

      g_object_thaw_notify (G_OBJECT (cell));
    }
}

/**
 * ctk_cell_renderer_get_fixed_size:
 * @cell: A #CtkCellRenderer
 * @width: (out) (allow-none): location to fill in with the fixed width of the cell, or %NULL
 * @height: (out) (allow-none): location to fill in with the fixed height of the cell, or %NULL
 *
 * Fills in @width and @height with the appropriate size of @cell.
 **/
void
ctk_cell_renderer_get_fixed_size (CtkCellRenderer *cell,
				  gint            *width,
				  gint            *height)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (width)
    *width = priv->width;
  if (height)
    *height = priv->height;
}

/**
 * ctk_cell_renderer_set_alignment:
 * @cell: A #CtkCellRenderer
 * @xalign: the x alignment of the cell renderer
 * @yalign: the y alignment of the cell renderer
 *
 * Sets the renderer’s alignment within its available space.
 *
 * Since: 2.18
 **/
void
ctk_cell_renderer_set_alignment (CtkCellRenderer *cell,
                                 gfloat           xalign,
                                 gfloat           yalign)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (xalign >= 0.0 && xalign <= 1.0);
  g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

  priv = cell->priv;

  if ((xalign != priv->xalign) || (yalign != priv->yalign))
    {
      g_object_freeze_notify (G_OBJECT (cell));

      if (xalign != priv->xalign)
        {
          priv->xalign = xalign;
          g_object_notify (G_OBJECT (cell), "xalign");
        }

      if (yalign != priv->yalign)
        {
          priv->yalign = yalign;
          g_object_notify (G_OBJECT (cell), "yalign");
        }

      g_object_thaw_notify (G_OBJECT (cell));
    }
}

/**
 * ctk_cell_renderer_get_alignment:
 * @cell: A #CtkCellRenderer
 * @xalign: (out) (allow-none): location to fill in with the x alignment of the cell, or %NULL
 * @yalign: (out) (allow-none): location to fill in with the y alignment of the cell, or %NULL
 *
 * Fills in @xalign and @yalign with the appropriate values of @cell.
 *
 * Since: 2.18
 **/
void
ctk_cell_renderer_get_alignment (CtkCellRenderer *cell,
                                 gfloat          *xalign,
                                 gfloat          *yalign)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (xalign)
    *xalign = priv->xalign;
  if (yalign)
    *yalign = priv->yalign;
}

/**
 * ctk_cell_renderer_set_padding:
 * @cell: A #CtkCellRenderer
 * @xpad: the x padding of the cell renderer
 * @ypad: the y padding of the cell renderer
 *
 * Sets the renderer’s padding.
 *
 * Since: 2.18
 **/
void
ctk_cell_renderer_set_padding (CtkCellRenderer *cell,
                               gint             xpad,
                               gint             ypad)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (xpad >= 0 && ypad >= 0);

  priv = cell->priv;

  if ((xpad != priv->xpad) || (ypad != priv->ypad))
    {
      g_object_freeze_notify (G_OBJECT (cell));

      if (xpad != priv->xpad)
        {
          priv->xpad = xpad;
          g_object_notify (G_OBJECT (cell), "xpad");
        }

      if (ypad != priv->ypad)
        {
          priv->ypad = ypad;
          g_object_notify (G_OBJECT (cell), "ypad");
        }

      g_object_thaw_notify (G_OBJECT (cell));
    }
}

/**
 * ctk_cell_renderer_get_padding:
 * @cell: A #CtkCellRenderer
 * @xpad: (out) (allow-none): location to fill in with the x padding of the cell, or %NULL
 * @ypad: (out) (allow-none): location to fill in with the y padding of the cell, or %NULL
 *
 * Fills in @xpad and @ypad with the appropriate values of @cell.
 *
 * Since: 2.18
 **/
void
ctk_cell_renderer_get_padding (CtkCellRenderer *cell,
                               gint            *xpad,
                               gint            *ypad)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (xpad)
    *xpad = priv->xpad;
  if (ypad)
    *ypad = priv->ypad;
}

/**
 * ctk_cell_renderer_set_visible:
 * @cell: A #CtkCellRenderer
 * @visible: the visibility of the cell
 *
 * Sets the cell renderer’s visibility.
 *
 * Since: 2.18
 **/
void
ctk_cell_renderer_set_visible (CtkCellRenderer *cell,
                               gboolean         visible)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (priv->visible != visible)
    {
      priv->visible = visible ? TRUE : FALSE;
      g_object_notify (G_OBJECT (cell), "visible");
    }
}

/**
 * ctk_cell_renderer_get_visible:
 * @cell: A #CtkCellRenderer
 *
 * Returns the cell renderer’s visibility.
 *
 * Returns: %TRUE if the cell renderer is visible
 *
 * Since: 2.18
 */
gboolean
ctk_cell_renderer_get_visible (CtkCellRenderer *cell)
{
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (cell), FALSE);

  return cell->priv->visible;
}

/**
 * ctk_cell_renderer_set_sensitive:
 * @cell: A #CtkCellRenderer
 * @sensitive: the sensitivity of the cell
 *
 * Sets the cell renderer’s sensitivity.
 *
 * Since: 2.18
 **/
void
ctk_cell_renderer_set_sensitive (CtkCellRenderer *cell,
                                 gboolean         sensitive)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (priv->sensitive != sensitive)
    {
      priv->sensitive = sensitive ? TRUE : FALSE;
      g_object_notify (G_OBJECT (cell), "sensitive");
    }
}

/**
 * ctk_cell_renderer_get_sensitive:
 * @cell: A #CtkCellRenderer
 *
 * Returns the cell renderer’s sensitivity.
 *
 * Returns: %TRUE if the cell renderer is sensitive
 *
 * Since: 2.18
 */
gboolean
ctk_cell_renderer_get_sensitive (CtkCellRenderer *cell)
{
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (cell), FALSE);

  return cell->priv->sensitive;
}


/**
 * ctk_cell_renderer_is_activatable:
 * @cell: A #CtkCellRenderer
 *
 * Checks whether the cell renderer can do something when activated.
 *
 * Returns: %TRUE if the cell renderer can do anything when activated
 *
 * Since: 3.0
 */
gboolean
ctk_cell_renderer_is_activatable (CtkCellRenderer *cell)
{
  CtkCellRendererPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_RENDERER (cell), FALSE);

  priv = cell->priv;

  return (priv->visible &&
          (priv->mode == CTK_CELL_RENDERER_MODE_EDITABLE ||
           priv->mode == CTK_CELL_RENDERER_MODE_ACTIVATABLE));
}


/**
 * ctk_cell_renderer_stop_editing:
 * @cell: A #CtkCellRenderer
 * @canceled: %TRUE if the editing has been canceled
 * 
 * Informs the cell renderer that the editing is stopped.
 * If @canceled is %TRUE, the cell renderer will emit the 
 * #CtkCellRenderer::editing-canceled signal. 
 *
 * This function should be called by cell renderer implementations 
 * in response to the #CtkCellEditable::editing-done signal of 
 * #CtkCellEditable.
 *
 * Since: 2.6
 **/
void
ctk_cell_renderer_stop_editing (CtkCellRenderer *cell,
				gboolean         canceled)
{
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (priv->editing)
    {
      priv->editing = FALSE;
      if (canceled)
	g_signal_emit (cell, cell_renderer_signals[EDITING_CANCELED], 0);
    }
}

static void
ctk_cell_renderer_real_get_preferred_size (CtkCellRenderer   *cell,
                                           CtkWidget         *widget,
                                           CtkOrientation     orientation,
                                           gint              *minimum_size,
                                           gint              *natural_size)
{
  CtkRequisition min_req;

  /* Fallback on the old API to get the size. */
  if (CTK_CELL_RENDERER_GET_CLASS (cell)->get_size)
    CTK_CELL_RENDERER_GET_CLASS (cell)->get_size (CTK_CELL_RENDERER (cell), widget, NULL, NULL, NULL,
						  &min_req.width, &min_req.height);
  else
    {
      min_req.width = 0;
      min_req.height = 0;
    }

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (minimum_size)
	*minimum_size = min_req.width;

      if (natural_size)
	*natural_size = min_req.width;
    }
  else
    {
      if (minimum_size)
	*minimum_size = min_req.height;

      if (natural_size)
	*natural_size = min_req.height;
    }
}

static CtkSizeRequestMode 
ctk_cell_renderer_real_get_request_mode (CtkCellRenderer *cell G_GNUC_UNUSED)
{
  /* By default cell renderers are height-for-width. */
  return CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
ctk_cell_renderer_real_get_preferred_width (CtkCellRenderer *cell,
                                            CtkWidget       *widget,
                                            gint            *minimum_size,
                                            gint            *natural_size)
{
  ctk_cell_renderer_real_get_preferred_size (cell, widget, CTK_ORIENTATION_HORIZONTAL, 
                                             minimum_size, natural_size);
}

static void
ctk_cell_renderer_real_get_preferred_height (CtkCellRenderer *cell,
                                             CtkWidget       *widget,
                                             gint            *minimum_size,
                                             gint            *natural_size)
{
  ctk_cell_renderer_real_get_preferred_size (cell, widget, CTK_ORIENTATION_VERTICAL, 
                                             minimum_size, natural_size);
}


static void
ctk_cell_renderer_real_get_preferred_height_for_width (CtkCellRenderer *cell,
                                                       CtkWidget       *widget,
                                                       gint             width G_GNUC_UNUSED,
                                                       gint            *minimum_height,
                                                       gint            *natural_height)
{
  /* Fall back on the height reported from ->get_size() */
  ctk_cell_renderer_get_preferred_height (cell, widget, minimum_height, natural_height);
}

static void
ctk_cell_renderer_real_get_preferred_width_for_height (CtkCellRenderer *cell,
                                                       CtkWidget       *widget,
                                                       gint             height G_GNUC_UNUSED,
                                                       gint            *minimum_width,
                                                       gint            *natural_width)
{
  /* Fall back on the width reported from ->get_size() */
  ctk_cell_renderer_get_preferred_width (cell, widget, minimum_width, natural_width);
}


/* Default implementation assumes that a cell renderer will never use more
 * space than its natural size (this is fine for toggles and pixbufs etc
 * but needs to be overridden from wrapping/ellipsizing text renderers) */
static void
ctk_cell_renderer_real_get_aligned_area (CtkCellRenderer      *cell,
					 CtkWidget            *widget,
					 CtkCellRendererState  flags G_GNUC_UNUSED,
					 const CdkRectangle   *cell_area,
					 CdkRectangle         *aligned_area)
{
  gint opposite_size, x_offset, y_offset;
  gint natural_size;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (aligned_area != NULL);

  *aligned_area = *cell_area;

  /* Trim up the aligned size */
  if (ctk_cell_renderer_get_request_mode (cell) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
    {
      ctk_cell_renderer_get_preferred_width (cell, widget, 
					     NULL, &natural_size);

      aligned_area->width = MIN (aligned_area->width, natural_size);

      ctk_cell_renderer_get_preferred_height_for_width (cell, widget, 
							aligned_area->width, 
							NULL, &opposite_size);

      aligned_area->height = MIN (opposite_size, aligned_area->height);
    }
  else
    {
      ctk_cell_renderer_get_preferred_height (cell, widget, 
					      NULL, &natural_size);

      aligned_area->height = MIN (aligned_area->width, natural_size);

      ctk_cell_renderer_get_preferred_width_for_height (cell, widget, 
							aligned_area->height, 
							NULL, &opposite_size);

      aligned_area->width = MIN (opposite_size, aligned_area->width);
    }

  /* offset the cell position */
  _ctk_cell_renderer_calc_offset (cell, cell_area, 
				  ctk_widget_get_direction (widget),
				  aligned_area->width, 
				  aligned_area->height,
				  &x_offset, &y_offset);

  aligned_area->x += x_offset;
  aligned_area->y += y_offset;
}


/* An internal convenience function for some containers to peek at the
 * cell alignment in a target allocation (used to draw focus and align
 * cells in the icon view).
 *
 * Note this is only a trivial “align * (allocation - request)” operation.
 */
void
_ctk_cell_renderer_calc_offset    (CtkCellRenderer      *cell,
				   const CdkRectangle   *cell_area,
				   CtkTextDirection      direction,
				   gint                  width,
				   gint                  height,
				   gint                 *x_offset,
				   gint                 *y_offset)
{ 
  CtkCellRendererPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (x_offset || y_offset);

  priv = cell->priv;

  if (x_offset)
    {
      *x_offset = (((direction == CTK_TEXT_DIR_RTL) ?
		    (1.0 - priv->xalign) : priv->xalign) * 
		   (cell_area->width - width));
      *x_offset = MAX (*x_offset, 0);
    }
  if (y_offset)
    {
      *y_offset = (priv->yalign *
		   (cell_area->height - height));
      *y_offset = MAX (*y_offset, 0);
    }
}

/**
 * ctk_cell_renderer_get_request_mode:
 * @cell: a #CtkCellRenderer    instance
 *
 * Gets whether the cell renderer prefers a height-for-width layout
 * or a width-for-height layout.
 *
 * Returns: The #CtkSizeRequestMode preferred by this renderer.
 *
 * Since: 3.0
 */
CtkSizeRequestMode
ctk_cell_renderer_get_request_mode (CtkCellRenderer *cell)
{
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (cell), FALSE);

  return CTK_CELL_RENDERER_GET_CLASS (cell)->get_request_mode (cell);
}

/**
 * ctk_cell_renderer_get_preferred_width:
 * @cell: a #CtkCellRenderer instance
 * @widget: the #CtkWidget this cell will be rendering to
 * @minimum_size: (out) (allow-none): location to store the minimum size, or %NULL
 * @natural_size: (out) (allow-none): location to store the natural size, or %NULL
 *
 * Retreives a renderer’s natural size when rendered to @widget.
 *
 * Since: 3.0
 */
void
ctk_cell_renderer_get_preferred_width (CtkCellRenderer *cell,
                                       CtkWidget       *widget,
                                       gint            *minimum_size,
                                       gint            *natural_size)
{
  CtkCellRendererClass *klass;
  gint width;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_size || NULL != natural_size);

  ctk_cell_renderer_get_fixed_size (CTK_CELL_RENDERER (cell), &width, NULL);

  if (width < 0)
    {
      klass = CTK_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_width (cell, widget, minimum_size, natural_size);
    }
  else
    {
      if (minimum_size)
	*minimum_size = width;
      if (natural_size)
	*natural_size = width;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s returning minimum width: %d and natural width: %d",
	     G_OBJECT_TYPE_NAME (cell), 
	     minimum_size ? *minimum_size : 20000, 
	     natural_size ? *natural_size : 20000);
#endif
}


/**
 * ctk_cell_renderer_get_preferred_height:
 * @cell: a #CtkCellRenderer instance
 * @widget: the #CtkWidget this cell will be rendering to
 * @minimum_size: (out) (allow-none): location to store the minimum size, or %NULL
 * @natural_size: (out) (allow-none): location to store the natural size, or %NULL
 *
 * Retreives a renderer’s natural size when rendered to @widget.
 *
 * Since: 3.0
 */
void
ctk_cell_renderer_get_preferred_height (CtkCellRenderer *cell,
                                        CtkWidget       *widget,
                                        gint            *minimum_size,
                                        gint            *natural_size)
{
  CtkCellRendererClass *klass;
  gint height;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_size || NULL != natural_size);

  ctk_cell_renderer_get_fixed_size (CTK_CELL_RENDERER (cell), NULL, &height);

  if (height < 0)
    {
      klass = CTK_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_height (cell, widget, minimum_size, natural_size);
    }
  else
    {
      if (minimum_size)
	*minimum_size = height;
      if (natural_size)
	*natural_size = height;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s returning minimum height: %d and natural height: %d",
	     G_OBJECT_TYPE_NAME (cell), 
	     minimum_size ? *minimum_size : 20000, 
	     natural_size ? *natural_size : 20000);
#endif
}


/**
 * ctk_cell_renderer_get_preferred_width_for_height:
 * @cell: a #CtkCellRenderer instance
 * @widget: the #CtkWidget this cell will be rendering to
 * @height: the size which is available for allocation
 * @minimum_width: (out) (allow-none): location for storing the minimum size, or %NULL
 * @natural_width: (out) (allow-none): location for storing the preferred size, or %NULL
 *
 * Retreives a cell renderers’s minimum and natural width if it were rendered to 
 * @widget with the specified @height.
 *
 * Since: 3.0
 */
void
ctk_cell_renderer_get_preferred_width_for_height (CtkCellRenderer *cell,
                                                  CtkWidget       *widget,
                                                  gint             height,
                                                  gint            *minimum_width,
                                                  gint            *natural_width)
{
  CtkCellRendererClass *klass;
  gint width;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_width || NULL != natural_width);

  ctk_cell_renderer_get_fixed_size (CTK_CELL_RENDERER (cell), &width, NULL);

  if (width < 0)
    {
      klass = CTK_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_width_for_height (cell, widget, height, minimum_width, natural_width);
    }
  else
    {
      if (minimum_width)
	*minimum_width = width;
      if (natural_width)
	*natural_width = width;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s width for height: %d is minimum %d and natural: %d",
	     G_OBJECT_TYPE_NAME (cell), height,
	     minimum_width ? *minimum_width : 20000, 
	     natural_width ? *natural_width : 20000);
#endif
}

/**
 * ctk_cell_renderer_get_preferred_height_for_width:
 * @cell: a #CtkCellRenderer instance
 * @widget: the #CtkWidget this cell will be rendering to
 * @width: the size which is available for allocation
 * @minimum_height: (out) (allow-none): location for storing the minimum size, or %NULL
 * @natural_height: (out) (allow-none): location for storing the preferred size, or %NULL
 *
 * Retreives a cell renderers’s minimum and natural height if it were rendered to 
 * @widget with the specified @width.
 *
 * Since: 3.0
 */
void
ctk_cell_renderer_get_preferred_height_for_width (CtkCellRenderer *cell,
                                                  CtkWidget       *widget,
                                                  gint             width,
                                                  gint            *minimum_height,
                                                  gint            *natural_height)
{
  CtkCellRendererClass *klass;
  gint height;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_height || NULL != natural_height);

  ctk_cell_renderer_get_fixed_size (CTK_CELL_RENDERER (cell), NULL, &height);

  if (height < 0)
    {
      klass = CTK_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_height_for_width (cell, widget, width, minimum_height, natural_height);
    }
  else
    {
      if (minimum_height)
	*minimum_height = height;
      if (natural_height)
	*natural_height = height;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s height for width: %d is minimum %d and natural: %d",
	     G_OBJECT_TYPE_NAME (cell), width,
	     minimum_height ? *minimum_height : 20000, 
	     natural_height ? *natural_height : 20000);
#endif
}

/**
 * ctk_cell_renderer_get_preferred_size:
 * @cell: a #CtkCellRenderer instance
 * @widget: the #CtkWidget this cell will be rendering to
 * @minimum_size: (out) (allow-none): location for storing the minimum size, or %NULL
 * @natural_size: (out) (allow-none): location for storing the natural size, or %NULL
 *
 * Retrieves the minimum and natural size of a cell taking
 * into account the widget’s preference for height-for-width management.
 *
 * Since: 3.0
 */
void
ctk_cell_renderer_get_preferred_size (CtkCellRenderer *cell,
                                      CtkWidget       *widget,
                                      CtkRequisition  *minimum_size,
                                      CtkRequisition  *natural_size)
{
  gint min_width, nat_width;
  gint min_height, nat_height;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  if (ctk_cell_renderer_get_request_mode (cell) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
    {
      ctk_cell_renderer_get_preferred_width (cell, widget, &min_width, &nat_width);

      if (minimum_size)
	{
	  minimum_size->width = min_width;
	  ctk_cell_renderer_get_preferred_height_for_width (cell, widget, min_width,
                                                            &minimum_size->height, NULL);
	}

      if (natural_size)
	{
	  natural_size->width = nat_width;
	  ctk_cell_renderer_get_preferred_height_for_width (cell, widget, nat_width,
                                                            NULL, &natural_size->height);
	}
    }
  else /* CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT */
    {
      ctk_cell_renderer_get_preferred_height (cell, widget, &min_height, &nat_height);

      if (minimum_size)
	{
	  minimum_size->height = min_height;
	  ctk_cell_renderer_get_preferred_width_for_height (cell, widget, min_height,
                                                            &minimum_size->width, NULL);
	}

      if (natural_size)
	{
	  natural_size->height = nat_height;
	  ctk_cell_renderer_get_preferred_width_for_height (cell, widget, nat_height,
                                                            NULL, &natural_size->width);
	}
    }
}

/**
 * ctk_cell_renderer_get_aligned_area:
 * @cell: a #CtkCellRenderer instance
 * @widget: the #CtkWidget this cell will be rendering to
 * @flags: render flags
 * @cell_area: cell area which would be passed to ctk_cell_renderer_render()
 * @aligned_area: (out): the return location for the space inside @cell_area
 *                that would acually be used to render.
 *
 * Gets the aligned area used by @cell inside @cell_area. Used for finding
 * the appropriate edit and focus rectangle.
 *
 * Since: 3.0
 */
void
ctk_cell_renderer_get_aligned_area (CtkCellRenderer      *cell,
				    CtkWidget            *widget,
				    CtkCellRendererState  flags,
				    const CdkRectangle   *cell_area,
				    CdkRectangle         *aligned_area)
{
  CtkCellRendererClass *klass;

  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (aligned_area != NULL);

  klass = CTK_CELL_RENDERER_GET_CLASS (cell);
  klass->get_aligned_area (cell, widget, flags, cell_area, aligned_area);

  g_assert (aligned_area->x >= cell_area->x && aligned_area->x <= cell_area->x + cell_area->width);
  g_assert (aligned_area->y >= cell_area->y && aligned_area->y <= cell_area->y + cell_area->height);
  g_assert ((aligned_area->x - cell_area->x) + aligned_area->width <= cell_area->width);
  g_assert ((aligned_area->y - cell_area->y) + aligned_area->height <= cell_area->height);
}

/**
 * ctk_cell_renderer_get_state:
 * @cell: (nullable): a #CtkCellRenderer, or %NULL
 * @widget: (nullable): a #CtkWidget, or %NULL
 * @cell_state: cell renderer state
 *
 * Translates the cell renderer state to #CtkStateFlags,
 * based on the cell renderer and widget sensitivity, and
 * the given #CtkCellRendererState.
 *
 * Returns: the widget state flags applying to @cell
 *
 * Since: 3.0
 **/
CtkStateFlags
ctk_cell_renderer_get_state (CtkCellRenderer      *cell,
			     CtkWidget            *widget,
			     CtkCellRendererState  cell_state)
{
  CtkStateFlags state = 0;

  g_return_val_if_fail (!cell || CTK_IS_CELL_RENDERER (cell), 0);
  g_return_val_if_fail (!widget || CTK_IS_WIDGET (widget), 0);

  if (widget)
    state |= ctk_widget_get_state_flags (widget);

  state &= ~(CTK_STATE_FLAG_FOCUSED | CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_SELECTED | CTK_STATE_FLAG_DROP_ACTIVE);

  if ((state & CTK_STATE_FLAG_INSENSITIVE) != 0 ||
      (cell && !ctk_cell_renderer_get_sensitive (cell)) ||
      (cell_state & CTK_CELL_RENDERER_INSENSITIVE) != 0)
    {
      state |= CTK_STATE_FLAG_INSENSITIVE;
    }
  else
    {
      if ((widget && ctk_widget_has_focus (widget)) &&
          (cell_state & CTK_CELL_RENDERER_FOCUSED) != 0)
        state |= CTK_STATE_FLAG_FOCUSED;

      if ((cell_state & CTK_CELL_RENDERER_PRELIT) != 0)
        state |= CTK_STATE_FLAG_PRELIGHT;
    }

  if ((cell_state & CTK_CELL_RENDERER_SELECTED) != 0)
    state |= CTK_STATE_FLAG_SELECTED;

  return state;
}

/**
 * ctk_cell_renderer_class_set_accessible_type:
 * @renderer_class: class to set the accessible type for
 * @type: The object type that implements the accessible for @widget_class.
 *     The type must be a subtype of #CtkRendererCellAccessible
 *
 * Sets the type to be used for creating accessibles for cells rendered by
 * cell renderers of @renderer_class. Note that multiple accessibles will
 * be created.
 *
 * This function should only be called from class init functions of cell
 * renderers.
 **/
void
ctk_cell_renderer_class_set_accessible_type (CtkCellRendererClass *renderer_class,
                                             GType                 type)
{
  CtkCellRendererClassPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_RENDERER_CLASS (renderer_class));
  g_return_if_fail (g_type_is_a (type, CTK_TYPE_RENDERER_CELL_ACCESSIBLE));

  priv = renderer_class->priv;

  priv->accessible_type = type;
}

GType
_ctk_cell_renderer_get_accessible_type (CtkCellRenderer *renderer)
{
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (renderer), CTK_TYPE_RENDERER_CELL_ACCESSIBLE);

  return CTK_CELL_RENDERER_GET_CLASS (renderer)->priv->accessible_type;
}

