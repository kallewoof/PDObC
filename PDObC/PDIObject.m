//
// PDIObject.m
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
#import "NSObjects+PDIEntity.h"
#import "PDInstance.h"

#include "pd_internal.h"
#include "pd_stack.h"
#include "PDPipe.h"
#include "PDParser.h"

@interface PDIObject () {
    //PDObjectRef _obj;
    NSMutableDictionary *_dict;
    NSMutableArray *_arr;
    __weak PDInstance *_instance;
    NSMutableSet *_syncHooks;
    BOOL _mutable;
}

- (void)synchronize;

@property (nonatomic, readonly) PDObjectRef obj;

@end

void PDIObjectSynchronizer(void *parser, void *object, const void *syncInfo)
{
    PDIObject *ob = CFBridgingRelease(syncInfo);
    [ob synchronize];
}

@implementation PDIObject

- (void)dealloc
{
    PDRelease(_obj);
}

- (void)sharedInit
{
    PDObjectSetSynchronizationCallback(_obj, PDIObjectSynchronizer, CFBridgingRetain(self));
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
        [self sharedInit];
    }
    return self;
}

- (id)initWithDefinitionStack:(pd_stack)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID
{
    self = [super init];
    if (self) {
        _objectID = objectID;
        _generationID = generationID;
        _obj = PDObjectCreate(objectID, generationID);
        _obj->def = stack;
        [self sharedInit];
    }
    return self;
}

- (void)markMutable
{
    _mutable = YES;
}

- (void)synchronize
{
    for (dispatch_block_t block in _syncHooks) 
        block();
    [_syncHooks removeAllObjects];
}

- (void)addSynchronizeHook:(dispatch_block_t)block
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
    pd_stack def;
    char *s;
    PDObjectRef tobj = target.obj;
    
    if (tobj->deleteObject) {
        // we're supposed to be deleted, and that's all we need to do, obviously
        [self removeObject];
        return;
    }
    
    switch (_type) {
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
            // this is suspicious
            def = _obj->def;        _obj->def = tobj->def;              tobj->def = def;
            def = _obj->mutations;  _obj->mutations = tobj->mutations;  tobj->mutations = def;
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
}

- (void)enableMutationViaMimicSchedulingWithInstance:(PDInstance *)instance
{
    if (_instance || _mutable) return;
    if (! PDParserIsObjectStillMutable(PDPipeGetParser(instance.pipe), _objectID)) {
#ifdef DEBUG
        fprintf(stderr, "warning: object %ld has passed through pipe and can no longer be modified\n", (long)_objectID);
#endif
        return;
    }
        //[NSException raise:@"PDIObjectMutabilityImpossibleException" format:@"The object (id %ld) has already passed through the pipe and can no longer be modified.", (long)_objectID];
    [self setInstance:instance];
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

- (BOOL)isInsideObjectStream
{
    return PDObjectGetObStreamFlag(_obj) == true;
}

- (void)removeObject
{
    PDObjectDelete(_obj);
    [self scheduleMimicking];
}

- (void)removeStream
{
    _obj->skipStream = true;
    [self scheduleMimicking];
}

- (NSData *)allocStream
{
    if (_instance == nil) [NSException raise:@"PDInvalidObjectOperation" format:@"The object is not mutable, or is not a part of the original PDF."];
    char *bytes = PDParserFetchCurrentObjectStream(PDPipeGetParser([_instance pipe]), _objectID);
    return [[NSData alloc] initWithBytes:bytes length:_obj->extractedLen];
}

- (void)setStreamContent:(NSData *)content
{
    PDObjectSetStream(_obj, (char *)[content bytes], [content length], true, false);
    [self scheduleMimicking];
}

- (void)setStreamIsEncrypted:(BOOL)encrypted
{
    PDObjectSetStreamEncrypted(_obj, encrypted);
    [self scheduleMimicking];
}

- (NSString *)primitiveValue
{
    if (_type != PDObjectTypeString) 
        return nil;
    return [NSString stringWithPDFString:PDObjectGetValue(_obj)];
}

- (void)setPrimitiveValue:(NSString *)value
{
    _type = PDObjectTypeString;
    PDObjectSetValue(_obj, [value PDFString]);
    [self scheduleMimicking];
}

- (id)valueForKey:(NSString *)key
{
    id value = [_dict objectForKey:key];
    if (! value) {
        const char *vstr = PDObjectGetDictionaryEntry(_obj, [key cStringUsingEncoding:NSUTF8StringEncoding]);
        if (vstr) {
            value = [NSString stringWithPDFString:vstr];
            [_dict setObject:value forKey:key];
        }
    }
    _type = _obj->type;
    return value;
}

- (void)setValue:(id)value forKey:(NSString *)key
{
    PDAssert(value != NULL);
    const char *vstr = NULL;
    if ([value conformsToProtocol:@protocol(PDIEntity)]) {
        vstr = [value PDFString];
    } else {
        if (! [value isKindOfClass:[NSString class]]) {
            NSLog(@"Warning: %@ description is used.", value);
            // yes we do put the description into _dict as well, if we arrive here
            value = [value description];
        }
        // below should be vstr = [value PDFString], no?
        vstr = [value cStringUsingEncoding:NSUTF8StringEncoding];
    }
    PDObjectSetDictionaryEntry(_obj, [key cStringUsingEncoding:NSUTF8StringEncoding], vstr);
    [_dict setObject:value forKey:key];
    _type = _obj->type;
    [self scheduleMimicking];
}

- (NSDictionary *)constructDictionary
{
    /*if (_type != PDObjectTypeDictionary) {
        return nil;
    }*/
    
    pd_stack iter = _obj->def;
    char *key;
    char *value;
    while (pd_stack_get_next_dict_key(&iter, &key, &value)) {
        [self setValue:[NSString stringWithPDFString:value]
                forKey:[NSString stringWithPDFString:key]];
        free(value);
    }
    return _dict;
}

- (void)removeValueForKey:(NSString *)key
{
    PDObjectRemoveDictionaryEntry(_obj, [key cStringUsingEncoding:NSUTF8StringEncoding]);
    [_dict removeObjectForKey:key];
    [self scheduleMimicking];
}

- (void)replaceDictionaryWith:(NSDictionary *)dict
{
    NSSet *keys = [NSSet setWithArray:[_dict allKeys]];
    for (NSString *key in keys) 
        [self removeValueForKey:key];
    keys = [NSSet setWithArray:[dict allKeys]];
    for (NSString *key in keys) {
        [self setValue:[dict objectForKey:key] forKey:key];
    }
    [self scheduleMimicking];
}

- (void)replaceArrayWith:(NSArray *)array
{
    if (_arr.count == 0) [self setupArray];
    NSInteger ix = _arr.count < array.count ? _arr.count : array.count;
    for (NSInteger i = 0; i < ix; i++) {
        [self replaceValueAtIndex:i withValue:[array objectAtIndex:i]];
    }
    for (NSInteger i = ix; i < array.count; i++) {
        [self appendValue:[array objectAtIndex:i]];
    }
    for (NSInteger i = _arr.count - 1; i > ix; i--) {
        [self removeValueAtIndex:i];
    }
    for (NSInteger i = _arr.count-1; i >= 0; i--)
        [self removeValueAtIndex:i];
    for (id value in array) 
        [self appendValue:value];
    [self scheduleMimicking];
}

- (void)setupArray
{
    PDInteger count = PDObjectGetArrayCount(_obj);
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
    return -1;
}

- (id)valueAtIndex:(NSInteger)index
{
    if (_arr.count == 0) [self setupArray];
    if (index < 0 || index >= _arr.count) 
        [NSException raise:NSRangeException format:@"Index %ld is out of range (0..%ld)", (long)index, (long)_arr.count-1];
    id value = [_arr objectAtIndex:index];
    if (value == [NSNull null]) {
        const char *vstr = PDObjectGetArrayElementAtIndex(_obj, index);
        if (vstr) {
            value = [NSString stringWithPDFString:vstr];
            [_arr replaceObjectAtIndex:index withObject:value];
        }
    }
    return value;
}

- (void)replaceValueAtIndex:(NSInteger)index withValue:(id)value
{
    PDAssert(value != NULL);
    const char *vstr = NULL;
    if ([value conformsToProtocol:@protocol(PDIEntity)]) {
        vstr = [value PDFString];
    } else {
        if (! [value isKindOfClass:[NSString class]]) {
            NSLog(@"Warning: %@ description is used.", value);
            // yes we do put the description into _dict as well, if we arrive here
            value = [value description];
        }
        vstr = [value cStringUsingEncoding:NSUTF8StringEncoding];
    }
    PDObjectSetArrayElement(_obj, index, vstr);
    if (_arr.count == 0) [self setupArray];
    [_arr replaceObjectAtIndex:index withObject:value];
    [self scheduleMimicking];
}

- (void)removeValueAtIndex:(NSInteger)index
{
    if (_arr.count == 0) [self setupArray];
    if (index < 0 || index >= _arr.count) 
        [NSException raise:NSRangeException format:@"Index %ld is out of range (0..%ld)", (long)index, (long)_arr.count-1];
    PDObjectRemoveArrayElementAtIndex(_obj, index);
    [_arr removeObjectAtIndex:index];
    [self scheduleMimicking];
}

- (void)appendValue:(id)value
{
    PDAssert(value != NULL);
    const char *vstr = NULL;
    if ([value conformsToProtocol:@protocol(PDIEntity)]) {
        vstr = [value PDFString];
    } else {
        if (! [value isKindOfClass:[NSString class]]) {
            NSLog(@"Warning: %@ description is used.", value);
            // yes we do put the description into _dict as well, if we arrive here
            value = [value description];
        }
        vstr = [value cStringUsingEncoding:NSUTF8StringEncoding];
    }
    if (_arr.count == 0) [self setupArray];
    PDObjectAddArrayElement(_obj, vstr);
    [_arr addObject:value];
    [self scheduleMimicking];
}

- (NSArray *)constructArray
{
    /*if (_type != PDObjectTypeArray) {
        return nil;
    }*/
    
    if (_arr.count == 0) [self setupArray];
    
    NSInteger count = _arr.count;
    for (NSInteger i = 0; i < count; i++) {
        if ([_arr objectAtIndex:i] == [NSNull null]) {
            [self valueAtIndex:i];
        }
    }
    return _arr;
}

@end
