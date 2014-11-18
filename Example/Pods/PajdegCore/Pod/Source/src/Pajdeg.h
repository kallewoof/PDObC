//
// Pajdeg.h
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
 @mainpage Pajdeg PDF mutation library
 
 @section intro_sec Introduction
 
 Pajdeg is a C library for modifying existing PDF documents by passing them through a stream with tasks assigned based on object ID's, pages, object types, or similar.
 
 @note Unless you need to, using a @link WRAPPERS wrapper @endlink is recommended, as the core C library is fairly low level.
 
 Typical usage involves three things:
 
 - Setting up a PDPipe with in- and out paths.
 - Adding filters and/or mutator PDTasks to the pipe.
 - Executing.
 
 Tasks can be chained together, and appended to the stream at any time through other tasks, with the caveat that the requested object has not yet passed through the stream.
 
 @section features Features
 
 Pajdeg supports the following notable PDF related features:
 
 - Stream compression/decompression (via zlib)
 - Predictors
 - Standard encryption/decryption (RC4, not AES)
 - Object streams (PDF 1.5+ feature)
 - PDF catalogs (page-to-object mapping)
 
 More sophisticated functionality (such as manipulating annotations) can be found in @link WRAPPERS wrappers @endlink.
 
 @section quick_start Quick Start

 To get started with Pajdeg, you may want to 

 - @link SETUP set it up @endlink in your project
 - check out the @link QUICKSTART quick start page @endlink
 - check out the @link PDUSER user level module list @endlink
 
 @section helping_out Helping out
 
 One of the design goals with Pajdeg is that it should have minimal dependencies and be as broadly available as possible. As a consequence of this, some core components have been implemented in a rather crude form and are in desperate need of optimizing, in particular
 
 - the binary tree implementation is not balanced (pd_btree)
 - the static hash implementation is a bit of a hack
 
 Beyond the gorey detail stuff, there's also a big need for more, better and/or improved wrappers for other languages!
 
 And finally, especially if you're not technically inclined but still want to help out, throwing lots and lots of different PDF's at the library and making sure it does the right thing is crucial to taking it to the next step, ensuring it is 100% compatible and functional with all well-formed (and some malformed) PDF's out there.
 
 @section dependencies_sec Dependencies
 
 Pajdeg has very few dependencies. While the aim was to have none at all, certain PDFs require compression to function at all.
 
 Beyond this, Pajdeg does not contain any other dependencies beyond a relatively modern C compiler.
 
 @subsection libz_subsec libz
 
 Pajdeg can use libz for stream compression. Some PDFs, in particular PDFs made using the 1.5+ specification, require libz support to be parsed via Pajdeg, because the cross reference table can be a compressed object stream. If support for this is not desired, libz support can be disabled by removing the PD_SUPPORT_ZLIB definition in PDDefines.h.
 
 @section integrating_sec Integrating
 
 Adding Pajdeg to your project can be done by either compiling the static library using the provided Makefile, and putting the library and the .h files into your project, or by adding the .c and .h files directly. 
 */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 @page WRAPPERS Pajdeg Wrappers
 
 As Pajdeg is still fairly new (to the public), there's only one wrapper available. If you write one (even for a language already listed) please let me know so I can add it to this list!
 
 - @link PDOBC PDObC @endlink (Objective-C)
 */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 @page PDOBC PDObC (Objective-C Wrapper)
 
 The Objective-C wrapper has its own documentation set for Xcode, as well as its own GitHub repository at 
 
 http://github.com/kallewoof/PDObC
 */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 @page SETUP Setting up
 
 It's arguable whether a page dedicated to setting up is even necessary, but regardless. The first sections cover general instructions, followed by specific instructions, and last is a troubleshooting section. Right now, Xcode is the only example. If you have instructions for a different environment, please contribute!
 
 @section setup_sec Setting up
 
 Pajdeg is meant to be included in some other project, either as a static library built e.g. using the included Makefile, or by inserting the .c and .h files directly into a project. 
 
 @subsection setup_static_sec As a static library
 
 - Compile the sources using the provided Makefile, by doing `make` either from the `Pajdeg` directory or the `Pajdeg/src` directory. You should get a `libpajdeg.a` file in the current directory regardless which one you use.
 - Copy `libpajdeg.a` along with `*.h` from `src/` into your project
 - Include `libpajdeg.a` when compiling code that uses the library, e.g. `gcc minimal.c -lz ../libpajdeg.a -o minimal` from the `Pajdeg/examples` folder. 
 
 @subsection setup_xcode_sec Xcode
 
 For Xcode (4.x), dragging the Pajdeg folder as is into an existing project is all you need to do (and add the libz framework, if necessary), but if Xcode suggests adding a build phase for the included Makefiles, **you should skip that part** as the files will compile fine as is.
 
 @section setup_trouble_sec Troubleshooting
 
 @subsection setup_trouble_undefined_symbols_sec Undefined symbols
 
 If you get something like this:
 @code 
 Undefined symbols for architecture x86_64:
 "_deflate", referenced from:
 _fd_compress_proceed in libpajdeg.a(PDStreamFilterFlateDecode.o)
 "_deflateEnd", referenced from:
 _fd_compress_done in libpajdeg.a(PDStreamFilterFlateDecode.o)
 "_deflateInit_", referenced from:
 ...
 ld: symbol(s) not found for architecture x86_64
 collect2: ld returned 1 exit status
 @endcode
 
 you forgot to include libz (`-lz` flag for GCC/LLDB, or the libz framework in Xcode or whatever IDE you're using).
 
 If the function names are prefixed with `PD` or `_PD`, however, Pajdeg itself is not being linked into your executable correctly. (This may happen when developing iOS applications, if building Pajdeg inside of or as a sub-project, and neglecting to turn off `Build Active Architecture Only` for said sub-project.)
 
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 @page QUICKSTART Quick Start
 
 This guide goes through the example code in `examples` folder. You can compile these as you go by doing 
 
 @code gcc THEFILENAME -lz ../libpajdeg.a -o example @endcode
 
 from within the `Pajdeg/examples` folder, presuming you've done `make` in `Pajdeg` first.
 
 The quick guide goes through a number of examples of increasing complexity:
 - @subpage minimal
 - @subpage addmetadata
 - @subpage replacemetadata
 
 */

//--------------------------------------------------------------------------------

/**
 @page minimal Minimal example
 
 From @c samples/minimal.c :
 
 @include minimal.c
 
 This example takes the input PDF (first argument) and pipes it through Pajdeg to the output path (second argument) without applying any tasks to it. (The resulting PDF will most likely differ from the first, binary-wise, but they will be identical when viewed in a PDF viewer.)
 
 - PDPipeCreateWithFilePaths() sets up a new pipe, ensuring that the paths are valid,
 - PDPipeExecute() initiates the actual piping operation, which will execute all the way to the end of the input file, and
 - PDRelease() drops our reference to the pipe, so it can be cleaned up.
 
 Next up is @ref addmetadata "Adding metadata to a PDF"
 
 */

//--------------------------------------------------------------------------------

/**
 
 @page addmetadata Adding metadata to a PDF
 
 From @c samples/add-metadata.c :
 
 @dontinclude add-metadata.c
 
 We now want to add metadata to an existing PDF. If the PDF has metadata already, we explode, but that's fine, we'll deal with that soon. The first new thing we have to do is declare a *mutator* task function above `main`.
 
 @skip PDTaskResult
 @until PDObjectRef object
 
 It takes four arguments: the pipe, its owning task, the object it's supposed to do its magic on, and an info object that we can use to store info in. We'll get to this function in a bit.
 
 The next thing we have to do is create a new object. This is actually done straight off without using tasks. It may be a little confusing at first, but a mutator *mutates/changes* something, it never *creates* anything. 
 
 In any case, to create objects, we need to introduce a new friend, the parser. In our `main()`:
 
 @skip get the pipe
 @until NewObj
 
 The object has been given a unique object ID and is set up, ready to be stuffed into the output PDF as soon as we hit `execute`. 
 
 We want to tweak it first, of course. Here, we're setting the metadata to our own string. The three flags at the end are used to tell the object if we want it to set the Length property using our provided length, whether the passed buffer needs to be freed after the object is finished using it, and whether the passed content is encrypted or not. If you `strdup()`'d a string and passed it in, you would give `true` as the second argument, unless you planned to `free()` it yourself once you knew the object was done with it.
 
 @skip char
 @until SetStream
 
 Adding a metadata object is fine and all, but it won't do any good unless we point the PDF's root object at the new metadata object. That's what our task from before is for.
 
 @skip rootUpdater
 @until SetInfo
 
 We're creating a mutator task for the "root object" property type (i.e. the root object of the PDF), and we're passing our `addMetadata` function to it, and finally we're setting the `info` object to the `meta` object we made earlier. 
 
 Under the hood, this sets up a filter task for the root object's object ID, and attaches a mutator task to that filter task. The filter task will be pinged every time an object passes through the pipe and if the filter encounters the object whose ID matches the root object of the PDF, it will trigger its mutator task and hand it the object in question. That mutator task is our `addMetadata` function.
 
 Which we will get to very soon. Before we do, though, there are a few things left: adding our task to the pipe, 
 
 @skip PDPipeAdd
 @until ;
 
 executing the pipe, 
 
 @skip Exec
 @until processed
 
 and some clean-up.
 
 @skip PDRel
 @until meta
 
 Caution: releasing the meta object before calling PDPipeExecute() will cause a crash, because `addMetadata` uses it and `addMetadata` is not called until PDPipeExecute() has been called.
 
 The last part is the actual task callback. 
 
 @skip PDTaskRes
 @until {
 
 We could blindly change the root object's Metadata key to point to our new object. We could, but it would be very bad. We would leave a potentially huge abandoned object in the resulting PDF. Even worse, a PDF would have as many metadata objects as it had gone through our pipe, since we would be adding a new one every time.
 
 @until }

 Here, we are using a PDDictionary for the first time. It's simply a key/value pair container, used to represent dictionaries in PDFs. We can get the dictionary associated with a PDObject using PDObjectGetDictionary(). There is a corresponding PDObjectGetArray() for array type objects, and so on.
 
 In any case, if PDDictionaryGetEntry() returns a non-NULL value for the "Metadata" key, we explode. With that out of the way, setting the metadata is fairly straightforward.

 Our meta object is the info, passed to the task:
 
 @skip PDObj
 @until info

 We put this into the dictionary as the Metadata value:

 @skip PDDictionarySetEntry
 @until meta

 Note that while meta is a PDObject, by setting a PDDictionary entry's value to a PDObject, it will ultimately end up being a PDReference value. In other words, objects will translate into "<object id> <generation number> R" in a PDDictionary, when written to a PDF.

 Finally we return PDTaskDone to signal that we're finished:
 
 @skip return
 @until }
 
 Put together, this is what it all looks like:
 
 @include add-metadata.c
 
 You can check out a dissection of a `diff` resulting from a tiny PDF when piped using this program on the @subpage addmetadatadiff "Add metadata diff example" page.
 
 In the next part we'll be conditionally replacing or inserting metadata depending on whether it exists or not. There are a couple of ways of doing this, such as always deleting the current medata object and putting in a new one, but we're going to replace or insert. 
 
 Next up: @ref replacemetadata "Replacing metadata"
 
 */

//--------------------------------------------------------------------------------

/**
 
 @page addmetadatadiff Add metadata diff example
 
 This demonstrates what the modifications Pajdeg make to a PDF end up looking like, byte-wise. You can check this out yourself by doing `diff -a` on the original and new PDF files after using Pajdeg.
 
 @dontinclude outputs/output-example-add-metadata.diff
 
 @until endobj
 
 First off, we see a number of lines added to the new PDF that weren't in the old one. It's an object (ID 21, generation number 0) with a stream whose length is 12 bytes, and the stream itself consists of the 12 characters "Hello World!".
 
 @until 209c
 
 Next up we see a replaced PDF dictionary. If you look closer, you'll notice that the only change is that `/Metadata 21 0 R` was added to the new PDF.
 
 @until 232c
 
 At the very top, `0 21` is replaced with `0 22`. This is the XREF (cross reference) header, which is changed because the PDF has an extra object (our metadata object). This follows by a chunk of lines being replaced. These are XREF entries, and almost all of them have been updated, because almost every single object's byte position in the PDF has changed. That's unfortunate but normal. It can be alleviated to some extent when making small modifications by opting for the PDParserCreateAppendedObject() method over PDParserCreateNewObject(), but shouldn't be thought too hard into.
 
 @until 234c
 
 Next up is the *trailer object* which has been updated as well: `/Size` has been set to 22, because we now have 22 objects in the PDF.
 
 That's the whole diff. Chances are you won't get quite as clean results, because the original PDF here originated from Pajdeg, which often is not the case.
 
 Back to @ref addmetadata "Adding metadata to a PDF"
 
 */

//--------------------------------------------------------------------------------

/**
 
 @page replacemetadata Replacing metadata
 
 The code here builds on the @ref addmetadata "Adding metadata" example.
 
 From @c samples/replace-metadata.c :
 
 @dontinclude replace-metadata.c
 
 The idea is as follows: we check if there's a metadata entry; if there is, we update it, and if not, we create one and update the root object.
 
 We need two tasks declared for this.
 
 @skip mutator
 @until updateMeta
 
 Next, we need to do the actual conditional part in our `main()` function.
 
 @skip GetRoot
 @until mutator
 
 First case: we have a metadata entry; we plug the `updateMetadata` task into the metadata object.
 
 @until updateMeta
 
 Second case: we do what we did before; create a metadata object, stuff it with stuff, and hook a task up to edit the root object. We're using a different method for creating the object here. You may run into reasons for choosing one over the other one day.
 
 @until }
 @until }
 
 We then add the task (whichever one it was) and release it (it's retained by the PDPipe). 
 
 @skip AddT
 @until Release
 
 The task for adding metadata remains the same, which leaves the task for updating it:
 
 @skip updateMetadata
 @until }
 
 The resulting file ends up looking like this:

 @include replace-metadata.c
 
 And this is what it looks like when used:
 
 @include outputs/output-example-replace-metadata.txt
 
 */

#ifndef INCLUDED_PAJDEG_H
#   define INCLUDED_PAJDEG_H

#   define PAJDEG_VERSION   "0.1.0"

#   include "PDPipe.h"
#   include "PDObject.h"
#   include "PDTask.h"
#   include "PDParser.h"
#   include "PDReference.h"
#   include "PDScanner.h"
#endif
