//
// PDParserAttachment.h
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
 @file PDParserAttachment.h PDF parser attachment header file.
 
 @ingroup PDPARSER
 
 @brief An attachment between two parsers
 
 @ingroup PDUSER
 
 @{
 */

#ifndef INCLUDED_PDParserAttachment_h
#define INCLUDED_PDParserAttachment_h

#include "PDDefines.h"

/**
 *  Create a foreign parser attachment, so that importing objects into parser from foreignParser becomes possible.
 *
 *  @param parser        The native parser, which will receive imported objects
 *  @param foreignParser The foreign parser, which will provide existing objects in import operations
 *
 *  @return A retained PDParserAttachment instance
 */
extern PDParserAttachmentRef PDParserAttachmentCreate(PDParserRef parser, PDParserRef foreignParser);

/**
 *  Iterate over the given foreign object recursively, creating appended objects for every indirect reference encountered. The resulting (new) indirect references, along with any direct references and associated stream (if any) are copied into a new object which is returned to the caller autoreleased.
 *
 *  A set of keys which should not be imported can be set. This set only applies to the object itself, and is not recursive. Thus, if an excluded key is encountered in one of the indirectly referenced objects, it will be included.
 *
 *  @param parser           The parser which should import the foreign object
 *  @param attachment       The attachment to a foreign parser
 *  @param foreignObject    The foreign object
 *  @param excludeKeys      Array of strings that should not be imported, if any
 *  @param excludeKeysCount Size of excludeKeys array
 *
 *  @return The new, native object based on the foreign object
 */
extern PDObjectRef PDParserAttachmentImportObject(PDParserAttachmentRef attachment, PDObjectRef foreignObject, const char **excludeKeys, PDInteger excludeKeysCount);

#endif
