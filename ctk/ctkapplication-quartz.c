/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "ctkapplicationprivate.h"
#include "ctkbuilder.h"
#import <Cocoa/Cocoa.h>

typedef struct
{
  guint cookie;
  CtkApplicationInhibitFlags flags;
  char *reason;
  CtkWindow *window;
} CtkApplicationQuartzInhibitor;

static void
ctk_application_quartz_inhibitor_free (CtkApplicationQuartzInhibitor *inhibitor)
{
  g_free (inhibitor->reason);
  g_clear_object (&inhibitor->window);
  g_slice_free (CtkApplicationQuartzInhibitor, inhibitor);
}

typedef CtkApplicationImplClass CtkApplicationImplQuartzClass;

typedef struct
{
  CtkApplicationImpl impl;

  CtkActionMuxer *muxer;
  GMenu *combined;

  GSList *inhibitors;
  gint quit_inhibit;
  guint next_cookie;
  NSObject *delegate;
} CtkApplicationImplQuartz;

G_DEFINE_TYPE (CtkApplicationImplQuartz, ctk_application_impl_quartz, CTK_TYPE_APPLICATION_IMPL)
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
@interface CtkApplicationQuartzDelegate : NSObject <NSApplicationDelegate>
#else
@interface CtkApplicationQuartzDelegate : NSObject
#endif
{
  CtkApplicationImplQuartz *quartz;
}

- (id)initWithImpl:(CtkApplicationImplQuartz*)impl;
- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender;
- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;
@end

@implementation CtkApplicationQuartzDelegate
-(id)initWithImpl:(CtkApplicationImplQuartz*)impl
{
  [super init];
  quartz = impl;
  return self;
}

-(NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender
{
  /* We have no way to give our message other than to pop up a dialog
   * ourselves, which we should not do since the OS will already show
   * one when we return NSTerminateNow.
   *
   * Just let the OS show the generic message...
   */
  return quartz->quit_inhibit == 0 ? NSTerminateNow : NSTerminateCancel;
}

-(void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
  GFile **files;
  gint i;
  GApplicationFlags flags;

  flags = g_application_get_flags (G_APPLICATION (quartz->impl.application));

  if (~flags & G_APPLICATION_HANDLES_OPEN)
    {
      [theApplication replyToOpenOrPrint:NSApplicationDelegateReplyFailure];
      return;
    }

  files = g_new (GFile *, [filenames count]);

  for (i = 0; i < [filenames count]; i++)
    files[i] = g_file_new_for_path ([(NSString *)[filenames objectAtIndex:i] UTF8String]);

  g_application_open (G_APPLICATION (quartz->impl.application), files, [filenames count], "");

  for (i = 0; i < [filenames count]; i++)
    g_object_unref (files[i]);

  g_free (files);

  [theApplication replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}
@end

/* these exist only for accel handling */
static void
ctk_application_impl_quartz_hide (GSimpleAction *action,
                                  GVariant      *parameter,
                                  gpointer       user_data)
{
  [NSApp hide:NSApp];
}

static void
ctk_application_impl_quartz_hide_others (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{
  [NSApp hideOtherApplications:NSApp];
}

static void
ctk_application_impl_quartz_show_all (GSimpleAction *action,
                                      GVariant      *parameter,
                                      gpointer       user_data)
{
  [NSApp unhideAllApplications:NSApp];
}

static GActionEntry ctk_application_impl_quartz_actions[] = {
  { "hide",             ctk_application_impl_quartz_hide        },
  { "hide-others",      ctk_application_impl_quartz_hide_others },
  { "show-all",         ctk_application_impl_quartz_show_all    }
};

static void
ctk_application_impl_quartz_startup (CtkApplicationImpl *impl,
                                     gboolean            register_session)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;
  GSimpleActionGroup *ctkinternal;
  GMenuModel *app_menu;
  const gchar *pref_accel[] = {"<Primary>comma", NULL};
  const gchar *hide_others_accel[] = {"<Primary><Alt>h", NULL};
  const gchar *hide_accel[] = {"<Primary>h", NULL};
  const gchar *quit_accel[] = {"<Primary>q", NULL};

  if (register_session)
    {
      quartz->delegate = [[CtkApplicationQuartzDelegate alloc] initWithImpl:quartz];
      [NSApp setDelegate: (id)(quartz->delegate)];
    }

  quartz->muxer = ctk_action_muxer_new ();
  ctk_action_muxer_set_parent (quartz->muxer, ctk_application_get_action_muxer (impl->application));

  /* Add the default accels */
  ctk_application_set_accels_for_action (impl->application, "app.preferences", pref_accel);
  ctk_application_set_accels_for_action (impl->application, "ctkinternal.hide-others", hide_others_accel);
  ctk_application_set_accels_for_action (impl->application, "ctkinternal.hide", hide_accel);
  ctk_application_set_accels_for_action (impl->application, "app.quit", quit_accel);

  /* and put code behind the 'special' accels */
  ctkinternal = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (ctkinternal), ctk_application_impl_quartz_actions,
                                   G_N_ELEMENTS (ctk_application_impl_quartz_actions), quartz);
  ctk_application_insert_action_group (impl->application, "ctkinternal", G_ACTION_GROUP (ctkinternal));
  g_object_unref (ctkinternal);

  /* now setup the menu */
  app_menu = ctk_application_get_app_menu (impl->application);
  if (app_menu == NULL)
    {
      CtkBuilder *builder;

      /* If the user didn't fill in their own menu yet, add ours.
       *
       * The fact that we do this here ensures that we will always have the
       * app menu at index 0 in 'combined'.
       */
      builder = ctk_builder_new_from_resource ("/org/ctk/libctk/ui/ctkapplication-quartz.ui");
      ctk_application_set_app_menu (impl->application, G_MENU_MODEL (ctk_builder_get_object (builder, "app-menu")));
      g_object_unref (builder);
    }
  else
    ctk_application_impl_set_app_menu (impl, app_menu);

  /* This may or may not add an item to 'combined' */
  ctk_application_impl_set_menubar (impl, ctk_application_get_menubar (impl->application));

  /* OK.  Now put it in the menu. */
  ctk_application_impl_quartz_setup_menu (G_MENU_MODEL (quartz->combined), quartz->muxer);

  [NSApp finishLaunching];
}

static void
ctk_application_impl_quartz_shutdown (CtkApplicationImpl *impl)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;

  /* destroy our custom menubar */
  [NSApp setMainMenu:[[[NSMenu alloc] init] autorelease]];

  if (quartz->delegate)
    {
      [quartz->delegate release];
      quartz->delegate = NULL;
    }

  g_slist_free_full (quartz->inhibitors, (GDestroyNotify) ctk_application_quartz_inhibitor_free);
  quartz->inhibitors = NULL;
}

static void
ctk_application_impl_quartz_active_window_changed (CtkApplicationImpl *impl,
                                                   CtkWindow          *window)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;

  ctk_action_muxer_remove (quartz->muxer, "win");

  if (G_IS_ACTION_GROUP (window))
    ctk_action_muxer_insert (quartz->muxer, "win", G_ACTION_GROUP (window));
}

static void
ctk_application_impl_quartz_set_app_menu (CtkApplicationImpl *impl,
                                          GMenuModel         *app_menu)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;

  /* If there are any items at all, then the first one is the app menu */
  if (g_menu_model_get_n_items (G_MENU_MODEL (quartz->combined)))
    g_menu_remove (quartz->combined, 0);

  if (app_menu)
    g_menu_prepend_submenu (quartz->combined, "Application", app_menu);
  else
    {
      GMenu *empty;

      /* We must preserve the rule that index 0 is the app menu */
      empty = g_menu_new ();
      g_menu_prepend_submenu (quartz->combined, "Application", G_MENU_MODEL (empty));
      g_object_unref (empty);
    }
}

static void
ctk_application_impl_quartz_set_menubar (CtkApplicationImpl *impl,
                                         GMenuModel         *menubar)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;

  /* If we have the menubar, it is a section at index '1' */
  if (g_menu_model_get_n_items (G_MENU_MODEL (quartz->combined)) > 1)
    g_menu_remove (quartz->combined, 1);

  if (menubar)
    g_menu_append_section (quartz->combined, NULL, menubar);
}

static guint
ctk_application_impl_quartz_inhibit (CtkApplicationImpl         *impl,
                                     CtkWindow                  *window,
                                     CtkApplicationInhibitFlags  flags,
                                     const gchar                *reason)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;
  CtkApplicationQuartzInhibitor *inhibitor;

  inhibitor = g_slice_new (CtkApplicationQuartzInhibitor);
  inhibitor->cookie = ++quartz->next_cookie;
  inhibitor->flags = flags;
  inhibitor->reason = g_strdup (reason);
  inhibitor->window = window ? g_object_ref (window) : NULL;

  quartz->inhibitors = g_slist_prepend (quartz->inhibitors, inhibitor);

  if (flags & CTK_APPLICATION_INHIBIT_LOGOUT)
    quartz->quit_inhibit++;

  return inhibitor->cookie;
}

static void
ctk_application_impl_quartz_uninhibit (CtkApplicationImpl *impl,
                                       guint               cookie)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;
  GSList *iter;

  for (iter = quartz->inhibitors; iter; iter = iter->next)
    {
      CtkApplicationQuartzInhibitor *inhibitor = iter->data;

      if (inhibitor->cookie == cookie)
        {
          if (inhibitor->flags & CTK_APPLICATION_INHIBIT_LOGOUT)
            quartz->quit_inhibit--;
          ctk_application_quartz_inhibitor_free (inhibitor);
          quartz->inhibitors = g_slist_delete_link (quartz->inhibitors, iter);
          return;
        }
    }

  g_warning ("Invalid inhibitor cookie");
}

static gboolean
ctk_application_impl_quartz_is_inhibited (CtkApplicationImpl         *impl,
                                          CtkApplicationInhibitFlags  flags)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) impl;

  if (flags & CTK_APPLICATION_INHIBIT_LOGOUT)
    return quartz->quit_inhibit > 0;

  return FALSE;
}

static void
ctk_application_impl_quartz_init (CtkApplicationImplQuartz *quartz)
{
  /* This is required so that Cocoa is not going to parse the
     command line arguments by itself and generate OpenFile events.
     We already parse the command line ourselves, so this is needed
     to prevent opening files twice, etc. */
  [[NSUserDefaults standardUserDefaults] setObject:@"NO"
                                            forKey:@"NSTreatUnknownArgumentsAsOpen"];

  quartz->combined = g_menu_new ();
}

static void
ctk_application_impl_quartz_finalize (GObject *object)
{
  CtkApplicationImplQuartz *quartz = (CtkApplicationImplQuartz *) object;

  g_clear_object (&quartz->combined);

  G_OBJECT_CLASS (ctk_application_impl_quartz_parent_class)->finalize (object);
}

static void
ctk_application_impl_quartz_class_init (CtkApplicationImplClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  class->startup = ctk_application_impl_quartz_startup;
  class->shutdown = ctk_application_impl_quartz_shutdown;
  class->active_window_changed = ctk_application_impl_quartz_active_window_changed;
  class->set_app_menu = ctk_application_impl_quartz_set_app_menu;
  class->set_menubar = ctk_application_impl_quartz_set_menubar;
  class->inhibit = ctk_application_impl_quartz_inhibit;
  class->uninhibit = ctk_application_impl_quartz_uninhibit;
  class->is_inhibited = ctk_application_impl_quartz_is_inhibited;

  gobject_class->finalize = ctk_application_impl_quartz_finalize;
}
