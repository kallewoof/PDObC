//
// PDIXMPTemplate.c
//
// Copyright (c) 2013 Karl-Johan Alm (http://github.com/kallewoof)
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#import "Pajdeg.h"
#import "PDIXMPTemplate.h"
#import "PDIXMPArchive.h"
#import "PDIXMPElement.h"
#import "NSArray+PDIXMPArchive.h"
#import "NSObjects+PDIEntity.h"

@interface PDIXMPTemplate ()

@property (nonatomic, readwrite) PDIXMPLicense license;
@property (nonatomic, strong) NSString *licenseUrl;
@property (nonatomic, strong) NSString *licenseName;

@end

@implementation PDIXMPTemplate

static NSArray *licenseUrls = nil;
static NSArray *licenseNames = nil;
static NSArray *codedLicenses = nil;

/*
 OGL = Open Game License
 FDL = GNU Free Documentation License
 OPL = Open Publication License
 (if CC, add a CC+ field: particularly useful with NC; it's not commercial but go to this URL if you wanna buy commercial right, http://wiki.creativecommons.org/CCPlus )
 Public Domain = identified as
 CC0 = Owned by me, given as public domain
 */

static inline void PDIXMPTemplateSetup()
{
    licenseUrls = @[@"",
                    @"http://creativecommons.org/licenses/by/4.0",
                    @"http://creativecommons.org/licenses/by-sa/4.0",
                    @"http://creativecommons.org/licenses/by-nd/4.0",
                    @"http://creativecommons.org/licenses/by-nc/4.0",
                    @"http://creativecommons.org/licenses/by-nc-sa/4.0",
                    @"http://creativecommons.org/licenses/by-nc-nd/4.0",
                    @"",
                    ];
    
    licenseNames = @[@"Unspecified",
                     @"Attribution 4.0",
                     @"Attribution-ShareAlike 4.0",
                     @"Attribution-NoDerivs 4.0",
                     @"Attribution-NonCommercial 4.0",
                     @"Attribution-NonCommercial-ShareAlike 4.0",
                     @"Attribution-NonCommercial-NoDerivs 4.0",
                     @"Commercial",
                     ];
    
    /*
     adaptions  sharealike  commercial  code
     0          0           0           0
     1          0           0           1
     0          1           0           2       note: we consider adaptions=NO as overruling sharealike=YES
     1          1           0           3
     0          0           1           4
     1          0           1           5
     0          1           1           6       note: we consider adaptions=NO as overruling sharealike=YES
     1          1           1           7
     */
    codedLicenses = @[@(PDIXMPLicenseAttributionNonCommercialNoDerivs),
                      @(PDIXMPLicenseAttributionNonCommercial),
                      @(PDIXMPLicenseAttributionNonCommercialNoDerivs),
                      @(PDIXMPLicenseAttributionNonCommercialShareAlike),
                      @(PDIXMPLicenseAttributionNoDerivs),
                      @(PDIXMPLicenseAttribution),
                      @(PDIXMPLicenseAttributionNoDerivs),
                      @(PDIXMPLicenseAttributionShareAlike),
                      ];
    
#if 0
    PDIXMPLicenseNone = 0,                              ///< license undefined (not same as public domain)
    PDIXMPLicenseAttribution,                           ///< CC BY --- This license lets others distribute, remix, tweak, and build upon your work, even commercially, as long as they credit you for the original creation. This is the most accommodating of licenses offered. Recommended for maximum dissemination and use of licensed materials. 
    PDIXMPLicenseAttributionShareAlike,                 ///< CC BY-SA --- This license lets others remix, tweak, and build upon your work even for commercial purposes, as long as they credit you and license their new creations under the identical terms. This license is often compared to “copyleft” free and open source software licenses. All new works based on yours will carry the same license, so any derivatives will also allow commercial use. This is the license used by Wikipedia, and is recommended for materials that would benefit from incorporating content from Wikipedia and similarly licensed projects. 
    PDIXMPLicenseAttributionNoDerivs,                   ///< CC BY-ND --- This license allows for redistribution, commercial and non-commercial, as long as it is passed along unchanged and in whole, with credit to you. 
    PDIXMPLicenseAttributionNonCommercial,              ///< CC BY-NC --- This license lets others remix, tweak, and build upon your work non-commercially, and although their new works must also acknowledge you and be non-commercial, they don’t have to license their derivative works on the same terms.
    PDIXMPLicenseAttributionNonCommercialShareAlike,    ///< CC BY-NC-SA --- This license lets others remix, tweak, and build upon your work non-commercially, as long as they credit you and license their new creations under the identical terms.
    PDIXMPLicenseAttributionNonCommercialNoDerivs,      ///< CC BY-NC-ND --- This license is the most restrictive of our six main licenses, only allowing others to download your works and share them with others as long as they credit you, but they can’t change them in any way or use them commercially.
#endif
}

+ (NSArray *)licenseNames
{
    if (licenseUrls == nil) PDIXMPTemplateSetup();
    return licenseNames;
}

+ (id)templateForLicense:(PDIXMPLicense)license
{
    if (licenseUrls == nil) PDIXMPTemplateSetup();
    PDIXMPTemplate *template = [[PDIXMPTemplate alloc] init];
    template.license = license;
    template.licenseName = licenseNames[license];
    template.licenseUrl = licenseUrls[license];
    return template;
}

+ (id)templateWhichAllowsAdaptions:(BOOL)allowsAdaptions ifSharedAlike:(BOOL)ifSharedAlike allowsCommercialUse:(BOOL)allowsCommercialUse
{
    if (licenseUrls == nil) PDIXMPTemplateSetup();

    unsigned char code = (allowsAdaptions != 0) + ((ifSharedAlike != 0) << 1) + ((allowsCommercialUse != 0) << 2);
    /*
     adaptions  sharealike  commercial  code
     0          0           0           0
     1          0           0           1
     0          1           0           2       note: we consider adaptions=NO as overruling sharealike=YES
     1          1           0           3
     0          0           1           4
     1          0           1           5
     0          1           1           6       note: we consider adaptions=NO as overruling sharealike=YES
     1          1           1           7
     */
    return [self templateForLicense:[codedLicenses[code] intValue]];
}

+ (id)templateForXMPArchive:(PDIXMPArchive *)archive
{
    PDIXMPLicense license = [self licenseForXMPArchive:archive];
    if (license == PDIXMPLicenseUndefined) license = PDIXMPLicenseCommercial;
    
    PDIXMPTemplate *template = [self templateForLicense:license];
    
    [archive selectRoot];
    [archive selectElement:@"x:xmpmeta"];
    [archive selectElement:@"rdf:RDF"];
    NSString *rights = [[archive findElements:@"rdf:Description"] firstAvailableElementValueForPath:@[@"dc:rights", @"rdf:Alt", @"rdf:li"]];
    if (rights) template.rights = rights;
    
    return template;
}

+ (PDIXMPLicense)licenseForXMPArchive:(PDIXMPArchive *)archive
{
    if (licenseUrls == nil) PDIXMPTemplateSetup();
    
    [archive selectRoot];
    [archive selectElement:@"x:xmpmeta"];
    [archive selectElement:@"rdf:RDF"];
    
    if ([archive selectElement:@"rdf:Description" withAttributes:@{@"xmlns:cc": @"http://creativecommons.org/ns#"}]) {
        // we got a CC license expression in the form of a URL
        // the URL is in the format
        //  http://creativecommons.org/licenses/LICENSE_NAME/VERSION"
        NSString *url = [archive stringForAttribute:@"rdf:resource"];
        if (url) {
            // first we see if we have an exact match
            NSInteger index = [licenseUrls indexOfObject:url];
            if (index != NSNotFound) {
                // we do
                return (PDIXMPLicense) index;
            }
            
            /*if ([url hasPrefix:@"http://creativecommons.org/licenses/"]) {
                url = [url substringFromIndex:@"http://creativecommons.org/licenses/".length];
            } else {
                NSArray *comps = [url componentsSeparatedByString:@"licenses"];
                url = nil;
                if (comps.count == 2) {
                    url = [comps objectAtIndex:1];
                    if ([url hasPrefix:@"/"]) url = [url substringFromIndex:1];
                }
            }
            if (url) {
                // we now have a cropped string in the format
                //  LICENSE_NAME/VERSION
                NSMutableArray *comps = [[url componentsSeparatedByString:@"/"] mutableCopy];
                NSString *license, *version;

                version = nil;
                if (comps.count > 1) {
                    licenseUrls = [comps lastObject];
                    [comps removeLastObject];
                }
                license = [comps componentsJoinedByString:@"/"];
            }*/
        }
        [archive selectParent];
    } 
    
    if ([archive selectElement:@"rdf:Description"]) {
        // look for a dc:rights entry
        if ([archive selectElement:@"dc:rights"] && [archive selectElement:@"rdf:Alt"] && [archive selectElement:@"rdf:li"]) {
            //               <rdf:li xml:lang="x-default">Copyright © 2014, Corp. All rights reserved.</rdf:li>
            NSString *rights = [archive elementContent];
            if ([rights rangeOfString:@"all rights reserved" options:NSCaseInsensitiveSearch].location != NSNotFound) {
                return PDIXMPLicenseCommercial;
            }
        }
    }

    [archive selectRoot];
    [archive selectElement:@"x:xmpmeta"];
    [archive selectElement:@"rdf:RDF"];
    
    NSString *xmpString = [[NSString alloc] initWithData:[archive allocSubset] encoding:NSUTF8StringEncoding];
    NSRange ccRange = [xmpString rangeOfString:@"Creative Commons "];
    if (ccRange.location != NSNotFound) {
        NSString *string = [xmpString substringFromIndex:ccRange.location + ccRange.length];
        for (NSInteger i = 0; i < licenseNames.count; i++) {
            if ([string hasPrefix:licenseNames[i]]) {
                return (PDIXMPLicense) i;
            }
        }
    }
    
    return PDIXMPLicenseUndefined;
}

+ (NSString *)defaultRightsForLicense:(PDIXMPLicense)license withAuthor:(NSString *)author
{
    NSInteger year = [[[NSCalendar currentCalendar] components:NSCalendarUnitYear fromDate:[NSDate date]] year];
    return (license == PDIXMPLicenseCommercial
            ? [NSString stringWithFormat:@"Copyright %ld, %@. All rights reserved.", (long)year, author]
            : [NSString stringWithFormat:@"Copyright %ld, %@. Licensed to the public under Creative Commons %@", (long)year, author, licenseNames[license]]);
}

- (NSString *)declarationWithAuthorName:(NSString *)authorName extra:(NSDictionary *)extra
{
    if (_license == PDIXMPLicenseUndefined) return nil;
    
    NSMutableString *result = [[NSMutableString alloc] initWithCapacity:900];
    
    [result appendString:@"<?xpacket begin='' id=''?><x:xmpmeta xmlns:x='adobe:ns:meta/'>\
     <rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\
     \
     "];
    
    if (_license == PDIXMPLicenseCommercial && ! extra[@"dc:rights"]) {
        NSMutableDictionary *d = extra ? extra.mutableCopy : [NSMutableDictionary dictionary];
        d[@"dc:rights"] = [PDIXMPTemplate defaultRightsForLicense:_license withAuthor:authorName];
        extra = d;
    }
    
    if (extra) {
        [result appendString:@"<rdf:Description rdf:about=\"\"\n"];
        
        BOOL hasDC = NO; // purl.org/dc/elements/1.1/
        BOOL hasPDF = NO; // ns.adobe.com/pdf/1.3/
        BOOL hasXMP = NO; 
//        BOOL hasXMPMM = NO;
//        BOOL hasXMPRights = NO;
        BOOL hasPDFX = NO; // ns.adobe.com/pdfx/1.3/
        /*
            xmlns:dc="http://purl.org/dc/elements/1.1/"
            xmlns:pdf="http://ns.adobe.com/pdf/1.3/"
            xmlns:xmp="http://ns.adobe.com/xap/1.0/"
            xmlns:xmpMM="http://ns.adobe.com/xap/1.0/mm/"
            xmlns:xmpRights="http://ns.adobe.com/xap/1.0/rights/"
            xmlns:pdfx="http://ns.adobe.com/pdfx/1.3/">
         */
        for (NSString *k in extra.allKeys) {
            hasDC |= [k hasPrefix:@"dc:"];
            hasPDF |= [k hasPrefix:@"pdf:"];
            hasXMP |= [k hasPrefix:@"xmp:"];
            hasPDFX |= [k hasPrefix:@"pdfx:"];
        }
        if (hasDC)   [result appendString:@"xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"];
        if (hasPDF)  [result appendString:@"xmlns:pdf=\"http://ns.adobe.com/pdf/1.3/\"\n"];
        if (hasXMP)  [result appendString:@"xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\"\
                                    xmlns:xmpMM=\"http://ns.adobe.com/xap/1.0/mm/\"\
                                    xmlns:xmpRights=\"http://ns.adobe.com/xap/1.0/rights/\"\n"];
        if (hasPDFX) [result appendString:@"xmlns:pdfx=\"http://ns.adobe.com/pdfx/1.3/\""];

        [result appendString:@">\n"];
        
        for (NSString *key in extra.allKeys) {
            [result appendFormat:@"<%@>", key];
            id val = extra[key];
            if ([key hasPrefix:@"dc:"]) {
                if ([val isKindOfClass:[NSString class]]) {
                    [result appendFormat:@"<rdf:Alt><rdf:li xml:lang=\"x-default\">%@</rdf:li></rdf:Alt>", [val stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
                }
                else if ([val isKindOfClass:[NSArray class]]) {
                    [result appendString:@"<rdf:Seq>\n"];
                    for (NSString *v in val) {
                        [result appendFormat:@"<rdf:li xml:lang=\"x-default\">%@</rdf:li>\n", [v stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
                    }
                    [result appendString:@"</rdf:Seq>\n"];
                }
                else if ([val isKindOfClass:[NSSet class]]) {
                    [result appendString:@"<rdf:Bag>\n"];
                    for (NSString *v in val) {
                        [result appendFormat:@"<rdf:li xml:lang=\"x-default\">%@</rdf:li>\n", [v stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
                    }
                    [result appendString:@"</rdf:Bag>\n"];
                }
            } 
            else {
                [result appendFormat:@"%@", val];
            }
            [result appendFormat:@"</%@>\n", key];
        }
        
        [result appendString:@"</rdf:Description>\n"];
    }

    if (_license != PDIXMPLicenseCommercial) {
        [result appendString:@"<rdf:Description rdf:about=''\
                         xmlns:xapRights='http://ns.adobe.com/xap/1.0/rights/'>\
            <xapRights:Marked>True</xapRights:Marked> \
        </rdf:Description>\
        \
        <rdf:Description rdf:about=''\
                         xmlns:dc='http://purl.org/dc/elements/1.1/'>\
            <dc:rights>\
                <rdf:Alt>\
                    <rdf:li xml:lang='x-default' >"];
        [result appendString:[PDIXMPTemplate defaultRightsForLicense:_license withAuthor:authorName]];
        [result appendString:@".</rdf:li>\
                </rdf:Alt>\
            </dc:rights>\
        </rdf:Description>\
        \
        <rdf:Description rdf:about=''\
                         xmlns:cc='http://creativecommons.org/ns#'>\
            <cc:license rdf:resource='"];
        [result appendString:_licenseUrl];
        [result appendString:@"'/>\
        </rdf:Description>\
        \
         "];
    }

    [result appendString:@"</rdf:RDF>\
     </x:xmpmeta><?xpacket end='r'?>"];
    
    return result;
}

- (void)removeFromArchive:(PDIXMPArchive *)archive
{
    [archive selectRoot];
    if ([archive selectElement:@"x:xmpmeta"]) {
        if ([archive selectElement:@"rdf:RDF"]) {
    
            id rdfRoot = [archive cursorReference];
            assert(rdfRoot);
    
            while ([archive selectElement:@"rdf:Description"]) {
                [archive deleteElement]; // delete includes -selectParent
            }
        }
    }
}

- (void)applyToArchive:(PDIXMPArchive *)archive withAuthorName:(NSString *)authorName extra:(NSDictionary *)extra
{
    if (_license == PDIXMPLicenseUndefined) {
        [self removeFromArchive:archive];
        return;
    }
    
    [archive selectRoot];
    BOOL okay = [archive selectElement:@"x:xmpmeta"];
    NSAssert(okay, @"No x:xmpmeta key in PDIXMPArchive!");
    okay = [archive selectElement:@"rdf:RDF"];
    NSAssert(okay, @"No rdf:RDF key in PDIXMPArchive! They should be generated automagically!");
    
    id rdfRoot = [archive cursorReference];
    assert(rdfRoot);
    
    // to avoid duplicates due to differing attribute sets, we remove all pdf:Description entries from the XMP here
    [[archive findElements:@"rdf:Description"] makeObjectsPerformSelector:@selector(removeFromParent)];
    
    if (_license != PDIXMPLicenseCommercial) {
        [archive createElement:@"rdf:Description" withAttributes:@{@"rdf:about":       @"", 
                                                                   @"xmlns:xapRights": @"http://ns.adobe.com/xap/1.0/rights/"}]; {
            [archive createElement:@"xapRights:Marked"]; {
                [archive setElementContent:@"True"];
            } [archive selectParent];
        } [archive selectParent];
    }
    
    assert([archive cursorReference] == rdfRoot);
    
    NSMutableDictionary *d = extra ? extra.mutableCopy : [NSMutableDictionary dictionary];
    //2014-06-04T08:28:14-04:00
    NSString *datetimeString = [[NSDate date] datetimeString];
    d[@"xmp:MetadataDate"] = datetimeString;

    if (! extra[@"xmp:ModifyDate"])     d[@"xmp:ModifyDate"] = datetimeString;
    if (! extra[@"dc:rights"])          d[@"dc:rights"] = _rights ? _rights : [PDIXMPTemplate defaultRightsForLicense:_license withAuthor:authorName];
    if (! extra[@"xmp:CreatorTool"])    d[@"xmp:CreatorTool"] = [NSString stringWithFormat:@"Pajdeg Ob-C (Pajdeg v. " PAJDEG_VERSION ")"];

    extra = d;

    NSMutableDictionary *nslist = [[NSMutableDictionary alloc] init];
    nslist[@"rdf:about"] = @"";
    
    BOOL hasDC = NO; // purl.org/dc/elements/1.1/
    BOOL hasPDF = NO; // ns.adobe.com/pdf/1.3/
    BOOL hasXMP = NO; 
    //        BOOL hasXMPMM = NO;
    //        BOOL hasXMPRights = NO;
    BOOL hasPDFX = NO; // ns.adobe.com/pdfx/1.3/
    /*
     xmlns:dc="http://purl.org/dc/elements/1.1/"
     xmlns:pdf="http://ns.adobe.com/pdf/1.3/"
     xmlns:xapRights="http://ns.adobe.com/xap/1.0/rights/"
     xmlns:xmp="http://ns.adobe.com/xap/1.0/"
     xmlns:xmpMM="http://ns.adobe.com/xap/1.0/mm/"
     xmlns:xmpRights="http://ns.adobe.com/xap/1.0/rights/"
     xmlns:pdfx="http://ns.adobe.com/pdfx/1.3/">
     */
    for (NSString *k in extra.allKeys) {
        hasDC |= [k hasPrefix:@"dc:"];
        hasPDF |= [k hasPrefix:@"pdf:"];
        hasXMP |= [k hasPrefix:@"xmp:"];
        hasPDFX |= [k hasPrefix:@"pdfx:"];
    }
    if (hasDC)   nslist[@"xmlns:dc"] = @"http://purl.org/dc/elements/1.1/";
    if (hasPDF)  nslist[@"xmlns:pdf"] = @"http://ns.adobe.com/pdf/1.3/";
    if (hasXMP)  { 
        nslist[@"xmlns:xmp"] = @"http://ns.adobe.com/xap/1.0/";
        nslist[@"xmlns:xmpMM"] = @"http://ns.adobe.com/xap/1.0/mm/";
        nslist[@"xmlns:xmpRights"] = @"http://ns.adobe.com/xap/1.0/rights/";
    }
    if (hasPDFX) nslist[@"xmlns:pdfx"] = @"http://ns.adobe.com/pdfx/1.3/";
    
    [archive createElement:@"rdf:Description" withAttributes:nslist]; {

        for (NSString *key in extra.allKeys) {
            // we don't want to get duplicate entries so we remove existing values with the same key
            if ([archive selectElement:key]) 
                [archive deleteElement];
            
            [archive createElement:key]; {
                id val = extra[key];
                    // dc:format is special; it doesn't have the rdf:Alt/Seq/etc stuff so we ignore that one here
                if ([key hasPrefix:@"dc:"] && ! [key isEqualToString:@"dc:format"]) {
                    BOOL includeLang = YES;
                    // specialized keys exist and require specific handling
                    if ([key isEqualToString:@"dc:creator"]) {
                        // creator requires a Seq and does not have a lang
                        [archive createElement:@"rdf:Seq"];
                        includeLang = NO;
                        if ([val isKindOfClass:[NSString class]]) val = @[val];
                    }
                    else if ([key isEqualToString:@"dc:subject"]) {
                        // subject is a bag and does not include lang
                        [archive createElement:@"rdf:Bag"];
                        includeLang = NO;
                        if ([val isKindOfClass:[NSString class]]) val = @[val];
                    }
                    else if ([val isKindOfClass:[NSString class]]) {
                        [archive createElement:@"rdf:Alt"]; 
                        val = @[val];
                    }
                    else if ([val isKindOfClass:[NSArray class]]) {
                        [archive createElement:@"rdf:Seq"];
                    }
                    else if ([val isKindOfClass:[NSSet class]]) {
                        [archive createElement:@"rdf:Bag"];
                    }
                    
                    NSDictionary *liAttr = (includeLang ? @{@"xml:lang": @"x-default"} : nil);
                    for (NSString *v in val) {
                        [archive appendElement:@"rdf:li" withAttributes:liAttr]; {
                            [archive setElementContent:[v stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
                        } [archive selectParent];
                    }
                    
                    [archive selectParent];
                } else {
                    [archive setElementContent:[NSString stringWithFormat:@"%@", val]];
                }
            } [archive selectParent];
        }
        
//        [archive createElement:@"dc:rights"]; {
//            [archive createElement:@"rdf:Alt"]; {
//                [archive createElement:@"rdf:li" withAttributes:@{@"xml:lang":     @"x-default"}]; {
//                    [archive setElementContent:[NSString stringWithFormat:@"Copyright %ld, %@. Licensed to the public under Creative Commons %@.", (long)year, authorName, _licenseName]];
//                } [archive selectParent];
//            } [archive selectParent];
//        } [archive selectParent];

    } [archive selectParent];

    assert([archive cursorReference] == rdfRoot);
    
    if (_license != PDIXMPLicenseCommercial) {
        [archive createElement:@"rdf:Description" withAttributes:@{@"rdf:about":       @"",
                                                                   @"xmlns:cc":        @"http://creativecommons.org/ns#"}]; {
            [archive createElement:@"cc:license" withAttributes:@{@"rdf:resource":     _licenseUrl}];
            [archive selectParent];
        } [archive selectParent];
    }
    
}

- (void)relicense:(PDIXMPLicense)newLicense
{
    _license = newLicense;
    _licenseName = licenseNames[newLicense];
    _licenseUrl = licenseUrls[newLicense];
}

@end
