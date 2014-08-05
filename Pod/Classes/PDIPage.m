//
// PDIPage.c
//
// Copyright (c) 2012 - 2014 Karl-Johan Alm (http://github.com/kallewoof)
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

#import "PDPage.h"
#import "PDIPage.h"
#import "PDIObject.h"
#import "PDDefines.h"
#import "pd_internal.h"
#import "PDIConversion.h"
#import "PDContentStreamTextExtractor.h"

@interface PDIPage () {
    NSArray *_contentObjects;
    NSString *_text;
    __weak PDISession *_session;
}

@end

@implementation PDIPage

- (void)dealloc
{
    PDRelease(_pageRef);
}

- (id)initWithPage:(PDPageRef)page inSession:(PDISession *)session
{
    self = [super init];
    _pageRef = PDRetain(page);
    _pageObject = [[PDIObject alloc] initWithObject:_pageRef->ob];

    PDRect r = PDPageGetMediaBox(_pageRef);
    _mediaBox = (CGRect) PDRectToOSRect(r);
    _session = session;
    
    return self;
}

#pragma mark - Extended properties

- (NSArray *)contentObjects
{
    if (_contentObjects) return _contentObjects;
    
    NSInteger count = PDPageGetContentsObjectCount(_pageRef);
    NSMutableArray *result = [[NSMutableArray alloc] initWithCapacity:count];

    for (NSInteger i = 0; i < count; i++) {
        [result addObject:[[PDIObject alloc] initWithObject:PDPageGetContentsObjectAtIndex(_pageRef, i)]];
    }
    _contentObjects = result;
    return _contentObjects;
//    if (_contents) return _contents;
//    _contents = [[PDIObject alloc] initWithObject:PDPageGetContentsObject(_pageRef)];
//    return _contents;
}

- (NSString *)text
{
    if (_text) return _text;
    NSString *t;
    NSMutableString *result = [NSMutableString string];
    char *buf;
    for (PDIObject *contents in [self contentObjects]) {
        [contents enableMutationViaMimicSchedulingWithSession:_session];
        [contents prepareStream];
        PDContentStreamRef cs = PDContentStreamCreateTextExtractor(contents.objectRef, &buf);
        PDContentStreamExecute(cs);
        PDRelease(cs);
        t = nil;
        if (NULL != strstr(buf, "\xa9")) {
            t = [NSString stringWithCString:buf encoding:NSMacOSRomanStringEncoding];
        }
        if (t == nil) t = @(buf);
        if (t == nil) t = [NSString stringWithCString:buf encoding:NSMacOSRomanStringEncoding];
        if (t == nil) t = [NSString stringWithCString:buf encoding:NSISOLatin1StringEncoding];
        if (t == nil) t = @(buf);
        free(buf);
        if ([t rangeOfString:@"\\251"].location != NSNotFound) {
            t = [t stringByReplacingOccurrencesOfString:@"\\251" withString:@""];
        }
        [result appendString:t];
    }
    _text = result;
    return _text;
}

- (NSArray *)annotRefs
{
    return (id) [PDIConversion fromPDType:PDPageGetAnnotRefs(_pageRef)];
}

@end
