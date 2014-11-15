//
// PDDictionary.c
//
// Copyright (c) 2012 - 2014 Karl-Johan Alm (http://github.com/kallewoof)
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WdictANTY; without even the implied wdictanty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "Pajdeg.h"
#include "PDDictionary.h"
#include "pd_stack.h"
#include "pd_crypto.h"
#include "pd_pdf_implementation.h"

#include "PDString.h"
#include "pd_internal.h"

#define PDDictionaryFetchI(dict, key) \
    PDInteger index; \
    for (index = dict->count-1; index >= 0; index--) \
        if (0 == strcmp(key, dict->keys[index])) \
            break;\

void PDDictionaryDestroy(PDDictionaryRef dict)
{
    for (PDInteger i = dict->count-1; i >= 0; i--) {
        free(dict->keys[i]);
        PDRelease(dict->values[i]);
        pd_stack_destroy(&dict->vstacks[i]);
    }
    
#ifdef PD_SUPPORT_CRYPTO
    PDRelease(dict->ci);
#endif
    
    free(dict->keys);
    free(dict->values);
    free(dict->vstacks);
}

PDDictionaryRef PDDictionaryCreateWithCapacity(PDInteger capacity)
{
    PDDictionaryRef dict = PDAllocTyped(PDInstanceTypeDict, sizeof(struct PDDictionary), PDDictionaryDestroy, false);
    
    if (capacity < 1) capacity = 1;
    
#ifdef PD_SUPPORT_CRYPTO
    dict->ci = NULL;
#endif
    dict->count = 0;
    dict->capacity = capacity;
    dict->keys = malloc(sizeof(char *) * capacity);
    dict->values = malloc(sizeof(void *) * capacity);
    dict->vstacks = malloc(sizeof(pd_stack) * capacity);
    return dict;
}

PDDictionaryRef PDDictionaryCreateWithComplex(pd_stack stack)
{
    if (stack == NULL) {
        // empty dictionary
        return PDDictionaryCreateWithCapacity(1);
    }
    
    // this may be an entry that is a dictionary inside another dictionary; if so, we should see something like
    /*
stack<0x15778480> {
   0x532c24 ("de")
   DecodeParms
    stack<0x157785e0> {
       0x532c20 ("dict")
       0x532c1c ("entries")
        stack<0x15778520> {
            stack<0x15778510> {
               0x532c24 ("de")
               Columns
               3
            }
            stack<0x15778590> {
               0x532c24 ("de")
               Predictor
               12
            }
        }
    }
}
     */
    if (PDIdentifies(stack->info, PD_DE)) {
        stack = stack->prev->prev->info;
    }
    
    // we may have the DICT identifier
    if (PDIdentifies(stack->info, PD_DICT)) {
        if (stack->prev->prev == NULL) 
            stack = NULL;
        else 
            stack = stack->prev->prev->info;
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
    
    // determine size of stack
    PDInteger count;
    pd_stack s = stack;
    pd_stack entry;
    
    for (count = 0; s; s = s->prev)
        count++;
    PDDictionaryRef dict = PDDictionaryCreateWithCapacity(count);
//    dict->count = count;x/
    
    s = stack;
    pd_stack_set_global_preserve_flag(true);
    for (count = 0; s; s = s->prev) {
        entry = as(pd_stack, s->info)->prev;
        // entry must be a string; it's the key value
        char *key = entry->info;
        PDBool exists = false;
        for (int i = 0; !exists && i < count; i++) 
            exists |= !strcmp(key, dict->keys[i]);
        if (exists) {
            PDNotice("skipping duplicate dictionary key %s", key);
        } else {
            dict->keys[count] = strdup(key);
            entry = entry->prev;
            if (entry->type == PD_STACK_STRING) {
                dict->values[count] = PDStringCreate(strdup(entry->info));
                dict->vstacks[count] = NULL;
            } else {
                dict->vstacks[count] = /*entry =*/ pd_stack_copy(entry->info);
                //            entry->info = NULL;
                //            entry = dict->vstacks[count];
                dict->values[count] = NULL; // PDStringFromComplex(&entry);
            }
            count++;
        }
    }
    dict->count = count;
    pd_stack_set_global_preserve_flag(false);
    
    return dict;
}

void PDDictionaryClear(PDDictionaryRef dictionary)
{
    while (dictionary->count > 0)
        PDDictionaryDeleteEntry(dictionary, dictionary->keys[dictionary->count-1]);
}

PDInteger PDDictionaryGetCount(PDDictionaryRef dictionary)
{
    return dictionary->count;
}

char **PDDictionaryGetKeys(PDDictionaryRef dictionary)
{
    return dictionary->keys;
}

void *PDDictionaryGetEntry(PDDictionaryRef dictionary, const char *key)
{
    PDDictionaryFetchI(dictionary, key);
    if (index < 0) return NULL;
    
    if (! dictionary->values[index]) {
        if (dictionary->vstacks[index] != NULL) {
            pd_stack entry = dictionary->vstacks[index];
            pd_stack_set_global_preserve_flag(true);
            dictionary->values[index] = PDInstanceCreateFromComplex(&entry);
            pd_stack_set_global_preserve_flag(false);
#ifdef PD_SUPPORT_CRYPTO
            if (dictionary->values[index] && dictionary->ci) 
                (*PDInstanceCryptoExchanges[PDResolve(dictionary->values[index])])(dictionary->values[index], dictionary->ci, true);
#endif
        }
    }
    
    return dictionary->values[index];
}

void *PDDictionaryGetTypedEntry(PDDictionaryRef dictionary, const char *key, PDInstanceType type)
{
    void *ctr = PDDictionaryGetEntry(dictionary, key);
    return ctr && PDResolve(ctr) == type ? ctr : NULL;
}

void PDDictionarySetEntry(PDDictionaryRef dictionary, const char *key, void *value)
{
    PDDictionaryFetchI(dictionary, key);
    if (index == -1) index = dictionary->count;
    
    if (index == dictionary->count && dictionary->count == dictionary->capacity) {
        dictionary->capacity += dictionary->capacity + 1;
        dictionary->keys = realloc(dictionary->keys, sizeof(char *) * dictionary->capacity);
        dictionary->values = realloc(dictionary->values, sizeof(void *) * dictionary->capacity);
        dictionary->vstacks = realloc(dictionary->vstacks, sizeof(pd_stack) * dictionary->capacity);
    }

#ifdef PD_SUPPORT_CRYPTO
    if (dictionary->ci) (*PDInstanceCryptoExchanges[PDResolve(value)])(value, dictionary->ci, false);
#endif
    
    if (index == dictionary->count) {
        // increase count and set key
        dictionary->count++;
        dictionary->keys[index] = strdup(key);
    } else {
        // clear out old value
        PDRelease(dictionary->values[index]);
        pd_stack_destroy(&dictionary->vstacks[index]);
    }
    
    dictionary->values[index] = PDRetain(value);
    dictionary->vstacks[index] = NULL;
}

void PDDictionaryDeleteEntry(PDDictionaryRef dictionary, const char *key)
{
    PDDictionaryFetchI(dictionary, key);
    if (index < 0) return;
    
    free(dictionary->keys[index]);
    PDRelease(dictionary->values[index]);
    pd_stack_destroy(&dictionary->vstacks[index]);
    dictionary->count--;
    for (PDInteger i = index; i < dictionary->count; i++) {
        dictionary->keys[i] = dictionary->keys[i+1];
        dictionary->values[i] = dictionary->values[i+1];
        dictionary->vstacks[i] = dictionary->vstacks[i+1];
    }
}

char *PDDictionaryToString(PDDictionaryRef dictionary)
{
    PDInteger len = 6 + 20 * dictionary->count;
    char *str = malloc(len);
    PDDictionaryPrinter(dictionary, &str, 0, &len);
    return str;
}

void PDDictionaryPrint(PDDictionaryRef dictionary)
{
    char *str = PDDictionaryToString(dictionary);
    puts(str);
    free(str);
}

PDInteger PDDictionaryPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap)
{
    PDInstancePrinterInit(PDDictionaryRef, 0, 1);
    PDInteger len = 6 + 20 * i->count;
    PDInstancePrinterRequire(len, len);
    char *bv = *buf;
    PDInteger klen;
    bv[offs++] = '<';
    bv[offs++] = '<';
    bv[offs++] = ' ';
    for (PDInteger j = 0; j < i->count; j++) {
        klen = strlen(i->keys[j]);
        PDInstancePrinterRequire(klen + 10, klen + 10);
        bv = *buf;
        bv[offs++] = '/';
        strcpy(&bv[offs], i->keys[j]);
        offs += klen;
        bv[offs++] = ' ';
        if (! i->values[j] && i->vstacks[j]) {
            PDDictionaryGetEntry(i, i->keys[j]);
        }
        if (i->values[j]) {
            offs = (*PDInstancePrinters[PDResolve(i->values[j])])(i->values[j], buf, offs, cap);
        }
        PDInstancePrinterRequire(4, 4);
        bv = *buf;
        bv[offs++] = ' ';
    }
    bv[offs++] = '>';
    bv[offs++] = '>';
    bv[offs] = 0;
    return offs;
}

#ifdef PD_SUPPORT_CRYPTO

void PDDictionaryAttachCrypto(PDDictionaryRef dictionary, pd_crypto crypto, PDInteger objectID, PDInteger genNumber)
{
    dictionary->ci = PDCryptoInstanceCreate(crypto, objectID, genNumber);
    for (PDInteger i = 0; i < dictionary->count; i++) {
        if (dictionary->values[i]) 
            (*PDInstanceCryptoExchanges[PDResolve(dictionary->values[i])])(dictionary->values[i], dictionary->ci, false);
    }
}

void PDDictionaryAttachCryptoInstance(PDDictionaryRef dictionary, PDCryptoInstanceRef ci, PDBool encrypted)
{
    dictionary->ci = PDRetain(ci);
    for (PDInteger i = 0; i < dictionary->count; i++) {
        if (dictionary->values[i]) 
            (*PDInstanceCryptoExchanges[PDResolve(dictionary->values[i])])(dictionary->values[i], dictionary->ci, false);
    }
}

#endif // PD_SUPPORT_CRYPTO
