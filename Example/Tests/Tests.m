//
//  PajdegTests.m
//  PajdegTests
//
//  Created by Kalle Alm on 07/25/2014.
//  Copyright (c) 2014 Kalle Alm. All rights reserved.
//

#import "PDISession.h"
#import "PDIAnnotation.h"
#import "PDIAnnotGroup.h"
#import "PDIObject.h"
#import "PDIReference.h"
#import "PDPipe.h"
#import "PDParser.h"
#import "PDCatalog.h"
#import "NSArray+Sampling.h"

// not sure what to do here; there are a ton of PDFs, some private, some copyrighted/purchased, that the library is tested against; can't likely require travis to download a bunch of PDFs online either..
#define PAJDEG_PDFS @"/Users/user/Workspace/pajdeg-sample-pdfs/"
#define PAJDEG_INFS @"/Users/user/Workspace/pajdeg-inf-pdfs/"

SpecBegin(InitialSpecs)

NSFileManager *_fm = [NSFileManager defaultManager];
NSFileManager *fm = _fm;
__block PDISession *_session;

void (^configInOut)(NSString *file_in, NSString *file_out) = ^(NSString *file_in, NSString *file_out) {
    assert(file_in);
    assert(file_out);
    
    [_fm createDirectoryAtPath:NSTemporaryDirectory() withIntermediateDirectories:YES attributes:nil error:nil];
    [_fm removeItemAtPath:file_out error:NULL];
    
    _session = [[PDISession alloc] initWithSourcePDFPath:file_in destinationPDFPath:file_out];
};

//sharedExamplesFor(@"a regular PDF", ^(NSDictionary *data) {
//    NSString *pdf = data[@"pdf"];
//    NSString *path = data[@"path"];
//    NSString *infPath = data[@"infPath"];
//    
//    NSString *out1 = [NSTemporaryDirectory() stringByAppendingString:@"/out1.pdf"];
//    NSString *out2 = [NSTemporaryDirectory() stringByAppendingString:@"/out2.pdf"];
//
////    for (NSString *pdf in set) {
//        //if (! [pdf hasPrefix:@"Tactics of Persuasion"]) continue;
//        if ([[[pdf pathExtension] lowercaseString] isEqualToString:@"pdf"]) {
//            if ([[pdf lowercaseString] hasSuffix:@".infinite.pdf"]) {
//                [_fm removeItemAtPath:[infPath stringByAppendingString:pdf] error:NULL];
//                [_fm moveItemAtPath:[path stringByAppendingString:pdf] toPath:[infPath stringByAppendingString:pdf] error:NULL];
//            } else {
//                it(@"should exist", ^{
//                    expect([_fm fileExistsAtPath:[path stringByAppendingString:pdf]]).to.beTruthy();
//                });
//                
//                it(@"should process original successfully", ^{
//                    configInOut([path stringByAppendingString:pdf], out1);
//                    [_session execute];
//                    expect([_fm fileExistsAtPath:out1]).to.beTruthy();
//                });
//                
//                it(@"should process generated PDF successfully", ^{
//                    configInOut(out1, out2);
//                    [_session execute];
//                    expect([_fm fileExistsAtPath:out2]).to.beTruthy();
//                    _session = nil;
//                });
//            }
//        }
////    }
//});

sharedExamplesFor(@"a PDF with annotations updated", ^(NSDictionary *data) {
    NSString *pdf = data[@"pdf"];
    NSString *path = data[@"path"];
    NSString *infPath = data[@"infPath"];
    
    NSString *out1 = [NSTemporaryDirectory() stringByAppendingString:@"/out1.pdf"];
    NSString *out2 = [NSTemporaryDirectory() stringByAppendingString:@"/out2.pdf"];
    
    if ([[[pdf pathExtension] lowercaseString] isEqualToString:@"pdf"]) {
        if ([[pdf lowercaseString] hasSuffix:@".infinite.pdf"]) {
            [_fm removeItemAtPath:[infPath stringByAppendingString:pdf] error:NULL];
            [_fm moveItemAtPath:[path stringByAppendingString:pdf] toPath:[infPath stringByAppendingString:pdf] error:NULL];
        } else {
            it(@"should exist", ^{
                expect([fm fileExistsAtPath:[path stringByAppendingString:pdf]]).to.beTruthy();
            });

            it(@"should replace annotations correctly", ^{
                configInOut([path stringByAppendingString:pdf], out1);
                NSInteger pages = [_session numberOfPages];
                for (NSInteger i = 0; i < pages; i++) {
                    PDIObject *pageOb = [_session fetchReadonlyObjectWithID:[_session objectIDForPageNumber:i+1]];
                    [pageOb enableMutationViaMimicSchedulingWithSession:_session];
                    expect(pageOb).toNot.beNil();
                    PDIObject *annotsOb = [pageOb objectForKey:@"Annots"];
                    if (annotsOb) {
                        if ([annotsOb isKindOfClass:[PDIReference class]])
                            annotsOb = [_session fetchReadonlyObjectWithID:annotsOb.objectID];
                        expect(annotsOb).toNot.beNil();
                        PDIAnnotGroup *annotsGrp = [[PDIAnnotGroup alloc] initWithObject:annotsOb inSession:_session];
                        expect(annotsGrp).toNot.beNil();
                        for (PDIAnnotation *annotation in annotsGrp.annotations) {
                            expect([annotation isKindOfClass:[PDIAnnotation class]]).to.beTruthy();
                            if (annotation.uriString != nil) {
                                annotation.uriString = @"http://www.google.com";
                            }
                        }
                    }
                }
            });
            
            it(@"should execute annotation tasks correctly", ^{
                [_session execute];
                expect([fm fileExistsAtPath:out1]).to.beTruthy();
            });
            
            it(@"should process resulting PDF without issues", ^{
                configInOut(out1, out2);
                [_session execute];
                expect([fm fileExistsAtPath:out2]).to.beTruthy();
                _session = nil;
            });
        }
    }
});

sharedExamplesFor(@"a PDF with annotations created", ^(NSDictionary *data) {
    NSString *pdf = data[@"pdf"];
    NSString *path = data[@"path"];
    NSString *infPath = data[@"infPath"];
    
    NSString *out1 = [NSTemporaryDirectory() stringByAppendingString:@"/out1.pdf"];
    NSString *out2 = [NSTemporaryDirectory() stringByAppendingString:@"/out2.pdf"];
    
    if ([[[pdf pathExtension] lowercaseString] isEqualToString:@"pdf"]) {
        if ([[pdf lowercaseString] hasSuffix:@".infinite.pdf"]) {
            [_fm removeItemAtPath:[infPath stringByAppendingString:pdf] error:NULL];
            [_fm moveItemAtPath:[path stringByAppendingString:pdf] toPath:[infPath stringByAppendingString:pdf] error:NULL];
        } else {
            it(@"should exist", ^{
                expect([fm fileExistsAtPath:[path stringByAppendingString:pdf]]).to.beTruthy();
            });
            
            it(@"should create annotations correctly", ^{
                configInOut([path stringByAppendingString:pdf], out1);
                NSInteger pages = [_session numberOfPages];
                for (NSInteger i = 0; i < pages; i++) {
                    
                    PDIObject *annotsOb;
                    PDIObject *pageOb = [_session fetchReadonlyObjectWithID:[_session objectIDForPageNumber:i+1]];
                    [pageOb enableMutationViaMimicSchedulingWithSession:_session];
                    expect(pageOb).toNot.beNil();
                    annotsOb = [pageOb resolvedValueForKey:@"Annots"];
                    if (! annotsOb) {
                        [pageOb enableMutationViaMimicSchedulingWithSession:_session];
                        annotsOb = [_session appendObject];
                        [pageOb setValue:annotsOb forKey:@"Annots"];
                    }
                    
                    if ([annotsOb isKindOfClass:[NSArray class]]) {
                        // we need a proper object because we want to modify it
                        [pageOb enableMutationViaMimicSchedulingWithSession:_session];
                        PDIObject *realOb = [_session appendObject];
                        [pageOb setValue:realOb forKey:@"Annots"];
                        for (id v in (id) annotsOb) {
                            [realOb appendValue:v];
                        }
                        annotsOb = realOb;
                    }
                    
                    expect(annotsOb).toNot.beNil();
                    PDIAnnotGroup *annotsGrp = [[PDIAnnotGroup alloc] initWithObject:annotsOb inSession:_session];
                    expect(annotsGrp).toNot.beNil();
                    PDIAnnotation *annot = [annotsGrp appendAnnotation];
                    annot.rect = (CGRect){100,100,100,100};
                    annot.uriString = @"http://www.google.com";
                    
                }
            });
            
            it(@"should execute annotation tasks correctly", ^{
                [_session execute];
                expect([fm fileExistsAtPath:out1]).to.beTruthy();
            });
            
            it(@"should process resulting PDF without issues", ^{
                configInOut(out1, out2);
                [_session execute];
                expect([fm fileExistsAtPath:out2]).to.beTruthy();
                _session = nil;
            });
        }
    }
});


describe(@"PDF behaviors", ^{
    NSString *path = PAJDEG_PDFS;
    NSString *infPath = PAJDEG_INFS;
    NSArray *pdfs = [[fm contentsOfDirectoryAtPath:path error:NULL] sample:3];
    for (NSString *pdf in pdfs) {
        NSDictionary *d = @{@"pdf": pdf,
                            @"path": path,
                            @"infPath": infPath};
        itBehavesLike(@"a PDF with annotations updated", d);
        itBehavesLike(@"a PDF with annotations created", d);
    }
});

SpecEnd
