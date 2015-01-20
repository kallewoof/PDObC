//
// PDState.c
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

#include "Pajdeg.h"
#include "PDScanner.h"
#include "PDOperator.h"
#include "PDState.h"
#include "pd_internal.h"
#include "PDEnv.h"
#include "pd_stack.h"

void PDStateDestroy(PDStateRef state)
{
    PDInteger i;
    if (state->symbols) {
        for (i = state->symbols-1; i >= 0; i--) {
            //printf("releasing %s :: %s (symop)\n", state->name, state->symbol[i]);
            free(state->symbol[i]);
            PDRelease(state->symbolOp[i]);
        }
        free(state->symbol);
        free(state->symbolOp);
    }
    
    if (state->symindex) 
        free(state->symindex);
    
    PDRelease(state->delimiterOp);
    PDRelease(state->numberOp);
    PDRelease(state->fallbackOp);
    free(state->name);
}

PDStateRef PDStateCreate(char *name)
{
    PDStateRef state = PDAllocTyped(PDInstanceTypeState, sizeof(struct PDState), PDStateDestroy, true);
    state->name = strdup(name);
    return state;
}

void PDStateDefineSymbolOperator(PDStateRef state, const char *symbol, PDOperatorRef op)
{
    PDInteger syms = state->symbols;
    state->symbols++;
    state->symbol = realloc(state->symbol, sizeof(char*) * state->symbols);
    state->symbolOp = realloc(state->symbolOp, sizeof(PDOperatorRef) * state->symbols);
    state->symbol[syms] = strdup(symbol);
    state->symbolOp[syms] = PDRetain(op);
}

void PDStateDefineNumberOperator(PDStateRef state, PDOperatorRef op)
{
    if (state->numberOp) PDRelease(state->numberOp);
    state->numberOp = PDRetain(op);
}

void PDStateDefineDelimiterOperator(PDStateRef state, PDOperatorRef op)
{
    if (state->delimiterOp) PDRelease(state->delimiterOp);
    state->delimiterOp = PDRetain(op);
}

void PDStateDefineFallbackOperator(PDStateRef state, PDOperatorRef op)
{
    if (state->fallbackOp) PDRelease(state->fallbackOp);
    state->fallbackOp = PDRetain(op);
}

void PDStateDefineOperatorsWithDefinition(PDStateRef state, const void **defs)
{
    PDOperatorRef op;
    const char *c;
    PDInteger i = 0;
    while (defs[i]) {
        c = defs[i++];
        
        op = PDOperatorCreateFromDefinition((const void **)defs[i++]);
        switch (c[0]) {
            case 'F': // fallback operator
                PDStateDefineFallbackOperator(state, op);
                break;
            case 'D': // delimiter operator
                PDStateDefineDelimiterOperator(state, op);
                break;
            case 'N': // number operator
                PDStateDefineNumberOperator(state, op);
                break;
            case 'S': // symbol operator
                PDStateDefineSymbolOperator(state, &c[1], op);
                break;
            default: // ???
                PDAssert(0); // undefined operator type
                break;
        }
        PDRelease(op);
    }
}


void PDStateCompile(PDStateRef state)
{
    PDInteger i, j;
    if (state->symindex) return; // already compiled
    
    PDInteger symbols = state->symbols;
    short *hashes = calloc(symbols, sizeof(short));
    
    PDInteger slen;
    char *sym;
    short hash;
    for (i = 0; i < symbols; i++) {
        hash = 0;
        sym = state->symbol[i];
        slen = strlen(sym);
        for (j = 0; j < slen; j++) 
            hash -= (PDOperatorSymbolGlob[(unsigned char)sym[j]] - 1) * sym[j];
        hashes[i] = 10 * abs(hash) + slen;
    }
    
    // we want to define the symindex as 2^n >= symbols with minimal collision and minimal n
    PDInteger n = 2;
    PDInteger m;
    while (n < symbols) n <<= 1;    // weak to (very) big symbol tables
    
    short coll = 0;
    PDInteger *index;
    do {
        m = n - 1;
        
        // n = e.g. 64 = 1000000
        // m = e.g. 63 = 0111111

        index = calloc(n, sizeof(PDInteger));
        for (i = 0; i < symbols; i++) {
            hash = hashes[i] & m;
            if (index[hash]) {
                coll++;
                while (hash < n && index[hash]) hash++;
                if (hash == n) {
                    coll = n;
                    hash = 0;
                    i = symbols;
                }
            }
            index[hash] = i+1;
        }
        if (coll + symbols <= n) break;
        free(index);
        n <<= 1;
    } while (true);
    free(hashes);
    
    state->symindices = n;
    state->symindex = index;
    
    for (i = state->symbols - 1; i >= 0; i--) {
        PDOperatorCompileStates(state->symbolOp[i]);
    }
    if (state->delimiterOp) PDOperatorCompileStates(state->delimiterOp);
    if (state->fallbackOp)  PDOperatorCompileStates(state->fallbackOp);
    if (state->numberOp)    PDOperatorCompileStates(state->numberOp);
}

