//
// PDDictionary.h
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

/**
 @file PDDictionary.h Dictionary object
 
 @ingroup PDDICTIONARY
 
 @defgroup PDDICTIONARY PDDictionary
 
 @brief A dictionary construct.
 
 PDDictionaries maintain a list of objects, such as PDStrings, PDReferences, PDObjects, as associated key/value pairs. It is able to produce a string representation compatible with the PDF specification for its content.
 
 @{
 */

#ifndef INCLUDED_PDDICTIONARY_H
#define INCLUDED_PDDICTIONARY_H

#include "PDDefines.h"

/**
 *  Create a new, empty dictionary with the given capacity.
 *
 *  @param capacity The number of slots to allocate for added entrys
 *
 *  @return Empty dictionary
 */
extern PDDictionaryRef PDDictionaryCreateWithCapacity(PDInteger capacity);

/**
 *  Converts a PDF dictionary stack into a dictionary.
 *  
 *  @param stack The PDF dictionary stack
 *
 *  @return New PDDictionary based on the stack representation of a PDF dictionary
 */
extern PDDictionaryRef PDDictionaryCreateWithComplex(pd_stack stack);

/**
 *  Clear the given dictionary, removing all entries.
 *
 *  @param dict The dictionary
 */
extern void PDDictionaryClear(PDDictionaryRef dictionary);

/**
 *  Get the number of entrys in the dictionary
 *
 *  @param dictionary The dictionary
 *
 *  @return Number of entrys
 */
extern PDInteger PDDictionaryGetCount(PDDictionaryRef dictionary);

/**
 *  Get all the keys in the dictionary, as char**.
 *
 *  @param dictionary The dictionary
 *
 *  @return Char* array
 */
extern char **PDDictionaryGetKeys(PDDictionaryRef dictionary);

/**
 *  Return the entry at the given index.
 *
 *  @param dictionary The dictionary
 *  @param key        The key
 *
 *  @return Appropriate object.
 */
extern void *PDDictionaryGetEntry(PDDictionaryRef dictionary, const char *key);

/**
 *  Return the entry at the given index, provided that it's of the given type. Otherwise return NULL.
 *
 *  @param dictionary The dictionary
 *  @param key        The key
 *  @param type       The required type
 *
 *  @return Instance of given instance type, or NULL if the instance type does not match the encountered value
 */
extern void *PDDictionaryGetTypedEntry(PDDictionaryRef dictionary, const char *key, PDInstanceType type);

//#define PDDictionaryGetNumber(d,k)      PDDictionaryGetTypedEntry(d,k,PDInstanceTypeNumber)
#define PDDictionaryGetString(d,k)      PDDictionaryGetTypedEntry(d,k,PDInstanceTypeString)
#define PDDictionaryGetArray(d,k)       PDDictionaryGetTypedEntry(d,k,PDInstanceTypeArray)
#define PDDictionaryGetDictionary(d,k)  PDDictionaryGetTypedEntry(d,k,PDInstanceTypeDict)
#define PDDictionaryGetReference(d,k)   PDDictionaryGetTypedEntry(d,k,PDInstanceTypeRef)
#define PDDictionaryGetObject(d,k)      PDDictionaryGetTypedEntry(d,k,PDInstanceTypeObj)

#define PDDictionaryGetInteger(d,k)     PDNumberGetInteger(PDDictionaryGetEntry(d,k))

/**
 *  Set value of for the given key in the dictionary.
 *
 *  @param dictionary The dictionary
 *  @param key        The key
 *  @param value      The value
 */
extern void PDDictionarySetEntry(PDDictionaryRef dictionary, const char *key, void *value);

/**
 *  Delete the value for the given key.
 *
 *  @param dictionary The dictionary
 *  @param key        The key
 */
extern void PDDictionaryDeleteEntry(PDDictionaryRef dictionary, const char *key);

/**
 *  Generate a C string formatted according to the PDF specification for this dictionary.
 *
 *  @note The returned string must be freed.
 *
 *  @param dictionary The dictionary
 *
 *  @return C string representation of dictionary
 */
extern char *PDDictionaryToString(PDDictionaryRef dictionary);

extern PDInteger PDDictionaryPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap);

extern void PDDictionaryPrint(PDDictionaryRef dictionary);

#ifdef PD_SUPPORT_CRYPTO

/**
 *  Supply a crypto object to an dictionary, and associate the dictionary with a specific object. 
 *
 *  @param dictionary The dictionary
 *  @param crypto     The pd_crypt object
 *  @param objectID   The object ID of the owning object
 *  @param genNumber  Generation number of the owning object
 */
extern void PDDictionaryAttachCrypto(PDDictionaryRef dictionary, pd_crypto crypto, PDInteger objectID, PDInteger genNumber);

#endif // PD_SUPPORT_CRYPTO

#endif // INCLUDED_PDDICTIONARY_H

/** @} */
