/* cdkglobals-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "config.h"
#include "cdktypes.h"
#include "cdkprivate.h"
#include "cdkquartz.h"
#include "cdkinternal-quartz.h"

CdkDisplay *_cdk_display = NULL;
CdkScreen *_cdk_screen = NULL;
CdkWindow *_cdk_root = NULL;

CdkOSXVersion
cdk_quartz_osx_version (void)
{
  static gint32 minor = CDK_OSX_UNSUPPORTED;

  if (minor == CDK_OSX_UNSUPPORTED)
    {
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101000
      OSErr err = Gestalt (gestaltSystemVersionMinor, (SInt32*)&minor);

      g_return_val_if_fail (err == noErr, CDK_OSX_UNSUPPORTED);
#else
      NSOperatingSystemVersion version;

      version = [[NSProcessInfo processInfo] operatingSystemVersion];
      minor = version.minorVersion;
      if (version.majorVersion == 11)
        minor += 16;
#endif
    }

  if (minor < CDK_OSX_MIN)
    return CDK_OSX_UNSUPPORTED;
  else if (minor > CDK_OSX_CURRENT)
    return CDK_OSX_NEW;
  else
    return minor;
}
