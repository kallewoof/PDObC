//
//  NSArray+Sampling.h
//  Pajdeg
//
//  Created by Karl-Johan Alm on 26/07/14.
//  Copyright (c) 2014 Kalle Alm. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSArray (Sampling)

- (NSArray *)sample:(NSInteger)ratio;

@end
