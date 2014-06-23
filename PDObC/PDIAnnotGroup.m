//
// PDIAnnotGroup.m
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

#import "pd_internal.h"
#import "PDIAnnotGroup.h"
#import "PDIObject.h"
#import "PDIAnnotation.h"
#import "NSObjects+PDIEntity.h"
#import "PDInstance.h"
#import "PDArray.h"
#import "PDIReference.h"

@implementation PDIAnnotGroup {
    NSMutableArray *_annotations;
    PDInstance *_instance;
}

- (id)initWithObject:(PDIObject *)object inInstance:(PDInstance *)instance
{
    assert([object isKindOfClass:PDIObject.class]);
    self = [super init];
    if (self) {
        _instance = instance;
        
        _object = object;
        NSInteger count = object.count;
        if (count == 0 && object.type != PDObjectTypeArray) {
            // it may be uninitialized; if type is unknown, it is
            if (object.type != PDObjectTypeUnknown) {
                [NSException raise:@"PDIAnnotGroupInvalidObject" format:@"The object does not appear to be an Annots array."];
            }
            count = 1;
        }
        
        _annotations = [[NSMutableArray alloc] initWithCapacity:count];
        PDIObject *obj;
        NSArray *array = [object constructArray];
        NSInteger acount = array.count;
        for (NSInteger i = 0; i < acount; i++) {
            id entry = array[i];
            if ([entry isKindOfClass:[PDIReference class]]) {
                obj = [_instance fetchReadonlyObjectWithID:[entry objectID]];
            } else {
                obj = [[PDIObject alloc] initWrappingValue:entry PDValue:PDArrayGetElement(_object.objectRef->inst, i)];
//                obj = [instance appendObject];
//                [obj setObjectValue:entry];
//                [object replaceValueAtIndex:i withValue:obj];
            }
            
            PDIAnnotation *annot = [[PDIAnnotation alloc] initWithObject:obj inAnnotGroup:self withInstance:instance];
            [_annotations addObject:annot];
        }
        
        [_object enableMutationViaMimicSchedulingWithInstance:instance];
    }
    return self;
}

- (PDIAnnotation *)appendAnnotation
{
    PDIObject *annotObj = [_instance appendObject];
    [annotObj setValue:[PDIName nameWithString:@"/Annot"] forKey:@"Type"];
    PDIAnnotation *annot = [[PDIAnnotation alloc] initWithObject:annotObj inAnnotGroup:self withInstance:_instance];
    [_object appendValue:annotObj];
    [_annotations addObject:annot];
    return annot;
}

- (void)removeAnnotation:(PDIAnnotation *)annotation
{
    if (! annotation.object.willBeRemoved) {
        [annotation deleteAnnotation];
        return;
    }
    
    NSInteger index = [_annotations indexOfObject:annotation];
    assert(index != NSNotFound); // crash = you tried to call removeAnnotation: on a PDIAnnotGroup for a PDIAnnotation that WAS NOT CREATED BY THAT ANNOT GROUP -- alternatively, you called removeAnnotation: twice with the same object
    
    [_object removeValueAtIndex:index];
    [_annotations removeObjectAtIndex:index];
}

@end
