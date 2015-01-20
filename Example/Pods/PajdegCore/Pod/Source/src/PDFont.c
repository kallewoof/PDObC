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
#include "PDCMap.h"
#include "pd_internal.h"

void PDFontCompileUnicodeMapping(PDFontRef font);

void PDFontDestroy(PDFontRef font)
{
    PDRelease(font->parser);
    PDRelease(font->obj);
    PDRelease(font->toUnicode);
}

PDFontRef PDFontCreate(PDParserRef parser, PDObjectRef obj)
{
    PDFontRef font = PDAllocTyped(PDInstanceTypeFont, sizeof(struct PDFont), PDFontDestroy, false);
    font->parser = PDRetain(parser);
    font->obj = PDRetain(obj);
    font->toUnicode = NULL;
    font->enc = PDStringEncodingDefault;
    PDFontCompileUnicodeMapping(font);
    
    return font;
}

PDStringRef PDFontGetEncodingName(PDFontRef font)
{
    return PDDictionaryGet(PDObjectGetDictionary(font->obj), "Encoding");
}

PDStringEncoding PDFontGetEncoding(PDFontRef font)
{
    if (font->enc != PDStringEncodingDefault) return font->enc;

    font->enc = PDStringEncodingUndefined;
    PDStringRef encName = PDFontGetEncodingName(font);
    if (encName) font->enc = PDStringEncodingGetByName(PDStringBinaryValue(encName, NULL));

    return font->enc;
}

void PDFontCompileUnicodeMapping(PDFontRef font)
{
    PDReferenceRef toUnicodeRef = PDDictionaryGet(PDObjectGetDictionary(font->obj), "ToUnicode");
    if (toUnicodeRef) {
        PDObjectRef toUnicodeObj = PDParserLocateAndCreateObject(font->parser, PDReferenceGetObjectID(toUnicodeRef), true);
        char *stream = PDParserLocateAndFetchObjectStreamForObject(font->parser, toUnicodeObj);
        if (stream) {
            font->toUnicode = PDCMapCreateWithData(stream, PDObjectGetExtractedStreamLength(toUnicodeObj));
        } else {
            PDError("NULL stream for ToUnicode object %ld 0 R", PDObjectGetObID(toUnicodeObj));
        }
        PDRelease(toUnicodeObj);
    }
}
