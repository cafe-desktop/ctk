/* GDK - The GIMP Drawing Kit
 *
 * cdkglcontextprivate.h: GL context abstraction
 * 
 * Copyright © 2014  Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GDK_GL_CONTEXT_PRIVATE_H__
#define __GDK_GL_CONTEXT_PRIVATE_H__

#include "cdkglcontext.h"

G_BEGIN_DECLS

#define GDK_GL_CONTEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_CONTEXT, CdkGLContextClass))
#define GDK_IS_GL_CONTEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_CONTEXT))
#define GDK_GL_CONTEXT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_CONTEXT, CdkGLContextClass))

typedef struct _CdkGLContextClass       CdkGLContextClass;

struct _CdkGLContext
{
  GObject parent_instance;
};

struct _CdkGLContextClass
{
  GObjectClass parent_class;

  gboolean (* realize) (CdkGLContext *context,
                        GError **error);

  void (* end_frame)    (CdkGLContext *context,
                         cairo_region_t *painted,
                         cairo_region_t *damage);
  gboolean (* texture_from_surface) (CdkGLContext    *context,
                                     cairo_surface_t *surface,
                                     cairo_region_t  *region);
};

typedef struct {
  guint program;
  guint position_location;
  guint uv_location;
  guint map_location;
  guint flip_location;
} CdkGLContextProgram;

typedef struct {
  guint vertex_array_object;
  guint tmp_framebuffer;
  guint tmp_vertex_buffer;

  CdkGLContextProgram texture_2d_quad_program;
  CdkGLContextProgram texture_rect_quad_program;

  CdkGLContextProgram *current_program;

  guint is_legacy : 1;
  guint use_es : 1;
} CdkGLContextPaintData;

void                    cdk_gl_context_set_is_legacy            (CdkGLContext    *context,
                                                                 gboolean         is_legacy);

void                    cdk_gl_context_upload_texture           (CdkGLContext    *context,
                                                                 cairo_surface_t *image_surface,
                                                                 int              width,
                                                                 int              height,
                                                                 guint            texture_target);
CdkGLContextPaintData * cdk_gl_context_get_paint_data           (CdkGLContext    *context);
gboolean                cdk_gl_context_use_texture_rectangle    (CdkGLContext    *context);
gboolean                cdk_gl_context_has_framebuffer_blit     (CdkGLContext    *context);
gboolean                cdk_gl_context_has_frame_terminator     (CdkGLContext    *context);
gboolean                cdk_gl_context_has_unpack_subimage      (CdkGLContext    *context);
void                    cdk_gl_context_end_frame                (CdkGLContext    *context,
                                                                 cairo_region_t  *painted,
                                                                 cairo_region_t  *damage);

G_END_DECLS

#endif /* __GDK_GL_CONTEXT_PRIVATE_H__ */
