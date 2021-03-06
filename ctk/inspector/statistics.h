/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#ifndef _CTK_INSPECTOR_STATISTICS_H_
#define _CTK_INSPECTOR_STATISTICS_H_

#include <ctk/ctkbox.h>

#define CTK_TYPE_INSPECTOR_STATISTICS            (ctk_inspector_statistics_get_type())
#define CTK_INSPECTOR_STATISTICS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_STATISTICS, CtkInspectorStatistics))
#define CTK_INSPECTOR_STATISTICS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_STATISTICS, CtkInspectorStatisticsClass))
#define CTK_INSPECTOR_IS_STATISTICS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_STATISTICS))
#define CTK_INSPECTOR_IS_STATISTICS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_STATISTICS))
#define CTK_INSPECTOR_STATISTICS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_STATISTICS, CtkInspectorStatisticsClass))


typedef struct _CtkInspectorStatisticsPrivate CtkInspectorStatisticsPrivate;

typedef struct _CtkInspectorStatistics
{
  CtkBox parent;
  CtkInspectorStatisticsPrivate *priv;
} CtkInspectorStatistics;

typedef struct _CtkInspectorStatisticsClass
{
  CtkBoxClass parent;
} CtkInspectorStatisticsClass;

G_BEGIN_DECLS

GType      ctk_inspector_statistics_get_type   (void);

G_END_DECLS

#endif // _CTK_INSPECTOR_STATISTICS_H_

// vim: set et sw=2 ts=2:
