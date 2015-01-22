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

#include "Pajdeg.h"
#include "PDFontDictionary.h"
#include "pd_internal.h"
#include "PDParser.h"
#include "PDFont.h"

void PDFontDictionaryDestroy(PDFontDictionaryRef fontDict)
{
    PDRelease(fontDict->fonts);
    PDRelease(fontDict->parser);
    PDRelease(fontDict->encodings);
}

PDFontDictionaryRef PDFontDictionaryCreate(PDParserRef parser, PDObjectRef pageObject)
{
    PDFontDictionaryRef fontDict = PDAllocTyped(PDInstanceTypeFontDict, sizeof(struct PDFontDictionary), PDFontDictionaryDestroy, false);
    fontDict->fonts = NULL;
    fontDict->parser = PDRetain(parser);
    
    if (pageObject) {
    
        fontDict->encodings = PDRetain(parser->mfd->encodings);
        
        void *resourcesValue = PDDictionaryGet(PDObjectGetDictionary(pageObject), "Resources");
        
        if (resourcesValue) {
            PDDictionaryRef resourcesDict = NULL;
            PDInstanceType it = PDResolve(resourcesValue);
            if (it == PDInstanceTypeRef) {
                PDObjectRef resources = PDParserLocateAndCreateObject(parser, PDReferenceGetObjectID(resourcesValue), true);
                resourcesDict = PDObjectGetDictionary(resources);
                PDRelease(resources); // this is okay, because all parser generated objects survive until the parser dies, thus resourcesDict will survive
            } else if (it == PDInstanceTypeDict) {
                resourcesDict = resourcesValue;
            } else {
                PDError("unrecognized resource value %s", PDDescription(resourcesValue));
            }
            if (resourcesDict) {
                void *fonts = PDDictionaryGet(resourcesDict, "Font");
                if (PDResolve(fonts) == PDInstanceTypeRef) {
                    PDObjectRef fontObj = PDParserLocateAndCreateObject(parser, PDReferenceGetObjectID(fonts), true);
                    fontDict->fonts = PDRetain(PDObjectGetDictionary(fontObj));
                    PDRelease(fontObj);
                } else {
                    fontDict->fonts = PDRetain(fonts);
                }
            }
        }
        
    } else {
        // this is the master font dictionary
        fontDict->fonts = PDDictionaryCreate();
        fontDict->encodings = PDDictionaryCreate();
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
    
    // check master font dictionary
    void *mfont = PDDictionaryGet(fontDict->parser->mfd->fonts, name);
    if (mfont) {
        PDDictionarySet(fontDict->fonts, name, mfont);
        return mfont;
    }
    
    PDObjectRef obj = PDParserLocateAndCreateObject(fontDict->parser, PDReferenceGetObjectID(font), true);
    PDRequire(obj, NULL, "NULL object for font dictionary entry for %s", name);
    
    PDFontRef fontObj = PDFontCreate(fontDict->parser, fontDict, obj);
    PDRequire(fontObj, NULL, "Failed to generate font object for font dictionary entry for %s", name);
    PDDictionarySet(fontDict->parser->mfd->fonts, name, fontObj);
    PDDictionarySet(fontDict->fonts, name, fontObj);
    PDRelease(obj);
    PDRelease(fontObj);
    
    return fontObj;
}

void PDFontDictionaryApplyEncodingDictionary(PDFontDictionaryRef fontDict, PDFontRef font, PDDictionaryRef encoding)
{
    /*
<< /Type /Encoding 
   /Differences [ 24 /breve /caron /circumflex /dotaccent /hungarumlaut /ogonek /ring /tilde 
                  39 /quotesingle 
                  96 /grave
                  128 /bullet /dagger /daggerdbl /ellipsis /emdash /endash /florin /fraction /guilsinglleft /guilsinglright /minus /perthousand /quotedblbase /quotedblleft /quotedblright /quoteleft /quoteright /quotesinglbase /trademark /fi /fl /Lslash /OE /Scaron /Ydieresis /Zcaron /dotlessi /lslash /oe /scaron /zcaron 
                  164 /currency
                  166 /brokenbar
                  168 /dieresis /copyright /ordfeminine 
                  172 /logicalnot /.notdef /registered /macron /degree /plusminus /twosuperior /threesuperior /acute /mu
                  183 /periodcentered /cedilla /onesuperior /ordmasculine 
                  188 /onequarter /onehalf /threequarters
                  192 /Agrave /Aacute /Acircumflex /Atilde /Adieresis /Aring /AE /Ccedilla /Egrave /Eacute /Ecircumflex /Edieresis /Igrave /Iacute /Icircumflex /Idieresis /Eth /Ntilde /Ograve /Oacute /Ocircumflex /Otilde /Odieresis /multiply /Oslash /Ugrave /Uacute /Ucircumflex /Udieresis /Yacute /Thorn /germandbls /agrave /aacute /acircumflex /atilde /adieresis /aring /ae /ccedilla /egrave /eacute /ecircumflex /edieresis /igrave /iacute /icircumflex /idieresis /eth /ntilde /ograve /oacute /ocircumflex /otilde /odieresis /divide /oslash /ugrave /uacute /ucircumflex /udieresis /yacute /thorn /ydieresis ] >>
     */
    PDArrayRef darr = PDDictionaryGet(encoding, "Differences");
    if (PDResolve(darr) == PDInstanceTypeRef) {
        PDObjectRef darrOb = PDParserLocateAndCreateObject(fontDict->parser, PDReferenceGetObjectID((PDReferenceRef)darr), true);
        if (darrOb) {
            darr = PDObjectGetArray(darrOb);
        }
        PDRelease(darrOb); // okay because parser retains all created objects until the parser itself dies
    }
    if (darr) {
        unsigned char *em = font->encMap = malloc(256);
        memcpy(em, PDStringLatinPDFToWin, 256);
//        for (int i = 0; i < 256; i++) em[i] = i;
        
        PDDictionaryRef latin = PDStringLatinCharsetDict();
        
        PDInteger code = -1;
        PDInteger count = PDArrayGetCount(darr);
        for (PDInteger i = 0; i < count; i++) {
            void *v = PDArrayGetElement(darr, i);
            if (PDResolve(v) == PDInstanceTypeNumber) {
                // new code point; v is a number value
                code = PDNumberGetInteger(v);
            } else {
                // continuation; v is a name that should be placed into the code point
                PDAssert(PDResolve(v) == PDInstanceTypeString);
                PDNumberRef value = PDDictionaryGet(latin, PDStringBinaryValue(v, NULL));
                if (value) {
                    PDAssert(code >= 0 && code < 256);
                    em[code] = PDStringLatinPDFToWin[PDNumberGetInteger(value)];
                } else {
                    PDNotice("unknown latin name %s is ignored in encoding mapping", PDStringNameValue(v, false));
                }
                code++;
            }
        }
    }
}

extern void PDFontDictionaryApplyEncodingObject(PDFontDictionaryRef fontDict, PDFontRef font, PDObjectRef encodingOb)
{
    PDStringRef encMapString = PDDictionaryGet(fontDict->encodings, PDObjectGetReferenceString(encodingOb));
    if (encMapString) {
        font->encMap = (unsigned char *) encMapString->data;
        return;
    }
    
    PDDictionaryRef dict = PDObjectGetDictionary(encodingOb);
    if (dict) {
        PDFontDictionaryApplyEncodingDictionary(fontDict, font, dict);
        if (font->encMap) {
            encMapString = PDStringCreateBinary((char *)font->encMap, 256);
            PDDictionarySet(fontDict->encodings, PDObjectGetReferenceString(encodingOb), encMapString);
            PDRelease(encMapString);
        } else {
            PDWarn("no encMap after PDFontDictionaryApplyEncodingDictionary call");
        }
    } else {
        PDWarn("unknown encoding object %s", PDDescription(encodingOb));
    }
}
