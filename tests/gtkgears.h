#ifndef __CTK_GEARS_H__
#define __CTK_GEARS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
  CTK_GEARS_X_AXIS,
  CTK_GEARS_Y_AXIS,
  CTK_GEARS_Z_AXIS,

  CTK_GEARS_N_AXIS
};

#define CTK_TYPE_GEARS      (ctk_gears_get_type ())
#define CTK_GEARS(inst)     (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                             CTK_TYPE_GEARS,             \
                             GtkGears))
#define CTK_IS_GEARS(inst)  (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                             CTK_TYPE_GEARS))

typedef struct _GtkGears GtkGears;
typedef struct _GtkGearsClass GtkGearsClass;

struct _GtkGears {
  GtkGLArea parent;
};

struct _GtkGearsClass {
  GtkGLAreaClass parent_class;
};

GType      ctk_gears_get_type      (void) G_GNUC_CONST;

GtkWidget *ctk_gears_new           (void);
void       ctk_gears_set_axis      (GtkGears *gears,
                                    int       axis,
                                    double    value);
double     ctk_gears_get_axis      (GtkGears *gears,
                                    int       axis);
void       ctk_gears_set_fps_label (GtkGears *gears,
                                    GtkLabel *label);


G_END_DECLS

#endif /* __CTK_GEARS_H__ */
