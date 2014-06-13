//
// NSArray+PDIXMPArchive.m
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

#import "NSArray+PDIXMPArchive.h"
#import "PDIXMPElement.h"

@implementation NSArray (PDIXMPArchive)

- (NSString *)firstAvailableElementValueForPath:(NSArray *)path
{
    NSString *value;
    NSString *name = nil;
    NSArray *subPath = nil;
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
    NSString *name = nil;
    NSArray *subPath = nil;
    if (path.count) {
        name = path[0];
        subPath = [path subarrayWithRange:(NSRange){1, path.count-1}];
    }
    for (PDIXMPElement *e in self) {
        value = path.count ? [[e findChildrenWithName:name] firstAvailableValueOfAttribute:attributeName forPath:subPath] : [e stringOfAttribute:attributeName];
        if (value != nil) return value;
//        c = [e find:path createMissing:NO];
//        if ([c stringOfAttribute:attributeName]) return [c stringOfAttribute:attributeName];
    }
    return nil;
}

- (void)populateElementValues:(NSMutableArray *)values withPath:(NSArray *)path
{
    NSString *name = nil;
    NSArray *subPath = nil;
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
