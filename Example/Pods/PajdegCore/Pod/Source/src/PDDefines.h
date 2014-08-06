//
// PDDefines.h
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
 @file PDDefines.h
 @brief Definitions for the Pajdeg interface.
 */

#ifndef INCLUDED_PDDefines_h
#define INCLUDED_PDDefines_h

#include <sys/types.h>

/**
  Support zlib compression for filters.
  */
#define PD_SUPPORT_ZLIB

/**
 Support cryptography in PDFs. Currently includes RC4/MD5, but not AES.
 */
#define PD_SUPPORT_CRYPTO

/**
 @def DEBUG
 Turn on all assertions and warnings.
 
 This is recommended when writing or testing Pajdeg, but should never be turned on in production code.
 
 This is enabled by default in e.g. Apple's Xcode 4.x, for debug configurations.
 */
// #define DEBUG

/**
 @def PD_WARNINGS
 Show internal warnings on stderr.
 
 The PD_WARNINGS directive turns on printing of warnings to stderr. Enabled implicitly if DEBUG is defined.
 */
#define PD_WARNINGS

/**
 @def PD_NOTICES
 Show internal notices (weak warnings) on stderr.
 
 The PD_NOTICES directive turns on printing of notices (PDInfo output) to stderr. Must be turned on explicitly.
 */
//#define PD_NOTICES

/**
 @def PD_ASSERTS
 Enable internal assertions. 
 
 The PD_ASSERTS directive turns on assertions. If something is misbehaving, turning this on is a good idea as it provides a crash point and/or clue as to what's not going right. Enabled implicitly if DEBUG is defined.
 */
#define PD_ASSERTS

/**
 @def PD_DEBUG_TWINSTREAM_ASSERT_OBJECTS 
 Reassert output objects.
 
 Enables reassertions of every single object inserted into the output PDF, by seeking back to its supposed position (XREF-wise) and reading in the "num num obj" part.
 */
//#define PD_DEBUG_TWINSTREAM_ASSERT_OBJECTS

/**
 @def DEBUG_PARSER_PRINT_XREFS 
 Prints out the resulting XREF table to stdout on setup. 
 
 @warning This is not updated to reflect the binary format replacement of the XRef table and should not be enabled.
 */
//#define DEBUG_PARSER_PRINT_XREFS

/**
 @def DEBUG_PARSER_CHECK_XREFS
 Checks every single object (that is "in use") against the input PDF.
 
 This is done by seeking to the specified offset, reading in a chunk of data, and comparing said data to the expected object. Needless to say, expensive, but excellent starting point to determine if a PDF is broken or not (XREF table tends to break "first").
 */
//#define DEBUG_PARSER_CHECK_XREFS

/**
 @def DEBUG_SCANNER_SYMBOLS
 Prints to stdout every symbol scanned when reading input, tabbed and surrounded in asterixes (e.g. "           * startxref *").
 */
//#define DEBUG_SCANNER_SYMBOLS

/**
 @def DEBUG_PDTYPES
 Adds header to all PDType objects and asserts that an object is a real PDType when retained/released.
 
 Keeping this enabled is recommended, even for production code, unless it's very stable.
 */
#define DEBUG_PDTYPES

/**
 @def DEBUG_PD_RELEASES
 Adds debug information to all release calls to track down over-releases and invalid releases and such.
 
 This should be disabled unless you are debugging Pajdeg code.
 */
//#define DEBUG_PD_RELEASES

/**
 @defgroup CORE_GRP Core types
 @brief Internal type definitions.
 
 These have been matched up with the Core Graphics CGPDF* type definitions in most cases, except where this resulted in performance or other issues.
 
 @{
 */

/**
 PDF integer type in Pajdeg.
 */
typedef long                 PDInteger;

/**
 C atoXX function matching PDInteger size
 */
#define PDIntegerFromString atol

/**
 PDF real type in Pajdeg.
 */
typedef float                PDReal;

/**
 C atoXX function matching PDReal
 */
#define PDRealFromString atof

/**
 PDF boolean identifier.
 */
#ifdef bool
typedef bool                 PDBool;
#else
typedef unsigned char        PDBool;
#endif

/**
 Size type (unsigned).
 */
typedef size_t               PDSize;

/**
 C atoXX function matching PDSize
 */
#define PDSizeFromString     atol

/**
 Offset type (signed).
 */
typedef long long            PDOffset;

/**
 C atoXX function matching PDOffset
 */
#define PDOffsetFromString   atoll

/**
 A point with x and y coordinates.
 */
typedef struct PDPoint       PDPoint;
struct PDPoint {
    PDReal x, y;
};

/**
 A rectangle made up of two PDPoint structures.
 */
typedef struct PDRect        PDRect;
struct PDRect {
    PDPoint a, b;
};

/**
 Convert from an origin-size rect (e.g. CGRect) to a tl-br rect (PDRect).
 
 @param osr The rect structured as { {origin x, origin y}, {width, height} }. 
 @return PDRect version.
 */
#define PDRectFromOSRect(osr) (PDRect) {{osr.origin.x, osr.origin.y}, {osr.origin.x+osr.size.width, osr.origin.y+osr.size.height}}

/**
 Convert to an origin-size rect (e.g. CGRect) from a tl-br rect (PDRect).
 
 @param rect PDRect version.
 @return The rect structured as { {origin x, origin y}, {width, height} }. 
 */
#define PDRectToOSRect(rect) {{rect.a.x, rect.a.y}, {rect.b.x-rect.a.x, rect.b.y-rect.a.y}}

/**
 Convert an array string [a b c d] into a PDRect.
 */
//#define PDRectReadFromArrayString(rect, str) sscanf(str, "[%f %f %f %f]", &rect.a.x, &rect.a.y, &rect.b.x, &rect.b.y)

/**
 Convert a PDRect into an [a b c d] array string.
 */
//#define PDRectWriteToArrayString(rect, str) sprintf(str, "[%f %f %f %f]", rect.a.x, rect.a.y, rect.b.x, rect.b.y)

/**
 Identifier type.
 
 The PDID type in Pajdeg has two purposes: it works like a container for generic data of unknown origin, and it is a numeric pointer to a string identifier. 
 
 Wherever used, the PDID implies that its content is "protected" and should never be modified, such as freed.
 
 The pd_stack implementation adheres this convention in most cases, but some debug related functions, such as pd_stack_print, will assume that a PDID is of the latter
 type, and will attempt to print out the string pointed to in parentheses. 
 
 PDIDs are used extensively in the PDF specification implementation definitions located in pd_pdf_implementation.h
 */
typedef const char         **PDID;

/** @} // CORE_GRP */

/**
 @defgroup PDINTERNAL Internal
 
 @{
 */

/**
 Deallocation signature. 
 
 When encountered, the default is usually the built-in free() method, unless otherwise stated.
 */
typedef void (*PDDeallocator)(void *ob);

/**
 NOP function (e.g. used as NULL deallocator).
 */
extern void PDNOP(void*);

/**
 Synchronization signature.
 
 Used in the PDParser to require synchronization of the given object instance, as it is about to be written to the output stream.
 */
typedef void (*PDSynchronizer)(void *parser, void *object, const void *syncInfo);

/**
 Pajdeg type.
 
 The PDType structure facilitates the retain and release layer for Pajdeg objects.
 */
typedef union PDType *PDTypeRef;

/** @} // PDINTERNAL */

/**
 @defgroup PDALGO Algorithm-related
 
 @{
 */

/**
 A simple stack implementation.
 
 @ingroup pd_stack
 
 The pd_stack is tailored to handle some common types used in Pajdeg.
 */
typedef struct pd_stack      *pd_stack;

///**
// A low-performance array implementation.
// 
// @ingroup pd_array
// 
// The pd_array.
// */
//typedef struct pd_array     *pd_array;
//
///**
// A low-performance dictionary implementation.
// 
// @ingroup pd_dict
// 
// The pd_dict.
// */
//typedef struct pd_dict      *pd_dict;

/**
 A low-performance array implementation.
 
 @ingroup PDARRAY
 
 The array construct.
 */
typedef struct PDArray      *PDArrayRef;

/**
 A low-performance dictionary implementation.
 
 @ingroup PDDICTIONARY
 
 The dictionary construct.
 */
typedef struct PDDictionary *PDDictionaryRef;

/**
 Crypto methods.
 */
typedef enum {
    pd_crypto_method_none  = 0,
    pd_crypto_method_rc4   = 1,
    pd_crypto_method_aesv2 = 2,
} pd_crypto_method;

/**
 Crypto authentication events.
 */
typedef enum {
    pd_auth_event_none    = 0,
    pd_auth_event_docopen = 1,
} pd_auth_event;

/**
 Cryptography module for PDF encryption/decryption.
 
 @note If PD_SUPPORT_CRYPTO is not set, this structure is a dummy object.
 
 @ingroup pd_crypto
 
 The pd_crypto object.
 */
typedef struct pd_crypto    *pd_crypto;

/**
 A (very) simple hash table implementation.
 
 @ingroup PDSTATICHASH
 
 Limited to predefined set of primitive keys on creation. O(1) but triggers false positives in some instances.
 */
typedef struct PDStaticHash *PDStaticHashRef;

/**
 A binary splay tree implementation.
 
 @ingroup PDSPLAYTREE
 */
typedef struct PDSplayTree *PDSplayTreeRef;

/** @} // PDALGO */

/**
 @defgroup PDUSER User level
 
 @brief The user level (most commonly used) objects.
 
 @{
 */

/**
 *  A selection in a PDF
 *
 *  @ingroup PDSELECTION
 *
 *  Although selections may span across multiple pages, this is currently ignored.
 */
struct PDSelection {
    PDInteger page;
    PDRect position;
    const char *text;
};

/**
 *  A PDF selection.
 *
 *  @ingroup PDSELECTION
 */
typedef struct PDSelection *PDSelectionRef;

/**
 *  Text search operator function signature, used by PDContentStream's text searcher.
 *
 *  @param selection Selection that was matched to the search string
 *
 *  @return Whether or not search should continue or abort
 */
typedef PDBool (*PDTextSearchOperatorFunc)(PDSelectionRef selection);

/**
 A PDF object.
 
 @ingroup PDOBJECT
 */
typedef struct PDObject *PDObjectRef;

/**
 A PDF page.
 
 @ingroup PDPAGE
 */
typedef struct PDPage *PDPageRef;

/**
 *  An attachment to a foreign parser.
 *
 *  Attachments exist for the sole purpose of keeping track of which objects have been imported from the foreign parser already. 
 *  Since many indirect object references are shared between multiple objects, the attachment ensures that a given object is only
 *  imported once, even if several consecutive object imports are made. 
 */
typedef struct PDParserAttachment *PDParserAttachmentRef;

/**
 A PDF object stream.
 
 @ingroup PDOBJECTSTREAM
 */
typedef struct PDObjectStream *PDObjectStreamRef;

/**
 A PDF content stream.
 
 @ingroup PDCONTENTSTREAM
 */
typedef struct PDContentStream *PDContentStreamRef;

/**
 Content operator result type. 
 
 @ingroup PDCONTENTSTREAM
 
 Content operators are responsible for maintaining the operator state of their designated functions. For example, the 'q' and 'BT' operators need to return PDOperatorStatePush and the 'Q' and 'ET' operators need to return PDOperatorStatePop. The default behavior for all unattached operators is PDOperatorStateIndependent, i.e. the operator does not modify the current operators stack.
 */
typedef enum {
    PDOperatorStateIndependent = 0, ///< this operator does not push nor pop the stack
    PDOperatorStatePush        = 1, ///< this operator pushes onto the stack
    PDOperatorStatePop         = 2, ///< this operator pops the stack
} PDOperatorState;

/**
 Function signature for content operators. 
 
 @ingroup PDCONTENTSTREAM
 @param cs       Content stream reference
 @param args     Arguments as PDArray
 @param inState  The top-most entry in the operation stack, if any
 @param outState Pointer to the resulting state after this operation; only applies to operators that return PDOperatorStatePush
 
 @return State of operator (i.e. whether it pushes, pops, or does neither)
 */
typedef PDOperatorState (*PDContentOperatorFunc)(PDContentStreamRef cs, void *userInfo, PDArrayRef args, pd_stack inState, pd_stack *outState);

/**
 The PD instance type of a value.
 
 @ingroup PDOBJECT
 */
typedef enum {
    PDInstanceTypeUnset     =-2,    ///< The associated instance value has not been set yet
    PDInstanceTypeUnknown   =-1,    ///< Undefined / non-allocated instance
    PDInstanceTypeNull      = 0,    ///< NULL
    PDInstanceTypeNumber    = 1,    ///< PDNumber
    PDInstanceTypeString    = 2,    ///< PDString
    PDInstanceTypeArray     = 3,    ///< PDArray
    PDInstanceTypeDict      = 4,    ///< PDDictionary
    PDInstanceTypeRef       = 5,    ///< PDReference
    PDInstanceTypeObj       = 6,    ///< PDObject
} PDInstanceType;

/**
 *  String printing signature.
 *
 *  Used to write PDInstance objects into a C string buffer.
 */
typedef PDInteger (*PDInstancePrinter)(void *inst, char **buf, PDInteger offs, PDInteger *cap);

/**
 *  String printer function pointer array, matching up with all non-negative PDInstanceType entries.
 */
extern PDInstancePrinter PDInstancePrinters[];

/**
 The type of object.
 
 @ingroup PDOBJECT
 
 @note This enum is matched with CGPDFObject's type enum (Core Graphics), with extensions
 @warning Not all types are currently used.
 */
typedef enum {
    PDObjectTypeNull        = 0,    ///< Null object, often used to indicate the type of a dictionary entry for a key that it doesn't contain
    PDObjectTypeUnknown     = 1,    ///< The type of the object has not (yet) been determined
    PDObjectTypeBoolean,            ///< A boolean.
    PDObjectTypeInteger,            ///< An integer.
    PDObjectTypeReal,               ///< A real (internally represented as a float).
    PDObjectTypeName,               ///< A name. Names in PDFs are things that begin with a slash, e.g. /Info.
    PDObjectTypeString,             ///< A string.
    PDObjectTypeArray,              ///< An array.
    PDObjectTypeDictionary,         ///< A dictionary. Most objects are considered dictionaries.
    PDObjectTypeStream,             ///< A stream.
    PDObjectTypeReference,          ///< A reference to another object.
    PDObjectTypeSize,               ///< A size (not in CGPDFObject type)
} PDObjectType;

/**
 The class of an object;
 
 @ingroup PDOBJECT
 */
typedef enum {
    PDObjectClassRegular    = 1,    ///< A regular object in a PDF
    PDObjectClassCompressed = 2,    ///< An object inside of an object stream
    PDObjectClassTrailer    = 3,    ///< A trailer
} PDObjectClass;

/**
 A reference to a PDF object.
 
 @ingroup PDREFERENCE
 */
typedef struct PDReference  *PDReferenceRef;

/**
 The type of a string.
 
 @ingroup PDSTRING
 */
typedef enum {
    PDStringTypeEscaped = 1,        ///< A regular string, escaped and NUL-terminated
    PDStringTypeHex     = 2,        ///< A HEX string, NUL-terminated
    PDStringTypeBinary  = 3,        ///< A binary string, which may or may not be NUL-terminated, and may contain any character
    PDStringTypeName    = 4,        ///< A regular string prefixed with a forward slash
} PDStringType;

/**
 A number object.
 
 @ingroup PDNUMBER
 */
typedef struct PDNumber  *PDNumberRef;

/**
 A string object.
 
 @ingroup PDSTRING
 */
typedef struct PDString  *PDStringRef;

/**
 The type of a collection.
 
 @ingroup PDCOLLECTION
 */
typedef enum {
    PDCollectionTypeArray = 1,      ///< A PDArrayRef
    PDCollectionTypeDictionary = 2, ///< A PDDictionaryRef
    PDCollectionTypeStack = 3,      ///< A pd_stack
} PDCollectionType;

/**
 A collection object.
 
 @ingroup PDCOLLECTION
 */
typedef struct PDCollection *PDCollectionRef;

/**
 @defgroup PDPIPE_CONCEPT Pajdeg Pipes
 
 @brief Piping PDFs through Pajdeg
 */

/**
 A pipe.
 
 @ingroup PDPIPE
 */
typedef struct PDPipe       *PDPipeRef;

/**
 A task.
 
 @ingroup PDTASK
 */
typedef struct PDTask       *PDTaskRef;

/**
 PDF object types.
 
 @note If this is modified, typeHash in PDPipe.c must be updated accordingly.
 
 @ingroup PDTASK
 */
typedef enum {
    PDFTypePage = 1,            ///< all page objects
    PDFTypePages,               ///< all pages objects; pages objects are dictionaries with arrays of page objects (the "Kids")
    PDFTypeCatalog,             ///< all catalog objects; normally, the Root object is the only catalog in a PDF
    PDFTypeAnnot,               ///< all annotation objects, such as links
    PDFTypeFont,                ///< font objects
    PDFTypeFontDescriptor,      ///< font descriptor objects
    PDFTypeAction,              ///< action objects; usually associated with a link annotation
    
    _PDFTypeCount,              ///< enum item count
} PDFType;

/**
 Link between numeric PDFType enumerator (used for PDTask values which are restricted to integers) and the actual string to look for in the PDF data.
 */
#define kPDFTypeStrings \
    NULL, "/Page", "/Pages", "/Catalog", "/Annot", "/Font", "/FontDescriptor", "/Action"

/**
 Task filter property type.
 
 @ingroup PDTASK
 */
typedef enum {
    PDPropertyObjectId      = 1,///< value = object ID; only called for the live object, even if multiple copies exist
    PDPropertyRootObject,       ///< triggered when root object is encountered
    PDPropertyInfoObject,       ///< triggered when info object is encountered
    PDPropertyPDFType,          ///< triggered for each object of the given PDFType value
    PDPropertyPage,             ///< value = page number
} PDPropertyType;

/**
 Task result type. 
 
 @ingroup PDTASK
 
 Tasks are chained together. When triggered, the first task in the list of tasks for the specific state is executed. Before moving on to the next task in the list, each task has a number of options specified in the task result.
 
 - A task has the option of canceling the entire PDF creation process due to some serious issue, via PDTaskFailure. 
 - It may also "filter" out its child tasks dynamically by returning PDTaskSkipRest for given conditions. This will stop the iterator, and any tasks remaining in the queue will be skipped. 
 
 Returning PDTaskDone means the task ended normally, and that the next task, if any, may execute. This is usually the preferred return value.
 */
typedef enum {
    PDTaskFailure   = -1,       ///< the entire pipe operation is terminated and the destination file is left unfinished
    PDTaskDone      =  0,       ///< the task ended as normal
    PDTaskSkipRest  =  1,       ///< the task ended, and requires that remaining tasks in its pipeline are skipped
    PDTaskUnload    =  2,       ///< the task ended as normal; additionally, it should never be called again
} PDTaskResult;

/**
 Function signature for task callbacks. 
 
 @ingroup PDTASK
 */
typedef PDTaskResult (*PDTaskFunc)(PDPipeRef pipe, PDTaskRef task, PDObjectRef object, void *info);

/**
 A parser.
 
 @ingroup PDPARSER
 */
typedef struct PDParser     *PDParserRef;

/**
 A catalog object.
 
 @ingroup PDCATALOG
 */
typedef struct PDCatalog    *PDCatalogRef;

/** @} // PDUSER */

/**
 A double-edged stream.
 
 @ingroup PDTWINSTREAM
 */
typedef struct PDTwinStream *PDTwinStreamRef;

/**
 The twin stream has three methods available for reading data: read/write, random access, and reversed. 
 
 @ingroup PDTWINSTREAM
 
 - Read/write is the default method, and is the only method allowed for writing to the output stream.
 - Random access turns the stream temporarily into a regular file reader, permitting random access to the input file. This mode is used when collecting XREF tables on parser initialization.
 - Reverse turns the stream inside out, in the sense that the heap is filled from the bottom, and buffer fills begin at the end and iterate back toward the beginning.
 */
typedef enum {
    PDTwinStreamReadWrite,      ///< reads and writes from start to end of input into output, skipping/replacing/inserting as appropriate
    PDTwinStreamRandomAccess,   ///< random access, jumping to positions in the input, not writing anything to output
    PDTwinStreamReversed,       ///< reads from the end of the input file, filling the heap from the end up towards the beginning
} PDTwinStreamMethod;

/**
 Prediction filter type, i.e. which strategy to use. 
 
 @ingroup PDSTREAMFILTERPREDICTION
 
 Based on the PDF specification v 1.7, table 3.8, p. 76.
 */
typedef enum {
    PDPredictorNone = 1,        ///< no prediction (default)
    PDPredictorTIFF2 = 2,       ///< TIFF predictor 2
    PDPredictorPNG_NONE = 10,   ///< PNG prediction (on encoding, PNG None on all rows)
    PDPredictorPNG_SUB = 11,    ///< PNG prediction (on encoding, PNG Sub on all rows)
    PDPredictorPNG_UP = 12,     ///< PNG prediction (on encoding, PNG Up on all rows)
    PDPredictorPNG_AVG = 13,    ///< PNG prediction (on encoding, PNG Average on all rows)
    PDPredictorPNG_PAE = 14,    ///< PNG prediction (on encoding, PNG Paeth on all rows)
    PDPredictorPNG_OPT = 15,    ///< PNG prediction (on encoding, PNG Paeth on all rows)
} PDPredictorType;

/**
 The stream filter type.
 
 @ingroup PDSTREAMFILTER
 */
typedef struct PDStreamFilter *PDStreamFilterRef;

/**
 @defgroup PDSCANNER_CONCEPT Symbol scanning
 
 @ingroup PDINTERNAL
 
 @{
 */

/**
 An environment. 
 
 @ingroup PDENV
 
 Environments are instances of states. 
 */
typedef struct PDEnv        *PDEnvRef;

/**
 A state. 
 
 @ingroup PDSTATE
 
 A state in Pajdeg is a definition of a given set of conditions.
 
 @see pd_pdf_implementation.h
 */
typedef struct PDState      *PDStateRef;

/**
 An operator. 
 
 @ingroup PDOPERATOR
 */
typedef struct PDOperator   *PDOperatorRef;

/**
 Operator type.
 
 @ingroup PDOPERATOR
 
 Pajdeg's internal parser is configured using a set of states and operators. Operators are defined using a PDOperatorType, and an appropriate unioned argument.
 
 @see pd_pdf_implementation.h
 */
typedef enum {
    PDOperatorPushState = 1,    ///< Pushes another state; e.g. "<" pushes dict_hex, which on seeing "<" pushes "dict"
    PDOperatorPushWeakState,    ///< Identical to above, but does not 'retain' the target state, to prevent retain cycles (e.g. arb -> array -> arb -> ...)
    PDOperatorPopState,         ///< Pops back to previous state
    PDOperatorPopVariable,      ///< Pops entry off of results stack and stores as an attribute of a future complex result
    PDOperatorPopValue,         ///< Pops entry off of results stack and stores in variable stack, without including a variable name
    PDOperatorPushEmptyString,  ///< Pushes an empty string onto the results stack
    PDOperatorPushResult,       ///< Pushes current symbol onto results stack
    //PDOperatorAppendResult,     ///< Appends current symbol to last result on stack
    //PDOperatorPushContent,      ///< Pushes everything from the point where the state was entered to current offset to results
    PDOperatorPushComplex,      ///< Pushes object of type description "key" with variables (if any) as attributes onto results stack
    PDOperatorStoveComplex,     ///< Pushes a complex onto build stack rather than results stack (for popping as a chunk of objects later)
    PDOperatorPullBuildVariable,///< Takes build stack as is, and stores it as if it was a popped variable
    PDOperatorPushbackSymbol,   ///< Pushes the scanned symbol back onto the symbols stack, so that it is re-read the next time a symbol is popped from the scanner
    PDOperatorPushbackValue,    ///< Pushes the top value on the stack onto the symbols stack as if it were never read
    PDOperatorPopLine,          ///< Read to end of line
    PDOperatorReadToDelimiter,  ///< Read over symbols and whitespace until a delimiter is encountered
    PDOperatorMark,             ///< Mark current position in buffer for popping as is into results later
    PDOperatorPushMarked,       ///< Push everything from mark to current position in buffer as results
    PDOperatorNOP,              ///< Do nothing
    // debugging
    PDOperatorBreak,            ///< Break (presuming breakpoint is properly placed)
} PDOperatorType;

/**
 A scanner.
 
 @ingroup PDSCANNER
 */
typedef struct PDScanner    *PDScannerRef;

/**
 Called whenever the scanner needs more data.
 
 @ingroup PDSCANNER
 
 The scanner's buffer function requires that the buffer is intact in the sense that the content in the range 0..*size (on call) remains intact and in the same position relative to *buf; req may be set if the scanner has an idea of how much data it needs, but is most often 0.
 */
typedef void(*PDScannerBufFunc)(void *info, PDScannerRef scanner, char **buf, PDInteger *size, PDInteger req);

/**
 Pop function signature. 
 
 @ingroup PDSCANNER
 
 The pop function is used to lex symbols out of the buffer, potentially requesting more data via the supplied PDScannerBufFunc.
 
 It defaults to a simple read-forward but can be swapped for special purpose readers on demand. It is internally swapped for the reverse lexer when the initial "startxref" value is located.
 */
typedef void(*PDScannerPopFunc)(PDScannerRef scanner);

/** @} // PDSCANNER_CONCEPT */

#include "PDType.h"

#endif
