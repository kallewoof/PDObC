//
// PDStreamFilterPrediction.c
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

#include "pd_internal.h"
#include "pd_stack.h"
#include "PDDictionary.h"
#include "PDNumber.h"

#include "PDStreamFilterPrediction.h"

typedef struct PDPredictor *PDPredictorRef;
/**
 Predictor settings, including options and state data for an ongoing predictor/unpredictor operation.
 */
struct PDPredictor {
    unsigned char *prevRow;     ///< Previous row cache.
    PDInteger columns;          ///< Columns per row.
    PDPredictorType predictor;  ///< Predictor type (strategy)
};

PDInteger pred_init(PDStreamFilterRef filter)
{
    if (filter->initialized)
        return true;
    
    PDPredictorRef pred = malloc(sizeof(struct PDPredictor));
    pred->predictor = PDPredictorNone;
    pred->columns = 1;
    
    filter->data = pred;
    
    // parse options
    PDDictionaryRef dict = filter->options;
    PDNumberRef n;
    n = PDDictionaryGet(dict, "Columns");
    if (n) pred->columns = PDNumberGetInteger(n);
    n = PDDictionaryGet(dict, "Predictor");
    if (n) pred->predictor = (PDPredictorType)PDNumberGetInteger(n);
    
    // we only support given predictors; as more are encountered, support will be added
    switch (pred->predictor) {
        case PDPredictorNone:
        case PDPredictorPNG_UP:
        case PDPredictorPNG_OPT:
            break;
        case PDPredictorPNG_SUB:
        case PDPredictorPNG_AVG:
        case PDPredictorPNG_PAE:
            break;
            
        default:
            PDWarn("Unsupported predictor: %d\n", pred->predictor);
            return false;
    }
    
    pred->prevRow = calloc(1, pred->columns);

    filter->initialized = true;
    
    return true;
}

#define unpred_init pred_init
/*PDInteger unpred_init(PDStreamFilterRef filter)
{
    return pred_init(filter);
}*/

PDInteger pred_done(PDStreamFilterRef filter)
{
    PDAssert(filter->initialized);

    PDPredictorRef pred = filter->data;
    free(pred->prevRow);
    free(pred);

    filter->initialized = false;
    
    return true;
}

#define unpred_done pred_done
/*PDInteger unpred_done(PDStreamFilterRef filter)
{
    return pred_done(filter);
}*/

int paeth_predictor(int a, int b, int c)
{
    // a = left, b = above, c = upper left
    int p = a + b - c;
    int pa = abs(p - a); // distances to a, b, c
    int pb = abs(p - b);
    int pc = abs(p - c);
    // return nearest of a,b,c,
    // breaking ties in order a,b,c.
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

PDInteger pred_proceed(PDStreamFilterRef filter)
{
    PDInteger outputLength;
    
    PDPredictorRef pred = filter->data;
    
    if (filter->bufInAvailable == 0) {
        filter->finished = ! filter->hasInput;
        return 0;
    }
    
    if (pred->predictor == PDPredictorNone) {
        PDInteger amount = filter->bufOutCapacity > filter->bufInAvailable ? filter->bufInAvailable : filter->bufOutCapacity;
        memcpy(filter->bufOut, filter->bufIn, amount);
        filter->bufOutCapacity -= amount;
        filter->bufIn += amount;
        filter->bufInAvailable -= amount;
        return amount;
    }
    
    unsigned char *src = filter->bufIn;
    unsigned char *dst = filter->bufOut;
    PDInteger avail = filter->bufInAvailable;
    PDInteger cap = filter->bufOutCapacity;
    PDInteger bw = pred->columns;
    PDInteger rw = bw + 1;
    PDInteger i;
    
    //PDAssert(avail % bw == 0); // crash = this filter is bugged, or the input is corrupt

    unsigned char  prevRowMinus1 = 0;
    unsigned char *prevRow = pred->prevRow;
    unsigned char *row = calloc(1, bw);
    
    // if the predictor is OPT, we fall back to UP, which we fill into every row, otherwise we keep it
    PDPredictorType predictor = pred->predictor == PDPredictorPNG_OPT ? PDPredictorPNG_UP : pred->predictor;
    
    PDAssert(predictor >= 10);
    
#define pred_loop_begin() \
    while (avail >= bw && cap >= rw) {\
        memcpy(row, src, bw);\
        src += bw;\
        avail -= bw;\
\
        *dst = predictor - 10;\
        dst++;\
\
        for (i = 0; i < bw; i++) {
#define pred_loop_end() \
            prevRow[i] = row[i];\
            dst++;\
        }\
        cap -= rw;\
    }
#define pred_apply(formula, suffix) pred_loop_begin() *dst = (formula) & 0xff; suffix; pred_loop_end()
#define pred_case(pred, formula, suffix) \
        case pred: \
            pred_apply(formula, suffix); \
            break
    
    switch (predictor) {
            pred_case(PDPredictorPNG_UP,    row[i] - prevRow[i],);
            pred_case(PDPredictorPNG_SUB,   row[i] - (i ? row[i-1] : 0),);
            pred_case(PDPredictorPNG_AVG,   row[i] - ((i ? row[i-1] : 0) + prevRow[i]) / 2,);
            pred_case(PDPredictorPNG_PAE,   paeth_predictor(i ? row[i-1] : 0, prevRow[i], i ? prevRowMinus1 : 0), prevRowMinus1 = prevRow[i]);
        default:
            // unsupported; falling back to PNG_NONE
            pred_apply(row[i],);
            break;
    }
    
    free(row);
    
    outputLength = filter->bufOutCapacity - cap;

    filter->bufIn = src;
    filter->bufOut = dst;
    filter->bufInAvailable = avail;
    filter->bufOutCapacity = cap;
    filter->needsInput = avail < bw;
    filter->finished = avail == 0 && ! filter->hasInput;

    return outputLength;
}

PDInteger unpred_proceed(PDStreamFilterRef filter)
{
    PDInteger outputLength;
    
    PDPredictorRef pred = filter->data;
    
    if (filter->bufInAvailable == 0) {
        filter->finished = ! filter->hasInput;
        return 0;
    }
    
    if (pred->predictor == PDPredictorNone) {
        PDInteger amount = filter->bufOutCapacity > filter->bufInAvailable ? filter->bufInAvailable : filter->bufOutCapacity;
        memcpy(filter->bufOut, filter->bufIn, amount);
        filter->bufOutCapacity -= amount;
        filter->bufIn += amount;
        filter->bufInAvailable -= amount;
        return amount;
    }
    
    unsigned char *src = filter->bufIn;
    unsigned char *dst = filter->bufOut;
    PDInteger avail = filter->bufInAvailable;
    PDInteger cap = filter->bufOutCapacity;
    PDInteger bw = pred->columns;
    PDInteger rw = bw + 1;
    PDInteger i;
    
    // this throws incorrectly if input is incomplete
    //PDAssert(avail % rw == 0); // crash = this filter is bugged, or the input is corrupt
    
    unsigned char  prevRowMinus1 = 0;
    unsigned char *prevRow = pred->prevRow;
    unsigned char *row = calloc(1, bw);

    PDPredictorType predictor = pred->predictor;
    
#define unpred_loop_begin() \
    while (avail >= rw && cap >= bw) {\
        predictor = src[0] + 10;\
        memcpy(row, &src[1], bw);\
        src += rw;\
        avail -= rw;\
        \
        for (i = 0; i < bw; i++) {
#define unpred_loop_end() \
            dst++;\
        }\
        cap -= bw;\
    }
#define unpred_apply(formula, suffix) unpred_loop_begin() *dst = (formula) & 0xff; suffix; prevRow[i] = *dst; unpred_loop_end()
#define unpred_case(pred, formula, suffix) \
        case pred: \
            unpred_apply(formula, suffix); \
            break
    
    if (predictor == PDPredictorPNG_OPT) {
        while (avail >= rw && cap >= bw) {
            predictor = src[0] + 10;
            memcpy(row, &src[1], bw);
            src += rw;
            avail -= rw;
            
#define unpred_opt_case(pred, formula, suffix) \
                case pred:\
                    for (i = 0; i < bw; i++) {\
                        *dst = (formula) & 0xff; suffix; prevRow[i] = *dst;\
                        dst++;\
                    }\
                    break
            switch (predictor) {
                    unpred_opt_case(PDPredictorPNG_UP,  row[i] + prevRow[i],);
                    unpred_opt_case(PDPredictorPNG_SUB, row[i] + (i ? row[i-1] : 0),);
                    unpred_opt_case(PDPredictorPNG_AVG, row[i] + ((i ? row[i-1] : 0) + prevRow[i]),);
                    unpred_opt_case(PDPredictorPNG_PAE, row[i] + paeth_predictor(i ? row[i-1] : 0, prevRow[i], prevRowMinus1), prevRowMinus1 = prevRow[i]);
                default:
                    for (i = 0; i < bw; i++) {
                        unpred_apply(row[i],);
                        dst++;
                    }
            }
            cap -= bw;
        }
    } else {
        switch (predictor) {
                unpred_case(PDPredictorPNG_UP,  row[i] + prevRow[i],);
                unpred_case(PDPredictorPNG_SUB, row[i] + (i ? row[i-1] : 0),);
                unpred_case(PDPredictorPNG_AVG, row[i] + ((i ? row[i-1] : 0) + prevRow[i]),);
                unpred_case(PDPredictorPNG_PAE, row[i] + paeth_predictor(i ? row[i-1] : 0, prevRow[i], prevRowMinus1), prevRowMinus1 = prevRow[i]);
            default:
                unpred_apply(row[i],);
                break;
        }
    }
    
    free(row);
    
    outputLength = filter->bufOutCapacity - cap;
    
    filter->bufIn = src;
    filter->bufOut = dst;
    filter->bufInAvailable = avail;
    filter->bufOutCapacity = cap;
    filter->needsInput = avail < rw;
    filter->finished = avail == 0 && ! filter->hasInput;

    return outputLength;
}

PDInteger pred_begin(PDStreamFilterRef filter)
{
    return pred_proceed(filter);
}

PDInteger unpred_begin(PDStreamFilterRef filter)
{
    //if (as(PDPredictorRef, filter->data)->predictor >= 10)
    //   filter->bufInAvailable -= 10; // crc
    return unpred_proceed(filter);
}

PDStreamFilterRef pred_invert_with_options(PDStreamFilterRef filter, PDBool inputEnd)
{
    PDPredictorRef pred = filter->data;
    
    PDDictionaryRef opts = PDDictionaryCreate();
    PDDictionarySet(opts, "Predictor", PDNumberWithInteger(pred->predictor));
    PDDictionarySet(opts, "Columns", PDNumberWithInteger(pred->columns));
    
    return PDStreamFilterPredictionConstructor(inputEnd, opts);
}

PDStreamFilterRef pred_invert(PDStreamFilterRef filter)
{
    return pred_invert_with_options(filter, true);
}

PDStreamFilterRef unpred_invert(PDStreamFilterRef filter)
{
    return pred_invert_with_options(filter, false);
}

PDStreamFilterRef PDStreamFilterUnpredictionCreate(PDDictionaryRef options)
{
    return PDStreamFilterCreate(unpred_init, unpred_done, unpred_begin, unpred_proceed, unpred_invert, options);
}

PDStreamFilterRef PDStreamFilterPredictionCreate(PDDictionaryRef options)
{
    return PDStreamFilterCreate(pred_init, pred_done, pred_begin, pred_proceed, pred_invert, options);
}

PDStreamFilterRef PDStreamFilterPredictionConstructor(PDBool inputEnd, PDDictionaryRef options)
{
    return (inputEnd
            ? PDStreamFilterUnpredictionCreate(options)
            : PDStreamFilterPredictionCreate(options));
}
