//
//  NSArray+Sampling.m
//  Pajdeg
//
//  Created by Karl-Johan Alm on 26/07/14.
//  Copyright (c) 2014 Kalle Alm. All rights reserved.
//

#import "NSArray+Sampling.h"

@implementation NSArray (Sampling)

- (NSArray *)sample:(NSInteger)ratio
{
#if defined(DEBUG_SAMPLE_ALL)
    return self;
#else
    NSMutableArray *arr = [NSMutableArray arrayWithCapacity:(self.count + ratio) / ratio];
    for (id item in self) {
        if ((arc4random() % ratio) == 0) [arr addObject:item];
    }
    return arr;
#endif
}

@end
