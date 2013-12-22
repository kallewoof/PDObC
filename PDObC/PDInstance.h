//
// PDInstance.h
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

/**
 `PDInstance` is the main object for modifying PDF's in Objective-C via `Pajdeg`. It has a number of methods for performing common operations, and provides the means to do more fine grained operations via the operation enqueueing mechanism.
 
 ## PDF modification via Pajdeg
 
 `Pajdeg` is a C library for modifying existing PDF documents. The concept behind the library is somewhat similar to a stream. You set the in- and output, add tasks, then execute, at which point the library parses through the input PDF, triggers tasks as appropriate, and writes the results to the output destination. While this limits the flexibility, it means faster execution with a lower memory foot print.

 ## Minimal Usage Example
 
 The simplest example does nothing except pass an existing PDF through `Pajdeg` out to a new file. 
 
    PDInstance *instance = [[PDInstance alloc] initWithSourcePDFPath:@"/path/to/in.pdf" destinationPDFPath:@"/path/to/out.pdf"];
    [instance execute];
    [instance release];
 
 The execute method performs all enqueued tasks and frees up the internal `Pajdeg` object, and should be called at most once.
 
 ## Metadata Insertion / Replacement Example
 
 Adding metadata to a PDF can be done using the setRootStream:forKey: method directly. This is a convenience method which sets up tasks to perform the requested changes, including adding a `/Metadata` reference entry to the PDF root object. 
 
    PDInstance *instance = [[PDInstance alloc] initWithSourcePDFPath:@"/path/to/in.pdf" destinationPDFPath:@"/path/to/out.pdf"];
    [instance setRootStream:[@"Hello World" dataUsingEncoding:NSUTF8StringEncoding] forKey:@"Metadata"];
    [instance execute];
    [instance release];
 
 */

@class PDInstance;
@class PDIObject;
@class PDIReference;

#import "PDDefines.h"

/**
 Block method for object operations.
 
 Modifications to PDFs are exclusively done via PDIObjectOperation type blocks. Provided in the call are the instance and object references, and a PDTaskResult value must be returned (normally PDTaskDone).
 
 @param instance The instance reference for the session.
 @param object   One of the mutable objects associated with the operation.

 @return The task result, one of PDTaskDone, PDTaskFailure, PDTaskSkipRest, and PDTaskUnload. 
 */
typedef PDTaskResult (^PDIObjectOperation)(PDInstance *instance, PDIObject *object);

@interface PDInstance : NSObject

///---------------------------------------
/// @name Initializing a PDInstance object
///---------------------------------------

/**
 Sets up a new PD instance for a source PDF (read) and a destination PDF (updated version). 
 
 @param sourcePDFPath   The input PDF file. File must exist and be readable.
 @param destPDFPath     The output PDF file. Location must be read-writable.
 
 @note To perform read-only operations on a PDF, destPDFPath may be set to @"/dev/null"
 
 @warning Source and destination must not be the same.
 */
- (id)initWithSourcePDFPath:(NSString *)sourcePDFPath destinationPDFPath:(NSString *)destPDFPath;

///---------------------------------------
/// @name Document-wide operations
///---------------------------------------

/**
 Determine if the input PDF is encrypted or not. 
 */
@property (nonatomic, readonly) BOOL encrypted;

///---------------------------------------
/// @name Adding new objects to a PDF
///---------------------------------------

/**
 Adds a new object to the PDF at the current position.
 
 @return PDIObject instance
 */
- (PDIObject *)insertObject;

/**
 Adds a new object to the PDF at the end (recommended if object value is not defined on creation -- appended objects can be modified until the last existing object has been read from the input).
 
 @return PDIObject instance
 */
- (PDIObject *)appendObject;

///---------------------------------------
/// @name Pages and page objects
///---------------------------------------

/**
 Get the number of pages in the input PDF.
 */
- (NSInteger)numberOfPages;

/**
 Get the object ID for the page object with the given page number.
 
 @param pageNumber The page number of the object whose ID should be returned.
 */
- (NSInteger)objectIDForPageNumber:(NSInteger)pageNumber;

///---------------------------------------
/// @name Random access readonly instances 
///---------------------------------------

/**
 Fetch a read only copy of the object with the given ID.
 
 If modifications need to be made, the object's -scheduleMimicWithInstance: method must be called.

 @param objectID The ID of the object to fetch.
 */
- (PDIObject *)fetchReadonlyObjectWithID:(NSInteger)objectID;

///---------------------------------------
/// @name Modifying existing PDF objects
///---------------------------------------

/**
 Sets the stream for the given root object, creating a new one if necessary.

 These can be obtained via CoreGraphics through
 
    CGPDFDictionaryRef docDict = CGPDFDocumentGetCatalog(document);
    CGPDFStreamRef stream = 0;
    if (CGPDFDictionaryGetStream(docDict, "<key>", &stream)) {
        CGPDFDataFormat format = CGPDFDataFormatRaw;
        CFDataRef streamData = CGPDFStreamCopyData(stream, &format);
    }
 
 @param data The raw data. 
 @param key  The associated root key. This is usually "Metadata", but can be something else.
 
 @warning Data is explicitly declared to be *unencrypted* if the PDF has encryption enabled.
 
 @see [PDIObject setStreamIsEncrypted:]
 */
- (void)setRootStream:(NSData *)data forKey:(NSString *)key;

/**
 Enqueues a PDIObjectOperation for a specific object.
 
 The `PDIObjectOperation` is a block defined as
 
    typedef void (^PDIObjectOperation)(PDInstance *instance, PDIObject *object);
 
 @param objectID  The ID of the object in question. No generation ID is included, because `Pajdeg` does not trigger for deprecated objects.
 @param operation The `PDIObjectOperation` that should trigger. 
 
 @see PDIObject
 */
- (void)forObjectWithID:(NSInteger)objectID enqueueOperation:(PDIObjectOperation)operation;

/**
 Enqueue a PDIObjectOperation that is called for every single (live) object.
 
 @param operation The operation.
 */
- (void)enqueueOperation:(PDIObjectOperation)operation;

/**
 Executes the tasks required to perform the requested changes, and closes the stream. This is where all operation blocks will be executed.
 
 @return YES on success, NO on failure.
 
 @warning Calling this method more than once will trigger an assertion.
 */
- (BOOL)execute;

/**
 Obtain -- or create if necessary -- the /Metadata object, pointed to by the /Root object.
 */
- (PDIObject *)verifiedMetadataObject;

/**
 Reference to the PDF root object.
 */
@property (weak, nonatomic, readonly) PDIReference *rootReference;

/**
 Readonly representation of the root object.
 */
@property (weak, nonatomic, readonly) PDIObject *rootObject;

/**
 Reference to the PDF info object.
 */
@property (weak, nonatomic, readonly) PDIReference *infoReference;

/**
 Readonly representation of the info object.
 */
@property (weak, nonatomic, readonly) PDIObject *infoObject;

/**
 The source PDF path.
 */
@property (nonatomic, readonly) NSString *sourcePDFPath;

/**
 The destination PDF path.
 */
@property (nonatomic, readonly) NSString *destPDFPath;

/**
 The number of (live) objects seen in the input PDF. This property is undefined until -execute has been called.
 */
@property (nonatomic, readonly) NSInteger objectSum;

/**
 The total number of objects in the input PDF. This is always valid.
 */
@property (nonatomic, readonly) NSInteger totalObjectCount;

/**
 Direct access to pipe object.
 */
@property (nonatomic, readonly) PDPipeRef pipe;

@end
