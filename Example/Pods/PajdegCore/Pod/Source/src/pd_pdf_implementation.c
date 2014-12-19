//
// pd_pdf_implementation.c
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
#include "pd_pdf_implementation.h"
#include "pd_pdf_private.h"
#include "PDStaticHash.h"
#include "PDStreamFilterFlateDecode.h"
#include "PDStreamFilterPrediction.h"
#include "PDReference.h"
#include "PDString.h"
#include "PDDictionary.h"
#include "PDDictionary.h"
#include "PDArray.h"
#include "PDNumber.h"
#include "PDObject.h"
#include "PDArray.h"
#include "PDDictionary.h"

void PDDeallocatorNullFunc(void *ob) {}

PDInteger users = 0;
PDStateRef pdfRoot, xrefSeeker, stringStream, arbStream;

const char * PD_META       = "meta";
const char * PD_STRING     = "string";
const char * PD_NUMBER     = "number";
const char * PD_NULL       = "null";
const char * PD_NAME       = "name";
const char * PD_OBJ        = "obj";
const char * PD_REF        = "ref";
const char * PD_HEXSTR     = "hexstr";
const char * PD_ENTRIES    = "entries";
const char * PD_DICT       = "dict";
const char * PD_DE         = "de";
const char * PD_ARRAY      = "array";
const char * PD_AE         = "ae";
const char * PD_XREF       = "xref";
const char * PD_STARTXREF  = "startxref";
const char * PD_ENDSTREAM  = "endstream";

//////////////////////////////////////////
//
// PDF complex object conversion
//

typedef void (*PDStringConverter)(pd_stack*, PDStringConvRef);
void PDStringFromMeta(pd_stack *s, PDStringConvRef scv);
void PDStringFromObj(pd_stack *s, PDStringConvRef scv);
void PDStringFromRef(pd_stack *s, PDStringConvRef scv);
void PDStringFromHexString(pd_stack *s, PDStringConvRef scv);
void PDStringFromDict(pd_stack *s, PDStringConvRef scv);
void PDStringFromDictEntry(pd_stack *s, PDStringConvRef scv);
void PDStringFromArray(pd_stack *s, PDStringConvRef scv);
void PDStringFromArrayEntry(pd_stack *s, PDStringConvRef scv);
void PDStringFromArbitrary(pd_stack *s, PDStringConvRef scv);
void PDStringFromName(pd_stack *s, PDStringConvRef scv);
void PDStringFromNumber(pd_stack *s, PDStringConvRef scv);
void PDStringFromNull(pd_stack *s, PDStringConvRef scv);
void PDStringFromString(pd_stack *s, PDStringConvRef scv);

void PDPDFSetupConverters();
void PDPDFClearConverters();

//////////////////////////////////////////
//
// PDF parsing
//

void pd_pdf_implementation_use()
{
    static PDBool first = true;
    if (first) {
        first = false;
        // register predictor handler, even if flate decode is not available
        PDStreamFilterRegisterDualFilter("Predictor", PDStreamFilterPredictionConstructor);
#ifdef PD_SUPPORT_ZLIB
        // register FlateDecode handler
        PDStreamFilterRegisterDualFilter("FlateDecode", PDStreamFilterFlateDecodeConstructor);
#endif
        // set null deallocator
        PDDeallocatorNull = PDDeallocatorNullFunc;
        // set null number
        PDNullObject = PDNumberCreateWithBool(false);
        PDFlagGlobalObject(PDNullObject);
    }
    
    if (users == 0) {
        
        pd_pdf_conversion_use();
        
        PDOperatorSymbolGlobSetup();

#define s(name) name = PDStateCreate(#name)
        
        s(pdfRoot);
        s(xrefSeeker);
        s(stringStream);
        s(arbStream);
        
        PDStateRef s(comment_or_meta);     // %anything (comment) or %%EOF (meta)
        PDStateRef s(object_reference);    // "obid genid reftype"
        PDStateRef s(dict_hex);            // dictionary (<<...>>) or hex string (<A-F0-9.*>)
        PDStateRef s(dict);                // dictionary
        PDStateRef s(name_str);            // "/" followed by symbol, potentially wrapped in parentheses
        PDStateRef s(dict_hex_term);       // requires '>' termination
        PDStateRef s(dkey);                // dictionary key (/name as a string rather than a complex)
        PDStateRef s(paren);               // "(" followed by any number of nested "(" and ")" ending with a ")"
        PDStateRef s(arb);                 // arbitrary value (number, array, dict, etc)
        PDStateRef s(number_or_obref);     // number OR number number reftype
        PDStateRef s(number);              // number
        PDStateRef s(array);               // array ([ arb arb arb ... ])
        PDStateRef s(xref);                // xref table state
        PDStateRef s(end_numeric);         // for xref seeker, when a number is encountered, find out who refers to it
        
        //pdfArrayRoot = array;
        
        // this #define is disgusting, but it removes two billion
        // 'initialization makes pointer from integer without a cast'
        // warnings outside of gnu99 mode
#define and (void*)

        //
        // PDF root
        // This begins the PDF environment, and detects objects, their definitions, their streams, etc.
        //
        
        pdfRoot->iterates = true;
        PDStateDefineOperatorsWithDefinition(pdfRoot, 
                                             PDDef("S%", 
                                                   PDDef(PDOperatorPushState, comment_or_meta),
                                                   "F",
                                                   PDDef(PDOperatorPushbackSymbol,
                                                         and PDOperatorPushState, arb),
                                                   "Sstream",
                                                   PDDef(PDOperatorPushResult),
                                                   
                                                   // endstream, ndstream, or dstream; this is all depending on how the stream length was defined; some PDF writers say that the length is from the end of "stream" to right before the "endstrean", which includes newlines; other PDF writers exclude the initial newline after "stream" in the length; these will come out as "ndstream" here; if they are visitors from the past and use DOS newline formatting (\r\n), they will come out as "dstream"
                                                   // one might fear that this would result in extraneous "e" or "en" in the written stream, but this is never the case, because Pajdeg either passes everything through to the output as is (in which case it will come out as it was), or Pajdeg replaces the stream, in which case it writes its own "endstream" keyword
                                                   "Sendstream",
                                                   PDDef(PDOperatorPushComplex, &PD_ENDSTREAM),
                                                   "Sndstream",
                                                   PDDef(PDOperatorPushComplex, &PD_ENDSTREAM),
                                                   "Sdstream",
                                                   PDDef(PDOperatorPushComplex, &PD_ENDSTREAM),
                                                   "Sxref",
                                                   PDDef(PDOperatorPushState, xref),
                                                   "Strailer",
                                                   PDDef(PDOperatorPushResult),
                                                   "Sstartxref",
                                                   PDDef(PDOperatorPushState, number,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_STARTXREF),
                                                   "Sendobj",
                                                   PDDef(PDOperatorPushResult))
                                             );
        
        //
        // comment or meta
        //
        
        PDStateDefineOperatorsWithDefinition(comment_or_meta, 
                                             PDDef("S%",
                                                   PDDef(PDOperatorPopLine,
                                                         and PDOperatorPushResult,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_META,
                                                         and PDOperatorPopState),
                                                   "F", 
                                                   PDDef(PDOperatorPopLine,
                                                         and PDOperatorPopState))
                                             );
        
        //
        // arb: arbitrary value
        //
        
        PDStateDefineOperatorsWithDefinition(arb, 
                                             PDDef("N",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPushState, number_or_obref,
                                                         and PDOperatorPopState),
                                                   "Strue",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_NUMBER,
                                                         and PDOperatorPopState),
                                                   "Sfalse",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_NUMBER,
                                                         and PDOperatorPopState),
                                                   "Snull",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_NULL,
                                                         and PDOperatorPopState),
                                                   "S(",
                                                   PDDef(//PDOperatorBreak,
                                                         PDOperatorMark,
                                                         and PDOperatorPushState, paren,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_STRING,
                                                         and PDOperatorPopState),
                                                   "S[",
                                                   PDDef(PDOperatorPushState, array,
                                                         and PDOperatorPopState),
                                                   "S/",
                                                   PDDef(//PDOperatorBreak,
                                                         PDOperatorPushState, name_str,
//                                                         and PDOperatorPushState, name,
                                                         and PDOperatorPopState),
                                                   "S<",
                                                   PDDef(PDOperatorPushState, dict_hex,
                                                         and PDOperatorPopState)));
        
        
        //
        // xref state
        //
        
        xref->iterates = true;
        PDStateDefineOperatorsWithDefinition(xref, 
                                             PDDef("N", 
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPushWeakState, number,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_XREF),
                                                   "F",
                                                   PDDef(PDOperatorPushbackSymbol,
                                                         and PDOperatorPopState)));

        //
        // number: a number
        //
        
        PDStateDefineOperatorsWithDefinition(number, 
                                             PDDef("N",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPopState)));

        //
        // number or an obref
        //
        
        PDStateDefineOperatorsWithDefinition(number_or_obref, 
                                             PDDef("N",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPushState, object_reference,
                                                         and PDOperatorPopState),
                                                   "F",
                                                   PDDef(PDOperatorPushbackSymbol,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_NUMBER,
                                                         and PDOperatorPopState)));
        
        //
        // paren: something that ends with ")" and optionally contains nested ()s
        //

        PDStateDefineOperatorsWithDefinition(paren, 
                                             PDDef("S(", 
                                                   PDDef(PDOperatorPushWeakState, paren,
                                                         and PDOperatorPopValue),
                                                   "S)", 
                                                   PDDef(PDOperatorPushMarked,
                                                         and PDOperatorPopState),
                                                   "F",
                                                   PDDef(PDOperatorPushbackSymbol,
                                                         and PDOperatorReadToDelimiter)));
        
        
        //
        // array
        //

        PDStateDefineOperatorsWithDefinition(array, 
                                             PDDef("S]", 
                                                   PDDef(PDOperatorPullBuildVariable, &PD_ENTRIES,
                                                         and PDOperatorPushComplex, &PD_ARRAY,
                                                         and PDOperatorPopState),
                                                   "F",
                                                   PDDef(PDOperatorPushbackSymbol,
                                                         and PDOperatorPushWeakState, arb,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorStoveComplex, &PD_AE)));
        

        //
        // name_str: a symbol, or a delimiter that initiates a symbol
        //
        
        PDStateDefineOperatorsWithDefinition(name_str, 
                                             PDDef("S(", 
                                                   PDDef(PDOperatorMark,
                                                         and PDOperatorPushWeakState, paren,
                                                         and PDOperatorPopState),
                                                   "F",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_NAME,
                                                         and PDOperatorPopState)));
        
        //
        // name: a symbol, or a delimiter that initiates a symbol
        //

        PDStateDefineOperatorsWithDefinition(dkey, 
                                             PDDef("S(", 
                                                   PDDef(PDOperatorMark,
                                                         and PDOperatorPushWeakState, paren,
                                                         and PDOperatorPopState),
                                                   "F",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPopState)));                                             
//                                             PDDef(/*"S(", 
//                                                    PDDef(PDOperatorPushResult,
//                                                    PDOperatorPushState, paren,
//                                                    PDOperatorPopValue,
//                                                    PDOperatorPushComplex, &PD_NAME,
//                                                    PDOperatorPopState),*/
//                                                   "F",
//                                                   PDDef(PDOperatorPushbackSymbol,
//                                                         and PDOperatorPopValue,
//                                                         and PDOperatorPushComplex, &PD_NAME,
//                                                         and PDOperatorPopState)));
        
        
        
        //
        // dict_hex: enters "dict" for "<" and falls back to hex otherwise
        //
        
        PDStateDefineOperatorsWithDefinition(dict_hex, 
                                             PDDef("S<",
                                                   PDDef(PDOperatorPushState, dict,
                                                         and PDOperatorPopState),
                                                   "S>", 
                                                   PDDef(PDOperatorPushEmptyString,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_HEXSTR,
                                                         and PDOperatorPopState),
                                                   "F",
                                                   PDDef(PDOperatorPushbackSymbol,
                                                         and PDOperatorReadToDelimiter,
                                                         and PDOperatorPushResult,
                                                         and PDOperatorPushState, dict_hex_term,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_HEXSTR,
                                                         and PDOperatorPopState)));
        
        //
        // object references (<id> <gen> <type>)
        //
        
        // object reference; expects results stack to hold genid and obid, and expects "obj" or "R" as the symbol
        PDStateDefineOperatorsWithDefinition(object_reference, 
                                             PDDef("Sobj",
                                                   PDDef(PDOperatorPopValue,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_OBJ,
                                                         and PDOperatorPopState),
                                                   "SR", 
                                                   PDDef(PDOperatorPopValue,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_REF,
                                                         and PDOperatorPopState),
                                                   "F", 
                                                   PDDef(PDOperatorPushbackSymbol,      // we ran into the sequence "<num1> <num2> <???>": first we put <???> back on symbol stack 
                                                         and PDOperatorPushbackValue,   // then we grab <num2> from the value stack and put that back on the symbol stack too
                                                         and PDOperatorPopValue,        // then we pop <num1> and make a PD_NUMBER complex based on it
                                                         and PDOperatorPushComplex, &PD_NUMBER,
                                                         and PDOperatorPopState)));
        
        
        //
        // dict: reads pairs of /name <arbitrary> /name ... until '>>'
        //
        
        PDStateDefineOperatorsWithDefinition(dict, 
                                             PDDef("S>",
                                                   PDDef(PDOperatorPushWeakState, dict_hex_term,
                                                         and PDOperatorPullBuildVariable, &PD_ENTRIES,
                                                         and PDOperatorPushComplex, &PD_DICT,
                                                         and PDOperatorPopState),
                                                   "S/",
                                                   PDDef(PDOperatorPushState, dkey,
                                                         and PDOperatorPushWeakState, arb,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorPopValue,
                                                         and PDOperatorStoveComplex, &PD_DE)));

        //
        // dict_hex_term: expects '>'
        //
        
        PDStateDefineOperatorsWithDefinition(dict_hex_term, 
                                             PDDef("S>",
                                                   PDDef(PDOperatorPopState)));
        
        //
        // STRING STREAM
        //
        
        stringStream->iterates = true;
        PDStateDefineOperatorsWithDefinition(stringStream, 
                                             PDDef("F",
                                                   PDDef(PDOperatorPushResult)));
        
        //
        // ARBITRARY STREAM
        //
        
        arbStream->iterates = true;
        PDStateDefineOperatorsWithDefinition(arbStream, 
                                             PDDef("F",
                                                   PDDef(PDOperatorPushbackSymbol,
                                                         and PDOperatorPushState, arb)));
        
        // 
        // XREF SEEKER
        // This is a tiny environment used to discover the initial XREF
        //
        
        xrefSeeker->iterates = true;
        PDStateDefineOperatorsWithDefinition(xrefSeeker, 
                                             PDDef("F", 
                                                   PDDef(PDOperatorNOP),
                                                   "N",
                                                   PDDef(PDOperatorPushResult,
                                                         and PDOperatorPushState, end_numeric),
                                                   "S>",
                                                   PDDef(PDOperatorPopState)));
        
        
        //
        // startxref grabber for xrefSeeker
        //
        
        PDStateDefineOperatorsWithDefinition(end_numeric, 
                                             PDDef("Sstartxref", 
                                                   PDDef(PDOperatorPopValue,
                                                         and PDOperatorPushComplex, &PD_STARTXREF,
                                                         and PDOperatorPopState)));
        
        
        PDStateCompile(pdfRoot);
        
        PDStateCompile(xrefSeeker);
        
        PDStateCompile(arbStream);
        
        PDStateCompile(stringStream);
        
#undef s
#define s(s) PDRelease(s)
        s(comment_or_meta);
        s(object_reference);
        s(dict_hex);
        s(name_str);
        s(dict);
        s(dict_hex_term);
        s(dkey);
        s(paren);
        s(arb);
        s(number_or_obref);
        s(number);
        s(array);
        s(xref);
        s(end_numeric);
        
#if 0
        pd_stack s = NULL;
        pd_stack o = NULL;
        pd_btree seen = NULL;
        pd_stack_push_identifier(&s, (PDID)pdfRoot);
        pd_stack_push_identifier(&s, (PDID)xrefSeeker);
        PDStateRef t;
        PDOperatorRef p;
        while (NULL != (t = (PDStateRef)pd_stack_pop_identifier(&s))) {
            pd_btree_insert(&seen, t, t);
            printf("%s\n", t->name);
            for (PDInteger i = 0; i < t->symbols; i++)
                pd_stack_push_identifier(&o, (PDID)t->symbolOp[i]);
            if (t->numberOp) pd_stack_push_identifier(&o, (PDID)t->numberOp);
            if (t->delimiterOp) pd_stack_push_identifier(&o, (PDID)t->delimiterOp);
            if (t->fallbackOp) pd_stack_push_identifier(&o, (PDID)t->fallbackOp);

            while (NULL != (p = (PDOperatorRef)pd_stack_pop_identifier(&o))) {
                while (p) {
                    assert(p->type < PDOperatorBreak);
                    assert(p->type > 0);
                    if (p->type == PDOperatorPushState && ! pd_btree_fetch(seen, p->pushedState)) {
                        pd_btree_insert(&seen, t, t);
                        pd_stack_push_identifier(&s, (PDID)p->pushedState);
                    }
                    p = p->next;
                }
            }
        }
        pd_btree_destroy(seen);
        pd_stack_destroy(&s);
#endif
    }
    users++;
}

void pd_pdf_implementation_discard()
{
    users--;
    if (users == 0) {
        PDRelease(pdfRoot);
        PDRelease(xrefSeeker);
        PDRelease(arbStream);
        PDRelease(stringStream);
        
//        PDOperatorSymbolGlobClear();
        pd_pdf_conversion_discard();
    }
}

PDInteger ctusers = 0;
void pd_pdf_conversion_use()
{
    if (ctusers == 0) {
        PDPDFSetupConverters();
    }
    ctusers++;
}

void pd_pdf_conversion_discard()
{
    ctusers--;
    if (ctusers == 0) { 
        PDPDFClearConverters();
    }
}

static PDStaticHashRef converterTable = NULL;
static PDStaticHashRef typeTable = NULL;

#define converterTableHash(key) PDStaticHashIdx(converterTable, key)

void PDPDFSetupConverters()
{
    if (converterTable) {
        return;
    }
    
    converterTable = PDStaticHashCreate(12, (void*[]) {
        (void*)PD_META,     // 1
        (void*)PD_OBJ,      // 2
        (void*)PD_REF,      // 3
        (void*)PD_HEXSTR,   // 4
        (void*)PD_DICT,     // 5
        (void*)PD_DE,       // 6 
        (void*)PD_ARRAY,    // 7
        (void*)PD_AE,       // 8
        (void*)PD_NAME,     // 9
        (void*)PD_NUMBER,   // 10
        (void*)PD_STRING,   // 11
        (void*)PD_NULL,     // 12
    }, (void*[]) {
        &PDStringFromMeta,          // 1
        &PDStringFromObj,           // 2
        &PDStringFromRef,           // 3
        &PDStringFromHexString,     // 4
        &PDStringFromDict,          // 5
        &PDStringFromDictEntry,     // 6
        &PDStringFromArray,         // 7
        &PDStringFromArrayEntry,    // 8
        &PDStringFromName,          // 9
        &PDStringFromNumber,        // 10
        &PDStringFromString,        // 11
        &PDStringFromNull           // 12
    });
    
    PDStaticHashDisownKeysValues(converterTable, true, true);
    
    typeTable = PDStaticHashCreate(12, (void*[]) {
        (void*)PD_META,     // 1
        (void*)PD_OBJ,      // 2
        (void*)PD_REF,      // 3
        (void*)PD_HEXSTR,   // 4
        (void*)PD_DICT,     // 5
        (void*)PD_DE,       // 6 
        (void*)PD_ARRAY,    // 7
        (void*)PD_AE,       // 8
        (void*)PD_NAME,     // 9
        (void*)PD_NUMBER,   // 10
        (void*)PD_STRING,   // 11
        (void*)PD_NULL,     // 12
    }, (void*[]) {
        (void*)PDObjectTypeString,
        (void*)PDObjectTypeString,
        (void*)PDObjectTypeReference,
        (void*)PDObjectTypeString,
        (void*)PDObjectTypeDictionary,
        (void*)PDObjectTypeString,
        (void*)PDObjectTypeArray,
        (void*)PDObjectTypeString,
        (void*)PDObjectTypeName,
        (void*)PDObjectTypeInteger,
        (void*)PDObjectTypeString,
        (void*)PDObjectTypeNull,
    });

    PDStaticHashDisownKeysValues(typeTable, true, true);

}

void PDPDFClearConverters()
{
    PDRelease(converterTable);
    PDRelease(typeTable);
    converterTable = NULL;
    typeTable = NULL;
}

char *PDStringFromComplex(pd_stack *complex)
{
    struct PDStringConv scv = (struct PDStringConv) {malloc(30), 0, 30};
    
    // generate
    PDStringFromArbitrary(complex, &scv);
    
    // null terminate
    if (scv.left < 1) scv.allocBuf = realloc(scv.allocBuf, scv.offs + 1);
    scv.allocBuf[scv.offs] = 0;
    
    return scv.allocBuf;
}

void *PDInstanceCreateFromComplex(pd_stack *complex)
{
//    PDStringRef str;
    void *result = NULL;
    if (*complex == NULL) return NULL;
    
    if ((*complex)->type == PD_STACK_ID) {
        PDID tid = (*complex)->info; // pd_stack_pop_identifier(complex);
        if (PDIdentifies(tid, PD_REF)) {
            result = PDReferenceCreateFromStackDictEntry(*complex);
        }
        else if (PDIdentifies(tid, PD_HEXSTR)) {
            char *str = (*complex)->prev->info;
            PDInteger len = strlen(str);
            char *rewrapped = malloc(3 + len);
            rewrapped[0] = '<';
            strcpy(&rewrapped[1], str);
            rewrapped[len+1] = '>';
            rewrapped[len+2] = 0;
            result = PDStringCreateWithHexString(rewrapped);//(strdup((*complex)->prev->info));
        }
        else if (PDIdentifies(tid, PD_NAME)) {
//            char *buf = malloc(2 + strlen((*complex)->prev->info));
//            buf[0] = '/';
//            strcpy(&buf[1], (*complex)->prev->info);
            result = PDStringCreateWithName(strdup((*complex)->prev->info));
        }
        else if (PDIdentifies(tid, PD_STRING)) {
            result = PDStringCreate(strdup((*complex)->prev->info));
//            PDStringForceWrappedState(str, false);
//            result = PDStringCreateFromStringWithType(str, PDStringTypeEscaped, true);
//            PDRelease(str);
        }
        else if (PDIdentifies(tid, PD_NUMBER)) {
            result = PDNumberCreateWithCString((*complex)->prev->info);
        }
        else if (PDIdentifies(tid, PD_NULL)) {
            result = PDRetain(PDNullObject);
        }
        else if (PDIdentifies(tid, PD_DICT)) {
            result = PDDictionaryCreateWithComplex(*complex);
//            result = PDCollectionCreateWithDictionary(pd_dict_from_pdf_dict_stack(*complex));
        }
        else if (PDIdentifies(tid, PD_ARRAY)) {
            result = PDArrayCreateWithComplex(*complex);
//            result = PDCollectionCreateWithArray(pd_array_from_pdf_array_stack(*complex));
        }
        else if (PDIdentifies(tid, PD_DE)) {
            *complex = (*complex)->prev->prev->info;
            result = PDInstanceCreateFromComplex(complex);
        }
        else if (PDIdentifies(tid, PD_AE)) {
            *complex = (*complex)->prev->info;
            result = PDInstanceCreateFromComplex(complex);
        }
        else {
            PDError("uncaught type in PDTypeFromComplex()");
        }
    } else if ((*complex)->prev) {
        // short array is the "default" when a prev exists and type != PD_STACK_ID
        result = PDArrayCreateWithStackList(*complex);
//        result = PDCollectionCreateWithArray(pd_array_from_stack(*complex));
    } else if ((*complex)->type == PD_STACK_STRING) {
        // a stack string can be a number of things:
        // 1. true, false or null
        // 2. actually that's it, nowadays
        char *value = (*complex)->info;
        if      (0 == strcmp(value, "true"))  result = PDNumberCreateWithBool(true);
        else if (0 == strcmp(value, "false")) result = PDNumberCreateWithBool(false);
        else if (0 == strcmp(value, "null"))  result = PDRetain(PDNullObject);
        else {
            PDError("unknown string value type %s", value);
            result = PDStringCreate(strdup((*complex)->info));
        }
    } else {
        PDError("unable to create PDType from unknown complex structure");
    }
    
    return result;
}

PDObjectType PDObjectTypeFromIdentifier(PDID identifier)
{
    PDAssert(typeTable); // crash = must pd_pdf_conversion_use() first
    return PDStaticHashValueForKeyAs(typeTable, *identifier, PDObjectType);
}

//////////////////////////////////////////
//
// PDF converter functions
//

void PDStringFromMeta(pd_stack *s, PDStringConvRef scv)
{}

void PDStringFromObj(pd_stack *s, PDStringConvRef scv)
{
    PDStringFromObRef("obj", 3);
}

void PDStringFromRef(pd_stack *s, PDStringConvRef scv)
{
    PDStringFromObRef("R", 1);
}

void PDStringFromName(pd_stack *s, PDStringConvRef scv)
{
    char *namestr = pd_stack_pop_key(s);
    PDInteger len = strlen(namestr);
    PDStringGrow(len + 2);
    currchi = '/';
    putstr(namestr, len);
    PDDeallocateViaStackDealloc(namestr);
}

void PDStringFromNumber(pd_stack *s, PDStringConvRef scv)
{
    char *str = pd_stack_pop_key(s);
    PDInteger len = strlen(str);
    PDStringGrow(len + 1);
    putstr(str, len);
    PDDeallocateViaStackDealloc(str);
}

void PDStringFromNull(pd_stack *s, PDStringConvRef scv)
{
//    char *null =
    PDDeallocateViaStackDealloc(pd_stack_pop_key(s));
    PDStringGrow(5);
//    PDInteger len = strlen(str);
//    PDStringGrow(len + 1);
    putstr("null", 4);
//    PDDeallocateViaStackDealloc(str);
}

void PDStringFromString(pd_stack *s, PDStringConvRef scv)
{
    char *str = pd_stack_pop_key(s);
    PDInteger len = strlen(str);
    PDStringGrow(len + 1);
    putstr(str, len);
    PDDeallocateViaStackDealloc(str);
}

void PDStringFromHexString(pd_stack *s, PDStringConvRef scv)
{
    char *hexstr = pd_stack_pop_key(s);
    PDInteger len = strlen(hexstr);
    PDStringGrow(2 + len);
    currchi = '<';
    putstr(hexstr, len);
    currchi = '>';
    PDDeallocateViaStackDealloc(hexstr);
}

void PDStringFromDict(pd_stack *s, PDStringConvRef scv)
{
    pd_stack entries;
    pd_stack entry;

    PDStringGrow(30);
    
    currchi = '<';
    currchi = '<';
    currchi = ' ';
    
    pd_stack_assert_expected_key(s, PD_ENTRIES);
    entries = pd_stack_pop_stack(s);
    for (entry = pd_stack_pop_stack(&entries); entry; entry = pd_stack_pop_stack(&entries)) {
        PDStringFromDictEntry(&entry, scv);
        PDStringGrow(3);
        currchi = ' ';
    }
    
    currchi = '>';
    currchi = '>';
}

void PDStringFromDictEntry(pd_stack *s, PDStringConvRef scv)
{
    char *key;
    PDInteger req = 30;
    PDInteger len;
    
    pd_stack_assert_expected_key(s, PD_DE);
    key = pd_stack_pop_key(s);
    len = strlen(key);
    req = 10 + len;
    PDStringGrow(req);
    currchi = '/';
    putstr(key, len);
    currchi = ' ';
    PDStringFromAnything();
    PDDeallocateViaStackDealloc(key);
}

void PDStringFromArray(pd_stack *s, PDStringConvRef scv)
{
    pd_stack entries;
    pd_stack entry;

    PDStringGrow(10);
    
    currchi = '[';
    currchi = ' ';
    
    pd_stack_assert_expected_key(s, PD_ENTRIES);
    entries = pd_stack_pop_stack(s);
    
    for (entry = pd_stack_pop_stack(&entries); entry; entry = pd_stack_pop_stack(&entries)) {
        PDStringFromArrayEntry(&entry, scv);
        PDStringGrow(2);
        currchi = ' ';
    }
    
    currchi = ']';
}

void PDStringFromArrayEntry(pd_stack *s, PDStringConvRef scv)
{
    pd_stack_assert_expected_key(s, PD_AE);
    PDStringFromAnything();
}

void PDStringFromShortArray(pd_stack *s, PDStringConvRef scv) 
{
    PDStringGrow(10);
    currchi = '[';
    currchi = ' ';
    
    while (*s) {
        PDStringFromAnything();
        PDStringGrow(2);
        currchi = ' ';
    }
    
    currchi = ']';
}

void PDStringFromArbitrary(pd_stack *s, PDStringConvRef scv)
{
    // we accept arrays in short-hand format; if this has no identifier, it must be a stack of array entries
    if ((*s)->type == PD_STACK_ID) {
        PDID type = pd_stack_pop_identifier(s);
        PDInteger hash = PDStaticHashIdx(converterTable, *type);
        (*as(PDStringConverter, PDStaticHashValueForHash(converterTable, hash)))(s, scv);
    } else {
        PDStringFromShortArray(s, scv);
    }
}

//PDInstanceTypeUnset     =-2,    ///< The associated instance value has not been set yet
//PDInstanceTypeUnknown   =-1,    ///< Undefined / non-allocated instance
//PDInstanceTypeNull      = 0,    ///< NULL
//PDInstanceTypeNumber    = 1,    ///< PDNumber
//PDInstanceTypeString    = 2,    ///< PDString
//PDInstanceTypeArray     = 3,    ///< PDArray
//PDInstanceTypeDict      = 4,    ///< PDDictionary
//PDInstanceTypeRef       = 5,    ///< PDReference
//PDInstanceTypeObj       = 6,    ///< PDObject

PDInteger PDNullPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap)
{
    PDInstancePrinterRequire(5, 5);
    char *bv = *buf;
    strcpy(&bv[offs], "null");
    return offs + 4;
}

void PDNullExchange(void *inst, PDCryptoInstanceRef ci, PDBool encrypted)
{}

PDInstancePrinter PDInstancePrinters [] = {PDNullPrinter, PDNumberPrinter, PDStringPrinter, PDArrayPrinter, PDDictionaryPrinter, PDReferencePrinter, PDObjectPrinter, PDDictionaryPrinter};

#ifdef PD_SUPPORT_CRYPTO

PDInstanceCryptoExchange PDInstanceCryptoExchanges[] = {PDNullExchange, PDNullExchange, (PDInstanceCryptoExchange)PDStringAttachCryptoInstance, (PDInstanceCryptoExchange)PDArrayAttachCryptoInstance, (PDInstanceCryptoExchange)PDDictionaryAttachCryptoInstance, PDNullExchange, PDNullExchange, (PDInstanceCryptoExchange)PDDictionaryAttachCryptoInstance};

#endif

PDDeallocator PDDeallocatorNull;

