#ifndef __CTK_GEARS_H__
#define __CTK_GEARS_H__

#include <ctk/ctk.h>

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
                             CtkGears))
#define CTK_IS_GEARS(inst)  (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                             CTK_TYPE_GEARS))

typedef struct _CtkGears CtkGears;
typedef struct _CtkGearsClass CtkGearsClass;

struct _CtkGears {
  CtkGLArea parent;
};

struct _CtkGearsClass {
  CtkGLAreaClass parent_class;
};

GType      ctk_gears_get_type      (void) G_GNUC_CONST;

CtkWidget *ctk_gears_new           (void);
void       ctk_gears_set_axis      (CtkGears *gears,
                                    int       axis,
                                    double    value);
double     ctk_gears_get_axis      (CtkGears *gears,
                                    int       axis);
void       ctk_gears_set_fps_label (CtkGears *gears,
                                    CtkLabel *label);


G_END_DECLS

#endif /* __CTK_GEARS_H__ */
