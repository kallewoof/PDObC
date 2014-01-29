//
// PDInstance.m
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

#import "Pajdeg.h"
#import "pd_internal.h"
#import "pd_pdf_implementation.h"

#import "PDTaskBlocks.h"
#import "PDInstance.h"
#import "PDIObject.h"
#import "PDCatalog.h"
#import "PDIReference.h"
#import "PDIXMPArchive.h"

@interface PDInstance () {
    PDPipeRef _pipe;
    PDIObject *_rootObject;
    PDIObject *_infoObject;
    PDIObject *_metadataObject;
    PDIXMPArchive *_metadataXMPArchive;
    PDParserRef _parser;
    PDIReference *_rootRef;
    PDIReference *_infoRef;
}

@end

@interface PDIObject (PDInstance)

- (void)markImmutable;

- (void)setInstance:(PDInstance *)instance;

@end

@implementation PDInstance

- (void)dealloc
{
    if (_pipe) PDRelease(_pipe);
    
    pd_pdf_conversion_discard();
}

- (id)initWithSourcePDFPath:(NSString *)sourcePDFPath destinationPDFPath:(NSString *)destPDFPath
{
    self = [super init];
    if (self) {
        pd_pdf_conversion_use();
        
        if ([sourcePDFPath isEqualToString:destPDFPath]) {
            [NSException raise:NSInvalidArgumentException format:@"Input source and destination source must not be the same file."];
        }
        if (nil == sourcePDFPath || nil == destPDFPath) {
            [NSException raise:NSInvalidArgumentException format:@"Source and destination must be non-nil."];
        }
        _sourcePDFPath = sourcePDFPath;
        _destPDFPath = destPDFPath;
        _pipe = PDPipeCreateWithFilePaths([sourcePDFPath cStringUsingEncoding:NSUTF8StringEncoding], 
                                          [destPDFPath cStringUsingEncoding:NSUTF8StringEncoding]);
        if (NULL == _pipe) {
            return nil;
        }
        
        if (! PDPipePrepare(_pipe)) {
            return nil;
        }
        
        _parser = PDPipeGetParser(_pipe);
        
        // to avoid issues later on, we also set up the catalog here
        if ([self numberOfPages] == 0) {
            return nil;
        }
    }
    return self;
}

- (id)initWithSourceURL:(NSURL *)sourceURL destinationPDFPath:(NSString *)destPDFPath
{
    if ([sourceURL isFileURL]) {
        return [self initWithSourcePDFPath:[sourceURL path] destinationPDFPath:destPDFPath];
    }
    /// @todo: Network stream support (Pajdeg? PDObC?)
    return nil;
}

- (BOOL)execute
{
    NSAssert(_pipe, @"-execute called more than once, or initialization failed in PDInstance");
    
    _objectSum = PDPipeExecute(_pipe);
    
    PDRelease(_pipe);
    _pipe = NULL;
    
    return _objectSum != -1;
}

- (BOOL)encrypted
{
    return true == PDParserGetEncryptionState(_parser);
}

- (PDIObject *)verifiedMetadataObject
{
    if (_metadataObject) return _metadataObject;
    PDIObject *root = [self rootObject];
    NSString *md = [root valueForKey:@"Metadata"];
    if (md) {
        _metadataObject = [self fetchReadonlyObjectWithID:[PDIReference objectIDFromString:md]];
        [_metadataObject enableMutationViaMimicSchedulingWithInstance:self];
    } else {
        _metadataObject = [self appendObject];
        [root setValue:_metadataObject forKey:@"Metadata"];
        [root enableMutationViaMimicSchedulingWithInstance:self];
    }
    return _metadataObject;
}

- (PDIXMPArchive *)metadataXMPArchive
{
    if (_metadataXMPArchive) return _metadataXMPArchive;
    PDIObject *mdo = [self verifiedMetadataObject];

    _metadataXMPArchive = [[PDIXMPArchive alloc] initWithObject:mdo];
    if (_metadataXMPArchive == nil) {
        CGPDFDocumentRef doc = CGPDFDocumentCreateWithURL((CFURLRef)[NSURL fileURLWithPath:_sourcePDFPath]);
        _metadataXMPArchive = [[PDIXMPArchive alloc] initWithCGPDFDocument:doc];
        CGPDFDocumentRelease(doc);
    }

    return _metadataXMPArchive;
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
        _rootObject = [[PDIObject alloc] initWithObject:PDParserGetRootObject(_parser)];
    }
    return _rootObject;
}

- (PDIObject *)infoObject
{
    if (_infoObject == nil) {
        PDObjectRef infoObj = PDParserGetInfoObject(_parser);
        if (infoObj) {
            _infoObject = [[PDIObject alloc] initWithObject:infoObj];
        }
    }
    return _infoObject;
}

- (PDIObject *)verifiedInfoObject
{
    if (_infoObject || [self infoObject]) return _infoObject;
    
    _infoObject = [self appendObject];
    _parser->infoRef = PDRetain([[_infoObject reference] PDReference]);
    
    PDObjectRef trailer = _parser->trailer;
    PDIObject *trailerOb = [[PDIObject alloc] initWithObject:trailer];
    //[trailerOb setInstance:self];
    [trailerOb setValue:_infoObject forKey:@"Info"];

    return _infoObject;
}

- (PDIObject *)createObject:(BOOL)append
{
    PDObjectRef ob = append ? PDParserCreateAppendedObject(_parser) : PDParserCreateNewObject(_parser);
    PDIObject *iob = [[PDIObject alloc] initWithObject:ob];
    [iob markMutable];
    PDRelease(ob);
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

- (PDIObject *)fetchReadonlyObjectWithID:(NSInteger)objectID
{
    pd_stack defs = PDParserLocateAndCreateDefinitionForObject(_parser, objectID, true);
    PDIObject *object = [[PDIObject alloc] initWithInstance:self forDefinitionStack:defs objectID:objectID generationID:0];
    return object;
}

- (NSInteger)numberOfPages
{
    PDCatalogRef catalog = PDParserGetCatalog(_parser);
    return (catalog ? PDCatalogGetPageCount(catalog) : 0);
}

- (NSInteger)objectIDForPageNumber:(NSInteger)pageNumber
{
    PDCatalogRef catalog = PDParserGetCatalog(_parser);
    return PDCatalogGetObjectIDForPage(catalog, pageNumber);
}

- (void)enqueuePropertyType:(PDPropertyType)type value:(NSInteger)value operation:(PDIObjectOperation)operation
{
    PDTaskRef filter, task;
    
    filter = PDTaskCreateFilterWithValue(type, value);
    
    task = PDTaskCreateBlockMutator(^PDTaskResult(PDPipeRef pipe, PDTaskRef task, PDObjectRef object) {
        PDIObject *iob = [[PDIObject alloc] initWithObject:object];
        [iob markMutable];
        [iob setInstance:self];
        return operation(self, iob);
    });
    
    PDTaskAppendTask(filter, task);
    PDPipeAddTask(_pipe, filter);
    
    PDRelease(task);
    PDRelease(filter);
}

- (void)forObjectWithID:(NSInteger)objectID enqueueOperation:(PDIObjectOperation)operation
{
    NSAssert(objectID != 0, @"Object ID must not be 0.");
    [self enqueuePropertyType:PDPropertyObjectId value:objectID operation:operation];
}

- (void)enqueueOperation:(PDIObjectOperation)operation
{
    PDPipeAddTask(_pipe, PDTaskCreateBlockMutator(^PDTaskResult(PDPipeRef pipe, PDTaskRef task, PDObjectRef object) {
        PDIObject *iob = [[PDIObject alloc] initWithObject:object];
        return operation(self, iob);
    }));
}

- (void)setRootStream:(NSData *)data forKey:(NSString *)key
{
    if ([key isEqualToString:@"Metadata"]) {
        // We use the built-in verifiedMetadata method to do this; if we don't, we run the risk of double-tasking the metadata stream and overwriting requested changes.
        [self verifiedMetadataObject];
        [_metadataObject setStreamIsEncrypted:NO];
        [_metadataObject setStreamContent:data];
        return;
    }
    
    // got a Root?
    if ([self rootObject]) {
        NSString *ref = [_rootObject valueForKey:key];
        if (ref) {
            // got this already; we want to tweak it then
            [self forObjectWithID:[PDIReference objectIDFromString:ref] enqueueOperation:^PDTaskResult(PDInstance *instance, PDIObject *object) {
                [object setStreamIsEncrypted:NO];
                [object setStreamContent:data];
                return PDTaskDone;
            }];
        } else {
            // don't have the key; we want to add an object then
            PDIObject *ob = [self appendObject];
            [ob setStreamIsEncrypted:NO];
            [ob setStreamContent:data];
            
            [_rootObject enableMutationViaMimicSchedulingWithInstance:self];
            [_rootObject setValue:ob forKey:key];
            
            /*[self forObjectWithID:_rootObject.objectID enqueueOperation:^PDTaskResult(PDInstance *instance, PDIObject *object) {
                [object setValue:ob forKey:key];
                return PDTaskDone;
            }];*/
        }
    } else {
        // it's an easy enough thing to create a root object and stuff it into the trailer, but then again, a PDF with no root object is either misinterpreted beyond oblivion (by Pajdeg) or it's broken beyond repair (by someone) so we choose to die here; if you wish to support a PDF with no root object, some form of flag may be added in the future, but I can't see why anyone would want this
        fprintf(stderr, "I thought all pdfs had root objects...\n");
        [NSException raise:@"PDInvalidDocumentException" format:@"No root object exists in the document."];
    }
}

- (NSInteger)totalObjectCount
{
    return PDParserGetTotalObjectCount(_parser);
}

@end
