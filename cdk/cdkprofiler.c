/* CDK - The GIMP Drawing Kit
 *
 * cdkprofiler.c: A simple profiler
 *
 * Copyright © 2018 Matthias Clasen
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

#include "config.h"

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "cdkversionmacros.h"
#include "cdkprofilerprivate.h"
#include "cdkframeclockprivate.h"

#ifdef HAVE_SYSPROF_CAPTURE

#include <sysprof-capture.h>

static SysprofCaptureWriter *writer = NULL;
static gboolean running = FALSE;

static void
profiler_stop (void)
{
  if (writer)
    sysprof_capture_writer_unref (writer);
}

void
cdk_profiler_start (int fd)
{
  if (writer)
    return;

  sysprof_clock_init ();

  if (fd == -1)
    {
      gchar *filename;

      filename = g_strdup_printf ("ctk.%d.syscap", getpid ());
      g_print ("Writing profiling data to %s\n", filename);
      writer = sysprof_capture_writer_new (filename, 16*1024);
      g_free (filename);
    }
  else if (fd > 2)
    writer = sysprof_capture_writer_new_from_fd (fd, 16*1024);

  if (writer)
    running = TRUE;

  atexit (profiler_stop);
}

void
cdk_profiler_stop (void)
{
  running = FALSE;
}

gboolean
cdk_profiler_is_running (void)
{
  return running;
}

void
cdk_profiler_add_mark (gint64      start,
                       guint64     duration,
                       const char *name,
                       const char *message)
{
  if (!running)
    return;

  sysprof_capture_writer_add_mark (writer,
                                   start,
                                   -1, getpid (),
                                   duration,
                                   "ctk", name, message);
}

static guint
define_counter (const char *name,
                const char *description,
                int         type)
{
  SysprofCaptureCounter counter;

  if (!writer)
    return 0;

  counter.id = (guint) sysprof_capture_writer_request_counter (writer, 1);
  counter.type = type;
  counter.value.vdbl = 0;
  g_strlcpy (counter.category, "ctk", sizeof counter.category);
  g_strlcpy (counter.name, name, sizeof counter.name);
  g_strlcpy (counter.description, description, sizeof counter.name);

  sysprof_capture_writer_define_counters (writer,
                                          SYSPROF_CAPTURE_CURRENT_TIME,
                                          -1,
                                          getpid (),
                                          &counter,
                                          1);

  return counter.id;
}

guint
cdk_profiler_define_counter (const char *name,
                             const char *description)
{
  return define_counter (name, description, SYSPROF_CAPTURE_COUNTER_DOUBLE);
}

guint
cdk_profiler_define_int_counter (const char *name,
                                 const char *description)
{
  return define_counter (name, description, SYSPROF_CAPTURE_COUNTER_INT64);
}

void
cdk_profiler_set_counter (guint  id,
                          gint64 time,
                          double val)
{
  SysprofCaptureCounterValue value;

  if (!running)
    return;

  value.vdbl = val;
  sysprof_capture_writer_set_counters (writer,
                                       time,
                                       -1, getpid (),
                                       &id, &value, 1);
}

void
cdk_profiler_set_int_counter (guint  id,
                              gint64 time,
                              gint64 val)
{
  SysprofCaptureCounterValue value;

  if (!running)
    return;

  value.v64 = val;
  sysprof_capture_writer_set_counters (writer,
                                       time,
                                       -1, getpid (),
                                       &id, &value, 1);
}

#else

void
cdk_profiler_start (int fd)
{
}

void
cdk_profiler_stop (void)
{
}

gboolean
cdk_profiler_is_running (void)
{
  return FALSE;
}

void
cdk_profiler_add_mark (gint64      start,
                       guint64     duration,
                       const char *name,
                       const char *message)
{
}

guint
cdk_profiler_define_counter (const char *name,
                             const char *description)
{
 return 0;
}

void
cdk_profiler_set_counter (guint  id,
                          gint64 time,
                          double value)
{
}

guint
cdk_profiler_define_int_counter (const char *name,
                                 const char *description)
{
  return 0;
}

void
cdk_profiler_set_int_counter (guint  id,
                              gint64 time,
                              gint64 value)
{
}

#endif /* G_OS_WIN32 */
