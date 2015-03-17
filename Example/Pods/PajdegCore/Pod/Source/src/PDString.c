//
// PDString.c
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
#include "PDString.h"
#include "pd_stack.h"
#include "pd_crypto.h"
#include "PDNumber.h"

#include "pd_internal.h"

// Private declarations

char *PDStringTransform(const char *string, PDSize len, PDBool hasPrefix, PDBool hasWrapping, char addPrefix, char addLeft, char addRight);

//char *PDStringWrappedValue(const char *string, PDSize len, char prefix, char left, char right);     ///< "..."                  -> "L...R"
//char *PDStringUnwrappedValue(const char *string, PDSize len, PDBool hasPrefix, char prefix);        ///< "(...)", "<...>", .... -> "..."
// hex->*
char *PDStringHexToBinary(char *string, PDSize len, PDBool hasW, PDSize *outLength);                ///< "abc123"               -> 01101010
char *PDStringHexToEscaped(char *string, PDSize len, PDBool hasW, PDBool addW, char prefix);        ///< "abc123"               -> "foo\123"
// esc->*
char *PDStringEscapedToHex(char *string, PDSize len, PDBool hasW, PDBool addW);                     ///< "foo\123"              -> "abc123"
char *PDStringEscapedToBinary(char *string, PDSize len, PDBool hasW, PDSize *outLength);            ///< "foo\123"              -> 01101010
char *PDStringEscapedToName(char *string, PDSize len, PDBool hasW, PDBool addW);                    ///< "foo\123"              -> "/foo\123"
// bin->*
char *PDStringBinaryToEscaped(char *string, PDStringEncoding encoding, PDSize len, PDBool addW, char prefix, PDSize *outLength);
                                                                                                    ///<01101010               -> "foo\123"
char *PDStringBinaryToHex(char *string, PDSize len, PDBool addW);                                   ///< 01101010               -> "abc123"
// name->*
char *PDStringNameToEscaped(char *string, PDSize len, PDBool hasW, PDBool addW);                    ///< "/foo123"              -> "foo123"
char *PDStringNameToHex(char *string, PDSize len, PDBool hasW, PDBool addW);                        ///< "/foo123"              -> "abc123"
char *PDStringNameToBinary(char *string, PDSize len, PDBool hasW, PDSize *outLength);               ///< "/foo123"              -> "01101010"

// Defined in PDStringUTF.c
extern PDStringEncoding PDStringDetermineEncoding(PDStringRef string);

// Public

void PDStringDestroy(PDStringRef string)
{
#ifdef PD_SUPPORT_CRYPTO
    PDRelease(string->ci);
#endif
    PDRelease(string->alt);
    PDRelease(string->font);
    free(string->data);
}

#ifdef DEBUG
// This method exists to test the ownership of strings. It only catches accidental passes of constant strings.
void PDStringVerifyOwnership(char *string, PDSize len)
{
    char s = string[0];
    char e = string[len];
    string[0] = 0;
    string[len] = 0;
    string[0] = 255;
    string[len] = 255;
    string[0] = s;
    string[len] = e;
}
#endif

PDStringRef PDStringCreateL(char *string)
{
    return PDStringCreate(string, strlen(string));
}

PDStringRef PDStringCreate(char *string, PDSize length)
{
    if (length == 0) length = strlen(string);
#ifdef DEBUG
    PDStringVerifyOwnership(string, length);
#endif
    
    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->enc = PDStringEncodingDefault;
    res->data = string;
    res->alt = NULL;
    res->font = NULL;
    res->length = length;
    res->type = PDStringTypeEscaped;
    res->wrapped = (string[0] == '(' && string[res->length-1] == ')');
#ifdef PD_SUPPORT_CRYPTO
    res->ci = NULL;
#endif
    return res;
}

PDStringRef PDStringCreateUnescaped(char *unescapedString, PDSize length)
{
    if (length == 0) length = strlen(unescapedString);
#ifdef DEBUG
    PDStringVerifyOwnership(unescapedString, length);
#endif
    
    PDStringEncoding enc = PDStringEncodingUndefined;
    
    // PDF does not allow UTF-8 but instead requires UTF-16BE or plain ASCII.
    // we thus check the string for UTF-8 sequences, which is simply a check
    // if the highest bit is set; if it is, we generate a UTF-16BE string and
    // return that
    for (int j = 0; unescapedString[j]; j++) 
        if (unescapedString[j] & 0x80) {
            PDStringRef str = PDStringCreateBinary(unescapedString, strlen(unescapedString));
            if (PDStringDetermineEncoding(str) == PDStringEncodingUTF16BE) return str;
            PDStringRef ues = PDStringCreateUTF16Encoded(str);
            PDRelease(str);
            return ues;
        } else if (unescapedString[j] == 0) {
            // this is a UTF-16 sequence
            enc = PDStringEncodingUTF16BE;
        }
    
    char *str = PDStringBinaryToEscaped(unescapedString, enc, length, true, 0, &length);
    PDStringRef result = PDStringCreate(str, length);
    free(unescapedString);
    return result;
}

PDStringRef PDStringCreateWithName(char *name)
{
    PDSize len = strlen(name);
#ifdef DEBUG
    PDStringVerifyOwnership(name, len);
#endif
    
    if (name[0] != '/') {
        // names always include '/' in the data
        char *fixedName = malloc(len + 2);
        fixedName[0] = '/';
        strcpy(&fixedName[1], name);
        free(name);
        name = fixedName;
        len++;
    }
    
    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->enc = PDStringEncodingDefault;
    res->data = name;
    res->alt = NULL;
    res->font = NULL;
    res->length = len;
    res->type = PDStringTypeName;
    res->wrapped = res->length > 1 && (name[1] == '(' && name[res->length-1] == ')');
#ifdef PD_SUPPORT_CRYPTO
    res->ci = NULL;
#endif
    return res;
}

PDStringRef PDStringCreateBinary(char *data, PDSize length)
{
    if (length == 0) length = strlen(data);
#ifdef DEBUG
    PDStringVerifyOwnership(data, length);
#endif

    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->enc = PDStringEncodingDefault;
    res->data = data;
    res->alt = NULL;
    res->font = NULL;
    res->length = length;
    res->type = PDStringTypeBinary;
    res->wrapped = false;
#ifdef PD_SUPPORT_CRYPTO
    res->ci = NULL;
#endif
    return res;
}

PDStringRef PDStringCreateWithHexString(char *hex)
{
#ifdef DEBUG
    PDStringVerifyOwnership(hex, strlen(hex));
#endif
    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->enc = PDStringEncodingDefault;
    res->data = hex;
    res->alt = NULL;
    res->font = NULL;
    res->length = strlen(hex);
    res->type = PDStringTypeHex;
    res->wrapped = (hex[0] == '<' && hex[res->length-1] == '>');
#ifdef PD_SUPPORT_CRYPTO
    res->ci = NULL;
#endif
    return res;
}

PDStringRef PDStringCopy(PDStringRef string)
{
    char *data = malloc(string->length + 1);
    memcpy(data, string->data, string->length);
    data[string->length] = 0;
    
    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->enc = string->enc;
    res->data = data;
    res->alt = NULL; // we choose to NULL alt here, because copied strings are often modified internally, which necessitates the removal of the alt object
    res->font = PDRetain(string->font);
    res->length = string->length;
    res->type = string->type;
    res->wrapped = string->wrapped;
#ifdef PD_SUPPORT_CRYPTO
    res->ci = string->ci;
#endif
    return res;
}

PDStringRef PDStringCreateFromStringWithType(PDStringRef string, PDStringType type, PDBool wrap, PDBool requireCopy)
{
    if (string->type == type && (string->type == PDStringTypeBinary || string->wrapped == wrap)) {
        return requireCopy ? PDStringCopy(string) : PDRetain(string);
    }
    
    if (string->alt && string->alt->type == type && (string->alt->type == PDStringTypeBinary || string->alt->wrapped == wrap))
        return requireCopy ? PDStringCopy(string->alt) : PDRetain(string->alt);
    
    const char *res;
    char *buf;
    PDSize len;
    PDStringRef result;
    switch (type) {
        case PDStringTypeEscaped:
            res = PDStringEscapedValue(string, wrap, &len);
            buf = malloc(len + 1);
            memcpy(buf, res, len + 1);
            result = PDStringCreate(buf, len);
            break;
            
        case PDStringTypeName:
            result = PDStringCreateWithName(strdup(PDStringNameValue(string, wrap)));
            break;

        case PDStringTypeHex:
            result = PDStringCreateWithHexString(strdup(PDStringHexValue(string, wrap)));
            break;
            
        default:
            res = PDStringBinaryValue(string, &len);
            buf = malloc(len + 1);
            memcpy(buf, res, len + 1);
            result = PDStringCreateBinary(buf, len);
    }
    
#ifdef PD_SUPPORT_CRYPTO
    if (string->ci) PDStringAttachCryptoInstance(result, string->ci, string->encrypted);
#endif
    if (string->font) PDStringSetFont(result, string->font);
    return result;
}

void PDStringForceWrappedState(PDStringRef string, PDBool wrapped)
{
    PDAssert(string->type == PDStringTypeEscaped); // crash = attempt to set wrapped state for a string whose wrapping is never ambiguous (only regular/escaped strings are)
    string->wrapped = wrapped;
}

PDBool PDStringIsWrapped(PDStringRef string)
{
    return string->wrapped;
}

PDStringType PDStringGetType(PDStringRef string)
{
    return string->type;
}

PDStringEncoding PDStringGetEncoding(PDStringRef string)
{
    if (string->enc == PDStringEncodingDefault) PDStringDetermineEncoding(string);
    return string->enc;
}

const char *PDStringEscapedValue(PDStringRef string, PDBool wrap, PDSize *outLength)
{
    if (string == NULL) return NULL;
    
    if (PDResolve(string) == PDInstanceTypeNumber) {
        const char *numstr = PDNumberToString((PDNumberRef)string);
        if (outLength) *outLength = strlen(numstr);
        return numstr;
    }
    
    // see if we have what is asked for already
    if (string->type == PDStringTypeEscaped && string->wrapped == wrap) {
        if (outLength) *outLength = string->length;
        return string->data;
    } else if (string->alt && string->alt->type == PDStringTypeEscaped && string->alt->wrapped == wrap) {
        if (outLength) *outLength = string->alt->length;
        return string->alt->data;
    } 
    
    // we don't, so set up alternative; we use the PDString which offers the easiest conversion, which is
    //  escaped strings, then names, then binary strings, then hex strings
    PDStringRef source = ((string->alt && (string->alt->type == PDStringTypeEscaped || string->type == PDStringTypeHex || (string->alt->type == PDStringTypeName && string->type != PDStringTypeEscaped)))
                          ? string->alt
                          : string);
    
    char *data;
    if (source->type == PDStringTypeBinary) {
        data = PDStringBinaryToEscaped(source->data, PDStringDetermineEncoding(source), source->length, wrap, 0, outLength);
    } else if (source->type == PDStringTypeHex) {
        data = PDStringHexToEscaped(source->data, source->length, source->wrapped, wrap, 0);
        if (outLength) *outLength = strlen(data);
    } else {
        data = PDStringTransform(source->data, source->length, source->type == PDStringTypeName, source->wrapped, 0, wrap ? '(' : 0, wrap ? ')' : 0);
        if (outLength) *outLength = strlen(data);
    }
    
    PDRelease(string->alt);
    string->alt = PDStringCreate(data, strlen(data));
#ifdef PD_SUPPORT_CRYPTO
    if (string->ci) PDStringAttachCryptoInstance(string->alt, string->ci, string->encrypted);
#endif
    return data;
}

const char *PDStringNameValue(PDStringRef string, PDBool wrap)
{
    if (string == NULL) return NULL;
    
    // see if we have what is asked for already
    if (string->type == PDStringTypeName && string->wrapped == wrap) {
        return string->data;
    } else if (string->alt && string->alt->type == PDStringTypeName && string->alt->wrapped == wrap) {
        return string->alt->data;
    } 
    
    // we don't, so set up alternative; we use the PDString which offers the easiest conversion, which is
    //  names, escaped strings, then binary strings, then hex strings
    PDStringRef source = ((string->alt && (string->alt->type == PDStringTypeName || string->type == PDStringTypeHex || (string->alt->type == PDStringTypeEscaped && string->type != PDStringTypeName)))
                          ? string->alt
                          : string);
    
    char *data;
    if (source->type == PDStringTypeBinary)
        data = PDStringBinaryToEscaped(source->data, PDStringDetermineEncoding(source), source->length, wrap, '/', NULL);
    else if (source->type == PDStringTypeHex)
        data = PDStringHexToEscaped(source->data, source->length, source->wrapped, wrap, '/');
    else 
        data = PDStringTransform(source->data, source->length, source->type == PDStringTypeName, source->wrapped, '/', wrap ? '(' : 0, wrap ? ')' : 0);
//    else if (source->type == PDStringTypeEscaped) 
//        data = PDStringEscapedToName(source->data, source->length, source->wrapped, wrap);
//    else if (wrap) 
//        data = PDStringWrappedValue(source->data, source->length, '/', '(', ')');
//    else 
//        data = PDStringUnwrappedValue(source->data, source->length, source->type == PDStringTypeName, '/');
    
    PDRelease(string->alt);
    string->alt = PDStringCreateWithName(data);
#ifdef PD_SUPPORT_CRYPTO
    if (string->ci) PDStringAttachCryptoInstance(string->alt, string->ci, string->encrypted);
#endif
    return string->alt->data;
}

const char *PDStringPlainName(PDStringRef string)
{
    return string->type == PDStringTypeName ? &string->data[1] : string->data;
}

const char *PDStringBinaryValue(PDStringRef string, PDSize *outLength)
{
    if (string == NULL) return NULL;
    
    // see if we have what is asked for already
    if (string->type == PDStringTypeBinary) {
        if (outLength) *outLength = string->length;
        return string->data;
    } else if (string->alt && string->alt->type == PDStringTypeBinary) {
        if (outLength) *outLength = string->alt->length;
        return string->alt->data;
    } 
    
    // we don't, so set up alternative; we use the PDString which offers the easiest conversion, which is
    //  hex strings, then regular strings, then names
    PDStringRef source = ((string->alt && (string->alt->type == PDStringTypeHex || string->type == PDStringTypeName))
                          ? string->alt
                          : string);
    
    char *data;
    PDSize len;
    if (source->type == PDStringTypeEscaped)
        data = PDStringEscapedToBinary(source->data, source->length, source->wrapped, &len);
    else if (source->type == PDStringTypeName)
        data = PDStringNameToBinary(source->data, source->length, source->wrapped, &len);
    else
        data = PDStringHexToBinary(source->data, source->length, source->wrapped, &len);
    
    if (outLength) *outLength = len;
    
    PDRelease(string->alt);
    string->alt = PDStringCreateBinary(data, len);
#ifdef PD_SUPPORT_CRYPTO
    if (string->ci) PDStringAttachCryptoInstance(string->alt, string->ci, string->encrypted);
#endif
    return data;
}

const char *PDStringHexValue(PDStringRef string, PDBool wrap)
{
    if (string == NULL) return NULL;
    
    // see if we have what is asked for already
    if (string->type == PDStringTypeHex && string->wrapped == wrap) {
        return string->data;
    } else if (string->alt && string->alt->type == PDStringTypeHex && string->alt->wrapped == wrap) {
        return string->alt->data;
    } 
    
    // we don't, so set up alternative; we use the PDString which offers the easiest conversion, which is
    //  hex strings, then binary strings, then regular strings, then names
    PDStringRef source = ((string->alt && (string->alt->type == PDStringTypeHex || string->type == PDStringTypeName || (string->alt->type == PDStringTypeBinary && string->type != PDStringTypeHex)))
                          ? string->alt
                          : string);
    
    char *data;
    if (source->type == PDStringTypeBinary)
        data = PDStringBinaryToHex(source->data, source->length, wrap);
    else if (source->type == PDStringTypeEscaped)
        data = PDStringEscapedToHex(source->data, source->length, source->wrapped, wrap);
    else if (source->type == PDStringTypeName)
        data = PDStringNameToHex(source->data, source->length, source->wrapped, wrap);
    else
        data = PDStringTransform(source->data, source->length, false, source->wrapped, 0, wrap ? '<' : 0, wrap ? '>' : 0);
//        if (wrap) 
//        data = PDStringWrappedValue(source->data, source->length, 0, '<', '>');
//    else 
//        data = PDStringUnwrappedValue(source->data, source->length, false, 0);
    
    PDRelease(string->alt);
    string->alt = PDStringCreateWithHexString(data);
#ifdef PD_SUPPORT_CRYPTO
    if (string->ci) PDStringAttachCryptoInstance(string->alt, string->ci, string->encrypted);
#endif
    return data;
}

// Private

char *PDStringTransform(const char *string, PDSize len, PDBool hasPrefix, PDBool hasWrapping, char addPrefix, char addLeft, char addRight)
{
    PDAssert(hasPrefix == false || hasPrefix == true);
    PDAssert(hasWrapping == false || hasWrapping == true);
    // determine offset for copying from string
    PDSize copyoffs = hasPrefix + hasWrapping;
    // determine bytes to copy from string
    PDSize copysize = len - copyoffs - hasWrapping;
    // determine alloc requirement of new string; the requirement is the current length minus existing plr plus new plr (and 1 for NUL)
    PDSize allocreq = len + 1 - copyoffs - hasWrapping + (addPrefix!=0) + (addLeft!=0) + (addRight!=0);
    PDInteger index = 0;
    char *result = malloc(allocreq);
    result[index] = addPrefix;                              index += (addPrefix!=0);
    result[index] = addLeft;                                index += (addLeft!=0);
    strncpy(&result[index], &string[copyoffs], copysize);   index += copysize;
    result[index] = addRight;                               index += (addRight!=0);
    result[index] = 0;
    return result;
}

//char *PDStringWrappedValue(const char *string, PDSize len, char prefix, char left, char right)
//{
//    PDInteger index = prefix != 0;
//    char *res = malloc(len + index + 3);
//    res[0] = prefix;
//    res[index++] = left;
//    strcpy(&res[index], string);
//    res[len+1] = right;
//    res[len+2] = 0;    
//    return res;
//}
//
//char *PDStringUnwrappedValue(const char *string, PDSize len, PDBool hasPrefix, char prefix)
//{
//    if (prefix) {
//        char *unwrapped = malloc(len); // actual len+1 = NUL, actual len+2 = prefix char
//        unwrapped[0] = prefix;
//        strncpy(&unwrapped[1], &string[1+hasPrefix], len-2-hasPrefix);
//        return unwrapped;
//    } else return strndup(&string[1+hasPrefix], len-2-hasPrefix);
//}

char *PDStringHexToBinary(char *string, PDSize len, PDBool wrapped, PDSize *outLength)
{
    char *csr = string + (wrapped);
    PDSize ix = len - (wrapped<<1);
    PDSize rescap = 2 + ix/2; // optim: ix/2 -> (ix>>1)
    PDSize reslen = 0;
    char *res = malloc(rescap);
    PDSize i;
    
    for (i = 0; i < ix; i += 2) {
//        PDInteger a = PDOperatorSymbolGlobHex[csr[i]];
//        PDInteger b = PDOperatorSymbolGlobHex[csr[i+1]];
//        PDInteger c = (a << 4) + b;
//        PDInteger d = (PDOperatorSymbolGlobHex[csr[i]] << 4) + PDOperatorSymbolGlobHex[csr[i+1]];
        res[reslen++] = (PDOperatorSymbolGlobHex[(unsigned char)csr[i]] << 4) + PDOperatorSymbolGlobHex[(unsigned char)csr[i+1]];
        
        if (reslen == rescap) {
            // in theory we should never end up here; if we do, we just make sure we don't crash
            PDError("unexpectedly exhausted result cap in PDStringHexToBinary for input \"%s\"", string);
            rescap += 10 + (ix - i);
            res = realloc(res, rescap);
        }
    }
    
    res[reslen] = 0;
    *outLength = reslen;
    return res;
}

char *PDStringEscapedToBinary(char *string, PDSize len, PDBool wrapped, PDSize *outLength)
{
    const char *str = string + (wrapped);
    PDSize ix = len - (wrapped<<1);
    char *res = malloc(len+1);
    PDBool esc = false;
    PDSize si = 0;
    int escseq;
    for (int i = 0; i < ix; i++) {
        if (str[i] == '\\' && ! esc) {
            esc = true;
        } else {
            if (esc) {
                if (str[i] >= '0' && str[i] <= '9') {
                    res[si] = 0;
                    for (escseq = 0; escseq < 3 && str[i] >= '0' && str[i] <= '9'; escseq++, i++)
                        res[si] = (res[si] << 3) + (str[i] - '0');
                    i--;
                } else switch (str[i]) {
                    case '\n':
                    case '\r':
                        si--; // ignore newline by nulling the si++ below
                        break;
                    case 't': res[si] = '\t'; break;
                    case 'r': res[si] = '\r'; break;
                    case 'n': res[si] = '\n'; break;
                    case 'b': res[si] = '\b'; break;
                    case 'f': res[si] = '\f'; break;
                    case '0': res[si] = '\0'; break;
                    case 'a': res[si] = '\a'; break;
                    case 'v': res[si] = '\v'; break;
                    case 'e': res[si] = '\e'; break;
                        
                        // a number of things are simply escaped escapings (\, (, ))
                    case '%':
                    case '\\':
                    case '(':
                    case ')': 
                        res[si] = str[i]; break;
                        
                    default: 
                        PDError("unknown escape sequence: \\%c\n", str[i]);
                        res[si] = str[i]; 
                        break;
                }
                esc = false;
            } else {
                res[si] = str[i];
            }
            si++;
        }
    }
    res[si] = 0;
    if (outLength) *outLength = si;
    return res;
}

char *PDStringNameToBinary(char *string, PDSize len, PDBool wrapped, PDSize *outLength)
{
    return PDStringEscapedToBinary(&string[1], len-1, wrapped, outLength);
}

char *PDStringEscapedToName(char *string, PDSize len, PDBool hasW, PDBool addW)
{
    return PDStringTransform(string, len, false, hasW, '/', addW ? '(' : 0, addW ? ')' : 0);
}

char *PDStringNameToEscaped(char *string, PDSize len, PDBool hasW, PDBool addW)
{
    return PDStringTransform(string, len, true, hasW, 0, addW ? '(' : 0, addW ? ')' : 0);
}

char *PDStringNameToHex(char *string, PDSize len, PDBool hasW, PDBool addW)
{
    return PDStringEscapedToHex(&string[1], len-1, hasW, addW);
}

char *PDStringBinaryToHex(char *string, PDSize len, PDBool wrapped)
{
    PDSize rescap = 1 + (len << 1) + (wrapped << 1);
    if (rescap < 10) rescap = 10;
    PDSize reslen = 0;
    char *res = malloc(rescap);
    unsigned char ch;
    PDSize i;
    
    if (wrapped) res[reslen++] = '<';
    
    for (i = 0; i < len; i++) {
        ch = (unsigned char)string[i];
        res[reslen++] = PDOperatorSymbolGlobDehex[ch >> 4];
        res[reslen++] = PDOperatorSymbolGlobDehex[ch & 0xf];
    }

    if (wrapped) res[reslen++] = '>';

    res[reslen] = 0;
    return res;
}

char *PDStringBinaryToEscaped(char *string, PDStringEncoding encoding, PDSize len, PDBool addW, char prefix, PDSize *outLength)
{
    PDSize rescap = 2 + (len << 1) + (addW << 1) + (prefix != 0);
    if (rescap < 10) rescap = 10;
    PDSize reslen = 0;
    char *res = malloc(rescap);
    char ch, e;
    short ord;
    PDSize i;
    PDBool allowNull = outLength != NULL;
    PDAssert(allowNull || encoding != PDStringEncodingUTF16BE); // crash = a UTF-16 encoded string is escaped; the escaped result will have NUL values in it for all regular ASCII characters; the length of the string will be impossible to determine using regular strlen() etc methods and will be truncated at the first occurrence of such a code point
    
    if (prefix) res[reslen++] = prefix;
    if (addW) res[reslen++] = '(';
    
    if (encoding == PDStringEncodingUTF16BE) {
        // write 254, 255 (U+FEFF) indicating that the string is UTF-16 big-endian (see PDF Reference v 1.7, p. 158)
        res[reslen++] = 254;
        res[reslen++] = 255;
    }
    
    for (i = 0; i < len; i++) {
        ch = string[i];
        ord = ch < 0 ? ch + 256 : ch;
        // The NUL character is being printed unescaped by Adobe Acrobat Pro for UTF16-BE strings, so we choose to override the escaping definition for char code \x00, if outLength is provided (otherwise the string will be prematurely truncated at the first NUL). This means C strings break, but in return, UTF-16 strings in PDF do not come out as "\000T\000h\000i\000s\000 \000i\000s\000 [etc]".
        e = !ord && allowNull ? 0 : PDOperatorSymbolGlobEscaping[ord];
        switch (e) {
            case 0: // needs no escaping
                res[reslen++] = ch;
                break;
            case 1: // needs escaping using octal code
                reslen += sprintf(&res[reslen], "\\%s%o", ord < 8 ? "00" : ord < 128 ? "0" : "", ord);
                break;
            default: // can be escaped with a charcode
                res[reslen++] = '\\';
                res[reslen++] = e;
        }
        if (rescap - reslen < 5) {
            rescap += 10 + (len - i) * 2;
            res = realloc(res, rescap);
        }
    }
    
    if (addW) res[reslen++] = ')';
    
    res[reslen] = 0;
    
    if (outLength) *outLength = reslen;

#ifdef DEBUG_PD_STRINGS
    // ensure the opposite is identical
    PDSize opplen;
    char *opp = PDStringEscapedToBinary(&res[prefix?1:0], reslen-(prefix?1:0), addW, &opplen);
    PDAssert(opplen == len);
    for (i = 0; i < opplen; i++) {
        PDAssert(opp[i] == string[i]);
    }
    free(opp);
#endif
    
    return res;
}

char *PDStringHexToEscaped(char *string, PDSize len, PDBool hasW, PDBool addW, char prefix)
{
    // currently we do this by going to binary format first
    char *tmp = PDStringHexToBinary(string, len, hasW, &len);
    char *res = PDStringBinaryToEscaped(tmp, PDStringEncodingUndefined, len, addW, prefix, NULL);
    free(tmp);
    return res;
}

char *PDStringEscapedToHex(char *string, PDSize len, PDBool hasW, PDBool addW)
{
    // currently we do this by going to binary format first
    char *tmp = PDStringEscapedToBinary(string, len, hasW, &len);
    char *res = PDStringBinaryToHex(tmp, len, addW);
    free(tmp);
    return res;
}

PDInteger PDStringPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap)
{
    PDInstancePrinterInit(PDStringRef, 5 + i->length, 5 + i->length);

#ifdef PD_SUPPORT_CRYPTO
    if (i->type != PDStringTypeName && i->ci && i->ci->crypto && ! i->encrypted) {
        PDStringRef enc = PDStringCreateEncrypted(i);
        offs = PDStringPrinter(enc, buf, offs, cap);
        PDRelease(enc);
        return offs;
    }
#endif
    
    if (i->type == PDStringTypeBinary) {
        PDStringRef wrapped = PDStringCreateFromStringWithType(i, PDStringTypeEscaped, true, false);
        offs = PDStringPrinter(wrapped, buf, offs, cap);
        PDRelease(wrapped);
        return offs;
    }
    
    if ((i->type == PDStringTypeEscaped || i->type == PDStringTypeHex) && ! i->wrapped) {
        PDStringRef wrapped = PDStringCreateFromStringWithType(i, i->type, true, false);
        offs = PDStringPrinter(wrapped, buf, offs, cap);
        PDRelease(wrapped);
        return offs;
    }
    
    char *bv = *buf;
    memcpy(&bv[offs], i->data, i->length + 1);
    return offs + i->length;
}

PDBool PDStringEqualsCString(PDStringRef string, const char *cString)
{
    PDBool result;
    
    PDStringRef utf8 = PDStringCreateUTF8Encoded(string);
    PDStringRef compat = PDStringCreateFromStringWithType(utf8, cString[0] == '/' ? PDStringTypeName : PDStringTypeBinary, false, false);

    result = 0 == strcmp(compat->data, cString);
    if (! result) {
        // we may have a UTF starting symbol (which is invisible)
        unsigned char *c = (unsigned char *)compat->data;
        if (c[0] == 0xef && c[1] == 0xbb && c[2] == 0xbf) {
            result = 0 == strcmp(&compat->data[3], cString);
        }
    }
    
    PDRelease(compat);
    PDRelease(utf8);
    
    return result;
}

PDBool PDStringEqualsString(PDStringRef string, PDStringRef string2)
{
    PDBool releaseString2 = false;
    if (string->type != string2->type) {
        // if alt string fits, we use that
        if (string->alt && string->alt->type == string2->type) 
            return PDStringEqualsString(string->alt, string2);
        
        // we don't want to convert TO hex format, ever, unless both are hex already
        if (string->type == PDStringTypeHex)
            return PDStringEqualsString(string2, string);
        
        string2 = PDStringCreateFromStringWithType(string2, string->type, false, false);
        releaseString2 = true;
    }
    
    PDSize len1 = string->length - (string->wrapped<<1);
    PDSize len2 = string2->length - (string2->wrapped<<1);
    
    PDBool result = false;
    
    if (len1 == len2) {
        PDInteger start1 = string->wrapped;
        PDInteger start2 = string2->wrapped;
        
        result = 0 == strncmp(&string->data[start1], &string2->data[start2], len1);
    }    
    
    if (releaseString2) PDRelease(string2);
    
    return result;
}

#pragma mark - Crypto

#ifdef PD_SUPPORT_CRYPTO

PDBool PDStringIsEncrypted(PDStringRef string)
{
    return string->type != PDStringTypeName && string->ci && string->encrypted;
}

void PDStringAttachCrypto(PDStringRef string, pd_crypto crypto, PDInteger objectID, PDInteger genNumber, PDBool encrypted)
{
    PDCryptoInstanceRef ci = PDCryptoInstanceCreate(crypto, objectID, genNumber);
    string->ci = ci;
    string->encrypted = encrypted;
}

void PDStringAttachCryptoInstance(PDStringRef string, PDCryptoInstanceRef ci, PDBool encrypted)
{
    string->ci = PDRetain(ci);
    string->encrypted = encrypted;
}

PDStringRef PDStringCreateEncrypted(PDStringRef string)
{
    if (NULL == string || NULL == string->ci || string->encrypted) return PDRetain(string);
    
    PDSize len;
    char *dst;
    const char *str_in = PDStringBinaryValue(string, &len);
    char *str = malloc(len);
    memcpy(str, str_in, len);
    
    len = pd_crypto_encrypt(string->ci->crypto, string->ci->obid, string->ci->genid, &dst, str, len);
    PDStringRef encrypted = PDStringCreateBinary(dst, len);
    PDStringAttachCryptoInstance(encrypted, string->ci, true);
    return encrypted;
}

PDStringRef PDStringCreateDecrypted(PDStringRef string)
{
    if (NULL == string || NULL == string->ci || ! string->encrypted || string->length == 0) return PDRetain(string);
    
    PDSize len;
    const char *data_in = PDStringBinaryValue(string, &len);
    char *data = malloc(len + 1);
    memcpy(data, data_in, len);
    data[len] = 0;
    
    pd_crypto_convert(string->ci->crypto, string->ci->obid, string->ci->genid, data, len);
    PDStringRef decrypted = PDStringCreate(data, strlen(data));
    PDStringAttachCryptoInstance(decrypted, string->ci, false);
    return decrypted;
}

#endif
