//
// PDTwinStream.c
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

#include <assert.h>

#include "Pajdeg.h"
#include "PDTwinStream.h"
#include "PDScanner.h"

#include "pd_internal.h"

#define PIO_CHUNK_SIZE  512

void PDTwinStreamRealign(PDTwinStreamRef ts);

void PDTwinStreamDestroy(PDTwinStreamRef ts)
{
//    PDScannerContextPop();
    
    PDRelease(ts->scanner);
    if (ts->sidebuf) free(ts->sidebuf);
    free(ts->heap);
}

PDTwinStreamRef PDTwinStreamCreate(FILE *fi, FILE *fo)
{
    PDTwinStreamRef ts = PDAllocTyped(PDInstanceType2Stream, sizeof(struct PDTwinStream), PDTwinStreamDestroy, true);
    ts->fi = fi;
    ts->fo = fo;
    
    return ts;
}

//
// configuring / querying
//

PDScannerRef PDTwinStreamCreateScanner(PDTwinStreamRef ts, PDStateRef state)
{
    PDScannerRef scanner = PDScannerCreateWithState(state);
    PDScannerPushContext(scanner, ts, PDTwinStreamGrowInputBuffer);
    return scanner;
}

PDScannerRef PDTwinStreamGetScanner(PDTwinStreamRef ts)
{
    return ts->scanner;
}

PDScannerRef PDTwinStreamSetupScannerWithState(PDTwinStreamRef ts, PDStateRef state)
{
    PDRelease(ts->scanner);
    ts->scanner = PDScannerCreateWithState(state);
    PDScannerPushContext(ts->scanner, ts, PDTwinStreamGrowInputBuffer);
    return ts->scanner;
}

void PDTWinStreamSetMethod(PDTwinStreamRef ts, PDTwinStreamMethod method)
{
    PDAssert(ts->offso == 0);
    
    if (ts->method == method) return;
    
    PDBool reset = (ts->method == PDTwinStreamReversed) ^ (method == PDTwinStreamReversed);
    ts->method = method;
    
    if (reset) {
        // when switching direction, the heap is purged (in spirit) and the cursor is put at the start (for !rev) or end (for rev)
        PDBool reversedInput = ts->method == PDTwinStreamReversed;

        ts->cursor = ts->size * reversedInput;
        
        ts->holds = 0;
        
        // we also jump to the start or end of the file 
        
        PDAssert(ts->offso == 0); // crash = the stream was reversed/unreversed AFTER content was written to output; this is absolutely not supported anywhere or in any way shape form or color
        if (reversedInput) {
            fseek(ts->fi, 0, SEEK_END);
            fgetpos(ts->fi, &ts->offsi);
        } 
    }
    
    // note: we do not seek to start for RandomAccess, because of the nature of PDF:s -- the xref is at a byte offset, USUALLY at the very end of the file; we're probably AT the end of the file now, and we've probably just unreversed which probably means we've determined xref position and are about to jump the (short) distance there; ReadWrite obviously seeks back to start as it's preparing to begin the stream operation
    if (method == PDTwinStreamReadWrite) {
        fseek(ts->fi, 0, SEEK_SET);
        ts->offsi = 0;
        ts->holds = 0;
        ts->cursor = 0;
    }
}

//
// interlude! move along, nothing to see
//

//#define PDSTREAMOP_DEBUG
#ifdef PDSTREAMOP_DEBUG
static inline void print_stream(char *op, char *buf, PDInteger start, PDInteger length)
{
    printf("[%s.%d..%d]: [[[\n", op, start, length);
    for (PDInteger i = 0; i < length; i++) 
        putchar(buf[start+i] < '0' || buf[start+i] > 'z' ? '.' : buf[start+i]);
    printf("\n]]]\n");
}

static char *OPTYPE;
#   define PDSOp(op) OPTYPE = op
#   define PDSLog(length, fmt...) print_stream(OPTYPE, ts->heap, ts->cursor, length); printf(fmt)
#   define PDSLogg(fmt...) printf(fmt)
#else
#   define PDSOp(op) 
#   define PDSLog(length, fmt...) 
#   define PDSLogg(fmt...) 
#endif

//
// reading 
//

void PDTwinStreamGrowInputBuffer(void *_ts, PDScannerRef scanner, char **buf, PDInteger *size, PDInteger req)
{
    // req is the bytes of data the requester desires, beyond their current size (*size); presuming the buffer is 
    // at the very edge, we want to pull in `req' bytes straight off, and *size thus increases to *size + req
    // but if we have data preloaded, we want (at least) req - preloaded, where `preloaded' is defined as
    // 
    //     holds - (*buf - heap) - *size
    //
    // i.e. the entire internal buffer (directly on top of the heap) minus the byte offset of the buffer from
    // the heap, minus *size (the amount of data already claimed by the requester). note that `cursor' has 
    // nothing to do with this (it is neutralized by *buf - heap)
    
    PDAssert(*size > -1);
    
#ifdef DEBUG
    if (req > 70000) {
        PDNotice("huge request: %ldb\n", req);
    }
#endif
    
    PDTwinStreamRef ts = _ts;
    PDSize pos, capacity;
    long preloaded;
    
    if (NULL == (*buf)) {
        // buffer is new and starts at heap start
        *buf = ts->heap + ts->cursor;
        pos = ts->cursor;
    } else {
        // buffer pre-existing, and its content is defined from its relative pos to the heap + its current size
        PDAssert(ts->heap <= *buf && ts->heap + ts->size >= *buf + *size); // crash = heap was moved but buffer didn't join; break in realloc point below may be an idea
        pos = (*buf) - ts->heap;
    }
    
    // the available amount (preloaded) is
    preloaded = ts->holds - (*buf - ts->heap) - *size;
    PDAssert(preloaded >= 0);
    
    // if we have enough data to cover req, we can give it back instantly
    if (req > 0 && preloaded >= req) {
        //ts->heap + ts->holds > *buf + *size + req) {
        *size += preloaded;
        return;
    }

    // the capacity refers to allocation; if insufficient, reallocation is required
    capacity = ts->size - ts->holds; //*size - pos - 
    
    // we also normalize `req' to mean amount of memory that must be read from the file, rather than the amount requested
    req -= preloaded;
    
    // tweak request performance-wise to not result in too many fread()s
    if (req < PIO_CHUNK_SIZE) req = PIO_CHUNK_SIZE;
    
#if 0
    // grow heap if necessary
    if (req > capacity && req <= capacity + ts->cursor) {
        // always realign cursor if it means not having to grow the heap
        capacity += ts->cursor;
        pos -= ts->cursor;
        PDTwinStreamRealign(ts);
        *buf = ts->heap + pos;
    }
#endif
    
    if (req > capacity) {
        PDSize growth = PIO_CHUNK_SIZE * (1 + (req - capacity) / PIO_CHUNK_SIZE);
        ts->size += growth;
        if ((unsigned long long)ts->size > 10000000000000) {
            fprintf(stderr, "break!");
            assert(0); // crash = you just ran into an existing bug; please please please let us know how to reproduce this
        }
        char *h = realloc(ts->heap, ts->size);
        if (h != ts->heap) {
            // we have to realign buf as heap had to move somewhere else
            ts->heap = h;
            (*buf) = ts->heap + pos;
        }
    }
    
    // read into heap and update settings
    PDSLogg("reading input bytes %lld .. %lld\n", ts->offsi + ts->holds, ts->offsi + ts->holds + req);
    PDSize read = fread(&ts->heap[ts->holds], 1, req, ts->fi);
    PDSOp("grow");
    PDSLog(req, "grow heap\n");
    ts->holds += read;
    *size += preloaded + read;
    PDAssert(ts->holds <= ts->size);
    PDTwinStreamAsserts(ts);
}

void PDTwinStreamGrowInputBufferReversed(void *_ts, PDScannerRef scanner, char **buf, PDInteger *size, PDInteger req)
{
    PDTwinStreamRef ts = _ts;
    PDSize pos, capacity;
    
    /*
     when reading reversed, the ts->heap + ts->size - ts->holds defines the point from which content exists all the way
     to the end of the heap; when buffers are grown, the question is whether
     
     heap        holds   buf         size
                 ^^^^^^^^
               this amount
     
     is greater than the requested amount or not
     */
    
    if ((*size) == 0) {
        // buffer is new and starts at heap end
        *buf = ts->heap + ts->size;
        pos = ts->size;
    } else {
        // buffer pre-existing, and its content is defined from its relative pos to the heap end - its current size
        PDAssert(ts->heap <= *buf && ts->heap + ts->size >= *buf + *size); // crash = heap was moved but buffer didn't join; break in realloc point below may be an idea
        pos = (*buf) - ts->heap;
    }
    
    // if we have enough data to cover req, we can give it back instantly
    if (ts->holds > *size + req) {
        *size = ts->holds;
        *buf = ts->heap + ts->size - ts->holds;
        return;
    }
    // but alas, reading is necessary
    
    capacity = ts->size - ts->holds;
    
    // tweak request performance-wise to not result in too many fread()s and attempt to avoid reallocating heap if at all possible
    if (req < PIO_CHUNK_SIZE && (capacity < 50 || capacity >= PIO_CHUNK_SIZE))
        req = PIO_CHUNK_SIZE;
    
    // tweak request to not result in negative position in file
    if (req > ts->offsi) 
        req = (PDInteger)ts->offsi;
    
    // grow heap if necessary
    if (req > capacity) {
        // this is a *very* expensive operation, which means we make absolutely sure it only happens once or twice
        PDSize growth = (req - capacity) > 6 * PIO_CHUNK_SIZE ? PIO_CHUNK_SIZE * (1 + (req - capacity) / PIO_CHUNK_SIZE) : 6 * PIO_CHUNK_SIZE;
        PDSize offset = ts->size - ts->holds;

        ts->size += growth;
        char *h = malloc(ts->size);
        if (ts->heap) {
            memcpy(&h[growth + offset], &ts->heap[offset], ts->holds);
            free(ts->heap);
        }
        ts->heap = h;
        
        pos += growth;
        *buf = ts->heap + pos;
    }
    
    // read into heap and update settings
    
    PDAssert(ts->offsi >= req); // crash = somebody is trying to read beyond the first character of the file
    ts->offsi -= req;
    ts->holds += req;
    fseek(ts->fi, (long)ts->offsi, SEEK_SET);
    fread(&ts->heap[ts->size - ts->holds], 1, req, ts->fi);
    *size = ts->holds;
    *buf = ts->heap + ts->size - ts->holds;
    PDAssert(ts->holds <= ts->size);

    // note: support for a buffer that does not end at the same point as the heap is unsupported at this point in time
}

void PDTwinStreamDisallowGrowth(void *ts, PDScannerRef scanner, char **buf, PDInteger *size, PDInteger req)
{
    as(PDTwinStreamRef, ts)->outgrown = true;
}

void PDTwinStreamAdvance(PDTwinStreamRef ts, PDSize bytes)
{
    PDTwinStreamSeek(ts, (PDSize)ts->offsi + bytes);
}

void PDTwinStreamSeek(PDTwinStreamRef ts, PDSize position)
{
    PDAssert(ts->method == PDTwinStreamRandomAccess);
    
    if (ts->offsi <= position && ts->offsi + ts->holds > position) {
        ts->cursor = position - (PDSize)ts->offsi;
        return;
    } 
    
    ts->cursor = 
    ts->holds = 0;
    fseek(ts->fi, position, SEEK_SET);
    ts->offsi = position;
}

PDSize PDTwinStreamFetchBranch(PDTwinStreamRef ts, PDSize position, PDInteger bytes, char **buf)
{
    // discard existing branch buffer, if any
    if (ts->sidebuf) 
        PDTwinStreamCutBranch(ts, ts->sidebuf);

    // clear outgrown flag (this is only ever used for branches)
    ts->outgrown = false;
    
    PDInteger alignment = (PDInteger)(position - ts->offsi);
    PDInteger covered = (PDInteger)(ts->holds - alignment);
    
    // is position within heap?
    if (alignment >= 0 && covered > 0 && covered >= bytes) {
        // it is, so we point buf
        *buf = ts->heap + alignment;
        /*if (covered < bytes) {
            // not stretched enough, but we'll make it so
            PDTwinStreamGrowInputBuffer(ts, NULL, buf, &covered, bytes - covered);
        }*/
        return bytes;
    }
    
    // we set up a dedicated buffer for this request
    PDOffset cpos;
    fgetpos(ts->fi, &cpos);
    fseek(ts->fi, position, SEEK_SET);
    *buf = ts->sidebuf = malloc(bytes);
    PDSize read = fread(ts->sidebuf, 1, bytes, ts->fi);
    fseek(ts->fi, (long)cpos, SEEK_SET);
    return read;
}

#ifdef PD_DEBUG_TWINSTREAM_ASSERT_OBJECTS
void PDTwinStreamReassert(PDTwinStreamRef ts, PDOffset offset, char *expect, PDInteger len)
{
    // we set up a dedicated buffer for this request
    PDOffset cpos;
    fgetpos(ts->fo, &cpos);
    fseek(ts->fo, offset, SEEK_SET);
    char *tmpbuf = malloc(len + 1);
    PDSize read = fread(tmpbuf, 1, len, ts->fo);
    fseek(ts->fo, cpos, SEEK_SET);

    if (read != len || strncmp(tmpbuf, expect, len)) {
        tmpbuf[len] = 0;
        if (read != len) {
            fprintf(stderr, "* twin stream assertion : expected \"%s\" at %lld but output truncates at %zu of %ld bytes *\n", expect, offset, read, len);
        } else {
            fprintf(stderr, "* twin stream assertion : expected \"%s\" at %lld but got \"%s\" *\n", expect, offset, tmpbuf);
        }
        PDAssert(0);
    }
    
    free(tmpbuf);
}
#endif

void PDTwinStreamCutBranch(PDTwinStreamRef ts, char *buf)
{
    if (buf != ts->sidebuf) 
        return;
    free(ts->sidebuf);
    ts->sidebuf = NULL;
}

//
// committing
//

/*PDSize PDTwinStreamScannerCommitBytes(PDTwinStreamRef ts)
{
    return ts->scanner->buf - ts->heap + ts->scanner->boffset - ts->cursor;
}*/

void PDTwinStreamAsserts(PDTwinStreamRef ts)
{
    PDOffset fp;
    fgetpos(ts->fi, &fp);
    PDAssert(fp == ts->offsi + ts->holds);
    fgetpos(ts->fo, &fp);
    PDAssert(fp == ts->offso);

    /*
    if (ts->scanner && ts->scanner->buf) {
        PDSLogg("[%p <%u> [%p <%d>] %u]\n", ts->heap, ts->cursor, ts->scanner->buf, ts->scanner->bsize, ts->holds);
        PDAssert(ts->scanner->buf == ts->heap + ts->cursor);
        PDAssert(ts->scanner->bsize + (ts->scanner->buf - ts->heap) <= ts->holds);
    }
     */
}

void PDTwinStreamRealign(PDTwinStreamRef ts)
{
    PDAssert(ts->holds >= ts->cursor);
    ts->holds -= ts->cursor;
    // we use memcpy if we can, otherwise we use memmove
    if (ts->holds < ts->cursor) {
        memcpy(ts->heap, &ts->heap[ts->cursor], ts->holds);
    } else {
        memmove(ts->heap, &ts->heap[ts->cursor], ts->holds);
    }
    
    // we also realign the scanner, if present
    PDAssert(ts->scanner->buf - ts->heap >= ts->cursor);
    PDScannerAlign(ts->scanner, -ts->cursor);
    
    ts->offsi += ts->cursor;
    ts->cursor = 0;
    PDTwinStreamAsserts(ts);
}

void PDTwinStreamOperatorDiscard(PDTwinStreamRef ts, char *buf, PDSize bytes);

void PDTwinStreamOperateOnContent(PDTwinStreamRef ts, PDOffset bytes, void(*op)(PDTwinStreamRef, char *, PDSize))
{
    PDAssert(bytes >= 0);
    if (bytes <= 0) 
        return;

    PDTwinStreamAsserts(ts);
    
    PDSLogg("[stream] from %lld the next %lld bytes\n", ts->offsi + ts->cursor, bytes);
    
    if (ts->size < 6*PIO_CHUNK_SIZE && bytes > 6*PIO_CHUNK_SIZE) {
        // big requests will loop a lot if we get them early and heap is small
        PDTwinStreamGrowInputBuffer(ts, ts->scanner, &ts->scanner->buf, &ts->scanner->bsize, 6*PIO_CHUNK_SIZE);
    }
    
    if (ts->holds - ts->cursor < bytes) {
        // remainder of heap can be passed through
        PDSLog(ts->holds - ts->cursor, "[and more] (whole heap)\n");
        ts->offsi += ts->cursor + bytes;

        (*op)(ts, &ts->heap[ts->cursor], ts->holds - ts->cursor);
        bytes -= ts->holds - ts->cursor;
        ts->holds = ts->cursor = 0;
        
        if (bytes > 0) {
            if (op == &PDTwinStreamOperatorDiscard) {
                // the discard operator is NOP, so there's no point reading and discarding anything
                fseek(ts->fi, (long)bytes, SEEK_CUR);
            } else {
                // use the heap as a shuttle for remaining content
                PDSize req, read;
                while (bytes > 0) {
                    req = bytes < ts->size ? (PDInteger)bytes : ts->size;
                    read = fread(ts->heap, 1, req, ts->fi);
                    (*op)(ts, ts->heap, read);
                    bytes -= read;
                    if (read < req) {
                        PDAssert(0); // crash = attempt to operate on more content than input stream has available; sure sign of corruption
                        break;
                    }
                }
            }
        }
        
        // reset master scanner as it's now completely out of data
        PDScannerReset(ts->scanner);
        return;
    }

    // operate on content out of heap, iterate cursor and trim scanner
    PDSLog(bytes, "(in-heap)\n");
    (*op)(ts, &ts->heap[ts->cursor], (PDSize)bytes);
    ts->cursor += bytes;
    PDScannerTrim(ts->scanner, bytes);
    PDTwinStreamAsserts(ts);
    
    // we realign stream when opportune
    if (ts->cursor * 2 > ts->size) {
        PDTwinStreamRealign(ts);
    }
}

void PDTwinStreamOperatorPassthrough(PDTwinStreamRef ts, char *buf, PDSize bytes)
{
    fwrite(buf, 1, bytes, ts->fo);
    ts->offso += bytes;
}

void PDTwinStreamOperatorDiscard(PDTwinStreamRef ts, char *buf, PDSize bytes)
{}

void PDTWinStreamPassthroughContent(PDTwinStreamRef ts)//, PDSize bytes)
{
    PDSOp("pass");
    PDOffset bytes = ts->scanner->buf - ts->heap + ts->scanner->boffset - ts->cursor;
    PDTwinStreamOperateOnContent(ts, bytes, &PDTwinStreamOperatorPassthrough);
}

void PDTwinStreamDiscardContent(PDTwinStreamRef ts)//, PDSize bytes)
{
    PDSOp("discard");
    PDOffset bytes = ts->scanner->buf - ts->heap + ts->scanner->boffset - ts->cursor;
    PDTwinStreamOperateOnContent(ts, bytes, &PDTwinStreamOperatorDiscard);
}

void PDTwinStreamPrune(PDTwinStreamRef ts, PDOffset mark)
{
    PDSOp("prune");
    PDOffset bytes = mark - PDTwinStreamGetInputOffset(ts);
    if (bytes > 0) 
        PDTwinStreamOperateOnContent(ts, bytes, &PDTwinStreamOperatorPassthrough);
}

void PDTwinStreamInsertContent(PDTwinStreamRef ts, PDSize bytes, const char *content)
{
    ts->offso += fwrite(content, 1, bytes, ts->fo);
}
