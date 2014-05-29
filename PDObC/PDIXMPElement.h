//
// PDIXMPElement.h
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

#import <Foundation/Foundation.h>
#import "PDIXMPEntry.h"

@interface PDIXMPElement : NSObject

- (id)initWithName:(NSString *)name attributes:(NSDictionary *)attributes parent:(PDIXMPElement *)parent;

- (void)appendChild:(PDIXMPElement *)child;

/**
 *  Append a value to the element. The value can be either an NSString or a PDIXMPEntry.
 *
 *  @param value NSString or PDIXMPEntry
 */
- (void)appendContent:(id)value;

/**
 *  Set the value of the element. The value can be either an NSString or a PDIXMPEntry.
 *
 *  @param value NSString or PDIXMPEntry
 */
- (void)setContent:(id)value;

- (void)setString:(NSString *)string forAttribute:(NSString *)attribute;

- (NSString *)stringOfAttribute:(NSString *)attribute;

- (PDIXMPElement *)find:(NSArray *)lineage createMissing:(BOOL)createMissing;

- (PDIXMPElement *)findChild:(NSString *)name withAttributes:(NSDictionary *)attributes;

- (NSArray *)findChildrenWithName:(NSString *)name;

- (void)removeFromParent;

- (void)removeChild:(PDIXMPElement *)child;

- (void)removeAllChildren;

- (NSString *)stringValue;

- (void)populateString:(NSMutableString *)string withIndent:(NSString *)indent;

@property (nonatomic, readonly) PDIXMPElement *parent;      ///< Parent element, or nil if the root object
@property (nonatomic, readonly) NSArray *children;          ///< PDIXMPElement objects which exist within this element

@property (nonatomic, strong)   NSString *name;             ///< @"rdf:li"
@property (nonatomic, readonly) NSString *value;            ///< @"John Doe"; note that value is ignored if firstChild is non-nil
@property (nonatomic, readonly) PDIXMPEntry *XMPValue;      ///< the value of the element, with encoding applied -- if value = @"<foo>", result will be @"&lt;foo&gt;"; if XMPValue = @"<foo>", result will be @"<foo>"
@property (nonatomic, strong)   NSDictionary *attributes;   ///< {@"xml:lang": @"x-default"}

@end
