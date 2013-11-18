//
// PDIEntity.h
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

/**
 The `PDIEntiy` protocol provides a single readonly property for obtaining a PDF compatible string of the given object. It is used in the internal objects, and in category implementations for e.g. `NSDictionary`.
 */

@protocol PDIEntity <NSObject>

/**
 The object's C string representation according to the PDF specification. 
 */
@property (nonatomic, readonly) const char *PDFString;

@end

/**
 `PDIEntiy` is a quasi-abstract convenience implementation of the `PDIEntity` protocol. It has an instance variable that sub classes can use to cache the string, and the string is `free()`'d, if non-NULL on dealloc.
 */

@interface PDIEntity : NSObject<PDIEntity> {
    char *_PDFString;
}

@end

