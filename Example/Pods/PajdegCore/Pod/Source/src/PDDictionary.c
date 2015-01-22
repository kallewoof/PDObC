//
// PDDictionary.c
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

#include "PDDictionary.h"
#include "pd_internal.h"
#include "pd_pdf_implementation.h"
#include "pd_stack.h"
#include "PDArray.h"
#include "PDString.h"

#ifdef PDHM_PROF
#define prof_ctr_mask     1023 // the mask used to cycle
static int prof_counter = 0;   // cycles 0..1023 and prints profile info at every 0

static unsigned long long 
    operations = 0,             // # of operations in total
    creations = 0,              // # of hash map creations
    capCountSum = 0,            // the highest count seen on deletion
    destroys = 0,               // # of hash map destructions
    node_creations = 0,         // # of nodes created
    node_destroys = 0,          // # of nodes deleted
    totbucks = 0,               // total number of buckets created
    totemptybucks = 0,          // total number of empty buckets (even after destruction)
    totpopbucks = 0,            // total number of populated buckets (1 or more)
    totcollbucks = 0,           // total number of buckets with > 1 node
    totfinds = 0,               // total number of bucket find ops
    totsets = 0,                // total number of set operations
    totgets = 0,                // total number of get operations
    totdels = 0,                // total number of delete operations
    totreplaces = 0,            // total number of replacements (i.e. sets for pre-existing keys)
    totcolls = 0,               // total number of collisions (insertions into populated buckets)
    totsetcolls = 0,            // total number of collisions on set ops
    totgetcolls = 0,            // total number of collisions on get ops
    topbucksize = 0,            // biggest observed bucket size (in nodes)
    cstring_hashgens = 0,       // number of c string hash generations
    cstring_hashcomps = 0;      // number of c string hash comparisons

#define BS_TRACK_CAP 8
static unsigned long long buckets_sized[BS_TRACK_CAP] = {0};
#define reg_buck_insert(bucket) do { \
            PDSize bsize = PDArrayGetCount(bucket);\
            PDSize bscapped = bsize > BS_TRACK_CAP-1 ? BS_TRACK_CAP-1 : bsize;\
            if (bsize > 0) {\
                totcolls++; \
                totcollbucks += bsize == 1;\
                if (bsize + 1 > topbucksize) topbucksize = bsize + 1; \
                buckets_sized[bscapped]--; \
            }\
            buckets_sized[bscapped+1]++; \
        } while(0)

#define reg_buck_delete(bucket) do {\
            PDSize bsize = PDArrayGetCount(bucket);\
            if (bsize > BS_TRACK_CAP-1) bsize = BS_TRACK_CAP-1;\
            PDAssert(bsize > 0); \
            buckets_sized[bsize]--;\
            buckets_sized[bsize-1]++;\
        } while(0)

static inline void prof_report()
{
    printf("                     HASH MAP PROFILING\n"
           "=================================================================================================\n"
           "total operation count : %10llu\n"
           "average count / dict  : %10lf\n"
           "creations  : %10llu   destroys   : %10llu\n"
           "nodes      : %10llu              : %10llu\n"
           "bucket sum : %10llu   top sized  : %10llu\n"
           "   empty   : %10llu   populated  : %10llu  w/ collisions : %10llu\n"
           "set ops    : %10llu   get ops    : %10llu     deletions  : %10llu\n"
           "find ops   : %10llu   replace ops: %10llu\n"
           "C string g : %10llu   C string c : %10llu\n"
           "collisions : %10llu\n"
           "   on set  : %10llu   on get     : %10llu\n"
           "collision ratio : %f\n"
           "   set collisions : %f   get/del collisions : %f\n"
           , operations
           , (double)capCountSum / (double)destroys
           , creations, destroys
           , node_creations, node_destroys
           , totbucks, topbucksize
           , totemptybucks, totpopbucks, totcollbucks
           , totsets, totgets, totdels
           , totfinds, totreplaces
           , cstring_hashgens, cstring_hashcomps
           , totcolls
           , totsetcolls, totgetcolls
           , (float)totcolls / (float)(totsets + totgets + totdels)
           , (float)totsetcolls / (float)totsets, (float)totgetcolls / (float)(totgets+totdels));
    printf("bucket sizes:\n"
           "        0        1        2        3        4        5        6        7+\n");
    for (PDSize i = 0; i < BS_TRACK_CAP; i++) {
        printf(" %8llu", buckets_sized[i]);
    }
    printf("\n"
           "=================================================================================================\n");
}

#   define  prof(args...) do {\
                args;\
                if (!(prof_counter = (prof_counter + 1) & prof_ctr_mask))\
                prof_report();\
            } while(0)
#else
#   define reg_buck_insert(bucket) 
#   define reg_buck_delete(bucket) 
#   define prof(args...) 
#   define prof_report() 
#endif

void PDDictionaryDestroy(PDDictionaryRef hm)
{
    prof(capCountSum += hm->maxCount);
    prof(destroys++);
    
#ifdef PD_SUPPORT_CRYPTO
    PDRelease(hm->ci);
#endif    
    free(hm->buckets);
    PDRelease(hm->populated);
}

static inline PDSize PDHashGeneratorCString(const char *key) 
{
    prof(cstring_hashgens++);
    // from http://c.learncodethehardway.org/book/ex37.html
    size_t len = strlen(key);
    PDSize hash = 0;
    PDSize i = 0;
    
    for(hash = i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    
    return hash;
}

#ifdef PDHM_PROF
static inline int PDHashComparatorCString(const char *key1, const char *key2)
{
    prof(cstring_hashcomps++);
    return strcmp(key1, key2);
}
#else
#   define PDHashComparatorCString  strcmp
#endif

#define PD_HASHMAP_DEFAULT_BUCKETS  64

PDDictionaryRef _PDDictionaryCreateWithSettings(PDSize bucketc)
{
    prof(creations++; totbucks += bucketc; totemptybucks += bucketc);
    PDDictionaryRef hm = PDAllocTyped(PDInstanceTypeDict, sizeof(struct PDDictionary), PDDictionaryDestroy, false);
    hm->count = 0;
    prof(hm->maxCount = 0);
    hm->bucketc = bucketc;   // 128 (0b10000000)
    hm->bucketm = bucketc-1; // 127 (0b01111111)
    hm->buckets = calloc(sizeof(PDArrayRef), hm->bucketc);
    hm->populated = PDArrayCreateWithCapacity(bucketc);
    hm->ci = NULL;
    return hm;
}

PDDictionaryRef PDDictionaryCreateWithBucketCount(PDSize bucketCount)
{
    if (bucketCount < 16) bucketCount = 16;
    PDAssert(bucketCount < 1 << 24); // crash = absurd bucket count (over 16777216)
    PDSize bits = ceilf(log2f(bucketCount)); // ceil(log2(100)) == ceil(6.6438) == 7
    PDSize bucketc = 1 << bits; // 1 << 7 == 128
    return _PDDictionaryCreateWithSettings(bucketc);
}

PDDictionaryRef PDDictionaryCreate()
{
    return _PDDictionaryCreateWithSettings(PD_HASHMAP_DEFAULT_BUCKETS);
}

PDDictionaryRef PDDictionaryCreateWithComplex(pd_stack stack)
{
    PDDictionaryRef hm = _PDDictionaryCreateWithSettings(PD_HASHMAP_DEFAULT_BUCKETS);
    PDDictionaryAddEntriesFromComplex(hm, stack);
    return hm;
}

PDDictionaryRef PDDictionaryCreateWithKeyValueDefinition(const void **defs)
{
    PDDictionaryRef hm = _PDDictionaryCreateWithSettings(PD_HASHMAP_DEFAULT_BUCKETS);

    PDInteger i = 0;
    char *key;
    void *val;
    while (defs[i]) {
        key = (char *)defs[i++];
        val = (void *)defs[i++];
        PDDictionarySet(hm, key, val);
    }
    return hm;
}

void PDDictionaryAddEntriesFromComplex(PDDictionaryRef hm, pd_stack stack)
{
    if (stack == NULL) {
        // empty dictionary
        return;
    }
    
    // this may be an entry that is a dictionary inside another dictionary; if so, we should see something like

    if (PDIdentifies(stack->info, PD_DE)) {
        stack = stack->prev->prev->info;
    }
    
    // we may have the DICT identifier
    if (PDIdentifies(stack->info, PD_DICT)) {
        if (stack->prev->prev == NULL) 
            stack = NULL;
        else 
            stack = stack->prev->prev->info;
    }
    
    pd_stack s = stack;
    pd_stack entry;
    
    pd_stack_set_global_preserve_flag(true);
    while (s) {
        entry = as(pd_stack, s->info)->prev;
        
        // entry must be a string; it's the key value
        char *key = entry->info;
        entry = entry->prev;
        if (entry->type == PD_STACK_STRING) {
            PDDictionarySet(hm, key, PDStringCreate(strdup(entry->info)));
        } else {
            entry = entry->info;
            pd_stack_set_global_preserve_flag(true);
            void *v = PDInstanceCreateFromComplex(&entry);
            pd_stack_set_global_preserve_flag(false);
#ifdef PD_SUPPORT_CRYPTO
            if (v && hm->ci) (*PDInstanceCryptoExchanges[PDResolve(v)])(v, hm->ci, true);
#endif
            PDDictionarySet(hm, key, v);
            PDRelease(v);
        }
        s = s->prev;
    }
    pd_stack_set_global_preserve_flag(false);
}

static void PDDictionaryNodeDestroy(PDDictionaryNodeRef n)
{
    PDRelease(n->data);
    free(n->key);
    prof(node_destroys++);
}

static inline PDDictionaryNodeRef PDDictionaryNodeCreate(PDSize hash, char *key, void *data)
{
    prof(node_creations++);
    PDDictionaryNodeRef n = PDAlloc(sizeof(struct PDDictionaryNode), PDDictionaryNodeDestroy, false);
    n->hash = hash;
    n->key = key; // owned! freed on node destruction!
    n->data = PDRetain(data);
    return n;
}

static inline PDArrayRef PDDictionaryFindBucket(PDDictionaryRef hm, const char *key, PDSize hash, PDBool create, PDInteger *outIndex)
{
    prof(totfinds++);
    PDSize bucketIndex = hash & hm->bucketm;
    
    PDArrayRef bucket = hm->buckets[bucketIndex];
    *outIndex = -1;
    
    if (NULL == bucket) {
        if (! create) return NULL;
        prof(totemptybucks--; totpopbucks++); // when a bucket is asked to be created, it means it will be non-empty and populated, so we prof that here
        bucket = PDArrayCreateWithCapacity(4);
        hm->buckets[bucketIndex] = bucket;
        PDArrayAppend(hm->populated, bucket);
        PDRelease(bucket);
    } else {
        PDDictionaryNodeRef node;
        PDInteger len = PDArrayGetCount(bucket);
        for (PDInteger i = 0; i < len; i++) {
            node = PDArrayGetElement(bucket, i);
            if (node->hash == hash && !PDHashComparatorCString(key, node->key)) {
                *outIndex = i;
                break;
            }
            prof(if (create) totsetcolls++; else totgetcolls++);
        }
    }
    
    return bucket;
}

void PDDictionarySet(PDDictionaryRef hm, const char *key, void *value)
{
    PDAssert(key != NULL);  // crash = key is NULL; this is not allowed
    PDAssert(value != NULL); // crash = value is NULL; use delete to remove keys
    
#ifdef PD_SUPPORT_CRYPTO
    if (hm->ci) (*PDInstanceCryptoExchanges[PDResolve(value)])(value, hm->ci, false);
#endif
    
    prof(operations++);
    prof(totsets++);
    PDSize hash = PDHashGeneratorCString(key);
    PDInteger nodeIndex;
    PDArrayRef bucket = PDDictionaryFindBucket(hm, key, hash, true, &nodeIndex);
    PDAssert(bucket != NULL);
    
    if (nodeIndex != -1) {
        prof(totreplaces++);
        PDDictionaryNodeRef node = PDArrayGetElement(bucket, nodeIndex);
        PDRetain(value);
        PDRelease(node->data);
        node->data = value;
    } else {
        reg_buck_insert(bucket);
        hm->count++;
        prof(if (hm->count > hm->maxCount) hm->maxCount = hm->count);
        PDDictionaryNodeRef node = PDDictionaryNodeCreate(hash, strdup(key), value);
        PDArrayAppend(bucket, node);
        PDRelease(node);
    }
}

void *PDDictionaryGet(PDDictionaryRef hm, const char *key)
{
    PDAssert(hm);
    prof(operations++);
    prof(totgets++);
    PDSize hash = PDHashGeneratorCString(key);
    PDInteger nodeIndex;
    PDArrayRef bucket = PDDictionaryFindBucket(hm, key, hash, false, &nodeIndex);
    return nodeIndex > -1 ? ((PDDictionaryNodeRef)PDArrayGetElement(bucket, nodeIndex))->data : NULL;
}

void *PDDictionaryGetTyped(PDDictionaryRef dictionary, const char *key, PDInstanceType type)
{
    void *v = PDDictionaryGet(dictionary, key);
    return v && PDResolve(v) == type ? v : NULL;
}

void PDDictionaryDelete(PDDictionaryRef hm, const char *key)
{
    prof(operations++);
    prof(totdels++);
    PDSize hash = PDHashGeneratorCString(key);
    PDInteger nodeIndex;
    PDArrayRef bucket = PDDictionaryFindBucket(hm, key, hash, false, &nodeIndex);
    if (! bucket || nodeIndex == -1) return;
    reg_buck_delete(bucket);
    hm->count--;
    PDArrayDeleteAtIndex(bucket, nodeIndex);
}

void PDDictionaryClear(PDDictionaryRef hm)
{
    prof(operations++);
    PDInteger blen = PDArrayGetCount(hm->populated);
    for (PDInteger i = 0; i < blen; i++) {
        PDArrayRef bucket = PDArrayGetElement(hm->populated, i);
        PDArrayClear(bucket);
    }
    hm->count = 0;
}

PDSize PDDictionaryGetCount(PDDictionaryRef hm)
{
    return hm->count;
}

void PDDictionaryIterate(PDDictionaryRef hm, PDHashIterator it, void *ui)
{
    PDBool shouldStop = false;
    PDInteger blen = PDArrayGetCount(hm->populated);
    for (PDInteger i = 0; i < blen; i++) {
        PDArrayRef bucket = PDArrayGetElement(hm->populated, i);
        PDInteger len = PDArrayGetCount(bucket);
        for (PDInteger j = 0; j < len; j++) {
            PDDictionaryNodeRef node = PDArrayGetElement(bucket, j);
            it(node->key, node->data, ui, &shouldStop);
            if (shouldStop) return;
        }
    }
}

typedef struct hm_keygetter {
    int i;
    char **res;
} hm_keygetter;

typedef struct hm_printer {
    char **bv, **buf;
    PDInteger *cap;
    PDInteger *offs;
} hm_printer;

void pd_hm_getkeys(char *key, void *val, hm_keygetter *userInfo, PDBool *shouldStop)
{
    userInfo->res[userInfo->i++] = key;
}

void pd_hm_print(char *key, void *val, hm_printer *p, PDBool *shouldStop)
{
    char *bv;
    PDInteger *cap = p->cap;
    
    char **buf = p->buf;
    PDInteger offs = *p->offs;
    
    PDInteger klen = strlen(key);
    PDInstancePrinterRequire(klen + 10, klen + 10);
    bv = *buf;
    bv[offs++] = '/';
    strcpy(&bv[offs], key);
    offs += klen;
    bv[offs++] = ' ';
    offs = (*PDInstancePrinters[PDResolve(val)])(val, buf, offs, cap);
    PDInstancePrinterRequire(4, 4);
    bv = *buf;
    bv[offs++] = ' ';
    
    *p->bv = bv;
    *p->offs = offs;
    
    //    printf("\t%s: %s\n", key, val);
}

void PDDictionaryPopulateKeys(PDDictionaryRef hm, char **keys)
{
    hm_keygetter kg;
    kg.i = 0;
    kg.res = keys;
    PDDictionaryIterate(hm, (PDHashIterator)pd_hm_getkeys, &kg);
}

char *PDDictionaryToString(PDDictionaryRef hm)
{
    PDInteger len = 6 + 20 * hm->count;
    char *str = malloc(len);
    PDDictionaryPrinter(hm, &str, 0, &len);
    return str;
}

void PDDictionaryPrint(PDDictionaryRef hm)
{
    char *str = PDDictionaryToString(hm);
    puts(str);
    free(str);
}

PDInteger PDDictionaryPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap)
{
    hm_printer p;
    PDInstancePrinterInit(PDDictionaryRef, 0, 1);
    PDInteger len = 6 + 20 * i->count;
    PDInstancePrinterRequire(len, len);
    char *bv = *buf;
    bv[offs++] = '<';
    bv[offs++] = '<';
    bv[offs++] = ' ';
    
    p.buf = buf;
    p.cap = cap;
    
    p.bv = &bv;
    p.offs = &offs;
    
    PDDictionaryIterate(i, (PDHashIterator)pd_hm_print, &p);
    
    bv[offs++] = '>';
    bv[offs++] = '>';
    bv[offs] = 0;
    return offs;
}

#ifdef PD_SUPPORT_CRYPTO

void pd_hm_encrypt(void *key, void *val, PDCryptoInstanceRef ci, PDBool *shouldStop)
{
    (*PDInstanceCryptoExchanges[PDResolve(val)])(val, ci, false);
}

void PDDictionaryAttachCrypto(PDDictionaryRef hm, pd_crypto crypto, PDInteger objectID, PDInteger genNumber)
{
    hm->ci = PDCryptoInstanceCreate(crypto, objectID, genNumber);
    PDDictionaryIterate(hm, (PDHashIterator)pd_hm_encrypt, hm->ci);
}

void PDDictionaryAttachCryptoInstance(PDDictionaryRef hm, PDCryptoInstanceRef ci, PDBool encrypted)
{
    hm->ci = PDRetain(ci);
    PDDictionaryIterate(hm, (PDHashIterator)pd_hm_encrypt, hm->ci);
}

#endif
