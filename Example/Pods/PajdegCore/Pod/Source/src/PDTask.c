//
// PDTask.c
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

#include "pd_internal.h"

#include "PDTask.h"

void PDTaskDealloc(void *ob)
{
    PDTaskRef task = ob;
    PDRelease(task->child);
}

PDTaskResult PDTaskExec(PDTaskRef task, PDPipeRef pipe, PDObjectRef object)
{
    PDTaskRef parent = NULL;
    
    PDTaskResult res = PDTaskDone;
    
    while (task) {
        if (task->isActive)
            res = (*task->func)(pipe, task, object, task->info);
        else 
            res = PDTaskDone;
        
        if (PDTaskUnload == res) {
            // we can remove this task internally
            task->isActive = false;
            if (parent) {
                // we can remove it for real
                PDTaskRef child = task->child;
                task->child = NULL;
                parent->child = child;
                PDRelease(task);
                task = parent;
            }
        } 
        else if (PDTaskDone != res) {
            break;
        }
        
        parent = task;
        task = task->child;
    }
    
//    while (task && PDTaskDone == (res = (*task->func)(pipe, task, object, task->info))) {
//        parent = task;
//        task = task->child;
//    }
//    
//    if (parent && task && PDTaskUnload == res) {
//        // we can remove this task internally
//        res = task->child == NULL ? PDTaskDone : PDTaskSkipRest;
//        parent->child = NULL;
//        PDRelease(task);
//    }
    
    return res;
}

void PDTaskDestroy(PDTaskRef task)
{
    (*task->deallocator)(task);
}

PDTaskRef PDTaskCreateFilterWithValue(PDPropertyType propertyType, PDInteger value)
{
    PDTaskRef task = PDAllocTyped(PDInstanceTypeTask, sizeof(struct PDTask), PDTaskDestroy, false);
    task->isActive     =  true;
    task->deallocator  = &PDTaskDealloc;
    task->isFilter     = 1;
    task->propertyType = propertyType;
    task->value        = value;
    task->child        = NULL;
    return task;
}

PDTaskRef PDTaskCreateFilter(PDPropertyType propertyType)
{
    return PDTaskCreateFilterWithValue(propertyType, -1);
}

PDTaskRef PDTaskCreateMutator(PDTaskFunc mutatorFunc)
{
    PDTaskRef task = PDAlloc(sizeof(struct PDTask), PDTaskDestroy, false);
    task->isActive     =  true;
    task->deallocator  = &PDTaskDealloc;
    task->isFilter     = 0;
    task->func         = mutatorFunc;
    task->child        = NULL;
    task->info         = NULL;
    return task;
}

void PDTaskAppendTask(PDTaskRef parentTask, PDTaskRef childTask)
{
    PDTaskRef childParentTask = parentTask;
    while (childParentTask->child)
        childParentTask = childParentTask->child;
    childParentTask->child = PDRetain(childTask);
}

void PDTaskSetInfo(PDTaskRef task, void *info)
{
    if (task->isFilter)
        task = task->child;
    task->info = info;
}

//
// Convenience non-core
//

PDTaskRef PDTaskCreateMutatorForObject(long objectID, PDTaskFunc mutatorFunc)
{
    return PDTaskCreateMutatorForPropertyTypeWithValue(PDPropertyObjectId, objectID, mutatorFunc);
}

PDTaskRef PDTaskCreateMutatorForPropertyType(PDPropertyType propertyType, PDTaskFunc mutatorFunc)
{
    return PDTaskCreateMutatorForPropertyTypeWithValue(propertyType, -1, mutatorFunc);
}

PDTaskRef PDTaskCreateMutatorForPropertyTypeWithValue(PDPropertyType propertyType, PDInteger value, PDTaskFunc mutatorFunc)
{
    PDTaskRef filter;
    PDTaskRef mutator;
    
    filter = PDTaskCreateFilterWithValue(propertyType, value);
    mutator = PDTaskCreateMutator(mutatorFunc);
    PDTaskAppendTask(filter, mutator);
    PDRelease(mutator);
    
    return filter;
}

