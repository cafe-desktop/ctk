/* ctkcellareacontext.c
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

/**
 * SECTION:ctkcellareacontext
 * @Short_Description: Stores geometrical information for a series of rows in a CtkCellArea
 * @Title: CtkCellAreaContext
 *
 * The #CtkCellAreaContext object is created by a given #CtkCellArea
 * implementation via its #CtkCellAreaClass.create_context() virtual
 * method and is used to store cell sizes and alignments for a series of
 * #CtkTreeModel rows that are requested and rendered in the same context.
 *
 * #CtkCellLayout widgets can create any number of contexts in which to
 * request and render groups of data rows. However, it’s important that the
 * same context which was used to request sizes for a given #CtkTreeModel
 * row also be used for the same row when calling other #CtkCellArea APIs
 * such as ctk_cell_area_render() and ctk_cell_area_event().
 */

#include "config.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkcellareacontext.h"
#include "ctkprivate.h"

/* GObjectClass */
static void ctk_cell_area_context_dispose       (GObject            *object);
static void ctk_cell_area_context_get_property  (GObject            *object,
                                                 guint               prop_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);
static void ctk_cell_area_context_set_property  (GObject            *object,
                                                 guint               prop_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);

/* CtkCellAreaContextClass */
static void ctk_cell_area_context_real_reset    (CtkCellAreaContext *context);
static void ctk_cell_area_context_real_allocate (CtkCellAreaContext *context,
                                                 gint                width,
                                                 gint                height);

struct _CtkCellAreaContextPrivate
{
  CtkCellArea *cell_area;

  gint         min_width;
  gint         nat_width;
  gint         min_height;
  gint         nat_height;
  gint         alloc_width;
  gint         alloc_height;
};

enum {
  PROP_0,
  PROP_CELL_AREA,
  PROP_MIN_WIDTH,
  PROP_NAT_WIDTH,
  PROP_MIN_HEIGHT,
  PROP_NAT_HEIGHT
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkCellAreaContext, ctk_cell_area_context, G_TYPE_OBJECT)

static void
ctk_cell_area_context_init (CtkCellAreaContext *context)
{
  context->priv = ctk_cell_area_context_get_instance_private (context);
}

static void
ctk_cell_area_context_class_init (CtkCellAreaContextClass *class)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (class);

  /* GObjectClass */
  object_class->dispose      = ctk_cell_area_context_dispose;
  object_class->get_property = ctk_cell_area_context_get_property;
  object_class->set_property = ctk_cell_area_context_set_property;

  /* CtkCellAreaContextClass */
  class->reset    = ctk_cell_area_context_real_reset;
  class->allocate = ctk_cell_area_context_real_allocate;

  /**
   * CtkCellAreaContext:area:
   *
   * The #CtkCellArea this context was created by
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_CELL_AREA,
                                   g_param_spec_object ("area",
                                                        P_("Area"),
                                                        P_("The Cell Area this context was created for"),
                                                        CTK_TYPE_CELL_AREA,
                                                        CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkCellAreaContext:minimum-width:
   *
   * The minimum width for the #CtkCellArea in this context
   * for all #CtkTreeModel rows that this context was requested
   * for using ctk_cell_area_get_preferred_width().
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_MIN_WIDTH,
                                   g_param_spec_int ("minimum-width",
                                                     P_("Minimum Width"),
                                                     P_("Minimum cached width"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READABLE));

  /**
   * CtkCellAreaContext:natural-width:
   *
   * The natural width for the #CtkCellArea in this context
   * for all #CtkTreeModel rows that this context was requested
   * for using ctk_cell_area_get_preferred_width().
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_NAT_WIDTH,
                                   g_param_spec_int ("natural-width",
                                                     P_("Minimum Width"),
                                                     P_("Minimum cached width"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READABLE));

  /**
   * CtkCellAreaContext:minimum-height:
   *
   * The minimum height for the #CtkCellArea in this context
   * for all #CtkTreeModel rows that this context was requested
   * for using ctk_cell_area_get_preferred_height().
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_MIN_HEIGHT,
                                   g_param_spec_int ("minimum-height",
                                                     P_("Minimum Height"),
                                                     P_("Minimum cached height"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READABLE));

  /**
   * CtkCellAreaContext:natural-height:
   *
   * The natural height for the #CtkCellArea in this context
   * for all #CtkTreeModel rows that this context was requested
   * for using ctk_cell_area_get_preferred_height().
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_NAT_HEIGHT,
                                   g_param_spec_int ("natural-height",
                                                     P_("Minimum Height"),
                                                     P_("Minimum cached height"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READABLE));
}

/*************************************************************
 *                      GObjectClass                         *
 *************************************************************/
static void
ctk_cell_area_context_dispose (GObject *object)
{
  CtkCellAreaContext        *context = CTK_CELL_AREA_CONTEXT (object);
  CtkCellAreaContextPrivate *priv = context->priv;

  if (priv->cell_area)
    {
      g_object_unref (priv->cell_area);

      priv->cell_area = NULL;
    }

  G_OBJECT_CLASS (ctk_cell_area_context_parent_class)->dispose (object);
}

static void
ctk_cell_area_context_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  CtkCellAreaContext        *context = CTK_CELL_AREA_CONTEXT (object);
  CtkCellAreaContextPrivate *priv = context->priv;

  switch (prop_id)
    {
    case PROP_CELL_AREA:
      priv->cell_area = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_cell_area_context_get_property (GObject     *object,
                                    guint        prop_id,
                                    GValue      *value,
                                    GParamSpec  *pspec)
{
  CtkCellAreaContext        *context = CTK_CELL_AREA_CONTEXT (object);
  CtkCellAreaContextPrivate *priv = context->priv;

  switch (prop_id)
    {
    case PROP_CELL_AREA:
      g_value_set_object (value, priv->cell_area);
      break;
    case PROP_MIN_WIDTH:
      g_value_set_int (value, priv->min_width);
      break;
    case PROP_NAT_WIDTH:
      g_value_set_int (value, priv->nat_width);
      break;
    case PROP_MIN_HEIGHT:
      g_value_set_int (value, priv->min_height);
      break;
    case PROP_NAT_HEIGHT:
      g_value_set_int (value, priv->nat_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/*************************************************************
 *                    CtkCellAreaContextClass                *
 *************************************************************/
static void
ctk_cell_area_context_real_reset (CtkCellAreaContext *context)
{
  CtkCellAreaContextPrivate *priv = context->priv;

  g_object_freeze_notify (G_OBJECT (context));

  if (priv->min_width != 0)
    {
      priv->min_width = 0;
      g_object_notify (G_OBJECT (context), "minimum-width");
    }

  if (priv->nat_width != 0)
    {
      priv->nat_width = 0;
      g_object_notify (G_OBJECT (context), "natural-width");
    }

  if (priv->min_height != 0)
    {
      priv->min_height = 0;
      g_object_notify (G_OBJECT (context), "minimum-height");
    }

  if (priv->nat_height != 0)
    {
      priv->nat_height = 0;
      g_object_notify (G_OBJECT (context), "natural-height");
    }

  priv->alloc_width  = 0;
  priv->alloc_height = 0;

  g_object_thaw_notify (G_OBJECT (context));
}

static void
ctk_cell_area_context_real_allocate (CtkCellAreaContext *context,
                                     gint                width,
                                     gint                height)
{
  CtkCellAreaContextPrivate *priv = context->priv;

  priv->alloc_width  = width;
  priv->alloc_height = height;
}

/*************************************************************
 *                            API                            *
 *************************************************************/
/**
 * ctk_cell_area_context_get_area:
 * @context: a #CtkCellAreaContext
 *
 * Fetches the #CtkCellArea this @context was created by.
 *
 * This is generally unneeded by layouting widgets; however,
 * it is important for the context implementation itself to
 * fetch information about the area it is being used for.
 *
 * For instance at #CtkCellAreaContextClass.allocate() time
 * it’s important to know details about any cell spacing
 * that the #CtkCellArea is configured with in order to
 * compute a proper allocation.
 *
 * Returns: (transfer none): the #CtkCellArea this context was created by.
 *
 * Since: 3.0
 */
CtkCellArea *
ctk_cell_area_context_get_area (CtkCellAreaContext *context)
{
  CtkCellAreaContextPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_AREA_CONTEXT (context), NULL);

  priv = context->priv;

  return priv->cell_area;
}

/**
 * ctk_cell_area_context_reset:
 * @context: a #CtkCellAreaContext
 *
 * Resets any previously cached request and allocation
 * data.
 *
 * When underlying #CtkTreeModel data changes its
 * important to reset the context if the content
 * size is allowed to shrink. If the content size
 * is only allowed to grow (this is usually an option
 * for views rendering large data stores as a measure
 * of optimization), then only the row that changed
 * or was inserted needs to be (re)requested with
 * ctk_cell_area_get_preferred_width().
 *
 * When the new overall size of the context requires
 * that the allocated size changes (or whenever this
 * allocation changes at all), the variable row
 * sizes need to be re-requested for every row.
 *
 * For instance, if the rows are displayed all with
 * the same width from top to bottom then a change
 * in the allocated width necessitates a recalculation
 * of all the displayed row heights using
 * ctk_cell_area_get_preferred_height_for_width().
 *
 * Since 3.0
 */
void
ctk_cell_area_context_reset (CtkCellAreaContext *context)
{
  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  CTK_CELL_AREA_CONTEXT_GET_CLASS (context)->reset (context);
}

/**
 * ctk_cell_area_context_allocate:
 * @context: a #CtkCellAreaContext
 * @width: the allocated width for all #CtkTreeModel rows rendered
 *     with @context, or -1.
 * @height: the allocated height for all #CtkTreeModel rows rendered
 *     with @context, or -1.
 *
 * Allocates a width and/or a height for all rows which are to be
 * rendered with @context.
 *
 * Usually allocation is performed only horizontally or sometimes
 * vertically since a group of rows are usually rendered side by
 * side vertically or horizontally and share either the same width
 * or the same height. Sometimes they are allocated in both horizontal
 * and vertical orientations producing a homogeneous effect of the
 * rows. This is generally the case for #CtkTreeView when
 * #CtkTreeView:fixed-height-mode is enabled.
 *
 * Since 3.0
 */
void
ctk_cell_area_context_allocate (CtkCellAreaContext *context,
                                gint                width,
                                gint                height)
{
  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  CTK_CELL_AREA_CONTEXT_GET_CLASS (context)->allocate (context, width, height);
}

/**
 * ctk_cell_area_context_get_preferred_width:
 * @context: a #CtkCellAreaContext
 * @minimum_width: (out) (allow-none): location to store the minimum width,
 *     or %NULL
 * @natural_width: (out) (allow-none): location to store the natural width,
 *     or %NULL
 *
 * Gets the accumulative preferred width for all rows which have been
 * requested with this context.
 *
 * After ctk_cell_area_context_reset() is called and/or before ever
 * requesting the size of a #CtkCellArea, the returned values are 0.
 *
 * Since: 3.0
 */
void
ctk_cell_area_context_get_preferred_width (CtkCellAreaContext *context,
                                           gint               *minimum_width,
                                           gint               *natural_width)
{
  CtkCellAreaContextPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  priv = context->priv;

  if (minimum_width)
    *minimum_width = priv->min_width;

  if (natural_width)
    *natural_width = priv->nat_width;
}

/**
 * ctk_cell_area_context_get_preferred_height:
 * @context: a #CtkCellAreaContext
 * @minimum_height: (out) (allow-none): location to store the minimum height,
 *     or %NULL
 * @natural_height: (out) (allow-none): location to store the natural height,
 *     or %NULL
 *
 * Gets the accumulative preferred height for all rows which have been
 * requested with this context.
 *
 * After ctk_cell_area_context_reset() is called and/or before ever
 * requesting the size of a #CtkCellArea, the returned values are 0.
 *
 * Since: 3.0
 */
void
ctk_cell_area_context_get_preferred_height (CtkCellAreaContext *context,
                                            gint               *minimum_height,
                                            gint               *natural_height)
{
  CtkCellAreaContextPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  priv = context->priv;

  if (minimum_height)
    *minimum_height = priv->min_height;

  if (natural_height)
    *natural_height = priv->nat_height;
}

/**
 * ctk_cell_area_context_get_preferred_height_for_width:
 * @context: a #CtkCellAreaContext
 * @width: a proposed width for allocation
 * @minimum_height: (out) (allow-none): location to store the minimum height,
 *     or %NULL
 * @natural_height: (out) (allow-none): location to store the natural height,
 *     or %NULL
 *
 * Gets the accumulative preferred height for @width for all rows
 * which have been requested for the same said @width with this context.
 *
 * After ctk_cell_area_context_reset() is called and/or before ever
 * requesting the size of a #CtkCellArea, the returned values are -1.
 *
 * Since: 3.0
 */
void
ctk_cell_area_context_get_preferred_height_for_width (CtkCellAreaContext *context,
                                                      gint                width,
                                                      gint               *minimum_height,
                                                      gint               *natural_height)
{
  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  if (CTK_CELL_AREA_CONTEXT_GET_CLASS (context)->get_preferred_height_for_width)
    CTK_CELL_AREA_CONTEXT_GET_CLASS (context)->get_preferred_height_for_width (context,
                                                                               width,
                                                                               minimum_height,
                                                                               natural_height);
}

/**
 * ctk_cell_area_context_get_preferred_width_for_height:
 * @context: a #CtkCellAreaContext
 * @height: a proposed height for allocation
 * @minimum_width: (out) (allow-none): location to store the minimum width,
 *     or %NULL
 * @natural_width: (out) (allow-none): location to store the natural width,
 *     or %NULL
 *
 * Gets the accumulative preferred width for @height for all rows which
 * have been requested for the same said @height with this context.
 *
 * After ctk_cell_area_context_reset() is called and/or before ever
 * requesting the size of a #CtkCellArea, the returned values are -1.
 *
 * Since: 3.0
 */
void
ctk_cell_area_context_get_preferred_width_for_height (CtkCellAreaContext *context,
                                                      gint                height,
                                                      gint               *minimum_width,
                                                      gint               *natural_width)
{
  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  if (CTK_CELL_AREA_CONTEXT_GET_CLASS (context)->get_preferred_width_for_height)
    CTK_CELL_AREA_CONTEXT_GET_CLASS (context)->get_preferred_width_for_height (context,
                                                                               height,
                                                                               minimum_width,
                                                                               natural_width);
}

/**
 * ctk_cell_area_context_get_allocation:
 * @context: a #CtkCellAreaContext
 * @width: (out) (allow-none): location to store the allocated width, or %NULL
 * @height: (out) (allow-none): location to store the allocated height, or %NULL
 *
 * Fetches the current allocation size for @context.
 *
 * If the context was not allocated in width or height, or if the
 * context was recently reset with ctk_cell_area_context_reset(),
 * the returned value will be -1.
 *
 * Since: 3.0
 */
void
ctk_cell_area_context_get_allocation (CtkCellAreaContext *context,
                                      gint               *width,
                                      gint               *height)
{
  CtkCellAreaContextPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  priv = context->priv;

  if (width)
    *width = priv->alloc_width;

  if (height)
    *height = priv->alloc_height;
}

/**
 * ctk_cell_area_context_push_preferred_width:
 * @context: a #CtkCellAreaContext
 * @minimum_width: the proposed new minimum width for @context
 * @natural_width: the proposed new natural width for @context
 *
 * Causes the minimum and/or natural width to grow if the new
 * proposed sizes exceed the current minimum and natural width.
 *
 * This is used by #CtkCellAreaContext implementations during
 * the request process over a series of #CtkTreeModel rows to
 * progressively push the requested width over a series of
 * ctk_cell_area_get_preferred_width() requests.
 *
 * Since: 3.0
 */
void
ctk_cell_area_context_push_preferred_width (CtkCellAreaContext *context,
                                            gint                minimum_width,
                                            gint                natural_width)
{
  CtkCellAreaContextPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  priv = context->priv;

  g_object_freeze_notify (G_OBJECT (context));

  if (minimum_width > priv->min_width)
    {
      priv->min_width = minimum_width;

      g_object_notify (G_OBJECT (context), "minimum-width");
    }

  if (natural_width > priv->nat_width)
    {
      priv->nat_width = natural_width;

      g_object_notify (G_OBJECT (context), "natural-width");
    }

  g_object_thaw_notify (G_OBJECT (context));
}

/**
 * ctk_cell_area_context_push_preferred_height:
 * @context: a #CtkCellAreaContext
 * @minimum_height: the proposed new minimum height for @context
 * @natural_height: the proposed new natural height for @context
 *
 * Causes the minimum and/or natural height to grow if the new
 * proposed sizes exceed the current minimum and natural height.
 *
 * This is used by #CtkCellAreaContext implementations during
 * the request process over a series of #CtkTreeModel rows to
 * progressively push the requested height over a series of
 * ctk_cell_area_get_preferred_height() requests.
 *
 * Since: 3.0
 */
void
ctk_cell_area_context_push_preferred_height (CtkCellAreaContext *context,
                                             gint                minimum_height,
                                             gint                natural_height)
{
  CtkCellAreaContextPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));

  priv = context->priv;

  g_object_freeze_notify (G_OBJECT (context));

  if (minimum_height > priv->min_height)
    {
      priv->min_height = minimum_height;

      g_object_notify (G_OBJECT (context), "minimum-height");
    }

  if (natural_height > priv->nat_height)
    {
      priv->nat_height = natural_height;

      g_object_notify (G_OBJECT (context), "natural-height");
    }

  g_object_thaw_notify (G_OBJECT (context));
}
