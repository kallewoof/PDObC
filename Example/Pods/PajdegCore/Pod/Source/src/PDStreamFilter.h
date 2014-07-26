//
// PDStreamFilter.h
//
// Copyright (c) 2012 - 2014 Karl-Johan Alm (http://github.com/kallewoof)
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
 @file PDStreamFilter.h
 
 @ingroup PDSTREAMFILTER

 @defgroup PDSTREAMFILTER PDStreamFilter
 
 @brief The stream filter interface.
 
 @ingroup PDINTERNAL
 
 Stream filters are used to read or write content that is filtered in some way, such as FlateDecode streams (compressed).
 
 @section filter_reg Globally registered filters
 
 Filters can be registered globally using PDStreamFilterRegisterDualFilter() and be accessed from anywhere via PDStreamFilterObtain(). 
 
 By default, if PD_SUPPORT_ZLIB is defined, the first call to pd_pdf_implementation_use() will permanently register two filters, "FlateDecode" and "Predictor". 
 
 Registered filters can be overridden by registering a filter with the same name as an already existing one -- the most recently registered filter always takes precedence.
 
 @section filter_chains Chaining filters together
 
 Filters can be chained together by linking them to each other via the nextFilter property. 
 
 Some filters will hook child filters up automatically if they are passed options that they recognize. 
 
 The FlateDecode (compression) filter will add a Predictor filter to itself, if initialized with options that include the key "Predictor". 
 
 This goes both ways: 
 
 - if a FlateDecode compression filter is created with Predictor options, it will replace itself with a Predictor filter and attach a FlateDecode filter replacing itself to the Predictor
 - if a FlateDecode decompression filter is created with Predictor options, it will add an Unpredictor filter to itself.
 
 Chained filters are transparent to the caller, in the sense that the first filter in the chain holds the output buffer and capacity for the whole filter chain. 
 
 @note Chained filters are an extension mechanism enabled via PDStreamFilterBegin() and PDStreamFilterProceed(). Calling a filter's begin or proceed function pointers directly will only execute that filter.
 
 @section filter_dual Dual filters and inversion
 
 Currently all filters are, but need not be, dual. This means all filters come in pairs complementing each other, so that if data is passed into one and through the other, presuming the options are identical for both, the data remains unmodified. 
 
 FlateDecode has the compression (inflate) and decompression (deflate) filters, and Predictor has the predict and unpredict filters.
 
 A non-dual filter must return NULL for the unsupported method (input or output) in the dual filter construction callback. There is no such thing as a non dual filter construction callback.
 
 All filters are, but again, need not be, inversible. This means the filter has the means to create a new filter that is its inverse, including the correct filter options. 
 
 Chained filters can also be inversed, if all filters in the chain can be inversed. A PNG_UP (6 col) predictor with a FlateDecode compressor attached to it is laid out as such
 
 @code
 encode            = [filter: Predictor-Predictor <Strategy PNG_UP, Columns 6>]
 encode.nextFilter = [filter: FlateDecode-Compression]
 @endcode
 
 Setting `decode` to the results of PDStreamFilterCreateInversionForFilter() for `encode` produces
 
 @code
 decode            = [filter: FlateDecode-Decompression]
 decode.nextFilter = [filter: Predictor-Unpredictor <Strategy PNG_UP, Columns 6>]
 @endcode
 
 Passing plaintext data into `encode` would produce predictor-filtered compressed data, and passing that data into `decode` gives back the original plaintext data.
 
 Using the inversion mechanism is strongly recommended when modifying filtered content via Pajdeg, rather than setting up two filter chains manually, as the inverter code evolves with the filters (while manual code stays the same). 
 
 @note Chained filter inversion is an extension mechanism enabled via PDStreamFilterCreateInversionForFilter(). Calling a filter's createInversion function directly will produce an inversion for that filter only, excluding its nextFilter chain.
 
 @{
 */

#ifndef INCLUDED_PDStreamFilter_h
#define INCLUDED_PDStreamFilter_h

#include "PDDefines.h"

/**
 Filter function signature.
 */
typedef PDInteger (*PDStreamFilterFunc)(PDStreamFilterRef filter);

/**
 Filter processing signature. Used for inverting filters currently.
 */
typedef PDStreamFilterRef (*PDStreamFilterPrcs)(PDStreamFilterRef filter);

/**
 Dual filter construction signature. 
 
 If inputEnd is set, the reader variant is returned, otherwise the writer variant is returned.
 */
typedef PDStreamFilterRef (*PDStreamDualFilterConstr)(PDBool inputEnd, PDDictionaryRef options);

/**
 The stream filter struct.
 */
struct PDStreamFilter {
    PDBool initialized;                 ///< Determines if filter was set up or not. 
    PDBool compatible;                  ///< If false after initialization, the filter initialized fine but encountered compatibility issues that could potentially cause problems.
    PDBool needsInput;                  ///< Whether the filter needs data. If true, its input buffer must not be modified.
    PDBool hasInput;                    ///< Whether the filter has pending input that, for capacity reasons, has not been added to the buffer yet.
    PDBool finished;                    ///< Whether the filter is finished.
    PDBool failing;                     ///< Whether the filter is failing to handle its input.
    PDDictionaryRef options;            ///< Filter options
    void *data;                         ///< User info object.
    unsigned char *bufIn;               ///< Input buffer.
    unsigned char *bufOut;              ///< Output buffer.
    PDInteger bufInAvailable;           ///< Available data.
    PDInteger bufOutCapacity;           ///< Output buffer capacity.
    PDStreamFilterFunc init;            ///< Initialization function. Called once before first use.
    PDStreamFilterFunc done;            ///< Deinitialization function. Called once after last use.
    PDStreamFilterFunc begin;           ///< Processing function. Called any number of times, at most once per new input buffer.
    PDStreamFilterFunc proceed;         ///< Proceed function. Called any number of times to request more output from last begin call.
    PDStreamFilterPrcs createInversion; ///< Inversion function. Returns, if possible, a filter chain that reverts the current filter.
    PDStreamFilterRef nextFilter;       ///< The next filter that should receive the output end of this filter as its input, or NULL if no such requirement exists.
    float growthHint;                   ///< The growth hint is an indicator for how the filter expects the size of its resulting data to be relative to the unfiltered data. 
    unsigned char *bufOutOwned;         ///< Internal output buffer, that will be freed on destruction. This is used internally for chained filters.
    PDInteger bufOutOwnedCapacity;      ///< Capacity of internal output buffer.
};

/**
 Set up a stream filter with given callbacks.
 
 @param init The initializer. Returns 0 on failure, some other value on success.
 @param done The deinitializer. Returns 0 on failure, some other vaule on success.
 @param begin The begin function; called once when new data is put into bufIn. Returns # of bytes stored into output buffer.
 @param proceed The proceed function; called repeatedly after a begin call was made, until the filter returns 0 lengths. Returns # of bytes stored into output buffer.
 @param createInversion The inversion function; calling this will produce a filter that inverts this filter.
 @param options Options passed to the filter, in the form of a dictionary.
 
 @see PDStreamFilterGenerateOptionsFromDictionaryStack
 */
extern PDStreamFilterRef PDStreamFilterCreate(PDStreamFilterFunc init, PDStreamFilterFunc done, PDStreamFilterFunc begin, PDStreamFilterFunc proceed, PDStreamFilterPrcs createInversion, PDDictionaryRef options);

/**
 Register a dual filter with a given name.
 
 @param name The name of the filter
 @param constr The dual filter construction function.
 */
extern void PDStreamFilterRegisterDualFilter(const char *name, PDStreamDualFilterConstr constr);

/**
 Obtain a filter for given name and type, where the type is a boolean value for whether the filter should be a reader (i.e. decompress) or writer (i.e. compress).
 
 @param name The name of the filter.
 @param inputEnd Whether the input end or output end should be returned. The input end is the reader part (decoder) and the output end is the writer part (encoder). 
 @param options Options passed to the filter, in the form of a simplified pd_stack dictionary (with keys and values and no complex objects). A scanner-produced pd_stack dictionary can be converted into a stream filter options dictionary via PDStreamFilterGenerateOptionsFromDictionaryStack().
 
 @return A created PDStreamFilterRef or NULL if no match.
 
 @see PDStreamFilterGenerateOptionsFromDictionaryStack
 */
extern PDStreamFilterRef PDStreamFilterObtain(const char *name, PDBool inputEnd, PDDictionaryRef options);

/**
 Append a filter to the filter, causing a chain.
 
 The filter is appended to the end (i.e. it will run at the very *last*), if there are more than one filters connected already. 
 
 @param filter The pre-filter.
 @param next The filter to append.
 */
extern void PDStreamFilterAppendFilter(PDStreamFilterRef filter, PDStreamFilterRef next);

/**
 Initialize a filter.
 
 @param filter The filter.
 */
extern PDBool PDStreamFilterInit(PDStreamFilterRef filter);

/**
 Deinitialize a filter.
 
 @param filter The filter.
 */
extern PDBool PDStreamFilterDone(PDStreamFilterRef filter);

/**
 Begin processing, meaning that the filter should take its input buffer as new content. 
 
 @warning Performing multiple separate filtering operations is not done by calling PDStreamFilterBegin() at the start of each new operation. The purpose of PDStreamFilterBegin() is to tell the filter that new or additional data for one consecutive operation has been put into its input buffer. To reuse a filter, it must be deinitialized via PDStreamFilterDone(), and then reinitialized via PDStreamFilterInit().
 
 @param filter The filter

 @return Number of bytes produced
 */
extern PDInteger PDStreamFilterBegin(PDStreamFilterRef filter);

/**
 Proceed with a filter, meaning that it should continue filtering its input, which must not have been altered. This is called if a filter's output hits the output buffer capacity when processing its current input.
 
 @param filter The filter
 
 @return Number of bytes produced
 */
extern PDInteger PDStreamFilterProceed(PDStreamFilterRef filter);

/**
 Apply a filter to the given buffer, creating a new buffer and size. This is a convenience method for applying a filter (chain) to some data and getting a newly allocated buffer containing the results back, along with the result size.
 
 @param filter          The filter to apply
 @param src             The source buffer
 @param dstPtr          The destination buffer pointer
 @param len             The length of the source buffer content
 @param newlenPtr       The filtered content length pointer
 @param allocatedlenPtr The resulting allocation size of the destination buffer (optional, use NULL if unneeded)
 
 @return true on success, false on failure.
 */
extern PDBool PDStreamFilterApply(PDStreamFilterRef filter, unsigned char *src, unsigned char **dstPtr, PDInteger len, PDInteger *newlenPtr, PDInteger *allocatedlenPtr);

/**
 Create the inversion of the given filter, so that invert(filter(data)) == data
 
 @param filter The filter to invert.
 @return Inversion filter, or NULL if the filter or any of its chained filters is unable to invert itself.
 */
extern PDStreamFilterRef PDStreamFilterCreateInversionForFilter(PDStreamFilterRef filter);

///**
// Convert a PDScanner dictionary stack into a stream filter options stack.
// 
// @note The original stack remains untouched.
// 
// @param dictStack The dictionary stack to convert
// @return New allocated stack suited for stream filter options.
// */
//extern pd_stack PDStreamFilterGenerateOptionsFromDictionaryStack(pd_stack dictStack);

/**
 Allocate an uninitialized, non-zeroed stream filter object.
 
 @note This is used by filters to create other filters, and should not be called outside of filter contexts.
 */
extern PDStreamFilterRef PDStreamFilterAlloc(void);

/**
 Convenience macro for setting buffers and sizes.
 */
#define PDStreamFilterPrepare(f, buf_in, len_in, buf_out, len_out) do { \
    f->bufIn = (unsigned char *)buf_in; \
    f->bufOut = (unsigned char *)buf_out; \
    f->bufInAvailable = len_in; \
    f->bufOutCapacity = len_out; \
} while (0)

#endif

/** @} */

