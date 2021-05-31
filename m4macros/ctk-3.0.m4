# Configure paths for GTK+
# Owen Taylor     1997-2001

# Version number used by aclocal, see `info automake Serials`.
# Increment on every change.
#serial 1

dnl AM_PATH_CTK_3_0([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GTK+, and define CTK_CFLAGS and CTK_LIBS, if gthread is specified in MODULES, 
dnl pass to pkg-config
dnl
AC_DEFUN([AM_PATH_CTK_3_0],
[m4_warn([obsolete], [AM_PATH_CTK_3_0 is deprecated, use PKG_CHECK_MODULES([GTK], [ctk+-3.0]) instead])
dnl Get the cflags and libraries from pkg-config
dnl
AC_ARG_ENABLE(ctktest, [  --disable-ctktest       do not try to compile and run a test GTK+ program],
		    , enable_ctktest=yes)
  min_ctk_version=ifelse([$1], [], [3.0.0], [$1])

  pkg_config_args="ctk+-3.0 >= $min_ctk_version"
  for module in . $4
  do
      case "$module" in
         gthread)
             pkg_config_args="$pkg_config_args gthread-2.0"
         ;;
      esac
  done

  no_ctk=""

  PKG_PROG_PKG_CONFIG([0.16])

  if test -z "$PKG_CONFIG"; then
    no_ctk=yes
  fi

  AC_MSG_CHECKING(for GTK+ - version >= $min_ctk_version)

  if test -n "$PKG_CONFIG"; then
    ## don't try to run the test against uninstalled libtool libs
    if $PKG_CONFIG --uninstalled $pkg_config_args; then
	  echo "Will use uninstalled version of GTK+ found in PKG_CONFIG_PATH"
	  enable_ctktest=no
    fi

    if $PKG_CONFIG $pkg_config_args; then
	  :
    else
	  no_ctk=yes
    fi
  fi

  if test x"$no_ctk" = x ; then
    CTK_CFLAGS=`$PKG_CONFIG $pkg_config_args --cflags`
    CTK_LIBS=`$PKG_CONFIG $pkg_config_args --libs`
    ctk_config_major_version=`$PKG_CONFIG --modversion ctk+-3.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    ctk_config_minor_version=`$PKG_CONFIG --modversion ctk+-3.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    ctk_config_micro_version=`$PKG_CONFIG --modversion ctk+-3.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_ctktest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $CTK_CFLAGS"
      LIBS="$CTK_LIBS $LIBS"
dnl
dnl Now check if the installed GTK+ is sufficiently new. (Also sanity
dnl checks the results of pkg-config to some extent)
dnl
      rm -f conf.ctktest
      AC_TRY_RUN([
#include <ctk/ctk.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  unsigned int major, minor, micro;

  fclose (fopen ("conf.ctktest", "w"));

  if (sscanf("$min_ctk_version", "%u.%u.%u", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_ctk_version");
     exit(1);
   }

  if ((ctk_major_version != $ctk_config_major_version) ||
      (ctk_minor_version != $ctk_config_minor_version) ||
      (ctk_micro_version != $ctk_config_micro_version))
    {
      printf("\n*** 'pkg-config --modversion ctk+-3.0' returned %d.%d.%d, but GTK+ (%d.%d.%d)\n", 
             $ctk_config_major_version, $ctk_config_minor_version, $ctk_config_micro_version,
             ctk_major_version, ctk_minor_version, ctk_micro_version);
      printf ("*** was found! If pkg-config was correct, then it is best\n");
      printf ("*** to remove the old version of GTK+. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If pkg-config was wrong, set the environment variable PKG_CONFIG_PATH\n");
      printf("*** to point to the correct configuration files\n");
    } 
  else if ((ctk_major_version != CTK_MAJOR_VERSION) ||
	   (ctk_minor_version != CTK_MINOR_VERSION) ||
           (ctk_micro_version != CTK_MICRO_VERSION))
    {
      printf("*** GTK+ header files (version %d.%d.%d) do not match\n",
	     CTK_MAJOR_VERSION, CTK_MINOR_VERSION, CTK_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     ctk_major_version, ctk_minor_version, ctk_micro_version);
    }
  else
    {
      if ((ctk_major_version > major) ||
        ((ctk_major_version == major) && (ctk_minor_version > minor)) ||
        ((ctk_major_version == major) && (ctk_minor_version == minor) && (ctk_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GTK+ (%u.%u.%u) was found.\n",
               ctk_major_version, ctk_minor_version, ctk_micro_version);
        printf("*** You need a version of GTK+ newer than %u.%u.%u. The latest version of\n",
	       major, minor, micro);
        printf("*** GTK+ is always available from ftp://ftp.ctk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the pkg-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GTK+, but you can also set the PKG_CONFIG environment to point to the\n");
        printf("*** correct copy of pkg-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_ctk=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_ctk" = x ; then
     AC_MSG_RESULT(yes (version $ctk_config_major_version.$ctk_config_minor_version.$ctk_config_micro_version))
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test -z "$PKG_CONFIG"; then
       echo "*** A new enough version of pkg-config was not found."
       echo "*** See http://pkgconfig.sourceforge.net"
     else
       if test -f conf.ctktest ; then
        :
       else
          echo "*** Could not run GTK+ test program, checking why..."
	  ac_save_CFLAGS="$CFLAGS"
	  ac_save_LIBS="$LIBS"
          CFLAGS="$CFLAGS $CTK_CFLAGS"
          LIBS="$LIBS $CTK_LIBS"
          AC_TRY_LINK([
#include <ctk/ctk.h>
#include <stdio.h>
],      [ return ((ctk_major_version) || (ctk_minor_version) || (ctk_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GTK+ or finding the wrong"
          echo "*** version of GTK+. If it is not finding GTK+, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occurred. This usually means GTK+ is incorrectly installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     CTK_CFLAGS=""
     CTK_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(CTK_CFLAGS)
  AC_SUBST(CTK_LIBS)
  rm -f conf.ctktest
])

dnl CTK_CHECK_BACKEND(BACKEND-NAME [, MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl   Tests for BACKEND-NAME in the GTK targets list
dnl
AC_DEFUN([CTK_CHECK_BACKEND],
[m4_warn([obsolete], [CTK_CHECK_BACKEND is deprecated, use PKG_CHECK_MODULES([CTK_X11], [ctk+-x11-3.0]) or similar instead])
  pkg_config_args=ifelse([$1],,ctk+-3.0, ctk+-$1-3.0)
  min_ctk_version=ifelse([$2],,3.0.0,$2)
  pkg_config_args="$pkg_config_args >= $min_ctk_version"

  PKG_PROG_PKG_CONFIG([0.16])
  AS_IF([test -z "$PKG_CONFIG"], [AC_MSG_ERROR([No pkg-config found])])

  if $PKG_CONFIG $pkg_config_args ; then
    target_found=yes
  else
    target_found=no
  fi

  if test "x$target_found" = "xno"; then
    ifelse([$4],,[AC_MSG_ERROR([Backend $backend not found.])],[$4])
  else
    ifelse([$3],,[:],[$3])
  fi
])
