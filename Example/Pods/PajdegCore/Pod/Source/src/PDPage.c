//
// PDPage.c
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

#include "pd_internal.h"
#include "pd_stack.h"
#include "PDScanner.h"
#include "pd_pdf_implementation.h"
#include "pd_pdf_private.h"
#include "PDStreamFilter.h"
#include "PDObject.h"
#include "PDPage.h"
#include "PDParser.h"
#include "PDParserAttachment.h"
#include "PDCatalog.h"
#include "PDArray.h"
#include "PDDictionary.h"
#include "PDNumber.h"

void PDPageDestroy(PDPageRef page)
{
    PDRelease(page->parser);
    PDRelease(page->ob);

    if (page->contentObs) {
        for (PDInteger i = 0; i < page->contentCount; i++) {
            PDRelease(page->contentObs[i]);
        }
        PDRelease(page->contentRefs);
        free(page->contentObs);
    }
}

PDPageRef PDPageCreateForPageWithNumber(PDParserRef parser, PDInteger pageNumber)
{
    PDCatalogRef catalog = PDParserGetCatalog(parser);
    PDInteger obid = PDCatalogGetObjectIDForPage(catalog, pageNumber);
    PDObjectRef ob = PDParserLocateAndCreateObject(parser, obid, true);
    PDPageRef page = PDPageCreateWithObject(parser, ob);
    
//    const char *contentsRef = PDDictionaryGetEntry(PDObjectGetDictionary(ob), "Contents");
//    assert(contentsRef);
//    PDObjectRef contentsOb = PDParserLocateAndCreateObject(parser, PDIntegerFromString(contentsRef), true);
//    char *stream = PDParserLocateAndFetchObjectStreamForObject(parser, contentsOb);
//    printf("page #%ld:\n===\n%s\n", (long)pageIndex, stream);
//    PDRelease(contentsOb);
    
    PDRelease(ob);
    return page;
}

PDPageRef PDPageCreateWithObject(PDParserRef parser, PDObjectRef object)
{
    PDPageRef page = PDAlloc(sizeof(struct PDPage), PDPageDestroy, false);
    page->parser = PDRetain(parser);
    page->ob = PDRetain(object);
    page->contentRefs = NULL;
    page->contentObs = NULL;
    page->contentCount = -1;
    return page;
}

PDTaskResult PDPageInsertionTask(PDPipeRef pipe, PDTaskRef task, PDObjectRef object, void *info)
{
//    char *buf = malloc(64);
    
    PDObjectRef *userInfo = info;
    PDObjectRef neighbor = userInfo[0];
    PDObjectRef importedObject = userInfo[1];
    free(userInfo);
    
    PDDictionaryRef dict = PDObjectGetDictionary(object);
    PDArrayRef kids = PDDictionaryGetArray(dict, "Kids");
//    pd_array_print(kids);
    if (NULL != neighbor) {
//        sprintf(buf, "%ld 0 R", PDObjectGetObID(neighbor));
        PDInteger index = PDArrayGetIndex(kids, neighbor);
        //pd_array_get_index_of_value(kids, buf);
        if (index < 0) {
            // neighbor not in there
            PDError("expected neighbor not found in Kids array");
            PDRelease(neighbor);
            neighbor = NULL;
        } else {
//            sprintf(buf, "%ld 0 R", PDObjectGetObID(importedObject));
            PDArrayInsertAtIndex(kids, index, importedObject);
//            pd_array_insert_at_index(kids, index, buf);
        }
    }
    
    if (NULL == neighbor) {
//        sprintf(buf, "%ld 0 R", PDObjectGetObID(importedObject));
        PDArrayAppend(kids, importedObject);
//        pd_array_append(kids, buf);
    }
//    pd_array_print(kids);
    
//    pd_dict_set_raw(dict, "Kids", pd_array_to_stack(kids));

//    free(buf);
    PDRelease(importedObject);
    PDRelease(neighbor);
    
    // we can (must, in fact) unload this task as it only applies to a specific page
    return PDTaskUnload;
}

PDTaskResult PDPageCountupTask(PDPipeRef pipe, PDTaskRef task, PDObjectRef object, void *info)
{
    // because every page add results in a countup task and we only need one (because parent is reused for each), we simply test if count is up to date and only update if it isn't
    /// @todo Add a PDTaskSkipSame flag and make PDTaskResults OR-able where appropriate
    PDObjectRef source = info;
    
    PDDictionaryRef obDict = PDObjectGetDictionary(object);
    PDNumberRef realCountNum = PDDictionaryGetEntry(PDObjectGetDictionary(source), "Count");
    PDInteger realCount = PDNumberGetInteger(realCountNum);
    PDInteger obCount   = PDDictionaryGetInteger(obDict, "Count");
    if (obCount != realCount) {
        PDDictionarySetEntry(obDict, "Count", realCountNum);
//    const char *realCount = PDDictionaryGetEntry(PDObjectGetDictionary(source), "Count");
//    if (strcmp(realCount, PDDictionaryGetEntry(PDObjectGetDictionary(object), "Count"))) {
//        PDDictionarySetEntry(PDObjectGetDictionary(object), "Count", realCount);
    }
    
    PDRelease(source);
    
    return PDTaskUnload;
}

PDPageRef PDPageInsertIntoPipe(PDPageRef page, PDPipeRef pipe, PDInteger pageNumber)
{
    PDParserAttachmentRef attachment = PDPipeConnectForeignParser(pipe, page->parser);
    
    PDParserRef parser = PDPipeGetParser(pipe);
    PDAssert(parser != page->parser); // attempt to insert page into the same parser object; this is currently NOT supported even though it's radically simpler
    
    // we start by importing the object and its indirect companions over into the target parser
    PDObjectRef importedObject = PDParserAttachmentImportObject(attachment, page->ob, (const char *[]) {"Parent"}, 1);
    
    // we now try to hook it up with a neighboring page's parent; what better neighbor than the page index itself, unless it exceeds the page count?
    PDCatalogRef cat = PDParserGetCatalog(parser);
    PDInteger pageCount = PDCatalogGetPageCount(cat);
    PDInteger neighborPI = pageNumber > pageCount ? pageCount : pageNumber;
    PDAssert(neighborPI > 0 && neighborPI <= pageCount); // crash = attempt to insert a page outside of the bounds of the destination PDF (i.e. the page index is higher than page count + 1, which would result in a hole with no pages)
    
    PDInteger neighborObID = PDCatalogGetObjectIDForPage(cat, neighborPI);
    PDObjectRef neighbor = PDParserLocateAndCreateObject(parser, neighborObID, true);
    PDDictionaryRef neighDict = PDObjectGetDictionary(neighbor);
    PDDictionaryRef ioDict = PDObjectGetDictionary(importedObject);
    PDReferenceRef parentRef = PDDictionaryGetReference(neighDict, "Parent");
//    const char *parentRef = PDDictionaryGetEntry(PDObjectGetDictionary(neighbor), "Parent");
    PDAssert(parentRef); // crash = ??? no such page? no parent? can pages NOT have parents?
    PDDictionarySetEntry(ioDict, "Parent", parentRef);
//    PDDictionarySetEntry(PDObjectGetDictionary(importedObject), "Parent", parentRef);
    
    PDInteger parentId = PDReferenceGetObjectID(parentRef);
//    PDAssert(parentId); // crash = parentRef was not a <num> <num> R?
    
    PDObjectRef *userInfo = malloc(sizeof(PDObjectRef) * 2);
    userInfo[0] = (neighborPI == pageNumber ? neighbor : NULL);
    userInfo[1] = PDRetain(importedObject);
    PDTaskRef task = PDTaskCreateMutatorForObject(parentId, PDPageInsertionTask);
    PDTaskSetInfo(task, userInfo);
    PDPipeAddTask(pipe, task);
    PDRelease(task);

    // also recursively update grand parents
    PDObjectRef parent = PDParserLocateAndCreateObject(parser, parentId, true);
    while (parent) {
//        char buf[16];
        PDDictionaryRef parentDict = PDObjectGetDictionary(parent);
        PDNumberRef count = PDNumberCreateWithInteger(1 + PDDictionaryGetInteger(parentDict, "Count"));
//        PDInteger count = 1 + PDIntegerFromString(PDDictionaryGetEntry(PDObjectGetDictionary(parent), "Count"));
//        sprintf(buf, "%ld", (long)count);
//        PDDictionarySetEntry(PDObjectGetDictionary(parent), "Count", buf);
        PDDictionarySetEntry(parentDict, "Count", count);
        PDRelease(count);
        
        PDTaskRef task = PDTaskCreateMutatorForObject(parentId, PDPageCountupTask);
        PDTaskSetInfo(task, PDRetain(parent));
        PDPipeAddTask(pipe, task);
        PDRelease(task);
        PDRelease(parent);
        
        parentRef = PDDictionaryGetReference(parentDict, "Parent");
        parent = NULL;
        if (parentRef) {
            parentId = PDReferenceGetObjectID(parentRef); //PDIntegerFromString(parentRef);
            parent = PDParserLocateAndCreateObject(parser, parentId, true);
            if (NULL == parent) {
                PDWarn("null page parent for id #%ld", (long)parentId);
            }
        }
    }

    // update the catalog
    PDCatalogInsertPage(cat, pageNumber, importedObject);
    
    // finally, set up the new page and return it
    PDPageRef importedPage = PDPageCreateWithObject(parser, importedObject);
    return PDAutorelease(importedPage);
}

PDInteger PDPageGetContentsObjectCount(PDPageRef page)
{
    if (page->contentCount == -1) PDPageGetContentsObjectAtIndex(page, 0);
    return page->contentCount;
}

PDObjectRef PDPageGetContentsObjectAtIndex(PDPageRef page, PDInteger index)
{
    if (page->contentObs == NULL) {
        // we need to set the array up first
        PDDictionaryRef d = PDObjectGetDictionary(page->ob);
//        pd_dict d = PDObjectGetDictionary(page->ob);
        void *contentsValue = PDDictionaryGetEntry(d, "Contents");
        if (PDResolve(contentsValue) == PDInstanceTypeArray) {
//        if (PDObjectTypeArray == pd_dict_get_type(d, "Contents")) {
//            page->contentRefs = pd_dict_get_copy(d, "Contents");
            page->contentRefs = PDRetain(contentsValue);
            page->contentCount = PDArrayGetCount(contentsValue);
            page->contentObs = calloc(page->contentCount, sizeof(PDObjectRef));
        } else {
//            const char *contentsRef = PDDictionaryGetEntry(PDObjectGetDictionary(page->ob), "Contents");
            if (contentsValue) {
                page->contentCount = 1;
                page->contentObs = malloc(sizeof(PDObjectRef));
                PDInteger contentsId = PDReferenceGetObjectID(contentsValue);
                page->contentObs[0] = PDParserLocateAndCreateObject(page->parser, contentsId, true);
            } else return NULL;
        }
    }
    
    PDAssert(index >= 0 && index < page->contentCount); // crash = index out of bounds
    
    if (NULL == page->contentObs[index]) {
        PDReferenceRef contentsRef = PDArrayGetReference(page->contentRefs, index);
//        PDInteger contentsId = PDArrayGetInteger(page->contentRefs, index);
//        PDInteger contentsId = PDIntegerFromString(pd_array_get_at_index(page->contentRefs, index));
        page->contentObs[index] = PDParserLocateAndCreateObject(page->parser, PDReferenceGetObjectID(contentsRef), true);
    }
    
    return page->contentObs[index];
}

PDRect PDPageGetMediaBox(PDPageRef page)
{
    PDRect rect = (PDRect) {{0,0}, {612,792}};
    PDDictionaryRef obdict = PDObjectGetDictionary(page->ob);
    void *mediaBoxValue = PDDictionaryGetEntry(obdict, "MediaBox");
    if (mediaBoxValue && PDInstanceTypeArray == PDResolve(mediaBoxValue)) {
//    if (PDObjectTypeArray == PDObjectGetDictionaryEntryType(page->ob, "MediaBox")) {
//        pd_array arr = PDObjectCopyDictionaryEntry(page->ob, "MediaBox");
//        if (4 == pd_array_get_count(arr)) {
        if (4 == PDArrayGetCount(mediaBoxValue)) {
            rect = (PDRect) {
                {
                    PDNumberGetReal(PDArrayGetElement(mediaBoxValue, 0)),
                    PDNumberGetReal(PDArrayGetElement(mediaBoxValue, 1)),
//                    PDRealFromString(pd_array_get_at_index(arr, 0)),
//                    PDRealFromString(pd_array_get_at_index(arr, 1))
                },
                {
                    PDNumberGetReal(PDArrayGetElement(mediaBoxValue, 2)),
                    PDNumberGetReal(PDArrayGetElement(mediaBoxValue, 3)),
//                    PDRealFromString(pd_array_get_at_index(arr, 2)),
//                    PDRealFromString(pd_array_get_at_index(arr, 3))
                }
            };
        } else {
            PDNotice("invalid count for MediaBox array: %ld (require 4: x1, y1, x2, y2)", PDArrayGetCount(mediaBoxValue));
        }
    }
    return rect;
}

PDArrayRef PDPageGetAnnotRefs(PDPageRef page)
{
    void *res = PDDictionaryGetEntry(PDObjectGetDictionary(page->ob), "Annots");
    if (PDResolve(res) == PDInstanceTypeRef) {
        PDReferenceRef ref = res;
        PDObjectRef ob = PDParserLocateAndCreateObject(page->parser, PDReferenceGetObjectID(ref), true);
        res = PDObjectGetArray(ob);
        PDAutorelease(ob);
    }
    return res;
}
