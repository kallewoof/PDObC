//
// PDScanner.h
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
 @file PDScanner.h Scanner header file.
 
 @ingroup PDSCANNER
 
 @defgroup PDSCANNER PDScanner
 
 @brief The generic scanner used to read symbols from a stream or arbitrary buffer.
 
 @ingroup PDSCANNER_CONCEPT
 
 The Pajdeg scanner takes a PDStateRef state and optionally a PDScannerPopFunc and allows the interpretation of symbols as defined by the state and its sub-states.
 
 The most public functions of the scanner are PDScannerPopString and PDScannerPopStack. The former attempts to retrieve a string from the input stream, and the other a pd_stack. If the next value scanned is not the requested type, the function keeps the value around and returns falsity. It is not uncommon to attempt to pop a stack, and upon failure, to pop a string and behave accordingly.

 @{
 */

#ifndef INCLUDED_PDScanner_h
#define INCLUDED_PDScanner_h

#include <sys/types.h>
#include "PDDefines.h"

/// @name Creating / deleting scanners

/**
 Create a scanner using the default pop function.
 
 @param state The root state to use in the scanner.
 */
extern PDScannerRef PDScannerCreateWithState(PDStateRef state);

/**
 Create a scanner using the provided pop function.
 
 @param state The root state to use in the scanner.
 @param popFunc The pop function to use.
 */
extern PDScannerRef PDScannerCreateWithStateAndPopFunc(PDStateRef state, PDScannerPopFunc popFunc);

/**
 Attach a fixed-size buffer to the scanner. The scanner will refuse to use the buffering function, if one is present.
 
 @param scanner The scanner.
 @param buf The buffer.
 @param len The length of the buffer.
 */
extern void PDScannerAttachFixedSizeBuffer(PDScannerRef scanner, char *buf, PDInteger len);

/// @name Using

/**
 *  Set up a temporary scanner with the given root state, and process buf, returning a pd_stack entry after tearing down the scanner again.
 *
 *  @param state The root state
 *  @param buf   Buffer
 *  @param len   Length of buffer in bytes
 *
 *  @return pd_stack entry or NULL on failure
 */
extern pd_stack PDScannerGenerateStackFromFixedBuffer(PDStateRef state, char *buf, PDInteger len);

/**
 Pop the next string. 
 
 @param scanner The scanner
 @param value Pointer to string variable. Must be freed
 @return true if the next value was a string
 */
extern PDBool PDScannerPopString(PDScannerRef scanner, char **value);

/**
 *  Pop the next stack.
 *
 *  @param scanner The scanner
 *  @param value   Pointer to stack ref. Must be freed
 *
 *  @return true if the next value was a stack
 */
extern PDBool PDScannerPopStack(PDScannerRef scanner, pd_stack *value);

/**
 *  Pop the next value, which the scanner was not able to recognize.
 *
 *  @param scanner The scanner
 *  @param value   Pointer to string variable. Must be freed
 *
 *  @return true if the next value was an unrecognizable string
 */
extern PDBool PDScannerPopUnknown(PDScannerRef scanner, char **value);

/**
 *  Determine if the scanner has reached the end of the stream
 *
 *  @param scanner Scanner object
 *
 *  @return Boolean value indicating whether the stream hit the end or not
 */
extern PDBool PDScannerEndOfStream(PDScannerRef scanner);

/**
 Require that the next result is a string, and that it is equal to the given value, or throw assertion.

 @param scanner The scanner.
 @param value Expected string.
 */
extern void PDScannerAssertString(PDScannerRef scanner, char *value);

// 
/**
 Require that the next result is a stack (the stack is discarded), or throw assertion.

 @param scanner The scanner.
 */
extern void PDScannerAssertStackType(PDScannerRef scanner);

/**
 Require that the next result is a complex of the given type, or throw assertion.

 @param scanner The scanner.
 @param identifier The identifier.
 */
extern void PDScannerAssertComplex(PDScannerRef scanner, const char *identifier);

/// @name Raw streams

/**
 Skip over a chunk of data internally, usually a PDF stream.
 
 @note The stream is not iterated, only the scanner's internal buffer offset is.
 
 @param scanner The scanner.
 @param bytes The amount of bytes to skip.
 */
extern void PDScannerSkip(PDScannerRef scanner, PDSize bytes);

/**
 Skip until the given symbol character type is encountered, stopping after the symbol character type.
 
 @note Buffer growth is never done in this method, which means if the scanner's buffer is only partially complete, it may stop prematurely.
 
 @param scanner The scanner.
 @param symbolCharType The symbol character type.
 @return The number of bytes skipped.
 */
extern PDInteger PDScannerPassSymbolCharacterType(PDScannerRef scanner, PDInteger symbolCharType);

/**
 Attach a stream filter to the scanner. 
 
 Stream filters are used to e.g. compress/decompress or encrypt/decrypt binary content.
 
 @param scanner The scanner.
 @param filter The filter.
 */
extern PDBool PDScannerAttachFilter(PDScannerRef scanner, PDStreamFilterRef filter);

/**
 Detach attached stream filter from the scanner.
 
 @param scanner The scanner.
 */
extern void PDScannerDetachFilter(PDScannerRef scanner);

/**
 Read parts or entire stream at current position via attached filter.
 
 Iterates scanner and stream (contrary to PDScannerSkip above, which only iterates scanner).
 
 @note If a decompression filter is attached, not all data may be read at once due to capacity limitations, and may require calls to PDScannerReadStreamNext().
 
 @see PDScannerAttachFilter
 @see PDScannerReadStreamNext
 
 @param scanner The scanner.
 @param bytes The number of raw bytes to read.
 @param dest The destination buffer.
 @param capacity The capacity of the destination buffer.
 @return The number of bytes stored in dest. If this value is equal to capacity, there may be more data available via PDScannerReadStreamNext.
 */
extern PDInteger PDScannerReadStream(PDScannerRef scanner, PDInteger bytes, char *dest, PDInteger capacity);

/**
 Continue reading stream data via attached filter.
 
 @see PDScannerAttachFilter
 @see PDScannerReadStream
 
 @param scanner The scanner.
 @param dest The destination buffer.
 @param capacity The capacity of the destination buffer.
 @return The number of bytes stored in dest.
 */
extern PDInteger PDScannerReadStreamNext(PDScannerRef scanner, char *dest, PDInteger capacity);

/// @name Adjusting scanner / source

/**
 *  Push a new buffer context onto a scanner, keeping the old one on a stack.
 *
 *  @param scanner    Scanner
 *  @param ctxInfo    Info object for the buffer function
 *  @param ctxBufFunc Buffer function
 */
extern void PDScannerPushContext(PDScannerRef scanner, void *ctxInfo, PDScannerBufFunc ctxBufFunc);

/**
 *  Pop the scanner's current context.
 *
 *  @param scanner Scanner
 */
extern void PDScannerPopContext(PDScannerRef scanner);

/**
 Set a cap on # of loops scanners make before considering a pop a failure.
 
 This is used when reading a PDF for the first time to not scan through the entire thing backwards looking for the startxref entry.
 
 The loop cap is reset after every successful pop.
 
 @param cap The cap.
 */
extern void PDScannerSetLoopCap(PDInteger cap);

/**
 Pop a symbol as normal, via forward reading of buffer.
 
 @param scanner The scanner.
 
 @see PDScannerCreateWithStateAndPopFunc
 */
extern void PDScannerPopSymbol(PDScannerRef scanner);

/**
 Pop a symbol reversedly, by iterating backward.

 @param scanner The scanner.
 
 @see PDScannerCreateWithStateAndPopFunc
 */
extern void PDScannerPopSymbolRev(PDScannerRef scanner);

/// @name Aligning, resetting, trimming scanner
//

/**
 Align buffer along with pointers with given offset (often negative).

 @param scanner The scanner.
 @param offset The offset.
 */
extern void PDScannerAlign(PDScannerRef scanner, PDOffset offset); 

/**
 Trim off of head from buffer (only used if buffer is a non-allocated pointer into a heap).
 
 @param scanner The scanner.
 @param bytes Bytes to trim off.
 */
extern void PDScannerTrim(PDScannerRef scanner, PDOffset bytes); 

/**
 Reset scanner buffer including size, offset, trail, etc., as well as discarding symbols and results.
 
 @param scanner The scanner.
 */
extern void PDScannerReset(PDScannerRef scanner);


/// @name Debugging

/**
 Print a trace of states to stdout for the scanner. 
 
 @param scanner The scanner.
 */
extern void PDScannerPrintStateTrace(PDScannerRef scanner);

#endif

/** @} */

/** @} */
