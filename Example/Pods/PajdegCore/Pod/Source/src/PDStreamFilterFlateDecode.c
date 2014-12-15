//
// PDStreamFilterFlateDecode.c
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

#include "pd_internal.h"

#ifdef PD_SUPPORT_ZLIB

#include "pd_stack.h"
#include "PDStreamFilterFlateDecode.h"
#include "zlib.h"
#include "PDDictionary.h"

PDInteger fd_compress_init(PDStreamFilterRef filter)
{
    if (filter->initialized)
        return true;
    
    if (filter->options) {
        PDDictionaryRef options = filter->options;
        void *value = PDDictionaryGet(options, "Predictor");
        if (value) {
            // we need a predictor as well
            PDStreamFilterRef predictor = PDStreamFilterObtain("Predictor", false, filter->options);
            if (predictor) {
                // because prediction has to come before compression we do a swap-a-roo here
                PDStreamFilterRef newSelf = PDStreamFilterAlloc();          // new field for us
                memcpy(newSelf, filter, sizeof(struct PDStreamFilter));     // move us there
                memcpy(filter, predictor, sizeof(struct PDStreamFilter));   // replace old us with predictor
                filter->nextFilter = newSelf;                               // set new us as predictor's next
                // we have two references to ->options; one in newSelf and one in filter (old self). We do not release newSelf->options, or we would be causing a -1 retain imbalance
                //PDRelease(newSelf->options);                                // clear out options from our new self, otherwise we will create predictors forever and ever
                newSelf->options = NULL;
                PDRelease(predictor);
                return (*filter->init)(filter);                             // init predictor, not us
                // from above return, we come into init for predictor, which has an uninitialized next (our new self), which it initializes; since we return here, though, that does not result in double initialization
            }
        }
    }
    
    z_stream *stream = filter->data = calloc(1, sizeof(z_stream));
    
    stream->zalloc = Z_NULL;
    stream->zfree = Z_NULL;
    stream->opaque = Z_NULL;
    
    if (Z_OK != deflateInit(stream, 5)) {
        free(stream);
        return false;
    }
    
    filter->initialized = true;
    
    return true;
}

PDInteger fd_decompress_init(PDStreamFilterRef filter)
{
    if (filter->initialized)
        return true;
    
    if (filter->options) {
        PDDictionaryRef options = filter->options;
        void *value = PDDictionaryGet(options, "Predictor");
        if (value) {
            // we need a predictor as well
            filter->nextFilter = PDStreamFilterObtain("Predictor", true, filter->options);
        }
    }
    
    z_stream *stream = filter->data = calloc(1, sizeof(z_stream));
    
    stream->zalloc = Z_NULL;
    stream->zfree = Z_NULL;
    stream->opaque = Z_NULL;
    stream->avail_in = 0;
    stream->next_in = Z_NULL;
    
    if (Z_OK != inflateInit(stream)) {
        free(stream);
        return false;
    }
    
    filter->initialized = true;
    
    return true;
}

PDInteger fd_compress_done(PDStreamFilterRef filter)
{
    PDAssert(filter->initialized);
    
    z_stream *stream = filter->data;
    
    deflateEnd(stream);
    free(stream);
    
    filter->initialized = false;
    
    return true;
}

PDInteger fd_decompress_done(PDStreamFilterRef filter)
{
    PDAssert(filter->initialized);
    
    z_stream *stream = filter->data;
    
    inflateEnd(stream);
    free(stream);
    
    filter->initialized = false;
    
    return true;
}

PDInteger fd_compress_proceed(PDStreamFilterRef filter)
{
    PDInteger outputLength;
    int ret;

    z_stream *stream = filter->data;

    stream->avail_out = (uInt)filter->bufOutCapacity;
    stream->next_out = filter->bufOut;
    
    // we flush stream, if we have no more input
    int flush = filter->hasInput ? Z_NO_FLUSH : Z_FINISH;
    ret = deflate(stream, flush);
    if (ret < 0) { PDWarn("deflate error: %s\n", stream->msg); }
    filter->finished = ret == Z_STREAM_END;
    PDAssert (ret != Z_STREAM_ERROR); // crash = screwed up setup
    PDAssert (ret != Z_BUF_ERROR);    // crash = buffer was trashed
    filter->failing = ret < 0;
    
    filter->bufInAvailable = stream->avail_in;
    outputLength = filter->bufOutCapacity - stream->avail_out;
    filter->bufOut += outputLength;
    
    filter->needsInput = 0 == stream->avail_in;
    filter->bufOutCapacity = stream->avail_out;
    
    return outputLength;
}

PDInteger fd_decompress_proceed(PDStreamFilterRef filter)
{
    PDInteger outputLength;
    int ret;
    
    z_stream *stream = filter->data;
    
    if (filter->bufInAvailable == 0) {
        // we are being asked to decompress but we haven't gotten any data; this indicates the input source is broken so we're going to just fail silently here
        // this is opposed to crashing hard at the Z_BUF_ERROR that occurs otherwise, below
        filter->finished = true;
        return 0;
    }
    
    stream->avail_out = (uInt)filter->bufOutCapacity;
    stream->next_out = filter->bufOut;
    
    ret = inflate(stream, Z_NO_FLUSH);
    if (ret < 0) { 
        PDError("inflate error: %s\n", stream->msg); 
    }
    filter->finished = ret == Z_STREAM_END;
    PDAssert (ret != Z_STREAM_ERROR); // crash = screwed up setup
    PDAssert (ret != Z_BUF_ERROR);    // crash = buffer was trashed
    switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(stream);
            filter->failing = true;
            return 0;
    }
    
    filter->bufInAvailable = stream->avail_in;
    outputLength = filter->bufOutCapacity - stream->avail_out;
    filter->bufOut += outputLength;
    
    filter->needsInput = 0 == stream->avail_in;
    filter->bufOutCapacity = stream->avail_out;

    return outputLength;
}

PDInteger fd_compress_begin(PDStreamFilterRef filter)
{
    z_stream *stream = filter->data;
    
    stream->avail_in = (uInt)filter->bufInAvailable;
    stream->next_in = filter->bufIn;
    
    return fd_compress_proceed(filter);
}

PDInteger fd_decompress_begin(PDStreamFilterRef filter)
{
    z_stream *stream = filter->data;
    
    stream->avail_in = (uInt)filter->bufInAvailable;
    stream->next_in = filter->bufIn;

    return fd_decompress_proceed(filter);
}

PDStreamFilterRef fd_compress_invert(PDStreamFilterRef filter)
{
    return PDStreamFilterFlateDecodeDecompressCreate(NULL);
}

PDStreamFilterRef fd_decompress_invert(PDStreamFilterRef filter)
{
    return PDStreamFilterFlateDecodeCompressCreate(NULL);
}

PDStreamFilterRef PDStreamFilterFlateDecodeCompressCreate(PDDictionaryRef options)
{
    return PDStreamFilterCreate(fd_compress_init, fd_compress_done, fd_compress_begin, fd_compress_proceed, fd_compress_invert, options);
}

PDStreamFilterRef PDStreamFilterFlateDecodeDecompressCreate(PDDictionaryRef options)
{
    return PDStreamFilterCreate(fd_decompress_init, fd_decompress_done, fd_decompress_begin, fd_decompress_proceed, fd_decompress_invert, options);
}

PDStreamFilterRef PDStreamFilterFlateDecodeConstructor(PDBool inputEnd, PDDictionaryRef options)
{
    return (inputEnd
            ? PDStreamFilterFlateDecodeDecompressCreate(options)
            : PDStreamFilterFlateDecodeCompressCreate(options));
}

#endif // PD_SUPPORT_ZLIB
