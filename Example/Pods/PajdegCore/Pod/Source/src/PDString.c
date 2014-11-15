//
// PDString.c
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
char *PDStringBinaryToEscaped(char *string, PDSize len, PDBool addW, char prefix);                  ///< 01101010               -> "foo\123"
char *PDStringBinaryToHex(char *string, PDSize len, PDBool addW);                                   ///< 01101010               -> "abc123"
// name->*
char *PDStringNameToEscaped(char *string, PDSize len, PDBool hasW, PDBool addW);                    ///< "/foo123"              -> "foo123"
char *PDStringNameToHex(char *string, PDSize len, PDBool hasW, PDBool addW);                        ///< "/foo123"              -> "abc123"
char *PDStringNameToBinary(char *string, PDSize len, PDBool hasW, PDSize *outLength);               ///< "/foo123"              -> "01101010"

// Public

void PDStringDestroy(PDStringRef string)
{
    PDRelease(string->alt);
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

PDStringRef PDStringCreate(char *string)
{
#ifdef DEBUG
    PDStringVerifyOwnership(string, strlen(string));
#endif
    
    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->data = string;
    res->alt = NULL;
    res->length = strlen(string);
    res->type = PDStringTypeEscaped;
    res->wrapped = (string[0] == '(' && string[res->length-1] == ')');
#ifdef PD_SUPPORT_CRYPTO
    res->ci = NULL;
#endif
    return res;
}

PDStringRef PDStringCreateUnescaped(char *unescapedString)
{
    PDSize len = strlen(unescapedString);
#ifdef DEBUG
    PDStringVerifyOwnership(unescapedString, len);
#endif
    
    PDStringRef result = PDStringCreate(PDStringBinaryToEscaped(unescapedString, len, true, 0));
    free(unescapedString);
    return result;
}

PDStringRef PDStringCreateWithName(char *name)
{
#ifdef DEBUG
    PDStringVerifyOwnership(name, strlen(name));
#endif
    
    if (name[0] != '/') {
        // names always include '/' in the data
        char *fixedName = malloc(strlen(name) + 2);
        fixedName[0] = '/';
        strcpy(&fixedName[1], name);
        free(name);
        name = fixedName;
    }
    
    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->data = name;
    res->alt = NULL;
    res->length = strlen(name);
    res->type = PDStringTypeName;
    res->wrapped = res->length > 1 && (name[1] == '(' && name[res->length-1] == ')');
#ifdef PD_SUPPORT_CRYPTO
    res->ci = NULL;
#endif
    return res;
}

PDStringRef PDStringCreateBinary(char *data, PDSize length)
{
#ifdef DEBUG
    PDStringVerifyOwnership(data, length);
#endif

    PDStringRef res = PDAllocTyped(PDInstanceTypeString, sizeof(struct PDString), PDStringDestroy, false);
    res->data = data;
    res->alt = NULL;
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
    res->data = hex;
    res->alt = NULL;
    res->length = strlen(hex);
    res->type = PDStringTypeHex;
    res->wrapped = (hex[0] == '<' && hex[res->length-1] == '>');
#ifdef PD_SUPPORT_CRYPTO
    res->ci = NULL;
#endif
    return res;
}

PDStringRef PDStringCreateFromStringWithType(PDStringRef string, PDStringType type, PDBool wrap)
{
    if (string->type == type && (string->type == PDStringTypeBinary || string->wrapped == wrap))
        return PDRetain(string);
    
    if (string->alt && string->alt->type == type && (string->alt->type == PDStringTypeBinary || string->alt->wrapped == wrap))
        return PDRetain(string->alt);
    
    const char *res;
    char *buf;
    PDSize len;
    PDStringRef result;
    switch (type) {
        case PDStringTypeEscaped:
            result = PDStringCreate(strdup(PDStringEscapedValue(string, wrap)));
            break;
            
        case PDStringTypeName:
            result = PDStringCreateWithName(strdup(PDStringNameValue(string, wrap)));
            break;

        case PDStringTypeHex:
            result = PDStringCreateWithHexString(strdup(PDStringHexValue(string, wrap)));
            break;
            
        default:
            res = PDStringBinaryValue(string, &len);
//            if (res) {
                buf = malloc(len);
                memcpy(buf, res, len);
                result = PDStringCreateBinary(buf, len);
//            } else {
//                result = NULL;
//            }
    }
    
#ifdef PD_SUPPORT_CRYPTO
    if (string->ci) PDStringAttachCryptoInstance(result, string->ci, string->encrypted);
#endif
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

const char *PDStringEscapedValue(PDStringRef string, PDBool wrap)
{
    if (string == NULL) return NULL;
    
    if (PDResolve(string) == PDInstanceTypeNumber) return PDNumberToString((PDNumberRef)string);
    
    // see if we have what is asked for already
    if (string->type == PDStringTypeEscaped && string->wrapped == wrap) {
        return string->data;
    } else if (string->alt && string->alt->type == PDStringTypeEscaped && string->alt->wrapped == wrap) {
        return string->alt->data;
    } 
    
    // we don't, so set up alternative; we use the PDString which offers the easiest conversion, which is
    //  escaped strings, then names, then binary strings, then hex strings
    PDStringRef source = ((string->alt && (string->alt->type == PDStringTypeEscaped || string->type == PDStringTypeHex || (string->alt->type == PDStringTypeName && string->type != PDStringTypeEscaped)))
                          ? string->alt
                          : string);
    
    char *data;
    if (source->type == PDStringTypeBinary)
        data = PDStringBinaryToEscaped(source->data, source->length, wrap, 0);
    else if (source->type == PDStringTypeHex)
        data = PDStringHexToEscaped(source->data, source->length, source->wrapped, wrap, 0);
    else 
        data = PDStringTransform(source->data, source->length, source->type == PDStringTypeName, source->wrapped, 0, wrap ? '(' : 0, wrap ? ')' : 0);
        
//        if (source->type == PDStringTypeName) 
//        data = PDStringNameToEscaped(source->data, source->length, source->wrapped, wrap);
//    else if (wrap) 
//        data = PDStringWrappedValue(source->data, source->length, 0, '(', ')');
//    else 
//        data = PDStringUnwrappedValue(source->data, source->length, false, 0);

    PDRelease(string->alt);
    string->alt = PDStringCreate(data);
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
        data = PDStringBinaryToEscaped(source->data, source->length, wrap, '/');
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
        data = strdup(PDStringBinaryToHex(source->data, source->length, wrap));
    else if (source->type == PDStringTypeEscaped)
        data = strdup(PDStringEscapedToHex(source->data, source->length, source->wrapped, wrap));
    else if (source->type == PDStringTypeName)
        data = strdup(PDStringNameToHex(source->data, source->length, source->wrapped, wrap));
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
        res[reslen++] = (PDOperatorSymbolGlobHex[csr[i]] << 4) + PDOperatorSymbolGlobHex[csr[i+1]];
        
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
    char *res = malloc(len);
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
                        res[si] = (res[si] << 4) + (str[i] - '0');
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
    *outLength = si;
    return res;
}

char *PDStringNameToBinary(char *string, PDSize len, PDBool wrapped, PDSize *outLength)
{
    return PDStringEscapedToBinary(&string[1], len-1, wrapped, outLength);
}

char *PDStringBinaryToName(char *string, PDSize len, PDBool wrapped)
{
    return PDStringBinaryToEscaped(string, len, wrapped, '/');
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
    char ch;
    PDSize i;
    
    if (wrapped) res[reslen++] = '<';
    
    for (i = 0; i < len; i++) {
        ch = string[i];
        res[reslen++] = PDOperatorSymbolGlobDehex[ch >> 4];
        res[reslen++] = PDOperatorSymbolGlobDehex[ch & 0xf];
    }

    if (wrapped) res[reslen++] = '>';

    res[reslen] = 0;
    return res;
}

char *PDStringBinaryToEscaped(char *string, PDSize len, PDBool addW, char prefix)
{
    PDSize rescap = (len << 1) + (addW << 1) + (prefix != 0);
    if (rescap < 10) rescap = 10;
    PDSize reslen = 0;
    char *res = malloc(rescap);
    char ch, e;
    short ord;
    PDSize i;
    
    if (prefix) res[reslen++] = prefix;
    if (addW) res[reslen++] = '(';
    
    for (i = 0; i < len; i++) {
        ch = string[i];
        ord = ch < 0 ? ch + 256 : ch;
        e = PDOperatorSymbolGlobEscaping[ord];
        switch (e) {
            case 0: // needs escaping using code
                reslen += sprintf(&res[reslen], "\\%s%d", ord < 10 ? "00" : ord < 100 ? "0" : "", ord);
                break;
            case 1: // needs no escaping
                res[reslen++] = ch;
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
    return res;
}

char *PDStringHexToEscaped(char *string, PDSize len, PDBool hasW, PDBool addW, char prefix)
{
    // currently we do this by going to binary format first
    char *tmp = PDStringHexToBinary(string, len, hasW, &len);
    char *res = PDStringBinaryToEscaped(tmp, len, addW, prefix);
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
    
    if ((i->type == PDStringTypeEscaped || i->type == PDStringTypeHex) && ! i->wrapped) {
        PDStringRef wrapped = PDStringCreateFromStringWithType(i, i->type, true);
        offs = PDStringPrinter(wrapped, buf, offs, cap);
        PDRelease(wrapped);
        return offs;
    }
    
    char *bv = *buf;
    strcpy(&bv[offs], i->data);
    return offs + i->length;
//    
//    PDBool ownsStr = false;
//    char *str = i->data;
//    PDSize len = i->length;
//    if (i->type == PDStringTypeBinary) {
//        ownsStr = true;
//        str = PDStringBinaryToEscaped(str, len, true);
//        len = strlen(str);
//        PDInstancePrinterRequire(1 + len, 1 + len);
//    } else if (! i->wrapped) {
//        ownsStr = true;
//        str = PDStringWrappedValue(str, len, i->type == PDStringTypeHex ? '<' : '(', i->type == PDStringTypeHex ? '>' : ')');
//        len += 2;
//    }
//    
//    char *bv = *buf;
//    strcpy(&bv[offs], str);
//    if (ownsStr) free(str);
//    return offs + len;
}

PDBool PDStringEqualsCString(PDStringRef string, const char *cString)
{
    PDBool result;
    
    PDStringRef compat = PDStringCreateFromStringWithType(string, cString[0] == '/' ? PDStringTypeName : PDStringTypeEscaped, false);
    
    result = 0 == strcmp(cString, compat->data);
    
    PDRelease(compat);
    
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
        
        string2 = PDStringCreateFromStringWithType(string2, string->type, false);
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
    char *data = malloc(len);
    memcpy(data, data_in, len);
    
    pd_crypto_convert(string->ci->crypto, string->ci->obid, string->ci->genid, data, len);
    PDStringRef decrypted = PDStringCreate(data);
    PDStringAttachCryptoInstance(decrypted, string->ci, false);
    return decrypted;
}

#endif
