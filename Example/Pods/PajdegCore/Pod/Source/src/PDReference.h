//
// PDReference.h
//
// Copyright (c) 2012 - 2015 Karl-Johan Alm (http://github.com/kallewoof)
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
 @file PDReference.h Object reference header file.
 
 @ingroup PDREFERENCE
 
 @defgroup PDREFERENCE PDReference
 
 @brief A PDF object reference.
 
 @ingroup PDUSER
 
 @{
 */

#ifndef INCLUDED_PDReference_h
#define INCLUDED_PDReference_h

#include "PDDefines.h"

/**
 Create a reference based on a stack. 
 
 The stack may be a dictionary entry containing a ref stack, or just a ref stack on its own.
 
 @param stack The dictionary or reference stack.
 */
extern PDReferenceRef PDReferenceCreateFromStackDictEntry(pd_stack stack);

/**
 Create a reference for the given object ID and generation number.
 
 @param obid Object ID.
 @param genid Generation number.
 */
extern PDReferenceRef PDReferenceCreate(PDInteger obid, PDInteger genid);

/**
 Get the object ID for the reference.
 
 @param reference The reference.
 */
extern PDInteger PDReferenceGetObjectID(PDReferenceRef reference);

/**
 Get the generation ID for the reference.
 
 @param reference The reference.
 */
extern PDInteger PDReferenceGetGenerationID(PDReferenceRef reference);

extern PDInteger PDReferencePrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap);

#endif

/** @} */
