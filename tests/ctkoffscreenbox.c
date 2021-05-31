/*
 * ctkoffscreenbox.c
 */

#include "config.h"

#include <math.h>
#include <ctk/ctk.h>

#include "ctkoffscreenbox.h"

static void        ctk_offscreen_box_realize       (CtkWidget       *widget);
static void        ctk_offscreen_box_unrealize     (CtkWidget       *widget);
static void        ctk_offscreen_box_get_preferred_width  (CtkWidget *widget,
                                                           gint      *minimum,
                                                           gint      *natural);
static void        ctk_offscreen_box_get_preferred_height (CtkWidget *widget,
                                                           gint      *minimum,
                                                           gint      *natural);
static void        ctk_offscreen_box_size_allocate (CtkWidget       *widget,
                                                    CtkAllocation   *allocation);
static gboolean    ctk_offscreen_box_damage        (CtkWidget       *widget,
                                                    GdkEventExpose  *event);
static gboolean    ctk_offscreen_box_draw          (CtkWidget       *widget,
                                                    cairo_t         *cr);

static void        ctk_offscreen_box_add           (CtkContainer    *container,
                                                    CtkWidget       *child);
static void        ctk_offscreen_box_remove        (CtkContainer    *container,
                                                    CtkWidget       *widget);
static void        ctk_offscreen_box_forall        (CtkContainer    *container,
                                                    gboolean         include_internals,
                                                    CtkCallback      callback,
                                                    gpointer         callback_data);
static GType       ctk_offscreen_box_child_type    (CtkContainer    *container);

#define CHILD1_SIZE_SCALE 1.0
#define CHILD2_SIZE_SCALE 1.0

G_DEFINE_TYPE (CtkOffscreenBox, ctk_offscreen_box, CTK_TYPE_CONTAINER);

static void
to_child_2 (CtkOffscreenBox *offscreen_box,
	    double widget_x, double widget_y,
	    double *x_out, double *y_out)
{
  CtkAllocation child_area;
  double x, y, xr, yr;
  double cos_angle, sin_angle;

  x = widget_x;
  y = widget_y;

  if (offscreen_box->child1 && ctk_widget_get_visible (offscreen_box->child1))
    {
      ctk_widget_get_allocation (offscreen_box->child1, &child_area);
      y -= child_area.height;
    }

  ctk_widget_get_allocation (offscreen_box->child2, &child_area);

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  cos_angle = cos (-offscreen_box->angle);
  sin_angle = sin (-offscreen_box->angle);

  xr = x * cos_angle - y * sin_angle;
  yr = x * sin_angle + y * cos_angle;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  *x_out = x;
  *y_out = y;
}

static void
to_parent_2 (CtkOffscreenBox *offscreen_box,
	     double offscreen_x, double offscreen_y,
	     double *x_out, double *y_out)
{
  CtkAllocation child_area;
  double x, y, xr, yr;
  double cos_angle, sin_angle;

  ctk_widget_get_allocation (offscreen_box->child2, &child_area);

  x = offscreen_x;
  y = offscreen_y;

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  cos_angle = cos (offscreen_box->angle);
  sin_angle = sin (offscreen_box->angle);

  xr = x * cos_angle - y * sin_angle;
  yr = x * sin_angle + y * cos_angle;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  if (offscreen_box->child1 && ctk_widget_get_visible (offscreen_box->child1))
    {
      ctk_widget_get_allocation (offscreen_box->child1, &child_area);
      y += child_area.height;
    }

  *x_out = x;
  *y_out = y;
}

static void
ctk_offscreen_box_class_init (CtkOffscreenBoxClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  widget_class->realize = ctk_offscreen_box_realize;
  widget_class->unrealize = ctk_offscreen_box_unrealize;
  widget_class->get_preferred_width = ctk_offscreen_box_get_preferred_width;
  widget_class->get_preferred_height = ctk_offscreen_box_get_preferred_height;
  widget_class->size_allocate = ctk_offscreen_box_size_allocate;
  widget_class->draw = ctk_offscreen_box_draw;

  g_signal_override_class_closure (g_signal_lookup ("damage-event", CTK_TYPE_WIDGET),
                                   CTK_TYPE_OFFSCREEN_BOX,
                                   g_cclosure_new (G_CALLBACK (ctk_offscreen_box_damage),
                                                   NULL, NULL));

  container_class->add = ctk_offscreen_box_add;
  container_class->remove = ctk_offscreen_box_remove;
  container_class->forall = ctk_offscreen_box_forall;
  container_class->child_type = ctk_offscreen_box_child_type;
}

static void
ctk_offscreen_box_init (CtkOffscreenBox *offscreen_box)
{
  ctk_widget_set_has_window (CTK_WIDGET (offscreen_box), TRUE);
}

CtkWidget *
ctk_offscreen_box_new (void)
{
  return g_object_new (CTK_TYPE_OFFSCREEN_BOX, NULL);
}

static GdkWindow *
pick_offscreen_child (GdkWindow *offscreen_window,
		      double widget_x, double widget_y,
		      CtkOffscreenBox *offscreen_box)
{
 CtkAllocation child_area;
 double x, y;

 /* First check for child 2 */
 if (offscreen_box->child2 &&
     ctk_widget_get_visible (offscreen_box->child2))
    {
      to_child_2 (offscreen_box,
		  widget_x, widget_y,
		  &x, &y);

      ctk_widget_get_allocation (offscreen_box->child2, &child_area);

      if (x >= 0 && x < child_area.width &&
	  y >= 0 && y < child_area.height)
	return offscreen_box->offscreen_window2;
    }

 if (offscreen_box->child1 && ctk_widget_get_visible (offscreen_box->child1))
   {
     x = widget_x;
     y = widget_y;

     ctk_widget_get_allocation (offscreen_box->child1, &child_area);

     if (x >= 0 && x < child_area.width &&
	 y >= 0 && y < child_area.height)
       return offscreen_box->offscreen_window1;
   }

  return NULL;
}

static void
offscreen_window_to_parent1 (GdkWindow       *offscreen_window,
			     double           offscreen_x,
			     double           offscreen_y,
			     double          *parent_x,
			     double          *parent_y,
			     CtkOffscreenBox *offscreen_box)
{
  *parent_x = offscreen_x;
  *parent_y = offscreen_y;
}

static void
offscreen_window_from_parent1 (GdkWindow       *window,
			       double           parent_x,
			       double           parent_y,
			       double          *offscreen_x,
			       double          *offscreen_y,
			       CtkOffscreenBox *offscreen_box)
{
  *offscreen_x = parent_x;
  *offscreen_y = parent_y;
}

static void
offscreen_window_to_parent2 (GdkWindow       *offscreen_window,
			     double           offscreen_x,
			     double           offscreen_y,
			     double          *parent_x,
			     double          *parent_y,
			     CtkOffscreenBox *offscreen_box)
{
  to_parent_2 (offscreen_box,
	      offscreen_x, offscreen_y,
	      parent_x, parent_y);
}

static void
offscreen_window_from_parent2 (GdkWindow       *window,
			       double           parent_x,
			       double           parent_y,
			       double          *offscreen_x,
			       double          *offscreen_y,
			       CtkOffscreenBox *offscreen_box)
{
  to_child_2 (offscreen_box,
	      parent_x, parent_y,
	      offscreen_x, offscreen_y);
}

static cairo_surface_t *
gdk_offscreen_box_create_alpha_image_surface (GdkWindow *offscreen,
                                              gint       width,
                                              gint       height)
{
  return cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
}

static void
ctk_offscreen_box_realize (CtkWidget *widget)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (widget);
  CtkAllocation allocation, child_area;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  guint border_width;
  CtkRequisition child_requisition;
  int start_y = 0;

  ctk_widget_set_realized (widget, TRUE);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x + border_width;
  attributes.y = allocation.y + border_width;
  attributes.width = allocation.width - 2 * border_width;
  attributes.height = allocation.height - 2 * border_width;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = ctk_widget_get_events (widget)
			| GDK_EXPOSURE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_SCROLL_MASK
			| GDK_ENTER_NOTIFY_MASK
			| GDK_LEAVE_NOTIFY_MASK;

  attributes.visual = ctk_widget_get_visual (widget);
  attributes.wclass = GDK_INPUT_OUTPUT;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = gdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  gdk_window_set_user_data (window, widget);

  g_signal_connect (window, "pick-embedded-child",
		    G_CALLBACK (pick_offscreen_child), offscreen_box);

  attributes.window_type = GDK_WINDOW_OFFSCREEN;

  /* Child 1 */
  attributes.x = attributes.y = 0;
  if (offscreen_box->child1 && ctk_widget_get_visible (offscreen_box->child1))
    {
      ctk_widget_get_allocation (offscreen_box->child1, &child_area);

      attributes.width = child_area.width;
      attributes.height = child_area.height;
      start_y += child_area.height;
    }
  offscreen_box->offscreen_window1 = gdk_window_new (gdk_screen_get_root_window (ctk_widget_get_screen (widget)),
						     &attributes, attributes_mask);
  gdk_window_set_user_data (offscreen_box->offscreen_window1, widget);
  if (offscreen_box->child1)
    ctk_widget_set_parent_window (offscreen_box->child1, offscreen_box->offscreen_window1);

  gdk_offscreen_window_set_embedder (offscreen_box->offscreen_window1,
				     window);

  g_signal_connect (offscreen_box->offscreen_window1, "to-embedder",
		    G_CALLBACK (offscreen_window_to_parent1), offscreen_box);
  g_signal_connect (offscreen_box->offscreen_window1, "from-embedder",
		    G_CALLBACK (offscreen_window_from_parent1), offscreen_box);

  /* Child 2 */
  attributes.y = start_y;
  child_requisition.width = child_requisition.height = 0;
  if (offscreen_box->child2 && ctk_widget_get_visible (offscreen_box->child2))
    {
      ctk_widget_get_allocation (offscreen_box->child2, &child_area);

      attributes.width = child_area.width;
      attributes.height = child_area.height;
    }
  offscreen_box->offscreen_window2 = gdk_window_new (gdk_screen_get_root_window (ctk_widget_get_screen (widget)),
						     &attributes, attributes_mask);
  gdk_window_set_user_data (offscreen_box->offscreen_window2, widget);
  if (offscreen_box->child2)
    ctk_widget_set_parent_window (offscreen_box->child2, offscreen_box->offscreen_window2);
  gdk_offscreen_window_set_embedder (offscreen_box->offscreen_window2,
				     window);

  g_signal_connect (offscreen_box->offscreen_window2, "create-surface",
                    G_CALLBACK (gdk_offscreen_box_create_alpha_image_surface),
                    offscreen_box);
  g_signal_connect (offscreen_box->offscreen_window2, "to-embedder",
		    G_CALLBACK (offscreen_window_to_parent2), offscreen_box);
  g_signal_connect (offscreen_box->offscreen_window2, "from-embedder",
		    G_CALLBACK (offscreen_window_from_parent2), offscreen_box);

  gdk_window_show (offscreen_box->offscreen_window1);
  gdk_window_show (offscreen_box->offscreen_window2);
}

static void
ctk_offscreen_box_unrealize (CtkWidget *widget)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (widget);

  gdk_window_set_user_data (offscreen_box->offscreen_window1, NULL);
  gdk_window_destroy (offscreen_box->offscreen_window1);
  offscreen_box->offscreen_window1 = NULL;

  gdk_window_set_user_data (offscreen_box->offscreen_window2, NULL);
  gdk_window_destroy (offscreen_box->offscreen_window2);
  offscreen_box->offscreen_window2 = NULL;

  CTK_WIDGET_CLASS (ctk_offscreen_box_parent_class)->unrealize (widget);
}

static GType
ctk_offscreen_box_child_type (CtkContainer *container)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (container);

  if (offscreen_box->child1 && offscreen_box->child2)
    return G_TYPE_NONE;

  return CTK_TYPE_WIDGET;
}

static void
ctk_offscreen_box_add (CtkContainer *container,
		       CtkWidget    *widget)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (container);

  if (!offscreen_box->child1)
    ctk_offscreen_box_add1 (offscreen_box, widget);
  else if (!offscreen_box->child2)
    ctk_offscreen_box_add2 (offscreen_box, widget);
  else
    g_warning ("CtkOffscreenBox cannot have more than 2 children");
}

void
ctk_offscreen_box_add1 (CtkOffscreenBox *offscreen_box,
			CtkWidget       *child)
{
  g_return_if_fail (CTK_IS_OFFSCREEN_BOX (offscreen_box));
  g_return_if_fail (CTK_IS_WIDGET (child));

  if (offscreen_box->child1 == NULL)
    {
      ctk_widget_set_parent_window (child, offscreen_box->offscreen_window1);
      ctk_widget_set_parent (child, CTK_WIDGET (offscreen_box));
      offscreen_box->child1 = child;
    }
}

void
ctk_offscreen_box_add2 (CtkOffscreenBox  *offscreen_box,
			CtkWidget    *child)
{
  g_return_if_fail (CTK_IS_OFFSCREEN_BOX (offscreen_box));
  g_return_if_fail (CTK_IS_WIDGET (child));

  if (offscreen_box->child2 == NULL)
    {
      ctk_widget_set_parent_window (child, offscreen_box->offscreen_window2);
      ctk_widget_set_parent (child, CTK_WIDGET (offscreen_box));
      offscreen_box->child2 = child;
    }
}

static void
ctk_offscreen_box_remove (CtkContainer *container,
			  CtkWidget    *widget)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (container);
  gboolean was_visible;

  was_visible = ctk_widget_get_visible (widget);

  if (offscreen_box->child1 == widget)
    {
      ctk_widget_unparent (widget);

      offscreen_box->child1 = NULL;

      if (was_visible && ctk_widget_get_visible (CTK_WIDGET (container)))
	ctk_widget_queue_resize (CTK_WIDGET (container));
    }
  else if (offscreen_box->child2 == widget)
    {
      ctk_widget_unparent (widget);

      offscreen_box->child2 = NULL;

      if (was_visible && ctk_widget_get_visible (CTK_WIDGET (container)))
	ctk_widget_queue_resize (CTK_WIDGET (container));
    }
}

static void
ctk_offscreen_box_forall (CtkContainer *container,
			  gboolean      include_internals,
			  CtkCallback   callback,
			  gpointer      callback_data)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (container);

  g_return_if_fail (callback != NULL);

  if (offscreen_box->child1)
    (*callback) (offscreen_box->child1, callback_data);
  if (offscreen_box->child2)
    (*callback) (offscreen_box->child2, callback_data);
}

void
ctk_offscreen_box_set_angle (CtkOffscreenBox  *offscreen_box,
			     gdouble           angle)
{
  g_return_if_fail (CTK_IS_OFFSCREEN_BOX (offscreen_box));

  offscreen_box->angle = angle;
  ctk_widget_queue_draw (CTK_WIDGET (offscreen_box));

  /* TODO: Really needs to resent pointer events if over the rotated window */
}


static void
ctk_offscreen_box_size_request (CtkWidget      *widget,
				CtkRequisition *requisition)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (widget);
  int w, h;
  guint border_width;

  w = 0;
  h = 0;

  if (offscreen_box->child1 && ctk_widget_get_visible (offscreen_box->child1))
    {
      CtkRequisition child_requisition;

      ctk_widget_get_preferred_size ( (offscreen_box->child1),
                                 &child_requisition, NULL);

      w = MAX (w, CHILD1_SIZE_SCALE * child_requisition.width);
      h += CHILD1_SIZE_SCALE * child_requisition.height;
    }

  if (offscreen_box->child2 && ctk_widget_get_visible (offscreen_box->child2))
    {
      CtkRequisition child_requisition;

      ctk_widget_get_preferred_size ( (offscreen_box->child2),
                                 &child_requisition, NULL);

      w = MAX (w, CHILD2_SIZE_SCALE * child_requisition.width);
      h += CHILD2_SIZE_SCALE * child_requisition.height;
    }

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));
  requisition->width = border_width * 2 + w;
  requisition->height = border_width * 2 + h;
}

static void
ctk_offscreen_box_get_preferred_width (CtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  CtkRequisition requisition;

  ctk_offscreen_box_size_request (widget, &requisition);

  *minimum = *natural = requisition.width;
}

static void
ctk_offscreen_box_get_preferred_height (CtkWidget *widget,
                                        gint      *minimum,
                                        gint      *natural)
{
  CtkRequisition requisition;

  ctk_offscreen_box_size_request (widget, &requisition);

  *minimum = *natural = requisition.height;
}

static void
ctk_offscreen_box_size_allocate (CtkWidget     *widget,
				 CtkAllocation *allocation)
{
  CtkOffscreenBox *offscreen_box;
  gint start_y;
  guint border_width;

  offscreen_box = CTK_OFFSCREEN_BOX (widget);

  ctk_widget_set_allocation (widget, allocation);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  if (ctk_widget_get_realized (widget))
    gdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x + border_width,
                            allocation->y + border_width,
                            allocation->width - border_width * 2,
                            allocation->height - border_width * 2);

  start_y = 0;

  if (offscreen_box->child1 && ctk_widget_get_visible (offscreen_box->child1))
    {
      CtkRequisition child_requisition;
      CtkAllocation child_allocation;

      ctk_widget_get_preferred_size (offscreen_box->child1,
                                     &child_requisition, NULL);
      child_allocation.x = child_requisition.width * (CHILD1_SIZE_SCALE - 1.0) / 2;
      child_allocation.y = start_y + child_requisition.height * (CHILD1_SIZE_SCALE - 1.0) / 2;
      child_allocation.width = MAX (1, (gint) allocation->width - 2 * border_width);
      child_allocation.height = child_requisition.height;

      start_y += CHILD1_SIZE_SCALE * child_requisition.height;

      if (ctk_widget_get_realized (widget))
	gdk_window_move_resize (offscreen_box->offscreen_window1,
				child_allocation.x,
                                child_allocation.y,
				child_allocation.width,
                                child_allocation.height);

      child_allocation.x = child_allocation.y = 0;
      ctk_widget_size_allocate (offscreen_box->child1, &child_allocation);
    }

  if (offscreen_box->child2 && ctk_widget_get_visible (offscreen_box->child2))
    {
      CtkRequisition child_requisition;
      CtkAllocation child_allocation;

      ctk_widget_get_preferred_size (offscreen_box->child2,
                                     &child_requisition, NULL);
      child_allocation.x = child_requisition.width * (CHILD2_SIZE_SCALE - 1.0) / 2;
      child_allocation.y = start_y + child_requisition.height * (CHILD2_SIZE_SCALE - 1.0) / 2;
      child_allocation.width = MAX (1, (gint) allocation->width - 2 * border_width);
      child_allocation.height = child_requisition.height;

      start_y += CHILD2_SIZE_SCALE * child_requisition.height;

      if (ctk_widget_get_realized (widget))
	gdk_window_move_resize (offscreen_box->offscreen_window2,
				child_allocation.x,
                                child_allocation.y,
				child_allocation.width,
                                child_allocation.height);

      child_allocation.x = child_allocation.y = 0;
      ctk_widget_size_allocate (offscreen_box->child2, &child_allocation);
    }
}

static gboolean
ctk_offscreen_box_damage (CtkWidget      *widget,
                          GdkEventExpose *event)
{
  gdk_window_invalidate_rect (ctk_widget_get_window (widget),
                              NULL, FALSE);

  return TRUE;
}

static gboolean
ctk_offscreen_box_draw (CtkWidget *widget,
                        cairo_t   *cr)
{
  CtkOffscreenBox *offscreen_box = CTK_OFFSCREEN_BOX (widget);
  GdkWindow *window;

  window = ctk_widget_get_window (widget);
  if (ctk_cairo_should_draw_window (cr, window))
    {
      cairo_surface_t *surface;
      CtkAllocation child_area;

      if (offscreen_box->child1 && ctk_widget_get_visible (offscreen_box->child1))
        {
          surface = gdk_offscreen_window_get_surface (offscreen_box->offscreen_window1);

          cairo_set_source_surface (cr, surface, 0, 0);
          cairo_paint (cr);

          ctk_widget_get_allocation (offscreen_box->child1, &child_area);
          cairo_translate (cr, 0, child_area.height);
        }

      if (offscreen_box->child2 && ctk_widget_get_visible (offscreen_box->child2))
        {
          surface = gdk_offscreen_window_get_surface (offscreen_box->offscreen_window2);

          ctk_widget_get_allocation (offscreen_box->child2, &child_area);

          /* transform */
          cairo_translate (cr, child_area.width / 2, child_area.height / 2);
          cairo_rotate (cr, offscreen_box->angle);
          cairo_translate (cr, -child_area.width / 2, -child_area.height / 2);

          /* paint */
          cairo_set_source_surface (cr, surface, 0, 0);
          cairo_paint (cr);
        }
    }
  else if (ctk_cairo_should_draw_window (cr, offscreen_box->offscreen_window1))
    {
      ctk_render_background (ctk_widget_get_style_context (widget), cr,
                             0, 0,

                             gdk_window_get_width (offscreen_box->offscreen_window1),
                             gdk_window_get_height (offscreen_box->offscreen_window1));

      if (offscreen_box->child1)
        ctk_container_propagate_draw (CTK_CONTAINER (widget),
                                      offscreen_box->child1,
                                      cr);
    }
  else if (ctk_cairo_should_draw_window (cr, offscreen_box->offscreen_window2))
    {
      ctk_render_background (ctk_widget_get_style_context (widget), cr,
                             0, 0,
                             gdk_window_get_width (offscreen_box->offscreen_window2),
                             gdk_window_get_height (offscreen_box->offscreen_window2));

      if (offscreen_box->child2)
        ctk_container_propagate_draw (CTK_CONTAINER (widget),
                                      offscreen_box->child2,
                                      cr);
    }

  return FALSE;
}
