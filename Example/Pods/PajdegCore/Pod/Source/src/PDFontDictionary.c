//
// PDFontDictionary.c
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

#include "PDFontDictionary.h"
#include "pd_internal.h"
#include "PDDictionary.h"
#include "PDObject.h"
#include "PDParser.h"
#include "PDReference.h"
#include "PDFont.h"

void PDFontDictionaryDestroy(PDFontDictionaryRef fontDict)
{
    PDRelease(fontDict->fonts);
    PDRelease(fontDict->parser);
}

PDFontDictionaryRef PDFontDictionaryCreate(PDParserRef parser, PDObjectRef pageObject)
{
    PDFontDictionaryRef fontDict = PDAllocTyped(PDInstanceTypeFontDict, sizeof(struct PDFontDictionary), PDFontDictionaryDestroy, false);
    fontDict->fonts = NULL;
    fontDict->parser = PDRetain(parser);
    
    PDReferenceRef resourcesRef = PDDictionaryGet(PDObjectGetDictionary(pageObject), "Resources");
    
    if (resourcesRef) {
        PDObjectRef resources = PDParserLocateAndCreateObject(parser, PDReferenceGetObjectID(resourcesRef), true);
        fontDict->fonts = PDRetain(PDDictionaryGet(PDObjectGetDictionary(resources), "Font"));
        PDRelease(resources);
    }
    
    return fontDict;
}

PDFontRef PDFontDictionaryGetFont(PDFontDictionaryRef fontDict, const char *name)
{
    if (NULL == fontDict->fonts) return NULL;
    
    void *font = PDDictionaryGet(fontDict->fonts, name);
    PDInstanceType it = PDResolve(font);
    if (! font || PDInstanceTypeFont == it) return font;
    PDRequire(PDInstanceTypeRef == it, NULL, "Invalid instance type for font dictionary entry for %s: %d (expected %d)", name, it, PDInstanceTypeRef);
    
    PDObjectRef obj = PDParserLocateAndCreateObject(fontDict->parser, PDReferenceGetObjectID(font), true);
    PDRequire(obj, NULL, "NULL object for font dictionary entry for %s", name);
    
    PDFontRef fontObj = PDFontCreate(fontDict->parser, obj);
    PDRequire(fontObj, NULL, "Failed to generate font object for font dictionary entry for %s", name);
    PDDictionarySet(fontDict->fonts, name, fontObj);
    PDRelease(obj);
    PDRelease(fontObj);
    
    return fontObj;
}
