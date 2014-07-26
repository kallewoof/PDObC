//
// PDArray.h
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
 @file PDArray.h Array object
 
 @ingroup PDARRAY
 
 @defgroup PDARRAY PDArray
 
 @brief An array construct.

 PDArrays maintain a list of objects, such as PDStrings, PDReferences, PDObjects, etc. and are able to produce a string representation compatible with the PDF specification for their corresponding content.
 
 @{
 */

#ifndef INCLUDED_PDARRAY_H
#define INCLUDED_PDARRAY_H

#include "PDDefines.h"

/**
 *  Create a new, empty array with the given capacity.
 *
 *  @param capacity The number of slots to allocate for added elements
 *
 *  @return Empty array
 */
extern PDArrayRef PDArrayCreateWithCapacity(PDInteger capacity);

/**
 *  Converts a list of entries in the form of a stack into an array.
 *
 *  @note This simply takes each element as is. To convert a PDF array stack representation into a PDArray, use PDArrayCreateWithComplex().
 *
 *  @param stack The stack
 *
 *  @return New PDArray based on the stack
 */
extern PDArrayRef PDArrayCreateWithStackList(pd_stack stack);

/**
 *  Converts a PDF array stack into an array.
 *  
 *  @param stack The PDF array stack
 *
 *  @return New PDArray based on the stack representation of a PDF array
 */
extern PDArrayRef PDArrayCreateWithComplex(pd_stack stack);

///**
// *  Create a copy of the given array.
// *
// *  @param array The array to copy
// *
// *  @return A new array, identical to array
// */
//extern PDArrayRef PDArrayCopy(PDArrayRef array);

/**
 *  Clear the given array, removing all entries.
 *
 *  @param array The array
 */
extern void PDArrayClear(PDArrayRef array);

/**
 *  Get the number of elements in the array
 *
 *  @param array The array
 *
 *  @return Number of elements
 */
extern PDInteger PDArrayGetCount(PDArrayRef array);

/**
 *  Return the element at the given index.
 *
 *  @param array The array
 *  @param index The index of the element to retrieve
 *
 *  @return An appropriate object.
 */
extern void *PDArrayGetElement(PDArrayRef array, PDInteger index);

/**
 *  Return the element at the given index, provided that it's of the given type. Otherwise return NULL.
 *
 *  @param array The array
 *  @param index The index of the element to retrieve
 *  @param type  The required type
 *
 *  @return Instance of given instance type, or NULL if the instance type does not match the encountered value
 */
extern void *PDArrayGetTypedElement(PDArrayRef array, PDInteger index, PDInstanceType type);

//#define PDArrayGetNumber(a,i)      PDArrayGetTypedElement(a,i,PDInstanceTypeNumber)
#define PDArrayGetString(a,i)      PDArrayGetTypedElement(a,i,PDInstanceTypeString)
#define PDArrayGetArray(a,i)       PDArrayGetTypedElement(a,i,PDInstanceTypeArray)
#define PDArrayGetDictionary(a,i)  PDArrayGetTypedElement(a,i,PDInstanceTypeDict)
#define PDArrayGetReference(a,i)   PDArrayGetTypedElement(a,i,PDInstanceTypeRef)
#define PDArrayGetObject(a,i)      PDArrayGetTypedElement(a,i,PDInstanceTypeObj)

#define PDArrayGetInteger(a,i)     PDNumberGetInteger(PDArrayGetElement(a,i))

/**
 *  Append value to the end of the array.
 *
 *  @param array The array
 *  @param value The value
 */
extern void PDArrayAppend(PDArrayRef array, void *value);

/**
 *  Insert value at the given index in the array.
 *
 *  @param array The array
 *  @param index The index at which to insert the value
 *  @param value The value
 */
extern void PDArrayInsertAtIndex(PDArrayRef array, PDInteger index, void *value);

/**
 *  Get the index of the given value in the array, or -1 if the value was not found.
 *
 *  @param array The array
 *  @param value The value to search for
 *
 *  @return Index, or -1 if not found
 */
extern PDInteger PDArrayGetIndex(PDArrayRef array, void *value);

/**
 *  Delete the element at the given index.
 *
 *  @param array The array
 *  @param index The index of the element to delete
 */
extern void PDArrayDeleteAtIndex(PDArrayRef array, PDInteger index);

/**
 *  Replace the element at the given index with value.
 *
 *  @param array The array
 *  @param index The index whose value should be overwritten
 *  @param value The new value
 */
extern void PDArrayReplaceAtIndex(PDArrayRef array, PDInteger index, void *value);

/**
 *  Generate a C string formatted according to the PDF specification for this array.
 *
 *  @note The returned string must be freed.
 *
 *  @param array The array
 *
 *  @return C string representation of array
 */
extern char *PDArrayToString(PDArrayRef array);

extern PDInteger PDArrayPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap);

extern void PDArrayPrint(PDArrayRef array);

#define encryptable(str) (strlen(str) > 0 && str[0] == '(' && str[strlen(str)-1] == ')')

#ifdef PD_SUPPORT_CRYPTO

/**
 *  Supply a crypto object to an array, and associate the array with a specific object. 
 *
 *  @param array     The array
 *  @param crypto    The pd_crypt object
 *  @param objectID  The object ID of the owning object
 *  @param genNumber Generation number of the owning object
 */
extern void PDArrayAttachCrypto(PDArrayRef array, pd_crypto crypto, PDInteger objectID, PDInteger genNumber);

#endif // PD_SUPPORT_CRYPTO

#endif // INCLUDED_PDARRAY_H

/** @} */
