/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 2002 Owen Taylor
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

/*
 * A keybinding set implementing Emacs-like keybindings
 */

/*
 * Bindings for CtkTextView and CtkEntry
 */
@binding-set ctk-emacs-text-entry
{
  bind "<ctrl>b" { "move-cursor" (logical-positions, -1, 0) };
  bind "<shift><ctrl>b" { "move-cursor" (logical-positions, -1, 1) };
  bind "<ctrl>f" { "move-cursor" (logical-positions, 1, 0) };
  bind "<shift><ctrl>f" { "move-cursor" (logical-positions, 1, 1) };

  bind "<alt>b" { "move-cursor" (words, -1, 0) };
  bind "<shift><alt>b" { "move-cursor" (words, -1, 1) };
  bind "<alt>f" { "move-cursor" (words, 1, 0) };
  bind "<shift><alt>f" { "move-cursor" (words, 1, 1) };

  bind "<ctrl>a" { "move-cursor" (paragraph-ends, -1, 0) };
  bind "<shift><ctrl>a" { "move-cursor" (paragraph-ends, -1, 1) };
  bind "<ctrl>e" { "move-cursor" (paragraph-ends, 1, 0) };
  bind "<shift><ctrl>e" { "move-cursor" (paragraph-ends, 1, 1) };

  bind "<ctrl>w" { "cut-clipboard" () };
  bind "<ctrl>y" { "paste-clipboard" () };

  bind "<ctrl>d" { "delete-from-cursor" (chars, 1) };
  bind "<alt>d" { "delete-from-cursor" (word-ends, 1) };
  bind "<ctrl>k" { "delete-from-cursor" (paragraph-ends, 1) };
  bind "<alt>backslash" { "delete-from-cursor" (whitespace, 1) };

  bind "<alt>space" { "delete-from-cursor" (whitespace, 1)
                      "insert-at-cursor" (" ") };
  bind "<alt>KP_Space" { "delete-from-cursor" (whitespace, 1)
                         "insert-at-cursor" (" ")  };
  /*
   * Some non-Emacs keybindings people are attached to
   */
  bind "<ctrl>u" { "move-cursor" (paragraph-ends, -1, 0)
                   "delete-from-cursor" (paragraph-ends, 1) };

  bind "<ctrl>h" { "delete-from-cursor" (chars, -1) };
  bind "<ctrl>w" { "delete-from-cursor" (word-ends, -1) };
}

/*
 * Bindings for CtkTextView
 */
@binding-set ctk-emacs-text-view
{
  bind "<ctrl>p" { "move-cursor" (display-lines, -1, 0) };
  bind "<shift><ctrl>p" { "move-cursor" (display-lines, -1, 1) };
  bind "<ctrl>n" { "move-cursor" (display-lines, 1, 0) };
  bind "<shift><ctrl>n" { "move-cursor" (display-lines, 1, 1) };

  bind "<ctrl>space" { "set-anchor" () };
  bind "<ctrl>KP_Space" { "set-anchor" () };
}

/*
 * Bindings for CtkTreeView
 */
@binding-set ctk-emacs-tree-view
{
  bind "<ctrl>s" { "start-interactive-search" () };
  bind "<ctrl>f" { "move-cursor" (logical-positions, 1) };
  bind "<ctrl>b" { "move-cursor" (logical-positions, -1) };
}

/*
 * Bindings for menus
 */
@binding-set ctk-emacs-menu
{
  bind "<ctrl>n" { "move-current" (next) };
  bind "<ctrl>p" { "move-current" (prev) };
  bind "<ctrl>f" { "move-current" (child) };
  bind "<ctrl>b" { "move-current" (parent) };
}

entry {
  -ctk-key-bindings: ctk-emacs-text-entry;
}

textview {
  -ctk-key-bindings: ctk-emacs-text-entry, ctk-emacs-text-view;
}

treeview {
  -ctk-key-bindings: ctk-emacs-tree-view;
}

CtkMenuShell {
  -ctk-key-bindings: ctk-emacs-menu;
}
