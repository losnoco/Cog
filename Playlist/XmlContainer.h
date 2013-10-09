//
//  XmlContainer.h
//  Xml
//
//  Created by Christopher Snowhill on 10/9/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface XmlContainer : NSObject {

}

+ (NSURL *)urlForPath:(NSString *)path relativeTo:(NSString *)baseFilename;

+ (NSArray *)entriesForContainerURL:(NSURL *)url;

@end
