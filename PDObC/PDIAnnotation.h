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
@class PDIReference;
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

//@property (nonatomic, readwrite) CGRect border;                 ///< The border of the annotation (it's 3 digits i.e. not a rect; investigate & fix) (could it be x-offs, y-offs, width or some such?
@property (nonatomic, readwrite) CGRect             rect;       ///< The rect of the annotation
@property (nonatomic, strong)    NSString          *subtype;    ///< The sub type

// Dest (Dest) properties

/**
 The Dest pointer for this object.
 
 @warning If this property is set (either via this property or via -setDDestByPageIndex: below, the action properties below are more or less permanently damaged. Relying on being able to set dDest blindly and then change one's mind later and set A stuff will most assuredly break the annotation. Setting dDest to nil will undo some of the damage done (by setting it to a reference and then changing one's mind), but possibly not all of it.
 */
@property (nonatomic, strong)    PDIReference       *dDest;

/**
 Set dDest to the reference for the given page.
 
 @param pageIndex Destination page.
 */
- (void)setDDestByPageIndex:(NSInteger)pageIndex;

@property (nonatomic, strong)    NSArray           *fit;        ///< The fit value as an array starting with the fit type, followed by its arguments, e.g. @"/XYZ", @(36), @(661.68), @(0); using a helper macro is recommended (see bottom of this file)

// Action (A) properties

@property (nonatomic, strong)    NSString          *aType;      ///< The A type, which can be "action" and nothing else right now
@property (nonatomic, strong)    NSString          *sType;      ///< The S type, which can be "URI" and nothing else right now
@property (nonatomic, strong)    NSString          *uriString;  ///< The URI string; if set, it will result in subtype=Link, aType=action, sType=URI, and the potential creation of a new URI object

@end

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// helper macros

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Display the page designated by page, with the coordinates (left, top) posi- tioned at the upper-left corner of the window and the contents of the page magnified by the factor zoom. A null value for any of the parameters left, top, or zoom specifies that the current value of that parameter is to be retained un- changed. A zoom value of 0 has the same meaning as a null value. */
#define PDIAnnotationFitXYZ(l,t,z) @[@"/XYZ", @(l), @(t), @(z)]

/** Display the page designated by page, with its contents magnified just enough to fit the entire page within the window both horizontally and vertically. If the required horizontal and vertical magnification factors are different, use the smaller of the two, centering the page within the window in the other dimension. */
#define PDIAnnotationFitFit() @[@"/Fit"]

/** Display the page designated by page, with the vertical coordinate top posi- tioned at the top edge of the window and the contents of the page magnified just enough to fit the entire width of the page within the window. A null value for top specifies that the current value of that parameter is to be retained un- changed. */
#define PDIAnnotationFitH(t) @[@"/FitH", @(t)]

/** Display the page designated by page, with the horizontal coordinate left posi- tioned at the left edge of the window and the contents of the page magnified just enough to fit the entire height of the page within the window. A null val- ue for left specifies that the current value of that parameter is to be retained unchanged. */
#define PDIAnnotationFitV(l) @[@"/FitV", @(l)]

/** Display the page designated by page, with its contents magnified just enough to fit the rectangle specified by the coordinates left, bottom, right, and top entirely within the window both horizontally and vertically. If the required horizontal and vertical magnification factors are different, use the smaller of the two, centering the rectangle within the window in the other dimension. A null value for any of the parameters may result in unpredictable behavior. */
#define PDIAnnotationFitR(l,b,r,t) @[@"/FotR", @(l), @(b), @(r), @(t)]

/** (PDF 1.1) Display the page designated by page, with its contents magnified just enough to fit its bounding box entirely within the window both hori- zontally and vertically. If the required horizontal and vertical magnification factors are different, use the smaller of the two, centering the bounding box within the window in the other dimension. */
#define PDIAnnotationFitB() @[@"/FitB"]

/** (PDF 1.1) Display the page designated by page, with the vertical coordinate top positioned at the top edge of the window and the contents of the page magnified just enough to fit the entire width of its bounding box within the window. A null value for top specifies that the current value of that parameter is to be retained unchanged. */
#define PDIAnnotationFitBH(t) @[@"/FitBH", @(t)]

/** (PDF 1.1) Display the page designated by page, with the horizontal coordi- nate left positioned at the left edge of the window and the contents of the page magnified just enough to fit the entire height of its bounding box within the window. A null value for left specifies that the current value of that parameter is to be retained unchanged. */
#define PDIAnnotationFitBV(l) @[@"/FitBV", @(l)]

