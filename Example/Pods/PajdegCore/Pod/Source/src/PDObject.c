//
// PDObject.c
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

#include "Pajdeg.h"

#include "pd_internal.h"
#include "pd_stack.h"
#include "PDScanner.h"
#include "pd_pdf_implementation.h"
#include "pd_pdf_private.h"
#include "PDStreamFilter.h"
#include "PDObject.h"
#include "PDNumber.h"
#include "PDArray.h"
#include "PDDictionary.h"
#include "PDString.h"
#include "pd_crypto.h"

void PDObjectDestroy(PDObjectRef object)
{
    PDRelease(object->inst);

#ifdef PD_SUPPORT_CRYPTO
    PDRelease(object->cryptoInstance);
#endif
    
    pd_stack_destroy(&object->def);
    
    if (object->ovrDef) free(object->ovrDef);
    if (object->ovrStream && object->ovrStreamAlloc)
        free(object->ovrStream);
    if (object->refString) free(object->refString);
    if (object->extractedLen != -1) free(object->streamBuf);
}

PDObjectRef PDObjectCreate(PDInteger obid, PDInteger genid)
{
    PDObjectRef ob = PDAllocTyped(PDInstanceTypeObj, sizeof(struct PDObject), PDObjectDestroy, true);
    //PDObjectRef ob = calloc(1, sizeof(struct PDObject));
    ob->obid = obid;
    ob->genid = genid;
    ob->type = PDObjectTypeUnknown;
    ob->obclass = PDObjectClassRegular;
    ob->extractedLen = -1;
    return ob;
}

PDObjectRef PDObjectCreateFromDefinitionsStack(PDInteger obid, pd_stack defs)
{
    PDObjectRef ob = PDObjectCreate(obid, 0);
    ob->def = defs;
    return ob;
}

void PDObjectSetSynchronizationCallback(PDObjectRef object, PDSynchronizer callback, const void *syncInfo)
{
    object->synchronizer = callback;
    object->syncInfo = syncInfo;
}

void PDObjectDelete(PDObjectRef object)
{
    if (object->obclass != PDObjectClassCompressed) {
        object->skipObject = object->deleteObject = true;
    } else {
        PDError("Objects inside of object streams cannot be deleted");
    }
}

void PDObjectUndelete(PDObjectRef object)
{
    if (object->obclass != PDObjectClassCompressed) {
        object->skipObject = object->deleteObject = false;
    }
}

PDInteger PDObjectGetObID(PDObjectRef object)
{
    return object->obid;
}

PDInteger PDObjectGetGenID(PDObjectRef object)
{
    return object->genid;
}

PDBool PDObjectGetObStreamFlag(PDObjectRef object)
{
    return object->obclass == PDObjectClassCompressed;
}

PDObjectType PDObjectGetType(PDObjectRef object)
{
    return object->type;
}

void PDObjectSetType(PDObjectRef object, PDObjectType type)
{
    object->type = type;
}

PDObjectType PDObjectDetermineType(PDObjectRef object)
{
    if (object->inst) {
        switch (PDResolve(object->inst)) {
            case PDInstanceTypeString:  object->type = PDObjectTypeString; break;
            case PDInstanceTypeRef:     object->type = PDObjectTypeReference; break;
            case PDInstanceTypeArray:   object->type = PDObjectTypeArray; break;
            case PDInstanceTypeDict:    object->type = PDObjectTypeDictionary; break;
            case PDInstanceTypeNumber:  object->type = ((PDNumberRef)object->inst)->type; break;
            case PDInstanceTypeObj:     object->type = PDObjectTypeReference; break;
            default:                    object->type = PDObjectTypeUnknown; break;
        }
        if (object->type != PDObjectTypeUnknown) return object->type;
    }
    
    pd_stack st = object->def;
    if (st == NULL) return object->type;
    
    PDID stid = st->info;
    if (PDIdentifies(stid, PD_ARRAY))
        object->type = PDObjectTypeArray;
    else if (PDIdentifies(stid, PD_DICT))
        object->type = PDObjectTypeDictionary;
    else if (PDIdentifies(stid, PD_NAME))
        object->type = PDObjectTypeString; // todo: make use of more types
    else if (PDIdentifies(stid, PD_STRING))
        object->type = PDObjectTypeString;
    else if (PDIdentifies(stid, PD_NUMBER)) 
        object->type = PDObjectTypeUnknown; // we keep it at unknown for now, as numbers can be ints, reals, bools, and nulls
    else 
        PDError("unable to determine object type");
    return object->type;
}

const char *PDObjectGetReferenceString(PDObjectRef object)
{
    if (object->refString)
        return object->refString;
    
    char refbuf[64];
    sprintf(refbuf, "%ld %ld R", object->obid, object->genid);
    object->refString = strdup(refbuf);
    return object->refString;
}

PDBool PDObjectHasStream(PDObjectRef object)
{
    return object->hasStream;
}

PDInteger PDObjectGetRawStreamLength(PDObjectRef object)
{
    return object->streamLen;
}

PDInteger PDObjectGetExtractedStreamLength(PDObjectRef object)
{
    PDAssert(object->extractedLen != -1);
    return object->extractedLen;
}

PDBool PDObjectHasTextStream(PDObjectRef object)
{
    PDAssert(object->extractedLen != -1);
    if (object->extractedLen == 0) return false;
    
    PDInteger ix = object->extractedLen > 10 ? 10 : object->extractedLen - 1;
    PDInteger matches = 0;
    PDInteger thresh = ix * 0.8f;
    char ch;
//    if (object->streamBuf[object->extractedLen-1] != 0) 
//        return false;
    for (PDInteger i = 0; matches < thresh && i < ix; i++) {
        ch = object->streamBuf[i];
        matches += ((ch >= 'a' && ch <= 'z') || 
                    (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') ||
                    (ch >= 32 && ch <= 126) ||
                    ch == '\n' || ch == '\r');
    }
    return matches >= thresh;
}

char *PDObjectGetStream(PDObjectRef object)
{
    PDAssert(object->extractedLen != -1);
    return object->streamBuf;
}

void *PDObjectGetValue(PDObjectRef object)
{
    if (object->inst) return object->inst;
    pd_stack def = object->def;
    if (object->def) object->inst = PDInstanceCreateFromComplex(&def);
    return object->inst ? object->inst : object->def;
}

void PDObjectSetValue(PDObjectRef object, void *value)
{
    PDRelease(object->inst);
    object->inst = PDRetain(value);
    PDObjectDetermineType(object);
//    if (object->type == PDObjectTypeUnknown) object->type = PDObjectTypeString;
//    PDAssert(object->type == PDObjectTypeString);
//    if (object->def) 
//        free(object->def);
//    object->def = strdup(value);
}

#ifdef PD_SUPPORT_CRYPTO
PDCryptoInstanceRef PDObjectGetCryptoInstance(PDObjectRef object)
{
    if (object->crypto && ! object->cryptoInstance)
        object->cryptoInstance = PDCryptoInstanceCreate(object->crypto, object->obid, object->genid);
    return object->cryptoInstance;
}
#endif

void PDObjectInstantiate(PDObjectRef object)
{
    pd_stack s = object->def;
    object->inst = PDInstanceCreateFromComplex(&s);
    if (object->type == PDObjectTypeUnknown) {
        PDObjectDetermineType(object);
    }
#ifdef PD_SUPPORT_CRYPTO
    if (object->crypto && object->inst) {
        (*PDInstanceCryptoExchanges[PDResolve(object->inst)])(object->inst, PDObjectGetCryptoInstance(object), true);
    }
#endif
}

PDDictionaryRef PDObjectGetDictionary(PDObjectRef object)
{
    if (NULL == object->inst) {
        PDObjectInstantiate(object);
        if (NULL == object->inst) {
            object->type = PDObjectTypeDictionary;
            object->inst = PDDictionaryCreate();
        }
    }
    if (object->type == PDObjectTypeUnknown) 
        PDObjectDetermineType(object);
#ifdef DEBUG
    if (object->type != PDObjectTypeDictionary) {
        if (object->inst)
            PDNotice("request for object dictionary (type = %d) but object type = %d", PDInstanceTypeDict, PDResolve(object->inst));
        else
            PDNotice("request for object dictionary but inst is null");
    }
#endif
    return object->type == PDObjectTypeDictionary ? object->inst : NULL;
}

PDArrayRef PDObjectGetArray(PDObjectRef object)
{
    if (NULL == object->inst) {
        PDObjectInstantiate(object);
        if (NULL == object->inst) {
            object->type = PDObjectTypeArray;
            object->inst = PDArrayCreateWithCapacity(3);
        }
    }
    if (object->type == PDObjectTypeUnknown) 
        PDObjectDetermineType(object);
    return object->type == PDObjectTypeArray ? object->inst : NULL;
}

void PDObjectReplaceWithString(PDObjectRef object, char *str, PDInteger len)
{
    object->ovrDef = str;
    object->ovrDefLen = len;
}

void PDObjectSkipStream(PDObjectRef object)
{
    object->skipStream = true;
}

void PDObjectSetStream(PDObjectRef object, char *str, PDInteger len, PDBool includeLength, PDBool allocated, PDBool encrypted)
{
#ifdef PD_SUPPORT_CRYPTO
    if (! encrypted && object->crypto) {
        PDObjectGetCryptoInstance(object);
        if (!allocated) {
            char *res = malloc(len+1);
            memcpy(res, str, len);
            str = res;
            allocated = true;
        }
        pd_crypto_convert(object->crypto, object->obid, object->genid, str, len);
    }
#endif
    
    if (object->ovrStreamAlloc) {
        free(object->ovrStream);
    }
    object->ovrStream = str;
    object->ovrStreamLen = len;
    object->ovrStreamAlloc = allocated;
    if (includeLength)  {
//        char *lenstr = malloc(30);
//        sprintf(lenstr, "%ld", len);
        PDDictionarySet(PDObjectGetDictionary(object), "Length", PDNumberWithInteger(len));
//        PDDictionarySet(PDObjectGetDictionary(object), "Length", lenstr);
//        free(lenstr);
    }
}

PDBool PDObjectSetStreamFiltered(PDObjectRef object, char *str, PDInteger len, PDBool encrypted)
{
    // Need to get /Filter and /DecodeParms
    PDDictionaryRef obdict = PDObjectGetDictionary(object);
    PDStringRef filter = PDDictionaryGetString(obdict, "Filter");
    
    if (NULL == filter) {
        // no filter
        PDObjectSetStream(object, str, len, true, false, encrypted);
        return true;
    } 

    PDDictionaryRef decodeParms = PDDictionaryGetDictionary(obdict, "DecodeParms");
    
    PDBool success = true;
    PDStreamFilterRef sf = PDStreamFilterObtain(PDStringEscapedValue(filter, false), false, decodeParms);
    if (NULL == sf) {
        // we don't support this filter; that means we've been handed the filtered value, because we were not able to extract it either, so we can pass it over to PDObjectSetStream
        PDObjectSetStream(object, str, len, true, false, true);
        return true;
    } 
    
    if (success) success = PDStreamFilterInit(sf);
    // if !success, filter did not initialize properly

    success &= sf->compatible;
    // if !success, filter was not compatible with options

    char *filtered = NULL;
    PDInteger flen = 0;
    if (success) success = PDStreamFilterApply(sf, (unsigned char *)str, (unsigned char **)&filtered, len, &flen, NULL);
    // if !success, filter did not apply to input data successfully

    PDRelease(sf);
    
    if (success) PDObjectSetStream(object, filtered, flen, true, true, encrypted);
    
    return success;
}

void PDObjectSetFlateDecodedFlag(PDObjectRef object, PDBool state)
{
    if (object->inst == NULL) PDObjectGetDictionary(object);
    
    if (state) {
        PDDictionarySet(object->inst, "Filter", PDAutorelease(PDStringCreateWithName(strdup("/FlateDecode"))));
    } else {
        PDDictionaryDelete(object->inst, "Filter");
        PDDictionaryDelete(object->inst, "DecodeParms");
    }
}

void PDObjectSetPredictionStrategy(PDObjectRef object, PDPredictorType strategy, PDInteger columns)
{
    if (object->inst == NULL) PDObjectGetDictionary(object);
    
    PDDictionaryRef pred = PDDictionaryCreateWithBucketCount(2);
    PDDictionarySet(pred, "Predictor", PDNumberWithInteger(strategy));
    PDDictionarySet(pred, "Columns", PDNumberWithInteger(columns));
    PDDictionarySet(object->inst, "DecodeParms", pred);
    PDRelease(pred);
}

void PDObjectSetStreamEncrypted(PDObjectRef object, PDBool encrypted)
{
    if (object->inst == NULL) PDObjectGetDictionary(object);
    
    if (object->encryptedDoc) {
        if (encrypted) {
            // we don't support encryption except in that we undo our no-encryption stuff below
            // the above isn't strictly the case anymore, but we still don't support encrypting unepcrypted PDFs, or changing their encryption parameters
            PDDictionaryDelete(object->inst, "Filter");
            PDDictionaryDelete(object->inst, "DecodeParms");
        } else {
            PDArrayRef filterArray = PDArrayCreateWithCapacity(1);
            PDArrayAppend(filterArray, PDStringWithName(strdup("/Crypt")));
            PDDictionarySet(object->inst, "Filter", filterArray);
            PDRelease(filterArray);
            
            PDDictionaryRef decodeParms = PDDictionaryCreate();
            PDDictionarySet(decodeParms, "Type", PDStringWithName(strdup("/CryptFilterDecodeParms")));
            PDDictionarySet(decodeParms, "Name", PDStringWithName(strdup("/Identity")));
            PDDictionarySet(object->inst, "DecodeParms", decodeParms);
            PDRelease(decodeParms);
        }
    }
}

PDInteger PDObjectGenerateDefinition(PDObjectRef object, char **dstBuf, PDInteger capacity)
{
    struct PDStringConv sscv = (struct PDStringConv) {*dstBuf, 0, capacity};
    PDStringConvRef scv = &sscv;
    
    // we don't even want to start off with < 50 b
    PDStringGrow(50);
    
    pd_stack stack;
    PDInteger i;
    const char *cval;
    char *val;
    PDInteger sz;
    switch (object->obclass) {
        case PDObjectClassRegular:
            putfmt("%ld %ld obj\n", object->obid, object->genid);
            break;
        case PDObjectClassCompressed:
            break;
        case PDObjectClassTrailer:
            putstr("trailer\n", 8);
        default:
            PDAssert(0); // crash = undefined class which at a 99.9% accuracy means memory was trashed because that should never ever happen
    }
    switch (object->type) {
        case PDObjectTypeDictionary:
            if (object->inst == NULL) {
                PDObjectGetDictionary(object);
            }
            
            val = PDDictionaryToString(object->inst);
            
//            if (NULL == object->dict) {
//                // no dict probably means we can to-string but this needs to be tested; for now we instantiate dict and use that
//                object->dict = pd_dict_from_pdf_dict_stack(object->def);
//            } 
//            
//            val = pd_dict_to_string(object->dict);
            i = strlen(val);
            PDStringGrow(i + i);
            putstr(val, i);
            free(val);
            break;
            
        case PDObjectTypeArray:
            if (object->inst == NULL) {
                PDObjectGetArray(object);
            }
            
            val = PDArrayToString(object->inst);
            i = strlen(val);
            PDStringGrow(1 + i);
            putstr(val, i);
            free(val);
            break;
            
        case PDObjectTypeString:
            if (object->inst == NULL) {
                PDObjectInstantiate(object);
            }
            PDAssert(object->inst); // crash = failed to instantiate the string representation; reasons are unknown but may be that def was a char*, the old way to represent strings
            cval = PDStringEscapedValue(object->inst, true);
            PDStringGrow(2 + strlen(cval));
            putfmt("%s", cval);
            break;
            
        case PDObjectTypeNull:
        case PDObjectTypeInteger:
        case PDObjectTypeBoolean:
        case PDObjectTypeReal:
            if (object->inst == NULL) {
                PDObjectInstantiate(object);
            }
            PDAssert(object->inst); // crash = failed to instantiate the string representation; reasons are unknown but may be that def was a char*, the old way to represent strings
            val = PDNumberToString(object->inst);
            i = strlen(val);
            PDStringGrow(1 + i);
            putstr(val, i);
            free(val);
            break;
            
        default:
            pd_stack_set_global_preserve_flag(true);
            stack = object->def;
            PDAssert(stack); // crash = an object was appended or created, but it was not set to any value; this is not legal in PDFs; objects must contain something
            val = PDStringFromComplex(&stack);
            pd_stack_set_global_preserve_flag(false);
            PDStringGrow(1 + strlen(val));
            putfmt("%s", val);
            break;
            
    }
    
    PDStringGrow(2);
    currchi = '\n';
    currch = 0;
    
    *dstBuf = scv->allocBuf;

    return scv->offs;
}

PDInteger PDObjectPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap)
{
    PDInstancePrinterInit(PDObjectRef, 12, 50);
    char *bv = *buf;
    return offs + sprintf(&bv[offs], "%ld %ld R", i->obid, i->genid);;
}
