//
// PDObjectStream.h
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
 @file PDObjectStream.h PDF object stream header file.
 
 @ingroup PDOBJECTSTREAM
 
 @defgroup PDOBJECTSTREAM PDObjectStream
 
 @brief A PDF object stream, i.e. a stream of PDF objects inside a stream.
 
 @ingroup PDUSER
 
 Normally, objects are located directly inside of the PDF, but an alternative way is to keep objects as so called object streams (Chapter 3.4.6 of PDF specification v 1.7, p. 100). 
 
 When a filtering task is made for an object that is determined to be located inside of an object stream, supplementary tasks are automatically set up to generate the object stream instance of the container object and to present the given object as a regular, mutable instance to the requesting task. Upon completion (that is, when returning from the task callback), the object stream is "committed" as stream content to the actual containing object, which in turn is written to the output as normal.
 
 @{
 */

#ifndef ICViewer_PDObjectStream_h
#define ICViewer_PDObjectStream_h

#include "PDDefines.h"

/**
 Get the object with the given ID out of the object stream. 
 
 Object is mutable with regular object mutability conditions applied. 
 
 @param obstm The object stream.
 @param obid The id of the object to fetch.
 @return The object, or NULL if not found.
 */
extern PDObjectRef PDObjectStreamGetObjectByID(PDObjectStreamRef obstm, PDInteger obid);

/**
 Get the object at the given index out of the object stream.
 
 Assertion is thrown if index is out of bounds.
 
 The index of an object stream object can be determined through the /N dictionary entry in the object itself.
 
 @param obstm The object stream.
 @param index Object stream index.
 @return The object.
 */
extern PDObjectRef PDObjectStreamGetObjectAtIndex(PDObjectStreamRef obstm, PDInteger index);

#endif

/** @} */

/** @} */
