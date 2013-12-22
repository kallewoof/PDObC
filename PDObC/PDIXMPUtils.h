//
//  PDIXMPUtils.h
//  Infinite PDF
//
//  Created by Karl-Johan Alm on 04/12/13.
//  Copyright (c) 2013 Alacrity Software. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (PDIXMPUtils)

- (NSString *)stringByDecodingXMLEntities;
- (NSString *)stringByEncodingXMLEntities;
- (NSString *)stringByEncodingXMLEntitiesAndNewlines;

@end

