//
// PDIString.h
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
#import "PDIEntity.h"

/**
 `PDIString` is a convenience class for handling string values in PDFs.
 
 By default, all strings in a PDIObject instance are stored as is, and as such, they will be written into the resulting PDF exactly as printed. This is not how the PDF spec takes arbitrary strings, however. PDFs require that strings are wrapped in parentheses, and that parentheses inside of the strings are escaped or balanced. That's what PDIString helps out with. That's all it helps out with.
 
 @note This is not actually the case -- it doesn't check for balancing of parentheses; I've yet to see a string where this is a requirement. but the spec requires it so it's officially a TO DO.
 */

@interface PDIString : NSObject <PDIEntity>

+ (PDIString *)stringWithString:(NSString *)string;         ///< Use for strings which are not PDF encoded (e.g. @"a string")
+ (PDIString *)stringWithPDFString:(NSString *)PDFString;   ///< Use for strings which are already PDF encoded (e.g. @"(a string)"

+ (PDIString *)stringWithFormat:(NSString *)format, ...;

+ (id)stringOrObject:(id)object;                            ///< Return a PDIString instance if object is an NSString, otherwise return object as is.

- (id)initWithString:(NSString *)string;                    ///< Use for strings which are not PDF encoded (e.g. @"a string")
- (id)initWithPDFString:(NSString *)string;                 ///< Use for strings which are already PDF encoded (e.g. @"(a string)"
- (id)initWithFormat:(NSString *)format arguments:(va_list)argList NS_FORMAT_FUNCTION(1,0);

- (NSString *)stringValue; // turns @"(value)" into @"value"

@end

@interface NSString (PDIValue)

- (PDIString *)PDIString;

@end
