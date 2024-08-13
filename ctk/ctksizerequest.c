/* ctksizerequest.c
 * Copyright (C) 2007-2010 Openismus GmbH
 *
 * Authors:
 *      Mathias Hasselmann <mathias@openismus.com>
 *      Tristan Van Berkom <tristan.van.berkom@gmail.com>
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

#include <config.h>

#include "ctksizerequest.h"

#include "ctkdebug.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctksizegroup-private.h"
#include "ctksizerequestcacheprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkstyle.h"


#ifdef G_ENABLE_CONSISTENCY_CHECKS
static GQuark recursion_check_quark = 0;

static void
push_recursion_check (CtkWidget       *widget,
                      CtkOrientation   orientation,
                      gint             for_size)
{
  const char *previous_method;
  const char *method;

  if (recursion_check_quark == 0)
    recursion_check_quark = g_quark_from_static_string ("ctk-size-request-in-progress");

  previous_method = g_object_get_qdata (G_OBJECT (widget), recursion_check_quark);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      method = for_size < 0 ? "get_width" : "get_width_for_height";
    }
  else
    {
      method = for_size < 0 ? "get_height" : "get_height_for_width";
    }

  if (previous_method != NULL)
    {
      g_warning ("%s %p: widget tried to ctk_widget_%s inside "
                 " CtkWidget     ::%s implementation. "
                 "Should just invoke CTK_WIDGET_GET_CLASS(widget)->%s "
                 "directly rather than using ctk_widget_%s",
                 G_OBJECT_TYPE_NAME (widget), widget,
                 method, previous_method,
                 method, method);
    }

  g_object_set_qdata (G_OBJECT (widget), recursion_check_quark, (char*) method);
}

static void
pop_recursion_check (CtkWidget       *widget,
                     CtkOrientation   orientation G_GNUC_UNUSED)
{
  g_object_set_qdata (G_OBJECT (widget), recursion_check_quark, NULL);
}
#else
#define push_recursion_check(widget, orientation, for_size)
#define pop_recursion_check(widget, orientation)
#endif /* G_ENABLE_CONSISTENCY_CHECKS */

static const char *
get_vfunc_name (CtkOrientation orientation,
                gint           for_size)
{
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    return for_size < 0 ? "get_preferred_width" : "get_preferred_width_for_height";
  else
    return for_size < 0 ? "get_preferred_height" : "get_preferred_height_for_width";
}

static gboolean
widget_class_has_baseline_support (CtkWidgetClass *widget_class)
{
  CtkWidgetClass *parent_class;

  if (widget_class->get_preferred_height_and_baseline_for_width == NULL)
    return FALSE;

  /* This is kinda hacky, but for backwards compatibility reasons we have to handle the case
     where a class previously did not support get_preferred_height_and_baseline_for_width,
     but then gained support for it, and a subclass of it overrides the previous non-baseline
     methods. If this happens we need to call the overridden (non-baseline supporting) versions
     on the subclass, rather than the inherited but not overriddent new get_preferred_height_and_baseline_for_width.
  */

  /* Loop over all parent classes that inherit the same get_preferred_height_and_baseline_for_width */
  parent_class = g_type_class_peek_parent (widget_class);
  while (parent_class != NULL &&
	 parent_class->get_preferred_height_and_baseline_for_width == widget_class->get_preferred_height_and_baseline_for_width)
    {
      if (parent_class->get_preferred_height != widget_class->get_preferred_height ||
          parent_class->get_preferred_height_for_width != widget_class->get_preferred_height_for_width)
        return FALSE;

      parent_class = g_type_class_peek_parent (parent_class);
    }

  return TRUE;
}

gboolean
_ctk_widget_has_baseline_support (CtkWidget *widget)
{
  CtkWidgetClass *widget_class;

  widget_class = CTK_WIDGET_GET_CLASS (widget);

  return widget_class_has_baseline_support (widget_class);
}

static void
ctk_widget_query_size_for_orientation (CtkWidget        *widget,
                                       CtkOrientation    orientation,
                                       gint              for_size,
                                       gint             *minimum_size,
                                       gint             *natural_size,
                                       gint             *minimum_baseline,
                                       gint             *natural_baseline)
{
  SizeRequestCache *cache;
  CtkWidgetClass *widget_class;
  gint min_size = 0;
  gint nat_size = 0;
  gint min_baseline = -1;
  gint nat_baseline = -1;
  gboolean found_in_cache;

  ctk_widget_ensure_resize (widget);

  if (ctk_widget_get_request_mode (widget) == CTK_SIZE_REQUEST_CONSTANT_SIZE)
    for_size = -1;

  cache = _ctk_widget_peek_request_cache (widget);
  found_in_cache = _ctk_size_request_cache_lookup (cache,
                                                   orientation,
                                                   for_size,
                                                   &min_size,
                                                   &nat_size,
						   &min_baseline,
						   &nat_baseline);

  widget_class = CTK_WIDGET_GET_CLASS (widget);
  
  if (!found_in_cache)
    {
      gint adjusted_min, adjusted_natural, adjusted_for_size = for_size;

      ctk_widget_ensure_style (widget);

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          if (for_size < 0)
            {
	      push_recursion_check (widget, orientation, for_size);
              widget_class->get_preferred_width (widget, &min_size, &nat_size);
	      pop_recursion_check (widget, orientation);
            }
          else
            {
              gint ignored_position = 0;
              gint minimum_height;
              gint natural_height;

	      /* Pull the base natural height from the cache as it's needed to adjust
	       * the proposed 'for_size' */
	      ctk_widget_get_preferred_height (widget, &minimum_height, &natural_height);

              /* convert for_size to unadjusted height (for_size is a proposed allocation) */
              widget_class->adjust_size_allocation (widget,
						    CTK_ORIENTATION_VERTICAL,
						    &minimum_height,
						    &natural_height,
						    &ignored_position,
						    &adjusted_for_size);

	      push_recursion_check (widget, orientation, for_size);
              widget_class->get_preferred_width_for_height (widget,
							    MAX (adjusted_for_size, minimum_height),
							    &min_size, &nat_size);
	      pop_recursion_check (widget, orientation);
            }
        }
      else
        {
          if (for_size < 0)
            {
	      push_recursion_check (widget, orientation, for_size);
	      if (widget_class_has_baseline_support (widget_class))
		widget_class->get_preferred_height_and_baseline_for_width (widget, -1,
									   &min_size, &nat_size,
									   &min_baseline, &nat_baseline);
	      else
		widget_class->get_preferred_height (widget, &min_size, &nat_size);
	      pop_recursion_check (widget, orientation);
            }
          else
            {
              gint ignored_position = 0;
              gint minimum_width;
              gint natural_width;

	      /* Pull the base natural width from the cache as it's needed to adjust
	       * the proposed 'for_size' */
	      ctk_widget_get_preferred_width (widget, &minimum_width, &natural_width);

              /* convert for_size to unadjusted width (for_size is a proposed allocation) */
              widget_class->adjust_size_allocation (widget,
						    CTK_ORIENTATION_HORIZONTAL,
						    &minimum_width,
						    &natural_width,
						    &ignored_position,
						    &adjusted_for_size);

	      push_recursion_check (widget, orientation, for_size);
	      if (widget_class_has_baseline_support (widget_class))
		widget_class->get_preferred_height_and_baseline_for_width (widget, MAX (adjusted_for_size, minimum_width),
									   &min_size, &nat_size,
									   &min_baseline, &nat_baseline);
	      else
		widget_class->get_preferred_height_for_width (widget, MAX (adjusted_for_size, minimum_width),
							      &min_size, &nat_size);
	      pop_recursion_check (widget, orientation);
            }
        }

      if (min_size > nat_size)
        {
          g_warning ("%s %p reported min size %d and natural size %d in %s(); natural size must be >= min size",
                     G_OBJECT_TYPE_NAME (widget), widget, min_size, nat_size, get_vfunc_name (orientation, for_size));
        }

      adjusted_min     = min_size;
      adjusted_natural = nat_size;
      widget_class->adjust_size_request (widget,
					 orientation,
					 &adjusted_min,
					 &adjusted_natural);

      if (adjusted_min < min_size ||
          adjusted_natural < nat_size)
        {
          g_warning ("%s %p adjusted size %s min %d natural %d must not decrease below min %d natural %d",
                     G_OBJECT_TYPE_NAME (widget), widget,
                     orientation == CTK_ORIENTATION_VERTICAL ? "vertical" : "horizontal",
                     adjusted_min, adjusted_natural,
                     min_size, nat_size);
          /* don't use the adjustment */
        }
      else if (adjusted_min > adjusted_natural)
        {
          g_warning ("%s %p adjusted size %s min %d natural %d original min %d natural %d has min greater than natural",
                     G_OBJECT_TYPE_NAME (widget), widget,
                     orientation == CTK_ORIENTATION_VERTICAL ? "vertical" : "horizontal",
                     adjusted_min, adjusted_natural,
                     min_size, nat_size);
          /* don't use the adjustment */
        }
      else
        {
          /* adjustment looks good */
          min_size = adjusted_min;
          nat_size = adjusted_natural;
        }

      if (min_baseline != -1 || nat_baseline != -1)
	{
	  if (orientation == CTK_ORIENTATION_HORIZONTAL)
	    {
	      g_warning ("%s %p reported a horizontal baseline",
			 G_OBJECT_TYPE_NAME (widget), widget);
	      min_baseline = -1;
	      nat_baseline = -1;
	    }
	  else if (min_baseline == -1 || nat_baseline == -1)
	    {
	      g_warning ("%s %p reported baseline for only one of min/natural (min: %d, natural: %d)",
			 G_OBJECT_TYPE_NAME (widget), widget,
			 min_baseline, nat_baseline);
	      min_baseline = -1;
	      nat_baseline = -1;
	    }
	  else if (ctk_widget_get_valign_with_baseline (widget) != CTK_ALIGN_BASELINE)
	    {
	      /* Ignore requested baseline for non-aligned widgets */
	      min_baseline = -1;
	      nat_baseline = -1;
	    }
	  else
	    widget_class->adjust_baseline_request (widget,
						   &min_baseline,
						   &nat_baseline);
	}

      _ctk_size_request_cache_commit (cache,
                                      orientation,
                                      for_size,
                                      min_size,
                                      nat_size,
				      min_baseline,
				      nat_baseline);
    }

  if (minimum_size)
    *minimum_size = min_size;

  if (natural_size)
    *natural_size = nat_size;

  if (minimum_baseline)
    *minimum_baseline = min_baseline;

  if (natural_baseline)
    *natural_baseline = nat_baseline;

  g_assert (min_size <= nat_size);

  CTK_NOTE (SIZE_REQUEST, {
            GString *s;

            s = g_string_new ("");
            g_string_append_printf (s, "[%p] %s\t%s: %d is minimum %d and natural: %d",
                                    widget, G_OBJECT_TYPE_NAME (widget),
                                    orientation == CTK_ORIENTATION_HORIZONTAL
                                    ? "width for height"
                                    : "height for width",
                                    for_size, min_size, nat_size);
	    if (min_baseline != -1 || nat_baseline != -1)
              {
                g_string_append_printf (s, ", baseline %d/%d",
                                        min_baseline, nat_baseline);
              }
	    g_string_append_printf (s, " (hit cache: %s)\n",
		                    found_in_cache ? "yes" : "no");
            g_message ("%s", s->str);
            g_string_free (s, TRUE);
	    });
}

/* This is the main function that checks for a cached size and
 * possibly queries the widget class to compute the size if it's
 * not cached. If the for_size here is -1, then get_preferred_width()
 * or get_preferred_height() will be used.
 */
static void
ctk_widget_compute_size_for_orientation (CtkWidget        *widget,
                                         CtkOrientation    orientation,
                                         gint              for_size,
                                         gint             *minimum,
                                         gint             *natural,
                                         gint             *minimum_baseline,
                                         gint             *natural_baseline)
{
  GHashTable *widgets;
  GHashTableIter iter;
  gpointer key;
  gint    min_result = 0, nat_result = 0;

  if (!_ctk_widget_get_visible (widget) && !_ctk_widget_is_toplevel (widget))
    {
      if (minimum)
        *minimum = 0;
      if (natural)
        *natural = 0;
      if (minimum_baseline)
        *minimum_baseline = -1;
      if (natural_baseline)
        *natural_baseline = -1;
      return;
    }

  if (G_LIKELY (!_ctk_widget_get_sizegroups (widget)))
    {
      ctk_widget_query_size_for_orientation (widget, orientation, for_size, minimum, natural,
					     minimum_baseline, natural_baseline);
      return;
    }

  widgets = _ctk_size_group_get_widget_peers (widget, orientation);

  g_hash_table_iter_init (&iter, widgets);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      CtkWidget *tmp_widget = key;
      gint min_dimension, nat_dimension;

      ctk_widget_query_size_for_orientation (tmp_widget, orientation, for_size, &min_dimension, &nat_dimension, NULL, NULL);

      min_result = MAX (min_result, min_dimension);
      nat_result = MAX (nat_result, nat_dimension);
    }

  g_hash_table_destroy (widgets);

  /* Baselines make no sense with sizegroups really */
  if (minimum_baseline)
    *minimum_baseline = -1;

  if (natural_baseline)
    *natural_baseline = -1;

  if (minimum)
    *minimum = min_result;

  if (natural)
    *natural = nat_result;
}

/**
 * ctk_widget_get_request_mode:
 * @widget: a #CtkWidget instance
 *
 * Gets whether the widget prefers a height-for-width layout
 * or a width-for-height layout.
 *
 * #CtkBin widgets generally propagate the preference of
 * their child, container widgets need to request something either in
 * context of their children or in context of their allocation
 * capabilities.
 *
 * Returns: The #CtkSizeRequestMode preferred by @widget.
 *
 * Since: 3.0
 */
CtkSizeRequestMode
ctk_widget_get_request_mode (CtkWidget *widget)
{
  SizeRequestCache *cache;

  cache = _ctk_widget_peek_request_cache (widget);

  if (G_UNLIKELY (!cache->request_mode_valid))
    {
      cache->request_mode = CTK_WIDGET_GET_CLASS (widget)->get_request_mode (widget);
      cache->request_mode_valid = TRUE;
    }

  return cache->request_mode;
}

/**
 * ctk_widget_get_preferred_width:
 * @widget: a #CtkWidget instance
 * @minimum_width: (out) (allow-none): location to store the minimum width, or %NULL
 * @natural_width: (out) (allow-none): location to store the natural width, or %NULL
 *
 * Retrieves a widget’s initial minimum and natural width.
 *
 * This call is specific to height-for-width requests.
 *
 * The returned request will be modified by the
 * CtkWidgetClass::adjust_size_request virtual method and by any
 * #CtkSizeGroups that have been applied. That is, the returned request
 * is the one that should be used for layout, not necessarily the one
 * returned by the widget itself.
 *
 * Since: 3.0
 */
void
ctk_widget_get_preferred_width (CtkWidget *widget,
                                gint      *minimum_width,
                                gint      *natural_width)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (minimum_width != NULL || natural_width != NULL);

  ctk_widget_compute_size_for_orientation (widget,
                                           CTK_ORIENTATION_HORIZONTAL,
                                           -1,
                                           minimum_width,
                                           natural_width,
                                           NULL, NULL);
}


/**
 * ctk_widget_get_preferred_height:
 * @widget: a #CtkWidget instance
 * @minimum_height: (out) (allow-none): location to store the minimum height, or %NULL
 * @natural_height: (out) (allow-none): location to store the natural height, or %NULL
 *
 * Retrieves a widget’s initial minimum and natural height.
 *
 * This call is specific to width-for-height requests.
 *
 * The returned request will be modified by the
 * CtkWidgetClass::adjust_size_request virtual method and by any
 * #CtkSizeGroups that have been applied. That is, the returned request
 * is the one that should be used for layout, not necessarily the one
 * returned by the widget itself.
 *
 * Since: 3.0
 */
void
ctk_widget_get_preferred_height (CtkWidget *widget,
                                 gint      *minimum_height,
                                 gint      *natural_height)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (minimum_height != NULL || natural_height != NULL);

  ctk_widget_compute_size_for_orientation (widget,
                                           CTK_ORIENTATION_VERTICAL,
                                           -1,
                                           minimum_height,
                                           natural_height,
                                           NULL, NULL);
}



/**
 * ctk_widget_get_preferred_width_for_height:
 * @widget: a #CtkWidget instance
 * @height: the height which is available for allocation
 * @minimum_width: (out) (allow-none): location for storing the minimum width, or %NULL
 * @natural_width: (out) (allow-none): location for storing the natural width, or %NULL
 *
 * Retrieves a widget’s minimum and natural width if it would be given
 * the specified @height.
 *
 * The returned request will be modified by the
 * CtkWidgetClass::adjust_size_request virtual method and by any
 * #CtkSizeGroups that have been applied. That is, the returned request
 * is the one that should be used for layout, not necessarily the one
 * returned by the widget itself.
 *
 * Since: 3.0
 */
void
ctk_widget_get_preferred_width_for_height (CtkWidget *widget,
                                           gint       height,
                                           gint      *minimum_width,
                                           gint      *natural_width)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (minimum_width != NULL || natural_width != NULL);
  g_return_if_fail (height >= 0);

  ctk_widget_compute_size_for_orientation (widget,
                                           CTK_ORIENTATION_HORIZONTAL,
                                           height,
                                           minimum_width,
                                           natural_width,
                                           NULL, NULL);
}

/**
 * ctk_widget_get_preferred_height_for_width:
 * @widget: a #CtkWidget instance
 * @width: the width which is available for allocation
 * @minimum_height: (out) (allow-none): location for storing the minimum height, or %NULL
 * @natural_height: (out) (allow-none): location for storing the natural height, or %NULL
 *
 * Retrieves a widget’s minimum and natural height if it would be given
 * the specified @width.
 *
 * The returned request will be modified by the
 * CtkWidgetClass::adjust_size_request virtual method and by any
 * #CtkSizeGroups that have been applied. That is, the returned request
 * is the one that should be used for layout, not necessarily the one
 * returned by the widget itself.
 *
 * Since: 3.0
 */
void
ctk_widget_get_preferred_height_for_width (CtkWidget *widget,
                                           gint       width,
                                           gint      *minimum_height,
                                           gint      *natural_height)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (minimum_height != NULL || natural_height != NULL);
  g_return_if_fail (width >= 0);

  ctk_widget_compute_size_for_orientation (widget,
                                           CTK_ORIENTATION_VERTICAL,
                                           width,
                                           minimum_height,
                                           natural_height,
                                           NULL, NULL);
}

/**
 * ctk_widget_get_preferred_height_and_baseline_for_width:
 * @widget: a #CtkWidget instance
 * @width: the width which is available for allocation, or -1 if none
 * @minimum_height: (out) (allow-none): location for storing the minimum height, or %NULL
 * @natural_height: (out) (allow-none): location for storing the natural height, or %NULL
 * @minimum_baseline: (out) (allow-none): location for storing the baseline for the minimum height, or %NULL
 * @natural_baseline: (out) (allow-none): location for storing the baseline for the natural height, or %NULL
 *
 * Retrieves a widget’s minimum and natural height and the corresponding baselines if it would be given
 * the specified @width, or the default height if @width is -1. The baselines may be -1 which means
 * that no baseline is requested for this widget.
 *
 * The returned request will be modified by the
 * CtkWidgetClass::adjust_size_request and CtkWidgetClass::adjust_baseline_request virtual methods
 * and by any #CtkSizeGroups that have been applied. That is, the returned request
 * is the one that should be used for layout, not necessarily the one
 * returned by the widget itself.
 *
 * Since: 3.10
 */
void
ctk_widget_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
							gint       width,
							gint      *minimum_height,
							gint      *natural_height,
							gint      *minimum_baseline,
							gint      *natural_baseline)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (minimum_height != NULL || natural_height != NULL);
  g_return_if_fail (width >= -1);

  ctk_widget_compute_size_for_orientation (widget,
                                           CTK_ORIENTATION_VERTICAL,
                                           width,
                                           minimum_height,
                                           natural_height,
                                           minimum_baseline,
                                           natural_baseline);
}

/*
 * _ctk_widget_get_preferred_size_and_baseline:
 * @widget: a #CtkWidget instance
 * @minimum_size: (out) (allow-none): location for storing the minimum size, or %NULL
 * @natural_size: (out) (allow-none): location for storing the natural size, or %NULL
 *
 * Retrieves the minimum and natural size  and the corresponding baselines of a widget, taking
 * into account the widget’s preference for height-for-width management. The baselines may
 * be -1 which means that no baseline is requested for this widget.
 *
 * This is used to retrieve a suitable size by container widgets which do
 * not impose any restrictions on the child placement. It can be used
 * to deduce toplevel window and menu sizes as well as child widgets in
 * free-form containers such as CtkLayout.
 *
 * Handle with care. Note that the natural height of a height-for-width
 * widget will generally be a smaller size than the minimum height, since the required
 * height for the natural width is generally smaller than the required height for
 * the minimum width.
 */
void
_ctk_widget_get_preferred_size_and_baseline (CtkWidget      *widget,
                                             CtkRequisition *minimum_size,
                                             CtkRequisition *natural_size,
                                             gint           *minimum_baseline,
                                             gint           *natural_baseline)
{
  gint min_width, nat_width;
  gint min_height, nat_height;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (ctk_widget_get_request_mode (widget) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
    {
      ctk_widget_get_preferred_width (widget, &min_width, &nat_width);

      if (minimum_size)
	{
	  minimum_size->width = min_width;
	  ctk_widget_get_preferred_height_and_baseline_for_width (widget, min_width,
								  &minimum_size->height, NULL, minimum_baseline, NULL);
	}

      if (natural_size)
	{
	  natural_size->width = nat_width;
	  ctk_widget_get_preferred_height_and_baseline_for_width (widget, nat_width,
								  NULL, &natural_size->height, NULL, natural_baseline);
	}
    }
  else /* CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT or CONSTANT_SIZE */
    {
      ctk_widget_get_preferred_height_and_baseline_for_width (widget, -1, &min_height, &nat_height, minimum_baseline, natural_baseline);

      if (minimum_size)
	{
	  minimum_size->height = min_height;
	  ctk_widget_get_preferred_width_for_height (widget, min_height,
                                                     &minimum_size->width, NULL);
	}

      if (natural_size)
	{
	  natural_size->height = nat_height;
	  ctk_widget_get_preferred_width_for_height (widget, nat_height,
                                                     NULL, &natural_size->width);
	}
    }
}

/**
 * ctk_widget_get_preferred_size:
 * @widget: a #CtkWidget instance
 * @minimum_size: (out) (allow-none): location for storing the minimum size, or %NULL
 * @natural_size: (out) (allow-none): location for storing the natural size, or %NULL
 *
 * Retrieves the minimum and natural size of a widget, taking
 * into account the widget’s preference for height-for-width management.
 *
 * This is used to retrieve a suitable size by container widgets which do
 * not impose any restrictions on the child placement. It can be used
 * to deduce toplevel window and menu sizes as well as child widgets in
 * free-form containers such as CtkLayout.
 *
 * Handle with care. Note that the natural height of a height-for-width
 * widget will generally be a smaller size than the minimum height, since the required
 * height for the natural width is generally smaller than the required height for
 * the minimum width.
 *
 * Use ctk_widget_get_preferred_height_and_baseline_for_width() if you want to support
 * baseline alignment.
 *
 * Since: 3.0
 */
void
ctk_widget_get_preferred_size (CtkWidget      *widget,
                               CtkRequisition *minimum_size,
                               CtkRequisition *natural_size)
{
  _ctk_widget_get_preferred_size_and_baseline (widget, minimum_size, natural_size,
                                               NULL, NULL);
}

static gint
compare_gap (gconstpointer p1,
	     gconstpointer p2,
	     gpointer      data)
{
  CtkRequestedSize *sizes = data;
  const guint *c1 = p1;
  const guint *c2 = p2;

  const gint d1 = MAX (sizes[*c1].natural_size -
                       sizes[*c1].minimum_size,
                       0);
  const gint d2 = MAX (sizes[*c2].natural_size -
                       sizes[*c2].minimum_size,
                       0);

  gint delta = (d2 - d1);

  if (0 == delta)
    delta = (*c2 - *c1);

  return delta;
}

/**
 * ctk_distribute_natural_allocation:
 * @extra_space: Extra space to redistribute among children after subtracting
 *               minimum sizes and any child padding from the overall allocation
 * @n_requested_sizes: Number of requests to fit into the allocation
 * @sizes: An array of structs with a client pointer and a minimum/natural size
 *         in the orientation of the allocation.
 *
 * Distributes @extra_space to child @sizes by bringing smaller
 * children up to natural size first.
 *
 * The remaining space will be added to the @minimum_size member of the
 * CtkRequestedSize struct. If all sizes reach their natural size then
 * the remaining space is returned.
 *
 * Returns: The remainder of @extra_space after redistributing space
 * to @sizes.
 */
gint
ctk_distribute_natural_allocation (gint              extra_space,
				   guint             n_requested_sizes,
				   CtkRequestedSize *sizes)
{
  guint *spreading;
  gint   i;

  g_return_val_if_fail (extra_space >= 0, 0);

  spreading = g_newa (guint, n_requested_sizes);

  for (i = 0; i < n_requested_sizes; i++)
    spreading[i] = i;

  /* Distribute the container's extra space c_gap. We want to assign
   * this space such that the sum of extra space assigned to children
   * (c^i_gap) is equal to c_cap. The case that there's not enough
   * space for all children to take their natural size needs some
   * attention. The goals we want to achieve are:
   *
   *   a) Maximize number of children taking their natural size.
   *   b) The allocated size of children should be a continuous
   *   function of c_gap.  That is, increasing the container size by
   *   one pixel should never make drastic changes in the distribution.
   *   c) If child i takes its natural size and child j doesn't,
   *   child j should have received at least as much gap as child i.
   *
   * The following code distributes the additional space by following
   * these rules.
   */

  /* Sort descending by gap and position. */
  g_qsort_with_data (spreading,
		     n_requested_sizes, sizeof (guint),
		     compare_gap, sizes);

  /* Distribute available space.
   * This master piece of a loop was conceived by Behdad Esfahbod.
   */
  for (i = n_requested_sizes - 1; extra_space > 0 && i >= 0; --i)
    {
      /* Divide remaining space by number of remaining children.
       * Sort order and reducing remaining space by assigned space
       * ensures that space is distributed equally.
       */
      gint glue = (extra_space + i) / (i + 1);
      gint gap = sizes[(spreading[i])].natural_size
	- sizes[(spreading[i])].minimum_size;

      gint extra = MIN (glue, gap);

      sizes[spreading[i]].minimum_size += extra;

      extra_space -= extra;
    }

  return extra_space;
}

void
_ctk_widget_get_preferred_size_for_size (CtkWidget      *widget,
                                         CtkOrientation  orientation,
                                         gint            size,
                                         gint           *minimum,
                                         gint           *natural,
                                         gint           *minimum_baseline,
                                         gint           *natural_baseline)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (size >= -1);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (size < 0)
        ctk_widget_get_preferred_width (widget, minimum, natural);
      else
        ctk_widget_get_preferred_width_for_height (widget, size, minimum, natural);

      if (minimum_baseline)
        *minimum_baseline = -1;
      if (natural_baseline)
        *natural_baseline = -1;
    }
  else
    {
      ctk_widget_get_preferred_height_and_baseline_for_width (widget,
                                                              size,
                                                              minimum,
                                                              natural,
                                                              minimum_baseline,
                                                              natural_baseline);
    }
}

