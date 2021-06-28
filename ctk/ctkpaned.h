/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_PANED_H__
#define __CTK_PANED_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>

G_BEGIN_DECLS

#define CTK_TYPE_PANED                  (ctk_paned_get_type ())
#define CTK_PANED(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PANED, CtkPaned))
#define CTK_PANED_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PANED, CtkPanedClass))
#define CTK_IS_PANED(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PANED))
#define CTK_IS_PANED_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PANED))
#define CTK_PANED_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PANED, CtkPanedClass))


typedef struct _CtkPaned        CtkPaned;
typedef struct _CtkPanedClass   CtkPanedClass;
typedef struct _CtkPanedPrivate CtkPanedPrivate;

struct _CtkPaned
{
  CtkContainer container;

  /*< private >*/
  CtkPanedPrivate *priv;
};

struct _CtkPanedClass
{
  CtkContainerClass parent_class;

  gboolean (* cycle_child_focus)   (CtkPaned      *paned,
				    gboolean       reverse);
  gboolean (* toggle_handle_focus) (CtkPaned      *paned);
  gboolean (* move_handle)         (CtkPaned      *paned,
				    CtkScrollType  scroll);
  gboolean (* cycle_handle_focus)  (CtkPaned      *paned,
				    gboolean       reverse);
  gboolean (* accept_position)     (CtkPaned	  *paned);
  gboolean (* cancel_position)     (CtkPaned	  *paned);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType       ctk_paned_get_type     (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_paned_new          (CtkOrientation orientation);
CDK_AVAILABLE_IN_ALL
void        ctk_paned_add1         (CtkPaned       *paned,
                                    CtkWidget      *child);
CDK_AVAILABLE_IN_ALL
void        ctk_paned_add2         (CtkPaned       *paned,
                                    CtkWidget      *child);
CDK_AVAILABLE_IN_ALL
void        ctk_paned_pack1        (CtkPaned       *paned,
                                    CtkWidget      *child,
                                    gboolean        resize,
                                    gboolean        shrink);
CDK_AVAILABLE_IN_ALL
void        ctk_paned_pack2        (CtkPaned       *paned,
                                    CtkWidget      *child,
                                    gboolean        resize,
                                    gboolean        shrink);

CDK_AVAILABLE_IN_ALL
gint        ctk_paned_get_position (CtkPaned       *paned);
CDK_AVAILABLE_IN_ALL
void        ctk_paned_set_position (CtkPaned       *paned,
                                    gint            position);

CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_paned_get_child1   (CtkPaned       *paned);
CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_paned_get_child2   (CtkPaned       *paned);

CDK_AVAILABLE_IN_ALL
CdkWindow * ctk_paned_get_handle_window (CtkPaned  *paned);

CDK_AVAILABLE_IN_3_16
void        ctk_paned_set_wide_handle (CtkPaned    *paned,
                                       gboolean     wide);
CDK_AVAILABLE_IN_3_16
gboolean    ctk_paned_get_wide_handle (CtkPaned    *paned);


G_END_DECLS

#endif /* __CTK_PANED_H__ */
