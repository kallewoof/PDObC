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
#import "PDIXMPElement.h"
#import "Pajdeg.h"

#define kXMPAttributes  @" attributes "
#define kXMPContent     @" content "
#define kXMPElement     @" element "

@interface PDIXMPArchive () <NSXMLParserDelegate> {
    __strong NSData     *_data;
    BOOL                 _modified;
    
    PDIXMPElement       *_rootElement;
    PDIXMPElement       *_currentElement;
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
            //NSLog(@"about to parse XMP data:\n-------------------\n%@\n---------------------", str);
//#endif
            NSXMLParser *p = [[NSXMLParser alloc] initWithData:data];
            p.delegate = self;
            if (! [p parse]) {
                return nil;
            }
        } else {
            _rootElement = [[PDIXMPElement alloc] init];
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

- (NSData *)XMPData
{
    if (0 && ! _modified) {
        return _data;
    }
    if (! _modified) {
        NSLog(@"warning: generating xmp data even though modified is unset; disable when pdf code is stable and orderly");
    }
    
    _data = nil;
    
    NSMutableString *str = [[NSMutableString alloc] initWithCapacity:30000];
    
    // pre-fluff
    [str appendString:@"<?xpacket begin=\"\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n"];
    
    @autoreleasepool {
        for (PDIXMPElement *e in _rootElement.children)
            [e populateString:str withIndent:@""];
//        populateXMPString(str, _root);
    }
    
    // post-fluff
    [str appendString:@"<?xpacket end=\"r\"?>"];
    
    _data = [str dataUsingEncoding:NSUTF8StringEncoding];
    
    return _data;
}

- (NSData *)allocSubset
{
    NSMutableString *str = [[NSMutableString alloc] initWithCapacity:3000];
    
    @autoreleasepool {
        [_currentElement populateString:str withIndent:@""];
//        populateXMPString(str, _cursor);
    }
    
//    if (_detectMangledXML) {
//        NSUInteger len = str.length;
//        NSString *wrapperElement = [_cdict objectForKey:kXMPElement];
//        NSRange searchRange = (NSRange){0, str.length};
//        if (wrapperElement && str.length > 5 + wrapperElement.length * 2) 
//            searchRange = (NSRange){wrapperElement.length + 2, str.length - wrapperElement.length * 2 - 5};
//        if (len > 30 && [str rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"<>"] options:0 range:searchRange].location == NSNotFound) {
//            // suspicion: no angle brackets
//            @autoreleasepool {
//                if ([[str componentsSeparatedByString:@"&"] count] > 7) {
//                    // yeah this is mangled XML
//                    NSString *decodedString = [str stringByDecodingXMLEntities];
//                    str = (id)decodedString;
//                }
//            }
//        }
//    }
    
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
    return _currentElement;
}

// select the root path (i.e. cd /)
- (void)selectRoot
{
    _currentElement = _rootElement;
}

// select the parent of the current path (i.e. cd ..)
- (void)selectParent
{
    _currentElement = _currentElement.parent;
}

// delete the current element and select its parent
- (void)deleteElement
{
    PDIXMPElement *p = _currentElement.parent;
    [p removeChild:_currentElement];
    _currentElement = p;
}

- (BOOL)selectElement:(NSString *)element withAttributes:(NSDictionary *)attributes
{
    PDIXMPElement *e = [_currentElement findChild:element withAttributes:attributes];
    if (e) {
        _currentElement = e;
        return YES;
    }
    return NO;
}

// select element in current element (i.e. cd element) -- returns NO if the element does not exist
- (BOOL)selectElement:(NSString *)element
{
    return [self selectElement:element withAttributes:nil];
}

- (void)appendElement:(NSString *)element withAttributes:(NSDictionary *)attributes
{
    _modified = YES;
    _currentElement = [[PDIXMPElement alloc] initWithName:element attributes:attributes parent:_currentElement];
}

- (void)createElement:(NSString *)element withAttributes:(NSDictionary *)attributes
{
    if ([self selectElement:element withAttributes:attributes]) return;
    [self appendElement:element withAttributes:attributes];
}

// create and select element in current element (i.e. mkdir element) -- returns NO if the element already exists
- (void)createElement:(NSString *)element
{
    [self createElement:element withAttributes:nil];
}

// set/get attribute value
- (NSString *)stringForAttribute:(NSString *)attribute
{
    return _currentElement.attributes[attribute];
}

- (void)setString:(NSString *)string forAttribute:(NSString *)attribute
{
    _modified = YES;
    [_currentElement setString:string forAttribute:attribute];
}

// set/get content
- (NSString *)elementContent
{
    return _currentElement.value;
}

- (void)setElementContent:(id)content
{
    _modified = YES;
    [_currentElement setContent:content];
}

- (void)appendElementContent:(id)content
{
    _modified = YES;
    [_currentElement appendContent:content];
}

#pragma mark - NSXMLParser delegation

- (void)parserDidStartDocument:(NSXMLParser *)parser
{
    _currentElement = _rootElement = [[PDIXMPElement alloc] init];
}

- (void)parserDidEndDocument:(NSXMLParser *)parser
{
}

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict
{
    _currentElement = [[PDIXMPElement alloc] initWithName:elementName attributes:attributeDict parent:_currentElement];
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName
{
    _currentElement = _currentElement.parent;
}

- (void)parser:(NSXMLParser *)parser foundCharacters:(NSString *)string
{
    [_currentElement appendContent:string];
}

- (void)parser:(NSXMLParser *)parser parseErrorOccurred:(NSError *)parseError
{
    NSLog(@"parser parse error occurred: %@", [parseError description]);
}

- (void)parser:(NSXMLParser *)parser validationErrorOccurred:(NSError *)validationError
{
    NSLog(@"parser validation error occurred: %@", [validationError description]);
}

@end
