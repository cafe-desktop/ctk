/* GtkToolPalette -- A tool palette with categories and DnD support
 * Copyright (C) 2008  Openismus GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Mathias Hasselmann
 *      Jan Arne Petersen
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include "ctktoolpaletteprivate.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkcssnodeprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidgetprivate.h"


#define ANIMATION_TIMEOUT        50
#define ANIMATION_DURATION      (ANIMATION_TIMEOUT * 4)
#define DEFAULT_ANIMATION_STATE  TRUE
#define DEFAULT_EXPANDER_SIZE    16
#define DEFAULT_HEADER_SPACING   2

#define DEFAULT_LABEL            ""
#define DEFAULT_COLLAPSED        FALSE
#define DEFAULT_ELLIPSIZE        PANGO_ELLIPSIZE_NONE

/**
 * SECTION:ctktoolitemgroup
 * @Short_description: A sub container used in a tool palette
 * @Title: GtkToolItemGroup
 *
 * A #GtkToolItemGroup is used together with #GtkToolPalette to add
 * #GtkToolItems to a palette like container with different
 * categories and drag and drop support.
 *
 * # CSS nodes
 *
 * GtkToolItemGroup has a single CSS node named toolitemgroup.
 *
 * Since: 2.20
 */

enum
{
  PROP_NONE,
  PROP_LABEL,
  PROP_LABEL_WIDGET,
  PROP_COLLAPSED,
  PROP_ELLIPSIZE,
  PROP_RELIEF
};

enum
{
  CHILD_PROP_NONE,
  CHILD_PROP_HOMOGENEOUS,
  CHILD_PROP_EXPAND,
  CHILD_PROP_FILL,
  CHILD_PROP_NEW_ROW,
  CHILD_PROP_POSITION,
};

typedef struct _GtkToolItemGroupChild GtkToolItemGroupChild;

struct _GtkToolItemGroupPrivate
{
  GtkWidget         *header;
  GtkWidget         *label_widget;

  GtkCssNode        *arrow_node;

  GList             *children;

  gint64             animation_start;
  GSource           *animation_timeout;
  gint               expander_size;
  gint               header_spacing;

  gulong             focus_set_id;
  GtkWidget         *toplevel;

  GtkSettings       *settings;
  gulong             settings_connection;

  PangoEllipsizeMode ellipsize;

  guint              animation : 1;
  guint              collapsed : 1;
};

struct _GtkToolItemGroupChild
{
  GtkToolItem *item;

  guint        homogeneous : 1;
  guint        expand : 1;
  guint        fill : 1;
  guint        new_row : 1;
};

static void ctk_tool_item_group_tool_shell_init (GtkToolShellIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkToolItemGroup, ctk_tool_item_group, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (GtkToolItemGroup)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TOOL_SHELL,
                                                ctk_tool_item_group_tool_shell_init));

static GtkWidget*
ctk_tool_item_group_get_alignment (GtkToolItemGroup *group)
{
  return ctk_bin_get_child (CTK_BIN (group->priv->header));
}

static GtkOrientation
ctk_tool_item_group_get_orientation (GtkToolShell *shell)
{
  GtkWidget *parent = ctk_widget_get_parent (CTK_WIDGET (shell));

  if (CTK_IS_TOOL_PALETTE (parent))
    return ctk_orientable_get_orientation (CTK_ORIENTABLE (parent));

  return CTK_ORIENTATION_VERTICAL;
}

static GtkToolbarStyle
ctk_tool_item_group_get_style (GtkToolShell *shell)
{
  GtkWidget *parent = ctk_widget_get_parent (CTK_WIDGET (shell));

  if (CTK_IS_TOOL_PALETTE (parent))
    return ctk_tool_palette_get_style (CTK_TOOL_PALETTE (parent));

  return CTK_TOOLBAR_ICONS;
}

static GtkIconSize
ctk_tool_item_group_get_icon_size (GtkToolShell *shell)
{
  GtkWidget *parent = ctk_widget_get_parent (CTK_WIDGET (shell));

  if (CTK_IS_TOOL_PALETTE (parent))
    return ctk_tool_palette_get_icon_size (CTK_TOOL_PALETTE (parent));

  return CTK_ICON_SIZE_SMALL_TOOLBAR;
}

static PangoEllipsizeMode
ctk_tool_item_group_get_ellipsize_mode (GtkToolShell *shell)
{
  return CTK_TOOL_ITEM_GROUP (shell)->priv->ellipsize;
}

static gfloat
ctk_tool_item_group_get_text_alignment (GtkToolShell *shell)
{
  if (CTK_TOOLBAR_TEXT == ctk_tool_item_group_get_style (shell) ||
      CTK_TOOLBAR_BOTH_HORIZ == ctk_tool_item_group_get_style (shell))
    return 0.0;

  return 0.5;
}

static GtkOrientation
ctk_tool_item_group_get_text_orientation (GtkToolShell *shell)
{
  return CTK_ORIENTATION_HORIZONTAL;
}

static GtkSizeGroup *
ctk_tool_item_group_get_text_size_group (GtkToolShell *shell)
{
  GtkWidget *parent = ctk_widget_get_parent (CTK_WIDGET (shell));

  if (CTK_IS_TOOL_PALETTE (parent))
    return _ctk_tool_palette_get_size_group (CTK_TOOL_PALETTE (parent));

  return NULL;
}

static void
animation_change_notify (GtkToolItemGroup *group)
{
  GtkSettings *settings = group->priv->settings;
  gboolean animation;

  if (settings)
    g_object_get (settings,
                  "ctk-enable-animations", &animation,
                  NULL);
  else
    animation = DEFAULT_ANIMATION_STATE;

  group->priv->animation = animation;
}

static void
ctk_tool_item_group_settings_change_notify (GtkSettings      *settings,
                                            const GParamSpec *pspec,
                                            GtkToolItemGroup *group)
{
  if (strcmp (pspec->name, "ctk-enable-animations") == 0)
    animation_change_notify (group);
}

static void
ctk_tool_item_group_screen_changed (GtkWidget *widget,
                                    GdkScreen *previous_screen)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (widget);
  GtkToolItemGroupPrivate* priv = group->priv;
  GtkSettings *old_settings = priv->settings;
  GtkSettings *settings;

  if (ctk_widget_has_screen (CTK_WIDGET (group)))
    settings = ctk_widget_get_settings (CTK_WIDGET (group));
  else
    settings = NULL;

  if (settings == old_settings)
    return;

  if (old_settings)
  {
    g_signal_handler_disconnect (old_settings, priv->settings_connection);
    priv->settings_connection = 0;
    g_object_unref (old_settings);
  }

  if (settings)
  {
    priv->settings_connection =
      g_signal_connect (settings, "notify",
                        G_CALLBACK (ctk_tool_item_group_settings_change_notify),
                        group);
    priv->settings = g_object_ref (settings);
  }
  else
    priv->settings = NULL;

  animation_change_notify (group);
}

static void
ctk_tool_item_group_tool_shell_init (GtkToolShellIface *iface)
{
  iface->get_icon_size = ctk_tool_item_group_get_icon_size;
  iface->get_orientation = ctk_tool_item_group_get_orientation;
  iface->get_style = ctk_tool_item_group_get_style;
  iface->get_text_alignment = ctk_tool_item_group_get_text_alignment;
  iface->get_text_orientation = ctk_tool_item_group_get_text_orientation;
  iface->get_text_size_group = ctk_tool_item_group_get_text_size_group;
  iface->get_ellipsize_mode = ctk_tool_item_group_get_ellipsize_mode;
}

static gboolean
ctk_tool_item_group_header_draw_cb (GtkWidget *widget,
                                    cairo_t   *cr,
                                    gpointer   data)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (data);
  GtkToolItemGroupPrivate* priv = group->priv;
  GtkOrientation orientation;
  gint x, y, width, height;
  GtkTextDirection direction;
  GtkStyleContext *context;

  orientation = ctk_tool_shell_get_orientation (CTK_TOOL_SHELL (group));
  direction = ctk_widget_get_direction (widget);
  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);
  context = ctk_widget_get_style_context (widget);

  ctk_style_context_save_to_node (context, priv->arrow_node);

  if (CTK_ORIENTATION_VERTICAL == orientation)
    {
      ctk_style_context_add_class (context, CTK_STYLE_CLASS_VERTICAL);

      if (CTK_TEXT_DIR_RTL == direction)
        x = width;
      else
        x = 0;

      y = height / 2 - priv->expander_size / 2;
    }
  else
    {
      ctk_style_context_add_class (context, CTK_STYLE_CLASS_HORIZONTAL);
      x = width / 2 - priv->expander_size / 2;
      y = 0;
    }

  ctk_render_expander (context, cr, x, y,
                       priv->expander_size,
                       priv->expander_size);

  ctk_style_context_restore (context);

  return FALSE;
}

static void
ctk_tool_item_group_header_clicked_cb (GtkButton *button,
                                       gpointer   data)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (data);
  GtkToolItemGroupPrivate* priv = group->priv;
  GtkWidget *parent = ctk_widget_get_parent (data);

  if (priv->collapsed ||
      !CTK_IS_TOOL_PALETTE (parent) ||
      !ctk_tool_palette_get_exclusive (CTK_TOOL_PALETTE (parent), data))
    ctk_tool_item_group_set_collapsed (group, !priv->collapsed);
}

static void
ctk_tool_item_group_header_adjust_style (GtkToolItemGroup *group)
{
  GtkWidget *alignment = ctk_tool_item_group_get_alignment (group);
  GtkWidget *label_widget = ctk_bin_get_child (CTK_BIN (alignment));
  GtkWidget *widget = CTK_WIDGET (group);
  GtkToolItemGroupPrivate* priv = group->priv;
  gint dx = 0, dy = 0;
  GtkTextDirection direction = ctk_widget_get_direction (widget);

  ctk_widget_style_get (widget,
                        "header-spacing", &(priv->header_spacing),
                        "expander-size", &(priv->expander_size),
                        NULL);
  
  ctk_widget_set_size_request (alignment, -1, priv->expander_size);

  switch (ctk_tool_shell_get_orientation (CTK_TOOL_SHELL (group)))
    {
      case CTK_ORIENTATION_HORIZONTAL:
        dy = priv->header_spacing + priv->expander_size;

        if (CTK_IS_LABEL (label_widget))
          {
            ctk_label_set_ellipsize (CTK_LABEL (label_widget), PANGO_ELLIPSIZE_NONE);
            if (CTK_TEXT_DIR_RTL == direction)
              ctk_label_set_angle (CTK_LABEL (label_widget), -90);
            else
              ctk_label_set_angle (CTK_LABEL (label_widget), 90);
          }
       break;

      case CTK_ORIENTATION_VERTICAL:
        dx = priv->header_spacing + priv->expander_size;

        if (CTK_IS_LABEL (label_widget))
          {
            ctk_label_set_ellipsize (CTK_LABEL (label_widget), priv->ellipsize);
            ctk_label_set_angle (CTK_LABEL (label_widget), 0);
          }
        break;
    }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_alignment_set_padding (CTK_ALIGNMENT (alignment), dy, 0, dx, 0);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
update_arrow_state (GtkToolItemGroup *group)
{
  GtkToolItemGroupPrivate *priv = group->priv;
  GtkStateFlags state;

  state = ctk_widget_get_state_flags (CTK_WIDGET (group));

  if (priv->collapsed)
    state &= ~CTK_STATE_FLAG_CHECKED;
  else
    state |= CTK_STATE_FLAG_CHECKED;
  ctk_css_node_set_state (priv->arrow_node, state);
}

static void
ctk_tool_item_group_init (GtkToolItemGroup *group)
{
  GtkWidget *alignment;
  GtkToolItemGroupPrivate* priv;
  GtkCssNode *widget_node;

  group->priv = priv = ctk_tool_item_group_get_instance_private (group);

  priv->children = NULL;
  priv->header_spacing = DEFAULT_HEADER_SPACING;
  priv->expander_size = DEFAULT_EXPANDER_SIZE;
  priv->collapsed = DEFAULT_COLLAPSED;

  priv->label_widget = ctk_label_new (NULL);
  ctk_widget_set_halign (priv->label_widget, CTK_ALIGN_START);
  ctk_widget_set_valign (priv->label_widget, CTK_ALIGN_CENTER);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  alignment = ctk_alignment_new (0.5, 0.5, 1.0, 1.0);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_container_add (CTK_CONTAINER (alignment), priv->label_widget);
  ctk_widget_show_all (alignment);

  priv->header = ctk_button_new ();
  g_object_ref_sink (priv->header);
  ctk_widget_set_focus_on_click (priv->header, FALSE);
  ctk_container_add (CTK_CONTAINER (priv->header), alignment);
  ctk_widget_set_parent (priv->header, CTK_WIDGET (group));

  ctk_tool_item_group_header_adjust_style (group);

  g_signal_connect_after (alignment, "draw",
                          G_CALLBACK (ctk_tool_item_group_header_draw_cb),
                          group);

  g_signal_connect (priv->header, "clicked",
                    G_CALLBACK (ctk_tool_item_group_header_clicked_cb),
                    group);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (group));
  priv->arrow_node = ctk_css_node_new ();
  ctk_css_node_set_name (priv->arrow_node, I_("arrow"));
  ctk_css_node_set_parent (priv->arrow_node, widget_node);
  ctk_css_node_set_state (priv->arrow_node, ctk_css_node_get_state (widget_node));
  g_object_unref (priv->arrow_node);

  update_arrow_state (group);
}

static void
ctk_tool_item_group_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (object);

  switch (prop_id)
    {
      case PROP_LABEL:
        ctk_tool_item_group_set_label (group, g_value_get_string (value));
        break;

      case PROP_LABEL_WIDGET:
        ctk_tool_item_group_set_label_widget (group, g_value_get_object (value));
	break;

      case PROP_COLLAPSED:
        ctk_tool_item_group_set_collapsed (group, g_value_get_boolean (value));
        break;

      case PROP_ELLIPSIZE:
        ctk_tool_item_group_set_ellipsize (group, g_value_get_enum (value));
        break;

      case PROP_RELIEF:
        ctk_tool_item_group_set_header_relief (group, g_value_get_enum(value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ctk_tool_item_group_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (object);

  switch (prop_id)
    {
      case PROP_LABEL:
        g_value_set_string (value, ctk_tool_item_group_get_label (group));
        break;

      case PROP_LABEL_WIDGET:
        g_value_set_object (value,
			    ctk_tool_item_group_get_label_widget (group));
        break;

      case PROP_COLLAPSED:
        g_value_set_boolean (value, ctk_tool_item_group_get_collapsed (group));
        break;

      case PROP_ELLIPSIZE:
        g_value_set_enum (value, ctk_tool_item_group_get_ellipsize (group));
        break;

      case PROP_RELIEF:
        g_value_set_enum (value, ctk_tool_item_group_get_header_relief (group));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ctk_tool_item_group_finalize (GObject *object)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (object);

  g_list_free (group->priv->children);

  G_OBJECT_CLASS (ctk_tool_item_group_parent_class)->finalize (object);
}

static void
ctk_tool_item_group_dispose (GObject *object)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (object);
  GtkToolItemGroupPrivate* priv = group->priv;

  if (priv->toplevel)
    {
      /* disconnect focus tracking handler */
      g_signal_handler_disconnect (priv->toplevel,
                                   priv->focus_set_id);

      priv->focus_set_id = 0;
      priv->toplevel = NULL;
    }

  if (priv->settings_connection > 0)
    {
      g_signal_handler_disconnect (priv->settings, priv->settings_connection);
      priv->settings_connection = 0;
    }

  g_clear_object (&priv->settings);
  if (priv->header)
    ctk_widget_destroy (priv->header);
  g_clear_object (&priv->header);

  G_OBJECT_CLASS (ctk_tool_item_group_parent_class)->dispose (object);
}

static void
ctk_tool_item_group_get_item_size (GtkToolItemGroup *group,
                                   GtkRequisition   *item_size,
                                   gboolean          homogeneous_only,
                                   gint             *requested_rows)
{
  GtkWidget *parent = ctk_widget_get_parent (CTK_WIDGET (group));

  if (CTK_IS_TOOL_PALETTE (parent))
    _ctk_tool_palette_get_item_size (CTK_TOOL_PALETTE (parent), item_size, homogeneous_only, requested_rows);
  else
    _ctk_tool_item_group_item_size_request (group, item_size, homogeneous_only, requested_rows);
}

static void
ctk_tool_item_group_size_request (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (widget);
  GtkToolItemGroupPrivate* priv = group->priv;
  GtkOrientation orientation;
  GtkRequisition item_size;
  gint requested_rows;
  guint border_width;

  if (priv->children && ctk_tool_item_group_get_label_widget (group))
    {
      ctk_widget_get_preferred_size (priv->header,
                                     requisition, NULL);
      ctk_widget_show (priv->header);
    }
  else
    {
      requisition->width = requisition->height = 0;
      ctk_widget_hide (priv->header);
    }

  ctk_tool_item_group_get_item_size (group, &item_size, FALSE, &requested_rows);

  orientation = ctk_tool_shell_get_orientation (CTK_TOOL_SHELL (group));

  if (CTK_ORIENTATION_VERTICAL == orientation)
    requisition->width = MAX (requisition->width, item_size.width);
  else
    requisition->height = MAX (requisition->height, item_size.height * requested_rows);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));
  requisition->width += border_width * 2;
  requisition->height += border_width * 2;
}

static void
ctk_tool_item_group_get_preferred_width (GtkWidget *widget,
					 gint      *minimum,
					 gint      *natural)
{
  GtkRequisition requisition;

  ctk_tool_item_group_size_request (widget, &requisition);

  *minimum = *natural = requisition.width;
}

static void
ctk_tool_item_group_get_preferred_height (GtkWidget *widget,
					  gint      *minimum,
					  gint      *natural)
{
  GtkRequisition requisition;

  ctk_tool_item_group_size_request (widget, &requisition);

  *minimum = *natural = requisition.height;
}


static gboolean
ctk_tool_item_group_is_item_visible (GtkToolItemGroup      *group,
                                     GtkToolItemGroupChild *child)
{
  GtkToolbarStyle style;
  GtkOrientation orientation;

  orientation = ctk_tool_shell_get_orientation (CTK_TOOL_SHELL (group));
  style = ctk_tool_shell_get_style (CTK_TOOL_SHELL (group));

  /* horizontal tool palettes with text style support only homogeneous items */
  if (!child->homogeneous &&
      CTK_ORIENTATION_HORIZONTAL == orientation &&
      CTK_TOOLBAR_TEXT == style)
    return FALSE;

  return
    (ctk_widget_get_visible (CTK_WIDGET (child->item))) &&
    (CTK_ORIENTATION_VERTICAL == orientation ?
     ctk_tool_item_get_visible_vertical (child->item) :
     ctk_tool_item_get_visible_horizontal (child->item));
}

static inline unsigned
udiv (unsigned x,
      unsigned y)
{
  return (x + y - 1) / y;
}

static void
ctk_tool_item_group_real_size_query (GtkWidget      *widget,
                                     GtkAllocation  *allocation,
                                     GtkRequisition *inquery)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (widget);
  GtkToolItemGroupPrivate* priv = group->priv;

  GtkRequisition item_size;
  GtkAllocation item_area;

  GtkOrientation orientation;

  gint min_rows;
  guint border_width;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));
  orientation = ctk_tool_shell_get_orientation (CTK_TOOL_SHELL (group));

  /* figure out the size of homogeneous items */
  ctk_tool_item_group_get_item_size (group, &item_size, TRUE, &min_rows);

  if (CTK_ORIENTATION_VERTICAL == orientation)
    item_size.width = MIN (item_size.width, allocation->width);
  else
    item_size.height = MIN (item_size.height, allocation->height);

  item_size.width  = MAX (item_size.width, 1);
  item_size.height = MAX (item_size.height, 1);

  item_area.width = 0;
  item_area.height = 0;

  /* figure out the required columns (n_columns) and rows (n_rows)
   * to place all items
   */
  if (!priv->collapsed || !priv->animation || priv->animation_timeout)
    {
      guint n_columns;
      gint n_rows;
      GList *it;

      if (CTK_ORIENTATION_VERTICAL == orientation)
        {
          gboolean new_row = FALSE;
          gint row = -1;
          guint col = 0;

          item_area.width = allocation->width - 2 * border_width;
          n_columns = MAX (item_area.width / item_size.width, 1);

          /* calculate required rows for n_columns columns */
          for (it = priv->children; it != NULL; it = it->next)
            {
              GtkToolItemGroupChild *child = it->data;

              if (!ctk_tool_item_group_is_item_visible (group, child))
                continue;

              if (new_row || child->new_row)
                {
                  new_row = FALSE;
                  row++;
                  col = 0;
                }

              if (child->expand)
                new_row = TRUE;

              if (child->homogeneous)
                {
                  col++;
                  if (col >= n_columns)
                    new_row = TRUE;
                }
              else
                {
                  GtkRequisition req = {0, 0};
                  guint width;

                  ctk_widget_get_preferred_size (CTK_WIDGET (child->item),
                                                 &req, NULL);

                  width = udiv (req.width, item_size.width);
                  col += width;

                  if (col > n_columns)
                    row++;

                  col = width;

                  if (col >= n_columns)
                    new_row = TRUE;
                }
            }
          n_rows = row + 2;
        }
      else
        {
          guint *row_min_width;
          gint row = -1;
          gboolean new_row = TRUE;
          guint col = 0, min_col, max_col = 0, all_items = 0;
          gint i;

          item_area.height = allocation->height - 2 * border_width;
          n_rows = MAX (item_area.height / item_size.height, min_rows);

          row_min_width = g_new0 (guint, n_rows);

          /* calculate minimal and maximal required cols and minimal
           * required rows
           */
          for (it = priv->children; it != NULL; it = it->next)
            {
              GtkToolItemGroupChild *child = it->data;

              if (!ctk_tool_item_group_is_item_visible (group, child))
                continue;

              if (new_row || child->new_row)
                {
                  new_row = FALSE;
                  row++;
                  col = 0;
                  row_min_width[row] = 1;
                }

              if (child->expand)
                new_row = TRUE;

              if (child->homogeneous)
                {
                  col++;
                  all_items++;
                }
              else
                {
                  GtkRequisition req = {0, 0};
                  guint width;

                  ctk_widget_get_preferred_size (CTK_WIDGET (child->item),
                                                 &req, NULL);

                  width = udiv (req.width, item_size.width);

                  col += width;
                  all_items += width;

                  row_min_width[row] = MAX (row_min_width[row], width);
                }

              max_col = MAX (max_col, col);
            }

          /* calculate minimal required cols */
          min_col = udiv (all_items, n_rows);

          for (i = 0; i <= row; i++)
            {
              min_col = MAX (min_col, row_min_width[i]);
            }

          /* simple linear search for minimal required columns
           * for the given maximal number of rows (n_rows)
           */
          for (n_columns = min_col; n_columns < max_col; n_columns ++)
            {
              new_row = TRUE;
              row = -1;
              /* calculate required rows for n_columns columns */
              for (it = priv->children; it != NULL; it = it->next)
                {
                  GtkToolItemGroupChild *child = it->data;

                  if (!ctk_tool_item_group_is_item_visible (group, child))
                    continue;

                  if (new_row || child->new_row)
                    {
                      new_row = FALSE;
                      row++;
                      col = 0;
                    }

                  if (child->expand)
                    new_row = TRUE;

                  if (child->homogeneous)
                    {
                      col++;
                      if (col >= n_columns)
                        new_row = TRUE;
                    }
                  else
                    {
                      GtkRequisition req = {0, 0};
                      guint width;

                      ctk_widget_get_preferred_size (CTK_WIDGET (child->item),
                                                     &req, NULL);

                      width = udiv (req.width, item_size.width);
                      col += width;

                      if (col > n_columns)
                        row++;

                      col = width;

                      if (col >= n_columns)
                        new_row = TRUE;
                    }
                }

              if (row < n_rows)
                break;
            }
        }

      item_area.width = item_size.width * n_columns;
      item_area.height = item_size.height * n_rows;
    }

  inquery->width = 0;
  inquery->height = 0;

  /* figure out header widget size */
  if (ctk_widget_get_visible (priv->header))
    {
      GtkRequisition child_requisition;

      ctk_widget_get_preferred_size (priv->header,
                                     &child_requisition, NULL);

      if (CTK_ORIENTATION_VERTICAL == orientation)
        inquery->height += child_requisition.height;
      else
        inquery->width += child_requisition.width;
    }

  /* report effective widget size */
  inquery->width += item_area.width + 2 * border_width;
  inquery->height += item_area.height + 2 * border_width;
}

static void
ctk_tool_item_group_real_size_allocate (GtkWidget     *widget,
                                        GtkAllocation *allocation)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (widget);
  GtkToolItemGroupPrivate* priv = group->priv;
  GtkRequisition child_requisition;
  GtkAllocation child_allocation;

  GtkRequisition item_size;
  GtkAllocation item_area;

  GtkOrientation orientation;

  GList *it;

  gint n_columns, n_rows = 1;
  gint min_rows;
  guint border_width;
  GtkTextDirection direction;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  direction = ctk_widget_get_direction (widget);

  orientation = ctk_tool_shell_get_orientation (CTK_TOOL_SHELL (group));

  /* chain up */
  CTK_WIDGET_CLASS (ctk_tool_item_group_parent_class)->size_allocate (widget, allocation);

  child_allocation.x = border_width;
  child_allocation.y = border_width;

  /* place the header widget */
  if (ctk_widget_get_visible (priv->header))
    {
      ctk_widget_get_preferred_size (priv->header,
                                     &child_requisition, NULL);

      if (CTK_ORIENTATION_VERTICAL == orientation)
        {
          child_allocation.width = allocation->width;
          child_allocation.height = child_requisition.height;
        }
      else
        {
          child_allocation.width = child_requisition.width;
          child_allocation.height = allocation->height;

          if (CTK_TEXT_DIR_RTL == direction)
            child_allocation.x = allocation->width - border_width - child_allocation.width;
        }

      ctk_widget_size_allocate (priv->header, &child_allocation);

      if (CTK_ORIENTATION_VERTICAL == orientation)
        child_allocation.y += child_allocation.height;
      else if (CTK_TEXT_DIR_RTL != direction)
        child_allocation.x += child_allocation.width;
      else
        child_allocation.x = border_width;
    }
  else
    child_requisition.width = child_requisition.height = 0;

  /* figure out the size of homogeneous items */
  ctk_tool_item_group_get_item_size (group, &item_size, TRUE, &min_rows);

  item_size.width  = MAX (item_size.width, 1);
  item_size.height = MAX (item_size.height, 1);

  /* figure out the available columns and size of item_area */
  if (CTK_ORIENTATION_VERTICAL == orientation)
    {
      item_size.width = MIN (item_size.width, allocation->width);

      item_area.width = allocation->width - 2 * border_width;
      item_area.height = allocation->height - 2 * border_width - child_requisition.height;

      n_columns = MAX (item_area.width / item_size.width, 1);

      item_size.width = item_area.width / n_columns;
    }
  else
    {
      item_size.height = MIN (item_size.height, allocation->height);

      item_area.width = allocation->width - 2 * border_width - child_requisition.width;
      item_area.height = allocation->height - 2 * border_width;

      n_columns = MAX (item_area.width / item_size.width, 1);
      n_rows = MAX (item_area.height / item_size.height, min_rows);

      item_size.height = item_area.height / n_rows;
    }

  item_area.x = child_allocation.x;
  item_area.y = child_allocation.y;

  /* when expanded or in transition, place the tool items in a grid like layout */
  if (!priv->collapsed || !priv->animation || priv->animation_timeout)
    {
      gint col = 0, row = 0;

      for (it = priv->children; it != NULL; it = it->next)
        {
          GtkToolItemGroupChild *child = it->data;
          gint col_child;

          if (!ctk_tool_item_group_is_item_visible (group, child))
            {
              ctk_widget_set_child_visible (CTK_WIDGET (child->item), FALSE);

              continue;
            }

          /* for non homogeneous widgets request the required size */
          child_requisition.width = 0;

          if (!child->homogeneous)
            {
              ctk_widget_get_preferred_size (CTK_WIDGET (child->item),
                                             &child_requisition, NULL);
              child_requisition.width = MIN (child_requisition.width, item_area.width);
            }

          /* select next row if at end of row */
          if (col > 0 && (child->new_row || (col * item_size.width) + MAX (child_requisition.width, item_size.width) > item_area.width))
            {
              row++;
              col = 0;
              child_allocation.y += child_allocation.height;
            }

          col_child = col;

          /* calculate the position and size of the item */
          if (!child->homogeneous)
            {
              gint col_width;
              gint width;

              if (!child->expand)
                col_width = udiv (child_requisition.width, item_size.width);
              else
                col_width = n_columns - col;

              width = col_width * item_size.width;

              if (CTK_TEXT_DIR_RTL == direction)
                col_child = (n_columns - col - col_width);

              if (child->fill)
                {
                  child_allocation.x = item_area.x + col_child * item_size.width;
                  child_allocation.width = width;
                }
              else
                {
                  child_allocation.x =
                    (item_area.x + col_child * item_size.width +
                    (width - child_requisition.width) / 2);
                  child_allocation.width = child_requisition.width;
                }

              col += col_width;
            }
          else
            {
              if (CTK_TEXT_DIR_RTL == direction)
                col_child = (n_columns - col - 1);

              child_allocation.x = item_area.x + col_child * item_size.width;
              child_allocation.width = item_size.width;

              col++;
            }

          child_allocation.height = item_size.height;

          ctk_widget_size_allocate (CTK_WIDGET (child->item), &child_allocation);
          ctk_widget_set_child_visible (CTK_WIDGET (child->item), TRUE);
        }

      child_allocation.y += item_size.height;
    }

  /* or just hide all items, when collapsed */

  else
    {
      for (it = priv->children; it != NULL; it = it->next)
        {
          GtkToolItemGroupChild *child = it->data;

          ctk_widget_set_child_visible (CTK_WIDGET (child->item), FALSE);
        }
    }
}

static void
ctk_tool_item_group_size_allocate (GtkWidget     *widget,
                                   GtkAllocation *allocation)
{
  ctk_tool_item_group_real_size_allocate (widget, allocation);

  if (ctk_widget_get_mapped (widget))
    gdk_window_invalidate_rect (ctk_widget_get_window (widget), NULL, FALSE);
}

static void
ctk_tool_item_group_set_focus_cb (GtkWidget *window,
                                  GtkWidget *widget,
                                  gpointer   user_data)
{
  GtkAdjustment *adjustment;
  GtkAllocation allocation, p_allocation;
  GtkWidget *p;

  /* Find this group's parent widget in the focused widget's anchestry. */
  for (p = widget; p; p = ctk_widget_get_parent (p))
    if (p == user_data)
      {
        p = ctk_widget_get_parent (p);
        break;
      }

  if (CTK_IS_TOOL_PALETTE (p))
    {
      /* Check that the focused widgets is fully visible within
       * the group's parent widget and make it visible otherwise. */

      adjustment = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (p));

      if (adjustment)
        {
          int y;

          ctk_widget_get_allocation (widget, &allocation);
          ctk_widget_get_allocation (p, &p_allocation);

          /* Handle vertical adjustment. */
          if (ctk_widget_translate_coordinates
                (widget, p, 0, 0, NULL, &y) && y < 0)
            {
              y += ctk_adjustment_get_value (adjustment);
              ctk_adjustment_clamp_page (adjustment, y, y + allocation.height);
            }
          else if (ctk_widget_translate_coordinates (widget, p, 0, allocation.height, NULL, &y) &&
                   y > p_allocation.height)
            {
              y += ctk_adjustment_get_value (adjustment);
              ctk_adjustment_clamp_page (adjustment, y - allocation.height, y);
            }
        }

      adjustment = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (p));

      if (adjustment)
        {
          int x;

          ctk_widget_get_allocation (widget, &allocation);
          ctk_widget_get_allocation (p, &p_allocation);

          /* Handle horizontal adjustment. */
          if (ctk_widget_translate_coordinates
                (widget, p, 0, 0, &x, NULL) && x < 0)
            {
              x += ctk_adjustment_get_value (adjustment);
              ctk_adjustment_clamp_page (adjustment, x, x + allocation.width);
            }
          else if (ctk_widget_translate_coordinates (widget, p, allocation.width, 0, &x, NULL) &&
                   x > p_allocation.width)
            {
              x += ctk_adjustment_get_value (adjustment);
              ctk_adjustment_clamp_page (adjustment, x - allocation.width, x);
            }

          return;
        }
    }
}

static void
ctk_tool_item_group_set_toplevel_window (GtkToolItemGroup *group,
                                         GtkWidget        *toplevel)
{
  GtkToolItemGroupPrivate* priv = group->priv;

  if (toplevel != priv->toplevel)
    {
      if (priv->toplevel)
        {
          /* Disconnect focus tracking handler. */
          g_signal_handler_disconnect (priv->toplevel,
                                       priv->focus_set_id);

          priv->focus_set_id = 0;
          priv->toplevel = NULL;
        }

      if (toplevel)
        {
          /* Install focus tracking handler. We connect to the window's
           * set-focus signal instead of connecting to the focus signal of
           * each child to:
           *
           * 1) Reduce the number of signal handlers used.
           * 2) Avoid special handling for group headers.
           * 3) Catch focus grabs not only for direct children,
           *    but also for nested widgets.
           */
          priv->focus_set_id =
            g_signal_connect (toplevel, "set-focus",
                              G_CALLBACK (ctk_tool_item_group_set_focus_cb),
                              group);

          priv->toplevel = toplevel;
        }
    }
}

static void
ctk_tool_item_group_realize (GtkWidget *widget)
{
  GtkAllocation allocation;
  GtkWidget *toplevel_window;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  guint border_width;

  ctk_widget_set_realized (widget, TRUE);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));
  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x + border_width;
  attributes.y = allocation.y + border_width;
  attributes.width = allocation.width - border_width * 2;
  attributes.height = allocation.height - border_width * 2;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = ctk_widget_get_events (widget)
                         | GDK_VISIBILITY_NOTIFY_MASK
                         | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                         | GDK_BUTTON_MOTION_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = gdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);

  ctk_widget_register_window (widget, window);

  ctk_container_forall (CTK_CONTAINER (widget),
                        (GtkCallback) ctk_widget_set_parent_window,
                        window);

  ctk_widget_queue_resize_no_redraw (widget);

  toplevel_window = ctk_widget_get_ancestor (widget, CTK_TYPE_WINDOW);
  ctk_tool_item_group_set_toplevel_window (CTK_TOOL_ITEM_GROUP (widget),
                                           toplevel_window);
}

static void
ctk_tool_item_group_unrealize (GtkWidget *widget)
{
  ctk_tool_item_group_set_toplevel_window (CTK_TOOL_ITEM_GROUP (widget), NULL);
  CTK_WIDGET_CLASS (ctk_tool_item_group_parent_class)->unrealize (widget);
}

static gboolean
ctk_tool_item_group_draw (GtkWidget *widget,
                          cairo_t   *cr)
{
  ctk_render_background (ctk_widget_get_style_context (widget), cr,
                         0, 0,
                         ctk_widget_get_allocated_width (widget),
                         ctk_widget_get_allocated_height (widget));

  return CTK_WIDGET_CLASS (ctk_tool_item_group_parent_class)->draw (widget, cr);
}

static void
ctk_tool_item_group_style_updated (GtkWidget *widget)
{
  ctk_tool_item_group_header_adjust_style (CTK_TOOL_ITEM_GROUP (widget));
  CTK_WIDGET_CLASS (ctk_tool_item_group_parent_class)->style_updated (widget);
}

static void
ctk_tool_item_group_state_flags_changed (GtkWidget     *widget,
                                         GtkStateFlags  previous_flags)
{
  update_arrow_state (CTK_TOOL_ITEM_GROUP (widget));
}

static void
ctk_tool_item_group_add (GtkContainer *container,
                         GtkWidget    *widget)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (container));
  g_return_if_fail (CTK_IS_TOOL_ITEM (widget));

  ctk_tool_item_group_insert (CTK_TOOL_ITEM_GROUP (container),
                              CTK_TOOL_ITEM (widget), -1);
}

static void
ctk_tool_item_group_remove (GtkContainer *container,
                            GtkWidget    *child)
{
  GtkToolItemGroup *group;
  GtkToolItemGroupPrivate* priv;
  GList *it;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (container));
  group = CTK_TOOL_ITEM_GROUP (container);
  priv = group->priv;

  for (it = priv->children; it != NULL; it = it->next)
    {
      GtkToolItemGroupChild *child_info = it->data;

      if ((GtkWidget *)child_info->item == child)
        {
          g_object_unref (child);
          ctk_widget_unparent (child);

          g_free (child_info);
          priv->children = g_list_delete_link (priv->children, it);

          ctk_widget_queue_resize (CTK_WIDGET (container));
          break;
        }
    }
}

static void
ctk_tool_item_group_forall (GtkContainer *container,
                            gboolean      internals,
                            GtkCallback   callback,
                            gpointer      callback_data)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (container);
  GtkToolItemGroupPrivate* priv = group->priv;
  GList *children;

  if (internals && priv->header)
    callback (priv->header, callback_data);

  children = priv->children;
  while (children)
    {
      GtkToolItemGroupChild *child = children->data;
      children = children->next; /* store pointer before call to callback
				    because the child pointer is invalid if the
				    child->item is removed from the item group
				    in callback */

      callback (CTK_WIDGET (child->item), callback_data);
    }
}

static GType
ctk_tool_item_group_child_type (GtkContainer *container)
{
  return CTK_TYPE_TOOL_ITEM;
}

static GtkToolItemGroupChild *
ctk_tool_item_group_get_child (GtkToolItemGroup  *group,
                               GtkToolItem       *item,
                               gint              *position,
                               GList            **link)
{
  guint i;
  GList *it;

  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), NULL);
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (item), NULL);

  for (it = group->priv->children, i = 0; it != NULL; it = it->next, ++i)
    {
      GtkToolItemGroupChild *child = it->data;

      if (child->item == item)
        {
          if (position)
            *position = i;

          if (link)
            *link = it;

          return child;
        }
    }

  return NULL;
}

static void
ctk_tool_item_group_get_item_packing (GtkToolItemGroup *group,
                                      GtkToolItem      *item,
                                      gboolean         *homogeneous,
                                      gboolean         *expand,
                                      gboolean         *fill,
                                      gboolean         *new_row)
{
  GtkToolItemGroupChild *child;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (CTK_IS_TOOL_ITEM (item));

  child = ctk_tool_item_group_get_child (group, item, NULL, NULL);
  if (!child)
    return;

  if (expand)
    *expand = child->expand;

  if (homogeneous)
    *homogeneous = child->homogeneous;

  if (fill)
    *fill = child->fill;

  if (new_row)
    *new_row = child->new_row;
}

static void
ctk_tool_item_group_set_item_packing (GtkToolItemGroup *group,
                                      GtkToolItem      *item,
                                      gboolean          homogeneous,
                                      gboolean          expand,
                                      gboolean          fill,
                                      gboolean          new_row)
{
  GtkToolItemGroupChild *child;
  gboolean changed = FALSE;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (CTK_IS_TOOL_ITEM (item));

  child = ctk_tool_item_group_get_child (group, item, NULL, NULL);
  if (!child)
    return;

  ctk_widget_freeze_child_notify (CTK_WIDGET (item));

  if (child->homogeneous != homogeneous)
    {
      child->homogeneous = homogeneous;
      changed = TRUE;
      ctk_widget_child_notify (CTK_WIDGET (item), "homogeneous");
    }
  if (child->expand != expand)
    {
      child->expand = expand;
      changed = TRUE;
      ctk_widget_child_notify (CTK_WIDGET (item), "expand");
    }
  if (child->fill != fill)
    {
      child->fill = fill;
      changed = TRUE;
      ctk_widget_child_notify (CTK_WIDGET (item), "fill");
    }
  if (child->new_row != new_row)
    {
      child->new_row = new_row;
      changed = TRUE;
      ctk_widget_child_notify (CTK_WIDGET (item), "new-row");
    }

  ctk_widget_thaw_child_notify (CTK_WIDGET (item));

  if (changed
      && ctk_widget_get_visible (CTK_WIDGET (group))
      && ctk_widget_get_visible (CTK_WIDGET (item)))
    ctk_widget_queue_resize (CTK_WIDGET (group));
}

static void
ctk_tool_item_group_set_child_property (GtkContainer *container,
                                        GtkWidget    *child,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (container);
  GtkToolItem *item = CTK_TOOL_ITEM (child);
  gboolean homogeneous, expand, fill, new_row;

  if (prop_id != CHILD_PROP_POSITION)
    ctk_tool_item_group_get_item_packing (group, item,
                                          &homogeneous,
                                          &expand,
                                          &fill,
                                          &new_row);

  switch (prop_id)
    {
      case CHILD_PROP_HOMOGENEOUS:
        ctk_tool_item_group_set_item_packing (group, item,
                                              g_value_get_boolean (value),
                                              expand,
                                              fill,
                                              new_row);
        break;

      case CHILD_PROP_EXPAND:
        ctk_tool_item_group_set_item_packing (group, item,
                                              homogeneous,
                                              g_value_get_boolean (value),
                                              fill,
                                              new_row);
        break;

      case CHILD_PROP_FILL:
        ctk_tool_item_group_set_item_packing (group, item,
                                              homogeneous,
                                              expand,
                                              g_value_get_boolean (value),
                                              new_row);
        break;

      case CHILD_PROP_NEW_ROW:
        ctk_tool_item_group_set_item_packing (group, item,
                                              homogeneous,
                                              expand,
                                              fill,
                                              g_value_get_boolean (value));
        break;

      case CHILD_PROP_POSITION:
        ctk_tool_item_group_set_item_position (group, item, g_value_get_int (value));
        break;

      default:
        CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
        break;
    }
}

static void
ctk_tool_item_group_get_child_property (GtkContainer *container,
                                        GtkWidget    *child,
                                        guint         prop_id,
                                        GValue       *value,
                                        GParamSpec   *pspec)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (container);
  GtkToolItem *item = CTK_TOOL_ITEM (child);
  gboolean homogeneous, expand, fill, new_row;

  if (prop_id != CHILD_PROP_POSITION)
    ctk_tool_item_group_get_item_packing (group, item,
                                          &homogeneous,
                                          &expand,
                                          &fill,
                                          &new_row);

  switch (prop_id)
    {
      case CHILD_PROP_HOMOGENEOUS:
        g_value_set_boolean (value, homogeneous);
        break;

       case CHILD_PROP_EXPAND:
        g_value_set_boolean (value, expand);
        break;

       case CHILD_PROP_FILL:
        g_value_set_boolean (value, fill);
        break;

       case CHILD_PROP_NEW_ROW:
        g_value_set_boolean (value, new_row);
        break;

     case CHILD_PROP_POSITION:
        g_value_set_int (value, ctk_tool_item_group_get_item_position (group, item));
        break;

      default:
        CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
        break;
    }
}

static void
ctk_tool_item_group_class_init (GtkToolItemGroupClass *cls)
{
  GObjectClass       *oclass = G_OBJECT_CLASS (cls);
  GtkWidgetClass     *wclass = CTK_WIDGET_CLASS (cls);
  GtkContainerClass  *cclass = CTK_CONTAINER_CLASS (cls);

  oclass->set_property       = ctk_tool_item_group_set_property;
  oclass->get_property       = ctk_tool_item_group_get_property;
  oclass->finalize           = ctk_tool_item_group_finalize;
  oclass->dispose            = ctk_tool_item_group_dispose;

  wclass->get_preferred_width  = ctk_tool_item_group_get_preferred_width;
  wclass->get_preferred_height = ctk_tool_item_group_get_preferred_height;
  wclass->size_allocate        = ctk_tool_item_group_size_allocate;
  wclass->realize              = ctk_tool_item_group_realize;
  wclass->unrealize            = ctk_tool_item_group_unrealize;
  wclass->style_updated        = ctk_tool_item_group_style_updated;
  wclass->screen_changed       = ctk_tool_item_group_screen_changed;
  wclass->draw                 = ctk_tool_item_group_draw;
  wclass->state_flags_changed  = ctk_tool_item_group_state_flags_changed;

  cclass->add                = ctk_tool_item_group_add;
  cclass->remove             = ctk_tool_item_group_remove;
  cclass->forall             = ctk_tool_item_group_forall;
  cclass->child_type         = ctk_tool_item_group_child_type;
  cclass->set_child_property = ctk_tool_item_group_set_child_property;
  cclass->get_child_property = ctk_tool_item_group_get_child_property;

  g_object_class_install_property (oclass, PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        P_("Label"),
                                                        P_("The human-readable title of this item group"),
                                                        DEFAULT_LABEL,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_LABEL_WIDGET,
                                   g_param_spec_object  ("label-widget",
                                                        P_("Label widget"),
                                                        P_("A widget to display in place of the usual label"),
                                                        CTK_TYPE_WIDGET,
							CTK_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_COLLAPSED,
                                   g_param_spec_boolean ("collapsed",
                                                         P_("Collapsed"),
                                                         P_("Whether the group has been collapsed and items are hidden"),
                                                         DEFAULT_COLLAPSED,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (oclass, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      P_("ellipsize"),
                                                      P_("Ellipsize for item group headers"),
                                                      PANGO_TYPE_ELLIPSIZE_MODE, DEFAULT_ELLIPSIZE,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (oclass, PROP_RELIEF,
                                   g_param_spec_enum ("header-relief",
                                                      P_("Header Relief"),
                                                      P_("Relief of the group header button"),
                                                      CTK_TYPE_RELIEF_STYLE, CTK_RELIEF_NORMAL,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  ctk_widget_class_install_style_property (wclass,
                                           g_param_spec_int ("expander-size",
                                                             P_("Expander Size"),
                                                             P_("Size of the expander arrow"),
                                                             0, G_MAXINT, DEFAULT_EXPANDER_SIZE,
                                                             CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (wclass,
                                           g_param_spec_int ("header-spacing",
                                                             P_("Header Spacing"),
                                                             P_("Spacing between expander arrow and caption"),
                                                             0, G_MAXINT, DEFAULT_HEADER_SPACING,
                                                             CTK_PARAM_READABLE));

  ctk_container_class_install_child_property (cclass, CHILD_PROP_HOMOGENEOUS,
                                              g_param_spec_boolean ("homogeneous",
                                                                    P_("Homogeneous"),
                                                                    P_("Whether the item should be the same size as other homogeneous items"),
                                                                    TRUE,
                                                                    CTK_PARAM_READWRITE));

  ctk_container_class_install_child_property (cclass, CHILD_PROP_EXPAND,
                                              g_param_spec_boolean ("expand",
                                                                    P_("Expand"),
                                                                    P_("Whether the item should receive extra space when the group grows"),
                                                                    FALSE,
                                                                    CTK_PARAM_READWRITE)); 

  ctk_container_class_install_child_property (cclass, CHILD_PROP_FILL,
                                              g_param_spec_boolean ("fill",
                                                                    P_("Fill"),
                                                                    P_("Whether the item should fill the available space"),
                                                                    TRUE,
                                                                    CTK_PARAM_READWRITE));

  ctk_container_class_install_child_property (cclass, CHILD_PROP_NEW_ROW,
                                              g_param_spec_boolean ("new-row",
                                                                    P_("New Row"),
                                                                    P_("Whether the item should start a new row"),
                                                                    FALSE,
                                                                    CTK_PARAM_READWRITE));

  ctk_container_class_install_child_property (cclass, CHILD_PROP_POSITION,
                                              g_param_spec_int ("position",
                                                                P_("Position"),
                                                                P_("Position of the item within this group"),
                                                                0,
                                                                G_MAXINT,
                                                                0,
                                                                CTK_PARAM_READWRITE));

  ctk_widget_class_set_css_name (wclass, "toolitemgroup");
}

/**
 * ctk_tool_item_group_new:
 * @label: the label of the new group
 *
 * Creates a new tool item group with label @label.
 *
 * Returns: a new #GtkToolItemGroup.
 *
 * Since: 2.20
 */
GtkWidget*
ctk_tool_item_group_new (const gchar *label)
{
  return g_object_new (CTK_TYPE_TOOL_ITEM_GROUP, "label", label, NULL);
}

/**
 * ctk_tool_item_group_set_label:
 * @group: a #GtkToolItemGroup
 * @label: the new human-readable label of of the group
 *
 * Sets the label of the tool item group. The label is displayed in the header
 * of the group.
 *
 * Since: 2.20
 */
void
ctk_tool_item_group_set_label (GtkToolItemGroup *group,
                               const gchar      *label)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));

  if (!label)
    ctk_tool_item_group_set_label_widget (group, NULL);
  else
    {
      GtkWidget *child = ctk_label_new (label);
      ctk_widget_show (child);

      ctk_tool_item_group_set_label_widget (group, child);
    }

  g_object_notify (G_OBJECT (group), "label");
}

/**
 * ctk_tool_item_group_set_label_widget:
 * @group: a #GtkToolItemGroup
 * @label_widget: the widget to be displayed in place of the usual label
 *
 * Sets the label of the tool item group.
 * The label widget is displayed in the header of the group, in place
 * of the usual label.
 *
 * Since: 2.20
 */
void
ctk_tool_item_group_set_label_widget (GtkToolItemGroup *group,
                                      GtkWidget        *label_widget)
{
  GtkToolItemGroupPrivate* priv;
  GtkWidget *alignment;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (label_widget == NULL || CTK_IS_WIDGET (label_widget));
  g_return_if_fail (label_widget == NULL || ctk_widget_get_parent (label_widget) == NULL);

  priv = group->priv;

  if (priv->label_widget == label_widget)
    return;

  alignment = ctk_tool_item_group_get_alignment (group);

  if (priv->label_widget)
    {
      ctk_widget_set_state_flags (priv->label_widget, 0, TRUE);
      ctk_container_remove (CTK_CONTAINER (alignment), priv->label_widget);
    }


  if (label_widget)
      ctk_container_add (CTK_CONTAINER (alignment), label_widget);

  priv->label_widget = label_widget;

  if (ctk_widget_get_visible (CTK_WIDGET (group)))
    ctk_widget_queue_resize (CTK_WIDGET (group));

  /* Only show the header widget if the group has children: */
  if (label_widget && priv->children)
    ctk_widget_show (priv->header);
  else
    ctk_widget_hide (priv->header);

  g_object_freeze_notify (G_OBJECT (group));
  g_object_notify (G_OBJECT (group), "label-widget");
  g_object_notify (G_OBJECT (group), "label");
  g_object_thaw_notify (G_OBJECT (group));
}

/**
 * ctk_tool_item_group_set_header_relief:
 * @group: a #GtkToolItemGroup
 * @style: the #GtkReliefStyle
 *
 * Set the button relief of the group header.
 * See ctk_button_set_relief() for details.
 *
 * Since: 2.20
 */
void
ctk_tool_item_group_set_header_relief (GtkToolItemGroup *group,
                                       GtkReliefStyle    style)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));

  if (ctk_button_get_relief (CTK_BUTTON (group->priv->header)) != style)
    {
      ctk_button_set_relief (CTK_BUTTON (group->priv->header), style);
      g_object_notify (G_OBJECT (group), "header-relief");
    }
}

static gint64
ctk_tool_item_group_get_animation_timestamp (GtkToolItemGroup *group)
{
  return (g_source_get_time (group->priv->animation_timeout) -
          group->priv->animation_start) / 1000;
}

static void
ctk_tool_item_group_force_expose (GtkToolItemGroup *group)
{
  GtkToolItemGroupPrivate* priv = group->priv;
  GtkWidget *widget = CTK_WIDGET (group);

  if (ctk_widget_get_realized (priv->header))
    {
      GtkAllocation alignment_allocation;
      GtkWidget *alignment = ctk_tool_item_group_get_alignment (group);
      GdkRectangle area;

      /* Find the header button's arrow area... */
      ctk_widget_get_allocation (alignment, &alignment_allocation);
      area.x = alignment_allocation.x;
      area.y = alignment_allocation.y + (alignment_allocation.height - priv->expander_size) / 2;
      area.height = priv->expander_size;
      area.width = priv->expander_size;

      /* ... and invalidated it to get it animated. */
      gdk_window_invalidate_rect (ctk_widget_get_window (priv->header), &area, TRUE);
    }

  if (ctk_widget_get_realized (widget))
    {
      GtkAllocation allocation;
      GtkWidget *parent = ctk_widget_get_parent (widget);
      int x, y, width, height;

      /* Find the tool item area button's arrow area... */
      ctk_widget_get_allocation (widget, &allocation);
      width = allocation.width;
      height = allocation.height;

      ctk_widget_translate_coordinates (widget, parent, 0, 0, &x, &y);

      if (ctk_widget_get_visible (priv->header))
        {
          GtkAllocation header_allocation;

          ctk_widget_get_allocation (priv->header, &header_allocation);
          height -= header_allocation.height;
          y += header_allocation.height;
        }

      /* ... and invalidated it to get it animated. */
      ctk_widget_queue_draw_area (parent, x, y, width, height);
    }
}

static gboolean
ctk_tool_item_group_animation_cb (gpointer data)
{
  GtkToolItemGroup *group = CTK_TOOL_ITEM_GROUP (data);
  GtkToolItemGroupPrivate* priv = group->priv;
  gint64 timestamp = ctk_tool_item_group_get_animation_timestamp (group);
  gboolean retval;

  gdk_threads_enter ();

  /* Enque this early to reduce number of expose events. */
  ctk_widget_queue_resize_no_redraw (CTK_WIDGET (group));

  ctk_tool_item_group_force_expose (group);

  /* Finish animation when done. */
  if (timestamp >= ANIMATION_DURATION)
    priv->animation_timeout = NULL;

  retval = (priv->animation_timeout != NULL);

  gdk_threads_leave ();

  return retval;
}

/**
 * ctk_tool_item_group_set_collapsed:
 * @group: a #GtkToolItemGroup
 * @collapsed: whether the @group should be collapsed or expanded
 *
 * Sets whether the @group should be collapsed or expanded.
 *
 * Since: 2.20
 */
void
ctk_tool_item_group_set_collapsed (GtkToolItemGroup *group,
                                   gboolean          collapsed)
{
  GtkWidget *parent;
  GtkToolItemGroupPrivate* priv;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));

  priv = group->priv;

  parent = ctk_widget_get_parent (CTK_WIDGET (group));
  if (CTK_IS_TOOL_PALETTE (parent) && !collapsed)
    _ctk_tool_palette_set_expanding_child (CTK_TOOL_PALETTE (parent),
                                           CTK_WIDGET (group));
  if (collapsed != priv->collapsed)
    {
      if (priv->animation)
        {
          if (priv->animation_timeout)
            g_source_destroy (priv->animation_timeout);

          priv->animation_start = g_get_monotonic_time ();
          priv->animation_timeout = g_timeout_source_new (ANIMATION_TIMEOUT);

          g_source_set_callback (priv->animation_timeout,
                                 ctk_tool_item_group_animation_cb,
                                 group, NULL);
          g_source_attach (priv->animation_timeout, NULL);
        }
      else
        ctk_tool_item_group_force_expose (group);

      priv->collapsed = collapsed;
      update_arrow_state (group);
      g_object_notify (G_OBJECT (group), "collapsed");
    }
}

/**
 * ctk_tool_item_group_set_ellipsize:
 * @group: a #GtkToolItemGroup
 * @ellipsize: the #PangoEllipsizeMode labels in @group should use
 *
 * Sets the ellipsization mode which should be used by labels in @group.
 *
 * Since: 2.20
 */
void
ctk_tool_item_group_set_ellipsize (GtkToolItemGroup   *group,
                                   PangoEllipsizeMode  ellipsize)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));

  if (ellipsize != group->priv->ellipsize)
    {
      group->priv->ellipsize = ellipsize;
      ctk_tool_item_group_header_adjust_style (group);
      g_object_notify (G_OBJECT (group), "ellipsize");
      _ctk_tool_item_group_palette_reconfigured (group);
    }
}

/**
 * ctk_tool_item_group_get_label:
 * @group: a #GtkToolItemGroup
 *
 * Gets the label of @group.
 *
 * Returns: the label of @group. The label is an internal string of @group
 *     and must not be modified. Note that %NULL is returned if a custom
 *     label has been set with ctk_tool_item_group_set_label_widget()
 *
 * Since: 2.20
 */
const gchar*
ctk_tool_item_group_get_label (GtkToolItemGroup *group)
{
  GtkToolItemGroupPrivate *priv;

  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), NULL);

  priv = group->priv;

  if (CTK_IS_LABEL (priv->label_widget))
    return ctk_label_get_label (CTK_LABEL (priv->label_widget));
  else
    return NULL;
}

/**
 * ctk_tool_item_group_get_label_widget:
 * @group: a #GtkToolItemGroup
 *
 * Gets the label widget of @group.
 * See ctk_tool_item_group_set_label_widget().
 *
 * Returns: (transfer none): the label widget of @group
 *
 * Since: 2.20
 */
GtkWidget*
ctk_tool_item_group_get_label_widget (GtkToolItemGroup *group)
{
  GtkWidget *alignment = ctk_tool_item_group_get_alignment (group);

  return ctk_bin_get_child (CTK_BIN (alignment));
}

/**
 * ctk_tool_item_group_get_collapsed:
 * @group: a GtkToolItemGroup
 *
 * Gets whether @group is collapsed or expanded.
 *
 * Returns: %TRUE if @group is collapsed, %FALSE if it is expanded
 *
 * Since: 2.20
 */
gboolean
ctk_tool_item_group_get_collapsed (GtkToolItemGroup *group)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), DEFAULT_COLLAPSED);

  return group->priv->collapsed;
}

/**
 * ctk_tool_item_group_get_ellipsize:
 * @group: a #GtkToolItemGroup
 *
 * Gets the ellipsization mode of @group.
 *
 * Returns: the #PangoEllipsizeMode of @group
 *
 * Since: 2.20
 */
PangoEllipsizeMode
ctk_tool_item_group_get_ellipsize (GtkToolItemGroup *group)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), DEFAULT_ELLIPSIZE);

  return group->priv->ellipsize;
}

/**
 * ctk_tool_item_group_get_header_relief:
 * @group: a #GtkToolItemGroup
 *
 * Gets the relief mode of the header button of @group.
 *
 * Returns: the #GtkReliefStyle
 *
 * Since: 2.20
 */
GtkReliefStyle
ctk_tool_item_group_get_header_relief (GtkToolItemGroup   *group)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), CTK_RELIEF_NORMAL);

  return ctk_button_get_relief (CTK_BUTTON (group->priv->header));
}

/**
 * ctk_tool_item_group_insert:
 * @group: a #GtkToolItemGroup
 * @item: the #GtkToolItem to insert into @group
 * @position: the position of @item in @group, starting with 0.
 *     The position -1 means end of list.
 *
 * Inserts @item at @position in the list of children of @group.
 *
 * Since: 2.20
 */
void
ctk_tool_item_group_insert (GtkToolItemGroup *group,
                            GtkToolItem      *item,
                            gint              position)
{
  GtkWidget *parent, *child_widget;
  GtkToolItemGroupChild *child;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (CTK_IS_TOOL_ITEM (item));
  g_return_if_fail (position >= -1);

  parent = ctk_widget_get_parent (CTK_WIDGET (group));

  child = g_new (GtkToolItemGroupChild, 1);
  child->item = g_object_ref_sink (item);
  child->homogeneous = TRUE;
  child->expand = FALSE;
  child->fill = TRUE;
  child->new_row = FALSE;

  group->priv->children = g_list_insert (group->priv->children, child, position);

  if (CTK_IS_TOOL_PALETTE (parent))
    _ctk_tool_palette_child_set_drag_source (CTK_WIDGET (item), parent);

  child_widget = ctk_bin_get_child (CTK_BIN (item));

  ctk_widget_set_focus_on_click (child_widget, TRUE);

  ctk_widget_set_parent (CTK_WIDGET (item), CTK_WIDGET (group));
}

/**
 * ctk_tool_item_group_set_item_position:
 * @group: a #GtkToolItemGroup
 * @item: the #GtkToolItem to move to a new position, should
 *     be a child of @group.
 * @position: the new position of @item in @group, starting with 0.
 *     The position -1 means end of list.
 *
 * Sets the position of @item in the list of children of @group.
 *
 * Since: 2.20
 */
void
ctk_tool_item_group_set_item_position (GtkToolItemGroup *group,
                                       GtkToolItem      *item,
                                       gint              position)
{
  gint old_position;
  GList *link;
  GtkToolItemGroupChild *child;
  GtkToolItemGroupPrivate* priv;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (CTK_IS_TOOL_ITEM (item));
  g_return_if_fail (position >= -1);

  child = ctk_tool_item_group_get_child (group, item, &old_position, &link);
  priv = group->priv;

  g_return_if_fail (child != NULL);

  if (position == old_position)
    return;

  priv->children = g_list_delete_link (priv->children, link);
  priv->children = g_list_insert (priv->children, child, position);

  ctk_widget_child_notify (CTK_WIDGET (item), "position");
  if (ctk_widget_get_visible (CTK_WIDGET (group)) &&
      ctk_widget_get_visible (CTK_WIDGET (item)))
    ctk_widget_queue_resize (CTK_WIDGET (group));
}

/**
 * ctk_tool_item_group_get_item_position:
 * @group: a #GtkToolItemGroup
 * @item: a #GtkToolItem
 *
 * Gets the position of @item in @group as index.
 *
 * Returns: the index of @item in @group or -1 if @item is no child of @group
 *
 * Since: 2.20
 */
gint
ctk_tool_item_group_get_item_position (GtkToolItemGroup *group,
                                       GtkToolItem      *item)
{
  gint position;

  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), -1);
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (item), -1);

  if (ctk_tool_item_group_get_child (group, item, &position, NULL))
    return position;

  return -1;
}

/**
 * ctk_tool_item_group_get_n_items:
 * @group: a #GtkToolItemGroup
 *
 * Gets the number of tool items in @group.
 *
 * Returns: the number of tool items in @group
 *
 * Since: 2.20
 */
guint
ctk_tool_item_group_get_n_items (GtkToolItemGroup *group)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), 0);

  return g_list_length (group->priv->children);
}

/**
 * ctk_tool_item_group_get_nth_item:
 * @group: a #GtkToolItemGroup
 * @index: the index
 *
 * Gets the tool item at @index in group.
 *
 * Returns: (transfer none): the #GtkToolItem at index
 *
 * Since: 2.20
 */
GtkToolItem*
ctk_tool_item_group_get_nth_item (GtkToolItemGroup *group,
                                  guint             index)
{
  GtkToolItemGroupChild *child;

  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), NULL);

  child = g_list_nth_data (group->priv->children, index);

  return child != NULL ? child->item : NULL;
}

/**
 * ctk_tool_item_group_get_drop_item:
 * @group: a #GtkToolItemGroup
 * @x: the x position
 * @y: the y position
 *
 * Gets the tool item at position (x, y).
 *
 * Returns: (transfer none): the #GtkToolItem at position (x, y)
 *
 * Since: 2.20
 */
GtkToolItem*
ctk_tool_item_group_get_drop_item (GtkToolItemGroup *group,
                                   gint              x,
                                   gint              y)
{
  GtkAllocation allocation;
  GList *it;

  g_return_val_if_fail (CTK_IS_TOOL_ITEM_GROUP (group), NULL);

  ctk_widget_get_allocation (CTK_WIDGET (group), &allocation);

  g_return_val_if_fail (x >= 0 && x < allocation.width, NULL);
  g_return_val_if_fail (y >= 0 && y < allocation.height, NULL);

  for (it = group->priv->children; it != NULL; it = it->next)
    {
      GtkToolItemGroupChild *child = it->data;
      GtkToolItem *item = child->item;
      gint x0, y0;

      if (!item || !ctk_tool_item_group_is_item_visible (group, child))
        continue;

      ctk_widget_get_allocation (CTK_WIDGET (item), &allocation);

      x0 = x - allocation.x;
      y0 = y - allocation.y;

      if (x0 >= 0 && x0 < allocation.width &&
          y0 >= 0 && y0 < allocation.height)
        return item;
    }

  return NULL;
}

void
_ctk_tool_item_group_item_size_request (GtkToolItemGroup *group,
                                        GtkRequisition   *item_size,
                                        gboolean          homogeneous_only,
                                        gint             *requested_rows)
{
  GtkRequisition child_requisition;
  GList *it;
  gint rows = 0;
  gboolean new_row = TRUE;

  g_return_if_fail (CTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (NULL != item_size);

  item_size->width = item_size->height = 0;

  for (it = group->priv->children; it != NULL; it = it->next)
    {
      GtkToolItemGroupChild *child = it->data;

      if (!ctk_tool_item_group_is_item_visible (group, child))
        continue;

      if (child->new_row || new_row)
        {
          rows++;
          new_row = FALSE;
        }

      if (!child->homogeneous && child->expand)
          new_row = TRUE;

      ctk_widget_get_preferred_size (CTK_WIDGET (child->item),
                                     &child_requisition, NULL);

      if (!homogeneous_only || child->homogeneous)
        item_size->width = MAX (item_size->width, child_requisition.width);
      item_size->height = MAX (item_size->height, child_requisition.height);
    }

  if (requested_rows)
    *requested_rows = rows;
}

gint
_ctk_tool_item_group_get_size_for_limit (GtkToolItemGroup *group,
                                         gint              limit,
                                         gboolean          vertical,
                                         gboolean          animation)
{
  GtkRequisition requisition;
  GtkToolItemGroupPrivate* priv = group->priv;

  ctk_widget_get_preferred_size (CTK_WIDGET (group),
                                 &requisition, NULL);

  if (!priv->collapsed || priv->animation_timeout)
    {
      GtkAllocation allocation = { 0, 0, requisition.width, requisition.height };
      GtkRequisition inquery;

      if (vertical)
        allocation.width = limit;
      else
        allocation.height = limit;

      ctk_tool_item_group_real_size_query (CTK_WIDGET (group),
                                           &allocation, &inquery);

      if (vertical)
        inquery.height -= requisition.height;
      else
        inquery.width -= requisition.width;

      if (priv->animation_timeout && animation)
        {
          gint64 timestamp = ctk_tool_item_group_get_animation_timestamp (group);

          timestamp = MIN (timestamp, ANIMATION_DURATION);

          if (priv->collapsed)
            timestamp = ANIMATION_DURATION - timestamp;

          if (vertical)
            {
              inquery.height *= timestamp;
              inquery.height /= ANIMATION_DURATION;
            }
          else
            {
              inquery.width *= timestamp;
              inquery.width /= ANIMATION_DURATION;
            }
        }

      if (vertical)
        requisition.height += inquery.height;
      else
        requisition.width += inquery.width;
    }

  return (vertical ? requisition.height : requisition.width);
}

gint
_ctk_tool_item_group_get_height_for_width (GtkToolItemGroup *group,
                                           gint              width)
{
  return _ctk_tool_item_group_get_size_for_limit (group, width, TRUE, group->priv->animation);
}

gint
_ctk_tool_item_group_get_width_for_height (GtkToolItemGroup *group,
                                           gint              height)
{
  return _ctk_tool_item_group_get_size_for_limit (group, height, FALSE, TRUE);
}

static void
ctk_tool_palette_reconfigured_foreach_item (GtkWidget *child,
                                            gpointer   data)
{
  if (CTK_IS_TOOL_ITEM (child))
    ctk_tool_item_toolbar_reconfigured (CTK_TOOL_ITEM (child));
}


void
_ctk_tool_item_group_palette_reconfigured (GtkToolItemGroup *group)
{
  ctk_container_foreach (CTK_CONTAINER (group),
                         ctk_tool_palette_reconfigured_foreach_item,
                         NULL);

  ctk_tool_item_group_header_adjust_style (group);
}
