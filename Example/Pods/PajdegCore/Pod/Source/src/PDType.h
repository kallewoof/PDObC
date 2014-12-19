//
// PDType.h
//
// Copyright (c) 2012 - 2014 Karl-Johan Alm (http://github.com/kallewoof)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/**
 @file PDType.h Pajdeg type header.
 
 @ingroup PDTYPE
 
 @defgroup PDTYPE PDType
 
 @brief The generic Pajdeg object type.
 
 PDType provides the release and retain functionality to Pajdeg objects. 
 
 @warning Not all types in Pajdeg *are* Pajdeg objects. An object whose type is camel case and begins with capital `PD` is always a Pajdeg object, and can be retained and released, whereas objects using underscore separation in all lower case are non-Pajdeg objects (used by the Pajdeg system), such as pd_btree. The exception to this is primitive objects, such as PDInteger, PDBool, etc., and PDType itself.
 
 ## Concept behind Release and Retain
 
 Releasing and retaining is a method for structuring memory management, and is used e.g. in Objective-C and other languages. The concept is fairly straightforward:
 
 - calling a function that contains the word "Create" means you own the resulting object and must at some point release (PDRelease()) or autorelease (PDAutorelease()) it
 - retaining an object via PDRetain() means you have to release it at some point
 - objects are only valid until the end of the function executing unless they are retained
 
 There are also instances where an object will become invalid in the middle of a function body, if it is released and has no pending autorelease calls.
 
 For example, the following code snippet shows how this works:
 
 @dontinclude examples/add-metadata.c
 
 @skip create
 @until argv
 
 The `PDPipe` object is retained because the function name has the word `Create` in it, so it must be released.
 
 @skip parser
 @until GetPars
 
 The `PDParser` is not retained, as we obtained it through a function whose name did not contain `Create` -- consequently, the `parser` variable must not be released as we don't own a reference to it.
 
 @skip brand
 @until Create
 
 The `meta` object must be released by us, as it has the word `Create`.
 
 @ingroup PDINTERNAL
 
 @{
 */
#ifndef ICViewer_PDType_h
#define ICViewer_PDType_h

#include "PDDefines.h"

/**
 Release a Pajdeg object. 
 
 If the caller was the last referrer, the object will be destroyed immediately.
 
 @param pajdegObject A previously retained or created `PD` object.
 */
#ifdef DEBUG_PD_RELEASES
#define PDRelease(ob) _PDReleaseDebug(__FILE__,__LINE__,ob)
extern void _PDReleaseDebug(const char *file, int lineNumber, void *pajdegObject);
extern void PDReleaseFunc(void *pajdegObject);
#else
extern void PDRelease(void *pajdegObject);
#define PDReleaseFunc PDRelease
#endif

/**
 Retain a Pajdeg object.
 
 @param pajdegObject A `PD` object.
 */
#ifdef DEBUG_PD_RELEASES
#define PDRetain(ob) _PDRetainDebug(__FILE__,__LINE__,ob)
extern void *_PDRetainDebug(const char *file, int lineNumber, void *pajdegObject);
#else
extern void *PDRetain(void *pajdegObject);
#endif

/**
 Autorelease a Pajdeg object.
 
 Even if the caller was the last referrer, the object will not be destroyed until the PDPipeExecute() iteration for the current object has ended.
 
 Thus, it is possible to return an object with a zero retain count by autoreleasing it.
 
 @param pajdegObject The object that should, at some point, be released once.
 */
#ifdef DEBUG_PD_RELEASES
#define PDAutorelease(ob) _PDAutoreleaseDebug(__FILE__,__LINE__,ob)
extern void *_PDAutoreleaseDebug(const char *file, int lineNumber, void *pajdegObject);
#else
extern void *PDAutorelease(void *pajdegObject);
#endif

/**
 *  Resolve the PDInstanceType of the given object.
 *
 *  @param pajdegObject The object whose type should be resolved
 *
 *  @return PDInstanceType for the given object
 */
extern PDInstanceType PDResolve(void *pajdegObject);

#if defined(DEBUG_PD_LEAKS)
extern void PDDebugBeginSession();
extern PDInteger PDDebugEndSession();
extern void PDFlagGlobalObject(void *ob);
#else
#   define PDDebugBeginSession() 
#   define PDDebugEndSession() 0
#   define PDFlagGlobalObject(ob) 
#endif

#endif

/** @} */
