//
// PDPage.h
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
 @file PDPage.h PDF page header file.
 
 @ingroup PDPAGE
 
 @defgroup PDPAGE PDPage
 
 @brief A PDF page.
 
 @ingroup PDUSER

 A representation of a page inside a PDF document. It is associated with an object and has a reference to the PDParser instance for the owning document.
 
 @{
 */

#ifndef INCLUDED_PDPage_h
#define INCLUDED_PDPage_h

#include "PDDefines.h"

/**
 *  Create a new page instance for the given parser and page number. 
 *
 *  @note Page numbers start at 1, not 0.
 *
 *  @param parser     Parser reference
 *  @param pageNumber The page number of the page to fetch
 *
 *  @return New PDPage object
 */
extern PDPageRef PDPageCreateForPageWithNumber(PDParserRef parser, PDInteger pageNumber);

/**
 *  Create a new page instance for the given parser and object.
 *
 *  @note The object must belong to the parser, and may not be from a foreign instance of Pajdeg.
 *
 *  @note The object does not necessarily have to be a proper page at the time of creation, but it should (ideally) be one eventually.
 *
 *  @param parser The parser owning object
 *  @param object The /Page object
 *
 *  @return A new page instance for the associated object
 */
extern PDPageRef PDPageCreateWithObject(PDParserRef parser, PDObjectRef object);

/**
 *  Copy this page and all associated objects into the PDF document associated with the pipe, inserting it at pageNumber.
 *
 *  With two separate simultaneous Pajdeg instances A and B with pages A1 A2 A3 and B1 B2 B3, the following operation
 *
    @code
      PDPageRef pageA2 = PDPageCreateForPageWithNumber(parserA, 2); 
      PDPageRef pageB2 = PDPageInsertIntoPipe(pageA2, pipeB, 2);
    @endcode
 *
 *  will result in the output of B consisting of pages B1 A2 B2 B3, in that order.
 * 
 *  @param page       The page object that should be copied
 *  @param pipe       The pipe into which the page object should be inserted
 *  @param pageNumber The resulting page number of the inserted page
 *
 *  @return Reference to the inserted page, autoreleased
 */
extern PDPageRef PDPageInsertIntoPipe(PDPageRef page, PDPipeRef pipe, PDInteger pageNumber);

/**
 *  Determine the number of content objects this page has.
 *
 *  @param page Page to be checked
 *
 *  @return Number of content objects
 */
extern PDInteger PDPageGetContentsObjectCount(PDPageRef page);

/**
 *  Fetch (if unfetched) and return the index'th PDObject associated with the page's /Contents key.
 *  A lot of pages only have a single contents object, but they are allowed to have an array of them, in which case the resulting page is rendered as if the array of streams were concatenated.
 *
 *  @param page  Page whose contents object is requested
 *  @param index The index (starting at 0) of the contents object to fetch
 *
 *  @return The contents object for the given index, for the page
 */
extern PDObjectRef PDPageGetContentsObjectAtIndex(PDPageRef page, PDInteger index);

/**
 *  Fetch the media box for the given page.
 *
 *  @param page The page object
 *
 *  @note PDRects differ from e.g. CGRects in that they are composed of the two points making up the rectangle, as opposed to making up an origin pair and a size pair.
 *
 *  @return A PDRect representing the media box.
 */
extern PDRect PDPageGetMediaBox(PDPageRef page);

/**
 *  Fetch the annotations for this page, as an array of PDReferences and/or PDObjects. 
 *  The entries are normally references, but if annotations were replaced or added as PDObjects, they will be returned as is.
 *
 *  @param page The page object
 *
 *  @return An array of annotation references and objects.
 */
extern PDArrayRef PDPageGetAnnotRefs(PDPageRef page);

#endif

/** @} */

/** @} */ // unbalanced, but doxygen complains for some reason

