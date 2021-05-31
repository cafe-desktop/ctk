#ifndef __CTK_OFFSCREEN_BOX_H__
#define __CTK_OFFSCREEN_BOX_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

#define CTK_TYPE_OFFSCREEN_BOX              (ctk_offscreen_box_get_type ())
#define CTK_OFFSCREEN_BOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_OFFSCREEN_BOX, GtkOffscreenBox))
#define CTK_OFFSCREEN_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_OFFSCREEN_BOX, GtkOffscreenBoxClass))
#define CTK_IS_OFFSCREEN_BOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_OFFSCREEN_BOX))
#define CTK_IS_OFFSCREEN_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_OFFSCREEN_BOX))
#define CTK_OFFSCREEN_BOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_OFFSCREEN_BOX, GtkOffscreenBoxClass))

typedef struct _GtkOffscreenBox	  GtkOffscreenBox;
typedef struct _GtkOffscreenBoxClass  GtkOffscreenBoxClass;

struct _GtkOffscreenBox
{
  GtkContainer container;

  GtkWidget *child1;
  GtkWidget *child2;

  GdkWindow *offscreen_window1;
  GdkWindow *offscreen_window2;

  gdouble angle;
};

struct _GtkOffscreenBoxClass
{
  GtkBinClass parent_class;
};

GType	   ctk_offscreen_box_get_type           (void) G_GNUC_CONST;
GtkWidget* ctk_offscreen_box_new       (void);
void       ctk_offscreen_box_add1      (GtkOffscreenBox *offscreen,
					GtkWidget       *child);
void       ctk_offscreen_box_add2      (GtkOffscreenBox *offscreen,
					GtkWidget       *child);
void       ctk_offscreen_box_set_angle (GtkOffscreenBox *offscreen,
					double           angle);



G_END_DECLS

#endif /* __CTK_OFFSCREEN_BOX_H__ */
