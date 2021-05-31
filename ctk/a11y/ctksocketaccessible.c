/* CTK+ - accessibility implementations
 * Copyright 2019 Samuel Thibault <sthibault@hypra.fr>
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

#include "config.h"

#include <ctk/ctk.h>
#include "ctksocketaccessible.h"

/* We can not make CtkSocketAccessible inherit both from CtkContainerAccessible
 * and CtkSocket, so we make it the atk parent of an AtkSocket */

struct _CtkSocketAccessiblePrivate
{
  AtkObject *accessible_socket;
};

G_DEFINE_TYPE_WITH_CODE (CtkSocketAccessible, ctk_socket_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkSocketAccessible))

static AtkObject*
ctk_socket_accessible_ref_child (AtkObject *obj, int i)
{
  CtkSocketAccessible *socket = CTK_SOCKET_ACCESSIBLE (obj);

  if (i != 0)
    return NULL;

  return g_object_ref (socket->priv->accessible_socket);
}

static int
ctk_socket_accessible_get_n_children (AtkObject *obj)
{
  return 1;
}

static void
ctk_socket_accessible_finalize (GObject *object)
{
  CtkSocketAccessible *socket = CTK_SOCKET_ACCESSIBLE (object);
  CtkSocketAccessiblePrivate *priv = socket->priv;

  g_clear_object (&priv->accessible_socket);

  G_OBJECT_CLASS (ctk_socket_accessible_parent_class)->finalize (object);
}

static void
ctk_socket_accessible_initialize (AtkObject *socket, gpointer data)
{
  AtkObject *atk_socket;

  ATK_OBJECT_CLASS (ctk_socket_accessible_parent_class)->initialize (socket, data);

  atk_socket = atk_socket_new ();

  CTK_SOCKET_ACCESSIBLE(socket)->priv->accessible_socket = atk_socket;
  atk_object_set_parent (atk_socket, socket);
}

static void
ctk_socket_accessible_class_init (CtkSocketAccessibleClass *klass)
{
  CtkContainerAccessibleClass *container_class = (CtkContainerAccessibleClass*)klass;
  AtkObjectClass              *atk_class       = ATK_OBJECT_CLASS (klass);
  GObjectClass                *gobject_class   = G_OBJECT_CLASS (klass);

  container_class->add_ctk    = NULL;
  container_class->remove_ctk = NULL;

  atk_class->initialize     = ctk_socket_accessible_initialize;
  atk_class->get_n_children = ctk_socket_accessible_get_n_children;
  atk_class->ref_child      = ctk_socket_accessible_ref_child;

  gobject_class->finalize = ctk_socket_accessible_finalize;
}

static void
ctk_socket_accessible_init (CtkSocketAccessible *socket)
{
  socket->priv = ctk_socket_accessible_get_instance_private (socket);
}

void
ctk_socket_accessible_embed (CtkSocketAccessible *socket, gchar *path)
{
  atk_socket_embed (ATK_SOCKET (socket->priv->accessible_socket), path);
}
