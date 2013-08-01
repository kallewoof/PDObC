//
//  PDIObject.m
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

#import "PDIObject.h"
#import "PDInternal.h"
#import "PDStack.h"
#import "NSObjects+PDIEntity.h"

@interface PDIObject () {
    PDObjectRef _obj;
    NSMutableDictionary *_dict;
}

@end

@implementation PDIObject

- (void)dealloc
{
    PDRelease(_obj);
}

- (id)initWithObject:(PDObjectRef)object
{
    self = [super init];
    if (self) {
        _obj = PDRetain(object);
        _objectID = PDObjectGetObID(_obj);
        _generationID = PDObjectGetGenID(_obj);
        _type = PDObjectGetType(_obj);
        _dict = [[NSMutableDictionary alloc] initWithCapacity:3];
    }
    return self;
}

- (id)initWithDefinitionStack:(PDStackRef)stack objectID:(NSInteger)objectID generationID:(NSInteger)generationID
{
    self = [super init];
    if (self) {
        _objectID = objectID;
        _generationID = generationID;
        _obj = PDObjectCreate(objectID, generationID);
        _obj->def = stack;
        _dict = [[NSMutableDictionary alloc] initWithCapacity:3];
    }
    return self;
}

- (const char *)PDFString
{
    if (NULL == _PDFString) {
        _PDFString = strdup([[NSString stringWithFormat:@"%ld %ld R", (long)_objectID, (long)_generationID] cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return _PDFString;
}

- (void)removeObject
{
    _obj->skipObject = true;
}

- (void)removeStream
{
    _obj->skipStream = true;
}

- (void)setStreamContent:(NSData *)content
{
    PDObjectSetStream(_obj, (char *)[content bytes], [content length], true);
}

- (void)setStreamIsEncrypted:(BOOL)encrypted
{
    PDObjectSetStreamEncrypted(_obj, encrypted);
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
    PDObjectSetDictionaryEntry(_obj, [key cStringUsingEncoding:NSUTF8StringEncoding], vstr);
    [_dict setObject:value forKey:key];
    _type = _obj->type;
}

- (NSString *)primitiveValue
{
    if (_type != PDObjectTypeString) 
        return nil;
    return [NSString stringWithPDFString:PDObjectGetValue(_obj)];
}

- (NSDictionary *)constructDictionary
{
    if (_type != PDObjectTypeDictionary) {
        return nil;
    }
    
    PDStackRef iter = _obj->def;
    char *key;
    char *value;
    while (PDStackGetNextDictKey(&iter, &key, &value)) {
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
}

@end
