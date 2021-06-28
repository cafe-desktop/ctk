/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* CTK - The GIMP Toolkit
 * Copyright (C) 2015 Red Hat, Inc
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

#ifndef __CTK_FILE_FILTER_PRIVATE_H__
#define __CTK_FILE_FILTER_PRIVATE_H__

#include <ctk/ctkfilefilter.h>
#include <cdk/cdkconfig.h>

#ifdef GDK_WINDOWING_QUARTZ
#import <Foundation/Foundation.h>
#endif

G_BEGIN_DECLS

char ** _ctk_file_filter_get_as_patterns (CtkFileFilter      *filter);

#ifdef GDK_WINDOWING_QUARTZ
NSArray * _ctk_file_filter_get_as_pattern_nsstrings (CtkFileFilter *filter);
#endif


G_END_DECLS

#endif /* __CTK_FILE_FILTER_PRIVATE_H__ */
