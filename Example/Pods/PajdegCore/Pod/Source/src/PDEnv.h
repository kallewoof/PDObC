//
// PDEnv.h
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
 @file PDEnv.h Environment header file.
 
 @ingroup PDENV
 
 @defgroup PDENV PDEnv
 
 @brief An instance of a PDState in a scanner.
 
 @ingroup PDSCANNER_CONCEPT
 
 PDEnv objects are simple, low level instance representations of PDStateRef objects. That is to say, whenever a state is pushed onto the stack, an environment is created, wrapping that state, and the current environment is pushed onto the scanner's environment stack.
 
 In practice, environments also keep track of the build and var stacks.
 
 ### The build stack
 
 The build stack is a pd_stack of objects making up a bigger object in the process of being scanned. For instance, if the scanner has just finished reading

 @code
    <<  /Info 1 2 R 
        /Type /Metadata
        /Subtype 
 @endcode
 
 the build stack will consist of the /Info and /Type dictionary entries. 
 
 ### The var stack
 
 The var stack is very similar to the build stack, except it is made up of *one* object being parsed. In the example above, the var stack would contain the PDF name "Subtype", because the value of the dictionary entry has not yet been scanned.
 
 @{
 */

#ifndef INCLUDED_PDENV_h
#define INCLUDED_PDENV_h

#include "PDDefines.h"

/**
 Create an environment with the given state.
 
 @param state The state.
 @return The environment.
 */
extern PDEnvRef PDEnvCreate(PDStateRef state);

#endif

/** @} */
