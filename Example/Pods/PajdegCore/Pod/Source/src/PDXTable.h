//
// PDXTable.h
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
 @file PDXTable.h PDF XRef table header.
 
 @ingroup PDXTABLE
 
 @defgroup PDXTABLE PDXTable
 
 @brief PDF XRef (cross reference) table.
 
 @ingroup PDINTERNAL
 
 @{
 */


#ifndef INCLUDED_PDXTable_h
#define INCLUDED_PDXTable_h

#include <sys/types.h>
#include "PDDefines.h"

/**
 The XREF format, which can be one of text or binary. 
 
 Text XREFs have the appearance
 @code
 xref 3 0
 000000029 00000 n
 000000142 00000 n
 000013023 65535 f
 @endcode
 while binary XREFs are binary values stored in an object with /Type /XRefStm.
 */
typedef enum {
    PDXTableFormatText   = 0, ///< PDF-1.4 and below, using 20-byte leading zero space delimited entries
    PDXTableFormatBinary = 1, ///< PDX-1.5 and forward, using variable width object stream format
} PDXFormat;

/**
 An XREF entry's type, which is one of freed (unused), used, or compressed.
 
 Compressed objects are located inside of an object stream.
 */
enum {
    PDXTypeFreed    = 0,    ///< Freed object ('f' for text format entries)
    PDXTypeUsed     = 1,    ///< Used object ('n' for text format entries)
    PDXTypeComp     = 2,    ///< Compressed object (no text format equivalent)
};

typedef unsigned char PDXType;      ///< The type representation of an XREF entry.
//typedef u_int32_t PDXOffsetType;    ///< The offset representation of an XREF entry.
//typedef unsigned char PDXGenType;   ///< The generation / index representation of an XREF entry.

//#define PDXTypeSize     1   ///< The size of the XREF type. Note: this must not be changed
//#define PDXOffsSize     4   ///< The size of the offset type. Note: changing this requires updating PDXGet/SetOffsetForID
//#define PDXGenSize      1   ///< The generation number / object index type size. Note: this must not be changed

//#define PDXTypeAlign    0                               ///< The alignment of the XREF type inside its XREF field
//#define PDXOffsAlign    PDXTypeSize                     ///< The alignment of the offset type inside its XREF field
//#define PDXGenAlign     (PDXOffsAlign + PDXOffsSize)    ///< The alignment of the generation number inside its XREF field
//#define PDXWidth        (PDXGenAlign + PDXGenSize)      ///< The width of an XREF field

// Why can't I make this use the quoted value of the #defines above...?
//#define PDXWEntry       "[ 1 4 1 ]" ///< The PDF array expression for the sizes above (a [ followed by a the numbers in order followed by a ])

/**
 PDF XRef (cross reference) table
 */
struct PDXTable {
    PDInteger   allocx;
    PDInteger   obid;       ///< object containing this XRef, if binary (text XRefs are not proper objects)
    char       *xrefs;      ///< XRef entries stored as a chunk of memory
    
    PDXFormat   format;     ///< Original format of this entry, which can be text (PDF 1.4-) or binary (PDF 1.5+). Internally, there is no difference to how the data is maintained, but Pajdeg will use the same format used in the original in its own output.
    PDBool      linearized; ///< If set, unexpected XREF entries in the PDF are silently ignored by the parser.
    PDSize      cap;        ///< Capacity of the table's xrefs buffer, in entries.
    PDSize      count;      ///< Number of objects held by the XRef.
    PDSize      pos;        ///< Byte-wise position in the PDF where the XRef (and subsequent trailer, if text format) begins; reaching this point means the XRef ceases to apply
    
    PDXTableRef prev;       ///< previous (older) table (mostly debug related)
    PDXTableRef next;       ///< next (newer) table (mostly debug related)
    
    PDOffset    offsCap;    ///< threshold for offsets using current offsSize

    unsigned char typeSize;   ///< type size, current implementation requires this to be 1
    unsigned char offsSize;   ///< offset size
    unsigned char genSize;    ///< gen ID size
    unsigned char typeAlign;  ///< type align
    unsigned char offsAlign;  ///< offset align
    unsigned char genAlign;   ///< gen ID align
    unsigned char width;      ///< width of table entry
    
    PDArrayRef w;             ///< The W entry, if set.
};

/**
 Get the offset for the object with the given ID in the XREF table.
 
 @param table The PDXTable instance.
 @param obid The object ID.
 */
extern PDOffset PDXTableGetOffsetForID(PDXTableRef table, PDInteger obid);

/**
 Set the offset for the object with the given ID to the given offset in the XREF table.
 
 @param table The PDXTable instance.
 @param obid The object ID.
 @param offset The new offset.
 */
extern void PDXTableSetOffsetForID(PDXTableRef table, PDInteger obid, PDOffset offset);

/**
 Get the type for the given object.
 
 @param xrefs The XREF buffer
 @param id The object ID.
 */
#define PDXTableGetTypeForID(table, id)         (PDXType)((table->xrefs)[id*table->width])
//extern PDInteger PDXTableGetTypeForID(PDXTableRef table, PDInteger obid);

/**
 Set the type for the given object.
 
 @param xrefs The XREF buffer
 @param id The object ID.
 @param t The new type.
 */
#define PDXTableSetTypeForID(table, id, t)    *(PDXType*)&((table->xrefs)[id*table->width]) = t
//extern void PDXTableSetTypeForID(PDXTableRef table, PDInteger obid, PDInteger type);

/**
 Get the generation number or object stream index for the given object.
 
 @param xrefs The XREF buffer
 @param id The object ID.
 */
extern PDInteger PDXTableGetGenForID(PDXTableRef table, PDInteger obid);
//#define PDXGetGenForID(xrefs, id)         (PDXGenType)((xrefs)[PDXGenAlign+id*PDXWidth])

/**
 Debug
 */
//static inline PDXGenType _PDXGetGenForID(char *xrefs, PDInteger obid)
//{
//    return (PDXGenType)((xrefs)[PDXGenAlign+obid*PDXWidth]);
//}

/**
 Set the generation number / object stream index for the given object.
 
 @param xrefs The XREF buffer
 @param id The object ID.
 @param gen The new value.
 */
extern void PDXTableSetGenForID(PDXTableRef table, PDInteger obid, PDInteger gen);
//#define PDXSetGenForID(xrefs, id, gen)   *(PDXGenType*)&((xrefs)[PDXGenAlign+id*PDXWidth]) = gen

///**
// Get the type for the given object in the table.
// 
// @param xtable The PDXTableRef instance
// @param id The object ID.
// */
//#define PDXTableGetTypeForID(xtable, id)       PDXGetTypeForID(xtable->xrefs, id)
//
///**
// Set the type for the given object in the table.
// 
// @param xtable The PDXTableRef instance
// @param id The object ID.
// @param t The new type.
// */
//#define PDXTableSetTypeForID(xtable, id, t)     PDXSetTypeForID(xtable->xrefs, id, t)
//
///**
// Get the offset for the given object in the table.
// 
// @param xtable The PDXTableRef instance
// @param id The object ID.
// */
//#define PDXTableGetOffsetForID(xtable, id)     PDXGetOffsetForID(xtable->xrefs, id)
//
///**
// Get the generation number or object stream index for the given object in the table.
// 
// @param xtable The PDXTableRef instance
// @param id The object ID.
// */
//#define PDXTableGetGenForID(xtable, id)        PDXGetGenForID(xtable->xrefs, id)

/**
 Determine if the object with given ID is free.
 
 @param xtable The PDXTableRef instance
 @param id The object ID.
 */
#define PDXTableIsIDFree(xtable, id)        (PDXTypeFreed == PDXTableGetTypeForID(xtable, id))

///**
// Set the offset for the object with the given ID to the given offset in the XREF table.
// 
// @param xtable The PDX table instance.
// @param id The object ID.
// @param offs The new offset.
// */
//#define PDXTableSetOffset(xtable, id, offs) PDXSetOffsetForID(xtable->xrefs, id, (PDXOffsetType)offs)

/**
 Read a PDFs XREF data by jumping to the end of the file, reading in the startxref value and jumping to every
 XREF defined via e.g. /Prev, reading the XREFs in as a series with defined domains (by byte). 
 
 @note The master XREF (the one pointed to by the `startxref` offset) defines whether the output PDF uses text or binary format, regardless of what other entries happen to be formatted as. 
 
 Takes into account linearized PDFs.
 
 @param parser The parser.
 */
extern PDBool PDXTableFetchXRefs(PDParserRef parser);

/**
 Pass over an XREF entry in the input PDF.
 
 @note This is only called for text formatted XREF tables; binary XREF tables appear as regular objects with streams
 
 @param parser The parser.
 @param stack The XREF header stack scanned in from the PDScanner.
 @param includeTrailer Whether the trailer should also be consumed, or whether scanning should end after the XREF offsets.
 */
extern PDBool PDXTablePassoverXRefEntry(PDParserRef parser, pd_stack stack, PDBool includeTrailer);

/**
 Insert the one and only XREF table.
 
 @note Pajdeg does not support multiple XREF tables in a single PDF. The only downside to this is that linearized PDFs are delinearized in Pajdeg. Support for linearized PDFs is planned.
 
 @param parser The parser.
 @return true if the insertion was successful.
 */
extern PDBool PDXTableInsert(PDParserRef parser);

/**
 Get the W entry for the table.
 */
extern PDArrayRef PDXTableWEntry(PDXTableRef table);

/**
 Set the type, offset, and gen sizes for the table.
 */
extern void PDXTableSetSizes(PDXTableRef table, unsigned char typeSize, unsigned char offsSize, unsigned char genSize);

/**
 Grow the xref table to accomodate the given cap.
 */
extern void PDXTableGrow(PDXTableRef table, PDSize cap);

#endif

/** @} */
