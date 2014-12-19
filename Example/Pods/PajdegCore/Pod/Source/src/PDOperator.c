//
// PDOperator.c
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

#include "Pajdeg.h"
#include "PDOperator.h"
#include "PDState.h"

#include "pd_internal.h"

char *PDOperatorSymbolsWhitespace = "\x00\x09\x0A\x0C\x0D ";    // 0, 9, 10, 12, 13, 32 (character codes)
char *PDOperatorSymbolsDelimiters = "()<>[]{}/%";               // (, ), <, >, [, ], {, }, /, % (characters)
//char *PDOperatorSymbolsNumeric = "0123456789";                  // 0-9 (character range)
//char *PDOperatorSymbolsAlpha = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";        // a-zA-Z 
//char *PDOperatorSymbolsAlphanumeric = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; // a-zA-Z0-9

char *PDOperatorSymbolGlob = NULL;
char *PDOperatorSymbolGlobHex = NULL;
char *PDOperatorSymbolGlobEscaping = NULL;
char *PDOperatorSymbolGlobDehex = NULL;

void PDOperatorSymbolGlobSetup()
{
    if (PDOperatorSymbolGlob != NULL) return;
    
    PDInteger i;
    
    PDOperatorSymbolGlob = calloc(256, sizeof(char));
    for (i = strlen(&PDOperatorSymbolsWhitespace[1]); i >= 0; i--)
        PDOperatorSymbolGlob[(unsigned char)PDOperatorSymbolsWhitespace[i]] = PDOperatorSymbolGlobWhitespace;
    for (i = strlen(PDOperatorSymbolsDelimiters)-1; i >= 0; i--) 
        PDOperatorSymbolGlob[(unsigned char)PDOperatorSymbolsDelimiters[i]] = PDOperatorSymbolGlobDelimiter;
    
    PDOperatorSymbolGlobHex = calloc(256, sizeof(char));
    PDOperatorSymbolGlobDehex = calloc(16, sizeof(char));
    PDOperatorSymbolGlobEscaping = calloc(256, sizeof(char));

    for (i = '0'; i <= '9'; i++) {
        PDOperatorSymbolGlobDehex[i - '0'] = i;
        PDOperatorSymbolGlobHex[i] = i - '0';
        PDOperatorSymbolGlobEscaping[i] = 1;
    }
    
    char aAoffs = 'a' - 'A';
    for (i = 'A'; i <= 'F'; i++) {
        PDOperatorSymbolGlobDehex[10 + i - 'A'] = i;
        PDOperatorSymbolGlobHex[i] = PDOperatorSymbolGlobHex[i + aAoffs] = 10 + i - 'A';
        PDOperatorSymbolGlobEscaping[i] = PDOperatorSymbolGlobEscaping[i + aAoffs] = 1;
    }

    for (i = 'G'; i <= 'Z'; i++) {
        PDOperatorSymbolGlobEscaping[i] = PDOperatorSymbolGlobEscaping[i + aAoffs] = 1;
    }
    
#define _(z) PDOperatorSymbolGlobEscaping[z]
    _(' ') = _('.') = _('_') = _(',') = _('!') = _('@') = _('#') = _('$') = _('%') = _('^') = _('&') = _('*') = _('-') = _('=') = _('+') = _('[') = _(']') = _('{') = _('}') = _(';') = _('\'') = _('"') = _('/') = _('<') = _('>') = _('~') = _('|') = 1;
    _('\t') = 't';
    _('\n') = 'n';
    _('\r') = 'r';
    _('\b') = 'b';
    _('\\') = '\\';
    _('\f') = 'f';
    _('\a') = 'a';
    _('(') = '(';
    _(')') = ')';
#undef _
    
}

char PDOperatorSymbolGlobDefine(char *str)
{
    if (PDOperatorSymbolGlob[(unsigned char)str[0]] == PDOperatorSymbolGlobDelimiter) 
        return PDOperatorSymbolGlobDelimiter;

    char c;
    PDBool numeric, real;
    numeric = true;
    real = false;
    PDSymbolDetermineNumeric(numeric, real, c, str, strlen(str));

    return (numeric
            ? PDOperatorSymbolExtNumeric 
            : PDOperatorSymbolGlobRegular);
}

void PDOperatorDestroy(PDOperatorRef op)
{
    switch (op->type) {
        case PDOperatorPopVariable:
            free(op->key);
            break;
        case PDOperatorPushState:
            PDRelease(op->pushedState);
        default:
            break;
    }
    if (op->next) {
        PDRelease(op->next);
    }
}

PDOperatorRef PDOperatorCreate(PDOperatorType type)
{
    PDOperatorRef op = PDAllocTyped(PDInstanceTypeOperator, sizeof(struct PDOperator), PDOperatorDestroy, false);
    op->type = type;
    op->next = NULL;
    op->key = NULL;
    return op;
}

PDOperatorRef PDOperatorCreateWithKey(PDOperatorType type, const char *key)
{
    PDOperatorRef op = PDOperatorCreate(type);
    op->key = strdup(key);
    return op;
}

PDOperatorRef PDOperatorCreateWithPushedState(PDStateRef pushedState)
{
    PDOperatorRef op = PDOperatorCreate(PDOperatorPushState);
    op->pushedState = PDRetain(pushedState);
    return op;
}

void PDOperatorAppendOperator(PDOperatorRef op, PDOperatorRef next)
{
    PDAssert(op != next);
    while (op->next) {
        op = op->next;
        PDAssert(op != next);
    }
    op->next = PDRetain(next);
}

PDOperatorRef PDOperatorCreateFromDefinition(const void **defs)
{
    PDInteger i = 0;
    PDOperatorRef result, op, op2;
    result = op = NULL;
    PDOperatorType t;
    while (defs[i]) {
        t = (PDOperatorType)defs[i++];
        switch (t) {
            case PDOperatorPopVariable:
                op2 = PDOperatorCreateWithKey(t, defs[i++]);
                break;
            case PDOperatorPullBuildVariable:
            case PDOperatorStoveComplex:
            case PDOperatorPushComplex:
                op2 = PDOperatorCreate(t);
                op2->identifier = (PDID)defs[i++];
                break;
            case PDOperatorPushState:
            case PDOperatorPushWeakState:
                op2 = PDOperatorCreateWithPushedState((PDStateRef)defs[i++]);
                if (t == PDOperatorPushWeakState) {
                    op2->type = PDOperatorPushWeakState;
                    PDRelease(op2->pushedState);
                }
                break;
            default:
                op2 = PDOperatorCreate(t);
                break;
        }
        if (op) {
            /// @todo CLANG doesn't like homemade retaining
            PDOperatorAppendOperator(op, op2);
            PDRelease(op2);
            op = op2;
        } else {
            result = op = op2;
        }
    }
    return result;
}

void PDOperatorCompileStates(PDOperatorRef op)
{
    while (op) {
        if (op->type == PDOperatorPushState || op->type == PDOperatorPushWeakState) 
            PDStateCompile(op->pushedState);
        op = op->next;
    }
}
