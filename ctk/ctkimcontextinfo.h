/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __CTK_IM_CONTEXT_INFO_H__
#define __CTK_IM_CONTEXT_INFO_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

typedef struct _CtkIMContextInfo CtkIMContextInfo;

struct _CtkIMContextInfo
{
  const gchar *context_id;
  const gchar *context_name;
  const gchar *domain;
  const gchar *domain_dirname;
  const gchar *default_locales;
};


G_END_DECLS

#endif /* __CTK_IM_CONTEXT_INFO_H__ */
