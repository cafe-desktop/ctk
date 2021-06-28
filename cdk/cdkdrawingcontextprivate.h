#ifndef __CDK_DRAWING_CONTEXT_PRIVATE_H__
#define __CDK_DRAWING_CONTEXT_PRIVATE_H__

#include "cdkdrawingcontext.h"

G_BEGIN_DECLS

#define CDK_DRAWING_CONTEXT_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_DRAWING_CONTEXT, CdkDrawingContextClass))
#define CDK_IS_DRAWING_CONTEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_DRAWING_CONTEXT))
#define CDK_DRAWING_CONTEXT_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_DRAWING_CONTEXT, CdkDrawingContextClass))

struct _CdkDrawingContext
{
  GObject parent_instance;

  CdkWindow *window;

  cairo_region_t *clip;
  cairo_t *cr;
};

struct _CdkDrawingContextClass
{
  GObjectClass parent_instance;
};

G_END_DECLS

#endif /* __CDK_DRAWING_CONTEXT_PRIVATE_H__ */
