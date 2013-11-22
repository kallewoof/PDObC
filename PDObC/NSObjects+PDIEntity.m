//
// NSObjects+PDIEntity.m
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

#import "NSObjects+PDIEntity.h"

@implementation NSDictionary (PDIEntity)

- (const char *)PDFString
{
    id value;
    NSMutableString *str = [NSMutableString stringWithString:@"<<"];
    for (NSString *key in [self allKeys]) {
        [str appendFormat:@"/%@ ", key];

        value = [self objectForKey:key];
        if ([value conformsToProtocol:@protocol(PDIEntity)]) {
            [str appendFormat:@"%s", [value PDFString]];
        } else {
            [str appendFormat:@"%@", value];
        }
    }
    [str appendFormat:@">>"];
    
    return [str cStringUsingEncoding:NSUTF8StringEncoding];
}

@end

@implementation NSArray (PDIEntity)

+ (NSArray *)arrayFromPDRect:(PDRect)rect
{
    return @[@(rect.a.x), @(rect.a.y), @(rect.b.x), @(rect.b.y)];
}

- (const char *)PDFString
{
    NSMutableString *str = [NSMutableString stringWithString:@"["];
    for (id value in self) {
        if ([value conformsToProtocol:@protocol(PDIEntity)]) {
            [str appendFormat:@" %s", [value PDFString]];
        } else {
            [str appendFormat:@" %@", value];
        }
    }
    [str appendFormat:@" ]"];
    
    return [str cStringUsingEncoding:NSUTF8StringEncoding];
}

@end

@implementation NSString (PDIEntity)

+ (NSString *)stringWithPDFString:(const char *)PDFString
{
    NSString *str = [[NSString alloc] initWithCString:PDFString encoding:NSUTF8StringEncoding];
    if (str == nil) {
        str = [[NSString alloc] initWithCString:PDFString encoding:NSASCIIStringEncoding];
    }
    if (str == nil) {
        [NSException raise:@"PDUnknownEncodingException" format:@"PDF string was using an unknown encoding."];
    }
    return str;
}

- (NSString *)stringByRemovingPDFControlCharacters
{
    if ([self characterAtIndex:0] == '(' && [self characterAtIndex:self.length-1] == ')') {
        return [self substringWithRange:(NSRange){1, self.length - 2}];
    }
    return self;
}

- (NSString *)stringByAddingPDFControlCharacters
{
    return [NSString stringWithFormat:@"(%@)", self];
}

- (const char *)PDFString
{
    return [self cStringUsingEncoding:NSUTF8StringEncoding];
}

@end

@implementation NSDate (PDIEntity)

- (const char *)PDFString
{
    NSDateFormatter *df = [[NSDateFormatter alloc] init];
    [df setFormatterBehavior:NSDateFormatterBehavior10_4];
    [df setDateFormat:@"'(D:'yyyyMMddHHmmss')'"];
    return [[df stringFromDate:self] cStringUsingEncoding:NSUTF8StringEncoding];
}

@end
