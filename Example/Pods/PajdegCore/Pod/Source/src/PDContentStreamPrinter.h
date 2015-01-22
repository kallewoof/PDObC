//
// PDContentStreamPrinter.h
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
 @file PDContentStreamPrinter.h PDF content stream printer header file.
 
 @ingroup PDCONTENTSTREAM
 
 Prints the commands of a content stream to e.g. stdout.
 
 @{
 */

#ifndef INCLUDED_PDContentStreamPrinter_h
#define INCLUDED_PDContentStreamPrinter_h

#include "PDContentStream.h"

/**
 *  Create a content stream configured to print out every operation to the given file stream
 *
 *  This is purely for debugging Pajdeg and/or odd PDF content streams, or to learn what the various operators do and how they affect things.
 *
 *  @param stream The file stream to which printing should be made
 *
 *  @return A pre-configured content stream
 */
extern PDContentStreamRef PDContentStreamCreateStreamPrinter(FILE *stream);

#endif

/** @} */

/** @} */
