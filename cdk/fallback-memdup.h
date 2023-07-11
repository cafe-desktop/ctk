// Fallback for versions of GLib that do not have the g_memdup2 function.

#ifndef _FALLBACKMEMDUP_H_
#define _FALLBACKMEMDUP_H_

#if ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION < 68))
#undef g_memdup2
#define g_memdup2 g_memdup
#endif 

#endif 
