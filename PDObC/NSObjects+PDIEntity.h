//
//  NSObjects+PDIEntity.h
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
#import "PDIEntity.h"

/**
 `NSDictionary` category implementing the PDIEntity protocol. The implementation returns the NSDictionary entries wrapped in <<>>, each key prefixed with a forward slash, and each value resolved either through its `PDIEntity` implementation (if conformant) or description.
 */

@interface NSDictionary (PDIEntity) <PDIEntity>

@end

/**
 `NSArray` category implementing the PDIEntity protocol. The implementation returns the NSArray entries wrapped in [], each entry resolved either through its `PDIEntity` implementation (if conformant) or description.
 */

@interface NSArray (PDIEntity) <PDIEntity>

@end

