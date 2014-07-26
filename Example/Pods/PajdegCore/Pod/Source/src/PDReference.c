//
// PDReference.c
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

#include "PDReference.h"
#include "pd_internal.h"
#include "pd_stack.h"
#include "pd_pdf_implementation.h"

void PDReferenceDestroy(PDReferenceRef reference)
{
}

PDReferenceRef PDReferenceCreateFromStackDictEntry(pd_stack stack)
{
    PDReferenceRef ref = PDAllocTyped(PDInstanceTypeRef, sizeof(struct PDReference), PDReferenceDestroy, false);
    
    // ("de"), <key>, {ref, 789, 0}
    if (PDIdentifies(stack->info, PD_DE))
        stack = stack->prev->prev->info;
    
    // ref, 789, 0
    ref->obid = PDIntegerFromString(stack->prev->info);
    ref->genid = PDIntegerFromString(stack->prev->prev->info);
    return ref;
}

PDReferenceRef PDReferenceCreate(PDInteger obid, PDInteger genid)
{
    PDReferenceRef ref = PDAllocTyped(PDInstanceTypeRef, sizeof(struct PDReference), PDReferenceDestroy, false);
    ref->obid = obid;
    ref->genid = genid;
    return ref;
}

PDInteger PDReferenceGetObjectID(PDReferenceRef reference)
{
    return reference->obid;
}

PDInteger PDReferenceGetGenerationID(PDReferenceRef reference)
{
    return reference->genid;
}

PDInteger PDReferencePrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap)
{
    PDInstancePrinterInit(PDReferenceRef, 12, 50);
    char *bv = *buf;
    return offs + sprintf(&bv[offs], "%ld %ld R", i->obid, i->genid);;
}
