/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifdef JUCE_VIDEO_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#define JUCE_CORE_INCLUDE_OBJC_HELPERS 1
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1

#include "juce_video.h"

#if JUCE_MAC
  #import <AVFoundation/AVFoundation.h>

//==============================================================================
#elif JUCE_WINDOWS
 #if JUCE_QUICKTIME
  /* If you've got an include error here, you probably need to install the QuickTime SDK and
     add its header directory to your include path.

     Alternatively, if you don't need any QuickTime services, just set the JUCE_QUICKTIME flag to 0.
  */
  #include <Movies.h>
  #include <QTML.h>
  #include <QuickTimeComponents.h>
  #include <MediaHandlers.h>
  #include <ImageCodec.h>

  /* If you've got QuickTime 7 installed, then these COM objects should be found in
     the "\Program Files\Quicktime" directory. You'll need to add this directory to
     your include search path to make these import statements work.
  */
  #import <QTOLibrary.dll>
  #import <QTOControl.dll>

  #if JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
   #pragma comment (lib, "QTMLClient.lib")
  #endif
 #endif

 #if JUCE_USE_CAMERA || JUCE_DIRECTSHOW
  /* If you're using the camera classes, you'll need access to a few DirectShow headers.
     These files are provided in the normal Windows SDK. */
  #include <dshow.h>
  #include <dshowasf.h>
 #endif

 #if JUCE_DIRECTSHOW && JUCE_MEDIAFOUNDATION
  #include <evr.h>
 #endif

 #if JUCE_USE_CAMERA && JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
  #pragma comment (lib, "Strmiids.lib")
  #pragma comment (lib, "wmvcore.lib")
 #endif

 #if JUCE_MEDIAFOUNDATION && JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
  #pragma comment (lib, "mfuuid.lib")
 #endif

 #if JUCE_DIRECTSHOW && JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
  #pragma comment (lib, "strmiids.lib")
 #endif
#endif

//==============================================================================
using namespace juce;

namespace juce
{

#if JUCE_MAC || JUCE_IOS
 #if JUCE_USE_CAMERA
  #include "native/juce_mac_CameraDevice.mm"
 #endif

 #if JUCE_MAC
  #include "native/juce_mac_MovieComponent.mm"
 #endif

#elif JUCE_WINDOWS

 #if JUCE_USE_CAMERA
  #include "native/juce_win32_CameraDevice.cpp"
 #endif

 #if JUCE_DIRECTSHOW
  #include "native/juce_win32_DirectShowComponent.cpp"
 #endif

#elif JUCE_LINUX

#elif JUCE_ANDROID
 #if JUCE_USE_CAMERA
  #include "native/juce_android_CameraDevice.cpp"
 #endif
#endif

#if JUCE_USE_CAMERA
 #include "capture/juce_CameraDevice.cpp"
#endif

}
