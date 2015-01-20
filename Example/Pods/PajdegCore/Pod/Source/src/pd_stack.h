//
// pd_stack.h
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
 @file pd_stack.h Stack header file.
 
 @ingroup pd_stack

 @defgroup pd_stack pd_stack
 
 @brief Simple stack implementation tailored for Pajdeg's purposes. 
 
 @ingroup PDALGO
 
 The pd_stack works like any other stack, except it has some amount of awareness about certain object types. 
 
 @{
*/


#ifndef INCLUDED_pd_stack_h
#define INCLUDED_pd_stack_h

#include <sys/types.h>
#include "PDDefines.h"

/**
 Globally turn on or off destructive operations in stacks
 
 @param preserve Whether preserve should be enabled or not.
 
 @note Nests truths.
 */
extern void pd_stack_set_global_preserve_flag(PDBool preserve);

/// @name Pushing onto stack

/**
 Push a key (a string value) onto a stack.
 
 @param stack The stack.
 @param key The key. It is taken as is and freed on stack destruction.
 */
extern void pd_stack_push_key(pd_stack *stack, char *key);

/**
 Push an identifier onto a stack.
 
 @param stack The stack.
 @param identifier The identifier. Can be anything. Never touched.
 */
extern void pd_stack_push_identifier(pd_stack *stack, PDID identifier);

/**
 Push a freeable, arbitrary object. 
 
 @param stack The stack.
 @param info The object. Freed on stack destruction.
 */
extern void pd_stack_push_freeable(pd_stack *stack, void *info);

/**
 Push a stack onto the stack. 
 
 @note This is not an append operation; the pushed stack becomes a single entry that is a stack.
 
 @param stack The stack.
 @param pstack The stack to push. It is destroyed on stack destruction.
 */
extern void pd_stack_push_stack(pd_stack *stack, pd_stack pstack);

/**
 Push a PDType object.
 
 @param stack The stack.
 @param ob The object. It is retained on push and released on pop, or destroy.
 */
extern void pd_stack_push_object(pd_stack *stack, void *ob);

/**
 Unshift (put in from the start) a stack onto a stack.
 
 The difference between this and pushing a stack is demonstrated by the following:

 1. @code push    [a,b,c] onto [1,2,3] -> [1,2,3,[a,b,c]] @endcode
 2. @code unshift [a,b,c] onto [1,2,3] -> [[a,b,c],1,2,3] @endcode
 
 @note This is only here to deal with reversed dictionaries/arrays; support for unshifting was never intended.

 @param stack The stack.
 @param sstack The stack to unshift. It is destroyed on stack destruction.
 */
extern void pd_stack_unshift_stack(pd_stack *stack, pd_stack sstack);

/**
 Copy a stack in its entirety, strdup()ing as necessary to prevent double-free() calls when destroyed.
 
 In other words, this creates a stack which is entirely isolated from the original, which may be destroyed without incident.
 
 @param stack The stack to copy.
 @return A copy of the stack.
 */
extern pd_stack pd_stack_copy(pd_stack stack);

/// @name Popping from stack

/**
 Destroy a stack (getting rid of every item according to its type).
 
 @param stack The stack.
 */
extern void pd_stack_destroy(pd_stack *stack);

/**
 Pop a key off of the stack. Throws assertion if the next item is not a key.
 
 @param stack The stack.
 */
extern char *pd_stack_pop_key(pd_stack *stack);

/**
 Pop an identifier off of the stack. Throws assertion if the next item is not an identifier.
 
 @param stack The stack.
 */
extern PDID pd_stack_pop_identifier(pd_stack *stack);

/**
 Pop a stack off of the stack. Throws assertion if the next item is not a stack.
 
 @param stack The stack.
 */
extern pd_stack pd_stack_pop_stack(pd_stack *stack);

/**
 Pop a PDType object off of the stack. Throws assertion if the next item is not a PD object.
 
 @param stack The stack.
 */
extern void *pd_stack_pop_object(pd_stack *stack);

/**
 Pop a freeable off of the stack. Throws assertion if the next item is not a freeable.
 
 @param stack The stack.
 */
extern void *pd_stack_pop_freeable(pd_stack *stack);

/**
 Pop and convert key into a size_t value. Throws assertion if the next item is not a key.
 
 @param stack The stack.
 */
extern PDSize pd_stack_pop_size(pd_stack *stack);

/**
 Pop and convert key into an PDInteger value. Throws assertion if the next item is not a key.
 
 @param stack The stack.
 */
extern PDInteger pd_stack_pop_int(pd_stack *stack);

/**
 Pop the next key, verify that it is equal to the given key, and then discard it. 
 
 Throws assertion if any of this is not the case.
 
 @param stack The stack.
 @param key Expected key.
 */
extern void   pd_stack_assert_expected_key(pd_stack *stack, const char *key);

/**
 Pop the next key, verify that its integer value is equal to the given integer, and then discard it. 
 
 Throws assertion if any of this is not the case.
 
 @param stack The stack.
 @param i Expected integer value.
 */
extern void   pd_stack_assert_expected_int(pd_stack *stack, PDInteger i);

/**
 Look at the next PDInteger on the stack without popping it. Throws assertion if the next item is not a key.
 
 @param stack The stack.
 */
extern PDInteger pd_stack_peek_int(pd_stack stack);

/// @name Convenience features

/**
 Get the number of elements in the stack.
 
 @param stack The stack.
 @return Element count.
 */
extern PDInteger pd_stack_get_count(pd_stack stack);

/**
 Pop a value off of source and push it onto dest.
 
 @param dest The destination stack.
 @param source The source stack. Must not be NULL.
 */
extern void pd_stack_pop_into(pd_stack *dest, pd_stack *source);

/**
 Non-destructive stack iteration.
 
 This simply constructs a for loop that iterates over and puts the pd_stack entries of the stack into the iter stack one at a time.
 
 @param stack The stack to iterate.
 @param iter  The iteration pd_stack variable.
 */
#define pd_stack_for_each(stack, iter) for (iter = stack; iter; iter = iter->prev)

/**
 Dictionary get function.
 
 @param dictStack The dictionary stack.
 @param key The key.
 @param remove If true, the entry for the key is removed from the dictionary stack.
 @return The pd_stack for the given dictionary entry.
 */
extern pd_stack pd_stack_get_dict_key(pd_stack dictStack, const char *key, PDBool remove);

/**
 Non-destructive stack iteration.
 
 Converts the value of the dictionary entry into a string representation.
 
 @warning In order to use this function, a supplementary pd_stack must be set to the dictionary stack, and then used as the iterStack argument. Passing the master dictionary will result in the (memory / information) loss of the stack.
 
 @note Value must be freed, key must not.
 
 @param iterStack The iteration stack.
 @param key Pointer to a C string which should be pointed at the next key in the dictionary. Must not be pre-allocated. Must not be freed.
 @param value Pointer to the value string. Must not be pre-allocated. Must be freed.
 @return true if key and value were set, false if not.
 */
extern PDBool pd_stack_get_next_dict_key(pd_stack *iterStack, char **key, char **value);

/**
 Convert a complex array stack or a dictionary entry containing a complex array object into a simple array stack.
 
 @note Arrays are most often listed in reverse order in stacks.
 
 @warning The returned stack must not be modified or destroyed. It is still a part of arrStack.
 
 @param arrStack The array stack or DE.
 @return A simplified array stack, where each entry is a stack containing an (AE) identifier followed by the entry.
 */
extern pd_stack pd_stack_get_arr(pd_stack arrStack);

/**
 Replace info object in stack object with a new info object of the given type.
 
 @param stack The stack.
 @param type The new type.
 @param info The new info object.
 */
extern void pd_stack_replace_info_object(pd_stack stack, char type, void *info);

/**
 Set up a stack with NULL terminated list of values, put in backward.
 
 @param defs NULL terminated list of values.
 @return Stack with each value pushed as an identifier.
 */
extern pd_stack pd_stack_create_from_definition(const void **defs);

/// @name Debugging

/**
 Print out a stack (including pointer data).
 
 @param stack The stack.
 */
extern void pd_stack_print(pd_stack stack);
/**
 Show a stack (like printing, but in a more overviewable format).
 
 @param stack The stack.
 */
extern void pd_stack_show(pd_stack stack);

/** @} */

/** @} */

/**
 The global deallocator for stacks. Defaults to the built-in free() function, but is overridden when global preserve flag is set.
 
 @see pd_stack_set_global_preserve_flag
 */
extern PDDeallocator pd_stack_dealloc;

/**
 Deallocate something using stack deallocator.
 */
#define PDDeallocateViaStackDealloc(ob) pd_stack_dealloc(ob)

#endif
