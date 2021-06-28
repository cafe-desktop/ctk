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

#ifndef __CTK_BIN_H__
#define __CTK_BIN_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>

G_BEGIN_DECLS

#define CTK_TYPE_BIN                  (ctk_bin_get_type ())
#define CTK_BIN(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BIN, CtkBin))
#define CTK_BIN_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_BIN, CtkBinClass))
#define CTK_IS_BIN(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BIN))
#define CTK_IS_BIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_BIN))
#define CTK_BIN_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BIN, CtkBinClass))


typedef struct _CtkBin              CtkBin;
typedef struct _CtkBinPrivate       CtkBinPrivate;
typedef struct _CtkBinClass         CtkBinClass;

struct _CtkBin
{
  CtkContainer container;

  /*< private >*/
  CtkBinPrivate *priv;
};

/**
 * CtkBinClass:
 * @parent_class: The parent class.
 */
struct _CtkBinClass
{
  CtkContainerClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType      ctk_bin_get_type  (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_bin_get_child (CtkBin *bin);

void       _ctk_bin_set_child (CtkBin    *bin,
                               CtkWidget *widget);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkBin, g_object_unref)

G_END_DECLS

#endif /* __CTK_BIN_H__ */
