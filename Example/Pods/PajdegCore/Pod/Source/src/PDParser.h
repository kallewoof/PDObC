//
// PDParser.h
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
 @file PDParser.h Parser header file.
 
 @ingroup PDPARSER
 
 @defgroup PDPARSER PDParser
 
 @brief Parser implementation for PDF documents.
 
 @ingroup PDPIPE_CONCEPT
 
 The parser takes a PDTwinStreamRef on creation, and uses PDScannerRef instances to interpret the input PDF and to generate the output PDF document. It provides support for fetching objects from the PDF in a number of ways, inserting objects, and finding things out about the input PDF, such as encryption state, or about a given object, such as whether it is still mutable or not.
 
 @{
 */

#ifndef INCLUDED_PDParser_h
#define INCLUDED_PDParser_h

#include <sys/types.h>
#include "PDDefines.h"

/**
 Set up a parser with a twin stream
 
 @param stream The stream to use.
 */
extern PDParserRef PDParserCreateWithStream(PDTwinStreamRef stream);

/**
 Iterate to the next (living) object.
 
 @param parser The parser.
 @return false if there are no more objects
 */
extern PDBool PDParserIterate(PDParserRef parser);

/**
 Construct a PDObjectRef for the current object.
 
 @param parser The parser.
 
 @note Subsequent calls to PDParserConstructObject() if the parser has not iterated does nothing.
 */
extern PDObjectRef PDParserConstructObject(PDParserRef parser);

/**
 Create a new object with an appropriate object id (determined via the XREF table), for insertion.
 
 The object is inserted after the current object in the stream, or at the current position if in between objects.
 
 @param parser The parser.
 */
extern PDObjectRef PDParserCreateNewObject(PDParserRef parser);

/**
 Create a new object with an appropriate object id (determined via the XREF table) for appending to the end of the output document.
 
 @note Appended objects are put into a stack awaiting the end of the input PDF. Multiple objects will thus come out in the opposite order in the output PDF file.
 
 @param parser The parser.
 */
extern PDObjectRef PDParserCreateAppendedObject(PDParserRef parser);

/**
 Fetch the current object's stream, reading from the input source if necessary.
 
 Once fetched, this will simply return the stream buffer as is.
 
 @param parser The parser.
 @param obid The object ID of the current object. Assertion is thrown if it does not match the parser's expected ID, or if the current object is not in the original PDF (e.g. from PDParserCreateNewObject()).
 @return Stream buffer.
 */
extern char *PDParserFetchCurrentObjectStream(PDParserRef parser, PDInteger obid);

/**
 Fetch the object stream of the given object.
 
 Once fetched, this will simply return the stream buffer as is.
 
 @param parser The parser.
 @param object The object whose stream is to be fetched. Assertion is thrown if it is not in the original PDF.
 @return Stream buffer.
 */
extern const char *PDParserLocateAndFetchObjectStreamForObject(PDParserRef parser, PDObjectRef object);

/**
 Determine if the PDF is encrypted or not.
 
 @param parser The parser.
 @return true if encrypted, false if unencrypted.
 */
extern PDBool PDParserGetEncryptionState(PDParserRef parser);

/**
 Fetch the definition (as a pd_stack) of the object with the given id. 
 
 @note Use of PDParserLocateAndCreateObject is recommended, as it is generally faster and will not get confused about objects inserted *this session*.
 
 @warning This is an expensive operation that requires setting up a temporary buffer of sufficiently big size, seeking to the object in the input file, reading the definition, then seeking back.
 
 @warning This is no longer the recommended way to obtain random access objects. Use PDParserLocateAndCreateObject instead.
 
 @param parser The parser.
 @param obid The object ID
 @param master If true, the master PDX ref is referenced, otherwise the current PDX ref is used. Generally speaking, you always want to use the master (non-master is used internally to determine the deprecated length of a stream for a multi-part PDF).
 */
extern pd_stack PDParserLocateAndCreateDefinitionForObject(PDParserRef parser, PDInteger obid, PDBool master);

/**
 Fetch an object reference of the object with the given id.
 
 @warning This is an expensive operation that requires setting up a temporary buffer of sufficiently big size, seeking to the object in the input file, reading the definition, then seeking back.
 
 @note The object is returned as a retained object and must be PDRelease()d or it will leak.
 
 @param parser The parser.
 @param obid The object ID
 @param master If true, the master PDX ref is referenced, otherwise the current PDX ref is used. Generally speaking, you always want to use the master (non-master is used internally to determine the deprecated length of a stream for a multi-part PDF).
 */
extern PDObjectRef PDParserLocateAndCreateObject(PDParserRef parser, PDInteger obid, PDBool master);

/**
 Write remaining objects, XREF table, trailer, and end fluff to output PDF.
 
 @param parser The parser.
 */
extern void PDParserDone(PDParserRef parser);

/**
 Determine if object with given id has already been written to output stream (i.e. has become immutable).
 
 @warning This method *only* tells if the object with the given ID has been iterated past already. Whether a PDObjectRef instance is actually mutable or not depends on whether it was inherently immutable or not (objects based on PDParserLocateAndCreateDefinitionForObject() are inherently immutable, and include the object returned from PDPipeGetRootObject().).
 
 @param parser The parser.
 @param obid The object ID.
 
 @see PDPipeGetRootObject
 @see PDParserLocateAndCreateDefinitionForObject
 */
extern PDBool PDParserIsObjectStillMutable(PDParserRef parser, PDInteger obid);

/**
 Determine the object id of the object stream containing the object with the given id, if any.
 
 @param parser The parser.
 @param obid The object whose container object ID is to be determined.
 @return -1 if the object is not inside an object stream.
 */
extern PDInteger PDParserGetContainerObjectIDForObject(PDParserRef parser, PDInteger obid);

/**
 Get an immutable reference to the root object for the input PDF.
 
 @param parser The parser.
 */
extern PDObjectRef PDParserGetRootObject(PDParserRef parser);

/**
 Get an immutable reference to the info object for the input PDF, or NULL if the input PDF does not contain an info object.
 
 @param parser The parser
 @return NULL or the info object reference
 */
extern PDObjectRef PDParserGetInfoObject(PDParserRef parser);

/**
 *  Get a mutable reference to the trailer object for the PDF.
 *
 *  @param parser The parser
 *
 *  @return The trailer object, which is written at the very end and is thus mutable until the PDF is finalized
 */
extern PDObjectRef PDParserGetTrailerObject(PDParserRef parser);

/**
 Set up (if necessary) and return the PDCatalog object for the current PDF.
 
 @param parser The parser.
 @return The catalog containing information about the pages in the PDF.
 */
extern PDCatalogRef PDParserGetCatalog(PDParserRef parser);

/**
 Get the total number of objects in the input stream.
 
 @param parser The parser.
 */
extern PDInteger PDParserGetTotalObjectCount(PDParserRef parser);

/**
 * @page XREF versioning and indirect stream lengths
 * 
 * [1]: an intriguing problem arises with PDF's that have data appended to them, where multiple instances of the same object with the same generation ID exist, which have streams that have
 *      (inhale) lengths which are indirect references rather than sizes (exhale) -- is this accepted by the PDF spec, you wonder? Adobe InDesign does it, so I can only presume yes, but it
 *      seems rather odd; in any case, the problem is demonstrated by the following PDF fragments:
 *
 * %PDF
 * 1 0 obj
 * <</Length 2 0 R>>
 * stream
 * ...
 * endstream
 * endobj
 * 2 0 obj
 * 123
 * endobj
 * xref
 * 0 3
 * 0000000000 65535 n
 * 0000000005 00000 n   <-- obj 1
 * 0000000150 00000 n   <-- obj 2, defining obj 1 stream length
 * trailer
 * ...
 * %%EOF
 * % here, appended PDF kicks in
 * %PDF
 * 1 0 obj
 * <</Length 2 0 R>>
 * stram
 * ...
 * endstream
 * 2 0 obj
 * 457
 * endobj
 * xref
 * 0 3
 * 0000000000 65535 n
 * 0000000400 00000 n   <-- new obj 1
 * 0000000950 00000 n   <-- new obj 2, defining obj 1 stream length
 * ...
 *
 * When Pajdeg sets up the XREF table, it assumes that appended PDF's behave in a sane fashion, i.e. each append results in a set of replacements for given objects
 * which is indeed the case; however, since Pajdeg iterates over *all* objects, rather than jumping to objects on demand, it encounters objects that are deprecated,
 * such as 1 0 obj above; when Pajdeg hits the (old) 1 0 obj, it tries to move beyond it, but since it has a Length with a reference, Pajdeg looks up the reference
 * IN THE XREF TABLE, which has been overridden, and would incorrectly presume that the length of the (old) 1 0 obj's stream is 457 bytes, when in reality it is
 * 123 bytes. (This is fixed, but at the cost of multi-layer XREF tables.)
 * Additionally, since each PDF append in fact *is* a patch operation, each XREF table has to not only be kept separate, but has to be properly patched with the
 * previous tables' content. 
 */

#endif

/** @} */
