//
// NSObjects+PDIEntity.m
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

#import "NSObjects+PDIEntity.h"
#import "PDString.h"
#import "PDNumber.h"
#import "PDArray.h"
#import "PDDictionary.h"

@implementation NSDictionary (PDIEntity)

- (const char *)PDFString
{
    id value;
    NSMutableString *str = [NSMutableString stringWithString:@"<<"];
    for (NSString *key in [self allKeys]) {
        [str appendFormat:@"/%@ ", key];

        value = self[key];
        if ([value conformsToProtocol:@protocol(PDIEntity)]) {
            [str appendFormat:@"%s", [value PDFString]];
        } else {
            [str appendFormat:@"%@", value];
        }
    }
    [str appendFormat:@">>"];
    
    return [str cStringUsingEncoding:NSUTF8StringEncoding];
}

- (void *)PDValue
{
    PDDictionaryRef dict = PDDictionaryCreateWithCapacity(self.count);
    for (NSString *key in self.allKeys) {
        void *pdv;
        id value = self[key];
        if ([value conformsToProtocol:@protocol(PDIEntity)]) {
            pdv = [value PDValue];
        } else {
            pdv = [[value description] PDValue];
        }
        PDDictionarySetEntry(dict, [key cStringUsingEncoding:NSUTF8StringEncoding], pdv);
    }
    return PDAutorelease(dict);
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

- (void *)PDValue
{
    PDArrayRef array = PDArrayCreateWithCapacity(self.count);
    for (id v in self) {
        if ([v conformsToProtocol:@protocol(PDIEntity)]) {
            PDArrayAppend(array, [v PDValue]);
        } else {
            PDArrayAppend(array, [[v description] PDValue]);
        }
    }
    return PDAutorelease(array);
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

+ (id)objectWithPDString:(PDStringRef)PDString
{
    return (PDStringGetType(PDString) == PDStringTypeName
            ? [PDIName nameWithPDString:PDString]
            : [self stringWithPDFString:PDStringEscapedValue(PDString, false)]); //PDStringEscapedValue(PDString, false)];
}

- (NSString *)PXUString
{
    NSString *s = [[self hasPrefix:@"<"] && [self hasSuffix:@">"] ? [self substringWithRange:(NSRange){1,self.length-2}] : self lowercaseString];
    while (s.length < 32) s = [@"0" stringByAppendingString:s];

    return [NSString stringWithFormat:@"uuid:%@-%@-%@-%@-%@", 
            [s substringWithRange:(NSRange){ 0,8}], 
            [s substringWithRange:(NSRange){ 8,4}], 
            [s substringWithRange:(NSRange){12,4}], 
            [s substringWithRange:(NSRange){16,4}], 
            [s substringWithRange:(NSRange){20,12}] 
            ];
}

- (NSString *)stringByRemovingPDFControlCharacters
{
    if (self.length > 1 && [self characterAtIndex:0] == '(' && [self characterAtIndex:self.length-1] == ')') {
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

- (void *)PDValue
{
    return PDStringWithCString(strdup([self PDFString]));
}

@end

@interface PDIName () 
@property (nonatomic, strong) NSString *s;
@end

@implementation PDIName 

+ (PDIName *)nameWithPDString:(PDStringRef)PDString
{
    PDIName *p = [[PDIName alloc] init];
    p.s = [NSString stringWithPDFString:PDStringNameValue(PDString, false)];
    return p;
}

+ (PDIName *)nameWithString:(NSString *)string
{
    PDIName *p = [[PDIName alloc] init];
    p.s = string;
    return p;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"(PDIName: %p) \"%@\"", self, _s];
}

- (const char *)PDFString
{
    return [[NSString stringWithFormat:@"/%@", self] PDFString];
}

- (void *)PDValue
{
    return PDStringWithName(strdup([_s PDFString]));
}

- (BOOL)isEqualToString:(id)object
{
    return [_s isEqualToString:object];
}

- (NSString *)string
{
    return _s;
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

- (void *)PDValue
{
    return PDStringWithCString(strdup([self PDFString]));
}

- (NSString *)datetimeString
{
    static NSDateFormatter *df = nil;
    if (! df) {
        df = [[NSDateFormatter alloc] init];
        df.timeZone = [NSTimeZone timeZoneWithName:@"UTC"];
        [df setFormatterBehavior:NSDateFormatterBehavior10_4];
        [df setDateFormat:@"yyyy'-'MM'-'dd'T'HH':'mm':'ss'"];
    }
    return [df stringFromDate:self];
}

@end

@implementation NSNumber (PDIEntity)

- (const char *)PDFString
{
    return [[NSString stringWithFormat:@"%@", self] PDFString];
}

- (void *)PDValue
{
    NSString *d = [NSString stringWithFormat:@"%@", self];
    if ([d rangeOfString:@"."].location != NSNotFound) {
        // real
        return PDNumberWithReal(self.doubleValue);
    }
    
//    if ([d hasPrefix:@"-"]) {
        // integer
        return PDNumberWithInteger(self.integerValue);
//    }
    
//    return PDNumberWithSize(self.unsignedIntegerValue);
}

@end
