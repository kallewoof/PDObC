//
//  PDTaskBlocks.m
//
//  Copyright (c) 2013 Karl-Johan Alm (http://github.com/kallewoof)
// 
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
// 
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
// 
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#import "PDTaskBlocks.h"
#import "PDInternal.h"
#import "PDStaticHash.h" // just want as(), and don't wanna put it in a more public place in case other as()'es exist

extern void PDTaskDealloc(void *ob);

PDTaskResult PDBlockLauncher(PDPipeRef pipe, PDTaskRef task, PDObjectRef object)
{
    return as(PDTaskBlock, task->info)(pipe, task, object);
}

void PDBlockTaskDealloc(void *ob) 
{
    [as(PDTaskBlock, as(PDTaskRef, ob)->info) release];
    PDTaskDealloc(ob);
}

PDTaskRef PDTaskCreateBlockMutator(PDTaskBlock mutatorBlock)
{
    PDTaskRef task = PDTaskCreateMutator(&PDBlockLauncher);
    task->deallocator = &PDBlockTaskDealloc;
    task->info = [mutatorBlock copy];
    return task;
}

