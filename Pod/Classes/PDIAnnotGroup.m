//
// PDIAnnotGroup.m
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

#import "pd_internal.h"
#import "PDIAnnotGroup.h"
#import "PDIObject.h"
#import "PDIAnnotation.h"
#import "NSObjects+PDIEntity.h"
#import "PDISession.h"
#import "PDArray.h"
#import "PDIReference.h"

@implementation PDIAnnotGroup {
    NSMutableArray *_annotations;
    PDISession *_session;
}

- (id)initWithObject:(PDIObject *)object inSession:(PDISession *)session
{
    assert([object isKindOfClass:PDIObject.class]);
    self = [super init];
    if (self) {
        _session = session;
        
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
                obj = [_session fetchReadonlyObjectWithID:[(PDIReference *)entry objectID]];
            } else {
                obj = [[PDIObject alloc] initWrappingValue:entry PDValue:PDArrayGetElement(_object.objectRef->inst, i)];
//                obj = [session appendObject];
//                [obj setObjectValue:entry];
//                [object replaceValueAtIndex:i withValue:obj];
            }
            
            PDIAnnotation *annot = [[PDIAnnotation alloc] initWithObject:obj inAnnotGroup:self withSession:session];
            [_annotations addObject:annot];
        }
        
        [_object enableMutationViaMimicSchedulingWithSession:session];
    }
    return self;
}

- (PDIAnnotation *)appendAnnotation
{
    PDIObject *annotObj = [_session appendObject];
    [annotObj setValue:[PDIName nameWithString:@"/Annot"] forKey:@"Type"];
    PDIAnnotation *annot = [[PDIAnnotation alloc] initWithObject:annotObj inAnnotGroup:self withSession:_session];
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

@implementation PDIAnnotGroup (PDIDeprecated)

- (id)initWithObject:(PDIObject *)object inInstance:(PDISession *)instance
{
    return [self initWithObject:object inSession:instance];
}

@end