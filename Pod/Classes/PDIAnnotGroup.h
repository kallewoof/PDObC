//
// PDIAnnotGroup.h
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

@class PDISession;
@class PDIObject;
@class PDIAnnotation;

@interface PDIAnnotGroup : NSObject

/**
 Initialize a new annotations group for the given object, which is an /Annots entry e.g. from a page object.
 
 @param object The annots object.
 @param session The PDISession.
 */
- (id)initWithObject:(PDIObject *)object inSession:(PDISession *)session;

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

@property (nonatomic, readonly, strong) NSArray *annotations;   ///< Array of all annotations in this object.
@property (nonatomic, readonly, strong) PDIObject *object;      ///< Object associated with this annots group.

@end

@interface PDIAnnotGroup (PDIDeprecated)

/**
 Initialize a new annotations group for the given object, which is an /Annots entry e.g. from a page object.
 
 @warning Deprecated method. Use -initWithObject:inSession:.
 
 @param object The annots object.
 @param instance The PDISession.
 */
- (id)initWithObject:(PDIObject *)object inInstance:(PDISession *)instance PD_DEPRECATED(0.0, 0.2);

@end
