//
// PDOperator.h
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
 @file PDOperator.h Operator header file.
 
 @ingroup PDOPERATOR

 @defgroup PDOPERATOR PDOperator
 
 @ingroup PDSCANNER_CONCEPT
 
 @brief An operator for a given symbol or outcome in a state.
 
 Operators are either tied to some symbol in a state, or to one of the fallback types (numeric, delimiter, and the all-accepting). They define some form of action that should occur, and may optionally include a "next" operator, which is executed directly following the completion of the current operator's action.
 
 @note If an operator pushes a state, its action will be regarded as ongoing until the pushed state is poppsed again.
 
 @{
 */

#ifndef INCLUDED_PDOperator_h
#define INCLUDED_PDOperator_h

#include "PDDefines.h"

#define PDOperatorSymbolGlobRegular     0   ///< PDF regular character
#define PDOperatorSymbolGlobWhitespace  1   ///< PDF whitespace character
#define PDOperatorSymbolGlobDelimiter   4   ///< PDF delimiter character
#define PDOperatorSymbolExtNumeric      8   ///< Numeric value
#define PDOperatorSymbolExtFake         16  ///< "Fake" symbol
#define PDOperatorSymbolExtEOB          32  ///< End of buffer (usually synonymous with end of file) symbol

/**
 Global symbol table, a 256 byte mapping of ASCII characters to their corresponding PDF symbol type as defined in the PDF specification (v 1.7). 
 */
extern char *PDOperatorSymbolGlob;

/**
 Global HEX table, a 256 byte mapping of ASCII characters to their corresponding numeric values according to the hexademical system.
 */
extern char *PDOperatorSymbolGlobHex;

/**
 Global de-HEX table, a 16 (NOT 256) byte mapping of 0 through 15, where 0..9 return '0'..'9' and 10..15 return 'A'..'F'
 */
extern char *PDOperatorSymbolGlobDehex;

/**
 Global escape table, a 256 byte mapping of ASCII characters to 1 for "no escape", 0 for "escape as code", and a character for "escape using this escape code".
 */
extern char *PDOperatorSymbolGlobEscaping;

/**
 Set up the global symbol table.
 */
extern void PDOperatorSymbolGlobSetup();

/**
 Define the given symbol. Definitions detected are delimiters, numeric (including real) values, and (regular) symbols.
 */
extern char PDOperatorSymbolGlobDefine(char *str);

/**
 Create a PDOperatorRef chain based on a definition in the form of NULL terminated arrays of operator types followed by (if any) arguments of corresponding types.
 
 @param defs Definitions, null terminated.
 */
extern PDOperatorRef PDOperatorCreateFromDefinition(const void **defs);

/**
 Compile all states referenced by the operator.
 
 @see PDStateCompile
 
 @param op The operator.
 */
extern void PDOperatorCompileStates(PDOperatorRef op);

/**
 Update the numeric and real booleans based on the character.
 
 @param numeric The numeric boolean.
 @param real The real boolean.
 @param c The character.
 @param first_character Whether or not this is the first character.
 */
#define PDSymbolUpdateNumeric(numeric, real, c, first_character) \
    numeric &= ((c >= '0' && c <= '9') \
                || \
                (first_character && (c == '-' || c == '+')) \
                || \
                (! real && (real |= c == '.')))

/**
 Set numeric and real for the given symbol over the given length.
 
 @param numeric The numeric boolean.
 @param real The real boolean.
 @param c The character.
 @param sym The symbol buffer.
 @param len The length of the symbol.
 */
#define PDSymbolDetermineNumeric(numeric, real, c, sym, len) \
    do {                                          \
        PDInteger i;                              \
        for (i = len-1; numeric && i >= 0; i--) { \
            c = sym[i];                           \
            PDSymbolUpdateNumeric(numeric, real, c, i==0); \
        }                                         \
    } while (0)

#endif

/** @} */
