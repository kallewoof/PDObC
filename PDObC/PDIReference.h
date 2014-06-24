//
// PDIReference.h
//
// Copyright (c) 2012 - 2014 Karl-Johan Alm (http://github.com/kallewoof)
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
