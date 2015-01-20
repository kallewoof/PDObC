//
// PDCatalog.h
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
 @file PDCatalog.h PDF catalog header file.
 
 @ingroup PDCATALOG
 
 @defgroup PDCATALOG PDCatalog
 
 @brief PDF catalog, whose primary purpose is to map PDF objects to pages in a PDF document.
 
 @ingroup PDPIPE_CONCEPT

 The PDF catalog, usually derived from the "root" object of the PDF, consists of a media box (a rectangle defining the size of all pages) as well as an array of "kids", each kid being a page in the PDF. 
 
 The PDParser instantiates the catalog to the root object's Pages object on demand, e.g. when a task filter is set to some page, or simply when the developer requests it for the first time.
 
 @{
 */

#ifndef INCLUDED_PDCATALOG_h
#define INCLUDED_PDCATALOG_h

#include <sys/types.h>
#include "PDDefines.h"

/**
 Set up a catalog with a PDParser and a catalog object.
 
 The catalog will, via the parser, fetch the information needed to provide a complete representation of the pages supplied by the given catalog object. Normally, the catalog object is the root object of the PDF.
 
 @note It is recommended to use the parser's built-in PDParserGetCatalog() function for getting the root object catalog, if nothing else for sake of efficiency.
 
 @param parser  The PDParserRef instance.
 @param catalog The catalog object.
 @return The PDCatalog instance.
 */
extern PDCatalogRef PDCatalogCreateWithParserForObject(PDParserRef parser, PDObjectRef catalog);

/**
 Determine the object ID for the given page number, or throw an assertion if the page number is out of bounds.
 
 @warning Page numbers begin at 1, not 0.
 
 @param catalog The catalog object.
 @param pageNumber The page number whose object ID should be provided.
 @return The object ID.
 */
extern PDInteger PDCatalogGetObjectIDForPage(PDCatalogRef catalog, PDInteger pageNumber);

/**
 Determine the number of pages in this catalog.
 
 @warning Page numbers are valid in the range [1 .. pc], not [0 .. pc-1] where pc is the returned value.
 
 @param catalog The catalog object.
 @return The number of pages contained.
 */
extern PDInteger PDCatalogGetPageCount(PDCatalogRef catalog);

/**
 *  Insert a page into the catalog.
 *
 *  This does not affect the final representation of the output PDF in any way, but is here to ensure that
 *  the representation of a catalog is up to date with page modifications.
 *
 *  @note The parents' (and grand parents') Count is not updated. This must be done separately.
 *
 *  @param catalog      The catalog
 *  @param pageNumber   The page number (index starts at 1, not 0)
 *  @param pageObject   The page object being inserted
 */
extern void PDCatalogInsertPage(PDCatalogRef catalog, PDInteger pageNumber, PDObjectRef pageObject);

#endif

/** @} */
