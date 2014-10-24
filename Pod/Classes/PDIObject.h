//
// PDIObject.h
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

#import <Foundation/Foundation.h>
#import "PDObject.h"
#import "PDIEntity.h"

/**
 `PDIObject` is the Objective-C wrapper for the PDObject, and represents an object in a PDF document. It is mutable (can be modified).
 
 ## Mutability Limitation
 
 `PDIObject`s are mutable on instantiation, but mutability is only guaranteed at the instant of creation. `PDIObject`s can be referenced and kept around indefinitely, but any modifications made to them will go (silently) ignored, because the object has already been written to the output PDF.
 
 ## Encrypted PDFs and Streams
 
 Because `Pajdeg` does not inherently support encryption, changing an object's stream content for a PDF that is encrypted will result in a broken PDF, because the PDF viewer will expect the (plaintext) content to be encrypted. To resolve this issue, setStreamIsEncrypted: must be called with `NO` for plaintext data. It is safe to call this method even if the input PDF is not encrypted.
 */

@class PDISession;
@class PDIReference;

/**
 Comment out this #define to allow the -setValue:forKeyPath: to pass through to the default implementation.
 
 The default is to reroute it to setValue:forKey:, as Xcode tends to presume forKeyPath:, and it's subtle enough that it's near impossible to realize it happened at times.
 */
#define PDI_DISALLOW_KEYPATHS

@interface PDIObject : PDIEntity

///---------------------------------------
/// @name Instantiating objects
///---------------------------------------

/**
 Initialize an instance object from PDObjectRef.
 
 @param object The PDObjectRef. 
 */
- (id)initWithObject:(PDObjectRef)object;

/**
 Initialize an instance object from a pd_stack. "Isolated" in the method name indicates that the object is not properly set up for certain PDFs (encrypted PDFs, to be precise).
 
 @param stack The pd_stack containing the object definitions.
 @param objectID The object ID.
 @param generationID The generation ID.
 */
- (id)initWithIsolatedDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID;

/**
 Initialize an instance object from a pd_stack, configuring it with parameters from the PDF via the session object.
 
 @note Does not enable mutation, despite being handed an instance reference.
 
 @param instance Session.
 @param stack The pd_stack containing the object definitions.
 @param objectID The object ID.
 @param generationID The generation ID.
 */
- (id)initWithSession:(PDISession *)session forDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID;

/**
 *  Initialize a fake object wrapping the given value. Fake objects are handy when working with methods that require an object to work with.
 *
 *  @note The containing object should have mimic scheduling triggered, or changes may be lost.
 *
 *  @param value   The value, such as an NSMutableDictionary or the like
 *  @param PDValue The corresponding pajdeg value
 *
 *  @return PDIObject wrapping the given value
 */
- (id)initWrappingValue:(id)value PDValue:(void *)PDValue;

///---------------------------------------
/// @name Modifying objects
///---------------------------------------

/**
 The object type that this object believes itself to be. If set, the object will be forced to that type, regardless of its previous content.
 */
@property (nonatomic, readwrite) PDObjectType type;

/**
 Mimic the target object, i.e. make this object as identical to the target as possible.
 
 Does not change object or generation ID's.
 
 @warning The target is partially destroyed in the process. This method is mostly meant to transfer immutable-instance-data over from a previously readonly fetch of self, which was then given a task to save the actual changes.

 @param target The target object which we wish to look like.
 */
- (void)mimic:(PDIObject *)target;

/**
 Enable mutation via mimic scheduling for this object.
 
 This is different from scheduling mimicking directly, in that the object will not schedule mimicking unless it is actually modified.
 
 @param session The session object.
 @return true if mutation was made possible; false if the object cannot be made mutable any longer
 */
- (BOOL)enableMutationViaMimicSchedulingWithSession:(PDISession *)session;

/**
 Schedules mimicking of the object, which means readonly objects are made readwritable up until the object passes through the pipe into the output stream.
 
 @param session The session object.
 */
- (void)scheduleMimicWithSession:(PDISession *)session;

/**
 Get a PDIReference instance for this object.
 
 @return PDIReference object.
 */
- (PDIReference *)reference;

/**
 Determine if the object is in an object stream or not.
 */
- (BOOL)isInsideObjectStream;

/**
 Remove the object from the PDF.
 
 @note Objects inside of object streams cannot be removed.
 
 The object is completely removed from the resulting PDF.
 */
- (void)removeObject;

/**
 Unremove the object from the PDF.
 
 An object that was (potentially) told to remove itself will no longer be removed, presuming it hasn't been encountered yet in the pipe.
 */
- (void)unremoveObject;

/**
 Determine if the object is (to be) removed or not.
 */
- (BOOL)willBeRemoved;

/**
 Remove the object's stream (but keep the object).
 */
- (void)removeStream;

/**
 Prepare the object's stream so that functions which rely on it being prepared will not fail.
 
 @return YES if there is a stream, NO otherwise
 */
- (BOOL)prepareStream;

/**
 Get the stream content for the object as a retained NSData object. 
 */
- (NSData *)allocStream;

/**
 Determine if this object is the current object in the pipe.
 */
- (BOOL)isCurrentObject;

/**
 Get the value of the object. The value is the underlying instance, which can be an array, dictionary, string, number, etc. 
 */
- (id)objectValue;

/**
 Set the value of the object. 
 */
- (void)setObjectValue:(id)value;

// dictionary methods

/**
 Get the value of the given key. The value is of the corresponding Objective-C type, e.g. PDArrays are NSArrays, etc.
 
 @param key The dictionary key.
 */
- (id)valueForKey:(NSString *)key;

/**
 Get the resolved value of the given key; that is, if the key is in the form "<number> <number> R", load the given object and return its value, rather than returning the reference.
 
 @note Enabling mutations is required for the object to have an instance, through which to fetch external objects.
 
 @param key The dictionary key.
 
 @note In the current implementation, all values are returned as strings, but can be set using e.g. NSStrings, PDIObjects, PDIReferences or NSDictionary/Arrays of conformant objects.
 */
- (id)resolvedValueForKey:(NSString *)key;

/**
 *  Get the object value of the given key. If the given key is not an indirect reference (but a direct value), a new object is created with the value, and the new object is set as the value for the given key.
 *
 *  @note Enabling mutations is required for the object to have an instance, through which to fetch external objects.
 *
 *  @param key The key
 *
 *  @return A PDIObject instance for the value of the given key (created, if the value was not an indirect reference at the time).
 */
- (PDIObject *)objectForKey:(NSString *)key;

/**
 Remove the given key.
 
 @param key The dictionary key whose value should be removed.
 
 @note Calling setValue:forKey: with a NULL argument is akin to calling `[anNSDictionary setObject:nil forKey:key]` (i.e. exception/crash).
 */
- (void)removeValueForKey:(NSString *)key;

/**
 Set the value of the given key. 
 
 @param value The value. Must conform to PDIEntity or be an NSString, or any other object whose description results in a value compatible with the PDF specification for dictionary values.
 @param key The dictionary key.
 
 @warning Setting a value, and then changing the contents of that value, even if it is mutable, will not affect the resulting PDF. setValue:forKey: must be called *after* all modifications are done, regardless of the object type.
 */
- (void)setValue:(id)value forKey:(NSString *)key;

/**
 Construct an NSDictionary representation of the (dictionary) object's definition.
 
 @note If the object is not a dictionary, this returns nil.
 */
- (NSDictionary *)constructDictionary;

/**
 Purge and replace the entries in the object dictionary with the given dictionary.
 
 @param dict The NSDictionary which is to replace the current object dictionary (if any).
 */
- (void)replaceDictionaryWith:(NSDictionary *)dict;

// array methods

/**
 Get the number of entries in the array or dictionary. 
 
 @return The entry count, or 0 if this is neither an array nor a dictionary.
 */
- (NSInteger)count;

/**
 Purge and replace the entries in the object array with the given array.
 
 @param array The NSArray which is to replace the current object array (if any).
 */
- (void)replaceArrayWith:(NSArray *)array;

/**
 Get the array element at the given index.
 
 @note Throws an exception of index is out of range.
 
 @param index Array index.
 @return The value.
 */
- (id)valueAtIndex:(NSInteger)index;

/**
 Replace the value at the given index.
 
 @param index Array index.
 @param value The value.
 */
- (void)replaceValueAtIndex:(NSInteger)index withValue:(id)value;

/**
 Remove the value at the given index.
 
 @param index Index of value to be deleted from array.
 */
- (void)removeValueAtIndex:(NSInteger)index;

/**
 Append a value to the end of the array.
 
 @param value The value.
 */
- (void)appendValue:(id)value;

/**
 Construct an NSArray of the object array.
 */
- (NSArray *)constructArray;

// other

/**
 Replace the object stream with the given string.
 
 The "getter" is called -allocStream.
 
 @see setStreamIsEncrypted:
 
 @param content   The stream data. 
 @param encrypted Whether the data is currently encrypted (using the document's encryption method) or whether it needs to be encrypted before being written to the pipe.
 */
- (void)setStreamContent:(NSData *)content encrypted:(BOOL)encrypted;

/**
 Indicate that this object's stream is or is not encrypted. 
 
 @param encrypted Whether the stream is or isn't encrypted. Passing `NO` will trigger insertion of the keys Filter and DecodeParms declaring that the stream is *plaintext* and not encrypted. Passing `YES` will trigger removal of said keys.
 
 This method does nothing unless the document is in an encrypted state.
 */
- (void)setStreamIsEncrypted:(BOOL)encrypted;

/**
 Attach a block for when the object is about to be finalized, to do last minute synchronization.
 
 @param block Block to be called right before object is serialized.
 */
- (void)addSynchronizeHook:(void(^)(PDIObject *object))block;

///---------------------------------------
/// @name Basic object properties
///---------------------------------------

/**
 The object's ID.
 */
@property (nonatomic, readonly) NSInteger objectID;

/**
 The object's generation number.
 */
@property (nonatomic, readonly) NSInteger generationID;


///---------------------------------------
/// @name Advanced properties
///---------------------------------------

/**
 Indicate that the object is inherently mutable.
 
 This will prevent the object from attempting to schedule a mimic callback when edited.
 */
- (void)markMutable;

/**
 *  The internal PDObject reference.
 */
@property (nonatomic, readonly) PDObjectRef objectRef;

@end

@interface PDIObject (PDIDeprecated)

/**
 Initialize an instance object from a pd_stack, configuring it with parameters from the PDF via the instance object.
 
 @warning Deprecated method. Use -initWithSession:forDefinitionStack:objectID:generationID:.
 
 @note Does not enable mutation, despite being handed an instance reference.
 
 @param instance Instance.
 @param stack The pd_stack containing the object definitions.
 @param objectID The object ID.
 @param generationID The generation ID.
 */
- (id)initWithInstance:(PDISession *)instance forDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID PD_DEPRECATED(0.0, 0.2);

/**
 Enable mutation via mimic scheduling for this object.
 
 This is different from scheduling mimicking directly, in that the object will not schedule mimicking unless it is actually modified.
 
 @warning Deprecated method. Use -enableMutationViaMimicSchedulingWithSession:.
 
 @param instance The instance object.
 @return true if mutation was made possible; false if the object cannot be made mutable any longer
 */
- (BOOL)enableMutationViaMimicSchedulingWithInstance:(PDISession *)instance PD_DEPRECATED(0.0, 0.2);

/**
 Schedules mimicking of the object, which means readonly objects are made readwritable up until the object passes through the pipe into the output stream.
 
 @warning Deprecated method. Use -scheduleMimicWithInstance:.

 @param instance The instance object.
 */
- (void)scheduleMimicWithInstance:(PDISession *)instance PD_DEPRECATED(0.0, 0.2);

@end
