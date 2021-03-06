//
// PDStreamFilterPrediction.h
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

/**
 @file PDStreamFilterPrediction.h
 
 @ingroup PDSTREAMFILTERPREDICTION

 @defgroup PDSTREAMFILTERPREDICTION PDStreamFilterPrediction
 
 @brief Prediction filter
 
 @ingroup PDINTERNAL
 
 @implements PDSTREAMFILTER

 @{
 */

#ifndef INCLUDED_PDStreamFilterPrediction_h
#define INCLUDED_PDStreamFilterPrediction_h

#include "PDStreamFilter.h"

/**
 Set up a stream filter for prediction.
 */
//extern PDStreamFilterRef PDStreamFilterPredictionCreate(void);

/**
 Set up stream filter for unprediction.
 */
extern PDStreamFilterRef PDStreamFilterUnpredictionCreate(PDDictionaryRef options);
/**
 Set up a stream filter for prediction based on inputEnd boolean. 
 */
extern PDStreamFilterRef PDStreamFilterPredictionConstructor(PDBool inputEnd, PDDictionaryRef options);

#endif

/** @} */

