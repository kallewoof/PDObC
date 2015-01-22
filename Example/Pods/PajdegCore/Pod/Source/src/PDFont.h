//
// PDFont.h
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
 @file PDFont.h PDF font object
 
 @ingroup PDFONTS
 
 A font representation.
 
 @{
 */

#ifndef INCLUDED_PDFont_h
#define INCLUDED_PDFont_h

#include "PDDefines.h"

/**
 *  Create a PDFont object from a font object in a PDF dictionary.
 *
 *  @param parser   Parser from which to fetch content necessary to generate the font object
 *  @param fontDict Font dictionary owning the font object
 *  @param obj      Font object in PDF dictionary
 *
 *  @return A new PDFont object
 */
extern PDFontRef PDFontCreate(PDParserRef parser, PDFontDictionaryRef fontDict, PDObjectRef obj);

/**
 *  Get the name of the encoding used for the font.
 *
 *  @param font The font object
 *
 *  @return The string value of the encoding (e.g. /Identity-H or /WinAnsiEncoding) or NULL if undefined
 */
extern PDStringRef PDFontGetEncodingName(PDFontRef font);

/**
 *  Get the encoding used for the font.
 *
 *  @param font The font object
 *
 *  @return The PDStringEncoding value of the encoding, or PDStringEncodingUndefined if undefined
 */
extern PDStringEncoding PDFontGetEncoding(PDFontRef font);

/**
 *  Process the given string, applying ToUnicode CMap (if applicable), encMap
 *  byte mapping (if applicable), etc., and return the result as a new, auto-
 *  released string. If no applicable mappings are available, the string is 
 *  returned as is. 
 *  If possible, the string's enc value is defined in the process.
 *
 *  @param font   The font object
 *  @param string String to process
 *
 *  @return The same string, or a new string that has been processed
 */
extern PDStringRef PDFontProcessString(PDFontRef font, PDStringRef string);

#endif // INCLUDED_PDFont_h
