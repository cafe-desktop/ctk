#ifndef __CTK_OFFSCREEN_BOX_H__
#define __CTK_OFFSCREEN_BOX_H__


#include <cdk/cdk.h>
#include <ctk/ctk.h>


G_BEGIN_DECLS

#define CTK_TYPE_OFFSCREEN_BOX              (ctk_offscreen_box_get_type ())
#define CTK_OFFSCREEN_BOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_OFFSCREEN_BOX, CtkOffscreenBox))
#define CTK_OFFSCREEN_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_OFFSCREEN_BOX, CtkOffscreenBoxClass))
#define CTK_IS_OFFSCREEN_BOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_OFFSCREEN_BOX))
#define CTK_IS_OFFSCREEN_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_OFFSCREEN_BOX))
#define CTK_OFFSCREEN_BOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_OFFSCREEN_BOX, CtkOffscreenBoxClass))

typedef struct _CtkOffscreenBox	  CtkOffscreenBox;
typedef struct _CtkOffscreenBoxClass  CtkOffscreenBoxClass;

struct _CtkOffscreenBox
{
  CtkContainer container;

  CtkWidget *child1;
  CtkWidget *child2;

  CdkWindow *offscreen_window1;
  CdkWindow *offscreen_window2;

  gdouble angle;
};

struct _CtkOffscreenBoxClass
{
  CtkBinClass parent_class;
};

GType	   ctk_offscreen_box_get_type           (void) G_GNUC_CONST;
CtkWidget* ctk_offscreen_box_new       (void);
void       ctk_offscreen_box_add1      (CtkOffscreenBox *offscreen,
					CtkWidget       *child);
void       ctk_offscreen_box_add2      (CtkOffscreenBox *offscreen,
					CtkWidget       *child);
void       ctk_offscreen_box_set_angle (CtkOffscreenBox *offscreen,
					double           angle);



G_END_DECLS

#endif /* __CTK_OFFSCREEN_BOX_H__ */
