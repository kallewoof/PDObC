//
// PDPipe.c
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
#include "PDTwinStream.h"
#include "PDReference.h"
#include "PDSplayTree.h"
#include "pd_stack.h"
#include "PDDictionary.h"
#include "PDParserAttachment.h"
#include "PDCatalog.h"
#include "PDStaticHash.h"
#include "PDObjectStream.h"
#include "PDXTable.h"
#include "PDString.h"

static char *PDFTypeStrings[_PDFTypeCount] = {kPDFTypeStrings};

void PDPipeCloseFileStream(FILE *stream)
{
    fclose(stream);
}

FILE *PDPipeOpenInputStream(const char *path)
{
    return fopen(path, "r");
}

FILE *PDPipeOpenOutputStream(const char *path)
{
    return fopen(path, "w+");
}

//
//
//

void PDPipeDestroy(PDPipeRef pipe)
{
    PDTaskRef task;
    
    if (pipe->opened) {
        PDPipeCloseFileStream(pipe->fi);
        PDPipeCloseFileStream(pipe->fo);
        PDRelease(pipe->stream);
        PDRelease(pipe->parser);
    }
    free(pipe->pi);
    free(pipe->po);
    PDRelease(pipe->filter);
    PDRelease(pipe->attachments);
    
    for (int i = 0; i < _PDFTypeCount; i++) {
        pd_stack stack = pipe->typeTasks[i];
        while (NULL != (task = (PDTaskRef)pd_stack_pop_identifier(&stack))) {
            PDRelease(task);
        }
    }
}

PDPipeRef PDPipeCreateWithFilePaths(const char * inputFilePath, const char * outputFilePath)
{
    FILE *fi;
    FILE *fo;

    // input must be set
    if (inputFilePath == NULL) return NULL;
    // we do want to support NULL output for 'readonly mode' but we don't, now; NIX users can pass "/dev/null" to get this behavior
    if (outputFilePath == NULL) return NULL;
    
    // files must not be the same
    if (!strcmp(inputFilePath, outputFilePath)) return NULL;
    
    fi = PDPipeOpenInputStream(inputFilePath);
    if (NULL == fi) {
        return NULL;
    }
    PDPipeCloseFileStream(fi);
    
    fo = PDPipeOpenOutputStream(outputFilePath);
    if (NULL == fo) {
        return NULL;
    }
    PDPipeCloseFileStream(fo);
    
    PDPipeRef pipe = PDAlloc(sizeof(struct PDPipe), PDPipeDestroy, true);
    pipe->pi = strdup(inputFilePath);
    pipe->po = strdup(outputFilePath);
    pipe->attachments = PDSplayTreeCreateWithDeallocator(PDReleaseFunc);
    return pipe;
}

PDTaskResult PDPipeObStreamMutation(PDPipeRef pipe, PDTaskRef task, PDObjectRef object, void *info)
{
    PDTaskRef subTask;
    PDObjectRef ob;
    PDObjectStreamRef obstm;
    pd_stack mutators;
    pd_stack iter;
    char *stmbuf;
    
    obstm = PDObjectStreamCreateWithObject(object);
    stmbuf = PDParserFetchCurrentObjectStream(pipe->parser, object->obid);
    PDObjectStreamParseExtractedObjectStream(obstm, stmbuf);
    
    mutators = info;
    iter = mutators;
    
    while (iter) {
        // each entry is a PDTask that is a filter on a specific object, and the object is located inside of the stream
        subTask = iter->info;
        PDAssert(subTask->isFilter);
        ob = PDObjectStreamGetObjectByID(obstm, subTask->value);
        PDAssert(ob);
        if (PDTaskFailure == PDTaskExec(subTask->child, pipe, ob)) {
            return PDTaskFailure;
        }
        iter = iter->prev;
    }
    
    PDObjectStreamCommit(obstm);
    
    return PDTaskDone;
}

void PDPipeAddTask(PDPipeRef pipe, PDTaskRef task)
{
    PDAssert(pipe);
    
    PDCatalogRef catalog;
    long key;
    
    if (task->isFilter) {
        PDAssert(task->child);
        pipe->dynamicFiltering = true;
        
        if (! pipe->opened && ! PDPipePrepare(pipe)) 
            return;
        
        switch (task->propertyType) {
            case PDPropertyObjectId:
                key = task->value;
                break;
                
            case PDPropertyInfoObject:
                key = pipe->parser->infoRef ? pipe->parser->infoRef->obid : -1;
                break;
            
            case PDPropertyRootObject:
                key = pipe->parser->rootRef ? pipe->parser->rootRef->obid : -1;
                break;
            
            case PDPropertyPage:
                catalog = PDParserGetCatalog(pipe->parser);
                key = PDCatalogGetObjectIDForPage(catalog, task->value);
                break;
                
            case PDPropertyPDFType:
                pipe->typedTasks = true;
                PDAssert(task->value > 0 && task->value < _PDFTypeCount); // crash = value out of range; must be set to a PDFType!
                
                // task executes on every object of the given type
                pd_stack_push_identifier(&pipe->typeTasks[task->value], (PDID)PDRetain(task->child));
                return;
        }
        
        // if this is a reference to an object inside an object stream, we have to pull that open
        PDInteger containerOb = PDParserGetContainerObjectIDForObject(pipe->parser, key);
        if (containerOb != -1) {
            // force the value into the task, in case this was a root or info req
            task->value = key;
            PDTaskRef containerTask = PDSplayTreeGet(pipe->filter, containerOb);
            //pd_btree_fetch(pipe->filter, containerOb);
            if (NULL == containerTask) {
                // no container task yet so we set one up
                containerTask = PDTaskCreateMutator(PDPipeObStreamMutation);
                pipe->filterCount++;
                PDSplayTreeInsert(pipe->filter, containerOb, containerTask);
                //pd_btree_insert(&pipe->filter, containerOb, containerTask);
                containerTask->info = NULL;
            }
            pd_stack_push_object((pd_stack *)&containerTask->info, PDRetain(task));
            return;
        }
        
        PDTaskRef sibling = PDSplayTreeGet(pipe->filter, key);
        //pd_btree_fetch(pipe->filter, key);
        if (sibling) {
            // same filters; merge
            PDTaskAppendTask(sibling, task->child);
        } else {
            // not same filters; include
            pipe->filterCount++;
            PDSplayTreeInsert(pipe->filter, key, PDRetain(task->child));
            //pd_btree_insert(&pipe->filter, key, PDRetain(task->child));
        }
        
        if (pipe->opened && ! PDParserIsObjectStillMutable(pipe->parser, key)) {
            // pipe's open and we've already passed the object being filtered
            PDError("*** object %ld cannot be accessed as it has already been written ***\n", key);
            PDParserIsObjectStillMutable(pipe->parser, key);
            PDAssert(0); // crash = logic is flawed; object in question should be fetched after preparing pipe rather than dynamically appending filters as data is obtained; worst case, do two passes (one where the id of the offending object is determined and one where the mutations are made)
        }
    } else {
        // task executes on every iteration
        pd_stack_push_identifier(&pipe->typeTasks[0], (PDID)PDRetain(task));
    }
}

PDParserRef PDPipeGetParser(PDPipeRef pipe)
{
    if (! pipe->opened) 
        if (! PDPipePrepare(pipe))
            return NULL;
    
    return pipe->parser;
}

PDObjectRef PDPipeGetRootObject(PDPipeRef pipe)
{
    if (! pipe->opened) 
        if (! PDPipePrepare(pipe)) 
            return NULL;
    
    return PDParserGetRootObject(pipe->parser);
}

PDBool PDPipePrepare(PDPipeRef pipe)
{
    if (pipe->opened) {
        return true;
    }
    
    pipe->fi = PDPipeOpenInputStream(pipe->pi);
    if (NULL == pipe->fi) return false;
    pipe->fo = PDPipeOpenOutputStream(pipe->po);
    if (NULL == pipe->fo) {
        PDPipeCloseFileStream(pipe->fi);
        return false;
    }
    
    pipe->opened = true;
    
    pipe->stream = PDTwinStreamCreate(pipe->fi, pipe->fo);
    pipe->parser = PDParserCreateWithStream(pipe->stream);
    
    if (pipe->parser) {
#ifdef PD_SUPPORT_CRYPTO
        if (pipe->parser->crypto) {
            if (pipe->parser->crypto->cfMethod == pd_crypto_method_aesv2) {
                // we don't support AES right now
                return false;
            }
        }
#endif
            
        pipe->filter = PDSplayTreeCreateWithDeallocator(PDReleaseFunc);
    }

    return pipe->stream && pipe->parser;
}

static inline PDBool PDPipeRunStackedTasks(PDPipeRef pipe, PDParserRef parser, pd_stack *stack)
{
    PDTaskRef task;
    PDTaskResult result;
    pd_stack iter;
    pd_stack prevStack = NULL;

    pd_stack_for_each(*stack, iter) {
        task = iter->info;
        result = PDTaskExec(task, pipe, PDParserConstructObject(parser));
        if (PDTaskFailure == result) return false;
        if (PDTaskUnload == result) {
            // note that task unloading only reaches PDPipe for stacked tasks; regular filtered task unloading is always caught by the PDTask implementation
            if (prevStack) {
                prevStack->prev = iter->prev;
                iter->prev = NULL;
                pd_stack_destroy(&iter);
                iter = prevStack;
            } else {
                // since we're dropping the top item, we've lost our iteration variable, so we recall ourselves to start over (which means continuing, as this was the first item)
                pd_stack_pop_identifier(stack);//&pipe->typeTasks[0]);
                PDRelease(task);
                
                return PDPipeRunStackedTasks(pipe, parser, stack);
                //PDPipeRunUnfilteredTasks(pipe, parser);
            }
        }
        prevStack = iter;
    }
    return true;
}

PDInteger PDPipeExecute(PDPipeRef pipe)
{
    // if pipe is closed, we need to prepare
    if (! pipe->opened && ! PDPipePrepare(pipe)) 
        return -1;
    
    PDStaticHashRef sht = NULL;
    PDParserRef parser = pipe->parser;
    PDTaskRef task;
    PDObjectRef obj;
    PDStringRef pt;
    int pti;
    
    // at this point, we set up a static hash table for O(1) filtering before the O(n) tree fetch; the SHT implementation here triggers false positives and cannot be used on its own
    pipe->dynamicFiltering = pipe->typedTasks;
    
    if (! pipe->dynamicFiltering) {
        PDInteger entries = pipe->filterCount;
        void **keys = malloc(entries * sizeof(void*));
        PDSplayTreePopulateKeys(pipe->filter, (PDInteger*)keys);
        //pd_btree_populate_keys(pipe->filter, keys);
    
        sht = PDStaticHashCreate(entries, keys, keys);
    }
    
    //long fpos = 0;
    //long tneg = 0;
    PDBool proceed = true;
    PDInteger seen = 0;
    do {
        PDFlush();
        
        seen++;

        // run unfiltered tasks
        if (! (proceed &= PDPipeRunStackedTasks(pipe, parser, &pipe->typeTasks[0]))) break;
        
        /// check filtered tasks; @todo: CLANG: does not recognize that dynamicFiltering is always true if sht is nil.
        if (pipe->dynamicFiltering || PDStaticHashValueForKey(sht, parser->obid)) {

            // by object id
            task = PDSplayTreeGet(pipe->filter, parser->obid);
            if (task) 
                //printf("* task: object #%lu @ offset %lld *\n", parser->obid, PDTwinStreamGetInputOffset(parser->stream));
                if (! (proceed &= PDTaskFailure != PDTaskExec(task, pipe, PDParserConstructObject(parser)))) break;
            
            // by type
            if (proceed && pipe->typedTasks) {
                // @todo this really needs to be streamlined; for starters, a PDState object could be used to set up types instead of O(n)'ing
                obj = PDParserConstructObject(parser);
                if (PDObjectTypeDictionary == PDObjectGetType(obj)) {
                    pt = PDDictionaryGetEntry(PDObjectGetDictionary(obj), "Type");
                    if (pt) {
                        //printf("pt = %s\n", pt);
                        for (pti = 1; pti < _PDFTypeCount; pti++) // not = 0, because 0 = NULL and is reserved for 'unfiltered'
                            if (PDStringEqualsCString(pt, PDFTypeStrings[pti]))
                                break;
                        
                        if (pti < _PDFTypeCount) 
                            proceed &= PDPipeRunStackedTasks(pipe, parser, &pipe->typeTasks[pti]);
                    }
                }
            }

        } else { 
            //tneg++;
            PDAssert(!PDSplayTreeGet(pipe->filter, parser->obid));
        }
    } while (proceed && PDParserIterate(parser));
    PDRelease(sht);
    PDFlush();
    
    proceed &= parser->success;
    
    //if (proceed && pipe->onEndOfObjectsTask) 
    //   proceed &= PDTaskFailure != PDTaskExec(pipe->onEndOfObjectsTask, pipe, NULL);
    
    if (proceed) 
        PDParserDone(parser);
    
    PDRelease(pipe->filter);
    PDRelease(parser);
    PDRelease(pipe->stream);
    
    pipe->filter = NULL;
    pipe->parser = NULL;
    pipe->stream = NULL;
    
    PDPipeCloseFileStream(pipe->fi);
    PDPipeCloseFileStream(pipe->fo);
    pipe->opened = false;
    
    return proceed ? seen : -1;
}

const char *PDPipeGetInputFilePath(PDPipeRef pipe)
{
    return pipe->pi;
}

const char *PDPipeGetOutputFilePath(PDPipeRef pipe)
{
    return pipe->po;
}

PDParserAttachmentRef PDPipeConnectForeignParser(PDPipeRef pipe, PDParserRef foreignParser)
{
    PDParserAttachmentRef attachment = PDSplayTreeGet(pipe->attachments, (PDInteger)foreignParser);
    if (NULL == attachment) {
        attachment = PDParserAttachmentCreate(PDPipeGetParser(pipe), foreignParser);
        PDSplayTreeInsert(pipe->attachments, (PDInteger)foreignParser, attachment);
    }
    return attachment;
}
