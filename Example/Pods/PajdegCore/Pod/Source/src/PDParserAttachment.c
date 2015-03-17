//
// PDParserAttachment.c
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

#include "PDParserAttachment.h"
#include "PDParser.h"
#include "PDSplayTree.h"
#include "PDObject.h"

#include "pd_internal.h"
#include "pd_stack.h"
#include "pd_pdf_implementation.h"

extern void PDParserClarifyObjectStreamExistence(PDParserRef parser, PDObjectRef object);

// 
// Imports and parser attachments
// 

static PDParserAttachmentRef PDParserAttachmentHead = NULL, PDParserAttachmentTail = NULL;

struct PDParserAttachment {
    PDParserAttachmentRef prev, next;
    PDParserRef nativeParser;
    PDParserRef foreignParser;
    PDSplayTreeRef  obMap;
};

void PDParserAttachmentDestroy(PDParserAttachmentRef attachment)
{
    if (attachment == PDParserAttachmentHead) {
        PDParserAttachmentHead = attachment->next;
        if (attachment->next) 
            attachment->next->prev = NULL;
        else 
            PDParserAttachmentTail = NULL;
    } else if (attachment == PDParserAttachmentTail) {
        PDParserAttachmentTail = PDParserAttachmentTail->prev;
        PDParserAttachmentTail->next = NULL;
    } else {
        attachment->prev->next = attachment->next;
        attachment->next->prev = attachment->prev;
    }
    
    PDRelease(attachment->obMap);
}

PDParserAttachmentRef PDParserAttachmentCreate(PDParserRef parser, PDParserRef foreignParser)
{
    // we look through the list of existing attachments and return pre-existing ones with the given parser pair, to prevent the case where a user creates two attachments between the same objects and end up importing the same objects multiple times
    for (PDParserAttachmentRef att = PDParserAttachmentHead; att; att = att->next)
        if (att->nativeParser == parser && att->foreignParser == foreignParser) 
            return PDRetain(att);
    
    PDParserAttachmentRef attachment = PDAllocTyped(PDInstanceTypeParserAtt, sizeof(struct PDParserAttachment), PDParserAttachmentDestroy, false);
    attachment->nativeParser = parser;
    attachment->foreignParser = foreignParser;
    attachment->obMap = PDSplayTreeCreateWithDeallocator(PDReleaseFunc);
    
    attachment->next = NULL;
    attachment->prev = PDParserAttachmentTail;
    if (PDParserAttachmentTail) 
        PDParserAttachmentTail->next = attachment;
    else 
        PDParserAttachmentHead = attachment;
    
    PDParserAttachmentTail = attachment;
    
    return attachment;
}

void PDParserAttachmentPerformImport(PDParserAttachmentRef attachment, PDObjectRef dest, PDObjectRef source, const char **excludeKeys, PDInteger excludeKeysCount);

void PDParserAttachmentImportStack(PDParserAttachmentRef attachment, pd_stack *dst, pd_stack src, const char **excludeKeys, PDInteger excludeKeysCount)
{
    pd_stack backward = NULL;
    pd_stack tmp;
    PDInteger skipCount = 0;
    PDInteger ek;
    pd_stack s;
    pd_stack_for_each(src, s) {
        if (skipCount > 0) {
            skipCount--;
        } else {
            switch (s->type) {
                case PD_STACK_STRING:
                    pd_stack_push_key(&backward, strdup(s->info));
                    break;
                    
                case PD_STACK_ID:
                    if (excludeKeysCount && PDIdentifies(s->info, PD_DE)) {
                        // this is a dictionary entry, which we may not want to include
                        /*
                         stack<0x1136ba20> {
                         0x3f9998 ("de")
                         Kids
                         stack<0x11368f20> {
                         0x3f999c ("array")
                         */
                        const char *key = s->prev->info;
                        for (ek = 0; ek < excludeKeysCount; ek++) 
                            if (0 == strcmp(key, excludeKeys[ek]))
                                break;
                        skipCount = (ek < excludeKeysCount) << 1;
                    } 
                    else if (PDIdentifies(s->info, PD_REF)) {
                        // we need to deal with object references (by copying them over!)
                        char buf[15];
                        PDInteger refObID = atol(s->prev->info);
                        PDObjectRef iob = PDSplayTreeGet(attachment->obMap, refObID);
                        if (iob == NULL) {
                            iob = PDParserCreateAppendedObject(attachment->nativeParser);
                            PDObjectRef eob = PDParserLocateAndCreateObject(attachment->foreignParser, refObID, true);
                            if (eob) {
                                PDParserAttachmentPerformImport(attachment, iob, eob, NULL, 0);
                                PDRelease(eob);
                            } else {
                                PDNotice("NULL object for %ld", refObID);
                            }
                        }
                        pd_stack_push_identifier(&backward, s->info);
                        sprintf(buf, "%ld", (long)PDObjectGetObID(iob));
                        pd_stack_push_key(&backward, strdup(buf));
                        pd_stack_push_key(&backward, strdup("0"));
                        skipCount = 2;
                    }
                    
                    if (! skipCount) {
                        pd_stack_push_identifier(&backward, s->info);
                    }
                    break;
                    
                case PD_STACK_STACK:
                    tmp = NULL;
                    PDParserAttachmentImportStack(attachment, &tmp, s->info, excludeKeys, excludeKeysCount);
                    // if tmp comes out NULL, it means a skip occurred and we don't push it onto backward
                    if (tmp != NULL)
                        pd_stack_push_stack(&backward, tmp);
                    break;
                    
                    // we dont want pdobs as they could be indirect refs (in fact they most certainly are) and we would need to properly copy them
                    //                case PD_STACK_PDOB:
                    //                    pd_stack_push_object(&backward, PDRetain(s->info));
                    //                    break;
                default: // case PD_STACK_FREEABLE:
                    // we don't allow arbitrary freeables (or any other types) in clones
                    PDWarn("skipping arbitrary object in pd_stack_copy operation [PDParserImportStack()]");
                    break;
            }
        }
    }
    
    // backward is backward
    while (backward) pd_stack_pop_into(dst, &backward);
}

void PDParserAttachmentPerformImport(PDParserAttachmentRef attachment, PDObjectRef dest, PDObjectRef source, const char **excludeKeys, PDInteger excludeKeysCount)
{
    PDSplayTreeInsert(attachment->obMap, PDObjectGetObID(source), PDRetain(dest));
    
    PDAssert(dest->def == NULL); // crash = the destination is not a new object, or something broke somewhere
    pd_stack def = NULL;
    PDParserAttachmentImportStack(attachment, &def, source->def, excludeKeys, excludeKeysCount);
    dest->def = def;
    PDObjectDetermineType(dest);
    
    // add stream, if any
    
    PDParserClarifyObjectStreamExistence(attachment->foreignParser, source);
    if (source->hasStream) {
        PDParserLocateAndFetchObjectStreamForObject(attachment->foreignParser, source);
        PDObjectSetStreamFiltered(dest, source->streamBuf, source->extractedLen, false, false);
    }
}

PDObjectRef PDParserAttachmentImportObject(PDParserAttachmentRef attachment, PDObjectRef foreignObject, const char **excludeKeys, PDInteger excludeKeysCount)
{
    PDObjectRef mainObject = PDSplayTreeGet(attachment->obMap, PDObjectGetObID(foreignObject));
    if (mainObject) return mainObject;
    
    mainObject = PDParserCreateAppendedObject(attachment->nativeParser);
    PDParserAttachmentPerformImport(attachment, mainObject, foreignObject, excludeKeys, excludeKeysCount);
    return PDAutorelease(mainObject);
}
