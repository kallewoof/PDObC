//
// PDIXMPArchive.m
//
// Copyright (c) 2013 Karl-Johan Alm (http://github.com/kallewoof)
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

#import "PDIObject.h"
#import "PDIXMPArchive.h"
#import "PDIXMPEntry.h"
#import "PDIXMPUtils.h"
#import "Pajdeg.h"

#define kXMPAttributes  @" attributes "
#define kXMPContent     @" content "
#define kXMPElement     @" element "

@interface PDIXMPArchive () <NSXMLParserDelegate> {
    __strong NSData     *_data;
    BOOL                 _modified;
    NSMutableArray      *_root;
    NSMutableArray      *_target;
    NSMutableArray      *_targets;
    
    // iteration (manipulation) 
    NSMutableArray      *_cursors;
    NSMutableArray      *_cursor;
    NSMutableDictionary *_cdict;
    NSMutableDictionary *_cattrs;
}

@end

@implementation PDIXMPArchive

- (id)initWithData:(NSData *)data
{
    self = [super init];
    if (self) {
        _modified = NO;
#ifdef PDIXMPArchiveDetectMangledXML
        _detectMangledXML = YES;
#endif
        
        if (data) {
//#ifdef DEBUG
            //NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            //BriefLog(@"about to parse XMP data:\n-------------------\n%@\n---------------------", str);
//#endif
            NSXMLParser *p = [[NSXMLParser alloc] initWithData:data];
            p.delegate = self;
            if (! [p parse]) {
                return nil;
            }
        } else {
            _root = [[NSMutableArray alloc] init];
        }
        
        [self selectRoot];
        [self createElement:@"x:xmpmeta"];
        [self setString:@"adobe:ns:meta/" forAttribute:@"xmlns:x"];
        [self setString:@"Pajdeg " PAJDEG_VERSION forAttribute:@"x:xmptk"];
        [self createElement:@"rdf:RDF" withAttributes:@{@"xmlns:rdf": @"http://www.w3.org/1999/02/22-rdf-syntax-ns#"}];
        [self selectRoot];
        
        _modified = data != nil;
        _data = data;
    }
    return self;
}

- (id)initWithObject:(PDIObject *)object
{
    return [self initWithData:[object allocStream]];
}

- (id)initWithCGPDFDocument:(CGPDFDocumentRef)doc
{
    CFDataRef result = 0;
    
    CGPDFDictionaryRef docDict = CGPDFDocumentGetCatalog(doc);
    CGPDFStreamRef metastream = 0;
    
    if (CGPDFDictionaryGetStream(docDict, "Metadata", &metastream)) {
        CGPDFDataFormat format = CGPDFDataFormatRaw;
        CFDataRef streamData = CGPDFStreamCopyData(metastream, &format);
        if (streamData) {
            if (format == CGPDFDataFormatRaw) {
                result = streamData;
            } else {
                CFRelease(streamData);
            }
        }
    }
    
    if (! result) return nil;
    
    return [self initWithData:CFBridgingRelease(result)];
}

- (void)populateStreamInObject:(PDIObject *)object
{
    [object setStreamContent:[self XMPData]];
}

#pragma mark - XML generation

static inline NSString *NSStringFromXMPAttributesDict(NSDictionary *attrs)
{
    NSMutableString *str = [NSMutableString stringWithString:@""];
    for (NSString *key in attrs) {
        [str appendFormat:@" %@=\"%@\"", key, [[attrs objectForKey:key] stringByEncodingXMLEntitiesAndNewlines]];
    }
    return str;
}

static inline void populateXMPString(NSMutableString *str, NSArray *element)
{
    NSString *ename = nil;
    for (id item in element) {
        if ([item isKindOfClass:[NSDictionary class]]) {
            // element definition
            NSDictionary *attr = [item objectForKey:kXMPAttributes];
            ename = [item objectForKey:kXMPElement];
            [str appendFormat:@"<%@%@>", ename, NSStringFromXMPAttributesDict(attr)];
        }
        else if ([item isKindOfClass:[NSArray class]]) {
            // sub element
            populateXMPString(str, item);
        }
        else if ([item isKindOfClass:[NSString class]]) {
            // content
            [str appendString:[item stringByEncodingXMLEntities]];
        }
        else if ([item isKindOfClass:[PDIXMPEntry class]]) {
            // pre-encoded content
            [str appendString:[item xmlString]];
        }
        else {
            // what is this?
            BriefLog(@"error: undefined class for populateXMPString()");
            assert(0);
        }
    }
    if (ename) {
        [str appendFormat:@"</%@>", ename];
    }
}

- (NSData *)XMPData
{
    if (0 && ! _modified) {
        return _data;
    }
    if (! _modified) {
        BriefLog(@"warning: generating xmp data even though modified is unset; disable when pdf code is stable and orderly");
    }
    
    _data = nil;
    
    NSMutableString *str = [[NSMutableString alloc] initWithCapacity:30000];
    
    // pre-fluff
    [str appendString:@"<?xpacket begin=\"\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n"];
    
    @autoreleasepool {
        populateXMPString(str, _root);
    }
    
    // post-fluff
    [str appendString:@"<?xpacket end=\"r\"?>\n"];
    
    _data = [str dataUsingEncoding:NSUTF8StringEncoding];
    
    return _data;
}

- (NSData *)allocSubset
{
    NSMutableString *str = [[NSMutableString alloc] initWithCapacity:3000];
    
    @autoreleasepool {
        populateXMPString(str, _cursor);
    }
    
    if (_detectMangledXML) {
        NSUInteger len = str.length;
        NSString *wrapperElement = [_cdict objectForKey:kXMPElement];
        NSRange searchRange = (NSRange){0, str.length};
        if (wrapperElement && str.length > 5 + wrapperElement.length * 2) 
            searchRange = (NSRange){wrapperElement.length + 2, str.length - wrapperElement.length * 2 - 5};
        if (len > 30 && [str rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"<>"] options:0 range:searchRange].location == NSNotFound) {
            // suspicion: no angle brackets
            @autoreleasepool {
                if ([[str componentsSeparatedByString:@"&"] count] > 7) {
                    // yeah this is mangled XML
                    NSString *decodedString = [str stringByDecodingXMLEntities];
                    str = (id)decodedString;
                }
            }
        }
    }
    
    NSData *data = [str dataUsingEncoding:NSUTF8StringEncoding];
    return data;
}

- (NSData *)allocSubsetForRootElementPath:(NSArray *)elements
{
    [self selectRoot];
    for (NSString *el in elements) 
        if (! [self selectElement:el]) return nil;
    return [self allocSubset];
}

#pragma mark - Archive manipulation

- (id)cursorReference
{
    return _cursor;
}

// select the root path (i.e. cd /)
- (void)selectRoot
{
    if (! _cursors) _cursors = [[NSMutableArray alloc] initWithCapacity:3]; else [_cursors removeAllObjects];
    _cursor = _root;
    _cdict = nil;
    _cattrs = nil;
}

// select the parent of the current path (i.e. cd ..)
- (void)selectParent
{
    _cursor = [_cursors lastObject];
    [_cursors removeLastObject];
    if (_cursor == _root) {
        _cdict = nil;
        _cattrs = nil;
    } else {
        _cdict = [_cursor objectAtIndex:0];
        _cattrs = [_cdict objectForKey:kXMPAttributes];
    }
}

// delete the current element and select its parent
- (void)deleteElement
{
    NSArray *toDelete = _cursor;
    [self selectParent];
    [_cursor removeObject:toDelete];
}

- (BOOL)selectElement:(NSString *)element withAttributes:(NSDictionary *)attributes
{
    NSMutableDictionary *d;
    for (id item in _cursor) {
        if ([item isKindOfClass:[NSArray class]] && [item count] > 0) {
            d = [item objectAtIndex:0];
            if ([d isKindOfClass:[NSDictionary class]] && [[d objectForKey:kXMPElement] isEqualToString:element]) {
                BOOL applies = YES;
                NSDictionary *attrs = [d objectForKey:kXMPAttributes];
                if (attributes) {
                    for (NSString *key in attributes) {
                        if (! [[attrs objectForKey:key] isEqualToString:[attributes objectForKey:key]]) {
                            applies = NO;
                            break;
                        }
                    }
                }
                
                if (! applies) continue;
                
                if (! [d respondsToSelector:@selector(setObject:forKey:)]) {
                    d = [d mutableCopy];
                    [item replaceObjectAtIndex:0 withObject:d];
                }
                [_cursors addObject:_cursor];
                _cursor = item;
                _cdict = d;
                _cattrs = [attrs mutableCopy];
                [d setObject:_cattrs forKey:kXMPAttributes];
                return YES;
            }
        }
    }
    return NO;
}

// select element in current element (i.e. cd element) -- returns NO if the element does not exist
- (BOOL)selectElement:(NSString *)element
{
    return [self selectElement:element withAttributes:nil];
/*    NSMutableDictionary *d;
    for (id item in _cursor) {
        if ([item isKindOfClass:[NSArray class]] && [item count] > 0) {
            d = [item objectAtIndex:0];
            if ([d isKindOfClass:[NSDictionary class]] && [[d objectForKey:kXMPElement] isEqualToString:element]) {
                if (! [d respondsToSelector:@selector(setObject:forKey:)]) {
                    d = [d mutableCopy];
                    [item replaceObjectAtIndex:0 withObject:d];
                }
                [_cursors addObject:_cursor];
                _cursor = item;
                _cdict = d;
                _cattrs = [[d objectForKey:kXMPAttributes] mutableCopy];
                [d setObject:_cattrs forKey:kXMPAttributes];
                return YES;
            }
        }
    }
    return NO;*/
}

- (void)createElement:(NSString *)element withAttributes:(NSDictionary *)attributes
{
    if ([self selectElement:element withAttributes:attributes]) return;
    _modified = YES;
    
    [_cursor addObject:@"\t"];
    
    NSMutableArray *el = [[NSMutableArray alloc] init];
    
    NSMutableDictionary *a = attributes ? [attributes mutableCopy] : [[NSMutableDictionary alloc] init];
    NSMutableDictionary *d = [@{
                                kXMPAttributes : a, 
                                kXMPElement : element
                                } mutableCopy];
    [el addObject:d];
    
    [_cursor addObject:el];
    [_cursor addObject:@"\n"];

    [_cursors addObject:_cursor];
    _cursor = el;
    _cdict = d;
    _cattrs = a;
}

// create and select element in current element (i.e. mkdir element) -- returns NO if the element already exists
- (void)createElement:(NSString *)element
{
    [self createElement:element withAttributes:nil];
    /*if ([self selectElement:element]) return;
    _modified = YES;
    
    [_cursor addObject:@"\t"];
    
    NSMutableArray *el = [[NSMutableArray alloc] init];
    
    NSMutableDictionary *a = [[NSMutableDictionary alloc] init];
    NSMutableDictionary *d = [@{
                                kXMPAttributes : a, 
                                kXMPElement : element
                                } mutableCopy];
    [el addObject:d];
    
    [_cursor addObject:el];
    [_cursor addObject:@"\n"];
    _cursor = el;
    _cdict = d;
    _cattrs = a;*/
}

// set/get attribute value
- (NSString *)stringForAttribute:(NSString *)attribute
{
    return [_cattrs objectForKey:attribute];
}

- (void)setString:(NSString *)string forAttribute:(NSString *)attribute
{
    _modified = YES;
    [_cattrs setObject:string forKey:attribute];
}

// set/get content
- (NSString *)elementContent
{
    NSMutableString *str = [NSMutableString string];
    
    // we iterate over the element until we run into a non-string (aside from the first dict)
    for (id item in _cursor) {
        if ([item isKindOfClass:[NSString class]]) 
            [str appendString:item];
        else if (! [item isKindOfClass:[NSDictionary class]]) 
            return str;
    }
    
    return str;
}

- (void)setElementContent:(id)content
{
    _modified = YES;
    
    // iterate and delete all strings
    for (NSInteger i = _cursor.count-1; i >= 0; i--) {
        if ([[_cursor objectAtIndex:i] isKindOfClass:[NSString class]]) [_cursor removeObjectAtIndex:i];
    }
    [_cursor addObject:content];
}

- (void)appendElementContent:(id)content
{
    _modified = YES;
    [_cursor addObject:content];
}

#pragma mark - XML parse helper methods

- (void)pushTarget:(NSMutableArray *)target
{
    [_targets addObject:_target];
    _target = target;
}

- (NSMutableArray *)popTarget
{
    _target = [_targets lastObject];
    [_targets removeLastObject];
    return _target;
}

#pragma mark - NSXMLParser delegation

- (void)parserDidStartDocument:(NSXMLParser *)parser
{
    _targets = [[NSMutableArray alloc] initWithCapacity:5];
    _root = [[NSMutableArray alloc] initWithCapacity:5];
    _target = _root;
}

- (void)parserDidEndDocument:(NSXMLParser *)parser
{
    _targets = nil;
    _target = nil;
}

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict
{
    NSMutableArray *el = [[NSMutableArray alloc] init];
    [el addObject:@{
                    kXMPAttributes : attributeDict, 
                    kXMPElement : elementName
                    }];
    [_target addObject:el];
    [self pushTarget:el];
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName
{
    [self popTarget];
}

- (void)parser:(NSXMLParser *)parser foundCharacters:(NSString *)string
{
    [_target addObject:string];
}

- (void)parser:(NSXMLParser *)parser parseErrorOccurred:(NSError *)parseError
{
    BriefLog(@"parser parse error occurred: %@", [parseError description]);
}

- (void)parser:(NSXMLParser *)parser validationErrorOccurred:(NSError *)validationError
{
    BriefLog(@"parser validation error occurred: %@", [validationError description]);
}

@end
