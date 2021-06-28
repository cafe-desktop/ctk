#include <ctk/ctk.h>

#ifdef CDK_WINDOWING_DIRECTFB
#define cdk_display cdk_display_directfb
#include <cdk/directfb/cdkdirectfb.h>
#undef cdk_display
#undef CDK_DISPLAY
#undef CDK_ROOT_WINDOW
#endif

#ifdef CDK_WINDOWING_QUARTZ
#if HAVE_OBJC
#define cdk_display cdk_display_quartz
#include <cdk/quartz/cdkquartz.h>
#undef cdk_display
#undef CDK_DISPLAY
#undef CDK_ROOT_WINDOW
#endif
#endif

#ifndef __OBJC__
#ifdef CDK_WINDOWING_WIN32
#define cdk_display cdk_display_win32
#include <cdk/win32/cdkwin32.h>
#undef cdk_display
#undef CDK_DISPLAY
#undef CDK_ROOT_WINDOW
#endif
#endif

#ifdef CDK_WINDOWING_X11
#define cdk_display cdk_display_x11
#include <cdk/x11/cdkx.h>
#undef cdk_display
#undef CDK_DISPLAY
#undef CDK_ROOT_WINDOW
#endif

int main() { return 0; }
