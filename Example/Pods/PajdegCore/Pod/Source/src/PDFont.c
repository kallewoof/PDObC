//
// PDFont.c
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
#include "PDFont.h"
#include "PDFontDictionary.h"
#include "PDCMap.h"
#include "PDDictionaryStack.h"
#include "pd_internal.h"

void PDFontCompileUnicodeMapping(PDFontRef font);
void PDFontCompileEncoding(PDFontRef font, PDDictionaryRef encDict);

void PDFontDestroy(PDFontRef font)
{
    PDRelease(font->obj);
    PDRelease(font->toUnicode);
//    if (font->encMap) free(font->encMap);
}

PDFontRef PDFontCreate(PDParserRef parser, PDFontDictionaryRef fontDict, PDObjectRef obj)
{
    PDFontRef font = PDAllocTyped(PDInstanceTypeFont, sizeof(struct PDFont), PDFontDestroy, false);
    font->parser = parser;
    font->obj = PDRetain(obj);
    font->toUnicode = NULL;
    font->encMap = NULL;
    font->fontDict = fontDict; // unretained
    font->enc = PDStringEncodingDefault;
    
    PDFontCompileUnicodeMapping(font);
    
    return font;
}

PDStringRef PDFontGetEncodingName(PDFontRef font)
{
    void *encoding = PDDictionaryGet(PDObjectGetDictionary(font->obj), "Encoding");
    PDInstanceType it = PDResolve(encoding);
    if (encoding && it == PDInstanceTypeString) return encoding;
    
    // see if we've been here already
    if (font->encMap != NULL) return NULL;

    if (encoding && it != PDInstanceTypeString) {
        if (it == PDInstanceTypeRef) {
            encoding = PDAutorelease(PDParserLocateAndCreateObject(font->parser, PDReferenceGetObjectID(encoding), true));
            it = PDResolve(encoding);
        }
        if (it == PDInstanceTypeObj) {
            PDFontDictionaryApplyEncodingObject(font->fontDict, font, encoding);
            return NULL;
        }
        if (it == PDInstanceTypeString) {
            PDDictionarySet(PDObjectGetDictionary(font->obj), "Encoding", encoding);
        } else if (it == PDInstanceTypeDict) {
            PDFontDictionaryApplyEncodingDictionary(font->fontDict, font, encoding);
            encoding = NULL;
        } else {
            PDError("unknown instance type for encoding value in PDFontGetEncodingName");
        }
    }
    return encoding;
}

PDStringEncoding PDFontGetEncoding(PDFontRef font)
{
    if (font->enc != PDStringEncodingDefault) return font->enc;

    font->enc = PDStringEncodingCP1252;
    PDStringRef encName = PDFontGetEncodingName(font);
    if (encName) {
        font->enc = PDStringEncodingGetByName(PDStringBinaryValue(encName, NULL));
    } else if (font->encMap != NULL) {
        // the font maps to a custom encoding
        font->enc = PDStringEncodingCustom;
    }

    return font->enc;
}

void PDFontCompileUnicodeMapping(PDFontRef font)
{
    PDReferenceRef toUnicodeRef = PDDictionaryGet(PDObjectGetDictionary(font->obj), "ToUnicode");
    if (toUnicodeRef) {
        PDObjectRef toUnicodeObj = PDParserLocateAndCreateObject(font->parser, PDReferenceGetObjectID(toUnicodeRef), true);
        const char *stream = PDParserLocateAndFetchObjectStreamForObject(font->parser, toUnicodeObj);
        if (stream) {
            font->toUnicode = PDCMapCreateWithData(toUnicodeObj->streamBuf, PDObjectGetExtractedStreamLength(toUnicodeObj));
        } else {
            PDError("NULL stream for ToUnicode object %ld 0 R", PDObjectGetObID(toUnicodeObj));
        }
        PDRelease(toUnicodeObj);
    }
}

PDStringRef PDFontProcessString(PDFontRef font, PDStringRef string)
{
    PDStringRef source = string;
    
    // is there a toUnicode mapping? if so we can get a UTF16BE from it
    if (font->toUnicode) {
        source = PDCMapApply(source->font->toUnicode, source);
        source->enc = PDStringEncodingUTF16BE;
        return source;
    }

    // is there an encMap?
    if (font->encMap) {
        unsigned char *em = font->encMap;
        PDStringRef mapped = PDStringCreateFromStringWithType(source, PDStringTypeBinary, false, true);
        unsigned char *data = (unsigned char *)mapped->data;
        PDSize length = mapped->length;
        for (PDSize i = 0; i < length; i++) {
            data[i] = em[data[i]];
        }
        mapped->enc = PDStringEncodingCP1252;
        return PDAutorelease(mapped);
    }
    
    return string;
}
