//
// PDFontDictionary.h
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
 @file PDFontDictionary.h PDF font dictionary for a page or a set of pages.
 
 @ingroup PDFONTS
 
 Convenience methods for handling PDF font dictionaries.
 
 @{
 */

#ifndef INCLUDED_PDFontDictionary_h
#define INCLUDED_PDFontDictionary_h

#include "PDDefines.h"

/**
 *  Create a font dictionary for the specified page.
 *
 *  @param parser     Parser reference
 *  @param pageObject Page reference
 *
 *  @return A new font dictionary object.
 */
extern PDFontDictionaryRef PDFontDictionaryCreate(PDParserRef parser, PDObjectRef pageObject);

/**
 *  Get the font with the given name, or NULL if no such font exists in this dictionary.
 *
 *  @param fontDict The font dictionary
 *  @param name     The name of the font
 *
 *  @return PDFont object or NULL if no such font exists
 */
extern PDFontRef PDFontDictionaryGetFont(PDFontDictionaryRef fontDict, const char *name);

/**
 *  Apply the given encoding to the font.
 *  The reason why this is done in the font dictionary is because, often, several fonts inside the same
 *  font dictionary share the same encoding object. The font dictionary keeps track of all processed encodings,
 *  using cached values when the same encoding is encountered multiple times.
 *
 *  @param fontDict The font dictionary owning the font
 *  @param font     The font object
 *  @param encoding The encoding object
 */
extern void PDFontDictionaryApplyEncodingObject(PDFontDictionaryRef fontDict, PDFontRef font, PDObjectRef encodingOb);

/**
 *  Apply the given encoding to the font.
 *  Due to the missing object ID compared to PDFontDictionaryApplyEncodingObject, 
 *  this method does nothing practically useful except provide a convenient point
 *  for the actual processing of encodings.
 *
 *  @param fontDict The font dictionary owning the font
 *  @param font     The font object
 *  @param encoding The encoding dictionary
 */
extern void PDFontDictionaryApplyEncodingDictionary(PDFontDictionaryRef fontDict, PDFontRef font, PDDictionaryRef encoding);

#endif // INCLUDED_PDFontDictionary_h
