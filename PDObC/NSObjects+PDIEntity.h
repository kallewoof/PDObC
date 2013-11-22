//
// NSObjects+PDIEntity.h
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
#import "PDIEntity.h"
#import "PDDefines.h"

/**
 `NSString` category implementing PDIEntity protocol. This includes a method to get an NSString out of a C string that tries different encodings.
 */

@interface NSString (PDIEntity) <PDIEntity>

+ (NSString *)stringWithPDFString:(const char *)PDFString;

- (NSString *)stringByRemovingPDFControlCharacters;
- (NSString *)stringByAddingPDFControlCharacters;

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

@end
