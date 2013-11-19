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
 Initialize an instance object from a pd_stack.
 
 @param stack The pd_stack containing the object definitions.
 @param objectID The object ID.
 @param generationID The generation ID.
 */
- (id)initWithDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID;

///---------------------------------------
/// @name Modifying objects
///---------------------------------------

/**
 Remove the object from the PDF.
 
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
 Get the value of the primitive object. 
 
 @note If the object is not a primitive, this method will return nil.
 */
- (NSString *)primitiveValue;

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
 Replace the object stream with the given string.
 
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

@end
