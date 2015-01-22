//
//  PDDictionaryStack.c
//  Pods
//
//  Created by Karl-Johan Alm on 21/01/15.
//
//

#include "PDDictionaryStack.h"
#include "PDDictionary.h"
#include "PDNumber.h"
#include "pd_internal.h"
#include "pd_stack.h"

void PDDictionaryStackDestroy(PDDictionaryStackRef ds)
{
    pd_stack_destroy(&ds->dicts);
}

PDDictionaryStackRef PDDictionaryStackCreateWithDictionary(PDDictionaryRef dict)
{
    PDDictionaryStackRef ds = PDAllocTyped(PDInstanceTypeDictStack, sizeof(struct PDDictionaryStack), PDDictionaryStackDestroy, false);
    ds->dicts = NULL;
    pd_stack_push_object(&ds->dicts, PDRetain(dict));
    return ds;
}

void PDDictionaryStackPushDictionary(PDDictionaryStackRef ds, PDDictionaryRef dict)
{
    pd_stack_push_object(&ds->dicts, PDRetain(dict));
}

PDDictionaryRef PDDictionaryStackPopDictionary(PDDictionaryStackRef ds)
{
    return ds->dicts && ds->dicts->prev ? PDAutorelease(pd_stack_pop_object(&ds->dicts)) : NULL;
}

void PDDictionaryStackAddEntriesFromComplex(PDDictionaryStackRef hm, pd_stack stack)
{
    PDDictionaryAddEntriesFromComplex(hm->dicts->info, stack);
}

void PDDictionaryStackSet(PDDictionaryStackRef ds, const char *key, void *value)
{
    PDDictionarySet(ds->dicts->info, key, value);
}

void *PDDictionaryStackGet(PDDictionaryStackRef ds, const char *key)
{
    void *v = NULL;
    pd_stack s = ds->dicts;
    while (!v && s) {
        v = PDDictionaryGet(s->info, key);
        s = s->prev;
    }
    return v;
}

void PDDictionaryStackDelete(PDDictionaryStackRef ds, const char *key)
{
    PDDictionaryDelete(ds->dicts->info, key);
}

void PDDictionaryStackClear(PDDictionaryStackRef ds)
{
    PDDictionaryClear(ds->dicts->info);
}

typedef struct siui *siui;
struct siui {
    PDDictionaryRef iterated;
    PDHashIterator it;
    void *ui;
};

void PDDictionaryStackIterator(char *key, void *value, void *userInfo, PDBool *shouldStop)
{
    siui ui = userInfo;
    if (PDDictionaryGet(ui->iterated, key)) return;
    PDDictionarySet(ui->iterated, key, PDNullObject);
    ui->it(key, value, ui->ui, shouldStop);
}

void PDDictionaryStackIterate(PDDictionaryStackRef ds, PDHashIterator it, void *ui)
{
    struct siui sui;
    sui.iterated = PDDictionaryCreate();
    sui.it = it;
    sui.ui = ui;
    for (pd_stack st = ds->dicts; st; st = st->prev) {
        PDDictionaryIterate(st->info, PDDictionaryStackIterator, &sui);
    }
}
