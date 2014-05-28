//
// PDIXMPArchive.h
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

#import <Foundation/Foundation.h>

@class PDIObject;

// if this #define is enabled, detectMangledXML is ON by default, otherwise it is OFF by default
#define PDIXMPArchiveDetectMangledXML

@interface PDIXMPArchive : NSObject

/**
 Initialize an XMP archive instance with XMP content as data. 
 
 This can be data from a Core Graphics PDF instance or something else completely unrelated to Pajdeg.
 
 @param data XMP string as UTF-8 encoded character data.
 */
- (id)initWithData:(NSData *)data;

/**
 Initialize an XMP archive instance with the given PDIObject.
 
 THe object is probably the object pointed to by the /Metadata key in the PDF's /Root object's dictionary.
 
 @param object The PDIObject whose stream contains the XMP data.
 */
- (id)initWithObject:(PDIObject *)object;

/**
 Initialize XMP archive instance with the given CGPDFDocumentRef. 
 
 This is out of place, but the encryption implementation seems buggy at times, which means this is the proper
 fallback if the above method returns nil.
 */
- (id)initWithCGPDFDocument:(CGPDFDocumentRef)document;

/**
 Update the given object with the archive XMP data.
 
 This is identical to setting the object's stream to the results of -XMPData.
 
 @param object PDIObject to update.
 */
- (void)populateStreamInObject:(PDIObject *)object;

/**
 Generate XMP for this archive.
 
 @return NSData instance of UTF-8 encoded XMP representation of entire archive.
 */
- (NSData *)XMPData;

/**
 Obtain the current (see select operations below) element as a subset of the archive as XML data.
 
 @return NSData instance of UTF-8 encoded XMP representation of selected element and its sub-components.
 */
- (NSData *)allocSubset;

/**
 Select root, then the paths in order, and then alloc a subset.
 
 @param elements List of ordered elements to traverse before allocating subset.
 @return Allocated subset of the XML path /elements[0]/elements[1]/.../elements[elements.count-1].
 */
- (NSData *)allocSubsetForRootElementPath:(NSArray *)elements;

/**
 If set, attempts to detect and adjust mangled XML (&lt;xml&gt;blarf&lt;/xml&gt; -> <xml>blarf</xml>) in allocSubset* (slight performance loss).
 */
@property (nonatomic, readwrite) BOOL detectMangledXML;

///---------------------------------------
/// @name Archive manipulation
///---------------------------------------

/**
 Select the root path (i.e. cd /).
 */
- (void)selectRoot; 

/**
 Select the parent of the current path (i.e. cd ..).
 */
- (void)selectParent;

/**
 Delete the current element and select its parent.
 */
- (void)deleteElement;

/**
 Select element in current element (i.e. cd element) -- returns NO if the element does not exist.
 
 @param element The element to select.
 @return NO if the element does not exist; YES if it does.
 */
- (BOOL)selectElement:(NSString *)element;

/**
 *  Select the given path, that is call -selectElement: on each individual array entry until NO is returned or until the array is exhausted, and return NO in the former case and YES in the latter.
 *
 *  @param path Array of element names
 *
 *  @return YES if all elements in path existed and were successfully selected
 */
- (BOOL)selectPath:(NSArray *)path;

/**
 *  Find all elements of the given type and return them as an array. 
 *
 *  @see NSArray+PDIXMPArchive.h
 *
 *  @param element Element name that should be matched
 *
 *  @return Array of elements whose name is equal to the given element
 */
- (NSArray *)findElements:(NSString *)element;

/**
 Select first element of given type with given attributes.
 
 @param element The element to select.
 @param attributes The attributes to match.
 @return NO if the element with the given attributes does not exist; YES if it does.
 */
- (BOOL)selectElement:(NSString *)element withAttributes:(NSDictionary *)attributes;

/**
 Create and select element in current element (i.e. mkdir element).
 
 Works like selecting elements if the element to create already exists.
 
 @param element Element to create. 
 */
- (void)createElement:(NSString *)element;

/**
 Create and select element with given attributes.
 
 If an element of the given type exists with the given attributes, it is selected, otherwise a new element with the given attributes is created.
 
 @param element The element to create
 @param attributes The attributes to match
 */
- (void)createElement:(NSString *)element withAttributes:(NSDictionary *)attributes;

/**
 *  Create and select element with given attributes, even if another identical element already exists.
 *
 *  Even if an element of the given type exists with the given attributes, a new element is created.
 *
 *  @param element    The element to create
 *  @param attributes The attributes to set in the element
 */
- (void)appendElement:(NSString *)element withAttributes:(NSDictionary *)attributes;

/**
 Get attribute value as a string.
 
 @param attribute Name of attribute to look up.
 @return String value of attribute.
 */
- (NSString *)stringForAttribute:(NSString *)attribute;

/**
 Set attribute value.
 
 @param string Updated attribute value, as a string.
 @param attribute Name of attribute.
 */
- (void)setString:(NSString *)string forAttribute:(NSString *)attribute;

/**
 Get current element's content.
 */
- (NSString *)elementContent;

/**
 *  Get the content of the element at the given path, starting at the current element
 *
 *  @param path Path as array of element names
 *
 *  @return String value or nil if path is not defined in the archive
 */
- (NSString *)contentOfElementAtPath:(NSArray *)path;

/**
 Set element content.
 
 If a string is used, it is eventually encoded; if a PDIXMPEntry is used, it is expected to be XML encoded already.
 */
- (void)setElementContent:(id)content;

/**
 Append to element content.
 
 If a string is used, it is eventually encoded; if a PDIXMPEntry is used, it is expected to be XML encoded already.
 */
- (void)appendElementContent:(id)content;

/**
 Get reference to current position in archive. 
 
 This is used for debugging purposes to verify that traversal went as it should. 
 */
- (id)cursorReference;

@end
