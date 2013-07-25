//
//  PDInstance.m
//
//  Copyright (c) 2013 Karl-Johan Alm (http://github.com/kallewoof)
// 
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
// 
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
// 
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#import "Pajdeg.h"
#import "PDInternal.h"

#import "PDTaskBlocks.h"
#import "PDInstance.h"
#import "PDIObject.h"
#import "PDIReference.h"

@interface PDInstance () {
    PDPipeRef _pipe;
    PDIObject *_rootObject;
    PDParserRef _parser;
    PDIReference *_rootRef;
    PDIReference *_infoRef;
}

@end

@interface PDIObject (PDInstance)

- (void)markImmutable;

@end

@implementation PDInstance

- (void)dealloc
{
    [_rootObject release];
    [_rootRef release];
    [_infoRef release];
    [_sourcePDFPath release];
    [_destPDFPath release];
    if (_pipe) PDPipeDestroy(_pipe);
    [super dealloc];
}

- (id)initWithSourcePDFPath:(NSString *)sourcePDFPath destinationPDFPath:(NSString *)destPDFPath
{
    self = [super init];
    if (self) {
        if ([sourcePDFPath isEqualToString:destPDFPath]) {
            [NSException raise:NSInvalidArgumentException format:@"Input source and destination source must not be the same file."];
        }
        if (nil == sourcePDFPath || nil == destPDFPath) {
            [NSException raise:NSInvalidArgumentException format:@"Source and destination must be non-nil."];
        }
        _sourcePDFPath = [sourcePDFPath retain];
        _destPDFPath = [destPDFPath retain];
        _pipe = PDPipeCreateWithFilePaths([sourcePDFPath cStringUsingEncoding:NSUTF8StringEncoding], 
                                          [destPDFPath cStringUsingEncoding:NSUTF8StringEncoding]);
        if (NULL == _pipe) {
            [self release];
            return nil;
        }
        
        if (! PDPipePrepare(_pipe)) {
            [self release];
            return nil;
        }
        _parser = PDPipeGetParser(_pipe);
    }
    return self;
}

- (BOOL)execute
{
    NSAssert(_pipe, @"-execute called more than once, or initialization failed in PDInstance");
    
    _objectSum = PDPipeExecute(_pipe);
    
    PDPipeDestroy(_pipe);
    _pipe = NULL;
    
    return _objectSum != -1;
}

- (PDIReference *)rootReference
{
    if (_rootRef == nil && _parser->rootRef) {
        _rootRef = [[PDIReference alloc] initWithReference:_parser->rootRef];
    }
    return _rootRef;
}

- (PDIReference *)infoReference
{
    if (_infoRef == nil && _parser->infoRef) {
        _infoRef = [[PDIReference alloc] initWithReference:_parser->infoRef];
    }
    return _infoRef;
}

- (PDIObject *)rootObject
{
    if (_rootObject == nil) {
        if (_parser->rootRef) {
            PDStackRef rootDef = PDParserLocateAndCreateDefinitionForObject(_parser, _parser->rootRef->obid, 300, true);
            assert(rootDef);
            _rootObject = [[PDIObject alloc] initWithDefinitionStack:rootDef objectID:_parser->rootRef->obid generationID:_parser->rootRef->genid];
        }
    }
    return _rootObject;
}

- (PDIObject *)createObject:(BOOL)append
{
    PDObjectRef ob = append ? PDParserCreateAppendedObject(_parser) : PDParserCreateNewObject(_parser);
    PDIObject *iob = [[[PDIObject alloc] initWithObject:ob] autorelease];
    PDObjectRelease(ob);
    return iob;
}

- (PDIObject *)insertObject
{
    return [self createObject:NO];
}

- (PDIObject *)appendObject
{
    return [self createObject:YES];
}

- (void)enqueuePropertyType:(PDPropertyType)type value:(NSInteger)value operation:(PDIObjectOperation)operation
{
    PDTaskRef filter, task;
    
    filter = PDTaskCreateFilterWithValue(type, value);
    
    task = PDTaskCreateBlockMutator(^PDTaskResult(PDPipeRef pipe, PDTaskRef task, PDObjectRef object) {
        PDIObject *iob = [[PDIObject alloc] initWithObject:object];
        operation(self, iob);
        [iob release];
        return PDTaskDone;
    });
    
    PDTaskAppendTask(filter, task);
    PDPipeAddTask(_pipe, filter);
    
    PDTaskRelease(task);
    PDTaskRelease(filter);
}

- (void)forObjectWithID:(NSInteger)objectID enqueueOperation:(PDIObjectOperation)operation
{
    [self enqueuePropertyType:PDPropertyObjectId value:objectID operation:operation];
}

/*- (void)enqueueOperationAfterLastObject:(PDIObjectOperation)operation
{
    [self enqueuePropertyType:PDPropertyLate value:0 operation:operation];
}*/

- (void)setRootStream:(NSData *)data forKey:(NSString *)key
{
    // got a Root?
    if ([self rootObject]) {
        NSString *ref = [_rootObject valueForKey:key];
        if (ref) {
            // got this already; we want to tweak it then
            NSInteger refID = [[[ref componentsSeparatedByString:@" "] objectAtIndex:0] integerValue];
            [self forObjectWithID:refID enqueueOperation:^(PDInstance *instance, PDIObject *object) {
                [object setStreamIsEncrypted:NO];
                [object setStreamContent:data];
            }];
        } else {
            // don't have the key; we want to add an object then
            PDIObject *ob = [self appendObject];
            [ob setStreamIsEncrypted:NO];
            [ob setStreamContent:data];
            
            [self forObjectWithID:_rootObject.objectID enqueueOperation:^(PDInstance *instance, PDIObject *object) {
                [object setValue:ob forKey:key];
            }];
        }
    } else {
        // it's an easy enough thing to create a root object and stuff it into the trailer, but then again, a PDF with no root object is either misinterpreted beyond oblivion (by Pajdeg) or it's broken beyond repair (by someone) so we choose to die here; if you wish to support a PDF with no root object, some form of flag may be added in the future, but I can't see why anyone would want this
        fprintf(stderr, "I thought all pdfs had root objects...\n");
        [NSException raise:@"PDInvalidDocumentException" format:@"No root object exists in the document."];
    }
}

@end
