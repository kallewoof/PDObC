//
//  PDCMap.c
//  Pods
//
//  Created by Karl-Johan Alm on 16/01/15.
//
//

#include "Pajdeg.h"
#include "PDCMap.h"
#include "pd_internal.h"
#include "pd_ps_implementation.h"

struct PDCMapRange {
    PDStringRef startRange;
    PDStringRef endRange;
};

struct PDCMapRangeMapping {
    PDStringRef startRange;
    PDStringRef endRange;
    PDStringRef mapStart;
};

struct PDCMapCharMapping {
    PDStringRef startChar;
    PDStringRef endChar;
};

void PDCMapCompile(PDCMapRef cmap, char *data, PDSize len);

void PDCMapDestroy(PDCMapRef cmap)
{
    PDRelease(cmap->systemInfo);
    PDRelease(cmap->name);
    PDRelease(cmap->type);
    if (cmap->csrs) free(cmap->csrs);
    if (cmap->bfrs) free(cmap->bfrs);
    if (cmap->bfcs) free(cmap->bfcs);
}

PDCMapRef PDCMapCreateWithData(char *data, PDSize len)
{
    PDCMapRef cmap = PDAllocTyped(PDInstanceTypeCMap, sizeof(struct PDCMap), PDCMapDestroy, true);
    
    PDCMapCompile(cmap, data, len);
    
    return cmap;
}

PDCMapRef PDCMapCreate()
{
    return PDAllocTyped(PDInstanceTypeCMap, sizeof(struct PDCMap), PDCMapDestroy, true);
}

PDStringRef PDCMapApply(PDCMapRef cmap, PDStringRef string)
{
    PDStringRef binString = PDStringCreateFromStringWithType(string, PDStringTypeBinary, false, true);
    unsigned char *data = (unsigned char *)binString->data;
    PDSize length = binString->length;
    
    for (PDSize diter = 0; diter < length; diter += 2) {
        int b1 = data[diter];
        int b2 = data[diter+1];
        PDBool found = false;

        PDSize ix = cmap->bfrCount;
        for (PDSize i = 0; !found && i < ix; i++) {
            PDCMapRangeMapping *rm = &cmap->bfrs[i];
            int f1 = (unsigned char)rm->startRange->data[0];
            int f2 = (unsigned char)rm->startRange->data[1];
            int t1 = (unsigned char)rm->endRange->data[0];
            int t2 = (unsigned char)rm->endRange->data[1];
            if (f1 <= b1 && t1 >= b1 &&
                f2 <= b2 && t2 >= b2) {
                int m1 = rm->mapStart->data[0];
                int m2 = rm->mapStart->data[1];
                // due to rectangularity, we map the individual bytes so that map(b1, b2) -> (m1 + b1 - f1, m2 + b2 - f2)
                // i.e. if starting range is (1111), mapStart is (5060), and input value is 1234, then
                // map(12,34) -> (50 + 12 - 11, 60 + 34 - 11) = (51,83)
                data[diter] = m1 + b1 - f1;
                data[diter+1] = m2 + b2 - f2;
                found = true;
            }
        }
        
        ix = cmap->bfcCount;
        for (PDSize i = 0; ! found && i < ix; i++) {
            PDCMapCharMapping *cm = &cmap->bfcs[i];
            int c1 = (unsigned char)cm->startChar->data[0];
            int c2 = (unsigned char)cm->startChar->data[1];
            if (b1 == c1 && b2 == c2) {
                data[diter] = cm->endChar->data[0];
                data[diter+1] = cm->endChar->data[1];
                found = true;
            }
        }
    }
    
    // we explicitly kill the alt, since it is no longer an alternative representation of this string (the string has changed)
    PDRelease(binString->alt);
    binString->alt = NULL;
    return PDAutorelease(binString);
}

void PDCMapCompile_Iterator(char *key, void *value, void *userInfo, PDBool *shouldStop)
{
    PDCMapRef cmap = userInfo;
    PDDictionaryRef dict = value;
    
    if (cmap != PDDictionaryGet(dict, "#cmap#")) {
        // that's not very good; we hope that there were multiple resources defined and that we simply encountered the wrong one
        return;
    }

    *shouldStop = true;

    cmap->name = PDRetain(PDDictionaryGet(dict, "CMapName"));
    cmap->systemInfo = PDRetain(PDDictionaryGet(dict, "CIDSystemInfo"));
    cmap->type = PDRetain(PDDictionaryGet(dict, "CMapType"));
}

void PDCMapCompile(PDCMapRef cmap, char *stream, PDSize len)
{
    pd_ps_env pse = pd_ps_create();
    pd_ps_set_cmap(pse, cmap);
    PDBool success = pd_ps_execute_postscript(pse, stream, len);
    if (success) {
        // there should be a single resource in the CMap category
        PDDictionaryRef res = pse->resources;
        if (res) {
            PDDictionaryRef cmaps = PDDictionaryGet(res, "CMap");
            if (cmaps) {
                PDDictionaryIterate(cmaps, PDCMapCompile_Iterator, cmap);
            } else {
                PDWarn("No CMaps dictionary in PostScript resources dictionary");
            }
        } else {
            PDWarn("No resources dictionary after PostScript execution");
        }
    } else {
        PDError("PostScript compilation failure");
    }
    pd_ps_destroy(pse);
}

void PDCMapAllocateCodespaceRanges(PDCMapRef cmap, PDSize count)
{
    cmap->csrCap += count;
    cmap->csrs = realloc(cmap->csrs, sizeof(struct PDCMapRange) * cmap->csrCap);
}

void PDCMapAllocateBFChars(PDCMapRef cmap, PDSize count)
{
    cmap->bfcCap += count;
    cmap->bfcs = realloc(cmap->bfcs, sizeof(struct PDCMapCharMapping) * cmap->bfcCap);
}

void PDCMapAllocateBFRanges(PDCMapRef cmap, PDSize count)
{
    cmap->bfrCap += count;
    cmap->bfrs = realloc(cmap->bfrs, sizeof(struct PDCMapRangeMapping) * cmap->bfrCap);
}

void PDCMapAppendCodespaceRange(PDCMapRef cmap, PDStringRef rangeFrom, PDStringRef rangeTo)
{
    PDCMapRange *range = &cmap->csrs[cmap->csrCount++];
    range->startRange = PDStringCreateFromStringWithType(rangeFrom, PDStringTypeBinary, false, false);
    range->endRange = PDStringCreateFromStringWithType(rangeTo, PDStringTypeBinary, false, false);
}

void PDCMapAppendBFChar(PDCMapRef cmap, PDStringRef charFrom, PDStringRef charTo)
{
    PDCMapCharMapping *charmap = &cmap->bfcs[cmap->bfcCount++];
    charmap->startChar = PDStringCreateFromStringWithType(charFrom, PDStringTypeBinary, false, false);
    charmap->endChar = PDStringCreateFromStringWithType(charTo, PDStringTypeBinary, false, false);
}

void PDCMapAppendBFRange(PDCMapRef cmap, PDStringRef rangeFrom, PDStringRef rangeTo, PDStringRef mapTo)
{
    PDCMapRangeMapping *rangemap = &cmap->bfrs[cmap->bfrCount++];
    rangemap->startRange = PDStringCreateFromStringWithType(rangeFrom, PDStringTypeBinary, false, false);
    rangemap->endRange = PDStringCreateFromStringWithType(rangeTo, PDStringTypeBinary, false, false);
    rangemap->mapStart = PDStringCreateFromStringWithType(mapTo, PDStringTypeBinary, false, false);
}
