//
// PDIAnnotation.h
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
#import "PDDefines.h"

@class PDIObject;
@class PDIReference;
@class PDIAnnotGroup;
@class PDISession;

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
 
 @param object The object for the annotation
 @param annotGroup Annotations group to which this annotation will be added
 @param session Associated PDISession object
 */
- (id)initWithObject:(PDIObject *)object inAnnotGroup:(PDIAnnotGroup *)annotGroup withSession:(PDISession *)session;

/**
 Delete this annotation from the resulting PDF.
 */
- (void)deleteAnnotation;

@property (nonatomic, weak) PDIAnnotGroup *annotGroup;
@property (nonatomic, strong) PDIObject *object;

//@property (nonatomic, readwrite) CGRect border;                 ///< The border of the annotation (it's 3 digits i.e. not a rect; investigate & fix) (could it be x-offs, y-offs, width or some such?
@property (nonatomic, readwrite) CGRect             rect;       ///< The rect of the annotation
@property (nonatomic, copy)    NSString          *subtype;    ///< The sub type

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
- (void)setDDestByPageIndex:(NSUInteger)pageIndex;

@property (nonatomic, strong)    NSArray           *fit;        ///< The fit value as an array starting with the fit type, followed by its arguments, e.g. @"/XYZ", @(36), @(661.68), @(0); using a helper macro is recommended (see bottom of this file)

// Action (A) properties

@property (nonatomic, copy)    NSString          *aType;      ///< The A type, which can be "action" and nothing else right now
@property (nonatomic, copy)    NSString          *sType;      ///< The S type, which can be "URI" and nothing else right now
@property (nonatomic, copy)    NSString          *uriString;  ///< The URI string; if set, it will result in subtype=Link, aType=action, sType=URI, and the potential creation of a new URI object

@end

@interface PDIAnnotation (PDIDeprecated)

/**
 Initialize an annotation associated with the given annotations group.
 
 @warning Deprecated method. Use -initWithObject:inAnnotGroup:withSession:.
 
 @param object The object for the annotation.
 @param annotGroup Annotations group to which this annotation will be added.
 @param instance Instance.
 */
- (id)initWithObject:(PDIObject *)object inAnnotGroup:(PDIAnnotGroup *)annotGroup withInstance:(PDISession *)instance PD_DEPRECATED(0.0, 0.2);

@end

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// helper macros

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Display the page designated by page, with the coordinates (left, top) posi- tioned at the upper-left corner of the window and the contents of the page magnified by the factor zoom. A null value for any of the parameters left, top, or zoom specifies that the current value of that parameter is to be retained un- changed. A zoom value of 0 has the same meaning as a null value. */
#define PDIAnnotationFitXYZ(l,t,z) @[[PDIName nameWithString:@"/XYZ"], @(l), @(t), @(z)]

/** Display the page designated by page, with its contents magnified just enough to fit the entire page within the window both horizontally and vertically. If the required horizontal and vertical magnification factors are different, use the smaller of the two, centering the page within the window in the other dimension. */
#define PDIAnnotationFitFit() @[[PDIName nameWithString:@"/Fit"]]

/** Display the page designated by page, with the vertical coordinate top posi- tioned at the top edge of the window and the contents of the page magnified just enough to fit the entire width of the page within the window. A null value for top specifies that the current value of that parameter is to be retained un- changed. */
#define PDIAnnotationFitH(t) @[[PDIName nameWithString:@"/FitH"], @(t)]

/** Display the page designated by page, with the horizontal coordinate left posi- tioned at the left edge of the window and the contents of the page magnified just enough to fit the entire height of the page within the window. A null val- ue for left specifies that the current value of that parameter is to be retained unchanged. */
#define PDIAnnotationFitV(l) @[[PDIName nameWithString:@"/FitV"], @(l)]

/** Display the page designated by page, with its contents magnified just enough to fit the rectangle specified by the coordinates left, bottom, right, and top entirely within the window both horizontally and vertically. If the required horizontal and vertical magnification factors are different, use the smaller of the two, centering the rectangle within the window in the other dimension. A null value for any of the parameters may result in unpredictable behavior. */
#define PDIAnnotationFitR(l,b,r,t) @[[PDIName nameWithString:@"/FitR"], @(l), @(b), @(r), @(t)]

/** (PDF 1.1) Display the page designated by page, with its contents magnified just enough to fit its bounding box entirely within the window both hori- zontally and vertically. If the required horizontal and vertical magnification factors are different, use the smaller of the two, centering the bounding box within the window in the other dimension. */
#define PDIAnnotationFitB() @[[PDIName nameWithString:@"/FitB"]]

/** (PDF 1.1) Display the page designated by page, with the vertical coordinate top positioned at the top edge of the window and the contents of the page magnified just enough to fit the entire width of its bounding box within the window. A null value for top specifies that the current value of that parameter is to be retained unchanged. */
#define PDIAnnotationFitBH(t) @[[PDIName nameWithString:@"/FitBH"], @(t)]

/** (PDF 1.1) Display the page designated by page, with the horizontal coordi- nate left positioned at the left edge of the window and the contents of the page magnified just enough to fit the entire height of its bounding box within the window. A null value for left specifies that the current value of that parameter is to be retained unchanged. */
#define PDIAnnotationFitBV(l) @[[PDIName nameWithString:@"/FitBV"], @(l)]

