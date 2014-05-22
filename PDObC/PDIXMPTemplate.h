//
// PDIXMPTemplate.h
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

#import <Foundation/Foundation.h>

typedef enum {
    PDIXMPLicenseUndefined = 0,                         ///< license undefined (not same as public domain)
    PDIXMPLicenseAttribution,                           ///< CC BY --- This license lets others distribute, remix, tweak, and build upon your work, even commercially, as long as they credit you for the original creation. This is the most accommodating of licenses offered. Recommended for maximum dissemination and use of licensed materials. 
    PDIXMPLicenseAttributionShareAlike,                 ///< CC BY-SA --- This license lets others remix, tweak, and build upon your work even for commercial purposes, as long as they credit you and license their new creations under the identical terms. This license is often compared to “copyleft” free and open source software licenses. All new works based on yours will carry the same license, so any derivatives will also allow commercial use. This is the license used by Wikipedia, and is recommended for materials that would benefit from incorporating content from Wikipedia and similarly licensed projects. 
    PDIXMPLicenseAttributionNoDerivs,                   ///< CC BY-ND --- This license allows for redistribution, commercial and non-commercial, as long as it is passed along unchanged and in whole, with credit to you. 
    PDIXMPLicenseAttributionNonCommercial,              ///< CC BY-NC --- This license lets others remix, tweak, and build upon your work non-commercially, and although their new works must also acknowledge you and be non-commercial, they don’t have to license their derivative works on the same terms.
    PDIXMPLicenseAttributionNonCommercialShareAlike,    ///< CC BY-NC-SA --- This license lets others remix, tweak, and build upon your work non-commercially, as long as they credit you and license their new creations under the identical terms.
    PDIXMPLicenseAttributionNonCommercialNoDerivs,      ///< CC BY-NC-ND --- This license is the most restrictive of our six main licenses, only allowing others to download your works and share them with others as long as they credit you, but they can’t change them in any way or use them commercially.
    PDIXMPLicenseCommercial,                            ///< Commercial license
} PDIXMPLicense;

@class PDIXMPArchive;

@interface PDIXMPTemplate : NSObject

+ (NSArray *)licenseNames;

///////////////
///! Creation
///////////////

/**
 Create a template for the given license.
 */
+ (id)templateForLicense:(PDIXMPLicense)license;

/**
 Obtain the default rights string for the given license, if any.
 */
+ (NSString *)defaultRightsForLicense:(PDIXMPLicense)license withAuthor:(NSString *)author;

/**
 Create a Creative Commons license template based on restrictions. 
 
 Note that allowsAdaptions and ifSharedAlike are in reality a 3-value switch Yes, No, and Yes, if ShareAlike. This is interpreted internally as 
    "allows adaptions" if and only if allowsAdaptions is YES and ifSharedAlike is NO, 
    "allows adaptions if share alike" if and only if allowsAdaptions is YES and ifSharedAlike is YES.
 That is, if allowsAdaptions is NO, ifSharedAlike is ignored and no adaptions are allowed.
 
 @param allowsAdaptions Whether users are allowed to adapt the licensed work and make derivatives of it.
 @param ifSharedAlike (only applies if allowsAdaptions is YES) If users who make derivatives must license their new creations under the identical terms.
 @param allowsCommercialUse Whether commercial use of the work is allowed, i.e. if people can make money off of it.
 */
+ (id)templateWhichAllowsAdaptions:(BOOL)allowsAdaptions ifSharedAlike:(BOOL)ifSharedAlike allowsCommercialUse:(BOOL)allowsCommercialUse;

/**
 XMP declaration with given author name, used in copyright sentence, and an optional dictionary of key/values where each key/value pair translates into
 <KEY>
    <rdf:Alt>
        <rdf:li xml:lang="x-default">VALUE</rdf:li>
    </rdf:Alt>
    -if VALUE is a string, or-
    <rdf:Bag>
        <rdf:li>VALUE[0]</rdf:li>
        ...
    </rdf:Bag>
    -if VALUE is a set, or-
    <rdf:Seq>
        <rdf:li>VALUE[0]</rdf:li>
        ...
    </rdf:Seq>
    -if VALUE is an array-
    </rdf:Bag>
 </KEY>
 if the key begins with "dc:"; otherwise the pair is stored as a regular value in which case non-strings are -description'ed.
 */
- (NSString *)declarationWithAuthorName:(NSString *)authorName extra:(NSDictionary *)extra;

/**
 Apply the template to the given PDIXMPArchive instance, updating or inserting rdf:Description entries as appropriate.
 
 @param archive The XMP archive into which license related content should be inserted.
 */
- (void)applyToArchive:(PDIXMPArchive *)archive withAuthorName:(NSString *)authorName extra:(NSDictionary *)extra;

/**
 Remove licensing information from the given archive.
 
 This method is called by -applyToArchive:withAuthorName: if the license is PDIXMPLicenseUndefined.

 @param archive The XMP archive from which all license related content should be removed.
 */
- (void)removeFromArchive:(PDIXMPArchive *)archive;

/**
 *  Relicense the target of the template to the given new license.
 *
 *  @param newLicense New license
 */
- (void)relicense:(PDIXMPLicense)newLicense;

/**
 License.
 */
@property (nonatomic, readonly) PDIXMPLicense license;

/**
 License name.
 */
@property (nonatomic, readonly, strong) NSString *licenseName;

/**
 License URL.
 */
@property (nonatomic, readonly, strong) NSString *licenseUrl;

/**
 Rights string, e.g. "Copyright © 2014, Company. All rights reserved."
 
 A PDF with a "rights" string containing the sentence "all rights reserved" has the license PDIXMPLicenseCommercial.
 */
@property (nonatomic, strong) NSString *rights;

///////////////
///! Evaluating
///////////////

/**
 Determine the license used in the given (pre-existing) XMP archive.
 
 @param XMPArchive The archive for an existing PDF whose XMP data should be used to determine the PDF's license.
 @return PDIXMPLicense enum result.
 */
+ (PDIXMPLicense)licenseForXMPArchive:(PDIXMPArchive *)XMPArchive;

/**
 Generate a template for the given archive.
 
 @param XMPArchive The archive for an existing PDF whose XMP data should be used to determine the PDF's license.
 @return PDIXMPTemplate which inherits the license used by the given XMP archive, or nil if the license could not be determined or is not supported.
 */
+ (id)templateForXMPArchive:(PDIXMPArchive *)XMPArchive;


@end
