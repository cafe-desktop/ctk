#ifndef __FRAME_STATS_H__
#define __FRAME_STATS_H__

#include <ctk/ctk.h>

void frame_stats_add_options (GOptionGroup *group);
void frame_stats_ensure      (CtkWindow    *window);

#endif /* __FRAME_STATS_H__ */
