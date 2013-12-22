//
// PDIObject.h
//
// Copyright (c) 2013 Karl-Johan Alm (http://github.com/kallewoof)
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

@class PDInstance;

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
 Initialize an instance object from a pd_stack, configuring it with parameters from the PDF via the instance object.
 
 @note Does not enable mutation, despite being handed an instance reference.
 
 @param instance Instance.
 @param stack The pd_stack containing the object definitions.
 @param objectID The object ID.
 @param generationID The generation ID.
 */
- (id)initWithInstance:(PDInstance *)instance forDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID;

///---------------------------------------
/// @name Modifying objects
///---------------------------------------

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
 
 @param instance The instance object.
 @return true if mutation was made possible; false if the object cannot be made mutable any longer
 */
- (BOOL)enableMutationViaMimicSchedulingWithInstance:(PDInstance *)instance;

/**
 Schedules mimicking of the object, which means readonly objects are made readwritable up until the object passes through the pipe into the output stream.
 
 @param instance The instance object.
 */
- (void)scheduleMimicWithInstance:(PDInstance *)instance;

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
 Remove the object's stream (but keep the object).
 */
- (void)removeStream;

/**
 Get the stream content for the object as a retained NSData object. 
 */
- (NSData *)allocStream;

/**
 Determine if this object is the current object in the pipe.
 */
- (BOOL)isCurrentObject;

/**
 Get the value of the primitive object. 
 
 @note If the object is not a primitive, this method will return nil.
 */
- (NSString *)primitiveValue;

/**
 Set the value of the primitive object. 
 */
- (void)setPrimitiveValue:(NSString *)value;

// dictionary methods

/**
 Get the value of the given key.
 
 @param key The dictionary key.
 
 @note In the current implementation, all values are returned as strings, but can be set using e.g. NSStrings, PDIObjects, PDIReferences or NSDictionary/Arrays of conformant objects.
 */
- (id)valueForKey:(NSString *)key;

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
 
 @param content The stream data. 
 
 @see setStreamIsEncrypted:
 */
- (void)setStreamContent:(NSData *)content;

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
- (void)addSynchronizeHook:(dispatch_block_t)block;

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

/**
 The object's type.
 */
@property (nonatomic, readonly) PDObjectType type;


///---------------------------------------
/// @name Advanced properties
///---------------------------------------

/**
 Indicate that the object is inherently mutable.
 
 This will prevent the object from attempting to schedule a mimic callback when edited.
 */
- (void)markMutable;

@end
