//
// PDContentStream.h
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
 @file PDContentStream.h PDF content stream header file.
 
 @ingroup PDCONTENTSTREAM
 
 @defgroup PDCONTENTSTREAM PDContentStream
 
 @brief A content stream, i.e. a stream containing Adobe commands, such as for writing text and similar.
 
 @ingroup PDUSER
 
 Content streams may have a wide variety of purposes. One such purpose is the drawing of content on the page. 
 The content stream module contains support for working with the state machine to process the content in a
 variety of ways.
 
 The mode of operation goes as follows: 
 
 - there are two types of entries: arguments and operators
 - there is a stack of (space/line separated) arguments
 - there is a stack of current operators
 - when an argument is encountered, it's pushed onto the stack
 - when an operator is encountered, a defined number of arguments (based on operator name) are popped off the stack
 - some operators push onto or pop off of the current operators stack (BT pushes, ET pops, for example)
 
 At this point, no known example exists where the above complexity (in reference to arguments) is necessary. Instead, the following approximation is done:
 
 - put arguments into a list until the next operator is encountered
 - operator & list = the next operation
 
 Current operators stack is done exactly as defined.
 
 @{
 */

#ifndef INCLUDED_PDContentStream_h
#define INCLUDED_PDContentStream_h

#include <stdio.h>

#include "PDDefines.h"

/**
 *  A PDF content stream operation.
 *
 *  Content stream operations are simple pairs of operator names and their corresponding state stack, if any.
 *
 *  @ingroup PDCONTENTSTREAM
 */
struct PDContentStreamOperation {
    char    *name;  ///< name of operator
    pd_stack state; ///< state of operator; usually preserved values from its args
};

typedef struct PDContentStreamOperation *PDContentStreamOperationRef;

///! Basic operations

/**
 *  Set up a content stream based on an existing object and its (existing) stream.
 *
 *  @param object The object containing a stream.
 *
 *  @return The content stream object
 */
extern PDContentStreamRef PDContentStreamCreateWithObject(PDObjectRef object);

/**
 *  Attach an operator function to a given operator (replacing the current operator, if any).
 *
 *  @param cs       The content stream
 *  @param opname   The operator (e.g. "BT")
 *  @param op       The callback, which abides by the PDContentOperatorFunc signature
 *  @param userInfo User info value passed to the operator when called
 */
extern void PDContentStreamAttachOperator(PDContentStreamRef cs, const char *opname, PDContentOperatorFunc op, void *userInfo);

/**
 *  Attach a deallocator to a content stream. Content streams call every deallocator attached to it once before it destroys itself. 
 *  Deallocators in content streams are used to clean up user info objects that were allocated in the process of setting up operators.
 *
 *  @param cs          The content stream
 *  @param deallocator The deallocator callback. It will be handed userInfo as argument
 *  @param userInfo    The argument passed to deallocator
 */
extern void PDContentStreamAttachDeallocator(PDContentStreamRef cs, PDDeallocator deallocator, void *userInfo);

/**
 *  Attach a resetter to a content stream. Content streams call every resetter attached to it at the end of every call to 
 *  PDContentStreamExecute.
 *
 *  @param cs       The content stream
 *  @param resetter The resetter callback
 *  @param userInfo The argument passed to the resetter
 */
extern void PDContentStreamAttachResetter(PDContentStreamRef cs, PDDeallocator resetter, void *userInfo);

/**
 *  Attach a variable number of operator function pairs (opname, func, ...), each sharing the given user info object.
 *
 *  Pairs are provided using the PDDef() macro. The following code
 @code
    PDContentStreamAttachOperatorPairs(cs, ui, PDDef(
        "q",  myGfxStatePush,
        "Q",  myGfxStatePop,
        "BT", myBeginTextFunc, 
        "ET", myEndTextFunc
    ));
 @endcode
 *  is equivalent to
 @code
     PDContentStreamAttachOperator(cs, "q",  myGfxStatePush,  ui);
     PDContentStreamAttachOperator(cs, "Q",  myGfxStatePop,   ui);
     PDContentStreamAttachOperator(cs, "BT", myBeginTextFunc, ui);
     PDContentStreamAttachOperator(cs, "ET", myEndTextFunc,   ui);
 @endcode
 *
 *  @param cs       The content stream
 *  @param userInfo The shared user info object
 *  @param pairs    Pairs of operator name + operator callback
 */
extern void PDContentStreamAttachOperatorPairs(PDContentStreamRef cs, void *userInfo, const void **pairs);

/**
 *  Get the operator tree for the content stream. The operator tree is the representation of the operators
 *  in effect in the content stream. It is mutable, and updates to it, or to the content stream, will affect
 *  the original content stream.
 *
 *  @param cs The content stream
 *
 *  @return The operator tree
 */
extern PDSplayTreeRef PDContentStreamGetOperatorTree(PDContentStreamRef cs);

/**
 *  Replace the content stream's operator tree with the new tree (which may not be NULL). The content stream
 *  will use the new tree internally, thus making changes to and be affected by changes to the object.
 *
 *  @param cs           The content stream
 *  @param operatorTree The new operator tree
 */
extern void PDContentStreamSetOperatorTree(PDContentStreamRef cs, PDSplayTreeRef operatorTree);

/**
 *  Inherit a content stream, copying its resetters and operator tree into the destination. 
 *  This is the recommended way to "clone" content streams, since the addition of deallocators and resetters.
 *  This copies resetters as well as operator trees, but does not copy deallocators for obvious reasons. The master content stream must remain alive until all child streams have finished.
 *
 *  @param dest   Destination content stream (must be a clean content stream without, in particular, any resetters)
 *  @param source Source content stream, whose values should be cloned in dest
 */
extern void PDContentStreamInheritContentStream(PDContentStreamRef dest, PDContentStreamRef source);

/**
 *  Execute the content stream, i.e. parse the stream and call the operators as appropriate.
 *
 *  @param cs The content stream
 */
extern void PDContentStreamExecute(PDContentStreamRef cs);

/**
 *  Get the current operator stack from the content stream. 
 *
 *  The operator stack is a stack of PDContentStreamOperationRef objects; the values in the object can be obtained usign ob->name and ob->state.
 *
 *  @see PDContentStreamOperation
 *
 *  @param cs The content stream
 *
 *  @return Operator stack
 */
extern const pd_stack PDContentStreamGetOperators(PDContentStreamRef cs);

///! Advanced operations

/**
 *  Create a content stream configured to perform text search. 
 *
 *  @note Creating *one* text search content stream, and then using PDContentStreamGetOperatorTree and PDContentStreamSetOperatorTree to configure additional content streams is more performance efficient than creating a text search content stream for every content stream, when searching across multiple streams.
 *
 *  @param object       Object in which search should be performed
 *  @param searchString String to search for
 *  @param callback     Callback for matches
 *
 *  @return A pre-configured content stream
 */
extern PDContentStreamRef PDContentStreamCreateTextSearch(PDObjectRef object, const char *searchString, PDTextSearchOperatorFunc callback);

#endif

/** @} */

/** @} */
