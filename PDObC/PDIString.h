//
// PDIString.h
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
