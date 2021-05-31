#ifndef __GTKUTILS_H__
#define __GTKUTILS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

gboolean        ctk_scan_string         (const char     **pos,
                                         GString         *out);
gboolean        ctk_skip_space          (const char     **pos);
gint            ctk_read_line           (FILE            *stream,
                                         GString         *str);
char *          ctk_trim_string         (const char      *str);
char **         ctk_split_file_list     (const char      *str);
GBytes         *ctk_file_load_bytes     (GFile           *file,
                                         GCancellable    *cancellable,
                                         GError         **error);

G_END_DECLS

#endif
