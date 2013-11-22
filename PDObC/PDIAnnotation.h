//
// PDIAnnotation.h
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

@class PDIObject;
@class PDIAnnotGroup;
@class PDInstance;

#define PDIAnnotationSubtypeLink    @"/Link"

#define PDIAnnotationATypeAction    @"/Action"

#define PDIAnnotationSTypeURI       @"/URI"

/*typedef enum {
    PDIAnnotationSubtypeLink,
} PDIAnnotationSubtype;

typedef enum {
    PDIAnnotationATypeAction,
} PDIAnnotationAType;

typedef enum {
    PDIAnnotationSTypeURI,
} PDIAnnotationSType;*/

@interface PDIAnnotation : NSObject

/**
 Initialize an annotation associated with the given annotations group.
 
 @param object The object for the annotation.
 @param annotGroup Annotations group to which this annotation will be added.
 */
- (id)initWithObject:(PDIObject *)object inAnnotGroup:(PDIAnnotGroup *)annotGroup withInstance:(PDInstance *)instance;

@property (nonatomic, readonly)  PDIAnnotGroup *annotGroup;
@property (nonatomic, readonly)  PDIObject *object;

//@property (nonatomic, readwrite) CGRect border;                 ///< The border of the annotation (it's 3 digits i.e. not a rect; investigate & fix)
@property (nonatomic, readwrite) CGRect             rect;       ///< The rect of the annotation
@property (nonatomic, strong)    NSString          *subtype;    ///< The sub type

// Action (A) properties

@property (nonatomic, strong)    NSString          *aType;      ///< The A type, which can be "action" and nothing else right now
@property (nonatomic, strong)    NSString          *sType;      ///< The S type, which can be "URI" and nothing else right now
@property (nonatomic, strong)    NSString          *uriString;  ///< The URI string; if set, it will result in subtype=Link, aType=action, sType=URI, and the potential creation of a new URI object

@end
