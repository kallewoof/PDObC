//
//  PDInstance.h
//
//  Copyright (c) 2013 Karl-Johan Alm (http://github.com/kallewoof)
// 
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
// 
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
// 
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
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

// Block method for object operations.
typedef PDTaskResult (^PDIObjectOperation)(PDInstance *instance, PDIObject *object);

@interface PDInstance : NSObject

///---------------------------------------
/// @name Initializing a PDInstance object
///---------------------------------------

/**
 Sets up a new PD instance for a source PDF (read) and a destination PDF (updated version). 
 
 @param sourcePDFPath   The input PDF file. File must exist and be readable.
 @param destPDFPath     The output PDF file. Location must be read-writable.
 
 @warning Source and destination must not be the same.
 */
- (id)initWithSourcePDFPath:(NSString *)sourcePDFPath destinationPDFPath:(NSString *)destPDFPath;

///---------------------------------------
/// @name Adding new objects to a PDF
///---------------------------------------

/**
 Adds a new object to the PDF at the current position.
 
 @return PDIObject instance
 */
- (PDIObject *)insertObject;

/**
 Adds a new object to the PDF at the end (recommended if object value is changed repeatedly).
 
 @return PDIObject instance
 */
- (PDIObject *)appendObject;

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
 Reference to the PDF root object.
 */
@property (weak, nonatomic, readonly) PDIReference *rootReference;

/**
 Reference to the PDF info object.
 */
@property (weak, nonatomic, readonly) PDIReference *infoReference;

/**
 The source PDF path.
 */
@property (nonatomic, readonly) NSString *sourcePDFPath;

/**
 The destination PDF path.
 */
@property (nonatomic, readonly) NSString *destPDFPath;

/**
 The number of (live) objects seen in the input PDF. This property is undefined until execute has been called.
 */
@property (nonatomic, readonly) NSInteger objectSum;

/**
 The total number of objects in the input PDF. This is always valid.
 */
@property (nonatomic, readonly) NSInteger totalObjectCount;

@end
