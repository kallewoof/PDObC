//
// PDType.c
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

#include "PDDefines.h"
#include "pd_internal.h"
#include "PDType.h"
#include "pd_stack.h"
#include "PDSplayTree.h"
#include "pd_pdf_implementation.h"

static pd_stack arp = NULL;

// if you are having issues with a non-PDTypeRef being mistaken for a PDTypeRef, you can enable DEBUG_PDTYPES_BREAK to stop the assertion from happening and instead returning a NULL value (for the value-returning functions)
//#define DEBUG_PDTYPES_BREAK

#ifdef DEBUG_PD_RELEASES

#include "PDDictionary.h"
#include "PDNumber.h"

static PDDictionaryRef seqDict = NULL;

typedef struct rrc *rrc;
struct rrc {
    const char *op;
    char *file;
    int lineNo;
    int seq;
    int rc;
    rrc next;
};

void destroy_rrc(void *vv)
{
    rrc v = vv;
    free(v->file);
    free(v);
}

PDSplayTreeRef _retrels = NULL;
void *_PDFocusObject = NULL;

#define PDFocusCheck(ob) if (_PDFocusObject == ob) _PDBreak()

static inline char *_PDDebugFilenameTrim(const char *file) 
{
    static int prefixLength = 0;
    if (prefixLength == 0) {
        int len = strlen(file);
        int i;
        for (i = len-1; i > 0 && file[i] != '/'; i--);
        i += (i > 0 && file[i] == '/');
        prefixLength = i;
    }
    return strdup(&file[prefixLength]);
}

static inline int _PDDebugLogNextSeq(const char *op, const char *file, int lineNo, void *ob)
{
    static char *v = NULL;
    if (! v) v = malloc(1024);
    sprintf(v, "%s:%s/%d", op, file, lineNo);
    
    PDNumberRef n = PDDictionaryGet(seqDict, v);
    if (n) {
        n = PDNumberWithInteger(PDNumberGetInteger(n) + 1);
    } else {
        n = PDNumberWithInteger(1);
    }
    
    if (PDNumberGetInteger(n) == 7 && !strcmp(v,
                                              //"alloc:PDParser.c/84")) {
                                              "alloc:PDDictionary.c/178")) {
        _PDFocusObject = ob;
    }
        

    PDDictionarySet(seqDict, v, n);
    return PDNumberGetInteger(n);
}

void _PDDebugLogDisplay(void *ob);

static PDBool loggingRetrelCall = false;
void _PDDebugLogRetrelCall(const char *op, const char *file, int lineNo, void *ob, int resultingRC)
{
    // since alloc is logged, we need to prevent logging of calls made here
    if (loggingRetrelCall) return;
    loggingRetrelCall = true;
    
    if (_retrels == NULL) {
        _retrels = PDSplayTreeCreate();
        seqDict = PDDictionaryCreate();
    }
    rrc entry = PDSplayTreeGet(_retrels, (PDInteger)ob);
    rrc prev = malloc(sizeof(struct rrc));
    prev->next = entry;
    prev->op = op;
    prev->rc = resultingRC;
    prev->file = _PDDebugFilenameTrim(file);
    prev->lineNo = lineNo;
    prev->seq = _PDDebugLogNextSeq(op, prev->file, lineNo, ob);
    PDSplayTreeInsert(_retrels, (PDInteger)ob, prev);
    
//    pd_stack_push_identifier(&entry, (PDID)(PDInteger)lineNo);
//    pd_stack_push_key(&entry, strdup(file));
//    pd_stack_push_identifier(&entry, (PDID)op);
//    PDSplayTreeInsert(_retrels, (PDInteger)ob, entry);
    
    loggingRetrelCall = false;
}

void _PDDebugLogDisplay(void *ob)
{
    if (_retrels == NULL) return;
    rrc entry = PDSplayTreeGet(_retrels, (PDInteger)ob);
    if (entry) {
        printf("Retain/release log for %p:\n", ob);
        printf("seq:   rc:  op:         line:  file:\n");
        while (entry) {
//            char *op = (char *)entry->info ; entry = entry->prev;
//            char *file = (char *)entry->info ; entry = entry->prev;
//            int lineNo = (int)entry->info ; entry = entry->prev;
            printf("%6d %4d %11s %6d %s\n", entry->seq, entry->rc, entry->op, entry->lineNo, entry->file);
            entry = entry->next;
        }
    } else {
        printf("(no log for %p!)\n", ob);
    }
}

#ifdef DEBUG_PD_LEAKS

PDSplayTreeRef _living = NULL;

static inline char *PDInstanceTypeToString(PDInstanceType it) 
{
    static char **strings = NULL;
    if (strings == NULL) {
        strings = malloc(sizeof(char*) * (1 + PDInstanceType__SIZE));
        strings[PDInstanceTypeNull] = strdup("NULL");
        strings[PDInstanceTypeNumber] = strdup("PDNumber");
        strings[PDInstanceTypeString] = strdup("PDString");
        strings[PDInstanceTypeArray] = strdup("PDArray");
        strings[PDInstanceTypeDict] = strdup("PDDictionary");
        strings[PDInstanceTypeRef] = strdup("PDReference");
        strings[PDInstanceTypeObj] = strdup("PDObject");
        strings[PDInstanceTypeParser] = strdup("PDParser");
        strings[PDInstanceTypePipe] = strdup("PDPipe");
        strings[PDInstanceTypeScanner] = strdup("PDScanner");
        strings[PDInstanceTypeCStream] = strdup("PDContentStream");
        strings[PDInstanceTypeOStream] = strdup("PDObjectStream");
        strings[PDInstanceTypeOperator] = strdup("PDOperator");
        strings[PDInstanceTypePage] = strdup("PDPage");
        strings[PDInstanceTypeParserAtt] = strdup("PDParserAttachment");
        strings[PDInstanceTypeTree] = strdup("PDSplayTree");
        strings[PDInstanceTypeState] = strdup("PDState");
        strings[PDInstanceTypeSFilter] = strdup("PDStreamFilter");
        strings[PDInstanceTypeTask] = strdup("PDTask");
        strings[PDInstanceType2Stream] = strdup("PDTwinStream");
        strings[PDInstanceTypeXTable] = strdup("PDXTable");
        strings[PDInstanceTypeCSOper] = strdup("PDContentStreamOperation");
        /*
         PDInstanceTypeParser    = 7,
         PDInstanceTypePipe      = 8,
         PDInstanceTypeScanner   = 9,
         PDInstanceTypeCStream   = 10,   ///< PDContentStream
         PDInstanceTypeOStream   = 11,   ///< PDObjectStream
         PDInstanceTypeOperator  = 12,
         PDInstanceTypePage      = 13,
         PDInstanceTypeParserAtt = 14,
         PDInstanceTypeTree      = 15,   ///< PDSplayTree
         PDInstanceTypeState     = 16,   ///< PDState
         PDInstanceTypeSFilter   = 17,   ///< PDStreamFilter
         PDInstanceTypeTask      = 18,   ///< PDTask
         PDInstanceType2Stream   = 19,
         PDInstanceTypeXTable    = 20,   ///< PDXTable
         PDInstanceTypeCSOper    = 21,   /// Content stream operator
         */
        strings[PDInstanceType__SIZE] = strdup("???");
    }
    return strings[it < 0 || it > PDInstanceType__SIZE ? PDInstanceType__SIZE : it];
}

static inline void _PDDebugDeallocating(void *ob) 
{
    if (_living == NULL || loggingRetrelCall) return;
//    printf("[deallocating] %p\n", ob);
    PDSplayTreeDelete(_living, (PDInteger)ob);
}

static inline void _PDDebugAllocating(void *ob)
{
    if (_living == NULL || loggingRetrelCall) return;
//    printf("[allocating]   %p\n", ob);

    PDSplayTreeInsert(_living, (PDInteger)ob, ob);
}

static inline PDInteger _PDDebugLivingReport()
{
    if (_living == NULL) return 0;
    PDInteger count = PDSplayTreeGetCount(_living);
    PDInteger *keys = malloc(sizeof(PDInteger) * count);
    PDSplayTreePopulateKeys(_living, keys);
    printf("LEAK REPORT (%ld objects):\n"
           "===============================================\n", count);
    for (PDInteger i = 0; i < count; i++) {
        if ((void*)keys[i] == _PDFocusObject) {
            printf("*** FOCUS OBJECT %p BELOW ***\n", _PDFocusObject);
        }
        printf("%s (%p): [%ld / %ld]\n", PDInstanceTypeToString(PDResolve((void*)keys[i])), (void*)keys[i], i+1, count);
        _PDDebugLogDisplay((void*)keys[i]);
        if (PDResolve((void*)keys[i]) == PDInstanceTypeDict) {
            printf("");
        }
    }
    if (count > 0) {
        printf("");
    }
    return count;
}

void PDDebugBeginSession()
{
    if (_living) {
        PDError("Stacked PDDebugBeginSession() calls; discarding existing splay tree!");
        PDRelease(_living);
    }
    _living = PDSplayTreeCreate();
}

PDInteger PDDebugEndSession()
{
    PDInteger value = 0;
    if (_living) {
        if (arp != NULL) printf("NOTE: auto release pool is non-empty");
        value = _PDDebugLivingReport();
        if (arp != NULL) {
            printf("Flushing\n");
            PDFlush();
            printf("Report after flush:\n\n\n");
            _PDDebugLivingReport();
        }
        PDRelease(_living);
        _living = NULL;
    }
    return value;
}

void PDFlagGlobalObject(void *ob)
{
    _PDDebugDeallocating(ob);
}

#endif

#else
#define _PDDebugLogRetrelCall(...) 
#define _PDDebugLogDisplay(ob) 
#endif

#ifndef DEBUG_PD_LEAKS
#   define _PDDebugAllocating(ob) 
#   define _PDDebugDeallocating(ob) 
#   define _PDDebugLivingReport() 
#   define PDFocusCheck(ob) 
#endif

#ifdef DEBUG_PDTYPES
char *PDC = "PAJDEG";

#ifdef DEBUG_PDTYPES_BREAK

void breakHere()
{
    printf("");
}

#define PDTypeCheckFailed(err_ret) breakHere(); return err_ret

#else

#define PDTypeCheckFailed(err_ret) PDAssert(0)

#endif

#define PDTypeCheck(cmd, err_ret) \
    if (type->pdc != PDC) { \
        fprintf(stderr, "*** error : object being " cmd " is not a valid PDType instance : %p ***\n", pajdegObject); \
        _PDDebugLogDisplay(pajdegObject); \
        PDTypeCheckFailed(err_ret); \
    }

#else
#define PDTypeCheck(cmd) 
#endif

#ifdef DEBUG_PD_RELEASES
void *_PDAllocTypedDebug(const char *file, int lineNumber, PDInstanceType it, PDSize size, void *dealloc, PDBool zeroed)
#else
void *PDAllocTyped(PDInstanceType it, PDSize size, void *dealloc, PDBool zeroed)
#endif
{
    PDTypeRef chunk = (zeroed ? calloc(1, sizeof(union PDType) + size) : malloc(sizeof(union PDType) + size));
#ifdef DEBUG_PDTYPES
    chunk->pdc = PDC;
#endif
    chunk->it = it;
    chunk->retainCount = 1;
    chunk->dealloc = dealloc;
    _PDDebugAllocating(chunk + 1);
    _PDDebugLogRetrelCall("alloc", file, lineNumber, chunk + 1, 1);
    return chunk + 1;
}

//void *PDAlloc(PDSize size, void *dealloc, PDBool zeroed)
//{
//    return PDAllocTyped(PDInstanceTypeUnset, size, dealloc, zeroed);
//}

#ifdef DEBUG_PD_RELEASES
void PDReleaseFunc(void *pajdegObject) 
{
    _PDReleaseDebug("--", 0, pajdegObject);
}

void _PDReleaseDebug(const char *file, int lineNumber, void *pajdegObject)
#else
void PDRelease(void *pajdegObject)
#endif
{
    if (NULL == pajdegObject) return;
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
    _PDDebugLogRetrelCall("release", file, lineNumber, pajdegObject, type->retainCount - 1);
    PDFocusCheck(pajdegObject);
    PDTypeCheck("released", /* void */);
    type->retainCount--;
#ifdef DEBUG_PD_RELEASES
    // over-autorelease check
    pd_stack s;
    pd_stack_for_each(arp, s) {
        void *pdo = s->info;
        PDTypeRef t2 = (PDTypeRef)pdo - 1;
        PDAssert(t2->retainCount > 0); // crash = over-releasing autoreleased object
    }
#endif
    if (type->retainCount == 0) {
        _PDDebugLogRetrelCall("dealloc", file, lineNumber, pajdegObject, 0);
        PDFocusCheck(pajdegObject);
        _PDDebugDeallocating(pajdegObject);
        (*type->dealloc)(pajdegObject);
        free(type);
    }
}

#ifdef DEBUG_PD_RELEASES
extern void *_PDRetainDebug(const char *file, int lineNumber, void *pajdegObject)
#else
void *PDRetain(void *pajdegObject)
#endif
{
    if (NULL == pajdegObject) return pajdegObject;
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
    _PDDebugLogRetrelCall("retain", file, lineNumber, pajdegObject, type->retainCount + 1);
    PDFocusCheck(pajdegObject);
    PDTypeCheck("retained", NULL);
    
    // if the most recent autoreleased object matches, we remove it from the autorelease pool rather than retain the object
    if (arp != NULL && pajdegObject == arp->info) {
        _PDDebugLogRetrelCall("-ar/-r", file, lineNumber, pajdegObject, type->retainCount);
        pd_stack_pop_identifier(&arp);
    } else {
        type->retainCount++;
    }
    
    return pajdegObject;
}

#ifdef DEBUG_PD_RELEASES
void *_PDAutoreleaseDebug(const char *file, int lineNumber, void *pajdegObject)
#else
void *PDAutorelease(void *pajdegObject)
#endif
{
    if (NULL == pajdegObject) return NULL;
    _PDDebugLogRetrelCall("autorelease", file, lineNumber, pajdegObject, ((PDTypeRef)pajdegObject-1)->retainCount);
    
#ifdef DEBUG_PDTYPES
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
    PDFocusCheck(pajdegObject);
    PDTypeCheck("autoreleased", NULL);
#endif
    pd_stack_push_identifier(&arp, pajdegObject);
    return pajdegObject;
}

PDInstanceType PDResolve(void *pajdegObject)
{
    if (NULL == pajdegObject) return PDInstanceTypeNull;
    
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
    PDTypeCheck("resolved", NULL);
    return type->it;
}

PDInteger PDGetRetainCount(void *pajdegObject)
{
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
    PDTypeCheck("retain-counted", -1);
    return type->retainCount;
}

void PDFlush(void)
{
    void *obj;
    while ((obj = pd_stack_pop_identifier(&arp))) {
        PDRelease(obj);
    }
}

void PDNOP(void *val) {}

#ifdef PD_WARNINGS
void _PDBreak()
{
    /*
     * The sole purpose of this method is to be a gathering spot for break points triggered by calls to the PDError() macro.
     * If you are not a Pajdeg developer, you shouldn't worry about this.
     */
    return;
}
#endif
