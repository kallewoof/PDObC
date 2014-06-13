//
// PDIAnnotGroup.h
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

@class PDInstance;
@class PDIObject;
@class PDIAnnotation;

@interface PDIAnnotGroup : NSObject

/**
 Initialize a new annotations group for the given object, which is an /Annots entry e.g. from a page object.
 
 @param object The annots object.
 @param instance The PDInstance.
 */
- (id)initWithObject:(PDIObject *)object inInstance:(PDInstance *)instance;

/**
 Add an annotation object to this group.
 
 @return A new annotation object that has been added to the annotations array for this group.
 */
- (PDIAnnotation *)appendAnnotation;

/**
 Remove the given annotation object.
 
 If the annotation has not yet deleted its associated object(s), a call to -deleteAnnotation will be made.
 
 @param annotation The annotation to remove.
 */
- (void)removeAnnotation:(PDIAnnotation *)annotation;

@property (nonatomic, readonly) NSArray *annotations;   ///< Array of all annotations in this object.
@property (nonatomic, readonly) PDIObject *object;      ///< Object associated with this annots group.

@end
