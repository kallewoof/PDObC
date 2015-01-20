//
// PDStringUTF.c
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

#include <iconv.h>
#include "PDString.h"
#include "PDDictionary.h"
#include "PDFont.h"
#include "PDCMap.h"
#include "PDNumber.h"
#include "pd_internal.h"

PDBool iconv_unicode_mb_to_uc_fb_called = false;
PDBool iconv_unicode_uc_to_mb_fb_called = false;

void pdstring_iconv_unicode_mb_to_uc_fallback(const char* inbuf, size_t inbufsize,
                                              void (*write_replacement) (const unsigned int *buf, size_t buflen,
                                                                         void* callback_arg),
                                              void* callback_arg,
                                              void* data)
{
    iconv_unicode_mb_to_uc_fb_called = true;
}

void pdstring_iconv_unicode_uc_to_mb_fallback(unsigned int code,
                                              void (*write_replacement) (const char *buf, size_t buflen,
                                                                         void* callback_arg),
                                              void* callback_arg,
                                              void* data)
{
    iconv_unicode_uc_to_mb_fb_called = true;
}

const struct iconv_fallbacks pdstring_iconv_fallbacks = {
    pdstring_iconv_unicode_mb_to_uc_fallback,
    pdstring_iconv_unicode_uc_to_mb_fallback,
    NULL,
    NULL,
    NULL
};

void PDStringDetermineEncoding(PDStringRef string);

static PDStringEncoding autoList[] = {
    2, // starting index = utf-8
    0, // ascii -> default (end)
    6, // utf-8 -> mac-roman
    4, // utf-16be -> utf-16le
    5, // utf-16le -> utf-32
    7, // utf-32 -> euc-jp
    3, // mac-roman -> utf-16be
    8, // euc-jp -> shift-jis
    9, // shift-jis -> iso-8859-1
    10, // iso-8859-1 -> iso-8859-2
    1, // iso-8859-2 -> ascii
};

#define PDStringEncodingEnumerate(enc) \
            for (PDStringEncoding enc = autoList[0]; \
                 enc > 0 && enc < __PDSTRINGENC_END; \
                 enc = autoList[enc])

static const char *enc_ascii = "ASCII";
static const char *enc_utf8 = "UTF-8";
static const char *enc_utf16be = "UTF-16BE";
static const char *enc_utf16le = "UTF-16LE";
static const char *enc_utf32 = "UTF-32";
static const char *enc_macroman = "MACROMAN";
static const char *enc_eucjp = "EUC-JP";
static const char *enc_shift_jis = "SHIFT_JIS";
static const char *enc_iso_8859_1 = "ISO-8859-1";
static const char *enc_iso_8859_2 = "ISO-8859-2";
static const char *enc_cp1251 = "CP1251";
static const char *enc_cp1252 = "CP1252";
static const char *enc_cp1253 = "CP1253";
static const char *enc_cp1254 = "CP1254";
static const char *enc_cp1250 = "CP1250";
static const char *enc_iso_2022_jp = "ISO-2022-JP";
static const char **enc_names = NULL;
static PDDictionaryRef encMap = NULL;

static inline void setup_enc_names() 
{
    PDAssert(__PDSTRINGENC_END == 16);
    enc_names = malloc(sizeof(char*) * __PDSTRINGENC_END);
    enc_names[ 0] = enc_ascii;
    enc_names[ 1] = enc_utf8;
    enc_names[ 2] = enc_utf16be;
    enc_names[ 3] = enc_utf16le;
    enc_names[ 4] = enc_utf32;
    enc_names[ 5] = enc_macroman;
    enc_names[ 6] = enc_eucjp;
    enc_names[ 7] = enc_shift_jis;
    enc_names[ 8] = enc_iso_8859_1;
    enc_names[ 9] = enc_iso_8859_2;
    enc_names[10] = enc_cp1251;
    enc_names[11] = enc_cp1252;
    enc_names[12] = enc_cp1253;
    enc_names[13] = enc_cp1254;
    enc_names[14] = enc_cp1250;
    enc_names[15] = enc_iso_2022_jp;
    
    encMap = PDDictionaryCreate();
    
    // same as above; this is not strictly necessary, but for convenience, Pajdeg's PDStringEncodingByName also accepts the (internally canonical) iconv names
    PDDictionarySet(encMap, enc_ascii, PDNumberWithInteger(PDStringEncodingASCII));
    PDDictionarySet(encMap, enc_utf8, PDNumberWithInteger(PDStringEncodingUTF8));
    PDDictionarySet(encMap, enc_utf16be, PDNumberWithInteger(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, enc_utf16le, PDNumberWithInteger(PDStringEncodingUTF16LE));
    PDDictionarySet(encMap, enc_utf32, PDNumberWithInteger(PDStringEncodingUTF32));
    PDDictionarySet(encMap, enc_macroman, PDNumberWithInteger(PDStringEncodingMacRoman));
    PDDictionarySet(encMap, enc_eucjp, PDNumberWithInteger(PDStringEncodingEUCJP));
    PDDictionarySet(encMap, enc_shift_jis, PDNumberWithInteger(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, enc_iso_8859_1, PDNumberWithInteger(PDStringEncodingISO8859_1));
    PDDictionarySet(encMap, enc_iso_8859_2, PDNumberWithInteger(PDStringEncodingISO8859_2));
    PDDictionarySet(encMap, enc_cp1251, PDNumberWithInteger(PDStringEncodingCP1251));
    PDDictionarySet(encMap, enc_cp1252, PDNumberWithInteger(PDStringEncodingCP1252));
    PDDictionarySet(encMap, enc_cp1253, PDNumberWithInteger(PDStringEncodingCP1253));
    PDDictionarySet(encMap, enc_cp1254, PDNumberWithInteger(PDStringEncodingCP1254));
    PDDictionarySet(encMap, enc_cp1250, PDNumberWithInteger(PDStringEncodingCP1250));
    PDDictionarySet(encMap, enc_iso_2022_jp, PDNumberWithInteger(PDStringEncodingISO2022JP));
    
    // PDF specification names
    PDDictionarySet(encMap, "Identity-H", PDNumberWithInteger(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "Identity-V", PDNumberWithInteger(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "WinAnsiEncoding", PDNumberWithInteger(PDStringEncodingCP1252));
    PDDictionarySet(encMap, "MacRomanEncoding", PDNumberWithInteger(PDStringEncodingMacRoman));
    PDDictionarySet(encMap, "MacRomanEncodingASCII", PDNumberWithInteger(PDStringEncodingMacRoman));
}

const char *PDStringEncodingToIconvName(PDStringEncoding enc)
{
    if (enc < 1 || enc > __PDSTRINGENC_END) return NULL;
    if (enc_names == NULL) setup_enc_names();
    return enc_names[enc-1];
}

PDStringEncoding PDStringEncodingGetByName(const char *encodingName)
{
    if (enc_names == NULL) setup_enc_names();
    PDNumberRef encNum = PDDictionaryGet(encMap, encodingName);
    if (NULL == encNum) {
        PDError("Unknown encoding string: %s", encodingName);
        return PDStringEncodingUndefined;
    }
    return (PDStringEncoding) PDNumberGetInteger(encNum);
}

void PDStringSetEncoding(PDStringRef string, PDStringEncoding encoding)
{
    string->enc = encoding;
}

void PDStringSetFont(PDStringRef string, PDFontRef font)
{
    PDRetain(font);
    PDRelease(string->font);
    string->font = font;
    if (font) string->enc = PDFontGetEncoding(font);
}

PDStringRef PDUTF8String(PDStringRef string)
{
    PDStringRef source = string;
    
    // we get a lot of plain strings, so we check that first off
    PDBool onlyPlain = PDStringGetType(string) == PDStringTypeBinary || PDStringGetType(string) == PDStringTypeEscaped;
    for (int i = string->wrapped; onlyPlain && i < string->length - string->wrapped; i++) onlyPlain = string->data[i] == ' ' || (string->data[i] >= 'a' && string->data[i] <= 'z') || (string->data[i] >= 'A' && string->data[i] <= 'Z') || (string->data[i] >= '0' && string->data[i] <= '9') || string->data[i] == '\n' || string->data[i] == '\r' || string->data[i] == '\t' || string->data[i] == '\\' || string->data[i] == '"' || string->data[i] == '\'' || string->data[i] == '.' || string->data[i] == '(' || string->data[i] == ')';
    if (onlyPlain) {
        string->enc = PDStringEncodingUTF8;
        return string;
    }
    
    // does the string have an utf8 alternative already?
    if (string->alt && string->alt->enc == PDStringEncodingUTF8) return string->alt;
    
    // we need an escaped or binary representation of the string
    if (PDStringTypeBinary != string->type && PDStringTypeEscaped != string->type) {
        source = PDAutorelease(PDStringCreateBinaryFromString(string));
    }
    
    PDStringEncoding knownEncoding = PDStringEncodingDefault;
    
    // does this string have a font, with a toUnicode mapping? if so we can get a UTF16BE from it
    if (source->font && source->font->toUnicode) {
        source = PDCMapApply(source->font->toUnicode, source);
        knownEncoding = source->enc = PDStringEncodingUTF16BE;
    }
    
    iconv_t cd;
    
    PDInteger cap = (3 * source->length)>>1;
    char *results = malloc(sizeof(char) * cap);
    
    PDStringEncodingEnumerate(enc) {
        size_t targetLeft = cap;
        char *targetStart = results;
        
        if (knownEncoding) {
            enc = knownEncoding;
            knownEncoding = PDStringEncodingDefault;
        }

        cd = iconv_open(enc == PDStringEncodingUTF8 ? enc_utf16be : enc_utf8, PDStringEncodingToIconvName(enc));
        
        iconvctl(cd, ICONV_SET_FALLBACKS, (void*)&pdstring_iconv_fallbacks);
        
        char *sourceData = (char *)&source->data[source->wrapped];
        size_t sourceLeft = source->length - (source->wrapped<<1);
        size_t oldSourceLeft;
        
        while (1) {
            iconv_unicode_mb_to_uc_fb_called = iconv_unicode_uc_to_mb_fb_called = false;
            oldSourceLeft = sourceLeft;
            iconv(cd, &sourceData, &sourceLeft, &targetStart, &targetLeft);
            if (oldSourceLeft == sourceLeft || targetLeft > 4 || sourceLeft == 0 || iconv_unicode_uc_to_mb_fb_called || iconv_unicode_mb_to_uc_fb_called) break;
            
            targetLeft += cap;
            cap <<= 1;
            PDSize size = targetStart - results;
            results = realloc(results, cap);
            targetStart = results + size;
        }
        
        iconv_close(cd);
        
        if (sourceLeft == 0 && !iconv_unicode_mb_to_uc_fb_called && !iconv_unicode_uc_to_mb_fb_called) {
            string->enc = enc;
            if (string->enc == PDStringEncodingUTF8) {
                free(results);
                return string;
            }
            
            PDRelease(string->alt);
            string->alt = PDStringCreateBinary((char *)results, (targetStart-results));
            string->alt->enc = PDStringEncodingUTF8;
            return string->alt;
        }
    }
    
    free(results);
    
    // unable to determine string type
    string->enc = PDStringEncodingUndefined;
    return NULL;
}

PDStringRef PDUTF16String(PDStringRef string)
{
    PDStringRef source = string;
    
    // does the string have an utf16 alternative already?
    if (string->alt && string->alt->enc == PDStringEncodingUTF16BE) return string->alt;
    
    // we need an escaped or binary representation of the string
    if (PDStringTypeBinary != string->type && PDStringTypeEscaped != string->type) {
        source = PDAutorelease(PDStringCreateBinaryFromString(string));
    }
    
    // we need a defined encoding
    if (PDStringEncodingDefault == string->enc) PDStringDetermineEncoding(string);
    
    // is the string UTF16 already?
    if (PDStringEncodingUTF16BE == string->enc) return string;
    
    iconv_t cd;
    
    PDInteger cap = (3 * source->length)>>1;
    char *results = malloc(sizeof(char) * cap);
    
    for (PDStringEncoding enc = PDStringEncodingUTF16BE; enc > 0; enc -= 1 + (enc-1 == PDStringEncodingUTF16BE)) {
        size_t targetLeft = cap;
        char *targetStart = results;

        cd = iconv_open(enc == PDStringEncodingUTF16BE ? enc_utf8 : enc_utf16be, PDStringEncodingToIconvName(enc));
        
        char *sourceData = (char *)&source->data[source->wrapped];
        size_t sourceLeft = source->length - (source->wrapped<<1);
        size_t oldSourceLeft;
        
        while (1) {
            iconv_unicode_mb_to_uc_fb_called = iconv_unicode_uc_to_mb_fb_called = false;
            oldSourceLeft = sourceLeft;
            iconv(cd, &sourceData, &sourceLeft, &targetStart, &targetLeft);
            if (oldSourceLeft == sourceLeft || sourceLeft == 0 || iconv_unicode_uc_to_mb_fb_called || iconv_unicode_mb_to_uc_fb_called) break;
            
            targetLeft += cap;
            cap <<= 1;
            PDSize size = targetStart - results;
            results = realloc(results, cap);
            targetStart = results + size;
        }
        
        iconv_close(cd);
        
        if (sourceLeft == 0 && !iconv_unicode_mb_to_uc_fb_called && !iconv_unicode_uc_to_mb_fb_called) {
            string->enc = enc;
            if (string->enc == PDStringEncodingUTF16BE) {
                free(results);
                return string;
            }
            
            PDRelease(string->alt);
            string->alt = PDStringCreateBinary((char *)results, (targetStart-results));
            string->alt->enc = PDStringEncodingUTF16BE;
            return string->alt;
        }
        
        if (enc == PDStringEncodingUTF16BE) enc = __PDSTRINGENC_END + 1;
    }
    
    free(results);
    
    // failure
    return NULL;
}

void PDStringDetermineEncoding(PDStringRef string)
{
    if (string->enc != PDStringEncodingDefault) return;
    
    // we try to create UTF8 string; the string will have its encoding set on return
    if (! PDUTF8String(string)) {
        PDWarn("Undefined string encoding encountered");
    }
}

PDStringRef PDStringCreateUTF8Encoded(PDStringRef string)
{
    if (string->enc == PDStringEncodingDefault) {
        PDStringDetermineEncoding(string);
    }
    
    switch (string->enc) {
        case PDStringEncodingUndefined:
            return NULL;
        case PDStringEncodingDefault:
        case PDStringEncodingASCII:
        case PDStringEncodingUTF8:
            return PDRetain(string);
        default:
            return PDRetain(PDUTF8String(string));
    }
}

PDStringRef PDStringCreateUTF16Encoded(PDStringRef string)
{
    if (string->enc == PDStringEncodingDefault) {
        PDStringDetermineEncoding(string);
    }
    
    switch (string->enc) {
        case PDStringEncodingUndefined:
            return NULL;
        case PDStringEncodingUTF16BE:
            return PDRetain(string);
        default:
            return PDRetain(PDUTF16String(string));
    }
}
