//
// PDISession.h
//
// Copyright (c) 2012 - 2015 Karl-Johan Alm (http://github.com/kallewoof)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#import <Foundation/Foundation.h>

/**
 `PDISession` is the main object for modifying PDF's in Objective-C via `Pajdeg`. It has a number of methods for performing common operations, and provides the means to do more fine grained operations via the operation enqueueing mechanism.
 
 ## PDF modification via Pajdeg
 
 `Pajdeg` is a C library for modifying existing PDF documents. The concept behind the library is somewhat similar to a stream. You set the in- and output, add tasks, then execute, at which point the library parses through the input PDF, triggers tasks as appropriate, and writes the results to the output destination. While this limits the flexibility, it means faster execution with a lower memory foot print.

 ## Minimal Usage Example
 
 The simplest example does nothing except pass an existing PDF through `Pajdeg` out to a new file. 
 
    PDISession *session = [[PDISession alloc] initWithSourcePDFPath:@"/path/to/in.pdf" destinationPDFPath:@"/path/to/out.pdf"];
    [session execute];
    [session release];
 
 The execute method performs all enqueued tasks and frees up the internal `Pajdeg` object, and should be called at most once.
 
 ## Metadata Insertion / Replacement Example
 
 Adding metadata to a PDF can be done using the setRootStream:forKey: method directly. This is a convenience method which sets up tasks to perform the requested changes, including adding a `/Metadata` reference entry to the PDF root object. 
 
    PDISession *session = [[PDISession alloc] initWithSourcePDFPath:@"/path/to/in.pdf" destinationPDFPath:@"/path/to/out.pdf"];
    [session setRootStream:[@"Hello World" dataUsingEncoding:NSUTF8StringEncoding] forKey:@"Metadata"];
    [session execute];
    [session release];
 
 */

@class PDISession;
@class PDIObject;
@class PDIReference;
@class PDIPage;

#import "PDDefines.h"

/**
 Block method for object operations.
 
 Modifications to PDFs are exclusively done via PDIObjectOperation type blocks. Provided in the call are the session and object references, and a PDTaskResult value must be returned (normally PDTaskDone).
 
 @param session The session reference for the session.
 @param object   One of the mutable objects associated with the operation.

 @return The task result, one of PDTaskDone, PDTaskFailure, PDTaskSkipRest, and PDTaskUnload. 
 */
typedef PDTaskResult (^PDIObjectOperation)(PDISession *session, PDIObject *object);

@interface PDISession : NSObject

///---------------------------------------
/// @name Initializing a PDISession object
///---------------------------------------

/**
 Sets up a new PD session for a source PDF (read) and a destination PDF (updated version). 
 
 @param sourcePDFPath   The input PDF file. File must exist and be readable.
 @param destPDFPath     The output PDF file. Location must be read-writable.
 
 @note To perform read-only operations on a PDF, destPDFPath may be set to @"/dev/null"
 
 @warning Source and destination must not be the same.
 */
- (id)initWithSourcePDFPath:(NSString *)sourcePDFPath destinationPDFPath:(NSString *)destPDFPath;

/**
 Sets up a new PD session for a source PDF as a URL and a destination PDF (updated version) as a file path. 
 
 @param sourceURL       The input PDF source URL, which must be a file URL. The file must exist and be readable.
 @param destPDFPath     The output PDF file. Location must be read-writable.
 
 @note To perform read-only operations on a PDF, destPDFPath may be set to @"/dev/null"
 
 @warning Source and destination must not be the same.
 */
- (id)initWithSourceURL:(NSURL *)sourceURL destinationPDFPath:(NSString *)destPDFPath;

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
 
 @return PDIObject session
 */
- (PDIObject *)insertObject;

/**
 Adds a new object to the PDF at the end (recommended if object value is not defined on creation -- appended objects can be modified until the last existing object has been read from the input).
 
 @return PDIObject session
 */
- (PDIObject *)appendObject;

///---------------------------------------
/// @name Pages and page objects
///---------------------------------------

/**
 Get the number of pages in the input PDF.
 */
- (NSUInteger)numberOfPages;

/**
 Get the object ID for the page object with the given page number.
 
 @param pageNumber The page number of the object whose ID should be returned.
 */
- (NSInteger)objectIDForPageNumber:(NSUInteger)pageNumber;

/**
 *  Get the page object for the page at the given page number. 
 *
 *  @note Page numbers begin at 1, not 0.
 *
 *  @param pageNumber The page number
 *
 *  @return The page object for the given page
 */
- (PDIPage *)pageForPageNumber:(NSUInteger)pageNumber;

/**
 *  Inserts the given page into the PDF document, so that it becomes the new page at pageNumber (and the old, and subsequent pages, become the pages of pageNumber + 1, ...).
 *
 *  @param page       The page to insert, which must not be native to the session (i.e. it must be form a separate PDISession)
 *  @param pageNumber Page number at which the page should be inserted in the PDF
 *
 *  @return The new native PDIPage object based on page
 */
- (PDIPage *)insertPage:(PDIPage *)page atPageNumber:(NSUInteger)pageNumber;

///---------------------------------------
/// @name Random access readonly sessions 
///---------------------------------------

/**
 Fetch a read only copy of the object with the given ID.
 
 If modifications need to be made, the object's -scheduleMimicWithSession: method must be called.

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
 
    typedef void (^PDIObjectOperation)(PDISession *session, PDIObject *object);
 
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
 
 @return Created or existing metadata object.
 */
- (PDIObject *)verifiedMetadataObject;

/**
 Obtain -- or create if necessary -- the /Info object of the trailer.

 @return Created or existing info object.
 */
- (PDIObject *)verifiedInfoObject;

/**
 Reference to the PDF root object.
 */
@property (nonatomic, readonly, strong) PDIReference *rootReference;

/**
 Readonly representation of the root object.
 */
@property (nonatomic, readonly, strong) PDIObject *rootObject;

/**
 Reference to the PDF info object or nil if there is none.
 */
@property (nonatomic, readonly, strong) PDIReference *infoReference;

/**
 Mutable representation of the trailer object.
 */
@property (nonatomic, readonly, strong) PDIObject *trailerObject;

/**
 Readonly representation of the info object, or nil if there is none.
 
 @see -verifiedInfoObject
 */
@property (nonatomic, readonly, strong) PDIObject *infoObject;

/**
 The source PDF path.
 */
@property (nonatomic, readonly, copy) NSString *sourcePDFPath;

/**
 The destination PDF path.
 */
@property (nonatomic, readonly, copy) NSString *destPDFPath;

/**
 The number of (live) objects seen in the input PDF. This property is undefined until -execute has been called.
 */
@property (nonatomic, readonly) NSInteger objectSum;

/**
 The total number of objects in the input PDF. This is always valid.
 */
@property (nonatomic, readonly) NSInteger totalObjectCount;

#ifdef PD_SUPPORT_CRYPTO

/**
 The cryptography object associated with the input PDF.
 */
@property (nonatomic, readonly) pd_crypto cryptoObject;

#endif

/**
 Direct access to pipe object.
 */
@property (nonatomic, readonly) PDPipeRef pipe;

/**
 *  16-byte (32-letter) HEX string uniquely (in theory) identifying this PDF document.
 *  The PDF specification suggests that this is set to a unique value and kept across all versions of the PDF, so that 
 *  references to the PDF may be able to verify that the given file is the desired document, even if the version of 
 *  the document has changed.
 *
 *  This string, along with the documentInstanceID, are stored as an array in the trailer dictionary, named /ID.
 *  It is also written into the XMP metadata archive (if any) under the xmpMM:DocumentID and xmpMM:InstanceID element
 *  names.
 *
 *  Setting the documentID for a nil-value documentID and/or documentInstanceID PDF will have the side effect of setting
 *  the documentInstanceID to the same value, according to the recommendation in the PDF spec.
 */
@property (nonatomic, copy) NSString *documentID;

/**
 *  16-byte (32-letter) HEX string uniquely (in theory) identifying this VERSION of this PDF document.
 *  The PDF specification suggests that this is updated for every version of the document, and the suggestion is
 *  to base the value on the MD5 value of the current time, the file's path, the file's size in bytes, and the value
 *  of all the entries in the file's document information (/Info) dictionary.
 */
@property (nonatomic, copy) NSString *documentInstanceID;

/**
 *  A dictionary for storing arbitrary information (e.g. in a category on PDISession).
 */
@property (nonatomic, readonly) NSMutableDictionary *sessionDict;

@end
