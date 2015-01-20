//
// pd_pdf_private.h
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

#ifndef INCLUDED_pd_pdf_private_h
#define INCLUDED_pd_pdf_private_h

#define PDDeallocateViaStackDealloc(ob) pd_stack_dealloc(ob)

#define currch  (scv->allocBuf)[scv->offs]
#define currchi scv->left--; (scv->allocBuf)[(scv->offs)++]
#define currstr &currch
#define iterate(v) scv->offs += v; scv->left -= v

#define putfmt(fmt...)  \
    sz = sprintf(currstr, fmt); \
    iterate(sz)

#define putstr(str, len) \
    memcpy(currstr, str, len); \
    iterate(len)

#define PDStringGrow(req) \
    if (scv->left < req) { \
        scv->allocBuf = realloc(scv->allocBuf, scv->offs + req); \
        scv->left = req; \
    }

#define PDStringFromObRef(ref, reflen) \
    PDInteger obid = pd_stack_pop_int(s); \
    PDInteger genid = pd_stack_pop_int(s); \
    PDInteger sz;\
    \
    char req = 5 + reflen + 2; \
    if (obid > 999) req += 3; \
    if (genid > 99) req += 5; \
    PDStringGrow(req); \
    putfmt("%ld %ld " ref, obid, genid)

// get primitive if primtiive, otherwise delegate to arbitrary func
#define PDStringFromAnything() \
    if ((*s)->type == PD_STACK_STRING) {\
        char *str = pd_stack_pop_key(s);\
        PDInteger len = strlen(str);\
        PDStringGrow(len);\
        putstr(str, len);\
        PDDeallocateViaStackDealloc(str); \
    } else {\
        pd_stack co = pd_stack_pop_stack(s); \
        PDStringFromArbitrary(&co, scv);\
    }

#endif
