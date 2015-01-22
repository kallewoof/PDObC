//
// PDDictionaryStack.h
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
 @file PDDictionaryStack.h Dictionary stack object
 
 @ingroup PDDictionaryStack
 
 @defgroup PDDictionaryStack PDDictionaryStack
 
 @brief A stack of dictionaries.
 
 PDDictionaryStack is a special purpose dictionary implementation which is used
 to access a stack of dictionaries. 
 
 The dictionaries are located in a regular stack. Only the top-most stack is
 affected by changes, including entry deletions, dictionary clears, etc. 
 
 When polling a dictionary for a given key, the stack is traversed until a 
 dictionary is encountered whose value for the given key is non-NULL. This value
 is returned.
 
 The implementation allows for "modifications" to master dictionaries, such as
 the latin encoding dictionary, modified by Differences arrays in an Encoding
 dictionary.
 
 @{
 */

#ifndef INCLUDED_PDDICTIONARYSTACK_H
#define INCLUDED_PDDICTIONARYSTACK_H

#include "PDDefines.h"

/**
 *  Create a new dictionary stack with the given dictionary as the initial dictionary.
 *
 *  @param dict Initial dictionary
 *
 *  @return A new dictionary stack
 */
extern PDDictionaryStackRef PDDictionaryStackCreateWithDictionary(PDDictionaryRef dict);

/**
 *  Push the given dictionary on top of the dictionary stack.
 *
 *  @param ds   Dictionary stack
 *  @param dict Dictionary to push
 */
extern void PDDictionaryStackPushDictionary(PDDictionaryStackRef ds, PDDictionaryRef dict);

/**
 *  Pop the topmost dictionary from the dictionary stack.
 *
 *  @param ds Dictionary stack
 *
 *  @return The topmost dictionary, or NULL if the dictionary stack is empty
 */
extern PDDictionaryRef PDDictionaryStackPopDictionary(PDDictionaryStackRef ds);

/**
 *  Add all entries from the given PDF dictionary stack to the top dictionary.
 *  Entries will replace pre-existing key/value pairs when a key in the 
 *  pd_stack matches the key in the dictionary.
 *
 *  @param ds    Dictionary to add entries to
 *  @param stack PDF dictionary stack
 */
extern void PDDictionaryStackAddEntriesFromComplex(PDDictionaryStackRef hm, pd_stack stack);

/**
 *  Set key to value. If value is NULL, an assertion is thrown. To delete
 *  keys, use PDDictionaryDelete().
 *
 *  @param ds    The dictionary stack
 *  @param key   The key
 *  @param value The value
 */
extern void PDDictionaryStackSet(PDDictionaryStackRef ds, const char *key, void *value);

/**
 *  Get the value of the given key.
 *
 *  @param ds  The dictionary stack
 *  @param key The key
 */
extern void *PDDictionaryStackGet(PDDictionaryStackRef ds, const char *key);

/**
 *  Delete the given key from the top dictionary.
 *
 *  @param ds  The dictionary stack
 *  @param key The key to delete
 */
extern void PDDictionaryStackDelete(PDDictionaryStackRef ds, const char *key);

/**
 *  Remove all entries in the top dictionary.
 *
 *  @param ds Dictionary to clear
 */
extern void PDDictionaryStackClear(PDDictionaryStackRef ds);

/**
 *  Iterate over the dictionary stack entries using the given iterator.
 *
 *  @param ds The dictionary stack whose dictionaries should be iterated
 *  @param it The iterating function
 *  @param ui User information to pass to iterator, if any
 */
extern void PDDictionaryStackIterate(PDDictionaryStackRef ds, PDHashIterator it, void *ui);

#endif
