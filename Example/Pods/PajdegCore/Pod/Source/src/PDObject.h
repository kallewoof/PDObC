//
// PDObject.h
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

/**
 @file PDObject.h PDF object header file.
 
 @ingroup PDOBJECT

 @defgroup PDOBJECT PDObject
 
 @brief A PDF object.
 
 @ingroup PDUSER

 Objects in PDFs range from simple numbers indicating the length of some stream somewhere, to streams, images, and so on. In fact, the only things other than objects in a PDF, at the root level, are XREF (cross reference) tables, trailers, and the "startxref" marker.
 
 ### Pajdeg objects are momentarily mutable. 
 
 @warning Pajdeg object mutability expires. If you are attempting to modify a PDObjectRef instance and it's not reflected in the resulting PDF, you may be updating the object *too late*.
 
 What this means is that the objects can, with a few exceptions (see below paragraph), be modified at the moment of creation, and the modifications will be reflected in the resulting PDF document. An object can also be kept around indefinitely (by retaining it), but will at a certain point silently become immutable (changes made to the object instance will update the object itself, but the resulting PDF will not have the changes).
 
 Objects that are always immutable are:
 
 1. The PDParserRef's root object. To modify the root object, check its object ID and add a filter task.
 2. Any object fetched via PDParserLocateAndCreateDefinitionForObject() or PDParserLocateAndCreateObject(). Same deal; add a filter and mutator.
 
 ### Pinpointing mutability expiration
 
 A mutable object is mutable for as long as it has not been written to the output file. This happens as soon as the parser iterates to the next object via PDParserIterate(), and this will happen as soon as all the tasks triggered for the object finish executing.
 
 In other words, an object can be kept mutable forever by simply having a task do
 
 @code
    while (1) sleep(1);
 @endcode
 
 or, an arguably more useful example, an asynchronous operation can be triggered by simply keeping the task waiting for some flag, e.g.
 
 @code
 PDTaskResult asyncWait(PDPipeRef pipe, PDTaskRef task, PDObjectRef object)
 {
    PDInteger asyncDone = 0;
    do_asynchronous_thing(object, &asyncDone);
    while (! asyncDone) sleep(1);
    return PDTaskDone;
 }
 
 void do_asynchronous_thing(PDObjectRef object, PDInteger *asyncDone)
 {
    // start whatever asynchronous thing needs doing
 }
 
 void finish_asynchronous_thing(PDObjectRef object, PDInteger *asyncDone)
 {
    PDDictionarySet(PDObjectGetDictionary(object), "Foo", "bar");
    *asyncDone = 1;
 }
 @endcode

 @{
 */

#ifndef INCLUDED_PDObject_h
#define INCLUDED_PDObject_h

#include "PDDefines.h"

/// @name Creating and deleting

/**
 Create an object from a definitions stack (e.g. fetched via PDParserLocateAndCreateDefinitionForObject()).
 
 @warning The object is always immutable.
 
 @param defs The definitions for the object.
 @return An immutable object instance based on the defs.
 */
extern PDObjectRef PDObjectCreateFromDefinitionsStack(PDInteger obid, pd_stack defs);

/**
 Set the synchronization callback, called right before the parser serializes the object and writes it to the output stream.
 
 @note Only one callback is supported.
 
 @param object The object.
 @param callback The synchronization callback.
 @param syncInfo The user info object to be passed as the final parameter to the callback.
 */
extern void PDObjectSetSynchronizationCallback(PDObjectRef object, PDSynchronizer callback, const void *syncInfo);

/**
 Delete this object, thus excluding it from the output PDF file, and marking it as freed in the XREF table.
 
 @param object The object to remove.
 */
extern void PDObjectDelete(PDObjectRef object);

/**
 Undelete this object. 
 
 If PDObjectDelete() was called previously, calling this method will cancel the deletion. 
 
 The object may end up deleted anyway, if the pipe has moved past the object definition since the delete call.
 */
extern void PDObjectUndelete(PDObjectRef object);

/// @name Examining

/**
 Get object ID for object.
 
 @param object The object.
 */
extern PDInteger PDObjectGetObID(PDObjectRef object);

/**
 Get generation ID for an object.
 
 @param object The object.
 */
extern PDInteger PDObjectGetGenID(PDObjectRef object);

/**
 Get the obstream-flag for this object.
 
 The obstream-flag is true for objects which are embedded inside of other objects, as a part of an object stream.
 
 @param object The object.
 @return true if the object is in an object stream.
 */
extern PDBool PDObjectGetObStreamFlag(PDObjectRef object);

/**
 Get reference string for this object.
 
 @param object The object.
 */
extern const char *PDObjectGetReferenceString(PDObjectRef object);

/**
 Get type of an object.
 
 @param object The object.
 @return The PDObjectType of the object.
 
 @note Types are restricted to PDObjectTypeUnknown, PDObjectTypeDictionary, and PDObjectTypeString in the current implementation.
 */
extern PDObjectType PDObjectGetType(PDObjectRef object);

/**
 Attempt to determinee the type of the object based on its definitions stack.
 */
extern PDObjectType PDObjectDetermineType(PDObjectRef object);

/**
 *  Explicitly set the type of an object.
 *
 *  @param object The object whose type should be (re)defined.
 *  @param type   The (new) object type.
 */
extern void PDObjectSetType(PDObjectRef object, PDObjectType type);

/**
 Determine if the object has a stream or not.
 
 @param object The object.
 */
extern PDBool PDObjectHasStream(PDObjectRef object);

/**
 Determine the raw (unextracted) length of the object stream.
 
 This can be compared to the size of a file.txt.gz.
 
 @param object The object.
 */
extern PDInteger PDObjectGetRawStreamLength(PDObjectRef object);

/**
 Determine the extracted length of the previously fetched object stream. 
 
 This can be compared to the size of a file.txt after decompressing a file.txt.gz.
 
 @warning Assertion thrown if the object stream has not been fetched before this call.
 
 @param object The object.
 */
extern PDInteger PDObjectGetExtractedStreamLength(PDObjectRef object);

/**
 Determine if the object's stream is text or binary data. 
 
 This is determined by looking at the first 10 (or all, if length <= 10) bytes and seeing 80% or more of them
 are defined text characters. If this is the case, true is returned. The very last byte must also be 0 (the 
 string must be NULL-terminated).
 
 @warning Assertion thrown if the object stream has not been fetched before this call.
 
 @param object The object.
 @return true if the object's stream is text-based, false otherwise
 */
extern PDBool PDObjectHasTextStream(PDObjectRef object);

/**
 Get the object's stream. Assertion thrown if the stream has not been fetched via PDParserFetchCurrentObjectStream() first.
 
 @param object The object.
 */
extern char *PDObjectGetStream(PDObjectRef object);

/**
 Fetch the value of the given object, as an instantiation of the appropriate type, or as a char* if the object is represented as a pure string (this is the case for some constants, such as null).
 
 @param object The object.
 @return The appropriate instance type. Use PDResolve() to determine its type.
 */
extern void *PDObjectGetValue(PDObjectRef object);

/**
 Set the value of the given object.
 
 @param object The object
 @param The new value of the object (PDString, PDDictionary, etc).
 */
extern void PDObjectSetValue(PDObjectRef object, void *value);

/**
 *  Get the dictionary of the object, or NULL if the object does not have a dictionary.
 *
 *  @param object The object
 *
 *  @return PDDictionary instance for the object
 */
extern PDDictionaryRef PDObjectGetDictionary(PDObjectRef object);

/**
 *  Get the array of the object, or NULL if the object does not have an array.
 *
 *  @param object The object
 *
 *  @return PDArray instance for the object
 */
extern PDArrayRef PDObjectGetArray(PDObjectRef object);

/// @name Miscellaneous

/**
 Replaces the entire object's definition with the given string of the given length; does not replace the stream and the caller is responsible for asserting that the /Length key is preserved; if the stream was turned off, this may include a stream element by abiding by the PDF specification, which requires that
 
 1. the object is a dictionary, and has a /Length key with the exact length of the stream (excluding the keywords and newlines wrapping it), 
 2. the keyword
 @code
    stream
 @endcode
 is directly below the object dictionary on its own line followed by the stream content, and 
 3. followed by
 @code
    endstream
 @endcode
 right after the stream length (extraneous whitespace is allowed between the content's last byte and the 'endstream' keyword's beginning).
 
 Also note that filters and encodings are often used, but not required.
 
 @param object The object.
 @param str The replacement string.
 @param len The length of the replacement string.
 */
extern void PDObjectReplaceWithString(PDObjectRef object, char *str, PDInteger len);

/// @name PDF stream support

/**
 Removes the stream from the object.
 
 The stream will be skipped when written. This has no effect if the object had no stream to begin with.
 
 @param object The object.
 */
extern void PDObjectSkipStream(PDObjectRef object);

/**
 *  Replaces the stream with given data.
 *  
 *  @note The stream is inserted as is, with no filtering applied to it whatsoever. To insert a filtered stream, e.g. FlateDecoded, use PDObjectSetStreamFiltered() instead.
 *  
 *  @param object        The object.
 *  @param str           The stream data.
 *  @param len           The length of the stream data.
 *  @param includeLength Whether the object's /Length entry should be updated to reflect the new stream content length.
 *  @param allocated     Whether str should be free()d after the object is done using it.
 *  @param encrypted     If true, str is presumed to be already encrypted (e.g. copied from original PDF or pre-encrypted); if false, Pajdeg will encrypt the string before inserting it into the pipe. If the PDF is not encrypted, this argument has no effect
 */
extern void PDObjectSetStream(PDObjectRef object, char *str, PDInteger len, PDBool includeLength, PDBool allocated, PDBool encrypted);

/**
 *  Replaces the stream with given data, filtered according to the object's /Filter and /DecodeParams settings.
 *  
 *  @note Pajdeg only supports a limited number of filters. If the object's filter settings are not supported, the operation is aborted.
 *  
 *  @note If no filter is defined, PDObjectSetStream is called and and true is returned.
 *  
 *  @see PDObjectSetStream
 *  @see PDObjectSetFlateDecodedFlag
 *  @see PDObjectSetPredictionStrategy
 *  
 *  @warning str is not freed.
 *  
 *  @param object    The object.
 *  @param str       The stream data.
 *  @param len       The length of the stream data.
 *  @param allocated Whether str should be free()d after the object is done using it.
 *  @param encrypted If true, str is presumed to be already encrypted (e.g. copied from original PDF or pre-encrypted); if false, Pajdeg will encrypt the string before inserting it into the pipe. If the PDF is not encrypted, this argument has no effect
 *  
 *  @return Success value. If false is returned, the stream remains unset.
 */
extern PDBool PDObjectSetStreamFiltered(PDObjectRef object, char *str, PDInteger len, PDBool allocated, PDBool encrypted);

/**
 Enable or disable compression (FlateDecode) filter flag for the object stream.
 
 @note Passing false to the state will remove the Filter and DecodeParms dictionary entries from the object.
 
 @param object The object.
 @param state Boolean value of whether the stream is compressed or not.
 */
extern void PDObjectSetFlateDecodedFlag(PDObjectRef object, PDBool state);

/**
 Define prediction strategy for the stream.
 
 @warning Pajdeg currently only supports PDPredictorNone and PDPredictorPNG_UP. Updating an existing stream (e.g. fixing its predictor values) is possible, however, but replacing the stream or requiring Pajdeg to predict the content in some other way will cause an assertion.
 
 @param object The object.
 @param strategy The PDPredictorType value.
 @param columns Columns value.
 
 @see PDPredictorType
 @see PDStreamFilterPrediction.h
 */
extern void PDObjectSetPredictionStrategy(PDObjectRef object, PDPredictorType strategy, PDInteger columns);

/**
 Sets the encrypted flag for the object's stream.
 
 @warning If the document is encrypted, and the stream is not, this must be set to false or the stream will not function properly
 
 @param object The object.
 @param encrypted Whether or not the stream is encrypted.
 */
extern void PDObjectSetStreamEncrypted(PDObjectRef object, PDBool encrypted);

/// @name Conversion

/**
 Generates an object definition up to and excluding the stream definition, from "<obid> <genid> obj" to right before "endobj" or "stream" depending on whether a stream exists or not.
 
 The results are written into dstBuf, reallocating it if necessary (i.e. it must be a valid allocation and not a point inside a heap).
 
 @note This method ignores definition replacements via PDObjectReplaceWithString().
 
 @param object The object.
 @param dstBuf Pointer to buffer into which definition should be written. Must be a proper allocation.
 @param capacity The number of bytes allocated into *dstBuf already.
 @return Bytes written. 
 */
extern PDInteger PDObjectGenerateDefinition(PDObjectRef object, char **dstBuf, PDInteger capacity);

extern PDInteger PDObjectPrinter(void *inst, char **buf, PDInteger offs, PDInteger *cap);

#endif

/** @} */

/** @} */ // unbalanced, but doxygen complains for some reason

