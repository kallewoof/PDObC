//
// PDIAnnotation.m
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

#import "PDIObject.h"
#import "PDIAnnotation.h"
#import "PDIAnnotGroup.h"
#import "PDIReference.h"
#import "PDInstance.h"
#import "NSObjects+PDIEntity.h"
#import "PDIString.h"
#import "PDIConversion.h"
#import "PDNumber.h"

@implementation PDIAnnotation {
    BOOL _ownsA, _ownsURI, _ownsDest;
    PDIObject *_a;
    PDIObject *_uri;
    PDIObject *_dest;
    PDIReference *_dDest;
    PDInstance *_instance;
    PDIString *_directURI;
    PDIName *_subtype, *_aType, *_sType;
    NSMutableArray *_fit;
    CGRect _rect;
}

- (id)initWithObject:(PDIObject *)object inAnnotGroup:(PDIAnnotGroup *)annotGroup withInstance:(PDInstance *)instance
{
    self = [super init];
    if (self) {
        _instance = instance;
        _annotGroup = annotGroup;
        _object = object;
        [_object enableMutationViaMimicSchedulingWithInstance:instance];
        
//        PDIReference *ref;
        NSArray *arr = [_object valueForKey:@"Rect"];
        if (arr) assert([arr isKindOfClass:NSArray.class]);
        PDRect rect;
        if (arr && arr.count == 4) {
            arr = [arr arrayByResolvingPDValues];
            rect = (PDRect) { [arr[0] floatValue], [arr[1] floatValue], [arr[2] floatValue], [arr[3] floatValue] };
//            PDRectReadFromArrayString(rect, [s cStringUsingEncoding:NSUTF8StringEncoding]);
            _rect = (CGRect)PDRectToOSRect(rect);
        }
        
        _subtype = [_object valueForKey:@"Subtype"];
        
        _dest = [_object objectForKey:@"Dest"];
        if (_dest) {
            [_dest enableMutationViaMimicSchedulingWithInstance:_instance];
            if (_dest.type == PDObjectTypeString) {
                // this is a named destination
                // we don't deal with that right now
            } else {
                PDIReference *ref = [_dest valueAtIndex:0];
                if (ref) _dDest = ref; //[[PDIReference alloc] initWithString:s];
                if (_dest.count > 1) {
                    _fit = [NSMutableArray arrayWithCapacity:_dest.count-1];
                    for (int i = 1; i < _dest.count; i++) {
                        [_fit addObject:[_dest valueAtIndex:i]];
                    }
                }
            }
        }
        
        _a = [_object objectForKey:@"A"];
        if (_a) {
            _aType = [_a valueForKey:@"Type"];
            _sType = [_a valueForKey:@"S"];
            
            if ([_sType isEqualToString:PDIAnnotationSTypeURI]) {
                // fetch URI
                id uriValue = [_a valueForKey:@"URI"];
                if (uriValue) {
                    // it can be a ref or a direct URI
//                    ref = [[PDIReference alloc] initWithString:s];
                    if ([uriValue isKindOfClass:[PDIReference class]]) {
                        _uri = [_instance fetchReadonlyObjectWithID:[(PDIReference *)uriValue objectID]];
                        //[_uri scheduleMimicWithInstance:_instance];
                        //_mutatingURI = YES;
                    } else {
                        _directURI = [PDIString stringWithPDFString:uriValue];
                    }
                }
            }
        } else if (! _dest) {
            _ownsA = YES;
            _a = [_instance appendObject];
            [_object setValue:_a forKey:@"A"];
            [_a setValue:[PDIName nameWithString:PDIAnnotationATypeAction] forKey:@"Type"];
        }
    }
    
    return self;
}

- (void)deleteAnnotation
{
    [_object removeObject];
    if (_ownsA) [_a removeObject];
    if (_ownsURI) [_uri removeObject];
    if (_ownsDest) [_dest removeObject];
    [_annotGroup removeAnnotation:self];
}

#pragma mark - 

- (CGRect)rect
{
    return _rect;
}

- (void)setRect:(CGRect)rect
{
    _rect = rect;
    PDRect r = PDRectFromOSRect(_rect);
    [_object setValue:[NSArray arrayFromPDRect:r] forKey:@"Rect"];
}

- (NSString *)subtype
{
    return _subtype.string;
}

- (void)setSubtype:(NSString *)subtype
{
    if ([_subtype isEqualToString:subtype]) return;
    _subtype = [PDIName nameWithString:subtype];
    [_object setValue:_subtype forKey:@"Subtype"];
}

- (NSString *)aType
{
    return _aType.string;
}

- (void)setAType:(NSString *)aType
{
    if ([_aType isEqualToString:aType]) return;
    _aType = [PDIName nameWithString:aType];
    [_a setValue:_aType forKey:@"Type"];
}

- (NSString *)sType
{
    return _sType.string;
}

- (void)setSType:(NSString *)sType
{
    if ([_sType isEqualToString:sType]) return;
    _sType = [PDIName nameWithString:sType];
    [_a setValue:_sType forKey:@"S"];
}

- (NSString *)uriString
{
    return _directURI ? [_directURI stringValue] : [_uri objectValue];
}

- (void)setUriString:(NSString *)uriString
{
    [_uri unremoveObject];
    [_a unremoveObject];

    self.subtype = PDIAnnotationSubtypeLink;
    self.aType = PDIAnnotationATypeAction;
    self.sType = PDIAnnotationSTypeURI;

    if (_uri == nil) {
        _directURI = [PDIString stringWithString:uriString];
        [_a setValue:_directURI forKey:@"URI"];
    } else {
        [_uri setObjectValue:uriString];
    }
}

- (PDIReference *)dDest
{
    return _dDest;
}

- (void)setDDest:(PDIReference *)dDest
{
    if (dDest == nil) {
        if (_uri) {
            [_object setValue:_uri forKeyPath:@"URI"];
            [_uri unremoveObject];
        }
        if (_a) {
            [_object setValue:_a forKeyPath:@"A"];
            [_a unremoveObject];
        }
        if (_ownsDest) {
            [_dest removeObject];
        }
        _dDest = nil;
        [_object removeValueForKey:@"Dest"];
        return;
    }
    
    if (_uri) {
        [_object removeValueForKey:@"URI"];
        if (_ownsURI) [_uri removeObject];
    }
    if (_a) {
        [_object removeValueForKey:@"A"];
        if (_ownsA) [_a removeObject];
    }
    
    _dDest = dDest;
    
    if (_dest == nil) {
        _ownsDest = YES;
        NSMutableArray *a = [[NSMutableArray alloc] init];
        [_object setValue:a forKey:@"Dest"];
        _dest = [_object objectForKey:@"Dest"];
        [_dest appendValue:_dDest];
        if (_fit.count) 
            for (int i = 0; i < _fit.count; i++) {
                [_dest appendValue:_fit[i]];
            }
    } else {
        [_dest unremoveObject];
        if (_dest.count > 0)
            [_dest replaceValueAtIndex:0 withValue:dDest];
        else
            [_dest appendValue:dDest];
        [_object setValue:_dest.objectValue forKey:@"Dest"];
    }
}

- (void)setDDestByPageIndex:(NSInteger)pageIndex
{
    self.dDest = [[PDIReference alloc] initWithObjectID:[_instance objectIDForPageNumber:pageIndex] generationID:0];
}

- (NSArray *)fit
{
    return _fit;
}

- (void)setFit:(NSArray *)fit
{
    _fit = [fit mutableCopy];
    if (_dest) {
        for (NSInteger i = 0; i < _fit.count; i++) {
            if (i + 1 < _dest.count)
                [_dest replaceValueAtIndex:i+1 withValue:fit[i]];
            else
                [_dest appendValue:fit[i]];
        }
        NSInteger count = _fit.count + 1;
        while (_dest.count > count) {
            [_dest removeValueAtIndex:count];
        }
    }
}

@end
