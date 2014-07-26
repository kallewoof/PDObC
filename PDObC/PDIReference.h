//
// PDIReference.h
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
#import "PDReference.h"
#import "PDIEntity.h"

/**
 `PDIReference` is a reference to some object in a PDF document.
 */

@interface PDIReference : PDIEntity

///---------------------------------------
/// @name Class methods
///---------------------------------------

+ (NSInteger)objectIDFromString:(NSString *)refString;

/**
 *  Create a `PDReference` for a given object ID and generation ID. 
 *
 *  @return PDReference
 */
+ (void *)PDValueForObjectID:(NSInteger)objectID generationID:(NSInteger)generationID;

///---------------------------------------
/// @name Instantiating references
///---------------------------------------

/**
 Creates a `PDIReference` for an object.
 
 @param objectID     The object ID.
 @param generationID The generation ID. Usually 0.
 
 @return The `PDIReference`.
 
 */
- (id)initWithObjectID:(NSInteger)objectID generationID:(NSInteger)generationID;

/**
 Sets up a reference based on a `PDReferenceRef`.
 
 @param reference The `PDReferenceRef`.
 @return The `PDIReference`.
 */
- (id)initWithReference:(PDReferenceRef)reference;

/**
 Sets up a reference to an object from a definition stack.
 
 @param stack The `pd_stack` object. This can be a dictionary entry or a direct reference.
 @return The `PDIReference`.
 */
- (id)initWithDefinitionStack:(pd_stack)stack;

/**
 Sets up a reference to an object from a string in the form "nnn nnn ttt", e.g. "123 0 R"
 
 @param refString The string reference.
 @return The `PDIReference`.
 */
- (id)initWithString:(NSString *)refString;

/**
 Obtain a PDReference from this reference.
 
 @return A PDReference identical to this reference.
 */
- (PDReferenceRef)PDReference;

///---------------------------------------
/// @name Basic reference properties
///---------------------------------------

/**
 The object ID.
 */
@property (nonatomic, readonly) NSInteger objectID;

/**
 The generation number.
 */
@property (nonatomic, readonly) NSInteger generationID;

@end
