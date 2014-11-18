//
// PDPipe.h
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
 @file PDPipe.h Pipe header file.
 
 @ingroup PDPIPE
 
 @defgroup PDPIPE PDPipe
 
 @brief The Pajdeg pipe.
 
 @memberof PDTYPE
 
 @ingroup PDPIPE_CONCEPT
 
 Pipes provide functionality to define input and output files, add tasks, and start the streaming process. 
 
 Pipes contain a @link PDPARSER PDParser @endlink instance, accessible through the PDPipeGetParser() function, and any number of @link PDTASK PDTask @endlink instances. It also maintains a @link PDTWINSTREAM PDTwinStream @endlink instance, along with file pointers for the input and output files.
 
 The life cycle of a pipe that does not encounter any errors is made up of these 6 steps:
 
 1. define in/out files (on creation)
 2. add tasks/filters
 3. prepare pipe
 4. add tasks/filters
 5. execute
 6. destroy
 
 It is perfectly acceptable to add tasks from within a task, i.e. in the middle of an execution call, although be warned that the mutability of objects is subject to their location within the PDF file. The following sequence is fine:
 
 - ...
 - Object #1 > initial task object > creation of task for Object #2
 - ...
 - Object #2 > task for Object #2
 - ...
 
 whereas this sequence will trigger an assertion:

 - ...
 - Object #2
 - ...
 - Object #1 > initial task object > creation of task for Object #2

 To resolve this, there are a number of options:
 
 1. Object #1 can be fetched directly, rather than have a filtered task attacked to it, before the execute call. Object #2's identity can then be resolved without waiting for Object #1 in the stream, and a task can be made for Object #2 directly.
 2. If the Object #2 task is optional, it can be wrapped in a conditional based on PDParserIsObjectStillMutable().
 
 ## Prepared pipes
 
 A pipe that has opened the in- and output file handlers, configured a stream, and configured a parser, is considered "prepared". Pipes are not prepared by default, but many operations indirectly result in PDPipePrepare being called. 
 
 @{
 */

#ifndef INCLUDED_PDPipe_h
#define INCLUDED_PDPipe_h

#include "PDDefines.h"

/**
 Create a pipe with an input PDF file and an output PDF file.
 
 @param inputFilePath   The input PDF file (must be readable and exist).
 @param outputFilePath  The output PDF file (must be readwritable). If the file exists, it is overwritten. The file may not be the same as inputFilePath. On most operating systems, "/dev/null" can be passed to provide a readonly session (not MS Windows), but this must be explicitly passed, and NULL will result in failure.
 @return The PDPipeRef instance, or NULL if the pipe cannot be set up.
 */
extern PDPipeRef PDPipeCreateWithFilePaths(const char * inputFilePath, const char * outputFilePath);

/**
 Attach a task to a pipe. 
 
 The task is attached to the end. This was not the case up until Pajdeg 0.0.7.
 
 @param pipe The pipe. If the pipe is not prepared, and the task is a filter on the root or info object, this will trigger PDPipePrepare().
 @param task The task to add.
 
 @note In the current implementation, non-filter tasks which are added directly to the pipe will be triggered *once* when PDPipePrepare() is called, not once per object iteration.
 */
extern void PDPipeAddTask(PDPipeRef pipe, PDTaskRef task);

/**
 Prepares the pipe for execution, by setting up the streams and parser. 
 
 The parser will at this point open the input stream and fetch the XREF table and trailer data, which includes key refererences such as Info and Root. After this and before Execute, definitions for objects may be obtained as stack refs via the parser object's PDParserLocateAndCreateDefinitionForObject.
 
 @param pipe The pipe to prepare. If the pipe is already prepared, this does nothing.
 @return true if the pipe was prepared already, or if preparations were successful; false if an error occurred.
 */
extern PDBool PDPipePrepare(PDPipeRef pipe);

/**
 Get parser instance for pipe.
 
 @param pipe The pipe. If the pipe is not prepared, PDPipePrepare() will be called.
 @return The parser associated with the pipe.
 */
extern PDParserRef PDPipeGetParser(PDPipeRef pipe);

/**
 Get the root object.
 
 @param pipe The pipe. If the pipe is not prepared, PDPipePrepare() will be called.
 @return The PDObjectRef instance of the root object. 
 
 @warning The returned object is silently immutable. To perform mutations on the root object, add an object mutator to the pipe using the ID of the root object.
 */
extern PDObjectRef PDPipeGetRootObject(PDPipeRef pipe);

/**
 Perform the pipe operation, reading input file, performing all defined tasks, and writing the results to the output file.
 
 @param pipe The pipe. If the pipe is not prepared, PDPipePrepare() will be called.
 @return Returns # of objects seen on success, -1 on failure.
 
 @note After executing a pipe, it can in theory be reused, but this is currently experimental. Recreating a pipe is recommended for multi-pass operations.
 */
extern PDInteger PDPipeExecute(PDPipeRef pipe);

/**
 Get pipe input file path.
 
 @param pipe The pipe.
 @return The input file path, as provided when the pipe was created.
 */
extern const char *PDPipeGetInputFilePath(PDPipeRef pipe);

/**
 Get pipe output file path.
 
 @param pipe The pipe.
 @return The output file path, as provided when the pipe was created.
 */
extern const char *PDPipeGetOutputFilePath(PDPipeRef pipe);

/**
 *  Connect a foreign parser into pipe, making it possible to import objects from the foreign parser throughout the pipe's existence.
 *
 *  This sets up a permanent parser attachment to the foreign parser. Some operations, such as PDPageInsertIntoPipe, will
 *  do this automatically.
 *
 *  Multiple calls to connecting the same pipe/parser pair are ignored.
 *
 *  @param pipe          The native pipe into which importing of objects will be possible
 *  @param foreignParser The foreign parser from which imports should be possible
 *
 *  @return Parser attachment associated with the connection
 */
extern PDParserAttachmentRef PDPipeConnectForeignParser(PDPipeRef pipe, PDParserRef foreignParser);

#endif

/** @} */
