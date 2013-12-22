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

#import "PDIXMPTemplate.h"
#import "PDIXMPArchive.h"

@interface PDIXMPTemplate ()

@property (nonatomic, strong) NSString *licenseUrl;
@property (nonatomic, strong) NSString *licenseName;

@end

@implementation PDIXMPTemplate

static NSArray *licenseUrls = nil;
static NSArray *licenseNames = nil;
static NSArray *codedLicenses = nil;

static inline void PDIXMPTemplateSetup()
{
    licenseUrls = @[@"",
                    @"http://creativecommons.org/licenses/by/4.0",
                    @"http://creativecommons.org/licenses/by-sa/4.0",
                    @"http://creativecommons.org/licenses/by-nd/4.0",
                    @"http://creativecommons.org/licenses/by-nc/4.0",
                    @"http://creativecommons.org/licenses/by-nc-sa/4.0",
                    @"http://creativecommons.org/licenses/by-nc-nd/4.0",
                    ];
    
    licenseNames = @[@"Unspecified",
                     @"Attribution 4.0",
                     @"Attribution-ShareAlike 4.0",
                     @"Attribution-NoDerivs 4.0",
                     @"Attribution-NonCommercial 4.0",
                     @"Attribution-NonCommercial-ShareAlike 4.0",
                     @"Attribution-NonCommercial-NoDerivs 4.0",
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

+ (id)templateForLicense:(PDIXMPLicense)license
{
    if (licenseUrls == nil) PDIXMPTemplateSetup();
    PDIXMPTemplate *template = [[PDIXMPTemplate alloc] init];
    template.licenseName = [licenseNames objectAtIndex:license];
    template.licenseUrl = [licenseNames objectAtIndex:license];
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
    return [self templateForLicense:[[codedLicenses objectAtIndex:code] intValue]];
}

- (NSString *)declarationWithAuthorName:(NSString *)authorName
{
    NSInteger year = [[[NSCalendar currentCalendar] components:NSCalendarUnitYear fromDate:[NSDate date]] year];
    NSMutableString *result = [[NSMutableString alloc] initWithCapacity:900];
    
    [result appendString:@"<?xpacket begin='' id=''?><x:xmpmeta xmlns:x='adobe:ns:meta/'>\
    <rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\
\
     <rdf:Description rdf:about=''\
      xmlns:xapRights='http://ns.adobe.com/xap/1.0/rights/'>\
      <xapRights:Marked>True</xapRights:Marked> </rdf:Description>\
\
     <rdf:Description rdf:about=''\
      xmlns:dc='http://purl.org/dc/elements/1.1/'>\
      <dc:rights>\
       <rdf:Alt>\
        <rdf:li xml:lang='x-default' >Copyright "];
    [result appendFormat:@"%ld, %@. Licensed to the public under Creative Commons %@", (long)year, authorName, _licenseName];
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
    </rdf:RDF>\
     </x:xmpmeta><?xpacket end='r'?>"];
    
    return result;
}

- (void)applyToArchive:(PDIXMPArchive *)archive withAuthorName:(NSString *)authorName
{
    NSInteger year = [[[NSCalendar currentCalendar] components:NSCalendarUnitYear fromDate:[NSDate date]] year];

    [archive selectRoot];
    NSAssert([archive selectElement:@"x:xmpmeta"], @"No x:xmpmeta key in PDIXMPArchive!");
    NSAssert([archive selectElement:@"rdf:RDF"], @"No rdf:RDF key in PDIXMPArchive! They should be generated automagically!");
    
    id rdfRoot = [archive cursorReference];
    assert(rdfRoot);
    
    [archive createElement:@"rdf:Description" withAttributes:@{@"rdf:about":       @"", 
                                                               @"xmlns:xapRights": @"http://ns.adobe.com/xap/1.0/rights/"}]; {
        [archive createElement:@"xapRights:Marked"]; {
            [archive setElementContent:@"True"];
        } [archive selectParent];
    } [archive selectParent];
    
    assert([archive cursorReference] == rdfRoot);
    
    [archive createElement:@"rdf:Description" withAttributes:@{@"rdf:about":       @"",
                                                               @"xmlns:dc":        @"http://purl.org/dc/elements/1.1/"}]; {
        [archive createElement:@"dc:rights"]; {
            [archive createElement:@"rdf:Alt"]; {
                [archive createElement:@"rdf:li" withAttributes:@{@"xml:lang":     @"x-default"}]; {
                    [archive setElementContent:[NSString stringWithFormat:@"Copyright %ld, %@. Licensed to the public under Creative Commons %@.", (long)year, authorName, _licenseName]];
                } [archive selectParent];
            } [archive selectParent];
        } [archive selectParent];
    } [archive selectParent];

    assert([archive cursorReference] == rdfRoot);
    
    [archive createElement:@"rdf:Description" withAttributes:@{@"rdf:about":       @"",
                                                               @"xmlns:cc":        @"http://creativecommons.org/ns#"}]; {
        [archive createElement:@"cc:license" withAttributes:@{@"rdf:resource":     _licenseUrl}];
        [archive selectParent];
    } [archive selectParent];
    
}

@end
