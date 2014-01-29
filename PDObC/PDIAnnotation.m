//
// PDIAnnotation.m
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

#import "PDIObject.h"
#import "PDIAnnotation.h"
#import "PDIAnnotGroup.h"
#import "PDIReference.h"
#import "PDInstance.h"
#import "NSObjects+PDIEntity.h"
#import "PDIString.h"

@implementation PDIAnnotation {
    PDIObject *_a;
    PDIObject *_uri;
    PDInstance *_instance;
    PDIString *_directURI;
    NSString *_subtype, *_aType, *_sType;
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
        NSString *s = [_object valueForKey:@"Rect"];
        PDRect rect;
        if (s) {
            PDRectReadFromArrayString(rect, [s cStringUsingEncoding:NSUTF8StringEncoding]);
            _rect = (CGRect)PDRectToOSRect(rect);
        }
        
        _subtype = [_object valueForKey:@"Subtype"];
        
        _a = [_object objectForKey:@"A"];
        //s = [_object valueForKey:@"A"];
        if (_a) {
            //ref = [[PDIReference alloc] initWithString:s];
            //_a = [_instance fetchReadonlyObjectWithID:ref.objectID];
            //[_a scheduleMimicWithInstance:_instance];
            //_mutatingA = YES;
            
            _aType = [_a valueForKey:@"Type"];
            
            _sType = [_a valueForKey:@"S"];
            
            if ([_sType isEqualToString:PDIAnnotationSTypeURI]) {
                // fetch URI
                s = [_a valueForKey:@"URI"];
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
        } else {
            _a = [_instance appendObject];
            [_object setValue:_a forKey:@"A"];
            [_a setValue:PDIAnnotationATypeAction forKey:@"Type"];
        }
    }
    
    return self;
}

- (CGRect)rect
{
    return _rect;
}

- (void)setRect:(CGRect)rect
{
    _rect = rect;
    PDRect r = PDRectFromOSRect(_rect);
    [_object setValue:[NSArray arrayFromPDRect:r] forKey:@"Rect"];
    /*if (! _mutatingA) {
        _mutatingA = YES;
        [_a scheduleMimicWithInstance:_instance];
    }*/
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

@end
