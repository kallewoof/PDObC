//
// PDIXMPEntry.m
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

#import "PDIXMPEntry.h"
#import "PDIXMPUtils.h"

@interface PDIXMPEntry () 

@property (nonatomic, strong) NSString *xmlString;

@end

@implementation PDIXMPEntry

+ (PDIXMPEntry *)entryWithXMLString:(NSString *)xmlString
{
    PDIXMPEntry *entry = [[PDIXMPEntry alloc] init];
    
    entry.xmlString = xmlString;
    
    return entry;
}

- (PDIXMPEntry *)entryByAppendingObject:(id)object
{
    PDIXMPEntry *entry = [[PDIXMPEntry alloc] init];
    if ([object isKindOfClass:[PDIXMPEntry class]]) {
        entry.xmlString = [_xmlString stringByAppendingString:object];
    } else {
        entry.xmlString = [_xmlString stringByAppendingString:[object stringByEncodingXMLEntities]];
    }
    return entry;
}

@end
