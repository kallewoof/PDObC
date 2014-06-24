//
// NSArray+PDIXMPArchive.h
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

@interface NSArray (PDIXMPArchive)

/**
 *  Iterate through the array of PDIXMPElement objects, and return the first non-null value encountered by looking into the given path of each object in order.
 *
 *  @param path The path to scan through. If nil, the elements themselves are checked for values
 *
 *  @return The first non-nil value encountered for the given path, if any, or nil, if none.
 */
- (NSString *)firstAvailableElementValueForPath:(NSArray *)path;

/**
 *  Iterate through the array of PDIXMPElement objects, and return the first non-null value of the attribute attributeName encountered by looking into the given path of each object in order.
 *
 *  @param attributeName The attribute name whose value should be checked/returned
 *  @param path          The path to scan through. If nil, the attributes of the elements themselves are checked for values
 *
 *  @return The first non-nil attribute value encountered for the given path/attribute name, if any, or nil, if none.
 */
- (NSString *)firstAvailableValueOfAttribute:(NSString *)attributeName forPath:(NSArray *)path;

/**
 *  Iterate through the array of PDIXMPElement objects, and combine the non-null values encountered by looking into the given path of each object in order and joining them using the separator string.
 *
 *  @param separator Separator string to use in between two joined elements in the returned string
 *  @param path      The path to scan through. If nil, the elements themselves are checked for values
 *
 *  @return String of all non-nil values joined. If no values were encountered, nil is returned, not @""
 */
- (NSString *)elementValuesJoinedWithSeparator:(NSString *)separator forPath:(NSArray *)path;

@end
