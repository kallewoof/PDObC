//
// PDIXMPUtils.m
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

#import "PDIXMPUtils.h"

@implementation NSString (PDIXMPUtils)

static NSDictionary *entityDecodes = nil;

- (NSString *)stringByDecodingXMLEntities
{
    if (entityDecodes == nil) {
        entityDecodes = @{@"lt": @"<",
                         @"gt": @">",
                         @"amp": @"&",
                         @"apos": @"'",
                         @"quot": @"\""};
    }
    NSMutableArray *comps = [[self componentsSeparatedByString:@"&"] mutableCopy];
    // [the ], [apos;happy], [apos; tiger skipped]
    NSMutableString *res = [NSMutableString stringWithString:comps[0]];
    [comps removeObjectAtIndex:0];
    for (NSString *comp in comps) {
        NSMutableArray *entitySplit = [[comp componentsSeparatedByString:@";"] mutableCopy];
        if (entitySplit.count > 1) {
            NSString *entity = entitySplit[0];
            NSString *decoded = entityDecodes[entity];
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
