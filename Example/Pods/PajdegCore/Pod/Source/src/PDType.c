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



PDSplayTreeRef _retrels = NULL;

static PDBool loggingRetrelCall = false;
void _PDDebugLogRetrelCall(const char *op, const char *file, int lineNo, void *ob)
{
    // since alloc is logged, we need to prevent logging of calls made here
    if (loggingRetrelCall) return;
    loggingRetrelCall = true;
    
    if (_retrels == NULL) {
        _retrels = PDSplayTreeCreateWithDeallocator(PDDeallocatorNullFunc);
    }
    pd_stack entry = PDSplayTreeGet(_retrels, (PDInteger)ob);
    pd_stack_push_identifier(&entry, (PDID)(PDInteger)lineNo);
    pd_stack_push_key(&entry, strdup(file));
    pd_stack_push_identifier(&entry, (PDID)op);
    PDSplayTreeInsert(_retrels, (PDInteger)ob, entry);
    
    loggingRetrelCall = false;
}

void _PDDebugLogDisplay(void *ob)
{
    if (_retrels == NULL) return;
    pd_stack entry = PDSplayTreeGet(_retrels, (PDInteger)ob);
    if (entry) {
        printf("Retain/release log for %p:\n", ob);
        printf("op:         line:  file:\n");
        while (entry) {
            char *op = (char *)entry->info ; entry = entry->prev;
            char *file = (char *)entry->info ; entry = entry->prev;
            int lineNo = (int)entry->info ; entry = entry->prev;
            printf("%11s %6d %s\n", op, lineNo, file);
        }
    } else {
        printf("(no log for %p!)\n", ob);
    }
}
#else
#define _PDDebugLogRetrelCall(...) 
#define _PDDebugLogDisplay(ob) 
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
    _PDDebugLogRetrelCall("alloc", file, lineNumber, chunk + 1);
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
    _PDDebugLogRetrelCall("release", file, lineNumber, pajdegObject);
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
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
    _PDDebugLogRetrelCall("retain", file, lineNumber, pajdegObject);
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
    PDTypeCheck("retained", NULL);
    
    // if the most recent autoreleased object matches, we remove it from the autorelease pool rather than retain the object
    if (arp != NULL && pajdegObject == arp->info) {
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
    _PDDebugLogRetrelCall("autorelease", file, lineNumber, pajdegObject);
    
#ifdef DEBUG_PDTYPES
    PDTypeRef type = (PDTypeRef)pajdegObject - 1;
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
