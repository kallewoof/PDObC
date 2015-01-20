//
// pd_crypto.h
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


/**
 @file pd_crypto.h Cryptography header file.
 
 @ingroup pd_crypto
 
 @defgroup pd_crypto pd_crypto
 
 @brief Cryptography related functionality to deal with encrypted PDF content.
 
 @ingroup PDALGO
 
 The pd_crypto object.
 
 @{
 */

#ifndef INCLUDED_PD_CRYPTO_H
#define INCLUDED_PD_CRYPTO_H

#include <sys/types.h>
#include "PDDefines.h"

#ifdef PD_SUPPORT_CRYPTO

/**
 Create crypto object with given configuration. 
 
 @param trailerDict The pd_dict of the Trailer dictionary of the given PDF. Needed to obtain the /ID key.
 @param options The pd_dict of the Encrypt dictionary of the given PDF.
 @return Instance with given options or NULL if unsupported.
 */
extern pd_crypto pd_crypto_create(PDDictionaryRef trailerDict, PDDictionaryRef options);

/**
 Destroy crypto object, freeing up resources.
 */
extern void pd_crypto_destroy(pd_crypto crypto);

/**
 Unescape a PDF string which may optionally be wrapped in parentheses. The result is not wrapped in parentheses. The string is unescaped in-place and NUL-terminated.
 
 Strings, in particular encrypted strings, are stored using escaping to prevent null termination in the middle of strings and PDF misinterpretations and other nastiness.
 
 Escaping is done to control chars, such as \r, \n, \t, \b, and unreadable ascii characters using \octal(s) (1, 2 or 3).
 
 @param str The string.
 @return The length of the unescaped string.
 */
extern PDInteger pd_crypto_unescape(char *str);

/**
 Unescape a PDF string, explicitly defining the length of the string to allow for potential mid-NULs. May optionally be wrapped in parentheses. The result is not wrapped in parentheses. The string is unescaped in-place and NUL-terminated.
 
 Strings, in particular encrypted strings, are stored using escaping to prevent null termination in the middle of strings and PDF misinterpretations and other nastiness.
 
 Escaping is done to control chars, such as \r, \n, \t, \b, and unreadable ascii characters using \octal(s) (1, 2 or 3).
 
 @param str The string.
 @param len The length of the string;
 @return The length of the unescaped string.
 */
extern PDInteger pd_crypto_unescape_explicit_len(char *str, int len);

/**
 Escape a string. The result will be wrapped in parentheses.
 
 Strings, in particular encrypted strings, are stored using escaping to prevent null termination in the middle of strings and PDF misinterpretations and other nastiness.
 
 Escaping is done to control chars, such as \r, \n, \t, \b, and unreadable ascii characters using \octal(s) (1, 2 or 3).
 
 @param dst Pointer to destination string. Should not be pre-allocated.
 @param src String to escape.
 @param srcLen Length of string.
 @return The length of the escaped string.
 */
extern PDInteger pd_crypto_escape(char **dst, const char *src, PDInteger srcLen);

/**
 Generate a secure string from a given input string.
 
 The input string may optionally be wrapped in parentheses. The resulting string will be, regardless of input string.
 
 This method is roughly the equivalent of doing
    len = pd_crypto_unescape_explicit_len(src, srcLen);
    pd_crypto_escape(dst, src, len);
 
 @param dst Pointer to destination string. Should not be pre-allocated.
 @param src String to escape.
 @param srcLen Length of string.
 @return The length of the escaped string.
 */
extern PDInteger pd_crypto_secure(char **dst, const char *src, PDInteger srcLen);

/**
 Supply a user password. 
 
 @param crypto The crypto object.
 @param password The user password.
 @return true if the password was valid, false if not
 */
extern PDBool pd_crypto_authenticate_user(pd_crypto crypto, const char *password);

/**
 Encrypt the value of src of length len and store the value in dst, escaped and parenthesized.
 
 @param crypto Crypto instance.
 @param obid The object ID of the object whose content is being encrypted.
 @param genid The generation number of the object whose content is being encrypted.
 @param dst Pointer to char buffer into which results will be stored. Should not be pre-allocated.
 @param src The data to encrypt. The content will be in-place encrypted but escaped results go into *dst.
 @param len Length of data in bytes.
 @return Length of encrypted string, including parentheses and escaping.
 */
extern PDInteger pd_crypto_encrypt(pd_crypto crypto, PDInteger obid, PDInteger genid, char **dst, char *src, PDInteger len);

/**
 Decrypt, unescape and NUL-terminate the value of data in-place.
 
 @param crypto Crypto instance.
 @param obid The object ID of the object whose content is being decrypted.
 @param genid The generation number of the object whose content is being decrypted.
 @param data The data to decrypt. The content will be in-place replaced with the new data.
 */
extern void pd_crypto_decrypt(pd_crypto crypto, PDInteger obid, PDInteger genid, char *data);

/**
 Convert data of length len owned by object obid with generation number genid to/from encrypted version.
 
 This is the "low level" function used by the above two methods to encrypt/decrypt content. This version does not 
 escape/unescape or add/remove parentheses, which the above ones do. This version is used directly for streams
 which aren't escaped.
 
 @param crypto Crypto instance.
 @param obid Object ID of owning object.
 @param genid Generation number of owning object.
 @param data Data to convert.
 @param len Length of data.
 */
extern void pd_crypto_convert(pd_crypto crypto, PDInteger obid, PDInteger genid, char *data, PDInteger len);

extern PDStringRef pd_crypto_get_filter(pd_crypto crypto);
extern PDStringRef pd_crypto_get_subfilter(pd_crypto crypto);
extern PDInteger pd_crypto_get_version(pd_crypto crypto);
extern PDInteger pd_crypto_get_revision(pd_crypto crypto);
extern PDBool pd_crypto_get_encrypt_metadata_bool(pd_crypto crypto);
extern pd_crypto_method pd_crypto_get_method(pd_crypto crypto);
extern pd_auth_event pd_crypto_get_auth_event(pd_crypto crypto);

#endif

#endif

/** @} */
