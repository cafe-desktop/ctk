/* ctkcellrendererprogress.h
 * Copyright (C) 2002 Naba Kumar <kh_naba@users.sourceforge.net>
 * modified by Jörgen Scheibengruber <mfcn@gmx.de>
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

/*
 * Modified by the CTK+ Team and others 1997-2004.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_CELL_RENDERER_PROGRESS_H__
#define __CTK_CELL_RENDERER_PROGRESS_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellrenderer.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_RENDERER_PROGRESS (ctk_cell_renderer_progress_get_type ())
#define CTK_CELL_RENDERER_PROGRESS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_RENDERER_PROGRESS, CtkCellRendererProgress))
#define CTK_CELL_RENDERER_PROGRESS_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_RENDERER_PROGRESS, CtkCellRendererProgressClass))
#define CTK_IS_CELL_RENDERER_PROGRESS(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_RENDERER_PROGRESS))
#define CTK_IS_CELL_RENDERER_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_RENDERER_PROGRESS))
#define CTK_CELL_RENDERER_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_RENDERER_PROGRESS, CtkCellRendererProgressClass))

typedef struct _CtkCellRendererProgress         CtkCellRendererProgress;
typedef struct _CtkCellRendererProgressClass    CtkCellRendererProgressClass;
typedef struct _CtkCellRendererProgressPrivate  CtkCellRendererProgressPrivate;

struct _CtkCellRendererProgress
{
  CtkCellRenderer parent_instance;

  /*< private >*/
  CtkCellRendererProgressPrivate *priv;
};

struct _CtkCellRendererProgressClass
{
  CtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType		 ctk_cell_renderer_progress_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkCellRenderer* ctk_cell_renderer_progress_new      (void);

G_END_DECLS

#endif  /* __CTK_CELL_RENDERER_PROGRESS_H__ */
