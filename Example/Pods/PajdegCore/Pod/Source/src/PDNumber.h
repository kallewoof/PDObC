//
// PDNumber.h
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
 @file PDNumber.h Number wrapper
 
 @ingroup PDNUMBER
 
 @defgroup PDNUMBER PDNumber
 
 @brief A wrapper around PDF numbers.
 
 PDNumber objects exist to provide a unified way to maintain and convert between different number types. It also serves as a retainable wrapper around numeric values.
 
 @{
 */

#ifndef INCLUDED_PDNUMBER_H
#define INCLUDED_PDNUMBER_H

#include "PDDefines.h"

extern PDNumberRef PDNullObject;

// retained variants
extern PDNumberRef PDNumberCreateWithInteger(PDInteger i);
extern PDNumberRef PDNumberCreateWithSize(PDSize s);
extern PDNumberRef PDNumberCreateWithReal(PDReal r);
extern PDNumberRef PDNumberCreateWithBool(PDBool b);
extern PDNumberRef PDNumberCreateWithCString(const char *cString);

// autoreleased variants
#define PDNumberWithInteger(i)  PDAutorelease(PDNumberCreateWithInteger(i))
#define PDNumberWithSize(s)     PDAutorelease(PDNumberCreateWithSize(s))
#define PDNumberWithReal(r)     PDAutorelease(PDNumberCreateWithReal(r))
#define PDNumberWithBool(b)     PDAutorelease(PDNumberCreateWithBool(b))

// NULL n safe
extern PDInteger PDNumberGetInteger(PDNumberRef n);
extern PDSize PDNumberGetSize(PDNumberRef n);
extern PDReal PDNumberGetReal(PDNumberRef n);
extern PDBool PDNumberGetBool(PDNumberRef n);

extern PDInteger PDNumberPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap);

extern PDObjectType PDNumberGetObjectType(PDNumberRef n);

extern char *PDNumberToString(PDNumberRef n);

#endif // INCLUDED_PDNUMBER_H

/** @} */
