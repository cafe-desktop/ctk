/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __CDK_X11_KEYS_H__
#define __CDK_X11_KEYS_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#ifdef CDK_COMPILATION
typedef struct _CdkX11Keymap CdkX11Keymap;
#else
typedef CdkKeymap CdkX11Keymap;
#endif
typedef struct _CdkX11KeymapClass CdkX11KeymapClass;

#define CDK_TYPE_X11_KEYMAP              (cdk_x11_keymap_get_type())
#define CDK_X11_KEYMAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_X11_KEYMAP, CdkX11Keymap))
#define CDK_X11_KEYMAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_X11_KEYMAP, CdkX11KeymapClass))
#define CDK_IS_X11_KEYMAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_X11_KEYMAP))
#define CDK_IS_X11_KEYMAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_X11_KEYMAP))
#define CDK_X11_KEYMAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_X11_KEYMAP, CdkX11KeymapClass))

CDK_AVAILABLE_IN_ALL
GType cdk_x11_keymap_get_type (void);

CDK_AVAILABLE_IN_3_6
gint cdk_x11_keymap_get_group_for_state (CdkKeymap *keymap,
                                         guint      state);

CDK_AVAILABLE_IN_3_6
gboolean cdk_x11_keymap_key_is_modifier (CdkKeymap *keymap,
                                         guint      keycode);
G_END_DECLS

#endif /* __CDK_X11_KEYMAP_H__ */
