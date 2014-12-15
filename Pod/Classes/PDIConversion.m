//
//  PDIConversion.m
//  Infinite PDF
//
//  Created by Karl-Johan Alm on 17/06/14.
//  Copyright (c) 2014 Alacrity Software. All rights reserved.
//

#import "NSObjects+PDIEntity.h"
#import "PDIConversion.h"
#import "Pajdeg.h"
#import "PDString.h"
#import "PDNumber.h"
#import "PDDictionary.h"
#import "PDArray.h"
#import "PDIObject.h"
#import "PDIReference.h"
#import "pd_internal.h"

@implementation PDIConversion

+ (id<PDIEntity>)fromPDType:(void *)pdob 
{
    return [self fromPDType:pdob depth:2];
}

+ (id<PDIEntity>)fromPDType:(void *)pdob depth:(NSInteger)depth
{
    if (depth == 0) {
        return [PDIValue valueWithPDValue:pdob];
    }
    
    switch (PDResolve(pdob)) {
        case PDInstanceTypeArray: return [self array:pdob depth:depth];
        case PDInstanceTypeDict:  return [self dict:pdob depth:depth];
        case PDInstanceTypeNumber:return [self number:pdob depth:depth];
        case PDInstanceTypeObj:   return [self obj:pdob depth:depth];
        case PDInstanceTypeRef:   return [self ref:pdob depth:depth];
        case PDInstanceTypeString:return [self string:pdob depth:depth];
        case PDInstanceTypeNull:  //return [NSNull null depth:depth];
        default:
            return nil;
    }
}

+ (id<PDIEntity>)array:(void *)pdob depth:(NSInteger)depth
{
    void *pdv;
    id<PDIEntity> v;
    PDInteger count = PDArrayGetCount(pdob);
    NSMutableArray *arr = [NSMutableArray arrayWithCapacity:count];
    for (PDInteger i = 0; i < count; i++) {
        pdv = PDArrayGetElement(pdob, i);
        v = [self fromPDType:pdv depth:depth-1];
        if (! v) v = [PDIValue valueWithPDValue:pdv];
        if (v)
            [arr addObject:v];
        else 
            PDWarn("NULL value in conversion for array element %ld in %s", i, PDArrayToString(pdob));
    }
    
    return arr;
}

typedef struct dictIteratorUI *dictIteratorUI;

struct dictIteratorUI {
    void *dict;
    NSInteger depth;
};

void dictIterator(char *key, void *value, void *userInfo, PDBool *shouldStop)
{
    dictIteratorUI diui = userInfo;
    id<PDIEntity> v = [PDIConversion fromPDType:value depth:diui->depth];
    if (! v) v = [PDIValue valueWithPDValue:value];
    if (v) {
        ((__bridge NSMutableDictionary *)diui->dict)[[NSString stringWithPDFString:key]] = v;
    } else {
        PDWarn("NULL value in conversion for dictionary entry %s", key);
    }
}

+ (id<PDIEntity>)dict:(void *)pdob depth:(NSInteger)depth
{
    PDInteger count = PDDictionaryGetCount(pdob);
    NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithCapacity:count];
    
    struct dictIteratorUI diui;
    diui.dict = (__bridge void*)dict;
    diui.depth = depth - 1;
    PDDictionaryIterate(pdob, dictIterator, &diui);
    
//    char **keys = PDDictionaryGetKeys(pdob);
//    for (PDInteger i = 0; i < count; i++) {
//        pdv = PDDictionaryGet(pdob, keys[i]);
//        v = [self fromPDType:pdv depth:depth-1];
//        if (! v) v = [PDIValue valueWithPDValue:pdv];
//        if (v) 
//            dict[[NSString stringWithPDFString:keys[i]]] = v;
//        else
//            PDWarn("NULL value in conversion for dictionary entry %s in %s", keys[i], PDDictionaryToString(pdob));
//    }
    
    return dict;
}

+ (id<PDIEntity>)number:(void *)pdob depth:(NSInteger)depth
{
    PDNumberRef n = pdob;
    switch (n->type) {
        case PDObjectTypeBoolean:   return [NSNumber numberWithBool:n->b];
        case PDObjectTypeReal:      return [NSNumber numberWithDouble:n->r];
        case PDObjectTypeSize:      return [NSNumber numberWithUnsignedInteger:n->s];
        case PDObjectTypeInteger:   return [NSNumber numberWithInteger:n->i];
        default:
            PDNotice("Uncaught obtype for PDNumber (%u); using %ld", n->type, n->i);
            return [NSNumber numberWithInteger:n->i];
    }
}

+ (id<PDIEntity>)obj:(void *)pdob depth:(NSInteger)depth
{
    return [[PDIObject alloc] initWithObject:pdob];
}

+ (id<PDIEntity>)ref:(void *)pdob depth:(NSInteger)depth
{
    return [[PDIReference alloc] initWithReference:pdob];
}

+ (id<PDIEntity>)string:(void *)pdob depth:(NSInteger)depth
{
    PDStringRef str = pdob;
    if (PDStringIsEncrypted(str)) {
        // we dont want to store encrypted strings as NSString objects; and besides, the encrypted flag will not be preserved so we can't, really
        str = PDAutorelease(PDStringCreateDecrypted(str));
    }
    return [NSString objectWithPDString:str];
}

@end

@implementation PDIValue : NSObject 

+ (PDIValue *)valueWithPDValue:(void *)pdv
{
    PDIValue *v = [[PDIValue alloc] init];
    v.PDValue = PDRetain(pdv);
    return v;
}

- (void)dealloc
{
    PDRelease(_PDValue);
}

- (const char *)PDFString
{
    return [[NSString stringWithFormat:@"<PDIValue:%p>", self.PDValue] PDFString];
}

//- (void *)PDValue
//{
//    return [self pointerValue];
//}

@end

@implementation NSArray (PDIConversion)

- (NSArray *)arrayByResolvingPDValues
{
    NSMutableArray *na = [NSMutableArray arrayWithCapacity:self.count];
    for (id v in self) {
        [na addObject:([v isKindOfClass:[PDIValue class]]
                       ? [PDIConversion fromPDType:[v PDValue]]
                       : v)];
    }
    return na;
}

@end

@implementation NSDictionary (PDIConversion)

- (NSDictionary *)dictionaryByResolvingPDValues
{
    NSMutableDictionary *nd = [NSMutableDictionary dictionaryWithCapacity:self.count];
    for (NSString *k in self.allKeys) {
        id v = self[k];
        nd[k] = ([v isKindOfClass:[PDIValue class]]
                 ? [PDIConversion fromPDType:[v PDValue]]
                 : v);
    }
    return nd;
}

@end

@implementation NSMutableArray (PDIConversion)

- (void)resolvePDValues
{
    NSInteger ix = self.count;
    for (NSInteger i = 0; i < ix; i++) {
        id v = self[i];
        if ([v isKindOfClass:[PDIValue class]]) 
            [self replaceObjectAtIndex:i withObject:[PDIConversion fromPDType:[v PDValue]]];
    }
}

@end

@implementation NSMutableDictionary (PDIConversion)

- (void)resolvePDValues
{
    NSArray *keys = self.allKeys.copy;
    NSInteger ix = self.count;
    for (NSInteger i = 0; i < ix; i++) {
        id v = self[keys[i]];
        if ([v isKindOfClass:[PDIValue class]]) 
            self[keys[i]] = [PDIConversion fromPDType:[v PDValue]];
    }
}

@end
