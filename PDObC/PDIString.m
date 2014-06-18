//
// PDIString.m
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

#import "PDIString.h"
#import "NSObjects+PDIEntity.h"
#import "PDString.h"

@implementation PDIString {
    NSString *_s;
}

- (const char *)PDFString
{
    return [[NSString stringWithFormat:@"(%@)", _s] cStringUsingEncoding:NSUTF8StringEncoding];
}

- (void *)PDValue
{
    return PDStringWithCString(strdup(_s.PDFString));
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
