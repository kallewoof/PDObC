//
// PDCMap.h
//
// Copyright (c) 2015 Karl-Johan Alm (http://github.com/kallewoof)
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
 @file PDCMap.h PDF CMap handler
 
 @ingroup PDFONTS
 
 CMap handler
 
 @{
 */

#ifndef INCLUDED_PDCMAP_H
#define INCLUDED_PDCMAP_H

#include "PDDefines.h"

/**
 *  Create a CMap object based on data, a CMap program, len bytes long.
 *
 *  @param data CMap program
 *  @param len  Length of data in bytes
 *
 *  @return A PDCMap object
 */
extern PDCMapRef PDCMapCreateWithData(char *data, PDSize len);

/**
 *  Create a blank CMap object.
 *
 *  @return A blank CMap object
 */
extern PDCMapRef PDCMapCreate();

extern void PDCMapAllocateCodespaceRanges(PDCMapRef cmap, PDSize count);
extern void PDCMapAppendCodespaceRange(PDCMapRef cmap, PDStringRef rangeFrom, PDStringRef rangeTo);

extern void PDCMapAllocateBFChars(PDCMapRef cmap, PDSize count);
extern void PDCMapAppendBFChar(PDCMapRef cmap, PDStringRef charFrom, PDStringRef charTo);

extern void PDCMapAllocateBFRanges(PDCMapRef cmap, PDSize count);
extern void PDCMapAppendBFRange(PDCMapRef cmap, PDStringRef rangeFrom, PDStringRef rangeTo, PDStringRef mapTo);

/// @note: this function is a proof of concept; it is not meant to be used in production environments; it is not optimized at all, which in this case means it is extremely slow
extern PDStringRef PDCMapApply(PDCMapRef cmap, PDStringRef string);

#endif // INCLUDED_PDCMAP_H
