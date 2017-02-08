/***********************************************************************
    created:    Mon Jan 10 2011
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
//////////////////////////////////////////////////////////////////////////
/*************************************************************************

This file is automatically generated by CMake and should be changed by
setting CMake options and regenerating, rather than editing directly.

*************************************************************************/
//////////////////////////////////////////////////////////////////////////
#ifndef _CEGUIConfig_h_
#define _CEGUIConfig_h_

// CMake defines NDEBUG on all platforms, but not always DEBUG :-/
#if !defined(NDEBUG) && !defined(DEBUG)
#   define DEBUG 1
#endif

#define CEGUI_HAS_BUILD_SUFFIX
#ifdef CEGUI_HAS_BUILD_SUFFIX
#   ifndef CEGUI_BUILD_SUFFIX
#       if defined(DEBUG) || defined(_DEBUG)
#           define CEGUI_BUILD_SUFFIX "_d"
#       else
#           define CEGUI_BUILD_SUFFIX ""
#       endif
#   endif
#endif

//////////////////////////////////////////////////////////////////////////
// CEGUI supports custom memory allocation (optional)
//////////////////////////////////////////////////////////////////////////
// if this is defined, allocation can be customised
/* #undef CEGUI_CUSTOM_ALLOCATORS */
// if this is defined, allocations will pass debug info to allocators
/* #undef CEGUI_CUSTOM_ALLOCATORS_DEBUG */
// the file that contains allocators and allocation config
// (not used if CEGUI_CUSTOM_ALLOCATORS isn't defined)
#define CEGUI_CUSTOM_ALLOCATORS_INCLUDE "CEGUI/MemoryStdAllocator.h"

//////////////////////////////////////////////////////////////////////////
// CEGUI allows 3 different string class configurations
//////////////////////////////////////////////////////////////////////////
// Inbuilt Unicode (utf8 and utf32 aware)
#define CEGUI_STRING_CLASS_UNICODE 1
// plain std::string without allocators (makes pass by reference easy)
#define CEGUI_STRING_CLASS_STD 2
// std::basic_string allocated according to allocator config
#define CEGUI_STRING_CLASS_STD_AO 3

#define CEGUI_STRING_CLASS 1

//////////////////////////////////////////////////////////////////////////
// Set this to the default XMLParser to be used.
//////////////////////////////////////////////////////////////////////////
#ifndef CEGUI_DEFAULT_XMLPARSER
#	define CEGUI_DEFAULT_XMLPARSER ExpatParser
#endif

//////////////////////////////////////////////////////////////////////////
// Set this to the default ImageCodec to be used.
//////////////////////////////////////////////////////////////////////////
#ifndef CEGUI_DEFAULT_IMAGE_CODEC
#   define CEGUI_DEFAULT_IMAGE_CODEC SILLYImageCodec
#endif

//////////////////////////////////////////////////////////////////////////
// The following says which TinyXML api version has been found
//////////////////////////////////////////////////////////////////////////
/* #undef CEGUI_TINYXML_HAS_2_6_API */

//////////////////////////////////////////////////////////////////////////
// The following controls the version of Lua that is going to be used.
// Note that from 0.7.0 and up, Lua 5.0 is no longer supported.
// 51 is for Lua 5.1.x versions (and above?)
//////////////////////////////////////////////////////////////////////////
#ifndef CEGUI_LUA_VER
#   define CEGUI_LUA_VER 51
#endif

#ifndef CEGUI_IRR_SDK_VERSION
#   define CEGUI_IRR_SDK_VERSION 16
#endif

//////////////////////////////////////////////////////////////////////////
// Defines for Ogre version we were built against
// (Will be all zeroes if Ogre was not found)
//////////////////////////////////////////////////////////////////////////
#ifndef CEGUI_OGRE_VERSION_MAJOR
#   define CEGUI_OGRE_VERSION_MAJOR 0
#endif
#ifndef CEGUI_OGRE_VERSION_MINOR
#   define CEGUI_OGRE_VERSION_MINOR 0
#endif
#ifndef CEGUI_OGRE_VERSION_PATCH
#   define CEGUI_OGRE_VERSION_PATCH 0
#endif
#ifndef CEGUI_OGRE_VERSION
#   define CEGUI_OGRE_VERSION ((CEGUI_OGRE_VERSION_MAJOR << 16) | (CEGUI_OGRE_VERSION_MINOR << 8) | CEGUI_OGRE_VERSION_PATCH)
#endif


//////////////////////////////////////////////////////////////////////////
// The following controls whether freetype based font support will be
// compiled in (default).  If you want to build CEGUI /without/ freetype
// support, comment the following line.
//////////////////////////////////////////////////////////////////////////
#define CEGUI_HAS_FREETYPE

//////////////////////////////////////////////////////////////////////////
// PCRE library
// CEGUI uses the pcre library for it's regular expression based string
// validation as used in the Editbox (and derived classes, such as Spinner).
// To disable the use of PCRE (and therefore the validation factilities),
// comment the following line.  (Attempts to set validation string will throw).
//////////////////////////////////////////////////////////////////////////
#define CEGUI_HAS_PCRE_REGEX

//////////////////////////////////////////////////////////////////////////
// The following controls whether the default logger be (possibly) used.
// If you want to build CEGUI so that it does not use the default logger,
// comment the following line.
//
// Note: If you disable the default logger you MUST provide an alternative
// CEGUI::Logger based class and instantiate it before creating the main
// CEGUI::System object.
//////////////////////////////////////////////////////////////////////////
#define CEGUI_HAS_DEFAULT_LOGGER

//////////////////////////////////////////////////////////////////////////
// The following defines control bidirectional text support.
//
// Uncomment the CEGUI_BIDI_SUPPORT definition to enable bidirectional
// text in CEGUI.
//
// With bidirectional text support enabled, you must then choose to uncomment
// either the CEGUI_USE_MINIBIDI or CEGUI_USE_FRIBIDI definitions (but you
// should not uncomment both!)
//
// Uncommenting CEGUI_USE_MINIBIDI uses an integrated copy of minibidi to
// provide the bidirectional support.
//
// Uncommenting CEGUI_USE_FRIBIDI uses an external copy of fribidi to
// provide the bidirectional support (not supplied).
//////////////////////////////////////////////////////////////////////////
/* #undef CEGUI_BIDI_SUPPORT */

// Uncomment this to use the embedded minibidi library.
/* #undef CEGUI_USE_MINIBIDI */
// - or -
// Uncomment this to use an external fribidi library.
/* #undef CEGUI_USE_FRIBIDI */

// Controls whether to use GLEW as an OpenGL loading library.
#define CEGUI_USE_GLEW

// Controls whether to use Epoxy as an OpenGL loading library.
/* #undef CEGUI_USE_EPOXY */

//////////////////////////////////////////////////////////////////////////
// The following controls whether the MinizipResourceProvider will be
// built into the CEGUIBase library.  You can uncomment the following line
// to include this code (and it's dependency code).
// 
// The MinizipResourceProvider provides the ability to load resource files
// from locations within .zip files.
//////////////////////////////////////////////////////////////////////////
/* #undef CEGUI_HAS_MINIZIP_RESOURCE_PROVIDER */

// Define the default place where cegui will look for loadable modules
// this can be changed at runtime via the CEGUI_MODULE_DIR environment var.
#if defined(_WIN32) || defined(__WIN32__)
#   define CEGUI_MODULE_DIR "E:/GUITest/cegui-0.8.7/build/bin/"
#elif !defined(__ANDROID__)
#   define CEGUI_MODULE_DIR "C:/Program Files (x86)/cegui/lib/cegui-0.8/"
#endif

// This is defined when iconv is expecting a const char** and not a char**
// as type for its inbuf parameter. This is only used when building cegui
// itself, and likely will not be useful externally.
/* #undef CEGUI_ICONV_USES_CONST_INBUF */

#endif	// end of guard _CEGUIConfig_h_

