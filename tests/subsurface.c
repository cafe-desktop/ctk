#include <ctk/ctk.h>


static void     da_realize       (CtkWidget     *widget);
static void     da_size_allocate (CtkWidget     *widget,
                                  CtkAllocation *allocation);
static gboolean da_draw          (CtkWidget     *widget,
                                  cairo_t       *cr);

typedef CtkDrawingArea DArea;
typedef CtkDrawingAreaClass DAreaClass;

G_DEFINE_TYPE (DArea, da, CTK_TYPE_WIDGET)

static void
da_class_init (DAreaClass *class)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  widget_class->realize = da_realize;
  widget_class->size_allocate = da_size_allocate;
  widget_class->draw = da_draw;
}

static void
da_init (DArea *darea)
{
  ctk_widget_set_has_window (CTK_WIDGET (darea), TRUE);
}

CtkWidget*
da_new (void)
{
  return g_object_new (da_get_type (), NULL);
}

static void
da_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = CDK_WINDOW_SUBSURFACE;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = ctk_widget_get_events (widget) | CDK_EXPOSURE_MASK;

  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_register_window (widget, window);
  ctk_widget_set_window (widget, window);
}

static void
da_size_allocate (CtkWidget     *widget,
                  CtkAllocation *allocation)
{
  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);
}

static gboolean
da_draw (CtkWidget *widget G_GNUC_UNUSED,
         cairo_t   *cr)
{
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0); 
  cairo_paint (cr);

  return TRUE;
}

int
main (int   argc G_GNUC_UNUSED,
      char *argv[] G_GNUC_UNUSED)
{
  CtkWidget *window, *label, *box, *widget;
  CtkWidget *stack, *switcher;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_resizable (CTK_WINDOW (window), TRUE);
  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);

  stack = ctk_stack_new ();
  switcher = ctk_stack_switcher_new ();
  ctk_stack_switcher_set_stack (CTK_STACK_SWITCHER (switcher), CTK_STACK (stack));
  ctk_container_add (CTK_CONTAINER (box), switcher);
  ctk_container_add (CTK_CONTAINER (box), stack);

  label = ctk_label_new ("Test test");
  ctk_stack_add_titled (CTK_STACK (stack), label, "1", "One");
  widget = da_new ();
  ctk_widget_set_size_request (widget, 100, 100);
  ctk_stack_add_titled (CTK_STACK (stack), widget, "2", "Two");
  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
