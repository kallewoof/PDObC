//
// PDTask.h
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

/**
 @file PDTask.h Task header file.
 
 @ingroup PDTASK
 
 @defgroup PDTASK PDTask
 
 @brief A Pajdeg pipe task.
 
 @ingroup PDPIPE_CONCEPT
 
 @{
 */

#ifndef INCLUDED_PDTask_h
#define INCLUDED_PDTask_h

#include "PDDefines.h"

///---------------------------------------
/// @name Creating and manipulating filters
///---------------------------------------

/**
 Create a filtering task. Filtering tasks find objects in a pipe based on given criteria and forward these to their child tasks.
 */
extern PDTaskRef PDTaskCreateFilter(PDPropertyType propertyType);

/**
 Create a filtering task with the given argument.
 */
extern PDTaskRef PDTaskCreateFilterWithValue(PDPropertyType propertyType, PDInteger value);

/**
 Create a mutator task. Mutator tasks receive objects from the pipe if their parent tasks pass them through. A mutator task can be a filter, if it uses PDTaskSkipRest to abort for objects that should be skipped.
 */
extern PDTaskRef PDTaskCreateMutator(PDTaskFunc mutatorFunc);

/**
 *  Append a child task to a task. Child tasks are executed after their parent tasks, unless the parent decides to stop the chain. 
 *  The child is appended to the end, as one would intuitively expect. Adding child A, followed by adding child B, to parent P, will result in the execution order
 *
 *   P -> A -> B
 */
extern void PDTaskAppendTask(PDTaskRef parentTask, PDTaskRef childTask);

/**
 Set the info object for a task. 
 
 The info object is passed as is to the task upon execution. 
 
 @param task The task.
 @param info The info object.
 */
extern void PDTaskSetInfo(PDTaskRef task, void *info);

/**
 Execute a task, possibly resulting in a chain of tasks executing if the task has children.
 */
extern PDTaskResult PDTaskExec(PDTaskRef task, PDPipeRef pipe, PDObjectRef object);

///---------------------------------------
/// @name Convenience functions
///---------------------------------------

/**
 Create a mutator for the given object ID.
 */
extern PDTaskRef PDTaskCreateMutatorForObject(long objectID, PDTaskFunc mutatorFunc);

/**
 Create a mutator for the given (value-less) property type.
 */
extern PDTaskRef PDTaskCreateMutatorForPropertyType(PDPropertyType propertyType, PDTaskFunc mutatorFunc);

/**
 Create a mutator for the given property type with the given value.
 */
extern PDTaskRef PDTaskCreateMutatorForPropertyTypeWithValue(PDPropertyType propertyType, PDInteger value, PDTaskFunc mutatorFunc);

#endif

/** @} */

/** @} */


