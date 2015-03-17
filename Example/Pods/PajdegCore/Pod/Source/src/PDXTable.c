//
// PDXTable.c
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

#include <math.h>

#include "pd_internal.h"
#include "PDParser.h"
#include "PDTwinStream.h"
#include "PDScanner.h"
#include "pd_stack.h"
#include "PDXTable.h"
#include "PDReference.h"
#include "PDObject.h"
#include "PDSplayTree.h"
#include "PDStreamFilter.h"
#include "pd_pdf_implementation.h"
#include "PDDictionary.h"
#include "PDArray.h"
#include "PDString.h"
#include "PDNumber.h"

#define xrefalloc(tbl, cap, width)           malloc((cap) * (width) + 1); tbl->allocx = (cap) * (width) + 1
#define xrefrealloc(tbl, xref, cap, width)   realloc(xref, (cap) * (width) + 1); tbl->allocx = (cap) * (width) + 1

/**
 @todo Below functions swap endianness due to the fact numbers are big-endian in binary XRefs (and, I guess, in text-form XRefs as well); if the machine running Pajdeg happens to be big-endian as well, below methods need to be #ifdef-ified to handle this (by NOT swapping every byte around in the integral representation).
 */

PDOffset PDXGetOffsetForArbitraryRepresentation(char *rep, PDInteger len)
{
    PDInteger i;
    unsigned char *o = (unsigned char *)rep;
    PDOffset ot = 0;
    PDOffset shift = 0;
    for (i = len-1; i >= 0; i--) {
        if (shift > 8*sizeof(PDOffset)) {
            if (o[i] > 0) {
                PDWarn("XREF offset larger than largest value containable in the offset type. This is a bug, or the PDF is bigger than %.1f GBs.\n", pow(2.0, (8.0*sizeof(PDOffset))-30.0));
            }
        } else {
            ot |= (o[i] << shift);
        }
        shift += 8;
    }
    return ot;
}

PDOffset PDXTableGetOffsetForID(PDXTableRef table, PDInteger obid)
{
    unsigned char *o = (unsigned char *) &table->xrefs[table->offsAlign + obid * table->width];
    
    // the mask returns all-1 bits or all-0 bits based on the boolean expression; branch prediction should be very good here, but this alleviates the need for branching altogether
#define mask(index)  (-(index < table->offsSize))
#define shift(index) ((table->offsSize - index - 1) << 3)
#define value(index) ((o[index] & mask(index)) << shift(index))
    
    PDOffset r = (value(3) | value(2) | value(1) | value(0));

#undef mask
#undef shift
#undef value
    
//    // if DEBUG, we ensure the above optimization actually matches the correct value by calculating it twice; this will go away in the near future
//#ifdef DEBUG
//    // note: requires that offs size <= 4
//#define O(index, value) (((value) * (index < table->offsSize)) << (8 * (table->offsSize - index - 1)))
//    PDOffset r2 = (PDOffset)(O(3,o[3]) | O(2,o[2]) | O(1,o[1]) | O(0,o[0]));
//    assert(r == r2);
//#undef O
//#endif
    
    return r;
}

#define _PDXSetTypeForID(xrefs, table, id, t)    *(PDXType*)&((xrefs)[id*table->width]) = t

void _PDXSetOffsetForID(char *xrefs, PDXTableRef table, PDInteger obid, PDOffset offset)
{
    unsigned char *o = (unsigned char *) &xrefs[table->offsAlign + obid * table->width];
    PDOffset mask = 0xff;
    PDOffset shift = 0;
    for (int i = table->offsSize - 1; i >= 0; i--) {
        o[i] = (offset & mask) >> shift;
        mask <<= 8;
        shift += 8;
    }
}


void PDXTableSetOffsetForID(PDXTableRef table, PDInteger obid, PDOffset offset)
{
    // we may end up overflowing (or truncating, rather) if additions to the PDF resulted in exceeding the cap on bytes
    if (offset > table->offsCap) {
        // we indeed will overflow, so we need to resize the table
        PDXTableSetSizes(table, table->typeSize, table->offsSize + 1, table->genSize);
    }
    
    unsigned char *o = (unsigned char *) &table->xrefs[table->offsAlign + obid * table->width];
    PDOffset mask = 0xff;
    PDOffset shift = 0;
    for (int i = table->offsSize - 1; i >= 0; i--) {
        o[i] = (offset & mask) >> shift;
        mask <<= 8;
        shift += 8;
    }
}

PDInteger PDXTableGetGenForID(PDXTableRef table, PDInteger obid)
{
    unsigned char *o = (unsigned char *) &table->xrefs[table->genAlign + obid * table->width];
    
    // the mask returns all-1 bits or all-0 bits based on the boolean expression; branch prediction should be very good here, but this alleviates the need for branching altogether
#define mask(index)  (-(index < table->genSize))
#define shift(index) ((table->genSize - index - 1) << 3)
#define value(index) ((o[index] & mask(index)) << shift(index))
    
    PDInteger r = (value(1) | value(0));
    
#undef mask
#undef shift
#undef value
    
//    // if DEBUG, we ensure the above optimization actually matches the correct value by calculating it twice; this will go away in the near future
//#ifdef DEBUG
//    // note: requires that gen size <= 2
//#define O(index, value) (((value) * (index < table->genSize)) << (8 * (table->genSize - index - 1)))
//    PDInteger r2 = (PDInteger)(O(1,o[1]) | O(0,o[0]));
//    assert(r == r2);
//#undef O
//#endif
    
    return r;
}

void PDXTableSetGenForID(PDXTableRef table, PDInteger obid, PDInteger gen)
{
    unsigned char *o = (unsigned char *) &table->xrefs[table->genAlign + obid * table->width];
    PDOffset mask = 0xff;
    PDOffset shift = 0;
    for (int i = table->genSize - 1; i >= 0; i--) {
        o[i] = (gen & mask) >> shift;
        mask <<= 8;
        shift += 8;
    }
    PDAssert(PDXTableGetGenForID(table, obid) == gen);
}

void _PDXSetGenForID(char *xrefs, PDXTableRef table, PDInteger obid, PDInteger gen)
{
    unsigned char *o = (unsigned char *) &xrefs[table->genAlign + obid * table->width];
    PDOffset mask = 0xff;
    PDOffset shift = 0;
    for (int i = table->genSize - 1; i >= 0; i--) {
        o[i] = (gen & mask) >> shift;
        mask <<= 8;
        shift += 8;
    }
//    PDAssert(PDXTableGetGenForID(table, obid) == gen);
}


typedef struct PDXI *PDXI;

/**
 Internal container.
 */
struct PDXI {
    PDInteger mtobid;           ///< master trailer object id
    PDXTableRef pdx;            ///< current table
    PDParserRef parser;         ///< parser
    PDScannerRef scanner;       ///< scanner
    pd_stack queue;             ///< queue of byte offsets at which as yet unparsed XREFs are located
    pd_stack stack;             ///< generic stack
    PDDictionaryRef dict;       ///< generic dictionary
    PDTwinStreamRef stream;     ///< stream
    PDReferenceRef rootRef;     ///< reference to root object
    PDReferenceRef infoRef;     ///< reference to info object
    PDReferenceRef encryptRef;  ///< reference to encrypt object
    PDObjectRef trailer;        ///< trailer object
    PDInteger tables;           ///< number of XREF tables in the PDF
};

/**
 Convenience macro for zeroing a PDXI with a parser.
 
 @param parser The parser.
 */
#define PDXIStart(parser) (struct PDXI) { \
    0,\
    NULL, \
    parser, \
    parser->scanner, \
    NULL, \
    NULL, \
    NULL, \
    parser->stream, \
    NULL, \
    NULL, \
    NULL, \
    NULL, \
    0, \
}

void PDXTableDestroy(PDXTableRef xtable)
{
    if (xtable->nextOb) free(xtable->nextOb);
    PDRelease(xtable->w);
    free(xtable->xrefs);
}

// create new PDX table based off of pdx, which may be NULL in which case a new empty PDX table is returned
PDXTableRef PDXTableCreate(PDXTableRef pdx)
{
    if (pdx) {
        PDXTableRef pdxc = PDAllocTyped(PDInstanceTypeXTable, sizeof(struct PDXTable), PDXTableDestroy, false);
        memcpy(pdxc, pdx, sizeof(struct PDXTable));
        pdxc->xrefs = xrefalloc(pdx, pdx->cap, pdx->width); //malloc(pdx->cap * pdxc->width + 1);
        memcpy(pdxc->xrefs, pdx->xrefs, pdx->cap * pdxc->width);
        return pdxc;
    } 
    
    pdx = PDAlloc(sizeof(struct PDXTable), PDXTableDestroy, true);
    pdx->width = 6;
    pdx->typeSize = 1;
    pdx->typeAlign = 0;
    pdx->offsSize = 4;
    pdx->offsCap = 256 * 256 * 256 * 256 - 1;
    pdx->offsAlign = pdx->typeSize;
    pdx->genSize = 1;
    pdx->genAlign = pdx->offsAlign + pdx->offsSize;
    return pdx;
}

PDBool PDXTableInsertXRef(PDParserRef parser)
{
#define twinstream_printf(fmt...) \
    len = sprintf(obuf, fmt); \
    PDTwinStreamInsertContent(stream, len, obuf)
#define twinstream_put(len, buf) \
    PDTwinStreamInsertContent(stream, len, (char*)buf);
    char *obuf = malloc(512);
    PDInteger len;
    PDInteger i;
    PDTwinStreamRef stream = parser->stream;
    PDXTableRef mxt = parser->mxt;
    
    // write xref header
    twinstream_printf("xref\n%d %lu\n", 0, mxt->count);
    
    // write xref table
    twinstream_put(20, "0000000000 65535 f \n");
    for (i = 1; i < mxt->count; i++) {
        twinstream_printf("%010lld %05ld %c \n", PDXTableGetOffsetForID(mxt, i), PDXTableGetGenForID(mxt, i), PDXTableIsIDFree(mxt, i) ? 'f' : 'n');
    }
    
    PDObjectRef tob = parser->trailer;
    
    PDDictionaryRef tobd = PDObjectGetDictionary(tob);
    PDDictionarySet(tobd, "Size", PDNumberWithSize(parser->mxt->count));
    PDDictionaryDelete(tobd, "Prev");
    PDDictionaryDelete(tobd, "XRefStm");
    
    char *string = NULL;
    len = PDObjectGenerateDefinition(tob, &string, 0);
    // overwrite "0 0 obj" 
    //      with "trailer"
    memcpy(string, "trailer", 7);
    PDTwinStreamInsertContent(stream, len, string); 
    free(string);
    
    free(obuf);
    
    return true;
}

extern void PDParserPassthroughObject(PDParserRef parser);

PDBool PDXTableInsertXRefStream(PDParserRef parser)
{
    char *obuf = malloc(128);

    PDObjectRef trailer = parser->trailer;
    PDXTableRef mxt = parser->mxt;
    
    PDXTableSetOffsetForID(mxt, trailer->obid, (PDOffset)parser->oboffset);
    PDXTableSetTypeForID(mxt, trailer->obid, PDXTypeUsed);
    
    PDDictionaryRef tobd = PDObjectGetDictionary(trailer);
    PDDictionarySet(tobd, "Size", PDNumberWithSize(mxt->count));
    PDDictionarySet(tobd, "W", PDXTableWEntry(mxt));

    PDDictionaryDelete(tobd, "Prev");
    PDDictionaryDelete(tobd, "Index");
    PDDictionaryDelete(tobd, "XRefStm");

    // override filters/decode params always -- better than risk passing something on by mistake that makes the xref stream unreadable
    PDObjectSetFlateDecodedFlag(trailer, true);
    PDObjectSetPredictionStrategy(trailer, PDPredictorPNG_UP, mxt->width);
    
    PDObjectSetStreamFiltered(trailer, mxt->xrefs, mxt->width * mxt->count, false, true);

    // now chuck this through via parser
    parser->state = PDParserStateBase;
    parser->obid = trailer->obid;
    parser->genid = trailer->genid = 0;
    parser->construct = PDRetain(trailer);

    PDParserPassthroughObject(parser);
    
    free(obuf);
    
    return true;
}

PDBool PDXTableInsert(PDParserRef parser)
{
    if (parser->mxt->format == PDXTableFormatText) {
        return PDXTableInsertXRef(parser);
    } else {
        return PDXTableInsertXRefStream(parser);
    }
}

PDBool PDParserIterateXRefDomain(PDParserRef parser);

PDBool PDXTablePassoverXRefEntry(PDParserRef parser, pd_stack stack, PDBool includeTrailer)
{
    PDInteger count;
    PDBool running = true;
    PDScannerRef scanner = parser->scanner;
    
    do {
        // this stack = (xref,) startobid, count
        
        free(pd_stack_pop_key(&stack));
        count = pd_stack_pop_int(&stack);
        
        // we know the # of bytes to skip over, so we discard right away
        
        PDScannerSkip(scanner, count * 20);
        PDTwinStreamDiscardContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream) + count * 20);
        
        running = PDScannerPopStack(scanner, &stack);
        if (running) pd_stack_assert_expected_key(&stack, "xref");
    } while (running);
    
    if (includeTrailer) {
        // read the trailer
        PDScannerAssertString(scanner, "trailer");
        
        // read the trailer dict
        PDScannerAssertStackType(scanner);
        
        // next is the startxref, except some PDF creators (Pages?) drop this nice burp:
        // trailer^M<</Size 10619/Root 10086 0 R>>^Mxref^M0 1^M0000000000 65535 f
        // trailer^M<</Size 10619/Root 10086 0 R/Info 10082 0 R/ID[<B426B7E075B899285BA9A41C8E8C22AC><AE4FA9CBEC3A42C1A878E076F8C838A9>]/Prev 4324067/XRefStm 72147>>^Mstartxref^M4536499^M%%EOF

        PDScannerPopStack(scanner, &stack);
        if (stack->info == &PD_XREF) {
            // this is an xref which means we're iterating the domain, and if we're supposed to continue reading (i.e. this was not the last entry), we loop
            if (PDParserIterateXRefDomain(parser)) {
                pd_stack_pop_identifier(&stack);
                return PDXTablePassoverXRefEntry(parser, stack, true);
            }
            pd_stack_destroy(&stack);
            return false;
        }

        // read startxref
        pd_stack_assert_expected_key(&stack, "startxref");
        pd_stack_destroy(&stack);
        
        // next is EOF meta
        PDScannerPopStack(scanner, &stack);
        pd_stack_assert_expected_key(&stack, "meta");
        pd_stack_assert_expected_key(&stack, "%EOF");
        
        PDTwinStreamDiscardContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream));
    }
    
    return true;
}

static inline PDBool PDXTableFindStartXRef(PDXI X)
{
    PDScannerRef xrefScanner;
    
    PDTWinStreamSetMethod(X->stream, PDTwinStreamReversed);
    
    xrefScanner = PDScannerCreateWithStateAndPopFunc(xrefSeeker, &PDScannerPopSymbolRev);
    
    PDScannerPushContext(xrefScanner, X->stream, PDTwinStreamGrowInputBufferReversed);
//    PDScannerContextPush(X->stream, &PDTwinStreamGrowInputBufferReversed);
    
    // we expect a stack, because it should have skipped until it found startxref

    /// @todo If this is a corrupt PDF, or not a PDF at all, the scanner may end up scanning forever so we put a cap on # of loops -- 100 is overkill but who knows what crazy footers PDFs out there may have (the spec probably disallows that, though, so this should be investigated and truncated at some point)
    PDScannerSetLoopCap(100);
    if (! PDScannerPopStack(xrefScanner, &X->stack)) {
        PDRelease(xrefScanner);
//        PDScannerContextPop();
        return false;
    }
    
    // this stack should start out with "xref" indicating the ob type
    pd_stack_assert_expected_key(&X->stack, "startxref");
    // next is the offset 
    pd_stack_push_identifier(&X->queue, (PDID)pd_stack_pop_size(&X->stack));
    PDAssert(X->stack == NULL);
    PDRelease(xrefScanner);
    
    // we're now ready to skip to the first XRef
    PDTWinStreamSetMethod(X->stream, PDTwinStreamRandomAccess);
//    PDScannerContextPop();
    
    return true;
}

// a 1.5 ob stream header; pull in the definition, skip past stream, then move on
static inline PDBool PDXTableReadXRefStreamHeader(PDXI X)
{
    PDRelease(X->dict);
    pd_stack_pop_identifier(&X->stack);
    X->mtobid = pd_stack_pop_int(&X->stack);
    pd_stack_destroy(&X->stack);
    PDScannerPopStack(X->scanner, &X->stack);
    pd_stack s = X->stack;
    X->dict = PDInstanceCreateFromComplex(&s);
    PDScannerAssertString(X->scanner, "stream");
    PDInteger len = PDNumberGetInteger(PDDictionaryGet(X->dict, "Length"));
//    PDInteger len = PDIntegerFromString(pd_stack_get_dict_key(X->stack, "Length", false)->prev->prev->info);
    PDScannerSkip(X->scanner, len);
    PDTwinStreamAdvance(X->stream, X->stream->cursor + X->scanner->boffset);
    
    X->scanner->buf = &X->stream->heap[X->stream->cursor];
    X->scanner->boffset = 0;
    X->scanner->bsize = X->stream->holds - X->stream->cursor;

    PDScannerAssertComplex(X->scanner, PD_ENDSTREAM);
    PDScannerAssertString(X->scanner, "endobj");
        
    return true;
}

static inline PDBool PDXTableReadXRefStreamContent(PDXI X, PDOffset offset)
{
    PDSize size;
    char *xrefs;
    char *buf;
    char *bufi;
    PDBool aligned;
    PDInteger len;
    PDArrayRef byteWidths;
//    pd_stack index;
    PDArrayRef index;
    PDDictionaryRef filterOpts;
    PDInteger startob;
    PDInteger obcount;
//    pd_stack filterDef;
    PDStringRef filterName;
    PDInteger j;
    PDInteger i;
    PDInteger indexCtr, indexCount;
    PDInteger sizeT;
    PDInteger sizeO;
    PDInteger sizeI;
    PDInteger padT = 0;
    PDInteger padO = 0;
    PDInteger padI = 0;
    PDInteger shrT = 0;
    PDInteger shrO = 0;
    PDInteger shrI = 0;
    PDInteger capT = 0;
    PDInteger capO = 0;
    PDInteger capI = 0;
    PDXTableRef pdx;
    
    pdx = X->pdx;
    pdx->format = PDXTableFormatBinary;
    
    // pull in defs stack and get ready to read stream
    PDID id = pd_stack_pop_identifier(&X->stack);
    PDAssert(id == &PD_OBJ);
    pdx->obid = pd_stack_pop_int(&X->stack);
    pd_stack_destroy(&X->stack);
    PDScannerPopStack(X->scanner, &X->stack);
    pd_stack s = X->stack;
    PDRelease(X->dict);
    X->dict = PDInstanceCreateFromComplex(&s);

    PDScannerAssertString(X->scanner, "stream");
    len = PDNumberGetInteger(PDDictionaryGet(X->dict, "Length"));
//    len = PDIntegerFromString(pd_stack_get_dict_key(X->stack, "Length", false)->prev->prev->info);
    byteWidths = PDDictionaryGet(X->dict, "W");
//    byteWidths = pd_stack_get_dict_key(X->stack, "W", false);
    index = PDDictionaryGet(X->dict, "Index");
//    index = pd_stack_get_dict_key(X->stack, "Index", false);
//    filterDef = pd_stack_get_dict_key(X->stack, "Filter", false);
    filterName = PDDictionaryGet(X->dict, "Filter");
    size = PDNumberGetInteger(PDDictionaryGet(X->dict, "Size"));
//    size = PDIntegerFromString(pd_stack_get_dict_key(X->stack, "Size", false)->prev->prev->info);
    
    PDStreamFilterRef filter = NULL;
    if (filterName) {
        // ("name"), "filter name"
        filterOpts = PDDictionaryGet(X->dict, "DecodeParms");
//        filterDef = as(pd_stack, filterDef->prev->prev->info)->prev;
//        filterOpts = PDDictionaryCreateWithComplex(pd_stack_get_dict_key(X->stack, "DecodeParms", false));
//        if (filterOpts) 
//            filterOpts = PDStreamFilterGenerateOptionsFromDictionaryStack(filterOpts->prev->prev->info);
        filter = PDStreamFilterObtain(PDStringEscapedValue(filterName, false, NULL), true, filterOpts);
        if (NULL == filter) {
            PDError("unable to obtain filter %s!", filterName->data);
            PDRelease(filterName->alt);
            filterName->alt = NULL; // get rid of "cached" result
            PDStringEscapedValue(filterName, false, NULL);
        }
    }
    
    if (filter) {
        /// @todo We know from 'size' exactly how many bytes we expect out of this thing, so we can set buffer to this value instead of basing it off len (compressed stream length)
        
        PDInteger got = 0;
        PDInteger cap = len < 1024 ? 1024 : len * 4;
        buf = malloc(cap);
        
        PDScannerAttachFilter(X->scanner, filter);
        PDInteger bytes = PDScannerReadStream(X->scanner, len, buf, cap);
        while (bytes > 0) {
            got += bytes;
            if (! filter->finished && filter->bufOutCapacity < 512) {
                cap *= (cap < 8192 ? 4 : 2); // we don't want to hit caps in filters more than once or twice, but we don't want huge buffers either so we cap the rapid growth to 8k and then double after that
                buf = realloc(buf, cap);
            }
            bytes = PDScannerReadStreamNext(X->scanner, &buf[got], cap - got);
        }

        PDScannerDetachFilter(X->scanner);
    } else {
        buf = malloc(len);
        PDScannerReadStream(X->scanner, len, buf, len);
    }
    
    // process buf
    
//    byteWidths = as(pd_stack, byteWidths->prev->prev->info)->prev->prev->info;
    sizeT = PDNumberGetInteger(PDArrayGetElement(byteWidths, 0)); // PDIntegerFromString(as(pd_stack, byteWidths->info)->prev->info);
    sizeO = PDNumberGetInteger(PDArrayGetElement(byteWidths, 1)); // PDIntegerFromString(as(pd_stack, byteWidths->prev->info)->prev->info);
    sizeI = PDNumberGetInteger(PDArrayGetElement(byteWidths, 2)); //PDIntegerFromString(as(pd_stack, byteWidths->prev->prev->info)->prev->info);
    
    if (pdx->count == 0 || (sizeT >= pdx->typeSize && sizeO >= pdx->offsSize && sizeI >= pdx->genSize)) {
        // we can adopt the given sizes as is, as they won't force us to lose bytes
        PDXTableSetSizes(pdx, sizeT > 0 ? sizeT : 1, sizeO, sizeI);
        aligned = sizeT > 0;
    } else {
        // we may still have to resize the table to fit
        unsigned char maxT = sizeT > pdx->typeSize ? sizeT : pdx->typeSize;
        unsigned char maxO = sizeO > pdx->offsSize ? sizeO : pdx->offsSize;
        unsigned char maxI = sizeI > pdx->genSize  ? sizeI : pdx->genSize;
        if (maxT > pdx->typeSize || maxO > pdx->offsSize || maxI > pdx->genSize) {
            PDXTableSetSizes(pdx, maxT, maxO, maxI);
        }
        aligned = false;
    }
    
    if (size == X->mtobid) {
        // some PDF creators think it's wise to exclude the XRef binary object from the XRef. entirely. this can be signified by the XRef being the very last object in the PDF, and the XRef size being its own id (thus including all except itself)
        if (size >= pdx->cap) {
            // realloc; we only do this here because we want to avoid two big reallocs (one for 'size' and one for 'size+1')
            pdx->cap = size+1;
            pdx->xrefs = xrefrealloc(pdx, pdx->xrefs, size + 1, pdx->width); //realloc(pdx->xrefs, pdx->width * (size + 1));
        }
    }
    
    if (size > pdx->count) {
        pdx->count = size;
        if (size > pdx->cap) {
            /// @todo this size is known beforehand, or can be known beforehand, in pass 1; xrefs should never have to be reallocated, except for the initial setup
            pdx->cap = size;
            pdx->xrefs = xrefrealloc(pdx, pdx->xrefs, size, pdx->width); //realloc(pdx->xrefs, pdx->width * size);
        }
    }
    
//    // this layout may be optimized for Pajdeg; if it is, we can inject the buffer straight into the data structure
//    aligned = sizeT == PDXTypeSize && sizeO == PDXOffsSize && sizeI == PDXGenSize;
//    
    if (! aligned) {
        // not aligned, so need pad
#define setup_align(suf, our_size) \
        if (our_size > size##suf) { \
            cap##suf = size##suf; \
            pad##suf = our_size - size##suf; \
            shr##suf = 0; \
        } else { \
            cap##suf = our_size; \
            pad##suf = 0; \
            shr##suf = size##suf - our_size; \
        }

        setup_align(T, pdx->typeSize);
        setup_align(O, pdx->offsSize);
        setup_align(I, pdx->genSize);
        
#undef setup_align
#define transfer_pc(dst, src, pad, shr, cap, i) \
            for (i = 0; i < pad; i++) \
                dst[i] = 0; \
            for (i = 0; i < shr; i++) \
                PDAssert(src[i] == 0); \
            memcpy(&dst[pad], &src[shr], cap); \
            dst += pad + cap; \
            src += shr + cap
#define transfer_pcs(dst, src, i, suf) transfer_pc(dst, src, pad##suf, shr##suf, cap##suf, i)
    }
    
    // index, which is optional, can fine tune startob/obcount; it defaults to [0 Size]

#define index_pop() \
        startob = PDNumberGetInteger(PDArrayGetElement(index, indexCtr));\
        obcount = PDNumberGetInteger(PDArrayGetElement(index, indexCtr+1));\
        indexCtr += 2
//    startob = PDIntegerFromString(as(pd_stack, index->info)->prev->info); \
//    obcount = PDIntegerFromString(as(pd_stack, index->prev->info)->prev->info); \
//    index = index->prev->prev
    
    startob = 0;
    obcount = size;
    indexCtr = 0;
    indexCount = index ? PDArrayGetCount(index) : 0;

    if (indexCtr < indexCount) {
//        // move beyond header
//        index = as(pd_stack, index->prev->prev->info)->prev->prev->info;
//        // some crazy PDF creators out there may think it's a lovely idea to put an empty index in; if they do, we will presume they meant the default [0 Size]
//        if (index) {
        index_pop();
//        }
    }
    
    xrefs = pdx->xrefs;
    bufi = buf;
    
    do {
        PDAssert(startob + obcount <= size);
        
        if (aligned) {
            memcpy(&xrefs[startob * pdx->width], bufi, obcount * pdx->width);
            bufi += obcount * pdx->width;
        } else {
            char *dst = &xrefs[startob * pdx->width];
            
            for (i = 0; i < obcount; i++) {
                // transfer 
                transfer_pcs(dst, bufi, j, T);
                if (sizeT == 0) dst[-1] = PDXTypeUsed;
                transfer_pcs(dst, bufi, j, O);
                transfer_pcs(dst, bufi, j, I);
                PDAssert(((dst - xrefs) % pdx->width) == 0);
            }
        }
        
        obcount = 0;
        if (indexCtr < indexCount) {
            index_pop();
        }
    } while (obcount > 0);
    
#undef transfer_pcs
#undef transfer_pc
#undef index_pop
    
    free(buf);
    
    // 01 0E8A 0    % entry for object 2 (0x0E8A = 3722)
    // 02 0002 00   % entry for object 3 (in object stream 2, index 0)
    
    // Index defines what we've got 
    /*
     stack<0xb648ae0> {
         0x46d0b0 ("de")
         Index
         stack<0x172aef60> {
             0x46d0b4 ("array")
             0x46d0a8 ("entries")
             stack<0x172adff0> {
                 stack<0x172adfe0> {
                     0x46d0b8 ("ae")
                     1636
                 }
                 stack<0x172ae000> {
                     0x46d0b8 ("ae")
                     1
                 }
                 stack<0x172ae070> {
                     0x46d0b8 ("ae")
                     1660
                 }
                 stack<0x172ae0c0> {
                     0x46d0b8 ("ae")
                     1
                 }
                 stack<0x172ae160> {
                     0x46d0b8 ("ae")
                     1663
                 }
     */
    
    PDScannerAssertComplex(X->scanner, PD_ENDSTREAM);
    PDScannerAssertString(X->scanner, "endobj");
    
    if (size == X->mtobid && pdx->count == size) {
        // put in the XRef manually
        pdx->count++;
        PDXTableSetTypeForID(pdx, X->mtobid, PDXTypeUsed);
        PDXTableSetOffsetForID(pdx, X->mtobid, (PDOffset)offset);
        PDXTableSetGenForID(pdx, X->mtobid, 0);
    }
    
    return true;
}

static inline PDBool PDXTableReadXRefHeader(PDXI X)
{
    // we keep popping stacks until we fail, which means we've encountered the trailer (which is a string, not a stack)
    do {
        // this stack = xref, startobid, <startobid>, count, <count>
        pd_stack_assert_expected_key(&X->stack, "xref");
        pd_stack_pop_int(&X->stack);
        PDInteger count = pd_stack_pop_int(&X->stack);
        
        // we now have a stream (technically speaking) of xrefs
        count *= 20;
        PDScannerSkip(X->scanner, count);
        
        /// @todo: Sort out this mess; the problem is that PDTwinStreamAdvance() sometimes needs cursor included, and sometimes not, depending on circumstances; or is it boffset? in either case, it's not working very well, requiring the hack below
        PDTwinStreamAdvance(X->stream, X->stream->cursor + X->scanner->boffset);
        X->scanner->buf = &X->stream->heap[X->stream->cursor];
        X->scanner->boffset = 0;
        X->scanner->bsize = X->stream->holds - X->stream->cursor;
    } while (PDScannerPopStack(X->scanner, &X->stack));
    
    // we now get the trailer
    PDScannerAssertString(X->scanner, "trailer");
    
    // and the trailer dictionary
    PDScannerPopStack(X->scanner, &X->stack);

    return true;
}

static inline PDBool PDXTableReadXRefContent(PDXI X)
{
    PDSize size;
    PDInteger i;
    char *buf;
    char *src;
    char *dst;
    PDBool used;
    PDOffset offset;
//    PDInteger *freeLink;
    PDInteger prevFreeID;

    PDXTableRef pdx = X->pdx;
    pdx->format = PDXTableFormatText;
    PDXTableSetSizes(pdx, 1, 4, 2); // we do this because there's no guarantee that 0 <= generation number <= 255, which it must be for the default size setup
    
    do {
        // this stack = xref, startobid, <startobid>, count, <count>
        pd_stack_assert_expected_key(&X->stack, "xref");
        PDInteger startobid = pd_stack_pop_int(&X->stack);
        PDInteger count = pd_stack_pop_int(&X->stack);
        
        //printf("[%d .. %d]\n", startobid, startobid + count - 1);
        
        size = startobid + count;
        
        if (size > pdx->count) {
            pdx->count = size;
            if (size > pdx->cap) {
                // we must realloc xref as it can't contain all the xrefs
                pdx->cap = size;
                pdx->xrefs = xrefrealloc(pdx, pdx->xrefs, size, pdx->width); //realloc(pdx->xrefs, pdx->width * size);
            }
        }
        
        // we now have a stream (technically speaking) of xrefs
        PDInteger bytes = count * 20;
        buf = malloc(bytes);
        if (bytes != PDScannerReadStream(X->scanner, bytes, buf, bytes)) {
            free(buf);
            return false;
        }
        
        // convert into internal xref table
        src = buf;
        dst = &pdx->xrefs[pdx->width * startobid];
        prevFreeID = -1;
        for (i = 0; i < count; i++) {
#define PDXOffset(pdx)      fast_mutative_atol(pdx, 10)
#define PDXGenId(pdx)       fast_mutative_atol(&pdx[11], 5)
#define PDXUsed(pdx)        (pdx[17] == 'n')
            
            offset = (PDOffset)PDXOffset(src);
            
            // some PDF creators (determine who this is so they can be contacted; or determine if this is acceptable according to spec) incorrectly think setting generation number to 65536 is the same as setting the used character to 'f' (free) -- in order to not confuse Pajdeg, we address that here
            // other PDF creators think dumping 000000000 00000 n (i.e. this object can be found at offset 0, and it's in use) means "this object is unused"; we address that as well
#ifdef DEBUG
            if (PDXUsed(src) && (PDXGenId(src) == 65536 || offset == 0)) {
                PDNotice("warning: marking object #%ld as unused (gen = 65536 or offs = 0)", i);
            }
#endif
            used = PDXUsed(src) && (PDXGenId(src) != 65536) && (offset != 0);
            
            _PDXSetOffsetForID(dst, pdx, i, offset);

            if (used) {
                _PDXSetTypeForID(dst, pdx, i, PDXTypeUsed);
                _PDXSetGenForID(dst, pdx, i, PDXGenId(src));
            } else {
                // freed objects link to each other in obstreams
                _PDXSetTypeForID(dst, pdx, i, PDXTypeFreed);
                if (prevFreeID > -1)
                    _PDXSetGenForID(dst, pdx, prevFreeID, startobid + i);
                prevFreeID = i;
                _PDXSetGenForID(dst, pdx, prevFreeID, 0);
            }
            
            src += 20;
        }
        
        free(buf);
    } while (PDScannerPopStack(X->scanner, &X->stack));
    
    return true;
}

static inline void PDXTableParseTrailer(PDXI X)
{
    // if we have no Root or Info yet, grab them if found
    PDDictionaryRef dict = PDInstanceCreateFromComplex(&X->stack);
    if (dict == NULL) {
        PDError("Unable to parse trailer (NULL dictionary)");
        return;
    }
    
    if (X->rootRef == NULL && PDDictionaryGet(dict, "Root")) {
        X->rootRef = PDDictionaryGet(dict, "Root");
    }
    if (X->infoRef == NULL && PDDictionaryGet(dict, "Info")) {
        X->infoRef = PDDictionaryGet(dict, "Info");
    }
    if (X->encryptRef == NULL && PDDictionaryGet(dict, "Encrypt")) {
        X->encryptRef = PDDictionaryGet(dict, "Encrypt");
    }
    
    // a Prev key may or may not exist, in which case we want to hit it
    if (PDDictionaryGet(dict, "Prev")) {
        pd_stack_push_identifier(&X->queue, (PDID)PDNumberGetSize(PDDictionaryGet(dict, "Prev")));
    }
    
    // For 1.5+ PDF:s, an XRefStm may exist; it takes precedence over Prev, but does not override Prev
    if (PDDictionaryGet(dict, "XRefStm")) {
        pd_stack_push_identifier(&X->queue, (PDID)PDNumberGetSize(PDDictionaryGet(dict, "XRefStm")));
    }
    
    // update the trailer object in case additional info is included; note that we set def to the most recent object only and do not "append" like we do with XREF entries
    if (X->trailer->def == NULL) {
        X->trailer->obid = X->mtobid;
        X->trailer->def = X->stack;
        X->trailer->inst = dict;
    } else {
        pd_stack_destroy(&X->stack);
        PDRelease(dict);
    }
}

PDBool PDXTableFetchHeaders(PDXI X)
{
    PDBool success;
    pd_stack osstack;
    
    X->tables = 0;
    X->mtobid = 0;
    X->trailer = X->parser->trailer = PDObjectCreate(0, 0);
    
    osstack = NULL;
    
    do {
        // pull next offset out of queue into the offsets stack and jump there
        X->tables++;
        pd_stack_pop_into(&osstack, &X->queue);
        
        // jump to xref
        //printf("offset = %lld\n", (PDSize)osstack->info);
        PDTwinStreamSeek(X->stream, (PDSize)osstack->info);
        
        // set up scanner
        X->scanner = PDTwinStreamCreateScanner(X->parser->stream, pdfRoot);
//        X->scanner = PDScannerCreateWithState(pdfRoot);
        
        // if this is a v1.5 PDF, we may run into an object definition here; the object is the replacement for the trailer, and has a (usually compressed) stream of the XREF table
        if (PDScannerPopStack(X->scanner, &X->stack)) {
            // we determine this by checking the identifier for the popped stack
            if (PDIdentifies(X->stack->info, PD_OBJ)) {
                if (! PDXTableReadXRefStreamHeader(X)) {
                    PDWarn("Failed to read XRef stream header.");
                    return false;
                }
            } else {
                // this is a regular old xref table with a trailer at the end
                if (! PDXTableReadXRefHeader(X)) {
                    PDWarn("Failed to read XRef header.");
                    return false;
                }
            }
        }
        
        success = ! X->scanner->failed;
        
        PDXTableParseTrailer(X);
        
        PDRelease(X->scanner);
    } while (X->queue && success);
    
    X->stack = osstack;
    
    return success;
}

void PDXTablePrint(PDXTableRef pdx)
{
//    char *xrefs = pdx->xrefs;
    PDInteger i;
    
    char *types[] = {"free", "used", "compressed"};
    
    printf("XREF with %ld objects @ %ld:\n", pdx->count, pdx->pos);

    for (i = 0; i < pdx->count; i++) {
        PDOffset offs = PDXTableGetOffsetForID(pdx, i);
        printf("#%03ld: %010lld (%s)\n", i, offs, types[PDXTableGetTypeForID(pdx, i)]);
    }
}

PDBool PDXTableFetchContent(PDXI X)
{
    PDXTableRef *tables;
    PDSize      *offsets;
    PDSize       offs;
    PDInteger offscount;
    PDInteger j;
    PDInteger i;
    PDInteger gotTables;
    pd_stack osstack;
    PDXTableRef prev;
    PDXTableRef pdx;
    
    // fetch headers puts offset stack into X as stack so we get that out
    osstack = X->stack;
    X->stack = NULL;
    
    // we now have a stack in versioned order, so we start setting up xrefs
    offsets = malloc(X->tables * sizeof(PDSize));
    tables = malloc(X->tables * sizeof(PDXTableRef));
    offscount = 0;
    gotTables = 0;
    
    pdx = NULL;
    while (0 != (offs = (PDSize)pd_stack_pop_identifier(&osstack))) {
        gotTables++;
        prev = pdx;
        pdx = PDXTableCreate(pdx);
        
        pdx->prev = prev;
        X->pdx = pdx;
        
        // put offset in (sorted)
        for (i = 0; i < offscount && offsets[i] < offs; i++) ;
        for (j = offscount; j > i; j--) {
            offsets[j] = offsets[j-1];
            tables[j] = tables[j-1];
        }
        offsets[i] = offs;
        tables[i] = pdx;
        offscount++;
        
        pdx->pos = offs;
        
        // jump to xref
        PDTwinStreamSeek(X->stream, offs);
        
        // set up scanner
        X->scanner = PDTwinStreamCreateScanner(X->parser->stream, pdfRoot);
        //PDScannerCreateWithState(pdfRoot);
        
        // if this is a v1.5 PDF, we may run into an object definition here; the object is the replacement for the trailer, and has a (usually compressed) stream of the XREF table
        if (PDScannerPopStack(X->scanner, &X->stack)) {
            // we determine this by checking the identifier for the popped stack
            if (PDIdentifies(X->stack->info, PD_OBJ)) {
                if (! PDXTableReadXRefStreamContent(X, offs)) {
                    PDWarn("Failed to read XRef stream header.");
                    continue;
                }
            } else {
                // this is a regular old xref table with a trailer at the end
                if (! PDXTableReadXRefContent(X)) {
                    PDWarn("Failed to read XRef header.");
                    continue;
                }
            }
        }
        
        PDRelease(X->scanner);
        pd_stack_destroy(&X->stack);
    }
    
    // pdx is now the complete input xref table with all offsets correct, so we use it as the base for the master table
    X->parser->mxt = PDXTableCreate(pdx);
    
    // we now set up the xstack from the (byte-ordered) list of xref tables; if the PDF is or appears to be linearized, however, we flatten the stack into one entry
    X->parser->xstack = NULL;
    if (X->tables == 2 && pdx && pdx->prev && pdx->pos < pdx->prev->pos) {
        // master is before its precdecessor byte-wise, and the PDF has two XRef entries => linearized; flatten
        pdx->linearized = true;
        pdx->pos = pdx->prev->pos;
        if (pdx->format == PDXTableFormatBinary && pdx->prev->format == PDXTableFormatBinary)
            PDXTableSetTypeForID(X->parser->mxt, pdx->prev->obid, PDXTypeFreed);
        PDRelease(pdx->prev);
        pdx->prev = NULL;
        
        pd_stack_push_object(&X->parser->xstack, pdx);
    } else {
        // otherwise ensure that the master xref table is last; if it isn't, Pajdeg will end parsing prematurely
        for (i = gotTables - 1; i >= 0; i--) {
            // if the XREF comes after the master, the PDF is broken (?), and we bump the master XREF position and skip over the XREF entirely -- this is perfectly non-destructive in terms of data; the master XREF contains the complete set of changes applied in revision order; in theory, all XREF tables could be completely ignored with no side effects aside from safety harness of the parser seeing what it expects to be seeing; the one potential problem with dropping a revision is that indirect object referenced stream lengths for deprecated objects may receive the wrong length; I'm fine with Pajdeg failing at that point
            if (tables[i]->pos > pdx->pos) { /// @todo CLANG does not recognize that X->tables = 0 if pdx = nil, and no dereferencing of null or undefined ptr value will ever occur
                /// @todo this is not the case when XRefStm objects are included, as that puts table count > 2
                pdx->pos = tables[i]->pos;
                pdx->linearized = true;
                PDRelease(tables[i]);
            } else {
                pd_stack_push_object(&X->parser->xstack, tables[i]);
            }
        }
    }
    free(offsets);
    free(tables);
    
    return true;
}

PDBool PDXTableFetchXRefs(PDParserRef parser)
{
    struct PDXI X = PDXIStart(parser);
    
    // find starting XRef position in PDF
    if (! PDXTableFindStartXRef(&X)) {
        return false;
    }
    
    // pass over XRefs once, to get offsets in the right order (we want oldest first)
    if (! PDXTableFetchHeaders(&X)) {
        return false;
    }
    
    // pass over XRefs again, in right order, and parse through content this time
    if (! PDXTableFetchContent(&X)) {
        return false;
    }
    
    // and finally pull out the current table
    parser->cxt = pd_stack_pop_object(&parser->xstack);
    
    /*printf("xrefs:\n");
    printf("- mxt = %p: %ld\n", parser->mxt, ((PDTypeRef)parser->mxt - 1)->retainCount);
    printf("- cxt = %p: %ld\n", parser->cxt, ((PDTypeRef)parser->cxt - 1)->retainCount);
    for (pd_stack t = parser->xstack; t; t = t->prev)
        printf("- [-]: %ld\n", ((PDTypeRef)t->info - 1)->retainCount);*/
    
    parser->xrefnewiter = 1;
    
    parser->rootRef = PDRetain(X.rootRef);
    parser->infoRef = PDRetain(X.infoRef);
    parser->encryptRef = PDRetain(X.encryptRef);
    
    // we've got all the xrefs so we can switch back to the readwritable method
    PDTWinStreamSetMethod(X.stream, PDTwinStreamReadWrite);
    
    // clean up X struct
    PDRelease(X.dict);
    
    //#define DEBUG_PARSER_PRINT_XREFS
//#ifdef DEBUG_PARSER_PRINT_XREFS
//    printf("\n"
//           "       XREFS     \n"
//           "  OFFSET    GEN  U\n"
//           "---------- ----- -\n"
//           "%s", (char*)xrefs);
//#endif
    
//#define DEBUG_PARSER_CHECK_XREFS
#ifdef DEBUG_PARSER_CHECK_XREFS
    printf("* * * * *\nCHECKING XREFS\n* * * * * *\n");
    {
        PDXTableRef table = parser->mxt;
//        char *xrefs = table->xrefs;
        char *buf;
        char obdef[50];
        PDInteger bufl,obdefl,i,j;
        
        //char *types[] = {"free", "used", "compressed"};
        
        for (i = 0; i < parser->mxt->count; i++) {
            PDOffset offs = PDXTableGetOffsetForID(table, i);
            //printf("object #%3ld: %10lld (%s)\n", i, offs, types[PDXGetTypeForID(xrefs, i)]);
            if (PDXTypeUsed == PDXTableGetTypeForID(table, i)) {
                bufl = PDTwinStreamFetchBranch(X.stream, (PDSize) offs, 200, &buf);
                obdefl = sprintf(obdef, "%ld %ld obj", i, PDXTableGetGenForID(table, i));//PDXGenId(xrefs[i]));
                if (bufl < obdefl || strncmp(obdef, buf, obdefl)) {
                    printf("ERROR: object %ld definition did not start at %lld: instead, this was encountered: ", i, offs);
                    for (j = 0; j < 20 && j < bufl; j++) 
                        putchar(buf[j] < '0' || buf[j] > 'z' ? '.' : buf[j]);
                    printf("\n");
                }
            }
        }
    }
#endif
    
    return true;
}

PDArrayRef PDXTableWEntry(PDXTableRef table)
{
    if (table->w) return table->w;
    table->w = PDArrayCreateWithCapacity(3);
    PDArrayAppend(table->w, PDNumberWithInteger(table->typeSize));
    PDArrayAppend(table->w, PDNumberWithInteger(table->offsSize));
    PDArrayAppend(table->w, PDNumberWithInteger(table->genSize));
    return table->w;
//    sprintf(table->w, "[ %d %d %d ]", table->typeSize, table->offsSize, table->genSize);
//    return table->w;
}

void PDXTableSetSizes(PDXTableRef table, unsigned char typeSize, unsigned char offsSize, unsigned char genSize)
{
    PDAssert(typeSize == 1); // crash = type size <> 1 is not supported in this implementation; contact devs or update code to support this if needed (but why?)

    if (table->w) {
        PDRelease(table->w);
        table->w = NULL;
    }
    
    unsigned char newWidth = typeSize + offsSize + genSize;
    if (table->xrefs != NULL) {
        PDXTableRef tmp = PDXTableCreate(NULL);
        PDXTableSetSizes(tmp, typeSize, offsSize, genSize);
        char *newXrefs = xrefalloc(table, table->cap, newWidth); //malloc(table->cap * newWidth);
        for (int i = 0; i < table->count; i++) {
            _PDXSetTypeForID(newXrefs, tmp, i, PDXTableGetTypeForID(table, i));
            _PDXSetOffsetForID(newXrefs, tmp, i, PDXTableGetOffsetForID(table, i));
            _PDXSetGenForID(newXrefs, tmp, i, PDXTableGetGenForID(table, i));
        }
        PDRelease(tmp);
        free(table->xrefs);
        table->xrefs = newXrefs;
    }
    
    table->typeSize = typeSize;
    table->typeAlign = 0;
    
    table->offsSize = offsSize;
    table->offsAlign = typeSize;
    table->offsCap = 256;
    for (unsigned char i = 1; i < offsSize; i++) 
        table->offsCap *= 256;
    table->offsCap--;
    
    table->genSize = genSize;
    table->genAlign = typeSize + offsSize;
    
    table->width = newWidth;
}

void PDXTableGrow(PDXTableRef table, PDSize cap)
{
    table->cap = cap;
    table->xrefs = xrefrealloc(table, table->xrefs, cap, table->width);
}

//#define DEBUG_PDX_GNAI

struct gnai_s {
    PDInteger *nextOb;
    PDInteger  prevOb;
#ifdef DEBUG_PDX_GNAI
    PDInteger  prevOffset;
    PDSize     cap;
#endif
};

void PDXTableGNAIterator(PDInteger key, void *value, void *userInfo, PDBool *shouldStop)
{
    struct gnai_s *gna = userInfo;
    PDInteger obid = (PDInteger)value;
#ifdef DEBUG_PDX_GNAI
    PDInteger offs = key;
    PDAssert(gna->cap > obid);
    PDAssert(gna->prevOffset < offs);
    gna->prevOffset = offs;
#endif
    if (gna->prevOb) gna->nextOb[gna->prevOb] = obid;
    gna->prevOb = obid;
}

void PDXTableGenerateNextArray(PDXTableRef table)
{
    struct gnai_s userInfo;
    PDSize cap = table->count;
    PDSplayTreeRef tree = PDSplayTreeCreate();
    for (PDSize i = 1; i < cap; i++) {
        if (PDXTableGetTypeForID(table, i) == PDXTypeUsed) {
            PDOffset offs = PDXTableGetOffsetForID(table, i);
            PDSplayTreeInsert(tree, (PDInteger)offs, (void *)i);
        }
    }
    
    PDInteger *nextOb = table->nextOb = calloc(cap, sizeof(PDInteger));
    userInfo.nextOb = nextOb;
    userInfo.prevOb = 0;
#ifdef DEBUG_PDX_GNAI
    userInfo.prevOffset = -1;
    userInfo.cap = cap;
#endif
    PDSplayTreeIterate(tree, PDXTableGNAIterator, &userInfo);
    PDRelease(tree);
}

PDSize PDXTableDetermineObjectSize(PDXTableRef table, PDInteger obid)
{
    if (table->nextOb == NULL) {
        PDXTableGenerateNextArray(table);
    }
    PDInteger nextOb = table->nextOb[obid];
    if (! nextOb) {
        // we give back a rather high value, but we don't want to risk not being able to read in an object; and chances are this is the very last object in the file so reading until the end of the buffer will not be super time consuming (we will get some overhead in terms of mem use for a sec though)
        return 2500000;
    }
    
    PDOffset startOffset = PDXTableGetOffsetForID(table, obid);
    PDOffset endOffset = PDXTableGetOffsetForID(table, nextOb);
    PDAssert(endOffset > startOffset); // crash = the XRef table is broken; the object (obid) is said to be at the same position or after the object that is supposedly after it (nextOb)
    return (PDSize) (endOffset - startOffset);
}
