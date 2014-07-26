//
// PDStreamFilter.c
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

#include "pd_internal.h"
#include "PDStreamFilter.h"
#include "pd_stack.h"
#include "pd_pdf_implementation.h"

static pd_stack filterRegistry = NULL;

void PDStreamFilterRegisterDualFilter(const char *name, PDStreamDualFilterConstr constr)
{
    pd_stack_push_identifier(&filterRegistry, (PDID)constr);
    pd_stack_push_identifier(&filterRegistry, (PDID)name);
}

PDStreamFilterRef PDStreamFilterObtain(const char *name, PDBool inputEnd, PDDictionaryRef options)
{
    pd_stack iter = filterRegistry;
    while (iter && strcmp(iter->info, name)) iter = iter->prev->prev;
    if (iter) {
        PDStreamDualFilterConstr constr = iter->prev->info;
        return (*constr)(inputEnd, options);
    }
    
    return NULL;
}

void PDStreamFilterDestroy(PDStreamFilterRef filter)
{
    if (filter->initialized) 
        (*filter->done)(filter);
    
    PDRelease(filter->options);
//    pd_stack_destroy(&filter->options);
    
    if (filter->bufOutOwned)
        free(filter->bufOutOwned);
    
    PDRelease(filter->nextFilter);
}

PDStreamFilterRef PDStreamFilterAlloc(void)
{
    return PDAlloc(sizeof(struct PDStreamFilter), PDStreamFilterDestroy, false);
}

PDStreamFilterRef PDStreamFilterCreate(PDStreamFilterFunc init, PDStreamFilterFunc done, PDStreamFilterFunc begin, PDStreamFilterFunc proceed, PDStreamFilterPrcs createInversion, PDDictionaryRef options)
{
    PDStreamFilterRef filter = PDAlloc(sizeof(struct PDStreamFilter), PDStreamFilterDestroy, true);
    filter->growthHint = 1.f;
    filter->compatible = true;
    filter->options = PDRetain(options);
    filter->init = init;
    filter->done = done;
    filter->begin = begin;
    filter->proceed = proceed;
    filter->createInversion = createInversion;
    return filter;
}

void PDStreamFilterAppendFilter(PDStreamFilterRef filter, PDStreamFilterRef next)
{
    while (filter->nextFilter) filter = filter->nextFilter;
    filter->nextFilter = PDRetain(next);
}

PDBool PDStreamFilterApply(PDStreamFilterRef filter, unsigned char *src, unsigned char **dstPtr, PDInteger len, PDInteger *newlenPtr, PDInteger *allocatedlenPtr)
{
    if (filter == NULL) {
        PDWarn("NULL filter in call to PDStreamFilterApply(). Performing copy.");
        memcpy(*dstPtr, src, len);
        *newlenPtr = len;
        if (allocatedlenPtr) *allocatedlenPtr = len;
        return true;
    }
    
    if (! filter->initialized) {
        if (! PDStreamFilterInit(filter))
            return false;
    }
    
    unsigned char *resbuf;
    PDInteger dstCap = len * filter->growthHint;
    if (dstCap < 64) dstCap = 64;
    if (dstCap > 64 * 1024) dstCap = 64 * 1024;
    
    filter->bufIn = src;
    filter->bufInAvailable = len;
    resbuf = filter->bufOut = malloc(dstCap);
    filter->bufOutCapacity = dstCap;
    
    PDInteger bytes = PDStreamFilterBegin(filter);
    PDInteger got = 0;
    while (bytes > 0) {
        got += bytes;
        if (! filter->finished) {
            dstCap *= 3;
            resbuf = realloc(resbuf, dstCap);
        }
        filter->bufOut = &resbuf[got];
        filter->bufOutCapacity = dstCap - got;
        bytes = PDStreamFilterProceed(filter);
    }
    
    *dstPtr = resbuf;
    *newlenPtr = got;
    if (allocatedlenPtr) *allocatedlenPtr = dstCap;
    
    return ! filter->failing;
}

PDBool PDStreamFilterInit(PDStreamFilterRef filter)
{
    if (! (*filter->init)(filter)) 
        return false;

    if (filter->nextFilter) {
        if (! PDStreamFilterInit(filter->nextFilter))
            return false;
        filter->compatible &= filter->nextFilter->compatible;
    }
    return true;
}

PDBool PDStreamFilterDone(PDStreamFilterRef filter)
{
    while (filter) {
        if (! (*filter->done)(filter)) 
            return false;
        filter = filter->nextFilter;
    }
    return true;
}

PDInteger PDStreamFilterBegin(PDStreamFilterRef filter)
{
    // we set this up so that the first and last filters in the chain swap filters on entry/exit (for proceed); that will give the caller a consistent filter state which they can read from / update as they need
    PDStreamFilterRef curr, next;
    
    if (filter->nextFilter == NULL) 
        return (*filter->begin)(filter);
    
    // we pull out filter's output values
    unsigned char *bufOut = filter->bufOut;
    PDInteger bufOutCapacity = filter->bufOutCapacity;
    PDInteger cap;
    
    // set up new buffers for each of the filters in between
    //prev = NULL;
    for (curr = filter; curr->nextFilter; curr = next) {
        next = curr->nextFilter;

        cap = curr->bufInAvailable * curr->growthHint;
        if (cap < bufOutCapacity) cap = bufOutCapacity;
        if (cap > bufOutCapacity * 10) cap = bufOutCapacity * 10;
        curr->bufOutCapacity = curr->bufOutOwnedCapacity = cap;
        next->bufIn = curr->bufOutOwned = curr->bufOut = malloc(curr->bufOutCapacity);
        next->bufInAvailable = 0; //(*curr->begin)(curr);
        next->needsInput = true;
        next->hasInput = true;
    }
    
    filter->finished = false;
    filter->needsInput = true;
    filter->hasInput = false;
    filter->bufOut = bufOut;
    filter->bufOutCapacity = bufOutCapacity;
    
    // we can now call proceed as normal
    return PDStreamFilterProceed(filter);
}

PDInteger PDStreamFilterProceed(PDStreamFilterRef filter)
{
    PDStreamFilterRef curr, prev, next;
    
    // don't waste time
    if (filter->finished) return 0;
    
    // or energy
    if (filter->nextFilter == NULL) return (*filter->proceed)(filter);
    
    // we pull out filter's output values
    PDInteger result = 0;
    unsigned char *bufOut = filter->bufOut;
    PDInteger bufOutCapacity = filter->bufOutCapacity;
    
    // iterate over each filter
    prev = NULL;
    for (curr = filter; curr; curr = next) {
        next = curr->nextFilter;
        
        if (next) {
            
            if (! next->needsInput || next->bufInAvailable >= curr->bufOutOwnedCapacity / 2) {
                // the next filter is still processing data so we have to pass
                prev = curr;
                continue;
            }
            
            // filters sometimes need more data to process (e.g. predictor), so we can't blindly throw out and replace the buffers each time; the next->bufInAvailable declares how many bytes are still unprocessed
            if (next->bufInAvailable > 0) {
                // we can always memcpy() as no overlap will occur (or we would've skipped this filter)
                //assert(curr->bufOutOwned + curr->bufOutOwnedCapacity - next->bufInAvailable == next->bufIn);
                memcpy(curr->bufOutOwned, next->bufIn, next->bufInAvailable);
                next->bufIn = curr->bufOutOwned;
                curr->bufOut = curr->bufOutOwned + next->bufInAvailable;
                curr->bufOutCapacity = curr->bufOutOwnedCapacity - next->bufInAvailable;
            } else {
                // next filter ate all its breakfast, so we do this the easy way
                curr->bufOut = next->bufIn = curr->bufOutOwned;
                curr->bufOutCapacity = curr->bufOutOwnedCapacity;
            }
            
            next->bufInAvailable += (curr->needsInput ? (*curr->begin)(curr) : (*curr->proceed)(curr));
            next->hasInput = curr->bufInAvailable > 0;
            
        } else {
            
            // last filter, which means we plug it into the "real" output buffer
            curr->bufOut = bufOut;
            curr->bufOutCapacity = bufOutCapacity;
            result += curr->needsInput ? (*curr->begin)(curr) : (*curr->proceed)(curr);
            bufOut = curr->bufOut;
            bufOutCapacity = curr->bufOutCapacity;
        
        }

        if (curr->failing) {
            filter->failing = true;
            return 0;
        }
        
        // we may be in a situation where we consumed a lot of content and produced little; to prevent bottlenecks we will return to previous filters and reapply in these cases
        if (prev && prev->bufInAvailable > 0 && curr->bufInAvailable < 64 && curr->bufOutCapacity > 64) {
            // we have a prev, its input buffer has stuff, our input buffer has very little or no stuff, and our output buffer is capable of taking more stuff
            // this is a good sign that we may need to grow an inbetween buffer to not bounce around too much
            if (prev->bufOutOwnedCapacity < result + bufOutCapacity) {
                PDInteger offs = curr->bufIn - prev->bufOutOwned;
                prev->bufOutOwned = realloc(prev->bufOutOwned, result + bufOutCapacity);
                curr->bufIn = prev->bufOutOwned + offs;
                prev->bufOutOwnedCapacity = result + bufOutCapacity;
            }
            next = prev;
            curr = NULL;
        } 
        
        prev = curr;
    }
    
    // now restore filter's output buffers from prev (which is the last non-null curr)
    filter->bufOut = prev->bufOut;
    filter->bufOutCapacity = prev->bufOutCapacity;
    
    // we actually go through once more to set 'finished' as it may flip back and forth as the filters rewind
    PDBool finished = true;
    for (curr = filter; finished && curr; curr = curr->nextFilter)
        finished &= curr->finished;
    
    filter->finished = finished;

    return result;
}

PDStreamFilterRef PDStreamFilterCreateInversionForFilter(PDStreamFilterRef filter)
{
    PDStreamFilterRef inversionParent = NULL;
    
    if (! filter->initialized) (*filter->init)(filter);
    
    if (filter->createInversion == NULL) return NULL;
    
    if (filter->nextFilter) {
        if (filter->nextFilter->createInversion) 
            inversionParent = (*filter->nextFilter->createInversion)(filter);
        if (inversionParent == NULL) 
            return NULL;
    }
    
    PDStreamFilterRef inversion = (*filter->createInversion)(filter);
    if (! inversionParent) {
        return inversion;
    }
    
    // we went from  [1] ->  [2] ->  [3] to 
    //                      [2] ->  [3] to
    //                             ~[3] to
    //                     ~[3] -> ~[2]
    // and we keep getting the "super parent" back each time, so we have to step down
    // to the last child and put us below that, in order to get
    //             ~[3] -> ~[2] -> ~[1]
    // which is the only proper inversion (~ is not the proper sign for inversion, but ^-1 looks ugly)
    
    PDStreamFilterRef result = inversionParent;
    while (inversionParent->nextFilter) inversionParent = inversionParent->nextFilter;
    inversionParent->nextFilter = inversion;
    
    return result;
}

//pd_stack PDStreamFilterGenerateOptionsFromDictionaryStack(pd_stack dictStack)
//{
//    if (dictStack == NULL) return NULL;
//    
//    pd_stack stack = NULL;
//    pd_stack iter = dictStack->prev->prev->info;
//    pd_stack entry;
//    pd_stack_set_global_preserve_flag(true);
//    while (iter) {
//        entry = iter->info;
//        char *value = (entry->prev->prev->type == PD_STACK_STACK 
//                       ? PDStringFromComplex(entry->prev->prev->info)
//                       : strdup(entry->prev->prev->info));
//        pd_stack_push_key(&stack, value);
//        pd_stack_push_key(&stack, strdup(entry->prev->info));
//        iter = iter->prev;
//    }
//    pd_stack_set_global_preserve_flag(false);
//    return stack;
//}
/*
     stack<0x172adcd0> {
         0x46d0ac ("dict")
         0x46d0a8 ("entries")
         stack<0x172ad090> {
             stack<0x172ad080> {
                 0x46d0b0 ("de")
                 Columns
                 6
             }
             stack<0x172adca0> {
                 0x46d0b0 ("de")
                 Predictor
                 12
             }
         }
     }
*/
