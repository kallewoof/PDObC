//
//  NSObjects+PDIEntity.m
//
//  Copyright (c) 2013 Karl-Johan Alm (http://github.com/kallewoof)
// 
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
// 
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
// 
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
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

- (const char *)PDFString
{
    return [[NSString stringWithFormat:@"(%@)", self] cStringUsingEncoding:NSUTF8StringEncoding];
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
