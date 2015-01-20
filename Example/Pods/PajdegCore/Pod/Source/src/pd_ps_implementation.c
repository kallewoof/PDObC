//
//  pd_ps_implementation.c
//  Pods
//
//  Created by Karl-Johan Alm on 17/01/15.
//
//

#include "Pajdeg.h"
#include "pd_ps_implementation.h"
#include "pd_stack.h"
#include "pd_pdf_implementation.h"
#include "pd_internal.h"
#include "PDCMap.h"

//#define void name(char *funcLabel, pd_ps_env cenv) void name(char *funcLabel, pd_ps_env cenv)
typedef void (*PDPSFunc)(char *funcLabel, pd_ps_env cenv);
typedef PDDictionaryRef (*PDPSResourceLoader)(pd_ps_env cenv);

#define pd_ps_pop() pd_stack_pop_object(&cenv->operands)
#define pd_ps_push(ob) pd_stack_push_object(&cenv->operands, ob)

//////

// we define a minimal Pajdeg object here which is the exec (function) object
typedef struct PDPSX *PDPSXRef;
struct PDPSX {
    char *stream;
    PDSize length;
};

void PDPSXDestroy(PDPSXRef x)
{}
PDPSXRef PDPSXCreate(char *stream)
{
    PDPSXRef x = PDAllocTyped(PDInstanceTypePSExec, sizeof(struct PDPSX), PDPSXDestroy, false);
    x->stream = stream;
    return x;
}

//////

PDDictionaryRef PDPSResourceLoaders = NULL;
PDDictionaryRef PDPSOperatorTable = NULL;

void PDPSCreateOperatorTable(void);
PDBool pd_ps_compile_next(pd_ps_env cenv);

pd_ps_env pd_ps_create(void)
{
    pd_ps_env cenv = calloc(1, sizeof(struct pd_ps_env_s));
    cenv->operators = PDDictionaryCreate();
    cenv->userdict = PDDictionaryCreate();
    cenv->resources = PDDictionaryCreate();
    pd_stack_push_object(&cenv->dicts, cenv->userdict);
    
    return cenv;
}

void pd_ps_destroy(pd_ps_env pse)
{
    pse->userdict = NULL; // it's inside pse->dicts so it will be dealt with
    PDRelease(pse->operators);
    PDRelease(pse->scanner);
    PDRelease(pse->cmap);
    PDRelease(pse->resources);
    pd_stack_destroy(&pse->dicts);
    pd_stack_destroy(&pse->operands);
    pd_stack_destroy(&pse->execs);
    free(pse);
}

void pd_ps_set_cmap(pd_ps_env pse, PDCMapRef cmap)
{
    PDRetain(cmap);
    PDRelease(pse->cmap);
    pse->cmap = cmap;
    pse->explicitCMap = true;
}

PDCMapRef pd_ps_get_cmap(pd_ps_env pse)
{
    return pse->cmap;
}

PDBool pd_ps_execute_postscript(pd_ps_env cenv, char *stream, PDSize len)
{
    if (PDPSOperatorTable == NULL) PDPSCreateOperatorTable();
    
    // we use the pdf arbitrary parser as CMap data is a mixture of PDF content and PostScript commands
    PDScannerRef scanner;
    
    pd_pdf_implementation_use(); {
        scanner = PDScannerCreateWithState(arbStream);
        scanner->strict = false;
        PDScannerAttachFixedSizeBuffer(scanner, stream, len);
        cenv->scanner = scanner;
        while (! cenv->failure && pd_ps_compile_next(cenv)) {}
    } pd_pdf_implementation_discard();
    
    return ! cenv->failure;
}

PDDictionaryRef pd_ps_get_results(pd_ps_env pse)
{
    pd_stack s = pse->dicts;
    while (s && s->prev) s = s->prev;
    return s ? s->info : pse->userdict;
}

void *pd_ps_get_operand(pd_ps_env pse, PDSize index)
{
    pd_stack s = pse->operands;
    while (s && index > 0) { 
        index--;
        s = s->prev;
    }
    return s ? s->info : NULL;
}

void pd_ps_push_exec(pd_ps_env cenv, PDPSXRef x)
{
    PDScannerRef scanner = PDScannerCreateWithState(arbStream);
    scanner->strict = false;
    PDScannerAttachFixedSizeBuffer(scanner, x->stream, x->length);
    pd_stack_push_object(&cenv->execs, cenv->scanner);
    cenv->scanner = scanner;
}

PDBool pd_ps_compile_next(pd_ps_env cenv)
{
    pd_stack stack;
    char *string;
    void *value;
    
    char *operatorName;
    PDScannerRef scanner = cenv->scanner;
    
    operatorName = NULL;
    string = NULL;
    
    if (PDScannerPopStack(scanner, &stack)) {
        // we got a stack
        // push onto args and we're done
        value = PDInstanceCreateFromComplex(&stack);
        pd_ps_push(value);
        return true;
    }
    
    if (PDScannerPopString(scanner, &string) || PDScannerPopUnknown(scanner, &string)) {
        // we got a string
        operatorName = string;
    }
    else {
        if (! PDScannerEndOfStream(scanner)) {
            PDWarn("PostScript stream compilation stumbled near: \"%s\"", &scanner->buf[scanner->boffset > 20 ? scanner->boffset-20 : 0]);
        }
        if (cenv->execs) {
            // pop back to previous exec
            PDRelease(cenv->scanner);
            cenv->scanner = pd_stack_pop_object(&cenv->execs);
            return pd_ps_compile_next(cenv);
        }
        return false;
    }
    
    if (operatorName) {
        // look in operators dict (this can override system functions, I believe; this may be wrong in which case this and below blocks should swap places)
//        PDNumberRef funcPtr = PDDictionaryGet(cenv->operators, operatorName);
//        
//        if (funcPtr == NULL) {
//            // look in system func dict
//            funcPtr = PDDictionaryGet(PDPSOperatorTable, operatorName);
//        }
        PDNumberRef funcPtr = PDDictionaryGet(PDPSOperatorTable, operatorName);
        
        if (funcPtr) {
            PDPSFunc func = PDNumberGetPointer(funcPtr);
            func(operatorName, cenv);
            free(operatorName);
            return true;
        }
        
        // look in user dicts
        pd_stack_for_each(cenv->dicts, stack) {
            PDDictionaryRef dict = stack->info;
            void *value = PDDictionaryGet(dict, operatorName);
            if (value) {
                PDInstanceType it = PDResolve(value);
                switch (it) {
                    case PDInstanceTypePSExec:
                        // if this is an exec, we exec
                        pd_ps_push_exec(cenv, value);
                        return pd_ps_compile_next(cenv);

                    case PDInstanceTypeNumber:
                        // if this is a pointer, it points to an internal PD_PS_FUNC
                        if (PDObjectTypeReference == PDNumberGetObjectType(value)) {
                            PDPSFunc func = PDNumberGetPointer(value);
                            func(operatorName, cenv);
                            free(operatorName);
                            return true;
                        }
                        break;
                        
                    default:break;
                }

                // otherwise we just push the dict value onto operands
                pd_ps_push(PDRetain(value));
                return true;
            }
        }
        
        PDError("PostScript error: undefined: %s", operatorName);
        cenv->failure = true;
        return false;
    } 
    
    if (string) pd_ps_push(PDStringCreate(string));

    PDError("???");
    cenv->failure = true;
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void PDPSNop(char *funcLabel, pd_ps_env cenv) {}

//
// operand stack manipulation operators
//

void PDPSDup(char *funcLabel, pd_ps_env cenv)
{
    pd_ps_push(PDRetain(cenv->operands->info));
}

void PDPSCopy(char *funcLabel, pd_ps_env cenv)
// (... num): Duplicate top n elements
// (arr1 arr2): copy elements of arr1 to initial subarray of arr2 (???? what does that even mean????)
{
    pd_stack tmp = NULL;
    PDNumberRef count = pd_ps_pop();
    if (PDResolve(count) != PDInstanceTypeNumber) {
        PDError("PostScript: invalid argument to copy operator");
        cenv->failure = true;
        return;
    }
    
    PDSize i;
    PDSize n = PDNumberGetSize(count);
    pd_stack iter = cenv->operands;
    PDRelease(count);
    
    for (i = 0; iter && i < n; i++) {
        pd_stack_push_object(&tmp, PDRetain(iter->info)); // note that we retain as a destroyed stack will release, and we would end up releasing these objects twice but only retaining them once otherwise
        iter = iter->prev;
    }
    
    if (! iter && i > 0) {
        PDError("PostScript: insufficient number of entries in stack to copy %lu entries", (unsigned long)n);
        cenv->failure = true;
        return;
    }
    
    while (tmp) pd_stack_pop_into(&cenv->operands, &tmp);
}

void PDPSIndex(char *funcLabel, pd_ps_env cenv) // Duplicate arbitrary element
{
    PDNumberRef count = pd_ps_pop();
    if (PDResolve(count) != PDInstanceTypeNumber) {
        PDError("PostScript: invalid argument to index operator");
        cenv->failure = true;
        return;
    }
    
    PDSize i;
    PDSize n = PDNumberGetSize(count);
    pd_stack iter = cenv->operands;
    PDRelease(count);
    
    for (i = 0; iter && i < n; i++) {
        iter = iter->prev;
    }
    
    if (! iter) {
        PDError("PostScript: insufficient number of entries in stack to get entry at index %lu", (unsigned long)n);
        cenv->failure = true;
        return;
    }
    
    pd_ps_push(PDRetain(iter->info));
}

void PDPSPop(char *funcLabel, pd_ps_env cenv)
{
    PDRelease(pd_ps_pop());
}

void PDPSClear(char *funcLabel, pd_ps_env cenv)
{
    pd_stack_destroy(&cenv->operands);
}

void PDPSCount(char *funcLabel, pd_ps_env cenv)
{
    PDSize i = 0;
    pd_stack iter = cenv->operands;
    while (iter) {
        i++;
        iter = iter->prev;
    }
    pd_ps_push(PDNumberCreateWithSize(i));
}

void PDPSExch(char *funcLabel, pd_ps_env cenv)
{
    void *b = pd_ps_pop();
    void *a = pd_ps_pop();
    pd_ps_push(b);
    pd_ps_push(a);
}

// 
// arithmetic / math operators
//

void PDPSAdd(char *funcLabel, pd_ps_env cenv)
{
    PDNumberRef b = pd_ps_pop();
    PDNumberRef a = pd_ps_pop();
    PDNumberRef c = PDNumberCreateWithReal(PDNumberGetReal(a) + PDNumberGetReal(b));
    pd_ps_push(c);
    PDRelease(a);
    PDRelease(b);
}

void PDPSDiv(char *funcLabel, pd_ps_env cenv)
{
    PDNumberRef b = pd_ps_pop();
    PDNumberRef a = pd_ps_pop();
    PDNumberRef c = PDNumberCreateWithReal(PDNumberGetReal(a) / PDNumberGetReal(b));
    pd_ps_push(c);
    PDRelease(a);
    PDRelease(b);
}

//
//
//

void PDPSDict(char *funcLabel, pd_ps_env cenv)
{
    /*
     12 dict begin 
     //
     /CIDSystemInfo 3 dict dup begin
     /Registry (Adobe) def
     /Ordering (Japan1) def
     /Supplement 0 def
     end def
     */
    
    // arg is capacity but we don't care about that right now, beyond the fact we trigger explosion if that's not the case
    PDRelease(pd_ps_pop());
    
    pd_ps_push(PDDictionaryCreate());
}

void PDPSBegin(char *funcLabel, pd_ps_env cenv)
{
    PDDictionaryRef dict = pd_ps_pop();
    pd_stack_push_object(&cenv->dicts, dict);
    cenv->userdict = dict;
}

void PDPSEnd(char *funcLabel, pd_ps_env cenv)
{
    if (cenv->dicts->prev) {
        pd_stack_pop_object(&cenv->dicts);
        PDRelease(cenv->userdict);
        cenv->userdict = cenv->dicts->info;
    } else {
        PDWarn("unexpected end procedure call in PostScript stream is ignored");
    }
}

void PDPSCurrentDict(char *funcLabel, pd_ps_env cenv)
{
    pd_ps_push(PDRetain(cenv->userdict));
}

void PDPSDef(char *funcLabel, pd_ps_env cenv)
{
    void *value = pd_ps_pop();
    void *key = pd_ps_pop();
    PDDictionarySet(cenv->userdict, PDStringBinaryValue(key, NULL), value);
    PDRelease(value);
    PDRelease(key);
}

void PDPSLCBrace(char *funcLabel, pd_ps_env cenv)
{
    // "{" : starts a function definition
    PDScannerRef scanner = cenv->scanner;
    PDInteger oldOffset = scanner->boffset;
    PDPSXRef exec = PDPSXCreate(&scanner->buf[cenv->scanner->boffset]);
    while (false == scanner->outgrown) {
        PDScannerPassSymbolCharacterType(scanner, PDOperatorSymbolGlobDelimiter);
        if (scanner->buf[scanner->boffset-1] == '}') break;
    }
    exec->length = scanner->boffset - oldOffset - 1 /* sans } */;
    pd_ps_push(exec);
}

void PDPSGt(char *funcLabel, pd_ps_env cenv)
{
    // greater than
    PDNumberRef b = pd_ps_pop();
    PDNumberRef a = pd_ps_pop();
    PDNumberRef c = PDNumberCreateWithBool(PDNumberGetReal(a) > PDNumberGetReal(b));
    pd_ps_push(c);
    PDRelease(a);
    PDRelease(b);
}

void PDPSIfelse(char *funcLabel, pd_ps_env cenv)
{
    // (bool) (exec1) (exec2)
    // if (bool), (exec1) otherwise (exec2)
    PDPSXRef falseExec = pd_ps_pop();
    PDPSXRef truthExec = pd_ps_pop();
    PDNumberRef flag = pd_ps_pop();
    if (PDNumberGetBool(flag)) {
        pd_ps_push_exec(cenv, truthExec);
    } else {
        pd_ps_push_exec(cenv, falseExec);
    }
    PDRelease(flag);
    PDRelease(truthExec);
    PDRelease(falseExec);
    pd_ps_compile_next(cenv);
}

void PDPSFindResource(char *funcLabel, pd_ps_env cenv)
// Ref p. 89: The findresource operator is the key feature of the resource facility. Given a resource category name and an instance name, findresource returns an object. If the requested resource instance does not already exist as an object in VM, findresource gets it from an external source and loads it into VM. A PostScript program can access named resources without knowing whether they are already in VM or how they are obtained from external storage.
{
    PDStringRef category = pd_ps_pop();
    PDStringRef rname = pd_ps_pop();
    const char *categoryStr = PDStringBinaryValue(category, NULL);
    const char *rnameStr = PDStringBinaryValue(rname, NULL);
    
    // get dictionary for category, if any
    PDDictionaryRef sysCatDict = PDDictionaryGet(PDPSResourceLoaders, categoryStr);
    PDDictionaryRef usrCatDict = PDDictionaryGet(cenv->resources, categoryStr);
    PDDictionaryRef opTable = NULL, loaders = NULL;
    if (usrCatDict) {
        opTable = PDDictionaryGet(usrCatDict, rnameStr);
        if (! opTable) {
            // get loader
            loaders = PDDictionaryGet(usrCatDict, " __loaders__ ");
        }
    }
    if (! opTable && ! loaders && sysCatDict) {
        // try system dictionary
        opTable = PDDictionaryGet(sysCatDict, rnameStr);
        if (! opTable) {
            // get loader
            loaders = PDDictionaryGet(sysCatDict, " __loaders__ ");
        }
    }
    if (! opTable && loaders) {
        PDNumberRef ldrPtr = PDDictionaryGet(loaders, rnameStr);
        if (ldrPtr) {
            PDPSResourceLoader ldr = PDNumberGetPointer(ldrPtr);
            opTable = PDAutorelease(ldr(cenv));
            if (! usrCatDict) {
                usrCatDict = PDDictionaryCreateWithBucketCount(10);
                PDDictionarySet(cenv->resources, categoryStr, usrCatDict);
                PDRelease(usrCatDict);
            }
            PDDictionarySet(usrCatDict, rnameStr, opTable);
        }
    }
    
    if (opTable) {
        pd_ps_push(PDRetain(opTable)); // the PDAutorelease call above and this PDRetain may appear matching (thus both could be thrown out) but whenever loading is unnecessary the opTable will miss a +1 retain
    }  else {
        PDWarn("Undefined resource %s in category %s", rnameStr, categoryStr);
    }
    
    PDRelease(category);
    PDRelease(rname);
}

void PDPSDefineResource(char *funcLabel, pd_ps_env cenv)
/*
 Ref, p. 570 - 571: key instance category defineresource instance

 Associates a resource instance with a resource name in a specified category. category is a name object that identifies a resource category, such as Font (see Section 3.9.2, “Resource Categories”). key is a name or string object that will be used to identify the resource instance. (Names and strings are interchangeable; other types of keys are permitted but are not recommended.) instance is the resource instance itself; its type must be appropriate to the resource category.
 Before defining the resource instance, defineresource verifies that the instance ob- ject is the correct type. Depending on the resource category, it may also perform additional validation of the object and may have other side effects (see Section 3.9.2); these side effects are determined by the DefineResource procedure in the category implementation dictionary. Finally, defineresource makes the ob- ject read-only if its access is not already restricted.
 The lifetime of the definition depends on the VM allocation mode at the time defineresource is executed. If the current VM allocation mode is local (currentglobal returns false), the effect of defineresource is undone by the next nonnested restore operation. If the current VM allocation mode is global (currentglobal returns true), the effect of defineresource persists until global VM is restored at the end of the job. If the current job is not encapsulated, the effect of a global defineresource operation persists indefinitely, and may be visible to other execution contexts.
 Local and global definitions are maintained separately. If a new resource instance is defined with the same category and key as an existing one, the new definition overrides the old one. The precise effect depends on whether the old definition is local or global and whether the new definition (current VM allocation mode) is local or global. There are two main cases:
 • The new definition is local. defineresource installs the new local definition, replacing an existing local definition if there is one. If there is an existing global definition, defineresource does not disturb it. However, the global definition is obscured by the local one. If the local definition is later removed, the global definition reappears.
 • The new definition is global. defineresource first removes an existing local defi- nition if there is one. It then installs the new global definition, replacing an existing global definition if there is one.
 defineresource can be used multiple times to associate a given resource instance with more than one key.
 If the category name is unknown, an undefined error occurs. If the instance is of the wrong type for the specified category, a typecheck error occurs. If the instance is in local VM but the current VM allocation mode is global, an invalidaccess error occurs; this is analogous to storing a local object into a global dictionary. Other errors can occur for specific categories; for example, when dealing with the Font or CIDFont category, defineresource may execute an invalidfont error.
 */
{
    PDStringRef category = pd_ps_pop();
    PDDictionaryRef instance = pd_ps_pop();
    PDStringRef rname = pd_ps_pop();
    
    // check types
    PDBool b = true;
    PDAssert(!b || (b = PDResolve(category) == PDInstanceTypeString));
    PDAssert(!b || (b = PDResolve(instance) == PDInstanceTypeDict));
    PDAssert(!b || (b = PDResolve(rname) == PDInstanceTypeString))
    if (! b) {
        PDWarn("Invalid types in call to defineresource");
        // we leak the objects here; 
        return;
    }
    
    const char *categoryStr = PDStringBinaryValue(category, NULL);
    const char *rnameStr = PDStringBinaryValue(rname, NULL);
    
    // get dictionary for category, if any
    PDDictionaryRef catDict = PDDictionaryGet(cenv->resources, categoryStr);
    if (! catDict) {
        // category is new
        catDict = PDDictionaryCreateWithBucketCount(10);
        PDDictionarySet(cenv->resources, categoryStr, catDict);
        PDRelease(catDict);
    }

    // set loaded dictionary (instance)
    PDDictionarySet(catDict, rnameStr, instance);
    
    PDRelease(category);
    PDRelease(instance);
    PDRelease(rname);
}

PDDictionaryRef PDPSProcSetBitmapFontInit(pd_ps_env cenv);
PDDictionaryRef PDPSProcSetCIDInit(pd_ps_env cenv);
PDDictionaryRef PDPSProcSetColorRendering(pd_ps_env cenv);
PDDictionaryRef PDPSProcSetFontSetInit(pd_ps_env cenv);
PDDictionaryRef PDPSProcSetTrapping(pd_ps_env cenv);

void PDPSCreateOperatorTable(void)
{
#define PDPSFunctionTableBind(table, key, func) PDDictionarySet(table, key, PDNumberWithPointer(func))
#define PDPSOperatorTableBind(key, func) PDPSFunctionTableBind(PDPSOperatorTable, key, PDPS##func)
#define PDPSResourceLoaderBind(key) PDPSFunctionTableBind(loaders, #key, PDPSProcSet##key)
    
    PDPSResourceLoaders = PDDictionaryCreateWithBucketCount(10);
    PDDictionaryRef procSet = PDDictionaryCreateWithBucketCount(2);
    PDDictionaryRef loaders = PDDictionaryCreateWithBucketCount(7);
    PDDictionarySet(procSet, " __loaders__ ", loaders);
    PDDictionarySet(PDPSResourceLoaders, "ProcSet", procSet);
    
    PDPSResourceLoaderBind(CIDInit);
    PDPSResourceLoaderBind(ColorRendering);
    PDPSResourceLoaderBind(FontSetInit);
    PDPSResourceLoaderBind(Trapping);
    
    PDPSOperatorTable = PDDictionaryCreate();
    
    PDPSOperatorTableBind("{", LCBrace);

    //
    // operand stack manipulation operators
    //
    
    PDPSOperatorTableBind("pop", Pop);
    PDPSOperatorTableBind("exch", Exch);
    PDPSOperatorTableBind("dup", Dup);
    PDPSOperatorTableBind("copy", Copy);
    PDPSOperatorTableBind("index", Index);
    // roll
    PDPSOperatorTableBind("clear", Clear);
    PDPSOperatorTableBind("count", Count);
    // mark
    // cleartomark
    // counttomark

    // 
    // arithmetic / math operators
    //
    
    PDPSOperatorTableBind("add", Add);
    PDPSOperatorTableBind("div", Div);
    // idiv, mod, mul, sub, abs, neg, ceiling, floor, round, truncate, sqrt, atan, cos, sin, exp, ln, log, rand, srand, rrand
    
    //
    // array operators
    //
    
    // array, length, get, put, getinterval, putinterval, astore, aload, (copy), forall
    
    //
    // packed array operators
    //
    
    // packedarray, setpacking, currentpacking, length, get, getinterval, aload, copy, forall
    
    //
    // dictionary operators
    //
    
    PDPSOperatorTableBind("dict", Dict);
    // length, maxlength
    PDPSOperatorTableBind("begin", Begin);
    PDPSOperatorTableBind("end", End);
    PDPSOperatorTableBind("def", Def);
    // load, store, get, put, undef, known, where, copy, forall
    PDPSOperatorTableBind("currentdict", CurrentDict);
    // errordict, $error, systemdict, userdict, globaldict, statusdict, countdictstack, dictstack, cleardictstack
    
    //
    // string operators
    //
    
    // string, length, get, put, getinterval, putinterval, copy, forall, anchorsearch, search, token
    
    //
    // relational, boolean, and bitwise operators
    //
    
    // eq, ne, ge
    PDPSOperatorTableBind("gt", Gt);
    // le, lt, and, not, or, xor, (true), (false), bitshift
    
    //
    // control operators
    //
    
    // exec, if
    PDPSOperatorTableBind("ifelse", Ifelse);
    // for, repeat, loop, exit, stop, stopped, countexecstack, execstack, quit, start
    
    //
    // type, attribute, and conversion operators
    //
    
    // type, cvlit, cvx, xcheck, executeonly, noaccess, readonly, rcheck, wcheck, cvi, cvn, cvr, cvrs, cvs
    
    //
    // file operators
    //
    
    // file, filter, closefile, read, write, readhexstring, writehexstring, readstring, writestring, readline, token, bytesavailable, flush, flushfile, resetfile, status, status (2), run, currentfile, deletefile, renamefile, filenameforall, setfileposition, fileposition, print, =, ==, stack, pstack, printobject, writeobject, setobjectformat, currentobjectformat
    
    //
    // resource operators
    //
    
    PDPSOperatorTableBind("defineresource", DefineResource);
    // undefineresource
    PDPSOperatorTableBind("findresource", FindResource);
    // findcolorrendering, resourcestatus, resourceforall
    
    // 
    // virtual memory operators
    //
    
    // save, restore, setglobal, currentglobal, gcheck, startjob, defineuserobject, execuserobject, undefineuserobject, UserObjects
    
    //
    // miscellaneous operators
    //

    PDPSOperatorTableBind("bind", Nop);
    // (null), version, realtime, usertime, languagelevel, product, revision, serialnumber, executive, echo, prompt
    
    //
    // graphics state operators (device-independent)
    //
    
    // gsave, grestore, clipsave, cliprestore, grestoreall, initgraphics, gstate, setgstate, currentgstate, setlinewidth, currentlinewidth, setlinecap, currentlinecap, setlinejoin, currentlinejoin, setmiterlimit, currentmiterlimit, setstrokeadjust, currentstrokeadjust, setdash, currentdash, setcolorspace, currentcolorspace, setcolor, setcolor (2), setcolor (3), currentcolor, setgray, currentgray, sethsbcolor, currenthsbcolor, setrgbcolor, currentrgbcolor, setcmykcolor, currentcmykcolor
    
    //
    // graphics state operators (device-dependent)
    //
    
    // sethalftone, currenthalftone, setscreen, currentscreen, setcolorscreen, currentcolorscreen, settransfer, currenttransfer, setcolortransfer, currentcolortransfer, setblackgeneration, currentblackgeneration, ...
    
    //
    // coordinate system and matrix operators
    //
    
    // matrix, initmatrix, identmatrix, defaultmatrix, currentmatrix, setmatrix, translate, translate (2), scale, scale (2), rotate, rotate (2), ...
    
    //
    // ...
    //
    
    //
    // glyph and font operators
    //
    
    // definefont, composefont, undefinefont, findfont, scalefont, makefont, setfont, rootfont, currentfont, selectfont, show, ashow, widthshow, awidthshow, xshow, xyshow, yshow, glyphshow, stringwidth, cshow, kshow, FontDirectory, GlobalFontDirectory, StandardEncoding, ISOLatin1Encoding, findencoding, setcachedevice, setcachedevice2, setcharwidth
}

////////////////////////////////////////////////////////////////////////////////
// CIDInit procedure set
////////////////////////////////////////////////////////////////////////////////

void PDPSCIDInitParseMappings(pd_ps_env cenv, PDSize args, void *callback)
{
    PDAssert(1 < args && args < 4); // crash = args outside of defined bounds (2..3)
    void(*cb2)(PDCMapRef cmap, PDStringRef str1, PDStringRef str2) = args == 2 ? callback : NULL;
    void(*cb3)(PDCMapRef cmap, PDStringRef str1, PDStringRef str2, PDStringRef str3) = args == 3 ? callback : NULL;
    
    PDSize i;
    PDBool proceed;
    PDStringRef *operands = malloc(sizeof(PDStringRef) * args);
    cenv->mpbool = true; // when false, we are supposed to stop
    while (cenv->mpbool) {
        // pull next args from stream; note that mpbool goes false at the first one
        proceed = pd_ps_compile_next(cenv);
        if (! cenv->mpbool) break;
        
        if (proceed) operands[0] = pd_ps_pop();
        
        for (i = 1; proceed && i < args; i++) {
            proceed = pd_ps_compile_next(cenv);
            if (proceed) operands[i] = pd_ps_pop();
        }
        if (! proceed) {
            // failure
            cenv->failure = true;
            PDWarn("failed to parse mappings");
            break;
        }
        
        // get the args operands; these must be strings
        for (i = 0; i < args; i++) {
            if (! operands[i]) {
                PDWarn("NULL operand %ld", i);
                proceed = false;
                break;
            }
            if (PDResolve(operands[i]) != PDInstanceTypeString) {
                PDWarn("Invalid operand %ld: expected string (%d), got %d", i, PDInstanceTypeString, PDResolve(operands[i]));
                proceed = false;
                break;
            }
        }
        
        if (proceed) {
            // all is swell
            if (cb2) cb2(cenv->cmap, operands[0], operands[1]);
            if (cb3) cb3(cenv->cmap, operands[0], operands[1], operands[2]);
        }
        
        // clean up
        for (i = 0; i < args; i++) PDRelease(operands[i]);
        
        if (! proceed) break; // todo: push values back onto stack and perhaps move scanner back to avoid data loss
    }
    
    free(operands);
}

void PDPSCIDInitBeginCMap(char *funcLabel, pd_ps_env cenv)
{
    if (! cenv->explicitCMap || ! cenv->cmap) {
        PDRelease(cenv->cmap);
        cenv->cmap = PDCMapCreate();
    }
}

void PDPSCIDInitBeginCodespaceRange(char *funcLabel, pd_ps_env cenv)
// Ref, p. 537: begins the definition of n codespace ranges (completed by endcodespacerange); see Section 5.11.4, “CMap Dictionaries.”
{
    PDNumberRef count = pd_ps_pop();
    if (PDResolve(count) != PDInstanceTypeNumber) {
        PDError("Invalid type for begincodespacerange argument (expected %d, got %d)", PDInstanceTypeNumber, PDResolve(count));
        cenv->failure = true;
    } else {
        PDCMapAllocateCodespaceRanges(cenv->cmap, PDNumberGetSize(count));
    }
    PDRelease(count);
    
    PDPSCIDInitParseMappings(cenv, 2, PDCMapAppendCodespaceRange);
}

void PDPSCIDInitBeginBFCharMapping(char *funcLabel, pd_ps_env cenv)
// Ref, p. 537: starts a mapping (completed by endbfchar) from n individual character codes to character codes or names (which usually select characters in base fonts); see Section 5.11.4, “CMap Dictionaries.”
{
    PDNumberRef count = pd_ps_pop();
    if (PDResolve(count) != PDInstanceTypeNumber) {
        PDError("Invalid type for beginbfchar argument (expected %d, got %d)", PDInstanceTypeNumber, PDResolve(count));
        cenv->failure = true;
    } else {
        PDCMapAllocateBFChars(cenv->cmap, PDNumberGetSize(count));
    }
    PDRelease(count);
    
    PDPSCIDInitParseMappings(cenv, 2, PDCMapAppendBFChar);
}

void PDPSCIDInitBeginBFRangeMapping(char *funcLabel, pd_ps_env cenv)
// Ref, p. 537: starts a mapping (completed by endbfrange) from n character code ranges to character codes or names (which usually select characters in base fonts); see Section 5.11.4, “CMap Dictionaries.”
{
    PDNumberRef count = pd_ps_pop();
    if (PDResolve(count) != PDInstanceTypeNumber) {
        PDError("Invalid type for beginbfrange argument (expected %d, got %d)", PDInstanceTypeNumber, PDResolve(count));
        cenv->failure = true;
    } else {
        PDCMapAllocateBFRanges(cenv->cmap, PDNumberGetSize(count));
    }
    PDRelease(count);
    
    PDPSCIDInitParseMappings(cenv, 3, PDCMapAppendBFRange);
}

void PDPSCIDInitEndCMap(char *funcLabel, pd_ps_env cenv)
{
    PDDictionarySet(cenv->userdict, "#cmap#", cenv->cmap);
    PDRelease(cenv->cmap);
    cenv->cmap = NULL;
    cenv->explicitCMap = false;
}

void PDPSCIDInitEndCodespaceRange(char *funcLabel, pd_ps_env cenv)
{
    cenv->mpbool = false;
}

void PDPSCIDInitEndBFCharMapping(char *funcLabel, pd_ps_env cenv)
{
    cenv->mpbool = false;
}

void PDPSCIDInitEndBFRangeMapping(char *funcLabel, pd_ps_env cenv)
{
    cenv->mpbool = false;
}

#undef PDPSOperatorTableBind
#define PDPSOperatorTableBind(key, func) PDPSFunctionTableBind(opTable, key, PDPSCIDInit##func)

PDDictionaryRef PDPSProcSetCIDInit(pd_ps_env cenv)
{
    PDDictionaryRef opTable = PDDictionaryCreate();
    
    PDPSOperatorTableBind("beginbfchar", BeginBFCharMapping);
    PDPSOperatorTableBind("beginbfrange", BeginBFRangeMapping);
    // begincidchar, begincidrange
    PDPSOperatorTableBind("begincmap", BeginCMap);
    PDPSOperatorTableBind("begincodespacerange", BeginCodespaceRange);
    // beginnotdefchar, beginrearrangedfont, beginusematrix
    PDPSOperatorTableBind("endbfchar", EndBFCharMapping);
    PDPSOperatorTableBind("endbfrange", EndBFRangeMapping);
    // endcidchar, endcidrange
    PDPSOperatorTableBind("endcmap", EndCMap);
    PDPSOperatorTableBind("endcodespacerange", EndCodespaceRange);
    // endnotdefchar, endnotdefrange, endrearrangedfont, endusematrix, StartData, usecmap, usefont
    
    return opTable;
}

////////////////////////////////////////////////////////////////////////////////
// Placeholder procedure sets (not implemented)
////////////////////////////////////////////////////////////////////////////////

PDDictionaryRef PDPSProcSetBitmapFontInit(pd_ps_env cenv)
{
    PDDictionaryRef opTable = PDDictionaryCreate();
    return opTable;
}

PDDictionaryRef PDPSProcSetColorRendering(pd_ps_env cenv)
{
    PDDictionaryRef opTable = PDDictionaryCreate();
    return opTable;
}

PDDictionaryRef PDPSProcSetFontSetInit(pd_ps_env cenv)
{
    PDDictionaryRef opTable = PDDictionaryCreate();
    return opTable;
}

PDDictionaryRef PDPSProcSetTrapping(pd_ps_env cenv)
{
    PDDictionaryRef opTable = PDDictionaryCreate();
    return opTable;
}
