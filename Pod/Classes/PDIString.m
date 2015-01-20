//
// PDIString.m
//
// Copyright (c) 2012 - 2015 Karl-Johan Alm (http://github.com/kallewoof)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#import "PDIString.h"
#import "NSObjects+PDIEntity.h"
#import "PDString.h"
#import "pd_internal.h"

@implementation PDIString {
    NSString *_s;
}

- (const char *)PDFString
{
    return [[NSString stringWithFormat:@"(%@)", _s] cStringUsingEncoding:NSUTF8StringEncoding];
}

- (void *)PDValue
{
//    return PDStringWithCString(strdup(_s.PDFString));
    return PDStringEscapingCString(strdup(_s.PDFString));
}

- (id)initWithString:(NSString *)string
{
    self = [super init];
    if (self) {
        _s = string;
    }
    return self;
}

- (id)initWithPDFString:(NSString *)pdfString
{
    self = [super init];
    if (self) {
        _s = [pdfString stringByRemovingPDFControlCharacters];
    }
    return self;
}

- (id)initWithFormat:(NSString *)format arguments:(va_list)argList
{
    self = [super init];
    if (self) {
        _s = [[NSString alloc] initWithFormat:format arguments:argList];
    }
    return self;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"(PDF string:\"%@\")", _s];
}

+ (id)stringOrObject:(id)object
{
    return [object isKindOfClass:[NSString class]] ? [object PDIString] : object;
}

+ (PDIString *)stringWithString:(NSString *)string
{
    return [[PDIString alloc] initWithString:string];
}

+ (PDIString *)stringWithPDFString:(NSString *)PDFString
{
    return [[PDIString alloc] initWithPDFString:PDFString];
}

+ (PDIString *)stringWithFormat:(NSString *)format, ...
{
    va_list ap;
    va_start (ap, format);
    PDIString *body = [[PDIString alloc] initWithFormat:format arguments:ap];
    va_end (ap);
    return body;
}

- (NSString *)stringValue
{
    return _s;
}

@end

@implementation NSString (PDIValue)

- (PDIString *)PDIString
{
    return [PDIString stringWithString:self];
}

@end
