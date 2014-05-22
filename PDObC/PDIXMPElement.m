//
// PDIXMPElement.m
//
// Copyright (c) 2014 Karl-Johan Alm (http://github.com/kallewoof)
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

#import "PDIXMPElement.h"
#import "PDIXMPUtils.h"

static inline NSString *NSStringFromXMPAttributesDict(NSDictionary *attrs)
{
    NSMutableString *str = [NSMutableString stringWithString:@""];
    for (NSString *key in attrs) {
        [str appendFormat:@" %@=\"%@\"", key, [[attrs objectForKey:key] stringByEncodingXMLEntitiesAndNewlines]];
    }
    return str;
}

@interface PDIXMPElement () {
    NSMutableString *_value;
    PDIXMPEntry *_XMPValue;
    NSMutableArray *_children;
    NSMutableDictionary *_attributes;
}

@property (nonatomic, assign) PDIXMPElement *parent;

@end

@implementation PDIXMPElement

- (id)initWithName:(NSString *)name attributes:(NSDictionary *)attributes parent:(PDIXMPElement *)parent
{
    self = [super init];
    if (self) {
        _name = name;
        _attributes = attributes ? [attributes mutableCopy] : [[NSMutableDictionary alloc] init];
        if (parent) [parent appendChild:self];
    }
    return self;
}

- (void)appendChild:(PDIXMPElement *)child
{
    if (! _children) _children = [[NSMutableArray alloc] init];
    [_children addObject:child];
    child.parent = self;
}

- (void)appendContent:(id)value
{
    if (! _value) {
        _value = [[NSMutableString alloc] init];
    }

    if ([value isKindOfClass:[NSString class]]) {
        [_value appendString:value];
    } else {
        [_value appendString:[[value xmlString] stringByDecodingXMLEntities]];
        if (! _XMPValue) {
            _XMPValue = [PDIXMPEntry entryWithXMLString:[_value stringByEncodingXMLEntities]];
        }
    }
    _XMPValue = [_XMPValue entryByAppendingObject:value];
}

- (void)setContent:(id)value
{
    if ([value isKindOfClass:[PDIXMPEntry class]]) 
        [self setXMPValue:value];
    else
        [self setValue:value];
}

- (NSString *)value
{
    return _value;
}

- (PDIXMPEntry *)XMPValue
{
    if (_XMPValue == nil && _value != nil) _XMPValue = [PDIXMPEntry entryWithXMLString:[_value stringByEncodingXMLEntities]];
    return _XMPValue;
}

- (void)setValue:(NSString *)value
{
    if (! _value) _value = [[NSMutableString alloc] init];
    [_value setString:value];
    _XMPValue = [PDIXMPEntry entryWithXMLString:[_value stringByEncodingXMLEntities]];
}

- (void)setXMPValue:(PDIXMPEntry *)XMPValue
{
    _XMPValue = XMPValue;
    [_value setString:[XMPValue.xmlString stringByDecodingXMLEntities]];
}

- (void)setString:(NSString *)string forAttribute:(NSString *)attribute
{
    _attributes[attribute] = string;
}

- (PDIXMPElement *)find:(NSArray *)lineage createMissing:(BOOL)createMissing
{
    PDIXMPElement *e = self;
    PDIXMPElement *c;
    for (NSString *cname in lineage) {
        c = [e findChild:cname withAttributes:nil];
        if (c) {
            e = c;
        } else if (createMissing) {
            e = [[PDIXMPElement alloc] initWithName:cname attributes:nil parent:e];
        } else {
            return nil;
        }
    }
    return e;
}

- (PDIXMPElement *)findChild:(NSString *)name withAttributes:(NSDictionary *)attributes
{
    for (PDIXMPElement *e in _children) {
        if ([e.name isEqualToString:name]) {
            BOOL proceed = YES;
            NSDictionary *eattr = e.attributes;
            for (NSString *key in attributes.allKeys) {
                if (! [eattr[key] isEqual:attributes[key]]) {
                    proceed = NO;
                    break;
                }
            }
            if (proceed) {
                return e;
            }
        }
    }
    return nil;
}

- (void)removeChild:(PDIXMPElement *)child
{
    if (! [_children containsObject:child]) 
        [NSException raise:@"PDIXMPElementChildNotFoundException" format:@"![_children containsObject:child]"];
    
    [_children removeObject:child];
}

- (void)removeAllChildren
{
    _children = nil;
}

- (NSString *)stringValue
{
    NSMutableString *s = [NSMutableString stringWithString:@""];
    [self populateString:s withIndent:@""];
    return s;
}

- (void)populateString:(NSMutableString *)string withIndent:(NSString *)indent
{
    if (_children.count) {
        // this is a node with children
        [string appendFormat:@"%@<%@%@>\n", indent, _name, NSStringFromXMPAttributesDict(_attributes)];
        NSString *cindent = [indent stringByAppendingString:@"\t"];
        for (PDIXMPElement *c in _children) {
            [c populateString:string withIndent:cindent];
        }
        [string appendFormat:@"%@<\%@>\n", indent, _name];
    } else if (self.XMPValue) {
        // this is a term node with content
        [string appendFormat:@"%@<%@%@>%@</%@>\n", indent, _name, NSStringFromXMPAttributesDict(_attributes), [_XMPValue.xmlString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]], _name];
    } else {
        // this is a term node w/o content
        [string appendFormat:@"%@<%@%@ />\n", indent, _name, NSStringFromXMPAttributesDict(_attributes)];
    }
}


@end