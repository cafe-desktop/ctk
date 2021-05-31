/* Offscreen Windows/Rotated Button
 *
 * Offscreen windows can be used to transform parts of a widget
 * hierarchy. Note that the rotated button is fully functional.
 */
#include <math.h>
#include <gtk/gtk.h>

#define CTK_TYPE_ROTATED_BIN              (ctk_rotated_bin_get_type ())
#define CTK_ROTATED_BIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ROTATED_BIN, GtkRotatedBin))
#define CTK_ROTATED_BIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ROTATED_BIN, GtkRotatedBinClass))
#define CTK_IS_ROTATED_BIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ROTATED_BIN))
#define CTK_IS_ROTATED_BIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ROTATED_BIN))
#define CTK_ROTATED_BIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ROTATED_BIN, GtkRotatedBinClass))

typedef struct _GtkRotatedBin   GtkRotatedBin;
typedef struct _GtkRotatedBinClass  GtkRotatedBinClass;

struct _GtkRotatedBin
{
  GtkContainer container;

  GtkWidget *child;
  GdkWindow *offscreen_window;
  gdouble angle;
};

struct _GtkRotatedBinClass
{
  GtkContainerClass parent_class;
};

GType      ctk_rotated_bin_get_type  (void) G_GNUC_CONST;
GtkWidget* ctk_rotated_bin_new       (void);
void       ctk_rotated_bin_set_angle (GtkRotatedBin *bin,
                                      gdouble        angle);

/*** implementation ***/

static void     ctk_rotated_bin_realize       (GtkWidget       *widget);
static void     ctk_rotated_bin_unrealize     (GtkWidget       *widget);
static void     ctk_rotated_bin_get_preferred_width  (GtkWidget *widget,
                                                      gint      *minimum,
                                                      gint      *natural);
static void     ctk_rotated_bin_get_preferred_height (GtkWidget *widget,
                                                      gint      *minimum,
                                                      gint      *natural);
static void     ctk_rotated_bin_size_allocate (GtkWidget       *widget,
                                               GtkAllocation   *allocation);
static gboolean ctk_rotated_bin_damage        (GtkWidget       *widget,
                                               GdkEventExpose  *event);
static gboolean ctk_rotated_bin_draw          (GtkWidget       *widget,
                                               cairo_t         *cr);

static void     ctk_rotated_bin_add           (GtkContainer    *container,
                                               GtkWidget       *child);
static void     ctk_rotated_bin_remove        (GtkContainer    *container,
                                               GtkWidget       *widget);
static void     ctk_rotated_bin_forall        (GtkContainer    *container,
                                               gboolean         include_internals,
                                               GtkCallback      callback,
                                               gpointer         callback_data);
static GType    ctk_rotated_bin_child_type    (GtkContainer    *container);

G_DEFINE_TYPE (GtkRotatedBin, ctk_rotated_bin, CTK_TYPE_CONTAINER);

static void
to_child (GtkRotatedBin *bin,
          double         widget_x,
          double         widget_y,
          double        *x_out,
          double        *y_out)
{
  GtkAllocation child_area;
  double x, y, xr, yr;
  double c, s;
  double w, h;

  s = sin (bin->angle);
  c = cos (bin->angle);
  ctk_widget_get_allocation (bin->child, &child_area);

  w = c * child_area.width + s * child_area.height;
  h = s * child_area.width + c * child_area.height;

  x = widget_x;
  y = widget_y;

  x -= (w - child_area.width) / 2;
  y -= (h - child_area.height) / 2;

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  xr = x * c + y * s;
  yr = y * c - x * s;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  *x_out = x;
  *y_out = y;
}

static void
to_parent (GtkRotatedBin *bin,
           double         offscreen_x,
           double         offscreen_y,
           double        *x_out,
           double        *y_out)
{
  GtkAllocation child_area;
  double x, y, xr, yr;
  double c, s;
  double w, h;

  s = sin (bin->angle);
  c = cos (bin->angle);
  ctk_widget_get_allocation (bin->child, &child_area);

  w = c * child_area.width + s * child_area.height;
  h = s * child_area.width + c * child_area.height;

  x = offscreen_x;
  y = offscreen_y;

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  xr = x * c - y * s;
  yr = x * s + y * c;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  x -= (w - child_area.width) / 2;
  y -= (h - child_area.height) / 2;

  *x_out = x;
  *y_out = y;
}

static void
ctk_rotated_bin_class_init (GtkRotatedBinClass *klass)
{
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  widget_class->realize = ctk_rotated_bin_realize;
  widget_class->unrealize = ctk_rotated_bin_unrealize;
  widget_class->get_preferred_width = ctk_rotated_bin_get_preferred_width;
  widget_class->get_preferred_height = ctk_rotated_bin_get_preferred_height;
  widget_class->size_allocate = ctk_rotated_bin_size_allocate;
  widget_class->draw = ctk_rotated_bin_draw;

  g_signal_override_class_closure (g_signal_lookup ("damage-event", CTK_TYPE_WIDGET),
                                   CTK_TYPE_ROTATED_BIN,
                                   g_cclosure_new (G_CALLBACK (ctk_rotated_bin_damage),
                                                   NULL, NULL));

  container_class->add = ctk_rotated_bin_add;
  container_class->remove = ctk_rotated_bin_remove;
  container_class->forall = ctk_rotated_bin_forall;
  container_class->child_type = ctk_rotated_bin_child_type;
}

static void
ctk_rotated_bin_init (GtkRotatedBin *bin)
{
  ctk_widget_set_has_window (CTK_WIDGET (bin), TRUE);
}

GtkWidget *
ctk_rotated_bin_new (void)
{
  return g_object_new (CTK_TYPE_ROTATED_BIN, NULL);
}

static GdkWindow *
pick_offscreen_child (GdkWindow     *offscreen_window,
                      double         widget_x,
                      double         widget_y,
                      GtkRotatedBin *bin)
{
 GtkAllocation child_area;
 double x, y;

 if (bin->child && ctk_widget_get_visible (bin->child))
    {
      to_child (bin, widget_x, widget_y, &x, &y);

      ctk_widget_get_allocation (bin->child, &child_area);

      if (x >= 0 && x < child_area.width &&
          y >= 0 && y < child_area.height)
        return bin->offscreen_window;
    }

  return NULL;
}

static void
offscreen_window_to_parent (GdkWindow     *offscreen_window,
                            double         offscreen_x,
                            double         offscreen_y,
                            double        *parent_x,
                            double        *parent_y,
                            GtkRotatedBin *bin)
{
  to_parent (bin, offscreen_x, offscreen_y, parent_x, parent_y);
}

static void
offscreen_window_from_parent (GdkWindow     *window,
                              double         parent_x,
                              double         parent_y,
                              double        *offscreen_x,
                              double        *offscreen_y,
                              GtkRotatedBin *bin)
{
  to_child (bin, parent_x, parent_y, offscreen_x, offscreen_y);
}

static void
ctk_rotated_bin_realize (GtkWidget *widget)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (widget);
  GtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  guint border_width;
  GtkRequisition child_requisition;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);
  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

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
                    G_CALLBACK (pick_offscreen_child), bin);

  attributes.window_type = GDK_WINDOW_OFFSCREEN;

  child_requisition.width = child_requisition.height = 0;
  if (bin->child && ctk_widget_get_visible (bin->child))
    {
      GtkAllocation child_allocation;

      ctk_widget_get_allocation (bin->child, &child_allocation);
      attributes.width = child_allocation.width;
      attributes.height = child_allocation.height;
    }
  bin->offscreen_window = gdk_window_new (gdk_screen_get_root_window (ctk_widget_get_screen (widget)),
                                          &attributes, attributes_mask);
  gdk_window_set_user_data (bin->offscreen_window, widget);
  if (bin->child)
    ctk_widget_set_parent_window (bin->child, bin->offscreen_window);
  gdk_offscreen_window_set_embedder (bin->offscreen_window, window);
  g_signal_connect (bin->offscreen_window, "to-embedder",
                    G_CALLBACK (offscreen_window_to_parent), bin);
  g_signal_connect (bin->offscreen_window, "from-embedder",
                    G_CALLBACK (offscreen_window_from_parent), bin);

  gdk_window_show (bin->offscreen_window);
}

static void
ctk_rotated_bin_unrealize (GtkWidget *widget)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (widget);

  gdk_window_set_user_data (bin->offscreen_window, NULL);
  gdk_window_destroy (bin->offscreen_window);
  bin->offscreen_window = NULL;

  CTK_WIDGET_CLASS (ctk_rotated_bin_parent_class)->unrealize (widget);
}

static GType
ctk_rotated_bin_child_type (GtkContainer *container)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (container);

  if (bin->child)
    return G_TYPE_NONE;

  return CTK_TYPE_WIDGET;
}

static void
ctk_rotated_bin_add (GtkContainer *container,
                     GtkWidget    *widget)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (container);

  if (!bin->child)
    {
      ctk_widget_set_parent_window (widget, bin->offscreen_window);
      ctk_widget_set_parent (widget, CTK_WIDGET (bin));
      bin->child = widget;
    }
  else
    g_warning ("GtkRotatedBin cannot have more than one child");
}

static void
ctk_rotated_bin_remove (GtkContainer *container,
                        GtkWidget    *widget)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (container);
  gboolean was_visible;

  was_visible = ctk_widget_get_visible (widget);

  if (bin->child == widget)
    {
      ctk_widget_unparent (widget);

      bin->child = NULL;

      if (was_visible && ctk_widget_get_visible (CTK_WIDGET (container)))
        ctk_widget_queue_resize (CTK_WIDGET (container));
    }
}

static void
ctk_rotated_bin_forall (GtkContainer *container,
                        gboolean      include_internals,
                        GtkCallback   callback,
                        gpointer      callback_data)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (container);

  g_return_if_fail (callback != NULL);

  if (bin->child)
    (*callback) (bin->child, callback_data);
}

void
ctk_rotated_bin_set_angle (GtkRotatedBin *bin,
                           gdouble        angle)
{
  g_return_if_fail (CTK_IS_ROTATED_BIN (bin));

  bin->angle = angle;
  ctk_widget_queue_resize (CTK_WIDGET (bin));

  gdk_window_geometry_changed (bin->offscreen_window);
}

static void
ctk_rotated_bin_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (widget);
  GtkRequisition child_requisition;
  double s, c;
  double w, h;
  guint border_width;

  child_requisition.width = 0;
  child_requisition.height = 0;

  if (bin->child && ctk_widget_get_visible (bin->child))
    ctk_widget_get_preferred_size ( (bin->child),
                               &child_requisition, NULL);

  s = sin (bin->angle);
  c = cos (bin->angle);
  w = c * child_requisition.width + s * child_requisition.height;
  h = s * child_requisition.width + c * child_requisition.height;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));
  requisition->width = border_width * 2 + w;
  requisition->height = border_width * 2 + h;
}

static void
ctk_rotated_bin_get_preferred_width (GtkWidget *widget,
                                     gint      *minimum,
                                     gint      *natural)
{
  GtkRequisition requisition;

  ctk_rotated_bin_size_request (widget, &requisition);

  *minimum = *natural = requisition.width;
}

static void
ctk_rotated_bin_get_preferred_height (GtkWidget *widget,
                                      gint      *minimum,
                                      gint      *natural)
{
  GtkRequisition requisition;

  ctk_rotated_bin_size_request (widget, &requisition);

  *minimum = *natural = requisition.height;
}

static void
ctk_rotated_bin_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (widget);
  guint border_width;
  gint w, h;
  gdouble s, c;

  ctk_widget_set_allocation (widget, allocation);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  w = allocation->width - border_width * 2;
  h = allocation->height - border_width * 2;

  if (ctk_widget_get_realized (widget))
    gdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x + border_width,
                            allocation->y + border_width,
                            w, h);

  if (bin->child && ctk_widget_get_visible (bin->child))
    {
      GtkRequisition child_requisition;
      GtkAllocation child_allocation;

      s = sin (bin->angle);
      c = cos (bin->angle);

      ctk_widget_get_preferred_size (bin->child,
                                     &child_requisition, NULL);
      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.height = child_requisition.height;
      if (c == 0.0)
        child_allocation.width = h / s;
      else if (s == 0.0)
        child_allocation.width = w / c;
      else
        child_allocation.width = MIN ((w - s * child_allocation.height) / c,
                                      (h - c * child_allocation.height) / s);

      if (ctk_widget_get_realized (widget))
        gdk_window_move_resize (bin->offscreen_window,
                                child_allocation.x,
                                child_allocation.y,
                                child_allocation.width,
                                child_allocation.height);

      child_allocation.x = child_allocation.y = 0;
      ctk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static gboolean
ctk_rotated_bin_damage (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  gdk_window_invalidate_rect (ctk_widget_get_window (widget),
                              NULL, FALSE);

  return TRUE;
}

static gboolean
ctk_rotated_bin_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GtkRotatedBin *bin = CTK_ROTATED_BIN (widget);
  GdkWindow *window;
  gdouble s, c;
  gdouble w, h;

  window = ctk_widget_get_window (widget);
  if (ctk_cairo_should_draw_window (cr, window))
    {
      cairo_surface_t *surface;
      GtkAllocation child_area;

      if (bin->child && ctk_widget_get_visible (bin->child))
        {
          surface = gdk_offscreen_window_get_surface (bin->offscreen_window);
          ctk_widget_get_allocation (bin->child, &child_area);

          /* transform */
          s = sin (bin->angle);
          c = cos (bin->angle);
          w = c * child_area.width + s * child_area.height;
          h = s * child_area.width + c * child_area.height;

          cairo_translate (cr, (w - child_area.width) / 2, (h - child_area.height) / 2);
          cairo_translate (cr, child_area.width / 2, child_area.height / 2);
          cairo_rotate (cr, bin->angle);
          cairo_translate (cr, -child_area.width / 2, -child_area.height / 2);

          /* clip */
          cairo_rectangle (cr,
                           0, 0,
                           gdk_window_get_width (bin->offscreen_window),
                           gdk_window_get_height (bin->offscreen_window));
          cairo_clip (cr);
          /* paint */
          cairo_set_source_surface (cr, surface, 0, 0);
          cairo_paint (cr);
        }
    }
  if (ctk_cairo_should_draw_window (cr, bin->offscreen_window))
    {
      ctk_render_background (ctk_widget_get_style_context (widget),
                             cr,
                             0, 0,
                             gdk_window_get_width (bin->offscreen_window),
                             gdk_window_get_height (bin->offscreen_window));

      if (bin->child)
        ctk_container_propagate_draw (CTK_CONTAINER (widget),
                                      bin->child,
                                      cr);
    }

  return FALSE;
}

/*** ***/

static void
scale_changed (GtkRange      *range,
               GtkRotatedBin *bin)
{
  ctk_rotated_bin_set_angle (bin, ctk_range_get_value (range));
}

GtkWidget *
do_offscreen_window (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *bin, *vbox, *scale, *button;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Rotated Button");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_container_set_border_width (CTK_CONTAINER (window), 10);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL,
                                        0, G_PI/2, 0.01);
      ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);

      button = ctk_button_new_with_label ("A Button");
      bin = ctk_rotated_bin_new ();

      g_signal_connect (scale, "value-changed", G_CALLBACK (scale_changed), bin);

      ctk_container_add (CTK_CONTAINER (window), vbox);
      ctk_box_pack_start (CTK_BOX (vbox), scale, FALSE, FALSE, 0);
      ctk_box_pack_start (CTK_BOX (vbox), bin, TRUE, TRUE, 0);
      ctk_container_add (CTK_CONTAINER (bin), button);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
