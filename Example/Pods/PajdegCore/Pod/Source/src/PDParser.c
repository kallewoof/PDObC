//
// PDParser.c
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

#include "Pajdeg.h"
#include "PDParser.h"

#include "PDOperator.h"
#include "PDObjectStream.h"
#include "pd_internal.h"
#include "PDState.h"
#include "PDArray.h"
#include "pd_pdf_implementation.h"
#include "pd_stack.h"
#include "PDTwinStream.h"
#include "PDReference.h"
#include "PDSplayTree.h"
#include "PDStreamFilter.h"
#include "PDXTable.h"
#include "PDCatalog.h"
#include "pd_crypto.h"
#include "PDDictionary.h"
#include "PDString.h"
#include "PDNumber.h"
#include "PDScanner.h"

void PDParserDestroy(PDParserRef parser)
{
    /*printf("xrefs:\n");
    printf("- mxt = %p: %ld\n", parser->mxt, ((PDTypeRef)parser->mxt - 1)->retainCount);
    printf("- cxt = %p: %ld\n", parser->cxt, ((PDTypeRef)parser->cxt - 1)->retainCount);
    for (pd_stack t = parser->xstack; t; t = t->prev)
        printf("- [-]: %ld\n", ((PDTypeRef)t->info - 1)->retainCount);*/
    
    PDRelease(parser->aiTree);
    PDRelease(parser->catalog);
    PDRelease(parser->construct);
    PDRelease(parser->root);
    PDRelease(parser->info);
    PDRelease(parser->encrypt);
    PDRelease(parser->rootRef);
    PDRelease(parser->infoRef);
    PDRelease(parser->encryptRef);
    PDRelease(parser->trailer);
    PDRelease(parser->skipT);
    pd_stack_destroy(&parser->appends);
    pd_stack_destroy(&parser->inserts);
    
    PDRelease(parser->mxt);
    PDRelease(parser->cxt);
    pd_stack_destroy(&parser->xstack);
    
#ifdef PD_SUPPORT_CRYPTO
    if (parser->crypto) pd_crypto_destroy(parser->crypto);
#endif
    
    pd_pdf_implementation_discard();
}

PDParserRef PDParserCreateWithStream(PDTwinStreamRef stream)
{
    pd_pdf_implementation_use();
    
    PDParserRef parser = PDAllocTyped(PDInstanceTypeParser, sizeof(struct PDParser), PDParserDestroy, true);
    parser->stream = stream;
    parser->state = PDParserStateBase;
    parser->success = true;
    parser->aiTree = PDSplayTreeCreateWithDeallocator(PDReleaseFunc);
    
    if (! PDXTableFetchXRefs(parser)) {
        PDError("PDF is invalid or in an unsupported format.");
        //PDAssert(0); // the PDF is invalid or in a format that isn't supported
        PDRelease(parser);
        return NULL;
    }

    parser->skipT = PDSplayTreeCreateWithDeallocator(PDDeallocatorNull);
    
    PDTwinStreamAsserts(parser->stream);

    parser->scanner = PDTwinStreamSetupScannerWithState(stream, pdfRoot);

    PDTwinStreamAsserts(parser->stream);

    // we always grab the first object, for several reasons: (1) we need to iterate past the starting cruft (%PDF etc), and (2) if this is a linearized PDF, we need to indicate it NO LONGER IS a linearized PDF
    if (PDParserIterate(parser)) {
        // because we end up discarding old definitions, we want to first pass through eventual prefixes; in theory, nothing prevents a PDF from being riddled with comments in between every object, but in practice this tends to only be once, at the top
        PDTwinStreamPrune(parser->stream, parser->oboffset);
        
        // oboffset indicates the position in the PDF where the object begins; we need to pass that through
        PDObjectRef first = PDParserConstructObject(parser);
        if (first->type == PDObjectTypeDictionary) {
            pd_stack linearizedKey = pd_stack_get_dict_key(first->def, "Linearized", true);
            pd_stack_destroy(&linearizedKey);
        }
    }
    
    PDTwinStreamAsserts(parser->stream);
    
    if (parser->encryptRef) {
        // set up crypto instance 
        if (! parser->encrypt) {
            parser->encrypt = PDParserLocateAndCreateObject(parser, parser->encryptRef->obid, true);
            //pd_stack encDef = PDParserLocateAndCreateDefinitionForObject(parser, parser->encryptRef->obid, true);
            //parser->encrypt = PDObjectCreate(parser->encryptRef->obid, parser->encryptRef->genid);
            //parser->encrypt->def = encDef;
        }
#ifdef PD_SUPPORT_CRYPTO
        parser->crypto = pd_crypto_create(PDObjectGetDictionary(parser->trailer), PDObjectGetDictionary(parser->encrypt));
        if (parser->construct) parser->construct->crypto = parser->crypto;
#endif
    }
    
    return parser;
}

pd_stack PDParserLocateAndCreateDefinitionForObjectWithSize(PDParserRef parser, PDInteger obid, PDInteger bufsize, PDBool master, PDOffset *outOffset)
{
    PDAssert(obid != 0); // crash = invalid object id

    char *tb;
    char *string;
    PDTwinStreamRef stream;
    pd_stack stack;
    PDXTableRef xrefTable;
    
    stream = parser->stream;
    
    if (master) {
        xrefTable = parser->mxt;
    } else {
        xrefTable = parser->cxt;

        pd_stack iter;
        pd_stack_for_each(parser->xstack, iter) {
            if (obid >= xrefTable->cap) {
                xrefTable = (PDXTableRef)iter->info;
            }
        }

    }
    PDAssert(obid < xrefTable->cap);
    
    //xrefTable = (master ? parser->mxt : parser->cxt);

    // if the object is in an object stream, we need to fetch its container, otherwise we can fetch the object itself
    if (PDXTypeComp == PDXTableGetTypeForID(xrefTable, obid)) {
        // grab container definition
        
//        PDInteger len;
        PDObjectStreamRef obstm;
//        PDOffset containerOffset;
        PDInteger index = PDXTableGetGenForID(xrefTable, obid);
        PDInteger ctrobid = (PDInteger) PDXTableGetOffsetForID(xrefTable, obid);
        
        PDObjectRef obstmObject = PDParserLocateAndCreateObject(parser, ctrobid, master);
//        pd_stack containerDef = PDParserLocateAndCreateDefinitionForObjectWithSize(parser, ctrobid, bufsize, master, &containerOffset);
//        PDAssert(containerDef);
//        PDObjectRef obstmObject = PDObjectCreate(ctrobid, 0);
//        obstmObject->def = containerDef;
//        obstmObject->crypto = parser->crypto;
//        obstmObject->streamLen = len = pd_stack_peek_int(pd_stack_get_dict_key(containerDef, "Length", false)->prev->prev);
        
        if (obstmObject->extractedLen == -1) {
            PDParserLocateAndFetchObjectStreamForObject(parser, obstmObject);
        }
//        PDTwinStreamFetchBranch(stream, (PDSize) containerOffset, len + 20, &tb);
        tb = obstmObject->streamBuf;
//        len = obstmObject->extractedLen;
//        PDScannerRef streamrdr = PDScannerCreateWithState(pdfRoot);
//        PDScannerPushContext(streamrdr, stream, PDTwinStreamDisallowGrowth);
////        PDScannerContextPush(stream, &PDTwinStreamDisallowGrowth);
//        streamrdr->buf = tb;
//        streamrdr->boffset = 0;
//        streamrdr->bsize = len + 20;

//        char *rawBuf = malloc(len);
//        PDScannerAssertString(streamrdr, "stream");
//        PDScannerReadStream(streamrdr, len, rawBuf, len);
//        PDRelease(streamrdr);
//        PDTwinStreamCutBranch(stream, tb);
//        PDScannerContextPop();
        
        if (tb == NULL) {
            PDWarn("NULL object stream in PDParserLocateAndCreateDefinitionForObjectWithSize() for object #%ld; aborting", obid);
            PDRelease(obstmObject);
            return NULL;
        }
        
        obstm = PDObjectStreamCreateWithObject(obstmObject);
        PDRelease(obstmObject);
        
        // decrypt buffer, if encrypted
//        if (parser->crypto) {
        //            pd_crypto_convert(parser->crypto, ctrobid, 0, rawBuf, len);
        //        }
        
        stack = NULL;
        
        PDObjectStreamParseExtractedObjectStream(obstm, tb);
        if (obstm->elements[index].type == PDObjectTypeString) {
            stack = NULL;
            pd_stack_push_key(&stack, strdup(obstm->elements[index].def));
        } else {
            stack = pd_stack_copy(obstm->elements[index].def);
        }
        
        PDAssert(outOffset == NULL);
        
        PDRelease(obstm);
        
        return stack;
    } 
    
    PDOffset offset = PDXTableGetOffsetForID(xrefTable, obid);
    if (offset == 0) {
        PDNotice("zero offset for %ld is suspicious", obid);
    }
    if (outOffset) *outOffset = offset;
    PDSize readBytes = PDTwinStreamFetchBranch(stream, (PDSize) offset, bufsize, &tb);
    
    PDScannerRef tmpscan = PDScannerCreateWithState(pdfRoot);
    PDScannerPushContext(tmpscan, stream, PDTwinStreamDisallowGrowth);
//    PDScannerContextPush(stream, &PDTwinStreamDisallowGrowth);
    tmpscan->buf = tb;
    tmpscan->boffset = 0;
    tmpscan->bsize = readBytes;
    tmpscan->fixedBuf = true;
    
    if (PDScannerPopStack(tmpscan, &stack)) {
        if (! tmpscan->outgrown) {
            pd_stack_assert_expected_key(&stack, "obj");
            pd_stack_assert_expected_int(&stack, obid);
        }
        pd_stack_destroy(&stack);
    }
    
    stack = NULL;
    if (! PDScannerPopStack(tmpscan, &stack)) {
        if (PDScannerPopString(tmpscan, &string)) {
            pd_stack_push_key(&stack, string);
        }
    }
    
    if (outOffset != NULL) *outOffset += tmpscan->boffset;
    
    PDTwinStreamCutBranch(parser->stream, tb);
    
    stream->outgrown = tmpscan->outgrown;

    PDRelease(tmpscan);
//    PDScannerContextPop();
    
    if (stream->outgrown) {
        // the object did not fit in our expected buffer, which means it's unusually big; we bump the buffer size to 6k if it's smaller, otherwise we consider this a failure
        pd_stack_destroy(&stack);
        stack = NULL;
        if (bufsize < 64000 && readBytes == bufsize)
            return PDParserLocateAndCreateDefinitionForObjectWithSize(parser, obid, (bufsize + 1024) * 3, master, outOffset);
    }
    
    return stack;
}

pd_stack PDParserLocateAndCreateDefinitionForObject(PDParserRef parser, PDInteger obid, PDBool master)
{
    PDAssert(obid != 0); // crash = invalid object id
    if (parser->construct && parser->construct->obid == obid) {
        return pd_stack_copy(parser->construct->def);
    }
    return PDParserLocateAndCreateDefinitionForObjectWithSize(parser, obid, 4192, master, NULL);
}

PDObjectRef PDParserLocateAndCreateObject(PDParserRef parser, PDInteger obid, PDBool master)
{
    PDAssert(obid != 0); // crash = invalid object id

    PDObjectRef ob;
    
    if (parser->construct && parser->construct->obid == obid) {
        return PDRetain(parser->construct);
    }
    
    ob = PDSplayTreeGet(parser->aiTree, obid);
    if (NULL != ob) {
        return PDRetain(ob);
    }
    
    pd_stack defs = PDParserLocateAndCreateDefinitionForObject(parser, obid, master);
    if (defs == NULL) {
        PDNotice("unable to locate definitions for object %ld (%s)", obid, master ? "master XREF" : "current XREF");
        return NULL;
    }
    
    ob = PDObjectCreateFromDefinitionsStack(obid, defs);
    ob->crypto = parser->crypto;
    PDSplayTreeInsert(parser->aiTree, obid, PDRetain(ob));
    
    return ob;
}

void PDParserFetchStreamLengthFromObjectDictionary(PDParserRef parser, pd_stack entry)
{
    void *val = PDInstanceCreateFromComplex(&entry);
    if (PDInstanceTypeRef == PDResolve(val)) {
        PDInteger refid = PDReferenceGetObjectID(val);
        PDObjectRef ref = PDParserLocateAndCreateObject(parser, refid, false);
        parser->streamLen = PDNumberGetInteger(PDObjectGetValue(ref));
        PDRelease(ref);
    } else {
        // val is a PDNumber
        parser->streamLen = PDNumberGetInteger(val);
    }
    PDRelease(val);
//    entry = entry->prev->prev;
//    if (entry->type == PD_STACK_STACK) {
//        PDInteger refid;
//        pd_stack stack;
//        // this is a reference length
//        // e, Length, { ref, 1, 2 }
//        entry = entry->info;
//        PDAssert(PDIdentifies(entry->info, PD_REF));
//        refid = pd_stack_peek_int(entry->prev);
//        
//        stack = PDParserLocateAndCreateDefinitionForObject(parser, refid, false);
//        parser->streamLen = pd_stack_pop_int(&stack);
//    } else {
//        char *string;
//        // e, Length, 116
//        string = entry->info;
//        parser->streamLen = atol(string);
//        PDAssert(parser->streamLen > 0 || !strcmp("0", string));
//    }
}

void PDParserPrepareStreamData(PDParserRef parser, PDObjectRef ob, PDInteger len, PDStringRef filterName, char *rawBuf)
{
    PDInteger elen = len;
    
    if (parser->crypto) {
        pd_crypto_convert(parser->crypto, ob->obid, ob->genid, rawBuf, len);
    }
    
    if (filterName) {
        PDDictionaryRef filterOpts = PDDictionaryGet(PDObjectGetDictionary(ob), "DecodeParms");
        PDStreamFilterRef filter = PDStreamFilterObtain(PDStringEscapedValue(filterName, false), true, filterOpts);
        
        if (NULL == filter) {
            PDWarn("Unknown filter \"%s\" is ignored.", PDStringEscapedValue(filterName, false));
        } else {
            PDInteger allocated;
            char *extractedBuf;
            if (! PDStreamFilterApply(filter, (unsigned char *)rawBuf, (unsigned char **)&extractedBuf, len, &elen, &allocated)) {
                PDNotice("PDStreamFilterApply(%s, <buf>, <&ebuf>, %ld, <olen>, <&alloc>) failed; aborting", PDStringEscapedValue(filterName, true), (unsigned long)len);
                free(rawBuf);
                ob->extractedLen = -1;
                ob->streamBuf = NULL;
                return;
            } 
            
            free(rawBuf);
            rawBuf = extractedBuf;
            if (allocated == elen) {
                // we need another byte for \0 in case this is a text stream; this happens very seldom but has to be dealt with
                PDNotice("{ allocated == elen } hit; notify Pajdeg devs if repeated");
                rawBuf = realloc(rawBuf, elen + 1);
            }
            
            PDRelease(filter);
        }
    }
    
    // in order for this line not to sporadically crash, all mallocs of rawBuf (including extractedBuf for filtered streams above) must have used len + 1 up to this point
    rawBuf[elen] = 0;
    
    ob->extractedLen = elen;
    ob->streamBuf = rawBuf;
}

char *PDParserFetchCurrentObjectStream(PDParserRef parser, PDInteger obid)
{
    PDObjectRef ob = parser->construct;
    
    PDAssert(obid == parser->obid);
    PDAssert(ob);
    PDAssert(ob->obid == obid);
    PDAssert(ob->hasStream);
    
    if (ob->extractedLen != -1) return ob->streamBuf;
    
    PDAssert(parser->state == PDParserStateObjectAppendix);
    
    PDInteger len = parser->streamLen;
    PDStringRef filterName = NULL;
    void *filterValue = PDDictionaryGet(PDObjectGetDictionary(parser->construct), "Filter");
    PDInstanceType filterType = PDResolve(filterValue);
    switch (filterType) {
        case PDInstanceTypeArray:{
            PDInteger count = PDArrayGetCount(filterValue);
            if (count == 0) {
                PDWarn("Null filter (empty array value) encountered");
            } else {
                if (count > 1) {
                    PDInteger cap = 32;
                    char *buf = malloc(32);
                    PDArrayPrinter(filterValue, &buf, 0, &cap);
                    PDWarn("Unsupported chained filter in %s", buf);
                    free(buf);
                }
                filterValue = PDArrayGetElement(filterValue, 0);
            }
        } break;
            
        case PDInstanceTypeString:
            break;
            
        default:
            PDWarn("Unsupported filter type %d", filterType);
            filterValue = NULL;
            break;
    }
    
    filterName = filterValue;

    char *rawBuf = malloc(len + 1);
    PDScannerReadStream(parser->scanner, len, rawBuf, len);
    
    PDParserPrepareStreamData(parser, ob, len, filterName, rawBuf);
    
    /*if (filterName) {
        filterName = &filterName[1];
        pd_stack filterOpts = pd_stack_get_dict_key(ob->def, "DecodeParms", false);
        if (filterOpts) 
            filterOpts = PDStreamFilterGenerateOptionsFromDictionaryStack(filterOpts->prev->prev->info);
        PDStreamFilterRef filter = PDStreamFilterObtain(filterName, true, filterOpts);
        char *extractedBuf;
        PDStreamFilterApply(filter, (unsigned char *)rawBuf, (unsigned char **)&extractedBuf, len, &len);
        free(rawBuf);
        rawBuf = extractedBuf;
        PDRelease(filter);
    }
    
    if (parser->crypto) {
        pd_crypto_convert(parser->crypto, ob->obid, ob->genid, rawBuf, len);
    }
    
    ob->extractedLen = len;
    ob->streamBuf = rawBuf;*/
    
    parser->state = PDParserStateObjectPostStream;
    
    return ob->streamBuf;
}

void PDParserClarifyObjectStreamExistence(PDParserRef parser, PDObjectRef object)
{
    // objects that were random-access-fetched normally don't have their hasStream property set, but we can
    // set it based on their dictionary value /Length -- if it's non-zero (or a ref), they have a stream
    if (object->type == PDObjectTypeUnknown) 
        PDObjectDetermineType(object);
    if (! object->hasStream && object->type == PDObjectTypeDictionary) {
        void *lengthValue = PDDictionaryGet(PDObjectGetDictionary(object), "Length");
        if (lengthValue) {
            // length could be a ref, or a number
            PDInteger lenInt;
            
            if (PDResolve(lengthValue) == PDInstanceTypeRef) {
                // it's a ref, fetch it
                PDObjectRef lenOb = PDParserLocateAndCreateObject(parser, PDReferenceGetObjectID(lengthValue), true);
//                pd_stack lenDef = PDParserLocateAndCreateDefinitionForObject(parser, PDReferenceGetObjectID(lengthValue), true);
                lenInt = PDNumberGetInteger(PDObjectGetValue(lenOb));
//                const char *realLength = pd_stack_pop_key(&lenDef);
//                lenInt = PDIntegerFromString(realLength);
                PDRelease(lenOb);
            } else {
                lenInt = PDNumberGetInteger(lengthValue); //PDIntegerFromString(length);
            }
            object->hasStream = lenInt > 0;
            object->streamLen = lenInt;
        }
    }
}

char *PDParserLocateAndFetchObjectStreamForObject(PDParserRef parser, PDObjectRef object)
{
    if (parser->obid == object->obid) {
        // use the (faster) FetchCurrentObjectStream
        return PDParserFetchCurrentObjectStream(parser, object->obid);
    }

    PDParserClarifyObjectStreamExistence(parser, object);
    
    PDAssert(object);
    PDAssert(object->hasStream);
    if (object->extractedLen != -1) return object->streamBuf;

    PDInteger len = object->streamLen;
    PDStringRef filterName = PDDictionaryGet(PDObjectGetDictionary(object), "Filter");
    
    char *rawBuf = malloc(len + 1);
    
    char *tb;
    char *string;
    pd_stack stack;

    PDOffset offset = PDXTableGetOffsetForID(parser->mxt, object->obid);
    PDTwinStreamFetchBranch(parser->stream, (PDSize) offset, 10000 + len, &tb);
    
    PDScannerRef tmpscan = PDScannerCreateWithState(pdfRoot);
    PDScannerPushContext(tmpscan, parser->stream, PDTwinStreamDisallowGrowth);
    tmpscan->buf = tb;
    tmpscan->boffset = 0;
    tmpscan->bsize = 10000 + len;
    
    if (PDScannerPopStack(tmpscan, &stack)) {
        if (! parser->stream->outgrown) {
            pd_stack_assert_expected_key(&stack, "obj");
            pd_stack_assert_expected_int(&stack, object->obid);
        }
        pd_stack_destroy(&stack);
    }
    
    stack = NULL;
    if (! PDScannerPopStack(tmpscan, &stack)) {
        if (PDScannerPopString(tmpscan, &string)) {
            free(string);
        }
    }
    pd_stack_destroy(&stack);
    
    PDScannerPopString(tmpscan, &string);
    // we expect 'stream'
    PDAssert(!strcmp(string, "stream"));
    free(string);
        
    PDScannerReadStream(tmpscan, len, rawBuf, len);
    
    PDParserPrepareStreamData(parser, object, len, filterName, rawBuf);
        
    /*if (filterName) {
        filterName = &filterName[1];
        pd_stack filterOpts = pd_stack_get_dict_key(object->def, "DecodeParms", false);
        if (filterOpts) 
            filterOpts = PDStreamFilterGenerateOptionsFromDictionaryStack(filterOpts->prev->prev->info);
        PDStreamFilterRef filter = PDStreamFilterObtain(filterName, true, filterOpts);
        char *extractedBuf;
        PDStreamFilterApply(filter, (unsigned char *)rawBuf, (unsigned char **)&extractedBuf, len, &len);
        free(rawBuf);
        rawBuf = extractedBuf;
        PDRelease(filter);
    }
    
    if (parser->crypto) {
        pd_crypto_convert(parser->crypto, object->obid, object->genid, rawBuf, len);
    }
        
    object->extractedLen = len;
    object->streamBuf = rawBuf;*/
        
    PDTwinStreamCutBranch(parser->stream, tb);
    
    PDRelease(tmpscan);
//    PDScannerContextPop();
    
    return object->streamBuf;
}

void PDParserUpdateObject(PDParserRef parser)
{
    char *string;
    PDInteger len;

    // old (input)              new (output)
    // <<<<<<<<<<<<<<<<<<<<     >>>>>>>>>>>>>>>>>>>>
    // 1 2 obj                  1 2 obj
    // << old definition >>     << new definition >>
    // stream                   stream
    // [*]old stream content    new stream content
    // endstream                endstream
    // endobj                   endobj
    // ([*] is scanner position)
    
    PDScannerRef scanner = parser->scanner;
    PDObjectRef ob = parser->construct;
    
    if (ob->synchronizer) (*ob->synchronizer)(parser, ob, ob->syncInfo);
    
    if (ob->deleteObject) {
        ob->skipObject = true;
        PDXTableSetTypeForID(parser->mxt, ob->obid, PDXTypeFreed);
    }
    
    ob->skipStream |= ob->skipObject; // simplify by always killing entire object including stream, if object is skipped
    
    // we discard old definition first; if object has a stream but wants it nixed, we iterate beyond that before discarding; we may have passed beyond the appendix already, in which case we do nothing (we're already done)
    if (parser->state == PDParserStateObjectAppendix && ob->hasStream) {
        if (ob->skipStream) {
            // invalid; see other commented assertion // PDAssert(parser->streamLen > 0);

            PDScannerSkip(scanner, parser->streamLen);
            PDTwinStreamDiscardContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream));
            
            
            PDScannerAssertComplex(scanner, PD_ENDSTREAM);
            //PDScannerAssertString(scanner, "endstream");
        }

        // normalize trail
        ////scanner->btrail = scanner->boffset;
    }

    // discard up to this point (either up to 'endobj' or up to 'stream' dependingly)
    PDTwinStreamDiscardContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream));
    
    // old (input)              new (output)
    //                         >>>>>>>>>>>>>>>>>>>>
    // 1 2 obj                  1 2 obj
    // << old definition >>     << new definition >>
    // stream                   stream
    // <<<<<<<<<<<<<<<<<<<<     
    // [*]old stream content    new stream content
    // endstream                endstream
    // [*]endobj                endobj
    // (two potential scanner locs; the latter one for 'skip stream' or 'no stream' case)

    // push object def, unless it should be skipped
    if (! ob->skipObject) {
        // we have to deal with the stream, in case we're post stream; the reason is that 
        // ob's definition may change as a result of this
        if (ob->hasStream && !ob->skipStream && !ob->ovrStream && parser->state == PDParserStateObjectPostStream) {
            PDObjectSetStreamFiltered(ob, ob->streamBuf, ob->extractedLen, false);
        }
        
        if (ob->ovrDef) {
            PDTwinStreamInsertContent(parser->stream, ob->ovrDefLen, ob->ovrDef);
        } else {
            string = NULL;
            len = PDObjectGenerateDefinition(ob, &string, 0);
            PDTwinStreamInsertContent(parser->stream, len, string);
            free(string);
        }

        // old (input)              new (output)
        // 1 2 obj                  1 2 obj
        // << old definition >>     << new definition >>
        //                         >>>>>>>>>>>>>>>>>>>>
        // stream                   stream
        // <<<<<<<<<<<<<<<<<<<<     
        // [*]old stream content    new stream content
        // endstream                endstream
        // [*]endobj                endobj

        // if we have a stream that shouldn't be skipped, we want to prep for that
        /*if (ob->hasStream && ! ob->skipStream) {
            PDTwinStreamInsertContent(parser->stream, 7, "stream\n");

            PDScannerPopString(scanner, &string);
            
            // now we only expect 'e'(ndobj)
            if (string[0] == 'e') {
                PDAssert(!strcmp(string, "endobj"));
                free(string);
            } else {
                fprintf(stderr, "unknown type: %s\n", string);
                assert(0);
            }
            
            // normalize trail
            scanner->btrail = scanner->boffset;
        }*/
        
        // if we have a stream, get it onto the heap or whip past it
        if (ob->hasStream) {
            if (parser->state != PDParserStateObjectPostStream) {
                PDScannerSkip(scanner, parser->streamLen);
            }
            
            if (ob->skipStream || ob->ovrStream) {
                PDTwinStreamDiscardContent(parser->stream);
            } 
            
            // we may be post-stream here, in which case we have to actually print the stream out as we've discarded the input version already
            else if (parser->state != PDParserStateObjectPostStream) {
//                // we do this by setting the stream to itself; this applies filters and such, and also sets ovrStream flag (which is inserted as appropriate, below)
//                PDObjectSetStreamFiltered(ob, ob->streamBuf, ob->extractedLen);
//            } else {
                // we've just discarded "stream", so we have to put that in first of all
                PDTwinStreamInsertContent(parser->stream, 7, "stream\n");
                
                PDTWinStreamPassthroughContent(parser->stream);
            }
            
            PDScannerAssertComplex(scanner, PD_ENDSTREAM);
            //PDScannerAssertString(scanner, "endstream");
          
            // no matter what, we want to get past endobj keyword for this object
            PDScannerAssertString(scanner, "endobj");
        }
        
        ////scanner->btrail = scanner->boffset;

        // 1 2 obj                  1 2 obj
        // << old definition >>     << new definition >>
        //                         >>>>>>>>>>>>>>>>>>>>
        // stream                   stream
        // [*]old stream content    new stream content
        // endstream                endstream
        // [*]endobj                endobj
        // <<<<<<<<<<<<<<<<<<<<     
        
        // we may want a stream but did not have one -- hasStream defines original conditions, not our desire, hence 'hasStream' rather than 'wantsStream'
        if ((ob->hasStream && ! ob->skipStream) || ob->ovrStream) {
            if (ob->ovrStream) {
                // discard old and write new
                PDTwinStreamDiscardContent(parser->stream);
                //                                            012345 6
                PDTwinStreamInsertContent(parser->stream, 7, "stream\n");
                PDTwinStreamInsertContent(parser->stream, ob->ovrStreamLen, ob->ovrStream);
                //                                              0123456789 0123456 7
                PDTwinStreamInsertContent(parser->stream, 18, "\nendstream\nendobj\n");
            } else {
                // pass through endstream and endobj
                PDTWinStreamPassthroughContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream));
            }

            // 1 2 obj                  1 2 obj
            // << old definition >>     << new definition >>
            // stream                   stream
            // [*]old stream content    new stream content
            // endstream                endstream
            // endobj                   endobj
            // <<<<<<<<<<<<<<<<<<<<     >>>>>>>>>>>>>>>>>>>>     
        } else {
            // invalid; see other commented assertion // PDAssert(ob->hasStream == (parser->streamLen > 0));
            // discard and print out endobj; we do not pass through here, because we may be dealing with a brand new object that doesn't have anything for us to pass
            PDTwinStreamDiscardContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream));
            PDTwinStreamInsertContent(parser->stream, 7, "endobj\n");
        }
    }
    
    PDRelease(ob);
    
    parser->state = PDParserStateBase;
    parser->streamLen = 0;
    
    // pop the next construct off of the inserts stack
    ob = parser->construct = pd_stack_pop_object(&parser->inserts);
    if (ob) {
        // we had another object, so we update the parser values
        parser->obid = ob->obid;
        parser->genid = ob->genid;
        parser->streamLen = ob->streamLen;
    }
}

void PDParserPassoverObject(PDParserRef parser);

void PDParserPassthroughObject(PDParserRef parser)
{
    char *string;
    pd_stack stack, entry;
    PDScannerRef scanner;
    
    // update xref entry; we do this even if this ends up being an xref; if it's an old xref, it will be removed anyway, and if it's the master, it will have its offset set at the end anyway
    PDXTableSetOffsetForID(parser->mxt, parser->obid, parser->oboffset);
    //PDXWrite((char*)&parser->mxt->fields[parser->obid], parser->oboffset, 10);
    
    // if we have a construct, we need to serialize that into the output stream; note that PDParserUpdateObject() will dequeue constructs, if any, from the inserts queue, so we need to while() as well
    if (parser->construct) {
        while (parser->construct) {
            PDParserUpdateObject(parser);
            PDTwinStreamAsserts(parser->stream);
#ifdef PD_DEBUG_TWINSTREAM_ASSERT_OBJECTS
            char expect[100];
            PDInteger len = sprintf(expect, "%zd %zd obj", parser->obid, parser->genid);
            PDTwinStreamReassert(parser->stream, parser->oboffset, expect, len);
#endif
            parser->oboffset = (PDSize)PDTwinStreamGetOutputOffset(parser->stream);
        }
        return;
    }
    
    scanner = parser->scanner;
    
    switch (parser->state) {
        case PDParserStateObjectDefinition:
            if (PDScannerPopStack(scanner, &stack)) {
                if (parser->encryptRef && parser->obid == parser->encryptRef->obid) {
                    // this is an encryption dictionary; those have a Length field that is not the length of the object stream
                } else {
                    entry = pd_stack_get_dict_key(stack, "Length", false);
                    if (entry) {
                        PDParserFetchStreamLengthFromObjectDictionary(parser, entry);
                    }
                }

                // this may be an xref object stream; we can those; if it is the master object, it will be appended to the end of the stream regardless
                entry = pd_stack_get_dict_key(stack, "Type", false);
                if (entry) {
                    entry = entry->prev->prev->info; // entry value is a stack (a name stack)
                    PDAssert(PDIdentifies(entry->info, PD_NAME));
                    if (!strcmp("XRef", entry->prev->info)) {
                        pd_stack_destroy(&stack);
                        PDXTableSetTypeForID(parser->mxt, parser->obid, PDXTypeFreed);
                        parser->state = PDParserStateObjectAppendix;
                        PDParserPassoverObject(parser);
                        // we also have a startxref (apparently not always)
#if 0
                        PDScannerAssertComplex(scanner, PD_STARTXREF);
                        PDTwinStreamDiscardContent(parser->stream);
                        // we most likely also have a %%EOF
                        if (PDScannerPopStack(scanner, &stack)) {
                            if (PDIdentifies(stack->info, PD_META)) {
                                // yeh
                                PDTwinStreamDiscardContent(parser->stream);
                                pd_stack_destroy(&stack);
                            } else {
                                // whoops, no
                                pd_stack_push_stack(&scanner->resultStack, stack);
                            } 
                        }
#endif
                        return;
                    }
                }
                pd_stack_destroy(&stack);
            } else {
                PDScannerPopString(scanner, &string);
                free(string);
            }
            // <-- pass thru
        case PDParserStateObjectAppendix:
            PDTwinStreamAsserts(parser->stream);
            PDScannerPopString(scanner, &string);
            // we expect 'e'(ndobj) or 's'(tream)
            if (string[0] == 's')  {
                PDAssert(!strcmp(string, "stream"));
                free(string);
                // below assert is not valid; this actually came up on a PDF:
                /*
8 0 obj
<< /Length 19 0 R >>
stream
endstream
endobj
19 0 obj
0
endobj
                 */
                
                //PDAssert(parser->streamLen > 0);
                PDScannerSkip(scanner, parser->streamLen);
                PDTWinStreamPassthroughContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream));
                PDScannerAssertComplex(scanner, PD_ENDSTREAM);
                //PDScannerAssertString(scanner, "endstream");
                PDScannerPopString(scanner, &string);
            }
            
            // now we only expect 'e'(ndobj)
            if (string[0] == 'e') {
                PDAssert(!strcmp(string, "endobj"));
                free(string);
            } else {
                PDWarn("unknown type: %s\n", string);
                PDAssert(0);
            }
        default:
            break;
    }
    
    // to make things easy, we nudge the trail so the entire object is passed through
    ////scanner->btrail = scanner->boffset;
    
    // pass through the object; scanner is the master scanner, and will be adjusted by the stream
    PDTWinStreamPassthroughContent(parser->stream);//, PDTwinStreamScannerCommitBytes(parser->stream));
    
#ifdef PD_DEBUG_TWINSTREAM_ASSERT_OBJECTS
    char expect[100];
    PDInteger len = sprintf(expect, "%zd %zd obj", parser->obid, parser->genid);
    PDTwinStreamReassert(parser->stream, parser->oboffset, expect, len);
#endif
    
    parser->state = PDParserStateBase;
    
    parser->oboffset = (PDSize)PDTwinStreamGetOutputOffset(parser->stream);
    PDTwinStreamAsserts(parser->stream);
}

void PDParserPassoverObject(PDParserRef parser)
{
    char *string;
    pd_stack stack, entry;
    PDScannerRef scanner;
    
    scanner = parser->scanner;
    
    switch (parser->state) {
        case PDParserStateObjectDefinition:
            if (PDScannerPopStack(scanner, &stack)) {
                if (parser->encryptRef && parser->obid == parser->encryptRef->obid) {
                    // this is an encryption dictionary; those have a Length field that is not the length of the object stream
                } else {
                    entry = pd_stack_get_dict_key(stack, "Length", false);
                    if (entry) {
                        PDParserFetchStreamLengthFromObjectDictionary(parser, entry);
                    }
                }
                pd_stack_destroy(&stack);
            } else {
                PDScannerPopString(scanner, &string);
                free(string);
            }
            // <-- pass thru
        case PDParserStateObjectAppendix:
            PDTwinStreamAsserts(parser->stream);
            PDScannerPopString(scanner, &string);
            // we expect 'e'(ndobj) or 's'(tream)
            if (string[0] == 's')  {
                PDAssert(!strcmp(string, "stream"));
                free(string);
                // invalid; see other commented assertion // PDAssert(parser->streamLen > 0);
                PDScannerSkip(scanner, parser->streamLen);
                PDTwinStreamDiscardContent(parser->stream);
                PDScannerAssertComplex(scanner, PD_ENDSTREAM);
                //PDScannerAssertString(scanner, "endstream");
                PDScannerPopString(scanner, &string);
            }
            
            // now we only expect 'e'(ndobj)
            if (string[0] == 'e') {
                PDAssert(!strcmp(string, "endobj"));
                free(string);
            } else {
                PDWarn("unknown type: %s\n", string);
                PDAssert(0);
            }
        default:
            break;
    }
    
    // discard content
    PDTwinStreamDiscardContent(parser->stream);
    
    parser->state = PDParserStateBase;
    
    PDTwinStreamAsserts(parser->stream);
}

// append objects
void PDParserAppendObjects(PDParserRef parser)
{
    PDObjectRef obj;
    
    if (parser->state != PDParserStateBase)
        PDParserPassthroughObject(parser);
    
    while (parser->appends) {
        obj = parser->construct = pd_stack_pop_object(&parser->appends);
        parser->obid = obj->obid;
        parser->genid = obj->genid;
        parser->streamLen = obj->streamLen;
        PDParserPassthroughObject(parser);
    }
}

// advance to the next XRef domain
PDBool PDParserIterateXRefDomain(PDParserRef parser)
{
    // linearized XREF = ignore extraneous XREF records
    if (parser->cxt->linearized && parser->cxt->pos > PDTwinStreamGetInputOffset(parser->stream)) 
        return true;
    
    if (parser->xstack == NULL) {
        parser->done = true;
        
        PDParserAppendObjects(parser);
#ifdef DEBUG
        if (PDSplayTreeGetCount(parser->skipT) > 0) {
            PDInteger count = PDSplayTreeGetCount(parser->skipT);
            PDWarn("%ld skipped obsolete objects were never captured:", count);
            PDInteger *keys = malloc(sizeof(PDInteger) * count);
            PDSplayTreePopulateKeys(parser->skipT, keys);
            fprintf(stderr, "  ");
            for (PDInteger i = 0; i < count; i++) {
                fprintf(stderr, i?", %ld":"%ld", keys[i]);
            }
            fprintf(stderr, "\n");
        }
#endif
        PDAssert(0 == PDSplayTreeGetCount(parser->skipT)); // crash = we lost objects
        parser->success &= NULL == parser->xstack && 0 == PDSplayTreeGetCount(parser->skipT);
        return false;
    }
    
    PDXTableRef next = pd_stack_pop_object(&parser->xstack);
    
    PDBool retval = true;
    if (parser->cxt != parser->mxt) {
        PDRelease(parser->cxt);
        parser->cxt = next;

        if (parser->cxt->pos < PDTwinStreamGetInputOffset(parser->stream)) 
            retval = PDParserIterateXRefDomain(parser);
    } else {
        if (next->pos < PDTwinStreamGetInputOffset(parser->stream)) 
            retval = PDParserIterateXRefDomain(parser);
        PDRelease(next);
    }
    
    return retval;
}

// iterate to the next (non-deprecated) object
PDBool PDParserIterate(PDParserRef parser)
{
    PDBool running;
    PDBool skipObject;
    pd_stack stack;
    PDID typeid;
    PDScannerRef scanner = parser->scanner;
    PDXTableRef mxt = parser->mxt;
    size_t nextobid, nextgenid;

    PDTwinStreamAsserts(parser->stream);
    
    // parser may be done
    if (parser->done) 
        return false;
    
    // move past half-read objects
    if (PDParserStateBase != parser->state || NULL != parser->construct) {
        PDParserPassthroughObject(parser);
    }

    PDTwinStreamAsserts(parser->stream);
    
    while (true) {
        // we may have passed beyond the current binary XREF table
        if (parser->cxt->format == PDXTableFormatBinary && PDTwinStreamGetInputOffset(parser->stream) >= parser->cxt->pos) {
            if (! PDParserIterateXRefDomain(parser)) 
                // we've reached the end
                return false;
        }

        // discard up to this point
        if (scanner->boffset > 0) {
            PDTwinStreamDiscardContent(parser->stream);
        }
        
        PDTwinStreamAsserts(parser->stream);
        
        if (PDScannerPopStack(scanner, &stack)) {
            // mark output position
            parser->oboffset = scanner->bresoffset + (PDSize)PDTwinStreamGetOutputOffset(parser->stream);
            
            PDTwinStreamAsserts(parser->stream);
            
            // first is the type, returned as an identifier
            
            typeid = pd_stack_pop_identifier(&stack);

            // we expect PD_XREF, PD_STARTXREF, or PD_OBJ
            
            if (typeid == &PD_XREF) {
                // xref entry; note that we use running as the 'should consume trailer' argument to passover as the two states coincide; we also and result to the result of passover, as it may result in an XREF iteration in some special cases (in which case it may result in PDF end)
                running = PDParserIterateXRefDomain(parser);
                running &= PDXTablePassoverXRefEntry(parser, stack, running); // if !running, will all compilers enter PDXTablePassover...()? if not, this is bad

                if (! running) return false;
                
                continue;
            }
            
            if (typeid == &PD_OBJ) {
                // object definition; this is what we're after, unless this object is deprecated
                parser->obid = nextobid = pd_stack_pop_int(&stack);
                PDAssert(nextobid < parser->mxt->cap);
                parser->genid = nextgenid = pd_stack_pop_int(&stack);
                pd_stack_destroy(&stack);
                
                skipObject = false;
                parser->state = PDParserStateObjectDefinition;
                
                //printf("object %zd (genid = %zd)\n", nextobid, nextgenid);
                
                if (nextgenid != PDXTableGetGenForID(mxt, nextobid)) {
                    // this is the wrong object
                    skipObject = true;
                } else {
                    PDOffset xrefOffset = PDXTableGetOffsetForID(mxt, nextobid);
                    long long offset = scanner->bresoffset + PDTwinStreamGetInputOffset(parser->stream) - xrefOffset;
                    if (offset != 0) {
                        // okay, getting a slight bit out of hand here, but when we run into a bad offset, it sometimes is because someone screwed up and gave us:
                        // \r     \r\n1753 0 obj\r<</DecodeParm....... and claimed that the starting offset for 1753 0 was at the first \r, which it of course is not, it's 7 bytes later
                        PDInteger wsi = 0;
                        while (offset < 0 && PDOperatorSymbolGlob[(unsigned char)parser->stream->heap[parser->stream->cursor+(wsi++)]] == PDOperatorSymbolGlobWhitespace) 
                            offset++;
                        // we also got this fine situation:
                        // \n       4 0 obj, where the XREF pointed at the point right after the \n, resulting in a 7 byte offset
                        wsi = 1;
                        while (offset > 0 && PDOperatorSymbolGlob[(unsigned char)parser->stream->heap[parser->stream->cursor+scanner->bresoffset-(wsi++)]] == PDOperatorSymbolGlobWhitespace) 
                            offset--;
                    }
                    //printf("offset = %lld\n", offset);
                    if (offset < 2 && offset > -2) {
                        PDSplayTreeDelete(parser->skipT, nextobid);
                        //pd_btree_remove(&parser->skipT, nextobid);
                    } else {
                        // this is an old version of the object (in reality, PDF:s should not have the same object defined twice; in practice, Adobe InDesign CS5.5 (7.5) happily spews out 2 (+?) copies of the same object with different versions in the same PDF (admittedly the obs are separated by %%EOF and in separate appendings, but regardless)

                        // this object may be freed up or otherwise confused (why hello there, Acrobat Distiller 4.05), so we only add it to the expected objects tree if the master XREF considers it in use
                        if (xrefOffset > 0 && PDXTableGetTypeForID(mxt, nextobid) != PDXTypeFreed) {
                            //printf("offset mismatch for object %zd\n", nextobid);
                            PDSplayTreeInsert(parser->skipT, nextobid, (void*)nextobid);
                            //pd_btree_insert(&parser->skipT, nextobid, (void*)nextobid);
                        }
                        skipObject = true;
                    }
                }
                
                if (skipObject) {
                    // move past object
                    PDParserPassoverObject(parser);
                    continue;
                } 
                
                return true;
            }
            
            if (typeid == &PD_STARTXREF) {
                // a trailing startxref entry
                pd_stack_destroy(&stack);
                PDScannerAssertComplex(scanner, PD_META);
                // snip
                PDTwinStreamDiscardContent(parser->stream);
                continue;
            }

            PDWarn("unknown type: %s\n", *typeid);
            PDAssert(0);
        } else {
            // we failed to get a stack which is very odd
            PDWarn("failed to pop stack from PDF stream; the unexpected string is \"%s\"\n", (char*)scanner->resultStack->info);
            PDAssert(0);
            return false;
        }
    }
    
    return false;
}

PDObjectRef PDParserCreateObject(PDParserRef parser, pd_stack *queue)
{
    size_t newiter, count, cap;
//    char *xrefs;
    PDXTableRef table;
    
    // we enqueue the object rather than making it the current construct, if we have a construct already, or if it's supposed to be appended
    if (queue == NULL && (parser->state != PDParserStateBase || parser->construct)) {
        // we have a construct, so put it in the inserts queue
        queue = &parser->inserts;
    }
    
    // [deprecated in favor of inserts queue] we cannot be inside another object when we do this
    //if (parser->state != PDParserStateBase || parser->construct) 
    //    PDParserPassthroughObject(parser);
    
    newiter = parser->xrefnewiter;
    count = parser->mxt->count;
    cap = parser->mxt->cap;
    table = parser->mxt;
//    xrefs = table->xrefs;
    
    while (newiter < count && PDXTypeFreed != PDXTableGetTypeForID(table, newiter))
        newiter++;
    if (newiter == cap) {
        // we must realloc xref as it can't contain all the xrefs
        table->count = cap = cap + 1;
        PDXTableGrow(table, cap);
//        xrefs = table->xrefs;
//        xrefs = table->xrefs = realloc(table->xrefs, table->width * cap);
    }
    PDXTableSetTypeForID(table, newiter, PDXTypeUsed);
    PDXTableSetGenForID(table, newiter, 0);
    
    parser->xrefnewiter = newiter;
    
    PDObjectRef object = PDObjectCreate(newiter, 0);
    object->encryptedDoc = PDParserGetEncryptionState(parser);
    object->crypto = parser->crypto;
    PDSplayTreeInsert(parser->aiTree, newiter, PDRetain(object));
    
    if (queue) {
        pd_stack_push_object(queue, object);
    } else {
        parser->obid = newiter;
        parser->genid = 0;
        parser->streamLen = 0;
        parser->construct = object;
    }
    
    // we need to retain the object twice; once because this returns a retained object, and once because we retain it ourselves, and release it in "UpdateObject"
    return PDRetain(object);
}

PDObjectRef PDParserCreateNewObject(PDParserRef parser)
{
    return PDParserCreateObject(parser, NULL);
}

PDObjectRef PDParserCreateAppendedObject(PDParserRef parser)
{
    return PDParserCreateObject(parser, &parser->appends);
}

// construct a PDObjectRef for the current object
PDObjectRef PDParserConstructObject(PDParserRef parser)
{
    // we may have an object already; this is the case if someone constructs twice, or if someone constructs the very first object, which is always constructed
    if (parser->construct && parser->construct->obid == parser->obid)
        return parser->construct;
    
    PDAssert(parser->state == PDParserStateObjectDefinition);
    PDAssert(parser->construct == NULL);
    PDObjectRef object = parser->construct = PDObjectCreate(parser->obid, parser->genid);
    object->crypto = parser->crypto;
    object->encryptedDoc = PDParserGetEncryptionState(parser);

    char *string;
    pd_stack stack;
    
    PDScannerRef scanner = parser->scanner;
    
    if (PDScannerPopStack(scanner, &stack)) {
        object->def = stack;
        object->type = PDObjectTypeFromIdentifier(stack->info);
        
        if (parser->encryptRef && parser->obid == parser->encryptRef->obid) {
            // this is an encryption dictionary; those have a Length field that is not the length of the object stream
            parser->streamLen = 0;
        } else {
            if ((stack = pd_stack_get_dict_key(stack, "Length", false))) {
                PDParserFetchStreamLengthFromObjectDictionary(parser, stack);
                object->streamLen = parser->streamLen;
            } else {
                parser->streamLen = 0;
            }
        }
    } else {
        PDScannerPopString(scanner, &string);
        pd_stack_push_freeable(&object->def, string);
        pd_stack_push_identifier(&object->def, &PD_STRING);
//        object->def = string;
        object->type = PDObjectTypeString;
    }
    
    PDScannerPopString(scanner, &string);
    // we expect 'e'(ndobj) or 's'(tream)
    if (string[0] == 's') {
        PDAssert(!strcmp(string, "stream"));
        free(string);
        object->hasStream = true;
    
        parser->state = PDParserStateObjectAppendix;
    } else if (string[0] == 'e') {
        PDAssert(!strcmp(string, "endobj"));
        free(string);

        // align trail as if we passed over this object (because we essentially did, even though we still may submit a modified version of it)
        ////scanner->btrail = scanner->boffset;
        
        parser->state = PDParserStateBase;
    }
    
    return object;
}

PDBool PDParserGetEncryptionState(PDParserRef parser)
{
    return NULL != parser->encryptRef;
}

void PDParserDone(PDParserRef parser)
{
    PDAssert(parser->success);

#define twinstream_printf(fmt...) \
        len = sprintf(obuf, fmt); \
        PDTwinStreamInsertContent(stream, len, obuf)
#define twinstream_put(len, buf) \
        PDTwinStreamInsertContent(stream, len, (char*)buf);
    char *obuf = malloc(512);
    PDInteger len;
    PDTwinStreamRef stream = parser->stream;
    
    // iterate past all remaining objects, if any
    while (PDParserIterate(parser));
    
    // the output offset is our new startxref entry
    PDSize startxref = (PDSize)PDTwinStreamGetOutputOffset(parser->stream);
    
    // write XREF table and trailer
    PDXTableInsert(parser);
    
    // write startxref entry
    twinstream_printf("startxref\n%zu\n%%%%EOF\n", startxref);
    
    free(obuf);
}

PDInteger PDParserGetContainerObjectIDForObject(PDParserRef parser, PDInteger obid)
{
    if (PDXTypeComp != PDXTableGetTypeForID(parser->mxt, obid)) 
        return -1;
    
    return (PDInteger) PDXTableGetOffsetForID(parser->mxt, obid);
}

PDBool PDParserIsObjectStillMutable(PDParserRef parser, PDInteger obid)
{
    return (PDTwinStreamGetInputOffset(parser->stream) <= PDXTableGetOffsetForID(parser->mxt, (PDXTableGetTypeForID(parser->mxt, obid) == PDXTypeComp
                                                                                               ? (PDInteger)PDXTableGetOffsetForID(parser->mxt, obid) 
                                                                                               : obid)));
}

PDObjectRef PDParserGetRootObject(PDParserRef parser)
{
    if (! parser->root) {
        parser->root = PDParserLocateAndCreateObject(parser, parser->rootRef->obid, true);
        //pd_stack rootDef = PDParserLocateAndCreateDefinitionForObject(parser, parser->rootRef->obid, true);
        //parser->root = PDObjectCreate(parser->rootRef->obid, parser->rootRef->genid);
        //parser->root->def = rootDef;
        //parser->root->crypto = parser->crypto;
    }
    return parser->root;
}

PDObjectRef PDParserGetInfoObject(PDParserRef parser)
{
    if (! parser->info && parser->infoRef) {
        parser->info = PDParserLocateAndCreateObject(parser, parser->infoRef->obid, true);
        //pd_stack infoDef = PDParserLocateAndCreateDefinitionForObject(parser, parser->infoRef->obid, true);
        //parser->info = PDObjectCreate(parser->infoRef->obid, parser->infoRef->genid);
        //parser->info->def = infoDef;
        //parser->info->crypto = parser->crypto;
    }
    return parser->info;
}

PDObjectRef PDParserGetTrailerObject(PDParserRef parser)
{
    return parser->trailer;
}

PDCatalogRef PDParserGetCatalog(PDParserRef parser)
{
    if (! parser->catalog) {
        PDObjectRef root = PDParserGetRootObject(parser);
        parser->catalog = PDCatalogCreateWithParserForObject(parser, root);
    }
    return parser->catalog;
}

PDInteger PDParserGetTotalObjectCount(PDParserRef parser)
{
    return parser->mxt->count;
}

