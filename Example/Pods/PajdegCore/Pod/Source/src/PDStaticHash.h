//
// PDStaticHash.h
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
 @file PDStaticHash.h Static hash header file.
 
 @ingroup PDSTATICHASH
 
 @defgroup PDSTATICHASH PDStaticHash
 
 @brief A (very) simple hash table implementation.
 
 @ingroup PDALGO
 
 Limited to predefined set of primitive keys on creation. \f$O(1)\f$ but triggers false positives for undefined keys.
 
 This is used as a pre-filter in the PDPipe implementation to reduce the misses for the more cumbersome pd_btree containing tasks. It is also used in the PDF spec implementation's PDStringFromComplex() function. 
 
 The static hash generates a hash table of \f$2^n\f$ size, where \f$n\f$ is the lowest possible number where the primitive value of the range of keys produces all unique indices based on the following formula, where \f$K\f$, \f$s\f$, and \f$m\f$ are the key, shift and mask parameters
 
    \f$(K \oplus (K \gg s)) \land m\f$
 
  The shifting is necessary because the range of keys tend to align, and in bad cases, the alignment happens in the low bits, resulting in redundancy. Unfortunately, determining the mask and shift values for a given instance is expensive and fairly unoptimized at the moment, with the excuse that creation does not occur repeatedly.
 
 @see PDPIPE
 @see PDPDF_GRP
 
 @{
 */

#ifndef INCLUDED_PDStaticHash_h
#define INCLUDED_PDStaticHash_h

#include "PDDefines.h"

/**
 Generate a hash for the given key based on provided mask and shift.
 
 @note Does not rely on a PDStaticHash instance.
 
 @param mask The mask.
 @param shift The shift.
 @param key The key.
 */
#define static_hash_idx(mask, shift, key)    (((long)key ^ ((long)key >> shift)) & mask)

/**
 Generate a hash for the given key in the given static hash.
 
 @param stha The PDStaticHashRef instance.
 @param key The key.
 */
#define PDStaticHashIdx(stha, key)     static_hash_idx(stha->mask, stha->shift, key)

/**
 Obtain value for given hash.
 
 @param stha The PDStaticHashRef instance.
 @param hash The hash.
 */
#define PDStaticHashValueForHash(stha, hash) stha->table[hash]

/**
 Obtain value for given key.
 
 @param stha The PDStaticHashRef instance.
 @param key The key.
 */
#define PDStaticHashValueForKey(stha, key)   stha->table[PDStaticHashIdx(stha, key)]

/**
 Obtain typecast value for hash.

 @param stha The PDStaticHashRef instance.
 @param hash The hash.
 @param type The type.
 */
#define PDStaticHashValueForHashAs(stha, hash, type) as(type, PDStaticHashValueForHash(stha, hash))

/**
 Obtain typecast value for key.
 
 @param stha The PDStaticHashRef instance.
 @param key The key.
 @param type The type.
 */
#define PDStaticHashValueForKeyAs(stha, key, type)   as(type, PDStaticHashValueForKey(stha, key))

/**
 Create a static hash with keys and values.
 
 Sets up a static hash with given entries, where given keys are hashed as longs into indices inside the internal table, with guaranteed non-collision and O(1) mapping of keys to values.
 
 @note Setting up is expensive.
 @warning Triggers false-positives for non-complete input.
 
 @param entries The number of entries.
 @param keys The array of keys. Note that keys are primitives.
 @param values The array of values corresponding to the array of keys.
 */
extern PDStaticHashRef PDStaticHashCreate(PDInteger entries, void **keys, void **values);

/**
 Indicate that keys and/or values are not owned by the static hash and should not be freed on destruction; enabled flags are not disabled even if false is passed in (once disowned, always disowned)
 
 @param sh The static hash
 @param disownKeys Whether keys should be disowned.
 @param disownValues Whether values should be disowned.
 */
extern void PDStaticHashDisownKeysValues(PDStaticHashRef sh, PDBool disownKeys, PDBool disownValues);

// add key/value entry to a static hash; often expensive
//extern void PDStaticHashAdd(void *key, void *value);

/** @} */

#endif
