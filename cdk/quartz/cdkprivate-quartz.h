/* cdkprivate-quartz.h
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#ifndef __CDK_PRIVATE_QUARTZ_H__
#define __CDK_PRIVATE_QUARTZ_H__


#include <cdk/cdk.h>

#include "cdkinternals.h"

#include "config.h"

#define CDK_WINDOW_IS_QUARTZ(win)        (CDK_IS_WINDOW_IMPL_QUARTZ (((CdkWindow *)win)->impl))


/* Display */

CdkDisplay *    _cdk_quartz_display_open (const gchar *name);


#endif /* __CDK_PRIVATE_QUARTZ_H__ */
