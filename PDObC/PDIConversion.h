//
//  PDIConversion.h
//  Infinite PDF
//
//  Created by Karl-Johan Alm on 17/06/14.
//  Copyright (c) 2014 Alacrity Software. All rights reserved.
//

#import "PDIEntity.h"

@interface PDIConversion : NSObject

/**
 *  Generate a objective-c object from a PD value. The resulting type can be an NSString, NSNumber, PDIObject, PDIReference, NSArray, NSDictionary, or PDIValue.
 *
 *  @warning pdob must be a proper PD object type in the PDInstanceType list; a lot of objects are proper PD objects which do not support conversion. The objects that do support conversion are: PDString, PDNumber, PDArray, PDDictionary, PDObject, and PDReference. Any other PDRetainable value passed in will return nil. Any non-PDRetainable value passed in will result in a crash.
 *
 *  @note Depth caps recursion. Arrays and dictionaries will only resolve values down by max depth steps. A depth of 1 means an array will be filled with PDIValues. Depth 2 means an array will have all its values set right, but any arrays or dictionaries within the array will be filled with PDIValues. Existing arrays and dictionaries with PDIValues can be resolved using -resolvePDValues.
 *
 *  @param pdob  A PDString, PDNumber, PDArray, PDDictionary, PDObject, or PDReference
 *  @param depth Number of recursions allowed
 *
 *  @return An NSString, PDIName, NSNumber, NSArray, NSDictionary, PDIObject, PDIReference, PDIValue, or nil
 */
+ (id<PDIEntity>)fromPDType:(void *)pdob depth:(NSInteger)depth;

/**
 *  Short-hand for depth = 2
 */
+ (id<PDIEntity>)fromPDType:(void *)pdob;

@end

@interface PDIValue : NSObject <PDIEntity>

+ (PDIValue *)valueWithPDValue:(void *)pdv;

//- (void *)PDValue;

@property (nonatomic, readwrite) void *PDValue;

@end

@interface NSArray (PDIConversion) 

- (NSArray *)arrayByResolvingPDValues;

@end

@interface NSDictionary (PDIConversion)

- (NSDictionary *)dictionaryByResolvingPDValues;

@end

@interface NSMutableArray (PDIConversion)

- (void)resolvePDValues;

@end

@interface NSMutableDictionary (PDIConversion)

- (void)resolvePDValues;

@end
