//
// PDIPage.c
//
// Copyright (c) 2014 Karl-Johan Alm (http://github.com/kallewoof)
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

#import "PDPage.h"
#import "PDIPage.h"
#import "PDIObject.h"
#import "PDDefines.h"
#import "pd_internal.h"
#import "PDContentStream.h"

@interface PDIPage () {
    NSArray *_contentObjects;
    NSString *_text;
    __weak PDInstance *_instance;
}

@end

@implementation PDIPage

- (void)dealloc
{
    PDRelease(_pageRef);
}

- (id)initWithPage:(PDPageRef)page inInstance:(PDInstance *)instance
{
    self = [super init];
    _pageRef = PDRetain(page);
    _pageObject = [[PDIObject alloc] initWithObject:_pageRef->ob];

    PDRect r = PDPageGetMediaBox(_pageRef);
    _mediaBox = (CGRect) PDRectToOSRect(r);
    _instance = instance;
    
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
        [contents enableMutationViaMimicSchedulingWithInstance:_instance];
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

@end
