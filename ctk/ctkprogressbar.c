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

#include <string.h>

#include "ctkprogressbar.h"
#include "ctkorientableprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkprogresstrackerprivate.h"

#include "a11y/ctkprogressbaraccessible.h"

#include "fallback-c89.c"

/**
 * SECTION:ctkprogressbar
 * @Short_description: A widget which indicates progress visually
 * @Title: CtkProgressBar
 *
 * The #CtkProgressBar is typically used to display the progress of a long
 * running operation. It provides a visual clue that processing is underway.
 * The CtkProgressBar can be used in two different modes: percentage mode
 * and activity mode.
 *
 * When an application can determine how much work needs to take place
 * (e.g. read a fixed number of bytes from a file) and can monitor its
 * progress, it can use the CtkProgressBar in percentage mode and the
 * user sees a growing bar indicating the percentage of the work that
 * has been completed. In this mode, the application is required to call
 * ctk_progress_bar_set_fraction() periodically to update the progress bar.
 *
 * When an application has no accurate way of knowing the amount of work
 * to do, it can use the #CtkProgressBar in activity mode, which shows
 * activity by a block moving back and forth within the progress area. In
 * this mode, the application is required to call ctk_progress_bar_pulse()
 * periodically to update the progress bar.
 *
 * There is quite a bit of flexibility provided to control the appearance
 * of the #CtkProgressBar. Functions are provided to control the orientation
 * of the bar, optional text can be displayed along with the bar, and the
 * step size used in activity mode can be set.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * progressbar[.osd]
 * ├── [text]
 * ╰── trough[.empty][.full]
 *     ╰── progress[.pulse]
 * ]|
 *
 * CtkProgressBar has a main CSS node with name progressbar and subnodes with
 * names text and trough, of which the latter has a subnode named progress. The
 * text subnode is only present if text is shown. The progress subnode has the
 * style class .pulse when in activity mode. It gets the style classes .left,
 * .right, .top or .bottom added when the progress 'touches' the corresponding
 * end of the CtkProgressBar. The .osd class on the progressbar node is for use
 * in overlays like the one Epiphany has for page loading progress.
 */

#define MIN_HORIZONTAL_BAR_WIDTH   150
#define MIN_HORIZONTAL_BAR_HEIGHT  6
#define MIN_VERTICAL_BAR_WIDTH     7
#define MIN_VERTICAL_BAR_HEIGHT    80

#define DEFAULT_PULSE_DURATION     250000000

struct _CtkProgressBarPrivate
{
  gchar         *text;

  CtkCssGadget  *gadget;
  CtkCssGadget  *text_gadget;
  CtkCssGadget  *trough_gadget;
  CtkCssGadget  *progress_gadget;

  gdouble        fraction;
  gdouble        pulse_fraction;

  double         activity_pos;
  guint          activity_blocks;

  CtkOrientation orientation;

  guint              tick_id;
  CtkProgressTracker tracker;
  gint64             pulse1;
  gint64             pulse2;
  gdouble            last_iteration;

  guint          activity_dir  : 1;
  guint          activity_mode : 1;
  guint          ellipsize     : 3;
  guint          show_text     : 1;
  guint          inverted      : 1;
};

enum {
  PROP_0,
  PROP_FRACTION,
  PROP_PULSE_STEP,
  PROP_INVERTED,
  PROP_TEXT,
  PROP_SHOW_TEXT,
  PROP_ELLIPSIZE,
  PROP_ORIENTATION,
  NUM_PROPERTIES = PROP_ORIENTATION
};

static GParamSpec *progress_props[NUM_PROPERTIES] = { NULL, };

static void ctk_progress_bar_set_property         (GObject        *object,
                                                   guint           prop_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void ctk_progress_bar_get_property         (GObject        *object,
                                                   guint           prop_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);
static void ctk_progress_bar_size_allocate        (CtkWidget      *widget,
                                                   CtkAllocation  *allocation);
static void ctk_progress_bar_get_preferred_width  (CtkWidget      *widget,
                                                   gint           *minimum,
                                                   gint           *natural);
static void ctk_progress_bar_get_preferred_height (CtkWidget      *widget,
                                                   gint           *minimum,
                                                   gint           *natural);

static gboolean ctk_progress_bar_draw             (CtkWidget      *widget,
                                                   cairo_t        *cr);
static void     ctk_progress_bar_act_mode_enter   (CtkProgressBar *progress);
static void     ctk_progress_bar_act_mode_leave   (CtkProgressBar *progress);
static void     ctk_progress_bar_finalize         (GObject        *object);
static void     ctk_progress_bar_set_orientation  (CtkProgressBar *progress,
                                                   CtkOrientation  orientation);
static void     ctk_progress_bar_direction_changed (CtkWidget        *widget,
                                                    CtkTextDirection  previous_dir);
static void     ctk_progress_bar_state_flags_changed (CtkWidget      *widget,
                                                      CtkStateFlags   previous_state);

static void     ctk_progress_bar_measure           (CtkCssGadget        *gadget,
                                                    CtkOrientation       orientation,
                                                    gint                 for_size,
                                                    gint                *minimum,
                                                    gint                *natural,
                                                    gint                *minimum_baseline,
                                                    gint                *natural_baseline,
                                                    gpointer             data);
static void     ctk_progress_bar_allocate          (CtkCssGadget        *gadget,
                                                    const CtkAllocation *allocation,
                                                    gint                 baseline,
                                                    CtkAllocation       *out_clip,
                                                    gpointer             data);
static gboolean ctk_progress_bar_render            (CtkCssGadget        *gadget,
                                                    cairo_t             *cr,
                                                    gint                 x,
                                                    gint                 y,
                                                    gint                 width,
                                                    gint                 height,
                                                    gpointer             data);
static void     ctk_progress_bar_allocate_trough   (CtkCssGadget        *gadget,
                                                    const CtkAllocation *allocation,
                                                    gint                 baseline,
                                                    CtkAllocation       *out_clip,
                                                    gpointer             data);
static void     ctk_progress_bar_measure_trough    (CtkCssGadget        *gadget,
                                                    CtkOrientation       orientation,
                                                    gint                 for_size,
                                                    gint                *minimum,
                                                    gint                *natural,
                                                    gint                *minimum_baseline,
                                                    gint                *natural_baseline,
                                                    gpointer             data);
static gboolean ctk_progress_bar_render_trough     (CtkCssGadget        *gadget,
                                                    cairo_t             *cr,
                                                    gint                 x,
                                                    gint                 y,
                                                    gint                 width,
                                                    gint                 height,
                                                    gpointer             data);
static void     ctk_progress_bar_measure_progress  (CtkCssGadget        *gadget,
                                                    CtkOrientation       orientation,
                                                    gint                 for_size,
                                                    gint                *minimum,
                                                    gint                *natural,
                                                    gint                *minimum_baseline,
                                                    gint                *natural_baseline,
                                                    gpointer             data);
static void     ctk_progress_bar_measure_text      (CtkCssGadget        *gadget,
                                                    CtkOrientation       orientation,
                                                    gint                 for_size,
                                                    gint                *minimum,
                                                    gint                *natural,
                                                    gint                *minimum_baseline,
                                                    gint                *natural_baseline,
                                                    gpointer             data);
static gboolean ctk_progress_bar_render_text       (CtkCssGadget        *gadget,
                                                    cairo_t             *cr,
                                                    gint                 x,
                                                    gint                 y,
                                                    gint                 width,
                                                    gint                 height,
                                                    gpointer             data);

G_DEFINE_TYPE_WITH_CODE (CtkProgressBar, ctk_progress_bar, CTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (CtkProgressBar)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL))

static void
ctk_progress_bar_class_init (CtkProgressBarClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (CtkWidgetClass *) class;

  gobject_class->set_property = ctk_progress_bar_set_property;
  gobject_class->get_property = ctk_progress_bar_get_property;
  gobject_class->finalize = ctk_progress_bar_finalize;

  widget_class->draw = ctk_progress_bar_draw;
  widget_class->size_allocate = ctk_progress_bar_size_allocate;
  widget_class->get_preferred_width = ctk_progress_bar_get_preferred_width;
  widget_class->get_preferred_height = ctk_progress_bar_get_preferred_height;
  widget_class->direction_changed = ctk_progress_bar_direction_changed;
  widget_class->state_flags_changed = ctk_progress_bar_state_flags_changed;

  g_object_class_override_property (gobject_class, PROP_ORIENTATION, "orientation");

  progress_props[PROP_INVERTED] =
      g_param_spec_boolean ("inverted",
                            P_("Inverted"),
                            P_("Invert the direction in which the progress bar grows"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  progress_props[PROP_FRACTION] =
      g_param_spec_double ("fraction",
                           P_("Fraction"),
                           P_("The fraction of total work that has been completed"),
                           0.0, 1.0,
                           0.0,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  progress_props[PROP_PULSE_STEP] =
      g_param_spec_double ("pulse-step",
                           P_("Pulse Step"),
                           P_("The fraction of total progress to move the bouncing block when pulsed"),
                           0.0, 1.0,
                           0.1,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  progress_props[PROP_TEXT] =
      g_param_spec_string ("text",
                           P_("Text"),
                           P_("Text to be displayed in the progress bar"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkProgressBar:show-text:
   *
   * Sets whether the progress bar will show a text in addition
   * to the bar itself. The shown text is either the value of
   * the #CtkProgressBar:text property or, if that is %NULL,
   * the #CtkProgressBar:fraction value, as a percentage.
   *
   * To make a progress bar that is styled and sized suitably for
   * showing text (even if the actual text is blank), set
   * #CtkProgressBar:show-text to %TRUE and #CtkProgressBar:text
   * to the empty string (not %NULL).
   *
   * Since: 3.0
   */
  progress_props[PROP_SHOW_TEXT] =
      g_param_spec_boolean ("show-text",
                            P_("Show text"),
                            P_("Whether the progress is shown as text."),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkProgressBar:ellipsize:
   *
   * The preferred place to ellipsize the string, if the progress bar does
   * not have enough room to display the entire string, specified as a
   * #PangoEllipsizeMode.
   *
   * Note that setting this property to a value other than
   * %PANGO_ELLIPSIZE_NONE has the side-effect that the progress bar requests
   * only enough space to display the ellipsis ("..."). Another means to set a
   * progress bar's width is ctk_widget_set_size_request().
   *
   * Since: 2.6
   */
  progress_props[PROP_ELLIPSIZE] =
      g_param_spec_enum ("ellipsize",
                         P_("Ellipsize"),
                         P_("The preferred place to ellipsize the string, if the progress bar "
                            "does not have enough room to display the entire string, if at all."),
                         PANGO_TYPE_ELLIPSIZE_MODE,
                         PANGO_ELLIPSIZE_NONE,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, progress_props);

  /**
   * CtkProgressBar:xspacing:
   *
   * Extra spacing applied to the width of a progress bar.
   *
   * Deprecated: 3.20: Use the standard CSS padding and margins; the
   *     value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("xspacing",
                                                             P_("X spacing"),
                                                             P_("Extra spacing applied to the width of a progress bar."),
                                                             0, G_MAXINT, 2,
                                                             G_PARAM_READWRITE|G_PARAM_DEPRECATED));

  /**
   * CtkProgressBar:yspacing:
   *
   * Extra spacing applied to the height of a progress bar.
   *
   * Deprecated: 3.20: Use the standard CSS padding and margins; the
   *     value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("yspacing",
                                                             P_("Y spacing"),
                                                             P_("Extra spacing applied to the height of a progress bar."),
                                                             0, G_MAXINT, 2,
                                                             G_PARAM_READWRITE|G_PARAM_DEPRECATED));

  /**
   * CtkProgressBar:min-horizontal-bar-width:
   *
   * The minimum horizontal width of the progress bar.
   *
   * Since: 2.14
   *
   * Deprecated: 3.20: Use the standard CSS property min-width.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-horizontal-bar-width",
                                                             P_("Minimum horizontal bar width"),
                                                             P_("The minimum horizontal width of the progress bar"),
                                                             1, G_MAXINT, MIN_HORIZONTAL_BAR_WIDTH,
                                                             G_PARAM_READWRITE|G_PARAM_DEPRECATED));
  /**
   * CtkProgressBar:min-horizontal-bar-height:
   *
   * Minimum horizontal height of the progress bar.
   *
   * Since: 2.14
   *
   * Deprecated: 3.20: Use the standard CSS property min-height.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-horizontal-bar-height",
                                                             P_("Minimum horizontal bar height"),
                                                             P_("Minimum horizontal height of the progress bar"),
                                                             1, G_MAXINT, MIN_HORIZONTAL_BAR_HEIGHT,
                                                             G_PARAM_READWRITE|G_PARAM_DEPRECATED));
  /**
   * CtkProgressBar:min-vertical-bar-width:
   *
   * The minimum vertical width of the progress bar.
   *
   * Since: 2.14
   *
   * Deprecated: 3.20: Use the standard CSS property min-width.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-vertical-bar-width",
                                                             P_("Minimum vertical bar width"),
                                                             P_("The minimum vertical width of the progress bar"),
                                                             1, G_MAXINT, MIN_VERTICAL_BAR_WIDTH,
                                                             G_PARAM_READWRITE|G_PARAM_DEPRECATED));
  /**
   * CtkProgressBar:min-vertical-bar-height:
   *
   * The minimum vertical height of the progress bar.
   *
   * Since: 2.14
   *
   * Deprecated: 3.20: Use the standard CSS property min-height.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-vertical-bar-height",
                                                             P_("Minimum vertical bar height"),
                                                             P_("The minimum vertical height of the progress bar"),
                                                             1, G_MAXINT, MIN_VERTICAL_BAR_HEIGHT,
                                                             G_PARAM_READWRITE|G_PARAM_DEPRECATED));

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_PROGRESS_BAR_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "progressbar");
}

static void
update_fraction_classes (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv = pbar->priv;
  gboolean empty = FALSE;
  gboolean full = FALSE;

  /* Here we set classes based on fill-level unless we're in activity-mode.
   */

  if (!priv->activity_mode)
    {
      if (priv->fraction <= 0.0)
        empty = TRUE;
      else if (priv->fraction >= 1.0)
        full = TRUE;
    }

  if (empty)
    ctk_css_gadget_add_class (priv->trough_gadget, "empty");
  else
    ctk_css_gadget_remove_class (priv->trough_gadget, "empty");

  if (full)
    ctk_css_gadget_add_class (priv->trough_gadget, "full");
  else
    ctk_css_gadget_remove_class (priv->trough_gadget, "full");
}

static void
update_node_classes (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv = pbar->priv;
  gboolean left = FALSE;
  gboolean right = FALSE;
  gboolean top = FALSE;
  gboolean bottom = FALSE;

  /* Here we set positional classes, depending on which end of the
   * progressbar the progress touches.
   */

  if (priv->activity_mode)
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          left = priv->activity_pos <= 0.0;
          right = priv->activity_pos >= 1.0;
        }
      else
        {
          top = priv->activity_pos <= 0.0;
          bottom = priv->activity_pos >= 1.0;
        }
    }
  else /* continuous */
    {
      gboolean inverted;

      inverted = priv->inverted;
      if (ctk_widget_get_direction (CTK_WIDGET (pbar)) == CTK_TEXT_DIR_RTL)
        {
          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
            inverted = !inverted;
        }

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          left = !inverted || priv->fraction >= 1.0;
          right = inverted || priv->fraction >= 1.0;
        }
      else
        {
          top = !inverted || priv->fraction >= 1.0;
          bottom = inverted || priv->fraction >= 1.0;
        }
    }

  if (left)
    ctk_css_gadget_add_class (priv->progress_gadget, CTK_STYLE_CLASS_LEFT);
  else
    ctk_css_gadget_remove_class (priv->progress_gadget, CTK_STYLE_CLASS_LEFT);

  if (right)
    ctk_css_gadget_add_class (priv->progress_gadget, CTK_STYLE_CLASS_RIGHT);
  else
    ctk_css_gadget_remove_class (priv->progress_gadget, CTK_STYLE_CLASS_RIGHT);

  if (top)
    ctk_css_gadget_add_class (priv->progress_gadget, CTK_STYLE_CLASS_TOP);
  else
    ctk_css_gadget_remove_class (priv->progress_gadget, CTK_STYLE_CLASS_TOP);

  if (bottom)
    ctk_css_gadget_add_class (priv->progress_gadget, CTK_STYLE_CLASS_BOTTOM);
  else
    ctk_css_gadget_remove_class (priv->progress_gadget, CTK_STYLE_CLASS_BOTTOM);

  update_fraction_classes (pbar);
}

static void
update_node_state (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv = pbar->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (CTK_WIDGET (pbar));

  ctk_css_gadget_set_state (priv->gadget, state);
  ctk_css_gadget_set_state (priv->trough_gadget, state);
  ctk_css_gadget_set_state (priv->progress_gadget, state);
  if (priv->text_gadget)
    ctk_css_gadget_set_state (priv->text_gadget, state);
}

static void
ctk_progress_bar_init (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv;
  CtkCssNode *widget_node;

  pbar->priv = ctk_progress_bar_get_instance_private (pbar);
  priv = pbar->priv;

  priv->orientation = CTK_ORIENTATION_HORIZONTAL;
  priv->inverted = FALSE;
  priv->pulse_fraction = 0.1;
  priv->activity_pos = 0;
  priv->activity_dir = 1;
  priv->activity_blocks = 5;
  priv->ellipsize = PANGO_ELLIPSIZE_NONE;
  priv->show_text = FALSE;

  priv->text = NULL;
  priv->fraction = 0.0;

  ctk_widget_set_has_window (CTK_WIDGET (pbar), FALSE);

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (pbar));

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (pbar));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (pbar),
                                                     ctk_progress_bar_measure,
                                                     ctk_progress_bar_allocate,
                                                     ctk_progress_bar_render,
                                                     NULL,
                                                     NULL);

  priv->trough_gadget = ctk_css_custom_gadget_new ("trough",
                                                   CTK_WIDGET (pbar),
                                                   priv->gadget,
                                                   NULL,
                                                   ctk_progress_bar_measure_trough,
                                                   ctk_progress_bar_allocate_trough,
                                                   ctk_progress_bar_render_trough,
                                                   NULL,
                                                   NULL);

  priv->progress_gadget = ctk_css_custom_gadget_new ("progress",
                                                     CTK_WIDGET (pbar),
                                                     priv->trough_gadget,
                                                     NULL,
                                                     ctk_progress_bar_measure_progress,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL);

  update_node_state (pbar);
  update_node_classes (pbar);
}

static void
ctk_progress_bar_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  CtkProgressBar *pbar;

  pbar = CTK_PROGRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      ctk_progress_bar_set_orientation (pbar, g_value_get_enum (value));
      break;
    case PROP_INVERTED:
      ctk_progress_bar_set_inverted (pbar, g_value_get_boolean (value));
      break;
    case PROP_FRACTION:
      ctk_progress_bar_set_fraction (pbar, g_value_get_double (value));
      break;
    case PROP_PULSE_STEP:
      ctk_progress_bar_set_pulse_step (pbar, g_value_get_double (value));
      break;
    case PROP_TEXT:
      ctk_progress_bar_set_text (pbar, g_value_get_string (value));
      break;
    case PROP_SHOW_TEXT:
      ctk_progress_bar_set_show_text (pbar, g_value_get_boolean (value));
      break;
    case PROP_ELLIPSIZE:
      ctk_progress_bar_set_ellipsize (pbar, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_progress_bar_get_property (GObject      *object,
                               guint         prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  CtkProgressBar *pbar = CTK_PROGRESS_BAR (object);
  CtkProgressBarPrivate* priv = pbar->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, priv->inverted);
      break;
    case PROP_FRACTION:
      g_value_set_double (value, priv->fraction);
      break;
    case PROP_PULSE_STEP:
      g_value_set_double (value, priv->pulse_fraction);
      break;
    case PROP_TEXT:
      g_value_set_string (value, priv->text);
      break;
    case PROP_SHOW_TEXT:
      g_value_set_boolean (value, priv->show_text);
      break;
    case PROP_ELLIPSIZE:
      g_value_set_enum (value, priv->ellipsize);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_progress_bar_new:
 *
 * Creates a new #CtkProgressBar.
 *
 * Returns: a #CtkProgressBar.
 */
CtkWidget*
ctk_progress_bar_new (void)
{
  CtkWidget *pbar;

  pbar = g_object_new (CTK_TYPE_PROGRESS_BAR, NULL);

  return pbar;
}

static void
ctk_progress_bar_finalize (GObject *object)
{
  CtkProgressBar *pbar = CTK_PROGRESS_BAR (object);
  CtkProgressBarPrivate *priv = pbar->priv;

  if (priv->activity_mode)
    ctk_progress_bar_act_mode_leave (pbar);

  g_free (priv->text);

  g_clear_object (&priv->text_gadget);
  g_clear_object (&priv->progress_gadget);
  g_clear_object (&priv->trough_gadget);
  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_progress_bar_parent_class)->finalize (object);
}

static gchar *
get_current_text (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv = pbar->priv;

  if (priv->text)
    return g_strdup (priv->text);
  else
    return g_strdup_printf (C_("progress bar label", "%.0f %%"), priv->fraction * 100.0);
}

static void
ctk_progress_bar_measure (CtkCssGadget   *gadget,
                          CtkOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline,
                          gpointer        data)
{
  CtkWidget *widget;
  CtkProgressBar *pbar;
  CtkProgressBarPrivate *priv;
  gint text_minimum, text_natural;
  gint trough_minimum, trough_natural;

  widget = ctk_css_gadget_get_owner (gadget);
  pbar = CTK_PROGRESS_BAR (widget);
  priv = pbar->priv;

  if (priv->show_text)
    ctk_css_gadget_get_preferred_size (priv->text_gadget,
                                       orientation,
                                       -1,
                                       &text_minimum, &text_natural,
                                       NULL, NULL);
  else
    text_minimum = text_natural = 0;

  ctk_css_gadget_get_preferred_size (priv->trough_gadget,
                                     orientation,
                                     -1,
                                     &trough_minimum, &trough_natural,
                                     NULL, NULL);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          *minimum = MAX (text_minimum, trough_minimum);
          *natural = MAX (text_natural, trough_natural);
        }
      else
        {
          *minimum = text_minimum + trough_minimum;
          *natural = text_natural + trough_natural;
        }
    }
  else
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          *minimum = text_minimum + trough_minimum;
          *natural = text_natural + trough_natural;
        }
      else
        {
          *minimum = MAX (text_minimum, trough_minimum);
          *natural = MAX (text_natural, trough_natural);
        }
    }
}

static PangoLayout *
ctk_progress_bar_get_layout (CtkProgressBar *pbar)
{
  PangoLayout *layout;
  gchar *buf;
  CtkCssStyle *style;
  PangoAttrList *attrs;
  PangoFontDescription *desc;

  buf = get_current_text (pbar);
  layout = ctk_widget_create_pango_layout (CTK_WIDGET (pbar), buf);

  style = ctk_css_node_get_style (ctk_css_gadget_get_node (pbar->priv->text_gadget));

  attrs = ctk_css_style_get_pango_attributes (style);
  desc = ctk_css_style_get_pango_font (style);

  pango_layout_set_attributes (layout, attrs);
  pango_layout_set_font_description (layout, desc);

  if (attrs)
    pango_attr_list_unref (attrs);
  pango_font_description_free (desc);

  g_free (buf);

  return layout;
}

static void
ctk_progress_bar_measure_text (CtkCssGadget   *gadget,
                               CtkOrientation  orientation,
                               int             for_size,
                               int            *minimum,
                               int            *natural,
                               int            *minimum_baseline,
                               int            *natural_baseline,
                               gpointer        data)
{
  CtkWidget *widget;
  CtkProgressBar *pbar;
  CtkProgressBarPrivate *priv;
  PangoLayout *layout;
  PangoRectangle logical_rect;

  widget = ctk_css_gadget_get_owner (gadget);
  pbar = CTK_PROGRESS_BAR (widget);
  priv = pbar->priv;

  layout = ctk_progress_bar_get_layout (pbar);

  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (priv->ellipsize)
        {
          PangoContext *context;
          PangoFontMetrics *metrics;
          gint char_width;

          /* The minimum size for ellipsized text is ~ 3 chars */
          context = pango_layout_get_context (layout);
          metrics = pango_context_get_metrics (context,
                                               pango_layout_get_font_description (layout),
                                               pango_context_get_language (context));

          char_width = pango_font_metrics_get_approximate_char_width (metrics);
          pango_font_metrics_unref (metrics);

          *minimum = PANGO_PIXELS (char_width) * 3;
        }
      else
        *minimum = logical_rect.width;

      *natural = MAX (*minimum, logical_rect.width);
    }
  else
    *minimum = *natural = logical_rect.height;

  g_object_unref (layout);
}

static gint
get_number (CtkCssStyle *style,
            guint        property)
{
  double d = _ctk_css_number_value_get (ctk_css_style_get_value (style, property), 100.0);

  if (d < 1)
    return ceil (d);
  else
    return floor (d);
}

static void
ctk_progress_bar_measure_trough (CtkCssGadget   *gadget,
                                 CtkOrientation  orientation,
                                 int             for_size,
                                 int            *minimum,
                                 int            *natural,
                                 int            *minimum_baseline,
                                 int            *natural_baseline,
                                 gpointer        data)
{
  CtkWidget *widget;
  CtkProgressBarPrivate *priv;
  CtkCssStyle *style;

  widget = ctk_css_gadget_get_owner (gadget);
  priv = CTK_PROGRESS_BAR (widget)->priv;

  style = ctk_css_gadget_get_style (gadget);
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      gdouble min_width;

      min_width = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_MIN_WIDTH), 100.0);

      if (min_width > 0.0)
        *minimum = 0;
      else if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_style_get (widget, "min-horizontal-bar-width", minimum, NULL);
      else
        ctk_widget_style_get (widget, "min-vertical-bar-width", minimum, NULL);
    }
  else
    {
      gdouble min_height;

      min_height = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_MIN_HEIGHT), 100.0);

      if (min_height > 0.0)
        *minimum = 0;
      else if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_style_get (widget, "min-horizontal-bar-height", minimum, NULL);
      else
        ctk_widget_style_get (widget, "min-vertical-bar-height", minimum, NULL);
    }

  *natural = *minimum;

  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
}

static void
ctk_progress_bar_measure_progress (CtkCssGadget   *gadget,
                                   CtkOrientation  orientation,
                                   int             for_size,
                                   int            *minimum,
                                   int            *natural,
                                   int            *minimum_baseline,
                                   int            *natural_baseline,
                                   gpointer        data)
{
  CtkWidget *widget;
  CtkProgressBar *pbar;
  CtkProgressBarPrivate *priv;
  CtkCssStyle *style;

  widget = ctk_css_gadget_get_owner (gadget);
  pbar = CTK_PROGRESS_BAR (widget);
  priv = pbar->priv;

  style = ctk_css_gadget_get_style (gadget);
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      gint min_width;

      min_width = get_number (style, CTK_CSS_PROPERTY_MIN_WIDTH);

      if (min_width != 0)
        *minimum = min_width;
      else if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        *minimum = 0;
      else
        ctk_widget_style_get (widget, "min-vertical-bar-width", minimum, NULL);
    }
  else
    {
      gint min_height;

      min_height = get_number (style, CTK_CSS_PROPERTY_MIN_HEIGHT);

      if (min_height != 0)
        *minimum = min_height;
      else if (priv->orientation == CTK_ORIENTATION_VERTICAL)
        *minimum = 0;
      else
        ctk_widget_style_get (widget, "min-horizontal-bar-height", minimum, NULL);
    }

  *natural = *minimum;

  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
}

static void
ctk_progress_bar_size_allocate (CtkWidget     *widget,
                                CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CTK_PROGRESS_BAR (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_progress_bar_allocate (CtkCssGadget        *gadget,
                           const CtkAllocation *allocation,
                           int                  baseline,
                           CtkAllocation       *out_clip,
                           gpointer             data)
{
  CtkWidget *widget;
  CtkProgressBarPrivate *priv;
  gint bar_width, bar_height;
  gint text_width, text_height, text_min, text_nat;
  CtkAllocation alloc;
  CtkAllocation text_clip;

  widget = ctk_css_gadget_get_owner (gadget);
  priv = CTK_PROGRESS_BAR (widget)->priv;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      ctk_css_gadget_get_preferred_size (priv->trough_gadget,
                                         CTK_ORIENTATION_VERTICAL,
                                         -1,
                                         &bar_height, NULL,
                                         NULL, NULL);
      bar_width = allocation->width;
    }
  else
    {
      ctk_css_gadget_get_preferred_size (priv->trough_gadget,
                                         CTK_ORIENTATION_HORIZONTAL,
                                         -1,
                                         &bar_width, NULL,
                                         NULL, NULL);
      bar_height = allocation->height;
    }

  alloc.x = allocation->x + allocation->width - bar_width;
  alloc.y = allocation->y + allocation->height - bar_height;
  alloc.width = bar_width;
  alloc.height = bar_height;

  ctk_css_gadget_allocate (priv->trough_gadget, &alloc, -1, out_clip);

  if (!priv->show_text)
    return;

  ctk_css_gadget_get_preferred_size (priv->text_gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     &text_min, &text_nat,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (priv->text_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &text_height, NULL,
                                     NULL, NULL);

  text_width = CLAMP (text_nat, text_min, allocation->width);

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      alloc.x = allocation->x + (allocation->width - text_width) / 2;
      alloc.y = allocation->y;
      alloc.width = text_width;
      alloc.height = text_height;
    }
  else
    {
      alloc.x = allocation->x + allocation->width - text_width;
      alloc.y = allocation->y + (allocation->height - text_height) / 2;
      alloc.width = text_width;
      alloc.height = text_height;
    }

  ctk_css_gadget_allocate (priv->text_gadget, &alloc, -1, &text_clip);

  cdk_rectangle_union (out_clip, &text_clip, out_clip);
}


static void
ctk_progress_bar_allocate_trough (CtkCssGadget        *gadget,
                                  const CtkAllocation *allocation,
                                  int                  baseline,
                                  CtkAllocation       *out_clip,
                                  gpointer             data)
{
  CtkWidget *widget;
  CtkProgressBarPrivate *priv;
  CtkAllocation alloc;
  gint width, height;
  gboolean inverted;

  widget = ctk_css_gadget_get_owner (gadget);
  priv = CTK_PROGRESS_BAR (widget)->priv;

  inverted = priv->inverted;
  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        inverted = !inverted;
    }

  ctk_css_gadget_get_preferred_size (priv->progress_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &height, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (priv->progress_gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     &width, NULL,
                                     NULL, NULL);

  if (priv->activity_mode)
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          alloc.width = width + (allocation->width - width) / priv->activity_blocks;
          alloc.x = allocation->x + priv->activity_pos * (allocation->width - alloc.width);
          alloc.y = allocation->y + (allocation->height - height) / 2;
          alloc.height = height;
        }
      else
        {

          alloc.height = height + (allocation->height - height) / priv->activity_blocks;
          alloc.y = allocation->y + priv->activity_pos * (allocation->height - alloc.height);
          alloc.x = allocation->x + (allocation->width - width) / 2;
          alloc.width = width;
        }
    }
  else
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          alloc.width = width + (allocation->width - width) * priv->fraction;
          alloc.height = height;
          alloc.y = allocation->y + (allocation->height - height) / 2;

          if (!inverted)
            alloc.x = allocation->x;
          else
            alloc.x = allocation->x + allocation->width - alloc.width;
        }
      else
        {
          alloc.width = width;
          alloc.height = height + (allocation->height - height) * priv->fraction;
          alloc.x = allocation->x + (allocation->width - width) / 2;

          if (!inverted)
            alloc.y = allocation->y;
          else
            alloc.y = allocation->y + allocation->height - alloc.height;
        }
    }

  ctk_css_gadget_allocate (priv->progress_gadget, &alloc, -1, out_clip);
}

static void
ctk_progress_bar_get_preferred_width (CtkWidget *widget,
                                      gint      *minimum,
                                      gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_PROGRESS_BAR (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_progress_bar_get_preferred_height (CtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_PROGRESS_BAR (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static gboolean
tick_cb (CtkWidget     *widget,
         CdkFrameClock *frame_clock,
         gpointer       user_data)
{
  CtkProgressBar *pbar = CTK_PROGRESS_BAR (widget);
  CtkProgressBarPrivate *priv = pbar->priv;
  gint64 frame_time;
  gdouble iteration, pulse_iterations, current_iterations, fraction;

  if (priv->pulse2 == 0 && priv->pulse1 == 0)
    return G_SOURCE_CONTINUE;

  frame_time = cdk_frame_clock_get_frame_time (frame_clock);
  ctk_progress_tracker_advance_frame (&priv->tracker, frame_time);

  g_assert (priv->pulse2 > priv->pulse1);

  pulse_iterations = (priv->pulse2 - priv->pulse1) / (gdouble) G_USEC_PER_SEC;
  current_iterations = (frame_time - priv->pulse1) / (gdouble) G_USEC_PER_SEC;

  iteration = ctk_progress_tracker_get_iteration (&priv->tracker);
  /* Determine the fraction to move the block from one frame
   * to the next when pulse_fraction is how far the block should
   * move between two calls to ctk_progress_bar_pulse().
   */
  fraction = priv->pulse_fraction * (iteration - priv->last_iteration) / MAX (pulse_iterations, current_iterations);
  priv->last_iteration = iteration;

  if (current_iterations > 3 * pulse_iterations)
    {
      priv->pulse1 = 0;
      return G_SOURCE_CONTINUE;
    }

  /* advance the block */
  if (priv->activity_dir == 0)
    {
      priv->activity_pos += fraction;
      if (priv->activity_pos > 1.0)
        {
          priv->activity_pos = 1.0;
          priv->activity_dir = 1;
        }
    }
  else
    {
      priv->activity_pos -= fraction;
      if (priv->activity_pos <= 0)
        {
          priv->activity_pos = 0;
          priv->activity_dir = 0;
        }
    }

  update_node_classes (pbar);

  ctk_widget_queue_allocate (widget);

  return G_SOURCE_CONTINUE;
}

static void
ctk_progress_bar_act_mode_enter (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv = pbar->priv;
  CtkWidget *widget = CTK_WIDGET (pbar);
  gboolean inverted;

  ctk_css_gadget_add_class (priv->progress_gadget, CTK_STYLE_CLASS_PULSE);

  inverted = priv->inverted;
  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        inverted = !inverted;
    }

  /* calculate start pos */
  if (!inverted)
    {
      priv->activity_pos = 0.0;
      priv->activity_dir = 0;
    }
  else
    {
      priv->activity_pos = 1.0;
      priv->activity_dir = 1;
    }

  update_node_classes (pbar);
  /* No fixed schedule for pulses, will adapt after calls to update_pulse. Just
   * start the tracker to repeat forever with iterations every second.*/
  ctk_progress_tracker_start (&priv->tracker, G_USEC_PER_SEC, 0, INFINITY);
  priv->tick_id = ctk_widget_add_tick_callback (widget, tick_cb, NULL, NULL);
  priv->pulse2 = 0;
  priv->pulse1 = 0;
  priv->last_iteration = 0;
}

static void
ctk_progress_bar_act_mode_leave (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv = pbar->priv;

  if (priv->tick_id)
    ctk_widget_remove_tick_callback (CTK_WIDGET (pbar), priv->tick_id);
  priv->tick_id = 0;

  ctk_css_gadget_remove_class (priv->progress_gadget, CTK_STYLE_CLASS_PULSE);
  update_node_classes (pbar);
}

static gboolean
ctk_progress_bar_render_text (CtkCssGadget *gadget,
                              cairo_t      *cr,
                              int           x,
                              int           y,
                              int           width,
                              int           height,
                              gpointer      data)
{
  CtkWidget *widget;
  CtkProgressBar *pbar;
  CtkProgressBarPrivate *priv;
  CtkStyleContext *context;
  PangoLayout *layout;

  widget = ctk_css_gadget_get_owner (gadget);
  pbar = CTK_PROGRESS_BAR (widget);
  priv = pbar->priv;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save_to_node (context, ctk_css_gadget_get_node (gadget));

  layout = ctk_progress_bar_get_layout (pbar);
  pango_layout_set_ellipsize (layout, priv->ellipsize);
  if (priv->ellipsize)
    pango_layout_set_width (layout, width * PANGO_SCALE);

  ctk_render_layout (context, cr, x, y, layout);

  g_object_unref (layout);

  ctk_style_context_restore (context);

  return FALSE;
}

static gboolean
ctk_progress_bar_render_trough (CtkCssGadget *gadget,
                                cairo_t      *cr,
                                int           x,
                                int           y,
                                int           width,
                                int           height,
                                gpointer      data)
{
  CtkWidget *widget;
  CtkProgressBarPrivate *priv;

  widget = ctk_css_gadget_get_owner (gadget);
  priv = CTK_PROGRESS_BAR (widget)->priv;

  ctk_css_gadget_draw (priv->progress_gadget, cr);

  return FALSE;
}

static gboolean
ctk_progress_bar_render (CtkCssGadget *gadget,
                         cairo_t      *cr,
                         int           x,
                         int           y,
                         int           width,
                         int           height,
                         gpointer      data)
{
  CtkWidget *widget;
  CtkProgressBarPrivate *priv;

  widget = ctk_css_gadget_get_owner (gadget);
  priv = CTK_PROGRESS_BAR (widget)->priv;

  ctk_css_gadget_draw (priv->trough_gadget, cr);
  if (priv->show_text)
    ctk_css_gadget_draw (priv->text_gadget, cr);

  return FALSE;
}

static gboolean
ctk_progress_bar_draw (CtkWidget *widget,
                       cairo_t   *cr)
{
  CtkProgressBar *pbar = CTK_PROGRESS_BAR (widget);
  CtkProgressBarPrivate *priv = pbar->priv;

  ctk_css_gadget_draw (priv->gadget, cr);

  return FALSE;
}

static void
ctk_progress_bar_set_activity_mode (CtkProgressBar *pbar,
                                    gboolean        activity_mode)
{
  CtkProgressBarPrivate *priv = pbar->priv;

  activity_mode = !!activity_mode;

  if (priv->activity_mode != activity_mode)
    {
      priv->activity_mode = activity_mode;

      if (priv->activity_mode)
        ctk_progress_bar_act_mode_enter (pbar);
      else
        ctk_progress_bar_act_mode_leave (pbar);

      ctk_widget_queue_resize (CTK_WIDGET (pbar));
    }
}

/**
 * ctk_progress_bar_set_fraction:
 * @pbar: a #CtkProgressBar
 * @fraction: fraction of the task that’s been completed
 *
 * Causes the progress bar to “fill in” the given fraction
 * of the bar. The fraction should be between 0.0 and 1.0,
 * inclusive.
 */
void
ctk_progress_bar_set_fraction (CtkProgressBar *pbar,
                               gdouble         fraction)
{
  CtkProgressBarPrivate* priv;

  g_return_if_fail (CTK_IS_PROGRESS_BAR (pbar));

  priv = pbar->priv;

  priv->fraction = CLAMP (fraction, 0.0, 1.0);
  ctk_progress_bar_set_activity_mode (pbar, FALSE);
  ctk_widget_queue_allocate (CTK_WIDGET (pbar));
  update_fraction_classes (pbar);

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_FRACTION]);
}

static void
ctk_progress_bar_update_pulse (CtkProgressBar *pbar)
{
  CtkProgressBarPrivate *priv = pbar->priv;
  gint64 pulse_time = g_get_monotonic_time ();

  if (priv->pulse2 == pulse_time)
    return;

  priv->pulse1 = priv->pulse2;
  priv->pulse2 = pulse_time;
}

/**
 * ctk_progress_bar_pulse:
 * @pbar: a #CtkProgressBar
 *
 * Indicates that some progress has been made, but you don’t know how much.
 * Causes the progress bar to enter “activity mode,” where a block
 * bounces back and forth. Each call to ctk_progress_bar_pulse()
 * causes the block to move by a little bit (the amount of movement
 * per pulse is determined by ctk_progress_bar_set_pulse_step()).
 */
void
ctk_progress_bar_pulse (CtkProgressBar *pbar)
{
  g_return_if_fail (CTK_IS_PROGRESS_BAR (pbar));

  ctk_progress_bar_set_activity_mode (pbar, TRUE);
  ctk_progress_bar_update_pulse (pbar);
}

/**
 * ctk_progress_bar_set_text:
 * @pbar: a #CtkProgressBar
 * @text: (allow-none): a UTF-8 string, or %NULL
 *
 * Causes the given @text to appear next to the progress bar.
 *
 * If @text is %NULL and #CtkProgressBar:show-text is %TRUE, the current
 * value of #CtkProgressBar:fraction will be displayed as a percentage.
 *
 * If @text is non-%NULL and #CtkProgressBar:show-text is %TRUE, the text
 * will be displayed. In this case, it will not display the progress
 * percentage. If @text is the empty string, the progress bar will still
 * be styled and sized suitably for containing text, as long as
 * #CtkProgressBar:show-text is %TRUE.
 */
void
ctk_progress_bar_set_text (CtkProgressBar *pbar,
                           const gchar    *text)
{
  CtkProgressBarPrivate *priv;

  g_return_if_fail (CTK_IS_PROGRESS_BAR (pbar));

  priv = pbar->priv;

  /* Don't notify again if nothing's changed. */
  if (g_strcmp0 (priv->text, text) == 0)
    return;

  g_free (priv->text);
  priv->text = g_strdup (text);

  ctk_widget_queue_resize (CTK_WIDGET (pbar));

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_TEXT]);
}

static void
ctk_progress_bar_text_style_changed (CtkCssNode        *node,
                                     CtkCssStyleChange *change,
                                     CtkProgressBar    *pbar)
{
  if (change == NULL ||
      ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_TEXT_ATTRS) ||
      ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_FONT))
    {
      ctk_widget_queue_resize (CTK_WIDGET (pbar));
    }
}

/**
 * ctk_progress_bar_set_show_text:
 * @pbar: a #CtkProgressBar
 * @show_text: whether to show text
 *
 * Sets whether the progress bar will show text next to the bar.
 * The shown text is either the value of the #CtkProgressBar:text
 * property or, if that is %NULL, the #CtkProgressBar:fraction value,
 * as a percentage.
 *
 * To make a progress bar that is styled and sized suitably for containing
 * text (even if the actual text is blank), set #CtkProgressBar:show-text to
 * %TRUE and #CtkProgressBar:text to the empty string (not %NULL).
 *
 * Since: 3.0
 */
void
ctk_progress_bar_set_show_text (CtkProgressBar *pbar,
                                gboolean        show_text)
{
  CtkProgressBarPrivate *priv;

  g_return_if_fail (CTK_IS_PROGRESS_BAR (pbar));

  priv = pbar->priv;

  show_text = !!show_text;

  if (priv->show_text == show_text)
    return;

  priv->show_text = show_text;

  if (show_text)
    {
      priv->text_gadget = ctk_css_custom_gadget_new ("text",
                                                     CTK_WIDGET (pbar),
                                                     priv->gadget,
                                                     priv->trough_gadget,
                                                     ctk_progress_bar_measure_text,
                                                     NULL,
                                                     ctk_progress_bar_render_text,
                                                     NULL,
                                                     NULL);
      g_signal_connect (ctk_css_gadget_get_node (priv->text_gadget), "style-changed",
                        G_CALLBACK (ctk_progress_bar_text_style_changed), pbar);

      update_node_state (pbar);
    }
  else
    {
      if (priv->text_gadget)
        ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->text_gadget), NULL);
      g_clear_object (&priv->text_gadget);
    }

  ctk_widget_queue_resize (CTK_WIDGET (pbar));

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_SHOW_TEXT]);
}

/**
 * ctk_progress_bar_get_show_text:
 * @pbar: a #CtkProgressBar
 *
 * Gets the value of the #CtkProgressBar:show-text property.
 * See ctk_progress_bar_set_show_text().
 *
 * Returns: %TRUE if text is shown in the progress bar
 *
 * Since: 3.0
 */
gboolean
ctk_progress_bar_get_show_text (CtkProgressBar *pbar)
{
  g_return_val_if_fail (CTK_IS_PROGRESS_BAR (pbar), FALSE);

  return pbar->priv->show_text;
}

/**
 * ctk_progress_bar_set_pulse_step:
 * @pbar: a #CtkProgressBar
 * @fraction: fraction between 0.0 and 1.0
 *
 * Sets the fraction of total progress bar length to move the
 * bouncing block for each call to ctk_progress_bar_pulse().
 */
void
ctk_progress_bar_set_pulse_step (CtkProgressBar *pbar,
                                 gdouble         fraction)
{
  CtkProgressBarPrivate *priv;

  g_return_if_fail (CTK_IS_PROGRESS_BAR (pbar));

  priv = pbar->priv;

  priv->pulse_fraction = fraction;

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_PULSE_STEP]);
}

static void
ctk_progress_bar_direction_changed (CtkWidget        *widget,
                                    CtkTextDirection  previous_dir)
{
  CtkProgressBar *pbar = CTK_PROGRESS_BAR (widget);

  update_node_classes (pbar);
  update_node_state (pbar);

  CTK_WIDGET_CLASS (ctk_progress_bar_parent_class)->direction_changed (widget, previous_dir);
}

static void
ctk_progress_bar_state_flags_changed (CtkWidget     *widget,
                                      CtkStateFlags  previous_state)
{
  CtkProgressBar *pbar = CTK_PROGRESS_BAR (widget);

  update_node_state (pbar);

  CTK_WIDGET_CLASS (ctk_progress_bar_parent_class)->state_flags_changed (widget, previous_state);
}

static void
ctk_progress_bar_set_orientation (CtkProgressBar *pbar,
                                  CtkOrientation  orientation)
{
  CtkProgressBarPrivate *priv = pbar->priv;

  if (priv->orientation == orientation)
    return;

  priv->orientation = orientation;

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (pbar));
  update_node_classes (pbar);
  ctk_widget_queue_resize (CTK_WIDGET (pbar));

  g_object_notify (G_OBJECT (pbar), "orientation");
}

/**
 * ctk_progress_bar_set_inverted:
 * @pbar: a #CtkProgressBar
 * @inverted: %TRUE to invert the progress bar
 *
 * Progress bars normally grow from top to bottom or left to right.
 * Inverted progress bars grow in the opposite direction.
 */
void
ctk_progress_bar_set_inverted (CtkProgressBar *pbar,
                               gboolean        inverted)
{
  CtkProgressBarPrivate *priv;

  g_return_if_fail (CTK_IS_PROGRESS_BAR (pbar));

  priv = pbar->priv;

  if (priv->inverted == inverted)
    return;

  priv->inverted = inverted;

  update_node_classes (pbar);
  ctk_widget_queue_resize (CTK_WIDGET (pbar));

  g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_INVERTED]);
}

/**
 * ctk_progress_bar_get_text:
 * @pbar: a #CtkProgressBar
 *
 * Retrieves the text that is displayed with the progress bar,
 * if any, otherwise %NULL. The return value is a reference
 * to the text, not a copy of it, so will become invalid
 * if you change the text in the progress bar.
 *
 * Returns: (nullable): text, or %NULL; this string is owned by the widget
 * and should not be modified or freed.
 */
const gchar*
ctk_progress_bar_get_text (CtkProgressBar *pbar)
{
  g_return_val_if_fail (CTK_IS_PROGRESS_BAR (pbar), NULL);

  return pbar->priv->text;
}

/**
 * ctk_progress_bar_get_fraction:
 * @pbar: a #CtkProgressBar
 *
 * Returns the current fraction of the task that’s been completed.
 *
 * Returns: a fraction from 0.0 to 1.0
 */
gdouble
ctk_progress_bar_get_fraction (CtkProgressBar *pbar)
{
  g_return_val_if_fail (CTK_IS_PROGRESS_BAR (pbar), 0);

  return pbar->priv->fraction;
}

/**
 * ctk_progress_bar_get_pulse_step:
 * @pbar: a #CtkProgressBar
 *
 * Retrieves the pulse step set with ctk_progress_bar_set_pulse_step().
 *
 * Returns: a fraction from 0.0 to 1.0
 */
gdouble
ctk_progress_bar_get_pulse_step (CtkProgressBar *pbar)
{
  g_return_val_if_fail (CTK_IS_PROGRESS_BAR (pbar), 0);

  return pbar->priv->pulse_fraction;
}

/**
 * ctk_progress_bar_get_inverted:
 * @pbar: a #CtkProgressBar
 *
 * Gets the value set by ctk_progress_bar_set_inverted().
 *
 * Returns: %TRUE if the progress bar is inverted
 */
gboolean
ctk_progress_bar_get_inverted (CtkProgressBar *pbar)
{
  g_return_val_if_fail (CTK_IS_PROGRESS_BAR (pbar), FALSE);

  return pbar->priv->inverted;
}

/**
 * ctk_progress_bar_set_ellipsize:
 * @pbar: a #CtkProgressBar
 * @mode: a #PangoEllipsizeMode
 *
 * Sets the mode used to ellipsize (add an ellipsis: "...") the
 * text if there is not enough space to render the entire string.
 *
 * Since: 2.6
 */
void
ctk_progress_bar_set_ellipsize (CtkProgressBar     *pbar,
                                PangoEllipsizeMode  mode)
{
  CtkProgressBarPrivate *priv;

  g_return_if_fail (CTK_IS_PROGRESS_BAR (pbar));
  g_return_if_fail (mode >= PANGO_ELLIPSIZE_NONE &&
                    mode <= PANGO_ELLIPSIZE_END);

  priv = pbar->priv;

  if ((PangoEllipsizeMode)priv->ellipsize != mode)
    {
      priv->ellipsize = mode;

      g_object_notify_by_pspec (G_OBJECT (pbar), progress_props[PROP_ELLIPSIZE]);
      ctk_widget_queue_resize (CTK_WIDGET (pbar));
    }
}

/**
 * ctk_progress_bar_get_ellipsize:
 * @pbar: a #CtkProgressBar
 *
 * Returns the ellipsizing position of the progress bar.
 * See ctk_progress_bar_set_ellipsize().
 *
 * Returns: #PangoEllipsizeMode
 *
 * Since: 2.6
 */
PangoEllipsizeMode
ctk_progress_bar_get_ellipsize (CtkProgressBar *pbar)
{
  g_return_val_if_fail (CTK_IS_PROGRESS_BAR (pbar), PANGO_ELLIPSIZE_NONE);

  return pbar->priv->ellipsize;
}
