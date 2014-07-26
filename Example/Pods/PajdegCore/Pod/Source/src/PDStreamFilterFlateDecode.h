//
// PDStreamFilterFlateDecode.h
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

/**
 @file PDStreamFilterFlateDecode.h
 
 @ingroup PDSTREAMFILTERFLATEDECODE
 
 @defgroup PDSTREAMFILTERFLATEDECODE PDStreamFilterFlateDecode
 
 @brief Flate Decode (compression/decompression) stream filter
 
 @ingroup PDINTERNAL

 PDF streams are often compressed using FlateDecode. Normally this isn't a problem unless the user needs to change or insert stream content in a stream that requires compression, or, more importantly, when a PDF is using the 1.5+ object stream feature for the XREF table, and is not providing a fallback option for 1.4- readers.
 
 @warning This filter depends on the zlib library. It is enabed through the PD_SUPPORT_ZLIB define in PDDefines.h. Disabling this feature means zlib is not required for Pajdeg, but means that some PDFs (1.5 with no fallback) will not be parsable via Pajdeg!
 
 @see PDDefines.h
 
 @{
 */

#ifndef INCLUDED_PDStreamFilterFlateDecode_h
#define INCLUDED_PDStreamFilterFlateDecode_h

#include "PDStreamFilter.h"

#ifdef PD_SUPPORT_ZLIB

/**
 Set up a stream filter for FlateDecode compression.
 */
extern PDStreamFilterRef PDStreamFilterFlateDecodeCompressCreate(PDDictionaryRef options);

/**
 Set up stream filter for FlateDecode decompression.
 */
extern PDStreamFilterRef PDStreamFilterFlateDecodeDecompressCreate(PDDictionaryRef options);

/**
 Set up a stream filter for FlateDecode based on inputEnd boolean. 
 */
extern PDStreamFilterRef PDStreamFilterFlateDecodeConstructor(PDBool inputEnd, PDDictionaryRef options);

#endif

#endif

/** @} */

