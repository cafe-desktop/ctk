Please do not compile this package (CTK+) in paths that contain
spaces in them-as strange problems may occur during compilation or during
the use of the library.

A more detailed outline for instructions on building the CTK+ with Visual
C++ can be found in the following GNOME Live! page:

https://wiki.gnome.org/Projects/CTK+/Win32/MSVCCompilationOfCTKStack

This VS10 solution and the projects it includes are intented to be used
in a CTK+ source tree unpacked from a tarball. In a git checkout you
first need to use some Unix-like environment or manual work to expand
the files needed, like config.h.win32.in into config.h.win32 and the
.vcxprojin and .vcxproj.filtersin files here into corresponding actual
.vcxproj and vcxproj.filters files.

You will need the parts from below in the CTK+ stack: GDK-Pixbuf, Pango,
ATK and GLib.  External dependencies are at least Cairo
(with Cairo-GObject support, meaning Cairo 1.10.x or later), zlib, libpng,
gettext-runtime, fontconfig*, freetype*, expat*.  See the 
build/win32/vs10/README.txt file in glib for details where to unpack them.

You will also need a Python 2.6+/3.x interpretor installed on your system,
which can be obtained from the official installers available from
http://www.python.org.  Please note that the Python interpretor (python.exe)
either needs to be in your PATH before attempting the build of CTK+, or it
can be found in the path specified by PythonDir in ctk-version-paths.props.
If you happen to change the PythonDir setting in ctk-version-paths.props after
opening ctk+.sln with Visual Studio, you will need to close the ctk+.sln solution,
delete all the *.sdf, *.suo and *.user files before re-attempting the build.

It is recommended that one builds the dependencies with VS10 as far as
possible, especially those from and using the CTK+ stack (i.e. GLib,
Cairo, ATK, Pango, GDK-Pixbuf), so that crashes caused by mixing calls
to different CRTs can be kept at a minimum.

zlib, libpng, and Cairo do contain support for compiling under VS10
using VS project files and/or makefiles at this time of writing, For the
CTK+ stack, VS10 project files are either available under
$(srcroot)/build/vs10 in the case of GLib (stable/unstable), ATK
(stable/unstable) and GDK-Pixbuf (unstable), and should be in the next
unstable version of Pango.  There is no known official VS10 build
support for fontconfig (along with freetype and expat) and
gettext-runtime, so please use the binaries from: 

ftp://ftp.gnome.org/pub/GNOME/binaries/win32/dependencies/ (32 bit)
ftp://ftp.gnome.org/pub/GNOME/binaries/win64/dependencies/ (64 bit)

Unzip the binaries obtained from ftp.gnome.org in <root>\vs10\<PlatformName>,
and build the following, if not already done so: 

Note: put the resulting zlib, libpng, pcre and Cairo files as follows:
 .dll files: <root>\vs10\<PlatformName>\bin
 .lib files: <root>\vs10\<PlatformName>\lib
 .h files: <root>\vs10\<PlatformName>\include

The recommended build order for these dependencies:
(first unzip any dependent binaries downloaded from the ftp.gnome.org
 as described in the README.txt file in the build/win32/vs10 folder)
-zlib
-libpng
-(for GDK-Pixbuf, if not using GDI+) IJG JPEG or libjpeg-turbo
-(for GDK-Pixbuf, if not using GDI+) libtiff
 [libtiff requires zlib and IJG JPEG or libjpeg-turbo]
-(for GDK-Pixbuf, if not using GDI+) jasper [jpeg-2000 library]
-(optional for GLib) PCRE (version 8.12 or later, use of CMake to
  build PCRE is recommended-see build/win32/vs10/README.txt of GLib)
-GLib **
-Cairo (inclusive of Cairo-GObject)
-ATK**
-Pango**
-GDK-Pixbuf**
(note the last 3 dependencies are not interdependent, so the last 3
 dependencies can be built in any order)

The "install" project will copy build results and headers into their
appropriate location under <root>\vs10\<PlatformName>. For instance,
built DLLs go into <root>\vs10\<PlatformName>\bin, built LIBs into
<root>\vs10\<PlatformName>\lib and CTK+ headers into
<root>\vs10\<PlatformName>\include\ctk-3.0. This is then from where
project files higher in the stack are supposed to look for them, not
from a specific GLib source tree.

There is now a "ctk3-introspect" project that is used to build the
introspection files.  In order for this to work, check that the paths for
PythonDir (32-bit builds) and PythonDirX64 (x64 builds) are correct for your
system.  Note that it must be the same Python installation that was used to
build GObject-Introspection (G-I), and a complete G-I build/installation
needs to be found in <root>\vs10\<PlatformName>\, with the introspection files
for ATK, Pango and GDK-Pixbuf.  Note also that this is not built by default,
so you will need to right-click on the project to build it, which will build
and "install" the other projects that are normally built, if those were not yet
built.  The introspection files that are built will be "installed" to
<root>\vs10\<PlatformName>\share\gir-1.0 (the .gir file(s)) and
<root>\vs10\<PlatformName>\lib\girepository-1.0 (the .typelib files(s)) upon
successful build.  for building this in a different configuration, therefore,
you will need to clean this project specifically and then rebuild.

Please note, as CTK+ uses the Advaita theme for all platforms by default,
most icons used are not included with CTK+ (which *are* needed), so please see
https://live.gnome.org/CTK%2B/Win32/MSVCCompilationOfCTKStack (under the
CTK+ section) on how to get those icons set up for use with CTK+.

*About the dependencies marked with *: These dependencies are optional
 as those are not compulsory components for building and running CTK+
 itself, but note that they are needed for people running and building
 GIMP or those who need complex script support via fontconfig.  They
 are referred to by components in Cairo and Pango mainly.
 Decide whether you need fontconfig support prior to building Cairo
 and Pango.

**:Put the sources of the packages marked with ** in <root>\<package-
    source-tree>, and build with VS10 from there.

--Tor Lillqvist <tml@iki.fi>
--Updated by Chun-wei Fan <fanc999@yahoo.com.tw>
