/* GTK - The GIMP Toolkit
 *
 * ctkglarea.h: A GL drawing area
 *
 * Copyright © 2014  Emmanuele Bassi
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

#ifndef __CTK_GL_AREA_H__
#define __CTK_GL_AREA_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_GL_AREA                (ctk_gl_area_get_type ())
#define CTK_GL_AREA(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GL_AREA, CtkGLArea))
#define CTK_IS_GL_AREA(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GL_AREA))
#define CTK_GL_AREA_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GL_AREA, CtkGLAreaClass))
#define CTK_IS_GL_AREA_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GL_AREA))
#define CTK_GL_AREA_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GL_AREA, CtkGLAreaClass))

typedef struct _CtkGLArea               CtkGLArea;
typedef struct _CtkGLAreaClass          CtkGLAreaClass;

/**
 * CtkGLArea:
 *
 * A #CtkWidget used for drawing with OpenGL.
 *
 * Since: 3.16
 */
struct _CtkGLArea
{
  /*< private >*/
  CtkWidget parent_instance;
};

/**
 * CtkGLAreaClass:
 * @render: class closure for the #CtkGLArea::render signal
 * @resize: class closeure for the #CtkGLArea::resize signal
 * @create_context: class closure for the #CtkGLArea::create-context signal
 *
 * The `CtkGLAreaClass` structure contains only private data.
 *
 * Since: 3.16
 */
struct _CtkGLAreaClass
{
  /*< private >*/
  CtkWidgetClass parent_class;

  /*< public >*/
  gboolean       (* render)         (CtkGLArea        *area,
                                     GdkGLContext     *context);
  void           (* resize)         (CtkGLArea        *area,
                                     int               width,
                                     int               height);
  GdkGLContext * (* create_context) (CtkGLArea        *area);

  /*< private >*/
  gpointer _padding[6];
};

GDK_AVAILABLE_IN_3_16
GType ctk_gl_area_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_16
CtkWidget *     ctk_gl_area_new                         (void);

GDK_AVAILABLE_IN_3_22
void            ctk_gl_area_set_use_es                  (CtkGLArea    *area,
                                                         gboolean      use_es);
GDK_AVAILABLE_IN_3_22
gboolean        ctk_gl_area_get_use_es                  (CtkGLArea    *area);
GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_set_required_version        (CtkGLArea    *area,
                                                         gint          major,
                                                         gint          minor);
GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_get_required_version        (CtkGLArea    *area,
                                                         gint         *major,
                                                         gint         *minor);
GDK_AVAILABLE_IN_3_16
gboolean        ctk_gl_area_get_has_alpha               (CtkGLArea    *area);
GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_set_has_alpha               (CtkGLArea    *area,
                                                         gboolean      has_alpha);
GDK_AVAILABLE_IN_3_16
gboolean        ctk_gl_area_get_has_depth_buffer        (CtkGLArea    *area);
GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_set_has_depth_buffer        (CtkGLArea    *area,
                                                         gboolean      has_depth_buffer);
GDK_AVAILABLE_IN_3_16
gboolean        ctk_gl_area_get_has_stencil_buffer      (CtkGLArea    *area);
GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_set_has_stencil_buffer      (CtkGLArea    *area,
                                                         gboolean      has_stencil_buffer);
GDK_AVAILABLE_IN_3_16
gboolean        ctk_gl_area_get_auto_render             (CtkGLArea    *area);
GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_set_auto_render             (CtkGLArea    *area,
                                                         gboolean      auto_render);
GDK_AVAILABLE_IN_3_16
void           ctk_gl_area_queue_render                 (CtkGLArea    *area);


GDK_AVAILABLE_IN_3_16
GdkGLContext *  ctk_gl_area_get_context                 (CtkGLArea    *area);

GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_make_current                (CtkGLArea    *area);
GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_attach_buffers              (CtkGLArea    *area);

GDK_AVAILABLE_IN_3_16
void            ctk_gl_area_set_error                   (CtkGLArea    *area,
                                                         const GError *error);
GDK_AVAILABLE_IN_3_16
GError *        ctk_gl_area_get_error                   (CtkGLArea    *area);

G_END_DECLS

#endif /* __CTK_GL_AREA_H__ */
