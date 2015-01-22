//
// PDContentStreamTextExtractor.h
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
 @file PDContentStreamTextExtractor.h PDF content stream text extractor header file.
 
 @ingroup PDCONTENTSTREAM
 
 Extracts text from a PDF as a string.
 
 @{
 */

#ifndef INCLUDED_PDContentStreamTextExtractor_h
#define INCLUDED_PDContentStreamTextExtractor_h

#include "PDContentStream.h"

/**
 *  Create a content stream configured to write all string values of the stream into a string, allocated to fit any amount of content, then pointing *result to the string.
 *
 *  @param page   Page object, from which font information is fetched
 *  @param result Pointer to char * into which results are to be written
 *
 *  @return A pre-configured content stream
 */
extern PDContentStreamRef PDContentStreamCreateTextExtractor(PDPageRef page, char **result);

#endif

/** @} */

/** @} */
