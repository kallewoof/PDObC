//
// PDIAnnotation.m
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
    PDIObject *_a;
    PDIObject *_uri;
    PDIObject *_dest;
    PDIReference *_dDest;
    PDInstance *_instance;
    PDIString *_directURI;
    NSString *_subtype, *_aType, *_sType;
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
        
        PDIReference *ref;
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
            NSString *s = [_dest valueAtIndex:0];
            if (s) _dDest = [[PDIReference alloc] initWithString:s];
            if (_dest.count > 1) {
                _fit = [NSMutableArray arrayWithCapacity:_dest.count-1];
                for (int i = 1; i < _dest.count; i++) {
                    [_fit addObject:[_dest valueAtIndex:i]];
                }
            }
        }
        
        _a = [_object objectForKey:@"A"];
        if (_a) {
            // WHY IS THERE NO ENABLEMUTATION FOR _a ??????
            _aType = [_a valueForKey:@"Type"];
            
            _sType = [_a valueForKey:@"S"];
            
            if ([_sType isEqualToString:PDIAnnotationSTypeURI]) {
                // fetch URI
                NSString *s = [_a valueForKey:@"URI"];
                if (s) {
                    // it can be a ref or a direct URI
                    ref = [[PDIReference alloc] initWithString:s];
                    if (ref) {
                        _uri = [_instance fetchReadonlyObjectWithID:ref.objectID];
                        //[_uri scheduleMimicWithInstance:_instance];
                        //_mutatingURI = YES;
                    } else {
                        _directURI = [PDIString stringWithPDFString:s];
                    }
                }
            }
        } else if (! _dest) {
            _a = [_instance appendObject];
            [_object setValue:_a forKey:@"A"];
            [_a setValue:PDIAnnotationATypeAction forKey:@"Type"];
        }
    }
    
    return self;
}

- (void)deleteAnnotation
{
    [_object removeObject];
    [_a removeObject];
    [_uri removeObject];
    [_dest removeObject];
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
    return _subtype;
}

- (void)setSubtype:(NSString *)subtype
{
    if ([subtype isEqualToString:_subtype]) return;
    _subtype = subtype;
    [_object setValue:_subtype forKey:@"Subtype"];
}

- (NSString *)aType
{
    return _aType;
}

- (void)setAType:(NSString *)aType
{
    if ([aType isEqualToString:_aType]) return;
    _aType = aType;
    [_a setValue:_aType forKey:@"Type"];
}

- (NSString *)sType
{
    return _sType;
}

- (void)setSType:(NSString *)sType
{
    if ([sType isEqualToString:_sType]) return;
    _sType = sType;
    [_a setValue:_sType forKey:@"S"];
}

- (NSString *)uriString
{
    return _directURI ? [_directURI stringValue] : [_uri primitiveValue];
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
        /*if (! _mutatingA) {
            _mutatingA = YES;
            [_a scheduleMimicWithInstance:_instance];
        }*/
    } else {
        [_uri setPrimitiveValue:uriString];
        /*if (! _mutatingURI) {
            _mutatingURI = YES;
            [_uri scheduleMimicWithInstance:_instance];
        }*/
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
        [_dest removeObject];
        _dDest = nil;
        [_object removeValueForKey:@"Dest"];
        return;
    }
    
    if (_uri) {
        [_object removeValueForKey:@"URI"];
        [_uri removeObject];
    }
    if (_a) {
        [_object removeValueForKey:@"A"];
        [_a removeObject];
    }
    
    _dDest = dDest;
    
    if (_dest == nil) {
        _dest = [_instance appendObject];
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
    }
    
    [_object setValue:_dest forKey:@"Dest"];
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
