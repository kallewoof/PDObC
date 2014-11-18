//
// PDIObject.m
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
#import "NSObjects+PDIEntity.h"
#import "PDISession.h"
#import "PDIReference.h"
#import "PDIConversion.h"

#include "pd_internal.h"
#include "pd_stack.h"
#include "PDPipe.h"
#include "PDParser.h"
#include "PDDictionary.h"
#include "PDArray.h"
#include "PDString.h"
#include "PDNumber.h"

@interface PDIObject () {
    NSMutableDictionary *_dict;
    NSMutableArray *_arr;
    __weak PDISession *_instance;
    NSMutableSet *_syncHooks;
    PDObjectType _type;
    BOOL _mutable;
}

- (void)synchronize;

@property (nonatomic, readonly) PDObjectRef obj;

@end

void PDIObjectSynchronizer(void *parser, void *object, const void *syncInfo)
{
    PDIObject *ob = (__bridge PDIObject *)(syncInfo);
    [ob synchronize];
}

@implementation PDIObject

- (void)dealloc
{
    PDObjectSetSynchronizationCallback(_obj, NULL, NULL);
    PDRelease(_obj);
}

- (void)sharedSetup
{
    if (_objectID != 0) {
        PDObjectSetSynchronizationCallback(_obj, PDIObjectSynchronizer, (__bridge const void *)(self));
    }
        
    _dict = [[NSMutableDictionary alloc] initWithCapacity:3];
    _arr = [[NSMutableArray alloc] init];
    _type = PDObjectDetermineType(_obj);
}

- (id)initWithObject:(PDObjectRef)object
{
    self = [super init];
    if (self) {
        _syncHooks = [[NSMutableSet alloc] init];
        _obj = PDRetain(object);
        _objectID = PDObjectGetObID(_obj);
        _generationID = PDObjectGetGenID(_obj);
        [self sharedSetup];
    }
    return self;
}

- (id)initWithIsolatedDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID
{
    self = [super init];
    if (self) {
        _objectID = objectID;
        _generationID = generationID;
        _obj = PDObjectCreate(objectID, generationID);
        _obj->def = stack;
        [self sharedSetup];
    }
    return self;
}

- (id)initWithSession:(PDISession *)instance forDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID
{
    self = [self initWithIsolatedDefinitionStack:stack objectID:objectID generationID:generationID];
    if (self) {
        PDParserRef parser = PDPipeGetParser(instance.pipe);
        _obj->crypto = parser->crypto;
    }
    return self;
}

- (id)initWrappingValue:(id)value PDValue:(void *)PDValue
{
    PDObjectRef ob = PDObjectCreate(0, 0);
    PDObjectSetValue(ob, PDValue);
    ob->obclass = PDObjectClassCompressed;
    self = [self initWithObject:ob];
    if (self) {
        _mutable = YES;
        switch (_type) {
            case PDObjectTypeArray:
                _arr = value;
                break;
            case PDObjectTypeDictionary:
                _dict = value;
                break;
            default:
                PDNotice("wrapping value that is not a dictionary nor an array");
                break;
        }
    }
    PDRelease(ob);
    return self;
}

- (void)markMutable
{
    _mutable = YES;
}

- (void)synchronize
{
    for (void(^block)(PDIObject *) in _syncHooks) 
        block(self);
    [_syncHooks removeAllObjects];
}

- (void)addSynchronizeHook:(void (^)(PDIObject *))block
{
    if (! _instance)
        [NSException raise:@"PDIObjectImmutableException" format:@"The object (id %ld) is not mutable. Schedule mimicking or add a task to the mutable version of the object.", (long)_objectID];
    [_syncHooks addObject:[block copy]];
}

- (void)mimic:(PDIObject *)target
{
    if (_type != target.type) {
        _type = target.type;
    }
    
    PDInteger i;
    void *def;
    char *s;
    PDObjectRef tobj = target.obj;
    
    if (tobj->deleteObject) {
        // we're supposed to be deleted, and that's all we need to do, obviously
        [self removeObject];
        return;
    }
    
    switch (_type) {
        case PDObjectTypeInteger:
        case PDObjectTypeSize:
        case PDObjectTypeBoolean:
        case PDObjectTypeName:
        case PDObjectTypeReal:
        case PDObjectTypeReference:
        case PDObjectTypeString:
            PDObjectSetValue(_obj, PDObjectGetValue(tobj));
            break;
            
        case PDObjectTypeArray:
            [self replaceArrayWith:[target constructArray]];
            break;
            
        case PDObjectTypeDictionary:
            [self replaceDictionaryWith:[target constructDictionary]];
            break;
            
        default:
            // this is suspicious; what is this object anyway?
            PDNotice("suspicious object in mimic call: ob type = %u", _type);
            def = _obj->def;  _obj->def = tobj->def;   tobj->def = def;
            def = _obj->inst; _obj->inst = tobj->inst; tobj->inst = def;
    }
    
    _obj->skipStream = tobj->skipStream;
    _obj->encryptedDoc = tobj->encryptedDoc;
    
    if (tobj->ovrDef) {
        s = _obj->ovrDef; _obj->ovrDef = tobj->ovrDef; tobj->ovrDef = s;
    }
    if (tobj->ovrDefLen) {
        i = _obj->ovrDefLen; _obj->ovrDefLen = tobj->ovrDefLen; tobj->ovrDefLen = i;
    }
    if (tobj->ovrStream) {
        s = _obj->ovrStream; _obj->ovrStream = tobj->ovrStream; tobj->ovrStream = s;
        i = _obj->ovrStreamAlloc; _obj->ovrStreamAlloc = tobj->ovrStreamAlloc; tobj->ovrStreamAlloc = i;
    }
    if (tobj->ovrStreamLen) {
        i = _obj->ovrStreamLen; _obj->ovrStreamLen = tobj->ovrStreamLen; tobj->ovrStreamLen = i;
    }
}

- (BOOL)enableMutationViaMimicSchedulingWithSession:(PDISession *)instance
{
    if (_mutable) _instance = instance;
    if (_instance) return YES;
    
    [self setSession:instance];
    if (! _mutable && ! PDParserIsObjectStillMutable(PDPipeGetParser(instance.pipe), _objectID)) {
        PDWarn("object %ld has passed through pipe and can no longer be modified", (long)_objectID);
        return NO;
    }
        //[NSException raise:@"PDIObjectMutabilityImpossibleException" format:@"The object (id %ld) has already passed through the pipe and can no longer be modified.", (long)_objectID];
    return YES;
}

- (void)scheduleMimicking
{
    if (_mutable || ! _instance) return;
    _mutable = YES;
    [_instance forObjectWithID:_objectID enqueueOperation:^PDTaskResult(PDISession *session, PDIObject *object) {
        [object mimic:self];
        return PDTaskUnload;
    }];
}

- (void)scheduleMimicWithSession:(PDISession *)instance
{
    if (_mutable) return;
    [self enableMutationViaMimicSchedulingWithSession:instance];
    [self scheduleMimicking];
}

- (void)markInherentlyMutable
{
    _mutable = YES;
}

- (void)setSession:(PDISession *)instance
{
    _instance = instance;
}

- (const char *)PDFString
{
    if (NULL == _PDFString) {
        _PDFString = strdup([[NSString stringWithFormat:@"%ld %ld R", (long)_objectID, (long)_generationID] cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return _PDFString;
}

- (void *)PDValue
{
    return [PDIReference PDValueForObjectID:_objectID generationID:_generationID];
}

- (PDIReference *)reference
{
    return [[PDIReference alloc] initWithObjectID:_objectID generationID:_generationID];
}

- (BOOL)isInsideObjectStream
{
    return PDObjectGetObStreamFlag(_obj) == true;
}

- (void)removeObject
{
    PDObjectDelete(_obj);
    [self scheduleMimicking];
}

- (void)unremoveObject
{
    PDObjectUndelete(_obj);
}

- (BOOL)willBeRemoved
{
    return _obj->skipObject;
}

- (void)removeStream
{
    _obj->skipStream = true;
    [self scheduleMimicking];
}

- (BOOL)prepareStream
{
    if (_instance == nil) {
        // this may seem like a disturbing decision, but when an object without an instance is asked to allocate a stream, it simply means the object was created and the caller just blindly asked for the stream to be allocated -- both random-access obs and objects without streams say 'hasStream = NO'; this should probably change, however
        return NO;
    }
    
    PDParserRef parser = PDPipeGetParser([_instance pipe]);
    if (parser->obid == _objectID) {
        PDParserFetchCurrentObjectStream(parser, _objectID);
    } else {
        PDParserLocateAndFetchObjectStreamForObject(parser, _obj);
    }
    
    return _obj->extractedLen >= 0;
}

- (NSData *)allocStream
{
    if (! [self prepareStream]) {
        return nil;
    }
    
    return [[NSData alloc] initWithBytes:_obj->streamBuf length:_obj->extractedLen];
}

- (BOOL)isCurrentObject
{
    return (_instance && PDPipeGetParser([_instance pipe])->obid == _objectID);
}

- (void)setStreamContent:(NSData *)content encrypted:(BOOL)encrypted
{
    PDObjectSetStreamFiltered(_obj, (char *)[content bytes], [content length], encrypted);
    [self scheduleMimicking];
}

- (void)setStreamIsEncrypted:(BOOL)encrypted
{
    PDObjectSetStreamEncrypted(_obj, encrypted);
    [self scheduleMimicking];
}

- (id)objectValue
{
    return [PDIConversion fromPDType:PDObjectGetValue(_obj)];
}

- (void)setObjectValue:(id)value
{
    PDObjectSetValue(_obj, [value PDValue]);
    _type = _obj->type;
    if ([value isKindOfClass:[NSDictionary class]]) [_dict setDictionary:value];
    if ([value isKindOfClass:[NSArray class]]) [_arr setArray:value];
    [self scheduleMimicking];
}

- (id)valueForKey:(NSString *)key
{
    id value = _dict[key];
    if (! value) {
        void *pdv = PDDictionaryGetEntry(PDObjectGetDictionary(_obj), [key cStringUsingEncoding:NSUTF8StringEncoding]);
        if (pdv) {
            value = [PDIConversion fromPDType:pdv];
            _dict[key] = value;
        }
    }
    if ([value isKindOfClass:[PDIValue class]]) {
        value = [PDIConversion fromPDType:[(PDIValue*)value PDValue]];
        _dict[key] = value;
    }
    _type = _obj->type;
    return value;
}

- (id)resolvedValueForKey:(NSString *)key
{
    PDAssert(_instance); // crash: resolvedValueForKey: requires that an instance has been given to the PDIObject
    id value = [self valueForKey:key];
    if ([value isKindOfClass:[PDIReference class]]) {
        PDIObject *object = [_instance fetchReadonlyObjectWithID:[(PDIReference *)value objectID]];
        if (object) {
            _dict[key] = object;
            value = object;
        }
    }
    return value;
}

- (PDIObject *)objectForKey:(NSString *)key
{
    PDAssert(_instance);
    id ob = [self resolvedValueForKey:key];
    if (ob && ! [ob isKindOfClass:[PDIObject class]]) {
        // create fake object
            PDIObject *wrapOb = [[PDIObject alloc] initWrappingValue:ob PDValue:PDDictionaryGetEntry(_obj->inst, [key UTF8String])];
            return wrapOb;
    }
    return ob;
}

#ifdef PDI_DISALLOW_KEYPATHS
- (void)setValue:(id)value forKeyPath:(NSString *)keyPath
{
    PDError("Notice: setValue:forKeyPath: reroutes to setValue:forKey: -- if you require the key path functionality, remove the PDI_DISALLOW_KEYPATHS #define from PDIObject.h (chances are high that you only meant to write setValue:forKey:)");
    [self setValue:value forKey:keyPath];
}
#endif

- (void)setValue:(id)value forKey:(NSString *)key
{
    PDAssert(value != NULL);
    void *pdv;
    if ([value conformsToProtocol:@protocol(PDIEntity)]) {
        pdv = [value PDValue];
    } else {
        if (! [value isKindOfClass:[NSString class]]) {
            NSLog(@"Warning: %@ description is used.", value);
            // yes we do put the description into _dict as well, if we arrive here
            value = [value description];
        }
        pdv = [value PDValue];
    }
    PDDictionarySetEntry(PDObjectGetDictionary(_obj), [key PDFString], pdv);
    _dict[key] = value;
    _type = _obj->type;
    [self scheduleMimicking];
}

- (NSDictionary *)constructDictionary
{
    PDDictionaryRef dict = PDObjectGetDictionary(_obj);
    char **keys = PDDictionaryGetKeys(dict);
    PDInteger count = PDDictionaryGetCount(dict);
    
    [_dict resolvePDValues];
    for (PDInteger i = 0; i < count; i++) {
        [self valueForKey:[NSString stringWithPDFString:keys[i]]];
    }

    return _dict;
}

- (void)removeValueForKey:(NSString *)key
{
    PDDictionaryDeleteEntry(PDObjectGetDictionary(_obj), key.PDFString);
    [_dict removeObjectForKey:key];
    [self scheduleMimicking];
}

- (void)replaceDictionaryWith:(NSDictionary *)dict
{
    PDDictionaryClear(PDObjectGetDictionary(_obj));
    if (_dict) 
        [_dict removeAllObjects];
    else
        _dict = [[NSMutableDictionary alloc] initWithCapacity:dict.count];
    for (NSString *key in dict.allKeys) {
        [self setValue:dict[key] forKey:key];
    }
    [self scheduleMimicking];
}

- (void)replaceArrayWith:(NSArray *)array
{
    PDArrayClear(PDObjectGetArray(_obj));
    if (_arr)
        [_arr removeAllObjects];
    else 
        _arr = [[NSMutableArray alloc] initWithCapacity:array.count];
    for (id v in array) {
        [self appendValue:v];
    }
    [self scheduleMimicking];
}

- (void)setupArray
{
    PDInteger count = PDArrayGetCount(PDObjectGetArray(_obj));
    while (_arr.count < count) [_arr addObject:[NSNull null]];
    _type = _obj->type;
}

- (NSInteger)count
{
    if (_type == PDObjectTypeDictionary) {
        [self constructDictionary];
        return [_dict count];
    }
    if (_type == PDObjectTypeArray) {
        if (_arr.count == 0) [self setupArray];
        return [_arr count];
    }
    return 0;
}

- (id)valueAtIndex:(NSInteger)index
{
    if (_arr.count == 0) [self setupArray];
    if (index < 0 || index >= _arr.count) 
        [NSException raise:NSRangeException format:@"Index %ld is out of range (0..%ld)", (long)index, (long)_arr.count-1];
    id value = _arr[index];
    if (value == [NSNull null]) {
        void *pdv = PDArrayGetElement(PDObjectGetArray(_obj), index);
        if (pdv) {
            value = [PDIConversion fromPDType:pdv];
            _arr[index] = value;
        }
    }
    if ([value isKindOfClass:[PDIValue class]]) {
        value = [PDIConversion fromPDType:[(PDIValue*)value PDValue]];
        _arr[index] = value;
    }
    return value;
}

- (void)replaceValueAtIndex:(NSInteger)index withValue:(id)value
{
    PDAssert(value != NULL);
    void *pdv = NULL;
    if ([value conformsToProtocol:@protocol(PDIEntity)]) {
        pdv = [value PDValue];
    } else {
        if (! [value isKindOfClass:[NSString class]]) {
            if (! [value isKindOfClass:[NSNumber class]]) {
                NSLog(@"Warning: %@ description is used.", value);
            }
            // yes we do put the description into _dict as well, if we arrive here
            value = [value description];
        }
        pdv = [value PDValue];
    }
    PDArrayReplaceAtIndex(PDObjectGetArray(_obj), index, pdv);
    if (_arr.count == 0) [self setupArray];
    _arr[index] = value;
    [self scheduleMimicking];
}

- (void)removeValueAtIndex:(NSInteger)index
{
    if (_arr.count == 0) [self setupArray];
    if (index < 0 || index >= _arr.count) 
        [NSException raise:NSRangeException format:@"Index %ld is out of range (0..%ld)", (long)index, (long)_arr.count-1];
    PDArrayDeleteAtIndex(PDObjectGetArray(_obj), index);
    [_arr removeObjectAtIndex:index];
    [self scheduleMimicking];
}

- (void)appendValue:(id)value
{
    PDAssert(value != NULL);
    
    void *pdv = NULL;
    if ([value conformsToProtocol:@protocol(PDIEntity)]) {
        pdv = [value PDValue];
    } else {
        if (! [value isKindOfClass:[NSString class]]) {
            NSLog(@"Warning: %@ description is used.", value);
            // yes we do put the description into _dict as well, if we arrive here
            value = [value description];
        }
        pdv = [value PDValue];
    }
    
    if (_arr.count == 0) [self setupArray];
    PDArrayAppend(PDObjectGetArray(_obj), pdv);
    [_arr addObject:value];
    [self scheduleMimicking];
}

- (NSArray *)constructArray
{
    if (_arr.count == 0) [self setupArray];
    [_arr resolvePDValues];
    
    NSInteger count = _arr.count;
    for (NSInteger i = 0; i < count; i++) {
        if (_arr[i] == [NSNull null]) {
            [self valueAtIndex:i];
        }
    }
    return _arr;
}

#pragma mark - Extended properties

- (PDObjectType)type
{
    return _type;
}

- (void)setType:(PDObjectType)type
{
    _type = type;
    PDObjectSetType(_obj, type);
}

- (PDObjectRef)objectRef
{
    return _obj;
}

@end

@implementation PDIObject (PDIDeprecated)

- (id)initWithInstance:(PDISession *)instance forDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID
{
    return [self initWithSession:instance forDefinitionStack:stack objectID:objectID generationID:generationID];
}

- (BOOL)enableMutationViaMimicSchedulingWithInstance:(PDISession *)instance
{
    return [self enableMutationViaMimicSchedulingWithSession:instance];
}

- (void)scheduleMimicWithInstance:(PDISession *)instance
{
    [self scheduleMimicWithSession:instance];
}

@end
