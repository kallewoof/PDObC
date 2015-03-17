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
#include "PDArray.h"
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

PDStringEncoding PDStringDetermineEncoding(PDStringRef string);

static PDStringEncoding autoList[__PDSTRINGENC_END] = {0};

#define PDStringEncodingEnumerate(enc) \
            for (PDStringEncoding enc = autoList[0]; \
                 enc > 0 && enc < __PDSTRINGENC_END; \
                 enc = autoList[enc])

// Apple based encodings
static const char *enc_ascii = "ASCII";
static const char *enc_pdf = "*PDFDocEncoding";
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

// PDF specification encodings
static const char *enc_euccn = "EUC-CN";
static const char *enc_gbk = "GBK";
static const char *enc_gb18030 = "GB18030";
static const char *enc_ucs2 = "UCS-2BE";
static const char *enc_big5 = "BIG5";
static const char *enc_big5hkscs = "BIG5-HKSCS";
static const char *enc_cp950 = "CP950";
static const char *enc_euctw = "EUC-TW";
static const char *enc_euckr = "EUC-KR";
static const char *enc_uhc = "UHC";

static const char **enc_names = NULL;
static PDDictionaryRef encMap = NULL;

static inline void setup_autolist()
{
#define map(a, b) autoList[a] = b
    map(PDStringEncodingDefault, PDStringEncodingUTF8);//16BE);
    map(PDStringEncodingUTF16BE, PDStringEncodingMacRoman);//UTF8);
    map(PDStringEncodingUTF8, PDStringEncodingUTF16BE);//MacRoman);
    //    map(PDStringEncodingPDF, PDStringEncodingMacRoman);
    map(PDStringEncodingMacRoman, PDStringEncodingUTF16LE);
    map(PDStringEncodingUTF16LE, PDStringEncodingUTF32);
    map(PDStringEncodingUTF32, PDStringEncodingEUCJP);
    map(PDStringEncodingEUCJP, PDStringEncodingSHIFTJIS);
    map(PDStringEncodingSHIFTJIS, PDStringEncodingISO8859_1);
    map(PDStringEncodingISO8859_1, PDStringEncodingISO8859_2);
    map(PDStringEncodingISO8859_2, PDStringEncodingASCII);
    map(PDStringEncodingASCII, PDStringEncodingDefault);
#undef map
}

static inline void setup_enc_names() 
{
    PDAssert(__PDSTRINGENC_END == 28);

    enc_names = malloc(sizeof(char*) * __PDSTRINGENC_END);
    int ite = 0;
    enc_names[ite++] = enc_ascii;
    enc_names[ite++] = enc_pdf;
    enc_names[ite++] = enc_utf8;
    enc_names[ite++] = enc_utf16be;
    enc_names[ite++] = enc_utf16le;
    enc_names[ite++] = enc_utf32;
    enc_names[ite++] = enc_macroman;
    enc_names[ite++] = enc_eucjp;
    enc_names[ite++] = enc_shift_jis;
    enc_names[ite++] = enc_iso_8859_1;
    enc_names[ite++] = enc_iso_8859_2;
    enc_names[ite++] = enc_cp1251;
    enc_names[ite++] = enc_cp1252;
    enc_names[ite++] = enc_cp1253;
    enc_names[ite++] = enc_cp1254;
    enc_names[ite++] = enc_cp1250;
    enc_names[ite++] = enc_iso_2022_jp;
    enc_names[ite++] = enc_euccn;
    enc_names[ite++] = enc_gbk;
    enc_names[ite++] = enc_gb18030;
    enc_names[ite++] = enc_ucs2;
    enc_names[ite++] = enc_big5;
    enc_names[ite++] = enc_big5hkscs;
    enc_names[ite++] = enc_cp950;
    enc_names[ite++] = enc_euctw;
    enc_names[ite++] = enc_euckr;
    enc_names[ite++] = enc_uhc;
    
    encMap = PDDictionaryCreate();
    
    // same as above; this is not strictly necessary, but for convenience, Pajdeg's PDStringEncodingByName also accepts the (internally canonical) iconv names
#define E(v) PDNumberWithInteger(v)
    PDDictionarySet(encMap, enc_ascii, E(PDStringEncodingASCII));
    PDDictionarySet(encMap, enc_pdf, E(PDStringEncodingPDF));
    PDDictionarySet(encMap, enc_utf8, E(PDStringEncodingUTF8));
    PDDictionarySet(encMap, enc_utf16be, E(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, enc_utf16le, E(PDStringEncodingUTF16LE));
    PDDictionarySet(encMap, enc_utf32, E(PDStringEncodingUTF32));
    PDDictionarySet(encMap, enc_macroman, E(PDStringEncodingMacRoman));
    PDDictionarySet(encMap, enc_eucjp, E(PDStringEncodingEUCJP));
    PDDictionarySet(encMap, enc_shift_jis, E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, enc_iso_8859_1, E(PDStringEncodingISO8859_1));
    PDDictionarySet(encMap, enc_iso_8859_2, E(PDStringEncodingISO8859_2));
    PDDictionarySet(encMap, enc_cp1251, E(PDStringEncodingCP1251));
    PDDictionarySet(encMap, enc_cp1252, E(PDStringEncodingCP1252));
    PDDictionarySet(encMap, enc_cp1253, E(PDStringEncodingCP1253));
    PDDictionarySet(encMap, enc_cp1254, E(PDStringEncodingCP1254));
    PDDictionarySet(encMap, enc_cp1250, E(PDStringEncodingCP1250));
    PDDictionarySet(encMap, enc_iso_2022_jp, E(PDStringEncodingISO2022JP));
    
    // PDF specification names
    PDDictionarySet(encMap, "Identity-H", E(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "Identity-V", E(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "WinAnsiEncoding", E(PDStringEncodingCP1252));
    PDDictionarySet(encMap, "MacRomanEncoding", E(PDStringEncodingMacRoman));
    PDDictionarySet(encMap, "MacRomanEncodingASCII", E(PDStringEncodingMacRoman));
    PDDictionarySet(encMap, "PDFDocEncoding", E(PDStringEncodingPDF));
    
    // Chinese (simplified)
    PDDictionarySet(encMap, "GB-EUC-H", E(PDStringEncodingEUCCN));
    PDDictionarySet(encMap, "GB-EUC-V", E(PDStringEncodingEUCCN));
    PDDictionarySet(encMap, "GBpc-EUC-H", E(PDStringEncodingEUCCN));
    PDDictionarySet(encMap, "GBpc-EUC-V", E(PDStringEncodingEUCCN));
    PDDictionarySet(encMap, "GBK-EUC-H", E(PDStringEncodingGBK));
    PDDictionarySet(encMap, "GBK-EUC-V", E(PDStringEncodingGBK));
    PDDictionarySet(encMap, "GBKp-EUC-H", E(PDStringEncodingGBK));
    PDDictionarySet(encMap, "GBKp-EUC-V", E(PDStringEncodingGBK));
    PDDictionarySet(encMap, "GBK2K-H", E(PDStringEncodingGB18030));
    PDDictionarySet(encMap, "GBK2K-V", E(PDStringEncodingGB18030));
    PDDictionarySet(encMap, "UniGB-UCS2-H", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniGB-UCS2-V", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniGB-UTF16-H", E(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "UniGB-UTF16-V", E(PDStringEncodingUTF16BE));
    
    // Chinese (traditional)
    PDDictionarySet(encMap, "B5pc-H", E(PDStringEncodingBIG5));
    PDDictionarySet(encMap, "B5pc-V", E(PDStringEncodingBIG5));
    PDDictionarySet(encMap, "HKscs-B5-H", E(PDStringEncodingBIG5HKSCS));
    PDDictionarySet(encMap, "HKscs-B5-V", E(PDStringEncodingBIG5HKSCS));
    PDDictionarySet(encMap, "ETen-B5-H", E(PDStringEncodingBIG5));
    PDDictionarySet(encMap, "ETen-B5-V", E(PDStringEncodingBIG5));
    PDDictionarySet(encMap, "ETenms-B5-H", E(PDStringEncodingBIG5));
    PDDictionarySet(encMap, "ETenms-B5-V", E(PDStringEncodingBIG5));
    PDDictionarySet(encMap, "CNS-EUC-H", E(PDStringEncodingEUCTW));
    PDDictionarySet(encMap, "CNS-EUC-V", E(PDStringEncodingEUCTW));
    PDDictionarySet(encMap, "UniCNS-UCS2-H", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniCNS-UCS2-V", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniCNS-UTF16-H", E(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "UniCNS-UTF16-V", E(PDStringEncodingUTF16BE));
    
    // Japanese
    PDDictionarySet(encMap, "83pv-RKSJ-H", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "90ms-RKSJ-H", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "90ms-RKSJ-V", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "90msp-RKSJ-H", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "90msp-RKSJ-V", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "90pv-RKSJ-H", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "Add-RKSJ-H", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "Add-RKSJ-V", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "EUC-H", E(PDStringEncodingEUCJP));
    PDDictionarySet(encMap, "EUC-V", E(PDStringEncodingEUCJP));
    PDDictionarySet(encMap, "Ext-RKSJ-H", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "Ext-RKSJ-V", E(PDStringEncodingSHIFTJIS));
    PDDictionarySet(encMap, "H", E(PDStringEncodingISO2022JP));
    PDDictionarySet(encMap, "V", E(PDStringEncodingISO2022JP));
    PDDictionarySet(encMap, "UniJIS-UCS2-H", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniJIS-UCS2-V", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniJIS-UCS2-HW-H", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniJIS-UCS2-HW-V", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniJIS-UTF16-H", E(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "UniJIS-UTF16-V", E(PDStringEncodingUTF16BE));
    
    // Korean
    PDDictionarySet(encMap, "KSC-EUC-H", E(PDStringEncodingEUCKR));
    PDDictionarySet(encMap, "KSC-EUC-V", E(PDStringEncodingEUCKR));
    PDDictionarySet(encMap, "KSCms-UHC-H", E(PDStringEncodingUHC));
    PDDictionarySet(encMap, "KSCms-UHC-V", E(PDStringEncodingUHC));
    PDDictionarySet(encMap, "KSCms-UHC-HW-H", E(PDStringEncodingUHC));
    PDDictionarySet(encMap, "KSCms-UHC-HW-V", E(PDStringEncodingUHC));
    PDDictionarySet(encMap, "KSCpc-EUC-H", E(PDStringEncodingEUCKR)); // these may be wrong; adobe spec goes: Mac OS, KS X 1001:1992 character set with Mac OS KH extensions, Script Manager Code 3
    PDDictionarySet(encMap, "UniKS-UCS2-H", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniKS-UCS2-V", E(PDStringEncodingUCS2));
    PDDictionarySet(encMap, "UniKS-UTF16-H", E(PDStringEncodingUTF16BE));
    PDDictionarySet(encMap, "UniKS-UTF16-V", E(PDStringEncodingUTF16BE));

#undef E
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
    if (string->alt) PDStringSetFont(string->alt, font);
}

PDStringRef PDUTF8String(PDStringRef string)
{
    if (autoList[0] == 0) setup_autolist();
    
    PDStringRef source = string;
    
    // we get a lot of plain strings, so we check that first off
    PDBool onlyPlain = string->enc == PDStringEncodingDefault && (PDStringGetType(string) == PDStringTypeBinary || PDStringGetType(string) == PDStringTypeEscaped);
    for (int i = string->wrapped; onlyPlain && i < string->length - string->wrapped; i++) onlyPlain = string->data[i] == ' ' || (string->data[i] >= 'a' && string->data[i] <= 'z') || (string->data[i] >= 'A' && string->data[i] <= 'Z') || (string->data[i] >= '0' && string->data[i] <= '9') || string->data[i] == '\n' || string->data[i] == '\r' || string->data[i] == '\t' || string->data[i] == '\\' || string->data[i] == '"' || string->data[i] == '\'' || string->data[i] == '.' || string->data[i] == '(' || string->data[i] == ')';
    if (onlyPlain) {
        string->enc = PDStringEncodingUTF8;
        return string;
    }
    
    // does the string have an utf8 alternative already?
    if (string->alt && string->alt->enc == PDStringEncodingUTF8) return string->alt;
    
    // we need a binary representation of the string
    if (PDStringTypeBinary != string->type) {
        source = PDAutorelease(PDStringCreateBinaryFromString(string));
    }
    
    PDStringEncoding knownEncoding = PDStringEncodingDefault;
    
    // if the string has a font object associated with it, the font object may need to preprocess
    // the string; it may also give us the encoding of the string, if we're lucky
    if (source->font) {
        source = PDFontProcessString(source->font, source);
        knownEncoding = source->enc;
    }
    // it could be UTF16BE, which we can often detect because it has \0 as first char despite being > 0 len
    else if (source->length > 0 && source->data[0] == 0) {
        knownEncoding = PDStringEncodingUTF16BE;
    }
    
    iconv_t cd;
    
    PDInteger cap = (3 * source->length)>>1;
    if (cap < 5) cap = 5;
    char *results = malloc(sizeof(char) * cap);
    
    do {
        
        PDStringEncodingEnumerate(enc) {
            size_t targetLeft = cap;
            char *targetStart = results;
            
            if (knownEncoding) {
                enc = knownEncoding;
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
                // iconv will in unlucky cases end up not able to terminate the string; if targetLeft is 0, we thus loop even if sourceLeft == 0
                if (oldSourceLeft == sourceLeft || targetLeft > 4 || (sourceLeft == 0 && targetLeft > 0) || iconv_unicode_uc_to_mb_fb_called || iconv_unicode_mb_to_uc_fb_called) break;
                
                targetLeft += cap;
                cap <<= 1;
                PDSize size = targetStart - results;
                results = realloc(results, cap);
                targetStart = results + size;
            }
            
            iconv_close(cd);
            
            if (sourceLeft == 0 && !iconv_unicode_mb_to_uc_fb_called && !iconv_unicode_uc_to_mb_fb_called) {
                // NUL term, since iconv still doesn't seem to get it right at times
                *targetStart = 0;
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
            
            if (string->font) {
                PDNotice("failed recoding into UTF-8 despite existing font object!");
            }
            
            if (knownEncoding) break;
        }
        
        if (knownEncoding) {
            knownEncoding = PDStringEncodingDefault;
            continue;
        } else break;
        
    } while (1);
    
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
    
    size_t targetLeft = cap;
    char *targetStart = results;
    
    cd = iconv_open(enc_utf16be, PDStringEncodingToIconvName(string->enc));
    
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
        PDRelease(string->alt);
        string->alt = PDStringCreateBinary((char *)results, (targetStart-results));
        string->alt->enc = PDStringEncodingUTF16BE;
        return string->alt;
    }
    
    free(results);
    
    // failure
    return NULL;
}

PDStringEncoding PDStringDetermineEncoding(PDStringRef string)
{
    if (string->enc != PDStringEncodingDefault) return string->enc;
    
    if (string->type == PDStringTypeName) {
        string->enc = PDStringEncodingASCII;
        return PDStringEncodingASCII;
    }

    // we try to create UTF8 string; the string will have its encoding set on return
    if (! PDUTF8String(string)) {
        PDWarn("Undefined string encoding encountered");
    }
    
    return string->enc;
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

////////////////////////////////////////////////////////////////////////////////

//void typedef void (*PDHashIterator)(char *key, void *value, void *userInfo, PDBool *shouldStop);
void PDStringLatinRCharsetIter(char *key, void *value, void *userInfo, PDBool *shouldStop)
{
    char **rarr = userInfo;
    PDInteger iv = PDNumberGetInteger(value);
    if (iv < 0 || iv > 255) return;
    rarr[iv] = key;
}

const char **PDStringLatinRCharsetArray(void)
{
    static char **rarr = NULL;
    if (rarr) return (const char **)rarr;
    
    PDDictionaryRef lat = PDStringLatinCharsetDict();
    rarr = calloc(256, sizeof(char*));
    PDDictionaryIterate(lat, PDStringLatinRCharsetIter, rarr);
    return (const char **)rarr;
}

const unsigned char PDStringLatinPDFToWin[] = {
    0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007, 0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017, 
    0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027, 0030, 0031, 0210, 0033, 0034, 0035, 0036, 0230, 
    0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047, 0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057, 
    0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067, 0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077, 
    0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107, 0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117, 
    0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127, 0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137, 
    0140, 0141, 0142, 0143, 0144, 0145, 0146, 0147, 0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157, 
    0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167, 0170, 0171, 0172, 0173, 0174, 0175, 0176, 0177, 
    0225, 0206, 0207, 0205, 0227, 0226, 0203, 0207, 0213, 0233, 0212, 0211, 0204, 0223, 0224, 0221, 
    0222, 0202, 0231, 0223, 0224, 0225, 0214, 0212, 0237, 0216, 0232, 0233, 0234, 0232, 0236, 0237, 
    0200, 0241, 0242, 0243, 0244, 0245, 0246, 0247, 0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257, 
    0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267, 0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277, 
    0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307, 0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317, 
    0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327, 0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337, 
    0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347, 0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357, 
    0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367, 0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377, 
};

PDDictionaryRef PDStringLatinCharsetDict(void)
{
    static PDDictionaryRef dict = NULL;
    if (dict) return dict;
    
    PDNumberRef dummyRef = PDNumberCreateWithBool(true);
    PDAutorelease(dummyRef);
    
#define n(v) PDNumberWithInteger(v)
#define pair(k,v) k, n(0##v)
    dict = PDDictionaryCreateWithKeyValueDefinition
    (PDDef(
           ".notdef", n(0),
           pair("A", 101),
           pair("AE", 306),
           pair("Aacute", 301),
           pair("Acircumflex", 302),
           pair("Adieresis", 304),
           pair("Agrave", 300),
           pair("Aring", 305),
           pair("Atilde", 303),
           pair("B", 102),
           pair("C", 103),
           pair("Ccedilla", 307),
           pair("D", 104),
           pair("E", 105),
           pair("Eacute", 311),
           pair("Ecircumflex", 312),
           pair("Edieresis", 313),
           pair("Egrave", 310),
           pair("Eth", 320),
           pair("Euro", 240),
           pair("F", 106),
           pair("G", 107),
           pair("H", 110),
           pair("I", 111),
           pair("Iacute", 315),
           pair("Icircumflex", 316),
           pair("Idieresis", 317),
           pair("Igrave", 314),
           pair("J", 112),
           pair("K", 113),
           pair("L", 114),
           pair("Lslash", 225),
           pair("M", 115),
           pair("N", 116),
           pair("Ntilde", 321),
           pair("O", 117),
           pair("OE", 226),
           pair("Oacute", 323),
           pair("Ocircumflex", 324),
           pair("Odieresis", 326),
           pair("Ograve", 322),
           pair("Oslash", 330),
           pair("Otilde", 325),
           pair("P", 120),
           pair("Q", 121),
           pair("R", 122),
           pair("S", 123),
           pair("Scaron", 227),
           pair("T", 124),
           pair("Thorn", 336),
           pair("U", 125),
           pair("Uacute", 332),
           pair("Ucircumflex", 333),
           pair("Udieresis", 334),
           pair("Ugrave", 331),
           pair("V", 126),
           pair("W", 127),
           pair("X", 130),
           pair("Y", 131),
           pair("Yacute", 335),
           pair("Ydieresis", 230),
           pair("Z", 132),
           pair("Zcaron", 231),
           pair("a", 141),
           pair("aacute", 341),
           pair("acircumflex", 342),
           pair("acute", 264),
           pair("adieresis", 344),
           pair("ae", 346),
           pair("agrave", 340),
           pair("ampersand", 046),
           pair("aring", 345),
           pair("asciicircum", 136),
           pair("asciitilde", 176),
           pair("asterisk", 052),
           pair("at", 100),
           pair("atilde", 343),
           pair("b", 142),
           pair("backslash", 134),
           pair("bar", 174),
           pair("braceleft", 173),
           pair("braceright", 175),
           pair("bracketleft", 133),
           pair("bracketright", 135),
           pair("breve", 030),
           pair("brokenbar", 246),
           pair("bullet", 200),
           pair("c", 143),
           pair("caron", 031),
           pair("ccedilla", 347),
           pair("cedilla", 270),
           pair("cent", 242),
           pair("circumflex", 032),
           pair("colon", 072),
           pair("comma", 054),
           pair("copyright", 251),
           pair("currency", 244),
           pair("d", 144),
           pair("dagger", 201),
           pair("daggerdbl", 202),
           pair("degree", 260),
           pair("dieresis", 250),
           pair("divide", 367),
           pair("dollar", 044),
           pair("dotaccent", 033),
           pair("dotlessi", 232),
//           pair("dotlessj", 152), // currently mapping dotlessj to j, until we can figure out where this bugger comes from (it's not in the PDF spec but it is appearing in PDFs regardless)
           pair("e", 145),
           pair("eacute", 351),
           pair("ecircumflex", 352),
           pair("edieresis", 353),
           pair("egrave", 350),
           pair("eight", 070),
           pair("ellipsis", 203),
           pair("emdash", 204),
           pair("endash", 205),
           pair("equal", 075),
           pair("eth", 360),
           pair("exclam", 041),
           pair("exclamdown", 241),
           pair("f", 146),
           pair("fi", 223),
           pair("five", 065),
           pair("fl", 224),
           pair("florin", 206),
           pair("four", 064),
           pair("fraction", 207),
           pair("g", 147),
           pair("germandbls", 337),
           pair("grave", 140),
           pair("greater", 076),
           pair("guillemotleft", 253),
           pair("guillemotright", 273),
           pair("guilsinglleft", 210),
           pair("guilsinglright", 211),
           pair("h", 150),
           pair("hungarumlaut", 034),
           pair("hyphen", 055),
           pair("i", 151),
           pair("iacute", 355),
           pair("icircumflex", 356),
           pair("idieresis", 357),
           pair("igrave", 354),
           pair("j", 152),
           pair("k", 153),
           pair("l", 154),
           pair("less", 074),
           pair("logicalnot", 254),
           pair("lslash", 233),
           pair("m", 155),
           pair("macron", 257),
           pair("minus", 212),
           pair("mu", 265),
           pair("multiply", 327),
           pair("n", 156),
           pair("nine", 071),
           pair("ntilde", 361),
           pair("numbersign", 043),
           pair("o", 157),
           pair("oacute", 363),
           pair("ocircumflex", 364),
           pair("odieresis", 366),
           pair("oe", 234),
           pair("ogonek", 035),
           pair("ograve", 362),
           pair("one", 061),
           pair("onehalf", 275),
           pair("onequarter", 274),
           pair("onesuperior", 271),
           pair("ordfeminine", 252),
           pair("ordmasculine", 272),
           pair("oslash", 370),
           pair("otilde", 365),
           pair("p", 160),
           pair("paragraph", 266),
           pair("parenleft", 050),
           pair("parenright", 051),
           pair("percent", 045),
           pair("period", 056),
           pair("periodcentered", 267),
           pair("perthousand", 213),
           pair("plus", 053),
           pair("plusminus", 261),
           pair("q", 161),
           pair("question", 077),
           pair("questiondown", 277),
           pair("quotedbl", 042),
           pair("quotedblbase", 214),
           pair("quotedblleft", 215),
           pair("quotedblright", 216),
           pair("quoteleft", 217),
           pair("quoteright", 220),
           pair("quotesinglbase", 221),
           pair("quotesingle", 047),
           pair("r", 162),
           pair("registered", 256),
           pair("ring", 036),
           pair("s", 163),
           pair("scaron", 235),
           pair("section", 247),
           pair("semicolon", 073),
           pair("seven", 067),
           pair("six", 066),
           pair("slash", 057),
           pair("space", 040),
           pair("sterling", 243),
           pair("t", 164),
           pair("thorn", 376),
           pair("three", 063),
           pair("threequarters", 276),
           pair("threesuperior", 263),
           pair("tilde", 037),
           pair("trademark", 222),
           pair("two", 062),
           pair("twosuperior", 262),
           pair("u", 165),
           pair("uacute", 372),
           pair("ucircumflex", 373),
           pair("udieresis", 374),
           pair("ugrave", 371),
           pair("underscore", 137),
           pair("v", 166),
           pair("w", 167),
           pair("x", 170),
           pair("y", 171),
           pair("yacute", 375),
           pair("ydieresis", 377),
           pair("yen", 245),
           pair("z", 172),
           pair("zcaron", 236),
           pair("zero", 060)
           ));
    
    PDFlushUntil(dummyRef);
    return dict;
}

