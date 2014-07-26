//
// pd_stack.c
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

#include "pd_stack.h"
#include "pd_internal.h"
#include "PDEnv.h"
#include "PDState.h"
#include "pd_pdf_implementation.h"

static PDInteger pd_stack_preserve_users = 0;
PDDeallocator pd_stack_dealloc = free;
void pd_stack_preserve(void *ptr)
{}

void pd_stack_set_global_preserve_flag(PDBool preserve)
{
    pd_stack_preserve_users += preserve ? 1 : -1;
    PDAssert(pd_stack_preserve_users >= 0);
    if (pd_stack_preserve_users - preserve < 1)
        pd_stack_dealloc = preserve ? &pd_stack_preserve : &free;
}

void pd_stack_push_identifier(pd_stack *stack, PDID identifier)
{
    pd_stack s = malloc(sizeof(struct pd_stack));
    s->prev = *stack;
    s->info = identifier;
    s->type = PD_STACK_ID;
    *stack = s;
}

void pd_stack_push_key(pd_stack *stack, char *key)
{
    pd_stack s = malloc(sizeof(struct pd_stack));
    s->prev = *stack;
    s->info = key;//strdup(key); free(key);// we can't do the strdup/free trick, ever, because it breaks any code that uses pd_stack as a garbage collector
    s->type = PD_STACK_STRING;
    *stack = s;
}

void pd_stack_push_freeable(pd_stack *stack, void *freeable)
{
    pd_stack s = malloc(sizeof(struct pd_stack));
    s->prev = *stack;
    s->info = freeable;
    s->type = PD_STACK_FREEABLE;
    *stack = s;
}

void pd_stack_push_stack(pd_stack *stack, pd_stack pstack)
{
    pd_stack s = malloc(sizeof(struct pd_stack));
    s->prev = *stack;
    s->info = pstack;
    s->type = PD_STACK_STACK;
    *stack = s;
}

void pd_stack_unshift_stack(pd_stack *stack, pd_stack sstack) 
{
    pd_stack vtail;
    if (*stack == NULL) {
        pd_stack_push_stack(stack, sstack);
        return;
    }
    
    for (vtail = *stack; vtail->prev; vtail = vtail->prev) ;

    pd_stack s = malloc(sizeof(struct pd_stack));
    s->prev = NULL;
    s->info = sstack;
    s->type = PD_STACK_STACK;
    vtail->prev = s;
}

void pd_stack_push_object(pd_stack *stack, void *ob)
{
    PDTYPE_ASSERT(ob);
    pd_stack s = malloc(sizeof(struct pd_stack));
    s->prev = *stack;
    s->info = ob;
    s->type = PD_STACK_PDOB;
    *stack = s;
}

PDID pd_stack_pop_identifier(pd_stack *stack)
{
    if (*stack == NULL) return NULL;
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_ID);
    *stack = popped->prev;
    PDID identifier = popped->info;
    (*pd_stack_dealloc)(popped);
    return identifier;
}

void pd_stack_assert_expected_key(pd_stack *stack, const char *key)
{
    PDAssert(*stack != NULL);
    
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_STRING || popped->type == PD_STACK_ID);

    char *got = popped->info;
    if (popped->type == PD_STACK_STRING) {
        PDAssert(got == key || !strcmp(got, key));
        (*pd_stack_dealloc)(got);
    } else {
        PDAssert(!strcmp(*(char**)got, key));
    }
    
    *stack = popped->prev;
    (*pd_stack_dealloc)(popped);
}

void pd_stack_assert_expected_int(pd_stack *stack, PDInteger i)
{
    PDAssert(*stack != NULL);
    
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_STRING);
    
    char *got = popped->info;
    PDAssert(i == atoi(got));
    
    *stack = popped->prev;
    (*pd_stack_dealloc)(got);
    (*pd_stack_dealloc)(popped);
}

PDSize pd_stack_pop_size(pd_stack *stack)
{
    if (*stack == NULL) return 0;
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_STRING);
    *stack = popped->prev;
    char *key = popped->info;
    PDSize st = atol(key);
    (*pd_stack_dealloc)(key);
    (*pd_stack_dealloc)(popped);
    return st;
}

PDInteger pd_stack_pop_int(pd_stack *stack)
{
    if (*stack == NULL) return 0;
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_STRING);
    *stack = popped->prev;
    char *key = popped->info;
    PDInteger st = atol(key);
    (*pd_stack_dealloc)(key);
    (*pd_stack_dealloc)(popped);
    return st;
}

PDInteger pd_stack_peek_int(pd_stack popped)
{
    if (popped == NULL) return 0;
    PDAssert(popped->type == PD_STACK_STRING);
    return PDIntegerFromString(popped->info);
}

char *pd_stack_pop_key(pd_stack *stack)
{
    if (*stack == NULL) return NULL;
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_STRING);
    *stack = popped->prev;
    char *key = popped->info;
    (*pd_stack_dealloc)(popped);
    return key;
}

pd_stack pd_stack_pop_stack(pd_stack *stack)
{
    if (*stack == NULL) return NULL;
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_STACK);
    *stack = popped->prev;
    pd_stack pstack = popped->info;
    (*pd_stack_dealloc)(popped);
    return pstack;
}

void *pd_stack_pop_object(pd_stack *stack)
{
    if (*stack == NULL) return NULL;
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_PDOB);
    *stack = popped->prev;
    void *ob = popped->info;
    (*pd_stack_dealloc)(popped);
    return ob;
}

void *pd_stack_pop_freeable(pd_stack *stack)
{
    if (*stack == NULL) return NULL;
    pd_stack popped = *stack;
    PDAssert(popped->type == PD_STACK_FREEABLE);
    *stack = popped->prev;
    void *key = popped->info;
    (*pd_stack_dealloc)(popped);
    return key;
}

void pd_stack_pop_into(pd_stack *dest, pd_stack *source)
{
    if (*source == NULL) {
        PDAssert(*source != NULL);  // must never pop into from a null stack
        return;
    }
    
    pd_stack popped = *source;
    *source = popped->prev;
    popped->prev = *dest;
    *dest = popped;
}

pd_stack pd_stack_copy(pd_stack stack)
{
    pd_stack backward = NULL;
    pd_stack forward = NULL;
    
    for (pd_stack s = stack; s; s = s->prev) {
        switch (s->type) {
            case PD_STACK_STRING:
                pd_stack_push_key(&backward, strdup(s->info));
                break;
            case PD_STACK_ID:
                pd_stack_push_identifier(&backward, s->info);
                break;
            case PD_STACK_STACK:
                pd_stack_push_stack(&backward, pd_stack_copy(s->info));
                break;
            case PD_STACK_PDOB:
                pd_stack_push_object(&backward, PDRetain(s->info));
                break;
            default: // case PD_STACK_FREEABLE:
                // we don't allow arbitrary freeables (or any other types) in clones
                PDWarn("skipping arbitrary object in pd_stack_copy operation");
                break;
        }
    }
    
    // flip order of backward stack, as it is the opposite order of the copied one
    while (backward) pd_stack_pop_into(&forward, &backward);
    return forward;
}

void pd_stack_destroy_internal(pd_stack stack);

static inline void pd_stack_free_info(pd_stack stack)
{
    switch (stack->type) {
        case PD_STACK_STRING:
        case PD_STACK_FREEABLE:
            free(stack->info);
            break;
        case PD_STACK_STACK:
            pd_stack_destroy_internal(stack->info);
            break;
        case PD_STACK_PDOB:
            PDRelease(stack->info);
            break;
    }
}

void pd_stack_replace_info_object(pd_stack stack, char type, void *info)
{
    pd_stack_free_info(stack);
    stack->type = type;
    stack->info = info;
}

void pd_stack_destroy_internal(pd_stack stack)
{
    pd_stack p;
    while (stack) {
        //printf("-stack %p\n", stack);
        p = stack->prev;
        pd_stack_free_info(stack);
        free(stack);
        stack = p;
    }
}

void pd_stack_destroy(pd_stack *stack)
{
    pd_stack_destroy_internal(*stack);
    *stack = NULL;
}

pd_stack pd_stack_get_dict_key(pd_stack dictStack, const char *key, PDBool remove)
{
    // dicts are set up (reversedly) as
    // "dict"
    // "entries"
    // [stack]
    if (dictStack == NULL || ! PDIdentifies(dictStack->info, PD_DICT)) 
        return NULL;
    
    pd_stack prev = dictStack->prev->prev;
    pd_stack stack = dictStack->prev->prev->info;
    pd_stack entry;
    while (stack) {
        // entries are stacks, with
        // e
        // ID
        // entry
        entry = stack->info;
        if (! strcmp((char*)entry->prev->info, key)) {
            if (remove) {
                if (prev == dictStack->prev->prev) {
                    // first entry; container stack must reref
                    prev->info = stack->prev;
                } else {
                    // not first entry; simply reref prev
                    prev->prev = stack->prev;
                }
                // disconnect stack from its siblings and from prev (or prev is destroyed), then destroy stack and we can return prev
                stack->info = NULL;
                stack->prev = NULL;
                pd_stack_destroy_internal(stack);
                return entry;
            }
            return entry;
        }
        prev = stack;
        stack = stack->prev;
    }
    return NULL;
}

PDBool pd_stack_get_next_dict_key(pd_stack *iterStack, char **key, char **value)
{
    // dicts are set up (reversedly) as
    // "dict"
    // "entries"
    // [stack]
    pd_stack stack = *iterStack;
    pd_stack entry;
    
    // two instances where we hit falsity; stack is indeed a dictionary, but it's empty: we will initially have a stack but it will be void after below if case (hence, stack truthy is included), otherwise it is the last element, in which case it's NULL on entry
    if (stack && PDIdentifies(stack->info, PD_DICT)) {
        *iterStack = stack = stack->prev->prev->info;
    }
    if (! stack) return false;
    
    // entries are stacks, with
    // de
    // ID
    // entry
    entry = stack->info;
    *key = (char*)entry->prev->info;
    entry = (pd_stack)entry->prev->prev;
    
    // entry is now iterated past e, ID and is now at
    // entry
    // so we see if type is primitive or not
    if (entry->type == PD_STACK_STACK) {
        // it's not primitive, so we set the preserve flag and stringify
        pd_stack_set_global_preserve_flag(true);
        entry = (pd_stack)entry->info;
        *value = PDStringFromComplex(&entry);
        pd_stack_set_global_preserve_flag(false);
    } else {
        // it is primitive (we presume)
        PDAssert(entry->type == PD_STACK_STRING);
        *value = strdup((char*)entry->info);
    }
    
    *iterStack = stack->prev;
    
    return true;
}

/*
 stack<0x1136ba20> {
   0x3f9998 ("de")
   Kids
    stack<0x11368f20> {
       0x3f999c ("array")
       0x3f9990 ("entries")
        stack<0x113ea650> {
            stack<0x113c5c10> {
               0x3f99a0 ("ae")
                stack<0x113532b0> {
                   0x3f9988 ("ref")
                   557
                   0
                }
            }
            stack<0x113d49b0> {
               0x3f99a0 ("ae")
                stack<0x113efa50> {
                   0x3f9988 ("ref")
                   558
                   0
                }
            }
            stack<0x113f3c40> {
               0x3f99a0 ("ae")
                stack<0x1136df80> {
                   0x3f9988 ("ref")
                   559
                   0
                }
            }
            stack<0x113585b0> {
               0x3f99a0 ("ae")
                stack<0x11368e30> {
                   0x3f9988 ("ref")
                   560
                   0
                }
            }
            stack<0x1135df20> {
               0x3f99a0 ("ae")
                stack<0x113f3470> {
                   0x3f9988 ("ref")
                   1670
                   0
                }
            }
        }
    }
}
 */

PDInteger pd_stack_get_count(pd_stack stack)
{
    PDInteger count;
    for (count = 0; stack; count++) 
        stack = stack->prev;
    return count;
}

pd_stack pd_stack_get_arr(pd_stack arrStack)
{
    pd_stack stack = arrStack;
    
    // often, arrStack is a dictionary entry
    if (stack && PDIdentifies(stack->info, PD_DE)) {
        stack = stack->prev->prev->info;
    }
    if (! stack) return NULL;
    
    /* arrays have 
       0x3f999c ("array")
       0x3f9990 ("entries")
        stack<0x113ea650> {
     and we want to move beyond first 2, and step into the 3rd which has a series of
            stack<0x113c5c10> {
               0x3f99a0 ("ae")
                stack<0x113532b0> {
                   0x3f9988 ("ref")
                   557
                   0
                }
            }
            stack<0x113d49b0> {
               0x3f99a0 ("ae")
                stack<0x113efa50> {
                   0x3f9988 ("ref")
                   558
                   0
                }
            }
     */
    stack = stack->prev->prev->info;
    
    // on this level, each stack entry is one element, which is what we want
    return stack;
}

pd_stack pd_stack_create_from_definition(const void **defs)
{
    PDInteger i;
    pd_stack stack = NULL;
    
    for (i = 0; defs[i]; i++) {
        pd_stack_push_identifier(&stack, (const char **)defs[i]);
    }
    
    return stack;
}

//
// debugging
//


static char *sind = NULL;
static PDInteger cind = 0;
void pd_stack_print_(pd_stack stack, PDInteger indent)
{
    PDInteger res = cind;
    sind[cind] = ' ';
    sind[indent] = 0;
    printf("%sstack<%p> {\n", sind, stack);
    sind[indent] = ' ';
    cind = indent + 2;
    sind[cind] = 0;
    while (stack) {
        switch (stack->type) {
            case PD_STACK_ID:
                printf("%s %p (\"%s\")\n", sind, stack->info, *(char **)stack->info);
                break;
            case PD_STACK_STRING:
                printf("%s %s\n", sind, (char*)stack->info);
                break;
            case PD_STACK_FREEABLE:
                printf("%s %p\n", sind, stack->info);
                break;
            case PD_STACK_STACK:
                pd_stack_print_(stack->info, cind + 2);
                break;
            case PD_STACK_PDOB:
                printf("%s object (%p)", sind, stack->info);
                break;
            default:
                printf("%s ?????? %p", sind, stack->info);
                break;
        }
        stack = stack->prev;
    }
    sind[cind] = ' ';
    cind -= 2;
    sind[indent] = 0;
    printf("%s}\n", sind);
    cind = res;
    sind[indent] = ' ';
    sind[cind] = 0;
}

void pd_stack_print(pd_stack stack)
{
    if (sind == NULL) sind = strdup("                                                                                                                                                                                                                                                       ");
    pd_stack_print_(stack, 0);
}

// 
// the "pretty" version (above is debuggy)
//

void pd_stack_show_(pd_stack stack)
{
    PDBool stackLumping = false;
    while (stack) {
        stackLumping &= (stack->type == PD_STACK_STACK);
        if (stackLumping) putchar('\t');
        
        switch (stack->type) {
            case PD_STACK_ID:
                printf("@%s", *(char **)stack->info);
                break;
            case PD_STACK_STRING:
                printf("\"%s\"", (char*)stack->info);
                break;
            case PD_STACK_FREEABLE:
                printf("%p", stack->info);
                break;
            case PD_STACK_STACK:
                if (! stackLumping && (stackLumping |= stack->prev && stack->prev->type == PD_STACK_STACK)) 
                    putchar('\n');
                //stackLumping |= stack->prev && stack->prev->type == PD_STACK_STACK;
                if (stackLumping) {
                    printf("\t{ ");
                    pd_stack_show_(stack->info);
                    printf(" }");
                } else {
                    printf("{ ");
                    pd_stack_show_(stack->info);
                    printf(" }");
                }
                break;
            case PD_STACK_PDOB:
                printf("<%p>", stack->info);
                break;
            default:
                printf("??? %d %p ???", stack->type, stack->info);
                break;
        }
        stack = stack->prev;
        if (stack) printf(stackLumping ? ",\n" : ", ");
    }
}

void pd_stack_show(pd_stack stack)
{
    printf("{ ");
    pd_stack_show_(stack);
    printf(" }\n");
}


