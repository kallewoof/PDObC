//
// pd_internal.h
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
 @file pd_internal.h
 @brief Internal definitions for Pajdeg.
 
 @ingroup PDDEV
 
 @defgroup PDDEV Pajdeg Development
 
 Only interesting to people intending to improve or expand Pajdeg.
 */

#ifndef INCLUDED_pd_internal_h
#define INCLUDED_pd_internal_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PDDefines.h"
#include "PDOperator.h"

/**
 @def true 
 The truth value. 
 
 @def false
 The false value.
 */
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

/// @name Construction

/**
 Create PDObjectRef with given ID and generation number. 
 
 @note Normally this method is not called directly, but instead PDParserRef's PDParserCreateNewObject() is used.
 
 @param obid Object ID
 @param genid Generation number
 @return A detached object.
 */
extern PDObjectRef PDObjectCreate(PDInteger obid, PDInteger genid);

/// @name Private structs

/**
 The length of PDType, in pointers.
 */
#ifdef DEBUG_PDTYPES
#define PDTYPE_PTR_LEN  4
#else
#define PDTYPE_PTR_LEN  3
#endif

/**
 @cond IGNORE
 
 The PDType union. 
 
 It is force-aligned to the given size using PDTYPE_PTR_LEN.
 
 @todo Determine: Is the align done correctly? Is it even necessary?
 
 (Note: this is purposefully not doxygenified as Doxygen bugs out over the layout.)
 */
union PDType {
    struct {
#ifdef DEBUG_PDTYPES
        char *pdc;                  // Pajdeg signature
#endif
        PDInstanceType it;          // Instance type, if any.
        PDInteger retainCount;      // Retain count. If the retain count of an object hits zero, the object is disposed of.
        PDDeallocator dealloc;      // Deallocation method.
    };
    void *align[PDTYPE_PTR_LEN];    // Force-align. 
};

/** @endcond // IGNORE */

/**
 Allocate a new PDType object, with given size and deallocator.
 
 @param s Size of object.
 @param d Dealloc method as a (*)(void*).
 @param z Whether the allocated block should be zeroed or not. If true, calloc is used to allocate the memory.
 */
#define PDAlloc(s,d,z) PDAllocTyped(PDInstanceTypeUnset,s,d,z)

#ifdef DEBUG_PD_RELEASES
#define PDAllocTyped(it,s,d,z) _PDAllocTypedDebug(__FILE__,__LINE__,it,s,d,z)
extern void *_PDFocusObject;
extern void *_PDAllocTypedDebug(const char *file, int lineNumber, PDInstanceType it, PDSize size, void *dealloc, PDBool zeroed);
#else
/**
 Allocate a new PDType object with a defined instance type, with given size and deallocator.
 
 @param it Instance type.
 @param size Size of object.
 @param dealloc Dealloc method as a (*)(void*).
 @param zeroed Whether the allocated block should be zeroed or not. If true, calloc is used to allocate the memory.
 */
extern void *PDAllocTyped(PDInstanceType it, PDSize size, void *dealloc, PDBool zeroed);
#endif

/**
 Flush autorelease pool.
 */
extern void PDFlush(void);

/**
 *  Flush until the given object is encountered, or until the autorelease pool is exhausted. 
 *  The object is released as well.
 *
 *  @param ob Termination marker for flush
 */
extern void PDFlushUntil(void *ob);

/**
 Identifier for PD type checking.
 */
extern char *PDC;

/**
 *  A macro for asserting that an object is a proper PDType.
 */
#ifdef DEBUG_PDTYPES
#   define PDTYPE_ASSERT(ob) PDAssert(((PDTypeRef)ob - 1)->pdc == PDC)
#else
#   define PDTYPE_ASSERT(ob) 
#endif

#ifdef PD_SUPPORT_CRYPTO

/**
 *  Crypto instance for arrays/dicts.
 */
typedef struct PDCryptoInstance *PDCryptoInstanceRef;

extern PDCryptoInstanceRef PDCryptoInstanceCreate(pd_crypto crypto, PDInteger obid, PDInteger gennum);

/**
 *  Crypto object exchange function signature.
 *
 *  Used to hand crypto information to arbitrary objects.
 */
typedef void (*PDInstanceCryptoExchange)(void *inst, PDCryptoInstanceRef ci, PDBool encrypted);

/**
 *  Crypto object exchange function array, matching up with all non-negative PDInstanceType entries.
 */
extern PDInstanceCryptoExchange PDInstanceCryptoExchanges[];

#endif

/**
 *  Object internal structure
 *
 *  @ingroup PDOBJECT
 */

struct PDObject {
    PDInteger           obid;           ///< object id
    PDInteger           genid;          ///< generation id
    PDObjectClass       obclass;        ///< object class (regular, compressed, or trailer)
    PDObjectType        type;           ///< data structure of def below
    pd_stack            def;            ///< the object content
    void               *inst;           ///< instance of def, or NULL if not yet instantiated
    PDBool              hasStream;      ///< if set, object has a stream
    PDInteger           streamLen;      ///< length of stream (if one exists)
    PDInteger           extractedLen;   ///< length of extracted stream; -1 until stream has been fetched via the parser
    char               *streamBuf;      ///< the stream, if fetched via parser, otherwise an undefined value
    PDBool              skipStream;     ///< if set, even if an object has a stream, the stream (including keywords) is skipped when written to output
    PDBool              skipObject;     ///< if set, entire object is discarded
    PDBool              deleteObject;   ///< if set, the object's XREF table slot is marked as free
    char               *ovrStream;      ///< stream override
    PDInteger           ovrStreamLen;   ///< length of ^
    PDBool              ovrStreamAlloc; ///< if set, ovrStream will be free()d by the object after use
    char               *ovrDef;         ///< definition override
    PDInteger           ovrDefLen;      ///< take a wild guess
    PDBool              encryptedDoc;   ///< if set, the object is contained in an encrypted PDF; if false, PDObjectSetStreamEncrypted is NOP
    char               *refString;      ///< reference string, cached from calls to 
    PDSynchronizer      synchronizer;   ///< synchronizer callback, called right before the object is serialized and written to the output stream
    const void         *syncInfo;       ///< user info object for synchronizer callback (usually a class instance, for wrappers)
#ifdef PD_SUPPORT_CRYPTO
    pd_crypto           crypto;         ///< crypto object, if available
    PDCryptoInstanceRef cryptoInstance; ///< crypto instance, if set up
#endif
};

//
// object stream
//

typedef struct PDObjectStreamElement *PDObjectStreamElementRef;

/**
 @addtogroup PDOBJECTSTREAM

 @{ 
 */

/**
 Object stream element structure
 
 This is a wrapper around an object inside of an object stream.
 */
struct PDObjectStreamElement {
    PDInteger obid;                     ///< object id of element
    PDInteger offset;                   ///< offset inside object stream
    PDInteger length;                   ///< length of the (stringified) definition; only valid during a commit
    PDObjectType type;                  ///< element object type
    void *def;                          ///< definition; NULL if a construct has been made for this element
};

/**
 Object stream internal structure
 
 The object stream is an object in a PDF which itself contains a stream of objects in a specially formatted form.
 */
struct PDObjectStream {
    PDObjectRef ob;                     ///< obstream object
    PDInteger n;                        ///< number of objects
    PDInteger first;                    ///< first object's offset
    PDStreamFilterRef filter;           ///< filter used to extract the initial raw content
    PDObjectStreamElementRef elements;  ///< n sized array of elements (non-pointered!)
    PDSplayTreeRef constructs;              ///< instances of objects (i.e. constructs)
};

/**
 Createa an object stream with the given object.
 
 @param object Object whose stream is an object stream.
 */
extern PDObjectStreamRef PDObjectStreamCreateWithObject(PDObjectRef object);

/**
 Parse the raw object stream rawBuf and set up the object stream structure.
 
 If the object has a defined filter, the object stream decodes the content before parsing it.
 
 @param obstm The object stream.
 @param rawBuf The raw buffer.
 @return True if object stream was parsed successfully, false if an error occurred.
 */
extern PDBool PDObjectStreamParseRawObjectStream(PDObjectStreamRef obstm, char *rawBuf);

/**
 Parse the extracted object stream and set up the object stream structure.
 
 This is identical to PDObjectStreamParseRawObjectStream except that this method presumes that decoding has been done, if necessary.
 
 @param obstm The object stream.
 @param buf The buffer.
 */
extern void PDObjectStreamParseExtractedObjectStream(PDObjectStreamRef obstm, char *buf);

/**
 Commit an object stream to its associated object. 
 
 If changes to an object in an object stream are made, they are not automatically reflected. 
 */
extern void PDObjectStreamCommit(PDObjectStreamRef obstm);

/** @} */

/**
 @addtogroup PDCONTENTSTREAM
 
 @{ 
 */

/**
 Content stream internal structure
 
 The content stream is a simple wrapper around an object, with additional support for PDF operators (the ones used to draw stuff on screen, for example).
 */
struct PDContentStream {
    PDSplayTreeRef opertree;            ///< operator tree
    PDArrayRef args;                    ///< pending operator arguments
    pd_stack opers;                     ///< current operators stack
    pd_stack deallocators;              ///< deallocator (func, userInfo) pairs -- called when content stream object is about to be destroyed
    pd_stack resetters;                 ///< resetter (func, userInfo) pairs -- called at the end of every execution call; these adhere to the PDDeallocator signature
    const char *lastOperator;           ///< the last operator that was encountered
};

/** @} */

/**
 @addtogroup PDPAGE
 
 @{ 
 */

/**
 Page internal structure
 */
struct PDPage {
    PDObjectRef  ob;           ///< the /Page object
    PDParserRef  parser;       ///< the parser associated with the owning PDF document
    PDInteger    contentCount; ///< # of content objects in the page
    PDArrayRef   contentRefs;  ///< array of content references for the page
    PDObjectRef *contentObs;   ///< array of content objects for the page, or NULL if unfetched
    PDFontDictionaryRef fontDict; ///< The font dictionary for the page; lazily constructed on first call to PDPageGetFont().
};

/** @} */


/// @name Environment

/**
 PDState wrapping structure
 */
struct PDEnv {
    PDStateRef    state;            ///< The wrapped state.
    pd_stack      buildStack;       ///< Build stack (for sub-components)
    pd_stack      varStack;         ///< Variable stack (for incomplete components)
    //PDInteger     entryOffset;     
};

/**
 Destroy an environment
 
 @param env The environment
 */
extern void PDEnvDestroy(PDEnvRef env);

/// @name Binary tree

/**
 Binary tree structure
 */
struct pd_btree {
    PDInteger key;                  ///< The (primitive) key.
    void *value;                    ///< The value.
    //PDInteger balance;              
    PDSplayTreeRef branch[2];           ///< The left and right branches of the tree.
};

/// @name Operator

/**
 The PDperator internal structure
 */
struct PDOperator {
    PDOperatorType   type;          ///< The operator type

    union {
        PDStateRef   pushedState;   ///< for "PushNewEnv", this is the environment being pushed
        char        *key;           ///< the argument to the operator, for PopVariable, Push/StoveComplex, PullBuildVariable
        PDID         identifier;    ///< identifier (constant string pointer pointer)
    };
    
    PDOperatorRef    next;          ///< the next operator, if any
};

/// @name Parser

/**
 PDXTable
 
 @ingroup PDXTABLE
 */
typedef struct PDXTable *PDXTableRef;

/**
 The state of a PDParser instance. 
 */
typedef enum {
    PDParserStateBase,              ///< parser is in between objects
    PDParserStateObjectDefinition,  ///< parser is right after 1 2 obj and right before whatever the object consists of
    PDParserStateObjectAppendix,    ///< parser is right after the object's content, and expects to see endobj or stream next
    PDParserStateObjectPostStream,  ///< parser is right after the endstream keyword, at the endobj keyword
} PDParserState;

/**
 The PDParser internal structure.
 */
struct PDParser {
    PDTwinStreamRef stream;         ///< The I/O stream from the pipe
    PDScannerRef scanner;           ///< The main scanner
    PDParserState state;            ///< The parser state
    
    // xref related
    pd_stack xstack;                ///< A stack of partial xref tables based on offset; see [1] below
    PDXTableRef mxt;                ///< master xref table, used for output
    PDXTableRef cxt;                ///< current input xref table
    PDBool done;                    ///< parser has passed the last object in the input PDF
    PDSize xrefnewiter;             ///< iterator for locating unused id's for usage in master xref table
    
    // object related
    pd_stack appends;               ///< stack of objects that are meant to be appended at the end of the PDF
    pd_stack inserts;               ///< stack of objects that are meant to be inserted as soon as the current object is dealt with
    PDSplayTreeRef aiTree;              ///< bin-tree identifying the (in-memory) PDObjectRefs in appends and inserts by their object ID's
    PDObjectRef construct;          ///< cannot be relied on to contain anything; is used to hold constructed objects until iteration (at which point they're released)
    PDSize streamLen;               ///< stream length of the current object
    PDSize obid;                    ///< object ID of the current object
    PDSize genid;                   ///< generation number of the current object
    PDSize oboffset;                ///< offset of the current object
    
    // document-wide stuff
    PDReferenceRef rootRef;         ///< reference to the root object
    PDReferenceRef infoRef;         ///< reference to the info object
    PDReferenceRef encryptRef;      ///< reference to the encrypt object
    PDObjectRef trailer;            ///< the trailer object
    PDObjectRef root;               ///< the root object, if instantiated
    PDObjectRef info;               ///< the info object, if instantiated
    PDObjectRef encrypt;            ///< the encrypt object, if instantiated
    PDCatalogRef catalog;           ///< the root catalog, if instantiated
    pd_crypto crypto;               ///< crypto instance, if the document is encrypted
    
    // miscellaneous
    PDBool success;                 ///< if true, the parser has so far succeeded at parsing the input file
    PDSplayTreeRef skipT;           ///< whenever an object is ignored due to offset discrepancy, its ID is put on the skip tree; when the last object has been parsed, if the skip tree is non-empty, the parser aborts, as it means objects were lost
    PDFontDictionaryRef mfd;        ///< Master font dictionary, containing all fonts processed so far
};

/**
 The PDPageReference internal structure.
 */
typedef struct PDPageReference PDPageReference;
struct PDPageReference {
    PDBool collection;              ///< If set, this is a /Type /Pages object, which is a group of page and pages references
    union {
        struct {
            PDInteger count;        ///< Number of entries
            PDPageReference *kids;           ///< Kids
        };
        struct {
            PDInteger obid;         ///< The object ID
            PDInteger genid;        ///< The generation ID
        };
    };
};

/**
 The PDCatalog internal structure.
 */
struct PDCatalog {
    PDParserRef parser;             ///< The parser owning the catalog
    PDObjectRef object;             ///< The object representation of the catalog
    PDRect      mediaBox;           ///< The media box of the catalog object
    PDPageReference pages;          ///< The root pages
    PDInteger   count;              ///< Number of pages (in total)
    PDInteger   capacity;           ///< Size of kids array.
    PDInteger  *kids;               ///< Array of object IDs for all pages
};

/// @name Scanner

typedef struct PDScannerSymbol *PDScannerSymbolRef;

/**
 Scanner symbol type.
 */
typedef enum {
    PDScannerSymbolTypeDefault    = PDOperatorSymbolGlobRegular,    ///< standard symbol
    PDScannerSymbolTypeWhitespace = PDOperatorSymbolGlobWhitespace, ///< PDF whitespace character
    PDScannerSymbolTypeDelimiter  = PDOperatorSymbolGlobDelimiter,  ///< PDF delimiter character
    PDScannerSymbolTypeNumeric    = PDOperatorSymbolExtNumeric,     ///< a numeric symbol
    PDScannerSymbolTypeEOB        = PDOperatorSymbolExtEOB,         ///< end of buffer marked
    PDScannerSymbolTypeFake       = PDOperatorSymbolExtFake,        ///< fake symbol, which is when sstart is actually a real string, rather than a pointer into the stream buffer
} PDScannerSymbolType;

/**
 A scanner symbol.
 */
struct PDScannerSymbol {
    char         *sstart;       ///< symbol start
    short         shash;        ///< symbol hash (not normalized)
    PDInteger     slen;         ///< symbol length
    PDScannerSymbolType stype;  ///< symbol type
};

/**
 The internal scanner structure.
 */
struct PDScanner {
    PDEnvRef   env;             ///< the current environment
    PDScannerBufFunc bufFunc;   ///< buffer function
    void *bufFuncInfo;          ///< buffer function info object
    pd_stack contextStack;      ///< context stack for buffer function/info
    
    pd_stack envStack;          ///< environment stack; e.g. root -> arb -> array -> arb -> ...
    pd_stack resultStack;       ///< results stack
    pd_stack symbolStack;       ///< symbols stack; used to "rewind" when misinterpretations occur (e.g. for "number_or_obref" when one or two numbers)
    pd_stack garbageStack;      ///< temporary allocations; only used in operator function when a symbol is regenerated from a malloc()'d string
    
    PDStreamFilterRef filter;   ///< filter, if any
    
    char         *buf;          ///< buffer
    PDInteger     bresoffset;   ///< previously popped result's offset relative to buf
    PDInteger     bsize;        ///< buffer capacity
    PDInteger     boffset;      ///< buffer offset (we are at position &buf[boffset]
    PDInteger     bmark;        ///< buffer mark
    PDScannerSymbolRef sym;     ///< the latest symbol
    PDScannerPopFunc popFunc;   ///< the symbol pop function
    PDBool        fixedBuf;     ///< if set, the buffer is fixed (i.e. buffering function should not be called)
    PDBool        failed;       ///< if set, the scanner aborted due to a failure
    PDBool        outgrown;     ///< if true, a scanner with fixedBuf set needed more data
    PDBool        strict;       ///< if true, the scanner will complain loudly when erroring out, otherwise it will silently fail
};

/// @name Stack

typedef enum {
    PD_STACK_STRING = 0,        ///< Stack string type
    PD_STACK_ID     = 1,        ///< Stack identifier type
    PD_STACK_STACK  = 2,        ///< Stack stack type
    PD_STACK_PDOB   = 3,        ///< Stack object (PDTypeRef managed) type
    PD_STACK_FREEABLE = 4       ///< Stack freeable type
} pd_stack_type;

/**
 The internal stack structure
 */
struct pd_stack {
    pd_stack      prev;         ///< Previous object in stack
    pd_stack_type type;         ///< Stack type
    void         *info;         ///< The stack content, based on its type
};


///**
// "Get string object for key" signature for arrays/dictionaries. (Arrays pass integers as keys.)
// */
//typedef const char *(*_list_getter)(void *ref, const void *key);
//
///**
// "Get raw object for key" signature for arrays/dictionaries. (Arrays pass integers as keys.)
// */
//typedef const pd_stack (*_list_getter_raw)(void *ref, const void *key);
//
///**
// "Remove object for key" signature for arrays/dictionaries. (Arrays pass integers as keys.)
// */
//typedef void (*_list_remover)(void *ref, const void *key);
//
///**
// "Set object for key to value" signature for arrays/dictionaries.
// */
//typedef PDInteger (*_list_setter)(void *ref, const void *key, const char *value);
//
///**
// "Make room at index" signature for arrays.
// */
//typedef void (*_list_push_index)(void *ref, PDInteger index);
//
///**
// The internal array structure.
// */
//struct pd_array {
//    PDInteger        count;     ///< Number of elements
//    PDInteger        capacity;  ///< Capacity of array
//    char           **values;    ///< Content
//    pd_stack        *vstacks;   ///< Values in pd_stack form
//    _list_getter     g;         ///< Getter
//    _list_getter_raw rg;        ///< Raw getter
//    _list_setter     s;         ///< Setter
//    _list_remover    r;         ///< Remover
//    _list_push_index pi;        ///< Push-indexer
//    void            *info;      ///< Info object (used for encrypted arrays)
//};
//
///**
// The internal dictionary structure.
// */
//struct pd_dict {
//    PDInteger        count;     ///< Number of entries
//    PDInteger        capacity;  ///< Capacity of dictionary
//    char           **keys;      ///< Keys
//    char           **values;    ///< Values
//    pd_stack        *vstacks;   ///< Values in pd_stack form
//    _list_getter     g;         ///< Getter
//    _list_getter_raw rg;        ///< Raw getter
//    _list_setter     s;         ///< Setter
//    _list_remover    r;         ///< Remover
//    void            *info;      ///< Info object (used for encrypted arrays)
//};

/**
 The internal array structure.
 */
struct PDArray {
    PDInteger        count;     ///< Number of elements
    PDInteger        capacity;  ///< Capacity of array
    void           **values;    ///< Resolved values
    pd_stack        *vstacks;   ///< Unresolved values in pd_stack form
#ifdef PD_SUPPORT_CRYPTO
    PDCryptoInstanceRef ci;     ///< Crypto instance, if array is encrypted
#endif
};

typedef struct PDDictionaryNode *PDDictionaryNodeRef;
struct PDDictionaryNode {
    char            *key;       ///< the key for this node
    void            *data;      ///< the data
#ifdef PD_SUPPORT_CRYPTO
    void            *decrypted; ///< the data, in decrypted form
#endif
    PDSize           hash;      ///< the hash code
};

/**
 *  If set, profiling is done (and printed to stdout occasionally) about how well the hash map is performing
 */
//#define PDHM_PROF

/**
 The internal dictionary structure.
 */
struct PDDictionary {
    PDInteger        count;     ///< Number of entries
#ifdef PDHM_PROF
    PDInteger        maxCount;  ///< Max entries seen in this dictionary
#endif
    PDInteger        bucketc;   ///< Number of buckets
    PDInteger        bucketm;   ///< Bucket mask
    PDArrayRef      *buckets;   ///< Buckets containing content
    PDArrayRef       populated; ///< Array of buckets which were created (as opposed to remaining NULL due to index never being touched)
#ifdef PD_SUPPORT_CRYPTO
    PDCryptoInstanceRef ci;     ///< Crypto instance, if dictionary is encrypted
#endif
};

/**
 The internal dictionary stack structure.
 */
struct PDDictionaryStack {
    pd_stack dicts;             ///< Stack of PDDictionaries
};

/**
 *  The internal font dictionary structure.
 */
struct PDFontDictionary {
    PDParserRef     parser;     ///< The owning parser
    PDDictionaryRef fonts;      ///< Dictionary mapping font names to their values
    PDDictionaryRef encodings;  ///< Encoding dictionary
};

/**
 *  The internal font object structure.
 */
struct PDFont {
    PDParserRef     parser;     ///< Parser reference
    PDFontDictionaryRef fontDict;///< The owning font dictionary
    PDObjectRef     obj;        ///< Font object reference
    PDCMapRef       toUnicode;  ///< CMap, or NULL if not yet compiled or if non-existent
    PDStringEncoding enc;       ///< String encoding, or NULL if not a string encoding
    unsigned char  *encMap;     ///< Encoding map, a 256 byte array mapping bytes to bytes
};

typedef struct PDCMapRange PDCMapRange;                 ///< Private structure for ranges (<hex> <hex>), where multi-byte ranges are rectangles, not sequences (see PDF specification)
typedef struct PDCMapRangeMapping PDCMapRangeMapping;   ///< Private structure for range mappings (<hex> <hex> <hex>)
typedef struct PDCMapCharMapping PDCMapCharMapping;     ///< Private structure for individual char mappings (<hex> <hex>)

/**
 *  The internal CID map object structure.
 */
struct PDCMap {
    PDDictionaryRef systemInfo; ///< The CIDSystemInfo dictionary, which has /Registry, /Ordering, and /Supplement
    PDStringRef     name;       ///< CMapName
    PDNumberRef     type;       ///< CMapType
    PDSize          csrCap;     ///< Codespace range capacity
    PDSize          bfrCap;     ///< BF range capacity
    PDSize          bfcCap;     ///< BF char capacity
    PDSize          csrCount;   ///< Number of codespace ranges
    PDSize          bfrCount;   ///< Number of BF ranges
    PDSize          bfcCount;   ///< Number of BF chars
    PDSize          bfcLength;  ///< Length (in bytes) of input characters in BF ranges
    PDCMapRange    *csrs;       ///< Array of codespace ranges
    PDCMapRangeMapping *bfrs;   ///< BF ranges
    PDCMapCharMapping  *bfcs;   ///< BF chars
};

#ifdef PD_SUPPORT_CRYPTO

/**
 Crypto instance for arrays/dicts.
 */
struct PDCryptoInstance {
    pd_crypto crypto;           ///< Crypto object.
    PDInteger obid;             ///< Associated object ID.
    PDInteger genid;            ///< Associated generation number.
};

/**
 The internal crypto structure.
 */
struct pd_crypto {
    // common values
    PDStringRef identifier;     ///< PDF /ID found in the trailer dictionary
    PDStringRef filter;         ///< filter name
    PDStringRef subfilter;      ///< sub-filter name
    PDInteger version;          ///< algorithm version (V key in PDFs)
    PDInteger length;           ///< length of the encryption key, in bits; must be a multiple of 8 in the range 40 - 128; default = 40
    
    // standard security handler 
    PDInteger revision;         ///< revision ("R") of algorithm: 2 if version < 2 and perms have no 3 or greater values, 3 if version is 2 or 3, or P has rev 3 stuff, 4 if version = 4
    PDStringRef owner;          ///< owner string ("O"), 32-byte string based on owner and user passwords, used to compute encryption key and determining whether a valid owner password was entered
    PDStringRef user;           ///< user string ("U"), 32-byte string based on user password, used in determining whether to prompt the user for a password and whether given password was a valid user or owner password
    int32_t privs;              ///< privileges (see Table 3.20 in PDF spec v 1.7, p. 123-124)
    PDBool encryptMetadata;     ///< whether metadata should be encrypted or not ("/EncryptMetadata true")
    PDStringRef enckey;         ///< encryption key
    
    // standard crypt filter
    PDInteger cfLength;         ///< crypt filter length, e.g. 16 for AESV2
    pd_crypto_method cfMethod;  ///< crypt filter method
    pd_auth_event cfAuthEvent;  ///< when authentication occurs; currently only supports '/DocOpen'
};

#else

struct pd_crypto {
    
};

#endif

/// @name State

/**
 The internal PDState structure
 */
struct PDState {
    PDBool         iterates;    ///< if true, scanner will stop while in this state, after reading one entry
    char          *name;        ///< name of the state
    char         **symbol;      ///< symbol strings
    PDInteger      symbols;     ///< number of symbols in total
    
    PDInteger     *symindex;    ///< symbol indices (for hash)
    short          symindices;  ///< number of index slots in total (not = `symbols`, often bigger)
    
    PDOperatorRef *symbolOp;    ///< symbol operators
    PDOperatorRef  numberOp;    ///< number operator
    PDOperatorRef  delimiterOp; ///< delimiter operator
    PDOperatorRef  fallbackOp;  ///< fallback operator
};

/// @name Static Hash

/**
 The internal static hash structure
 */
struct PDStaticHash {
    PDInteger entries;          ///< Number of entries in static hash
    PDInteger mask;             ///< The mask
    PDInteger shift;            ///< The shift
    PDBool    leaveKeys;        ///< if set, the keys are not deallocated on destruction; default = false (i.e. dealloc keys)
    PDBool    leaveValues;      ///< if set, the values are not deallocated on destruction; default = false (i.e. dealloc values)
    void    **keys;             ///< Keys array
    void    **values;           ///< Values array
    void    **table;            ///< The static hash table
};

/// @name Tasks

/**
 The internal task structure
 */
struct PDTask {
    PDBool          isActive;       ///< Whether task is still active; sometimes tasks cannot be unloaded properly even though the task returned PDTaskUnload; these tasks have their active flag unset instead
    PDBool          isFilter;       ///< Whether task is a filter or not. Internally, a task is only a filter if it is assigned to a specific object ID or IDs.
    PDPropertyType  propertyType;   ///< The filter property type
    PDInteger       value;          ///< The filter value, if any
    PDTaskFunc      func;           ///< The function callback, if the task is not a filter
    PDTaskRef       child;          ///< The task's child task; child tasks are called in order.
    PDDeallocator   deallocator;    ///< The deallocator for the task.
    void           *info;           ///< The (user) info object.
};

/// @name Twin streams

/**
 The internal twin stream structure
 */
struct PDTwinStream {
    PDTwinStreamMethod method;      ///< The current method
    PDScannerRef scanner;           ///< the master scanner
    
    FILE    *fi;                    ///< Reader
    FILE    *fo;                    ///< writer
    fpos_t   offsi;                 ///< absolute offset in input for heap
    fpos_t   offso;                 ///< absolute offset in output for file pointer
    
    char    *heap;                  ///< heap in which buffer is located
    PDSize   size;                  ///< size of heap
    PDSize   holds;                 ///< bytes in heap
    PDSize   cursor;                ///< position in heap (bytes 0..cursor have been written (unless discarded) to output)
    
    char    *sidebuf;               ///< temporary buffer (e.g. for Fetch)
    
    PDBool   outgrown;              ///< if true, a buffer with growth disallowed attempted to grow and failed
};

/**
 Internal structure.
 
 @ingroup PDPIPE
 */
struct PDPipe {
    PDBool          opened;             ///< Whether pipe has been opened or not
    PDBool          dynamicFiltering;   ///< Whether dynamic filtering is necessary; if set, the static hash filtering of filters is skipped and filters are checked for all objects
    PDBool          typedTasks;         ///< Whether type tasks (excluding unfiltered tasks) are activated; activation results in a slight decrease in performance due to all dictionary objects needing to be resolved in order to check their Type dictionary key
    char           *pi;                 ///< The path of the input file
    char           *po;                 ///< The path of the output file
    FILE           *fi;                 ///< Reader
    FILE           *fo;                 ///< Writer
    PDInteger       filterCount;        ///< Number of filters in the pipe
    PDTwinStreamRef stream;             ///< The pipe stream
    PDParserRef     parser;             ///< The parser
    PDSplayTreeRef      filter;             ///< The filters, in a tree with the object ID as key
    pd_stack
    typeTasks[_PDFTypeCount];           ///< Tasks which run depending on all objects of the given type; the 0'th element (type NULL) is triggered for all objects, and not just objects without a /Type dictionary key
    PDSplayTreeRef      attachments;        ///< PDParserAttachment entries
};

extern void PDPipeCloseFileStream(FILE *stream);
extern FILE *PDPipeOpenInputStream(const char *path);
extern FILE *PDPipeOpenOutputStream(const char *path);

/// @name Reference

/**
 Internal reference structure
 
 @ingroup PDREFERENCE
 */
struct PDReference {
    PDInteger obid;         ///< The object ID
    PDInteger genid;        ///< The generation number
};

/// @name Number

/**
 Internal number structure.
 
 @ingroup PDNUMBER
 */
struct PDNumber {
    PDObjectType type;      ///< Type of the number; as a special case, PDObjectTypeReference is used for pointers
    union {
        PDInteger i;
        PDReal r;
        PDBool b;
        PDSize s;
        void  *p;
    };
};

/// @name String

/**
 Internal string structure.
 
 @ingroup PDSTRING
 */
struct PDString {
    PDStringType type;      ///< Type of the string
    PDFontRef font;         ///< The font associated with the string
    PDStringEncoding enc;   ///< Encoding of the string
    PDSize length;          ///< Length of the string
    PDBool wrapped;         ///< Whether the string is wrapped
    char *data;             ///< Buffer containing string data
    PDStringRef alt;        ///< Alternative representation
#ifdef PD_SUPPORT_CRYPTO
    PDCryptoInstanceRef ci; ///< Crypto instance
    PDBool encrypted;       ///< Flag indicating whether the string is encrypted or not; the value of this flag is UNDEFINED if ci == NULL
#endif
};

extern void PDStringAttachCryptoInstance(PDStringRef string, PDCryptoInstanceRef ci, PDBool encrypted);
extern void PDArrayAttachCryptoInstance(PDArrayRef array, PDCryptoInstanceRef ci, PDBool encrypted);
extern void PDDictionaryAttachCryptoInstance(PDDictionaryRef dictionary, PDCryptoInstanceRef ci, PDBool encrypted);
extern void PDDictionaryAttachCryptoInstance(PDDictionaryRef hm, PDCryptoInstanceRef ci, PDBool encrypted);

/// @name Conversion (PDF specification)

typedef struct PDStringConv *PDStringConvRef;

/**
 Internal string conversion structure
 */
struct PDStringConv {
    char *allocBuf;         ///< The allocated buffer
    PDInteger offs;         ///< The current offset inside the buffer
    PDInteger left;         ///< The current bytes left (remaining) in the buffer
};

/// @name Macros / convenience

/**
 Pajdeg definition list.
 */
#define PDDEF const void*[]

/**
 Wrapper for null terminated definitions.
 */
#define PDDef(defs...) (PDDEF){(void*)defs, NULL}


/**
 @def PDError
 Print an error to stderr, if user has turned on PD_WARNINGS.
 
 @param args Formatted variable argument list.
 */
#ifndef PDError
#   if defined(DEBUG) || defined(PD_WARNINGS)
#       define PD_WARNINGS
extern void _PDBreak();
#       define PDError(args...) do {\
            fprintf(stderr, "[pajdeg::error] %s:%d - ", __FILE__,__LINE__); \
            fprintf(stderr, args); \
            fprintf(stderr, "\n"); \
            _PDBreak();\
        } while (0)
#   else
#       define PDError(args...) 
#   endif
#endif

/**
 @def PDWarn
 Print a warning to stderr, if user has turned on PD_WARNINGS.
 
 @param args Formatted variable argument list.
 */
#ifndef PDWarn
#   if defined(DEBUG) || defined(PD_WARNINGS)
#       define PD_WARNINGS
#       define PDWarn(args...) do { \
            fprintf(stderr, "[pajdeg::warning] %s:%d - ", __FILE__,__LINE__); \
            fprintf(stderr, args); \
            fprintf(stderr, "\n"); \
        } while (0)
#   else
#       define PDWarn(args...) 
#   endif
#endif

/**
 @def PDNotice
 Print an informational message (a "weak" warning) to stderr, if user has turned on PD_NOTICES.
 
 @param args Formatted variable argument list.
 */
#ifndef PDNotice
#   if defined(PD_NOTICES)
#       define PDNotice(args...) do { \
            fprintf(stderr, "[pajdeg::notice]  %s:%d - ", __FILE__,__LINE__); \
            fprintf(stderr, args); \
            fprintf(stderr, "\n"); \
        } while (0)
#   else
#       define PDNotice(args...) 
#   endif
#endif

/**
 @def PDAssert
 Assert that expression is non-false. 
 
 If PD_WARNINGS is set, prints out the expression to stderr along with "assertion failure", then re-asserts expression using stdlib's assert()
 
 If PD_WARNINGS is unset, simply re-asserts expression using stdlib's assert().
 
 @param args Expression which must resolve to non-false (i.e. not 0, not nil, not NULL, etc).
 */
#ifndef PDAssert
#   if defined(DEBUG) || defined(PD_ASSERTS)
#       include <assert.h>
#       if defined(PD_WARNINGS)
#           define PDAssert(args...) \
                if (!(args)) { \
                    PDWarn("assertion failure : %s", #args); \
                    assert(args); \
                }
#       else
#           define PDAssert(args...) assert(args)
#       endif
#   else
#       define PDAssert(args...) 
#   endif
#endif

/**
 @def PDRequire
 Require that the given state is true, and print out msg (format string), and return retval if it is not.
 In addition, if asserts are enabled, throw an assertion. The difference between this and PDAssert is that
 this code is guaranteed to abort the operation even if in a production environment, whereas PDAssert will
 be silently ignored for !DEBUG && !PD_ASSERTS.
 */
#define PDRequire(state, retval, msg...) \
        if (!(state)) { \
            PDWarn("requirement failure : " msg); \
            PDAssert(state); \
            return retval; \
        }

/**
 Macro for making casting of types a bit less of an eyesore. 
 
 as(PDInteger, stack->info)->prev is the same as ((PDInteger)(stack->info))->prev
 
 @param type The cast-to type
 @param expr The expression that should be cast
 */
#define as(type, expr...) ((type)(expr))

#ifdef DEBUG
/**
 Perform assertions related to the twin stream's internal state.
 
 @param ts The twin stream.
 */
extern void PDTwinStreamAsserts(PDTwinStreamRef ts);
#else 
#   define PDTwinStreamAsserts(ts) 
#endif

#define PDInstancePrinterRequire(b, r) \
    if (*cap - offs < b) { \
        *cap += r; \
        *buf = realloc(*buf, *cap); \
    }

#define PDInstancePrinterInit(itype, b, r) \
    itype i = (itype) inst;\
    PDInstancePrinterRequire(b, r)

/**
 @def fmatox(x, ato)
 
 Fast mutative atoXXX inline function generation macro.
 
 @param x Function return type.
 @param ato Method
 */
#define fmatox(x, ato) \
static inline x fast_mutative_##ato(char *str, PDInteger len) \
{ \
    char t = str[len]; \
    str[len] = 0; \
    x l = ato(str); \
    str[len] = t; \
    return l; \
}

fmatox(long long, atoll)
fmatox(long, atol)

#endif
