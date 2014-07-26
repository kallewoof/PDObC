//
// PDStaticHash.c
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
#include "PDDefines.h"
#include "PDScanner.h"
#include "PDOperator.h"
#include "pd_stack.h"
#include "PDStaticHash.h"
#include "pd_pdf_private.h"

void PDStaticHashDestroy(PDStaticHashRef sh)
{
    free(sh->table);
    if (! sh->leaveKeys)   free(sh->keys);
    if (! sh->leaveValues) free(sh->values);
}

PDStaticHashRef PDStaticHashCreate(PDInteger entries, void **keys, void **values)
{
    PDStaticHashRef sh = PDAlloc(sizeof(struct PDStaticHash), PDStaticHashDestroy, false);
#define converterTableHash(key) static_hash_idx(converterTableMask, converterTableShift, key)
    
    sh->entries = entries;
    sh->keys = keys;
    sh->values = values;
    sh->leaveKeys = 0;
    sh->leaveValues = keys == values;

    PDInteger i, hash;
    PDInteger converterTableMask = 0;
    PDInteger converterTableShift = 0;
    PDInteger converterTableSize = 4;
    
    // determine no-collision table size, if necessary (everything is constant throughout execution, so we only do this once, no matter what)
    PDInteger maxe = entries << 7;
    if (maxe < 128) maxe = 128;
    char *coll = calloc(1, maxe);
    char k = 0;
    char sizeBits = 2;
    
    do {
        i = 0;
        sizeBits++;
        converterTableSize <<= 1;
        
        /// @todo had 1 assert below; so this needs to be ripped out asap
        //PDAssert(converterTableSize < maxe); // crash = this entire idea is rotten and should be thrown out and replaced with a real dictionary handler
        converterTableMask = converterTableSize - 1;
        for (converterTableShift = sizeBits; i < entries && converterTableShift < 30; converterTableShift++) {
            k++;
            //printf("#%d: <<%d, & %d\n", k, converterTableShift, converterTableMask);
            for (i = 0; i < entries && k != coll[converterTableHash(keys[i])]; i++)  {
                //printf("\t%d = %ld", i, converterTableHash(keys[i]));
                coll[converterTableHash(keys[i])] = k;
            }
            //if (i < entries) printf("\t%d = %ld", i, converterTableHash(keys[i]));
            //printf("\n");
        }
        //printf("<<%d, ts %d\n", converterTableShift, converterTableSize);
    } while (i < entries);
    free(coll);
    
    converterTableShift--;
    
    void **converterTable = calloc(sizeof(void*), converterTableMask + 1);
    for (i = 0; i < entries; i++) {
        hash = converterTableHash(keys[i]);
        PDAssert(NULL == converterTable[hash]);
        converterTable[hash] = values[i];
    }
    
    sh->table = converterTable;
    sh->mask = converterTableMask;
    sh->shift = converterTableShift;
    
    return sh;
}

void PDStaticHashDisownKeysValues(PDStaticHashRef sh, PDBool disownKeys, PDBool disownValues)
{
    sh->leaveKeys |= disownKeys;
    sh->leaveValues |= disownValues;
}
