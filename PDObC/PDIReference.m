//
//  PDIReference.m
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

#import "PDIReference.h"
#import "pd_internal.h"

@interface PDIReference () {
    PDReferenceRef _ref;
}
@end

@implementation PDIReference

- (void)dealloc
{
    PDRelease(_ref);
}

- (id)initWithReference:(PDReferenceRef)reference
{
    self = [super init];
    if (self) {
        _ref = PDRetain(reference);
        _objectID = _ref->obid;
        _generationID = _ref->genid;
    }
    return self;
}

- (id)initWithObjectID:(NSInteger)objectID generationID:(NSInteger)generationID
{
    return [self initWithReference:PDReferenceCreate(objectID, generationID)];
}

- (id)initWithDefinitionStack:(pd_stack)stack
{
    return [self initWithReference:PDReferenceCreateFromStackDictEntry(stack)];
}

- (const char *)PDFString
{
    if (NULL == _PDFString) {
        _PDFString = strdup([[NSString stringWithFormat:@"%ld %ld R", (long)_objectID, (long)_generationID] cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return _PDFString;
}

@end
