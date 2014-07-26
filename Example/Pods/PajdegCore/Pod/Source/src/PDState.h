//
// PDState.h
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
 @file PDState.h The state header.
 
 @ingroup PDSTATE

 @defgroup PDSTATE PDState
 
 @brief A state.
 
 @ingroup PDSCANNER_CONCEPT
 
 A state in Pajdeg is a definition of a given set of conditions.
 
 @see pd_pdf_implementation.h
 
 @{
 */

#ifndef INCLUDED_PDState_h
#define INCLUDED_PDState_h

#include "PDDefines.h"

/**
 Create a new state with the given name.
 
 @param name The name of the state, e.g. "dict".
 */
extern PDStateRef PDStateCreate(char *name);

/**
 Compile a state.
 
 @param state The state.
 */
extern void PDStateCompile(PDStateRef state);

/**
 Define state operators using a PDDef definition. 
 
 @param state The state.
 @param defs PDDef definitions.
 */
extern void PDStateDefineOperatorsWithDefinition(PDStateRef state, const void **defs);

#endif

/** @} */
