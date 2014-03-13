//
//  PDIXMPUtils.m
//  Infinite PDF
//
//  Created by Karl-Johan Alm on 04/12/13.
//  Copyright (c) 2013 Alacrity Software. All rights reserved.
//

#import "PDIXMPUtils.h"

@implementation NSString (PDIXMPUtils)

static NSDictionary *entityDecodes = nil;

- (NSString *)stringByDecodingXMLEntities
{
    if (entityDecodes == nil) {
        entityDecodes = [[NSDictionary alloc] initWithObjectsAndKeys:
                         @"<", @"lt",
                         @">", @"gt",
                         @"&", @"amp",
                         @"'", @"apos",
                         @"\"", @"quot",
                         nil];
    }
    NSMutableArray *comps = [[self componentsSeparatedByString:@"&"] mutableCopy];
    // [the ], [apos;happy], [apos; tiger skipped]
    NSMutableString *res = [NSMutableString stringWithString:[comps objectAtIndex:0]];
    [comps removeObjectAtIndex:0];
    for (NSString *comp in comps) {
        NSMutableArray *entitySplit = [[comp componentsSeparatedByString:@";"] mutableCopy];
        if (entitySplit.count > 1) {
            NSString *entity = [entitySplit objectAtIndex:0];
            NSString *decoded = [entityDecodes objectForKey:entity];
            if (decoded == nil) {
                NSLog(@"warning: undecoded entity: %@", entity);
                decoded = [NSString stringWithFormat:@"&%@;", entity];
            }
            [entitySplit removeObjectAtIndex:0];
            [res appendFormat:@"%@%@", decoded, [entitySplit componentsJoinedByString:@";"]];
        }
    }
    return res;
}

- (NSString *)stringByEncodingXMLEntities
{
    NSMutableString *res = [NSMutableString string];
    
    NSUInteger ix = [self length];
    
    const unichar *buffer = CFStringGetCharactersPtr((CFStringRef)self);
	if (! buffer) {
		// We want this buffer to be autoreleased.
		NSMutableData *data = [NSMutableData dataWithLength:ix * sizeof(unichar)];
		if (! data) {
			return nil;
		}
		[self getCharacters:[data mutableBytes]];
		buffer = [data bytes];
    }
    
    NSRange range;
    NSString *rep;
    range = (NSRange){0,0};
    for (NSUInteger i = 0; i < ix; i++) {
        if (buffer[i] == '<') rep = @"&lt;";
        else if (buffer[i] == '>') rep = @"&gt;";
        else if (buffer[i] == '&') rep = @"&amp;";
        else if (buffer[i] == '\'') rep = @"&apos;";
        else if (buffer[i] == '"') rep = @"&quot;";
        else {
            rep = nil;
        }
        if (rep != nil) {
            [res appendFormat:@"%@%@", [self substringWithRange:(NSRange){range.location, range.length}], rep];
            range = (NSRange){range.location+range.length+1, -1};
        }
        range.length++;
    }
    return range.length > 0 ? [res stringByAppendingString:[self substringFromIndex:range.location]] : res;
}

- (NSString *)stringByEncodingXMLEntitiesAndNewlines
{
    NSMutableString *res = [NSMutableString string];
    
    NSUInteger ix = [self length];
    
    const unichar *buffer = CFStringGetCharactersPtr((CFStringRef)self);
	if (! buffer) {
		// We want this buffer to be autoreleased.
		NSMutableData *data = [NSMutableData dataWithLength:ix * sizeof(unichar)];
		if (! data) {
			return nil;
		}
		[self getCharacters:[data mutableBytes]];
		buffer = [data bytes];
    }
    
    NSRange range;
    NSString *rep;
    range = (NSRange){0,0};
    for (NSUInteger i = 0; i < ix; i++) {
        if (buffer[i] == '<') rep = @"&lt;";
        else if (buffer[i] == '>') rep = @"&gt;";
        else if (buffer[i] == '&') rep = @"&amp;";
        else if (buffer[i] == '\'') rep = @"&apos;";
        else if (buffer[i] == '"') rep = @"&quot;";
        else if (buffer[i] == '\n') rep = @"\\n";
        else {
            rep = nil;
        }
        if (rep != nil) {
            [res appendFormat:@"%@%@", [self substringWithRange:(NSRange){range.location, range.length}], rep];
            range = (NSRange){range.location+range.length+1, -1};
        }
        range.length++;
    }
    return range.length > 0 ? [res stringByAppendingString:[self substringFromIndex:range.location]] : res;
}

@end
