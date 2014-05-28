//
//  NSArray+PDIXMPArchive.h
//  PDF Link Inspector
//
//  Created by Karl-Johan Alm on 28/05/14.
//  Copyright (c) 2014 ReOrient Media. All rights reserved.
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
