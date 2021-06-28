/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include <stdio.h>
#include <unistd.h>
#include <ctk/ctk.h>
#include "config.h"

#ifdef USE_PTHREADS
#include <pthread.h>

static int nthreads = 0;
static pthread_mutex_t nthreads_mutex = PTHREAD_MUTEX_INITIALIZER;

void
close_cb (CtkWidget *w, gint *flag)
{
  *flag = 1;
}

gint
delete_cb (CtkWidget *w, CdkEvent *event, gint *flag)
{
  *flag = 1;
  return TRUE;
}

void *
counter (void *data)
{
  gchar *name = data;
  gint flag = 0;
  gint counter = 0;
  gchar buffer[32];
  
  CtkWidget *window;
  CtkWidget *vbox;
  CtkWidget *label;
  CtkWidget *button;

  cdk_threads_enter();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), name);
  ctk_widget_set_size_request (window, 100, 50);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, FALSE, 0);

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (delete_cb), &flag);

  ctk_container_add (CTK_CONTAINER (window), vbox);

  label = ctk_label_new ("0");
  ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, FALSE, 0);

  button = ctk_button_new_with_label ("Close");
  g_signal_connect (button, "clicked",
                    G_CALLBACK (close_cb), &flag);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  ctk_widget_show_all (window);

  /* Since flag is only checked or set inside the CTK lock,
   * we don't have to worry about locking it explicitly
   */
  while (!flag)
    {
      sprintf(buffer, "%d", counter);
      ctk_label_set_text (CTK_LABEL (label), buffer);
      cdk_threads_leave();
      counter++;
      /* Give someone else a chance to get the lock next time.
       * Only necessary because we don't do anything else while
       * releasing the lock.
       */
      sleep(0);
      
      cdk_threads_enter();
    }

  ctk_widget_destroy (window);

  pthread_mutex_lock (&nthreads_mutex);
  nthreads--;
  if (nthreads == 0)
    ctk_main_quit();
  pthread_mutex_unlock (&nthreads_mutex);

  cdk_threads_leave();

  return NULL;
}

#endif

int 
main (int argc, char **argv)
{
#ifdef USE_PTHREADS
  int i;

  if (!cdk_threads_init())
    {
      fprintf(stderr, "Could not initialize threads\n");
      exit(1);
    }

  ctk_init (&argc, &argv);

  pthread_mutex_lock (&nthreads_mutex);

  for (i=0; i<5; i++)
    {
      char buffer[5][10];
      pthread_t thread;
      
      sprintf(buffer[i], "Thread %i", i);
      if (pthread_create (&thread, NULL, counter, buffer[i]))
	{
	  fprintf(stderr, "Couldn't create thread\n");
	  exit(1);
	}
      nthreads++;
    }

  pthread_mutex_unlock (&nthreads_mutex);

  cdk_threads_enter();
  ctk_main();
  cdk_threads_leave();
  fprintf(stderr, "Done\n");
#else /* !USE_PTHREADS */
  fprintf (stderr, "CTK+ not compiled with threads support\n");
  exit (1);
#endif /* USE_PTHREADS */  

  return 0;
}
