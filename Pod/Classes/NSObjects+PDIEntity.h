//
// NSObjects+PDIEntity.h
//
// Copyright (c) 2012 - 2015 Karl-Johan Alm (http://github.com/kallewoof)
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
#import "PDIEntity.h"
#import "PDDefines.h"

/**
 `NSString` category implementing PDIEntity protocol. This includes a method to get an NSString out of a C string that tries different encodings.
 */

@interface NSString (PDIEntity) <PDIEntity>

+ (NSString *)stringWithPDFString:(const char *)PDFString;

+ (id)objectWithPDString:(PDStringRef)PDString;

/**
 *  Return a PDF XMP archive formatted UUID string, fitted for xmpMM:DocumentID / xmpMM:InstanceID. The input string must be a series of hex characters,
 *  and the resulting output is in the format "uuid:aabbccdd-eeff-0011-2233-445566778899"
 *
 *  @note Strings shorter than 32 characters are padded with zeroes from the left. Strings longer than 32 characters are truncated. The hexadecimal correctness of the string is not checked. Thus, the string "foo" will happily return @"uuid:00000000-0000-0000-0000-000000000foo"
 *
 *  @note If present, wrapping <>'s are trimmed out.
 *
 *  @return Formatted XMP archive UUID string
 */
- (NSString *)PXUString;

- (NSString *)stringByRemovingPDFControlCharacters;
- (NSString *)stringByAddingPDFControlCharacters;

@end

/**
 *  A PDIName has as its sole purpose the task of retaining the PDString name type when converting to and from PDString and NSString. Because NSString representations of PDStrings are
 *  always unwrapped, there's no way of separating the string (/Helvetica) from the name /Helvetica (both do occur, albeit with extra parameters added in most cases, for the string type).
 */
@interface PDIName : NSObject <PDIEntity>

+ (PDIName *)nameWithString:(NSString *)string;
+ (PDIName *)nameWithPDString:(PDStringRef)PDString;
- (BOOL)isEqualToString:(id)object;
- (NSString *)string;

@end

/**
 `NSDictionary` category implementing the PDIEntity protocol. The implementation returns the NSDictionary entries wrapped in <<>>, each key prefixed with a forward slash, and each value resolved either through its `PDIEntity` implementation (if conformant) or description.
 */

@interface NSDictionary (PDIEntity) <PDIEntity>

@end

/**
 `NSArray` category implementing the PDIEntity protocol. The implementation returns the NSArray entries wrapped in [], each entry resolved either through its `PDIEntity` implementation (if conformant) or description.
 */

@interface NSArray (PDIEntity) <PDIEntity>

/**
 Create an NSArray object based on a PDRect struct.
 
 @param rect The PDRect object.
 */
+ (NSArray *)arrayFromPDRect:(PDRect)rect;

@end

/**
 `NSDate` category implementing PDIEntity protocol. This converts an NSDate into a PDF date in the format D:YYYYMMDDHHMMSSZ00'00'
 */

@interface NSDate (PDIEntity) <PDIEntity>

/**
 *  Returns the given date as an date string according to http://www.w3.org/TR/NOTE-datetime in the format
 *      YYYY-MM-DDThh:mm:ss
 *
 *  @return This date as datetime string
 */
- (NSString *)datetimeString;

@end

@interface NSNumber (PDIEntity) <PDIEntity>

@end
