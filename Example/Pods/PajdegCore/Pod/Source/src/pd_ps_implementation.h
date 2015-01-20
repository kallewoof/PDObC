//
// pd_ps_implementation.h
//
// Copyright (c) 2015 Karl-Johan Alm (http://github.com/kallewoof)
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
 @defgroup PDPS_GRP Pajdeg PostScript Implementation
 
 The PostScript implementation in Pajdeg is a simplified interpreter which is used
 to process e.g. CMap (ToUnicode) content.
 @{
 */

/**
 @file pd_ps_implementation.h
 */

#ifndef INCLUDED_PD_PS_IMPLEMENTATION_H
#define INCLUDED_PD_PS_IMPLEMENTATION_H

#include "PDDefines.h"

/**
 *  A PostScript interpreter environment.
 */
typedef struct pd_ps_env_s *pd_ps_env;
struct pd_ps_env_s {
    PDScannerRef scanner;
    PDDictionaryRef userdict;
    PDDictionaryRef operators;
    pd_stack operands;
    pd_stack dicts;
    pd_stack execs;
    PDBool failure;
    PDBool mpbool;
    PDBool explicitCMap;
    PDCMapRef cmap;
    PDDictionaryRef resources;
};

/**
 *  Create a new PostScript environment for interpreting.
 *
 *  @return A clean environment
 */
extern pd_ps_env pd_ps_create(void);

/**
 *  Destroy a PostScript environment.
 *
 *  @param pse Environment that is no longer needed
 */
extern void pd_ps_destroy(pd_ps_env pse);

/**
 *  Set the explicit CMap object for the PostScript environment.
 *  This will force the system to use this CMap object, rather than creating a new one,
 *  for the first "begincmap" encountered. Subsequent (if multiple) "begincmap" executions
 *  will create additional CMaps as normal.
 *
 *  @param pse  The PostScript environment
 *  @param cmap The CMap object
 */
extern void pd_ps_set_cmap(pd_ps_env pse, PDCMapRef cmap);

/**
 *  Get the CMap object currently being defined in the PostScript environment.
 *
 *  @param pse The PostScript environment
 *
 *  @return CMap object or NULL if none is being defined currently
 */
extern PDCMapRef pd_ps_get_cmap(pd_ps_env pse);

/**
 *  Execute the given stream of data of the given length (in bytes), inside the
 *  given PostScript environment.
 *  The same PostScript environment can be used to execute multiple pieces of
 *  data.
 *
 *  @param pse    The PostScript environment
 *  @param stream The stream of ASCII characters representing a PostScript program
 *  @param len    The length of the stream, in bytes
 *
 *  @return Boolean value indicating whether the execution was successful (true) or not (false)
 */
extern PDBool pd_ps_execute_postscript(pd_ps_env pse, char *stream, PDSize len);

/**
 *  Get the results of the PostScript execution(s) for the given environment.
 *
 *  @param pse The PostScript environment
 *
 *  @return A PDDictionary containing the root user dictionary
 */
extern PDDictionaryRef pd_ps_get_results(pd_ps_env pse);

/**
 *  Get the operand at the given index (zero-based) of the operand stack. 
 *  The operand stack contains all results that have not been consumed by other
 *  operations yet. For example, executing "/MyFontName /CID findresource" after
 *  executing a CMap resource definition for a font named "MyFontName" will put
 *  the "MyFontName" dictionary at the top of the operand stack. This can then
 *  be obtained using 
 *      PDDictionaryRef myCMapDict = pd_ps_get_operand(pse, 0);
 *  from which the actual PDCMap object is in the "#cmap#" key.
 *
 *  @param pse   The PostScript environment
 *  @param index The index (zero-based) in the stack which should be fetched
 *
 *  @return The object at the given index, or NULL if the index was beyond the size of the operand stack
 */
extern void *pd_ps_get_operand(pd_ps_env pse, PDSize index);

#endif

/** @} */
