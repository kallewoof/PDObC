//
// PDIAnnotGroup.m
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

#import "PDIAnnotGroup.h"
#import "PDIObject.h"
#import "PDIAnnotation.h"
#import "PDInstance.h"
#import "PDIReference.h"

@implementation PDIAnnotGroup {
    NSMutableArray *_annotations;
    PDInstance *_instance;
}

- (id)initWithObject:(PDIObject *)object inInstance:(PDInstance *)instance
{
    self = [super init];
    if (self) {
        _instance = instance;
        
        _object = object;
        NSInteger count = object.count;
        if (count == 0) {
            // it may be uninitialized; if type is unknown, it is
            if (object.type != PDObjectTypeUnknown) {
                [NSException raise:@"PDIAnnotGroupInvalidObject" format:@"The object does not appear to be an Annots array."];
            }
            count = 1;
        }
        _annotations = [[NSMutableArray alloc] initWithCapacity:count];
        for (NSString *ref in [object constructArray]) {
            PDIObject *obj = [_instance fetchReadonlyObjectWithID:[PDIReference objectIDFromString:ref]];
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
    [annotObj setValue:@"/Annot" forKey:@"Type"];
    PDIAnnotation *annot = [[PDIAnnotation alloc] initWithObject:annotObj inAnnotGroup:self withInstance:_instance];
    [_object appendValue:annotObj];
    [_annotations addObject:annot];
    return annot;
}

@end
