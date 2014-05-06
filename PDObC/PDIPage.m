//
// PDIPage.c
//
// Copyright (c) 2014 Karl-Johan Alm (http://github.com/kallewoof)
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

#import "PDPage.h"
#import "PDIPage.h"
#import "PDIObject.h"
#import "PDDefines.h"
#import "pd_internal.h"

@interface PDIPage () {
    PDIObject *_contents;
}

//@property (nonatomic, assign) PDInstance *instance;
//@property (nonatomic, retain) PDIObject *object;

@end

@implementation PDIPage

- (void)dealloc
{
    PDRelease(_pageRef);
}

- (id)initWithPage:(PDPageRef)page
{
    self = [super init];
    _pageRef = PDRetain(page);
    _pageObject = [[PDIObject alloc] initWithObject:_pageRef->ob];

    PDRect r = PDPageGetMediaBox(_pageRef);
    _mediaBox = (CGRect) PDRectToOSRect(r);
    
    return self;
}

#pragma mark - Extended properties

- (PDIObject *)contents
{
    if (_contents) return _contents;
    _contents = [[PDIObject alloc] initWithObject:PDPageGetContentsObject(_pageRef)];
    return _contents;
}

@end
