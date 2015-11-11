/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#ifndef JUCE_OPENGL_H_INCLUDED
#define JUCE_OPENGL_H_INCLUDED

#include "../juce_gui_extra/juce_gui_extra.h"

#undef JUCE_OPENGL
#define JUCE_OPENGL 1

#if JUCE_IOS || JUCE_ANDROID
 #define JUCE_OPENGL_ES 1
#endif

#if ! JUCE_ANDROID
 #define JUCE_OPENGL_CREATE_JUCE_RENDER_THREAD 1
#endif

#if JUCE_WINDOWS
 #ifndef APIENTRY
  #define APIENTRY __stdcall
  #define CLEAR_TEMP_APIENTRY 1
 #endif
 #ifndef WINGDIAPI
  #define WINGDIAPI __declspec(dllimport)
  #define CLEAR_TEMP_WINGDIAPI 1
 #endif
 #include <gl/GL.h>
 #ifdef CLEAR_TEMP_WINGDIAPI
  #undef WINGDIAPI
  #undef CLEAR_TEMP_WINGDIAPI
 #endif
 #ifdef CLEAR_TEMP_APIENTRY
  #undef APIENTRY
  #undef CLEAR_TEMP_APIENTRY
 #endif
#elif JUCE_LINUX
 #include <GL/gl.h>
 #undef KeyPress
#elif JUCE_IOS
 #if defined (__IPHONE_7_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_7_0
  #include <OpenGLES/ES3/gl.h>
 #else
  #include <OpenGLES/ES2/gl.h>
 #endif
#elif JUCE_MAC
 #if defined (MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7)
  #define JUCE_OPENGL3 1
  #include <OpenGL/gl3.h>
  #include <OpenGL/gl3ext.h>
 #else
  #include <OpenGL/gl.h>
  #include <OpenGL/glext.h>
 #endif
#elif JUCE_ANDROID
 #include <GLES2/gl2.h>
#endif

#if GL_ES_VERSION_3_0
 #define JUCE_OPENGL3 1
#endif

//=============================================================================
/** This macro is a helper for use in GLSL shader code which needs to compile on both OpenGL 2.1 and OpenGL 3.0.
    It's mandatory in OpenGL 3.0 to specify the GLSL version.
*/
#if JUCE_OPENGL3
 #if JUCE_OPENGL_ES
  #define JUCE_GLSL_VERSION "#version 300 es"
 #else
  #define JUCE_GLSL_VERSION "#version 150"
 #endif
#else
 #define JUCE_GLSL_VERSION ""
#endif

//=============================================================================
#if JUCE_OPENGL_ES || defined (DOXYGEN)
 /** This macro is a helper for use in GLSL shader code which needs to compile on both GLES and desktop GL.
     Since it's mandatory in GLES to mark a variable with a precision, but the keywords don't exist in normal GLSL,
     these macros define the various precision keywords only on GLES.
 */
 #define JUCE_MEDIUMP  "mediump"

 /** This macro is a helper for use in GLSL shader code which needs to compile on both GLES and desktop GL.
     Since it's mandatory in GLES to mark a variable with a precision, but the keywords don't exist in normal GLSL,
     these macros define the various precision keywords only on GLES.
 */
 #define JUCE_HIGHP    "highp"

 /** This macro is a helper for use in GLSL shader code which needs to compile on both GLES and desktop GL.
     Since it's mandatory in GLES to mark a variable with a precision, but the keywords don't exist in normal GLSL,
     these macros define the various precision keywords only on GLES.
 */
 #define JUCE_LOWP     "lowp"
#else
 #define JUCE_MEDIUMP
 #define JUCE_HIGHP
 #define JUCE_LOWP
#endif

//=============================================================================
namespace juce
{

class OpenGLTexture;
class OpenGLFrameBuffer;
class OpenGLShaderProgram;

#include "geometry/juce_Quaternion.h"
#include "geometry/juce_Matrix3D.h"
#include "geometry/juce_Vector3D.h"
#include "geometry/juce_Draggable3DOrientation.h"
#include "native/juce_MissingGLDefinitions.h"
#include "opengl/juce_OpenGLHelpers.h"
#include "opengl/juce_OpenGLPixelFormat.h"
#include "native/juce_OpenGLExtensions.h"
#include "opengl/juce_OpenGLRenderer.h"
#include "opengl/juce_OpenGLContext.h"
#include "opengl/juce_OpenGLFrameBuffer.h"
#include "opengl/juce_OpenGLGraphicsContext.h"
#include "opengl/juce_OpenGLHelpers.h"
#include "opengl/juce_OpenGLImage.h"
#include "opengl/juce_OpenGLRenderer.h"
#include "opengl/juce_OpenGLShaderProgram.h"
#include "opengl/juce_OpenGLTexture.h"
#include "utils/juce_OpenGLAppComponent.h"

}

#endif   // JUCE_OPENGL_H_INCLUDED
