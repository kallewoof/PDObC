//
// PDCatalog.c
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
#include "PDCatalog.h"

#include "PDParser.h"

#include "pd_internal.h"
#include "pd_stack.h"
#include "PDDictionary.h"
#include "PDArray.h"
#include "PDNumber.h"
#include "PDString.h"
#include "pd_pdf_implementation.h"

void PDPageReferenceDestroy(PDPageReference * page)
{
    if (page->collection) {
        for (long i = page->count - 1; i >= 0; i--)
            PDPageReferenceDestroy(&page->kids[i]);
        free(page->kids);
    }
}

void PDCatalogDestroy(PDCatalogRef catalog)
{
    PDPageReferenceDestroy(&catalog->pages);
    PDRelease(catalog->object);
    free(catalog->kids);
}

void PDCatalogAppendPages(PDCatalogRef catalog, PDPageReference *pages, PDDictionaryRef ddict)
{
//    pd_stack s = defs;
//    PDDictionaryRef ddict = PDInstanceCreateFromComplex(&s);
    pages->collection = true;
    PDParserRef parser = catalog->parser;
    
    PDInteger count = PDNumberGetInteger(PDDictionaryGet(ddict, "Count"));
//    PDInteger count = PDIntegerFromString(pd_stack_get_dict_key(defs, "Count", false)->prev->prev->info);
    PDAssert(count > 0);
    if (count + catalog->count >= catalog->capacity) {
        catalog->capacity = catalog->count + count;
        catalog->kids = realloc(catalog->kids, sizeof(PDInteger) * catalog->capacity);
    }
    
    PDArrayRef kidsArray = PDDictionaryGet(ddict, "Kids");
//    pd_stack kidsStack = pd_stack_get_dict_key(defs, "Kids", true);
//    pd_stack_destroy(&defs);
//    pd_stack kidsArr = pd_stack_get_arr(kidsStack);
    
    PDInteger lcount = pages->count = PDArrayGetCount(kidsArray); //PDArrayGetCount(PDObjectGetArray(kidsArray));
    
    PDInteger *ckids = catalog->kids;
    PDPageReference *kids = pages->kids = malloc(sizeof(PDPageReference) * lcount);
    
    /*
     stack<0x113c5c10> {
         0x3f99a0 ("ae")
         stack<0x113532b0> {
             0x3f9988 ("ref")
             557
             0
         }
     }
     stack<0x113d49b0> {
         0x3f99a0 ("ae")
         stack<0x113efa50> {
             0x3f9988 ("ref")
             558
             0
         }
     }
     */
//    pd_stack stack;
    PDInteger oid;
    for (PDInteger i = 0; i < lcount; i++) {
        PDReferenceRef ref = PDArrayGetElement(kidsArray, i);
        oid = PDReferenceGetObjectID(ref);
//        stack = as(pd_stack, as(pd_stack, iter->info)->prev->info)->prev;
//        oid = PDIntegerFromString(stack->info);
        PDObjectRef kob = PDParserLocateAndCreateObject(parser, oid, true);
//        pd_stack defs = PDParserLocateAndCreateDefinitionForObject(parser, oid, true);
//        PDAssert(defs); // crash = above function is failing; it may start failing if an object is "weird", or if the code to fetch objects is broken (e.g. PDScanner, PDTwinStream, or even PDParser)
        PDDictionaryRef kdict = PDObjectGetDictionary(kob); //PDInstanceCreateFromComplex(&defs);
//        pd_stack_destroy(&defs);
        PDStringRef type = PDDictionaryGet(kdict, "Type");
//        char *type = as(pd_stack, pd_stack_get_dict_key(defs, "Type", false)->prev->prev->info)->prev->info;
        if (PDStringEqualsCString(type, "Pages")) {
//        if (0 == strcmp(type, "Pages")) {
            PDCatalogAppendPages(catalog, &kids[i], kdict);
        } else {
            ckids[catalog->count++] = oid;
            kids[i].collection = false;
            kids[i].obid = oid;
            kids[i].genid = PDReferenceGetGenerationID(ref); //PDIntegerFromString(stack->prev->info);
//            pd_stack_destroy(&defs);
        }
        PDRelease(kob);
//        i++;
    }
    
//    pd_stack_destroy(&kidsStack);
    /*for (PDInteger i = 0; i < count; i++) {
     //sref = PDArrayGetElement(PDObjectGetArray(kidsArray), i); // 2 0 R
     stack = pd_stack_get_arr_element(kidsStack, i);
     kids[i] = PDIntegerFromString(sref);
     }
     
     PDRelease(kidsArray);*/
}

PDCatalogRef PDCatalogCreateWithParserForObject(PDParserRef parser, PDObjectRef catalogObject)
{
    PDReferenceRef sref;
    
    PDCatalogRef catalog = PDAlloc(sizeof(struct PDCatalog), PDCatalogDestroy, false);
    catalog->parser = parser;
    catalog->object = PDRetain(catalogObject);
    catalog->count = 0;
    catalog->capacity = 0;
    catalog->kids = NULL;
    catalog->pages.collection = false;
    
    // catalogObject has the form
    //  << /Type /Catalog /Pages 3 0 R >>
    // so the first thing we do is fetch that /Pages object
    PDDictionaryRef coDict = PDObjectGetDictionary(catalogObject);
    sref = PDDictionaryGet(coDict, "Pages");
    PDAssert(sref != NULL); // catalogObject is most likely not a valid catalog

    // the Pages object looks like:
    //  << /Type /Pages /MediaBox [0 0 1024 768] /Count 1 /Kids [ 2 0 R ] >>
    // note that MediaBox is currently ignored
    
    PDObjectRef pages = PDParserLocateAndCreateObject(parser, PDReferenceGetObjectID(sref), true);
//    pd_stack defs = PDParserLocateAndCreateDefinitionForObject(parser, PDReferenceGetObjectID(sref), true);
    if (! pages) {
        PDWarn("null pages object for catalog");
        PDRelease(catalog);
        return NULL;
    }
    PDDictionaryRef dict = PDObjectGetDictionary(pages); //PDInstanceCreateFromComplex(&defs);
    PDCatalogAppendPages(catalog, &catalog->pages, dict);
//    pd_stack_destroy(&defs);
//    PDRelease(dict);
    PDRelease(pages);
    
    return catalog;
}

PDInteger PDCatalogGetObjectIDForPage(PDCatalogRef catalog, PDInteger pageNumber)
{
    PDAssert(pageNumber > 0 && pageNumber <= catalog->count);
    return catalog->kids[pageNumber-1];
}

PDInteger PDCatalogGetPageCount(PDCatalogRef catalog)
{
    return catalog->count;
}

void PDCatalogInsertPage(PDCatalogRef catalog, PDInteger pageNumber, PDObjectRef pageObject)
{
    if (catalog->count == catalog->capacity) {
        catalog->capacity += 5;
        catalog->kids = realloc(catalog->kids, sizeof(PDInteger) * catalog->capacity);
    }
    PDInteger pageObjectID = PDObjectGetObID(pageObject);
    PDInteger t;
    for (PDInteger i = pageNumber - 1; i <= catalog->count; i++) {
        t = catalog->kids[i];
        catalog->kids[i] = pageObjectID;
        pageObjectID = t;
    }
    catalog->count++;
}
