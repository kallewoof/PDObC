//
// PDIEntity.h
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

/**
 *  The `PDIEntiy` protocol provides a single readonly property for obtaining a PDF compatible string of the given object. It is used in the internal objects, and in category implementations for e.g. `NSDictionary`.
 */

@protocol PDIEntity <NSObject>

/**
 *  The object's C string representation according to the PDF specification. 
 */
@property (nonatomic, readonly) const char *PDFString;

/**
 *  A PD value based on this type.
 */
@property (nonatomic, readonly) void *PDValue;

@end

/**
 *  `PDIEntiy` is a quasi-abstract convenience implementation of the `PDIEntity` protocol. It has an instance variable that sub classes can use to cache the string, and the string is `free()`'d, if non-NULL on dealloc.
 */

@interface PDIEntity : NSObject<PDIEntity> {
    char *_PDFString;
}

@end

