//
//  NSArray+PDIXMPArchive.m
//  PDF Link Inspector
//
//  Created by Karl-Johan Alm on 28/05/14.
//  Copyright (c) 2014 ReOrient Media. All rights reserved.
//

#import "NSArray+PDIXMPArchive.h"
#import "PDIXMPElement.h"

@implementation NSArray (PDIXMPArchive)

- (NSString *)firstAvailableElementValueForPath:(NSArray *)path
{
    NSString *value;
    NSString *name;
    NSArray *subPath;
    if (path.count) {
        name = path[0];
        subPath = [path subarrayWithRange:(NSRange){1, path.count-1}];
    }
    for (PDIXMPElement *e in self) {
        value = path.count ? [[e findChildrenWithName:name] firstAvailableElementValueForPath:subPath] : e.value;
        if (value != nil) return value;
//        c = [e find:path createMissing:NO];
    }
    return nil;
}

- (NSString *)firstAvailableValueOfAttribute:(NSString *)attributeName forPath:(NSArray *)path
{
    NSString *value;
    NSString *name;
    NSArray *subPath;
    if (path.count) {
        name = path[0];
        subPath = [path subarrayWithRange:(NSRange){1, path.count-1}];
    }
    for (PDIXMPElement *e in self) {
        value = path.count ? [[e findChildrenWithName:name] firstAvailableValueOfAttribute:attributeName forPath:path] : [e stringOfAttribute:attributeName];
        if (value != nil) return value;
//        c = [e find:path createMissing:NO];
//        if ([c stringOfAttribute:attributeName]) return [c stringOfAttribute:attributeName];
    }
    return nil;
}

- (void)populateElementValues:(NSMutableArray *)values withPath:(NSArray *)path
{
    NSString *name;
    NSArray *subPath;
    if (path.count) {
        name = path[0];
        subPath = [path subarrayWithRange:(NSRange){1, path.count-1}];
    }
    for (PDIXMPElement *e in self) {
        if (path.count) 
            [[e findChildrenWithName:name] populateElementValues:values withPath:subPath];
        else if (e.value)
            [values addObject:e.value];
    }
}

- (NSString *)elementValuesJoinedWithSeparator:(NSString *)separator forPath:(NSArray *)path
{
    NSMutableArray *values = [[NSMutableArray alloc] init];
    [self populateElementValues:values withPath:path];
    
    return values.count ? [values componentsJoinedByString:separator] : nil;
}

@end
