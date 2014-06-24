//
// PDIObject.m
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
#import "NSObjects+PDIEntity.h"
#import "PDInstance.h"
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
    //PDObjectRef _obj;
    NSMutableDictionary *_dict;
    NSMutableArray *_arr;
    __weak PDInstance *_instance;
    NSMutableSet *_syncHooks;
    PDObjectType _type;
    BOOL _mutable;
}

- (void)synchronize;

@property (nonatomic, readonly) PDObjectRef obj;

@end

//static long synx = 0;
//static long syncz = 0;

void PDIObjectSynchronizer(void *parser, void *object, const void *syncInfo)
{
//    synx++; NSLog(@"syncs: %ld / %ld", synx, syncz);
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
//        syncz++;
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

- (id)initWithInstance:(PDInstance *)instance forDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID
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
    }
    if (tobj->ovrStreamLen) {
        i = _obj->ovrStreamLen; _obj->ovrStreamLen = tobj->ovrStreamLen; tobj->ovrStreamLen = i;
    }
}

- (BOOL)enableMutationViaMimicSchedulingWithInstance:(PDInstance *)instance
{
    if (_mutable) _instance = instance;
    if (_instance) return YES;
    
    [self setInstance:instance];
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
    [_instance forObjectWithID:_objectID enqueueOperation:^PDTaskResult(PDInstance *instance, PDIObject *object) {
        [object mimic:self];
        return PDTaskUnload;
    }];
}

- (void)scheduleMimicWithInstance:(PDInstance *)instance
{
    if (_mutable) return;
    [self enableMutationViaMimicSchedulingWithInstance:instance];
    [self scheduleMimicking];
}

- (void)markInherentlyMutable
{
    _mutable = YES;
}

- (void)setInstance:(PDInstance *)instance
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

- (void)setStreamContent:(NSData *)content
{
    PDObjectSetStreamFiltered(_obj, (char *)[content bytes], [content length]);
//    PDObjectSetStream(_obj, (char *)[content bytes], [content length], true, false);
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
//    return [NSString stringWithPDFString:PDObjectGetValue(_obj)];
}

- (void)setObjectValue:(id)value
{
//    _type = PDObjectTypeString;
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
//        const char *vstr = PDDictionaryGetEntry(PDObjectGetDictionary(_obj), [key cStringUsingEncoding:NSUTF8StringEncoding]);
//        if (vstr) {
//            value = [NSString stringWithPDFString:vstr];
//            _dict[key] = value;
//        }
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
        PDIObject *object = [_instance fetchReadonlyObjectWithID:[value objectID]];
        if (object) {
            _dict[key] = object;
            value = object;
        }
    }
    
//    if ([value isKindOfClass:[NSString class]]) {
//        NSInteger obid = [PDIReference objectIDFromString:value];
//        if (obid) {
//            PDIObject *object = [_instance fetchReadonlyObjectWithID:obid];
//            if (object) {
//                _dict[key] = object;
//                value = object;
//            }
//        }
//    }
    
//    if (value && ! [value isKindOfClass:[NSString class]]) {
//        if ([value isKindOfClass:[PDIObject class]]) {
//            value = [value primitiveValue];
//        } else {
//            value = [NSString stringWithPDFString:[value PDFString]];
//        }
//    }

    return value;
}

- (PDIObject *)objectForKey:(NSString *)key
{
    PDAssert(_instance);
    id ob = [self resolvedValueForKey:key];
    if (ob && ! [ob isKindOfClass:[PDIObject class]]) {
        // create fake object
//        if (_obj->obclass == PDObjectClassCompressed) {
            PDIObject *wrapOb = [[PDIObject alloc] initWrappingValue:ob PDValue:PDDictionaryGetEntry(_obj->inst, [key UTF8String])];
            return wrapOb;
//        }
//        PDIObject *realOb = [_instance appendObject];
//        void *v = [ob PDValue];
//        switch (PDResolve(v)) {
//            case PDInstanceTypeArray:
//                for (id v in ob) {
//                    [realOb appendValue:v];
//                }
//                break;
//            case PDInstanceTypeDict:
//                for (id v in [ob allKeys]) {
//                    [realOb setValue:[ob objectForKey:v] forKey:v];
//                }
//                break;
//            default:
//                [realOb setObjectValue:ob];
////                realOb.obj->inst = PDRetain(v);
////                realOb.type = PDObjectDetermineType(realOb.obj);
//        }
////        realOb.obj->inst = [ob PDValue];
//        [self setValue:realOb forKey:key];
//        ob = realOb;
    }
    return ob;
//    // use resolvedValue to fetch the object, if necessary
//    [self resolvedValueForKey:key];
//    
//    PDDictionaryRef dict = PDObjectGetDictionary(_obj);
//    PDIObject *object = _dict[key];
//    if (object && ! [object isKindOfClass:[PDIObject class]]) {
//        const char *ckey = [key cStringUsingEncoding:NSUTF8StringEncoding];
//        // we probably have this value stored directly, hence we get a string back
//        pd_stack defs = PDObjectGetDictionaryEntryRaw(_obj, ckey);
//        if (defs) {
//            // that does seem right; we got a definition
//            object = [[PDIObject alloc] initWithIsolatedDefinitionStack:defs objectID:0 generationID:0];
//            
//            object.obj->obclass = PDObjectClassCompressed; // this isn't strictly the case, but it will result in no object identifying headers (e.g. 'trailer' or '1 2 obj') which is what we want
//            _dict[key] = object;
//            if (_instance) {
//                [object setInstance:_instance];
//                [object markInherentlyMutable];
//                
//                [self addSynchronizeHook:^(PDIObject *myself) {
//                    char *strdef = malloc(256);
//                    PDObjectGenerateDefinition(object.obj, &strdef, 256);
//                    [myself setValue:@(strdef) forKey:key];
//                    free(strdef);
//                }];
//            }
//        } else {
//            // hum; let's just pretend this object does not exist then
//            PDWarn("unexpected value in objectForKey:\"%s\"", ckey);
//            object = NULL;
//        }
//    }
//    
//    return object;
}

#ifdef PDI_DISALLOW_KEYPATHS
- (void)setValue:(id)value forKeyPath:(NSString *)keyPath
{
    NSLog(@"Notice: setValue:forKeyPath: reroutes to setValue:forKey: -- if you require the key path functionality, remove the PDI_DISALLOW_KEYPATHS #define from PDIObject.h (chances are high that you only meant to write setValue:forKey:)");
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
//    PDDictionarySetEntry(PDObjectGetDictionary(_obj), [key cStringUsingEncoding:NSUTF8StringEncoding], vstr);
    _dict[key] = value;
    _type = _obj->type;
    [self scheduleMimicking];
}

- (NSDictionary *)constructDictionary
{
    PDDictionaryRef dict = PDObjectGetDictionary(_obj);
    char **keys = PDDictionaryGetKeys(dict);
    PDInteger count = PDDictionaryGetCount(dict);
    
//    pd_dict dict = PDObjectGetDictionary(_obj);
//    const char **keys = pd_dict_keys(dict);
//    PDInteger count = pd_dict_get_count(dict);
    
    [_dict resolvePDValues];
    for (PDInteger i = 0; i < count; i++) {
        [self valueForKey:[NSString stringWithPDFString:keys[i]]];
    }

    return _dict;
}

- (void)removeValueForKey:(NSString *)key
{
    PDDictionaryDeleteEntry(PDObjectGetDictionary(_obj), key.PDFString);
//    PDDictionaryDeleteEntry(PDObjectGetDictionary(_obj), [key cStringUsingEncoding:NSUTF8StringEncoding]);
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
//    NSSet *keys = [NSSet setWithArray:[_dict allKeys]];
//    for (NSString *key in keys) 
//        [self removeValueForKey:key];
//    NSSet *keys = [NSSet setWithArray:[dict allKeys]];
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
//    if (_arr.count == 0) [self setupArray];
//    NSInteger ix = _arr.count < array.count ? _arr.count : array.count;
//    for (NSInteger i = 0; i < ix; i++) {
//        [self replaceValueAtIndex:i withValue:array[i]];
//    }
//    for (NSInteger i = ix; i < array.count; i++) {
//        [self appendValue:array[i]];
//    }
//    for (NSInteger i = _arr.count - 1; i > ix; i--) {
//        [self removeValueAtIndex:i];
//    }
//    for (NSInteger i = _arr.count-1; i >= 0; i--)
//        [self removeValueAtIndex:i];
//    for (id value in array) 
//        [self appendValue:value];
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
