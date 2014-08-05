
// PDISession.m
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

#import "Pajdeg.h"
#import "pd_internal.h"
#import "pd_pdf_implementation.h"

#import "PDITaskBlocks.h"
#import "PDISession.h"
#import "PDIObject.h"
#import "PDIPage.h"
#import "PDPage.h"
#import "PDCatalog.h"
#import "PDPage.h"
#import "PDIReference.h"
#import "NSObjects+PDIEntity.h"

#import "PDString.h"
#import "PDDictionary.h"
#import "PDArray.h"

@interface PDISession () {
    PDPipeRef _pipe;
    PDIObject *_rootObject;
    PDIObject *_infoObject;
    PDIObject *_trailerObject;
    PDIObject *_metadataObject;
    PDParserRef _parser;
    PDIReference *_rootRef;
    PDIReference *_infoRef;
    NSString *_documentID;
    NSString *_documentInstanceID;
    NSMutableDictionary *_pageDict;
    BOOL _fetchedDocIDs;
}

@end

@interface PDIPage (PDISession)

- (id)initWithPage:(PDPageRef)page inSession:(PDISession *)session;

@end

@interface PDIObject (PDISession)

- (void)markImmutable;

- (void)setSession:(PDISession *)session;

@end

@implementation PDISession

- (void)dealloc
{
    PDRelease(_pipe);
    
    pd_pdf_conversion_discard();
}

- (id)initWithSourcePDFPath:(NSString *)sourcePDFPath destinationPDFPath:(NSString *)destPDFPath
{
    self = [super init];
    if (self) {
        pd_pdf_conversion_use();
        
        _sessionDict = [[NSMutableDictionary alloc] init];
        
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
            PDError("PDPipeCreateWithFilePaths() failure");
            return nil;
        }
        
        if (! PDPipePrepare(_pipe)) {
            PDError("PDPipePrepare() failure");
            PDRelease(_pipe);
            _pipe = NULL;
            return nil;
        }
        
        _parser = PDPipeGetParser(_pipe);
        
        // to avoid issues later on, we also set up the catalog here
        if ([self numberOfPages] == 0) {
            PDError("numberOfPages == 0 (this is considered a failure)");
            return nil;
        }
        
        _pageDict = [[NSMutableDictionary alloc] init];
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
    NSAssert(_pipe, @"-execute called more than once, or initialization failed in PDISession");
    
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
    _metadataObject = [root resolvedValueForKey:@"Metadata"];
//    NSString *md = [root valueForKey:@"Metadata"];
    if (_metadataObject) {
        if ([_metadataObject isKindOfClass:[PDIReference class]]) {
            _metadataObject = [self fetchReadonlyObjectWithID:[(PDIReference *)_metadataObject objectID]];
        }
//        _metadataObject = [self fetchReadonlyObjectWithID:[PDIReference objectIDFromString:md]];
        [_metadataObject enableMutationViaMimicSchedulingWithSession:self];
    } else {
        _metadataObject = [self appendObject];
        _metadataObject.type = PDObjectTypeDictionary; // we set the type explicitly, because the metadata object isn't always modified; if it isn't modified, Pajdeg considers it illegal to add it, as it requires that new objects have a type
        [root enableMutationViaMimicSchedulingWithSession:self];
        [root setValue:_metadataObject forKey:@"Metadata"];
    }
    return _metadataObject;
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
        [_rootObject setSession:self];
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

- (PDIObject *)trailerObject
{
    if (_trailerObject == nil) {
        PDObjectRef trailerObj = PDParserGetTrailerObject(_parser);
        _trailerObject = [[PDIObject alloc] initWithObject:trailerObj];
        [_trailerObject setSession:self];
        [_trailerObject markMutable];
    }
    return _trailerObject;
}

- (PDIObject *)verifiedInfoObject
{
    if (_infoObject || [self infoObject]) return _infoObject;
    
    _infoObject = [self appendObject];
    _parser->infoRef = PDRetain([[_infoObject reference] PDReference]);
    
    PDObjectRef trailer = _parser->trailer;
    PDIObject *trailerOb = [[PDIObject alloc] initWithObject:trailer];
    //[trailerOb setSession:self];
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
    NSAssert(objectID != 0, @"Zero is not a valid object ID");
    PDObjectRef obj = PDParserLocateAndCreateObject(_parser, objectID, true);
    PDIObject *object = [[PDIObject alloc] initWithObject:obj];
    PDRelease(obj);
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

- (PDIPage *)pageForPageNumber:(NSInteger)pageNumber
{
    if (pageNumber < 1 || pageNumber > [self numberOfPages]) {
        [NSException raise:@"PDInstanceBoundsException" format:@"The page number %ld is not within the bounds 1..%ld", (long)pageNumber, (long)[self numberOfPages]];
    }
    PDIPage *page = _pageDict[@(pageNumber)];
    if (page) return page;
    
    PDPageRef pageRef = PDPageCreateForPageWithNumber(_parser, pageNumber);
    page = [[PDIPage alloc] initWithPage:pageRef inSession:self];
    PDRelease(pageRef);
    
    _pageDict[@(pageNumber)] = page;
    
    return page;
}

- (PDIPage *)insertPage:(PDIPage *)page atPageNumber:(NSInteger)pageNumber
{
    PDPageRef nativePage = PDPageInsertIntoPipe(page.pageRef, _pipe, pageNumber);
    PDIPage *newPage = [[PDIPage alloc] initWithPage:nativePage inSession:self];

    NSMutableDictionary *newPageDict = [[NSMutableDictionary alloc] initWithCapacity:_pageDict.count + 1];
    for (NSNumber *n in _pageDict.allKeys) {
        NSNumber *m = (n.integerValue >= pageNumber) ? @(n.integerValue+1) : n;
        newPageDict[m] = _pageDict[n];
    }
    newPageDict[@(pageNumber)] = newPage;
    _pageDict = newPageDict;
    return newPage;
}

- (void)enqueuePropertyType:(PDPropertyType)type value:(NSInteger)value operation:(PDIObjectOperation)operation
{
    PDTaskRef filter, task;
    
    filter = PDTaskCreateFilterWithValue(type, value);
    
    __weak PDISession *bself = self;
    
    task = PDITaskCreateBlockMutator(^PDTaskResult(PDPipeRef pipe, PDTaskRef task, PDObjectRef object) {
        PDIObject *iob = [[PDIObject alloc] initWithObject:object];
        [iob markMutable];
        [iob setSession:bself];
        return operation(bself, iob);
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
    __weak PDISession *bself = self;

    PDPipeAddTask(_pipe, PDITaskCreateBlockMutator(^PDTaskResult(PDPipeRef pipe, PDTaskRef task, PDObjectRef object) {
        PDIObject *iob = [[PDIObject alloc] initWithObject:object];
        return operation(bself, iob);
    }));
}

- (void)setRootStream:(NSData *)data forKey:(NSString *)key
{
    if ([key isEqualToString:@"Metadata"]) {
        // We use the built-in verifiedMetadata method to do this; if we don't, we run the risk of double-tasking the metadata stream and overwriting requested changes.
        [self verifiedMetadataObject];
//        [_metadataObject setStreamIsEncrypted:NO];
        [_metadataObject setStreamContent:data encrypted:NO];
        return;
    }
    
    // got a Root?
    if ([self rootObject]) {
        PDIReference *ref = [_rootObject valueForKey:key];
        if (ref) {
            // got this already; we want to tweak it then
            if ([ref isKindOfClass:[PDIObject class]]) ref = [(PDIObject*)ref reference];
            [self forObjectWithID:[ref objectID] enqueueOperation:^PDTaskResult(PDISession *session, PDIObject *object) {
//                [object setStreamIsEncrypted:NO];
                [object setStreamContent:data encrypted:NO];
                return PDTaskDone;
            }];
        } else {
            // don't have the key; we want to add an object then
            PDIObject *ob = [self appendObject];
//            [ob setStreamIsEncrypted:NO];
            [ob setStreamContent:data encrypted:NO];
            
            [_rootObject enableMutationViaMimicSchedulingWithSession:self];
            [_rootObject setValue:ob forKey:key];
            
            /*[self forObjectWithID:_rootObject.objectID enqueueOperation:^PDTaskResult(PDISession *session, PDIObject *object) {
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

- (void)setupDocumentIDs
{
    _fetchedDocIDs = YES;
    PDDictionaryRef d = PDObjectGetDictionary(self.trailerObject.objectRef);
    void *idValue = PDDictionaryGetEntry(d, "ID");
    if (PDInstanceTypeArray == PDResolve(idValue)) {
        PDArrayRef a = idValue;
        {
            NSInteger count = PDArrayGetCount(a);
            _documentID = count > 0 ? [NSString stringWithUTF8String:PDStringHexValue(PDArrayGetElement(a, 0), false)] : nil;
            _documentInstanceID = count > 1 ? [NSString stringWithUTF8String:PDStringHexValue(PDArrayGetElement(a, 1), false)] : nil;
        }
//        pd_array_destroy(a);
    }
}

- (NSString *)documentID
{
    if (_documentID) return _documentID;
    if (! _fetchedDocIDs) [self setupDocumentIDs];
    return _documentID;
}

- (NSString *)documentInstanceID
{
    if (_documentInstanceID) return _documentInstanceID;
    if (! _fetchedDocIDs) [self setupDocumentIDs];
    return _documentInstanceID;
}

- (void)setDocumentID:(NSString *)documentID
{
    if (! _fetchedDocIDs) [self setupDocumentIDs];
    if (_documentInstanceID == nil) _documentInstanceID = documentID;
    _documentID = documentID;
    if (_documentID) [_trailerObject setValue:@[_documentID, _documentInstanceID] forKey:@"ID"];
}

- (void)setDocumentInstanceID:(NSString *)documentInstanceID
{
    if (! _fetchedDocIDs) [self setupDocumentIDs];
    if (_documentID == nil) _documentID = documentInstanceID;
    _documentInstanceID = documentInstanceID;
    if (_documentInstanceID) [_trailerObject setValue:@[_documentID, _documentInstanceID] forKey:@"ID"];
}

#ifdef PD_SUPPORT_CRYPTO

- (pd_crypto)cryptoObject
{
    return _parser->crypto;
}

#endif

@end
