//
// PDString.h
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
 @file PDString.h String wrapper
 
 @ingroup PDSTRING
 
 @defgroup PDSTRING PDString
 
 @brief A wrapper around PDF strings.

 PDString objects exist to provide a unified way to maintain and convert between different string types. In particular, it is responsible for providing a means to escape and unescape content, as well as convert to and from hex strings (written as <abc123> in PDFs).
 
 @{
 */

#ifndef INCLUDED_PDSTRING_H
#define INCLUDED_PDSTRING_H

#include "PDDefines.h"

/**
 *  Create a PDString from an existing, escaped string. 
 *
 *  @note Ownership of the string is taken, and the string is freed when the PDString object is released.
 *  @note Use PDStringCreateBinary for strings that need to be escaped, even if they're proper NUL-terminated strings.
 *
 *  @param string NUL-terminated, escaped string optionally wrapped in parentheses.
 *
 *  @return New PDString instance for string
 */
extern PDStringRef PDStringCreate(char *string);

/**
 *  Create a PDString from an existing, unescaped string by escaping the string and keeping the resulting value.
 *
 *  @param unescapedString A string that needs to be, but is not currently, escaped.
 *
 *  @return New PDString instance for escaped variant of unescapedString
 */
extern PDStringRef PDStringCreateUnescaped(char *unescapedString);

/**
 *  Create a PDString from an existing, escaped name. Names differ from regular strings in one single way: they have a slash prefix. Beyond that, they can be wrapped or non-wrapped, just like normal.
 *
 *  @param name Name string, with or without prefix slash, with or without wrapping.
 *
 *  @return New PDString instance for name
 */
extern PDStringRef PDStringCreateWithName(char *name);

/**
 *  Create a PDString from an existing, unescaped or binary string of the given length.
 *
 *  @note Ownership of the data is taken, and the data is freed when the PDString object is released.
 *
 *  @param data   Data containing binary string
 *  @param length Length of the data, in bytes
 *
 *  @return New PDString instance for data
 */
extern PDStringRef PDStringCreateBinary(char *data, PDSize length);

/**
 *  Create a PDString from an existing hex string.
 *
 *  @note Ownership of the data is taken, and the data is freed when the PDString object is released.
 *
 *  @param hex Hex string, optionally wrapped in less/greater-than signs.
 *
 *  @return New PDString instance for hex
 */
extern PDStringRef PDStringCreateWithHexString(char *hex);

/**
 *  Create a PDString from an existing PDString instance, with an explicit type.
 *  In effect, this creates a new PDString object whose type matches the given type. 
 *  This method is recommended for converting between types, unless the C string value is explicitly desired.
 *
 *  @note If string is already of the given type, it is retained and returned as is.
 *
 *  @param string PDString instance used in transformation
 *  @param type   String type to transform to
 *  @param wrap   Whether the string should be wrapped ("()" for escaped, "<>" for hex)
 *
 *  @return A PDString whose value is identical to that of string, and whose type is the given type
 */
extern PDStringRef PDStringCreateFromStringWithType(PDStringRef string, PDStringType type, PDBool wrap);

/**
 *  Force the wrapped state of a string. 
 *  Normally, when a regular string is set up, its 'wrapped' flag is set to true if the string is equal
 *  to "(whatever)", but in some rare instances, the wrapped string is "((value))", which unwrapped 
 *  appears as "(value)", but is interpreted as "value"; wrapped = true.  
 *
 *  @param string  PDString
 *  @param wrapped The wrapped state of the PDString
 */
extern void PDStringForceWrappedState(PDStringRef string, PDBool wrapped);

/**
 *  Short hand for creating a binary string from an existing PDStringRef
 *
 *  @param string String whose binary form is desired
 *
 *  @return Binary PDString object
 */
#define PDStringCreateBinaryFromString(string) PDStringCreateFromStringWithType(string, PDStringTypeBinary, false)

//#define PDStringWithUnescapedCString(unescapedCString) PDAutorelease(PDStringCreateBinary(unescapedCString, strlen(unescapedCString)))

#define PDStringEscapingCString(unescapedCString) PDAutorelease(PDStringCreateUnescaped(unescapedCString))

#define PDStringWithCString(cString) PDAutorelease(PDStringCreate(cString))

#define PDStringWithName(name) PDAutorelease(PDStringCreateWithName(name))

/**
 *  Generate a C string containing the escaped contents of string and return it. 
 *  If wrap is set, the string is wrapped in parentheses.
 *
 *  @note The returned string should not be freed.
 *  @note It is safe to pass a NULL string value. If string is NULL, NULL is returned.
 *
 *  @param string PDString instance
 *  @param wrap   Whether or not the returned string should be enclosed in parentheses
 *
 *  @return Escaped NUL-terminated C string.
 */
extern const char *PDStringEscapedValue(PDStringRef string, PDBool wrap);

/**
 *  Generate a C string containing the escaped contents of string as a name, i.e. with a forward slash preceding it.
 *  If wrap is set, the string (after forward slash) is wrapped in parentheses.
 *
 *  @note The returned string should not be freed.
 *  @note It is safe to pass a NULL string value. If string is NULL, NULL is returned.
 *
 *  @param string PDString instance
 *  @param wrap   Whether or not the returned string should be enclosed in parentheses
 *
 *  @return Escaped NUL-terminated C string with a forward slash prefix.
 */
extern const char *PDStringNameValue(PDStringRef string, PDBool wrap);

/**
 *  Generate the binary value of string, writing its length to the PDSize pointed to by outLength and returning the 
 *  binary value.
 *
 *  @note The returned string should not be freed.
 *  @note It is safe to pass a NULL string value. If string is NULL, NULL is returned.
 *
 *  @param string    PDString instance
 *  @param outLength Pointer to PDSize object into which the length of the returned binary data is to be written. May be NULL.
 *
 *  @return C string pointer to binary data.
 */
extern const char *PDStringBinaryValue(PDStringRef string, PDSize *outLength);

/**
 *  Generate a hex string based on the value of string, returning it.
 *
 *  @note The returned string should not be freed.
 *  @note It is safe to pass a NULL string value. If string is NULL, NULL is returned.
 *
 *  @param string PDString instance
 *  @param wrap   Whether or not the returned string should be enclosed in less/greater-than signs
 *
 *  @return Hex string.
 */
extern const char *PDStringHexValue(PDStringRef string, PDBool wrap);

extern PDBool PDStringEqualsCString(PDStringRef string, const char *cString);

extern PDBool PDStringEqualsString(PDStringRef string, PDStringRef string2);

extern PDBool PDStringIsWrapped(PDStringRef string);

extern PDStringType PDStringGetType(PDStringRef string);

#ifdef PD_SUPPORT_CRYPTO

/**
 *  Determine if the given string is currently encrypted or not.
 *
 *  @param string String to check
 *
 *  @return true if encrypted, false if not
 */
extern PDBool PDStringIsEncrypted(PDStringRef string);

/**
 *  Attach a crypto object to the string, and associate the array with a specific object. 
 *  The encrypted flag is used to determine if the PDString is encrypted or not in its current state.
 *
 *  @param string    PDString instance whose crypto object is to be set
 *  @param crypto    pd_crypto object
 *  @param objectID  The object ID of the owning object
 *  @param genNumber Generation number of the owning object
 *  @param encrypted Whether string is encrypted or not, currently
 */
extern void PDStringAttachCrypto(PDStringRef string, pd_crypto crypto, PDInteger objectID, PDInteger genNumber, PDBool encrypted);

/**
 *  Create a PDString by encrypting string.
 *
 *  @note If string is already encrypted, or if no crypto has been attached, the string is retained and returned as is.
 *
 *  @param string PDString instance to encrypt
 *
 *  @return A retained encrypted PDStringRef for string
 */
extern PDStringRef PDStringCreateEncrypted(PDStringRef string);

/**
 *  Create a PDString by decrypting string.
 *
 *  @note If string is not encrypted, it is retained and returned as is.
 *
 *  @param string PDString instance
 *
 *  @return A retained decrypted PDStringRef for string
 */
extern PDStringRef PDStringCreateDecrypted(PDStringRef string);

extern PDInteger PDStringPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap);

#endif // PD_SUPPORT_CRYPTO

#endif // INCLUDED_PDSTRING_H

/** @} */
